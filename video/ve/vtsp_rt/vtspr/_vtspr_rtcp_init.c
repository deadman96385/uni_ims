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
 * ======== _VTSPR_rtcpInit() ========
 *
 * This function is used to intialize an RTCP stream.
 */
vint _VTSPR_rtcpInit(
    _VTSPR_RtcpObject *rtcp_ptr,
    VTSPR_NetObj      *net_ptr,
    vint               streamId,
    vint               infc)
{
    OSAL_memSet(rtcp_ptr, 0, sizeof(_VTSPR_RtcpObject));
    rtcp_ptr->streamId = streamId;
    rtcp_ptr->infc        = infc;
    rtcp_ptr->enableMask  = (VTSP_MASK_RTCP_SR | VTSP_MASK_RTCP_XR | VTSP_MASK_RTCP_BYE);
    rtcp_ptr->tos         = _VTSPR_RTCP_TOS_VALUE;

    /* Create default CNAME */
    _VTSPR_rtcpSetCname(net_ptr, infc, "CNAME NOT SET");

    return (_VTSPR_RTP_OK);
}
