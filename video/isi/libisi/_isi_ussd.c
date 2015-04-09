/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18781 $ $Date: 2012-11-09 12:35:30 -0800 (Fri, 09 Nov 2012) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"

/* 
 * ======== _ISIT_sendNextUssd() ========
 * This function is called everytime a ussd message transaction completes.
 * The function will check and see if there are other ussd messages waiting
 * to be sent, if there are then this function will stimulate the next 
 * ussd message transaction.
 *
 * Returns: 
 *  nothing
 */
static void _ISIT_sendNextUssd(
    ISID_ServiceId *service_ptr)
{
    /* 
     * Send any active messages that were waiting for 
     * a previous message to complete 
     */
    ISID_UssdId  *ussd_ptr;
    
    /* Then look for a ussd message that is associated with a service only */
    if (ISID_ussdGetViaServiceIdAndState(service_ptr, &ussd_ptr, 
            ISID_USSD_STATE_ACTIVE) == ISI_RETURN_OK) {
        /* Then there was an active ussd message that was waiting to be sent */
        ISIQ_writeProtocolQueue(&ussd_ptr->isiMsg);
    }
}

/* 
 * ======== ISIT_protoUssd() ========
 * This function is the FSM entry point of commands related to 
 * Ussd.  Ussd messages are serialized before going to the underlying protocol.  
 * Meaning, before a ussd message is sent to the underlying protocol, the ISI will 
 * wait and queue ussd messages until the previous ussd message 
 * completes.
 *
 * Returns: 
 *  nothing
 */
void ISIT_protoUssd(
    ISI_Id        ussdId, 
    ISIP_Message *msg_ptr)
{
    ISIP_UssdReason   reason;
    ISID_ServiceId   *service_ptr;
    ISI_EventMessage  event;
    ISIP_Ussd        *m_ptr;
    ISID_UssdId      *u_ptr;
    ISI_Return        ret;
    ISID_UssdState    state;
    
    m_ptr           = &msg_ptr->msg.ussd;
    reason          = m_ptr->reason;

    /* Lock the database of Ussds and  Services*/
    if (ISID_lockUssds() != ISI_RETURN_OK) {
        return;
    }

    if ((ussdId == 0 && reason == ISIP_USSD_REASON_REQUEST) ||
            (ussdId == 0 && reason == ISIP_USSD_REASON_DISCONNECT) || 
            (ussdId == 0 && reason == ISIP_USSD_REASON_SEND_ERROR)) {
        /* Then this is a new ussd message, so create it */
        ret = ISID_serviceGet(m_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            /* This service ID does not exist or is not active */
            ISID_unlockUssds();
            return;
        }
        /* All is well so create the ussd message */
        if ((u_ptr = ISID_ussdCreate(service_ptr)) == NULL) {
            /* No resource to deliver so just return */
            ISID_unlockUssds();
            return;
        }
        u_ptr->service_ptr = service_ptr;
        /* Copy the ISIP_USSD portion of the message */
        OSAL_memCpy(&u_ptr->isiMsg.msg.ussd, m_ptr, sizeof(ISIP_Ussd));

        /* Add this ISID to the database of Ussds */
        if (ISID_ussdAdd(u_ptr, &ussdId) != ISI_RETURN_OK) {
            /* exceeded all available resources */
            ISI_free(u_ptr, ISI_OBJECT_USSD_ID);
            ISID_unlockUssds();
            return;
        }
    }
    else {
        /* find the ussd in the database */
        if (ISID_ussdGet(ussdId, &u_ptr) != ISI_RETURN_OK) {
            /* Then this non-zero ussdId is invalid */
            ISID_unlockUssds();
            return;
        }
    }

    state = u_ptr->state;
    
    event.serviceId = u_ptr->service_ptr->e.desc;
    event.id = ussdId;
    event.idType = ISI_ID_TYPE_USSD;
    event.event = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != m_ptr->reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, m_ptr->reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    service_ptr = u_ptr->service_ptr;
    
    /* Run the state machine */
    OSAL_logMsg("%s: reason=%d\n", __FUNCTION__, reason);
    switch (reason) {
    case ISIP_USSD_REASON_REQUEST:
        event.event = ISI_EVENT_USSD_REQUEST;
        state = ISID_USSD_STATE_WAITING;
        break;
    case ISIP_USSD_REASON_DISCONNECT:
        event.event = ISI_EVENT_USSD_DISCONNECT;
        state = ISID_USSD_STATE_WAITING;
        break;
    case ISIP_USSD_REASON_SEND_OK:
        event.event = ISI_EVENT_USSD_SEND_OK;
        event.id = 0;
        ISID_ussdDestroy(ussdId);
        u_ptr = NULL;
        /* since we just completed a ussd message, 
              * see if there are others waiting to be sent 
             */
        _ISIT_sendNextUssd(service_ptr);
        break;
    case ISIP_USSD_REASON_INVALID:
    case ISIP_USSD_REASON_SEND_ERROR:
        event.event = ISI_EVENT_USSD_SEND_FAILED;
        event.id = 0;
        ISID_ussdDestroy(ussdId);
        u_ptr = NULL;
        /* since we just completed a test message, 
              * see if there are others waiting to be sent 
              */
        _ISIT_sendNextUssd(service_ptr);
        break;
    default:
        break;
    }
    if (event.event != ISI_EVENT_NONE) {
        /* Then send it to the app queue */
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }
    if (u_ptr) {
        u_ptr->state = state;
    }
    ISID_unlockUssds();
}

/* 
 * ======== ISIT_appUssd() ========
 * This function is the FSM entry point of commands related to ussd.
 *
 * Returns: 
 *  ISI_RETURN_OK       : The command successfully passed through the FSM
 *                        and was sent to the underlying protocol.
 *  ISI_RETURN_FAILED   : The command failed, the call was in the wrong state. 
 *                        the command and was NOT sent to the underlying 
 *                        protocol.
 */

void ISIT_appUssd(
    ISID_UssdId  *ussd_ptr, 
    ISIP_Message *msg_ptr)
{
    ISID_UssdId *u_ptr;
    ISI_Return   ret;
    
    /* 
     * First see if there is any other messages that are active for this 
     * service. If so, then we don't send to the protocol, rather we mark
     * the state as active and keep it queued until the currently
     * active message complete 
     */
    ret = ISID_ussdGetViaServiceIdAndState(ussd_ptr->service_ptr, 
                &u_ptr, ISID_USSD_STATE_ACTIVE);
    
    ussd_ptr->state = ISID_USSD_STATE_ACTIVE;
    
    if (ret != ISI_RETURN_OK) {
        /* There are no active messages, so send to the protocol */
        ISIQ_writeProtocolQueue(msg_ptr);
    }
    return;
}


