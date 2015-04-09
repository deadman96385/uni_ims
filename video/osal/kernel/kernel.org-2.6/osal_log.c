/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL  
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE  
 * APPLIES: "COPYRIGHT 2004,2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"  
 *  
 * $D2Tech$ $Rev: 7076 $ $Date: 2008-07-22 15:41:43 -0400 (Tue, 22 Jul 2008) $
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
    char    buf[256]; /* 
                       * Surely this would be enough without overflowing 4K 
                       * limit of kernel tasks. sufficient because only 
                       * short debug messages ought to be printed from 
                       * kernel space.
                       */
    va_start(args,
            format_ptr);

    buf[0] = 0;
    vsnprintf(buf,
            sizeof(buf) - 1,
            format_ptr, 
            args);

    printk("%s\n",
            buf);

    va_end(args);

    return(OSAL_SUCCESS);
}

EXPORT_SYMBOL(OSAL_logMsg);
