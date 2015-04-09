/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12730 $ $Date: 2010-08-09 20:55:01 -0400 (Mon, 09 Aug 2010) $
 *
 */

#include "_ve_private.h"

/*
 * ======== _VE_rtpInitSeq() ========
 *
 */
void _VE_rtpInitSeq(
    _VE_RtpObject *rtp_ptr,
    uint16            sequenceNumber)
{
    rtp_ptr->baseSequence = sequenceNumber;
    rtp_ptr->maxSequence = sequenceNumber;
    rtp_ptr->badSequence = _VE_RTP_SEQ_MOD + 1;
    rtp_ptr->cycles = 0;
    rtp_ptr->received = 0;
    rtp_ptr->receivedPrior = 0;
    rtp_ptr->expectedPrior = 0;

    return;
}


/*
 * ======== _VE_rtpUpdateSeq() ========
 *
 * This function is used update statistics based in incoming RTP packets.
 */
vint _VE_rtpUpdateSeq(
    _VE_RtpObject   *rtp_ptr)
{
    uint16 sequenceNumber;
    uint16 delta;

    sequenceNumber = rtp_ptr->recvRtpObj.pkt.rtpMinHdr.seqn;
    delta = sequenceNumber - rtp_ptr->maxSequence;

    if (rtp_ptr->probation > _VE_RTP_MIN_SEQUENTIAL) {
        /*
         * The stream has just started, initialize parameters.
         */
        _VE_rtpInitSeq(rtp_ptr, sequenceNumber);
        rtp_ptr->probation = _VE_RTP_MIN_SEQUENTIAL;
        return (1);
    }

    if (rtp_ptr->probation != 0) {
        if (sequenceNumber == (rtp_ptr->maxSequence + 1)) {
            rtp_ptr->probation--;
            rtp_ptr->maxSequence = sequenceNumber;
            if (0 == rtp_ptr->probation) {
                _VE_rtpInitSeq(rtp_ptr, sequenceNumber);
                rtp_ptr->received++;
                return (1);
            }
        }
        else {
            rtp_ptr->probation = _VE_RTP_MIN_SEQUENTIAL - 1;
            rtp_ptr->maxSequence = sequenceNumber;
        }
        return (0);
    }
    else if (delta < _VE_RTP_MAX_DROPOUT) {
        if (sequenceNumber < rtp_ptr->maxSequence) {
            rtp_ptr->cycles += _VE_RTP_SEQ_MOD;
        }
        rtp_ptr->maxSequence = sequenceNumber;
    }
    else if (delta <= (_VE_RTP_SEQ_MOD - _VE_RTP_MAX_MISORDER)) {
        if (sequenceNumber == rtp_ptr->badSequence) {
            _VE_rtpInitSeq(rtp_ptr, sequenceNumber);
        }
        else {
            rtp_ptr->badSequence = (sequenceNumber + 1) &
                (_VE_RTP_SEQ_MOD - 1);
            return (0);
        }
    }
    rtp_ptr->received++;
    return (1);
}

/*
 * ======== _VE_rtpRecv() ========
 *
 * This routine polls all sockets that are actively receiving data from the
 * network.
 */
vint _VE_rtpRecv(
    _VE_Obj        *ve_ptr,
    _VE_Queues     *q_ptr,
    _VE_Dsp        *dsp_ptr)
{
    vint             infc;
    _VE_StreamObj *stream_ptr;
    VTSP_Stun        stunMsg;
    vint             pktLen;
    vint             streamId;
    OSAL_NetAddress  remoteAddr;
    uint8           *buf_ptr;
    RTP_Obj         *rtpObj_ptr;
    _VE_RtpObject   *rtp_ptr;
    OSAL_SelectSet  readSet;
    OSAL_SelectTimeval timesel;
    OSAL_SelectTimeval time;
    OSAL_Boolean boolean;

    /*
     * Will read based on select with a 10 ms timeout
     * so that we dont get into a busy loop.
     */
    OSAL_selectSetInit(&readSet);

    /*
     * If the UDP socket has data, read it and process it.
     * Note: infc increments to match the RTP object socket<->infc mapping,
     * set during initialization; i.e. must use FOR_ALL_INFC_PP, infc++, here.
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc, streamId);

            if (_VE_RTP_NOT_BOUND == rtp_ptr->inUse) {
                /*
                 * If the RTP data stream is ready. Poll it for data. If not,
                 * go to the next RTP object.
                 */
                continue;
            }
            OSAL_selectAddId(&rtp_ptr->socket, &readSet);
        }
    }

    timesel.sec = 0;
    timesel.usec = 10000;
    if (OSAL_FAIL == OSAL_select(&readSet, NULL, &timesel, &boolean)) {
        OSAL_taskDelay(10);
        _VE_TRACE(__FILE__, __LINE__);
        return (_VE_RTP_ERROR);
    }
    else if (OSAL_TRUE == boolean) {
        return (_VE_RTP_OK);
    }

    /*
     * Now read all ready packets.
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc, streamId);
            buf_ptr = rtp_ptr->fromNetBuffer;
            OSAL_selectIsIdSet(&rtp_ptr->socket, &readSet, &boolean);

            if ((_VE_RTP_NOT_BOUND == rtp_ptr->inUse) || (OSAL_FALSE == boolean)) {
                /*
                 * If the RTP data stream is ready. Poll it for data. If not,
                 * go to the next RTP object.
                 */
                continue;
            }

            /*
             * Get UDP packet from socket. recvfrom returns a value > 0 when
             * data is available. If pktLen is less than 0, an error has
             * occurred. This is expected if there is no data available.
             */
            /*
             * In order to support both IPv4 and IPv6, we have to specify which
             * type of socket is used. So that the receive function could 
             * prepare data structure to recevice correct data.
             */
            remoteAddr.type = stream_ptr->streamParam.localAddr.type;

            while ((pktLen = _VE_netRecvfrom(rtp_ptr->socket, buf_ptr,
                    RTP_BUFSZ_MAX, &remoteAddr)) > 0) {
                /*
                 * Add to bitrate;
                 * Assume 20 bytes IPv4 header.
                 * Assume 8 bytes UDP header.
                 * Times 8 for bits.
                 * This is actual packet bitrate incident on the receive side.
                 */
                ve_ptr->bitrateDec += ((pktLen + 20 + 8) << 3);

                /*
                 * Demux packets.
                 * Bug 2542
                 */
                if (RTP_VERSION != ((*buf_ptr >> 6) & 3)) { 
                    /*
                     * Not an RTP packet.
                     * Send to user space immediately.
                     * XXX: If the user can use some information here to discern
                     * packets and filter.
                     * XXX: Warning! DOS attack can overflow packet queue.
                     */
                    if ((uvint)pktLen <= sizeof(stunMsg.payload)) {
                        OSAL_memCpy(stunMsg.payload, buf_ptr, pktLen);
                        stunMsg.pktSize      = pktLen;
                        stunMsg.infc         = infc;
                        stunMsg.streamId     = streamId;
                        OSAL_memCpy(&stunMsg.remoteAddr, &remoteAddr, 
                                sizeof(OSAL_NetAddress));
                        OSAL_memCpy(&stunMsg.localAddr, &rtp_ptr->localAddr, 
                                sizeof(OSAL_NetAddress));

                        if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->stunRecvQ,
                                (char *)&(stunMsg), sizeof(VTSP_Stun),
                                OSAL_NO_WAIT, NULL)) {
                            _VE_TRACE(__FILE__, __LINE__);
                        }
                        /* Set STUN event Flag */
                        stream_ptr->dec.stunEvent = VTSP_EVENT_ACTIVE;
                    }
                    continue;
                 }

                /*
                 * If the IP address matches the expected address, and the
                 * jitter buffer is enabled, and the RTP stream is marked as
                 * ready, process the data into the jitter buffer.
                 */
                else if (_VE_netIsSameAddr(remoteAddr, rtp_ptr->remoteAddr)
                        && (_VE_RTP_READY == rtp_ptr->recvActive)) {

                    rtpObj_ptr = &(rtp_ptr->recvRtpObj);

                    /*
                     * Parse payload. The input packet to RTP is in buf_ptr. The
                     * output is placed in recvRtpObj.pkt.payload.
                     */
                    if (RTP_OK == RTP_recvPkt(rtpObj_ptr, buf_ptr, pktLen)) {

                        /* Put redundant info and data into Jitter Buffer */
                        OSAL_selectGetTime(&time);
                        dsp_ptr->jbPkt.atime = (uint64)time.sec;
                        dsp_ptr->jbPkt.atime *= 1000000;
                        dsp_ptr->jbPkt.atime += (uint64)time.usec;
                        dsp_ptr->jbPkt.ts = rtpObj_ptr->pkt.rtpMinHdr.ts;
                        dsp_ptr->jbPkt.mark = rtpObj_ptr->pkt.rtpMinHdr.marker;
                        dsp_ptr->jbPkt.seqn = rtpObj_ptr->pkt.rtpMinHdr.seqn;
                        dsp_ptr->jbPkt.pSize = rtpObj_ptr->payloadSize;
                        dsp_ptr->jbPkt.type = _VE_dynamicToLocalDecoder(
                                &(stream_ptr->streamParam),
                                rtpObj_ptr->pkt.rtpMinHdr.type);;
                        dsp_ptr->jbPkt.data_ptr = rtp_ptr->recvRtpObj.pkt.payload;
                        dsp_ptr->jbPkt.valid = 1;
                        JBV_putPkt(&stream_ptr->dec.jbObj, &dsp_ptr->jbPkt);
                    }
                }
                else {
                    /* Pkt received on open socket but inactive stream */
                    /* _VE_TRACE(__FILE__, __LINE__); */
                }
            }
        }
    }
    return (_VE_RTP_OK);
}
