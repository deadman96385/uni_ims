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
#include "vsp_bsm.h"
#include "vsp_drv_tiger.h"
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

void H264Dec_InitBitstream_sw (DEC_BS_T * stream, void *pOneFrameBitstream, int32 length);
PUBLIC MMDecRet H264Dec_InitBitstream(void *pOneFrameBitstream, int32 length);
//PUBLIC uint32 BitstreamShowBits(DEC_BS_T *bs_ptr, uint32 nbits);
//PUBLIC void BitstreamFlushBits(DEC_BS_T *bs_ptr, uint32 nbits);
PUBLIC uint32 BitstreamReadBits (DEC_BS_T * stream, uint32 nbits);
PUBLIC uint32 inline READ_BITS1(DEC_BS_T *stream);
PUBLIC uint32 READ_UE_V (DEC_BS_T * stream);
PUBLIC int32 READ_SE_V (DEC_BS_T * stream);
PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream);
PUBLIC void H264Dec_flush_left_byte (void);
void fillStreamBfr (DEC_BS_T * stream);

#if 0
#if !defined(CHIP_ENDIAN_LITTLE)	//big endian
#define BITSTREAMSHOWBITS(bitstream, (nBits)) \
((nBits) <= bitstream->bitsLeft) ? ((*bitstream->rdptr) >> (bitstream->bitsLeft - (nBits))) & g_msk [(nBits)] : \
	(((*bitstream->rdptr) & g_msk [bitstream->bitsLeft]) << ((nBits) - bitstream->bitsLeft)) | ((*(bitstream->rdptr+1)) >> (32 - ((nBits) - bitstream->bitsLeft))) 

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
  if (stream->rdptr >=  stream->rdbfr + BITSTREAM_BFR_SIZE)\
  fillStreamBfr (stream); \
} \
}

#else//little endian
#define BITSTREAMSHOWBITS(bitstream, nBits) \
	((nBits) <= bitstream->bitsLeft) ? ( (bitstream->bufa >> (bitstream->bitsLeft - (nBits)) ) & g_msk [(nBits)] ) \
	: ((bitstream->bufa << ((nBits) - bitstream->bitsLeft)) | (bitstream->bufb >> (32 - (nBits) + bitstream->bitsLeft))) & g_msk [(nBits)]

#define BITSTREAMFLUSHBITS(stream, nbits) \
{ \
	uint8 * pCharTmp; \
	stream->bitcnt += (nbits); \
	if (nbits < stream->bitsLeft) \
		stream->bitsLeft -= (nbits);	\
	else \
{\
		stream->bitsLeft += 32 - (nbits); \
		stream->bufa = stream->bufb; \
		if (stream->rdptr >=  stream->rdbfr + BITSTREAM_BFR_SIZE) \
		fillStreamBfr (stream); \
		\
		pCharTmp = (uint8 *)stream->rdptr; \
		((uint8 *)&stream->bufb)[0] = pCharTmp[3]; \
		((uint8 *)&stream->bufb)[1] = pCharTmp[2]; \
		((uint8 *)&stream->bufb)[2] = pCharTmp[1]; \
		((uint8 *)&stream->bufb)[3] = pCharTmp[0]; \
		stream->rdptr++; \
}\
}		

#endif

#endif //0

#define BITSTREAMSHOWBITS(bitstream, nBits) \
	((nBits) <= bitstream->bitsLeft) ? ( (bitstream->bufa >> (bitstream->bitsLeft - (nBits)) ) & g_msk [(nBits)] ) \
	: ((bitstream->bufa << ((nBits) - bitstream->bitsLeft)) | (bitstream->bufb >> (32 - (nBits) + bitstream->bitsLeft))) & g_msk [(nBits)]
#define BITSTREAMFLUSHBITS(stream, nbits) \
{ \
	uint8 * pCharTmp; \
	stream->bitcnt += (nbits); \
	if (nbits < stream->bitsLeft) \
		stream->bitsLeft -= (nbits);	\
	else \
{\
		stream->bitsLeft += 32 - (nbits); \
		stream->bufa = stream->bufb; \
		if (stream->rdptr >=  stream->rdbfr + BITSTREAM_BFR_SIZE) \
		fillStreamBfr (stream); \
		\
		pCharTmp = (uint8 *)stream->rdptr; \
		((uint8 *)&stream->bufb)[0] = pCharTmp[3]; \
		((uint8 *)&stream->bufb)[1] = pCharTmp[2]; \
		((uint8 *)&stream->bufb)[2] = pCharTmp[1]; \
		((uint8 *)&stream->bufb)[3] = pCharTmp[0]; \
		stream->rdptr++; \
}\
}	

//#define BITSTREAMSHOWBITS(stream, nbits) BitstreamShowBits(stream, nbits)
//#define BITSTREAMFLUSHBITS(stream, nbits) BitstreamFlushBits(stream, nbits)
#define SHOW_FLC(stream, nbits)	BITSTREAMSHOWBITS(stream, nbits)
#define READ_FLC(stream, nbits)	BitstreamReadBits(stream, nbits)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BITSTREAM_H_
