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
#include "sc8800g_video_header.h"
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
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF,"read BSM_TOTAL_BITS reg ");
	return (nDecTotalBits+7)/8;
}
#endif

PUBLIC int32 SHOW_FLC(int32 nbits)
{
	uint32 val;

	//show
#if _CMODEL_
	show_nbits(nbits);
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (0<<0), "BSM_CFG2: configure show n bits");
	val = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_RDATA_OFF, "BSM_RDATA: show n bits from bitstream");

	return val;
}

PUBLIC int32 READ_FLC(int32 nbits)
{
	uint32 val;

	//show
#if _CMODEL_
	show_nbits(nbits);
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (0<<0), "BSM_CFG2: configure show n bits");
	val = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_RDATA_OFF, "BSM_RDATA: show n bits from bitstream");

	//flush
#if _CMODEL_
	flush_nbits(nbits);
#endif //_CMODEL_
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (1<<0), "BSM_CFG2: configure flush n bits");
	
	return val;
}

PUBLIC MMDecRet H264Dec_InitBitstream(void *pOneFrameBitstream, int32 length)
{
	uint32 flushBytes;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;

    if( (PNULL != pOneFrameBitstream) && (length > 0) )
    {
        //MMU_InvalideDCACHE();
#ifndef _VSP_LINUX_
        MMU_DmaCacheSync((uint32)pOneFrameBitstream, length, DMABUFFER_TO_DEVICE);
#endif
    }

	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_30|V_BIT_29|V_BIT_28, 0, TIME_OUT_CLK, "BSM_DEBUG: polling bsm is in idle status");

	VSP_WRITE_REG(VSP_GLB_REG_BASE+ VSP_BSM_RST_OFF, 1, "reset bsm module");

	if(!g_firstBsm_init_h264)
	{
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (1<<2) | (1<<1), "BSM_CFG2: clear bsm and clear counter");
	}
	g_firstBsm_init_h264 = FALSE;

	if(READ_REG_POLL(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, V_BIT_17, V_BIT_17, TIME_OUT_CLK, "DBK_DEBUG: polling slice idle (WAIT_MBC_DONE or Idle) status"))
	{
		img_ptr->error_flag |= ER_DBK_ID;
        img_ptr->return_pos |= (1<<0);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_HW_ERROR;
	}

	if(READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
   		img_ptr->error_flag |= ER_AHB_ID;
        img_ptr->return_pos |= (1<<1);
        H264Dec_get_HW_status(img_ptr);
		return MMDEC_HW_ERROR;
	}

	open_vsp_iram();
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, g_stream_offset>>2, "BSM_CFG1: configure the bitstream address");
	flushBytes = g_stream_offset & 0x3;
#else //actual envioronment

#ifdef _VSP_LINUX_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ((uint32)H264Dec_GetStreamPhyAddr(pOneFrameBitstream)>>8),"source bitstream buffer0 address");
#else
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, ((uint32)pOneFrameBitstream>>8),"source bitstream buffer0 address");	
#endif
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (((uint32)pOneFrameBitstream>>2) & 0x3f), "BSM_CFG1: configure the bitstream address");
	flushBytes = (uint32)pOneFrameBitstream & 0x3;
	g_nalu_ptr->len += (int32)flushBytes; //MUST!! for more_rbsp_data()
#endif
	close_vsp_iram();

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | 0xfffff, "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#else
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | ((((uint32)length+flushBytes+800)>>2)&0x3ffff), "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
#endif
	
	/*polling bsm status*/
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "BSM_DEBUG: polling bsm status");

#if _CMODEL_
	g_bs_pingpang_bfr0 = (uint8 *)pOneFrameBitstream ;
	g_bs_pingpang_bfr_len = length+1;

	init_bsm();
#endif //_CMODEL_

	flush_unalign_bytes(flushBytes);

	return MMDEC_OK;
}

PUBLIC uint32 H264Dec_ShowBits(uint32 nbits)
{
	uint32 val;

#if _CMODEL_
	show_nbits(nbits);
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, (nbits << 24) | (0<<0), "BSM_CFG2: configure show n bits");
	val = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_RDATA_OFF, "BSM_RDATA: show n bits from bitstream");
	
	return val;
}

PUBLIC uint32 READ_UE_V (void)
{
	uint32 val;

#if _CMODEL_
	ue_v();
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_GLO_OPT_OFF, (1 << 0) , "BSM_GLO_OPT: configure ue_v() operation");
	val = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_GLO_RESULT_OFF, "BSM_GLO_RESULT: show ue_v() result");

	return val;
}

PUBLIC int32 READ_SE_V (void)
{
	int32 val;

#if _CMODEL_
	se_v();
#endif //_CMODEL_

	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_GLO_OPT_OFF, (1 << 1) , "BSM_GLO_OPT: configure se_v() operation");
	val = (int32)VSP_READ_REG(VSP_BSM_REG_BASE+BSM_GLO_RESULT_OFF, "BSM_GLO_RESULT: show se_v() result");

	return val;
}

PUBLIC int32 H264Dec_Long_SEV (void)
{
	uint32 tmp;
	int32 leading_zero = 0;

	tmp = SHOW_FLC (16);

	if (tmp == 0)
	{
		READ_FLC (16);
		leading_zero = 16;

		do {
			tmp = READ_FLC (1);
			leading_zero++;
		} while(!tmp);

		leading_zero--;
		tmp = READ_FLC (leading_zero);

		return tmp;
	}else
	{
		return READ_UE_V ();
	}
}

LOCAL void H264Dec_byte_align (void)
{
	int32 left_bits;
	int32 flush_bits;
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");

	left_bits = 8 - (nDecTotalBits&0x7);

	flush_bits = (left_bits == 8)?8: left_bits;

	READ_FLC(flush_bits);

	return;
}

PUBLIC void H264Dec_flush_left_byte (void)
{
	int32 i;
	int32 dec_len;
	int32 left_bytes;
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");

	H264Dec_byte_align ();

	dec_len = nDecTotalBits>>3;

	left_bytes = g_nalu_ptr->len - dec_len;

	for (i = 0; i < left_bytes; i++)
	{
		READ_FLC(8);
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
