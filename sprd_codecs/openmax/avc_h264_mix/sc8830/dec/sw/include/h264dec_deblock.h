/******************************************************************************
** File Name:      h264dec_deblock.h                                         *
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
#ifndef _H264DEC_DEBLOCK_H_
#define _H264DEC_DEBLOCK_H_

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

typedef struct
{
    uint8 qp_y;
    uint8 qp_u;
    uint8 qp_v;
    uint8 is_uv;

    uint32 linesize;
    uint32 dir;

} DEBLK_PARAS_T;

void H264Dec_BS_and_Para (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);
void H264Dec_register_deblock_func(H264DecContext *img_ptr);
void H264Dec_deblock_one_frame (H264DecContext *img_ptr);
void H264Dec_deblock_picture(H264DecContext *img_ptr, DEC_STORABLE_PICTURE_T *dec_picture_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_DEBLOCK_H_