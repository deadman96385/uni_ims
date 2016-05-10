/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12730 $ $Date: 2010-08-09 20:55:01 -0400 (Mon, 09 Aug 2010) $
 *
 */

#include "_vc_private.h"

/*
 * ======== _VC_rtpDir() ========
 *
 * Set the direction of RTP streams.
 */
vint _VC_rtpDir(
    _VC_RtpObject *rtp_ptr,
    vint              dir)
{
    /*
     * Now set the direction.
     */
    switch (dir) {
        case VTSP_STREAM_DIR_SENDONLY:
            rtp_ptr->sendActive = _VC_RTP_READY;
            rtp_ptr->recvActive = _VC_RTP_NOTREADY;
            break;

        case VTSP_STREAM_DIR_RECVONLY:
            rtp_ptr->sendActive = _VC_RTP_NOTREADY;
            if (_VC_RTP_NOTREADY == rtp_ptr->recvActive) {
                /*
                 * Initialize RTP sequence number check. Received stream
                 * switched from inactive state.
                 */
                rtp_ptr->probation = _VC_RTP_MIN_SEQUENTIAL + 1;
            }
            rtp_ptr->recvActive = _VC_RTP_READY;
            break;

        case VTSP_STREAM_DIR_SENDRECV:
            rtp_ptr->sendActive = _VC_RTP_READY;
            if (_VC_RTP_NOTREADY == rtp_ptr->recvActive) {
                /*
                 * Initialize RTP sequence number check. Received stream
                 * switched from inactive state.
                 */
                rtp_ptr->probation = _VC_RTP_MIN_SEQUENTIAL + 1;
            }
            rtp_ptr->recvActive = _VC_RTP_READY;
            break;
        default:
            rtp_ptr->sendActive = _VC_RTP_NOTREADY;
            rtp_ptr->recvActive = _VC_RTP_NOTREADY;
            break;
    }
    return (0);
}

/*
 * ======== _VC_rtpOpen() ========
 *
 * This function is used to open an RTP flow.
 */
vint _VC_rtpOpen(
    _VC_RtpObject     *rtp_ptr,
    VTSP_StreamDir     dir,
    OSAL_NetAddress    remoteAddr,
    OSAL_NetAddress    localAddr,
    uint8              rdnDynType,
    uint16             srtpSecurityType,
    char              *srtpSendKey,
    char              *srtpRecvKey)
{
    vint   newBind;
    uint32 random;
    uint32 firstRtpTime;

    char dummy[16] = {0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (0 == rtp_ptr->open) {
        if (0 == rtp_ptr->tsRandom) {
            firstRtpTime = 0;

        }
        else {
        OSAL_randomGetOctets((char *)&random, sizeof(random));
            firstRtpTime = random;
        }

        rtp_ptr->sendRtpObj.seqRandom = rtp_ptr->seqRandom;
        rtp_ptr->recvRtpObj.seqRandom = rtp_ptr->seqRandom;
        rtp_ptr->sendRtpObj.tsRandom  = rtp_ptr->tsRandom;
        rtp_ptr->recvRtpObj.tsRandom  = rtp_ptr->tsRandom;

        /*
         * Initialize the RTP objects.
         * Redundant cache buffers for RFC2198 have been allocated
         * during _VC_rtpInit()
         */
        RTP_init(&(rtp_ptr->sendRtpObj));
        RTP_init(&(rtp_ptr->recvRtpObj));

        /* init ssrc of the recvRtpObj as 0*/
        rtp_ptr->recvRtpObj.pkt.rtpMinHdr.ssrc = 0;

        rtp_ptr->rtpTime = firstRtpTime;
        rtp_ptr->tsMs            = 0;
        rtp_ptr->payloadOffset   = 0; /* bug 3782 */
        rtp_ptr->lastLocVCoder   = VTSP_CODER_VIDEO_H264;
        rtp_ptr->lastLocalCoder  = VTSP_CODER_VIDEO_H264;

        OSAL_semAcquire(rtp_ptr->info.mutexLock, OSAL_WAIT_FOREVER);
        rtp_ptr->info.firstRtpTime = firstRtpTime;
        rtp_ptr->info.firstRtpTimeMs = 0;
        rtp_ptr->info.sendPacketCount = 0;
        rtp_ptr->info.sendOctetCount  = 0;
        OSAL_semGive(rtp_ptr->info.mutexLock);
    }

#ifdef VTSP_ENABLE_SRTP
    RTP_srtpinit(&(rtp_ptr->sendRtpObj), srtpSecurityType, srtpSendKey,
         RTP_KEY_STRING_MAX_LEN);
    RTP_srtpinit(&(rtp_ptr->recvRtpObj), srtpSecurityType, srtpRecvKey,
         RTP_KEY_STRING_MAX_LEN);
#endif
    /*
     * Check if need rebind socket.
     */
    if (localAddr.type != rtp_ptr->localAddr.type ||
            localAddr.port != rtp_ptr->localAddr.port) {
        newBind = 1;
    }
    else if (OSAL_NET_SOCK_UDP == localAddr.type ||
            OSAL_NET_SOCK_TCP == localAddr.type) {
        /* IPv4 Address type */
        if (localAddr.ipv4 != rtp_ptr->localAddr.ipv4) {
            newBind = 1;
        }
        else {
            newBind = 0;
        }
    }
    else {
        /* IPv6 Address type */
        if ( 0 != OSAL_memCmp(localAddr.ipv6, rtp_ptr->localAddr.ipv6,
                sizeof(localAddr.ipv6))) {
            newBind = 1;
        }
        else {
            newBind = 0;
        }
    }


    OSAL_memCpy(&rtp_ptr->remoteAddr, &remoteAddr, sizeof(remoteAddr));
    OSAL_memCpy(&rtp_ptr->localAddr, &localAddr, sizeof(localAddr));

    /*
     * If the port was being used and the port changed, then rebind the port by
     * closing, opening, then binding.
     */
    if ((_VC_RTP_BOUND == rtp_ptr->inUse) && (1 == newBind)) {
        rtp_ptr->sendActive = _VC_RTP_NOTREADY;
        rtp_ptr->recvActive = _VC_RTP_NOTREADY;
        rtp_ptr->inUse      = _VC_RTP_NOT_BOUND;
        /*
         * If this routine has previously been called. Close all sockets and
         * reopen them.
         */
        if (_VC_netClose(rtp_ptr->socket) != _VC_RTP_OK) {
            //_VC_TRACE(__FILE__, __LINE__);
            OSAL_logMsg("%s: failed to close net\n", __FUNCTION__);
            return (_VC_RTP_ERROR);
        }
        rtp_ptr->open = 0;
    }

    /*
     * Create socket if socket is not opened.
     */

    if (0 == rtp_ptr->open) {
        if ((rtp_ptr->socket =
                    _VC_netSocket(localAddr.type, rtp_ptr->tos)) < 0) {
           // _VC_TRACE(__FILE__, __LINE__);
            OSAL_logMsg("%s: failed to create socket\n", __FUNCTION__);
            return (_VC_RTP_ERROR);
        }
        rtp_ptr->open = 1;
    }

    /*
     * Now bind the socket so that RTP data can be exchanged. Note this is only
     * called if the port is non-zero. If the port is zero, the application
     * effectively is closing the port.
     */
    _VC_rtpDir(rtp_ptr, dir);
    if ((localAddr.port != 0) && (newBind == 1)) {
        if (_VC_RTP_OK != _VC_netBind(rtp_ptr->socket, localAddr)) {
            /*
             * Set RTP socket into not ready state.
            */
            rtp_ptr->sendActive = _VC_RTP_NOTREADY;
            rtp_ptr->recvActive = _VC_RTP_NOTREADY;
            rtp_ptr->inUse      = _VC_RTP_NOT_BOUND;
            //_VC_TRACE(__FILE__, __LINE__);
            OSAL_logMsg("%s: failed to bind net\n", __FUNCTION__);
            return (_VC_RTP_ERROR);
        }
        rtp_ptr->inUse = _VC_RTP_BOUND;
    }
    else {
        if ((0 == remoteAddr.port) ||
                (OSAL_netIsAddrZero(&remoteAddr) == OSAL_TRUE)) {
            rtp_ptr->sendActive = _VC_RTP_NOTREADY;
        }
        if (0 == localAddr.port) {
            rtp_ptr->sendActive = _VC_RTP_NOTREADY;
            rtp_ptr->recvActive = _VC_RTP_NOTREADY;
            rtp_ptr->inUse      = _VC_RTP_NOT_BOUND;
        }
    }

    /*
     * Open up the NAT quickly so we dont lose any incoming packets.
     */
    if ((0 != remoteAddr.port) &&
            (OSAL_netIsAddrZero(&remoteAddr) != OSAL_TRUE)) {
        //_VC_netSendto(rtp_ptr->socket, &dummy, sizeof(dummy),
        //        rtp_ptr->remoteAddr);
    }

    return (_VC_RTP_OK);
}
