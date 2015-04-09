/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal_select.h>

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
    struct timeval  time;
    struct timeval *time_ptr;
    int             retval;

    if (NULL != timeout_ptr) {
        time.tv_sec = timeout_ptr->sec;
        time.tv_usec = timeout_ptr->usec;
        time_ptr = &time;
    }
    else {
        time_ptr = NULL;
    }
/*OSAL_SELECT_LOOP:*/
    retval = select(FD_SETSIZE, readSet_ptr, writeSet_ptr, NULL, time_ptr);

    if (0 == retval) {
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
    FD_SET(*id_ptr, set_ptr);
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
    FD_CLR(*id_ptr, set_ptr);
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_netInitFdset() ========
 *
 * This function is used to init a FD set.
 *
 * Returns:
 *  OSAL_SUCCESS: Success.
 */ 
OSAL_Status OSAL_selectSetInit(
    OSAL_SelectSet *set_ptr)
{
    FD_ZERO(set_ptr);
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
    if (FD_ISSET(*id_ptr, set_ptr)) {
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
    struct timeval tp;
    if (0 == gettimeofday(&tp, NULL)) {
        timeout_ptr->sec = tp.tv_sec;
        timeout_ptr->usec = tp.tv_usec;
        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}
