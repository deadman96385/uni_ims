/******************************************************************************
 ** File Name:    h264enc_bitstrm.c                                           *
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
#include "sc6800x_video_header.h"
 
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#ifdef SIM_IN_WIN
/*****************************************************************************
 **	Name : 			H264Enc_InitBitStream
 ** Description:	init bitsteam 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void H264Enc_InitBitStream(ENC_IMAGE_PARAMS_T *img_ptr)
{
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, ((1<<31) | (1<<30) | (img_ptr->OneframeStreamLen>>2) & 0xFFFFF), "configure bsm buffer size");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (/*(0x003d0000)|*/(1<<31) | (0 & 0xFFFFF)), "configure bsm buffer address");//bitstream address for each frame

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_2, "clear bsmw bit counter"); //clear bits counter

	open_vsp_iram();
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ENC_BIT_STREAM, "encoded bitstream buffer0 address");
#else //actual envioronment
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ((uint32)img_ptr->pOneFrameBitstream>>8), "AHBM_FRM_ADDR_6: encoded bitstream buffer0 address");

#endif
	close_vsp_iram();

#if _CMODEL_
	g_bs_pingpang_bfr0 = img_ptr->pOneFrameBitstream;
	g_bs_pingpang_bfr_len = img_ptr->OneframeStreamLen;

	init_bsm();
#endif //_CMODEL_
}

#else	// SIM_IN_WIN

int32 i_size0_255[256] = 
{
	1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};

void write_ue_v(uint32 val)
{
	int32 i_size = 0;
	
	if (val == 0)
	{
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, (1&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x0C, 1, "ORSC: BSM_WDATA");
	}else
	{
		uint32 tmp = ++val;
		
		if (tmp >= 0x00010000)
		{
			i_size += 16;
			tmp >>= 16;
		}
		
		if (tmp >= 0x100)
		{
			i_size += 8;
			tmp >>= 8;
		}
		
		i_size += i_size0_255[tmp];

		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, ((2*i_size-1)&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x0C, val, "ORSC: BSM_WDATA");
	}
}
#endif // SIM_IN_WIN

/*****************************************************************************
 **	Name : 			Mp4Enc_OutputBits
 ** Description:	output bitsteam 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint32 H264Enc_OutputBits(uint32 val, uint32 nbits)
{
#ifdef SIM_IN_WIN // #if _CMODEL_
	write_nbits(val, nbits, 0);
#ifdef TV_OUT
//	FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
#endif
	if(or1200_print)
#endif //_CMODEL_
	{
		// OpenRISC Write BSM FIFO
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, (nbits&0x3f) << 24, "ORSC: BSM_OPERATE: Set OPT_BITS");
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x0C, val, "ORSC: BSM_WDATA");
	}

#ifdef SIM_IN_WIN
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1, TIME_OUT_CLK, "BSM_READY: polling bsm rfifo ready");

	/*configure write data length*/
	VSP_WRITE_REG (VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits&0x3f)<<24, "BSM_CFG2: configure bsmw write data length");
	
	/*configure the write data*/
	VSP_WRITE_REG (VSP_BSM_REG_BASE+BSM_WDATA_OFF, val, "BSM_WDATA: configure the value to be written to bitstream");
#endif
 	return nbits;
}

/*****************************************************************************
 **	Name : 			H264Enc_ByteAlign
 ** Description:	Byte Align 
 ** Author:			Xiaowei Luo
 **	Note:           now, only support mpeg4, @2007.06.19
 *****************************************************************************/
uint32 H264Enc_ByteAlign(int32 is_align1)
{
	uint32 stuffing_bits;
	uint32 bitsLeft;
	uint32 total_bits;
	uint32 NumBits = 0;

#ifdef SIM_IN_WIN
	total_bits = VSP_READ_REG (VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "TOTAL_BITS: read bsmw total bits");
#else
	total_bits = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS");
#endif

	bitsLeft = 32 - (total_bits % 32);

	stuffing_bits = bitsLeft & 0x7;

	if(stuffing_bits != 0)
	{	
		if(is_align1)  //h.263,lxw,@0615
       	{
			NumBits += H264Enc_OutputBits((1 << (stuffing_bits - 1)) - 1, stuffing_bits);
       	}else//mpeg4,old code
       	{	       	
			NumBits += H264Enc_OutputBits(0, stuffing_bits);
		}
	}

	return NumBits;	
}

void WRITE_UE_V(uint32 val)
{

//#if _CMODEL_
	write_ue_v(val);
//#endif //_CMODEL_

#ifdef SIM_IN_WIN
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1, TIME_OUT_CLK, "BSM_READY: polling bsm rfifo ready");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_GLO_OPT_OFF, (0x2 << 0) , "BSM_GLO_OPT: configure ue_v() operation");
#endif
}

void WRITE_SE_V (int32 val)
{
	WRITE_UE_V(val <= 0 ? -val*2 : val*2-1);
}

void WRITE_TE_V (int32 x, int32 val)
{
	if (x == 1)
	{	
		H264Enc_OutputBits(1&~val, 1);
	}else if (x > 1)
	{
		WRITE_UE_V(val);
	}
}

void H264Enc_rbsp_trailing (void)
{
	H264Enc_OutputBits (1, 1);
	
	H264Enc_ByteAlign(0);
}

/* long nal start code (we always use long ones)*/
void H264Enc_write_nal_start_code (void)
{
#ifdef SIM_IN_WIN
	uint32 cmd = VSP_READ_REG (VSP_BSM_REG_BASE+BSM_CFG1_OFF, "read BSM_CFG1 before modify it");
	cmd &= ~(1<<31); //disable stuffing
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, cmd, "configure bsm buffer size");
#endif
	//write "0x00000001"
	//H264Enc_OutputBits (0x0000, 16);
	//H264Enc_OutputBits (0x0001, 16);
	H264Enc_OutputBits (0xffff, 16);// derek 2012-12-05
	H264Enc_OutputBits (0xffff, 16);// replaced with customized code, will change back when frame finished

#ifdef SIM_IN_WIN
	cmd |= (1<<31); //enable stuffing
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, cmd, "configure bsm buffer size");
#endif
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
