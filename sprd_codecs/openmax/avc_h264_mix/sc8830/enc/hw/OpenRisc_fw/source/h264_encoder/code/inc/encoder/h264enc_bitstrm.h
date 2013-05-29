/******************************************************************************
 ** File Name:      h264enc_bitstrm.h                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of bitstream   *
 **                 operation of mp4 encoder.                                 *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264ENC_BITSTRM_H_
#define _H264ENC_BITSTRM_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
//#include "mp4_basic.h"
#include "h264enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

void H264Enc_InitBitStream(ENC_IMAGE_PARAMS_T *img_ptr);
uint32 H264Enc_OutputBits(uint32 val, uint32 nbits);
uint32 H264Enc_ByteAlign(int32 is_align0);
void WRITE_UE_V(uint32 val);
void WRITE_SE_V (int32 val);
void WRITE_TE_V (int32 x, int32 val);
void H264Enc_rbsp_trailing (void);
void H264Enc_write_nal_start_code (void);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_H264ENC_BITSTRM_H_
