/******************************************************************************
 ** File Name:      mp4enc_header.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of header      *
 **                 mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_HERDER_H_
#define _MP4ENC_HERDER_H_

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

int32 Mp4Enc_EncH263PicHeader(ENC_VOP_MODE_T *pVop_mode);
uint32 Mp4Enc_EncSequenceHeader(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode);
uint32 Mp4Enc_EncVOHeader( ENC_VOP_MODE_T *pVop_mode);
uint32 Mp4Enc_EncVOLHeader(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode);
uint32 Mp4Enc_EncVOPHeader(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp);
uint32 Mp4Enc_EncGOBHeader(ENC_VOP_MODE_T *pVop_mode, int32 gob_quant);
uint32 Mp4Enc_ReSyncHeader(ENC_VOP_MODE_T * pVop_mode, int Qp, int32 time_stamp);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

#endif //_MP4ENC_ENCHERDER_H_
