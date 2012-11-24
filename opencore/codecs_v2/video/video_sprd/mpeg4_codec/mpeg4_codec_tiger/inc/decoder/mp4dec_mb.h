/******************************************************************************
 ** File Name:      mp4dec_mb.h                                            *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of macroblock  *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_MB_H_
#define _MP4DEC_MB_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_DecIntraMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr );
PUBLIC void Mp4Dec_DecInterMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_DecMBHeaderBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_DecIntraMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_DecInterMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC BOOLEAN Mp4Dec_IsIntraDCSwitch(DEC_MB_MODE_T *mb_mode_ptr, int32 intra_dc_vlc_thr);
PUBLIC void Mp4Dec_MBC_DBK_Cmd(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_ConfigVldMB(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr);
PUBLIC void Mp4Dec_flv_SwVLD(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr);

PUBLIC void Mp4Dec_VspMBInit (int32 mb_x, int32 mb_y);
PUBLIC void Mp4Dec_CheckMBCStatus(DEC_VOP_MODE_T *vop_mode_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_DEC_MB_H_
