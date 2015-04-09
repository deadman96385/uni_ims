/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29950 $ $Date: 2014-11-20 09:42:47 +0800 (Thu, 20 Nov 2014) $
 */

#ifndef _SIP_TIMERS_H_
#define _SIP_TIMERS_H_

void SIPTIMER_Init(void);

#define SIPTIMER_REGISTER_BUFFER_TIMER_MS (5000)
#define SIPTIMER_SEND_MSG_TIMER_MS        (10)

typedef void (*tpfSipTimerCB)(tSipHandle hTimer, void *pArg);

typedef struct sSipTimerEntry
{
    tDLListEntry  dll;    /* Must always be first in any DLL managed structure */
    uint32        id;
    tpfSipTimerCB pfCB;
    void         *pArg;
    tSipHandle    hContext; /* used to identify a thread or msg queue that the timer was created in */
    OSAL_TmrId    tmrId;
    OSAL_TmrId    retryTmrId; /* used to send message again, if send failed */
}tSipTimerEntry;

void SIPTIMER_Init(void);

void SIPTIMER_KillModule(void);

void SIPTIMER_RegisterDispatcher(tpfSipDispatcher pfDispatcher);

tSipHandle SIPTIMER_Create(tSipHandle hContext);

void SIPTIMER_Start(
    tSipHandle    hTimer, 
    tpfSipTimerCB pfCB, 
    void         *pArg, 
    uint32        time, 
    vint          repeat);

void SIPTIMER_Stop(tSipHandle hTimer);

void SIPTIMER_Destroy(tSipHandle hTimer);

void SIPTIMER_Expiry(tSipHandle hTimer, uint32 id);

void SIPTIMER_WakeUp(tSipHandle hTimer);

void SIPTIMER_AddWakeUpTime(uint32 msTime);

void SIPTIMER_getWakeUpTime(OSAL_TimeVal *time);

#endif
