/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30325 $ $Date: 2014-12-10 18:01:04 +0800 (Wed, 10 Dec 2014) $
 */
#include <osal.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_isi.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_csm.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Ringing";

/*
 * ======== _CFSM_ringingTimerCb ========
 * Private helper for CFSM ringing timer events
 */
static int32 _CFSM_ringingTimerCb(
    void *arg_ptr)
{
    CFSM_Context_Ptr context_ptr = (CFSM_Context_Ptr)arg_ptr;
    if (CSM_OK != _CFSM_sendRingingTimeout(context_ptr)) {
        OSAL_tmrStart(context_ptr->retryTmrId, _CFSM_ringingTimerCb,
                arg_ptr, CSM_CALL_SEND_MSG_RETRY_MS);
    }
    return (0);
}

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    vint            callId;
    CFSM_State_Ptr  nextState_ptr;
    CSM_CallObject *call_ptr;

    callId = context_ptr->callId;
    call_ptr = CSM_getCall(context_ptr->callId);
    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_RINGING;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_SWAP:
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CALL;
            break;
        case CSM_CALL_EVT_REASON_FAILED:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CFSM_processCallModify(event_ptr, context_ptr);
            break;
        case CSM_CALL_EVT_REASON_OTHER_DISCONNECT:
            /*
             * Then some other call disconnected. If we were a 'call waiting'
             * then adjust the call state used in the call report based on
             * the status of other calls.
             */
            if (CSM_CALL_STATE_WAITING == context_ptr->state) {
                context_ptr->state = CSM_CALL_STATE_INCOMING;
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_ANSWER:
            if (ISI_RETURN_OK == _CSM_isiAcceptCall(context_ptr->callId,
                    context_ptr->protoName_ptr)) {
                CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
                /* go to answered state to waiting for ACK */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_ANSWERED;
            }
            else {
                CSM_sendError(0, NULL, context_ptr->csmOutput_ptr);
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            }
            break;
        case CSM_CALL_EVT_REASON_ACCEPTED:
            /* 
             * ACK received, call active, go to in call state.
             * We wait for remote acknowledged to 200 OK then change call
             * state to in call state.
             */
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CALL;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_MEDIA_CONTROL:
            /* Handle user aceept audio only. */
            if (CSM_CALL_NEG_STATUS_UNCONDITIONAL == event_ptr->negStatus) {
                /* Update negSessionType and set session type to ISI. */
                call_ptr->negSessionType = event_ptr->callSessionType;
                _CSM_isiSetSessionType(callId, event_ptr->callSessionType);
#if defined(PROVIDER_CMCC)
                if(OSAL_TRUE == context_ptr->isPreconditionUsed) {
                    /* Notify remote side */
                    _CSM_isiCallModify(context_ptr->callId, NULL);
                }
#endif
            }
            CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
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
    _CSM_isiAcknowledgeCall(context_ptr->callId, context_ptr->protoName_ptr);
    /* Start a timer to kick in 30 seconds. */
    OSAL_tmrStart(context_ptr->timerId, _CFSM_ringingTimerCb,
            context_ptr, CFSM_RINGING_TIMER_RETRY_MS);
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
    /* Stop ringing timer or aSRVCC timer if any. */
    OSAL_tmrStop(context_ptr->timerId);
    OSAL_tmrStop(context_ptr->retryTmrId);

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_SWAP:
            _CSM_isiAcceptCall(context_ptr->callId,
                    context_ptr->protoName_ptr);
            /*
             * No AT 'OK' is sent here, it was sent by the
             * other active call being placed on hold.
             */
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            _CSM_isiAcceptCall(context_ptr->callId,
                    context_ptr->protoName_ptr);
            /*
             * No AT 'OK' is sent by the call going to
             * the foreground since it was sent on the call
             * that was being terminated.
             */
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            _CFSM_rejectCall(context_ptr, event_ptr, event_ptr->reasonDesc);
            break;
        default:
            break;
    }
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_RINGING = {
    _processEvent, /* Process events */
    _stateEnter,   /* Nothing to do on state enter */
    _stateExit,    /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
