/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1.6 $ $Date: 2007/09/17 18:33:04 $
 */
#include <osal.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_isi.h"
#include "../_csm_isi_call.h"
#include "../_csm_response.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Hanging_up";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CFSM_State_Ptr nextState_ptr;

    nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_HANGINGUP;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_DISCONNECT:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_TERMINATED;
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
    /* Clear negExchange. */    
    _CFSM_setNegExchange(context_ptr, CSM_CALL_NEG_EXCHANGE_NONE);

    context_ptr->state = CSM_CALL_STATE_INVALID;
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_HANGINGUP = {
    _processEvent, /* Process events */
    _stateEnter,   /* Prepare to enter state */
    NULL,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
