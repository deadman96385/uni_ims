/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 25157 $ $Date: 2014-03-17 00:59:26 +0800 (Mon, 17 Mar 2014) $
 */

#ifndef __GSM_CFG_H__
#define __GSM_CFG_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <osal_types.h>
#include <osal.h>
#include <osal_select.h>
#include <osal_sem.h>
#include <osal_log.h>
#include <osal_net.h>

#include <sys/types.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ezxml.h>

/* Used for debug logging */
#ifndef GSM_DEBUG
#define GSM_dbgPrintf(x, ...)
#else
#ifdef VPORT_4G_PLUS_APROC
#define GSM_dbgPrintf(fmt, args...) \
         OSAL_logMsg("AP:" fmt, ## args)
#else
#define GSM_dbgPrintf(fmt, args...) \
         OSAL_logMsg("MP:" fmt, ## args)
#endif
#endif


/* 
 * Used for testing with fd=1 which is a console, 
 * otherwise comment out 
 */
/* #define GSM_TEST_FORCE_FD (1) */

#ifndef GSM_CONFIG_DEFAULT_PATH
#define GSM_CONFIG_DEFAULT_PATH     "."
#endif

#ifdef VPORT_4G_PLUS_APROC
/*
 * Define different message queue name for GSM in application processor.
 */
#define GSM_EVENT_MSG_Q_NAME        "aproc.sm.event"
#else
#define GSM_EVENT_MSG_Q_NAME        "gsm.event"
#endif

#define GSM_READ_TASK_STACK_SZ      (4096)

#define GSM_EVT_Q_LEN               (4)

#define GSM_BUFFER_SZ               (512)

#define GSM_CMD_PIPE_READ           (0)

#define GSM_CMD_PIPE_WRITE          (1)

#define GSM_TMR_RING_TIMEOUT_MS     (6000)

#define GSM_TMR_CID_TIMEOUT_MS      (100)

#define GSM_TMR_REPORT_TIMEOUT_MS   (1000)

#define GSM_CMD_REPORT_TIMEOUT_SECS (2)

#define GSM_READ_DFLT_TIMEOUT_SECS  (1)

#endif
