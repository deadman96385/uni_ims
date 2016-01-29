/******************************************************************************
 ** File Name:      h264Dec_vld.h                                              *
 ** Author:         jiang.lin                                                *
 ** DATE:           06/01/2006                                                *
 ** Copyright:      2006 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                      *
 ******************************************************************************/

#ifndef _H264DEC_VLD_H_
#define _H264DEC_VLD_H_

#include "h264dec_basic.h"
#include "h264dec_mode.h"

void H264Dec_init_vld_table (void);
void decode_mb_cabac (void *vo, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void decode_mb_cavlc (void *vo, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

#endif
