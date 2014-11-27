/******************************************************************************
** File Name:      h264dec_mv.h                                            *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_MV_H_
#define _H264DEC_MV_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int32 H264Dec_get_te (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list);
int32 decode_cavlc_mb_mvd(void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list);

void H264Dec_read_motionAndRefId (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void H264Dec_read_motionAndRefId_cabac (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void H264Dec_direct_mv_spatial (void *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void H264Dec_direct_mv_temporal (void *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void H264Dec_mv_prediction (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

int32 H264Dec_mv_prediction_P8x8_8x8 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);
int32 H264Dec_mv_prediction_P8x8_8x4 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);
int32 H264Dec_mv_prediction_P8x8_4x8 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);
int32 H264Dec_mv_prediction_P8x8_4x4 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);
int8 H264Dec_mv_prediction_skipped_bslice_spatial (void *img, DEC_MB_CACHE_T *mb_cache_ptr);
int8 H264Dec_mv_prediction_skipped_bslice_temporal (void *img, DEC_MB_CACHE_T *mb_cache_ptr);
int32 H264Dec_MC8x8_direct_spatial(void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);
int32 H264Dec_MC8x8_direct_temporal(void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_MV_H_
