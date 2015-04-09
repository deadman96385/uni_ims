/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 27294 $ $Date: 2014-07-07 13:38:14 +0800 (Mon, 07 Jul 2014) $
 */
#include "_csm.h"
#include "_csm_service.h"
#include "_csm_isi_service.h"
#include "fsm.h"
#include "_fsm.h"
#include "_csm_response.h"

static const char _stateName[] = "Service:Login";

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
        case FSM_EVENT_TYPE_INACTIVE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
            break;
        case FSM_EVENT_TYPE_FAILURE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
            break;
        case FSM_EVENT_TYPE_STOP:
            if (OSAL_TRUE == ((CSM_ServiceMngr *)
                    context_ptr->serviceMngr_ptr)->isImsEnabled) {
                break;
            }

            /* Set interface address as 0 to cancel registration retry. */
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
        case FSM_EVENT_TYPE_NEW_IP:
            /* then goto reset. */
            nextState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE);
            break;
        case FSM_EVENT_TYPE_ACTIVE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ACTIVE_STATE);
            break;
        case FSM_EVENT_TYPE_AUTH_FAIL:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_AUTH_FAIL_STATE);
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
        case FSM_EVENT_TYPE_ACTIVATING:
            break;
        case FSM_EVENT_TYPE_TIMER:
        default:
            break;
    }
    _FSM_setState(context_ptr, nextState_ptr, event_ptr);
}

/*
 * ======== _stateEnter ========
 * Perform actions and send registeration state response.
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
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_IN_PROGRESS,
                CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
    }
    else if (_FSM_isServiceEmergencyIp(context_ptr)) {
        /* Send the IMS emergency registration state to GAPP */
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_IN_PROGRESS,
                CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
    }
}

/*
 * ======== _stateExit ========
 * reset failed count.
 */
static void _stateExit(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
}

/*
 * This variable holds function pointers to routines that are executed for processing an event,
 * entering a state, and exiting a state.
 */
const FSM_State _FSM_LOGIN_STATE = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,    /* Nothing to do when exiting state */
    _stateName     /* The name of this state */
};

