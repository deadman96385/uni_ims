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
 * ======== _VC_rtcpInit() ========
 *
 * This function is used to intialize an RTCP stream.
 */
vint _VC_rtcpInit(
    _VC_RtcpObject *rtcp_ptr,
    _VC_NetObj     *net_ptr,
    vint            streamId,
    vint            infc)
{
    OSAL_memSet(rtcp_ptr, 0, sizeof(_VC_RtcpObject));
    rtcp_ptr->streamId = streamId;
    rtcp_ptr->infc        = infc;
    rtcp_ptr->enableMask  = (VTSP_MASK_RTCP_SR | VTSP_MASK_RTCP_XR | VTSP_MASK_RTCP_BYE);

    rtcp_ptr->tos         = _VC_RTCP_TOS_VALUE;
    rtcp_ptr->configure.reducedMinIntervalMillis = _VC_RTCP_MIN_INTERVAL_MILLIS;
    rtcp_ptr->configure.rtcpFeedbackSendMask = 0;
    rtcp_ptr->feedback.firSeqNumber = 1;
    /* Initialize the default required RTCP Masks. */
    rtcp_ptr->configure.enableMask = (VTSP_MASK_RTCP_SR | VTSP_MASK_RTCP_BYE |
            VTSP_MASK_RTCP_FB_NACK | VTSP_MASK_RTCP_FB_PLI | VTSP_MASK_RTCP_FB_FIR |
            VTSP_MASK_RTCP_FB_TMMBR | VTSP_MASK_RTCP_FB_TMMBN);
    /* Create default CNAME */
    _VC_rtcpSetCname(net_ptr, "CNAME NOT SET");

    return (_VC_RTP_OK);
}
