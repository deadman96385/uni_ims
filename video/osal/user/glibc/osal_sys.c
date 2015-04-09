/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 20936 $ $Date: 2013-06-05 11:47:54 +0800 (Wed, 05 Jun 2013) $
 * 
 */

#include "osal_sys.h"
#include "osal_mem.h"
#include "osal_string.h"


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

