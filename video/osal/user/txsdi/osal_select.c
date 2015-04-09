/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal_select.h>
#include <osal_time.h>
/*
 * ======== OSAL_select() ========
 *
 * This function is used to selectively wait on a set of file descriptors for
 * I/O operation availability.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL:    Failure with proper OSAL error code set.
 */ 
OSAL_Status OSAL_select(
    OSAL_SelectSet     *readSet_ptr,
    OSAL_SelectSet     *writeSet_ptr,
    OSAL_SelectTimeval *timeout_ptr,
    OSAL_Boolean       *isTimeout_ptr)
{
    long            tv;
    int             retval;
    OSAL_SelectSet  readFd;
    OSAL_SelectSet  writeFd;


    if (NULL != timeout_ptr) {
        tv = timeout_ptr->sec * 10;
        tv += timeout_ptr->usec / 100;
    }
    else {
        /*
         * Wait forever is not support by sci_sock_select.
         * Implement it here by waiting 1 second and re-lopp if timed out.
         */
        tv = 10;
    }
    if (readSet_ptr != NULL) {
        readFd = *readSet_ptr;
    }
    if (writeSet_ptr != NULL) {
        writeFd = *writeSet_ptr;
    }

_OSAL_SELECT_LOOP:
    retval = sci_sock_select(readSet_ptr, writeSet_ptr, NULL, tv);
    if (0 == retval) {
        /* Timed out. */
        if (NULL == timeout_ptr) {
            if (readSet_ptr != NULL) {
                *readSet_ptr = readFd;
            }
            if (writeSet_ptr != NULL) {
                *writeSet_ptr = writeFd;
            }       
            /* Wait forever, select again. */
            goto _OSAL_SELECT_LOOP;
        }

        if (NULL != isTimeout_ptr) {
            *isTimeout_ptr = OSAL_TRUE;
        }
        return (OSAL_SUCCESS);
    }
    else if (retval > 0) {
        if (NULL != isTimeout_ptr) {
            *isTimeout_ptr = OSAL_FALSE;
        }
        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_selectAddId() ========
 *
 * This function is used to add an ID (file descriptor) to a bit set.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 */ 
OSAL_Status OSAL_selectAddId(
    OSAL_SelectId   *id_ptr,
    OSAL_SelectSet  *set_ptr)
{
    SCI_FD_SET(*id_ptr, set_ptr);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_selectRemoveId() ========
 *
 * This function is used to remove a socket descriptor from a FD set.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 */ 
OSAL_Status OSAL_selectRemoveId(
    OSAL_SelectId   *id_ptr,
    OSAL_SelectSet  *set_ptr)
{
    SCI_FD_CLR(*id_ptr, set_ptr);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_selectSetInit() ========
 *
 * This function is used to init a FD set.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 */ 
OSAL_Status OSAL_selectSetInit(
    OSAL_SelectSet *set_ptr)
{
    SCI_FD_ZERO(set_ptr);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_selectIsIdSet() ========
 *
 * This function is used to check if a file descriptor is set in a select
 * descriptor set.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 */ 
OSAL_Status OSAL_selectIsIdSet(
    OSAL_SelectId  *id_ptr,
    OSAL_SelectSet *set_ptr,
    OSAL_Boolean   *isSet_ptr)
{
    if (SCI_FD_ISSET(*id_ptr, set_ptr)) {
        *isSet_ptr = OSAL_TRUE;
    }
    else {
        *isSet_ptr = OSAL_FALSE;
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_selectGetTime() ========
 *
 * Gets system time since boot up in OSAL_SelectTimeval.
 * This is a 64-bit time value in sec-usec format.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 *  OSAL_FAIL   : Failure.
 */ 
OSAL_Status OSAL_selectGetTime(
     OSAL_SelectTimeval *timeout_ptr)
{
    OSAL_TimeVal tv;
    if (OSAL_SUCCESS == OSAL_timeGetTimeOfDay(&tv)) {
        timeout_ptr->sec = tv.sec;
        timeout_ptr->usec = tv.usec;
        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}
