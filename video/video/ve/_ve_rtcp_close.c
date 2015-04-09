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
 * ======== _VE_rtcpClose() ========
 *
 * This function is used to close an RTCP stream.
 */
vint _VE_rtcpClose(
    _VE_Queues *q_ptr,
    _VE_NetObj *net_ptr,
    uvint         infc,
    uvint          streamId)
{
    _VTSP_RtcpCmdMsg   msg;
    _VE_RtcpObject *rtcp_ptr;

    /*
     * Send a BYE message 
     */
    rtcp_ptr = _VE_streamIdToRtcpPtr(net_ptr, infc, streamId);
    if (rtcp_ptr->enableMask & VTSP_MASK_RTCP_BYE) {
        _VE_rtcpBye(q_ptr, net_ptr, infc, streamId);
    }

    /*
     * Clear addresses 
     */
    rtcp_ptr = _VE_streamIdToRtcpPtr(net_ptr, infc, streamId);
    OSAL_memSet(&rtcp_ptr->remoteAddr, 0, sizeof(OSAL_NetAddress));
    OSAL_memSet(&rtcp_ptr->localAddr, 0, sizeof(OSAL_NetAddress));

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
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *)&msg,
                sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
        _VE_TRACE(__FILE__, __LINE__);
        return (_VE_RTP_ERROR);
    }
    return (_VE_RTP_OK);
}
