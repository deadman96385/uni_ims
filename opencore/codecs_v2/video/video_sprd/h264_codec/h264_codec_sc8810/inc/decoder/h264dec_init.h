/******************************************************************************
** File Name:      h264dec_init.h                                            *
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
#ifndef _H264DEC_INIT_H_
#define _H264DEC_INIT_H_

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

void H264Dec_init_global_para (void);

//sw
void H264Dec_setClipTab (void);
void H264Dec_init_mc_function (void);
void H264Dec_init_intra4x4Pred_function (void);
void H264Dec_init_intra8x8Pred_function (void);
void H264Dec_init_intra16x16Pred_function (void);
void H264Dec_init_intraChromaPred_function (void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_INIT_H_
