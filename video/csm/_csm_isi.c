/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29738 $ $Date: 2014-11-10 14:31:33 +0800 (Mon, 10 Nov 2014) $
 */

#include <osal.h>
#include <csm_event.h>
#include <isi.h>
#include <rpm.h>
#include "_csm_print.h"
#include "_csm_isi.h"
#include "_csm.h"
#include "_csm_utils.h"
#include "_csm_isi_service.h"
#include "_csm_isi_tel.h"
#include "_csm_isi_ussd.h"
#include <isi_rpc.h>

/* 
 * CSM ISI Manager private methods
 */

/*
 * ======== _CSM_isiNoneTypeEventHandler() ========
 *
 * Private helper function which processes ISI "none" type message.  These 
 * events are typically related to protocol registration / de-registration
 *
 * Returns: 
 *      none
 */
OSAL_INLINE void _CSM_isiNoneTypeEventHandler(
    CSM_IsiMngr *isiMngr_ptr,
    int          evt, 
    int          protocolId) 
{
    CSM_PrivateInputEvt *event_ptr = &isiMngr_ptr->csmInputEvent;
    vint reason;
    int  x;

    CSM_dbgPrintf("\n");

    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_SERVICE;

    // Then it's a protocol event
    reason = -1;
    for (x = 0 ; x < CSM_ISI_NUM_SERVICES ; x++) {
        /* Don't send PROTOCOL_REGISTERED twice for sip emergency service */
        if (isiMngr_ptr->service[x].protocol == protocolId) {
            if (ISI_EVENT_PROTOCOL_READY == evt) {
                isiMngr_ptr->service[x].isRegistered = 1;
                reason = CSM_SERVICE_REASON_PROTOCOL_REGISTERED;
            }
            else if (ISI_EVENT_PROTOCOL_FAILED == evt) {
                isiMngr_ptr->service[x].isRegistered = 0;
                reason = CSM_SERVICE_REASON_PROTOCOL_DIED;
            }
        }
    }

    /* Send event */
    if (-1 != reason) {
        event_ptr->evt.service.protocol = protocolId;
        event_ptr->evt.service.reason = (CSM_IsiServiceReason)reason;
        /* Notify Account package */
        CSM_isiSendEvent(isiMngr_ptr, event_ptr);
    }
    return;
}

/*
 * ======== _CSM_isiProcessEvent() ========
 *
 * Private helper function which processes any new event from ISI
 *
 * Returns: 
 *      none
 */
void _CSM_isiProcessEvent(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId,
    ISI_Id       id,
    ISI_IdType   idType,
    ISI_Event    event,
    char        *desc_ptr)
{
    /* Switch on the event type and pass to the appropriate event handler */ 
    switch (idType) {
        case ISI_ID_TYPE_NONE:
            _CSM_isiNoneTypeEventHandler(isiMngr_ptr, event, id);
            break;

        case ISI_ID_TYPE_SERVICE:
            _CSM_isiServiceTypeEventHandler(isiMngr_ptr, serviceId, event, 
                    desc_ptr);
            break;

        case ISI_ID_TYPE_CALL:
            _CSM_isiCallTypeEventHandler(isiMngr_ptr, serviceId, id, event, 
                    desc_ptr);
            break;

        case ISI_ID_TYPE_PRESENCE:
            _CSM_isiCallPresenceTypeEventHandler(isiMngr_ptr, serviceId, id, 
                    event, desc_ptr);
            break;

        case ISI_ID_TYPE_TEL_EVENT:
            _CSM_isiTelTypeEventHandler(isiMngr_ptr, serviceId, id, event, 
                    desc_ptr);
            break;

        case ISI_ID_TYPE_MESSAGE:
            _CSM_isiSmsTypeEventHandler(isiMngr_ptr, serviceId, id, event, 
                    desc_ptr);
            break;

        case ISI_ID_TYPE_USSD:
            _CSM_isiUssdTypeEventHandler(isiMngr_ptr, serviceId, id, event, 
                    desc_ptr);
            break;

        case ISI_ID_TYPE_CHAT:
        case ISI_ID_TYPE_FILE:
        default:
            CSM_dbgPrintf("event not processed in vPort4G\n");
            break;
    } // End of switch
}

/* 
 * ======== _CSM_isiThread() ========
 *
 * This function is the entry point for a thread that handles events from ISI.
 * This thread waits (blocks) on ISI and processes events as they are received.
 *
 * Returns: 
 *   Nothing and never.
 */
static OSAL_TaskReturn _CSM_isiThread(
    OSAL_TaskArg arg_ptr)
{
    CSM_IsiMngr *isiMngr_ptr;
    ISI_Id       serviceId;
    ISI_Id       id;
    ISI_IdType   type;
    ISI_Event    evt;
    ISI_Return   ret;
    char         eventDesc[ISI_EVENT_DESC_STRING_SZ + 1];
 
    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);
    
    /* Cast arg to ISI Manager */
    isiMngr_ptr = (CSM_IsiMngr *)arg_ptr;

    while (1) {
        /*
         * Give a timeout value is necessary for vport 4G+ with RCS
         * provisioning enabled, otherwise ISI Server might be blocked
         * in ISI_getEvent().
         */
        ret = ISI_serverGetEvent(&serviceId, &id, &type, &evt, eventDesc, OSAL_WAIT_FOREVER);        
        if (ret == ISI_RETURN_OK) {
            /* Then the app has an event from ISI */
            /* Send some log message to the gui */
            CSM_isiPrintEvent(serviceId, id, type, evt, eventDesc);
            /* Process the event */
            _CSM_isiProcessEvent(isiMngr_ptr, serviceId, id, type, evt, 
                    eventDesc);
        }
        else if (ret == ISI_RETURN_NOT_INIT) {
            /* Then ISI is destroyed, let's bail out. */
            return (0);
        }
        else if (ret == ISI_RETURN_DONE) {
            /* It's a event routes to ISI client. Get next event then. */
        }
        else if (ret == ISI_RETURN_TIMEOUT) {
            /* Timeout */
        }
        else {
            OSAL_logMsg("%s:%d Error !ISI_RETURN_OK\n", __FUNCTION__, __LINE__);
            OSAL_taskDelay(1);
        }
    }
}

/* 
 * CSM ISI Manager package public methods 
 */

/*
 * ======== CSM_isiAllocate() ========
 *
 * Public routine for allocating the CSM ISI module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint CSM_isiAllocate(
    CSM_IsiMngr *isiMngr_ptr,
    OSAL_MsgQId  qId)
{
    vint       i; /* Service index */

    CSM_dbgPrintf("\n");

    /* Store event Q id for talking to CSM worker thread */
    isiMngr_ptr->eventQ = qId;

    i = 0;
    /* Init ISI Services */
    isiMngr_ptr->service[i].protocol = CSM_ISI_PROTOCOL_MODEM_IMS;
    isiMngr_ptr->service[i].protoName = CSM_ISI_PROTOCOL_MODEM_IMS_NAME;
    isiMngr_ptr->service[i].isEmergency = 0;
    isiMngr_ptr->service[i].isInitialized = OSAL_FALSE;
    isiMngr_ptr->service[i].rtMedia = CSM_TRASPORT_PROTO_NONE;
    i++;

    /* No need GSM and emergency service for Seattle */
    if (CSM_ISI_PROTOCOL_MODEM_IMS == CSM_ISI_PROTOCOL_SIP) {
        isiMngr_ptr->service[i].protocol = CSM_ISI_PROTOCOL_GSM;
        isiMngr_ptr->service[i].protoName = CSM_ISI_PROTOCOL_GSM_NAME;
        isiMngr_ptr->service[i].isEmergency = 0;
        isiMngr_ptr->service[i].isInitialized = OSAL_FALSE;
        isiMngr_ptr->service[i].rtMedia = CSM_TRASPORT_PROTO_NONE;
        i++;

        /* SIP emergency service */
        isiMngr_ptr->service[i].protocol = CSM_ISI_PROTOCOL_SIP;
        isiMngr_ptr->service[i].protoName = CSM_ISI_PROTOCOL_SIP_NAME;
        isiMngr_ptr->service[i].isEmergency = 1;
        isiMngr_ptr->service[i].isInitialized = OSAL_FALSE;
        isiMngr_ptr->service[i].rtMedia = CSM_TRASPORT_PROTO_NONE;
        i++;
    }

    /* SIP RCS, i.e ASAPP */
    isiMngr_ptr->service[i].protocol = CSM_ISI_PROTOCOL_SIP_RCS;
    isiMngr_ptr->service[i].protoName = CSM_ISI_PROTOCOL_SIP_RCS_NAME;
    isiMngr_ptr->service[i].isEmergency = 0;
    isiMngr_ptr->service[i].isInitialized = OSAL_FALSE;
    isiMngr_ptr->service[i].rtMedia = CSM_TRASPORT_PROTO_NONE;
    i++;

    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_isiStart() ========
 *
 * Public routine for starting the CSM ISI module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint CSM_isiStart(
     CSM_IsiMngr *isiMngr_ptr)
{
    /* Create ISI_Thread */
    if (0 == (isiMngr_ptr->task = OSAL_taskCreate(CSM_ISI_EVENT_THREAD_NAME,
            OSAL_TASK_PRIO_NRT, CSM_ISI_TASK_STACK_SZ, (OSAL_TaskPtr)_CSM_isiThread,
            (void *)isiMngr_ptr))) {
        OSAL_logMsg("Error starting ISI thread\n");
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_isiInit() ========
 *
 * Public routine for initializing the ISI manager package.
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_isiInit(
    CSM_IsiMngr *isiMngr_ptr,
    OSAL_MsgQId  qId)
{
    ISI_Return r;

    if (OSAL_FAIL == CSM_isiAllocate(isiMngr_ptr, qId)) {
        return (CSM_ERR);
    }

    /* Init ISI native */
    r = ISI_init(NULL);
    if (ISI_RETURN_OK != r) {
        OSAL_logMsg("The ISI module could not be initialized ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (OSAL_FAIL);
    }

    /* Init ISI server */
    if (OSAL_SUCCESS != ISI_serverInit()) {
        OSAL_logMsg("%s:%d ERROR initial ISI Server\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    if (OSAL_FAIL == CSM_isiStart(isiMngr_ptr)) {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== CSM_isiSendEvent() ========
 *
 * Public routine for sending CSM commands from ISI thread to CSM worker /
 * looper thread
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_isiSendEvent(
    CSM_IsiMngr         *isiMngr_ptr,
    CSM_PrivateInputEvt *event_ptr)
{
    /* Send the event to CSM */
    if (OSAL_SUCCESS != OSAL_msgQSend(isiMngr_ptr->eventQ, (char *)event_ptr,
            sizeof(CSM_PrivateInputEvt), OSAL_WAIT_FOREVER, NULL)) {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_isiDeallocate() ========
 *
 * Internal routine for free up the CSM ISI module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _CSM_isiDeallocate(
     CSM_IsiMngr *isiMngr_ptr)
{
    return (OSAL_SUCCESS);
}

/*
 * ======== _CSM_isiStop() ========
 *
 * Internal routine for stoping the CSM ISI module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _CSM_isiStop(
     CSM_IsiMngr *isiMngr_ptr)
{
    /* Kill ISI event thread */
    OSAL_taskDelete(isiMngr_ptr->task);

    return (OSAL_SUCCESS);
}

/*
 * ======== CSM_isiDestroy() ========
 *
 * Public routine for destroy the CSM ISI module.
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_isiDestroy(
    CSM_IsiMngr *isiMngr_ptr)
{
    CSM_dbgPrintf("CSM_isiShutdown\n");

    _CSM_isiDeallocate(isiMngr_ptr);
    _CSM_isiStop(isiMngr_ptr);

    return (CSM_OK);
}

/*
 * ======== CSM_isiShutdown() ========
 *
 * Public routine for shutting down the ISI manager package.
 *
 * Returns:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint CSM_isiShutdown(
    CSM_IsiMngr *isiMngr_ptr)
{
    CSM_dbgPrintf("CSM_isiShutdown\n");

    CSM_isiDestroy(isiMngr_ptr);

    /* Shutdown ISI Server, xxx _deallocate and _stop */
    ISI_serverShutdown();

    /* Shutdown ISI native */
    ISI_shutdown();

    return (CSM_OK);
}

/* 
 * ======== CSM_isiGetServiceViaId() ========
 *
 * Public routine for getting the CSM ISI Service for a given service ID
 *
 * Returns: 
 *      CSM_IsiService*, if successful
 *      NULL, if not found
 */
CSM_IsiService* CSM_isiGetServiceViaId(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id serviceId)
{
    vint x;
    for (x = 0 ; x < CSM_ISI_NUM_SERVICES ; x++) {
        if (isiMngr_ptr->service[x].serviceId == serviceId) {
            return &isiMngr_ptr->service[x];
        }
    }
    return NULL;
}

/* 
 * ======== CSM_isiGetServiceViaProtocol() ========
 *
 * Public routine for getting the CSM ISI Service for a given protocol ID
 *
 * Returns: 
 *      CSM_IsiService*, if succesfull
 *      NULL, if not found
 */
CSM_IsiService* CSM_isiGetServiceViaProtocol(
    CSM_IsiMngr *isiMngr_ptr,
    vint protocol,
    vint isEmergency)
{
    vint x;
    for (x = 0 ; x < CSM_ISI_NUM_SERVICES ; x++) {
        if ((isiMngr_ptr->service[x].protocol == protocol) &&
                (isiMngr_ptr->service[x].isEmergency == isEmergency)) {
            return &isiMngr_ptr->service[x];
        }
    }
    return NULL;
}

/* 
 * ======== CSM_isiNormalizeOutboundAddress() ========
 *
 * Public routine for getting normalizing an outbound address
 *
 * Returns: 
 *      CSM_IsiService*, if a service is found for call/send
 *      NULL, if not found
 */
CSM_IsiService* CSM_isiNormalizeOutboundAddress(
    CSM_IsiMngr         *isiMngr_ptr,
    char const          *address_ptr,
    char                *out_ptr,
    vint                 maxOutLen,
    RPM_FeatureType      callType)
{
    char                normalizedAddress[CSM_EVENT_STRING_SZ + 1];
    RPM_RadioInterface  radioInfc;
    CSM_IsiService     *service_ptr;

    CSM_dbgPrintf("\n");

    /* Ask RPM what the active radio interface it */
    RPM_getActiveRadio(callType, &radioInfc);

    /* Get the ISI service for the correct outbound radio choice */
    if ((RPM_RADIO_TYPE_LTE == radioInfc.radioType) && 
        (OSAL_FALSE == OSAL_netIsAddrZero(&radioInfc.ipAddr))) {
        service_ptr = CSM_isiGetServiceViaProtocol(isiMngr_ptr,
                CSM_ISI_PROTOCOL_MODEM_IMS, 0);
        CSM_dbgPrintf("RPM says use IMS\n");
    }
    else if ((RPM_RADIO_TYPE_LTE_EMERGENCY == radioInfc.radioType) &&
        (OSAL_FALSE == OSAL_netIsAddrZero(&radioInfc.ipAddr))) {
        service_ptr = CSM_isiGetServiceViaProtocol(isiMngr_ptr,
                CSM_ISI_PROTOCOL_MODEM_IMS, 1);
        CSM_dbgPrintf("RPM says use emergency IMS\n");
    }
    else if ((RPM_RADIO_TYPE_WIFI == radioInfc.radioType) &&
        (OSAL_FALSE == OSAL_netIsAddrZero(&radioInfc.ipAddr))) {
        service_ptr = CSM_isiGetServiceViaProtocol(isiMngr_ptr,
                CSM_ISI_PROTOCOL_SIP_RCS, 0);
        CSM_dbgPrintf("RPM says use IMS over wifi.\n");
    }
    else {
        service_ptr = CSM_isiGetServiceViaProtocol(isiMngr_ptr,
                CSM_ISI_PROTOCOL_GSM, 0);
        CSM_dbgPrintf("RPM says use CS\n");
    }

    /* Normalize the address */
    if (RPM_RETURN_OK == RPM_normalizeOutboundAddress(service_ptr->realm,
            address_ptr, normalizedAddress, CSM_EVENT_STRING_SZ, callType)) {
        /* Then let's re-write the address. */
        OSAL_snprintf(out_ptr, maxOutLen, "%s", normalizedAddress);
    }
    else {
        OSAL_snprintf(out_ptr, maxOutLen, "%s", address_ptr);
        OSAL_logMsg("%s:%d Failed to normalize address...trying address:%s",
                __FUNCTION__, __LINE__, address_ptr);
    }
 
    return (service_ptr);
}

/* 
 * ======== CSM_isiNormalizeInboundAddress() ========
 *
 * Public routine for getting normalizing an inbound address
 *
 * Returns: 
 *      none
 */
void CSM_isiNormalizeInboundAddress(
    CSM_IsiService      *service_ptr,
    char const          *address_ptr,
    char                *outAddress_ptr,
    vint                 maxOutAddressLen,
    CSM_CallAddressType *outAddressType_ptr,
    char                *outAlpha_ptr,
    vint                 maxOutAlphaLen)
{
    RPM_RadioType radioType;

    CSM_dbgPrintf("address:%s\n", address_ptr);

    if (CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) {
        radioType = RPM_RADIO_TYPE_LTE;
    }
    else {
        radioType = RPM_RADIO_TYPE_CS;
    }
    /* Use RPM normalization routine */
    if (RPM_RETURN_OK != RPM_normalizeInboundAddress(radioType,
            service_ptr->realm, address_ptr, outAddress_ptr, maxOutAddressLen, 
            outAlpha_ptr, maxOutAlphaLen)) {
        /* set the address to what ever we got. */
        OSAL_strncpy(outAddress_ptr, address_ptr, maxOutAddressLen);
        OSAL_logMsg("%s:%d Failed to normalize address...trying address:%s",
                __FUNCTION__, __LINE__, address_ptr);
        *outAlpha_ptr = 0;
    }
    /* Set the address type if a '+' is present */
    if (outAddress_ptr[0] == '+') {
        *outAddressType_ptr = CSM_CALL_ADDRESS_INTERNATIONAL;
    }
    else {
        *outAddressType_ptr = CSM_CALL_ADDRESS_NATIONAL;
    }
}

/* 
 * ======== CSM_isiProtoNameViaServiceId() ========
 *
 * Public routine for getting the ISIP service name for a given service ID
 *
 * Returns:
 *      char*, buffer to name if found
 *      "Unknown", if not found
 */
const char* CSM_isiProtoNameViaServiceId(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId)
{
    CSM_IsiService *s_ptr;
    s_ptr = CSM_isiGetServiceViaId(isiMngr_ptr, serviceId);
    if (NULL != s_ptr) {
        return (s_ptr->protoName);
    }
    return ("Unknown");
}

/* 
 * ======== CSM_isiGetMasterSipService() ========
 *
 * Public helper routine for getting master sip service.
 *
 * Returns: 
 *      CSM_IsiService*, if successful
 *      NULL, if not found
 */
CSM_IsiService* CSM_isiGetMasterSipService(
    CSM_IsiMngr *isiMngr_ptr)
{
    vint x;
    for (x = 0 ; x < CSM_ISI_NUM_SERVICES ; x++) {
        if (isiMngr_ptr->service[x].isMasterSip) {
            CSM_dbgPrintf("Master service id:%d, protocol:%d\n",
                    isiMngr_ptr->service[x].serviceId,
                    isiMngr_ptr->service[x].protocol);
            return &isiMngr_ptr->service[x];
        }
    }
    return NULL;
}

/*
 * ======== CSM_isiGetServiceIsActive() ========
 *
 * Public function to check if the service is actived.
 *
 * Returns:
 *   OSAL_TRUE: service is active.
 *   OSAL_FALSE: service is not active
 */
OSAL_Boolean CSM_isiGetServiceIsActive(
    CSM_IsiService *service_ptr)
{
    return (CSM_isiServiceIsActive(service_ptr));
}

