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

PUBLIC void Mp4Enc_VspFrameInit(MP4EncHandle* mp4Handle);
PUBLIC void Mp4Enc_UpdateRefFrame(MP4EncHandle* mp4Handle);
PUBLIC int32 Mp4Enc_EncIVOP(MP4EncHandle* mp4Handle, int32 time_stamp);

PUBLIC int32 Mp4Enc_EncPVOP(MP4EncHandle* mp4Handle, int32 time_stamp, int32 * intra_mb_num);
PUBLIC int32 Mp4Enc_EncNVOP(MP4EncHandle* mp4Handle, int32 time_stamp);

PUBLIC VOP_PRED_TYPE_E Mp4Enc_JudgeFrameType(uint16 iPCount, BOOLEAN bIs_prev_frame_success);


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

#endif  //_MP4ENC_ENCVOP_H_
