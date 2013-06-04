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
#if _CMODEL_
#include "bsm_global.h"
#include "common_global.h"
#endif //_CMODEL_
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif
#ifdef _VSP_LINUX_
uint32 H264Dec_ByteConsumed();
#endif

PUBLIC void H264Dec_InitBitstream_sw (DEC_BS_T * stream, void *pOneFrameBitstream, int32 length);

//big endian
#define BITSTREAMSHOWBITS(bitstream, nBits) \
((nBits) <= bitstream->bitsLeft) ? (((bitstream->rdptr[0]) >> (bitstream->bitsLeft - (nBits))) & g_msk [(nBits)]) : \
	((((bitstream->rdptr[0])  << ((nBits) - bitstream->bitsLeft)) | ((bitstream->rdptr[1]) >> (32 - (nBits) + bitstream->bitsLeft))) & g_msk[(nBits)]) 

#define BITSTREAMFLUSHBITS(stream, nbits) \
{ \
	stream->bitcnt += (nbits); \
	if (nbits < stream->bitsLeft) \
	{		\
		stream->bitsLeft -= (nbits);	\
	} \
	else \
	{\
		stream->bitsLeft += 32 - (nbits);\
		stream->rdptr++;\
	}	\
}

PUBLIC uint32 READ_BITS1(DEC_BS_T *stream);
PUBLIC void REVERSE_BITS(DEC_BS_T *stream, uint32 nbits);
PUBLIC uint32 READ_BITS (DEC_BS_T * stream, uint32 nbits);
PUBLIC uint32 READ_UE_V (DEC_BS_T * stream);
PUBLIC int32 READ_SE_V (DEC_BS_T * stream);
PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream);
PUBLIC void H264Dec_flush_left_byte (void);

#define READ_FLC(stream, nbits)	READ_BITS(stream, nbits)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BITSTREAM_H_
