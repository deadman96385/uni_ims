/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1.6 $ $Date: 2007/09/17 18:33:04 $
 */
#include <osal.h>
#include "_rfsm.h"
#include "../_rpm.h"

static const char _stateName[] = "RADIO:None";

/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    RFSM_Context *context_ptr,
    RPM_Event    *event_ptr)
{
    /* Set next state to current state */
    RFSM_State *nextState_ptr = (RFSM_State *)&_RFSM_STATE_NONE;

    switch (event_ptr->type) {
        case RPM_EVENT_TYPE_IP_CHANGE:
            break;
        case RPM_EVENT_TYPE_SERVICE_STATE_CHANGE:
            /* Check what changed and transition accordingly */
            if (RPM_RADIO_TYPE_LTE == event_ptr->radioType) {
                if (RPM_SERVICE_STATE_ACTIVE == event_ptr->u.serviceState) {
                    nextState_ptr = (RFSM_State *)&_RFSM_STATE_LTE;
                }
            }
            else if (RPM_RADIO_TYPE_WIFI == event_ptr->radioType) {
                if (RPM_SERVICE_STATE_ACTIVE == event_ptr->u.serviceState) {
                    nextState_ptr = (RFSM_State *)&_RFSM_STATE_WIFI;
                }
            }
            else if (RPM_RADIO_TYPE_CS == event_ptr->radioType) {
                if (RPM_SERVICE_STATE_ACTIVE == event_ptr->u.serviceState) {
                    nextState_ptr = (RFSM_State *)&_RFSM_STATE_CS;
                }
            }
            else if (RPM_RADIO_TYPE_LTE_EMERGENCY == event_ptr->radioType) {
                if (RPM_SERVICE_STATE_ACTIVE == event_ptr->u.serviceState) {
                    /*
                     * PS emergency service is up, set emergnecy radio infc
                     * to RPM_RADIO_TYPE_LTE_EMERGENCY.
                     */
                    context_ptr->emergencyRadioInfc.radioType =
                            RPM_RADIO_TYPE_LTE_EMERGENCY;
                }
                else {
                    /* Otherwise emergency call goes to CS */
                    context_ptr->emergencyRadioInfc.radioType =
                            RPM_RADIO_TYPE_CS;
                }
            }
            break;
        case RPM_EVENT_TYPE_EMERGENCY_CALL:
        case RPM_EVENT_TYPE_SRVCC_START:
        case RPM_EVENT_TYPE_SRVCC_FAIL:
        default:
            OSAL_logMsg("%s:%d Invalid RPM event\n", __FUNCTION__, __LINE__);
            break;
    }
    _RFSM_setState(context_ptr, nextState_ptr, event_ptr);
}

/*
 * ======== _stateEnter ========
 * Perform actions needed when entering this state.
 *
 * Return Values:
 * none
 */
static void _stateEnter(
    RFSM_Context *context_ptr,
    RPM_Event    *event_ptr)
{
    /* Clear all states */
    context_ptr->csState  = RPM_SERVICE_STATE_INACTIVE;
    context_ptr->lteState = RPM_SERVICE_STATE_INACTIVE;
    /* Set up radio interface for No Radio */
    context_ptr->radioInfc.radioType = RPM_RADIO_TYPE_NONE;
    /* Update emergency radio infc */
    context_ptr->emergencyRadioInfc.radioType = RPM_RADIO_TYPE_NONE;
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const RFSM_State _RFSM_STATE_NONE = {
    _processEvent, /* Process events */
    _stateEnter,   /* State enter */
    NULL,          /* Nothing to do when exiting state */
    _stateName     /* The name of this state */
};
