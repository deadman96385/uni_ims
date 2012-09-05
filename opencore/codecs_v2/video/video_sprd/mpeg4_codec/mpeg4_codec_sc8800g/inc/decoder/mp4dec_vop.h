/******************************************************************************
 ** File Name:      mp4dec_vop.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of vop         *
 **                 of mp4 deccoder.                                          *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_VOP_H_
#define _MP4DEC_VOP_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_bitstream.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_exit_picture(DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Dec_Reset (DEC_VOP_MODE_T * vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_VspFrameInit(DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput * dec_input_ptr);
PUBLIC MMDecRet Mp4Dec_DecIVOP(DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_DecPVOP(DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_DecBVOP(DEC_VOP_MODE_T *vop_mode_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_VOP_H_