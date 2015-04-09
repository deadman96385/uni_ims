/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 16995 $ $Date: 2012-04-02 02:58:25 -0700 (Mon, 02 Apr 2012) $
 *
 */

#include <osal_tmr.h>
#include <osal_mem.h>
#include "osa_interface.h"
#include "sdi_cfg_entity.h"
#include "os_api.h"

extern void osa_set_periodic_timer(
    osa_timerid           timer_id,
    osa_timer_func_ptr    handler_func_ptr,
    void                 *handler_param_ptr,
    uint32            delay );

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
    osa_timerid tid_ptr = (osa_timerid)tId; 
    if (NULL == tid_ptr) { 
        return(OSAL_FAIL); 
    }
    osa_set_timer(tid_ptr, (osa_timer_func_ptr)fx_ptr, arg, msTimeout); 
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
    osa_timerid tid_ptr;

    tid_ptr = (osa_timerid)osa_create_timer(
        ENTITY_IMS_STACK,
        MEM_HDL_STACK_DEFAULT,
        "OSAL tmr");    
    if (NULL == tid_ptr) {
        return (NULL);
    }

    return ((OSAL_TmrId)tid_ptr);
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
    osa_timerid tid_ptr = (osa_timerid)tId;
    
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    /* If the timer is still running, stop it. */
    osa_delete_timer(tid_ptr);

    return(OSAL_SUCCESS);
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
    osa_timerid tid_ptr = (osa_timerid)tId;

    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    /* 
     * These are invalid delay values for periodic and 1-shot timers
     */
    if ((OSAL_WAIT_FOREVER == msTimeout) || (OSAL_NO_WAIT == msTimeout)) {
        return (OSAL_FAIL);
    }

    osa_set_timer(tid_ptr,
        (osa_timer_func_ptr)fx_ptr, arg,
        msTimeout);
    return(OSAL_SUCCESS);
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
    osa_timerid tid_ptr = (osa_timerid)tId;

    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    /* 
     * These are invalid delay values for periodic and 1-shot timers
     */
    if ((OSAL_WAIT_FOREVER == msTimeout) || (OSAL_NO_WAIT == msTimeout)) {
        return (OSAL_FAIL);
    }

    osa_set_periodic_timer(tid_ptr,
        (osa_timer_func_ptr)fx_ptr, arg,
        msTimeout);
    return(OSAL_SUCCESS);
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
    osa_timerid tid_ptr = (osa_timerid)tId;
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    osa_cancel_timer(tid_ptr);
    return(OSAL_SUCCESS);
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
    /*
     *not support on ac501 platform
     */
    return (OSAL_SUCCESS);
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
    osa_timerid tid_ptr = (osa_timerid)tId;
    uint32 time_remaining;
    
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    time_remaining = osa_get_time_remaining(tid_ptr);
    if (0 == time_remaining) {
        return(OSAL_FAIL);
    }
    
    return(OSAL_SUCCESS);
}

