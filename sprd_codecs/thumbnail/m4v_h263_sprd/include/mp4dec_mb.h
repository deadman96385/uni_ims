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

void Mp4Dec_DecIntraMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr );
void Mp4Dec_DecInterMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
void Mp4Dec_DecMBHeaderBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
BOOLEAN Mp4Dec_IsIntraDCSwitch(DEC_MB_MODE_T *mb_mode_ptr, int32 intra_dc_vlc_thr);
void Mp4Dec_GetNeighborMBPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);

void Mp4Dec_DecIntraMBTexture_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
void Mp4Dec_DecInterMBTexture_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);

void Mp4Dec_DecIntraMBTexture_sw(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr);
void Mp4Dec_DecInterMBTexture_sw(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr);

void Mp4Dec_DecInterMBTexture_DP_vt(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_DEC_MB_H_
