/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24440 $ $Date: 2014-02-06 12:42:12 +0800 (Thu, 06 Feb 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_conf.h"

/* 
 * ======== ISIF_protoMsg() ========
 * This function is the FSM entry point of commands related to 'conferencing'.
 * Currently no commands are expected to be received from the underlying 
 * protocol that are related for conferencing.  However that function is 
 * defined for future use.
 *
 * Returns: 
 *  nothing
 */
void ISIF_protoMsg(
    ISI_Id        confId, 
    ISIP_Message *msg_ptr)
{
    /* Nothing defined for this */
    return;
}

/* 
 * ======== ISIF_appMsg() ========
 * This function is the FSM entry point of commands related to 'calls'.
 * These commands come as a result of an API call.
 * Currently this function contains no true FSM, it simply passes
 * the commands downstream to the underlying protocol.
 *
 * Returns: 
 *  ISI_RETURN_OK       : The command successfully passed through the FSM
 *                        and was sent to the underlying protocol.
 */
ISI_Return ISIF_appMsg(
    ISID_ConfId   *conf_ptr, 
    ISIP_Message  *msg_ptr)
{
    ISID_CallId    *call_ptr;
    vint            x;

    /*
     * The remaining call(s) will be put on hold after splitting a call from
     * conference.
     */
    if (ISIP_MEDIA_REASON_CONFSTOP == msg_ptr->msg.media.reason) {
        for (x = 0; x < ISI_CONF_USERS_NUM ; x++) {
            call_ptr = conf_ptr->aCall[x];
            if ((NULL != call_ptr) && (0 != conf_ptr->aConfMask[x])) {
                call_ptr->state = ISI_CALL_STATE_ONHOLD;
            }
        }
    }

    ISIQ_writeProtocolQueue(msg_ptr);
    /* Free the msg_ptr */
    ISIM_free(msg_ptr);
    return(ISI_RETURN_OK);
}

