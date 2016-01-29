/******************************************************************************
** File Name:      osal_log.h                                           *
** Author:         Xiaowei Luo, Leon.Li                                               *
** DATE:           10/15/2014                                                *
** Copyright:      2014 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 10/15/2014    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _OSAL_LOG
#define _OSAL_LOG

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum _LOG_LEVEL
{
    _LOG_NONE,
    _LOG_ERROR,
    _LOG_WARNING,
    _LOG_INFO,
    _LOG_DEBUG,
    _LOG_TRACE
} _LOG_LEVEL;

#define OMX_DEBUG_LEVEL 4 /* _LOG_INFO */

#define SPRD_CODEC_LOGV(...) OSAL_Log(_LOG_TRACE, __VA_ARGS__)
#define SPRD_CODEC_LOGD(...) if (vo->trace_enabled) {OSAL_Log(_LOG_DEBUG, __VA_ARGS__);}
#define SPRD_CODEC_LOGI(...) OSAL_Log(_LOG_INFO, __VA_ARGS__)
#define SPRD_CODEC_LOGW(...) OSAL_Log(_LOG_WARNING, __VA_ARGS__)
#define SPRD_CODEC_LOGE(...) OSAL_Log(_LOG_ERROR, __VA_ARGS__)

void OSAL_Log(_LOG_LEVEL logLevel, const char *msg, ...);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_OSAL_LOG

