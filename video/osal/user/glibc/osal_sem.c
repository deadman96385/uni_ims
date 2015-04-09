/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8989 $ $Date: 2009-02-26 23:16:38 +0800 (Thu, 26 Feb 2009) $
 *
 */

#include <osal_sem.h>
#include <osal_mem.h>
#include <osal_string.h>

/*
 * Supported types
 */
typedef enum {
    _OSAL_SEM_NONE = 0,
    _OSAL_SEM_COUNT,
    _OSAL_SEM_MUTEX
} _OSAL_SemType;

/*
 * Internal struct
 */
typedef struct {
    union {
        pthread_mutex_t mutex;
        sem_t           sema;
    } sem;
    _OSAL_SemType       type;
} _OSAL_SemParams;

/*
 * ======== OSAL_semMutexCreate() ========
 *
 * Create a mutex.
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
            sizeof(_OSAL_SemParams),
            1,
            0))) {
        return(NULL);
    }

    /*
     * Create
     */
    if (0 != pthread_mutex_init(&sem_ptr->sem.mutex,
            NULL)) {
        OSAL_memFree(sem_ptr, 0);
        return(NULL);
    }

    sem_ptr->type = _OSAL_SEM_MUTEX;
    return((OSAL_SemId)sem_ptr);
}

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
    if (-1 == sem_init(&sem_ptr->sem.sema,
            0,
            initCount)) {
        OSAL_memFree(sem_ptr, 0);
        return(NULL);
    }

    sem_ptr->type = _OSAL_SEM_COUNT;
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
     */
    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    switch(sem_ptr->type) {
        case _OSAL_SEM_COUNT:
            if (0 != sem_destroy(&sem_ptr->sem.sema)) {
                return(OSAL_FAIL);
            }
            break;
        case _OSAL_SEM_MUTEX:
            if (0 != pthread_mutex_destroy(&sem_ptr->sem.mutex)) {
                return(OSAL_FAIL);
            }
            break;
        default:
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
    struct timespec  abst;
    struct timeval   tp;

    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    /*
     * Three types of wait: blocking, nonblocking, timed.
     * Two types of semaphores.
     */
    switch(sem_ptr->type) {
        case _OSAL_SEM_COUNT:
            if (OSAL_NO_WAIT == msTimeout) {
                if (0 != sem_trywait(&sem_ptr->sem.sema)) {
                    return(OSAL_FAIL);
                }
            }
            else if (OSAL_WAIT_FOREVER == msTimeout) {
                if (0 != sem_wait(&sem_ptr->sem.sema)) {
                    return(OSAL_FAIL);
                }
            }
            else {
                if (0 != gettimeofday(&tp, NULL)) {
                    return (OSAL_FAIL);
                }
                abst.tv_sec  = tp.tv_sec;
                abst.tv_nsec = tp.tv_usec * 1000;
                abst.tv_sec  += msTimeout / 1000;
                msTimeout    %= 1000;
                abst.tv_nsec += msTimeout * 1000000;
                if (abst.tv_nsec > 1000000000) {
                    abst.tv_nsec -= 1000000000;
                    abst.tv_sec  += 1;
                }
                /*
                 * Reutrn failed on timeout because sem was not acquired.
                 */
                if (0 != sem_timedwait(&sem_ptr->sem.sema, &abst)) {
                    return(OSAL_FAIL);
                }
            }
            break;
        case _OSAL_SEM_MUTEX:
            if (OSAL_NO_WAIT == msTimeout) {
                if (0 != pthread_mutex_trylock(&sem_ptr->sem.mutex)) {
                    return(OSAL_FAIL);
                }
            }
            else if (OSAL_WAIT_FOREVER == msTimeout) {
                if (0 != pthread_mutex_lock(&sem_ptr->sem.mutex)) {
                    return(OSAL_FAIL);
                }
            }
            else {
                if (0 != gettimeofday(&tp, NULL)) {
                    return (OSAL_FAIL);
                }
                abst.tv_sec  = tp.tv_sec;
                abst.tv_nsec = tp.tv_usec * 1000;
                abst.tv_sec  += msTimeout / 1000;
                msTimeout    %= 1000;
                abst.tv_nsec += msTimeout * 1000000;
                if (abst.tv_nsec > 1000000000) {
                    abst.tv_nsec -= 1000000000;
                    abst.tv_sec  += 1;
                }
                /*
                 * Reutrn failed on timeout because sem was not acquired.
                 */
                if (0 != pthread_mutex_timedlock(&sem_ptr->sem.mutex, &abst)) {
                    return(OSAL_FAIL);
                }
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
        case _OSAL_SEM_COUNT:
            if (0 != sem_post(&sem_ptr->sem.sema)) {
                return(OSAL_FAIL);
            }
            break;
        case _OSAL_SEM_MUTEX:
            if (0 != pthread_mutex_unlock(&sem_ptr->sem.mutex)) {
                return(OSAL_FAIL);
            }
            break;
        default:
            return(OSAL_FAIL);
    }
    return(OSAL_SUCCESS);
}

