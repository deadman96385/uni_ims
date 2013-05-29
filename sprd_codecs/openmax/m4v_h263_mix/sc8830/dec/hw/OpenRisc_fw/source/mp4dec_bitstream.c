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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
#if SIM_IN_WIN
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
#endif

#if SIM_IN_WIN
PUBLIC uint32 Mp4Dec_ShowBits(uint32 nbits)
{
	uint32 val;


	show_nbits(nbits);
	if(OR1200_Vaild) 
	{
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
		OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
	}



	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (0<<0), "BSM_CFG2: configure show n bits");
	val = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_RDATA_OFF, "BSM_RDATA: show n bits from bitstream");
	
	return val;
}
#else
PUBLIC uint32 Mp4Dec_ShowBits(uint32 nbits)
{
		uint32 val;
		
	    OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24),"BSM_rd n bits");
		val =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_RDATA_OFF,"BSM_rd dara");
        
        	return val;
}
#endif //_CMODEL_

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
#if SIM_IN_WIN
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");
#else
    uint32 nDecTotalBits = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
#endif
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
#if SIM_IN_WIN
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");
#else
     uint32 nDecTotalBits = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
#endif
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
	#if SIM_IN_WIN
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");
#else
    uint32 nDecTotalBits = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
#endif
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

//		PRINTF("Warning! show bit length is large than 32!");
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
 
#if SIM_IN_WIN
extern int OR1200_Vaild;
#endif
PUBLIC void Mp4Dec_FlushBits(uint32 nbits)
{
#if SIM_IN_WIN
	flush_nbits(nbits);
	if(OR1200_Vaild) 
	{
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24|0x01),"Flush n bits");
	}
	
	//VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (1<<0), "BSM_CFG2: configure flush n bits");
#else
	{
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (nbits<<24|0x01),"Flush n bits");
	}
#endif

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
#if SIM_IN_WIN
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "BSM_TOTAL_BITS: read BSMR_TOTAL_BITS reg");
#else
    uint32 nDecTotalBits = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");
#endif

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
