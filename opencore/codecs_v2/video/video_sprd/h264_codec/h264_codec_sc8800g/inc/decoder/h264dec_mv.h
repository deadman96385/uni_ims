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

PUBLIC void H264Dec_read_motionAndRefId (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
PUBLIC void H264Dec_mv_prediction (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

PUBLIC void H264Dec_mv_prediction_P8x8_8x8 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8);
PUBLIC void H264Dec_mv_prediction_P8x8_8x4 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8);
PUBLIC void H264Dec_mv_prediction_P8x8_4x8 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8);
PUBLIC void H264Dec_mv_prediction_P8x8_4x4 (DEC_MB_INFO_T *mb_info_ptr, int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 b8);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_MV_H_