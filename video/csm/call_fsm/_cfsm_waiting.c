/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30280 $ $Date: 2014-12-09 14:24:57 +0800 (Tue, 09 Dec 2014) $
 */
#include <osal.h>
#include <isi.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_isi.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Waiting";

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
    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_WAITING;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_BEING_FORWARDED:
            /* forwarding on conditions happened after ringing
             * (i.e. after acknowledged). notify the app this supsrv event.
             */
            context_ptr->supsrvNotication |= CSM_SUPSRV_MO_CALL_IS_WAITING;
            _CSM_generateSupsrvReport(CSM_getCall(context_ptr->callId),
                    context_ptr->csmOutput_ptr, NULL);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            if (call_ptr->participants[0].callIndex == event_ptr->u.callIndex) {
                /* This is the call user want to terminate */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
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
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_REJECTED:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CSM_isiAcceptCallModify(callId, context_ptr->protoName_ptr);
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
    context_ptr->state = CSM_CALL_STATE_ALERTING;
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
    /* Stop aSRVCC timer if any. */
    OSAL_tmrStop(context_ptr->timerId);
    OSAL_tmrStop(context_ptr->retryTmrId);

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
const CFSM_State _CFSM_STATE_WAITING = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,    /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
