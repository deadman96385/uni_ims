/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5344 $ $Date: 2008-02-22 16:47:41 -0600 (Fri, 22 Feb 2008) $
 */

#ifndef _RIR_LOG_H_
#define _RIR_LOG_H_

#include <osal.h>
#include <stdio.h>

extern FILE *_RIR_log_ptr;
extern char *_RIR_logFileName_ptr;

#ifdef RIR_DEBUG_LOG
#define _RIR_logMsg(fmt, args...) \
        _RIR_log_ptr = fopen(_RIR_logFileName_ptr, "a+"); \
        if (NULL != _RIR_log_ptr)  { \
            fprintf(_RIR_log_ptr, fmt, ## args); \
            fclose(_RIR_log_ptr); \
        } \
        else OSAL_logMsg(fmt, ## args)
#else
#define _RIR_logMsg(fmt, args...)
#endif
#endif
