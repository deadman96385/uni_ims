/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */
#include "_ve_private.h"


/*
 * ======== _VE_genEventStreamQuery() ========
 *
 * Generate multiple reporting events for Stream Query.
 * 
 */
void _VE_genEventStreamQuery(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr,
    vint          infc,
    vint          streamId)
{
    _VE_StreamObj   *stream_ptr;
    _VE_RtpObject  *rtp_ptr;
    _VTSP_RtcpCmdMsg   rtcpCmdMsg;
    uvint              delay;

    stream_ptr  = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
    rtp_ptr     =_VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc, streamId);

    /* RR */
    _VE_rtcpReceiverBlock(rtp_ptr, &rtcpCmdMsg, 0);

    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.msg.rtp.reason   = VTSP_EVENT_RTCP_RR;
    q_ptr->eventMsg.msg.rtp.streamId = streamId;
    q_ptr->eventMsg.msg.rtp.ssrc =
            OSAL_netHtonl(rtp_ptr->sendRtpObj.pkt.rtpMinHdr.ssrc);
    /*
     * arg1 = frac lost+cum pkt lost
     * arg2 = n/a
     * arg3 = ext hi seq
     * arg4 = jitter
     * arg5 = recv pkt count
     * arg6 = recv byte count
     */
    q_ptr->eventMsg.msg.rtp.arg1 = rtcpCmdMsg.msg.payload[1];
    q_ptr->eventMsg.msg.rtp.arg2 = 0;
    q_ptr->eventMsg.msg.rtp.arg3 = 0;
    q_ptr->eventMsg.msg.rtp.arg4 = rtcpCmdMsg.msg.payload[3];
    q_ptr->eventMsg.msg.rtp.arg5 = stream_ptr->dec.decodePkt;
    q_ptr->eventMsg.msg.rtp.arg6 = stream_ptr->dec.decodeBytes;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* SR */
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.msg.rtp.reason   = VTSP_EVENT_RTCP_SR;
    q_ptr->eventMsg.msg.rtp.streamId = streamId;
    q_ptr->eventMsg.msg.rtp.ssrc = rtp_ptr->sendRtpObj.pkt.rtpMinHdr.ssrc;
    /*
     * arg1 = ntp hi (0)
     * arg2 = ntp lo (0)
     * arg3 = rtp timestamp (machine byte order)
     * arg4 = send pkt count
     * arg5 = send byte count
     */
    q_ptr->eventMsg.msg.rtp.arg1 = 0;
    q_ptr->eventMsg.msg.rtp.arg2 = 0;
    q_ptr->eventMsg.msg.rtp.arg3 = rtp_ptr->sendRtpObj.pkt.rtpMinHdr.ts;
    q_ptr->eventMsg.msg.rtp.arg4 = stream_ptr->enc.encodePkt;
    q_ptr->eventMsg.msg.rtp.arg5 = stream_ptr->enc.encodeBytes;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* SS */
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.msg.rtp.reason   = VTSP_EVENT_RTCP_SS;
    q_ptr->eventMsg.msg.rtp.streamId = streamId;
    q_ptr->eventMsg.msg.rtp.ssrc     = rtp_ptr->sendRtpObj.pkt.rtpMinHdr.ssrc;
    /*
     * arg1 = flags
     * arg2 = beg_end seq
     * arg3 = lost_pkts
     * arg4 = dup pkts
     * arg5 = mean jit
     * arg6 = min jit
     * arg7 = max jit
     * arg8 = dev jit
     */
    q_ptr->eventMsg.msg.rtp.arg1 = 0;
    q_ptr->eventMsg.msg.rtp.arg2 = 0;
    q_ptr->eventMsg.msg.rtp.arg3 = 0;
    q_ptr->eventMsg.msg.rtp.arg4 = 0;
    q_ptr->eventMsg.msg.rtp.arg5 = 0;
    q_ptr->eventMsg.msg.rtp.arg6 = 0;
    q_ptr->eventMsg.msg.rtp.arg7 = 0;
    q_ptr->eventMsg.msg.rtp.arg8 = 0;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* MR */
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.msg.rtp.reason   = VTSP_EVENT_RTCP_MR;
    q_ptr->eventMsg.msg.rtp.streamId = streamId;
    q_ptr->eventMsg.msg.rtp.ssrc     = rtp_ptr->sendRtpObj.pkt.rtpMinHdr.ssrc;
    /* 
     * arg1 = loss rate
     * arg2 = discard rate
     * arg3 = end sys delay (estimate, or zero)
     * arg4 = rerl
     * arg5 = jb nom
     * arg6 = jb max
     * arg7 = jb abs max
     */
    /* end sys delay = jit + decode + encode + playout delays(VHW) */
    delay                        = 0;
    q_ptr->eventMsg.msg.rtp.arg1 = 0;
    q_ptr->eventMsg.msg.rtp.arg2 = 0;
    q_ptr->eventMsg.msg.rtp.arg3 = delay;
    q_ptr->eventMsg.msg.rtp.arg5 = 0;
    q_ptr->eventMsg.msg.rtp.arg6 = 0;
    q_ptr->eventMsg.msg.rtp.arg7 = 0;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* CS */
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.msg.rtp.reason   = VTSP_EVENT_RTCP_CS;
    q_ptr->eventMsg.msg.rtp.streamId = streamId;
    q_ptr->eventMsg.msg.rtp.ssrc     = rtp_ptr->sendRtpObj.pkt.rtpMinHdr.ssrc;
    /* 
     * arg1 = enc bytes
     * arg2 = dec bytes
     * arg3 = enc pkts
     * arg4 = dec pkts
     * arg5 = enc cn pkts
     * arg6 = dec cn pkts
     * arg7 = run plc
     * arg8 = run nse
     * arg9 = Avg Ticks-Mhz
     */
    q_ptr->eventMsg.msg.rtp.arg1 = stream_ptr->enc.encodeBytes;
    q_ptr->eventMsg.msg.rtp.arg2 = stream_ptr->dec.decodeBytes;
    q_ptr->eventMsg.msg.rtp.arg3 = stream_ptr->enc.encodePkt;
    q_ptr->eventMsg.msg.rtp.arg4 = stream_ptr->dec.decodePkt;
    q_ptr->eventMsg.msg.rtp.arg5 = 0;
    q_ptr->eventMsg.msg.rtp.arg6 = 0;
    q_ptr->eventMsg.msg.rtp.arg7 = 0;
    q_ptr->eventMsg.msg.rtp.arg8 = 0;
    q_ptr->eventMsg.msg.rtp.arg9 = 0;
    _VE_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
    return;
}


/*
 * ======== _VE_genEventStream() ========
 *
 * Get msg from pool, fill from event msg,
 * 
 */
void _VE_genEventStream(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr)
{
    _VE_StreamObj  *stream_ptr;
    vint              infc;
    vint              streamId;

    /*
     * Stream Events
     */
    for (infc = VTSP_INFC_VIDEO; infc == VTSP_INFC_VIDEO; infc++) {
        for (streamId = 0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            /* Check STUN events for received packets */
            if (VTSP_EVENT_ACTIVE == stream_ptr->dec.stunEvent) {
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_STUN_RECV;
                q_ptr->eventMsg.infc = infc;
                _VE_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                stream_ptr->dec.stunEvent = VTSP_EVENT_INACTIVE;
            }
        }
    }
}
