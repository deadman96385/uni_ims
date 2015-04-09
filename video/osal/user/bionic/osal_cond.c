/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7201 $ $Date: 2008-08-01 19:59:16 -0400 (Fri, 01 Aug 2008) $
 *
 */

#include <osal_cond.h>
#include <osal_mem.h>
#include <osal_sem.h>

/*
 * Private structure for exit condition.
 */
typedef struct {
    OSAL_SemId       sem;
    struct sigaction act;
} _OSAL_CondExitParams;

/*
 * This is on data segment since an application can register an exit callback
 * only once. Subsequent calls to register will fail.
 */
static _OSAL_CondExitParams *_OSAL_condExitParams_ptr = NULL;

/*
 * ======== _OSAL_condApplicationExitFunction() ========
 *
 * This will give a sempahore so that application exit can proceed.
 *
 * Returns
 *  void
 */
static void _OSAL_condApplicationExitFunction(
    int signal)
{
    if (NULL == _OSAL_condExitParams_ptr) {
        return;
    }
    
    if (NULL == _OSAL_condExitParams_ptr->sem) {
        return;
    }

    /*
     * Signal that a exit condition was received.
     */
    OSAL_semGive(_OSAL_condExitParams_ptr->sem);
}

/*
 * ======== OSAL_condApplicationExitRegister() ========
 *
 * This will create a condition to be raised on application exit, and call an
 * exit function.
 *
 * Not that this is to be done once per application.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_condApplicationExitRegister(
    void)
{
    if (NULL == _OSAL_condExitParams_ptr) {
        _OSAL_condExitParams_ptr = (_OSAL_CondExitParams *)OSAL_memCalloc(
                1,
                sizeof(_OSAL_CondExitParams),
                0);
    }
    else {
        return (OSAL_FAIL);
    }

    /*
     * Exit condition sem, create as empty so that we can block
     */
    if (NULL == (_OSAL_condExitParams_ptr->sem = OSAL_semCountCreate(0))) {
        OSAL_memFree(_OSAL_condExitParams_ptr,
                0);
    }

    /* 
     * Set the handler for the desired signals
     */
    _OSAL_condExitParams_ptr->act.sa_handler =
            _OSAL_condApplicationExitFunction;

    /* 
     * Block all other signals while this one is executing
     */
    if (0 != sigfillset(&_OSAL_condExitParams_ptr->act.sa_mask)) {
        OSAL_semDelete(_OSAL_condExitParams_ptr->sem);
        OSAL_memFree(_OSAL_condExitParams_ptr,
                0);
        _OSAL_condExitParams_ptr = NULL;
        return(OSAL_FAIL);
    }

    /* 
     * Set the desired signals
     */
    if (0 != sigaction(SIGINT,
            &_OSAL_condExitParams_ptr->act,
            NULL)) {
        OSAL_semDelete(_OSAL_condExitParams_ptr->sem);
        OSAL_memFree(_OSAL_condExitParams_ptr,
                0);
        _OSAL_condExitParams_ptr = NULL;
        return(OSAL_FAIL);
    }

    if (0 != sigaction(SIGTERM,
            &_OSAL_condExitParams_ptr->act,
            NULL)) {
        OSAL_semDelete(_OSAL_condExitParams_ptr->sem);
        OSAL_memFree(_OSAL_condExitParams_ptr,
                0);
        _OSAL_condExitParams_ptr = NULL;
        return(OSAL_FAIL);
    }

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_condApplicationExitWaitForCondition() ========
 *
 * This will block till exit condition is received.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_condApplicationExitWaitForCondition(
    void)
{
    if (NULL == _OSAL_condExitParams_ptr) {
        return (OSAL_FAIL);
    }
    
    if (NULL == _OSAL_condExitParams_ptr->sem) {
        return (OSAL_FAIL);
    }
 
    /*
     * Block till application exit condition received.
     */
    return(OSAL_semAcquire(_OSAL_condExitParams_ptr->sem, OSAL_WAIT_FOREVER));
}

/*
 * ======== OSAL_condApplicationExitUnregister() ========
 *
 * This will delete a condition to be raised on application exit, previously
 * registered.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 *
 */
OSAL_Status OSAL_condApplicationExitUnregister(
    void)
{
    if (NULL == _OSAL_condExitParams_ptr) {
        return (OSAL_FAIL);
    }
    
    if (NULL != _OSAL_condExitParams_ptr->sem) {
        OSAL_semDelete(_OSAL_condExitParams_ptr->sem);
        _OSAL_condExitParams_ptr->sem = NULL;
    }

    OSAL_memFree(_OSAL_condExitParams_ptr,
            0);

    _OSAL_condExitParams_ptr = NULL;

    return (OSAL_SUCCESS);
}
