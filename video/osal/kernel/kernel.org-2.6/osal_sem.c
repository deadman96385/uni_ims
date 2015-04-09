/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 15035 $ $Date: 2011-06-22 18:29:25 +0800 (Wed, 22 Jun 2011) $
 *
 */

#include <osal_sem.h>
#include <osal_mem.h>

/*
 * Supported types
 */
typedef enum {
    _OSAL_SEM_NONE = 0,
    _OSAL_SEM_BINARY,
    _OSAL_SEM_COUNT,
    _OSAL_SEM_MUTEX
} _OSAL_SemType;

/*
 * Internal struct
 */
typedef struct {
    union {
        struct semaphore sema;
    } sem;
    _OSAL_SemType        type;
    struct {
        int32       recurseCount;
        struct      task_struct *ts_owner;
    } mutex;
} _OSAL_SemParams;

/*
 * ======== OSAL_semCountCreate() ========
 *
 * Create a counting semaphore.
 *
 * Returns
 *  Semaphore ID, NULL means error
 */
OSAL_SemId OSAL_semCountCreate(
    int32 initCount)
{
    _OSAL_SemParams *sem_ptr;

    if (initCount < 0) {
        return(NULL);
    }

    /*
     * Alloc
     */
    if (NULL == (sem_ptr = OSAL_memCalloc(
            sizeof(_OSAL_SemParams),
            1,
            0))) {
        return(NULL);
    }

    /*
     * Create
     */
    sema_init(&sem_ptr->sem.sema, initCount); 

    sem_ptr->type = _OSAL_SEM_COUNT;
    return((OSAL_SemId)sem_ptr);
}

/*
 * ======== OSAL_semBinaryCreate() ========
 *
 * Create a binary semaphore. Init state 0 = unavailable, 1 = available
 *
 * Returns
 *  Semaphore ID, NULL means error
 */
OSAL_SemId OSAL_semBinaryCreate(
    OSAL_SemBState initState)
{
    _OSAL_SemParams *sem_ptr;

    /* These enumerations must be 0 and 1 for linux */
    if ((OSAL_SEMB_AVAILABLE != initState) &&
            (OSAL_SEMB_UNAVAILABLE != initState)) {
        return(NULL);
    }

    /*
     * Alloc
     */
    if (NULL == (sem_ptr = OSAL_memCalloc(
            1,
            sizeof(_OSAL_SemParams),
            0))) {
        return(NULL);
    }

    /*
     * Create
     */
    sema_init(&sem_ptr->sem.sema, initState); 

    sem_ptr->type = _OSAL_SEM_BINARY;
    return((OSAL_SemId)sem_ptr);
}

/*
 * ======== OSAL_semMutexCreate() ========
 *
 * Create a mutex semaphore.
 *
 * Returns
 *  Semaphore ID, NULL means error
 */
OSAL_SemId OSAL_semMutexCreate(
    void)
{
    _OSAL_SemParams *sem_ptr;

    /*
     * Alloc
     */
    if (NULL == (sem_ptr = OSAL_memCalloc(
            1,
            sizeof(_OSAL_SemParams),
            0))) {
        return(NULL);
    }

    sem_ptr->type = _OSAL_SEM_MUTEX;
    sem_ptr->mutex.recurseCount = -1;
    sem_ptr->mutex.ts_owner = (struct task_struct *) NULL;
    sema_init(&sem_ptr->sem.sema, 1);
    return((OSAL_SemId)sem_ptr);
}
/*
 * ======== OSAL_semDelete() ========
 *
 * Delete a mutex or a semaphore.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_semDelete(
    OSAL_SemId sId)
{
    _OSAL_SemParams *sem_ptr = (_OSAL_SemParams *) sId;

    /*
     * ZK:
     * XXX: Effect of destroying semaphores is unknown on blocking tasks (
     *  See man sem_destroy)
     *  Luckily (un) in Linux semaphores cannot be deleted.
     */
    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    sem_ptr->type = _OSAL_SEM_NONE;

    return(OSAL_memFree(sem_ptr, 0));
}

/*
 * ======== OSAL_semAcquire() ========
 *
 * Acquire a mutex or a semaphore.
 *
 * Returns
 *  OSAL_FAIL (timeout, error) or OSAL_SUCCESS (aquired)
 */
OSAL_Status OSAL_semAcquire(
    OSAL_SemId sId,
    uint32     msTimeout)
{
    _OSAL_SemParams *sem_ptr = (_OSAL_SemParams *)sId;

    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    /*
     * Three types of wait: blocking, nonblocking, timed.
     * Two types of semaphores.
     * Timeout not supported in kernel space.
     */

    /* handle mutex recursion first */
    if ((_OSAL_SEM_MUTEX == sem_ptr->type) &&
       (sem_ptr->mutex.ts_owner == current)) {
        /* it's owned by us */
        sem_ptr->mutex.recurseCount++;
        return (OSAL_SUCCESS);
    }
    
    switch(sem_ptr->type) {
        case _OSAL_SEM_COUNT:
        case _OSAL_SEM_BINARY:
        case _OSAL_SEM_MUTEX:
            if (OSAL_NO_WAIT == msTimeout) {
                if (_OSAL_SEM_MUTEX == sem_ptr->type) {
                    /*
                     * mutex recursion complicates things since it implies 
                     * exclusive ownership whereby only the owner can
                     * recursively take the thing. And then give it the same
                     * number of times. Guard using IRQ lockouts...
                     */
                    if (sem_ptr->mutex.ts_owner) {
                        return (OSAL_FAIL);
                    }
                    if (0 == down_trylock(&sem_ptr->sem.sema)) {
                        /* 
                         * we own it and we didn't block
                         */
                        sem_ptr->mutex.ts_owner = current;
                        sem_ptr->mutex.recurseCount = 0;
                        return (OSAL_SUCCESS);
                    }
                    return (OSAL_FAIL);
                }
                
                if (0 == down_trylock(&sem_ptr->sem.sema)) {
                    /* 
                     * we own it and we didn't block
                     */
                    return(OSAL_SUCCESS);
                }
                /* 
                 * sem is unavail but we cant block
                 */
                return(OSAL_FAIL);
            }
            else if (OSAL_WAIT_FOREVER == msTimeout) {
                if (_OSAL_SEM_MUTEX == sem_ptr->type) {
                    if (down_interruptible(&sem_ptr->sem.sema)) {
                        return (OSAL_FAIL);
                    }

                    /* we now own this mutex sem */
                    sem_ptr->mutex.ts_owner = current;
                    sem_ptr->mutex.recurseCount = 0;
                    return (OSAL_SUCCESS);
                }
                
                if (down_interruptible(&sem_ptr->sem.sema)) {
                    /*
                     * Signal received.
                     */
                    return(OSAL_FAIL);
                }
            }
            else {
                return(OSAL_FAIL);
            }
            break;
        default:
            return(OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_semGive() ========
 *
 * Give a mutex or a semaphore.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_semGive(
    OSAL_SemId sId)
{
    _OSAL_SemParams *sem_ptr = (_OSAL_SemParams *)sId;

    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    switch(sem_ptr->type) {
        case _OSAL_SEM_MUTEX:
            if (sem_ptr->mutex.ts_owner == current) {
                /* its already owned by us */
                sem_ptr->mutex.recurseCount--;
                if (-1 == sem_ptr->mutex.recurseCount) {
                    /* we no longer own it */
                    sem_ptr->mutex.ts_owner = (struct task_struct *) NULL;
                    up(&sem_ptr->sem.sema);
                    return (OSAL_SUCCESS);
                }
                return (OSAL_SUCCESS);
            }
            /* current is trying to give it, but it has not taken it.. */
            return (OSAL_FAIL);
            break;

        case _OSAL_SEM_COUNT:
            up(&sem_ptr->sem.sema);
            break;

        case _OSAL_SEM_BINARY:
            /*
             * If count is < 1 then up.
             */

            if (sem_ptr->sem.sema.count <= 0) {
                up(&sem_ptr->sem.sema);
            }
            else {
                return(OSAL_FAIL);
            }
            break;

        default:
            return(OSAL_FAIL);
    }

    return(OSAL_SUCCESS);
}

EXPORT_SYMBOL(OSAL_semBinaryCreate);
EXPORT_SYMBOL(OSAL_semCountCreate);
EXPORT_SYMBOL(OSAL_semMutexCreate);
EXPORT_SYMBOL(OSAL_semDelete);
EXPORT_SYMBOL(OSAL_semAcquire);
EXPORT_SYMBOL(OSAL_semGive);
