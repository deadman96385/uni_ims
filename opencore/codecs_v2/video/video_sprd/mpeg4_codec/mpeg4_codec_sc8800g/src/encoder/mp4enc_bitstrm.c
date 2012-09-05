/******************************************************************************
 ** File Name:    mp4enc_bitstrm.c                                            *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
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

/*****************************************************************************
 **	Name : 			Mp4Enc_InitBitStream
 ** Description:	init bitsteam 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Enc_InitBitStream(ENC_VOP_MODE_T *pVop_mode)
{
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, ((0<<31) |pVop_mode->OneframeStreamLen & 0xFFFFF), "configure bsm buffer size");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (/*(0x003d0000)|*/(0<<31)), "configure bsm buffer address");//bitstream address for each frame

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_2, "clear bsmw bit counter"); //clear bits counter

	open_vsp_iram();
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ENC_BIT_STREAM, "encoded bitstream buffer0 address");
#else //actual envioronment

#ifdef _VSP_LINUX_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ((uint32)Mp4Enc_ExtraMem_V2Phy(pVop_mode->pOneFrameBitstream)>>8), "AHBM_FRM_ADDR_6: encoded bitstream buffer0 address");
#else
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ((uint32)pVop_mode->pOneFrameBitstream>>8), "AHBM_FRM_ADDR_6: encoded bitstream buffer0 address");
#endif

#endif
	close_vsp_iram();

#if _CMODEL_
	g_bs_pingpang_bfr0 = pVop_mode->pOneFrameBitstream;
	g_bs_pingpang_bfr_len = pVop_mode->OneframeStreamLen;

	init_bsm();
#endif //_CMODEL_
}

/*****************************************************************************
 **	Name : 			Mp4Enc_OutputBits
 ** Description:	output bitsteam 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 Mp4Enc_OutputBits(uint32 val, uint32 nbits)
{
#if _CMODEL_
	write_nbits(val, nbits, 0);
#endif //_CMODEL_

	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1, TIME_OUT_CLK, "BSM_READY: polling bsm rfifo ready");

	/*configure write data length*/
	VSP_WRITE_REG (VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits&0x3f)<<24, "BSM_CFG2: configure bsmw write data length");
	
	/*configure the write data*/
	VSP_WRITE_REG (VSP_BSM_REG_BASE+BSM_WDATA_OFF, val, "BSM_WDATA: configure the value to be written to bitstream");

 	return nbits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_ByteAlign
 ** Description:	Byte Align 
 ** Author:			Xiaowei Luo
 **	Note:           now, only support mpeg4, @2007.06.19
 *****************************************************************************/
uint32 Mp4Enc_ByteAlign(BOOLEAN is_short_header)
{
	uint32 stuffing_bits;
	uint32 bitsLeft;
	uint32 total_bits;
	uint32 NumBits = 0;
	
	total_bits = VSP_READ_REG (VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "TOTAL_BITS: read bsmw total bits");

	bitsLeft = 32 - (total_bits % 32);

	stuffing_bits = bitsLeft & 0x7;

	if(stuffing_bits != 0)
	{	
		if(is_short_header)  //h.263,lxw,@0615
       	{
	       	NumBits += Mp4Enc_OutputBits(0, stuffing_bits);
       	}else//mpeg4,old code
       	{
			NumBits += Mp4Enc_OutputBits((1 << (stuffing_bits - 1)) - 1, stuffing_bits);
		}
	}else
	{
		if(!is_short_header)
		{
			stuffing_bits = 8;
			NumBits += Mp4Enc_OutputBits(0x7f, stuffing_bits);
		}
	}

	return NumBits;	
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
