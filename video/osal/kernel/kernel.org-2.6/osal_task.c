/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 8754 $ $Date: 2009-01-21 16:46:51 -0500 (Wed, 21 Jan 2009) $
 *
 */

#include <osal_task.h>
#include <osal_mem.h>
#include <osal_log.h>

/*
 * Private task structure
 */
typedef struct {
    OSAL_TaskArg        arg;
    OSAL_TaskPtr        func_ptr;
    struct task_struct *ts_ptr;
    char                name[128];
    int32               priority;
    struct semaphore    complete;
    int                 kt;
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
    int32 exitCode;
    int   retval;
    _OSAL_TaskParams *tid_ptr = (_OSAL_TaskParams *)arg_ptr;
    mm_segment_t oldfs;
    struct sched_param sparam;

    /*
     * Let this task detach from parent.
     */
    daemonize(tid_ptr->name);

    /* 
     * Setup own struct.
     */
    tid_ptr->ts_ptr = current;

    /*
     * in the kernel, scheduler uses ts->prio to id the runqueue. It works like 
     * this: if the rt priority is say 98, and the max is 99, then 99-98=1
     * indicating that bit 1 is set in the bitarray for the runqueue
     * and that this task is queued on queue[1] Thus theres an inverse 
     * relationship and in the task struct, a lower prio indicates a higher
     * priority... However, from the kernel user's pt of view this is
     * backwards
     */
    sparam.sched_priority = tid_ptr->priority;
    if ((retval = sched_setscheduler_nocheck(tid_ptr->ts_ptr,
                SCHED_FIFO,
                &sparam)) < 0) {
        OSAL_logMsg("Warning! task priority not set, error code %d\n",
                retval);
    }
    
    /*
     * OK running now.
     */
    up(&tid_ptr->complete);
    
    /* finally, invoke the user's entry to do the user's real work */
    exitCode = (int32)(tid_ptr->func_ptr)(tid_ptr->arg);
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    OSAL_syscall3(__NR_exit, (long)exitCode, (long)0, (long)0);
    set_fs(oldfs);
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
    _OSAL_TaskParams *tid_ptr;
    int               p;

    p = (int)priority;

    if((NULL == name_ptr) || (NULL == fx_ptr)) {
        return(NULL);
    }
    
    /* 
     * linux limits kthreads to have a stack size <= 1 kernel pg.
     */
    if(stackByteSize > 8192) {
        return(NULL);
    }
    
    /*
     * linux kernel implements two priority ranges. The first is the nice range 
     * which has a range of -20 to 19 with the default being 0. Larger nice
     * values correspond to lower priority. The second range is the real-time 
     * priority which ranges from 0 to 99 which is what kthreads use. 
     * All real-time processes are at a higher priority than normal 
     * (non-realtime) processes. In the kernel, we'll use the real-time policy:
     * SCHED_FIFO and only allow 0...MAX_USER_RT_PRIO-1 (currently 99)
     * (Note that 0 is lowest, 99 is highest - a task running at priority 99 
     * will always preempt tasks running lower than it.)
     */
    if((p < 0) || (p > (MAX_USER_RT_PRIO - 1))) {
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
    tid_ptr->priority = p;
    strncpy(tid_ptr->name,
            name_ptr,
            sizeof(tid_ptr->name) - 1); 

    /*
     * Init startup semaphore as not available.
     */
    sema_init(&tid_ptr->complete,
            0);
   
    /* 
     * create kernel thread
     */
    tid_ptr->kt = kernel_thread((int (*)(void *))_OSAL_taskChild,
            (void *)tid_ptr,
            0); 

    /*
     * Wait till the newly created task has finished initializing.
     */
    down(&tid_ptr->complete);
    
    return(tid_ptr);
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
    struct task_struct *ts_ptr;
    _OSAL_TaskParams   *tid_ptr;
    mm_segment_t        oldfs;

    tid_ptr = (_OSAL_TaskParams *)taskId;
    
    if (NULL == tid_ptr) {
        return(OSAL_FAIL);
    }

    /*
     * ensure task is still present and has not been killed (or died) without
     * our knowledge
     */
    rcu_read_lock();
    ts_ptr = tid_ptr->kt ? find_task_by_vpid(tid_ptr->kt) : current;
    rcu_read_unlock();
    if((NULL == ts_ptr) || (ts_ptr != tid_ptr->ts_ptr)) {
        return(OSAL_FAIL);
    }

    OSAL_memFree(tid_ptr,
            0);
    
    /* 
     * kill the task
     */
    if(ts_ptr == current) {
        /* 
         * If self.
         * we will not return from here
         */
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        OSAL_syscall3(__NR_exit, (long)0, (long)0, (long)0);
        set_fs(oldfs);
    }
    else {
        force_sig(SIGKILL, ts_ptr);
        wake_up_process(ts_ptr);
    }

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
    int32              ticks;
    int32              expire;
    int32              newMs;

    if(OSAL_NO_WAIT == msTimeout) {
        return(OSAL_SUCCESS);
    }
    
    if(OSAL_WAIT_FOREVER == msTimeout) {
        return(OSAL_FAIL);
    }
    
    ticks = (msTimeout * HZ) / 1000;
    newMs = (ticks * 1000) / HZ;
    
    if((msTimeout - newMs) > 0) {
        ticks++;
    }
    
    expire = jiffies + ticks;

    current->state = TASK_INTERRUPTIBLE;
    schedule_timeout(ticks);
    
    if(jiffies < expire) {
        return(OSAL_FAIL);
    }

    return(OSAL_SUCCESS);
}

EXPORT_SYMBOL(OSAL_taskCreate);
EXPORT_SYMBOL(OSAL_taskDelete);
EXPORT_SYMBOL(OSAL_taskDelay);
