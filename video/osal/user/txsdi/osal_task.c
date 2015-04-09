/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14409 $ $Date: 2011-04-05 18:07:42 -0700 (Tue, 05 Apr 2011) $
 *
 */

#include <osal_task.h>
#include <osal_mem.h>
#include <osal_log.h>

#include "osa_interface.h"
#include "sdi_cfg_entity.h"

/*
 * ======== OSAL_taskCreate() ========
 *
 * OSAL task creation.
 *
 * Returns
 *  Task ID, NULL means error
 */
OSAL_TaskId OSAL_taskCreate(
    char          *name_ptr,
    OSAL_TaskPrio  priority,
    uint32         stackByteSize,
    OSAL_TaskPtr   fx_ptr,
    OSAL_TaskArg   arg)
{
    osa_taskid        tid_ptr;
    osa_priority_type p;

    p = (osa_priority_type)priority;

    if ((NULL == name_ptr) || (NULL == fx_ptr)) {
        return(NULL);
    }
    
    /* 
        * linux limits kthreads to have a stack size <= 1 kernel pg. 
        */ 
    if(stackByteSize > 16384) { 
        return(NULL);
    }
    /* 
     * create kernel thread
     */
    if (NULL == (tid_ptr = (osa_taskid)osa_create_task(
           ENTITY_STACK,
           MEM_HDL_INFRA_DEFAULT,
           name_ptr,
           p,
           stackByteSize,
           (osa_task_func_ptr)fx_ptr, /* no returned value */
           arg,
           OSA_TRUE))) {
        return(NULL);
    }

    return((OSAL_TaskId)tid_ptr);
}

/*
 * ======== OSAL_taskDelete() ========
 *
 * OSAL task deletion.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_taskDelete(
    OSAL_TaskId taskId)
{


    osa_taskid   tid_ptr;

    tid_ptr = (osa_taskid)taskId;
    
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }
    /* XXX Do not need to delete task in platform AC501 */
    /* osa task delete would cause crash */
#if 0
    /* 
     * kill the task
     */
    osa_delete_task(tid_ptr);
#endif
    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_taskDelay() ========
 *
 * Delay task (self only) for specified ms.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status  OSAL_taskDelay(
    uint32 msTimeout)
{
    osa_sleep_task(msTimeout);

    return(OSAL_SUCCESS);
}
