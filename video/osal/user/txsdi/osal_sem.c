/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8989 $ $Date: 2009-02-26 07:16:38 -0800 (Thu, 26 Feb 2009) $
 *
 */

#include <osal_sem.h>
#include <osal_mem.h>
#include <sdi_msg.h>
#include <sdi_cfg.h>
#include <sdi_cfg_entity.h>

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
    osa_semid sem_ptr;

    if (initCount < 0) {
        return(NULL);
    }

    /*
     * Create
     */
    if (NULL == (sem_ptr = osa_create_count_sem(
            ENTITY_DEFAULT,
            MEM_HDL_INFRA_DEFAULT,
            "OSAL sem",
            initCount))) {
        return(NULL);
    }

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
    osa_semid sem_ptr;

    /* These enumerations must be 0 and 1 for linux */
    if ((OSAL_SEMB_AVAILABLE != initState) &&
            (OSAL_SEMB_UNAVAILABLE != initState)) {
        return(NULL);
    }

    /*
     * Create
     */
    if (NULL == (sem_ptr = osa_create_count_sem(
            ENTITY_DEFAULT,
            MEM_HDL_INFRA_DEFAULT,
            "OSAL sem",
            (OSAL_SEMB_AVAILABLE != initState) ? 0 : 1))) {
        return(NULL);
    }

    return((OSAL_SemId)sem_ptr);
}

/*
 * ======== OSAL_semMutexCreate() ========
 *
 *  OS Independent Mutex Semaphore Creation
 *
 * Params:   void
 *
 * Returns:  OSAL_SemId - a descriptor of the semaphore
 *
 */
OSAL_SemId OSAL_semMutexCreate(
    void)
{
    return OSAL_semBinaryCreate(OSAL_SEMB_AVAILABLE);
}
/*
 * ======== OSAL_semDelete() ========
 *
 *  OS Independent Semaphore Delete
 *
 * Params:   sId - sem created in one of the above routines
 *
 * Returns:  OSAL_Status to indicate success or failure
 *
 */
OSAL_Status OSAL_semDelete(
    OSAL_SemId sId)
{
    osa_semid sem_ptr = (osa_semid) sId;

    /*
     * ZK:
     * XXX: Effect of destroying semaphores is unknown on blocking tasks (
     *  See man sem_destroy)
     *  Luckily (un) in Linux semaphores cannot be deleted.
     */
    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    osa_delete_sem(sem_ptr);

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_semAcquire() ========
 *
 *  OS Independent Semaphore acquisition
 *
 * Params:   sId -  from one of the OSAL Semaphore create routines above
 *           timeout - in ms
 *
 * Returns:  OSAL_Status indicating success or failure
 *
 */
OSAL_Status OSAL_semAcquire(
    OSAL_SemId sId,
    uint32     msTimeout)
{
    OSAL_SemId sem_ptr = (OSAL_SemId)sId;
    osa_status sem_status;

    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    sem_status = osa_take_sem(sem_ptr, 
        (msTimeout == (uint32)(~0)) ? OSA_INFINITE_WAIT : OSA_NO_WAIT);
    if(sem_status != OSA_SUCCESS) {
        return(OSAL_FAIL);
    }
    
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_semGive() ========
 *
 *  OS Independent Semaphore give
 *
 * Params:   sId -  from one of the OSAL Semaphore create routines above
 *
 * Returns:  OSAL_Status indicating success or failure
 *
 */
OSAL_Status OSAL_semGive(
    OSAL_SemId sId)
{
    OSAL_SemId sem_ptr = (OSAL_SemId)sId;

    if (NULL == sem_ptr) {
        return(OSAL_FAIL);
    }

    osa_give_sem(sem_ptr);

    return(OSAL_SUCCESS);
}

