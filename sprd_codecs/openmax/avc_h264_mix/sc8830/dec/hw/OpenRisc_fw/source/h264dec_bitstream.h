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
#include "vsp_drv_sc8800g.h"
 #include "h264dec_global.h"
#if SIM_IN_WIN
#include "vsp_bsm.h"
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

void H264Dec_InitBitstream_sw (DEC_BS_T * stream, void *pOneFrameBitstream, int32 length);
PUBLIC MMDecRet H264Dec_InitBitstream(void *pOneFrameBitstream, int32 length);
uint32 BitstreamReadBits (DEC_BS_T * stream, int32 nbits);
PUBLIC uint32 READ_UE_V (DEC_BS_T * stream);
PUBLIC int32 READ_SE_V (DEC_BS_T * stream);
PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream);
PUBLIC void H264Dec_flush_left_byte (void);
void fillStreamBfr (DEC_BS_T * stream);
#ifdef PARSER_CMODEL
PUBLIC uint32 H264Dec_byte_aligned (void); //same as H.264 byte_aligned function
PUBLIC void H264Dec_byte_align (void);
#endif

#if SIM_IN_WIN//weihu
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
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24)|0x1,"BSM_flush n bits");\
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
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24)|0x1,"BSM_flush n bits");\
}
#endif		
#else//SIM_IN_WIN
#define BITSTREAMSHOWBITS(bitstream, nbits) OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara") 
#define BITSTREAMFLUSHBITS(stream, nbits) \
	    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24)|0x1,"BSM_flush n bits")
#endif



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
