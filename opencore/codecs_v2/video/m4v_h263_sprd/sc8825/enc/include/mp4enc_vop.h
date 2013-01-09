/******************************************************************************
 ** File Name:      mp4enc_vop.h                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of vop of      *
 **                 mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_VOP_H_
#define _MP4ENC_VOP_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Enc_VspFrameInit(ENC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Enc_UpdateRefFrame(ENC_VOP_MODE_T *pVop_mode);
PUBLIC int32 Mp4Enc_EncIVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp);
PUBLIC int32 Mp4Enc_EncPVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp);
PUBLIC int32 Mp4Enc_EncNVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

#endif  //_MP4ENC_ENCVOP_H_
