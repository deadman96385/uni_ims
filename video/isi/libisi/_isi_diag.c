/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 17269 $ $Date: 2012-05-29 02:09:52 -0700 (Tue, 29 May 2012) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_service.h"
#include "_isi_dbg.h"
#include "_isi_diag.h"

/*
 * ======== ISIDIAG_appMsg() ========
 * This function is the FSM entry point of commands related to 'diagnostic'
 * commands. These commands come as a result of an API call.
 *
 * It simply passes the commands downstream to the underlying protocol.
 *
 * Returns:
 *   ISI_RETURN_OK : always.
 */

ISI_Return ISIDIAG_appMsg(
    ISIP_Message *msg_ptr)
{
    ISIG_log(__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logDiag(msg_ptr->msg.diag.reason);

    /* ADD any special handling if required */

    ISIQ_writeProtocolQueue(msg_ptr);
    ISIM_free(msg_ptr);
    return (ISI_RETURN_OK);
}

