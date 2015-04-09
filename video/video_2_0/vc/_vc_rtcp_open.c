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
 * ======== _VC_rtcpOpen() ========
 *
 * This function is used to open an RTCP stream.
 */
vint _VC_rtcpOpen(
    _VC_Obj         *vc_ptr,
    vint            streamId,
    OSAL_NetAddress remoteAddr,
    OSAL_NetAddress localAddr)
{
    _VC_RtcpCmdMsg  message;
    _VC_StreamObj   *stream_ptr;
    stream_ptr = _VC_streamIdToStreamPtr(vc_ptr->dsp_ptr, streamId);

    /* Start RTCP Task if not started. */
    if (!stream_ptr->rtcpEnable) {
        stream_ptr->rtcpEnable = 1;
        _VC_startRtcpSendRecvTask(stream_ptr);
    }

    message.cmd = _VC_RTCP_CMD_OPEN;
    message.streamId = streamId;
    message.address.local = localAddr;
    message.address.remote = remoteAddr;
    _VC_sendRtcpCommand(vc_ptr->q_ptr, &message);

    return (_VC_RTP_OK);
}

/*
 * ======== _VC_rtcpOpen() ========
 *
 * This function is used to open an RTCP stream.
 */
vint _VC_rtcpHandleOpen(
    _VC_Queues      *q_ptr,
    _VC_Dsp         *dsp_ptr,
    _VC_RtcpObject  *rtcp_ptr,
    _VC_RtcpCmdMsg  *cmd_ptr)
{
    _VTSP_RtcpCmdMsg message;
    /*
     * Save addresses for comparison when a stream is modified.
     */
    OSAL_memCpy(&rtcp_ptr->remoteAddr, &cmd_ptr->address.remote, sizeof(OSAL_NetAddress));
    OSAL_memCpy(&rtcp_ptr->localAddr,  &cmd_ptr->address.local, sizeof(OSAL_NetAddress));

    rtcp_ptr->lastSR       = 0;
    rtcp_ptr->recvLastSR   = 0;
    rtcp_ptr->currentCount = 0;

    /*
     * Create message to network task.
     */
    message.command = _VTSP_RTCP_CMD_OPEN;
    message.streamId = rtcp_ptr->streamId;
    message.infc = rtcp_ptr->infc;
    message.msg.open.infc = rtcp_ptr->infc;
    OSAL_memCpy(&message.msg.open.remoteAddr, &cmd_ptr->address.remote, sizeof(OSAL_NetAddress));
    OSAL_memCpy(&message.msg.open.localAddr,  &cmd_ptr->address.local,  sizeof(OSAL_NetAddress));
    message.msg.open.tos = rtcp_ptr->tos;

    /*
     * Send message
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *)&message,
                sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
        _VC_TRACE(__FILE__, __LINE__);
        return (_VC_RTP_ERROR);
    }

    return (_VC_RTP_OK);
}
