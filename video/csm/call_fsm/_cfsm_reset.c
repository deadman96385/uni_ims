/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30280 $ $Date: 2014-12-09 14:24:57 +0800 (Tue, 09 Dec 2014) $
 */
#include <osal.h>
#include <rpm.h>
#include "../_csm_event.h"
#include "../_csm_isi.h"
#include "../_csm_response.h"
#include "_cfsm.h"

static const char _stateName[] = "Call:Reset";
/*
 * ======== _processEvent ========
 * Process the incoming event using the supplied state machine context.
 */
static void _processEvent(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr)
{
    CSM_CallObject *call_ptr;
    CFSM_State_Ptr  nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_RESET;

    switch (event_ptr->reason) {
        case CSM_CALL_EVT_REASON_AT_CMD_DIAL:
        case CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE:
            if (OSAL_TRUE != CSM_isiGetServiceIsActive(
                    CSM_isiGetServiceViaId(context_ptr->isiMngr_ptr,
                    context_ptr->serviceId))) {
                CSM_sendError(30, event_ptr->reasonDesc,
                        context_ptr->csmOutput_ptr);
            }
            else {
                context_ptr->callId = 0;
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_DIALING;
            }
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL:
            context_ptr->callId = 0;
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_DIALING;
            break;
        case CSM_CALL_EVT_REASON_AT_CMD_CONF_ADHOC:
            nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_IMS_CONF;
            break;
        case CSM_CALL_EVT_REASON_NEW_INCOMING:
            context_ptr->callId = event_ptr->id;
            call_ptr = CSM_getCall(context_ptr->callId);
            /* Cache session type that remote proposed. */
            call_ptr->negSessionType = call_ptr->sessionType;
            if (CSM_CALL_STATE_INITIALIZING == context_ptr->state) {
                /* Resource is not ready, goes to initializing state */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_INITIALIZING;
            }
            else {
                /* Resource is ready, goes to ringing state */
                nextState_ptr = (CFSM_State_Ptr)&_CFSM_STATE_RINGING;
                /*
                 * Set the negExchange for reporting when state change to
                 * ringing state.
                 */
                call_ptr->negExchange = CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED;
            }
            break;
        default:
            OSAL_logMsg("%s:%d Invalid event\n", __FUNCTION__, __LINE__);
            break;
    }
    _CFSM_setState(context_ptr, nextState_ptr, event_ptr);
}

/*
 * This constant holds pointers to functions that are used to process events
 * and handle state transitions.
 */
const CFSM_State _CFSM_STATE_RESET = {
    _processEvent, /* Process events */
    NULL,          /* Nothing to do on state enter */
    NULL,          /* Nothing to do when exiting state */
    _stateName,     /* The name of this state */
};
