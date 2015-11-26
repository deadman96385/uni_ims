/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef __VC_PRIVATE_H_
#define __VC_PRIVATE_H_

#include <osal.h>
#include <h264.h>
#include <h263.h>
#include "_vc_struct.h"

#ifdef VIDEO_DEBUG_LOG
# define _VC_TRACE(x, y) OSAL_logMsg("%s %d\n", x, y)
# define _VC_LOG(fmt, args...) \
         OSAL_logMsg("%s: " fmt, __FUNCTION__, ## args)
#else
# define _VC_TRACE(x,y)
# define _VC_LOG(fmt, args...)
#endif

/* Task related settings. */

#define VC_VTSP_INFC_TASK_NAME     "VES_vtspCmd"
#define VC_RTP_INFC_TASK_NAME      "VES_rtp"
#define VC_RTCP_INFC_TASK_NAME     "VES_rtcp"


#define BUFFER_FLAG_SYNC_FRAME      (1)
#define BUFFER_FLAG_CODEC_CONFIG    (2)

#define FRAME_SPS_PPS_SIZE          (32)

/* the time unit of rx bitrate statistics is 1s */
#define BITRATE_STAT_UNIT           (1000000)
/* the period of rx bitrate statistics is 10s */
#define BITRATE_STAT_PERIOD         (10 * BITRATE_STAT_UNIT)

/* the interval threshold of the two adjacent pkts is 600 ms*/
#define BITRATE_PKT_INTV_THRESHOLD  (600000)

/* bitrate factor level */
#define BITRATE_FACTOR_LEVEL3       (3)
#define BITRATE_FACTOR_LEVEL2       (2)
#define BITRATE_FACTOR_LEVEL1       (1)
#define BITRATE_FACTOR_LEVEL0       (0)

/* bitrate factor base MUST be a power of 2 */
#define BITRATE_FACTOR_BASE_BIT     (2)
#define BITRATE_FACTOR_BASE         (1 << BITRATE_FACTOR_BASE_BIT)

/* bitrate time us gap, 3s*/
#define BITRATE_TIME_US_GAP         (3000000)

/*
 * VC - VCI interfac methods.
 */
vint VC_init(
    void);

void VC_shutdown(
    void);

vint VC_getAppEvent(
    VC_Event *event_ptr,
    char     *eventDesc_ptr,
    vint     *codecType_ptr,
    vint      timeout);

vint VC_sendEncodedFrame(
    uint8 *data_ptr,
    vint   length,
    uint64 tsMs,
    uint8  rcsRtpExtnPayload);

vint VC_getEncodedFrame(
    uint8 **data_ptr,
    vint   *length,
    uint64 *tsMs,
    vint   *flags,
    uint8  *rcsRtpExtnPayload);

vint VC_sendFIR(
    void);

/*
 * Net
 */
OSAL_NetSockId _VC_netSocket(
    OSAL_NetSockType type,
    int              flags);

vint _VC_netClose(
    OSAL_NetSockId socketFd);

vint _VC_netBind(
    OSAL_NetSockId  socketFd,
    OSAL_NetAddress address);

int _VC_netRecvfrom(
    OSAL_NetSockId   socketFd,
    void            *buf_ptr,
    int              maxSize,
    OSAL_NetAddress *sentAddr);

uvint _VC_netSendto(
    OSAL_NetSockId socketFd,
    void            *buf_ptr,
    int              bufSize,
    OSAL_NetAddress  sendAddr);

OSAL_Boolean _VC_netIsSameAddr(
    OSAL_NetAddress addr1,
    OSAL_NetAddress addr2);

/*
 * RTP
 */
vint _VC_rtpClose(
    _VC_RtpObject  *rtp_ptr);

vint _VC_rtpDir(
    _VC_RtpObject  *rtp_ptr,
    vint            dir);

vint _VC_rtpInit(
    _VC_RtpObject  *rtp_ptr);

vint _VC_rtpOpen(
    _VC_RtpObject    *rtp_ptr,
    VTSP_StreamDir    dir,
    OSAL_NetAddress   sendAddr,
    OSAL_NetAddress   recvAddr,
    uint8             rdnDynType,
    uint16            srtpSecurityType,
    char             *srtpSendKey,
    char             *srtpRecvKey);

vint _VC_rtpRecv(
    _VC_Obj        *vc_ptr);

void _VC_rtpSend(
    _VC_Obj           *vc_ptr,
    _VC_RtpObject     *rtp_ptr,
    VTSP_BlockHeader  *hdr_ptr,
    uint8             *data_ptr,
    vint               sendBytes,
    vint               frameCnt);

vint _VC_rtpShutdown(
    _VC_RtpObject  *rtp_ptr);

/*
 * Defaults
 */
void _VC_defaults(
    _VC_Dsp *dsp_ptr);

/*
 * Map
 */
OSAL_INLINE uvint _VC_localToDynamicEncoder(
    VTSP_StreamVideo *streamParam_ptr,
    vint              local);

OSAL_INLINE uvint _VC_dynamicToLocalDecoder(
    VTSP_StreamVideo  *streamParam_ptr,
    vint               dynamic);

OSAL_INLINE _VC_StreamObj *_VC_streamIdToStreamPtr(
        _VC_Dsp *dsp_ptr,
        vint     streamId);

OSAL_INLINE _VC_RtcpObject *_VC_streamIdToRtcpPtr(
        _VC_NetObj   *net_ptr,
        uvint         streamId);

OSAL_INLINE _VC_RtpObject *_VC_streamIdToRtpPtr(
        _VC_NetObj   *net_ptr,
        uvint         streamId);

/*
 * RTCP
 */
vint _VC_rtcpBye(
    _VC_Queues   *q_ptr,
    _VC_NetObj   *net_ptr,
    uvint         infc,
    uvint         streamId);

vint _VC_rtcpClose(
    _VC_Queues    *q_ptr,
    uvint         streamId);

vint _VC_rtcpCname(
    _VC_NetObj       *net_ptr,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset,
    uint32            ssrc);

vint _VC_rtcpInit(
    _VC_RtcpObject   *rtcp_ptr,
    _VC_NetObj       *net_ptr,
    vint              streamId,
    vint              infc);

void _VC_rtcpNullReport(
    _VC_NetObj       *net_ptr,
    uvint             streamId,
    _VTSP_RtcpCmdMsg *msg_ptr);

vint _VC_rtcpOpen(
    _VC_Obj         *vc_ptr,
    vint             streamId,
    OSAL_NetAddress  remoteAddr,
    OSAL_NetAddress  localAddr);

vint _VC_rtcpReceiverBlock(
    _VC_RtpObject    *rtpObj_ptr,
    _VTSP_RtcpCmdMsg *msg_ptr,
    vint              offset);

vint _VC_rtcpRecv(
    _VC_Queues *q_ptr,
    _VC_Dsp    *dsp_ptr,
    _VC_NetObj *net_ptr);

vint _VC_rtcpSend(
    _VC_Obj          *vc_ptr,
    _VC_StreamObj    *stream_ptr,
    vint              infc,
    vint              streamId);

void _VC_rtcpSetCname(
    _VC_NetObj   *net_ptr,
    const char   *name_ptr);

void _VC_rtcpSetControl(
    _VC_Queues         *q_ptr,
    uint16              streamId,
    _VTSP_CmdMsgConfig *config_ptr);

vint _VC_rtcpShutdown(
    _VC_RtcpObject *object_ptr);

/*
 * Send Events
 */
void _VC_sendVtspEvent(
    _VC_Queues     *q_ptr,
    VTSP_EventMsg  *msg_ptr);

void _VC_sendVtspCommand(
    _VC_Queues   *q_ptr,
    _VTSP_CmdMsg  *msg_ptr);

void _VC_sendRtcpCommand(
    _VC_Queues   *q_ptr,
    _VC_RtcpCmdMsg  *msg_ptr);

void _VC_sendAppEvent(
    _VC_Queues   *q_ptr,
    VC_Event      event,
    const char   *eventDesc_ptr,
    vint          codecType);

/*
 * Lip Sync Related
 */
void _VC_LipSync_rtcpRecv(
    _VC_Queues      *q_ptr,
    _VC_RtcpObject   *rtcp_ptr);

void _VC_LipSync_fpsAdjust(
    int32        fps,
    JBV_Obj     *obj_ptr);

void _VC_LipSync_rtpTs(
    uvint        streamId,
    _VC_Queues  *q_ptr,
    JBV_Pkt     *pkt_ptr);

/*
 * Commands
 */
void _VC_runDnCmd(
    _VC_Obj     *vc_ptr);

/*
 * State
 */
void _VC_algStateStream(
    _VC_Dsp   *dsp_ptr,
    uvint      streamId,
    uint32     clearMask,
    uint32     setMask);

/*
 * Stream
 */
vint _VC_videoStreamSendEncodedData(
    _VC_Obj *vc_ptr,
    uint8   *data_ptr,
    vint     length,
    uint64   tsMs,
    uint8    rcsRtpExtnPayload);

vint _VC_videoStreamGetDataToDecode(
    _VC_Obj  *vc_ptr,
    uint8   **data_ptr,
    vint     *length,
    uint64   *tsMs_ptr,
    vint     *flags,
    uint8    *rcsRtpExtnPayload);

vint _VC_videoGetCodedData(
    _VC_StreamObj   *stream_ptr,
    Video_Packet    *pkt_ptr,
    uint8           *data_ptr,
    vint             length,
    uint64           tsMs,
    uvint            encType);

vint _VC_videoRequestKeyFrame(
    _VC_StreamObj   *stream_ptr,
    uvint            encType,
    vint             infc);

vint _VC_videoRequestResolutionChange(
    _VC_StreamObj   *stream_ptr,
    uvint            encType,
    vint             width,
    vint             height,
    vint             infc);

/*
 * ======== _VC_videoStreamSendFir() ========
 *
 * Sends a FIR RTCP Feedback packet
 *
 */
vint _VC_videoStreamSendFir(
    _VC_Obj    *vc_ptr);

/*
 * Task Related
 */
void _VC_task(
    _VC_Obj *vc_ptr);

vint _VC_startRtpRecvTask(
    _VC_StreamObj *stream_ptr);

vint _VC_startRtcpSendRecvTask(
    _VC_StreamObj *stream_ptr);

#endif /* End __VC_PRIVATE_H_ */
