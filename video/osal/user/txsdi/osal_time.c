/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 556 $ $Date: 2006-09-30 03:22:10 +0800 (Sat, 30 Sep 2006) $
 */

#include <osal_time.h>

/*
 * ======== OSAL_timeGetTimeOfDay() ========
 *
 * Gets the number of seconds. 
 * The timeVal_ptr argument is a pointer to a struct OSAL_TimeVal
 * (as specified in osal_time.h:
 *
 * Returns:
 * OSAL_SUCCESS: timeVal_ptr will be populated.
 * OSAL_FAIL: The contents of timeVal_ptr are invalid.
 */
OSAL_Status OSAL_timeGetTimeOfDay(
    OSAL_TimeVal *timeVal_ptr)
{
    SCI_TIME_T  st;

    if (ERR_TM_NONE != TM_GetSysTime(&st)) {
        return (OSAL_FAIL);
    }
    timeVal_ptr->sec = st.hour * 3600 + st.min * 60 + st.sec;
    timeVal_ptr->usec = 0;

    return (OSAL_SUCCESS);
}

/*
 * ======== OSAL_timeGetISO8601() ========
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
OSAL_Status OSAL_timeGetISO8601(
    uint8 *buff_ptr,
    vint  *size_ptr)
{
    vint        size;
    SCI_DATE_T  sdate;
    SCI_TIME_T  stime;
    
    if(ERR_TM_NONE != TM_GetSysDate(&sdate)) {
        return (OSAL_FAIL);
    }
    
    if(ERR_TM_NONE != TM_GetSysTime(&stime)) {
        return (OSAL_FAIL);
    }

    /* YYYY-MM-DD hh:mm:ss */
    size = (vint)_snprintf((char*)buff_ptr, *size_ptr, "%d-%d-%d %d:%d:%d", 
        sdate.year, sdate.mon, sdate.mday, stime.hour, stime.min, stime.sec);

    if((size > (*size_ptr)) || (size == (vint)-1)) {
        size = (*size_ptr);
    }
    
    /* Indicate the size back. */
    *size_ptr = size;
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
    SCI_TM_T    realtime;

    TM_GetTime(&realtime);

    timeLocal_ptr->sec   = realtime.tm_sec;
    timeLocal_ptr->min   = realtime.tm_min;
    timeLocal_ptr->hour  = realtime.tm_hour;
    timeLocal_ptr->mday  = realtime.tm_mday;
    timeLocal_ptr->mon   = realtime.tm_mon;
    timeLocal_ptr->year  = realtime.tm_year;
    timeLocal_ptr->wday  = realtime.tm_wday;
    timeLocal_ptr->yday  = realtime.tm_yday;
    timeLocal_ptr->isdst = realtime.tm_isdst;

    return (OSAL_SUCCESS);
}
