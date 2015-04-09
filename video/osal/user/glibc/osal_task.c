/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28324 $ $Date: 2014-08-19 15:02:17 +0800 (Tue, 19 Aug 2014) $
 *
 */

#include <osal_task.h>
#include <osal_mem.h>
#include <osal_log.h>
#include <osal_string.h>

typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  c;
    unsigned char   s;
} _OSAL_TaskLock;

/*
 * Private task structure
 */
typedef struct {
    OSAL_TaskArg        arg;
    OSAL_TaskPtr        func_ptr;
    char                name[32];
    pthread_t           tId;
    OSAL_TaskPrio       priority;
    int                 policy;
} _OSAL_TaskParams;

/*
 * ======== _OSAL_taskChild() ========
 *
 * Header and footer of each task.
 *
 * Returns
 *  void
 */
static void _OSAL_taskChild(
    void *arg_ptr)
{
    void                   *exitCode;
    struct sched_param      param;
    pid_t                   pid = syscall(224);
    _OSAL_TaskParams       *tid_ptr = (_OSAL_TaskParams *)arg_ptr;

    /*
     * Set task priority
     */
    param.sched_priority = (int)tid_ptr->priority;
    if (-1 == sched_setscheduler(pid, tid_ptr->policy, &param)) {
        OSAL_logMsg("Warning! task priority not set for task %s(%d) "
                "(run as root?)\n", tid_ptr->name, pid);
    }
    else if (SCHED_FIFO == tid_ptr->policy) {
        OSAL_logMsg("Task %s(%d) is a real time task\n", tid_ptr->name, pid);
    }

    exitCode = (void *)(tid_ptr->func_ptr)(tid_ptr->arg);

    /* we will not return from here */
    pthread_exit(exitCode);
}

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
    _OSAL_TaskParams   *tid_ptr;

    if((NULL == name_ptr) || (NULL == fx_ptr)) {
        return(NULL);
    }

    if (NULL == (tid_ptr = (_OSAL_TaskParams *)OSAL_memCalloc(
           sizeof(_OSAL_TaskParams),
           1,
           0))) {
        return(NULL);
    }

    /*
     * in pthreads and uspace in general, there doesn't need to be a stack lim
     * so stackByteSize will be ignored
     */
    tid_ptr->func_ptr = fx_ptr;
    tid_ptr->arg = arg;
    OSAL_strncpy(tid_ptr->name, name_ptr, sizeof(tid_ptr->name));

    /*
     * Only support 2 policies.
     * SCHED_OTHER is for non real time tasks, set priority to 0 for such tasks
     * SCHED_FIFO is for real time tasks, set priority to > 0 for such tasks
     */
    tid_ptr->priority = priority;
    if (priority < (OSAL_TaskPrio)sched_get_priority_min(SCHED_FIFO)) {
        tid_ptr->policy = SCHED_OTHER;
    }
    else {
        tid_ptr->policy = SCHED_FIFO;
    }

    /*
     * Now create task
     */
    if(0 != pthread_create(
            &tid_ptr->tId,
            NULL,
            (void *(*)(void *))_OSAL_taskChild,
            (void *)tid_ptr)) {
        OSAL_memFree(tid_ptr, 0);
        return(NULL);
    }
    else {
        sched_yield();
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
    OSAL_TaskId tId)
{
    _OSAL_TaskParams *tid_ptr = (_OSAL_TaskParams *)tId;

    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    if (0 == tid_ptr->tId) {
        return(OSAL_FAIL);
    }

    if (0 != pthread_cancel(tid_ptr->tId)) {
        return(OSAL_FAIL);
    }

    return(OSAL_memFree(tid_ptr, 0));
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
    usleep(msTimeout * 1000);
    return(OSAL_SUCCESS);
}
