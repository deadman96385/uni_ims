/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2014 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30280 $ $Date: 2014-12-09 14:24:57 +0800 (Tue, 09 Dec 2014) $
 */

#include <osal.h>
#include <isi.h>
#include "../_csm_event.h"
#include "../_csm.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "../_csm_calls.h"
#include "_cfsm.h"
#include "_csm_utils.h"

static const char _stateName[] = "Calls:in_ims_conf";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CFSM_State_Ptr  nextState_ptr;
    CSM_CallObject *call_ptr;
    int             x;
    char            address[CSM_EVENT_STRING_SZ + 1];
    int             errorCode;

    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IMS_CONF;
    call_ptr = CSM_getCall(context_ptr->callId);
    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_ADD_PARTY:
            /*  add a party to existing conf-call */
            _CSM_isiConfAddParties(context_ptr->callId,
                    context_ptr->remoteParticipants_ptr,
                    NULL);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE:
            if (CSM_CALL_STATE_ACTIVE == context_ptr->state) {
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
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_FAILED:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X:
            CSM_sendError(3, "operation not allowed",
                    context_ptr->csmOutput_ptr);
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X:
            x = -1;
            while (x < (CSM_EVENT_MAX_CALL_LIST_SIZE -1)) {
                x++;
                if (call_ptr->participants[x].callIndex == 
                        event_ptr->u.callIndex) {
                    if (NULL != CSM_isiNormalizeOutboundAddress(context_ptr->isiMngr_ptr, 
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
            errorCode = CSM_utilGetReasonCode(event_ptr->reasonDesc);
            if (errorCode >= CSM_ERROR_CODE_SIP_START && 
                    errorCode <= CSM_ERROR_CODE_SIP_END) {
                CSM_sendError(errorCode, event_ptr->reasonDesc, 
                        context_ptr->csmOutput_ptr);
            }
            break;
        case CSM_CALL_EVT_REASON_XFER_DONE:
            /*  done.. */
            break;
        case CSM_CALL_EVT_REASON_PARTICIPANT_INFO:
            _CFSM_processGroupParticipantInfo(context_ptr, event_ptr);
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
    CSM_CallObject *call_ptr;

    context_ptr->state = CSM_CALL_STATE_ACTIVE;

    call_ptr = CSM_getCall(context_ptr->callId);


    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL:
            // key actions for init conference call
            _CFSM_initiateConfCall(context_ptr, event_ptr,
                    context_ptr->remoteParticipants_ptr);
            /* Send OK to dial command. */
            CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
            break;
        default:
            break;
    }
    call_ptr->multiParty = CSM_CALL_IMS_CONF;
    /* xxx for both init and refer? Set CallId to isi */
    _CSM_isiSetConfCallId(context_ptr->callId);
    /* Let's start the consultative transfer of the other calls to here. */

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
const CFSM_State _CFSM_STATE_IMS_CONF = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    _stateExit,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};

