/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7201 $ $Date: 2008-08-01 19:59:16 -0400 (Fri, 01 Aug 2008) $
 *
 */

#include <osal_cond.h>
#include <osal_mem.h>
#include <osal_sem.h>

/*
 * ======== _OSAL_condApplicationExitFunction() ========
 *
 * This will give a sempahore so that application exit can proceed.
 *
 * Returns
 *  void
 */
static void _OSAL_condApplicationExitFunction(
    int signal)
{

}

/*
 * ======== OSAL_condApplicationExitRegister() ========
 *
 * This will create a condition to be raised on application exit, and call an
 * exit function.
 *
 * Not that this is to be done once per application.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_condApplicationExitRegister(
    void)
{

    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_condApplicationExitWaitForCondition() ========
 *
 * This will block till exit condition is received.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 */
OSAL_Status OSAL_condApplicationExitWaitForCondition(
    void)
{
    return (OSAL_FAIL);

}

/*
 * ======== OSAL_condApplicationExitUnregister() ========
 *
 * This will delete a condition to be raised on application exit, previously
 * registered.
 *
 * Returns
 *  OSAL_FAIL or OSAL_SUCCESS
 *
 */
OSAL_Status OSAL_condApplicationExitUnregister(
    void)
{

    return (OSAL_SUCCESS);
}
