/******************************************************************************
** File Name:      h264dec_cabac.h			                                 *
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
#ifndef _H264DEC_CABAC_H_
#define _H264DEC_CABAC_H_

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

void cabac_new_slice(H264DecContext *img_ptr);
int8 decode_cabac_mb_transform_size (H264DecContext *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
int32 decode_cabac_mb_intra4x4_pred_mode (H264DecContext *img_ptr);
int32 decode_cabac_mb_chroma_pre_mode (H264DecContext *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
int32 decode_cabac_mb_cbp (H264DecContext *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
int32 decode_cabac_mb_dqp (H264DecContext *img_ptr, DEC_MB_INFO_T *curr_mb_ptr);
uint32 decode_cabac_mb_sub_type (void *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr);
int32 decode_cabac_mb_ref (void *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list);
int32 decode_cabac_mb_mvd (void *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list);
int32 decode_cabac_intra_mb_type(H264DecContext *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 ctx_base, int32 intra_slice);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_CABAC_H_
