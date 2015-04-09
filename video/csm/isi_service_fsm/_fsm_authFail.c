/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29316 $ $Date: 2014-10-14 09:57:28 +0800 (Tue, 14 Oct 2014) $
 */
#include "_csm.h"
#include "_csm_service.h"
#include "_csm_isi_service.h"
#include "fsm.h"
#include "_fsm.h"
#include "_csm_response.h"

static const char _stateName[] = "Service:AuthFail";

/*
 * ======== _FSM_authFailProcessEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    FSM_State_Ptr nextState_ptr = NULL;

    switch (event_ptr->eventType) {
        case FSM_EVENT_TYPE_INACTIVE:
        case FSM_EVENT_TYPE_FAILURE:
            break;
        case FSM_EVENT_TYPE_STOP:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE);
            break;
        case FSM_EVENT_TYPE_START:
            /* Got account info from ISIM */
            if (CSM_OK != _CSM_isiServiceSetup(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId)) {
                /* Then go to the error state so we try again later. */
                nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
                break;
            }
            /* Fall through */
        case FSM_EVENT_TYPE_NEW_IP:
            if (OSAL_TRUE != ((CSM_ServiceMngr *) context_ptr->
                    serviceMngr_ptr)->isImsEnabled) {
                break;
            }
            /* Log in. */
            if (CSM_OK == _CSM_isiServiceActivate(context_ptr->serviceMngr_ptr, 
                   context_ptr->serviceId)) {
                nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
            }
            else {
                /* Then go to the error state so we trry again later. */
                nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
            }
            break;
        case FSM_EVENT_TYPE_ACTIVE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ACTIVE_STATE);
            break;
        case FSM_EVENT_TYPE_TIMER:
            break;
        case FSM_EVENT_TYPE_AUTH_FAIL:
            break;
        case FSM_EVENT_TYPE_AKA_REQUIRED:
            _CSM_isiServiceSendAkaChallenge(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId, context_ptr->csmOutput_ptr);
            /* AKA auth required, goto login state for next response */
            nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
            break;
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
const FSM_State _FSM_AUTH_FAIL_STATE = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    NULL,          /* Nothing to do when exiting state */
    _stateName     /* The name of this state */
};
