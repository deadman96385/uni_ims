/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 27660 $ $Date: 2014-07-21 13:26:30 +0800 (Mon, 21 Jul 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_evt.h"

/* 
 * ======== _ISIE_sendNextEvent() ========
 * This function is called everytime an event transaction completes.
 * The function will check and see if there are other events waiting
 * to be sent, if there are then this function will stimulate the next 
 * event transaction.
 *
 * Returns: 
 *  nothing
 */
static void _ISIE_sendNextEvent(
    ISID_ServiceId *service_ptr,
    ISID_CallId    *call_ptr)
{
    ISID_EvtId    *evt_ptr;
    
    /* Send active events that are waiting for a previous event to complete */
    if (NULL == call_ptr) {
        /* Then look for the next event associated with the service */
        if (ISID_evtGetViaServiceIdAndState(service_ptr, &evt_ptr, 
                ISID_EVENT_STATE_ACTIVE) == ISI_RETURN_OK) {
            /* Then there was an active event waiting to be sent */
            ISIQ_writeProtocolQueue(&evt_ptr->isiMsg);
        }
    }
    else {
        /* Then look for the next event associated with a call */
        if (ISID_evtGetViaCallIdAndState(call_ptr, &evt_ptr, 
                ISID_EVENT_STATE_ACTIVE) == ISI_RETURN_OK) {
            /* Then there was an active event waiting to be sent */
            ISIQ_writeProtocolQueue(&evt_ptr->isiMsg);
        }
    }
}

/* 
 * ======== ISIE_protoMsg() ========
 * This function is the FSM entry point of commands related to events.
 * (a.k.a. telephone events, etc.).  Events are serialized before going to 
 * the underlying protocol.  Meaning, before an event is sent to the 
 * underlying protocol, the ISI will queue the events if there is already a
 * event transaction currently is progress. Once an event completes
 * the ISI will see if there are anymore events waiting to be sent.
 * If there are the ISI will send the next event in the queue.
 *
 * Returns: 
 *  nothing
 */
void ISIE_protoMsg(
    ISI_Id        evtId, 
    ISIP_Message *msg_ptr)
{
    ISIP_TelEvent    *e_ptr;
    ISID_CallId      *call_ptr;
    ISID_EvtId       *evt_ptr;
    ISI_EventMessage  event;
    ISIP_TelEvtReason reason;
    ISI_Return        ret;
    ISID_ServiceId   *service_ptr;
    
    if (ISID_lockEvts() != ISI_RETURN_OK) {
        return;
    }

    e_ptr   = &msg_ptr->msg.event;
    reason  = e_ptr->reason;
    
    if (evtId == 0 && reason == ISIP_TEL_EVENT_REASON_NEW) {
        /* Then this is a new tel event, so create it */
        ret = ISID_serviceGet(e_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            /* This service ID does not exist or is not active */
            ISID_unlockEvts();
            return;
        }
        
        /* All is well so create the tel event */
        if ((evt_ptr = ISID_evtCreate()) == NULL) {
            /* No resource to deliver so just return */
            ISID_unlockEvts();
            return;
        }

        /* Determinate whether or not it's inside the context of a call */
        call_ptr = NULL;
        ISID_callGet(e_ptr->callId, &call_ptr);
        evt_ptr->call_ptr = call_ptr;
        evt_ptr->service_ptr = service_ptr;
        /* 
         * Copy the ISIP_TelEvent part of the ISI event. We need it when the 
         * user retrieves the ISI event.
         */
        OSAL_memCpy(&evt_ptr->isiMsg.msg.event, e_ptr, sizeof(ISIP_TelEvent));
        
        if (ISID_evtAdd(evt_ptr, &evtId) != ISI_RETURN_OK) {
            /* exceeded all available resources */
            ISI_free(evt_ptr, ISI_OBJECT_EVT_ID);
            ISID_unlockEvts();
            return;
        }
    }
    else {
        /* find the tel event in the database */
        if (ISID_evtGet(evtId, &evt_ptr) != ISI_RETURN_OK) {
            /* Then this non-zero evtId is invalid */
            ISID_unlockEvts();
            return;
        }
    }

    /* Cache the call pointer */
    call_ptr = evt_ptr->call_ptr;
    service_ptr = evt_ptr->service_ptr;
    
    event.serviceId = service_ptr->e.desc;
    event.id        = evtId;
    event.idType    = ISI_ID_TYPE_TEL_EVENT;
    event.event     = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != msg_ptr->msg.event.reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, msg_ptr->msg.event.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    switch (e_ptr->reason) {
        case ISIP_TEL_EVENT_REASON_COMPLETE:
            event.event = ISI_EVENT_TEL_EVENT_SEND_OK;
            ISID_evtDestroy(evtId);
            _ISIE_sendNextEvent(service_ptr, call_ptr);
            break;
        case ISIP_TEL_EVENT_REASON_INVALID:
        case ISIP_TEL_EVENT_REASON_ERROR:
            event.event = ISI_EVENT_TEL_EVENT_SEND_FAILED;
            ISID_evtDestroy(evtId);
            _ISIE_sendNextEvent(service_ptr, call_ptr);
            break;
        case ISIP_TEL_EVENT_REASON_NEW:
            event.event = ISI_EVENT_TEL_EVENT_RECEIVED;
            break;
        default:
            break;
    }

    if (event.event != ISI_EVENT_NONE) {
        /* Then send it to the app queue */
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }
    /* **************************************
     * NOTE: You don't have to free msg_ptr, 
     * because it came from .bss and not heap
     **************************************** 
     */
    ISID_unlockEvts();
}

/* 
 * ======== ISIE_appMsg() ========
 * This function is the FSM entry point of commands related to 'events'.
 * (a.k.a. telephone events, etc). These commands come as a result of an API 
 * call.
 *
 * Returns: 
 *  Nothing
 */
void ISIE_appMsg(
    ISID_EvtId   *evt_ptr, 
    ISIP_Message *msg_ptr)
{
    ISID_EvtId *e_ptr;
    ISI_Return  ret; 
    
    if (NULL == evt_ptr->call_ptr) {
        /* Then this event is not happening within a call, rather a service */
        
        /* 
         * First see if there is any other service events that are active for
         * this service. If so, then we don't send to the protocol, rather we 
         * mark the state as active and keep it queued until the currently
         * active event completes
         */

        ret = ISID_evtGetViaServiceIdAndState(evt_ptr->service_ptr, 
                &e_ptr, ISID_EVENT_STATE_ACTIVE);
        
        evt_ptr->state = ISID_EVENT_STATE_ACTIVE;
        
        if (ISI_RETURN_OK != ret) {
            /* Then there are no active events, so send to the protocol */
            ISIQ_writeProtocolQueue(msg_ptr);
        }
    }
    else {
        /* Then it's an event that belongs to a particular call */
        
        /* 
         * First see if there is any other events are active.
         * If so, then we don't send to the protocol, rather we mark
         * the state as active and keep it queued until the currently
         * active event completes. 
         */
        ret = ISID_evtGetViaCallIdAndState(evt_ptr->call_ptr, &e_ptr, 
                ISID_EVENT_STATE_ACTIVE);

        evt_ptr->state = ISID_EVENT_STATE_ACTIVE;

        if (ret != ISI_RETURN_OK) {
            /* Nothing else is pending so update state and send it*/
            ISIQ_writeProtocolQueue(msg_ptr);
        }
    }
    return;
}

