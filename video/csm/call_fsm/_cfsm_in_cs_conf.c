/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 26722 $ $Date: 2014-06-03 17:20:29 +0800 (Tue, 03 Jun 2014) $
 */
#include <osal.h>
#include <isi.h>
#include "../_csm_event.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_cfsm.h"

static const char _stateName[] = "Calls:In_conf_cs_call";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_CallObject* call_ptr;
    CFSM_State_Ptr  nextState_ptr;
    vint            numCalls;

    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CS_CONF;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            call_ptr = CSM_getCall(context_ptr->callId);
            call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE:
            if (CSM_CALL_STATE_ACTIVE == context_ptr->state) {
                call_ptr = CSM_getCall(context_ptr->callId);
                call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                if (ISI_RETURN_OK == _CSM_isiConferenceRemove(
                        CSM_getCsConferenceId(),
                        context_ptr->callId)) {
                    call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CALL;
                    CSM_decrementCsConference();
                }
            }
            else {
                /*
                 * Check if this call is still in conferecne after splitting
                 * another call from conference.
                 */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HOLD_LOCAL;
                numCalls = CSM_getCsConferenceCallNums();
                if (call_ptr->participants[0].callIndex <
                        event_ptr->u.callIndex) {
                    /*
                     * Current call index is prior than the splitted call index.                     * It doestn't decrease numCalls yet.
                     */
                    numCalls -= 1;
                }
                if (numCalls < 2) {
                    call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
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
                CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_DIGIT:
            _CSM_isiSendDigit(context_ptr->callId, context_ptr->protoName_ptr,
                    event_ptr->u.digit, event_ptr->extraArgument);
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
            call_ptr = CSM_getCall(context_ptr->callId);
            call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_FAILED:
            call_ptr = CSM_getCall(context_ptr->callId);
            call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
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
    CSM_CallObject *call_ptr;
    context_ptr->state = CSM_CALL_STATE_ACTIVE;
    call_ptr = CSM_getCall(context_ptr->callId);
    call_ptr->multiParty = CSM_CALL_CONFERENCE;
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
        case CSM_CALL_EVT_REASON_DISCONNECT:
            CSM_decrementCsConference();
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
const CFSM_State _CFSM_STATE_CS_CONF = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
