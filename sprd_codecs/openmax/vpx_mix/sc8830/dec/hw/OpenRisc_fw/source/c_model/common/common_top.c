/******************************************************************************
 ** File Name:    common_top.c                                                *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
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

void * safe_malloc(uint32 mem_size)
{
	uint8 *pMem = PNULL;

	pMem = (uint8 *)malloc(mem_size * sizeof(uint8));
	assert(pMem != PNULL);

	memset(pMem, 0, mem_size*sizeof(uint8));

	return (void *)pMem;
}

PUBLIC void AllocMCUBuf(void)
{
#if defined(MPEG4_DEC)||defined(MPEG4_ENC)||defined(H264_DEC)||defined(REAL_DEC)||defined(VP8_DEC)
#else

	#if defined(JPEG_ENC)
		JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGEncCodec();
	#elif defined(JPEG_DEC)
		JPEG_CODEC_T *jpeg_fw_codec = Get_JPEGDecCodec();
	#endif

	uint16 block_id = 0, block_num = 0;
	uint8 component_id = 0, i = 0;
	
	block_id = 0;
	for (component_id = 0; component_id < 3; component_id++)
	{
		block_num = (jpeg_fw_codec->ratio[component_id].h_ratio) * (jpeg_fw_codec->ratio[component_id].v_ratio);
		for (i=0; i<block_num; i++)
		{
			g_blocks[block_id] = g_mcu_buf + block_id * 64;
			g_org_blocks[block_id] = g_mcu_org_buf +block_id * 64;
			g_blocks_membership[block_id] = component_id;
			block_id++;
		}
	}
#endif //MPEG4_DEC
}

int g_tv_cmd_num;
int g_tv_cmd_data_num;

#define INIT_SUB_MODEL(module_reg, module_struct) {\
	strcpy(module_tv_file_path, test_vect_file_path);\
        assert(NULL != (tv_file = fopen(strcat(module_tv_file_path, module_file_name),"w")));\
}

VSP_DCAM_REG_T * g_dcam_reg_ptr;
VSP_GLB_REG_T  * g_glb_reg_ptr;
VSP_AHBM_REG_T * g_ahbm_reg_ptr;
VSP_VLD_REG_T  * g_vld_reg_ptr;
VSP_HVLD_REG_T * g_hvld_reg_ptr;
VSP_RVLD_REG_T * g_rvld_reg_ptr;
VSP_BSM_REG_T  * g_bsm_reg_ptr;
VSP_MBC_REG_T  * g_mbc_reg_ptr;
VSP_DBK_REG_T  * g_dbk_reg_ptr;
VSP_MEA_REG_T  * g_mea_reg_ptr;
VSP_DCT_REG_T  * g_dct_reg_ptr;
VSP_MCA_REG_T  * g_mca_reg_ptr;
VSP_VLC_REG_T  * g_vlc_reg_ptr;
//VSP_FH_REG_T   * g_fh_reg_ptr;
VSP_ISYN_BUF_T * g_isyn_buf_ptr;
VSP_PPA_LINE_BUF_T * g_ppal_buf_ptr;
#ifdef PARSER_CMODEL
VSP_PPA_BUF_T  * g_ppa_buf_ptr;
VSP_PAR_REG_T  * g_par_reg_ptr;
#endif

uint8 g_block_num_in_one_mcu;

PUBLIC void VSP_Init_CModel(void)
{
//#if defined(_LIB)
//	g_trace_enable_flag = 0;
//	g_vector_enable_flag = 0;
//#else
	g_trace_enable_flag = 0;//0x7ff;//TRACE_ENABLE_MEA;
	g_vector_enable_flag = 0x7ff;//0;
//#endif
	
	g_tv_cmd_num = 0;

#ifdef TV_OUT
	InitVectorFiles();
#endif
//	InitTraceFiles();

	g_dcam_reg_ptr	= (VSP_DCAM_REG_T *)safe_malloc(sizeof(VSP_DCAM_REG_T));
	g_glb_reg_ptr	= (VSP_GLB_REG_T  *)safe_malloc(sizeof(VSP_GLB_REG_T));
	g_ahbm_reg_ptr	= (VSP_AHBM_REG_T *)safe_malloc(sizeof(VSP_AHBM_REG_T));
	g_vld_reg_ptr	= (VSP_VLD_REG_T  *)safe_malloc(sizeof(VSP_VLD_REG_T));
	g_hvld_reg_ptr  = (VSP_HVLD_REG_T *)safe_malloc(sizeof(VSP_HVLD_REG_T));
	g_rvld_reg_ptr  = (VSP_RVLD_REG_T *)safe_malloc(sizeof(VSP_RVLD_REG_T));
	g_bsm_reg_ptr	= (VSP_BSM_REG_T  *)safe_malloc(sizeof(VSP_BSM_REG_T));
	g_mbc_reg_ptr	= (VSP_MBC_REG_T  *)safe_malloc(sizeof(VSP_MBC_REG_T));
	g_dbk_reg_ptr	= (VSP_DBK_REG_T  *)safe_malloc(sizeof(VSP_DBK_REG_T));
	g_mea_reg_ptr	= (VSP_MEA_REG_T  *)safe_malloc(sizeof(VSP_MEA_REG_T));
	g_dct_reg_ptr	= (VSP_DCT_REG_T  *)safe_malloc(sizeof(VSP_DCT_REG_T));
	g_mca_reg_ptr	= (VSP_MCA_REG_T  *)safe_malloc(sizeof(VSP_MCA_REG_T));
	g_vlc_reg_ptr	= (VSP_VLC_REG_T  *)safe_malloc(sizeof(VSP_VLC_REG_T));
//	g_fh_reg_ptr	= (VSP_FH_REG_T	*)safe_malloc(sizeof(VSP_FH_REG_T));
	g_isyn_buf_ptr	= (VSP_ISYN_BUF_T *)safe_malloc(sizeof(VSP_ISYN_BUF_T));
	g_ppal_buf_ptr	= (VSP_PPA_LINE_BUF_T *)safe_malloc(sizeof(VSP_PPA_LINE_BUF_T));
#ifdef PARSER_CMODEL
	g_ppa_buf_ptr	= (VSP_PPA_BUF_T  *)safe_malloc(sizeof(VSP_PPA_BUF_T));
	g_par_reg_ptr	= (VSP_PAR_REG_T  *)safe_malloc(sizeof(VSP_PAR_REG_T));
#endif

	VSP_InitBfrAddr();
	init_mbc_module();
	init_vld();
	init_vlc_module();
	init_mca();
	//InitDbkTrace();
#ifndef VP8_DEC
	HdbkTestVecInit();
	memset (g_dbk_line_buf, 0x00, 4096*sizeof(uint32));
#endif
#if defined(MPEG4_DEC)
	Mp4Dec_VldTraceInit();
//	dbk_RegPitchInit();
#elif defined(MPEG4_ENC)
	Mp4Enc_TestVecInit();
	GenMPEG4VLCTable ();
	init_mea_trace();
#elif defined(H264_DEC)
	//InitVldTrace();
	init_hvld();
#elif defined(REAL_DEC)
	RvldInit ();
#else
#endif
}

PUBLIC void VSP_Delete_CModel(void)
{
	DenitVectorFiles();
	DenitTraceFiles();

	SAFE_FREE(g_dcam_reg_ptr);
	SAFE_FREE(g_glb_reg_ptr);
	SAFE_FREE(g_ahbm_reg_ptr);
	SAFE_FREE(g_vld_reg_ptr);
	SAFE_FREE(g_hvld_reg_ptr);
	SAFE_FREE(g_rvld_reg_ptr);
	SAFE_FREE(g_bsm_reg_ptr);
	SAFE_FREE(g_mbc_reg_ptr);
	SAFE_FREE(g_dbk_reg_ptr);
	SAFE_FREE(g_mea_reg_ptr);
	SAFE_FREE(g_dct_reg_ptr);
	SAFE_FREE(g_mca_reg_ptr);
	SAFE_FREE(g_vlc_reg_ptr);
//	SAFE_FREE(g_fh_reg_ptr);
	SAFE_FREE(g_isyn_buf_ptr);
	SAFE_FREE(g_ppal_buf_ptr);
	VSP_DelBfrAddr();	
	
#if defined(MPEG4_DEC)
	Mp4Dec_VldTraceInit();
#endif
}

uint32 *get_module_reg_ptr(uint32 reg_addr)
{
	uint32 *pAddr = PNULL;

	if ((reg_addr >= VSP_GLB_REG_BASE) && (reg_addr < VSP_GLB_REG_BASE+VSP_GLB_REG_SIZE))		//global register
	{
		pAddr = (uint32 *)(((uint8 *)g_glb_reg_ptr) + (reg_addr - VSP_GLB_REG_BASE));
	}
	else if ((reg_addr >= VSP_BSM_REG_BASE) && (reg_addr < VSP_BSM_REG_BASE+VSP_BSM_REG_SIZE))	//bsm register
	{
		pAddr = (uint32 *)(((uint8 *)g_bsm_reg_ptr) + (reg_addr - VSP_BSM_REG_BASE));
	}
	else if ((reg_addr >= VSP_VLD_REG_BASE) && (reg_addr < VSP_VLD_REG_BASE+VSP_VLD_REG_SIZE))	//vld register
	{
		if (((g_glb_reg_ptr->VSP_CFG0 >> 8)&0x7) == VSP_H264)
		{
			pAddr = (uint32 *)(((uint8 *)g_hvld_reg_ptr) + (reg_addr - VSP_VLD_REG_BASE));
		}else if( (((g_glb_reg_ptr->VSP_CFG0 >> 8)&0x7) == VSP_RV8)||(((g_glb_reg_ptr->VSP_CFG0 >> 8)&0x7) == VSP_RV9))//weihu
		{
			pAddr = (uint32 *)(((uint8 *)g_rvld_reg_ptr) + (reg_addr - VSP_VLD_REG_BASE));
		}else
		{
			pAddr = (uint32 *)(((uint8 *)g_vld_reg_ptr) + (reg_addr - VSP_VLD_REG_BASE));
		}		
	}
	else if ((reg_addr >= VSP_VLC_REG_BASE) && (reg_addr < VSP_VLC_REG_BASE+VSP_VLC_REG_SIZE))	//vlc register
	{
		pAddr = (uint32 *)(((uint8 *)g_vlc_reg_ptr) + (reg_addr - VSP_VLC_REG_BASE));
	}
	else if ((reg_addr >= VSP_DCT_REG_BASE) && (reg_addr < VSP_DCT_REG_BASE+VSP_DCT_REG_SIZE))	//DCT register
	{
		pAddr = (uint32 *)(((uint8 *)g_dct_reg_ptr) + (reg_addr - VSP_DCT_REG_BASE));
	}
	else if ((reg_addr >= VSP_MCA_REG_BASE) && (reg_addr < VSP_MCA_REG_BASE+VSP_MCA_REG_SIZE))	//mca register
	{
		pAddr = (uint32 *)(((uint8 *)g_mca_reg_ptr) + (reg_addr - VSP_MCA_REG_BASE));
	}
	else if ((reg_addr >= VSP_MBC_REG_BASE) && (reg_addr < VSP_MBC_REG_BASE+VSP_MBC_REG_SIZE))//mbc register
	{
		pAddr = (uint32 *)(((uint8 *)g_mbc_reg_ptr) + (reg_addr - VSP_MBC_REG_BASE));
	}
	else if ((reg_addr >= VSP_DBK_REG_BASE) && (reg_addr < VSP_DBK_REG_BASE+VSP_DBK_REG_SIZE))//dbk register
	{
		pAddr = (uint32 *)(((uint8 *)g_dbk_reg_ptr) + (reg_addr - VSP_DBK_REG_BASE));
	}
	else if ((reg_addr >= VSP_MEA_REG_BASE) && (reg_addr < VSP_MEA_REG_BASE+VSP_MEA_REG_SIZE))//mea register
	{
		pAddr = (uint32 *)(((uint8 *)g_mea_reg_ptr) + (reg_addr - VSP_MEA_REG_BASE));
	}
	else if ((reg_addr >= VSP_DCAM_REG_BASE) && (reg_addr < VSP_DCAM_REG_BASE + VSP_DCAM_REG_SIZE))//dcam register
	{
		pAddr = (uint32 *)(((uint8 *)g_dcam_reg_ptr) + (reg_addr - VSP_DCAM_REG_BASE));
	}
	else if ((reg_addr >= VSP_AHBM_REG_BASE) && (reg_addr < VSP_AHBM_REG_BASE + VSP_AHBM_REG_SIZE))//dcam register
	{
		pAddr = (uint32 *)(((uint8 *)g_ahbm_reg_ptr) + (reg_addr - VSP_AHBM_REG_BASE));
	}
	else if ((reg_addr >= VSP_MEMO0_ADDR) && (reg_addr < VSP_MEMO0_ADDR+MEMO0_ADDR_SIZE))
	{
		pAddr = (uint32 *)(((uint8 *)vsp_quant_tab) + (reg_addr - VSP_MEMO0_ADDR));
	}
	else if ((reg_addr >= VSP_MEMO1_ADDR) && (reg_addr < VSP_MEMO1_ADDR+MEMO1_ADDR_SIZE))
	{
		PRINTF ("wrong address!\n");
		pAddr = PNULL;
	}
	else if ((reg_addr >= VSP_MEMO2_ADDR) && (reg_addr < VSP_MEMO2_ADDR+MEMO2_ADDR_SIZE))
	{
		PRINTF ("wrong address!\n");
		pAddr = PNULL;
	}
	else if ((reg_addr >= VSP_MEMO3_ADDR) && (reg_addr < VSP_MEMO3_ADDR+MEMO3_ADDR_SIZE))
	{
		PRINTF ("wrong address!\n");
		pAddr = PNULL;
	}
	else if ((reg_addr >= VSP_MEMO4_ADDR) && (reg_addr < VSP_MEMO4_ADDR+MEMO4_ADDR_SIZE))
	{
		PRINTF ("wrong address!\n");
		pAddr = PNULL;
	}
	else if ((reg_addr >= VSP_MEMO14_ADDR) && (reg_addr < VSP_MEMO14_ADDR+MEMO14_ADDR_SIZE))
	{
#if defined(REAL_DEC)
		pAddr = (int32 *)(((uint8 *)g_rvld_huff_tab) + (reg_addr - VSP_MEMO14_ADDR));
#else
		pAddr = (int32 *)(((uint8 *)vsp_huff_dcac_tab) + (reg_addr - VSP_MEMO14_ADDR));
#endif
	}
	else if ((reg_addr >= VSP_MEMO6_ADDR) && (reg_addr < VSP_MEMO6_ADDR+MEMO6_ADDR_SIZE))
	{
		pAddr = (int32 *)(((uint8 *)vsp_dct_io_1) + (reg_addr - VSP_MEMO6_ADDR));
	}
	else if ((reg_addr >= VSP_MEMO7_ADDR) && (reg_addr < VSP_MEMO7_ADDR+MEMO7_ADDR_SIZE))
	{
		pAddr = (int32 *)(((uint8 *)vsp_mbc_out_bfr) + (reg_addr - VSP_MEMO7_ADDR));
	}
	else if ((reg_addr >= VSP_MEMO8_ADDR) && (reg_addr < VSP_MEMO8_ADDR+MEMO8_ADDR_SIZE))
	{
		pAddr = (int32 *)(((uint8 *)vsp_dbk_out_bfr) + (reg_addr - VSP_MEMO8_ADDR));
	}
	else if ((reg_addr >= VSP_MEMO9_ADDR) && (reg_addr < VSP_MEMO9_ADDR+MEMO9_ADDR_SIZE))
	{
		PRINTF ("wrong address!\n");
		pAddr = PNULL;
	}else if ((reg_addr >= VSP_MEMO10_ADDR) && (reg_addr < VSP_MEMO10_ADDR+MEMO10_ADDR_SIZE))
	{
		pAddr = (int32 *)(((uint8 *)vsp_frame_addr_bfr) + (reg_addr - VSP_MEMO10_ADDR));
	}
	else if (reg_addr == VSP_RESET_ADDR)
	{
		;
	}else
	{
 		PRINTF ("wrong address!\n");
		pAddr = PNULL;
	}

	return pAddr;
}

uint32 vlc_cabac_num = 0;
PUBLIC void  write_vld_cabac_bfr(uint32 addr, int32 value, char *pstring)	
{
	if(g_vector_enable_flag&VECTOR_ENABLE_VLD)
	{
		fprintf(g_fp_vld_cabac_tbl_data, "%08x\n", value);
		vlc_cabac_num++;
	}
}

PUBLIC void vsp_write_register(uint32 reg_addr, int32 value, char *pstring)	
{	
	uint32 *pAddr = PNULL;

	if (value == 0x001cfffa)
	{
		value = value;
	}
	if (MCA_CMD_Num > 400)
	{
		value = value;
	}
	
	if((VSP_RESET_ADDR == reg_addr) ||
		(DCAM_CLOCK_EN == reg_addr))
	{
		;
	}else
	{
		pAddr = get_module_reg_ptr(reg_addr);

		if( PNULL == pAddr)
		{
			SCI_ASSERT(0);
		}

		*pAddr = value;
	}

	/*if ((reg_addr >= VSP_MCA_REG_BASE) && (reg_addr < VSP_MCA_REG_BASE+VSP_MCA_REG_SIZE))	//mca register
	{
		//mca command
		uint32 RegAddrOffset = reg_addr - VSP_MCA_REG_BASE;

		if(RegAddrOffset)
		{
			MCA_CMD_Buf[MCA_CMD_Num++] = value;
		}
	}*/

	
#if _FW_TEST_VECTOR_
// 	OUTPUT_FW_CMD_VECTOR(1, reg_addr, value, pstring);
  	fprintf(g_fw_tv_cmd_data, "%08x\n", value);   FFLUSH(g_fw_tv_cmd_data);
	if(g_vector_enable_flag&VECTOR_ENABLE_FW)
	{
		fprintf(g_fw_tv_cmd_data_notes,"%x,%08x,%08x \t  ",1, reg_addr, value);
		PRINTF_TV_NOTES(g_fw_tv_cmd_data_notes,"//%s", pstring);
		fprintf(g_fw_tv_cmd_data_notes,"\n");
		FFLUSH(g_fw_tv_cmd_data_notes);
	}
#endif
}

PUBLIC uint32 vsp_read_register (uint32 reg_addr, int8 *pString)
{
	uint32 value = 0;
	uint32 *pAddr = PNULL;
	
	if((VSP_RESET_ADDR == reg_addr) ||
		(DCAM_CLOCK_EN == reg_addr))
	{
		;
	}else
	{
		pAddr = get_module_reg_ptr(reg_addr);

		if( PNULL == pAddr)
		{
			SCI_ASSERT(0);
		}

		value = *pAddr;
	}

#if _FW_TEST_VECTOR_
// 	OUTPUT_FW_CMD_VECTOR(0, reg_addr, value, pString);
#endif //_FW_TEST_VECTOR_

	return value;
}

PUBLIC void vsp_read_reg_poll(uint32 reg_addr, uint32 shift, uint32 msk_data,uint32 msked_data, char *pstring)
{
#if _FW_TEST_VECTOR_
	uint32 value = 0x0;

	if (g_tv_cmd_data_num == 21544)
	{
		g_tv_cmd_data_num = g_tv_cmd_data_num;
	}
	
	if(g_vector_enable_flag&VECTOR_ENABLE_FW)
	{
	//	FPRINTF(g_fw_tv_cmd,"%x, %x, %x, %08x, %08x \t  //%s\n",2,addr,value,mask,exp_value, pstring);
		value = (shift<<24) | ((msk_data&0xfff)<<12) | (msked_data&0xfff);
 	 	fprintf(g_fw_tv_cmd_data, "%08x\n", value); FFLUSH(g_fw_tv_cmd_data);
 		fprintf(g_fw_tv_cmd_data_notes, "%x, %x, %08x\t  //%s\n", 2, reg_addr, value, pstring);
	}

	g_tv_cmd_data_num++;

	
#endif// _FW_TEST_VECTOR_
	return ;	
}

PUBLIC void vsp_write_cmd_info(uint32 cmd_info)
{
	if(g_vector_enable_flag&VECTOR_ENABLE_FW)
	{
		fprintf(g_fw_tv_cmd_info, "%08x\n", cmd_info);
	}
}

PUBLIC void WRITE_REG(uint32 reg_addr, int32 value, char *pstring)	
{	
	uint32 *pAddr = PNULL;
	
	if((VSP_RESET_ADDR == reg_addr) ||
		(DCAM_CLOCK_EN == reg_addr))
	{
		;
	}else
	{
		pAddr = get_module_reg_ptr(reg_addr);

		if( PNULL == pAddr)
		{
			SCI_ASSERT(0);
		}

		*pAddr = value;
	}

	if ((reg_addr >= VSP_MCA_REG_BASE) && (reg_addr < VSP_MCA_REG_BASE+VSP_MCA_REG_SIZE))	//mca register
	{
		//mca command
		uint32 RegAddrOffset = reg_addr - VSP_MCA_REG_BASE;

		if(RegAddrOffset)
		{
			MCA_CMD_Buf[MCA_CMD_Num++] = value;
		}
	}
	
#if _FW_TEST_VECTOR_
	OUTPUT_FW_CMD_VECTOR(1, reg_addr, value, pstring);
#endif
}

PUBLIC uint32 READ_REG (uint32 reg_addr, int8 *pString)
{
	uint32 value = 0;
	uint32 *pAddr = PNULL;
	
	if((VSP_RESET_ADDR == reg_addr) ||
		(DCAM_CLOCK_EN == reg_addr))
	{
		;
	}else
	{
		pAddr = get_module_reg_ptr(reg_addr);

		if( PNULL == pAddr)
		{
			SCI_ASSERT(0);
		}

		value = *pAddr;
	}

#if _FW_TEST_VECTOR_
	OUTPUT_FW_CMD_VECTOR(0, reg_addr, value, pString);
#endif //_FW_TEST_VECTOR_

	return value;
}

PUBLIC int32 READ_REG_POLL(uint32 addr, uint32 mask,uint32 exp_value, uint32 time, char *pstring)
{
#if _FW_TEST_VECTOR_
	int32 value = 0x0;
	
if(g_vector_enable_flag&VECTOR_ENABLE_FW)
	FPRINTF(g_fw_tv_cmd,"%x, %x, %x, %08x, %08x \t  //%s\n",2,addr,value,mask,exp_value, pstring);
	g_tv_cmd_num++;

	
#endif// _FW_TEST_VECTOR_
	return 0;	
}

PUBLIC int	g_stream_type;

//////////////////////////////////////////////////////////////////////////
//  TestVector FILES
//////////////////////////////////////////////////////////////////////////
FILE *g_fp_vld_cabac_tbl_data;
FILE *g_fw_tv_cmd_data_notes;
FILE *g_fw_tv_cmd_info;
FILE *g_fw_tv_cmd_data;
FILE *g_fw_tv_cmd;
FILE *g_fp_vld_tv_run;
FILE *g_fp_vld_tv_lev;
FILE *g_fp_isqt_tv;
FILE *g_fp_ipred_tv;
FILE *g_fp_dbk_tv;
FILE *g_fp_dbk_para_tv;
FILE *g_fp_before_flt_tv;
FILE *g_fp_dbk_mb_tv;
FILE *g_fp_dbk_frame_tv;
FILE *g_fp_dbk_info_tv;
FILE *g_fp_mca_hor_tv;
FILE *g_fp_mca_tv;
FILE *g_fp_mca_para_tv;
FILE *g_fp_vld_coeffPos;
FILE *g_fp_dct_tv;
FILE *g_fp_vlc_tv;
FILE *g_fp_mea_tv;
FILE *g_fp_mea_src_mb_tv;
FILE *g_fp_mea_ref_mb_tv;
FILE *g_fp_mea_src_frm_y_tv;
FILE *g_fp_mea_src_frm_u_tv;
FILE *g_fp_mea_src_frm_v_tv;
FILE *g_fp_mbc_tv;
FILE *g_fp_mbc_para_tv;
FILE *g_fp_bsm_tv;
FILE *g_fp_idct_tv;
FILE *g_fp_dct_para_tv;
FILE *g_fp_vld_tv;
FILE *g_fp_rec_frm_tv;
FILE *g_fp_ref_frm0_tv;
FILE *g_fp_ref_frm1_tv;
FILE *g_fp_ref_frm2_tv;

// Frame Header VLD
FILE *g_fp_vp8_bs_off_tv;
FILE *g_fp_vp8_fh_cfg_tv;
FILE *g_fp_vp8_ppa_cfg_tv;
FILE *g_fp_vp8_prob_tv;
FILE *g_fp_vp8_part_buf_tv;
FILE *g_fp_vp8_isyn_buf_tv;
FILE *g_fp_vp8_ppal_buf_tv;

FILE *g_fp_vp8_iq_tv;
FILE *g_fp_vp8_mca_in_luma_tv;
FILE *g_fp_vp8_mca_in_chroma_tv;

// GLB REG
FILE *g_vsp_glb_reg_fp;

//////////////////////////////////////////////////////////////////////////
//  TRACE FILES
//////////////////////////////////////////////////////////////////////////
FILE *g_fp_trace_isqt;
FILE *g_fp_trace_ipred;
FILE *g_fp_trace_dbk;
FILE *g_fp_trace_mbc;
FILE *g_ipred_log_fp;
FILE *g_fp_trace_mca;
FILE *g_fp_trace_vlc;
FILE *g_fp_trace_mea;
FILE *g_fp_trace_mea_src_mb;
FILE *g_fp_trace_mea_ref_mb;
FILE *g_fp_trace_mea_sad;
FILE *g_fp_trace_vld;//@Jesse: same with LinJiang's dctCoeff.txt
FILE *g_fp_trace_fw;
FILE *g_fp_trace_fw_bs;
FILE *g_fp_dbp;
FILE *g_pf_before_iqidct;
FILE *g_pf_after_iq;
FILE *g_pf_after_iqidct;
FILE *pFmoFile;
FILE *g_fp_trace_mb_info;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 