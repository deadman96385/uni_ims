/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */

#include "_vc_private.h"
#include "_vc_rtcp.h"
/*
 * ======== _VC_rtcpRecv() ========
 */
vint _VC_rtcpRecv(
    _VC_Queues  *q_ptr,
    _VC_Dsp     *dsp_ptr,
    _VC_NetObj  *net_ptr)
{
    _VC_RtcpObject     *rtcp_ptr;
    _VC_StreamObj      *stream_ptr;
    OSAL_MsgQId         qId;
    vint                retVal;
    uvint               infc;
    uvint               streamId;
    VTSP_NtpTime        restrictionPeriod;
    VTSP_NtpTime        startTime;
    _VTSP_RtcpEventMsg  message;
    uint64              mantissa;
    uint64              exponent;
    uint64              bitrateBps;
    char                buffer[20];

    qId = q_ptr->rtcpEvent;
    _VC_RTCP_LOG("RTCP recv\n");
    OSAL_TimeVal currentTime;

    /*
     * Read and process messages. Then relay event to application.
     */
    while ((retVal = OSAL_msgQRecv(qId, (char *)&message,
            _VTSP_Q_RTCP_EVENT_SZ, _VC_RTCP_MIN_INTERVAL_MILLIS, NULL)) > 0) {

        _VC_RTCP_LOG("RTCP recvd something, rtcp_ptr->configure.enableMask=%u\n",
                rtcp_ptr->configure.enableMask);
        /*
         * If the message is a sender report, save the middle 32 bits in the
         * timestamp and calculate. Save the internal time when the report
         * was received.
         */
        infc = message.infc;
        streamId = message.streamId;
        
        stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

        rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, message.streamId);

        switch (message.reason) {
            case VTSP_EVENT_RTCP_SR:
                OSAL_timeGetTimeOfDay(&currentTime);
                /* The middle 32 bits of the 64 bit NTP timestamp */
                rtcp_ptr->lastSR = _VC_NTP_64_TO_32(message.arg1, message.arg2);
                /* Create another 32 bit NTP timestamp from the current time */
                rtcp_ptr->recvLastSR = message.receivedTime;

                rtcp_ptr->ntpTime.sec  = _VC_NTP_MSW_TO_SEC(message.arg1);
                rtcp_ptr->ntpTime.usec = _VC_NTP_LSW_TO_USEC(message.arg2);
                rtcp_ptr->rtpTime  = message.arg3;

                /* Notify video RTCP SR reception. */
                _VC_RTCP_LOG("VC receive RTCP SR sec:%u, usec:%u rtpTs:%u\n",
                        message.arg1, message.arg2, message.arg3);
                _VC_LipSync_rtcpRecv(q_ptr, rtcp_ptr);
                break;
            case VTSP_EVENT_RTCP_RR:
                OSAL_timeGetTimeOfDay(&currentTime);
                rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, message.streamId);

                if (0 == message.arg5) {
                    rtcp_ptr->roundTripTime = 0;
                }
                else {
                    /* Round trip time as per specification of RFC 3550. See DLSR/Page 39 */
                    rtcp_ptr->roundTripTime =
                            message.receivedTime -     /* A    */
                            message.arg5 -              /* LSR  */
                            message.arg6;               /* DLSR */
                }

                _VC_RTCP_LOG("Round Trip Time: %d ms", _VC_NTP_32_TO_MSEC(rtcp_ptr->roundTripTime));
                break;
            case VTSP_EVENT_RTCP_FB_GNACK:
                _VC_RTCP_LOG("RTCP received NACK PID %x", message.arg2);
                _VC_RTCP_LOG("RTCP received NACK BLP %x", message.arg3);
                break;
            case VTSP_EVENT_RTCP_FB_TMMBR:
                mantissa = ((message.arg3 >> 9) & 0x1ffff);
                exponent = ((message.arg3 >> 26) & 0x3f);
                _VC_RTCP_LOG("RTCP received TMMBR SSRC %x", message.arg2);
                _VC_RTCP_LOG("RTCP received TMMBR MxTBR Exp %llu", exponent);
                _VC_RTCP_LOG("RTCP received TMMBR MxTBR Mantissa %llu", mantissa);
                _VC_RTCP_LOG("RTCP received TMMBR MxTBR Overhead %x", message.arg3 & 0x1ff);
                /* TMMBR in bits per second = mantissa * 2^exp. */
                bitrateBps =  mantissa << exponent;
                if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_TMMBR) {
                    /* Store the received TMMBR value in kbps. */
                    rtcp_ptr->feedback.recvTmmbrInKbps = (bitrateBps >> 10);
                    /* TMMBN should be sent as reply to TMMBR. Set TMMBN bit mask in rtcpFeedbackSendMask. */
                    rtcp_ptr->configure.rtcpFeedbackSendMask |= VTSP_MASK_RTCP_FB_TMMBN;
                    /* Send a TMMBN accepting the requested TMMBR value. */
                    rtcp_ptr->feedback.sendTmmbnInKbps = rtcp_ptr->feedback.recvTmmbrInKbps;
                    /* Send Event to JVCE to notify Remote Max BW in kbps. */
                    OSAL_itoa((rtcp_ptr->feedback.recvTmmbrInKbps), buffer, sizeof(buffer));
                    _VC_sendAppEvent(q_ptr, VC_EVENT_REMOTE_RECV_BW_KBPS, buffer, stream_ptr->streamParam.encoder);
                }
                break;
            case VTSP_EVENT_RTCP_FB_TMMBN:
                OSAL_logMsg("%s: RTCP received TMMBN\n", __FUNCTION__);
                mantissa = ((message.arg3 >> 9) & 0x1ffff);
                exponent = ((message.arg3 >> 26) & 0x3f);
                _VC_RTCP_LOG("RTCP received TMMBN SSRC %x", message.arg2);
                _VC_RTCP_LOG("RTCP received TMMBN MxTBR Exp %llu", exponent);
                _VC_RTCP_LOG("RTCP received TMMBN MxTBR Mantissa %llu", mantissa);
                _VC_RTCP_LOG("RTCP received TMMBN MxTBR Overhead %x", message.arg3 & 0x1ff);
                /* TMMBN in bits per second = mantissa * 2^exp. */
                bitrateBps =  mantissa << exponent;
                if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_TMMBR) {
                    /* Store the received TMMBN value in kbps. */
                    rtcp_ptr->feedback.recvTmmbnInKbps = (bitrateBps >> 10);
                    rtcp_ptr->feedback.state = _VC_TMMBR_STATE_DONE;
                    OSAL_logMsg("%s: received TMMBN, recvTmmbnInKbps=%u Kbps\n",
                            __FUNCTION__, rtcp_ptr->feedback.recvTmmbnInKbps);
                }
                break;
            case VTSP_EVENT_RTCP_FB_PLI:
                _VC_RTCP_LOG("RTCP received PLI SSRC %x", message.arg1);
                rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, message.streamId);
                restrictionPeriod = rtcp_ptr->roundTripTime + _VC_RTCP_IFRAME_GENERATION_DELAY;

                /* Check the restriction period. If not enough time has elapsed, ignore this PLI */
                startTime = rtcp_ptr->feedback.lastPli;
                if ((message.receivedTime - startTime) > restrictionPeriod) {
                    /* Restriction period has ended. Check if the feature is enabled */
                    if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_PLI) {
                        _VC_sendAppEvent(q_ptr, VC_EVENT_SEND_KEY_FRAME, "VC - Restart Enc", stream_ptr->streamParam.encoder);
                    }
                    rtcp_ptr->feedback.lastPli = message.receivedTime;
                }
                else {
                    _VC_RTCP_LOG("Ignoring PLI due to restriction period");
                }
                break;
            case VTSP_EVENT_RTCP_FB_FIR:
                _VC_RTCP_LOG("RTCP received FIR SSRC %x", message.arg2);
                _VC_RTCP_LOG("RTCP received FIR Sequence number %x", message.arg3);
                rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, message.streamId);
                restrictionPeriod = 2000;//rtcp_ptr->roundTripTime + _VC_RTCP_IFRAME_GENERATION_DELAY;
                /* Check the restriction period. If not enough time has elapsed, ignore this FIR */
                startTime = rtcp_ptr->feedback.lastFir;
                if (_VC_NTP_32_TO_MSEC(message.receivedTime - startTime) > restrictionPeriod) {
                    /* Restriction period has ended. Check if the feature is enabled */
                    if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_FB_FIR) {
                        _VC_RTCP_LOG("RTCP received FIR, sent FIR event to VCE for key frame");
                        _VC_sendAppEvent(q_ptr, VC_EVENT_SEND_KEY_FRAME, "VC - Restart Enc", stream_ptr->streamParam.encoder);
                        //update the lastFir once this event is sent to APP
                        rtcp_ptr->feedback.lastFir = message.receivedTime;
                    }
                } else {
                    //_VC_RTCP_LOG("RTCP received FIR, Ignoring due to restriction period");
                    OSAL_logMsg("%s:RTCP received FIR, Ignoring due to restriction period\n " , __FUNCTION__);
                }
                break;
            default:
                break;
        }
        /*
         * Send Event to Application.
         */
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RTCP;
        q_ptr->eventMsg.infc = infc;
        q_ptr->eventMsg.tick = 0;
        q_ptr->eventMsg.msg.rtcp.reason = message.reason;
        q_ptr->eventMsg.msg.rtcp.streamId = streamId;
        q_ptr->eventMsg.msg.rtcp.ssrc = message.ssrc;
        q_ptr->eventMsg.msg.rtcp.arg1 = message.arg1;
        q_ptr->eventMsg.msg.rtcp.arg2 = message.arg2;
        q_ptr->eventMsg.msg.rtcp.arg3 = message.arg3;
        q_ptr->eventMsg.msg.rtcp.arg4 = message.arg4;
        q_ptr->eventMsg.msg.rtcp.arg5 = message.arg5;
        q_ptr->eventMsg.msg.rtcp.arg6 = message.arg6;

        _VC_sendVtspEvent(q_ptr, &(q_ptr->eventMsg));
    }
    return (_VC_OK);
}
