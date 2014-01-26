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
#include "mp4dec_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_bitstream.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void Mp4Dec_exit_picture(Mp4DecObject *vo);
MMDecRet Mp4Dec_InitVop(Mp4DecObject *vo, MMDecInput * dec_input_ptr);

void write_display_frame(DEC_VOP_MODE_T *vop_mode_ptr,DEC_FRM_BFR *pDecFrame);
void write_display_frame_uvinterleaved(DEC_VOP_MODE_T *vop_mode_ptr,DEC_FRM_BFR *pDecFrame);

void Mp4Dec_ExtendFrame(DEC_VOP_MODE_T *vop_mode_ptr, uint8**Frame );
void Mp4Dec_output_one_frame (Mp4DecObject *vo, MMDecOutput *dec_output_ptr);

MMDecRet Mp4Dec_DecIVOP(DEC_VOP_MODE_T *vop_mode_ptr);
MMDecRet Mp4Dec_DecPVOP(DEC_VOP_MODE_T *vop_mode_ptr);
MMDecRet Mp4Dec_DecBVOP(DEC_VOP_MODE_T *vop_mode_ptr);

void Mp4Dec_Deblock_vop(Mp4DecObject *vo);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_VOP_H_
