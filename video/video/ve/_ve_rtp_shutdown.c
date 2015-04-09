/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */        

#include "_ve_private.h"

/*
 * ======== _VE_rtpShutdown() ========
 *
 * This function is used to shutdown an RTP stream.
 */
vint _VE_rtpShutdown(
    _VE_RtpObject *rtp_ptr)
{
    vint              stream;

    /*
     * Close the socket
     */
    for (stream = 0; stream < _VTSP_STREAM_NUM; stream++) {
        /*
         * Mark the stream no longer in use.
         */
        rtp_ptr->inUse = _VE_RTP_NOTREADY;
        rtp_ptr->sendActive = _VE_RTP_NOTREADY;
        rtp_ptr->recvActive = _VE_RTP_NOTREADY;
        /*
         * Close socket.
         */
        if (_VE_RTP_OK != _VE_netClose(rtp_ptr->socket)) { 
            /* Keep going; continue shutdown attempt
             */
            _VE_TRACE(__FILE__, __LINE__);
        }

        RTP_shutdown(&(rtp_ptr->sendRtpObj));
        RTP_shutdown(&(rtp_ptr->recvRtpObj));

        rtp_ptr++;
    }

    return (_VE_RTP_OK);
}

