/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1.6 $ $Date: 2007/09/17 18:33:04 $
 */

#include "../_rpm.h"
#include "_rfsm.h"

/*
 * ======== RFSM_init ========
 * Initialize an instance of the state machine.
 *
 * Return Values:
 *     none
 */
void RFSM_init(
    RFSM_Context *context_ptr)
{
    RPM_dbgPrintf("\n");

    /* Start in No Radio "NONE" state */
    context_ptr->currentState_ptr  = (RFSM_State *)(&_RFSM_STATE_NONE);
    /* Init the context to inactive / none */
    context_ptr->radioInfc.radioType = RPM_RADIO_TYPE_NONE;
    context_ptr->csState  = RPM_SERVICE_STATE_INACTIVE;
    context_ptr->lteState = RPM_SERVICE_STATE_INACTIVE;
}

/*
 * ======== _RFSM_shouldProcessEvent ========
 * Private helper used to filter out unwanted events.
 *
 * Return Values:
 *     OSAL_TRUE - if caller should process the event
 *     OSAL_FALSE - otherwise
 */
static OSAL_Boolean _RFSM_shouldProcessEvent(
    RFSM_Context *context_ptr,
    RPM_Event     *event_ptr)
{
    return (OSAL_TRUE);
}

/*
 * ======== RFSM_processEvent ========
 * Process the incoming event. This function handles any pre or post event
 * processing. In addition, it calls the process event function associated
 * with the current state.
 *
 * Return Values:
 * none
 */
void RFSM_processEvent(
    RFSM_Context *context_ptr,
    RPM_Event     *event_ptr)
{
    if (OSAL_TRUE == _RFSM_shouldProcessEvent(context_ptr, event_ptr)) {
        context_ptr->currentState_ptr->processEvent(context_ptr, event_ptr);
    }
}

/*
 * ======== _RFSM_stateEnter ========
 * Enter the new state. If a function is available that is run when entering
 * the state, it is run by this function. Any common code that is run before
 * or after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _RFSM_stateEnter(
    RFSM_Context *context_ptr,
    RPM_Event     *event_ptr)
{
    RPM_dbgPrintf("entering state %s\n", _RFSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateEnter != NULL) {
        context_ptr->currentState_ptr->stateEnter(context_ptr, event_ptr);
    }
}

/*
 * ======== _RFSM_stateExit ========
 * Exit the current state. If a function is available that is run when exiting
 * the state, it is run by this function. Any common code that is run before or
 * after the function can be run at this time.
 *
 * Return Values:
 * none
 */
static void _RFSM_stateExit(
    RFSM_Context *context_ptr,
    RPM_Event     *event_ptr)
{
    RPM_dbgPrintf("exiting state %s\n", _RFSM_toString(context_ptr));
    if (context_ptr->currentState_ptr->stateExit != NULL) {
        context_ptr->currentState_ptr->stateExit(context_ptr, event_ptr);
    }
}

/*
 * ======== _RFSM_setState ========
 * This function is used by to change the state of the state machine. It first
 * checks to see if a change occurred. If it did, it runs the state exit
 * function associated with the current state. Then it changes the state and
 * runs the state enter function of the new state.
 *
 * Return Values:
 * none
 */
void _RFSM_setState(
    RFSM_Context *context_ptr,
    RFSM_State   *nextState_ptr,
    RPM_Event     *event_ptr)
{
    /*
     * Check if the state has changed
     */
    if (context_ptr->currentState_ptr != nextState_ptr) {
        /* Exist previous state */
        _RFSM_stateExit(context_ptr, event_ptr);
        /* Set new state */
        context_ptr->currentState_ptr = nextState_ptr;
        /* Enter new state*/
        _RFSM_stateEnter(context_ptr, event_ptr);
    }
}

/*
 * ======== _RFSM_toString ========
 * This function is a helper to get the state name string.
 *
 * Return Values:
 * none
 */
const char *_RFSM_toString(
    RFSM_Context *context_ptr)
{
    return (context_ptr->currentState_ptr->stateName_ptr);
}
