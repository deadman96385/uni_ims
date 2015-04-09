/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19429 $ $Date: 2012-12-22 03:07:07 +0800 (Sat, 22 Dec 2012) $
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
    if (0 == gettimeofday(&tv, NULL)) {
        timeVal_ptr->sec = tv.tv_sec;
        timeVal_ptr->usec = tv.tv_usec;
        return (OSAL_SUCCESS);
    }
    return (OSAL_FAIL);
}

/*
 * ======== OSAL_timeGetTimeOfDay() ========
 *
 * Gets the time of day in the time format defined in ISO 8601 and
 * then writes a NULL terminated string to the buffer indicated by
 * the buff_ptr parameter. The size_ptr parameter indicates the
 * max size of the buffer pointed to by buff_ptr. Upon successful
 * completion the size of the formatted ISO 8601 string will be
 * written to the size_ptr parameter.
 *
 * Returns:
 * OSAL_SUCCESS: buff_ptr and size_ptr are correctly populated.
 * OSAL_FAIL: The contents of buff_ptr are unknown and size_ptr
 * contains the original value.
 */
OSAL_Status OSAL_timeGetISO8601(uint8 *buff_ptr, vint *size_ptr) {
    char *pos_ptr = (char*)buff_ptr;
    vint size = *size_ptr;
    vint bytes;
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        return (OSAL_FAIL);
    }
    if (0 == (bytes = strftime(pos_ptr, size, "%F", tmp))) {
        return (OSAL_FAIL);
    }
    /* advance the pointer and max size */
    pos_ptr += bytes;
    size -= bytes;

    if (2 > size) {
        /* then we have no room. */
        return (OSAL_FAIL);
    }
    /* Add the "T" */
    *pos_ptr++ = 'T';
    size--;

    if (0 == (bytes = strftime(pos_ptr, size, "%T", tmp))) {
        return (OSAL_FAIL);
    }
    /* advance the pointer and max size */
    pos_ptr += bytes;
    /* Indicate the size back. */
    *size_ptr = (pos_ptr - ((char*)buff_ptr));
    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_timeLocalTime() ========
 *
 * This function is to get the currecnt time and convert to OSAL_TimeLocal
 * format.
 *
 * time_ptr: A pointer to a OSAL_TimeLocal object
 *
 * Returns:
 * OSAL_SUCCESS: Get current time successfully and populated to timeLocal_ptr
 * OSAL_FAIL: NULL targetTm_ptr
 */
OSAL_Status OSAL_timeLocalTime(
    OSAL_TimeLocal *timeLocal_ptr)
{
    time_t    t;
    struct tm *tm_ptr;

    if (NULL == timeLocal_ptr) {
        return (OSAL_FAIL);
    }

    t = time(NULL);
    tm_ptr = localtime(&t);

    timeLocal_ptr->sec   = tm_ptr->tm_sec;
    timeLocal_ptr->min   = tm_ptr->tm_min;
    timeLocal_ptr->hour  = tm_ptr->tm_hour;
    timeLocal_ptr->mday  = tm_ptr->tm_mday;
    timeLocal_ptr->mon   = tm_ptr->tm_mon;
    timeLocal_ptr->year  = tm_ptr->tm_year;
    timeLocal_ptr->wday  = tm_ptr->tm_wday;
    timeLocal_ptr->yday  = tm_ptr->tm_yday;
    timeLocal_ptr->isdst = tm_ptr->tm_isdst;

    return (OSAL_SUCCESS);
}
