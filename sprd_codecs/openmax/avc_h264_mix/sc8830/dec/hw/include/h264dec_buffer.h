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
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define H264DEC_UNBIND_FRAME(vo, frame)\
{\
    if((frame)->pBufferHeader!=NULL)\
    {\
        (*((vo)->avcHandle->VSP_unbindCb))((vo)->avcHandle->userdata,(frame)->pBufferHeader);\
        (frame)->pBufferHeader = NULL;\
    }\
}

#define H264DEC_BIND_FRAME(vo, frame)\
{\
    if((frame)->pBufferHeader!=NULL)\
    {\
        (*((vo)->avcHandle->VSP_bindCb))((vo)->avcHandle->userdata, (frame)->pBufferHeader);\
    }\
}


void H264Dec_clear_delayed_buffer(H264DecObject *vo);
MMDecRet H264Dec_init_img_buffer (H264DecObject *vo);
MMDecRet H264Dec_init_dpb (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int type);
void H264Dec_store_picture_in_dpb (H264DecObject *vo, DEC_STORABLE_PICTURE_T *picture_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr);
void H264Dec_reorder_list (H264DecObject *vo);
void H264Dec_reorder_list_mvc (H264DecObject *vo);
MMDecRet H264Dec_init_list (H264DecObject *vo, int32 curr_slice_type);
void H264Dec_flush_dpb (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr);
DEC_FRAME_STORE_T * H264Dec_search_frame_from_DBP(H264DecObject *vo, DEC_STORABLE_PICTURE_T* frame);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_BUFFER_H_
