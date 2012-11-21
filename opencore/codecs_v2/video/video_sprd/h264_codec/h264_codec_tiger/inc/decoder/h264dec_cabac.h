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

void cabac_new_slice();
int32 readIntraPredMode_CABAC (DEC_IMAGE_PARAMS_T *img_ptr);
int32 readCIPredMode_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
int32 readCBP_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
int32 readDquant_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr);
int32 readMB_skip_flagInfo_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
uint32 readB8_typeInfo_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr);
int32 readRefFrame_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list);
int32 readMVD_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list);

int read_significance_map (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr,  int type,  int coeff[]);
void read_significant_coefficients (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr,  int type,  int coeff[]);

PUBLIC void H264Dec_register_readMB_type_func (DEC_IMAGE_PARAMS_T *img_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_CABAC_H_
