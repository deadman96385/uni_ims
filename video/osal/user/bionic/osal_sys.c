/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 28791 $ $Date: 2014-09-11 15:28:46 +0800 (Thu, 11 Sep 2014) $
 * 
 */

#include <osal_sys.h>
#include <osal_mem.h>
#include <osal_string.h>


/*
 * Private process structure.
 */
typedef struct {
    char cmd[256];
} _OSAL_SysProcInfo;

/*
 * ======== OSAL_sysExecute ========
 *
 * Execute a process.
 *
 * Returns: OSAL_SUCCESS or OSAL_FAIL;
 */
OSAL_ProcessId OSAL_sysExecute(
    char *exeName_ptr,
    char *argList_ptr)
{
    _OSAL_SysProcInfo *proc_ptr;

    if (NULL == (proc_ptr = (_OSAL_SysProcInfo *)OSAL_memCalloc(
           sizeof(_OSAL_SysProcInfo),
           1,
           0))) {
        return(NULL);
    }
    
    OSAL_snprintf(proc_ptr->cmd, sizeof(proc_ptr->cmd),
            "%s %s", exeName_ptr, argList_ptr);

    system(proc_ptr->cmd);

    return((OSAL_ProcessId)proc_ptr);
}

/*
 * ======== OSAL_sysTerminate ========
 *
 * Terminate a process.
 *
 * Returns: OSAL_SUCCESS or OSAL_FAIL;
 */
OSAL_Status OSAL_sysTerminate(
    OSAL_ProcessId procId)
{
    _OSAL_SysProcInfo *proc_ptr = (_OSAL_SysProcInfo *)procId;

    if (NULL == proc_ptr) {
        return (OSAL_FAIL);
    }
    return (OSAL_memFree(proc_ptr, 0));
}


/*
 * ======== OSAL_sysAcquireWakelock ========
 *
 * Lock Wakelcok to prevent suspend
 * adapter Android Power managerment
 * 
 * Returns: OSAL_SUCCESS or OSAL_FAIL;
 */
OSAL_Status OSAL_sysAcquireWakelock(void)
{
    int fd;
    int len;
    fd = open("/sys/power/wake_lock", O_RDWR);
    if (fd < 0) {
        return(OSAL_FAIL);
    }
    
    len = write(fd, "D2_SAPP", sizeof("D2_SAPP"));
    close(fd);
    
    if (len > 0) {
        return (OSAL_SUCCESS);
    }
    else {
        return (OSAL_FAIL);
    }
}

/*
 * ======== OSAL_sysReleaseWakelock ========
 *
 * Release Wakelcok of suspend
 * adapter Android Power managerment
 * 
 * Returns: OSAL_SUCCESS or OSAL_FAIL;
 */
OSAL_Status OSAL_sysReleaseWakelock(void)
{
    int fd;
    int len;
    fd = open("/sys/power/wake_unlock", O_RDWR);
    if (fd < 0) {
        return(OSAL_FAIL);
    }
    
    len = write(fd, "D2_SAPP", sizeof("D2_SAPP"));
    close(fd);
    
    if (len > 0) {
        return (OSAL_SUCCESS);
    }
    else {
        return (OSAL_FAIL);
    }
}

/*
 * ======== OSAL_sysGetPid ========
 *
 * Get pid
 *
 * Returns: pid
 */
int OSAL_sysGetPid(
    void)
{
    return getpid();
}

