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
 * ======== _VC_rtpSendTx() ========
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
void _VC_rtpSendTx(
    _VC_Obj        *vc_ptr,
    _VC_RtpObject *rtp_ptr,
    VTSP_BlockHeader *hdr_ptr)
{
    vint sendErr;
    uint32 opts;
    
    /*
     * Create the RTP payload.  Place it in netBuffer[]
     */
    if (0 != (hdr_ptr->extension & VTSP_MASK_EXT_MARKER)) {
        opts = RTP_MARKER;
        /* Packet is Marked. Check if Extension bit should also be set. */
        if (0 != (hdr_ptr->extension & VTSP_MASK_EXT_RTP_EXTN)) {
            opts |= RTP_EXTENSION;
        }
        RTP_setOpts(&(rtp_ptr->sendRtpObj), opts);
    }
    else {
        RTP_setOpts(&(rtp_ptr->sendRtpObj), 0);
    }

    /*
     * Set timestamp for this transmission
     */
    rtp_ptr->sendRtpObj.tStamp = rtp_ptr->rtpTime;

    /*
     * Create the RTP payload.  Place it in netBuffer[]
     */
    RTP_xmitPkt(&(rtp_ptr->sendRtpObj), rtp_ptr->netBuffer,
            rtp_ptr->payloadOffset,
            rtp_ptr->lastCoder);

    DBG("RTP Send Seqn: %d", rtp_ptr->sendRtpObj.pkt.rtpMinHdr.seqn);

    /*
     * Copy the packet length to netBufferLen
     */
    rtp_ptr->netBufferLen = rtp_ptr->sendRtpObj.packetSize;

    /*
     * Update counts for RTCP.
     */
    OSAL_semAcquire(rtp_ptr->info.mutexLock, OSAL_WAIT_FOREVER);
    rtp_ptr->info.sendPacketCount += 1;
    rtp_ptr->info.sendOctetCount += rtp_ptr->payloadOffset;
    OSAL_semGive(rtp_ptr->info.mutexLock);

    /*
     * Send it to the network
     */
    if (rtp_ptr->netBufferLen > 0 && (sendErr = _VC_netSendto(
            rtp_ptr->socket,
            rtp_ptr->netBuffer,
            rtp_ptr->netBufferLen,
            rtp_ptr->remoteAddr))
            != rtp_ptr->netBufferLen) {
        DBG("sendto error %d:", sendErr);
    }
    else {
        /*
         * Add to bitrate;
         * Assume 20 bytes IPv4 header.
         * Assume 8 bytes for UDP
         * Times 8 for bits.
         * This is actual packet bitrate incident on the receive side.
         */
        vc_ptr->bitrateEnc += ((rtp_ptr->netBufferLen + 20 + 8) << 3);
    }
}

/*
 * ======== _VC_rtpSend() ========
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
void _VC_rtpSend(
    _VC_Obj        *vc_ptr,
    _VC_RtpObject *rtp_ptr,
    VTSP_BlockHeader *hdr_ptr,
    uint8            *data_ptr,
    vint              sendBytes,
    vint              frameCnt)
{

    /*
     * First check to see if the packet is destined for an open stream.
     */
    if (rtp_ptr->sendActive != _VC_RTP_READY) {
        /* _VC_TRACE(__FILE__, rtp_ptr->rtpTime); */
        return;
    }

    /*
     * Always update the dynamic coder, and the previous local coder.
     */
    rtp_ptr->lastCoder = hdr_ptr->dynamicCoder;
    rtp_ptr->lastLocalCoder = hdr_ptr->localCoder;

    DBG("");

    if ((hdr_ptr->extension & VTSP_MASK_EXT_SEND) &&
        (rtp_ptr->payloadOffset != 0)) {
        /*
         * SEND bit is set, send the payload now, on this cycle.
         */
        _VC_rtpSendTx(vc_ptr, rtp_ptr, hdr_ptr);
        rtp_ptr->payloadOffset = 0;
    }
}

