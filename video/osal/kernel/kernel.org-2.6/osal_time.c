/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal_time.h>

/*
 * ======== OSAL_timeGetTimeOfDay() ========
 *
 * Gets the number of seconds and microseconds since the Epoch.
 * The timeVal_ptr argument is a pointer to a struct OSAL_TimeVal
 * (as specified in osal_time.h:
 *
 * Returns:
 * OSAL_SUCCESS: timeVal_ptr will be populated.
 * OSAL_FAIL: The contents of timeVal_ptr are invalid.
 */
OSAL_Status OSAL_timeGetTimeOfDay(OSAL_TimeVal *timeVal_ptr) {
    struct timeval tv;

    do_gettimeofday(&tv);
    timeVal_ptr->sec = tv.tv_sec;
    timeVal_ptr->usec = tv.tv_usec;

    return (OSAL_SUCCESS);
    
}

EXPORT_SYMBOL(OSAL_timeGetTimeOfDay);

