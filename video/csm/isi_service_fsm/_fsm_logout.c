/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 23805 $ $Date: 2013-12-30 15:01:12 +0800 (Mon, 30 Dec 2013) $
 */
#include "_csm.h"
#include "_csm_service.h"
#include "_csm_isi_service.h"
#include "fsm.h"
#include "_fsm.h"
#include "_csm_response.h"

static const char _stateName[] = "Service:Logout";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    FSM_State_Ptr nextState_ptr = NULL;

    switch (event_ptr->eventType) {
        case FSM_EVENT_TYPE_NEW_IP:
        case FSM_EVENT_TYPE_START:
            /*
             * IMS enabled or got a new IP while de-registering.
             * Go to reset state, when inactive event received,
             * it will register again.
             */
        case FSM_EVENT_TYPE_STOP:
        case FSM_EVENT_TYPE_AUTH_FAIL:
            /* then goto reset. */
            nextState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE);
            break;
        case FSM_EVENT_TYPE_INACTIVE:
        case FSM_EVENT_TYPE_FAILURE:
            /*
            * If de-registering is triggered by disabling IMS, go to reset
            * state. If not, it needs to activate again and go to login state.
            */
            if (OSAL_FALSE == ((CSM_ServiceMngr *) context_ptr->
                    serviceMngr_ptr)->isImsEnabled) {
                nextState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE);
                break;
            }
            /* Got account info from ISIM */
            if (CSM_OK != _CSM_isiServiceSetup(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId)) {
                /* Then go to the error state so we try again later. */
                nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
                CSM_dbgPrintf("Failed to setup isi service: %d\n",
                        context_ptr->serviceId);
                break;
            }
            if (!_FSM_isServiceReady(context_ptr)) {
                /*
                 * There is no available IP can be use or it's slave sip service
                 * or master sip service is not active now, then go to reset
                 * state.
                 */
                nextState_ptr = (FSM_State_Ptr)(&_FSM_RESET_STATE); 
                CSM_dbgPrintf("Service is not ready to be activated: %d\n",
                        context_ptr->serviceId);
                break;
            }
            if (CSM_OK == _CSM_isiServiceActivate(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId)) {
                nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
            }
            else {
                /* Then go to the error state so we try again later. */
                nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
                CSM_dbgPrintf("Failed to activated isi service: %d\n",
                        context_ptr->serviceId);
            }
            break;
        case FSM_EVENT_TYPE_AKA_REQUIRED:
            _CSM_isiServiceSendAkaChallenge(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId, context_ptr->csmOutput_ptr);
            /* No state change is required. */
            break;
        case FSM_EVENT_TYPE_AKA_RESPONSE:
            _CSM_isiServiceSetAkaResponse(context_ptr->serviceMngr_ptr,
                    context_ptr->serviceId);
            /* No state change is required. */
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
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_DEREGISTERING,
                CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
    }
    else if (_FSM_isServiceEmergencyIp(context_ptr)) {
        /* Send the IMS emergency registration state to GAPP */
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_DEREGISTERING,
                CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
    }
}

/*
 * This variable holds function pointers to routines that are executed for processing an event,
 * entering a state, and exiting a state.
 */
const FSM_State _FSM_LOGOUT_STATE = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    NULL,          /* Nothing to do when exiting state */
    _stateName     /* The name of this state */
};

