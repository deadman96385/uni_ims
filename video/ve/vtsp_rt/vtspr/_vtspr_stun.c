/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12523 $ $Date: 2010-07-14 06:57:44 +0800 (Wed, 14 Jul 2010) $
 *
 */

#include "_vtspr_stun.h"
#include "_vtspr_private.h"

/*
 * ======== _VTSPR_stunProcess ========
 *
 * Process packet data to and from user space.
 */
vint _VTSPR_stunProcess(
    VTSPR_Queues     *q_ptr,
    VTSPR_NetObj     *net_ptr)
{
    _VTSPR_RtpObject *rtp_ptr;
    VTSP_Stun         stunMsg;
    vint              infc;
    vint              streamId;

    /*
     * Get packet from user space. Process all packets within 10 ms.
     */
    while ((sizeof(VTSP_Stun) == OSAL_msgQRecv(q_ptr->stunSendQ,
            (char *)&(stunMsg), sizeof(VTSP_Stun), OSAL_NO_WAIT, NULL))) {
        infc = stunMsg.infc;
        streamId = stunMsg.streamId;
        rtp_ptr = _VTSPR_streamIdToRtpPtr(net_ptr, infc, streamId);

        if ((rtp_ptr->localAddr.ipv4 == stunMsg.localAddr.ipv4) &&
                (rtp_ptr->localAddr.port== stunMsg.localAddr.port) &&
                (rtp_ptr->inUse != _VTSPR_RTP_NOT_BOUND) &&
                (RTP_VERSION != ((*stunMsg.payload >> 6) & 3))) {
            if (stunMsg.pktSize != _VTSPR_netSendto(rtp_ptr->socket, 
                    (void *)stunMsg.payload, stunMsg.pktSize,
                    stunMsg.remoteAddr)) {
                _VTSP_TRACE(__FILE__, __LINE__);
            }
        }
    }
    return (VTSP_OK);
}
