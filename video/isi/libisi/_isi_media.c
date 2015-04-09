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
#include "isi_errors.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_media.h"
#include "_isi_dbg.h"
#include "_isi_msg.h"
#include "_isi_call.h"

/*
 * ======== ISIM_protoMsg() ========
 * This function is the FSM entry point of commands related to audio.
 * For example, STUN Packet.  Currently there's no "real" state here.
 *
 * Returns:
 *  nothing
 */
void ISIM_protoMsg(
    ISIP_Message *msg_ptr)
{
    ISIP_MediaReason reason;
    ISID_CallId    *call_ptr;
    ISIP_Message   *callMsg_ptr;

    reason = msg_ptr->msg.media.reason;

    /* ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logAudio(reason); */

    /* Currently only STUN packets are supported for incoming AUDIO */
    switch (reason) {
    case ISIP_MEDIA_REASON_PKT_SEND:
    case ISIP_MEDIA_REASON_PKT_RECV:
        /* Forward this STUN packet */
        ISIQ_writeProtocolQueue(msg_ptr);
        break;
    case ISIP_MEDIA_REASON_RTP_RTCP_INACTIVE:
        /* Terminate the call, because RTP inactive timeout */
        if (!ISID_getState()) {
            break;
        }
        /* Lock the database of calls */
        if (ISID_lockCalls() != ISI_RETURN_OK) {
            break;
        }
        if (ISID_callGet(msg_ptr->id, &call_ptr) != ISI_RETURN_OK) {
            /* Then there is no active call */
            ISID_unlockCalls();
            break;
        }
        /* Now notify the protocol */
        if ((callMsg_ptr = ISIM_updateCall(call_ptr, ISIP_CALL_REASON_TERMINATE,
                 ISIP_STATUS_INVALID, NULL, NULL)) == NULL) {
            ISID_unlockCalls();
            break;
        }
        /* Add reason description, "RTP-RTCP Timeout" */
        OSAL_snprintf(callMsg_ptr->msg.call.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ, "%s", ISI_RTP_RTCP_TIMEOUT_STR);

        if (ISIC_appMsg(call_ptr, callMsg_ptr) != ISI_RETURN_OK) {
            ISID_unlockCalls();
            break;
        }
        ISID_unlockCalls();

        break;
    /* Add other AUDIO reason handlers here */
    default:
        break;
    }

    /* **************************************
     * NOTE: You don't have to free msg_ptr,
     * because it came from .bss and not heap
     ****************************************
     */
    return;
}

/*
 * ======== ISIM_appMsg() ========
 * This function is the FSM entry point of API commands related to
 * 'audio control'.  These commands come as a result of an API call.
 * Currently there is not state for audio commands.
 *
 * Returns:
 *  Nothing
 */
void ISIM_appMsg(
    ISIP_Message *msg_ptr)
{
    ISIQ_writeProtocolQueue(msg_ptr);
    ISI_free(msg_ptr, ISI_OBJECT_ISIP_MSG);
    return;
}

