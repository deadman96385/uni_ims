/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29167 $ $Date: 2014-10-08 10:42:37 +0800 (Wed, 08 Oct 2014) $
 */
#include <osal.h>
#include <isi.h>
#include "../_csm_event.h"
#include "../_csm.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_cfsm.h"

static const char _stateName[] = "Calls:In_conf_ip_call";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    ISI_Id          isiId;
    CFSM_State_Ptr  nextState_ptr;
    CSM_CallObject *call_ptr;
    int             x;
    char            address[CSM_EVENT_STRING_SZ + 1];
    CSM_IsiMngr    *isiMngr_ptr;

    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IP_CONF;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            call_ptr = CSM_getCall(context_ptr->callId);
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            if (CSM_CALL_STATE_ACTIVE == context_ptr->state) {
                call_ptr = CSM_getCall(context_ptr->callId);
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
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
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_FAILED:
            call_ptr = CSM_getCall(context_ptr->callId);
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
            CSM_sendError(3, "operation not allowed",
                    context_ptr->csmOutput_ptr);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            call_ptr = CSM_getCall(context_ptr->callId);
            x = -1;
            while (x < (CSM_EVENT_MAX_CALL_LIST_SIZE -1)) {
                x++;
                if (call_ptr->participants[x].callIndex == 
                        event_ptr->u.callIndex) {
                    isiMngr_ptr = CSM_getIsiMngr();
                    if (NULL != CSM_isiNormalizeOutboundAddress(isiMngr_ptr, 
                            call_ptr->participants[x].number, address, 
                            CSM_EVENT_STRING_SZ,
                            RPM_FEATURE_TYPE_CALL_NORMAL)) {
                        _CSM_isiConsultativeRemove(context_ptr->callId, address);
                        CSM_clearCallIndex(call_ptr->participants[x].callIndex);
                        OSAL_strncpy(call_ptr->participants[x].normalizedAddress,
                                address, CSM_ALPHA_STRING_SZ);
                        /* Clear the participant now. */
                        call_ptr->participants[x].callIndex = 0;
                    }
                    break;
                }
            };
            CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            break;            
        case CSM_CALL_EVT_REASON_XFER_FAILED:
        case CSM_CALL_EVT_REASON_XFER_DONE:
            while (0 != (isiId = CSM_getIpConferenceInvitation())) {
                if (ISI_RETURN_OK == _CSM_isiConsultativeTransfer(
                        context_ptr->callId, isiId)) {
                    break;
                }
            }
            break;
        case CSM_CALL_EVT_REASON_PARTICIPANT_INFO:
            _CFSM_processParticipantInfo(context_ptr, event_ptr);
            call_ptr = CSM_getCall(context_ptr->callId);
            if (_CFSM_getParticipantsCount(call_ptr) <= 0) {
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            }
            break;
        case CSM_CALL_EVT_REASON_MODIFY:
            _CSM_isiAcceptCallModify(context_ptr->callId, context_ptr->protoName_ptr);
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
    ISI_Id isiId;
    CSM_CallObject *call_ptr;
    context_ptr->state = CSM_CALL_STATE_ACTIVE;

    call_ptr = CSM_getCall(context_ptr->callId);
    call_ptr->multiParty = CSM_CALL_CONFERENCE;
    /* Set CallId to isi */
    _CSM_isiSetConfCallId(context_ptr->callId);
    /* Let's start the consultative transfer of the other calls to here. */

    /*  key actions for refer conference call */
    while (0 != (isiId = CSM_getIpConferenceInvitation())) {
        if (ISI_RETURN_OK == _CSM_isiConsultativeTransfer(context_ptr->callId, isiId)) {
            break;
        }
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
    /* Clear conference call id from ISI  */
    _CSM_isiSetConfCallId(0);
    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_END:
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
        case CSM_CALL_EVT_REASON_PARTICIPANT_INFO:
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
const CFSM_State _CFSM_STATE_IP_CONF = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
