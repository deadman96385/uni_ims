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
 * ======== _VC_rtpInitSeq() ========
 * The RTP Object should be locked before calling this method.
 */
void _VC_rtpInitSeq(
    _VC_RtpObject    *rtp_ptr,
    uint16            sequenceNumber)
{
    rtp_ptr->badSequence = _VC_RTP_SEQ_MOD + 1;
    rtp_ptr->info.baseSequence = sequenceNumber;
    rtp_ptr->info.maxSequence = sequenceNumber;
    rtp_ptr->info.cycles = 0;
    rtp_ptr->info.received = 0;
    return;
}


/*
 * ======== _VC_rtpLockedUpdateSeq() ========
 *
 * This function is used update statistics based in incoming RTP packets,
 * but the RTP Info structure should be locked before calling this method.
 */
static vint _VC_rtpLockedUpdateSeq(
    _VC_RtpObject   *rtp_ptr)
{
    uint16 sequenceNumber;
    uint16 delta;

    sequenceNumber = rtp_ptr->recvRtpObj.pkt.rtpMinHdr.seqn;
    delta = sequenceNumber - rtp_ptr->info.maxSequence;

    if (rtp_ptr->probation > _VC_RTP_MIN_SEQUENTIAL) {
        /*
         * The stream has just started, initialize parameters.
         */
        _VC_rtpInitSeq(rtp_ptr, sequenceNumber);
        rtp_ptr->probation = _VC_RTP_MIN_SEQUENTIAL;
        return (1);
    }

    if (rtp_ptr->probation != 0) {
        if (sequenceNumber == (rtp_ptr->info.maxSequence + 1)) {
            rtp_ptr->probation--;
            rtp_ptr->info.maxSequence = sequenceNumber;
            if (0 == rtp_ptr->probation) {
                _VC_rtpInitSeq(rtp_ptr, sequenceNumber);
                rtp_ptr->info.received++;
                return (1);
            }
        }
        else {
            rtp_ptr->probation = _VC_RTP_MIN_SEQUENTIAL - 1;
            rtp_ptr->info.maxSequence = sequenceNumber;
        }
        return (0);
    }
    else if (delta < _VC_RTP_MAX_DROPOUT) {
        if (sequenceNumber < rtp_ptr->info.maxSequence) {
            rtp_ptr->info.cycles += _VC_RTP_SEQ_MOD;
        }
        rtp_ptr->info.maxSequence = sequenceNumber;
    }
    else if (delta <= (uint16)(_VC_RTP_SEQ_MOD - _VC_RTP_MAX_MISORDER)) {
        if (sequenceNumber == rtp_ptr->badSequence) {
            _VC_rtpInitSeq(rtp_ptr, sequenceNumber);
        }
        else {
            rtp_ptr->badSequence = (sequenceNumber + 1) &
                (_VC_RTP_SEQ_MOD - 1);
            return (0);
        }
    }
    rtp_ptr->info.received++;
    return (1);
}

/*
 * ======== _VC_calcRxBitrate() =======
 *
 * This function is used to calculate the rx Bitrate.
 * */
vint _VC_calcRxBitrate(_VC_RtpObject *rtp_ptr,
    OSAL_SelectTimeval *tv,
    JBV_Pkt *pkt_ptr)
{
    _VC_RtpRtcpInfoObject *info;
    _VC_RtpBitrateStat    *br_stat;
    uint32 size;
    uint16 seqn;
    uint64 time_us;
    static uint32 cnt = 0;

    if (rtp_ptr == NULL || pkt_ptr == NULL){
        JBV_errLog(" the input params are invalid, rtp_ptr 0x%x, pkt_ptr %x\n",
                rtp_ptr, pkt_ptr);
        return _VC_ERROR;
    }

    info = &rtp_ptr->info;
    br_stat = &info->rxBitrateStat;
    size = pkt_ptr->pSize;
    seqn = pkt_ptr->seqn;

    time_us = tv->sec * 1000000;
    time_us += tv->usec;

    br_stat->totalSize += size;

    /*
     *
     * if a new period begins, we reset the statistics.
     *
     * if the interval is greater than 500 ms and the seqn is sequencial,
     * we here assume that the peer UE is paused for a while then resumes,
     * but the session keeps running.
     *
     * if the seqn interval between adjust pkts is greater than 500, we consider
     * that the sender has renew the seqn.
     *
     * if the time gap is greater than BITRATE_TIME_US_GAP.
     * */
    if (br_stat->periodFlag == 0 ||
            ((time_us - br_stat->lastTime) > BITRATE_PKT_INTV_THRESHOLD && (seqn - br_stat->lastSeqn) <= 3) ||
            ((int16)(seqn - br_stat->lastSeqn) > 500 || (int16)(seqn - br_stat->lastSeqn) < -500) ||
            (time_us - br_stat->lastTime) > BITRATE_TIME_US_GAP) {
        /* update the statistics, except the bitrate, begin a new period  */
        if (br_stat->periodFlag != 0) {
            br_stat->bitrate = 0;
        }
        br_stat->lastSize = br_stat->totalSize - size;
        br_stat->startTime = time_us;
        br_stat->periodFlag = 1;
        OSAL_logMsg("%s: restart the bitrate calculattion peroid\n", __FUNCTION__);
    } else if((int64)(time_us - br_stat->startTime) > BITRATE_STAT_PERIOD){
        /* calculate the bitrate */
        br_stat->bitrate = (uint32)((((br_stat->totalSize - br_stat->lastSize) << 3) * 1000000 / (time_us - br_stat->startTime)) >> 10);
        OSAL_logMsg("%s: cur_br update to %u Kbps, totalSize %llu, lastSize %llu, startTime %llu, time_us %llu, seqn %u\n",
                __FUNCTION__, br_stat->bitrate, br_stat->totalSize, br_stat->lastSize, br_stat->startTime, time_us, seqn);
        br_stat->periodFlag = 0;
        br_stat->lastSize = br_stat->totalSize;
    }

    br_stat->lastTime = time_us;
    br_stat->lastSeqn = seqn;

    return _VC_OK;
}

/*
 * ======== _VC_rtpUpdateSeq() ========
 *
 * This function is used update statistics based in incoming RTP packets.
 */
vint _VC_rtpUpdateSeq(
    _VC_RtpObject *rtp_ptr,
    OSAL_SelectTimeval *tv,
    JBV_Pkt *pkt_ptr)
{
    vint ret;
    OSAL_semAcquire(rtp_ptr->info.mutexLock, OSAL_WAIT_FOREVER);
    ret = _VC_rtpLockedUpdateSeq(rtp_ptr);
    _VC_calcRxBitrate(rtp_ptr, tv, pkt_ptr);
    OSAL_semGive(rtp_ptr->info.mutexLock);
    return ret;
}

/*
 * ======== _VC_rtpRecv() ========
 *
 * This routine polls all sockets that are actively receiving data from the
 * network.
 */
vint _VC_rtpRecv(
    _VC_Obj        *vc_ptr)
{
    _VC_Dsp           *dsp_ptr;
    vint               infc;
    _VC_StreamObj     *stream_ptr;
    vint               pktLen;
    vint               streamId;
    OSAL_NetAddress    remoteAddr;
    uint8             *buf_ptr;
    RTP_Obj           *rtpObj_ptr;
    _VC_RtpObject     *rtp_ptr;
    OSAL_SelectSet     readSet;
    OSAL_SelectTimeval timesel;
    OSAL_SelectTimeval time;
    OSAL_Boolean       boolean;
    vint               headerExtnOffset = 0;
    OSAL_SelectTimeval tv;

    dsp_ptr = vc_ptr->dsp_ptr;
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
            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);

            if (_VC_RTP_NOT_BOUND == rtp_ptr->inUse) {
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
       // _VC_TRACE(__FILE__, __LINE__);
        OSAL_logMsg("%s:failed to select RTP\n", __FUNCTION__);
        return (_VC_RTP_ERROR);
    }
    else if (OSAL_TRUE == boolean) {
        return (_VC_RTP_OK);
    }

    /*
     * Now read all ready packets.
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);
            buf_ptr = rtp_ptr->fromNetBuffer;
            OSAL_selectIsIdSet(&rtp_ptr->socket, &readSet, &boolean);

            if ((_VC_RTP_NOT_BOUND == rtp_ptr->inUse) || (OSAL_FALSE == boolean)) {
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

            while ((pktLen = _VC_netRecvfrom(rtp_ptr->socket, buf_ptr,
                    RTP_BUFSZ_MAX, &remoteAddr)) > 0) {
    
                /* Reset the headerExtnOffset to 0 */
                headerExtnOffset = 0;

                /*
                 * Add to bitrate;
                 * Assume 20 bytes IPv4 header.
                 * Assume 8 bytes UDP header.
                 * Times 8 for bits.
                 * This is actual packet bitrate incident on the receive side.
                 */
                vc_ptr->bitrateDec += ((pktLen + 20 + 8) << 3);

                /*
                 * Demux packets.
                 * Bug 2542
                 */
                if (RTP_VERSION != ((*buf_ptr >> 6) & 3)) { 
                    /*
                     * Not an RTP packet.
                    */
                    continue;
                 }

                /*
                 * If the IP address matches the expected address, and the
                 * jitter buffer is enabled, and the RTP stream is marked as
                 * ready, process the data into the jitter buffer.
                 */
                else if (_VC_netIsSameAddr(remoteAddr, rtp_ptr->remoteAddr)
                        && (_VC_RTP_READY == rtp_ptr->recvActive)) {

                    rtpObj_ptr = &(rtp_ptr->recvRtpObj);

                    /*
                     * Parse payload. The input packet to RTP is in buf_ptr. The
                     * output is placed in recvRtpObj.pkt.payload.
                     */
                    if (RTP_OK == RTP_recvPkt(rtpObj_ptr, buf_ptr, pktLen)) {
                        /* Put redundant info and data into Jitter Buffer */
                        OSAL_selectGetTime(&time);
                        dsp_ptr->jbPutPkt.atime = (uint64)time.sec;
                        dsp_ptr->jbPutPkt.atime *= 1000000;
                        dsp_ptr->jbPutPkt.atime += (uint64)time.usec;
                        dsp_ptr->jbPutPkt.tsOrig = rtpObj_ptr->pkt.rtpMinHdr.ts;
                        dsp_ptr->jbPutPkt.mark = rtpObj_ptr->pkt.rtpMinHdr.marker;
                        dsp_ptr->jbPutPkt.seqn = rtpObj_ptr->pkt.rtpMinHdr.seqn;

                        /* Check if RTP Extension bit is set. */
                        if (1 == rtpObj_ptr->pkt.rtpMinHdr.extension) {
                            /* Remove the 8 bytes of Rcs 5.1 Header Extension. */
                            headerExtnOffset = 8;
                            /* Extract the RCS RTP Extn Payload. */
                            dsp_ptr->jbPutPkt.rcsRtpExtnPayload = rtp_ptr->recvRtpObj.pkt.payload[5];
                            DBG("Rcs Rtp Extn Payload %d", dsp_ptr->jbPutPkt.rcsRtpExtnPayload);
                        }
                        else {
                            dsp_ptr->jbPutPkt.rcsRtpExtnPayload = 0x00;
                        }

                        dsp_ptr->jbPutPkt.pSize = rtpObj_ptr->payloadSize - headerExtnOffset;
                        dsp_ptr->jbPutPkt.type = _VC_dynamicToLocalDecoder(
                                &(stream_ptr->streamParam),
                                rtpObj_ptr->pkt.rtpMinHdr.type);
                        dsp_ptr->jbPutPkt.data_ptr = rtp_ptr->recvRtpObj.pkt.payload + headerExtnOffset;
                        dsp_ptr->jbPutPkt.valid = 1;

                        OSAL_selectGetTime(&tv);
                        //DBG("RTP Rcv Seqn: %d", dsp_ptr->jbPutPkt.seqn);
                        JBV_putPkt(&stream_ptr->dec.jbObj, &dsp_ptr->jbPutPkt, (JBV_Timeval *)&tv);

                        //_VC_calcRxBitrate(rtp_ptr, &tv, &dsp_ptr->jbPutPkt);
                        _VC_rtpUpdateSeq(rtp_ptr, &tv, &dsp_ptr->jbPutPkt);
                    }
                }
                else {
                    /* Pkt received on open socket but inactive stream */
                    /* _VC_TRACE(__FILE__, __LINE__); */
                }
            }
        }
    }
    return (_VC_RTP_OK);
}
