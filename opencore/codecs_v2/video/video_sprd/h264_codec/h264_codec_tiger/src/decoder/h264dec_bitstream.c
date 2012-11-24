/******************************************************************************
 ** File Name:    h264dec_bitstream.c                                         *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "tiger_video_header.h"
#ifndef _VSP_LINUX_
#include "arm_mmu.h"
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#ifdef _VSP_LINUX_
uint32 H264Dec_ByteConsumed()
{
	DEC_BS_T *bs_ptr = g_image_ptr->bitstrm_ptr;

#if 0	
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF,"read BSM_TOTAL_BITS reg ");
#else
	uint32 nDecTotalBits = bs_ptr->bitcnt;
#endif
	return (nDecTotalBits+7)/8;

}
#endif

void H264Dec_InitBitstream_sw (DEC_BS_T * stream, void *pOneFrameBitstream, int32 length)
{
	int32 startcode_len = 0;
	int32 destuffing_cnt = 0;
	uint8 *strm_ptr = (uint8 *)pOneFrameBitstream;

	stream->stream_len = length;
	stream->stream_len_left = length;
	stream->p_stream   = (uint8 *)strm_ptr;
	
	stream->bitsLeft= 0;
	stream->bitcnt = 0;	
	stream->bitcnt_before_vld = stream->bitcnt;

#if defined(CHIP_ENDIAN_LITTLE)
	stream->rdptr = stream->rdbfr;
#else
//	SCI_TRACE_LOW("H264Dec_InitBitstream_sw 2\n");
	stream->rdptr = stream->rdbfr + 1;
#endif
	
	if (BITSTREAM_BFR_SIZE * sizeof (uint32) >= length)
	{
		memcpy (stream->rdptr, stream->p_stream, length);
		stream->stream_len_left = 0;
	}
	else
	{
		memcpy (stream->rdptr, stream->p_stream, BITSTREAM_BFR_SIZE * sizeof (uint32));
		stream->p_stream += BITSTREAM_BFR_SIZE * sizeof (uint32);
		stream->stream_len_left -= BITSTREAM_BFR_SIZE * (int)sizeof (uint32);
	}	

#if defined(CHIP_ENDIAN_LITTLE)
	{
		uint8 * pCharTmp;
		
		pCharTmp = (uint8 *)stream->rdptr;
		
		((uint8 *)(&stream->bufa)) [0] = pCharTmp[3];
		((uint8 *)(&stream->bufa)) [1] = pCharTmp[2];
		((uint8 *)(&stream->bufa)) [2] = pCharTmp[1];
		((uint8 *)(&stream->bufa)) [3] = pCharTmp[0];
		
		((uint8 *)(&stream->bufb)) [0] = pCharTmp[7];
		((uint8 *)(&stream->bufb)) [1] = pCharTmp[6];
		((uint8 *)(&stream->bufb)) [2] = pCharTmp[5];
		((uint8 *)(&stream->bufb)) [3] = pCharTmp[4];
		
		stream->rdptr += 2;
	}
#endif

	stream->bitsLeft = 32;	
}

PUBLIC MMDecRet H264Dec_InitBitstream(void *pOneFrameBitstream, int32 length)
{
	uint32 flushBytes;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;

	SCI_TRACE_LOW("H264Dec_InitBitstream: bit addr %0x, len: %d\n", pOneFrameBitstream, length);

//	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_30|V_BIT_29|V_BIT_28, 0, TIME_OUT_CLK, "BSM_DEBUG: polling bsm is in idle status");
	VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 28, 7, 0, "BSM_DEBUG: polling bsm is in idle status");
	VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | ((1<<7) | BSM_DEBUG_WOFF));

	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+ VSP_BSM_RST_OFF, 1, "reset bsm module");
	VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (1<<24) | VSP_BSM_RST_WOFF);

	if(!g_firstBsm_init_h264)
	{
		VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (1<<2) | (1<<1), "BSM_CFG2: clear bsm and clear counter");
		VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);
	}
	g_firstBsm_init_h264 = FALSE;

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (0<<31)|(g_stream_offset>>2), "BSM_CFG1: configure the bitstream address, disable DE-STUFFING");
	flushBytes = g_stream_offset & 0x3;
#else //actual envioronment
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (1<<31), "BSM_CFG1: configure the bitstream address, disable DE-STUFFING");
	flushBytes = (uint32)pOneFrameBitstream & 0x3;
	#if 0 //xweiluo@20110520, removed it due to sw vld.
	g_nalu_ptr->len += flushBytes; //MUST!! for more_rbsp_data()
	#endif
#endif

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | 0x3ffff, "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#else
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | (((length+flushBytes+800)>>2)&0x3ffff), "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#endif
	
	/*polling bsm status*/
//	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");
	VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 31, 1, 1, "BSM_DEBUG: polling bsm status");
	VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (3<<24) | (((1<<7)|BSM_DEBUG_WOFF)<<16) | (BSM_CFG0_WOFF<<8) | BSM_CFG1_WOFF);

#if _CMODEL_
	g_bs_pingpang_bfr0 = (uint8 *)pOneFrameBitstream ;
	g_bs_pingpang_bfr_len = length+1;

	init_bsm();
#endif //_CMODEL_

	flush_unalign_bytes(flushBytes);

	return MMDEC_OK;
}

PUBLIC uint32 BitstreamShowBits(DEC_BS_T *bs_ptr, uint32 nbits)
{
	uint32 bitsLeft = bs_ptr->bitsLeft;
	uint32 ret;
	
#if defined(CHIP_ENDIAN_LITTLE)
	if (nbits <= bitsLeft)	
	{
		ret = ( bs_ptr->bufa >> (bitsLeft - nbits) ) & g_msk [nbits];	
	}
	else
	{
		int nBitsInBfrb = nbits - bitsLeft;
		
		ret = ((bs_ptr->bufa << nBitsInBfrb) | (bs_ptr->bufb >> (32 - nBitsInBfrb))) & g_msk [nbits];
	}
#else
	if (nbits <= bitsLeft)	
	{
		return  ((*bs_ptr->rdptr) >> (bitsLeft - nbits)) & g_msk [nbits];
	}
	else
	{
		nbits = nbits - bitsLeft;
		return (((*bs_ptr->rdptr) & g_msk [bitsLeft]) << nbits) | ((*(bs_ptr->rdptr+1)) >> (32 - nbits));
	}
#endif

	
	return ret;
}

PUBLIC void BitstreamFlushBits(DEC_BS_T *bs_ptr, uint32 nbits)
{
	bs_ptr->bitcnt += nbits;
	if (nbits < bs_ptr->bitsLeft)
	{		
		bs_ptr->bitsLeft -= nbits;	
	}
	else
	{
		bs_ptr->bitsLeft += 32 - nbits;
		
#if defined(CHIP_ENDIAN_LITTLE)
		bs_ptr->bufa = bs_ptr->bufb;
#else
		bs_ptr->rdptr++;
#endif
		
		if (bs_ptr->rdptr >=  bs_ptr->rdbfr + BITSTREAM_BFR_SIZE)
		{
			uint32 * ptr;			

#if defined(CHIP_ENDIAN_LITTLE)
			ptr = bs_ptr->rdbfr;		
#else
			bs_ptr->rdbfr[0] = *bs_ptr->rdptr;
			ptr = bs_ptr->rdbfr + 1;
#endif
			
			if (BITSTREAM_BFR_SIZE * sizeof (int) > bs_ptr->stream_len_left)
			{
				if (bs_ptr->stream_len_left != 0)
				{
					memcpy (ptr, bs_ptr->p_stream, bs_ptr->stream_len_left);
					bs_ptr->stream_len_left = 0;
				}
			}
			else
			{
			
				memcpy (ptr, bs_ptr->p_stream, BITSTREAM_BFR_SIZE * sizeof (int));
				bs_ptr->p_stream += BITSTREAM_BFR_SIZE * sizeof (int);
				bs_ptr->stream_len_left -= BITSTREAM_BFR_SIZE * sizeof (int);
			}		
			
			bs_ptr->rdptr = bs_ptr->rdbfr;  
		}
#if defined(CHIP_ENDIAN_LITTLE)
		/*little endian*/
		{
			uint8 *pCharTmp;
			
			pCharTmp = (uint8 *)bs_ptr->rdptr;			
			
			((uint8 *)&bs_ptr->bufb)[0] = pCharTmp[3];
			((uint8 *)&bs_ptr->bufb)[1] = pCharTmp[2];
			((uint8 *)&bs_ptr->bufb)[2] = pCharTmp[1];
			((uint8 *)&bs_ptr->bufb)[3] = pCharTmp[0];			
		}
		bs_ptr->rdptr++;
#endif
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ReadBits
 ** Description:	Read out nbits data from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 BitstreamReadBits(DEC_BS_T *bitstream_ptr, uint32 nbits)
{
	uint32 val;
	
	val = BitstreamShowBits(bitstream_ptr, nbits);
	BitstreamFlushBits(bitstream_ptr, nbits);	
	
	return val;	
}

PUBLIC uint32 inline READ_BITS1(DEC_BS_T *stream)
{
	uint32 val;

	val = ((stream->bufa >> (stream->bitsLeft - 1)) & 0x1);
	stream->bitcnt++;
	stream->bitsLeft--;

	if (!stream->bitsLeft)
	{
		stream->bitsLeft = 32;
		stream->bufa = stream->bufb;
		
		if (stream->rdptr >=  stream->rdbfr + BITSTREAM_BFR_SIZE)
		{
			uint32 * ptr;			
			ptr = stream->rdbfr;		
			
			if (BITSTREAM_BFR_SIZE * sizeof (int) > stream->stream_len_left)
			{
				if (stream->stream_len_left != 0)
				{
					memcpy (ptr, stream->p_stream, stream->stream_len_left);
					stream->stream_len_left = 0;
				}
			}
			else
			{
			
				memcpy (ptr, stream->p_stream, BITSTREAM_BFR_SIZE * sizeof (int));
				stream->p_stream += BITSTREAM_BFR_SIZE * sizeof (int);
				stream->stream_len_left -= BITSTREAM_BFR_SIZE * sizeof (int);
			}		
			
			stream->rdptr = stream->rdbfr;  
		}
		
		/*little endian*/
		{
			uint8 *pCharTmp;
			
			pCharTmp = (uint8 *)stream->rdptr;			
			
			((uint8 *)&stream->bufb)[0] = pCharTmp[3];
			((uint8 *)&stream->bufb)[1] = pCharTmp[2];
			((uint8 *)&stream->bufb)[2] = pCharTmp[1];
			((uint8 *)&stream->bufb)[3] = pCharTmp[0];			
		}
		stream->rdptr++;
	}

	return val;
}

void fillStreamBfr (DEC_BS_T * stream)
{
	uint32 * ptr = NULL;			
				
#if defined(CHIP_ENDIAN_LITTLE)
	ptr = stream->rdbfr;		
#else
	stream->rdbfr[0] = *stream->rdptr;
	ptr = stream->rdbfr + 1;
#endif
	
	if (BITSTREAM_BFR_SIZE * sizeof (int32) > stream->stream_len_left)
	{
		if (stream->stream_len_left != 0)
		{
			memcpy (ptr, stream->p_stream, stream->stream_len_left);
			stream->stream_len_left = 0;
		}
	}
	else
	{
		memcpy (ptr, stream->p_stream, BITSTREAM_BFR_SIZE * sizeof (int32));
		stream->p_stream += BITSTREAM_BFR_SIZE * sizeof (int32);
		stream->stream_len_left -= (int) (BITSTREAM_BFR_SIZE * sizeof (int32));
	}		
	
	stream->rdptr = stream->rdbfr;  				
}

PUBLIC uint32 READ_UE_V (DEC_BS_T * stream)
{
	int32 info;
	uint32 ret;
	uint32 tmp;
	int32 leading_zero = 0;

	//tmp = BitstreamShowBits (stream, 32);
	tmp = (uint32)(BITSTREAMSHOWBITS (stream, 32));

	/*find the leading zero number*/
#if _CMODEL_
	if (!tmp)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	while ( (tmp & (1 << (31 - leading_zero))) == 0 )
		leading_zero++;
#else
//	__asm {
//		clz leading_zero, tmp
//	}
	__asm__("clz %0, %1":"=&r"(leading_zero):"r"(tmp):"cc");
#endif

	//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
	if (leading_zero > 16)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	BITSTREAMFLUSHBITS(stream, leading_zero * 2 + 1);

	info = (tmp >> (32 - 2 * leading_zero -1)) & g_msk [leading_zero];
	ret = (1 << leading_zero) + info - 1;

	return ret;
}

PUBLIC int32 READ_SE_V (DEC_BS_T * stream)
{
	int32 ret;
	int32 info;
	int32 tmp;
	int32 leading_zero = 0;
	
	//tmp = BitstreamShowBits (stream, 32);
	tmp = BITSTREAMSHOWBITS(stream, 32);
	
	/*find the leading zero number*/
#if _CMODEL_
	if (!tmp)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	while ( (tmp & (1 << (31 - leading_zero))) == 0 )
		leading_zero++;
#else
//	__asm {
//		clz leading_zero, tmp
//	}
	__asm__("clz %0, %1":"=&r"(leading_zero):"r"(tmp):"cc");
#endif

	//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
	if (leading_zero > 16)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	BITSTREAMFLUSHBITS (stream, leading_zero * 2 + 1);

	info = (tmp >> (32 - 2 * leading_zero -1)) & g_msk [leading_zero];
	tmp = (1 << leading_zero) + info - 1;
	ret = (tmp + 1) / 2;

	if ( (tmp & 1) == 0 )
		ret = -ret;

	return ret;
}

PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream)
{
	uint32 tmp;
	int32 leading_zero = 0;

	tmp = SHOW_FLC (stream, 16);

	if (tmp == 0)
	{
		READ_FLC (stream, 16);
		leading_zero = 16;

		do {
			tmp = READ_BITS1 (stream);
			leading_zero++;

			if (leading_zero >= 16)
			{
				g_image_ptr->error_flag = TRUE;
				return 0;
			}
		} while(!tmp);

		leading_zero--;
		tmp = READ_FLC (stream, leading_zero);

		return tmp;
	}else
	{
		return READ_SE_V (stream);
	}
}

LOCAL void H264Dec_byte_align (void)
{
	int32 left_bits;
	int32 flush_bits;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	uint32 nDecTotalBits = stream->bitcnt;

	left_bits = 8 - (nDecTotalBits&0x7);

	flush_bits = (left_bits == 8)?8: left_bits;

	READ_FLC(stream, flush_bits);

	return;
}

PUBLIC void H264Dec_flush_left_byte (void)
{
	int32 i;
	int32 dec_len;
	int32 left_bytes;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	uint32 nDecTotalBits = stream->bitcnt;

	H264Dec_byte_align ();

	dec_len = nDecTotalBits>>3;

	left_bytes = g_nalu_ptr->len - dec_len;

	for (i = 0; i < left_bytes; i++)
	{
		READ_FLC(stream, 8);
	}

	return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
