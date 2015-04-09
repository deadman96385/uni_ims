/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20256 $ $Date: 2013-03-29 11:54:40 +0800 (Fri, 29 Mar 2013) $
 *
 */

#include <osal_log.h>

/*
 * ======== OSAL_logMsg() ========
 *
 * Used for logging.
 *
 * Returns
 *  OSAL_SUCCESS
 */
OSAL_Status OSAL_logMsg(
    const char *format_ptr,
    ...)
{
    va_list args;

    va_start(args,
            format_ptr);

    vprintf(format_ptr,
            args);

    va_end(args);

    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_logString() ========
 *
 * Used for logging.  Will NOT attempt to 'construct' a debug log string
 * using a static buffer and printf'esque type routimes like OSAL_logMsg().
 * It will simply print the NULL terminated string provided in 'string_ptr'.
 * Any limitations regarding the length of the string printed will be enforced
 * by the underlying OS and not OSAL.
 *
 * Returns
 *  OSAL_SUCCESS
 */
OSAL_Status OSAL_logString(
    char *string_ptr)
{
    printf("%s", string_ptr);
    return(OSAL_SUCCESS);
}
