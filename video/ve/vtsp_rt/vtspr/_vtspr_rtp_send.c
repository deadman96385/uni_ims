/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28012 $ $Date: 2014-08-08 17:36:29 +0800 (Fri, 08 Aug 2014) $
 *
 */

#include "_vtspr_rtp.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_getIncrTime() ========
 *
 * Find the amount to increment the rtpTime, depending on coder and lastVCoder
 * This routine implements Table 3 of the SDD.
 * Inputs:   coder      (local coder, not dynamic, from ???)
 *           lastVCoder (local coder, from VTSPR_RtpObject.lastLocVCoder)
 * Outputs:
 * Return value: the number (in samples) to add to rtpTime; either 80 or 160
 */
uint32 _VTSPR_getIncrTime(vint coder, vint lastVCoder)
{
    vint    speechCoder;
    uint32  retVal;

    speechCoder = (_VTSPR_isCoderPassthrough(coder) ? lastVCoder : coder);
    retVal = (_VTSPR_isCoderWb(speechCoder) ?
                    VTSPR_NSAMPLES_10MS_16K : VTSPR_NSAMPLES_10MS_8K);

    if (VTSP_CODER_G722 == speechCoder) {
        /*
         * G722 is a wideband coder but rtp timestamp
         * increments at 8000 rate as described in
         * rfc3551.   
         */   
         retVal = VTSPR_NSAMPLES_10MS_8K;
     }

    return (retVal);
}

/*
 * ======== _VTSPR_getRtpClockRate() ========
 *
 * Gets the clock rate depending on coder
 * Inputs:   coder      (local coder)
 * Outputs:
 * Return value: the sampling rate of the coder in KHZ, either 8 or 16
 */
uint32 _VTSPR_getRtpClockRate(vint coder)
{
    uint32  retVal;
    retVal = (_VTSPR_isCoderWb(coder) ?
            VTSPR_SAMPLING_RATE_16KHZ : VTSPR_SAMPLING_RATE_8KHZ);

    if (VTSP_CODER_G722 == coder) {
        /*
         * G722 is a wideband coder but rtp timestamp
         * increments at 8000 rate as described in
         * rfc3551.
         */
         retVal = VTSPR_SAMPLING_RATE_8KHZ;
     }

    return (retVal);
}

/*
 * ======== _VTSPR_getPktTStamp() ========
 *
 * Find the value to put in the RTP timestamp.
 * Implements Table 4 of the SDD using rtpTime and localCoder.
 * Inputs:   coder     (local coder type, from BlockHeader.localCoder)
 *           VTSPR_RtpObject, members lastDtmfTime, rtpTime, lastLocVCoder
 *           framesM1: number of 10 ms frames in coded packet, minus 1
 *                  (0 <= framesM1 <= 5)
 * Outputs:
 *           modified VTSPR_RtpObject, members lastDtmfTime
 * Return value: the timestamp to put into the RTP packet
 */
static uint32 _VTSPR_getPktTStamp(
        _VTSPR_RtpObject *rtp_ptr,
        vint              coder,           /* from BlockHeader.localCoder */
        vint              framesM1)
{
    if (VTSP_CODER_CN == coder) {   /* comfort noise */
        rtp_ptr->lastDtmfTime = _VTSPR_RTP_NOT_DTMF_PKT;
        return rtp_ptr->rtpTime;
    }
    else if (VTSP_CODER_DTMF == coder) {
        if (_VTSPR_RTP_NOT_DTMF_PKT == rtp_ptr->lastDtmfTime) { /* 1st */
            rtp_ptr->lastDtmfTime =
                    rtp_ptr->rtpTime - ((
                    _VTSPR_getIncrTime(coder, rtp_ptr->lastLocVCoder)
                    * 7) >> 1);
        }
        return rtp_ptr->lastDtmfTime; /* 1st DTMF pkt, and subsequent pkts */
    }
    else {
        /* voice codec, VTSP_CODER_TONE, or any remaining coder type */
        rtp_ptr->lastDtmfTime = _VTSPR_RTP_NOT_DTMF_PKT;
        if (framesM1 > 0) {
            return (rtp_ptr->rtpTime -
                    (_VTSPR_getIncrTime(coder, rtp_ptr->lastLocVCoder) *
                    framesM1));
        }
        else { /* for speed, don't call _VTSPR_getIncrTime() if framesM1==0 */
            return (rtp_ptr->rtpTime);
        }
    }
}

/*
 * ======== _VTSPR_rtpSendTx() ========
 *
 * Format and send RTP packet to the network.
 * Inputs:   &RtpObject,   all members
 *           &BlockHeader: members extension, dynamicCoder, localCoder,
 * Outputs:
 *           &RtpObject:   members sentRtpObj, sendPacketCount, sendOctetCount,
 *                                 netBufferLen,
 *                                 lastCoder, lastLocalCoder, payloadOffset
 *           (&BlockHeader)->extension
 * Return value: None
 */
void _VTSPR_rtpSendTx(
    VTSPR_Obj        *vtspr_ptr,
    _VTSPR_RtpObject *rtp_ptr,
    VTSP_BlockHeader *hdr_ptr)
{
    vint sendErr;
    OSAL_SelectTimeval timeval;

    /*
     * Create the RTP payload.  Place it in netBuffer[]
     */
    if (0 != (hdr_ptr->extension & VTSP_MASK_EXT_MARKER)) {
        RTP_setOpts(&(rtp_ptr->sendRtpObj), RTP_MARKER);
    }
    else {
        RTP_setOpts(&(rtp_ptr->sendRtpObj), 0);
    }
    /*
     * Set timestamp for this transmission
     */
    rtp_ptr->sendRtpObj.tStamp = _VTSPR_getPktTStamp(
            rtp_ptr,
            hdr_ptr->localCoder,
            rtp_ptr->framesM1);

    if (rtp_ptr->firstRtpTimeMs == 0) {
        /*
         * For the first frame, Use the randomly initialized rtpTime.
         * Also record the OSAL Time so that we can calculate the time elapsed
         * when sending RTCP SR. This is approximately the sending time.
         * NOTE: For improved accuracy, rtp_ptr->firstRtpTimeMs value should be adjusted by calculating
         * AudioSampling-to-Send path latency and subtracting it.
         */
        OSAL_selectGetTime(&timeval);
        rtp_ptr->firstRtpTimeMs = (timeval.sec * 1000) + (timeval.usec / 1000);
    }

#ifdef VTSP_ENABLE_BENCHMARK
    _VTSPR_benchmarkStart(vtspr_ptr, _VTSPR_BENCHMARK_RTP_XMIT, 0);
#endif

    /*
     * Create the RTP payload.  Place it in netBuffer[]
     */
    RTP_xmitPkt(&(rtp_ptr->sendRtpObj), rtp_ptr->netBuffer,
            rtp_ptr->payloadOffset,
            rtp_ptr->lastCoder);

#ifdef VTSP_ENABLE_BENCHMARK
    _VTSPR_benchmarkStop(vtspr_ptr, _VTSPR_BENCHMARK_RTP_XMIT, 0);
#endif

    /*
     * Copy the packet length to netBufferLen
     */
    rtp_ptr->netBufferLen = rtp_ptr->sendRtpObj.packetSize;

    /*
     * Update counts for RTCP.
     */
    rtp_ptr->sendPacketCount += 1;
    rtp_ptr->sendOctetCount += rtp_ptr->payloadOffset;

    /*
     * Send it to the network
     */
    if (rtp_ptr->netBufferLen > 0 && (sendErr = _VTSPR_netSendto(
            rtp_ptr->socket,
            rtp_ptr->netBuffer,
            rtp_ptr->netBufferLen,
            rtp_ptr->remoteAddr))
            != rtp_ptr->netBufferLen) {
        _VTSP_TRACE(__FILE__, __LINE__);
        _VTSP_TRACE("sendto error:", sendErr);
    }
    else {
        /*
         * Add to bitrate;
         * Assume 20 bytes IPv4 header.
         * Assume 8 bytes for UDP
         * Times 8 for bits.
         * This is actual packet bitrate incident on the receive side.
         */
        vtspr_ptr->bitrate += ((rtp_ptr->netBufferLen + 20 + 8) << 3);
    }
}

/*
 * ======== _VTSPR_rtpSend() ========
 *
 * Process data to be sent to the network.
 * Inputs:   &RtpObject,   all members
 *           &BlockHeader: members extension, dynamicCoder, localCoder,
 *           data_ptr[]
 * Outputs:
 *           &RtpObject:   members sentRtpObj, sendPacketCount, sendOctetCount,
 *                                 netBufferLen,
 *                                 lastCoder, lastLocalCoder, payloadOffset
 *           (&BlockHeader)->extension
 * Return value: OSAL_SUCCESS or OSAL_FAIL
 */
void _VTSPR_rtpSend(
    VTSPR_Obj        *vtspr_ptr,
    _VTSPR_RtpObject *rtp_ptr,
    VTSP_BlockHeader *hdr_ptr,
    uint8            *data_ptr,
    vint              sendBytes,
    vint              frameCnt)
{
    uint8 *payload_ptr;

    /*
     * First check to see if the packet is destined for an open stream.
     */
    if (rtp_ptr->sendActive != _VTSPR_RTP_READY) {
        /* _VTSP_TRACE(__FILE__, rtp_ptr->rtpTime); */
        return;
    }

    if (0 != (hdr_ptr->extension & VTSP_MASK_EXT_DR)) {
        /*
         * If DTMF relay is active, dump any stale payload in the append buffer
         */
        rtp_ptr->payloadOffset = 0;
        hdr_ptr->extension   |= VTSP_MASK_EXT_APPEND;
    }

    if ((!(hdr_ptr->extension & VTSP_MASK_EXT_APPEND)) &&
            (rtp_ptr->payloadOffset != 0)) {
        /*
         * APPEND not set and stale payload data is in the send buffer.
         * Send the old payload here.
         */
        _VTSPR_rtpSendTx(vtspr_ptr, rtp_ptr, hdr_ptr);
        rtp_ptr->payloadOffset = 0;
    }

    /*
     * Always update the dynamic coder, and the previous local coder.
     */
    rtp_ptr->lastCoder = hdr_ptr->dynamicCoder;
    rtp_ptr->lastLocalCoder = hdr_ptr->localCoder;
    if (OSAL_FALSE == _VTSPR_isCoderPassthrough(hdr_ptr->localCoder)) {
        rtp_ptr->lastLocVCoder = hdr_ptr->localCoder;
    }

    /*
     * Place new voice payload in the send buffer. Note that if the
     * append bit was not set, any previous stale payload has been sent. 
     */
    if (((rtp_ptr->payloadOffset + sendBytes) <= RTP_BUFSZ_MAX)) {
        if (sendBytes != 0) {
            /*
             * Copy in the new payload
             * and update framesM1
             */
            payload_ptr = rtp_ptr->sendRtpObj.pkt.payload;
            payload_ptr += rtp_ptr->payloadOffset;
            OSAL_memCpy(payload_ptr, data_ptr, sendBytes);
            rtp_ptr->payloadOffset += sendBytes;
            rtp_ptr->framesM1 = frameCnt;
        }
    }
    else {
        /* Bug 3134, error condition, drop this packet to re-synch */
        _VTSP_TRACE(__FILE__, __LINE__);
        rtp_ptr->payloadOffset = 0;
        return;
    }

    if ((hdr_ptr->extension & VTSP_MASK_EXT_SEND) &&
        (rtp_ptr->payloadOffset != 0)) {
        /*
         * SEND bit is set, send the payload now, on this cycle.
         */
        _VTSPR_rtpSendTx(vtspr_ptr, rtp_ptr, hdr_ptr);
        rtp_ptr->payloadOffset = 0;
    }
}

