/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-14 06:57:44 +0800 (Wed, 14 Jul 2010) $
 *
 */

#include <osal.h>
#include <_vtspr_rtcp.h>
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_rtcpClose() ========
 *
 * This function is used to close an RTCP stream.
 */
vint _VTSPR_rtcpClose(
    VTSPR_Queues *q_ptr,
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint          streamId)
{
    _VTSP_RtcpCmdMsg   msg;
    _VTSPR_RtcpObject *rtcp_ptr;

    /*
     * Send a BYE message 
     */
    rtcp_ptr = _VTSPR_streamIdToRtcpPtr(net_ptr, infc, streamId);
    if (rtcp_ptr->enableMask & VTSP_MASK_RTCP_BYE) {
        _VTSPR_rtcpBye(q_ptr, net_ptr, infc, streamId);
    }

    /*
     * Clear addresses 
     */
    rtcp_ptr = _VTSPR_streamIdToRtcpPtr(net_ptr, infc, streamId);
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
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }
    return (_VTSPR_RTP_OK);
}
