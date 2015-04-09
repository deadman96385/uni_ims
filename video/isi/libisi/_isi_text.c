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
#include "_isi_text.h"

/* 
 * ======== _ISIT_sendNextMessage() ========
 * This function is called everytime a text message transaction completes.
 * The function will check and see if there are other text messages waiting
 * to be sent, if there are then this function will stimulate the next 
 * text message transaction.
 *
 * Returns: 
 *  nothing
 */
static void _ISIT_sendNextMessage(
    ISID_ServiceId *service_ptr)
{
    /* 
     * Send any active messages that were waiting for 
     * a previous message to complete 
     */
    ISID_TextId  *text_ptr;
    
    /* Then look for a text message that is associated with a service only */
    if (ISID_textGetViaServiceIdAndState(service_ptr, &text_ptr, 
            ISID_TEXT_STATE_ACTIVE) == ISI_RETURN_OK) {
        /* Then there was an active text message that was waiting to be sent */
        ISIQ_writeProtocolQueue(&text_ptr->isiMsg);
    }
}

/* 
 * ======== ISIT_protoMsg() ========
 * This function is the FSM entry point of commands related to 
 * 'text messaging' (a.k.a. Instant Messages or IM).  Text messages are 
 * serialized before going to the underlying protocol.  Meaning, before 
 * a text message is sent to the underlying protocol, the ISI will 
 * wait and queue text messages until the previous text message 
 * completes.
 *
 * Returns: 
 *  nothing
 */
void ISIT_protoMsg(
    ISI_Id        textId, 
    ISIP_Message *msg_ptr)
{
    /* We just received a response to a text message 
     * or we have a new text message
     */

    ISIP_TextReason  reason;
    ISID_ServiceId   *service_ptr;
    ISI_EventMessage  event;
    ISIP_Text        *m_ptr;
    ISID_TextId      *t_ptr;
    ISI_Return        ret;
    ISID_TextState    state;
    
    m_ptr           = &msg_ptr->msg.message;
    reason          = m_ptr->reason;
    
    if (ISID_lockTexts() != ISI_RETURN_OK) {
        return;
    }

    if ((textId == 0 && reason == ISIP_TEXT_REASON_NEW) ||
            (textId == 0 && reason == ISIP_TEXT_REASON_REPORT) ||
            (textId == 0 && reason == ISIP_TEXT_REASON_COMPOSING_IDLE) ||
            (textId == 0 && reason == ISIP_TEXT_REASON_COMPOSING_ACTIVE)) {
        /* Then this is a new text message, so create it */
        ret = ISID_serviceGet(m_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            /* This service ID does not exist or is not active */
            ISID_unlockTexts();
            return;
        }
        /* All is well so create the text message */
        if ((t_ptr = ISID_textCreate()) == NULL) {
            /* No resource to deliver so just return */
            ISID_unlockTexts();
            return;
        }
        /* Set whether or not there is a chat associated with this message */
        t_ptr->chatId = m_ptr->chatId;
        t_ptr->service_ptr = service_ptr;
        /* Copy the ISIP_Text portion of the message */
        OSAL_memCpy(&t_ptr->isiMsg.msg.message, m_ptr, sizeof(ISIP_Text));
        t_ptr->msgLen = m_ptr->pduLen;

        if (ISID_textAdd(t_ptr, &textId) != ISI_RETURN_OK) {
            /* exceeded all available resources */
            ISI_free(t_ptr, ISI_OBJECT_TEXT_ID);
            ISID_unlockTexts();
            return;
        }
    }
    else {
        /* find the text in the database */
        if (ISID_textGet(textId, &t_ptr) != ISI_RETURN_OK) {
            /* Then this non-zero textId is invalid */
            ISID_unlockTexts();
            return;
        }
    }

    state = t_ptr->state;
    
    event.serviceId = t_ptr->service_ptr->e.desc;
    event.id = textId;
    event.idType = ISI_ID_TYPE_MESSAGE;
    event.event = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != m_ptr->reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, m_ptr->reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    service_ptr = t_ptr->service_ptr;
    
    /* Run the state machine */
    switch (reason) {
    case ISIP_TEXT_REASON_NEW:
        event.event = ISI_EVENT_MESSAGE_RECEIVED;
        state = ISID_TEXT_STATE_WAITING;
        break;
    case ISIP_TEXT_REASON_REPORT:
        event.event = ISI_EVENT_MESSAGE_REPORT_RECEIVED;
        state = ISID_TEXT_STATE_WAITING;
        break;
    case ISIP_TEXT_REASON_COMPLETE:
        event.event = ISI_EVENT_MESSAGE_SEND_OK;
        ISID_textDestroy(textId);
        t_ptr = NULL;
        /* since we just completed a test message, 
         * see if there are others waiting to be sent 
         */
        _ISIT_sendNextMessage(service_ptr);
        break;
    case ISIP_TEXT_REASON_INVALID:
    case ISIP_TEXT_REASON_ERROR:
        event.event = ISI_EVENT_MESSAGE_SEND_FAILED;
        ISID_textDestroy(textId);
        t_ptr = NULL;
        /* since we just completed a test message, 
         * see if there are others waiting to be sent 
         */
        _ISIT_sendNextMessage(service_ptr);
        break;
    case ISIP_TEXT_REASON_COMPOSING_IDLE:
        event.event = ISI_EVENT_MESSAGE_COMPOSING_IDLE;
        state = ISID_TEXT_STATE_WAITING;
        break;
    case ISIP_TEXT_REASON_COMPOSING_ACTIVE:
        event.event = ISI_EVENT_MESSAGE_COMPOSING_ACTIVE;
        state = ISID_TEXT_STATE_WAITING;
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
    if (t_ptr) {
        t_ptr->state = state;
    }
    ISID_unlockTexts();
}

/**
 * When a chat request includes a first message, then the chat code
 * will insert the message into the text db via this routine.
 */
void ISIT_chatMsg(
    ISID_ServiceId   *service_ptr,
    ISIP_Text        *m_ptr,
    ISI_Id           *isiId_ptr)
{
    ISID_TextId      *t_ptr;

    if (m_ptr->reason == ISIP_TEXT_REASON_NEW_NO_EVENT) {
        /* Then this is a new text message, so create it */
        if ((t_ptr = ISID_textCreate()) == NULL) {
            /* No resource to deliver so just return */
            return;
        }
        /* Set whether or not there is a chat associated with this message */
        t_ptr->chatId = m_ptr->chatId;
        t_ptr->service_ptr = service_ptr;
        /* Copy the ISIP_Text portion of the message */
        OSAL_memCpy(&t_ptr->isiMsg.msg.message, m_ptr, sizeof(ISIP_Text));
        t_ptr->msgLen = OSAL_strlen(m_ptr->message);

        if (ISID_textAdd(t_ptr, isiId_ptr) != ISI_RETURN_OK) {
            /* exceeded all available resources */
            ISI_free(t_ptr, ISI_OBJECT_TEXT_ID);
            return;
        }
        t_ptr->state = ISID_TEXT_STATE_WAITING;
    }
}

/* 
 * ======== ISIT_appMsg() ========
 * This function is the FSM entry point of commands related to 'im'.
 * (a.k.a. instant messaging). These commands come as a result of an API call.
 *
 * Returns: 
 *  ISI_RETURN_OK       : The command successfully passed through the FSM
 *                        and was sent to the underlying protocol.
 *  ISI_RETURN_FAILED   : The command failed, the call was in the wrong state. 
 *                        the command and was NOT sent to the underlying 
 *                        protocol.
 */
void ISIT_appMsg(
    ISID_TextId  *text_ptr, 
    ISIP_Message *msg_ptr)
{
    ISID_TextId *t_ptr;
    ISI_Return   ret;
    
    /* 
     * First see if there is any other messages that are active for this 
     * service. If so, then we don't send to the protocol, rather we mark
     * the state as active and keep it queued until the currently
     * active message complete 
     */
    ret = ISID_textGetViaServiceIdAndState(text_ptr->service_ptr, 
                &t_ptr, ISID_TEXT_STATE_ACTIVE);
    
    text_ptr->state = ISID_TEXT_STATE_ACTIVE;
    
    if (ret != ISI_RETURN_OK) {
        /* There are no active messages, so send to the protocol */
        ISIQ_writeProtocolQueue(msg_ptr);
    }
    return;
}

