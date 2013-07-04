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


MMDecRet Mp4Dec_InitVop(Mp4DecObject *vo, MMDecInput * dec_input_ptr);
MMDecRet Mp4Dec_decode_vop(Mp4DecObject *vo);
void Mp4Dec_output_one_frame (Mp4DecObject *vo, MMDecOutput *dec_output_ptr);
void Mp4Dec_exit_picture(Mp4DecObject *vo);


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_VOP_H_
