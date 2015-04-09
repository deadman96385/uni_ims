/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-13 18:57:44 -0400 (Tue, 13 Jul 2010) $
 *
 */

#include "_ve_private.h"
/*
 * ======== _VE_rtcpRecv() ========
 */
vint _VE_rtcpRecv(
    _VE_Queues  *q_ptr,
    _VE_Dsp     *dsp_ptr,
    _VE_NetObj  *net_ptr)
{
    _VE_RtpObject   *rtp_ptr;
    _VE_RtcpObject  *rtcp_ptr;
    OSAL_MsgQId         qId;
    vint                retVal;
    uvint               infc;
    uvint               streamId;
    _VTSP_RtcpEventMsg  message;
    _VTSP_RtcpCmdMsg    cmdMessage;

    qId = q_ptr->rtcpEvent;
    /*
     * Read and process messages. Then relay event to application.
     */
    while ((retVal = OSAL_msgQRecv(qId, (char *)&message,
            _VTSP_Q_RTCP_EVENT_SZ, OSAL_NO_WAIT, NULL)) > 0) {

        /*
         * If the message is a sender report, save the middle 32 bits in the
         * timestamp and calculate. Save the internal time when the report
         * was received.
         */
        infc = message.infc;
        streamId = message.streamId;
        if (VTSP_EVENT_RTCP_SR == message.reason) {
            rtp_ptr = _VE_streamIdToRtpPtr(net_ptr, infc, streamId);
            rtp_ptr->lastSR = (message.arg1 << 16) |
                    ((message.arg2 >> 16) & 0xffff);
            rtp_ptr->recvLastSR = rtp_ptr->receiveTime;

            rtcp_ptr = _VE_streamIdToRtcpPtr(net_ptr, infc, streamId);
            if (rtcp_ptr->enableMask & VTSP_MASK_RTCP_RR) {
                /*
                 * create Receiver Report
                 */
                _VE_rtcpReceiverReport(net_ptr, infc, streamId, &cmdMessage);
                /*
                 * Send message
                 */
                cmdMessage.command  = _VTSP_RTCP_CMD_SEND;
                cmdMessage.infc     = infc;
                cmdMessage.streamId = streamId;            
                if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *)&cmdMessage,
                        sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
                    _VE_TRACE(__FILE__, __LINE__);
                    return (_VE_RTP_ERROR);
                }
            }
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

        _VE_sendEvent(q_ptr, &(q_ptr->eventMsg), message.infc);
    }
    return (_VE_RTP_OK);
}
