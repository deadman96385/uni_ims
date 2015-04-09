/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 28444 $ $Date: 2014-08-25 07:51:30 +0800 (Mon, 25 Aug 2014) $
 * 
 */

#include "osal_ut.h"
#include "_utlatch.h"

/*
 * ======== OSALUT_latchInit() ========
 * Initialize a latch with a given key number.
 *
 */
OSAL_Status OSALUT_latchInit(
    OSALUT_Latch *l_ptr,
    uint8         keyNum)
{
    if (NULL == l_ptr) {
        return (OSAL_FAIL);
    }

    l_ptr->semId = OSAL_semMutexCreate();
    l_ptr->count = keyNum;

    OSAL_logMsg("Latch %p inited %d\n", l_ptr, keyNum);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSALUT_latchUnlock() ========
 * Unlock a latch.
 *
 */
OSAL_Status OSALUT_latchUnlock(
    OSALUT_Latch *l_ptr)
{
    if (NULL == l_ptr) {
        return (OSAL_FAIL);
    }
    OSAL_semAcquire(l_ptr->semId, 0);
    if (0 == l_ptr->count) {
        OSAL_logMsg("Latch %p already unlocked.\n", l_ptr);
        return (OSAL_FAIL);
    }
    l_ptr->count--;
    OSAL_logMsg("Latch %p unlocked count:%d.\n", l_ptr, l_ptr->count);
    OSAL_semGive(l_ptr->semId);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSALUT_latchWait() ========
 * Wait a latch until all count are unlocked
 * Note: It will be blocked until all count are unlocked.
 *
 */
OSAL_Status OSALUT_latchWait(
    OSALUT_Latch *l_ptr)
{
    if (NULL == l_ptr) {
        return (OSAL_FAIL);
    }

    while (1) {
        OSAL_semAcquire(l_ptr->semId, 0);
        if (0 == l_ptr->count) {
            OSAL_logMsg("Latch %p wait done!\n", l_ptr);
            return (OSAL_SUCCESS);
        }
        OSAL_semGive(l_ptr->semId);
        /* Wait a while and try again. */
        OSAL_taskDelay(1000);
    }
}
    
/*
 * ======== OSALUT_latchDestroy() ========
 * Destory a latch.
 *
 */
OSAL_Status OSALUT_latchDestroy(
    OSALUT_Latch *l_ptr)
{
    if (NULL == l_ptr) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}
