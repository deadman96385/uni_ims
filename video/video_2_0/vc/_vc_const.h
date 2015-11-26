/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VC_CONST_H_
#define _VC_CONST_H_

#include <osal.h>
/* 
 * Real time task control
 *
 * WAITING : waiting for application to 'start' video
 * RUN : running normally
 * STOP : application control to 'stop' video
 * FINISHED : task has exited
 */
#define _VC_TASK_WAIT            (0x02)
#define _VC_TASK_RUN             (0x04)
#define _VC_TASK_STOP            (0x08)
#define _VC_TASK_FINISHED        (0x10)

#define _VC_TASK_MAIN_PRIORITY   (OSAL_TASK_PRIO_VENC)
#define _VC_TASK_RTP_PRIORITY    (40)
#define _VC_TASK_RTCP_PRIORITY   (30)
#define _VC_TASK_STACK_SZ        (OSAL_STACK_SZ_LARGE)

#define _VC_APP_Q_EVENT_MSG_SZ   (sizeof(_VC_AppEventMsg))

#define _VC_APP_Q_MAX_DEPTH      (16)
#define _VC_RTCP_Q_MAX_DEPTH     (16)
#define _VC_RTCP_Q_CMD_MSG_SZ    (sizeof(_VC_RtcpCmdMsg))

#define _VC_H264_ENC_MAX_PKTS    (64)

#define _VC_VIDEO_CLOCK_RATE_IN_KHZ (90)

/*
 * In this state we ignore any video packet loss.
 * In this state the TMMBR will not be sent.
 */
#define _VC_TMMBR_STATE_INHIBIT (1)
/*
 * In this state the video packet loss stats is observed.
 * In this state the MBR is same as Remote AS session bandwidth parameter.
 */
#define _VC_TMMBR_STATE_NORMAL (2)
/*
 * In this state we ignore any video packet loss.
 * In this state we are waiting for the other side to react to the TMMBR.
 */
#define _VC_TMMBR_STATE_WAITING (3)
/*
 * In this state the video packet loss stats is observed.
 * In this state the MBR is same as Remote AS session bandwidth parameter.
 */
#define _VC_TMMBR_STATE_BACK_OFF (4)


/* adjust direction of tmmbr */
#define _VC_TMMBR_DIR_LEVEL    (1)
#define _VC_TMMBR_DIR_UP       (2)
#define _VC_TMMBR_DIR_DOWN     (3)

/* new tmmbr FSM is designed to repalce the stale one */
#define _VC_TMMBR_STATE_DONE         (0)
#define _VC_TMMBR_STATE_PENDING      (1)

/* step threshold */
#define _VC_TMMBR_STEP_MIN           (30) /* 30 Kbps */

/* Number of incoming packets to be ignored when TMMBR state is inhibited. */
#define _VC_TMMBR_N_INHIBT ((5) * JBV_OBSERVATION_WINDOW)
/* Number of incoming packets to be ignored when TMMBR state is waiting for response. */
#define _VC_TMMBR_N_WAITING ((10) * JBV_OBSERVATION_WINDOW)
/* Number of incoming packets to be observed when TMMBR state is Normal. */
#define _VC_TMMBR_N_NORMAL (JBV_OBSERVATION_WINDOW)
/* Number of incoming packets to be observed when TMMBR state is Back off. */
#define _VC_TMMBR_N_BACK_OFF ((5) * JBV_OBSERVATION_WINDOW)

/* Lowest bitrate (in kbps) that can be requested using TMMBR. */
#define _VC_TMMBR_LOW_KBPS  (256)
/* Packet loss rate 1 - send TMMBR to reduce bit rate when packet loss % is greater than this value. */
#define _VC_TMMBR_PLR_1 (3)
/* Packet loss rate 2 - send TMMBR to increase bit rate when packet loss % is lesser than this value.*/
#define _VC_TMMBR_PLR_2 (1)

/*
 * Bug 8351. Maximum size causes packet loss in some cellular network.
 * Reduce some bytes for workaround.
 */
#define _VC_H264_ENC_MAX_RTP_SIZE (VIDEO_MAX_RTP_SZ - 50)

/*
 *Returns
 */
#define _VC_OK                (1)
#define _VC_ERROR             (-1)
#define _VC_ERROR_INIT        (-2)

/* 
 * Stream alg masks (32-bit)
 */
#define _VC_ALG_STREAM_MUTE_RECV (0x000001)   /* from net mute */
#define _VC_ALG_STREAM_MUTE_SEND (0x000002)   /* to net mute */
#define _VC_ALG_STREAM_JB        (0x000800)   /* jitter buffer */
#define _VC_ALG_STREAM_LOOPBACK  (0x008000)  /* send = recv */


/*
 * RTP Constants.
 */
#define _VC_RTP_OK          (1)
#define _VC_RTP_ERROR       (-1)
#define _VC_RTP_NOTREADY    (-1)
#define _VC_RTP_NOT_BOUND   (0)
#define _VC_RTP_BOUND       (1)
#define _VC_RTP_READY       (1)

#define _VC_RTP_MIN_SEQUENTIAL (2)
#define _VC_RTP_MAX_MISORDER   (100)
#define _VC_RTP_MAX_DROPOUT    (300)
#define _VC_RTP_SEQ_MOD        (1 << 16)

/* Default min interval for RTCP reports. RFC */
#define _VC_RTCP_MIN_INTERVAL_MILLIS    (5000)

/*
 * RTCP Constants
 */
#define _VC_RTCP_TOS_VALUE     (0xB8)

typedef enum {
    _VC_RTCP_CMD_SEND_RTCP_FB,
    _VC_RTCP_CMD_OPEN,
    _VC_RTCP_CMD_CLOSE,
    _VC_RTCP_CMD_UPDATE_MIN_INTERVAL,
    _VC_RTCP_CMD_CONFIGURE,
} _VC_CmdCode;

/*
 * Set socket priority (TOS bits) to 10111000b = 0xB8 per bug 908.
 */
#define _VC_RTP_IP_TOS  (0xB8)

/*
 * Encoder decoder.
 */
#define _VC_ENCODER (1)
#define _VC_DECODER (0)

#endif
