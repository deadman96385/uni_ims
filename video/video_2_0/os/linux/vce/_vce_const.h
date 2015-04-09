/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12522 $ $Date: 2010-07-13 18:52:44 -0400 (Tue, 13 Jul 2010) $
 */

#ifndef _VCE_CONST_H_
#define _VCE_CONST_H_

#include <osal.h>
/* 
 * Real time task control
 *
 * WAITING : waiting for application to 'start' video
 * RUN : running normally
 * STOP : application control to 'stop' video
 * FINISHED : task has exited
 */
#define _VCE_TASK_WAIT            (0x02)
#define _VCE_TASK_RUN             (0x04)
#define _VCE_TASK_STOP            (0x08)
#define _VCE_TASK_FINISHED        (0x10)

#define _VCE_TASK_MAIN_PRIORITY   (OSAL_TASK_PRIO_VENC)
#define _VCE_TASK_RTP_PRIORITY    (40)
#define _VCE_TASK_STACK_SZ        (OSAL_STACK_SZ_LARGE)

#define _VCE_APP_Q_EVENT_MSG_SZ   (sizeof(_VCE_AppEventMsg))

#define _VCE_APP_Q_MAX_DEPTH      (16)
/*
 * Bug 8351. Maximum size causes packet loss in some cellular network.
 * Reduce some bytes for workaround.
 */
#define _VCE_H264_ENC_MAX_RTP_SIZE (VIDEO_MAX_RTP_SZ - 50)

/*
 *Returns
 */
#define _VCE_OK                (1)
#define _VCE_ERROR             (-1)
#define _VCE_ERROR_INIT        (-2)

#endif
