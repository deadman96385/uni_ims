/******************************************************************************
 ** File Name:      mp4enc_init.h					                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of header      *
 **                 mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_INIT_H_
#define _MP4ENC_INIT_H_

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

PUBLIC void Mp4Enc_InitVolVopPara(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode, uint32 frame_rate);
PUBLIC void Mp4Enc_InitSession(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

#endif //_MP4ENC_INIT_H_
