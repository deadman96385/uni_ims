/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 29950 $ $Date: 2014-11-20 09:42:47 +0800 (Thu, 20 Nov 2014) $
 */

#include "sip_sip.h"
#include "sip_list.h"
#include "sip_timers.h"
#include "sip_port.h"
#include "sip_mem_pool.h"

/*
 * Local data types. the free list caches available timer
 * objects to use in the future.
 */
static tpfSipDispatcher _pfTimerDispatcher = NULL;
static OSAL_TimeVal _targetTime = {0, 0};

static tSipTimerEntry* _SIPTIMER_Alloc(void)
{
   tSipTimerEntry *pT;

    pT = (tSipTimerEntry *)SIP_memPoolAlloc(eSIP_OBJECT_TIMER);

   return pT;
}

static void _SIPTIMER_Dealloc(tSipTimerEntry *pT)
{
   /* Release timer block */
   SIP_memPoolFree(eSIP_OBJECT_TIMER, (tDLListEntry *)pT);
}


vint _SIPTIMER_Dispatcher(tSipHandle arg)
{

    tSipTimerEntry *pT = (tSipTimerEntry*)arg;
    tSipIpcMsg ipcMsg;

    /* Initialize value to zero */
    OSAL_memSet(&ipcMsg, 0, sizeof(ipcMsg));

    ipcMsg.type = eSIP_IPC_TIMER_MSG;
    ipcMsg.hOwner = (tSipHandle)pT;
    ipcMsg.id = pT->id;

    /*
     * hContext is really a handle to the UA's 'event' object which
     * contains everything needed to handling events destrined for the
     * application.
     */
    ipcMsg.hContext = pT->hContext;

    if (SIP_FAILED == (*_pfTimerDispatcher)(ipcMsg.hContext, &ipcMsg)) {
        /* Send message failed, start a retry timer to send again */
        OSAL_tmrStart(pT->retryTmrId, (OSAL_TmrPtr)_SIPTIMER_Dispatcher,
                arg, SIPTIMER_SEND_MSG_TIMER_MS);
    }
    return(SIP_OK);
}

/*
 *****************************************************************************
 * ================SIPTIMER_KillModule()===================
 *
 * Initializes the timer module.  Called at SIP init time.
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_Init(void) 
{
    /* Do nothing */
}

/*
 *****************************************************************************
 * ================SIPTIMER_KillModule()===================
 *
 * This function is used to destroy the entire timer module.
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_KillModule(void)
{
    /* Do nothing */
}


/*
 *****************************************************************************
 * ================SIPTIMER_RegisterDispatcher()===================
 *
 * This function registers a fourinte that will be used to dispatch
 * a tinmer event to an appropriate thread.
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_RegisterDispatcher(tpfSipDispatcher pfDispatcher)
{
    _pfTimerDispatcher = pfDispatcher;
    return;
}

/* 
 *****************************************************************************
 * ================SIPTIMER_Create()===================
 *
 * This function returns an instance of a timer object.  The handle returned 
 * is used for future calls to start, stop and destroy timers.
 *
 * RETURNS:
 *         tSipHandle: A handle to a timer resource
 *
 ******************************************************************************
 */
tSipHandle SIPTIMER_Create(tSipHandle hContext)
{
    tSipTimerEntry *pT;

    if (NULL == (pT = _SIPTIMER_Alloc())) {
        return (NULL);
    }
    
    pT->hContext = hContext;
    /* Generate a unique id for this timer. */
    pT->id = SIP_randInt(1, SIP_MAX_POSTIVE_INT);
    return pT;
}

/* 
 *****************************************************************************
 * ================SIPTIMER_Start()===================
 *
 * This function start a timer.
 *
 * hTimer = A handle to a timer that was returned when TIMER_Create() 
 *          was called.
 *
 * pfCB = A pointer to a function that will be called when the timer expires.
 *
 * pArg = An argument that will be called in the parameter list when pfCB 
 *        is fired.
 *
 * time = The time in milliseconds whne the timer should fire and call pfCB
 *
 * repeat = If TRUE the timer will restart every time it fires
 *          If FALSE then the timer just fires onec and stops.
 *
 * RETURNS:
 *         Nothing.
 *
 ******************************************************************************
 */
void SIPTIMER_Start(
    tSipHandle    hTimer, 
    tpfSipTimerCB pfCB,
    void         *pArg,
    uint32        time, 
    vint          repeat)
{
    tSipTimerEntry *pT = (tSipTimerEntry *)hTimer;
    
    if (OSAL_tmrIsRunning(pT->tmrId)) {
        SIPTIMER_Stop(hTimer);
    }
    if (OSAL_tmrIsRunning(pT->retryTmrId)) {
        OSAL_tmrStop(pT->retryTmrId);
    }

    SIP_DebugLog(SIP_DB_TIMER_LVL_2,
            "TIMER_Start: %X time:%d, repeat:%d", (int)pT, time, repeat);

    pT->pfCB = pfCB;
    pT->pArg = pArg;
    if (0 == repeat) {
        OSAL_tmrStart(pT->tmrId, (OSAL_TmrPtr)_SIPTIMER_Dispatcher, hTimer, time);
    }
    else {
        OSAL_tmrPeriodicStart(pT->tmrId, (OSAL_TmrPtr)_SIPTIMER_Dispatcher, hTimer, time);
    }
}

/* 
 *****************************************************************************
 * ================SIPTIMER_Stop()===================
 *
 * This function stops a timer.
 *
 * hTimer = A handle to a timer that was returned when SIPTIMER_Create()
 *          was called.
 *
 * RETURNS:
 *         nothing
 *
 ******************************************************************************
 */
void SIPTIMER_Stop(tSipHandle hTimer)
{
    tSipTimerEntry *pT = (tSipTimerEntry *)hTimer;
    SIP_DebugLog(SIP_DB_TIMER_LVL_2, "SIPTIMER_Stop: %X", (int)pT, 0, 0);
    /* Reset the the id which marks this timer object is now unused/old */
    pT->id = 0;
    OSAL_tmrStop(pT->tmrId);
    OSAL_tmrStop(pT->retryTmrId);
}

/* 
 *****************************************************************************
 * ================SIPTIMER_Destroy()===================
 *
 * This function destroys a timer and returns the timer resource back to the
 * pool of timers.
 *
 * hTimer = A handle to a timer that was returned when TIMER_Create() 
 *          was called.
 *
 * RETURNS:
 *         nothing
 *
 ******************************************************************************
 */
void SIPTIMER_Destroy(tSipHandle hTimer)
{
    tSipTimerEntry *pTimer = (tSipTimerEntry *)hTimer;
    SIP_DebugLog(SIP_DB_TIMER_LVL_2, "SIPTIMER_Destroy: %X", (int)pTimer, 0, 0);
    OSAL_tmrStop(pTimer->tmrId);
    _SIPTIMER_Dealloc(pTimer);
}

/* 
 *****************************************************************************
 * ================SIPTIMER_Expiry()===================
 *
 * This function should be called when a thread gets a timer message.
 * it will then call the callback function registered with the timer object
 *
 * hTimer = This is the handle to the timer that was returned in TIMER_Create()
 *          and also the timer handle that was found in the timer message.
 *
 * id = Is a value that marks if the timer event expiry is old due to 
 *      anopther thread/process killing the timer.
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_Expiry(tSipHandle hTimer, uint32 id)
{
    tSipTimerEntry *pT = (tSipTimerEntry*)hTimer;
    if (id != pT->id) {
        SIP_DebugLog(SIP_DB_TIMER_LVL_1,
                "SIPTIMER_Expiry: Trying to kick off old timer (%X)",
                (int)pT, 0, 0);
        return;
    }
    SIP_DebugLog(SIP_DB_TIMER_LVL_3,
            "SIPTIMER_Expiry: Kicking off timer callback (%X)", (int)pT, 0, 0);
    if (pT->pfCB) {
        pT->pfCB(hTimer, pT->pArg);
    }
    return;
}

/*
 *****************************************************************************
 * ================SIPTIMER_WakeUp()===================
 *
 * This function should be called when the underlying OSAL timers
 * should be awakened and reset. This is typically neccessary sfter a device's
 * hardward had been suspended or placed into a sleep state
 *
 * hTimer = This is the handle to the timer that was returned in TIMER_Create()
 *          and also the timer handle that was found in the timer message.
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_WakeUp(tSipHandle hTimer)
{
    tSipTimerEntry *pT = (tSipTimerEntry*)hTimer;
    if (NULL != pT) {
        if (0 != pT->tmrId) {
            OSAL_tmrInterrupt(pT->tmrId);
        }
    }
}


/*
 *****************************************************************************
 * ================SIPTIMER_AddWakeUpTime()===================
 *
 * This function should be called when the underlying OSAL timers
 * need a watch dog timer. This is typically neccessary sfter a device's
 * hardward had been suspended or placed into a sleep state
 *
 * msTime = This is duration in ms , 0: reset timer
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_AddWakeUpTime(uint32 msTime)
{
    OSAL_TimeVal now;
    
    if (msTime == 0) {
        _targetTime.sec = _targetTime.usec = 0;
    } 
    else {
        /* get current time to compare*/
        OSAL_timeGetTimeOfDay(&now);

        _targetTime.sec = (now.sec + (msTime / 1000));
        msTime = msTime % 1000;
        _targetTime.usec = (now.usec + (msTime * 1000));
    }
}

/*
 *****************************************************************************
 * ================SIPTIMER_getWakeUpTime()===================
 *
 * This function should be called to setup watch dog timer number
 * Watchdog timer will compare current time to this target time.
 *
 * struct timespec *time : target time
 *
 * RETURNS:
 *         Nothing
 *
 ******************************************************************************
 */
void SIPTIMER_getWakeUpTime(OSAL_TimeVal *time)
{
    time->sec = _targetTime.sec;
    time->usec = _targetTime.usec;
}





