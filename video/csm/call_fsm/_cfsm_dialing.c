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
#include "_cfsm.h"

static const char _stateName[] = "Call:Dialing";
/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 *
 * RETRUNS
 *     nothing
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CFSM_State_Ptr      nextState_ptr;

    nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_DIALING;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_TRYING:
            nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_TRYING;
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
                    _CFSM_initiateCall(context_ptr, event_ptr);
                    /* Keep in the same state */
                    break;
                }
            }
            nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_REJECTED:
            nextState_ptr = (CFSM_State_Ptr) &_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CFSM_processCallModify(event_ptr, context_ptr);
            break;
        case CSM_CALL_EVT_REASON_SERVICE_ACTIVE:
            break;
        case CSM_CALL_EVT_REASON_SERVICE_INACTIVE:
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
    CFSM_Context_Ptr context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    /* Send response with call index. */
    _CFSM_sendCallIndex(context_ptr);
    /* Send OK to dial command. */
    CSM_sendOk(NULL, context_ptr->csmOutput_ptr);

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_DIAL:
        case CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE:
            _CFSM_initiateCall(context_ptr, event_ptr);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL:
            /*  key action for refer conference call */
            _CFSM_initiateConfCall(context_ptr, event_ptr,
                    context_ptr->remoteParticipants_ptr);
            break;
        default:
            break;
    }
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
            context_ptr->state = CSM_CALL_STATE_INVALID;
            break;
        default:
            break;
    }
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_DIALING = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state. */
    _stateExit,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
