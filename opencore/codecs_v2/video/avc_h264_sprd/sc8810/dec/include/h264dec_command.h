/******************************************************************************
** File Name:      h264dec_command.h                                         *
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
#ifndef _H264DEC_COMMAND_H_
#define _H264DEC_COMMAND_H_

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

MMDecRet H264Dec_InitBitstream(/*void *pOneFrameBitstream,*/ int32 length);
MMDecRet H264Dec_init_slice_VSP(DEC_IMAGE_PARAMS_T *img_ptr, DEC_SLICE_T *curr_slice_ptr);
void H264Dec_SeqLevelConfig (DEC_IMAGE_PARAMS_T *img_ptr);
MMDecRet h264Dec_PicLevelSendRefAddressCommmand (DEC_IMAGE_PARAMS_T *img_ptr);
MMDecRet H264Dec_Picture_Level_Sync (DEC_IMAGE_PARAMS_T *img_ptr);
void H264Dec_mb_level_sync (DEC_IMAGE_PARAMS_T *img_ptr);
void H264Dec_ComputeCBPIqtMbc (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_COMMAND_H_
