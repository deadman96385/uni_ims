/******************************************************************************
** File Name:      h264dec_biaridecod.h		                                 *
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
#ifndef _H264DEC_BIARIDECOD_H_
#define _H264DEC_BIARIDECOD_H_

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

uint32 arideco_start_decoding(DEC_IMAGE_PARAMS_T *img_ptr);
void biari_init_context (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *ctx, const int* ini);
uint32 biari_decode_symbol (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr);
uint32 biari_decode_final (DEC_IMAGE_PARAMS_T *img_ptr);
uint32 unary_bin_max_decode (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr, int32 ctx_offset, uint32 max_symbol);
uint32 unary_bin_decode (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr, int32 ctx_offset);
uint32 unary_exp_golomb_mv_decode (DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr, uint32 max_bin);
uint32 biari_decode_symbol_eq_prob(DEC_IMAGE_PARAMS_T *img_ptr);
uint32 exp_golomb_decode_eq_prob(DEC_IMAGE_PARAMS_T *img_ptr, int32 k);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BIARIDECOD_H_
