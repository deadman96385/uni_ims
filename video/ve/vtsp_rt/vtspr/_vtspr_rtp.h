/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28012 $ $Date: 2014-08-08 17:36:29 +0800 (Fri, 08 Aug 2014) $
 *
 */
#ifndef __VTSPR_RTP_H_
#define __VTSPR_RTP_H_

#include "osal.h"
#include <osal_net.h>


#include <rtp.h>
#include <vtsp.h>
#include "../vtsp/vtsp_private/_vtsp_private.h"
#include "vtspr.h"

/*
 * Prototypes
 */
vint _VTSPR_rtpClose(
    _VTSPR_RtpObject  *rtp_ptr);

vint _VTSPR_rtpDir(
    _VTSPR_RtpObject  *rtp_ptr,
    vint               dir);

vint _VTSPR_rtpInit(
    _VTSPR_RtpObject  *rtp_ptr);

vint _VTSPR_rtpOpen(
    _VTSPR_RtpObject *rtp_ptr,
    VTSP_StreamDir    dir,
    OSAL_NetAddress   sendAddr,
    OSAL_NetAddress   recvAddr,
    uint8             rdnDynType,
    uint16            srtpSecurityType,
    char             *srtpSendKey,
    char             *srtpRecvKey);

vint _VTSPR_rtpBufInit(
    _VTSPR_RtpObject  *rtp_ptr);

vint _VTSPR_rtpRecv(
    VTSPR_Obj        *vtspr_ptr,
    VTSPR_Queues     *q_ptr,
    VTSPR_DSP        *dsp_ptr,
    _VTSPR_RtpObject *rtp_ptr);

void _VTSPR_rtpSend(
    VTSPR_Obj         *vtspr_ptr,
    _VTSPR_RtpObject  *rtp_ptr,
    VTSP_BlockHeader  *hdr_ptr,
    uint8             *data_ptr,
    vint               sendBytes,
    vint               frameCnt);

vint _VTSPR_rtpShutdown(
    _VTSPR_RtpObject  *rtp_ptr);

uint32 _VTSPR_getIncrTime(
    vint               coder,
    vint               lastVCoder);

uint32 _VTSPR_getRtpClockRate(
    vint coder);
#endif /* __RTPD_H_ */
