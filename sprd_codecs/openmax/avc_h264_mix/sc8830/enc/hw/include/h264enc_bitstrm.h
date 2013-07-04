/******************************************************************************
 ** File Name:      h264enc_bitstrm.h                                         *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/17/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file is the declaration of bitstream operation of    *
 **                 h264 encoder.                                             *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/17/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264ENC_BITSTRM_H_
#define _H264ENC_BITSTRM_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int32 write_ue_v(H264EncObject *vo, uint32 val);
void H264Enc_rbsp_trailing (H264EncObject *vo);
void H264Enc_write_nal_start_code (H264EncObject *vo);
uint32 H264Enc_OutputBits(H264EncObject *vo, uint32 val, uint32 nbits);

#define WRITE_SE_V( val) \
{\
    uint32 val_se = (val) <= 0 ? -(val)*2 : (val)*2-1;    \
    write_ue_v(vo, val_se); \
}

#define WRITE_UE_V(val)     write_ue_v(vo, val)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_H264ENC_BITSTRM_H_
