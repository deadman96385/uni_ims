/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */

#include "_vc_private.h"

/*
 * ======== _VC_rtpClose() ========
 *
 * This function is used to close an RTP stream.
 */
vint _VC_rtpClose(
    _VC_RtpObject *rtp_ptr)
{
    /*
     * If the socket is open, close it. Then reopen it for next time use.
     */
        if (_VC_netClose(rtp_ptr->socket) != _VC_RTP_OK) {
            _VC_TRACE(__FILE__, __LINE__);
            return (_VC_RTP_ERROR);
        }

        /*
     * Mark the stream no longer in use.
     */
    rtp_ptr->inUse      = _VC_RTP_NOT_BOUND;
    rtp_ptr->sendActive = _VC_RTP_NOTREADY;
    rtp_ptr->recvActive = _VC_RTP_NOTREADY;

    /*
     * Clear any old addresses.
     */
    OSAL_memSet(&rtp_ptr->remoteAddr, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&rtp_ptr->localAddr, 0, sizeof(OSAL_NetAddress));
    rtp_ptr->open = 0;

    return (_VC_RTP_OK);
}
