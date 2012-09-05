/******************************************************************************
 ** File Name:    mp4dec_bitstream.c                                          *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/25/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#ifdef _VSP_LINUX_
uint32 Mp4Dec_ByteConsumed()
{
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF,"read BSM_TOTAL_BITS reg ");
	return (nDecTotalBits+7)/8;
}
#endif

PUBLIC void Mp4Dec_VerifyBitstrm(uint8 *pStream, int32 strmLen)
{
	uint8 *pStreamEnd = pStream + strmLen;
    uint8 *tempPos = pStream;
//     uint32  packetLen; 
    DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

 	if (vop_mode_ptr->video_std == MPEG4) //only verify MPEG4 bitstrm
	{
		while (tempPos < pStreamEnd)
		{
			if (tempPos[0] == 0x00 && tempPos[1] == 0x00)
			{
				if (tempPos[2] == 0x01 && tempPos[3] == 0xB6) /* MPEG4 VOP start code */
				{
					vop_mode_ptr->video_std = MPEG4;
					return;
				}
				else if ((tempPos[2] & 0xFC) == 0x80 && (tempPos[3] & 0x03)==0x02) /* H.263 PSC*/
				{
					vop_mode_ptr->video_std = ITU_H263;
					return;
				}
			}
			tempPos++;
		}

		if (tempPos == pStreamEnd)
		{
			vop_mode_ptr->error_flag = TRUE;
		}
	}	

	return;
}

PUBLIC void Mp4Dec_InitBitstream(void *pOneFrameBitstream, int32 length)
{
	uint32 flushBytes;

	if(!g_firstBsm_init)
	{
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (1<<2) | (1<<1), "BSM_CFG2: clear bsm and clear counter");
	}
	g_firstBsm_init = FALSE;

	open_vsp_iram();
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, g_stream_offset>>2, "BSM_CFG1: configure the bitstream address");
	flushBytes = g_stream_offset & 0x3;
#else //actual envioronment
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ((uint32)pOneFrameBitstream>>8), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (((uint32)pOneFrameBitstream>>2) & 0x3f), "BSM_CFG1: configure the bitstream address");
	flushBytes = (uint32)pOneFrameBitstream & 0x3;
#endif
	close_vsp_iram();

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | ((((uint32)length+flushBytes+300)>>2)&0x3ffff), "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
	
	/*polling bsm status*/
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");

	flush_unalign_bytes(flushBytes);
	
#if _CMODEL_
	g_bs_pingpang_bfr0 = pOneFrameBitstream;
	g_bs_pingpang_bfr_len = length;

	init_bsm();
#endif //_CMODEL_
}

PUBLIC uint32 Mp4Dec_ShowBits(uint32 nbits)
{
	uint32 val;

#if _CMODEL_
	show_nbits(nbits);
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (0<<0), "BSM_CFG2: configure show n bits");
	val = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_RDATA_OFF, "BSM_RDATA: show n bits from bitstream");
	
	return val;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ReadBits
 ** Description:	Read out nbits data from bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 Mp4Dec_ReadBits(uint32 nbits)
{
	uint32 val;
	
	val = Mp4Dec_ShowBits(nbits);
	Mp4Dec_FlushBits(nbits);	
	
	return val;	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ByteAlign_Mp4
 ** Description:	Byte align function when decode mp4 bitstream.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ByteAlign_Mp4(void)
{
	uint32 n_stuffed, bitsLeft;	
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");

	bitsLeft = 32 - (nDecTotalBits % 32);
	n_stuffed = bitsLeft % 8;

	if(n_stuffed == 0)
	{
		n_stuffed = 8;
	}

	Mp4Dec_FlushBits(n_stuffed);
	
	return n_stuffed;	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ByteAlign_Startcode
 ** Description:	Byte align the startcode.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ByteAlign_Startcode(void)
{
	uint32 nStuffedBits;
	uint32 nByteBits = 8;
	uint32 bitsLeft;
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");

	bitsLeft = 32 - (nDecTotalBits % 32);
	nStuffedBits = (bitsLeft%nByteBits);
	
	Mp4Dec_FlushBits(nStuffedBits);
	return nStuffedBits;	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ShowBitsByteAlign
 ** Description:	Mp4Dec_ShowBitsByteAlign function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Dec_ShowBitsByteAlign(int32 nbits)
{
	int32 nAlignBitsLeft;
	int32 nStuffBit;
	uint32 tmpVar;
	uint32 tmpLen;
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");
	uint32 bitsLeft = 32 - (nDecTotalBits % 32);

	nStuffBit = bitsLeft & 0x7;
	
	/*left byte align bits in bufa*/
	nAlignBitsLeft = (bitsLeft >> 3) << 3;

	tmpLen = bitsLeft - (uint32)nAlignBitsLeft;

	if(nStuffBit == 0)
	{
		nAlignBitsLeft -= 8;
		tmpLen += 8;
	}

	tmpLen += (uint32)nbits;

	if(tmpLen > 0x20)
	{	
		/*uint32 tmpVar1 = */Mp4Dec_ShowBits(tmpLen - 0x20);
		tmpVar = Mp4Dec_ShowBits(0x20);

		PRINTF("Warning! show bit length is large than 32!");
	}else
	{
		tmpVar = Mp4Dec_ShowBits(tmpLen);
	}
	
	return (tmpVar & ((1 << nbits) - 1));
}		

/*****************************************************************************
 **	Name : 			Mp4Dec_ShowBitsByteAlign_H263
 ** Description:	Mp4Dec_ShowBitsByteAlign_H263 function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC uint32 Mp4Dec_ShowBitsByteAlign_H263(int32 nbits)
{
	int32 nStuffBit;
	uint32 tmpVar;
	uint32 nShowBits;
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");

	nStuffBit = 8 -  (nDecTotalBits&7);   //bitsLeft & 0x7;

	if(nStuffBit == 8)
	{
		nStuffBit = 0;
	}

	nShowBits = nbits + nStuffBit;

	tmpVar = Mp4Dec_ShowBits(nShowBits);

	return (tmpVar & ((1 << nbits) - 1));
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
