/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28481 $ $Date: 2014-08-26 14:54:55 +0800 (Tue, 26 Aug 2014) $
 *
 */        

#include <osal.h>
#include <_vtspr_rtp.h>

/*
 * ======== _VTSPR_rtpDir() ========
 *
 * Set the direction of RTP streams.
 */
vint _VTSPR_rtpDir(
    _VTSPR_RtpObject *rtp_ptr,
    vint              dir)
{
    /*
     * Now set the direction.
     */
    switch (dir) {
        case VTSP_STREAM_DIR_SENDONLY:
            rtp_ptr->sendActive = _VTSPR_RTP_READY;
            rtp_ptr->recvActive = _VTSPR_RTP_NOTREADY;
            break;

        case VTSP_STREAM_DIR_RECVONLY:
            rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
            if (_VTSPR_RTP_NOTREADY == rtp_ptr->recvActive) {
                /*
                 * Initialize RTP sequence number check. Received stream
                 * switched from inactive state.
                 */
                rtp_ptr->probation = _VTSPR_RTP_MIN_SEQUENTIAL + 1;
            }
            rtp_ptr->recvActive = _VTSPR_RTP_READY;
            break;

        case VTSP_STREAM_DIR_SENDRECV:
            rtp_ptr->sendActive = _VTSPR_RTP_READY;
            if (_VTSPR_RTP_NOTREADY == rtp_ptr->recvActive) {
                /*
                 * Initialize RTP sequence number check. Received stream
                 * switched from inactive state.
                 */
                rtp_ptr->probation = _VTSPR_RTP_MIN_SEQUENTIAL + 1;
            }
            rtp_ptr->recvActive = _VTSPR_RTP_READY;
            break;
        default:
            rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
            rtp_ptr->recvActive = _VTSPR_RTP_NOTREADY;
            break;
    }
    return (0);
}

/*
 * ======== _VTSPR_rtpOpen() ========
 *
 * This function is used to open an RTP flow.
 */
vint _VTSPR_rtpOpen(
    _VTSPR_RtpObject  *rtp_ptr,
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

    if (0 == rtp_ptr->open) {
        if (0 == rtp_ptr->tsRandom) {
            rtp_ptr->firstRtpTime = 0;
            rtp_ptr->rtpTime = 0;
        }
        else {
            OSAL_randomGetOctets((char *)&random, sizeof(random));
            rtp_ptr->firstRtpTime = random;
        }

        rtp_ptr->sendRtpObj.seqRandom = rtp_ptr->seqRandom;
        rtp_ptr->recvRtpObj.seqRandom = rtp_ptr->seqRandom;
        rtp_ptr->sendRtpObj.tsRandom  = rtp_ptr->tsRandom;
        rtp_ptr->recvRtpObj.tsRandom  = rtp_ptr->tsRandom;

        /*
         * Initialize the RTP objects.
         * Redundant cache buffers for RFC2198 have been allocated
         * during _VTSPR_rtpInit(), the 2nd parameter 'redunBufSize' must be 0
         */
        RTP_init(&(rtp_ptr->sendRtpObj));
        RTP_init(&(rtp_ptr->recvRtpObj));

        rtp_ptr->rtpTime = rtp_ptr->firstRtpTime;
        rtp_ptr->firstRtpTimeMs = 0;
        rtp_ptr->sendPacketCount = 0;
        rtp_ptr->sendOctetCount  = 0;
        rtp_ptr->receiveTime     = 0;
        rtp_ptr->lastReceiveTime = 0;
        rtp_ptr->lastTimeStamp   = 0;
        rtp_ptr->framesM1        = 0;
        rtp_ptr->payloadOffset   = 0; /* bug 3782 */
        rtp_ptr->lastLocVCoder   = VTSP_CODER_G711U;
        rtp_ptr->lastLocalCoder  = VTSP_CODER_G711U;
        rtp_ptr->lastDtmfTime    = _VTSPR_RTP_NOT_DTMF_PKT;
        rtp_ptr->jitter          = 0;
        rtp_ptr->lastSR          = 0;
        rtp_ptr->recvLastSR      = 0;
    }
    
    /*
     * Assign the dynamic payload type for RFC2198
     * and reset the redundant cache buffer.
     */
    RTP_redunReset(&(rtp_ptr->sendRtpObj), rdnDynType);
    RTP_redunReset(&(rtp_ptr->recvRtpObj), rdnDynType);

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
    if ((_VTSPR_RTP_BOUND == rtp_ptr->inUse) && (1 == newBind)) {
        rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
        rtp_ptr->recvActive = _VTSPR_RTP_NOTREADY;
        rtp_ptr->inUse      = _VTSPR_RTP_NOT_BOUND;
        /*
         * If this routine has previously been called. Close all sockets and
         * reopen them.
         */
        if (_VTSPR_netClose(rtp_ptr->socket) != _VTSPR_RTP_OK) {
            _VTSP_TRACE(__FILE__, __LINE__);
            return (_VTSPR_RTP_ERROR);
        }
        rtp_ptr->open = 0;
    }

    /* 
     * Create socket if socket is not opened.
     */
    
    if (0 == rtp_ptr->open) {
        rtp_ptr->socket = _VTSPR_netSocket(localAddr.type, rtp_ptr->tos);
        if (OSAL_SUCCESS != _VTSPR_netIsSocketIdValid(rtp_ptr->socket)) {
            _VTSP_TRACE(__FILE__, __LINE__);
            return (_VTSPR_RTP_ERROR);
        }
        rtp_ptr->open = 1;
    }

    /*
     * Now bind the socket so that RTP data can be exchanged. Note this is only
     * called if the port is non-zero. If the port is zero, the application
     * effectively is closing the port.
     */
    _VTSPR_rtpDir(rtp_ptr, dir);

    if ((localAddr.port != 0) && (newBind == 1)) {
        if (_VTSPR_RTP_OK != _VTSPR_netBind(rtp_ptr->socket, localAddr)) {
            /*
             * Set RTP socket into not ready state.
             */
            rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
            rtp_ptr->recvActive = _VTSPR_RTP_NOTREADY;
            rtp_ptr->inUse      = _VTSPR_RTP_NOT_BOUND;
            _VTSP_TRACE(__FILE__, __LINE__);
            return (_VTSPR_RTP_ERROR);
        }
        rtp_ptr->inUse = _VTSPR_RTP_BOUND;
    }
    else {
        if ((0 == remoteAddr.port) || OSAL_netIsAddrZero(&remoteAddr)) {
            rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
        }
        if (0 == localAddr.port) {
            rtp_ptr->sendActive = _VTSPR_RTP_NOTREADY;
            rtp_ptr->recvActive = _VTSPR_RTP_NOTREADY;
            rtp_ptr->inUse      = _VTSPR_RTP_NOT_BOUND;
        }
    }

    return (_VTSPR_RTP_OK);
}
