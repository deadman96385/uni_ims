/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#include "fsm.h"
#include "_fsm.h"

/*
 * ======== FSM_init ========
 * Initialize an instance of the state machine.
 *
 * Return Values:
 * FSM_Context - an initialize context for a new state machine.
 */
FSM_Context_Ptr FSM_init(
    FSM_Context_Ptr   context_ptr,
    CODEC_Ptr         codec_ptr)
{
    context_ptr->active    = OSAL_FALSE;
    context_ptr->codec_ptr = codec_ptr;
    /* Start in IDLE state */
    context_ptr->currentState_ptr = (FSM_State_Ptr)(&_FSM_STATE_IDLE);
    return context_ptr;
}

/*
 * ======== FSM_destroy ========
 * Shutdown an instance of the state machine.
 *
 * Return Values:
 *    none
 */
void FSM_destroy(
    FSM_Context_Ptr context_ptr)
{

}

/*
 * ======== FSM_processEvent ========
 * Process the incoming event. This function handles any pre or post event
 * processing. In addition, it calls the process event function associated
 * with the current state.
 *
 * Return Values:
 * none
 */
void FSM_processEvent(
    FSM_Context_Ptr context_ptr,
    VCE_Event       event,
    vint            codecType,
    char           *eventDesc_ptr)
{
    context_ptr->currentState_ptr->processEvent(context_ptr, event, codecType, eventDesc_ptr);
}

/*
 * ======== _FSM_stateEnter ========
 * Enter the new state. If a function is available that is run when entering
 * the state, it is run by this function. Any common code that is run before
 * or after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _FSM_stateEnter(
    FSM_Context_Ptr context_ptr,
    VCE_Event       event,
    vint            codecType)
{
    VCE_dbgPrintf("entering state %s\n", _FSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateEnter != NULL) {
        context_ptr->currentState_ptr->stateEnter(context_ptr, event, codecType);
    }
}

/*
 * ======== _FSM_stateExit ========
 * Exit the current state. If a function is available that is run when exiting
 * the state, it is run by this function. Any common code that is run before or
 * after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _FSM_stateExit(
    FSM_Context_Ptr  context_ptr,
    VCE_Event        event)
{
    VCE_dbgPrintf("exiting state %s\n", _FSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateExit != NULL) {
        context_ptr->currentState_ptr->stateExit(context_ptr, event);
    }
}

/*
 * ======== _FSM_setState ========
 * This function is used by to change the state of the state machine. It first
 * checks to see if a change occurred. If it did, it runs the state exit
 * function associated with the current state. Then it changes the state and
 * runs the state enter function of the new state.
 *
 * Return Values:
 * none
 */
void _FSM_setState(
    FSM_Context_Ptr context_ptr,
    FSM_State_Ptr   nextState_ptr,
    VCE_Event       event,
    vint            codecType)
{
    /*
     * Check if the state has changed
     */
    if (context_ptr->currentState_ptr != nextState_ptr) {
        /* Exist previous state */
        _FSM_stateExit(context_ptr, event);
        /* Set new state */
        context_ptr->currentState_ptr = nextState_ptr;
        /* Enter new state*/
        _FSM_stateEnter(context_ptr, event, codecType);
    }
}

/*
 * ======== _FSM_toString ========
 * This function is a helper to get the state name string.
 *
 * Return Values:
 * none
 */
const char *_FSM_toString(
    FSM_Context_Ptr context_ptr)
{
    return (context_ptr->currentState_ptr->stateName_ptr);
}

