/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 21868 $ $Date: 2013-08-15 10:53:30 +0800 (Thu, 15 Aug 2013) $
 *
 */

#include <osal_tmr.h>
#include <osal_mem.h>
#include <osal_string.h>

/*
 * Private timer structure
 */
typedef struct {
    OSAL_TmrArg         arg;
    OSAL_TmrPtr         func_ptr;
    pthread_t           timert;
    int                 restart;
    uint32              msTimeout;
    struct timespec     ts;
    pthread_cond_t      cond;
    pthread_mutex_t     mutex;
    int                 running;
} _OSAL_TimerParams;
        

/*
 * ======== _OSAL_tmrFunc() ========
 *
 * Timer body.
 *
 * Returns
 *  void 
 */
void *_OSAL_tmrFunc(
    void *arg_ptr)
{
    _OSAL_TimerParams   *tid_ptr = (_OSAL_TimerParams *)arg_ptr;
    struct timespec      ts;
    uint32               timeout;
    pthread_mutex_t      cmutex;
    vint                 retVal;
    
    if (NULL == tid_ptr) {
        return (NULL);
    }
    if (NULL == tid_ptr->func_ptr) {
        return (NULL);
    }
    
    if (0 != pthread_mutex_init(&cmutex, NULL)) {
        return (NULL);
    }

    /*
     * Lock resources.
     */
    if (0 != pthread_mutex_lock(&tid_ptr->mutex)) {
        return (NULL);
    }

    /*
     * Periodic
     */
    do {
       
        /*
         * Keep original time so that there is no delta involved due to calling
         * user function.
         */
        tid_ptr->ts.tv_sec   += tid_ptr->msTimeout / 1000;
        timeout = tid_ptr->msTimeout % 1000;
        tid_ptr->ts.tv_nsec  += timeout * 1000000;
        
        if (tid_ptr->ts.tv_nsec > 1000000000) {
            tid_ptr->ts.tv_nsec -= 1000000000;
            tid_ptr->ts.tv_sec  += 1;
        }
        
        memcpy(&ts, &tid_ptr->ts, sizeof(ts));

        if (0 != pthread_mutex_lock(&cmutex)) {
            break;
        }
_OSAL_TIMER_WAIT:
        /*
         * To prevent it stucks in pthread_cond_timedwait()
         * if the timer is stopped before _OSAL_tmrFunc() is running
         */
        if (!tid_ptr->running) {
            break;
        }

        retVal = pthread_cond_timedwait(&tid_ptr->cond, &cmutex, &ts);
        if (!tid_ptr->running) {
            /* Then we have been commanded to stop! */
            break;
        }
        if (0 == retVal) {
            /*
             * Then we were signalled and we are still running, let's 'reset'
             * ourself
             */
            goto _OSAL_TIMER_WAIT;
        }
        if (ETIMEDOUT != retVal) {
            pthread_mutex_unlock(&cmutex);
            break;
        }
        if (0 != pthread_mutex_unlock(&cmutex)) {
            break;
        }

        /*
         * Call user function
         */
        (tid_ptr->func_ptr)(tid_ptr->arg);
       
    } while (tid_ptr->restart); /* This task only reads */

    /*
     * Unlock resources
     */
    pthread_mutex_unlock(&tid_ptr->mutex);
    return (NULL);
}

/*
 * ======== _OSAL_tmrStart() ========
 *
 * Start a timer.
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL 
 */
static OSAL_Status _OSAL_tmrStart(
    OSAL_TmrId    tId,
    OSAL_TmrPtr   fx_ptr,
    OSAL_TmrArg   arg,
    uint32        msTimeout,
    int           periodic)
{
    struct timeval      tp;
    _OSAL_TimerParams  *tid_ptr = (_OSAL_TimerParams *)tId;

    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    if (0 != tid_ptr->timert) {
        return(OSAL_FAIL);
    }
        
    if (tid_ptr->running) {
        return(OSAL_FAIL);
    }
    tid_ptr->running = 1;

    /* 
     * These are invalid delay values for periodic and 1-shot timers
     */
    if ((OSAL_WAIT_FOREVER == msTimeout) || (OSAL_NO_WAIT == msTimeout)) {
        tid_ptr->running = 0;
        return (OSAL_FAIL);
    }

    if (0 != gettimeofday(&tp, NULL)) {
        tid_ptr->running = 0;
        return (OSAL_FAIL);
    }

    /*
     * Count from here on when to timeout
     */
    tid_ptr->ts.tv_sec  = tp.tv_sec;
    tid_ptr->ts.tv_nsec = tp.tv_usec * 1000;

    tid_ptr->arg = arg;
    tid_ptr->func_ptr = fx_ptr;
    tid_ptr->restart = periodic;
    tid_ptr->msTimeout = msTimeout;
    
    /*
     * Thread create.
     */
    if (0 != pthread_create(
            &tid_ptr->timert,
            NULL,
            (void *(*)(void *))_OSAL_tmrFunc,
            (void *)tid_ptr)) {
        tid_ptr->running = 0;
        return(OSAL_FAIL);
    }
   
    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_tmrCreate() ========
 *
 * Create a timer.
 *
 * Returns
 *  OSAL_TmrId - a descriptor for the timer, or NULL on failure
 */
OSAL_TmrId OSAL_tmrCreate(
    void)
{
    _OSAL_TimerParams *tid_ptr;

    tid_ptr = (_OSAL_TimerParams *)OSAL_memCalloc(sizeof(_OSAL_TimerParams),
            1,
            0);

    if (tid_ptr) {
        /*
         * Init conditions for exit and wait
         */
        if (0 != pthread_mutex_init(&tid_ptr->mutex, NULL)) {
            OSAL_memFree(tid_ptr, 0);
            return ((OSAL_TmrId)NULL);
        }
        if (0 != pthread_cond_init(&tid_ptr->cond, NULL)) {
            OSAL_memFree(tid_ptr, 0);
            return ((OSAL_TmrId)NULL);
        }
    }

    return((OSAL_TmrId)tid_ptr);
}

/*
 * ======== OSAL_tmrDelete() ========
 *
 * Delete a timer.
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL 
 */
OSAL_Status OSAL_tmrDelete(
    OSAL_TmrId tId)
{
    _OSAL_TimerParams *tid_ptr = (_OSAL_TimerParams *)tId;
    
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    /* If the timer is still running, stop it. */
    if (tid_ptr->running) {
        if (OSAL_FAIL == OSAL_tmrStop(tId)) {
            return (OSAL_FAIL);
        }
    }

    pthread_mutex_destroy(&tid_ptr->mutex);
    pthread_cond_destroy(&tid_ptr->cond);

    return(OSAL_memFree(tid_ptr, 0));
}

/*
 * ======== OSAL_tmrStart() ========
 *
 * Start a timer.
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL 
 */
OSAL_Status OSAL_tmrStart(
    OSAL_TmrId    tId,
    OSAL_TmrPtr   fx_ptr,
    OSAL_TmrArg   arg,
    uint32        msTimeout)
{
    return(_OSAL_tmrStart(tId,
            fx_ptr,
            arg,
            msTimeout,
            0));
}

/*
 * ======== OSAL_tmrPeriodicStart() ========
 *
 * Start a periodic timer.
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL 
 */
OSAL_Status OSAL_tmrPeriodicStart(
    OSAL_TmrId    tId,
    OSAL_TmrPtr   fx_ptr,
    OSAL_TmrArg   arg,
    uint32        msTimeout)
{
    return(_OSAL_tmrStart(tId,
            fx_ptr,
            arg,
            msTimeout,
            1));
}

/*
 * ======== OSAL_tmrStop() ========
 *
 * Stop a timer.
 *
 * Returns
 *  OSAL_SUCCESS or OSAL_FAIL 
 */
OSAL_Status OSAL_tmrStop(
    OSAL_TmrId tId)
{
    _OSAL_TimerParams *tid_ptr = (_OSAL_TimerParams *)tId;
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    if (!tid_ptr->running) {
        return(OSAL_FAIL);
    }

    /*
     * This will break timer sleep.
     */
    tid_ptr->running = 0;

    /* Yield the CPU for timer thread */
    sched_yield();

    pthread_cond_signal(&tid_ptr->cond);

    /*
     * This will wait till timer task exits, or till user function exits.
     */
    tid_ptr->restart = 0;
    pthread_mutex_lock(&tid_ptr->mutex);
    pthread_mutex_unlock(&tid_ptr->mutex);
    
    if (0 != tid_ptr->timert) {
        /* Release pthread resource */
        pthread_join(tid_ptr->timert, NULL);
        tid_ptr->timert = 0;
    }

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_tmrIsRunning() ========
 *
 * Checks if a timer is running.
 *
 * Returns
 * 0 : Not running
 * 1 : Running
 */
OSAL_Status OSAL_tmrIsRunning(
    OSAL_TmrId tId)
{
    _OSAL_TimerParams *tid_ptr = (_OSAL_TimerParams *)tId;
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    return(tid_ptr->running);
}

/*
 * ======== OSAL_tmrInterrupt() ========
 *
 * Wakes up a timer if it's been sleeping and either fires, if the time has
 * elasped, or reset and sleeps until the specified expiration.  This routine
 * is typically called when a mobile device is placed into suspend mode and then
 * awakens from suspend mode.  Calling this routine is used in the case to
 * wake up and reset timers that were totally idle during suspend mode.
 *
 * Returns
 * 0 : Interrupt was successfully signalled.
 * 1 : No interrupt was signalled.
 */
OSAL_Status OSAL_tmrInterrupt(
    OSAL_TmrId tId)
{
    _OSAL_TimerParams *tid_ptr = (_OSAL_TimerParams *)tId;
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    if (!tid_ptr->running) {
        return(OSAL_FAIL);
    }

    /* Signal the condition so the timer task wakes up */
    pthread_cond_signal(&tid_ptr->cond);
    return(OSAL_SUCCESS);
}
