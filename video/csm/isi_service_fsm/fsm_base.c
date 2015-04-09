/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29969 $ $Date: 2014-11-20 16:14:07 +0800 (Thu, 20 Nov 2014) $
 */
#include <osal.h>
#include <osal_types.h>
#include "_csm.h"
#include "_csm_utils.h"
#include "_csm_response.h"
#include "_csm_isi_service.h"
#include "_csm_utils.h"

#include "fsm.h"
#include "_fsm.h"

/*
 * String Array for print debug
 */
#ifdef CSM_DEBUG
static const char* _CSM_EVENT_STRING[] = {
    "FSM_EVENT_TYPE_NONE",
    "FSM_EVENT_TYPE_FAILURE",
    "FSM_EVENT_TYPE_NEW_IP",
    "FSM_EVENT_TYPE_ACTIVE",
    "FSM_EVENT_TYPE_ACTIVATING",
    "FSM_EVENT_TYPE_INACTIVE",
    "FSM_EVENT_TYPE_TIMER",
    "FSM_EVENT_TYPE_AUTH_FAIL",
    "FSM_EVENT_TYPE_START",
    "FSM_EVENT_TYPE_AKA_REQUIRED",
    "FSM_EVENT_TYPE_AKA_RESPONSE",
    "FSM_EVENT_TYPE_STOP",
    "FSM_EVENT_TYPE_LAST",
};
#endif

/*
 * ======== FSM_init ========
 * Initialize an instance of the state machine.
 *
 * Return Values:
 * FSM_Context - an initialize context for a new state machine.
 */
vint FSM_init(
    FSM_Context_Ptr  context_ptr,
    void            *serviceMngr_ptr,
    ISI_Id           serviceId,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_dbgPrintf("\n");

    context_ptr->csmOutput_ptr = csmOutput_ptr;
    context_ptr->currentState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE);
    context_ptr->serviceMngr_ptr = serviceMngr_ptr;
    context_ptr->serviceId = serviceId;
    return (CSM_OK);
}

/*
 * ======== FSM_destroy ========
 * Shutdown an instance of the state machine.
 *
 * Return Values:
 *    none
 */
void FSM_destroy(
    FSM_Context_Ptr context_ptr)
{
    return;
}

/*
 * ======== _FSM_toString ========
 * This function is a helper to get the state name string.
 *
 * Return Values:
 *    string with the current FSM state name
 */
const char *_FSM_toString(
    FSM_Context_Ptr context_ptr)
{
    return (context_ptr->currentState_ptr->stateName_ptr);
}

/*
 * ======== _FSM_isServiceIp ========
 * This function is a helper find out if the FSM is IP/IMS or not
 * Return Values:
 *     OSAL_TRUE: if service is IP based
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isServiceIp(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    if ((CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) ||
            (CSM_ISI_PROTOCOL_SIP_RCS == service_ptr->protocol)) {
        return (OSAL_TRUE);
    }
    return (OSAL_FALSE);
}

/*
 * ======== _FSM_isServiceReady() ========
 * This function is a helper find out if a service is ready to be activated.
 *
 * Return Values:
 *     OSAL_TRUE: The service is ready to be activated.
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isServiceReady(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr   *srvcMngr_ptr;
    CSM_IsiService    *service_ptr;
    CSM_IsiService    *masterSipService_ptr;
    RPM_FeatureType    featureType;
    RPM_RadioInterface radioInfc;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    if ((CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) ||
            (CSM_ISI_PROTOCOL_SIP_RCS == service_ptr->protocol)) {
        /* This service is sip */
        if (service_ptr->isEmergency) {
            featureType = RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE;
        }
        else {
            featureType = RPM_FEATURE_TYPE_IMS_SERVICE;
        }
        /* get ip address by RPM */
        RPM_getAvailableRadio(featureType, &radioInfc);
        /* If no ip address then no need to activate service now */
        if (OSAL_netIsAddrZero(&radioInfc.ipAddr)) {
            return (OSAL_FALSE);
        }

        if (service_ptr->isEmergency) {
            return (OSAL_TRUE);
        }
        else {
            /*
             * If it's slave sip service and master is not active then don't
             * activate slave service now.
             */
            masterSipService_ptr = CSM_isiGetMasterSipService(
                    srvcMngr_ptr->isiMngr_ptr);
            if (NULL == masterSipService_ptr) {
                /* No master service found */
                return (OSAL_FALSE);
            }

            if ((service_ptr == masterSipService_ptr) ||
                _FSM_isActive(&masterSipService_ptr->fsm)) {
                /* This is master sip service */
                return (OSAL_TRUE);
            }
            /* Otherwise this is slave service but master is not active */
            return (OSAL_FALSE);
        }
    }

    /* Return true for CS service */
    return (OSAL_TRUE);
}

/*
 * ======== _FSM_isIpAvailable() ========
 * This function is a helper find out if there is ip address available
 * for specific service.
 *
 * Return Values:
 *     OSAL_TRUE: There is ip address available
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isIpAvailable(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr   *srvcMngr_ptr;
    CSM_IsiService    *service_ptr;
    RPM_FeatureType    featureType;
    RPM_RadioInterface radioInfc;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    if ((CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) ||
            (CSM_ISI_PROTOCOL_SIP_RCS == service_ptr->protocol)) {
        if (service_ptr->isEmergency) {
            featureType = RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE;
        }
        else {
            featureType = RPM_FEATURE_TYPE_IMS_SERVICE;
        }
        /* get ip address by RPM */
        RPM_getAvailableRadio(featureType, &radioInfc);
        return (OSAL_netIsAddrZero(&radioInfc.ipAddr));
    }

    /* Return true for CS service */
    return (OSAL_TRUE);
}

/*
 * ======== _FSM_isServiceLteIp ========
 * This function is a helper find out if the FSM is IP/IMS over LTE or not
 *
 * Return Values:
 *     OSAL_TRUE: if service is IP based
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isServiceLteIp(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    if ((CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) &&
            (!service_ptr->isEmergency)) {
        return (OSAL_TRUE);
    }
    else {
        return (OSAL_FALSE);
    }
}

/*
 * ======== _FSM_isServiceRcsIp ========
 * This function is a helper find out if the FSM is IP/IMS in application
 * processor(ASAPP) or not.
 *
 * Return Values:
 *     OSAL_TRUE: if service is IP based
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isServiceRcsIp(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    if (CSM_ISI_PROTOCOL_SIP_RCS == service_ptr->protocol) {
        return (OSAL_TRUE);
    }
    else {
        return (OSAL_FALSE);
    }
}

/*
 * ======== _FSM_isServiceEmergencyIp ========
 * This function is a helper find out if the FSM is emergency IP/IMS or not
 *
 * Return Values:
 *     OSAL_TRUE: if service is IP based
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isServiceEmergencyIp(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    if ((CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) &&
            (service_ptr->isEmergency)) {
        return (OSAL_TRUE);
    }
    else {
        return (OSAL_FALSE);
    }
}

/*
 * ======== _FSM_isServiceMasterIp ========
 * This function is a helper find out if the service is master sip service, i.e.
 * the sip service with registration.
 *
 * Return Values:
 *     OSAL_TRUE: if service is master sip service.
 *     OSAL_FALSE: otherwise
 */
OSAL_Boolean _FSM_isServiceMasterIp(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    return ((OSAL_Boolean)service_ptr->isMasterSip);
}

/*
 * ======== FSM_processEvent ========
 * Process the incoming event. This function handles any pre or post event
 * processing. In addition, it calls the process event function associated
 * with the current state.
 *
 * Return Values:
 * none
 */
void FSM_processEvent(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    /* Protect process event before FSM initialized, igore the event */
    if (NULL == context_ptr->currentState_ptr) {
        CSM_dbgPrintf("FSM is not initialized\n");
        return;
    }

    if (event_ptr->eventType < FSM_EVENT_TYPE_LAST) {
        CSM_dbgPrintf("event:%s\n", _CSM_EVENT_STRING[event_ptr->eventType]);
    }
    else {
        OSAL_logMsg("Unknown eventType:%d\n", event_ptr->eventType);
    }

    CSM_dbgPrintf("Service Id: %d, Current State: %s\n",
            context_ptr->serviceId, _FSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->processEvent != NULL) {
        context_ptr->currentState_ptr->processEvent(context_ptr, event_ptr);
    }
}

/*
 * ======== _FSM_stateEnter ========
 * Enter the new state. If a function is available that is run when entering
 * the state, it is run by this function. Any common code that is run before
 * or after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _FSM_stateEnter(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    CSM_dbgPrintf("Service Id: %d, entering state %s\n",
            context_ptr->serviceId, _FSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateEnter != NULL) {
        context_ptr->currentState_ptr->stateEnter(context_ptr, event_ptr);
    }
}

/*
 * ======== _FSM_stateExit ========
 * Exit the current state. If a function is available that is run when exiting
 * the state, it is run by this function. Any common code that is run before or
 * after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _FSM_stateExit(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    CSM_dbgPrintf("Service Id: %d, exiting state %s\n",
            context_ptr->serviceId, _FSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateExit != NULL) {
        context_ptr->currentState_ptr->stateExit(context_ptr, event_ptr);
    }
}

/*
 * ======== _FSM_setState ========
 * This function is used by to change the state of the state machine. It first
 * checks to see if a change occurred. If it did, it runs the state exit
 * function associated with the current state. Then it changes the state and
 * runs the state enter function of the new state.
 *
 * Return Values:
 * none
 */
void _FSM_setState(
    FSM_Context_Ptr context_ptr,
    FSM_State_Ptr   nextState_ptr,
    FSM_Event_Ptr   event_ptr)
{
    if ((NULL != nextState_ptr) &&
            (context_ptr->currentState_ptr != nextState_ptr)) {
        _FSM_stateExit(context_ptr, event_ptr);
        context_ptr->currentState_ptr = nextState_ptr;
        _FSM_stateEnter(context_ptr, event_ptr);
    }
}

/*
 * ======== _FSM_isActive ========
 * This function is a private helper for checking if FSM is in
 * active state or not
 *
 * Return Values:
 *    OSAL_TRUE if FSM in Active State
 *    OSAL_FALSE otherwise
 */
OSAL_Boolean _FSM_isActive(FSM_Context_Ptr context_ptr)
{
    if (&_FSM_ACTIVE_STATE == context_ptr->currentState_ptr) {
        return OSAL_TRUE;
    }
    return OSAL_FALSE;
}

/*
 * ======== FSM_getEventTypeFromReason ========
 * This function is a helper for constructing an event type from a reason code.
 *
 */
void FSM_getEventTypeFromReason(
    const char   *reason_ptr,
    FSM_Event_Ptr event_ptr)
{
    vint code = CSM_utilGetReasonCode(reason_ptr);
    switch (code) {
        case CSM_UTILS_RETURN_INVALID_CODE:
            event_ptr->eventType = FSM_EVENT_TYPE_INACTIVE;
            event_ptr->errorCode = 0;
            break;
        //case 0:
        //    return FSM_NO_NET;
        case 401:
        case 407:
        case 403:
        case 404:
        case 402:
            event_ptr->eventType = FSM_EVENT_TYPE_AUTH_FAIL;
            event_ptr->errorCode = code;
            break;
        default:
            event_ptr->eventType = FSM_EVENT_TYPE_FAILURE;
            event_ptr->errorCode = code;
            break;
    }
    return;
}

/*
 * ======== _FSM_sendServiceStateEvent() ========
 * This function is a helper sending service state output events
 *
 * Return Values:
 *    none
 */
void _FSM_sendServiceStateEvent(
    CSM_ServiceState state,
    vint             errorCode,
    vint             isEmergency,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_OutputService *serviceEvt_ptr;

    /* Construct and send output event */
    csmOutput_ptr->type    = CSM_EVENT_TYPE_SERVICE;
    serviceEvt_ptr         = &csmOutput_ptr->evt.service;
    serviceEvt_ptr->reason = CSM_OUTPUT_REASON_SERVICE_STATE;
    serviceEvt_ptr->state  = state;
    serviceEvt_ptr->errorCode  = errorCode;
    serviceEvt_ptr->isEmergency  = isEmergency;
    serviceEvt_ptr->reasonDesc[0] = '\0';

    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== _FSM_sendServiceNotification() ========
 * This function is a helper sending service notification output event.
 *
 * Return Values:
 *    none
 */
void _FSM_sendServiceNotification(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    CSM_OutputEvent   *csmOutput_ptr;
    CSM_OutputService *serviceEvt_ptr;
    char              *uri_ptr;
    uvint               size;

    /* Check if any notificaiton needs to be reported. */
    if (CSM_ERR != _CSM_utilGetValue(event_ptr->reasonDesc,
            CSM_ISI_ALIAS_URI_STR, &uri_ptr, &size)) {
        /* Then we got alias URI. */
        csmOutput_ptr = context_ptr->csmOutput_ptr;
        /* Construct and send output event */
        csmOutput_ptr->type    = CSM_EVENT_TYPE_SERVICE;
        serviceEvt_ptr         = &csmOutput_ptr->evt.service;
        serviceEvt_ptr->reason = CSM_OUTPUT_REASON_SERVICE_NOTIFY;
        serviceEvt_ptr->u.notify.type = CSM_SERVICE_NOTIFY_ALIAS_URI;
        if (size > (sizeof(serviceEvt_ptr->reasonDesc) - 1)) {
            /* URI trancated */
            CSM_dbgPrintf("Alias URI size larger then reasonDesc.\n");
            return;
        }
        /* Plus 1 null termination. */
        OSAL_strncpy(serviceEvt_ptr->reasonDesc, uri_ptr, size + 1);

        CSM_sendOutputEvent(csmOutput_ptr);
    }
    /* Add more notification if neede. */
    else {
        /* Nothing to notify. */
        return;
    }
    return;
}

/*
 * ======== _FSM_sendServiceStateEventToCall() ========
 * This function is a helper sending service state to call manager
 *
 * Return Values:
 *    none
 */
void _FSM_sendServiceStateEventToCall(
    FSM_Context_Ptr context_ptr,
    FSM_EventType   eventType)
{
    CSM_PrivateInputEvt    *event_ptr;

    event_ptr = &((CSM_ServiceMngr *)context_ptr->serviceMngr_ptr)->
            isiMngr_ptr->csmInputEvent;
    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_CALL;
    if (FSM_EVENT_TYPE_ACTIVE == eventType) {
        event_ptr->evt.call.reason = CSM_CALL_REASON_EVT_SERVICE_ACTIVE;
    }
    else {
        event_ptr->evt.call.reason = CSM_CALL_REASON_EVT_SERVICE_INACTIVE;
    }
    event_ptr->evt.call.id = 0;
    event_ptr->evt.call.serviceId = context_ptr->serviceId;
    event_ptr->evt.call.type = CSM_CALL_EVENT_TYPE_ISI;

    CSM_isiSendEvent(((CSM_ServiceMngr *)
            context_ptr->serviceMngr_ptr)->isiMngr_ptr, event_ptr);
}
/*
 * ======== _FSM_sendErrorEvent() ========
 * This function is a helper sending error events
 *
 * Return Values:
 *    none
 */
void _FSM_sendErrorEvent(
    FSM_EventType    eventType,
    char            *reasonDesc_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    int errorCode;
    if (FSM_EVENT_TYPE_FAILURE != eventType &&
            FSM_EVENT_TYPE_AUTH_FAIL != eventType) {
        return;
    }
    /* Go to error state from a Failure eventType. We report this error.*/
    errorCode = CSM_utilGetReasonCode(reasonDesc_ptr);
    /* Skip 401 unauthorized error */
    if (errorCode == CSM_ERROR_CODE_SIP_UNAUTH) {
        return;
    }
    if ((CSM_ERROR_CODE_NO_NETWORK_AVAILABLE == errorCode) ||
            (CSM_ERROR_CODE_XPORT_INIT_FAILURE == errorCode) ||
            (errorCode >= CSM_ERROR_CODE_SIP_START && 
            errorCode <= CSM_ERROR_CODE_SIP_END)) {
        CSM_sendServiceError(errorCode, reasonDesc_ptr, csmOutput_ptr);
    }
}


