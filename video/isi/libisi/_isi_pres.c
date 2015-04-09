/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_pres.h"

/* 
 * ======== _ISIR_sendNextPresence ========
 * This function is called everytime a presence transaction completes.
 * The function will check and see if there are other presence transactions
 * waiting to be sent, if there are then this function will stimulate the next 
 * presence transaction.
 *
 * Returns: 
 *  nothing
 */
static void _ISIR_sendNextPresence(
    ISID_ServiceId *service_ptr)
{
    ISID_PresId  *pres_ptr;
    
    /* 
     * Send any active presence messages that were waiting for 
     * a previous presence message to complete.
     */
    if (ISID_presGetViaServiceIdAndState(service_ptr, &pres_ptr, 
            ISID_PRES_STATE_ACTIVE) == ISI_RETURN_OK) {
        /* There's an active presence message that is waiting to be sent */
        ISIQ_writeProtocolQueue(&pres_ptr->isiMsg);
    }
}

/* 
 * ======== ISIR_protoMsg() ========
 * This function is the FSM entry point of commands related to 'presence'.
 * transactions.  Presence transactions are serialized before going to 
 * the underlying protocol.  Meaning, before a presence signal is sent to the 
 * underlying protocol, the ISI will queue the transaction if there is already
 * a transaction currently is progress. Once a presence transaction completes
 * the ISI will see if there are anymore transaction waiting to be sent.
 * If there are the ISI will send the next presence transaction in the queue.
 *
 * Returns: 
 *  nothing
 */
void ISIR_protoMsg(
    ISI_Id        presId, 
    ISIP_Message *msg_ptr)
{
    /* 
     * We just received a response to a presence
     * message or we have a new incoming presence message
     */

    ISIP_PresReason   reason;
    ISID_ServiceId   *service_ptr;
    ISI_EventMessage  event;
    ISIP_Presence    *m_ptr;
    ISID_PresId      *pres_ptr;
    ISIP_Presence    *p_ptr;
    ISI_Return        ret; 
    ISID_PresState    state;
    
    m_ptr           = &msg_ptr->msg.presence;
    reason          = m_ptr->reason;

    if (ISID_lockPres() != ISI_RETURN_OK) {
        return;
    }
    
    if ((presId == 0) && (reason == ISIP_PRES_REASON_PRESENCE || 
                reason == ISIP_PRES_REASON_CAPABILITIES ||
                reason == ISIP_PRES_REASON_CAPABILITIES_REQUEST ||
                reason == ISIP_PRES_REASON_CONTACT_ADD ||
                reason == ISIP_PRES_REASON_CONTACT_RM ||
                reason == ISIP_PRES_REASON_SUB_TO_PRES ||
                reason == ISIP_PRES_REASON_UNSUB_TO_PRES ||
                reason == ISIP_PRES_REASON_SUB_ALLOWED ||
                reason == ISIP_PRES_REASON_SUB_REFUSED)) {
        /* Then this is a new presence message, so create it */
        ret = ISID_serviceGet(m_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            OSAL_logMsg("ServiceGet() failed to find an active service");

            /* This service ID does not exist or is not active */
            ISID_unlockPres();
            return;
        }
        /* All is well so create the presence object */
        if ((pres_ptr = ISID_presCreate()) == NULL) {
            OSAL_logMsg("ISID_presCreate() failed to create a presence object");
            /* No resource to deliver so just return */
            ISID_unlockPres();
            return;
        }
        p_ptr = &pres_ptr->isiMsg.msg.presence;

        /* Set whether or not there is a chat associated with this presence */
        pres_ptr->chatId = m_ptr->chatId;

        pres_ptr->service_ptr = service_ptr;
        pres_ptr->presLen = OSAL_strlen(m_ptr->presence);
        
        OSAL_logMsg("presence string: %s\n", m_ptr->presence);


        /* Copy the ISIP_Presence */
        OSAL_memCpy(p_ptr, m_ptr, sizeof(ISIP_Presence));
        
        if (ISID_presAdd(pres_ptr, &presId) != ISI_RETURN_OK) {
            /* exceeded all available resources */
            ISI_free(pres_ptr, ISI_OBJECT_PRES_ID);
            ISID_unlockPres();
            return;
        }
    }
    else {
        /* find the presence in the database */
        if (ISID_presGet(presId, &pres_ptr) != ISI_RETURN_OK) {
            /* Then this non-zero textId is invalid */
            ISID_unlockPres();
            return;
        }
    }

    state = pres_ptr->state;
    
    event.serviceId = pres_ptr->service_ptr->e.desc;
    event.id        = presId;
    event.idType    = ISI_ID_TYPE_PRESENCE;
    event.event     = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != m_ptr->reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, m_ptr->reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    /* Cache the service_ptr */
    service_ptr = pres_ptr->service_ptr;

    /* Run the state machine */
    switch (reason) {
    case ISIP_PRES_REASON_PRESENCE:
        /* If it for a chat then it must be a 'group chat' */
        if (0 < pres_ptr->chatId) {
            event.event = ISI_EVENT_GROUP_CHAT_PRES_RECEIVED;
        }
        else {
            event.event = ISI_EVENT_PRES_RECEIVED;
        }
        state = ISID_PRES_STATE_WAITING;
        break;
    case ISIP_PRES_REASON_CAPABILITIES:
        event.event = ISI_EVENT_PRES_CAPS_RECEIVED;
        state = ISID_PRES_STATE_WAITING;
        break;
    case ISIP_PRES_REASON_CONTACT_ADD:
    case ISIP_PRES_REASON_CONTACT_RM:
        event.event = ISI_EVENT_CONTACT_RECEIVED;
        state = ISID_PRES_STATE_WAITING;
        break;
    case ISIP_PRES_REASON_SUB_ALLOWED:
    case ISIP_PRES_REASON_SUB_REFUSED:
        event.event = ISI_EVENT_SUBSCRIPTION_RESP_RECEIVED;
        state = ISID_PRES_STATE_WAITING;
        break;
    case ISIP_PRES_REASON_UNSUB_TO_PRES:
    case ISIP_PRES_REASON_SUB_TO_PRES:
        event.event = ISI_EVENT_SUB_TO_PRES_RECEIVED;
        state = ISID_PRES_STATE_WAITING;
        break;
    case ISIP_PRES_REASON_COMPLETE:
        switch (pres_ptr->reason) {
        case ISIP_PRES_REASON_CONTACT_ADD:
        case ISIP_PRES_REASON_CONTACT_RM:
            event.event = ISI_EVENT_CONTACT_SEND_OK;
            break;
        case ISIP_PRES_REASON_SUB_TO_PRES:
        case ISIP_PRES_REASON_UNSUB_TO_PRES:
            event.event = ISI_EVENT_SUB_TO_PRES_SEND_OK;
            break;
        case ISIP_PRES_REASON_SUB_ALLOWED:
        case ISIP_PRES_REASON_SUB_REFUSED:
            event.event = ISI_EVENT_SUBSCRIPTION_RESP_SEND_OK;
            break;
        case ISIP_PRES_REASON_PRESENCE:
        default:
            event.event = ISI_EVENT_PRES_SEND_OK;
            break;
        }
        ISID_presDestroy(presId);
        pres_ptr = NULL;
        /* See if there are other presence messages waiting to be sent */
        _ISIR_sendNextPresence(service_ptr);
        break;
    case ISIP_PRES_REASON_INVALID:
    case ISIP_PRES_REASON_ERROR:
        /* Switch on the original reason this error was generated */
        switch (pres_ptr->reason) {
        case ISIP_PRES_REASON_CONTACT_ADD:
        case ISIP_PRES_REASON_CONTACT_RM:
            event.event = ISI_EVENT_CONTACT_SEND_FAILED;
            break;
        case ISIP_PRES_REASON_SUB_TO_PRES:
        case ISIP_PRES_REASON_UNSUB_TO_PRES:
            event.event = ISI_EVENT_SUB_TO_PRES_SEND_FAILED;
            break;
        case ISIP_PRES_REASON_SUB_ALLOWED:
        case ISIP_PRES_REASON_SUB_REFUSED:
            event.event = ISI_EVENT_SUBSCRIPTION_RESP_SEND_FAILED;
            break;
        case ISIP_PRES_REASON_PRESENCE:
        default:
            event.event = ISI_EVENT_PRES_SEND_FAILED;
            break;
        }
        ISID_presDestroy(presId);
        pres_ptr = NULL;
        /* See if there are other presence messages waiting to be sent */
        _ISIR_sendNextPresence(service_ptr);
        break;
    default:
        break;
    }
    if (event.event != ISI_EVENT_NONE) {
        /* Then send it to the app queue */
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }
    /*
     ****************************************
     * NOTE: You don't have to free msg_ptr, 
     * because it came from .bss and not heap
     **************************************** 
     */
    if (pres_ptr) {
        pres_ptr->state = (ISID_PresState)state;
    }
    ISID_unlockPres();
}

/* 
 * ======== ISIR_appMsg() ========
 * This function is the FSM entry point of commands related to 'presence'.
 * transactions. These commands come as a result of an API call.
 *
 * Currently this function contains no true FSM, it simply passes
 * the commands downstream to the underlying protocol.
 *
 * Returns: 
 *  Nothing
 */
void ISIR_appMsg(
    ISID_PresId  *pres_ptr, 
    ISIP_Message *msg_ptr)
{
    ISID_PresId *p_ptr;
    ISI_Return   ret;
    
    /* 
     * First see if there is any other presence messages that are active
     * for this service. If so, then we don't send to the protocol, rather
     * we mark the state as active and keep it queued until the currently
     * active message complete 
     */

    ret = ISID_presGetViaServiceIdAndState(pres_ptr->service_ptr, 
            &p_ptr, ISID_PRES_STATE_ACTIVE);
    
    pres_ptr->state = ISID_PRES_STATE_ACTIVE;
    
    if (ret != ISI_RETURN_OK) {
        /* There is no active message, so send to the protocol */
        ISIQ_writeProtocolQueue(msg_ptr);
    }
    return;
}

