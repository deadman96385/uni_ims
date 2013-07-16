/******************************************************************************
 ** File Name:      mp4dec_bitstream.h                                        *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of bitstream   *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_BITSTREAM_H_
#define _MP4DEC_BITSTREAM_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_basic.h"
#include "mp4dec_global.h"
#include "mp4dec_mode.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/*----------------------------------------------------------------------------*
**                            Mcaro Definitions                               *
**---------------------------------------------------------------------------*/
uint32 Mp4Dec_ByteConsumed(DEC_VOP_MODE_T *vop_mode_ptr);
void Mp4Dec_VerifyBitstrm(uint8 *pStream, int32 strmLen);
uint32 Mp4Dec_Show32Bits(DEC_BS_T *bs_ptr);
uint32 Mp4Dec_ShowBits(DEC_BS_T *bs_ptr, uint32 nbits);
void Mp4Dec_FlushBits(DEC_BS_T *bs_ptr, uint32 nbits);
uint32 Mp4Dec_ByteAlign_Mp4(DEC_BS_T *bs_ptr);
uint32 Mp4Dec_ReadBits(DEC_BS_T *bs_ptr, uint32 nbits);
uint32 Mp4Dec_ShowBitsByteAlign(DEC_BS_T *bs_ptr, int32 nbits);
uint32 Mp4Dec_ByteAlign_Startcode(DEC_BS_T *bs_ptr);
uint32 Mp4Dec_ShowBitsByteAlign_H263(DEC_BS_T *bs_ptr, int32 nbits);

void Mp4Dec_InitBitstream(DEC_BS_T *bitstream_ptr, void *pOneFrameBitstream, int32 length);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_BITSTREAM_H_
