/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 14352 $ $Date: 2011-03-28 15:57:46 -0700 (Mon, 28 Mar 2011) $
 *
 */

#include "osal_sys.h"


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
    return ((OSAL_ProcessId) OSAL_FAIL);
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
    return (OSAL_SUCCESS);
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
    return (OSAL_FAIL);
}

