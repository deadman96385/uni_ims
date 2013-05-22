/******************************************************************************
 ** File Name:      mp4enc_command.h                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/26/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    command  generation and send.                             *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_COMMAND_H_
#define _MP4DEC_COMMAND_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

MMEncRet Mp4Enc_VSPInit (MP4EncHandle* mp4Handle );
void Mp4Enc_MeaCommand(ENC_VOP_MODE_T *pVop_mode, MOTION_VECTOR_T *pmvPred, uint32 bIsIVop, uint32 mb_pos_x);
void Mp4Enc_MbcCommand (int mb_x, int mb_y, int bInter, int cbp);
PUBLIC void Mp4Enc_CheckMBCStatus(ENC_VOP_MODE_T *vop_mode_ptr);
void Mp4Enc_VspMBInit(ENC_VOP_MODE_T *pVop_mode, uint32 mb_pos_x);
void Mp4Enc_ConfigDctQuantMB(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode);
void Mp4Enc_ConfigIqIdctMB(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode);
void Mp4Enc_ConfigVLC(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode);
void Mp4Enc_Picture_Level_Sync(MP4EncHandle* mp4Handle);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_COMMAND_H_
