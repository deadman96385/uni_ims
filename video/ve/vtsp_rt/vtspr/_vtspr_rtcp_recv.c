/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28422 $ $Date: 2014-08-22 11:55:05 +0800 (Fri, 22 Aug 2014) $
 *
 */

#include <osal.h>
#include "_vtspr_rtcp.h"
#include "_vtspr_private.h"
/*
 * ======== _VTSPR_rtcpRecv() ========
 */
vint _VTSPR_rtcpRecv(
    VTSPR_Queues  *q_ptr,
    VTSPR_DSP     *dsp_ptr,
    VTSPR_NetObj  *net_ptr)
{
    _VTSPR_RtpObject   *rtp_ptr;
    _VTSPR_RtcpObject  *rtcp_ptr;
    OSAL_MsgQId         qId;
    uvint               infc;
    uvint               streamId;
    _VTSP_RtcpEventMsg  message;
    _VTSP_RtcpCmdMsg    cmdMessage;

    qId = q_ptr->rtcpEvent;
    /*
     * Read and process messages. Then relay event to application.
     */
    while (OSAL_msgQRecv(qId, (char *)&message,
            _VTSP_Q_RTCP_EVENT_SZ, OSAL_NO_WAIT, NULL) > 0) {

        /*
         * If the message is a sender report, save the middle 32 bits in the
         * timestamp and calculate. Save the internal time when the report
         * was received.
         */
        infc = message.infc;
        streamId = message.streamId;
        if (VTSP_EVENT_RTCP_SR == message.reason) {
            rtp_ptr = _VTSPR_streamIdToRtpPtr(net_ptr, infc, streamId);
            rtp_ptr->lastSR = (message.arg1 << 16) |
                    ((message.arg2 >> 16) & 0xffff);
            rtp_ptr->recvLastSR = rtp_ptr->receiveTime;

            rtp_ptr->ntpTime.sec = message.arg1;
            rtp_ptr->ntpTime.usec = message.arg2;
            rtp_ptr->rtcpRtpTime = message.arg3;

            /* Notify audio RTCP SR reception. */
            OSAL_logMsg("VTSPR receive RTCP SR sec:%u, usec:%u rtpTs:%u\n",
                    message.arg1, message.arg2, message.arg3);
            _VTSPR_LipSync_rtcpRecv(infc, q_ptr, rtp_ptr);

            rtcp_ptr = _VTSPR_streamIdToRtcpPtr(net_ptr, infc, streamId);
            if (rtcp_ptr->enableMask & VTSP_MASK_RTCP_RR) {
                /*
                 * create Receiver Report
                 */
                _VTSPR_rtcpReceiverReport(net_ptr, infc, streamId, &cmdMessage);
                /*
                 * Send message
                 */
                cmdMessage.command  = _VTSP_RTCP_CMD_SEND;
                cmdMessage.infc     = infc;
                cmdMessage.streamId = streamId;            
                if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *)&cmdMessage,
                        sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
                    _VTSP_TRACE(__FILE__, __LINE__);
                    return (_VTSPR_RTP_ERROR);
                }
            }
        }
        /*
         * Send Event to Application.
         */
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RTCP;
        q_ptr->eventMsg.infc = infc;
        q_ptr->eventMsg.tick = dsp_ptr->tick1ms;
        q_ptr->eventMsg.msg.rtcp.reason = message.reason;
        q_ptr->eventMsg.msg.rtcp.streamId = streamId;
        q_ptr->eventMsg.msg.rtcp.ssrc = message.ssrc;
        q_ptr->eventMsg.msg.rtcp.arg1 = message.arg1;
        q_ptr->eventMsg.msg.rtcp.arg2 = message.arg2;
        q_ptr->eventMsg.msg.rtcp.arg3 = message.arg3;
        q_ptr->eventMsg.msg.rtcp.arg4 = message.arg4;
        q_ptr->eventMsg.msg.rtcp.arg5 = message.arg5;
        q_ptr->eventMsg.msg.rtcp.arg6 = message.arg6;

        VTSPR_sendEvent(q_ptr, &(q_ptr->eventMsg), message.infc);
    }
    return (_VTSPR_RTP_OK);
}
