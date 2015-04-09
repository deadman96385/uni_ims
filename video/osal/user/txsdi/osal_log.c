/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 15247 $ $Date: 2011-07-06 14:41:58 -0700 (Wed, 06 Jul 2011) $
 *
 */

#include <stdarg.h>
#include <osal_log.h>
#include <osal_string.h>
/*
#include "sdi_intra_task_config.h"
#include "sdi_intra_config.h"
#include "mta_context.h"
#include "com_lch_interface.h"
#include "mta_thinpack_if.h"
#include "infra_thinpack.h"
#include "mta_frame.h"*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "os_api.h"

#define MAX_STR_SIZE 384

/* Put spreadtrum trace data into sme buffer */
extern int SCI_SendTracePacket(
    uint8 *src,  // Message head
    uint32    len   // Message head len
    );

/* XXX Add a log prefix for easy filter D2 OSAL log. */
static const char _osalLogPrefix[] = "[D2IMS]:";
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
    va_list         arg_list;
    char            arg_str_buff[MAX_STR_SIZE+1];
    uint16          arg_str_len;

    /* Get args */
    va_start(arg_list, format_ptr);/*lint !e718*//*lint !e64*/
    arg_str_buff[MAX_STR_SIZE] = 0;
    /*arg_str_len = vsprintf(arg_str_buff, format_ptr, arg_list);*/
    OSAL_strcpy(arg_str_buff, _osalLogPrefix);
    arg_str_len = (uint16)_vsnprintf(
            arg_str_buff + OSAL_strlen(_osalLogPrefix),
            MAX_STR_SIZE - OSAL_strlen(_osalLogPrefix),
            format_ptr, arg_list) + OSAL_strlen(_osalLogPrefix);
    if ((arg_str_len>MAX_STR_SIZE) || (arg_str_len == (uint16) - 1))
    {
        arg_str_len = MAX_STR_SIZE;
    }
    SCI_ASSERT(arg_str_len <= MAX_STR_SIZE);
    va_end(arg_list);   

    /* No valid info ? */
    if( arg_str_len == 0 )
    {
        return(OSAL_SUCCESS);
    } 
    
    SCI_TRACE_LOW(arg_str_buff);
    /* XXX This API doesn't work???
    SCI_SendTracePacket((uint8 *)arg_str_buff, (uint32)arg_str_len);
    */
    return(OSAL_SUCCESS);
}

/*
 * ======== OSAL_logString() ========
 *
 * Used for logging.  Will NOT attempt to 'construct' a debug log string
 * using a static buffer and printf'esque type routines like OSAL_logMsg().
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
    /* XXX Any length limitation? */
     SCI_TRACE_LOW("%s%s", _osalLogPrefix, string_ptr);
    /* XXX This API doesn't work???
    SCI_SendTracePacket((uint8 *)string_ptr, OSAL_strlen(string_ptr));
    */
    return (OSAL_SUCCESS);
}
