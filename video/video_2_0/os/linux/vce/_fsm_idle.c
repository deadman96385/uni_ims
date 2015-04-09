/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#include <osal.h>
#include "_fsm.h"

static const char _stateName[] = "Codec:idle";
/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    FSM_Context_Ptr context_ptr,
    VCE_Event       event,
    vint            codecType,
    char           *eventDesc_ptr)
{
    FSM_State_Ptr  nextState_ptr = (FSM_State_Ptr)&_FSM_STATE_IDLE;

    switch (event) {
        case VCE_EVENT_INIT_COMPLETE:
            break;
        case VCE_EVENT_START_ENC:
            nextState_ptr = (FSM_State_Ptr)&_FSM_STATE_ENCODE;
            break;
        case VCE_EVENT_START_DEC:
            nextState_ptr = (FSM_State_Ptr)&_FSM_STATE_DECODE;
            break;
        case VCE_EVENT_REMOTE_RECV_BW_KBPS:
        case VCE_EVENT_SEND_KEY_FRAME:
            break;
        case VCE_EVENT_NONE:
        case VCE_EVENT_STOP_ENC:
        case VCE_EVENT_STOP_DEC: 
        default:
            OSAL_logMsg("%s:%d Invalid event\n", __FUNCTION__, __LINE__);
            break;
    }
    _FSM_setState(context_ptr, nextState_ptr, event, codecType);
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const FSM_State _FSM_STATE_IDLE = {
    _processEvent, /* Process events */
    NULL,          /* Nothing to do on state enter */
    NULL,          /* Nothing to do when exiting state */
    _stateName,    /* The name of this state */
};
