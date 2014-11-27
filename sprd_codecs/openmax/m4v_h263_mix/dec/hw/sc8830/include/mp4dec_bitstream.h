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

uint32 show_bits (Mp4DecObject *vo, uint32 nbits);
void flush_bits (Mp4DecObject *vo, uint32 nbits);
uint32 read_bits (Mp4DecObject *vo, uint32 nbits);


#define Mp4Dec_FlushBits(nbits) flush_bits(vo, nbits)
#define Mp4Dec_ShowBits(nbits) show_bits(vo, nbits)
#define Mp4Dec_ReadBits(nbits) read_bits(vo, nbits)

uint32 Mp4Dec_ByteAlign_Startcode(Mp4DecObject *vo);
uint32 Mp4Dec_ByteAlign_Mp4(Mp4DecObject *vo);
uint32 Mp4Dec_ShowBitsByteAlign(Mp4DecObject *vo, int32 nbits);
uint32 Mp4Dec_ShowBitsByteAlign_H263(Mp4DecObject *vo, int32 nbits);
MMDecRet Mp4Dec_VerifyBitstrm(Mp4DecObject *vo,uint8 *pStream, int32 strmLen);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_BITSTREAM_H_
