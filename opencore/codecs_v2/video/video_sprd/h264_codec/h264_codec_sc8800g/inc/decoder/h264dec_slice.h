/******************************************************************************
** File Name:      h264dec_slice.h                                           *
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
#ifndef _H264DEC_SLICE_H_
#define _H264DEC_SLICE_H_

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

PUBLIC int32 get_unit (uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len);
PUBLIC int32 get_unit_avc1 (uint8 *pInStream, int32 slice_unit_len);
PUBLIC int32 H264Dec_Process_slice (DEC_NALU_T *nalu_ptr);
PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, DEC_IMAGE_PARAMS_T *img_ptr);
PUBLIC void H264Dec_get_HW_status(DEC_IMAGE_PARAMS_T *img_ptr);
	
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_SLICE_H_