/******************************************************************************
** File Name:      mp4dec_command.h                                         *
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
#ifndef _MP4DEC_COMMAND_H_
#define _MP4DEC_COMMAND_H_

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

void Mp4Dec_init_frame_VSP(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr);
void VSP_Mp4DctFillQuantTblBfr(DEC_VOP_MODE_T *vop_mode_ptr);
void Mp4Dec_Reset (DEC_VOP_MODE_T * vop_mode_ptr);
void Mp4Dec_ConfigVldMB(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr);
void Mp4Dec_ConfigIqIdctMB(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
void BS_and_Para (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
void Mp4Dec_MBC_DBK_Cmd(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
void Mp4Dec_VspMBInit (int32 mb_x, int32 mb_y);
void Mp4Dec_VerifyBitstrm(uint8 *pStream, int32 strmLen);

void Mp4Dec_InitBitstream(void *pOneFrameBitstream, int32 length);
void Mp4Dec_CheckMBCStatus(DEC_VOP_MODE_T *vop_mode_ptr);

MMDecRet Mp4Dec_VspFrameInit(DEC_VOP_MODE_T *vop_mode_ptr, uint32 bitstm_addr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_COMMAND_H_
