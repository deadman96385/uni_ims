/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 *
 */

#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_dbg.h"

/* An FD bitmap used for selecting on ISI read queues */
static OSAL_MsgQGrpId   _ISIQ_group;

static OSAL_MsgQId      _ISIQ_app;

static OSAL_MsgQId      _ISIQ_proto;

/* Array of IPC used to communicate with underlying protocol applications */
static ISIQ_Ipc         _ISIQ_protocolArray[ISID_MAX_NUM_PROTOCOLS];

/*
 * ======== ISIQ_init() ========
 * This function initializes the global objects this file uses
 * to manage interprocess queues
 *
 * Returns:
 *  ISI_RETURN_OK All resources were successfully created/initialized.
 *  ISI_RETURN_FAILED Failed to create resources.  Check that valid
 *  FiFo names for the queues are being used.
 */
ISI_Return ISIQ_init(void)
{
    vint      x;

    /* define the ports to use for the service queues */
    for (x = 0 ; x < ISID_MAX_NUM_PROTOCOLS ; x++) {
        _ISIQ_protocolArray[x].protocol = 0;
        _ISIQ_protocolArray[x].isValid = OSAL_FALSE;
        _ISIQ_protocolArray[x].mediaIpc[0] = 0;
        _ISIQ_protocolArray[x].protocolIpc[0] = 0;
    }

    /* Create a Q group for all ISI traffic */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&_ISIQ_group)) {
        OSAL_logMsg("%s:%d OSAL_msgQGrpCreate() failed\n",
                __FUNCTION__, __LINE__);
        return (ISI_RETURN_FAILED);
    }

    /* Init the queue used for commands from the application */
    if (0 == (_ISIQ_app = OSAL_msgQCreate(ISIP_APPLICATION_PIPE,
            OSAL_MODULE_ISI, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISI_EventMessage,
            ISIQ_MAX_DEPTH, ISIQ_APP_MAX_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d OSAL_msgQCreate() failed\n",
                __FUNCTION__, __LINE__);
        return (ISI_RETURN_FAILED);
    }

    /* Init the queue used for events from protocol */
    if (0 == (_ISIQ_proto = OSAL_msgQCreate(ISIP_PROTOCOL_PIPE,
            OSAL_MODULE_REV_ONLY, OSAL_MODULE_ISI,
            OSAL_DATA_STRUCT_ISIP_Message,
            ISIQ_MAX_DEPTH, ISIQ_PROTO_MAX_MSG_SZ, 0))) {
        OSAL_logMsg("%s:%d OSAL_msgQCreate() failed\n",
                __FUNCTION__, __LINE__);
        OSAL_msgQDelete(_ISIQ_app);
        return (ISI_RETURN_FAILED);
    }

    /* Add the application queue to the group */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_ISIQ_group, _ISIQ_app)) {
        OSAL_logMsg("%s:%d OSAL_msgQGrpAddQ() failed\n",
                __FUNCTION__, __LINE__);
        OSAL_msgQDelete(_ISIQ_app);
        OSAL_msgQDelete(_ISIQ_proto);
        return (ISI_RETURN_FAILED);
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&_ISIQ_group, _ISIQ_proto)) {
        OSAL_logMsg("%s:%d OSAL_msgQGrpAddQ() failed\n",
                __FUNCTION__, __LINE__);
        OSAL_msgQDelete(_ISIQ_app);
        OSAL_msgQDelete(_ISIQ_proto);
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISIQ_destroyApp() ========
 * This function will destroy the queue used to manage interprocess
 * communication between ISI, and the application.
 *
 * Returns:
 *  nothing
 */
void ISIQ_destroyApp(void)
{
    OSAL_msgQDelete(_ISIQ_app);
    return;
}

/*
 * ======== ISIQ_destroyProto() ========
 * This function will destroy the queue used to manage interprocess
 * communication between ISI, and the protocols.
 *
 * Returns:
 *  nothing
 */
void ISIQ_destroyProto(void)
{
    OSAL_msgQDelete(_ISIQ_proto);
    return;
}

/*
 * ======== ISIQ_destroyGroupQ() ========
 * This function will destroy ISI group queue.
 *
 * Returns:
 *  nothing
 */
void ISIQ_destroyGroupQ(void)
{
    OSAL_msgQGrpDelete(&_ISIQ_group);
    return;
}

/*
 * ======== ISIQ_wakeAppQueue() ========
 * This function will write an event to the 'application' 
 * queue to wake it up.
 *
 */
void ISIQ_wakeAppQueue(void)
{
    ISI_EventMessage msg;
    msg.idType = ISI_ID_TYPE_INVALID;
    ISIQ_writeAppQueue((char*)&msg, sizeof(ISI_EventMessage));
    return;
}

/*
 * ======== ISIQ_writeAppQueue() ========
 * This function will write an event to the 'application' queue
 *
 * Returns:
 *  ISI_RETURN_OK : Queue was successfully written.
 *
 *  ISI_RETURN_FAILED : Queue was NOT successfully written to.
 */
ISI_Return ISIQ_writeAppQueue(
    char   *data_ptr,
    vint    length)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(_ISIQ_app, (char*)data_ptr, length,
            OSAL_WAIT_FOREVER, NULL)) {
        return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISIQ_writeProtocolQueue() ========
 * This function will write an event to the 'protocol' queue.
 * Protocol queues are the queues that the underlying
 * protocol uses to read commands from the ISI.
 *
 * Returns:
 *  ISI_RETURN_OK : Queue was successfully written.
 *
 *  ISI_RETURN_FAILED : Queue was NOT successfully written to.
 */
ISI_Return ISIQ_writeProtocolQueue(
    ISIP_Message  *msg_ptr)
{
    ISIQ_Ipc        *ipc_ptr;
    OSAL_MsgQId      ipcId;

    /* Find the queue(s) used by this protocol */
    if (NULL == (ipc_ptr = ISIQ_getProtocol(msg_ptr->protocol))) {
        /* Could not find this protocol?? */
        return (ISI_RETURN_FAILED);
    }

    /*
     * Determine which IPC (Application) the ISI Message should be sent to
     * based on it's type and then send (write).
     */
    if (ISIP_CODE_MEDIA == msg_ptr->code) {
        switch (msg_ptr->msg.media.reason) {
            case ISIP_MEDIA_REASON_RINGSTART:
            case ISIP_MEDIA_REASON_RINGSTOP:
            case ISIP_MEDIA_REASON_TONESTART:
            case ISIP_MEDIA_REASON_TONESTOP:
            case ISIP_MEDIA_REASON_TONESTART_CMD:
            case ISIP_MEDIA_REASON_TONESTOP_CMD:
                /* Send the ISI Message to the IPC for audio control */
                ipcId = ipc_ptr->mediaQId;
                break;
            case ISIP_MEDIA_REASON_STREAMSTART:
            case ISIP_MEDIA_REASON_STREAMSTOP:
            case ISIP_MEDIA_REASON_STREAMMODIFY:
            case ISIP_MEDIA_REASON_CONFSTART:
            case ISIP_MEDIA_REASON_CONFSTOP:
            case ISIP_MEDIA_REASON_PKT_SEND:
            case ISIP_MEDIA_REASON_SPEAKER:
            case ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_DISABLE:
            case ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_ENABLE:
                /* Send the ISI Message to the IPC used for "stream control". */
                ipcId = ipc_ptr->streamQId;
                break;
            case ISIP_MEDIA_REASON_PKT_RECV:
            default:
                ipcId = ipc_ptr->protocolQId;
                break;
        }
    }
    else if (ISIP_CODE_TEL_EVENT == msg_ptr->code) {
        switch (msg_ptr->msg.event.evt) {
            case ISI_TEL_EVENT_DTMF:
            case ISI_TEL_EVENT_DTMF_STRING:
                ipcId = ipc_ptr->streamQId;
                break;
            case ISI_TEL_EVENT_DTMF_OOB:
            case ISI_TEL_EVENT_VOICEMAIL:
            case ISI_TEL_EVENT_CALL_FORWARD:
            case ISI_TEL_EVENT_SEND_USSD:
            case ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE:
            default:
                ipcId = ipc_ptr->protocolQId;
                break;
        }
    }
    else {
        /* Then the ISI Message should be sent to the IPC of the Protocol */
        ipcId = ipc_ptr->protocolQId;
    }

    /* send to queue  */
    if (OSAL_SUCCESS != OSAL_msgQSend(ipcId, (char*)msg_ptr,
            sizeof(ISIP_Message), OSAL_WAIT_FOREVER, NULL)) {
         OSAL_logMsg("%s %d: ERROR send event failed.\n",
                __func__, __LINE__);
         return (ISI_RETURN_FAILED);
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISIQ_getEvent() ========
 * This function will wait on a group of queues for events and commands that
 * are in the 'application' and 'protocol' queues and process them
 * accordingly. Either these events will given to the application or they
 * are destined to be processed by the internals of the ISI module.
 *
 * Returns:
 *  ISI_RETURN_OK : There is an event ready to be processed.
 *  ISI_RETURN_TIMEOUT : The read wait times out and there is no data to be
 *                       processed.
 *  ISI_RETURN_FAILED : There is no data to process in the queue.  There was
 *                      some error with the read.
 */
ISI_Return ISIQ_getEvent(
    char  *data_ptr,
    vint  *len_ptr,
    vint  *isApplication_ptr,
    vint   timeout)
{
    OSAL_MsgQId  qId;
    vint         ret;
    OSAL_Boolean isTo;

    /* Set the timeout object */
    if (-1 == timeout) {
        /* Then wait indefinitely */
        timeout = OSAL_WAIT_FOREVER;
    }
    else if (0 == timeout) {
        /* Don't wait at all */
        timeout = OSAL_NO_WAIT;
    }

    isTo = OSAL_FALSE;
    ret = OSAL_msgQGrpRecv(&_ISIQ_group, data_ptr, *len_ptr, timeout, &qId,
            &isTo);
    if (0 >= ret) {
        if (OSAL_TRUE == isTo) {
            return (ISI_RETURN_TIMEOUT);
        }
        return (ISI_RETURN_FAILED);
    }

    /* Otherwise we have a valid read */
    if (qId == _ISIQ_app) {
        *isApplication_ptr = 1;
    }
    else {
        *isApplication_ptr = 0;
    }
    return (ISI_RETURN_OK);
}

/*
 * ======== ISIQ_addProtocol() ========
 * This function is called when an underlying protocol application
 * registers with ISI.  This function will cache the IPC names associated
 * with the protocol applicationa nd also it's voice engine (audio protocol).
 *
 * Returns:
 *  ISI_RETURN_OK : The protocol and it's associated protocol IPC and audio IPC
 *                  have been successfully saved.
 *  ISI_RETURN_FAILED : The array that saves protocol information is full.
 */
ISI_Return ISIQ_addProtocol(
    vint  protocol,
    char *protocolIpc_ptr,
    char *mediaIpc_ptr,
    char *streamIpc_ptr)
{
    vint    x;
    vint    srcModId, dstProModId, dstMediaModId, dstStreamModId;

    /* src dst module Id */
    srcModId = OSAL_MODULE_ISI;
    /* Look for dst module Id of protocol IPC by protocol */
    if (ISI_PROTOCOL_VTSP == protocol) {
        dstProModId    = OSAL_MODULE_MC;
        dstMediaModId  = OSAL_MODULE_MC;
        dstStreamModId = OSAL_MODULE_MC;
    }
    else if (ISI_PROTOCOL_SIP == protocol) {
        dstProModId    = OSAL_MODULE_SAPP;
        dstMediaModId  = OSAL_MODULE_MC;
        dstStreamModId = OSAL_MODULE_MC;
    }
    else if (ISI_PROTOCOL_GSM == protocol) {
        dstProModId    = OSAL_MODULE_GAPP;
        dstMediaModId  = OSAL_MODULE_MC;
        dstStreamModId = OSAL_MODULE_GAPP;
    }
    else {
        dstProModId    = OSAL_MODULE_ISI_MSG_RECV;
        dstMediaModId  = OSAL_MODULE_MC;
        dstStreamModId = OSAL_MODULE_MC;
    }


    /* First check if this protocol already exists */
    for (x = 0 ; x < ISID_MAX_NUM_PROTOCOLS ; x++) {
        /* Check for duplicate entries */
        if (OSAL_TRUE == _ISIQ_protocolArray[x].isValid &&
                protocol == _ISIQ_protocolArray[x].protocol) {
            /*
             * Check protocol ipc is ready and queue is created.
             * If not, create the queue
             */
            if (0 != OSAL_strcmp(_ISIQ_protocolArray[x].protocolIpc,
                    protocolIpc_ptr)) {
                /*
                 * the ipc is not same with original, so copy the info and
                 * delete original queue and then create a new message queue.
                 */
                OSAL_strncpy(_ISIQ_protocolArray[x].protocolIpc,
                        protocolIpc_ptr, ISI_ADDRESS_STRING_SZ);
                if (0 != _ISIQ_protocolArray[x].protocolQId) {
                    OSAL_msgQDelete(_ISIQ_protocolArray[x].protocolQId);
                }

                if (0 == (_ISIQ_protocolArray[x].protocolQId =
                        OSAL_msgQCreate(protocolIpc_ptr,
                        srcModId, dstProModId, OSAL_DATA_STRUCT_ISIP_Message,
                        ISIQ_MAX_DEPTH,
                        ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                    return (ISI_RETURN_FAILED);
               }
            }
            else {
                /*
                 * The ipc is same with original, so to check the message queue
                 * if has been created.
                 * If the message queue is not created yet, create a new one.
                 */
                if (0 == _ISIQ_protocolArray[x].protocolQId) {
                    if (0 == (_ISIQ_protocolArray[x].protocolQId =
                            OSAL_msgQCreate(protocolIpc_ptr,
                            srcModId, dstProModId,
                            OSAL_DATA_STRUCT_ISIP_Message,
                            ISIQ_MAX_DEPTH,
                            ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                        return (ISI_RETURN_FAILED);
                    }
                }
            }

            /*
             * Check media ipc is ready and queue is created.
             * If not, create the queue
             */
            if (0 != OSAL_strcmp(_ISIQ_protocolArray[x].mediaIpc,
                    mediaIpc_ptr)) {
                /*
                 * the ipc is not same with original, so copy the info and
                 * delete original queue and then create a new message queue.
                 */
                OSAL_strncpy(_ISIQ_protocolArray[x].mediaIpc,
                        mediaIpc_ptr, ISI_ADDRESS_STRING_SZ);
               if (0 != _ISIQ_protocolArray[x].mediaQId) {
                    OSAL_msgQDelete(_ISIQ_protocolArray[x].mediaQId);
                }
                if (0 == (_ISIQ_protocolArray[x].mediaQId =
                        OSAL_msgQCreate(mediaIpc_ptr,
                        srcModId, dstMediaModId, OSAL_DATA_STRUCT_ISIP_Message,
                        ISIQ_MAX_DEPTH,
                        ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                    return (ISI_RETURN_FAILED);
                }
            }
            else {
                /*
                 * The ipc is same with original, so to check the message queue
                 * if has been created.
                 * If the message queue is not created yet, create a new one.
                 */
                if (0 == _ISIQ_protocolArray[x].mediaQId) {
                    if (0 == (_ISIQ_protocolArray[x].mediaQId =
                            OSAL_msgQCreate(mediaIpc_ptr,
                            srcModId, dstMediaModId,
                            OSAL_DATA_STRUCT_ISIP_Message,
                            ISIQ_MAX_DEPTH,
                            ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                        return (ISI_RETURN_FAILED);
                    }
                }
            }

            /*
             * Check stream ipc is ready and queue is created.
             * If not, create the queue
             */
            if (0 != OSAL_strcmp(_ISIQ_protocolArray[x].streamIpc,
                    streamIpc_ptr)) {
                /*
                 * the ipc is not same with original, so copy the info and
                 * delete original queue and then create a new message queue.
                 */
                OSAL_strncpy(_ISIQ_protocolArray[x].streamIpc,
                        streamIpc_ptr, ISI_ADDRESS_STRING_SZ);
                if (0 != _ISIQ_protocolArray[x].streamQId) {
                    OSAL_msgQDelete(_ISIQ_protocolArray[x].streamQId);
                }
                if (0 == (_ISIQ_protocolArray[x].streamQId =
                        OSAL_msgQCreate(streamIpc_ptr,
                        srcModId, dstStreamModId, OSAL_DATA_STRUCT_ISIP_Message,
                        ISIQ_MAX_DEPTH,
                        ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                    return (ISI_RETURN_FAILED);
                }
            }
            else {
                /*
                 * The ipc is same with original, so to check the message queue
                 * if has been created.
                 * If the message queue is not created yet, create a new one.
                 */
                if (0 == _ISIQ_protocolArray[x].streamQId) {
                    if (0 == (_ISIQ_protocolArray[x].streamQId =
                            OSAL_msgQCreate(streamIpc_ptr,
                            srcModId, dstStreamModId,
                            OSAL_DATA_STRUCT_ISIP_Message,
                            ISIQ_MAX_DEPTH,
                            ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                        return (ISI_RETURN_FAILED);
                    }
                }
            }
            return (ISI_RETURN_OK);
        }
    }
    /* If we are here then the protocol doesn't exist so add it */
    for (x = 0 ; x < ISID_MAX_NUM_PROTOCOLS ; x++) {
        if(OSAL_FALSE == _ISIQ_protocolArray[x].isValid) {
            /* Found an available cell for this entry */
            _ISIQ_protocolArray[x].isValid = OSAL_TRUE;
            _ISIQ_protocolArray[x].protocol = protocol;
            OSAL_strncpy(_ISIQ_protocolArray[x].protocolIpc,
                    protocolIpc_ptr, ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(_ISIQ_protocolArray[x].mediaIpc,
                    mediaIpc_ptr, ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(_ISIQ_protocolArray[x].streamIpc,
                    streamIpc_ptr, ISI_ADDRESS_STRING_SZ);

            /* create/open a connection to the app queue  */
            if (0 == (_ISIQ_protocolArray[x].protocolQId =
                    OSAL_msgQCreate(protocolIpc_ptr,
                    srcModId, dstProModId, OSAL_DATA_STRUCT_ISIP_Message,
                    ISIQ_MAX_DEPTH,
                    ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                return (ISI_RETURN_FAILED);
            }

            if (0 == (_ISIQ_protocolArray[x].mediaQId =
                    OSAL_msgQCreate(mediaIpc_ptr,
                    srcModId, dstMediaModId, OSAL_DATA_STRUCT_ISIP_Message,
                    ISIQ_MAX_DEPTH,
                    ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                return (ISI_RETURN_FAILED);
            }

            if (0 == (_ISIQ_protocolArray[x].streamQId =
                    OSAL_msgQCreate(streamIpc_ptr,
                    srcModId, dstStreamModId, OSAL_DATA_STRUCT_ISIP_Message,
                    ISIQ_MAX_DEPTH,
                    ISIQ_PROTO_MAX_MSG_SZ, 0))) {
                return (ISI_RETURN_FAILED);
            }
            return (ISI_RETURN_OK);
        }
    }
    return (ISI_RETURN_FAILED);
}

ISIQ_Ipc* ISIQ_getProtocol(
    vint  protocol)
{
    vint x;
    for (x = 0 ; x < ISID_MAX_NUM_PROTOCOLS ; x++) {
        if (OSAL_TRUE == _ISIQ_protocolArray[x].isValid) {
            /* Found a valid entry */
            if (protocol == _ISIQ_protocolArray[x].protocol) {
                return (&_ISIQ_protocolArray[x]);
            }
        }
    }
    return (NULL);
}

ISI_Return ISIQ_clearProtocol(
    vint  protocol)
{
    vint x;
    for (x = 0 ; x < ISID_MAX_NUM_PROTOCOLS ; x++) {
        if (OSAL_TRUE == _ISIQ_protocolArray[x].isValid) {
            /* Found a valid entry */
            if (protocol == _ISIQ_protocolArray[x].protocol) {
                _ISIQ_protocolArray[x].isValid = OSAL_FALSE;
                /* delete queue */
                if (0 != _ISIQ_protocolArray[x].protocolQId) {
                    OSAL_msgQDelete(_ISIQ_protocolArray[x].protocolQId);
                }
                if (0 != _ISIQ_protocolArray[x].mediaQId) {
                    OSAL_msgQDelete(_ISIQ_protocolArray[x].mediaQId);
                }
                if (0 != _ISIQ_protocolArray[x].streamQId) {
                    OSAL_msgQDelete(_ISIQ_protocolArray[x].streamQId);
                }
                return (ISI_RETURN_OK);
            }
        }
    }
    return (ISI_RETURN_FAILED);
}
