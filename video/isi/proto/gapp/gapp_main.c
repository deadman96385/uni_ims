/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30233 $ $Date: 2014-12-08 10:29:54 +0800 (Mon, 08 Dec 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include "isi.h"
#include "isip.h"
#ifndef GAPP_DISABLE_GSM
#include "gsm.h"
#endif
#include "_gapp.h"
#include "_gapp_call.h"
#include "proxy.h"
#include "proxy_io.h"
#include <settings.h>

/* Global object. */
static GAPP_GlobalObj  _GAPP_globalObj;

/* 
 * ======== _GAPP_systemIsiEvt() ========
 *
 * This populates an ISI event related to 'system' level events.
 *
 * Return Values: 
 * None
 */   
static void _GAPP_systemIsiEvt(
    ISIP_SystemReason  reason,
    ISIP_Status         status,
    char               *ipcName_ptr,    
    char               *audioName_ptr,
    char               *streamName_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = 0;
    isi_ptr->code = ISIP_CODE_SYSTEM;
    isi_ptr->protocol = _GAPP_globalObj.protocolId;
    isi_ptr->msg.system.reason = reason;
    isi_ptr->msg.system.status = status;
    /* 
     * Tell ISI of the IPC names we are using for ISI communication, 
     * Audio control (tones) and stream control. 
     */
    isi_ptr->msg.system.protocolIpc[0] = 0;
    if (NULL != ipcName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.protocolIpc,
                ISI_ADDRESS_STRING_SZ, "%s", ipcName_ptr);
    }
        
    isi_ptr->msg.system.mediaIpc[0] = 0;
    if (NULL != audioName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.mediaIpc,
                ISI_ADDRESS_STRING_SZ, "%s", audioName_ptr);
    }
    
    isi_ptr->msg.system.streamIpc[0] = 0;
    if (NULL != streamName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.streamIpc,
                ISI_ADDRESS_STRING_SZ, "%s", streamName_ptr);
    }
    return;
}
#ifndef GAPP_DISABLE_GSM
/* 
 * ======== _GAPP_timerCb() ========
 *
 * This function is registered with a timer used for GAPP registration with
 * ISI.  The timer calls this function when it expires.  IT will send 
 * an ISI event indicating that it wants to register with ISI.
 *
 * Return Values: 
 * None
 */
static int32 _GAPP_timerCb(
    void *arg_ptr) 
{
    GAPP_Event *evt_ptr;

    evt_ptr = (GAPP_Event *)arg_ptr;

    /* This is a periodic timer, do not need to wait forever for sending msg. */
    if (OSAL_SUCCESS != OSAL_msgQSend(evt_ptr->isiEvtQId,
            (char *)&evt_ptr->isiMsg, sizeof(ISIP_Message),
            OSAL_NO_WAIT, NULL)) {
        GAPP_dbgPrintf("%s: ERROR to send event.\n", __FUNCTION__);
        return (GAPP_ERR);
    }

    return (GAPP_OK);
}

/* 
 * ======== _GAPP_timerInit() ========
 *
 * This function initializes a timer used to register GAPP with
 * ISI.  The timer fires at the specified interval until 
 * ISI returns a Command to GAPP indicating that it received
 * this event.
 *
 * Return Values: 
 * GAPP_OK The timer was succesfully initialized.
 * GAPP_ERR The timer failed to initialize
 */
static vint _GAPP_timerInit(
    GAPP_GsmObj  *gsm_ptr)
{
    if (0 == gsm_ptr->tmr.id) {
        /* Launch a timer that will attempt to register to ISI */
        if (0 == (gsm_ptr->tmr.id = OSAL_tmrCreate())) {
            /* Then we can't register with ISI! */
            return (GAPP_ERR);
        }
    }
    else {
        OSAL_tmrStop(gsm_ptr->tmr.id);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrPeriodicStart(gsm_ptr->tmr.id, _GAPP_timerCb, 
            &gsm_ptr->tmr.event, GAPP_REGISTER_TIMER_MS)) {
        OSAL_tmrDelete(gsm_ptr->tmr.id);
        gsm_ptr->tmr.id = 0;
        return (GAPP_ERR);
    }
    return (GAPP_OK);
}

/* 
 * ======== _GAPP_timerDestroy() ========
 *
 * This function kills/frees the timer used to register this app to ISI.
 *
 * Return Values: 
 * None.
 */
static void _GAPP_timerDestroy(
    GAPP_GsmObj *gsm_ptr)
{
    /* Kill/Free the timer */
    if (0 != gsm_ptr->tmr.id) {
        OSAL_tmrStop(gsm_ptr->tmr.id);
        OSAL_tmrDelete(gsm_ptr->tmr.id);
        gsm_ptr->tmr.id = 0;
    }   
    return;
}

/* 
 * ======== _GAPP_isiSystemCmd() ========
 * This function is the entry point for commands from ISI related to 
 * the protocol or "system".
 *
 * Return Values:
 * None.
 */
static void _GAPP_isiSystemCmd(
    GAPP_GsmObj   *gsm_ptr, 
    ISIP_Message  *cmd_ptr)
{
    if (ISIP_SYSTEM_REASON_START == cmd_ptr->msg.system.reason &&
            ISIP_STATUS_DONE == cmd_ptr->msg.system.status) {
        /* Then GAPP successfully contacted/registered ISI */
        _GAPP_timerDestroy(gsm_ptr);
        GAPP_dbgPrintf("%s: Successfully registered with ISI\n", __FUNCTION__);        
    }
    else if (ISIP_SYSTEM_REASON_SHUTDOWN == cmd_ptr->msg.system.reason &&
            ISIP_STATUS_DONE == cmd_ptr->msg.system.status) {
        /* Then restart the registration timer */
        _GAPP_timerInit(gsm_ptr);
    }
    return;
}
#endif
/* 
 * ======== _GAPP_task() ========
 * This function is the task handler for ISI commands and GSM event. 
 * It processes commands from ISI and result codes from the GSM module 
 * serially and executes appropriate state machines and command/event handlers.
 *
 * Return Values:
 * Nothing and Never
 */
static void _GAPP_task(
    void *arg_ptr)
{
    GAPP_Msg      *msg_ptr;
    GAPP_GsmObj   *gsm_ptr;
    OSAL_MsgQId    fromq;
#ifndef GAPP_DISABLE_GSM    
    char          *result_ptr;
    vint           resultLen;
    GSM_Id         gsmId;
    GAPP_Buffer    buff;
#endif
    PRXY_Return    ret;
    char           scratch[GAPP_AT_COMMAND_SIZE];
    int len;

    gsm_ptr = (GAPP_GsmObj*)arg_ptr;
    msg_ptr = &gsm_ptr->queue.msg;

        
_GAPP_GSM_TASK_TOP:

    while (0 >= (len = OSAL_msgQGrpRecv(gsm_ptr->queue.group_ptr,
            (char *)msg_ptr, sizeof(GAPP_Msg), OSAL_WAIT_FOREVER,
            &fromq, NULL))) {
        OSAL_taskDelay(100);
    }

    /* GAPP_dbgPrintf("%s: Got an ISI Command in the SM.\n", __FUNCTION__); */

    /* 
     * Before we do anything clear our the buffer used to construct isi events.
     * Since gsm_ptr->event.isiMsg.code will be ISIP_CODE_INVALID after this 
     * call, then we know if there is an event to write to isi at the bottom of
     * this function by seeing if anyone changed the isi event buffer from 
     * ISIP_CODE_INVALID to something else.
     */
    OSAL_memSet(&gsm_ptr->event.isiMsg, 0, sizeof(ISIP_Message));

    if (fromq == gsm_ptr->queue.proxyCmdEvt) {
        /* NULL terminate the command. */
        msg_ptr->u.proxy[len] = 0;  
        if (PRXY_RETURN_WAIT == PRXY_processAtCommand(&gsm_ptr->proxyCmdMngr, 
                msg_ptr->u.proxy)) {
            /* Then stop processing the command 'q' until we get a response. */
            gsm_ptr->queue.group_ptr = &gsm_ptr->queue.groupResponsesOnly;
        }
    }
    else if (fromq == gsm_ptr->queue.respEvt) {
        ret = PRXY_processCsmOutputEvent(&gsm_ptr->proxyCmdMngr,
                &msg_ptr->u.response);
        if (PRXY_RETURN_OK == ret) {
            /* Then we should start processing the command q again. */
            gsm_ptr->queue.group_ptr = &gsm_ptr->queue.groupAll;
        }
        else if (PRXY_RETURN_CONTINUE == ret) {
            /* Process CSM network registration event. */
            if (GAPP_OK == GAPP_processCsmRegOutputEvent(
                    &gsm_ptr->service, scratch)) {
                PRXY_ioWrite(scratch, OSAL_strlen(scratch));
            }
        }
    }
#ifndef GAPP_DISABLE_GSM
    else if (fromq == gsm_ptr->queue.gsmEvt) {
        /* Then we have an event from the GSM module. */
        result_ptr = msg_ptr->u.gsm.arg;
        resultLen = OSAL_strlen(result_ptr);
        gsmId = msg_ptr->u.gsm.id;
        GAPP_initBuffer(result_ptr, resultLen, &buff);

        ret = PRXY_processGsmEvent(
                &gsm_ptr->proxyCmdMngr, gsmId, result_ptr, resultLen);
        /* Check if this event belongs to the proxy command manager. */
        if (PRXY_RETURN_WAIT != ret) {
            if (PRXY_RETURN_CONTINUE == ret) {
                /* the event is pass-throught event 
                 * and it need to be processed by GAPP. 
                 */
                GAPP_initBuffer(NULL, 0, &buff);
                if (GAPP_OK == GAPP_networkRegSolicitedEvent(
                        &gsm_ptr->service, &buff, &gsm_ptr->event)) {
                    PRXY_ioWrite(result_ptr, OSAL_strlen(result_ptr));
                }
            }
            /* Then this was process by the proxy. */
            GAPP_dbgPrintf("%s: Proxy processed... gsmId:%d recv'd:%s\n",
                    __FUNCTION__, gsmId, result_ptr);
            /* Let's start processing commands again. */
            gsm_ptr->queue.group_ptr = &gsm_ptr->queue.groupAll;
        }
        else if (0 != gsmId) {
            GAPP_dbgPrintf("%s: ISI destined result  gsmId:%d recv'd:%s\n",
                        __FUNCTION__, gsmId, result_ptr);
            /* Check if it is a result code to a previous call command */
            if (GAPP_OK == GAPP_callResultEvent(&gsm_ptr->service,
                    &buff, gsmId, &gsm_ptr->event.isiMsg)) {
                /* Then the event is handled.  Nothing else to do */
                GAPP_dbgPrintf("%s: GSM Call related result processed\n",
                        __FUNCTION__);
                goto _GAPP_GSM_TASK_LOOP;
            }
            
            /* 
             * Check if it is a result code belongs to a service 
             * related command.  First 'reset' the result buffer stuff
             */
            GAPP_initBuffer(NULL, 0, &buff);
            if (GAPP_OK == GAPP_serviceResultEvent(&gsm_ptr->service,
                    &buff, gsmId, &gsm_ptr->event)) {
                /* Then the event is handled.  Nothing else to do */
                GAPP_dbgPrintf("%s: GSM Service related result processed\n",
                        __FUNCTION__);
                //PRXY_write(result_ptr, resultLen);
                goto _GAPP_GSM_TASK_LOOP;
            }
            /* 
             * Check if it is a result code belongs to a sms or text message
             * command.  First 'reset' the result buffer stuff
             */

            GAPP_initBuffer(NULL, 0, &buff);
            if (GAPP_OK == GAPP_smsResultEvent(&gsm_ptr->service,
                    &buff, gsmId, &gsm_ptr->event.isiMsg)) {
                /* Then the event is handled.  Nothing else to do */
                GAPP_dbgPrintf("%s: GSM SMS related result processed\n",
                        __FUNCTION__);
                goto _GAPP_GSM_TASK_LOOP;
            }
            /* 
             * Check if it is a result code belongs to a tel event 
             * command.  First 'reset' the result buffer stuff
             */
            GAPP_initBuffer(NULL, 0, &buff);
            if (GAPP_OK == GAPP_telEvtResultEvent(&gsm_ptr->service,
                    &buff, gsmId, &gsm_ptr->event.isiMsg)) {
                /* Then the event is handled.  Nothing else to do */
                /*GAPP_dbgPrintf("%s: GSM TelEvt related result processed\n",
                        __FUNCTION__);*/
                goto _GAPP_GSM_TASK_LOOP;
            }
            /* XXX: Add more handling for other command events here */
        }
        else {
            /* 
             * then gsmId is '0'. It's an 'unsolicited' result code. 
             * pass to all sub modules 
             */
            GAPP_dbgPrintf("%s: Event gsmId:%d received:%s ", __FUNCTION__, 
                    gsmId, result_ptr);
            /* Check if it is SRVCC failure event. */
            if (GAPP_OK == GAPP_unsolicitedSrvccEvent(&gsm_ptr->service,
                    &buff, &gsm_ptr->event)) {
                goto _GAPP_GSM_TASK_LOOP;
            }

            GAPP_initBuffer(NULL, 0, &buff);
            /* Check if it's related to a call. */
            if (GAPP_OK == GAPP_callUnsolicitedEvent(&gsm_ptr->service,
                    &buff, &gsm_ptr->event)) {
                /* Then it's been handled. Loop again for another event */
                GAPP_dbgPrintf(
                        "%s: GSM unsolicited Call related result processed\n",
                        __FUNCTION__);
                goto _GAPP_GSM_TASK_LOOP;
            }
            /* 
             * Check if this event belongs to a service.  
             * First 'reset' the result buffer stuff.
             */
            GAPP_initBuffer(NULL, 0, &buff);
            if (GAPP_OK == GAPP_serviceUnsolicitedEvent(&gsm_ptr->service, 
                    &buff, &gsm_ptr->event)) {
                /* Then it's been handled. Loop again for another event */
                /*GAPP_dbgPrintf(
                    "%s: GSM unsolicited Service related result processed\n",
                    __FUNCTION__);*/
                /* Also pass it up! */
                PRXY_ioWrite(result_ptr, resultLen);
                goto _GAPP_GSM_TASK_LOOP;
            }
            /* 
             * Check if this event belongs to a sms (text messaging).  
             * First 'reset' the result buffer stuff.
             */
            GAPP_initBuffer(NULL, 0, &buff);
            if (GAPP_OK == GAPP_smsUnsolicitedEvent(&gsm_ptr->service,
                    &buff, &gsm_ptr->event.isiMsg)) {
                /* Then it's been handled. Loop again for another event */
                GAPP_dbgPrintf(
                        "%s: GSM unsolicited SMS related result processed\n",
                        __FUNCTION__);
                goto _GAPP_GSM_TASK_LOOP;
            }

            /*
             * If we are here, then GAPP/ISI does not care about this 
             * unsolicited event.
             * Pass it up through the proxy.
             */
            PRXY_ioWrite(result_ptr, resultLen);
            /* XXX: Add more handling for other unsolicited events here */
        }
    }
    
    else if (fromq == gsm_ptr->queue.isiCmd) {
        /*
         * Process ISI command. determine the message type ("code") and process
         */
        if (_GAPP_globalObj.protocolId != msg_ptr->u.isi.protocol) {
            goto _GAPP_GSM_TASK_TOP;
        }
        
        switch (msg_ptr->u.isi.code) {
            case ISIP_CODE_SYSTEM:
                _GAPP_isiSystemCmd(
                        gsm_ptr, /* This is a pointer to GAPP_GlobalObj */
                        &msg_ptr->u.isi);
                break;
            case ISIP_CODE_SERVICE:
                GAPP_isiServiceCmd(gsm_ptr, &gsm_ptr->service, &msg_ptr->u.isi,
                        &gsm_ptr->event.isiMsg);
                break;
            case ISIP_CODE_CALL:
                GAPP_isiCallCmd(&gsm_ptr->service, &msg_ptr->u.isi,
                        &gsm_ptr->event);
                break;
            case ISIP_CODE_MESSAGE:
                GAPP_isiSmsCmd(&gsm_ptr->service, &msg_ptr->u.isi,
                        &gsm_ptr->event.isiMsg);
                break;
            case ISIP_CODE_MEDIA:
                GAPP_isiAudioCmd(&gsm_ptr->service, &msg_ptr->u.isi,
                        &gsm_ptr->event);
                break;
            case ISIP_CODE_TEL_EVENT:
                GAPP_isiTelEventCmd(&gsm_ptr->service, &msg_ptr->u.isi,
                        &gsm_ptr->event.isiMsg);
                break;
            default:
                break;
        } /* End of switch */
    }   
_GAPP_GSM_TASK_LOOP:
#endif
    if (gsm_ptr->event.isiMsg.code != ISIP_CODE_INVALID) {
        /* Then there is an event waiting to be written to ISI */
        GAPP_sendEvent(&gsm_ptr->event);
    }
    goto _GAPP_GSM_TASK_TOP;

}

/* 
 * ======== _GAPP_gsmInit() ========
 * Initializes all resources needed to service ISI commands and GSM events.
 * This includes...
 * 1) OSAL Queue used for ISI commands
 * 2) OSAL Queue used for GSM events
 * 3) An OSAL group to group the above queues
 * 4) A task used to service the OSAL queue group and process any state
 *    machines.
 *
 * Return Values:
 * GAPP_OK  All resource were successfully initialized
 * GAPP_ERR Failed to init all resources needed to service GAPP.
 */
static vint _GAPP_gsmInit(
    GAPP_GsmObj *gsm_ptr,
    char        *ipc_ptr,
    char        *isiIpc_ptr)
{
    GAPP_TaskObj *task_ptr;
#ifndef GAPP_DISABLE_GSM  
    if (0 == (gsm_ptr->event.isiEvtQId =
            OSAL_msgQCreate(isiIpc_ptr,
            OSAL_MODULE_GAPP, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
            GAPP_MAX_QUEUE_DEPTH,
            sizeof(ISIP_Message), 0))) {
        GAPP_dbgPrintf("%s: Error creating isi event queue\n", __FUNCTION__);
        return (GAPP_ERR);
    }
    /* ISI Command queue. */
    if (0 == (gsm_ptr->queue.isiCmd = OSAL_msgQCreate(ipc_ptr, 
            OSAL_MODULE_ISI, OSAL_MODULE_GAPP, OSAL_DATA_STRUCT_ISIP_Message,
            GAPP_MAX_QUEUE_DEPTH, sizeof(ISIP_Message), 0))) {
        return (GAPP_ERR);
    }
#endif
    /* Response queue. */
    if (0 == (gsm_ptr->queue.respEvt = 
            OSAL_msgQCreate(CSM_OUTPUT_EVENT_QUEUE_NAME,
            OSAL_MODULE_CSM_PUBLIC, OSAL_MODULE_GAPP,
            OSAL_DATA_STRUCT_CSM_OutputEvent,
            CSM_OUTPUT_EVENT_MSGQ_LEN, 
            sizeof(CSM_OutputEvent), 0))) {
        return (GAPP_ERR);
    }
 
    /* Queue group for all queues. */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&gsm_ptr->queue.groupAll)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }

    /* Queue group for queues the service responses only. */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&gsm_ptr->queue.groupResponsesOnly)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }
 #ifndef GAPP_DISABLE_GSM  
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupAll,
            gsm_ptr->queue.isiCmd)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupResponsesOnly,
            gsm_ptr->queue.isiCmd)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }
#endif
    /* Add the response queue to both the 'All' group and the 'Responses Only' Group. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupAll,
            gsm_ptr->queue.respEvt)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }
#ifndef GAPP_DISABLE_RESPONSE_ONLY
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupResponsesOnly,
            gsm_ptr->queue.respEvt)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }
#endif
    /* Init the one and only service */
    GAPP_initService(&gsm_ptr->service, gsm_ptr, 0, 0);
    
#ifndef GAPP_DISABLE_PROXY
    if (OSAL_SUCCESS != PRXY_init(gsm_ptr->proxyTerminalName)) {
        GAPP_dbgPrintf("%s: CAN't INIT proxy!\n", __FUNCTION__);
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        return (GAPP_ERR);
    }

    /*
     * Get the event Q from the PROXY module so we can
     * add it to the group q for this task.  If the PRXY_init() returns
     * successfully then we know this stuff will be successful
     */
    
    PRXY_ioGetEventQueue(&gsm_ptr->queue.proxyCmdEvt);
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupAll,
            gsm_ptr->queue.proxyCmdEvt)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        PRXY_destroy();
        GAPP_dbgPrintf("%s %d: ERROR!\n", __FUNCTION__, __LINE__);
        return (GAPP_ERR);
    }
#endif
    /* Init the GSM stuff. */
#ifndef GAPP_DISABLE_AT_DEVICE
    if (GSM_RETURN_OK != GSM_init(gsm_ptr->gsmDrvrFile,
            gsm_ptr->proxyCmdMngr.extDialCmdEnabled)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        GAPP_dbgPrintf("%s: GSM ERROR!\n", __FUNCTION__);
        PRXY_destroy();
        return (GAPP_ERR);
    }

    /*
     * Get the event Q from the GSM module so we can
     * add it to the group q for this task.  If the GSM_init() returns
     * successfully then we know this stuff will be successful
     */
    GSM_getEventQueue(&gsm_ptr->queue.gsmEvt);
    /* Add to 'All' group and 'Responses only' group */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupAll, gsm_ptr->queue.gsmEvt)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        PRXY_destroy();
        GSM_destroy();
        GAPP_dbgPrintf("%s: GSM ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&gsm_ptr->queue.groupResponsesOnly, gsm_ptr->queue.gsmEvt)) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        gsm_ptr->queue.respEvt = 0;
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
        gsm_ptr->queue.isiCmd = 0;
        PRXY_destroy();
        GSM_destroy();
        GAPP_dbgPrintf("%s: GSM ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }
#endif
    /* Now init the 'group' to start receiving on to 'All' */
    gsm_ptr->queue.group_ptr = &gsm_ptr->queue.groupAll;

    /* Start the main task that services ISI commands and GSM Events */
    task_ptr = &gsm_ptr->task;
    task_ptr->tId      = 0;
    task_ptr->stackSz  = GAPP_TASK_STACK_BYTES;
    task_ptr->pri      = OSAL_TASK_PRIO_NRT;
    task_ptr->func_ptr = _GAPP_task;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            GAPP_GSM_TASK_NAME);
    task_ptr->arg_ptr  = gsm_ptr;

    if (0 == (task_ptr->tId = OSAL_taskCreate(
            task_ptr->name,
            (OSAL_TaskPrio)task_ptr->pri,
            task_ptr->stackSz,
            (OSAL_TaskPtr)task_ptr->func_ptr,
            (void *)task_ptr->arg_ptr))) {
        OSAL_msgQDelete(gsm_ptr->queue.respEvt);
        OSAL_msgQDelete(gsm_ptr->queue.isiCmd);
#ifndef GAPP_DISABLE_GSM        
        GSM_destroy();
#endif
        PRXY_destroy();
        GAPP_dbgPrintf("%s: ERROR!\n", __FUNCTION__);
        return (GAPP_ERR);
    }

    return (GAPP_OK);
}

/* 
 * ======== _GAPP_gsmDestroy() ========
 * Frees the resources needed to service ISI commands and GSM events and 
 * process any state machines.
 *
 * Return Values:
 * GAPP_OK  All resources were successfully freed.  
 * GAPP_ERR There was an error freeing one or many of the resources.
 *
 */
static vint _GAPP_gsmDestroy(
    GAPP_GsmObj *gsm_ptr)
{
    vint ret; 
    /* Delete tasks, queues, devs, etc. */
    
    ret = GAPP_OK;

    if (OSAL_SUCCESS != OSAL_taskDelete(gsm_ptr->task.tId)) {
        ret = GAPP_ERR;
    }

    if (OSAL_SUCCESS != OSAL_msgQDelete(gsm_ptr->queue.isiCmd)) {
        ret = GAPP_ERR;
    }
    
    if (OSAL_SUCCESS != OSAL_msgQDelete(gsm_ptr->queue.respEvt)) {
        ret = GAPP_ERR;
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpDelete(&gsm_ptr->queue.groupAll)) {
        ret = GAPP_ERR;
    }

    if (OSAL_SUCCESS !=
            OSAL_msgQGrpDelete(&gsm_ptr->queue.groupResponsesOnly)) {
        ret = GAPP_ERR;
    }

    PRXY_destroy();
#ifndef GAPP_DISABLE_GSM    
    GSM_destroy();
#endif
    return (ret);
}

/* 
 * ======== _GAPP_start() ========
 * This function is the first function to be called.  It will initialize
 * all needed system resources and begin servicing commands from ISI and 
 * events from the GSM module.
 *
 * Return Values:
 * GAPP_OK  GAPP is successfully running.
 * GAPP_ERR There was an error launching the GAPP application.
 *
 */
static int _GAPP_start(
    void           *cfg_ptr,
    GAPP_GlobalObj *global_ptr)
{
    char        *value_ptr;
    char        *isiIpc_ptr;
    char        *mediaIpc_ptr;
    char        *streamIpc_ptr;
    char        *ipc_ptr;
    char        *temp_ptr;
    int          index = 0;
    /* Get the names of the ipc interfaces */
    ipc_ptr = NULL;
    isiIpc_ptr = NULL;
    mediaIpc_ptr = NULL;
    streamIpc_ptr = NULL;

    /* Get "this" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        ipc_ptr = value_ptr;
    }

    /* Get "isi" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_ISI))) {
        isiIpc_ptr = value_ptr;
    }

    /* Get "audio" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_AUDIO))) {
        mediaIpc_ptr = value_ptr;
    }

    /* Get "stream" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_STREAM))) {
        streamIpc_ptr = value_ptr;
    }


    if (NULL == ipc_ptr || NULL == isiIpc_ptr || NULL == mediaIpc_ptr ||
            NULL == streamIpc_ptr) {
        /* The IPC names are manditory */
        return (GAPP_ERR);
    }

    /* Set the ring template number to use when ringing a phone */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RING_TEMPLATE))) {
        global_ptr->gsmObj.service.ringTemplate = OSAL_atoi(value_ptr);
    }

    /* Get the protocol ID to use. */
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            NULL, NULL, SETTINGS_ATTR_ID))) {
        _GAPP_globalObj.protocolId = OSAL_atoi(value_ptr);
        if ((ISI_PROTOCOL_GSM != _GAPP_globalObj.protocolId) &&
                (ISI_PROTOCOL_LTE != _GAPP_globalObj.protocolId)) {
            GAPP_dbgPrintf("%s:%d Invalid protocol id %d\n",
                    __FUNCTION__, __LINE__, _GAPP_globalObj.protocolId);
        }
    }

    /* Set the default 'vdi' for VCC handoff */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_HANDOFF, NULL, SETTINGS_PARM_VDI))) {
        OSAL_snprintf(global_ptr->gsmObj.service.vdi, GAPP_STRING_SZ, "%s",
                value_ptr);
    }
#ifndef GAPP_DISABLE_GSM
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_GSM, NULL, SETTINGS_PARM_FILE))) {
        /* Set the file to init the underlying GSM driver */
        OSAL_snprintf(global_ptr->gsmObj.gsmDrvrFile, GAPP_STRING_SZ, "%s",
                value_ptr);
    }

    /* Get Extended dial command enabled. */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_GSM, NULL, SETTINGS_PARM_EXT_DIAL_CMD_ENABLED))) {
        global_ptr->gsmObj.proxyCmdMngr.extDialCmdEnabled =
                OSAL_atoi(value_ptr);
    }
#endif    
    /* Get the proxy terminal device name */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_PROXY,
            NULL, NULL, SETTINGS_PARM_TERMINAL))) {
        OSAL_snprintf(global_ptr->gsmObj.proxyTerminalName, GAPP_STRING_SZ,
                "%s", value_ptr);
    }

    /* Get SMS PDU type, TPDU, RPDU or plain text. Plain text is default */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SMS,
            NULL, NULL, SETTINGS_PARM_SMS_PDU_FMT))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_PDU_FMT_RPDU,
                sizeof(SETTINGS_PARM_VALUE_PDU_FMT_RPDU))) {
            global_ptr->gsmObj.proxyCmdMngr.sms.pduMode =
                    PRXY_SMS_PDU_MODE_RPDU;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_PDU_FMT_TPDU,
                sizeof(SETTINGS_PARM_VALUE_PDU_FMT_TPDU))) {
            global_ptr->gsmObj.proxyCmdMngr.sms.pduMode =
                    PRXY_SMS_PDU_MODE_TPDU;
        }
        else {
            /* Default is plain text. */
            global_ptr->gsmObj.proxyCmdMngr.sms.pduMode =
                    PRXY_SMS_PDU_MODE_TEXT;
        }
    }
    else {
        /* CSM will handle PDU defualt type is plain text. */
        global_ptr->gsmObj.proxyCmdMngr.sms.pduMode = PRXY_SMS_PDU_MODE_TEXT;
    }

    /* Get SMS Type, 3GPP or 3GPP2 */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SMS,
            NULL, NULL, SETTINGS_PARM_SMS_TYPE))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SMS_TYPE_3GPP,
                sizeof(SETTINGS_PARM_VALUE_SMS_TYPE_3GPP))) {
            global_ptr->gsmObj.proxyCmdMngr.sms.smsType = CSM_SMS_TYPE_PDU_3GPP;
        }
        else if (0 == OSAL_strncmp(value_ptr,
                SETTINGS_PARM_VALUE_SMS_TYPE_3GPP2,
                sizeof(SETTINGS_PARM_VALUE_SMS_TYPE_3GPP))) {
            global_ptr->gsmObj.proxyCmdMngr.sms.smsType =
                    CSM_SMS_TYPE_PDU_3GPP2;
        }
        else {
            global_ptr->gsmObj.proxyCmdMngr.sms.smsType = CSM_SMS_TYPE_PDU_3GPP;
        }
    }
    else {
        /* Default is 3GPP */
        global_ptr->gsmObj.proxyCmdMngr.sms.smsType = CSM_SMS_TYPE_PDU_3GPP;
    }
    /* Get emergency numbers */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_GAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_EMERGENCY,
            NULL, NULL, SETTINGS_PARM_EMERGENCY_NUMBER))) {
        /* start  to parse emergency nubmers */
        temp_ptr = OSAL_strtok(value_ptr,";");
        while (NULL != temp_ptr) {
            OSAL_strncpy(global_ptr->gsmObj.proxyCmdMngr.emergencyNumbers[index],
                temp_ptr, PRXY_EMERGENCY_NUMBER_STR_SZ + 1);
                GAPP_dbgPrintf("emergency number = %s\n",
                global_ptr->gsmObj.proxyCmdMngr.emergencyNumbers[index]);
            temp_ptr = OSAL_strtok (NULL, ";");            
            index++;
        }
    }

    /* Init all GAPP stuff */
    if (GAPP_OK != _GAPP_gsmInit(&global_ptr->gsmObj, ipc_ptr, isiIpc_ptr)) {
        GAPP_dbgPrintf("%s: Error initializing the GAPP task\n",
                    __FUNCTION__);
        return (GAPP_ERR);
    }
#ifndef GAPP_DISABLE_GSM    
    /* 
     * Init the timer event object. Set the IPC name of where to send events
     * i.e. ISI and also write the actual message to send.  We only do this 
     * once so we don't have to re-write the event everytime we send it.
     */
    if (0 == (global_ptr->gsmObj.tmr.event.isiEvtQId =
            OSAL_msgQCreate(isiIpc_ptr,
            OSAL_MODULE_GAPP, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
            GAPP_MAX_QUEUE_DEPTH,
            sizeof(ISIP_Message), 0))) {
        GAPP_dbgPrintf("%s: Error creating isi event queue\n", __FUNCTION__);
        return (GAPP_ERR);
    }
    /* Construct the ISI event that the timer will send */
    _GAPP_systemIsiEvt(ISIP_SYSTEM_REASON_START, ISIP_STATUS_TRYING,
            ipc_ptr, mediaIpc_ptr, streamIpc_ptr, 
            &global_ptr->gsmObj.tmr.event.isiMsg);
    
    /* Launch the timer used to register with ISI */
    if (GAPP_OK != _GAPP_timerInit(&global_ptr->gsmObj)) {
        /* Failed to init timer */
        _GAPP_gsmDestroy(&global_ptr->gsmObj);
        return (GAPP_ERR);
    }
#endif    
    return (GAPP_OK);
}

int GAPP_init(
    void *cfg_ptr)
{
    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);
 
    /* Init the global object that manages everything */
    OSAL_memSet(&_GAPP_globalObj, 0, sizeof(GAPP_GlobalObj));
    
    /* Let's start eveything !*/
    if (GAPP_OK != _GAPP_start(cfg_ptr, &_GAPP_globalObj)) {
        OSAL_logMsg("Failed to Init the GAPP Application\n");
        /* Free xml_ptr It came from heap when the file was parsed */
        OSAL_condApplicationExitUnregister();
        return (-1);
    }
    
    return (0);
}

void GAPP_shutdown(
    void)
{
    /* Notify ISI that we are dying */
    _GAPP_systemIsiEvt(ISIP_SYSTEM_REASON_SHUTDOWN, ISIP_STATUS_DONE, 
            NULL, NULL, NULL, &_GAPP_globalObj.gsmObj.event.isiMsg);
    
    GAPP_sendEvent(&_GAPP_globalObj.gsmObj.event);
    
    /* Clean house and exit */
    _GAPP_gsmDestroy(&_GAPP_globalObj.gsmObj);
#ifndef GAPP_DISABLE_GSM   
    /* Now kill the timer */
    _GAPP_timerDestroy(&_GAPP_globalObj.gsmObj);
#endif
    /* Delete ISI event queue */
    OSAL_msgQDelete(_GAPP_globalObj.gsmObj.tmr.event.isiEvtQId);
}
