/******************************************************************************
** File Name:      h264dec_buffer.h                                            *
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
#ifndef _H264DEC_BUFFER_H_
#define _H264DEC_BUFFER_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "h264dec_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void H264Dec_clear_delayed_buffer(H264DecContext *img_ptr);
MMDecRet H264Dec_init_img_buffer (H264DecContext *img_ptr);
MMDecRet H264Dec_init_dpb (H264DecContext *img_ptr, DEC_SPS_T *sps_ptr);
void H264Dec_store_picture_in_dpb (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr);
void H264Dec_reorder_list (H264DecContext *img_ptr, DEC_SLICE_T *curr_slice_ptr);
void H264Dec_init_list (H264DecContext *img_ptr, int32 curr_slice_type);
void H264Dec_flush_dpb (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_BUFFER_H_
