/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7649 $ $Date: 2008-09-16 17:46:42 -0400 (Tue, 16 Sep 2008) $
 */

#include <osal_string.h>

/* 
 * ======== OSAL_snprintf() ========
 * This function is a C library snprintf implementation.
 *
 * Returns:
 *  Number of chars printed.
 */   
vint OSAL_snprintf(
    char       *buf_ptr,
    vint        size,
    const char *format_ptr,
    ...)
{
    va_list ap;
    vint    n;
    
    va_start(ap, format_ptr);
    
    n = vsnprintf(buf_ptr, size, format_ptr, ap);
    
    va_end(ap);

    return (n);
}

EXPORT_SYMBOL(OSAL_snprintf);
