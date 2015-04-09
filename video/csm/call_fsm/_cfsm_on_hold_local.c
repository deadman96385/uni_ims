/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29762 $ $Date: 2014-11-11 15:13:25 +0800 (Tue, 11 Nov 2014) $
 */
#include <osal.h>
#include <isi.h>
#include "../_csm_event.h"
#include "../_csm_response.h"
#include "../_csm_isi_call.h"
#include "../_csm_calls.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:On_hold_local";
/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CFSM_State_Ptr nextState_ptr;
    CSM_CallObject *call_ptr;

    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HOLD_LOCAL;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE:
            if (OSAL_FALSE == context_ptr->isIpConference) {
                _CSM_isiResumeCall(context_ptr->callId, 
                        context_ptr->protoName_ptr);
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CS_CONF;
            }
            else {
                /*
                 * Resume the exsting IP CC for merging a new call to it.
                 */
                call_ptr = CSM_getCall(context_ptr->callId);
                if (CSM_CALL_CONFERENCE == call_ptr->multiParty) {
                    _CSM_isiResumeCall(context_ptr->callId, 
                            context_ptr->protoName_ptr);
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IP_CONF;
                }
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_SWAP:
            /*
             * Check if there is call in ringing, if yes then
             * the SWAP command will answer the incoming call
             * and will not put this on hold call to active.
             * This call should stay in on hold state.
             */
            call_ptr = CSM_getCall(context_ptr->callId);
            if (CSM_isCallInRing(call_ptr)) {
                CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
                break;
            }
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            _CSM_isiResumeCall(context_ptr->callId, context_ptr->protoName_ptr);
            /* Transition based on whether this is a conference call or not. */
            call_ptr = CSM_getCall(context_ptr->callId);
            if (CSM_CALL_CONFERENCE == call_ptr->multiParty) {
                /* Check the type of conference and transition appropriately */
                if (OSAL_TRUE == context_ptr->isIpConference) {
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IP_CONF;
                }
                else {
                    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CS_CONF;
                }
            }
            else {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_CALL;
            }
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
                break;
        case CSM_CALL_EVT_REASON_FAILED:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_PARTICIPANT_INFO:
            _CFSM_processParticipantInfo(context_ptr, event_ptr);
            call_ptr = CSM_getCall(context_ptr->callId);
            if (_CFSM_getParticipantsCount(call_ptr) <= 0) {
                call_ptr->multiParty = CSM_CALL_SINGLE_PARTY;
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CFSM_processCallModify(event_ptr, context_ptr);
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

    context_ptr->state = CSM_CALL_STATE_HOLD;

    /*
     * For placing a conference call on hold.
     * Only send 'OK' when the first active call to be held.
     */
    call_ptr = CSM_getCall(context_ptr->callId);
    if (CSM_CALL_CONFERENCE == call_ptr->multiParty) {
        if (_CSM_mapOtherConfCallState(call_ptr, CSM_CALL_STATE_HOLD) ==
                OSAL_FALSE) {
            CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            return;
        }
        else {
            return;
        }
    }

    CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
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
    CSM_CallObject *call_ptr;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING:
        case CSM_CALL_EVT_REASON_PARTICIPANT_INFO:
            _CFSM_terminateCall(context_ptr, event_ptr);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_SWAP:
            call_ptr = CSM_getCall(context_ptr->callId);
            /*
             * See if there is more than one call. If so then don't send
             * 'OK' because the 'OK' to the SWAP was sent by another call
             */
            if (1 == CSM_callsNumberInUse()) {
                CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            }
            /*
            * For resuming a conference call.
            * Only send 'OK' when the first held call to be resumed.
            */
            else if ((CSM_CALL_CONFERENCE == call_ptr->multiParty) &&
                        (_CSM_mapOtherConfCallState(call_ptr,
                        CSM_CALL_STATE_ACTIVE) == OSAL_FALSE)) {
                CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            /*
             * No AT 'OK' is sent here for the call going to to the
             * foreground since it is sent on the call that was terminated.
             */
        default:
            break;
    }
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_HOLD_LOCAL = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,    /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
