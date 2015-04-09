
/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_PORT_H_
#define _SIP_PORT_H_

#include <osal.h>
#include <osal_types.h>

#define SIP_MEMALLOC(x) OSAL_memAlloc(x, 0)
#define SIP_MEMFREE(x)  OSAL_memFree(x, 0)
#define SIP_LOG_PERR
typedef OSAL_SemId tSipMutex;
#define SIP_PORT_LOCK_INIT NULL

#define SIP_PORT_SIP_ERR_TASK_STACK  (4096 * 4)

#ifdef VPORT_4G_PLUS_APROC
#define SIP_PORT_SIP_ERR_TASK_NAME   ("aproc.SipErrTask")
#define SIP_PORT_SIP_ERR_QUEUE_NAME  ("aproc.sip.error")
#define SIP_PORT_SIP_NIC_TASK_NAME   ("aproc.SipNicTask")
#else
#define SIP_PORT_SIP_ERR_TASK_NAME   ("SipErrTask")
#define SIP_PORT_SIP_ERR_QUEUE_NAME  ("sip.error")
#define SIP_PORT_SIP_NIC_TASK_NAME   ("SipNicTask")
#endif

#define SIP_PORT_SIP_NIC_TASK_STACK  (4096 * 2)

typedef struct {
    long mtype;
    char *mdata;
} SIP_Msg;

void SIP_Lock(tSipMutex lock);

void SIP_Unlock(tSipMutex lock);

void SIP_MutexInit(tSipMutex *lock);

void SIP_MutexDestroy(tSipMutex lock);

void SIP_TaskExit(void);

#endif
