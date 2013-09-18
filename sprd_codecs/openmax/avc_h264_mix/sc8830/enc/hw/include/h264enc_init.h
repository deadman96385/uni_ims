/******************************************************************************
** File Name:      h264enc_init.h                                             *
** Author:         Xiaowei Luo	                                              *
** DATE:           06/17/2013                                                 *
** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    declaration of initialize function  			      *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 06/17/2013    Xiaowei.Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264ENC_INIT_H_
#define _H264ENC_INIT_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "mmcodec.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/

MMEncRet H264Enc_InitVSP(H264EncObject *vo);
void H264Enc_InitBSM(H264EncObject *vo);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_H264ENC_INIT_H_
