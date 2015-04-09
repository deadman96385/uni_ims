/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef __VE_PRIVATE_H_
#define __VE_PRIVATE_H_

#include <osal.h>
#include <video.h>
#include <h263enc.h>
#include <h263dec.h>
#include <h264enc.h>
#include <h264dec.h>
#include <vid.h>
#include <vcd.h>
#include <vdd.h>
#include <jbv.h>
#include "_ve_const.h"
#include "_ve_struct.h"

#define _VE_TRACE(x, y) OSAL_logMsg("%s %d\n", x, y)

/*
 * Shared Variables
 */

/*
 * Prototypes
 */

/*
 * Net
 */
OSAL_NetSockId _VE_netSocket(
    OSAL_NetSockType type,
    int              flags);

vint _VE_netClose(
    OSAL_NetSockId socketFd);

vint _VE_netBind(
    OSAL_NetSockId  socketFd,
    OSAL_NetAddress address);

int _VE_netRecvfrom(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              maxSize,
    OSAL_NetAddress *sentAddr);

uvint _VE_netSendto(
    OSAL_NetSockId socketFd,
    void            *buf_ptr,
    int              bufSize,
    OSAL_NetAddress  sendAddr);

OSAL_Boolean _VE_netIsSameAddr(
    OSAL_NetAddress addr1,
    OSAL_NetAddress addr2);

/*
 * RTP
 */
vint _VE_rtpClose(
    _VE_RtpObject  *rtp_ptr);

vint _VE_rtpDir(
    _VE_RtpObject  *rtp_ptr,
    vint            dir);

vint _VE_rtpInit(
    _VE_RtpObject  *rtp_ptr);

vint _VE_rtpOpen(
    _VE_RtpObject    *rtp_ptr,
    VTSP_StreamDir    dir,
    OSAL_NetAddress   sendAddr,
    OSAL_NetAddress   recvAddr,
    uint8             rdnDynType,
    uint16            srtpSecurityType,
    char             *srtpSendKey,
    char             *srtpRecvKey);

vint _VE_rtpBufInit(
    _VE_RtpObject  *rtp_ptr);

vint _VE_rtpRecv(
    _VE_Obj        *vtspr_ptr,
    _VE_Queues     *q_ptr,
    _VE_Dsp        *dsp_ptr);

void _VE_rtpSend(
    _VE_Obj         *vtspr_ptr,
    _VE_RtpObject  *rtp_ptr,
    VTSP_BlockHeader  *hdr_ptr,
    uint8             *data_ptr,
    vint               sendBytes,
    vint               frameCnt);

vint _VE_rtpShutdown(
    _VE_RtpObject  *rtp_ptr);

/*
 * Defaults
 */
void _VE_defaults(
    _VE_Dsp *dsp_ptr);

/*
 * map
 */

OSAL_INLINE uvint _VE_localToDynamicEncoder(
    VTSP_StreamVideo *streamParam_ptr,
    vint              local);

OSAL_INLINE uvint _VE_dynamicToLocalDecoder(
    VTSP_StreamVideo  *streamParam_ptr,
    vint               dynamic);

OSAL_INLINE _VE_StreamObj *_VE_streamIdToStreamPtr(
        _VE_Dsp *dsp_ptr,
        vint     infc,
        vint     streamId);

OSAL_INLINE _VE_RtcpObject *_VE_streamIdToRtcpPtr(
        _VE_NetObj *net_ptr,
        uvint         infc,
        uvint         streamId);

OSAL_INLINE _VE_RtpObject *_VE_streamIdToRtpPtr(
        _VE_NetObj *net_ptr,
        uvint         infc,
        uvint         streamId);

/*
 * RTCP
 */
vint _VE_rtcpBye(
    _VE_Queues *q_ptr,
    _VE_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);

vint _VE_rtcpClose(
    _VE_Queues *q_ptr,
    _VE_NetObj *net_ptr,
    uvint         infc,
    uvint         streamId);

vint _VE_rtcpCname(
    _VE_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset,
    uint32            ssrc);

vint _VE_rtcpInit(
    _VE_RtcpObject  *rtcp_ptr,
    _VE_NetObj       *net_ptr,
    vint                infc,
    vint                streamId);

vint _VE_rtcpNextInterval(
    _VE_RtcpObject  *rtcp_ptr);

void _VE_rtcpNullReport(
    _VE_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr);

vint _VE_rtcpOpen(
    _VE_Queues      *q_ptr,
    _VE_RtcpObject *rtcp_ptr,
    OSAL_NetAddress    remoteAddr,
    OSAL_NetAddress    localAddr);

vint _VE_rtcpReceiverBlock(
    _VE_RtpObject *rtpObj_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset);

void _VE_rtcpReceiverReport(
    _VE_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr);

vint _VE_rtcpRecv(
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr,
    _VE_NetObj *net_ptr);

vint _VE_rtcpSend(
    _VE_Obj          *vtspr_ptr,
    _VE_Queues       *q_ptr,
    _VE_Dsp          *dsp_ptr,
    _VE_StreamObj    *stream_ptr,
    vint                infc,
    vint                streamId);

void _VE_rtcpSenderReport(
    _VE_NetObj     *net_ptr,
    uvint             infc,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr,
    VTSP_StreamDir    dir);

void _VE_rtcpSetCname(
    _VE_NetObj *net_ptr,
    vint          infc,
    const char   *name_ptr);

void _VE_rtcpSetControl(
    _VE_RtcpObject *rtcp_ptr,
    uvint              control,
    uvint              val);

vint _VE_rtcpShutdown(
    _VE_RtcpObject *object_ptr);

/*
 * Send
 */
void _VE_sendEvent(
    _VE_Queues   *q_ptr,
    VTSP_EventMsg  *msg_ptr,
    uvint           infc);

/*
 * Event
 */
void _VE_genEventStreamQuery(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr,
    vint          infc,
    vint          streamId);

void _VE_genEventStream(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr);

/*
 * Recv
 */
void _VE_recvAllCmd(
    _VE_Obj     *ve_ptr,
    _VE_Queues  *q_ptr,
    _VE_Dsp     *dsp_ptr);

/*
 * Commands
 */
void _VE_runDnCmd(
    _VE_Obj     *ve_ptr,
    _VE_Queues  *q_ptr,
    _VE_Dsp     *dsp_ptr,
    _VTSP_CmdMsg  *cmd_ptr);

/*
 * State
 */
void _VE_algStateStream(
    _VE_Dsp   *dsp_ptr,
    uvint        infc,
    uvint        streamId,
    uint32       clearMask,
    uint32       setMask);

void _VE_initEncoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint             type);

void _VE_initDecoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint             type);

void _VE_shutDecoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint           type);

void _VE_shutEncoder(
    _VE_Dsp       *dsp_ptr,
    _VE_StreamObj *stream_ptr,
    vint           type);

/*
 * Coders
 *
 */
void _VE_videoRawToCoded(
    _VE_StreamObj   *stream_ptr,
    Video_Picture   *pic_ptr,
    uvint            encType,
    vint             infc);

vint _VE_videoGetCodedData(
    _VE_StreamObj   *stream_ptr,
    Video_Packet    *pkt_ptr,
    uvint            encType,
    vint             infc);

void _VE_videoCodedToRaw(
    _VE_StreamObj   *stream_ptr,
    Video_Packet    *pkt_ptr,
    uvint            decType,
    vint             infc);

vint _VE_videoGetRawData(
    _VE_StreamObj   *stream_ptr,
    Video_Picture   *pic_ptr,
    uvint            decType,
    vint             infc);

vint _VE_videoRequestKeyFrame(
    _VE_StreamObj   *stream_ptr,
    uvint            encType,
    vint             infc);

vint _VE_videoRequestResolutionChange(
    _VE_StreamObj   *stream_ptr,
    uvint            encType,
    vint             width,
    vint             height,
    vint             infc);

/*
 * Stream
 */

void _VE_videoStreamEncodeSendData(
        _VE_Obj    *ve_ptr,
        _VE_Queues *q_ptr,
        _VE_Dsp    *dsp_ptr);

void _VE_videoStreamEncode(
        _VE_Obj    *ve_ptr,
        _VE_Queues *q_ptr,
        _VE_Dsp    *dsp_ptr);

void _VE_videoStreamDecode(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr);

void _VE_videoStreamDecodeDrawData(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr);

void _VE_videoStreamShutdownEncoders(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr);

void _VE_videoStreamShutdownDecoders(
    _VE_Obj    *ve_ptr,
    _VE_Queues *q_ptr,
    _VE_Dsp    *dsp_ptr);

vint _VE_videoStreamIsActive(
    _VE_Dsp *dsp_ptr);

/*
 * STUN
 */
vint _VE_stunProcess(
    _VE_Queues     *q_ptr,
    _VE_NetObj     *net_ptr);

/*
 * Task
 */

void _VE_task(
    _VE_Obj *ve_ptr);

void _VE_taskEncode(
    _VE_Obj *ve_ptr);

#endif /* End __VE_PRIVATE_H_ */
