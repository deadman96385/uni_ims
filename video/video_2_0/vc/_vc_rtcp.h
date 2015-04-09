/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VC_RTCP_H_
#define _VC_RTCP_H_

#include "_vc_struct.h"

#ifdef VIDEO_RTCP_DEBUG_LOG
# define _VC_RTCP_LOG(fmt, args...) \
        OSAL_logMsg("%s:%d (%s) " fmt, __FILE__, __LINE__, __FUNCTION__, ## args)
#else
# define _VC_RTCP_LOG(fmt, args...)
#endif

/*
 * The reduced minimum RTCP interval in milli seconds
 * RFC 3550 Section 6.2
 * Rtcp reduced mininum Interval  = 360,000 / sessionBandwidthInkbps.
 */
#define _VC_RTCP_REDUCED_MIN_INTERVAL_MILLIS(x) ((360000)/(x))

/*
 * Delta between epoch time(00:00 GMT, Jan 1, 1970) and
 * ntp time(00:00 UTC Jan 1, 1900). This value fits in
 * an unsigned 32 bit int, but not a signed int.
 */
#define _VC_EPOCH_TO_NTPTIME        (2208988800U)

#define _VC_SEC_TO_NTP_MSW(X) \
    ( X + ((uint32) _VC_EPOCH_TO_NTPTIME) )
#define _VC_USEC_TO_NTP_LSW(X) \
    ( (uint32) ( ( ( (uint64) (X) ) << 32) / 1000000) )

#define _VC_NTP_MSW_TO_SEC(X) \
        ( X - ((uint32) _VC_EPOCH_TO_NTPTIME) )
#define _VC_NTP_LSW_TO_USEC(X) \
    ( (uint32) ( ( ( (uint64) (X) ) * 1000000 ) >> 32) )

#define _VC_NTP_64_TO_32(MSW, LSW) \
    ( (MSW << 16) | (LSW >> 16) )

#define _VC_NTP_32_TO_MSEC(X) ( (uint32) ( ( ( (uint64) (X) ) * 1000 ) >> 16) )

/* A PLI will be sent once N or more frames have been lost */
#define _VC_RTCP_PLI_N  (10)
/* The estimated duration of time (in ms) it takes for the encoder to generate a new I-Frame */
/* On a 4.3 GN, I measured a delay of up to 204ms in javaspace. Using 204ms as the estimate */
#define _VC_RTCP_IFRAME_GENERATION_DELAY    (13369)

/* The two 32bit parts of a fixed point 64bit NTP timestamp */
typedef struct {
    uint32 msw;
    uint32 lsw;
} _VC_RtcpNtpTime;

/*
 * ======== _VC_rtcpReadCommand() ========
 *
 * Reads from the rtcpCmdQ and processes any
 * commands in the queue.
 */
vint _VC_rtcpReadCommand(
        _VC_Queues      *q_ptr,
        _VC_Dsp         *dsp_ptr,
        _VC_NetObj      *net_ptr,
        _VC_StreamObj   *stream_ptr);

/* ===== Handle various commands that were sent to the RTCP thread ===== */

void _VC_rtcpHandleSetControl(
    _VC_RtcpObject *rtcp_ptr,
    _VTSP_CmdMsgConfig config);

vint _VC_rtcpHandleOpen(
        _VC_Queues      *q_ptr,
        _VC_Dsp         *dsp_ptr,
        _VC_RtcpObject  *rtcp_ptr,
        _VC_RtcpCmdMsg  *cmd_ptr);

vint _VC_rtcpHandleClose(
    _VC_Queues *q_ptr,
    _VC_NetObj *net_ptr,
    uvint          streamId);

void _VC_rtcpHandleUpdateRtcpMinInterval(
        _VC_Queues         *q_ptr,
        _VC_RtcpObject     *rtcp_ptr,
        VTSP_StreamVideo   *video_ptr);

#endif
