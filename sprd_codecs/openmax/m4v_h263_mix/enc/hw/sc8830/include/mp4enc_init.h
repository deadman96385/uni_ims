/******************************************************************************
 ** File Name:      mp4enc_init.h					                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/09/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of header      *
 **                 mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_INIT_H_
#define _MP4ENC_INIT_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_basic.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void Mp4Enc_InitVolVopPara(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr, uint32 frame_rate);
MMEncRet Mp4Enc_InitSession(Mp4EncObject *vo);
void Mp4Enc_InitVSP(Mp4EncObject *vo);
void Mp4Enc_InitBSM(Mp4EncObject *vo);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

#endif //_MP4ENC_INIT_H_
