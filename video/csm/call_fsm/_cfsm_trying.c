/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */
#include <osal.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "../_csm_isi_call.h"
#include "../_csm_isi.h"
#include "../_csm_utils.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Trying";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    vint               callId;
    CFSM_State_Ptr     nextState_ptr;
    ISI_ResourceStatus rsrcStatus;
    CSM_CallObject    *call_ptr;

    callId = context_ptr->callId;
    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TRYING;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                /* This is the call user want to terminate */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_BEING_FORWARDED:
            /*
             * unconditional forwarding happened before ringback
             * (i.e. before acknowledged). notify the app this supsrv event.
             */
            context_ptr->supsrvNotication |= CSM_SUPSRV_MO_CALL_BEING_FORWARDED;
            _CSM_generateSupsrvReport(CSM_getCall(context_ptr->callId),
                    context_ptr->csmOutput_ptr, NULL);
            break;
        case CSM_CALL_EVT_REASON_ACKNOWLEDGED:
            context_ptr->supsrvNotication = 0; /* supsrv flags are not sticky */
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_WAITING;

            /* get the supsrv header */
            /* check cw rignback tone event */
            if (_CSM_isiCheckSupsrvHeader(callId,
                    ISI_SUPSRV_HFEXIST_ALERT_INFO_CW)) {
                context_ptr->supsrvNotication |= CSM_SUPSRV_MO_CALL_IS_WAITING;
            }
            /* check virtual ring event */
            if (_CSM_isiCheckSupsrvHeader(callId,
                    ISI_SUPSRV_HFEXIST_VIRTUAL_RING)) {
                context_ptr->supsrvNotication |=
                        CSM_SUPSRV_MO_CALL_VIRTUAL_RING;
            }
            /* report unsolicited event to app if needed */
            if (CSM_SUPSRV_CALL_NONE != context_ptr->supsrvNotication) {
                _CSM_generateSupsrvReport(
                     CSM_getCall(context_ptr->callId),
                     context_ptr->csmOutput_ptr, NULL);
            }
            break;
        case CSM_CALL_EVT_REASON_ACCEPTED:
            context_ptr->supsrvNotication = 0; /* supsrv flags are not sticky */

            if (OSAL_TRUE == context_ptr->isIpConference) {
                if (OSAL_TRUE == context_ptr->isFocusOwner) {
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IMS_CONF;
                } else {
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IP_CONF;
                }
            }
            else {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CALL;
            }
            _CFSM_processRemoteAnswer(context_ptr);
            break;
        case CSM_CALL_EVT_REASON_FAILED:
            /* Check if it's emergency call and if needs to failover */
            if (_CFSM_isFailoverToCs(context_ptr)) {
                if (OSAL_TRUE ==_CFSM_failoverToCs(context_ptr, event_ptr)) {
                    /* Go to dialing state */
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_DIALING;
                    break;
                }
            }
            /* Terminate the call */
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_REJECTED:
            nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CSM_isiAcceptCallModify(callId, context_ptr->protoName_ptr);
            /* Check if resource is not ready */
            _CSM_isiGetCallResourceStatus(context_ptr->callId, &rsrcStatus, 
                    &context_ptr->csmOutput_ptr->evt.call.u.resourceMedia);
            if (!(rsrcStatus & ISI_RESOURCE_STATUS_LOCAL_READY)) {
                context_ptr->isPreconditionUsed = OSAL_TRUE;
                /*
                 * Local resource is not ready, send INITILIZING event
                 * to application.
                 */
                _CSM_generateInitilizingCallEvent(CSM_getCall(context_ptr->callId),
                        OSAL_FALSE, OSAL_FALSE, context_ptr->csmOutput_ptr);
            }
            break;
        case CSM_CALL_EVT_REASON_RESOURCE_INDICATION:
            context_ptr->isPreconditionUsed = OSAL_TRUE;
            if (1 == event_ptr->u.resourceStatus.audioReady) {
                call_ptr = CSM_getCall(context_ptr->callId);
                if ((call_ptr->sessionType & CSM_CALL_SESSION_TYPE_VIDEO) &&
                       (1 != event_ptr->u.resourceStatus.videoReady)) {
                     /*
                      * Video call but video resource is not ready. Downgrade
                      * to audio.
                      */
                     call_ptr->negSessionType = CSM_CALL_SESSION_TYPE_AUDIO;
                     _CSM_isiSetSessionType(callId, CSM_CALL_SESSION_TYPE_AUDIO);
                     /* Generate report to indicate session type is changed. */
                     _CSM_generateMonitorReport(NULL, context_ptr->csmOutput_ptr);
                }
                /* Tell ISI local resource is ready and stay in current state */
                _CSM_isiSetCallResourceStatus(context_ptr->callId,
                        ISI_RESOURCE_STATUS_LOCAL_READY);
                /*
                 * Call direction might be to inactive because of precondition,
                 * set to original direction sendrecv.
                 */
                _CSM_isiSetCallDirection(context_ptr->callId,
                        ISI_SESSION_DIR_SEND_RECV, context_ptr->protoName_ptr); 
                /* Modify the call */
                _CSM_isiCallModify(callId, NULL);
            }
            else {
                /* Resource reservation failed, cancel the call */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
                _CFSM_terminateCall(context_ptr, event_ptr);
            }
            break;
        case CSM_CALL_EVT_REASON_SERVICE_ACTIVE:
            break;
        case CSM_CALL_EVT_REASON_SERVICE_INACTIVE:
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_DIGIT:
            _CSM_isiSendDigit(context_ptr->callId, context_ptr->protoName_ptr,
                    event_ptr->u.digit, event_ptr->extraArgument);
            break;
        default:
            break;
    }
    _CFSM_setState(context_ptr, nextState_ptr, event_ptr);
}

/*
 * ======== _stateEnter ========
 * Perform actions needed when entering this state.
 *
 * Return Values:
 * none
 */
static void _stateEnter(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
}

/*
 * ======== _stateExit ========
 * Perform actions needed when exiting this state.
 *
 * Return Values:
 * none
 */
static void _stateExit(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            _CFSM_terminateCall(context_ptr, event_ptr);
            break;
        default:
            break;
    }
}


/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_TRYING = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,    /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
