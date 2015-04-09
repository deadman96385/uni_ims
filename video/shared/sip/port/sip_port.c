/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#include "sip_port.h"

void SIP_TaskExit(void)
{
#ifdef VERBOSE_DEBUG
    OSAL_logMsg("SIP_TaskExit!\n");
#endif
}

/******************************************************************************
 * ================SIP_Lock()===================
 *
 * This function is a MUTEX wrapper to 'lock' or 'take a semaphore".
 * It's used in areas of the stack that need data protection. 
 *
 * lock = A mutex or semaphore ID defined in sip_mutex.h
 *
 * RETURNS: nothing
 ******************************************************************************
 */
void SIP_Lock(tSipMutex lock)
{
    if (OSAL_semAcquire(lock, OSAL_WAIT_FOREVER) != OSAL_SUCCESS) {
        SIP_TaskExit();
    }
}

/******************************************************************************
 * ================SIP_Unlock()===================
 *
 * This function is a MUTEX wrapper to 'unlock' or 'give a semaphore".
 * It's used in areas of the stack that need data protection. 
 *
 * lock = A a mutex or semaphore ID defined in sip_mutex.h
 *
 * RETURNS: nothing
 ******************************************************************************
 */                                                                                                                        
void SIP_Unlock(tSipMutex lock)
{
    if (OSAL_semGive(lock) != OSAL_SUCCESS) {
        SIP_TaskExit();
    }
}

/******************************************************************************
 * ================SIP_MutexInit()===================
 *
 * This function is a MUTEX wrapper to initialize a mutex\semaphore.
 * It's used in areas of the stack that need data protection. 
 *
 * lock = A pointer to a mutex or semaphore object defined in sip_mutex.h
 *
 * RETURNS:
 *         nothing
 ******************************************************************************
 */        
void SIP_MutexInit(tSipMutex *lock)
{
    *lock = OSAL_semMutexCreate();
    if (*lock == NULL) {
        SIP_TaskExit();
    }
}

/******************************************************************************
 * ================SIP_MutexDestroy()===================
 *
 * This function is a MUTEX wrapper to destroy (free) a mutex\semaphore.
 * lock = A mutex or semaphore object ID defined in sip_mutex.h
 *
 * RETURNS:
 *         nothing
 ******************************************************************************
 */        
void SIP_MutexDestroy(tSipMutex lock)
{
    OSAL_semDelete(lock);
}


