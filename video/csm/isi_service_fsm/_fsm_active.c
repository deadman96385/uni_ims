/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 27449 $ $Date: 2014-07-15 13:03:08 +0800 (Tue, 15 Jul 2014) $
 */

#include <rpm.h>
#include "_csm.h"
#include "_csm_service.h"
#include "_csm_isi_service.h"
#include "fsm.h"
#include "_fsm.h"
#include "_csm_response.h"

static const char _stateName[] = "Service:Active";

/*
 * ======== _FSM_activateSlaveSip ========
 * This function is a helper to send event to activate slave sip service.
 *
 * Return Values:
 *     OSAL_TRUE: Event sent.
 *     OSAL_FALSE: Event not sent.
 */
static OSAL_Boolean _FSM_activateSlaveSip(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    /* Call CSM routine to activate slave sip */
    if (CSM_ERR == _CSM_isiServiceActivateSlaveSip(srvcMngr_ptr,
            service_ptr->protocol)) {
        return (OSAL_FALSE);
    }

    return (OSAL_TRUE);
}

/*
 * ======== _FSM_deactivateSlaveSip ========
 * This function is a helper to send event to deactivate slave sip service.
 *
 * Return Values:
 *     OSAL_TRUE: Event sent.
 *     OSAL_FALSE: Event not sent.
 */
static OSAL_Boolean _FSM_deactivateSlaveSip(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    /* Call CSM routine to activate slave sip */
    if (CSM_ERR == _CSM_isiServiceDeactivateSlaveSip(srvcMngr_ptr,
            service_ptr->protocol)) {
        return (OSAL_FALSE);
    }

    return (OSAL_TRUE);
}

/*
 * ======== _FSM_setServiceFeature() ========
 * This function is a helper set features to a sip service.
 *
 * Return Values:
 *     OSAL_TRUE: Event sent.
 *     OSAL_FALSE: Event not sent.
 */
static OSAL_Status _FSM_setServiceFeature(
    FSM_Context_Ptr context_ptr)
{
    CSM_ServiceMngr *srvcMngr_ptr;
    CSM_IsiService  *service_ptr;

    srvcMngr_ptr = (CSM_ServiceMngr *) context_ptr->serviceMngr_ptr;

    service_ptr = CSM_isiGetServiceViaId(srvcMngr_ptr->isiMngr_ptr,
            context_ptr->serviceId);

    /* Call CSM routine to set features to isi service */
    if (CSM_ERR == _CSM_isiServiceSetFeature(service_ptr)) {
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _FSM_activeProcessEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    FSM_State_Ptr nextState_ptr = NULL;

    switch (event_ptr->eventType) {
        case FSM_EVENT_TYPE_START:
            /*
             * User accounts updated.
             * De-activate and then goto reset and then re-register.
             */
            if (_FSM_isServiceIp(context_ptr)) {
                _CSM_isiServiceDeactivate(context_ptr->serviceMngr_ptr,
                        context_ptr->serviceId);
                nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGOUT_STATE);
            }
            break;
        case FSM_EVENT_TYPE_INACTIVE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
            break;
        case FSM_EVENT_TYPE_FAILURE:
            nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
            break;
        case FSM_EVENT_TYPE_STOP:
                _CSM_isiServiceDeactivate(context_ptr->serviceMngr_ptr,
                        context_ptr->serviceId);
                nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGOUT_STATE);
            break;
        case FSM_EVENT_TYPE_NEW_IP:
            /*
             * If the new ip isn't the same as the previous one, 
             * de-activate and then goto reset.
             */
            if (OSAL_FALSE == OSAL_netIsAddrEqual(&((CSM_ServiceMngr *) context_ptr->
                serviceMngr_ptr)->regIpAddress, &event_ptr->ipAddress)) {
                _CSM_isiServiceDeactivate(context_ptr->serviceMngr_ptr,
                        context_ptr->serviceId);
                nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGOUT_STATE);
            }
            break;
        case FSM_EVENT_TYPE_ACTIVATING:
            break;
        case FSM_EVENT_TYPE_ACTIVE:
            break;
        case FSM_EVENT_TYPE_TIMER:
            break;
        case FSM_EVENT_TYPE_AUTH_FAIL:
            if (OSAL_TRUE != ((CSM_ServiceMngr *) context_ptr->
                    serviceMngr_ptr)->isImsEnabled) {
                break;
            }
            /* Log in failed! Let's try agian. */
           if (CSM_OK == _CSM_isiServiceActivate(context_ptr->serviceMngr_ptr,
                   context_ptr->serviceId)) {
               nextState_ptr = (FSM_State_Ptr)(&_FSM_LOGIN_STATE);
           }
           else {
               /* Then go to the error state so we trry again later. */
               nextState_ptr = (FSM_State_Ptr)(&_FSM_ERROR_STATE);
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

    if (_FSM_isServiceLteIp(context_ptr)) {
        /*
         * Don't update to RPM if the service is slave sip service, i.e. the one
         * doesn't do registration.
         */
        if (_FSM_isServiceMasterIp(context_ptr)) {
            /* Send the IMS registration state to GAPP */
            _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_ACTIVE,
                    CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                    context_ptr->csmOutput_ptr);
            /* Send other service notification. */
            _FSM_sendServiceNotification(context_ptr, event_ptr);
            /* notify rpm that this service is active */
            RPM_setServiceState(RPM_RADIO_TYPE_LTE, RPM_SERVICE_STATE_ACTIVE);
            /* notify isi server what is the master service */
            CSM_isiServiceSetMasterNetworkMode(CSM_ISI_PROTOCOL_MODEM_IMS);
            /* Send event to activate slave sip service */
            _FSM_activateSlaveSip(context_ptr);
        }
        /* Set feature no matter it's master or slave */
        _FSM_setServiceFeature(context_ptr);
    }
    else if (_FSM_isServiceRcsIp(context_ptr)) {
        /*
         * Don't update to RPM if the service is slave sip service, i.e. the one
         * doesn't do registration.
         */
        if (_FSM_isServiceMasterIp(context_ptr)) {
            /* Send the IMS registration state to GAPP */
            _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_ACTIVE,
                    CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                    context_ptr->csmOutput_ptr);
            /* notify rpm that this service is active */
            RPM_setServiceState(RPM_RADIO_TYPE_WIFI, RPM_SERVICE_STATE_ACTIVE);
            /* notify isi server what is the master service */
            CSM_isiServiceSetMasterNetworkMode(CSM_ISI_PROTOCOL_SIP_RCS);
            /* Don't activate MSAPP in wifi mode */
            _FSM_activateSlaveSip(context_ptr);
        }
        /* Set feature no matter it's master or slave */
        _FSM_setServiceFeature(context_ptr);
    }
    else if (_FSM_isServiceEmergencyIp(context_ptr)) {
        /* Send the IMS emergency registration state to GAPP */
        _FSM_sendServiceStateEvent(CSM_SERVICE_STATE_ACTIVE,
                CSM_ERROR_CODE_NONE, service_ptr->isEmergency,
                context_ptr->csmOutput_ptr);
        /* notify rpm that this service is active */
        RPM_setServiceState(RPM_RADIO_TYPE_LTE_EMERGENCY,
                RPM_SERVICE_STATE_ACTIVE);
    }
    else {
        /* notify rpm that this service is active */
        RPM_setServiceState(RPM_RADIO_TYPE_CS, RPM_SERVICE_STATE_ACTIVE);
    }

    /* Notify service is actived to csm call */
    _FSM_sendServiceStateEventToCall(context_ptr, FSM_EVENT_TYPE_ACTIVE);
}

/*
 * ======== _FSM_errorExitState ========
 * Perform actions needed when entering Wifi state.
 */
static void _stateExit(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr)
{
    /* Notify RPM that this service is inactive */
    if (_FSM_isServiceLteIp(context_ptr)) {
        /* Tell RPM the service is inactive */
        RPM_setServiceState(RPM_RADIO_TYPE_LTE, RPM_SERVICE_STATE_INACTIVE);
        /*
         * Deactivate slave sip service when master sip service deactivated.
         */
        if (_FSM_isServiceMasterIp(context_ptr)) {
            /* Send event to deactivate slave sip service */
            _FSM_deactivateSlaveSip(context_ptr);
        }
    }
    else if (_FSM_isServiceRcsIp(context_ptr)) {
        /* Tell RPM the service is inactive */
        RPM_setServiceState(RPM_RADIO_TYPE_WIFI, RPM_SERVICE_STATE_INACTIVE);
        /*
         * Deactivate slave sip service when master sip service deactivated.
         */
        if (_FSM_isServiceMasterIp(context_ptr)) {
            /* Send event to activate slave sip service */
            _FSM_deactivateSlaveSip(context_ptr);
        }
    }
    else if (_FSM_isServiceEmergencyIp(context_ptr)) {
        /* notify rpm that this service is inactive */
        RPM_setServiceState(RPM_RADIO_TYPE_LTE_EMERGENCY,
                RPM_SERVICE_STATE_INACTIVE);
    }
    else {
        RPM_setServiceState(RPM_RADIO_TYPE_CS, RPM_SERVICE_STATE_INACTIVE);
    }
}


/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const FSM_State _FSM_ACTIVE_STATE = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,    /* Prepare to exit the state */
    _stateName     /* The name of this state */
};
