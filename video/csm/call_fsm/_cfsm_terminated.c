/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29866 $ $Date: 2014-11-17 14:42:10 +0800 (Mon, 17 Nov 2014) $
 */
#include <osal.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_isi.h"
#include "../_csm_response.h"
#include "_cfsm.h"
#include "_csm_utils.h"

static const char _stateName[] = "Call:Terminated";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    /*
     * This is a terminal state, any event received in this state is an error.
     */
    OSAL_logMsg("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
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
    int errorCode;
    /* Clear negExchange. */    
    _CFSM_setNegExchange(context_ptr, CSM_CALL_NEG_EXCHANGE_NONE);
    
    /* Go to terminate state from a Failure event. We report this error.*/
    if (event_ptr->reason == CSM_CALL_EVT_REASON_FAILED) {
        errorCode = CSM_utilGetReasonCode(event_ptr->reasonDesc);
        if (errorCode >= CSM_ERROR_CODE_SIP_START && 
                errorCode <= CSM_ERROR_CODE_SIP_END) {
            /* SIP error. */
            CSM_sendError(errorCode, event_ptr->reasonDesc, 
                    context_ptr->csmOutput_ptr);
        }
        else {
            /* Non-SIP error. */
            CSM_sendError(30, event_ptr->reasonDesc,
                    context_ptr->csmOutput_ptr);
        }
    }

    if (CSM_CALL_STATE_INVALID == context_ptr->state) {
        /* This is local hung up, so send response OK of hunging up AT */
        CSM_sendOk(NULL, context_ptr->csmOutput_ptr);
    }
    else {
        /* send '3' when remote disconnect */
        CSM_sendRemoteDisconnect(NULL, context_ptr->csmOutput_ptr);
        context_ptr->state = CSM_CALL_STATE_INVALID;
    }
    context_ptr->active = OSAL_FALSE;
    context_ptr->callId = 0;
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_TERMINATED = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to terminate state machine. */
    NULL,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
