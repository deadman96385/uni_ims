/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 22287 $ $Date: 2013-10-03 11:30:14 +0800 (Thu, 03 Oct 2013) $
 */
#include "_csm.h"
#include "_csm_service.h"
#include "_csm_isi_service.h"
#include "fsm.h"
#include "_fsm.h"
#include "_csm_response.h"

/*
 * The State name
 */
static const char _stateName[] = "Service:Error";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    FSM_State_Ptr nextState_ptr = NULL;
    RPM_RadioInterface radioInfc;
    RPM_FeatureType    featureType;  
    CSM_IsiService    *service_ptr;     

    switch (event_ptr->eventType) {
        case FSM_EVENT_TYPE_STOP:
            /* Get the service object for this service ID */
            service_ptr = CSM_isiGetServiceViaId(((CSM_ServiceMngr *)
                    context_ptr->serviceMngr_ptr)->isiMngr_ptr,
                    context_ptr->serviceId);
            if (CSM_ISI_PROTOCOL_GSM != service_ptr->protocol) {
                /* If protocol is not GSM, need to set interface address as 0 */
                if (service_ptr->isEmergency) {
                    featureType = RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE;
                }
                else {
                     featureType = RPM_FEATURE_TYPE_IMS_SERVICE;
                }
                /* get ip address by RPM */
                RPM_getAvailableRadio(featureType, &radioInfc); 
                if (OSAL_netIsAddrIpv6(&radioInfc.ipAddr)) {
                    OSAL_memSet(radioInfc.ipAddr.ipv6, 0,
                            sizeof(radioInfc.ipAddr.ipv6));
                }
                else {
                    radioInfc.ipAddr.ipv4 = 0;
                }
                _CSM_isiServiceProcessIpChange(context_ptr->serviceMngr_ptr,
                            radioInfc.radioType, &radioInfc);
            }
            nextState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE);
            break;
        case FSM_EVENT_TYPE_START:
            /* Got account info from ISIM */
            if (CSM_OK != _CSM_isiServiceSetup(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId)) {
                /* Keep in the same state */
                break;
            }
            /* Fall through */
        case FSM_EVENT_TYPE_NEW_IP:
        case FSM_EVENT_TYPE_TIMER:
            if (!_FSM_isServiceReady(context_ptr)) {
                /*
                 * There is no available IP can be used or it's slave sip service
                 * and master sip service is not active now, so stay in reset state
                 * and does not need to activate this service.
                 */
                break;
            }
            if (OSAL_TRUE != ((CSM_ServiceMngr *) context_ptr->
                    serviceMngr_ptr)->isImsEnabled) {
                break;
            }
            /* Log in. */
            if (CSM_OK == _CSM_isiServiceActivate(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId)) {
                nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
            }
            /* Else then do nothing, wait for the timer to kick again. */
            break;
        case FSM_EVENT_TYPE_ACTIVE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ACTIVE_STATE);
            break;
        case FSM_EVENT_TYPE_AKA_REQUIRED:
            _CSM_isiServiceSendAkaChallenge(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId, context_ptr->csmOutput_ptr);
            /* AKA auth required, stay in login state for next response */
            nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
            break;
        case FSM_EVENT_TYPE_AKA_RESPONSE:
            _CSM_isiServiceSetAkaResponse(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId);
            /* AKA auth response, stay in login state for next response */
            nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
            break;
        case FSM_EVENT_TYPE_FAILURE:
        case FSM_EVENT_TYPE_AUTH_FAIL:
        default:
            break;
    }
    _FSM_setState(context_ptr, nextState_ptr, event_ptr);
}

/*
 * ======== _stateEnter ========
 * Perform actions needed when entering Wifi state.
 */
static void _stateEnter(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    CSM_IsiService  *service_ptr;
    CSM_ServiceMngr *srvcMngr_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;
    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
             context_ptr->serviceId);
    /*
     * Don't update state if the service is slave sip service, i.e. the one
     * doesn't do registration.
     */
    if (OSAL_TRUE == _FSM_isServiceMasterIp(context_ptr)) {
        /* Send the IMS registration state to GAPP */
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_FAILED,
                event_ptr->errorCode, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
        _FSM_sendErrorEvent(event_ptr->eventType, event_ptr->reasonDesc,
                context_ptr->csmOutput_ptr);
    }
    else if (_FSM_isServiceEmergencyIp(context_ptr)) {
        /* Send the IMS emergency registration state to GAPP */
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_FAILED,
                event_ptr->errorCode, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
    }
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const FSM_State _FSM_ERROR_STATE = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    NULL,          /* Nothing to do when exiting state */
    _stateName     /* The name of this state */
};
