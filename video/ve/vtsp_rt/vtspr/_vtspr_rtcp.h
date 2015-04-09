/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28012 $ $Date: 2014-08-08 17:36:29 +0800 (Fri, 08 Aug 2014) $
 *
 */
#ifndef __VTSPR_RTCP_H_
#define __VTSPR_RTCP_H_

#include "osal.h"

#include <rtp.h>
#include <vtsp.h>
#include "../vtsp/vtsp_private/_vtsp_private.h"
#include "vtspr.h"


/*
 * Prototypes
 */
vint _VTSPR_rtcpBye(
    VTSPR_Queues *q_ptr,
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);

vint _VTSPR_rtcpClose(
    VTSPR_Queues *q_ptr,
    VTSPR_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);

vint _VTSPR_rtcpCname(
    VTSPR_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset,
    uint32            ssrc);

vint _VTSPR_rtcpInit(
    _VTSPR_RtcpObject  *rtcp_ptr,
    VTSPR_NetObj       *net_ptr,
    vint                infc,
    vint                streamId);

vint _VTSPR_rtcpNextInterval(
    _VTSPR_RtcpObject  *rtcp_ptr);

void _VTSPR_rtcpNullReport(
    VTSPR_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr);

vint _VTSPR_rtcpOpen(
    VTSPR_Queues      *q_ptr,
    _VTSPR_RtcpObject *rtcp_ptr,
    OSAL_NetAddress    remoteAddr,
    OSAL_NetAddress    localAddr);

vint _VTSPR_rtcpReceiverBlock(
    _VTSPR_RtpObject *rtpObj_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset);

void _VTSPR_rtcpReceiverReport(
    VTSPR_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr);

vint _VTSPR_rtcpRecv(
    VTSPR_Queues *q_ptr,
    VTSPR_DSP    *dsp_ptr,
    VTSPR_NetObj *net_ptr);

vint _VTSPR_rtcpSend(
    VTSPR_Obj          *vtspr_ptr,
    VTSPR_Queues       *q_ptr,
    VTSPR_DSP          *dsp_ptr,
    VTSPR_StreamObj    *stream_ptr,
    vint                infc,
    vint                streamId);

void _VTSPR_rtcpSenderReport(
    VTSPR_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr,
    VTSP_Stream       streamParam);

void _VTSPR_rtcpSetCname(
    VTSPR_NetObj *net_ptr,
    vint          infc,
    const char   *name_ptr);

void _VTSPR_rtcpSetControl(
    _VTSPR_RtcpObject *rtcp_ptr,
    uvint              control,
    uvint              val);

vint _VTSPR_rtcpShutdown(
    _VTSPR_RtcpObject *object_ptr);
#endif /* __VTSPR_RTCPD_H_ */
