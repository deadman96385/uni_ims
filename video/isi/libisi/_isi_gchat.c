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
#include "_isi_gchat.h"
#include "_isi_text.h"
#include "_isi_dbg.h"


static void _ISIG_updateContributionId(
    ISIP_Chat     *c_ptr,
    ISID_GChatId  *chat_ptr)
{
    if (0 != c_ptr->contributionId[0]) {
        /* append the contribution ID to the URI */
        OSAL_strncpy(chat_ptr->szContributionId, c_ptr->contributionId,
                ISI_ADDRESS_STRING_SZ);
    }
}

static void _ISIG_updateRemoteAddress(
    ISIP_Chat     *c_ptr,
    ISID_GChatId  *chat_ptr)
{
    if (0 != c_ptr->remoteAddress[0] &&
            0 != OSAL_strcmp(c_ptr->remoteAddress, chat_ptr->szRemoteAddress)) {
        OSAL_strncpy(chat_ptr->szRemoteAddress, c_ptr->remoteAddress,
                ISI_ADDRESS_STRING_SZ);
    }
}

static void _ISIG_RejectChat(
    ISIP_Message *msg_ptr)
{
    /*
     * Re-write the msg_ptr with an error and send back
     * to service queue
     */
     ISIM_chat(msg_ptr, ISIP_CHAT_REASON_REJECT, ISIP_STATUS_INVALID);
    ISIQ_writeProtocolQueue(msg_ptr);
}


/* 
 * ======== ISIG_protoMsg() ========
 * This function is the FSM entry point of commands related to
 * group chat rooms.
 *
 * Currently the FSM only looks for events related to the state
 * of the group chat room.  Meaning, it only wants events that trigger the
 * state as being in a "active" or "inactive" state.
 *
 * Returns: 
 *  nothing
 */
void ISIG_protoMsg(
    ISI_Id        chatId,
    ISIP_Message *msg_ptr)
{
    ISID_GChatId        *chat_ptr;
    ISIP_Chat           *c_ptr;
    ISI_ChatState        state;
    ISIP_ChatReason      reason;
    ISI_EventMessage     event;
    ISID_ServiceId      *service_ptr;
    ISI_Return           ret;
    char                *localIdentity_ptr;
    char                 conferenceUri[ISI_LONG_ADDRESS_STRING_SZ];

    c_ptr           = &msg_ptr->msg.groupchat;
    reason          = c_ptr->reason;

    if (ISID_lockGChats() != ISI_RETURN_OK) {
        return;
    }

    if (reason == ISIP_GROUP_CHAT_REASON_INITIATE ||
            reason == ISIP_CHAT_REASON_INITIATE ||
            reason == ISIP_DEFERRED_CHAT_REASON_INITIATE) {
        /* Then this is a new group chat, so create it. */

        ret = ISID_serviceGet(c_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            /* This service ID does not exist or has not been activated. */
            ISIG_log((char *)__FUNCTION__, "Failed to find a active service\n",
                    0, 0 ,0);
            _ISIG_RejectChat(msg_ptr);
            ISID_unlockGChats();
            return;
        }
        else {
            OSAL_strncpy(conferenceUri, c_ptr->remoteAddress, ISI_LONG_ADDRESS_STRING_SZ);
            if (reason == ISIP_GROUP_CHAT_REASON_INITIATE) {
                /* Then let's use our nickname as our identity. */
                localIdentity_ptr = service_ptr->szNickname;
                if (0 != c_ptr->contributionId[0]) {
                    OSAL_snprintf(conferenceUri, ISI_LONG_ADDRESS_STRING_SZ, "%s;%s=%s", 
                    c_ptr->remoteAddress, ISI_CONTRIBUTION_ID_PARAM_STR, c_ptr->contributionId);
                }
            }
            else {
                /* Otherwise use our URI */
                localIdentity_ptr = service_ptr->szUri;
            }
            /* All is well so create the group chat room */
            if (NULL == (chat_ptr = ISID_gchatCreate(
                    service_ptr, conferenceUri, c_ptr->remoteAddress,
                    localIdentity_ptr, c_ptr->subject, c_ptr->participants,
                    c_ptr->passwordRequired))) {
                ISIG_log((char *)__FUNCTION__,
                        "Failed to create a new group chat room\n", 0, 0 ,0);
                _ISIG_RejectChat(msg_ptr);
                ISID_unlockGChats();
                return;
            }
            /* Add the call to the database.
             * Note that the callId value was set to the ST UA handle */
            if (ISI_RETURN_FAILED == ISID_gchatAdd(chat_ptr, &chatId)) {
                /* There's an error so free the memory and simply return */
                ISIG_log((char *)__FUNCTION__,
                        "Failed to add a new group chat to the database\n", 0, 0 ,0);
                _ISIG_RejectChat(msg_ptr);
                ISI_free(chat_ptr, ISI_OBJECT_GCHAT_ID);
                ISID_unlockGChats();
                return;
            }
            /* If there's a first message associated with this event then add it to the message DB. */
            if (ISIP_TEXT_REASON_NEW_NO_EVENT == c_ptr->text.reason) {
                c_ptr->text.chatId = chatId;
                ISIT_chatMsg(service_ptr, &c_ptr->text, &chat_ptr->firstMessageId);
            }
        }
    }
    else {
        if (ISI_RETURN_OK != ISID_gchatGet(chatId, &chat_ptr)) {
            /* chat ID is not valid */
            /* ISIG_log((char *)__FUNCTION__,
                    "Failed to find the group chat for this event\n", 0, 0 ,0);
             */
            ISID_unlockGChats();
            return;
        }
    }

    state = chat_ptr->state;
    event.id = chat_ptr->e.desc;
    event.serviceId = chat_ptr->service_ptr->e.desc;
    event.idType    = ISI_ID_TYPE_CHAT;
    event.event     = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != c_ptr->reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, c_ptr->reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }
        
    ISIG_log((char *)__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logChat(chatId, state, reason, c_ptr->serviceId);

    if (ISI_CHAT_STATE_INITIATING == state) {
        switch (reason) {
            case ISIP_GROUP_CHAT_REASON_DESTROY:
            case ISIP_CHAT_REASON_REJECT:
            case ISIP_GROUP_CHAT_REASON_KICK:
            case ISIP_CHAT_REASON_FAILED:
            case ISIP_CHAT_REASON_TERMINATE:
                event.event = ISI_EVENT_CHAT_DISCONNECTED;
                chat_ptr->state = ISI_CHAT_STATE_INVALID;
                ISID_gchatDestroy(chatId);
                break;
            case ISIP_GROUP_CHAT_REASON_NET:
                /* Then the group chat room has a network error */
                event.event = ISI_EVENT_CHAT_FAILED;
                chat_ptr->state = ISI_CHAT_STATE_INVALID;
                ISID_gchatDestroy(chatId);
                break;
            case ISIP_GROUP_CHAT_REASON_AUTH:
                /* Then the group chat room has a credentials error */
                event.event = ISI_EVENT_GROUP_CHAT_NOT_AUTHORIZED;
                chat_ptr->state = ISI_CHAT_STATE_INVALID;
                ISID_gchatDestroy(chatId);
                break;
            case ISIP_CHAT_REASON_ACCEPT:
                /*
                 * The remote address needs to be updated if it's something different.
                 * This is because when making an Adhoc group chat we will use
                 * the conference factory as the remote address at first but once the
                 * group chat is established then the remote address will be the conference URI
                 * as assigned by the conference factory.
                 */
                _ISIG_updateRemoteAddress(c_ptr, chat_ptr);
                _ISIG_updateContributionId(c_ptr, chat_ptr);
                event.event = ISI_EVENT_CHAT_ACCEPTED;
                chat_ptr->state = ISI_CHAT_STATE_ACTIVE;
                break;
            case ISIP_CHAT_REASON_TRYING:
                event.event = ISI_EVENT_CHAT_TRYING;
                break;
            case ISIP_CHAT_REASON_ACK:
                event.event = ISI_EVENT_CHAT_ACKNOWLEDGED;
                break;
            case ISIP_DEFERRED_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_INITIATE:
            case ISIP_GROUP_CHAT_REASON_INITIATE:
            case ISIP_GROUP_CHAT_REASON_JOIN:
            case ISIP_GROUP_CHAT_REASON_INVITE:
            case ISIP_CHAT_REASON_INVALID:
            case ISIP_CHAT_REASON_LAST:
            default:
                break;
        }
    }
    else if (ISI_CHAT_STATE_ACTIVE == state) {
        switch (reason) {
            case ISIP_GROUP_CHAT_REASON_DESTROY:
            case ISIP_CHAT_REASON_REJECT:
            case ISIP_GROUP_CHAT_REASON_KICK:
            case ISIP_CHAT_REASON_TERMINATE:
            case ISIP_CHAT_REASON_FAILED:
                event.event = ISI_EVENT_CHAT_DISCONNECTED;
                chat_ptr->state = ISI_CHAT_STATE_INVALID;
                ISID_gchatDestroy(chatId);
                break;
            case ISIP_GROUP_CHAT_REASON_NET:
                /* Then the group chat room has a network error */
                event.event = ISI_EVENT_CHAT_FAILED;
                chat_ptr->state = ISI_CHAT_STATE_INVALID;
                ISID_gchatDestroy(chatId);
                break;
            case ISIP_GROUP_CHAT_REASON_AUTH:
                /* Then the group chat room has a credentials error */
                event.event = ISI_EVENT_GROUP_CHAT_NOT_AUTHORIZED;
                chat_ptr->state = ISI_CHAT_STATE_INVALID;
                ISID_gchatDestroy(chatId);
                break;
            case ISIP_DEFERRED_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_TRYING:
            case ISIP_CHAT_REASON_ACK:
            case ISIP_GROUP_CHAT_REASON_INITIATE:
            case ISIP_GROUP_CHAT_REASON_JOIN:
            case ISIP_GROUP_CHAT_REASON_INVITE:
            case ISIP_CHAT_REASON_ACCEPT:
            case ISIP_CHAT_REASON_INVALID:
            case ISIP_CHAT_REASON_LAST:
            default:
                break;
        }
    }
    else if (ISI_CHAT_STATE_INVALID == state) {
        switch (reason) {
            case ISIP_GROUP_CHAT_REASON_INITIATE:
                /* Then it's a new group chat invite */
                event.event = ISI_EVENT_GROUP_CHAT_INCOMING;
                chat_ptr->state = ISI_CHAT_STATE_INITIATING;
                break;
            case ISIP_CHAT_REASON_INITIATE:
                event.event = ISI_EVENT_CHAT_INCOMING;
                chat_ptr->state = ISI_CHAT_STATE_INITIATING;
                break;
            case ISIP_DEFERRED_CHAT_REASON_INITIATE:
                event.event = ISI_EVENT_DEFERRED_CHAT_INCOMING;
                chat_ptr->state = ISI_CHAT_STATE_INITIATING;
                break;
            case ISIP_CHAT_REASON_TRYING:
            case ISIP_CHAT_REASON_ACK:
            case ISIP_CHAT_REASON_FAILED:
            case ISIP_CHAT_REASON_TERMINATE:
            case ISIP_CHAT_REASON_ACCEPT:
            case ISIP_CHAT_REASON_REJECT:
            case ISIP_GROUP_CHAT_REASON_INVITE:
            case ISIP_GROUP_CHAT_REASON_KICK:
            case ISIP_GROUP_CHAT_REASON_DESTROY:
            case ISIP_GROUP_CHAT_REASON_JOIN:
            case ISIP_GROUP_CHAT_REASON_NET:
            case ISIP_GROUP_CHAT_REASON_AUTH:
            case ISIP_CHAT_REASON_INVALID:
            case ISIP_CHAT_REASON_LAST:
            default:
                break;
        }
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
    ISID_unlockGChats();
}

/* 
 * ======== ISIG_appMsg() ========
 * This function is the FSM entry point of commands related to 
 * group chat rooms. These commands come as a result of an API call.
 *
 * Returns: 
 *   ISI_RETURN_OK : always..
 */
ISI_Return ISIG_appMsg(
    ISID_GChatId *chat_ptr,
    ISIP_Message *msg_ptr)
{
    ISIP_ChatReason reason;
    ISI_ChatState   state;
    ISI_Return           ret = ISI_RETURN_FAILED;
    
    reason = msg_ptr->msg.groupchat.reason;
    
    ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logChat(msg_ptr->id, chat_ptr->state, reason,
        chat_ptr->service_ptr->e.desc);

    state = chat_ptr->state;

    if (ISI_CHAT_STATE_INITIATING == state) {
        switch (reason) {
            case ISIP_GROUP_CHAT_REASON_DESTROY:
            case ISIP_CHAT_REASON_TERMINATE:
            case ISIP_CHAT_REASON_REJECT:
                ret = ISI_RETURN_OK;
                break;
            case ISIP_CHAT_REASON_ACK:
            case ISIP_CHAT_REASON_ACCEPT:
                ret = ISI_RETURN_OK;
                break;
            case ISIP_DEFERRED_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_TRYING:
            case ISIP_CHAT_REASON_FAILED:
            case ISIP_GROUP_CHAT_REASON_INITIATE:
            case ISIP_GROUP_CHAT_REASON_JOIN:
            case ISIP_GROUP_CHAT_REASON_INVITE:
            case ISIP_GROUP_CHAT_REASON_KICK:
            case ISIP_GROUP_CHAT_REASON_NET:
            case ISIP_GROUP_CHAT_REASON_AUTH:
            case ISIP_CHAT_REASON_INVALID:
            case ISIP_CHAT_REASON_LAST:
            default:
                break;
        }
    }
    else if (ISI_CHAT_STATE_ACTIVE == state) {
        switch (reason) {
            case ISIP_GROUP_CHAT_REASON_INVITE:
                ret = ISI_RETURN_OK;
                break;
            case ISIP_CHAT_REASON_TERMINATE:
                ret = ISI_RETURN_OK;
                break;
            case ISIP_GROUP_CHAT_REASON_KICK:
                ret = ISI_RETURN_OK;
                break;
            case ISIP_GROUP_CHAT_REASON_DESTROY:
                ret = ISI_RETURN_OK;
                break;
            case ISIP_DEFERRED_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_TRYING:
            case ISIP_CHAT_REASON_ACK:
            case ISIP_CHAT_REASON_FAILED:
            case ISIP_GROUP_CHAT_REASON_INITIATE:
            case ISIP_GROUP_CHAT_REASON_JOIN:
            case ISIP_CHAT_REASON_ACCEPT:
            case ISIP_CHAT_REASON_REJECT:
            case ISIP_GROUP_CHAT_REASON_NET:
            case ISIP_GROUP_CHAT_REASON_AUTH:
            case ISIP_CHAT_REASON_INVALID:
            case ISIP_CHAT_REASON_LAST:
            default:
                break;
        }
    }
    else if (ISI_CHAT_STATE_INVALID == state) {
        switch (reason) {
            case ISIP_CHAT_REASON_INITIATE:
            case ISIP_GROUP_CHAT_REASON_JOIN:
            case ISIP_GROUP_CHAT_REASON_INITIATE:
                /* Then it's a new group chat invite */
                chat_ptr->state = ISI_CHAT_STATE_INITIATING;
                ret = ISI_RETURN_OK;
                break;
            case ISIP_DEFERRED_CHAT_REASON_INITIATE:
            case ISIP_CHAT_REASON_TRYING:
            case ISIP_CHAT_REASON_ACK:
            case ISIP_CHAT_REASON_FAILED:
            case ISIP_GROUP_CHAT_REASON_INVITE:
            case ISIP_CHAT_REASON_ACCEPT:
            case ISIP_CHAT_REASON_REJECT:
            case ISIP_GROUP_CHAT_REASON_KICK:
            case ISIP_GROUP_CHAT_REASON_DESTROY:
            case ISIP_GROUP_CHAT_REASON_NET:
            case ISIP_GROUP_CHAT_REASON_AUTH:
            case ISIP_CHAT_REASON_LAST:
            case ISIP_CHAT_REASON_INVALID:
            default:
                break;
        }
    }

    if (ret == ISI_RETURN_OK) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(msg_ptr);
    }
    ISIM_free(msg_ptr);
    return (ret);
}

