/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VE_CONST_H_
#define _VE_CONST_H_

#include <osal.h>

/* 
 * Real time task control
 *
 * WAITING : waiting for application to 'start' voice 
 * RUN : running normally
 * STOP : application control to 'stop' voice
 * FINISHED : task has exited
 */
#define _VE_TASK_WAIT            (0x02)
#define _VE_TASK_RUN             (0x04)
#define _VE_TASK_STOP            (0x08)
#define _VE_TASK_FINISHED        (0x10)

#define _VE_TASK_PRIORITY        (OSAL_TASK_PRIO_VDEC)
#define _VE_TASK_STACK_SZ        (OSAL_STACK_SZ_LARGE)       

#define _VE_ENCODE_TASK_PRIORITY    (OSAL_TASK_PRIO_VENC)
#define _VE_ENCODE_TASK_STACK_SZ    (OSAL_STACK_SZ_LARGE)


/* 
 * Stream alg masks (32-bit)
 */
#define _VE_ALG_STREAM_MUTE_RECV (0x000001)   /* from net mute */
#define _VE_ALG_STREAM_MUTE_SEND (0x000002)   /* to net mute */
#define _VE_ALG_STREAM_JB        (0x000800)   /* jitter buffer */
#define _VE_ALG_STREAM_LOOPBACK  (0x008000)  /* send = recv */


/*
 * RTP Constants.
 */
#define _VE_RTP_OK          (1)
#define _VE_RTP_ERROR       (-1)
#define _VE_RTP_NOTREADY    (-1)
#define _VE_RTP_NOT_BOUND   (0)
#define _VE_RTP_BOUND       (1)
#define _VE_RTP_READY       (1)

#define _VE_RTP_MIN_SEQUENTIAL (2)
#define _VE_RTP_MAX_MISORDER   (100)
#define _VE_RTP_MAX_DROPOUT    (300)
#define _VE_RTP_SEQ_MOD        (1 << 16)

/*
 * RTCP Constants
 */
#define _VE_RTCP_TOS_VALUE     (0xB8)

/*
 * Set socket priority (TOS bits) to 10111000b = 0xB8 per bug 908.
 */
#define _VE_RTP_IP_TOS  (0xB8)

/*
 * Encoder decoder.
 */
#define _VE_ENCODER (1)
#define _VE_DECODER (0)

#endif
