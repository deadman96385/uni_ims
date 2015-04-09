/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-14 06:57:44 +0800 (Wed, 14 Jul 2010) $
 *
 */        

#include <osal.h>
#include <_vtspr_rtp.h>

/*
 * ======== _VTSPR_rtpShutdown() ========
 *
 * This function is used to shutdown an RTP stream.
 */
vint _VTSPR_rtpShutdown(
    _VTSPR_RtpObject *rtp_ptr)
{
    vint              stream;

    /*
     * Close the socket
     */
    for (stream = 0; stream < _VTSP_STREAM_NUM; stream++) {
        /*
         * Mark the stream no longer in use.
         */
        rtp_ptr->inUse = _VTSPR_RTP_NOTREADY;
        rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
        rtp_ptr->recvActive = _VTSPR_RTP_NOTREADY;
        /*
         * Close socket.
         */
        if (_VTSPR_RTP_OK != _VTSPR_netClose(rtp_ptr->socket)) { 
            /* Keep going; continue shutdown attempt
             */
            _VTSP_TRACE(__FILE__, __LINE__);
        }

        RTP_shutdown(&(rtp_ptr->sendRtpObj));
        RTP_shutdown(&(rtp_ptr->recvRtpObj));

        rtp_ptr++;
    }

    return (_VTSPR_RTP_OK);
}

