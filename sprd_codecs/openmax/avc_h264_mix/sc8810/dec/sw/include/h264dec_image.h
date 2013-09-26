/******************************************************************************
** File Name:      h264dec_image.h                                           *
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
#ifndef _H264DEC_IMAGE_H_
#define _H264DEC_IMAGE_H_

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

int32 H264Dec_is_new_picture (H264DecContext *img_ptr);
void H264Dec_init_picture (H264DecContext *img_ptr);
void H264Dec_exit_picture (H264DecContext *img_ptr);
int32 H264Dec_remove_unused_frame_from_dpb(H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T * dpb_ptr);
void h264Dec_remove_frame_from_dpb (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int32 pos);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_IMAGE_H_