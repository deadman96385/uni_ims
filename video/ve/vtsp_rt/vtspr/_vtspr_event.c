/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30131 $ $Date: 2014-12-01 14:45:03 +0800 (Mon, 01 Dec 2014) $
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_genEventTone() ========
 *
 * Generate multiple reporting events for Tone.
 * 
 */
void _VTSPR_genEventTone(
    VTSPR_DSP       *dsp_ptr,
    VTSPR_Queues    *q_ptr,
    vint             infc,
    VTSPR_StreamObj *stream_ptr,
    VTSPR_ToneSeq   *tone_ptr)
{
    if (VTSP_EVENT_INACTIVE != tone_ptr->toneEvent) {
        /*
         * Tone is finished. Send tone complete event.
         */
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_GENERATE;
        q_ptr->eventMsg.infc = infc;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.toneGenerate.reason = tone_ptr->toneEvent;
        q_ptr->eventMsg.msg.toneGenerate.type   = tone_ptr->toneEventType;
        if (NULL != stream_ptr) { 
            /* Generate stream event details */
            q_ptr->eventMsg.msg.toneGenerate.direction = 
                    VTSP_EVENT_TONE_DIR_STREAM;
            q_ptr->eventMsg.msg.toneGenerate.streamId = 
                    stream_ptr->streamParam.streamId;
        }
        else {
            q_ptr->eventMsg.msg.toneGenerate.direction = 
                    VTSP_EVENT_TONE_DIR_LOCAL;
            /* Leave all other members as 0 */
        }
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

        tone_ptr->toneEvent = VTSP_EVENT_INACTIVE;
    }
}

/*
 * ======== _VTSPR_genEventDtmfRelay() ========
 *
 * Generate reports for DTMF Relay digit events
 *
 */
void _VTSPR_genEventDtmfRelay(
    VTSPR_DSP       *dsp_ptr,
    VTSPR_Queues    *q_ptr,
    vint             infc,
    vint             streamId)
{
#ifdef VTSP_ENABLE_DTMFR
    VTSPR_StreamObj    *stream_ptr;

    stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);

    if (0 != (stream_ptr->drEncodeObj.status & DR_DIGIT_END)) {
        /*
         * DTMF is finished. Send digit complete event.
         */
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_DIGIT_GENERATE;
        q_ptr->eventMsg.infc = infc;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.digitGenerate.reason = VTSP_EVENT_COMPLETE;
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        stream_ptr->drEncodeObj.status &= ~(DR_DIGIT_END);
    }

    if ((DR_EVENT_LE == stream_ptr->drDecodeObj_ptr->edge) ||
            (DR_EVENT_TE == stream_ptr->drDecodeObj_ptr->edge)) {
        /* Report RFC4733 DTMF detect event */
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
        q_ptr->eventMsg.infc = infc;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;

        q_ptr->eventMsg.msg.toneDetect.streamId  = streamId;
        q_ptr->eventMsg.msg.toneDetect.reason    = VTSP_EVENT_ACTIVE;
        q_ptr->eventMsg.msg.toneDetect.detect    = VTSP_DETECT_DTMF;
        q_ptr->eventMsg.msg.toneDetect.direction = VTSP_EVENT_TONE_DIR_STREAM;
        q_ptr->eventMsg.msg.toneDetect.tone      =
                stream_ptr->drDecodeObj_ptr->digit;
        if (DR_EVENT_LE == stream_ptr->drDecodeObj_ptr->edge) {
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_LEADING;
        }
        else {
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_TRAILING;
        }
        VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        /* Reset edge */
        stream_ptr->drDecodeObj_ptr->edge = 0;
    }
#endif
}

/*
 * ======== _VTSPR_genEventStreamQuery() ========
 *
 * Generate multiple reporting events for Stream Query.
 * 
 */
void _VTSPR_genEventStreamQuery(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr,
    vint          infc,
    vint          streamId)
{
#ifdef VTSP_ENABLE_ECSR
    VTSPR_ChanObj     *chan_ptr;
#endif
    VTSPR_StreamObj   *stream_ptr;
    _VTSPR_RtpObject  *rtp_ptr;
    JB_MsgRtp          jbRtpStat;
    _VTSP_RtcpCmdMsg   rtcpCmdMsg;
    uvint              delay;

    stream_ptr  = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#ifdef VTSP_ENABLE_ECSR
    chan_ptr    = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#endif
    rtp_ptr     =_VTSPR_streamIdToRtpPtr(vtspr_ptr->net_ptr, infc, streamId);

    /* RR */
    jbRtpStat.reason = JB_GETSTATS_REASON_RR;
    JB_getStats(&stream_ptr->jbObj, &jbRtpStat);
    _VTSPR_rtcpReceiverBlock(rtp_ptr, &rtcpCmdMsg, 0);

    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.tick             = dsp_ptr->tick1ms;
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
    q_ptr->eventMsg.msg.rtp.arg3 = jbRtpStat.arg3;
    q_ptr->eventMsg.msg.rtp.arg4 = rtcpCmdMsg.msg.payload[3];
    q_ptr->eventMsg.msg.rtp.arg5 = stream_ptr->count.decodePkt;
    q_ptr->eventMsg.msg.rtp.arg6 = stream_ptr->count.decodeBytes;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* SR */
    jbRtpStat.reason                 = JB_GETSTATS_REASON_SR;
    JB_getStats(&stream_ptr->jbObj, &jbRtpStat);
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.tick             = dsp_ptr->tick1ms;
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
    q_ptr->eventMsg.msg.rtp.arg4 = stream_ptr->count.encodePkt; 
    q_ptr->eventMsg.msg.rtp.arg5 = stream_ptr->count.encodeBytes;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* SS */
    jbRtpStat.reason                 = JB_GETSTATS_REASON_SS;
    JB_getStats(&stream_ptr->jbObj, &jbRtpStat);
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.tick             = dsp_ptr->tick1ms;
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
    q_ptr->eventMsg.msg.rtp.arg1 = jbRtpStat.arg1;
    q_ptr->eventMsg.msg.rtp.arg2 = jbRtpStat.arg2;
    q_ptr->eventMsg.msg.rtp.arg3 = jbRtpStat.arg3;
    q_ptr->eventMsg.msg.rtp.arg4 = jbRtpStat.arg4;
    q_ptr->eventMsg.msg.rtp.arg5 = jbRtpStat.arg5;
    q_ptr->eventMsg.msg.rtp.arg6 = jbRtpStat.arg6;
    q_ptr->eventMsg.msg.rtp.arg7 = jbRtpStat.arg7;
    q_ptr->eventMsg.msg.rtp.arg8 = jbRtpStat.arg8;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* MR */
    jbRtpStat.reason                 = JB_GETSTATS_REASON_MR;
    JB_getStats(&stream_ptr->jbObj, &jbRtpStat);
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.tick             = dsp_ptr->tick1ms;
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
    delay                        = jbRtpStat.arg5;
    delay += 20;    /* add rx/tx VHW delay */
    q_ptr->eventMsg.msg.rtp.arg1 = jbRtpStat.arg1;
    q_ptr->eventMsg.msg.rtp.arg2 = jbRtpStat.arg2;
    q_ptr->eventMsg.msg.rtp.arg3 = delay;
#ifdef VTSP_ENABLE_ECSR
    q_ptr->eventMsg.msg.rtp.arg4 = chan_ptr->ec_ptr->ecsrObj.rerl;
#endif
    q_ptr->eventMsg.msg.rtp.arg5 = jbRtpStat.arg5;
    q_ptr->eventMsg.msg.rtp.arg6 = jbRtpStat.arg6;
    q_ptr->eventMsg.msg.rtp.arg7 = jbRtpStat.arg7;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

    /* CS */
    q_ptr->eventMsg.code             = VTSP_EVENT_MSG_CODE_RTP;
    q_ptr->eventMsg.infc             = infc;
    q_ptr->eventMsg.tick             = dsp_ptr->tick1ms;
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
    q_ptr->eventMsg.msg.rtp.arg1 = stream_ptr->count.encodeBytes;
    q_ptr->eventMsg.msg.rtp.arg2 = stream_ptr->count.decodeBytes;
    q_ptr->eventMsg.msg.rtp.arg3 = stream_ptr->count.encodePkt;
    q_ptr->eventMsg.msg.rtp.arg4 = stream_ptr->count.decodePkt;
    q_ptr->eventMsg.msg.rtp.arg5 = stream_ptr->count.encodeCN;
    q_ptr->eventMsg.msg.rtp.arg6 = stream_ptr->count.decodeCN;
    q_ptr->eventMsg.msg.rtp.arg7 = stream_ptr->count.runPLC;
    q_ptr->eventMsg.msg.rtp.arg8 = stream_ptr->count.runNSE;
    q_ptr->eventMsg.msg.rtp.arg9 = dsp_ptr->measureAvg;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
    return;
}

/*
 * ======== _VTSPR_genEventFx() ========
 *
 * Get msg from pool, fill from event msg,
 * 
 */
void _VTSPR_genEventFx(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_ChanObj    *chan_ptr;
#ifndef VTSP_ENABLE_MP_LITE
    VTSPR_ToneSeq    *tone_ptr;
#endif
#ifdef VTSP_ENABLE_FMTD
    _VTSPR_FmtdObj   *fmtd_ptr; 
#endif 
#ifdef VTSP_ENABLE_T38
    _VTSPR_FR38Obj   *fr38_ptr; 
#endif  
#ifdef VTSP_ENABLE_CIDS
    _VTSPR_CidsObj   *cids_ptr;
    uint32            chanMask;
#endif
#ifdef VTSP_ENABLE_DTMF
    _VTSPR_DtmfObj   *dtmf_ptr; 
    _VTSPR_FlowObj   *flow_ptr;
    vint              type0;
    vint              tone0;
    vint              flowIndex;
    vint              le;
    vint              te;
#ifdef VTSP_ENABLE_DTMFR
    vint              dtmfPower;
    vint              dtmfRms;
#endif
#endif
    vint              infc;

    /* Audio processing events - tone detections, ...
     * --------
     */

    /* All Physical infc, FX Channel Events 
     */
    _VTSPR_FOR_ALL_FX(infc) {     /* All Physical infc */
        chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);
#ifdef VTSP_ENABLE_CIDS
        chanMask = _VTSPR_getAlgStateChan(chan_ptr, infc);;
#endif
#ifdef VTSP_ENABLE_T38
        fr38_ptr = chan_ptr->fr38_ptr;
        /* process FR38_run() return values that are events */
        if ((VTSP_EVENT_INACTIVE != fr38_ptr->fr38Event) && 
                (fr38_ptr->fr38EventLast != fr38_ptr->fr38Event)) {
            q_ptr->eventMsg.code           = VTSP_EVENT_MSG_CODE_T38;
            q_ptr->eventMsg.infc           = infc;
            q_ptr->eventMsg.tick           = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.t38.reason = fr38_ptr->fr38Event;
            fr38_ptr->fr38EventLast        = fr38_ptr->fr38Event;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }

        /* process FR38_run() events */
        _VTSPR_processT38EventLog(infc, vtspr_ptr, q_ptr, dsp_ptr);
#endif
#ifdef VTSP_ENABLE_DTMF
        dtmf_ptr = chan_ptr->dtmf_ptr;
        /*
         * Check for DTMF detect leading and trailing edges
         * using Bug 2976 enhancements
         */
        le = dtmf_ptr->dtmfLe;
        te = dtmf_ptr->dtmfTe;
        type0 = VTSP_EVENT_INACTIVE;
        tone0 = 0;

        if (le > -1) {
            /*
             * LE detected.
             */
            type0 = VTSP_EVENT_LEADING;
            tone0 = _VTSPR_dtmfToChar(le & 0xFF);
#ifdef VTSP_ENABLE_DTMFR
            /*
             * Assign digit power based on RMS of the signal
             * at leading edge detection.
             */
            dtmfRms = dtmf_ptr->dtmfObj.lowRms;

            /*
             * Step through the table of RMS values for dB levels 0 to -36
             * until the RMS of the DTMF digit is reached.
             */
            dtmfPower = -1;
            while ((dtmfPower++ < 36) &&
                    (dsp_ptr->dbTable_ptr[dtmfPower] > dtmfRms)) { 
                ;
            }

            /*
             * Since dtmfPower is constrained to be between 0 and 36 in the
             * above while loop, we do not have to mask the lower 0x3F bits
             * for insertion into the DR event object.
             */
            chan_ptr->drEventObj.newPower = dtmfPower;
            chan_ptr->drEventObj.relayMode = DR_RELAY_MODE_EVENT;
            chan_ptr->drEventObj.event |= DR_EVENT_LE;
            chan_ptr->drEventObj.newDigit = le & 0xFF;
            if (DR_SAMPLE_RATE_16K == chan_ptr->drDecodeObj.sampleRate) {
                chan_ptr->drEventObj.newPlayTime = DR_MAX_PLAYTIME_16K;
            }
            else {
                chan_ptr->drEventObj.newPlayTime = DR_MAX_PLAYTIME_8K;
            }
#endif
        }

        if (te > -1) {
            /*
             * TE detected.
             */
            type0 = VTSP_EVENT_TRAILING;
            tone0 = _VTSPR_dtmfToChar(te & 0xFF);
#ifdef VTSP_ENABLE_DTMFR
            chan_ptr->drEventObj.relayMode = DR_RELAY_MODE_EVENT;
            chan_ptr->drEventObj.event |= DR_EVENT_TE;
#endif
        }

#ifdef VTSP_ENABLE_CIDS
        if (0 != (VTSPR_ALG_CHAN_CIDCWS & chanMask)) {
            cids_ptr = chan_ptr->cids_ptr;
            /*
             * During CIDCWS, all DTMF events to VTSP are filtered.
             * The dtmf digit is reported to CIDCWS instead of the application.
             * Otherwise the application may receive DTMF digit
             * events during the CID ACK protocol.
             *
             * Also, during CIDCWS, events are filtered from DTMF Relay,
             * to disable DTMF Relay packet from the phone's DTMF ACK "D".
             * The DR encoder must be set to stop a current digit because
             * CIDCWS takes over the detector & terminates the DR encoded
             * digit.
             */
            type0 = VTSP_EVENT_INACTIVE;
#ifdef VTSP_ENABLE_DTMFR
            chan_ptr->drEventObj.event = 0;
            chan_ptr->drEventObj.stop = DR_STOP_SET;
#endif
            if (te > -1) { 
                /* Only detect at trailing edge of digit for CIDS */
                cids_ptr->cidcwsObj.dtmfDigit = te & 0xFF;
            }
        }
#endif /* VTSP_ENABLE_CIDS */

        if (VTSP_EVENT_INACTIVE != type0) {
            /* 
             * Report first (or only) DTMF detect event
             */
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.msg.toneDetect.reason = VTSP_EVENT_ACTIVE;
            q_ptr->eventMsg.msg.toneDetect.detect = VTSP_DETECT_DTMF;

            q_ptr->eventMsg.tick                    = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.edgeType = type0;
            q_ptr->eventMsg.msg.toneDetect.tone     = tone0;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
#endif /* VTSP_ENABLE_DTMF */

#ifdef VTSP_ENABLE_FMTD
        fmtd_ptr = chan_ptr->fmtd_ptr;
        /*
         * Report leading and trailing edge FMTD detections.
         *
         * fmtdLe and fmtdTe contain the FMTD bits set for any tones that have
         * a leading edge or a trailing edge detect.
         */

        if (0 != fmtd_ptr->fmtdLeInfc) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason    = fmtd_ptr->fmtdTypeInfc;
            q_ptr->eventMsg.msg.toneDetect.detect   = VTSP_DETECT_FMTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_LEADING;
            q_ptr->eventMsg.msg.toneDetect.tone      = fmtd_ptr->fmtdLeInfc;
            q_ptr->eventMsg.msg.toneDetect.direction = VTSP_EVENT_TONE_DIR_LOCAL;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
        if (0 != fmtd_ptr->fmtdLePeer) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason    = fmtd_ptr->fmtdTypePeer;
            q_ptr->eventMsg.msg.toneDetect.detect    = VTSP_DETECT_FMTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType  = VTSP_EVENT_LEADING;
            q_ptr->eventMsg.msg.toneDetect.tone      = fmtd_ptr->fmtdLePeer;
            q_ptr->eventMsg.msg.toneDetect.direction = VTSP_EVENT_TONE_DIR_STREAM;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
                
        if (0 != fmtd_ptr->fmtdTeInfc) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason    = fmtd_ptr->fmtdTypeInfc;
            q_ptr->eventMsg.msg.toneDetect.detect   = VTSP_DETECT_FMTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_TRAILING;
            q_ptr->eventMsg.msg.toneDetect.tone      = fmtd_ptr->fmtdTeInfc;
            q_ptr->eventMsg.msg.toneDetect.direction = VTSP_EVENT_TONE_DIR_LOCAL;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }
        if (0 != fmtd_ptr->fmtdTePeer) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason    = fmtd_ptr->fmtdTypePeer;
            q_ptr->eventMsg.msg.toneDetect.detect    = VTSP_DETECT_FMTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType  = VTSP_EVENT_TRAILING;
            q_ptr->eventMsg.msg.toneDetect.tone      = fmtd_ptr->fmtdTePeer;
            q_ptr->eventMsg.msg.toneDetect.direction = VTSP_EVENT_TONE_DIR_STREAM;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
        }

        if (0 != fmtd_ptr->fmtdTimeoutEvent) {
            q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_TONE_DETECT;
            q_ptr->eventMsg.infc = infc;
            q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
            q_ptr->eventMsg.msg.toneDetect.reason = fmtd_ptr->fmtdTimeoutEvent;
            q_ptr->eventMsg.msg.toneDetect.detect = VTSP_DETECT_FMTD;
            q_ptr->eventMsg.msg.toneDetect.edgeType = VTSP_EVENT_TRAILING;
            q_ptr->eventMsg.msg.toneDetect.tone = 0;
            q_ptr->eventMsg.msg.toneDetect.direction = 0;
            VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
            fmtd_ptr->fmtdTimeoutEvent = 0;
        }
#endif
#ifndef VTSP_ENABLE_MP_LITE
        /* Generate Tone Events */
        tone_ptr = &chan_ptr->toneSeq;
        _VTSPR_genEventTone(dsp_ptr, q_ptr, infc, NULL, tone_ptr);
#endif
#ifdef VTSP_ENABLE_TONE_QUAD
        tone_ptr = &chan_ptr->toneQuadSeq;
        _VTSPR_genEventTone(dsp_ptr, q_ptr, infc, NULL, tone_ptr);
#endif
    }

#ifdef VTSP_ENABLE_DTMF
    /*
     * Flow Events
     */
    for (flowIndex = 0; flowIndex < _VTSP_FLOW_PER_INFC; flowIndex++) {
        flow_ptr = &(dsp_ptr->flowObj[flowIndex]);

        /*
         * Now check that a digit did not cause the flow to stop. For this to
         * happen, the flow must either in a state where it is reading from the
         * FromAppQ (either ACTIVE or CLOSING). If an event matches the control
         * mask, put the flow in the ABORT state. This will be processed the
         * next block time.
         */
        if (((_VTSPR_FLOW_STATE_ACTIVE == flow_ptr->playState) ||
                (_VTSPR_FLOW_STATE_CLOSING == flow_ptr->playState))
                && (0 != flow_ptr->playControl)
                && (0 != (flow_ptr->flowDir & VTSP_FLOW_DIR_LOCAL_PLAY))) {
            /*
             * Get the DTMF event associated with this flow.
             */
            chan_ptr = _VTSPR_infcToChanPtr(dsp_ptr, infc);

            /*
             * Check for DTMF detect leading and trailing edges
             */
            dtmf_ptr = chan_ptr->dtmf_ptr;
            le = dtmf_ptr->dtmfLe;
            if (le > -1) {
                /*
                 * convert le to mask for control word.
                 */
                if (0 != (flow_ptr->playControl & (1 << (le & 0xf)))) {
                    infc = dsp_ptr->flowObj[flowIndex].infc;
                    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_FLOW;
                    q_ptr->eventMsg.infc = flow_ptr->infc;
                    q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                    q_ptr->eventMsg.msg.flow.reason = VTSP_EVENT_HALTED;
                    q_ptr->eventMsg.msg.flow.flowId = flowIndex;
                    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);

                    _VTSPR_flowAbortFlush(dsp_ptr, flow_ptr);
                    flow_ptr->playState = _VTSPR_FLOW_STATE_IDLE;
                }
            }
        }
    }
#endif
    return;
}

/*
 * ======== _VTSPR_genEventFx() ========
 *
 * Get msg from pool, fill from event msg,
 * 
 */
void _VTSPR_genEventStream(
    VTSPR_Obj    *vtspr_ptr,
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr)
{
    VTSPR_StreamObj  *stream_ptr;
    vint              infc;
#ifndef VTSP_ENABLE_MP_LITE
    VTSPR_ToneSeq    *tone_ptr;
#endif
    vint              streamId;
    
    /*
     * Stream Events
     */
    _VTSPR_FOR_ALL_INFC(infc) { /* All Physical infc, FX Infc audio events */
        for (streamId = _VTSP_STREAM_PER_INFC - 1; streamId >= 0; streamId--) { 
            stream_ptr = _VTSPR_streamIdToStreamPtr(dsp_ptr, infc, streamId);
#ifndef VTSP_ENABLE_MP_LITE
            /* Check TONE events */
            tone_ptr    = &stream_ptr->toneSeq;
            _VTSPR_genEventTone(dsp_ptr, q_ptr, infc, stream_ptr, tone_ptr);
#endif
            _VTSPR_genEventDtmfRelay(dsp_ptr, q_ptr, infc, streamId);

#ifdef VTSP_ENABLE_TONE_QUAD
            tone_ptr    = &stream_ptr->toneQuadSeq;
            _VTSPR_genEventTone(dsp_ptr, q_ptr, infc, stream_ptr, tone_ptr);
#endif
            /* Check STUN events for received packets */
            if (VTSP_EVENT_ACTIVE == stream_ptr->stunEvent) {
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_STUN_RECV;
                q_ptr->eventMsg.infc = infc;
                q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
                VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
                stream_ptr->stunEvent = VTSP_EVENT_INACTIVE;
            }
        }
    }
}
