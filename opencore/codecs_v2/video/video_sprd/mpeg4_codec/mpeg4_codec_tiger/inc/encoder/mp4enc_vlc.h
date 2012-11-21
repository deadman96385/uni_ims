/******************************************************************************
 ** File Name:      mp4enc_vlc.h                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of vlc         *
 **                 of mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_VLC_H_
#define _MP4ENC_VLC_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4enc_mode.h"
#include "mp4enc_bitstrm.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

uint32 Mp4Enc_OutputMCBPC_Intra(ENC_VOP_MODE_T *pVop_mode, int32 cbpc, uint32 mode);
uint32 Mp4Enc_OutputMCBPC_Inter(ENC_VOP_MODE_T *pVop_mode, int32 cbpc, uint32 mode);
uint32 Mp4Enc_OutputCBPY(ENC_VOP_MODE_T *pVop_mode,  uint32 cbpy);
uint32 Mp4Enc_OutputIntraDC_DPCM(ENC_VOP_MODE_T *pVop_mode, int32 val, BOOLEAN is_lum);
uint32 Mp4Enc_OutputMV(ENC_VOP_MODE_T *pVop_mode, int32 mvint);
uint32 Mp4Enc_OutputIntraBlkDc(ENC_VOP_MODE_T *pVop_mode, uint32 blk_num);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4ENC_VLC_H_
