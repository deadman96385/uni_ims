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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
#if SIM_IN_WIN
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


//	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_30|V_BIT_29|V_BIT_28, 0, TIME_OUT_CLK, "BSM_DEBUG: polling bsm is in idle status");
	VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 28, 7, 0, "BSM_DEBUG: polling bsm is in idle status");
	VSP_WRITE_CMD_INFO((VSP_BSM << MID_SHIFT_BIT) | (1<<24) | ((1<<7) | BSM_DEBUG_WOFF));

	VSP_WRITE_REG(VSP_GLB_REG_BASE+ VSP_BSM_RST_OFF, 1, "reset bsm module");
	VSP_WRITE_CMD_INFO((VSP_GLB << MID_SHIFT_BIT) | (1<<24) | VSP_BSM_RST_WOFF);

	if(!g_firstBsm_init_h264)
	{
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (1<<2) | (1<<1), "BSM_CFG2: clear bsm and clear counter");
		VSP_WRITE_CMD_INFO((VSP_BSM << MID_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);
	}
	g_firstBsm_init_h264 = FALSE;

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (1<<31)|(g_stream_offset>>2), "BSM_CFG1: configure the bitstream address");
	flushBytes = g_stream_offset & 0x3;
#else //actual envioronment
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (1<<31)|(((uint32)pOneFrameBitstream>>2) & 0x3f), "BSM_CFG1: configure the bitstream address");
	flushBytes = (uint32)pOneFrameBitstream & 0x3;
	#if 0 //xweiluo@20110520, removed it due to sw vld.
	g_nalu_ptr->len += flushBytes; //MUST!! for more_rbsp_data()
	#endif
#endif

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | 0x3ffff, "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#else
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | (((length+flushBytes+800)>>2)&0x3ffff), "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#endif
	
	/*polling bsm status*/
//	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");
	VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 31, 1, 1, "BSM_DEBUG: polling bsm status");
	VSP_WRITE_CMD_INFO((VSP_BSM << MID_SHIFT_BIT) | (3<<24) | (((1<<7)|BSM_DEBUG_WOFF)<<16) | (BSM_CFG0_WOFF<<8) | BSM_CFG1_WOFF);

#if _CMODEL_
	g_bs_pingpang_bfr0 = (uint8 *)pOneFrameBitstream ;
	g_bs_pingpang_bfr_len = length+1;

	init_bsm();
#endif //_CMODEL_

	flush_unalign_bytes(flushBytes);

	return MMDEC_OK;
}
#endif

uint32 BitstreamReadBits (DEC_BS_T * stream, int32 nbits)
{
	uint32 temp;

	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
#if SIM_IN_WIN
	OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
	//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24)|0x1,"BSM_rd n bits");
#endif
	//l = BitstreamShowBits (stream, nbits);
	temp = BITSTREAMSHOWBITS(stream, nbits);
	BITSTREAMFLUSHBITS(stream, nbits);

	return temp;	
}	
#if SIM_IN_WIN
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
			SCI_MEMCPY(ptr, stream->p_stream, stream->stream_len_left);
			stream->stream_len_left = 0;
		}
	}
	else
	{
		SCI_MEMCPY (ptr, stream->p_stream, BITSTREAM_BFR_SIZE * sizeof (int32));
		stream->p_stream += BITSTREAM_BFR_SIZE * sizeof (int32);
		stream->stream_len_left -= (int) (BITSTREAM_BFR_SIZE * sizeof (int32));
	}		
	
	stream->rdptr = stream->rdbfr;  				
}
#endif
PUBLIC uint32 READ_UE_V (DEC_BS_T * stream)
{
	
	uint32 ret;
    uint32 tmp;
#if SIM_IN_WIN
	int32 info;
	int32 leading_zero = 0;

    OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (32<<24),"BSM_rd n bits");
	OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
	//tmp = BitstreamShowBits (stream, 32);
	tmp = (uint32)(BITSTREAMSHOWBITS (stream, 32));

	/*find the leading zero number*/

	if (!tmp)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	while ( (tmp & (1 << (31 - leading_zero))) == 0 )
		leading_zero++;
//#else
//	__asm {
//		clz leading_zero, tmp
//	}
//#endif

	//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
	if (leading_zero > 16)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}

	info = (tmp >> (32 - 2 * leading_zero -1)) & g_msk [leading_zero];

	ret = (1 << leading_zero) + info - 1;

	BITSTREAMFLUSHBITS(stream, leading_zero * 2 + 1);
#else
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF, 1,"BSM_rd_UE cmd");	
	tmp =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF,"BSM_rd_UE check error");
    if(tmp&4)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	ret =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RDATA_OFF,"BSM_rd_UE dara");
	
#endif
	return ret;
}

PUBLIC int32 READ_SE_V (DEC_BS_T * stream)
{
	int32 ret;
	
	int32 tmp;
#if SIM_IN_WIN
	int32 info;
	int32 leading_zero = 0;
	
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (32<<24),"BSM_rd n bits");
	OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
	//tmp = BitstreamShowBits (stream, 32);
	tmp = BITSTREAMSHOWBITS(stream, 32);
//#if SIM_IN_WIN	
	/*find the leading zero number*/

	if (!tmp)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	while ( (tmp & (1 << (31 - leading_zero))) == 0 )
		leading_zero++;
//#else
//	__asm {
//		clz leading_zero, tmp
//	}
//#endif

	//must! because BITSTRM may be error and can't find the leading zero, xw@20100527
	if (leading_zero > 16)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}

	info = (tmp >> (32 - 2 * leading_zero -1)) & g_msk [leading_zero];

	tmp = (1 << leading_zero) + info - 1;

	ret = (tmp + 1) / 2;

	if ( (tmp & 1) == 0 )
		ret = -ret;

	BITSTREAMFLUSHBITS (stream, leading_zero * 2 + 1);
#else
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF, 2,"BSM_rd_SE cmd");	
	tmp =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RD_OFF,"BSM_rd_SE check error");
    if(tmp&4)
	{
		g_image_ptr->error_flag = TRUE;
		return 0;
	}
	ret =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+USEV_RDATA_OFF,"BSM_rd_SE dara");
	
#endif

	return ret;
}

PUBLIC int32 H264Dec_Long_UEV (DEC_BS_T * stream)
{
	uint32 tmp;
	int32 leading_zero = 0;
	int32 ret;
	
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
#if SIM_IN_WIN
	OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
#endif
	tmp = SHOW_FLC (stream, 16);
	
	if (tmp == 0)
	{
		READ_FLC (stream, 16);
		leading_zero = 16;
		
		do {
			tmp = READ_FLC (stream, 1);
			leading_zero++;
			
			if (leading_zero > 32)//weihu ?//>=16
			{
				g_image_ptr->error_flag = TRUE;
				return 0;
			}
		} while(!tmp);
		
		leading_zero--;
		tmp = READ_FLC (stream, leading_zero);		
		
		ret = (1 << leading_zero) + tmp - 1;
		
			
		return ret;
	}else
	{
		return READ_UE_V (stream);
	}
}

PUBLIC int32 H264Dec_Long_SEV (DEC_BS_T * stream)
{
	uint32 tmp;
	int32 leading_zero = 0;
	int32 ret;

	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
#if SIM_IN_WIN
	OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
#endif
	tmp = SHOW_FLC (stream, 16);

	if (tmp == 0)
	{
		READ_FLC (stream, 16);
		leading_zero = 16;

		do {
			tmp = READ_FLC (stream, 1);
			leading_zero++;

			if (leading_zero > 32)//weihu ?//>=16
			{
				g_image_ptr->error_flag = TRUE;
				return 0;
			}
		} while(!tmp);

		leading_zero--;
		tmp = READ_FLC (stream, leading_zero);


		tmp = (1 << leading_zero) + tmp - 1;
		
		ret = (tmp + 1) / 2;
		
		if ( (tmp & 1) == 0 )
			ret = -ret;

		return ret;
	}else
	{
		return READ_SE_V (stream);
	}
}
#ifdef PARSER_CMODEL
PUBLIC void H264Dec_byte_align (void)
#else
LOCAL void H264Dec_byte_align (void)
#endif
{
	int32 left_bits;
	int32 flush_bits;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	uint32 nDecTotalBits = stream->bitcnt;

	left_bits = 8 - (nDecTotalBits&0x7);

	flush_bits = (left_bits == 8)?0: left_bits;//注意?8:

	READ_FLC(stream, flush_bits);

	return;
}

#ifdef PARSER_CMODEL
PUBLIC uint32 H264Dec_byte_aligned (void) //same as H.264 byte_aligned function
{	
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	int32 left_bits = stream->bitsLeft;
	int32 bits_left_byte = (left_bits&0x8);

	return (bits_left_byte==0) ? TRUE : FALSE;
}
#endif

PUBLIC void H264Dec_flush_left_byte (void)
{
	int32 i;
	int32 dec_len;
	int32 left_bytes;
	uint32 nDecTotalBits;// = stream->bitcnt;
#if SIM_IN_WIN
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	H264Dec_byte_align ();//多移一个byte？

	nDecTotalBits = stream->bitcnt;
#else
	//DEC_BS_T * stream;
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 8,"BSM byte align");
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
	nDecTotalBits =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF,"BSM flushed bits cnt");
#endif
	//bytealign
	
	

	dec_len = nDecTotalBits>>3;

	left_bytes = g_nalu_ptr->len - dec_len;

	for (i = 0; i < left_bytes; i++)
	{
#if SIM_IN_WIN
		READ_FLC(stream, 8);
#else
        READ_FLC(NULL, 8);
#endif
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





















