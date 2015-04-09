/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 */

#include <_vtspr_rtp.h>
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_rtpGetTsIncRate() ========
 *
 * Set flags for rtp TS increment rate  (Bug 2951)
 */
static OSAL_INLINE void _VTSPR_rtpGetTsIncRate(
    VTSPR_DSP  *dsp_ptr,
    uint16      coder)
{
    if (OSAL_TRUE == _VTSPR_isCoderWb(coder) && (VTSP_CODER_G722 != coder)) {
        dsp_ptr->jbPkt.flags = JB_PKT_FLAGS_MASK_WB_INC;
    }
    else { /* Narrowband or G.722 */
        /*
         * G722 is a wideband coder but rtp timestamp
         * increments at 8K rate as described in rfc3551.   
         */
        dsp_ptr->jbPkt.flags = 0;
    }
}

/*
 * ======== _VTSPR_rtpInitSeq() ========
 *
 */
void _VTSPR_rtpInitSeq(
    _VTSPR_RtpObject *rtp_ptr,
    uint16            sequenceNumber)
{
    rtp_ptr->baseSequence = sequenceNumber;
    rtp_ptr->maxSequence = sequenceNumber;
    rtp_ptr->badSequence = _VTSPR_RTP_SEQ_MOD + 1;
    rtp_ptr->cycles = 0;
    rtp_ptr->received = 0;
    rtp_ptr->receivedPrior = 0;
    rtp_ptr->expectedPrior = 0;

    return;
}

/*
 * ======== _VTSPR_rtpUpdateArr() ========
 *
 * This fuction calculates the inter-arrival jitter times using the algorithm
 * specificied in RFC 3550. Note that the timer has minimum units of 10ms.
 */
void _VTSPR_rtpUpdateArr(
    _VTSPR_RtpObject *rtp_ptr)
{
    int32  difference;
    int32  jitter;
    uint32 receiveTime;

    receiveTime = rtp_ptr->receiveTime;
    if (rtp_ptr->lastReceiveTime != 0) {
        difference = (int32)(rtp_ptr->recvRtpObj.pkt.rtpMinHdr.ts -
            rtp_ptr->lastTimeStamp) -
            (int32)(receiveTime - rtp_ptr->lastReceiveTime);

        if (difference < 0) {
            difference = (-difference);
        }
        jitter = rtp_ptr->jitter;
        rtp_ptr->jitter = jitter + ((difference - jitter) >> 4);
    }

    rtp_ptr->lastTimeStamp = rtp_ptr->recvRtpObj.pkt.rtpMinHdr.ts;
    rtp_ptr->lastReceiveTime = receiveTime;
}

/*
 * ======== _VTSPR_rtpUpdateSeq() ========
 *
 * This function is used update statistics based in incoming RTP packets.
 */
vint _VTSPR_rtpUpdateSeq(
    _VTSPR_RtpObject   *rtp_ptr)
{
    uint16 sequenceNumber;
    uint16 delta;

    sequenceNumber = rtp_ptr->recvRtpObj.pkt.rtpMinHdr.seqn;
    delta = sequenceNumber - rtp_ptr->maxSequence;

    if (rtp_ptr->probation > _VTSPR_RTP_MIN_SEQUENTIAL) {
        /*
         * The stream has just started, initialize parameters.
         */
        _VTSPR_rtpInitSeq(rtp_ptr, sequenceNumber);
        rtp_ptr->probation = _VTSPR_RTP_MIN_SEQUENTIAL;
        return (1);
    }

    if (rtp_ptr->probation != 0) {
        if (sequenceNumber == (rtp_ptr->maxSequence + 1)) {
            rtp_ptr->probation--;
            rtp_ptr->maxSequence = sequenceNumber;
            if (0 == rtp_ptr->probation) {
                _VTSPR_rtpInitSeq(rtp_ptr, sequenceNumber);
                rtp_ptr->received++;
                return (1);
            }
        }
        else {
            rtp_ptr->probation = _VTSPR_RTP_MIN_SEQUENTIAL - 1;
            rtp_ptr->maxSequence = sequenceNumber;
        }
        return (0);
    }
    else if (delta < _VTSPR_RTP_MAX_DROPOUT) {
        if (sequenceNumber < rtp_ptr->maxSequence) {
            rtp_ptr->cycles += _VTSPR_RTP_SEQ_MOD;
        }
        rtp_ptr->maxSequence = sequenceNumber;
    }
    else if (delta <= (_VTSPR_RTP_SEQ_MOD - _VTSPR_RTP_MAX_MISORDER)) {
        if (sequenceNumber == rtp_ptr->badSequence) {
            _VTSPR_rtpInitSeq(rtp_ptr, sequenceNumber);
        }
        else {
            rtp_ptr->badSequence = (sequenceNumber + 1) &
                (_VTSPR_RTP_SEQ_MOD - 1);
            return (0);
        }
    }
    rtp_ptr->received++;
    return (1);
}

/*
 * ======== _VTSPR_rtpRecv() ========
 *
 * This routine polls all sockets that are actively receiving data from the
 * network.
 */

vint _VTSPR_rtpRecv(
    VTSPR_Obj        *vtspr_ptr,
    VTSPR_Queues     *q_ptr,
    VTSPR_DSP        *dsp_ptr,
    _VTSPR_RtpObject *rtp_ptr)
{
    vint             infc;
    VTSPR_StreamObj *stream_ptr;
    VTSP_Stun        stunMsg;
    vint             pktLen;
    vint             streamId;
    OSAL_NetAddress  remoteAddr;
    JB_Payload       decType;
    uint16           locType;
    uint8            buf_ptr[RTP_BUFSZ_MAX];
#ifdef VTSP_ENABLE_T38
    _VTSPR_FR38Obj  *fr38_ptr;
    vint             numEntry;
    vint             pktWordSize;
    uint16            *tmp_ptr;
    VTSPR_ChanObj   *chan_ptr;
#endif
    uint32           streamMask;
    uint8            redunCnt;
    RTP_RdnObj      *rdnObj_ptr;
    RTP_Obj         *rtpObj_ptr;
    uint32           primaryTs;
    uint16           primaryCoder;
    uint16           primarySeqn;
    uint16           primaryPSize;
    uint16           tsOffset;
    uint16           length;
    uint16           payloadSz10ms;
    uint8            incrTime10ms;
    uint16           incrTime;
    uint32           seqnDisp;
    uint8            rdnEnabled;

    /*
     * If the UDP socket has data, read it and process it.
     * Note: infc increments to match the RTP object socket<->infc mapping,
     * set during initialization; i.e. must use FOR_ALL_INFC_PP, infc++, here.
     */
    _VTSPR_FOR_ALL_INFC_PP(infc) {
#ifdef VTSP_ENABLE_T38
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#endif
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            
            if (_VTSPR_RTP_NOT_BOUND == rtp_ptr->inUse) {
                /*
                 * If the RTP data stream is ready. Poll it for data. If not,
                 * go to the next RTP object.
                 */
                rtp_ptr++;
                continue;
            }

            streamMask = stream_ptr->algStreamState;

            /*
             * If the socket is open update the timer associated with the RTP
             * stream. This timer used for jitter calculations.
             */
            rtp_ptr->receiveTime += 80;
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
            while ((pktLen = _VTSPR_netRecvfrom(rtp_ptr->socket, buf_ptr,
                    RTP_BUFSZ_MAX, &remoteAddr)) > 0) {
                /*
                 * Add to bitrate;
                 * Assume 20 bytes IPv4 header.
                 * Assume 8 bytes UDP header.
                 * Times 8 for bits.
                 * This is actual packet bitrate incident on the receive side.
                 */
                vtspr_ptr->bitrate += ((pktLen + 20 + 8) << 3);

                /*
                 * Demux packets.
                 * Bug 2542
                 */
                if ((RTP_VERSION != ((*buf_ptr >> 6) & 3)) 
                        && (0 == (VTSPR_ALG_STREAM_T38 & streamMask))) {
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
                                VTSP_TIMEOUT_NO_WAIT, NULL)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        /* Set STUN event Flag */
                        stream_ptr->stunEvent = VTSP_EVENT_ACTIVE;
                    }
                    continue;
                 }

                /*
                 * If the IP address matches the expected address, and T38 is
                 * active on this stream's channel, process the data as UDPTL
                 */
                if (_VTSPR_netIsSameAddr(remoteAddr, rtp_ptr->remoteAddr)
                        && (0 != (VTSPR_ALG_STREAM_T38 & streamMask))) {
#ifdef VTSP_ENABLE_T38
                    tmp_ptr = (uint16*)buf_ptr;
                    /*
                     * XXX: verify if pktLen would be odd or not
                     */
                    for (pktWordSize = (pktLen+1) >> 1; pktWordSize > 0; pktWordSize--) {
                        *tmp_ptr = OSAL_netNtohs(*tmp_ptr);
                        tmp_ptr++;
                    }

                    fr38_ptr = chan_ptr->fr38_ptr;
                    /*
                     * Check if packet length coming in is with in the
                     * allocated jitter buffer size.
                     */
                    if (pktLen <= FR38_MAX_T38_PACKET) {
                        OSAL_memCpy(fr38_ptr->t38ObjIn.data, 
                                buf_ptr, pktLen);
                        fr38_ptr->t38ObjIn.numBits = pktLen << 3;
                    }
                    else {
                        fr38_ptr->fr38Event = VTSP_EVENT_T38_PKT_SZ_E;
                        continue;
                    }
                    numEntry = FR38_putPacket(stream_ptr->fr38Obj_ptr);

                    if (numEntry < 0) {
                        /*
                         * Could not place packet in jitter buffer.
                         * Report event that jitter buffer full.
                         */
                        fr38_ptr->fr38Event = VTSP_EVENT_T38_BUF_FULL;
                    }
                    else if (FR38_NOT_FAX == numEntry) {
                        fr38_ptr->fr38Event = VTSP_EVENT_T38_NOT_FAX;
                    }
#endif
                }
                /*
                 * If the IP address matches the expected address, and the
                 * jitter buffer is enabled, and the RTP stream is marked as
                 * ready, process the data into the jitter buffer.
                 */
                else if (
#ifndef VTSP_ENABLE_NET_LOOPBACK
                        (_VTSPR_netIsSameAddr(remoteAddr, 
                                rtp_ptr->remoteAddr)) &&
#endif
                        (0 != (VTSPR_ALG_STREAM_JB & streamMask)) 
                        && (_VTSPR_RTP_READY == rtp_ptr->recvActive)) {

                    rtpObj_ptr = &(rtp_ptr->recvRtpObj);
                    rdnEnabled = 0;
                    if (NULL != rtpObj_ptr->rdnCache_ptr) {
                        rdnEnabled = rtpObj_ptr->rdnCache_ptr->level;
                        if (rdnEnabled) {
                            rtpObj_ptr->rdnCache_ptr->avail = 
                                    RTP_REDUN_LEVEL_MAX;
                        } else {
                            rtpObj_ptr->rdnCache_ptr->avail = 0;
                        }
                    }

#ifdef VTSP_ENABLE_BENCHMARK
                    _VTSPR_benchmarkStart(vtspr_ptr,
                            _VTSPR_BENCHMARK_RTP_RECV, 0);
#endif    
                    /*
                     * Parse payload. The input packet to RTP is in buf_ptr. The
                     * output is placed in recvRtpObj.pkt.payload.
                     */
                    if (RTP_OK == RTP_recvPkt(rtpObj_ptr, buf_ptr, pktLen)) {
#ifdef VTSP_ENABLE_BENCHMARK
                        _VTSPR_benchmarkStop(vtspr_ptr,
                                _VTSPR_BENCHMARK_RTP_RECV, 0);
#endif
                        dsp_ptr->jbPkt.valid = JB_PKT_VALID;
                        dsp_ptr->jbPkt.atime = VTSPR_getTime(dsp_ptr);

                        /* Put redundant info and data into Jitter Buffer */
                        primaryTs    = rtpObj_ptr->pkt.rtpMinHdr.ts;
                        primarySeqn  = rtpObj_ptr->pkt.rtpMinHdr.seqn;
                        primaryPSize = rtpObj_ptr->payloadSize;
                        primaryCoder = _VTSPR_dynamicToLocalDecoder(
                                &(stream_ptr->streamParam),
                                rtpObj_ptr->pkt.rtpMinHdr.type);
                        rtp_ptr->localDecoder = primaryCoder;

                        if (rdnEnabled) {
                            /* Get the payload size of 10ms for this coder */
                            payloadSz10ms = _VTSPR_get10msSize(primaryCoder);
                            /*
                             * Get the timestamp incremental interval of 10ms,
                             * typically 80.
                             */
                            incrTime10ms = _VTSPR_getIncrTime(primaryCoder,
                                    primaryCoder);
                            /*
                             * Calculate the timestamp incremental interval
                             * for current pRate.
                             */   
                            incrTime = incrTime10ms *
                                       (primaryPSize / payloadSz10ms);
                            if (incrTime < incrTime10ms) {
                                /* Invalid incrTime, the primary may be a CN */
                                redunCnt = 0;   /* Ignore all redundant data */
                            }
                            else {
                                /* Amount of redundant date passed from RTP */
                                redunCnt = rtpObj_ptr->rdnCache_ptr->avail;
                            }
                        }
                        else {                  /* RFC2198 disabled */
                            redunCnt = 0;       /* Discard redundant data */
                        }

                        while (redunCnt) {
                            rdnObj_ptr = &(rtpObj_ptr->rdnCache_ptr->rdnObj_ary[--redunCnt]);
                            seqnDisp = rdnObj_ptr->hdr.tStamp / incrTime;
                            if ((seqnDisp < 1) || 
                                    (seqnDisp > _VTSPR_SEQN_DISP_MAX)) {
                                /* Invalid seqnDisp  (too large or too small) */
                                continue;      /* Ignore this redundant datum */
                            }

                            locType = _VTSPR_dynamicToLocalDecoder(
                                    &(stream_ptr->streamParam),
                                    rdnObj_ptr->hdr.blkPT);
                            dsp_ptr->jbPkt.type
                                    = _VTSPR_localCoderToJbCoder(locType);

                            tsOffset = rdnObj_ptr->hdr.tStamp;
                            dsp_ptr->jbPkt.ts = primaryTs - tsOffset;

                            length  = rdnObj_ptr->hdr.blkLen;
                            dsp_ptr->jbPkt.pSize = length;
                            dsp_ptr->jbPkt.seqn  = primarySeqn - seqnDisp;

                            _VTSPR_rtpGetTsIncRate(dsp_ptr, locType);

                            /*
                             * Set DTMFR sample rate,
                             * if decType is JB_PT_DTRLY or JB_PT_TONERLY
                             */
                            if ((JB_PT_DTRLY == dsp_ptr->jbPkt.type) ||
                                    (JB_PT_TONERLY == dsp_ptr->jbPkt.type)) {
                                if (0 != (stream_ptr->streamParam.extension &
                                        VTSP_MASK_EXT_DTMFR_16K)) {
                                    dsp_ptr->jbPkt.flags |=
                                            JB_PKT_FLAGS_MASK_DTMFR_16K;
                                }
                                else {
                                    dsp_ptr->jbPkt.flags |=
                                            JB_PKT_FLAGS_MASK_DTMFR_8K;
                                }
                            }

                            OSAL_memCpy(dsp_ptr->jbPkt.payload,
                                    rdnObj_ptr->data, length);
#ifdef VTSP_ENABLE_BENCHMARK
                            _VTSPR_benchmarkStart(vtspr_ptr,
                                    _VTSPR_BENCHMARK_JB_PUT, 1);
#endif
                            JB_putPkt(&stream_ptr->jbObj, &dsp_ptr->jbPkt);
#ifdef VTSP_ENABLE_BENCHMARK
                            _VTSPR_benchmarkStop(vtspr_ptr,
                                    _VTSPR_BENCHMARK_JB_PUT, 1);
#endif
                        }

                        /*
                         * Block primary data into jitter buffer
                         *
                         * voice data is recvRtpObj.pkt.payload
                         *
                         * Map dynamic coder type to VTSPR coder type.
                         * Then map VTSPR coder type to JB coder type.
                         *
                         */
                        decType = _VTSPR_localCoderToJbCoder(primaryCoder);
                        dsp_ptr->jbPkt.type  = decType;
                        dsp_ptr->jbPkt.ts    = primaryTs;
                        dsp_ptr->jbPkt.seqn  = primarySeqn;
                        dsp_ptr->jbPkt.pSize = primaryPSize;
                        _VTSPR_rtpGetTsIncRate(dsp_ptr, primaryCoder);
                        /*
                         * Set DTMFR sample rate,
                         * if decType is JB_PT_DTRLY or JB_PT_TONERLY
                         */
                        if ((JB_PT_DTRLY == decType) ||
                                (JB_PT_TONERLY == decType)) {
                            if (0 != (stream_ptr->streamParam.extension &
                                    VTSP_MASK_EXT_DTMFR_16K)) {
                                dsp_ptr->jbPkt.flags |=
                                        JB_PKT_FLAGS_MASK_DTMFR_16K;
                            }
                            else {
                                dsp_ptr->jbPkt.flags |=
                                        JB_PKT_FLAGS_MASK_DTMFR_8K;
                            }
                        }

                        /*
                         * copy payload from recvRtpObj.pkt.payload to
                         * jbPkt.payload.
                         */
                        OSAL_memCpy(dsp_ptr->jbPkt.payload, 
                                rtpObj_ptr->pkt.payload, primaryPSize);

#ifdef VTSP_ENABLE_BENCHMARK
                        _VTSPR_benchmarkStart(vtspr_ptr,
                                _VTSPR_BENCHMARK_JB_PUT, 1);
#endif 
                        JB_putPkt(&stream_ptr->jbObj, &dsp_ptr->jbPkt); 
#ifdef VTSP_ENABLE_BENCHMARK
                        _VTSPR_benchmarkStop(vtspr_ptr, 
                                _VTSPR_BENCHMARK_JB_PUT, 1);
#endif 
                        /*
                         * Update RTCP sequence statistics.
                         */
                        _VTSPR_rtpUpdateSeq(rtp_ptr);
                        
                        /*
                         * Update RTCP arrival statistics.
                         */
                        _VTSPR_rtpUpdateArr(rtp_ptr);

                    }
                }
                else {
                    /* Pkt received on open socket but inactive stream */
                    /* _VTSP_TRACE(__FILE__, __LINE__); */
                }
            }
            rtp_ptr++;
        }
    }
    return (_VTSPR_RTP_OK);
}
