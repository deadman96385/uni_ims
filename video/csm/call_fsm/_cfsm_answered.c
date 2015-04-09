/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 22088 $ $Date: 2013-09-09 18:42:24 +0800 (Mon, 09 Sep 2013) $
 */
#include <osal.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_isi.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Answered";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    vint            callId;
    CFSM_State_Ptr nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_ANSWERED;
    CSM_CallObject *call_ptr;

    callId = context_ptr->callId;
    call_ptr = CSM_getCall(context_ptr->callId);

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            if (CSM_CALL_STATE_ACTIVE == context_ptr->state) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            if (call_ptr->participants[0].callIndex != event_ptr->u.callIndex) {
                if (ISI_RETURN_OK == _CSM_isiHoldCall(
                    context_ptr->callId, context_ptr->protoName_ptr)) {
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HOLD_LOCAL;
                }
                else {
                    /* Otherwise just report that the command completed. */
                    OSAL_logMsg("%s %s: Failed to hold call in 'in call' state",
                            IR92_DEBUG_TAG, __FUNCTION__);
                    CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
                }
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_SWAP:
            if (ISI_RETURN_OK == _CSM_isiHoldCall(
                    context_ptr->callId, context_ptr->protoName_ptr)) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HOLD_LOCAL;
            }
            else {
                /* Otherwise just report that the command completed. */
                OSAL_logMsg("%s %s: Failed to hold call in 'in call' state",
                        IR92_DEBUG_TAG, __FUNCTION__);
                CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_DIGIT:
            _CSM_isiSendDigit(context_ptr->callId, context_ptr->protoName_ptr,
                    event_ptr->u.digit, event_ptr->extraArgument);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE:
            if (OSAL_FALSE == context_ptr->isIpConference) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CS_CONF;
            }
            /* Otherwise ignore the command. */
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_FAILED:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CSM_isiAcceptCallModify(callId, context_ptr->protoName_ptr);
            break;
        case CSM_CALL_EVT_REASON_ACCEPT_ACK:
            /* 
             * ACK received, call active, go to in call state.
             * We wait for remote acknowledged to 200 OK then change call
             * state to in call state.
             */
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CALL;
            /* Update session type and send call report. */
            call_ptr->sessionType = call_ptr->negSessionType;
            _CFSM_setNegExchange(context_ptr, CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED);
            break;
        default:
            break;
    }
    _CFSM_setState(context_ptr, nextState_ptr, event_ptr);
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
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
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
const CFSM_State _CFSM_STATE_ANSWERED = {
    _processEvent, /* Process events */
    NULL,          /* Nothing to do on state enter */
    _stateExit,    /* Nothing to do when exiting state */
    _stateName,    /* The name of this state */
};
