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

/*
 * ======== _VTSPR_rtcpOpen() ========
 *
 * This function is used to open an RTCP stream.
 */
vint _VTSPR_rtcpOpen(
    VTSPR_Queues      *q_ptr,
    _VTSPR_RtcpObject *rtcp_ptr,
    OSAL_NetAddress    remoteAddr,
    OSAL_NetAddress    localAddr)
{
    _VTSP_RtcpCmdMsg message;

    /*
     * Save addresses for comparison when a stream is modified.
     */
    OSAL_memCpy(&rtcp_ptr->remoteAddr, &remoteAddr, sizeof(remoteAddr));
    OSAL_memCpy(&rtcp_ptr->localAddr, &localAddr, sizeof(localAddr));

    rtcp_ptr->currentCount = 0;
    _VTSPR_rtcpNextInterval(rtcp_ptr);
    /*
     * Create message to network task.
     */
    message.command = _VTSP_RTCP_CMD_OPEN;
    message.streamId = rtcp_ptr->streamId;
    message.infc = rtcp_ptr->infc;
    message.msg.open.infc = rtcp_ptr->infc;
    OSAL_memCpy(&message.msg.open.remoteAddr, &remoteAddr, sizeof(remoteAddr));
    OSAL_memCpy(&message.msg.open.localAddr, &localAddr, sizeof(localAddr));
    message.msg.open.tos = rtcp_ptr->tos;

    /*
     * Send message
     */
    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->rtcpMsg, (char *)&message,
                sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (_VTSPR_RTP_ERROR);
    }
    return (_VTSPR_RTP_OK);
}
