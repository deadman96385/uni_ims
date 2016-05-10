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
 * ======== _VC_rtcpClose() ========
 *
 * Generate and send an event to the RTCP thread
 * that will close the RTCP stream.
 */
vint _VC_rtcpClose(
    _VC_Queues *q_ptr,
    uvint       streamId)
{
    _VC_RtcpCmdMsg cmdMsg;
    cmdMsg.cmd = _VC_RTCP_CMD_CLOSE;
    cmdMsg.streamId = streamId;

    _VC_sendRtcpCommand(q_ptr, &cmdMsg);

    return (_VC_RTP_OK);
}

/*
 * ======== _VC_rtcpHandleClose() ========
 *
 * Respond to the RTCP Command to close. This should only
 * be run on the RTCP thread.
 */
vint _VC_rtcpHandleClose(
    _VC_Queues *q_ptr,
    _VC_NetObj *net_ptr,
    uvint       streamId)
{
    _VTSP_RtcpCmdMsg    msg;
    _VC_RtcpObject     *rtcp_ptr;
    uvint               infc = VTSP_INFC_VIDEO;

    /*
     * Send a BYE message
     */
    rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, streamId);
    if (rtcp_ptr->configure.enableMask & VTSP_MASK_RTCP_BYE) {
        _VC_rtcpBye(q_ptr, net_ptr, infc, streamId);
    }

    /*
     * Clear addresses
     */
    rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, streamId);
    OSAL_memSet(&rtcp_ptr->remoteAddr, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&rtcp_ptr->localAddr, 0, sizeof(OSAL_NetAddress));
    /* Reset the RTCP min interval to the initial value. */
    rtcp_ptr->configure.reducedMinIntervalMillis = _VC_RTCP_MIN_INTERVAL_MILLIS;
    rtcp_ptr->configure.rtcpFeedbackSendMask = 0;

    /*
     * Create message to network task.
     */
    msg.command = _VTSP_RTCP_CMD_CLOSE;
    msg.infc = infc;
    msg.streamId = streamId;

    /*
     * Send message to the task to close the sockets after the BYE message was
     * sent.
     */
    if (OSAL_SUCCESS != VIER_writeRtcpCmd((void*)&msg, sizeof(_VTSP_RtcpCmdMsg))) {
        OSAL_logMsg("%s: fail to write the RTCP_CMD_CLOSE to VIER", __FUNCTION__);
        return(_VC_RTP_ERROR);
    }

    return (_VC_RTP_OK);
}
