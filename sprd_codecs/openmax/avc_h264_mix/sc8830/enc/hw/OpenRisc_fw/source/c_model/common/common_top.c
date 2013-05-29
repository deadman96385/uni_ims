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
#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//void hmea_Init();

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
#if defined(MPEG4_DEC)||defined(MPEG4_ENC)||defined(H264_DEC)||defined(H264_ENC)
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

#define INIT_SUB_MODEL(module_reg, module_struct) {\
	strcpy(module_tv_file_path, test_vect_file_path);\
        assert(NULL != (tv_file = fopen(strcat(module_tv_file_path, module_file_name),"w")));\
}

VSP_DCAM_REG_T * g_dcam_reg_ptr;
VSP_GLB_REG_T  * g_glb_reg_ptr;
VSP_AHBM_REG_T * g_ahbm_reg_ptr;
VSP_VLD_REG_T  * g_vld_reg_ptr;
VSP_HVLD_REG_T * g_hvld_reg_ptr;
VSP_BSM_REG_T  * g_bsm_reg_ptr;
VSP_MBC_REG_T  * g_mbc_reg_ptr;
VSP_DBK_REG_T  * g_dbk_reg_ptr;
VSP_MEA_REG_T  * g_mea_reg_ptr;
VSP_HMEA_REG_T * g_hmea_reg_ptr; 
VSP_DCT_REG_T  * g_dct_reg_ptr;
VSP_MCA_REG_T  * g_mca_reg_ptr;
VSP_VLC_REG_T  * g_vlc_reg_ptr;
VSP_ISYN_BUF_T * g_isyn_buf_ptr;
VSP_PPA_LINE_BUF_T * g_ppal_buf_ptr;

uint8 g_block_num_in_one_mcu;

//for h.264 dec and enc
//store current mb's top and left refernce pixel; 
uint8 g_luma_top[17+4], g_luma_left[16];
uint8 g_chroma_top[2][9], g_chroma_left[2][8];
uint8 g_top_left[3];

PUBLIC void VSP_Init_CModel(void)
{
#if defined(_LIB)
	g_trace_enable_flag = 0;
	g_vector_enable_flag = 0;
#else
//	g_trace_enable_flag = 0x7ff;//TRACE_ENABLE_MEA;
//	g_vector_enable_flag = 0x7ff;//0;
#endif
	
	g_tv_cmd_num = 0;

//	InitVectorFiles();
//	InitTraceFiles();

	g_dcam_reg_ptr	= (VSP_DCAM_REG_T *)safe_malloc(sizeof(VSP_DCAM_REG_T));
	g_glb_reg_ptr	= (VSP_GLB_REG_T  *)safe_malloc(sizeof(VSP_GLB_REG_T));
	g_ahbm_reg_ptr	= (VSP_AHBM_REG_T *)safe_malloc(sizeof(VSP_AHBM_REG_T));
	g_vld_reg_ptr	= (VSP_VLD_REG_T  *)safe_malloc(sizeof(VSP_VLD_REG_T));
	g_hvld_reg_ptr  = (VSP_HVLD_REG_T *)safe_malloc(sizeof(VSP_HVLD_REG_T));
	g_bsm_reg_ptr	= (VSP_BSM_REG_T  *)safe_malloc(sizeof(VSP_BSM_REG_T));
	g_mbc_reg_ptr	= (VSP_MBC_REG_T  *)safe_malloc(sizeof(VSP_MBC_REG_T));
	g_dbk_reg_ptr	= (VSP_DBK_REG_T  *)safe_malloc(sizeof(VSP_DBK_REG_T));
	g_mea_reg_ptr	= (VSP_MEA_REG_T  *)safe_malloc(sizeof(VSP_MEA_REG_T));
	g_hmea_reg_ptr	= (VSP_HMEA_REG_T *)safe_malloc(sizeof(VSP_HMEA_REG_T));
	g_dct_reg_ptr	= (VSP_DCT_REG_T  *)safe_malloc(sizeof(VSP_DCT_REG_T));
	g_mca_reg_ptr	= (VSP_MCA_REG_T  *)safe_malloc(sizeof(VSP_MCA_REG_T));
	g_vlc_reg_ptr	= (VSP_VLC_REG_T  *)safe_malloc(sizeof(VSP_VLC_REG_T));
	g_isyn_buf_ptr	= (VSP_ISYN_BUF_T *)safe_malloc(sizeof(VSP_ISYN_BUF_T));
	g_ppal_buf_ptr	= (VSP_PPA_LINE_BUF_T *)safe_malloc(sizeof(VSP_PPA_LINE_BUF_T));

	VSP_InitBfrAddr();
	init_mbc_module();
	init_vld();
	init_vlc_module();
	init_mca();
	InitDbkTrace();
//	HdbkTestVecInit();

#if defined(MPEG4_DEC)
	Mp4Dec_VldTraceInit();
//	dbk_RegPitchInit();
#elif defined(MPEG4_ENC)
	Mp4Enc_TestVecInit();
	GenMPEG4VLCTable ();
	hmea_Init();
#elif defined(H264_DEC)
	//InitVldTrace();
	init_hvld();
#elif defined(H264_ENC)
	hmea_Init();
	memset (g_dbk_line_buf, 0x00, 1024*sizeof(uint32)); // derek 2012-11-01
#else
#endif

	//h264 ipred
	memset (g_luma_left, 0, 16);
	memset (g_chroma_left[0], 0, 8);
	memset (g_chroma_left[1], 0, 8);
	memset (g_top_left, 0, 3);
}

PUBLIC void VSP_Delete_CModel(void)
{
//	DenitVectorFiles();
//	DenitTraceFiles();

	SAFE_FREE(g_dcam_reg_ptr);
	SAFE_FREE(g_glb_reg_ptr);
	SAFE_FREE(g_ahbm_reg_ptr);
	SAFE_FREE(g_vld_reg_ptr);
	SAFE_FREE(g_bsm_reg_ptr);
	SAFE_FREE(g_mbc_reg_ptr);
	SAFE_FREE(g_dbk_reg_ptr);
	SAFE_FREE(g_mea_reg_ptr);
	SAFE_FREE(g_dct_reg_ptr);
	SAFE_FREE(g_mca_reg_ptr);
	SAFE_FREE(g_vlc_reg_ptr);
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
		if (((g_glb_reg_ptr->VSP_CFG0 >> 8)&0x7) == 0x4)
		{
			pAddr = (uint32 *)(((uint8 *)g_hvld_reg_ptr) + (reg_addr - VSP_VLD_REG_BASE));
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
		if (((g_glb_reg_ptr->VSP_CFG0 >> 8)&0x7) == 0x4)
		{
			pAddr = (uint32 *)(((uint8 *)g_hmea_reg_ptr) + (reg_addr - VSP_MEA_REG_BASE));
		}else
		{
			pAddr = (uint32 *)(((uint8 *)g_mea_reg_ptr) + (reg_addr - VSP_MEA_REG_BASE));
		}
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
	else if ((reg_addr >= VSP_MEMO5_ADDR) && (reg_addr < VSP_MEMO5_ADDR+MEMO5_ADDR_SIZE))
	{
		pAddr = (int32 *)(((uint8 *)vsp_huff_dcac_tab) + (reg_addr - VSP_MEMO5_ADDR));
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

PUBLIC void VSP_WRITE_REG(uint32 reg_addr, int32 value, char *pstring)	
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

PUBLIC uint32 VSP_READ_REG (uint32 reg_addr, int8 *pstring)
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
	OUTPUT_FW_CMD_VECTOR(0, reg_addr, value, pstring);
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
FILE *g_fw_tv_cmd;
FILE *g_fp_vld_tv_run;
FILE *g_fp_vld_tv_lev;
FILE *g_fp_isqt_tv;
FILE *g_fp_ipred_tv;
FILE *g_fp_dbk_tv;
FILE *g_fp_before_flt_tv;
FILE *g_fp_dbk_mb_tv;
FILE *g_fp_dbk_frame_tv;
FILE *g_fp_mca_tv;
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
FILE *g_fp_bsm_tv;
FILE *g_fp_rec_frm_tv;
FILE *g_fp_idct_tv;
FILE *g_fp_vld_tv;

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