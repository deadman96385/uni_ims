/******************************************************************************
** File Name:      h264dec_bitstream.h                                       *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_BITSTREAM_H_
#define _H264DEC_BITSTREAM_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "h264dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

uint32 H264Dec_ByteConsumed(DEC_BS_T *bs_ptr);
void H264Dec_InitBitstream (DEC_BS_T *bs_ptr, void *pOneFrameBitstream, int32 length);
void H264Dec_flush_left_byte(void);

void flush_bits(DEC_BS_T *bs_ptr, uint32 nbits);
uint32 show_bits(DEC_BS_T *bs_ptr, uint32 nbits);
uint32 read_bits(DEC_BS_T *bs_ptr, uint32 nbits);
uint32 ue_v (DEC_BS_T *bs_ptr);
int32 se_v (DEC_BS_T *bs_ptr);
int32 long_ue_v (DEC_BS_T *bs_ptr);
int32 long_se_v (DEC_BS_T *bs_ptr);
void revserse_bits (DEC_BS_T *bs_ptr, uint32 nbits);

#define SHOW_FLC(nbits)    show_bits(bs_ptr, nbits)
#define FLUSH_FLC(nbits)    flush_bits(bs_ptr, nbits)
#define READ_FLC(nbits)	read_bits(bs_ptr, nbits)
#define READ_FLAG()		(BOOLEAN)read_bits(bs_ptr, 1)
#define UE_V()    ue_v(bs_ptr)
#define SE_V()   se_v(bs_ptr)
#define Long_SE_V()    long_se_v(bs_ptr)
#define Long_UE_V()    long_ue_v(bs_ptr)
#define REVERSE_BITS(nbits) revserse_bits (bs_ptr, nbits)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_BITSTREAM_H_
