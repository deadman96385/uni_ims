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
#include "../_csm_isi.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Initializing";

/*
 * _CFSM_STATE_INITIALIZING is a intermediate state before entering 
 * _CFSM_STATE_RINGING state for a mobile terminated call if radio
 * resource is not ready when receiving CSM_CALL_EVT_REASON_NEW_INCOMING
 * event.
 * This state should handle CSM_CALL_REASON_RESOURCE_READY and
 * CSM_CALL_EVT_REASON_RESOURCE_FAILED event from application. And handle
 * CSM_CALL_EVT_REASON_MODIFY from underlying protocol for remote resource
 * update.
 * Refer to RFC 3312 Integration of Resource Management and Session
 * Initiation Protocol
 */

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
    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_INITIALIZING;
    call_ptr = CSM_getCall(callId);

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_RESOURCE_INDICATION:
            if (1 == event_ptr->u.resourceStatus.audioReady) {
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
                /* Local resources are ready. Set local resource ready. */
                _CSM_isiSetCallResourceStatus(context_ptr->callId,
                        ISI_RESOURCE_STATUS_LOCAL_READY);
                /* Notify remote side */
                _CSM_isiCallModify(callId, NULL);
                /* Check if the call resource is ready. */
                _CSM_isiGetCallResourceStatus(context_ptr->callId, &rsrcStatus,
                        &context_ptr->csmOutput_ptr->evt.call.u.resourceMedia);
                if ((rsrcStatus & ISI_RESOURCE_STATUS_REMOTE_READY) &&
                        (rsrcStatus & ISI_RESOURCE_STATUS_LOCAL_READY)) {
                    /* Resource is ready, goes to ringning state */
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_RINGING;
                    /*
                     * Set the negExchange for reporting when state change to
                     * ringing state.
                     */
                    call_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED;
                }
            }
            else {
                /* audio resource is not ready. */
                /* Set resoure failure and modify the call */
                _CSM_isiSetCallResourceStatus(context_ptr->callId,
                        ISI_RESOURCE_STATUS_FAILURE);
                _CSM_isiCallModify(callId, NULL);
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_FAILED:
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            /* Check if the call resource is ready */
            _CSM_isiGetCallResourceStatus(context_ptr->callId, &rsrcStatus, 
                    &context_ptr->csmOutput_ptr->evt.call.u.resourceMedia);
            /*
             * Accept modify no matter local resource is ready or not.
             * If current local resource is not ready yet, we will goes to
             * ringinig state when we receive resource ready event and sending
             * 180 Ringing. 180 Ringing implies resource is ready.
             */
            _CSM_isiAcceptCallModify(callId, context_ptr->protoName_ptr);
            if ((rsrcStatus & ISI_RESOURCE_STATUS_REMOTE_READY) ||
                    (rsrcStatus & ISI_RESOURCE_STATUS_LOCAL_READY)) {
                /* Resource is ready, goes to ringning state */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_RINGING;
                break;
            }
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
    context_ptr->isPreconditionUsed = OSAL_TRUE;
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
    OSAL_Boolean    isActive;
    CSM_CallObject *call_ptr;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            _CFSM_rejectCall(context_ptr, event_ptr, event_ptr->reasonDesc);
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            /*
             * Exiting this state because of resource is ready.
             * Generate report depends on if there is active call 
             */
            call_ptr = CSM_getCall(context_ptr->callId);
            /* Is there other active call */
            if (OSAL_TRUE == (isActive = _CSM_isCallInUse(call_ptr))) {
                /* It's call waiting call */
                context_ptr->state = CSM_CALL_STATE_WAITING;
            }
            else {
                context_ptr->state = CSM_CALL_STATE_INCOMING;
            }
            /* Generate incoming call event */
            _CSM_generateInitilizingCallEvent(call_ptr,
                    OSAL_TRUE, isActive, context_ptr->csmOutput_ptr);
            break;
        default:
            break;
    }
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_INITIALIZING = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,    /* Prepare to exit state */
    _stateName,     /* The name of this state */
};
