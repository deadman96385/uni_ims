/******************************************************************************
 ** File Name:    mp4enc_global.c											  *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

int32 g_enc_last_modula_time_base;
int32 g_enc_tr;
BOOLEAN g_enc_is_prev_frame_encoded_success;
int32 g_re_enc_frame_number;

VOL_MODE_T *g_enc_vol_mode_ptr;
ENC_VOP_MODE_T *g_enc_vop_mode_ptr;

uint8 *g_enc_yuv_src_frame[3];   // current source frame

uint32 *g_vlc_hw_ptr;

VOL_MODE_T *Mp4Enc_GetVolmode(void)
{
	return g_enc_vol_mode_ptr;
}

void Mp4Enc_SetVolmode(VOL_MODE_T *vop_mode_ptr)
{
	g_enc_vol_mode_ptr = vop_mode_ptr;
}

ENC_VOP_MODE_T *Mp4Enc_GetVopmode(void)
{
	return g_enc_vop_mode_ptr;
}

void Mp4Enc_SetVopmode(ENC_VOP_MODE_T *vop_mode_ptr)
{
	g_enc_vop_mode_ptr = vop_mode_ptr;
}

//rate control
RCMode	g_stat_rc;		// Rate control mode status
int		g_stat;
RateCtrlPara g_rc_par;

#if defined(SIM_IN_WIN)
FILE *	g_rgstat_fp;
FILE *	g_buf_full_pf;
FILE *  g_psnr_pf;
#endif

//MEA_FETCH_REF g_mea_fetch;

uint32 g_nFrame_enc;

uint32 g_enc_frame_skip_number;

//for mpeg-4 time
uint32 g_enc_first_frame;
uint32 g_enc_last_frame; //encoder first and last frame number
int32 g_enc_vop_time_incr;	
uint32 g_enc_bits_modulo_base;
int32 g_enc_modulo_base_disp;		//of the most recently displayed I/Pvop
int32 g_enc_modulo_base_decd;		//of the most recently decoded I/Pvop

uint8 *g_pEnc_output_bs_buffer;  //the pointer to the output encoded bistream buffer.

uint16 g_enc_p_frame_count;

uint32 g_ME_SearchCount;

uint8 or1200_print;//for or1200 cmd

 ENC_ANTI_SHAKE_T g_anti_shake;


void ORSC_Init(VOP_PRED_TYPE_E frame_type)
{
	uint32 cmd;

	uint32 slice_num, slice_num_of_frame;
	uint16 slice_first_mb_x, slice_first_mb_y,  slice_last_mb_y;
	int is_last_slice = 0;
#ifdef SIM_IN_WIN
	if(g_fp_global_tv != NULL)
		fprintf(g_fp_global_tv, "//ORSC_Init\n");
#endif
	slice_num = g_enc_vop_mode_ptr->mb_y / g_enc_vop_mode_ptr->mbline_num_slice;
	slice_num_of_frame = g_enc_vop_mode_ptr->MBNumY/g_enc_vop_mode_ptr->mbline_num_slice + ((g_enc_vop_mode_ptr->MBNumY%g_enc_vop_mode_ptr->mbline_num_slice)?1:0);

	slice_first_mb_x = 0;
	slice_first_mb_y = g_enc_vop_mode_ptr->mb_y - (g_enc_vop_mode_ptr->mb_y%g_enc_vop_mode_ptr->mbline_num_slice);
	
	slice_last_mb_y = slice_first_mb_y + g_enc_vop_mode_ptr->mbline_num_slice - 1;
	slice_last_mb_y = (slice_last_mb_y>(g_enc_vop_mode_ptr->MBNumY-1))?(g_enc_vop_mode_ptr->MBNumY-1):slice_last_mb_y;

	is_last_slice = ((g_enc_vop_mode_ptr->MBNumY-g_enc_vop_mode_ptr->mb_y) <= g_enc_vop_mode_ptr->mbline_num_slice) ? 1:0;

//	OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF + 0x80, V_BIT_0, 0x00000001, "ORSC: IMCU_STS: Polling IMCU_START");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x84, 0x1, "ORSC: CLR_START: Set CLR_START=1 to clear IMCU_START");
	cmd = 0;
	cmd |= (g_enc_vol_mode_ptr->short_video_header)?STREAM_ID_H263:STREAM_ID_MPEG4 << 0;	// VSP_standard[3:0], 0x1:STREAM_ID_MPEG4
	cmd |= 1 << 4;		// Work_mode[4], 1:encode, 0:decode
	cmd |= 0 << 5;		// Manual_mode[5], 1:enable manual mode
	//OR1200_READ_REG(ORSC_SHARERAM_OFF + 0, "ORSC: VSP_MODE: Read from share ram");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, cmd, "ORSC: VSP_MODE: Set standard, work mode and manual mode");
	cmd = 0;
	cmd |= 0 << 0;	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, cmd, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
	
	cmd = 0;
	cmd |= g_enc_vop_mode_ptr->MBNumX << 0;	// MB_X_MAX[7:0]
	cmd |= g_enc_vop_mode_ptr->MBNumY << 8;	// MB_Y_MAX[15:8]
	cmd |= (g_anti_shake.enable_anti_shake?(g_anti_shake.input_width>>3):(2*g_enc_vop_mode_ptr->MBNumX)) <<16; // CUR_IMG_WIDTH[24:16]Unit 8 BYTE
	//OR1200_READ_REG(ORSC_SHARERAM_OFF + 4, "ORSC: IMG_SIZE: Read from share ram");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + IMG_SIZE_OFF, cmd, "ORSC: IMG_SIZE: Set MB_X_MAX & MB_Y_MAX");

	cmd = 0;
	cmd |= ((frame_type==IVOP) ? 0:1);// FRM_TYPE[2:0], 0:I, 1:P
	cmd |= (g_enc_vop_mode_ptr->MBNum & 0x1fff) << 3;	// Max_mb_num[15:3]
	cmd |= ((slice_num /*+ slice_num_of_frame*g_nFrame_enc*/)&0x1ff) << 16;// Slice_num[24:16]
	cmd |= ((frame_type==IVOP) ? g_enc_vop_mode_ptr->StepI:g_enc_vop_mode_ptr->StepP)<<25;// SliceQP[30:25]
	//cmd	|= (!g_enc_image_ptr->sh.i_disable_deblocking_filter_idc) << 31;	// Deblocking_eb[31]
	cmd	|= (0) << 31;	// Deblocking_eb[31]//???????????
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG0_OFF, cmd, "ORSC: VSP_CFG0");

	cmd = 0;
	cmd	|= (0 & 0xff) << 0;	// Skip_threshold[7:0]
	cmd	|= 9 << 8;	// Ime_16X16_max[11:8], less than 9
	cmd	|= 1 << 12;	// Ime_8X8_max[15:12], less than 1
	cmd	|= 511 << 16;	// Ipred_mode_cfg[24:16], IEA prediction mode setting
	cmd |= ((frame_type==IVOP) ? g_enc_vop_mode_ptr->StepI:g_enc_vop_mode_ptr->StepP)<<25; // MB_Qp[30:25]
	//cmd |= is_last_slice<<31; //1:hardware auto bytealign at the end of each slice, Only for Encode mode
	//cmd |= ((g_enc_vol_mode_ptr->short_video_header&&is_last_slice)?1:0)<<31; //1:hardware auto bytealign at the end of each slice, Only for Encode mode
	cmd |= (g_enc_vol_mode_ptr->short_video_header?(is_last_slice?1:0):1)<<31;//1:hardware auto bytealign at the end of each slice, Only for Encode mode
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG1_OFF, cmd, "ORSC: VSP_CFG1");

	cmd = 0;
	cmd	|= slice_last_mb_y << 0;	// last_mb_y[6:0]
	cmd	|= (slice_num*g_enc_vop_mode_ptr->mbline_num_slice) << 8;	// First_mb_y[14:8]
	//cmd	|= (g_enc_vop_mode_ptr->mbline_num_slice*slice_num*g_enc_vop_mode_ptr->MBNumX) << 16;	// first_mb_num[28:15]
	cmd |= (g_anti_shake.shift_x/2) << 16; //CUR_IMG_ST_X[25:16]Horizontal start position of cur image; unit:2 pixel
	cmd	|= 0 << 29;	// Dct_h264_scale_en[29]
	cmd	|= 1 << 30;	// MCA_rounding_type[30], For MCA only
	cmd	|= 0 << 31;	// Ppa_info_vdb_eb[31], 1: PPA need write MB info to DDR
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG2_OFF, cmd, "ORSC: VSP_CFG2");
	
	cmd = 0;
	cmd	|= (g_enc_vop_mode_ptr->short_video_header & 0x1) << 0;	// is_short_header[0]
	cmd	|= (g_enc_vol_mode_ptr->bDataPartitioning & 0x1) << 1;		// bDataPartitioning[1]
	cmd	|= (g_enc_vol_mode_ptr->bReversibleVlc & 0x1) << 2;// bReversibleVlc[2]
	cmd	|= (g_enc_vop_mode_ptr->IntraDcSwitchThr & 0x7) << 3;	// IntraDcSwitchThr[5:3]
	cmd	|= (g_enc_vop_mode_ptr->mvInfoForward.FCode & 0x7) << 6;	// Vop_fcode_forward[8:6]
	cmd	|= 0 << 9;	// Vop_fcode_backward[11:9]
	cmd	|= 0 << 12;	// For MCA[12]
	cmd |= 0 << 18; // Reserved [31:18]
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG3_OFF, cmd, "ORSC: VSP_CFG3");

	//cmd	= (uint32)g_enc_vop_mode_ptr->pOneFrameBitstream;	// Bsm_buf0_frm_addr[31:0]
	//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + BSM0_FRM_ADDR_OFF, cmd, "ORSC: BSM0_FRM_ADDR");

	//cmd	= 0x04480000;	// Bsm_buf1_frm_addr[31:0]
	//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + BSM1_FRM_ADDR_OFF, cmd, "ORSC: BSM1_FRM_ADDR");
	
#ifdef SIM_IN_WIN
	// 0x200000		Current Y
	// 0x2400000	Current UV
	// 0x400000		Reconstruct Y
	// 0x2500000	Reconstruct UV
	// 0x600000		Reference Y
	// 0x2600000	Reference UV
	if (enable_anti_shake)
	{
		cmd	= (0x200000 + (shift_y*input_width + shift_x))>>3;	// Frm_addr8[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x20, cmd, "ORSC: Frm_addr8: Start address of current frame Y");
		cmd	= (0x2400000 + (shift_y*input_width/2 + shift_x))>>3; // Frm_addr9[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x24, cmd, "ORSC: Frm_addr9: Start address of current frame UV");
	}
	else
	{
		cmd	= 0x200000>>3;	// Frm_addr8[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x20, cmd, "ORSC: Frm_addr8: Start address of current frame Y");
		cmd	= 0x2400000>>3; // Frm_addr9[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x24, cmd, "ORSC: Frm_addr9: Start address of current frame UV");
	}
	
	if((g_nFrame_enc%2) == 0)
	{
		cmd	= 0x400000>>3;	// Frm_addr0[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
		cmd	= 0x2500000>>3; // Frm_addr1[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
		cmd	= 0x600000>>3;	// Frm_addr8[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y");
		cmd	= 0x2600000>>3; // Frm_addr9[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV");
	}
	else
	{
		cmd	= 0x600000>>3;	// Frm_addr0[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
		cmd	= 0x2600000>>3; // Frm_addr1[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
		cmd	= 0x400000>>3;	// Frm_addr8[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y");
		cmd	= 0x2500000>>3; // Frm_addr9[31:0]
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV");
	}
	cmd	= 0x00030000;// Frm_addr3[31:0]
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xC, cmd, "ORSC: Frm_addr3: Start address of VLC table");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x34, 0x80, "ORSC: VSP_SIZE_SET: VLC_table_size");
#else
	if (g_anti_shake.enable_anti_shake)
	{
		g_enc_vop_mode_ptr->pYUVSrcFrame->imgY += (g_anti_shake.shift_y*g_anti_shake.input_width + g_anti_shake.shift_x);
		g_enc_vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint32)g_enc_vop_mode_ptr->pYUVSrcFrame->imgY >> 3;
		g_enc_vop_mode_ptr->pYUVSrcFrame->imgU += (g_anti_shake.shift_y*g_anti_shake.input_width/2 + g_anti_shake.shift_x);
		g_enc_vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint32)g_enc_vop_mode_ptr->pYUVSrcFrame->imgU >> 3;
	}
	cmd = g_enc_vop_mode_ptr->pYUVSrcFrame->imgYAddr;
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x20, cmd, "ORSC: Frm_addr8: Start address of current frame Y");
	cmd = g_enc_vop_mode_ptr->pYUVSrcFrame->imgUAddr;
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x24, cmd, "ORSC: Frm_addr9: Start address of current frame UV");
	cmd	= g_enc_vop_mode_ptr->pYUVRecFrame->imgYAddr;	// Frm_addr0[31:0]
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
	cmd	= g_enc_vop_mode_ptr->pYUVRecFrame->imgUAddr; // Frm_addr1[31:0]
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
	cmd	= g_enc_vop_mode_ptr->pYUVRefFrame->imgYAddr;	// Frm_addr8[31:0]
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y");
	cmd	= g_enc_vop_mode_ptr->pYUVRefFrame->imgUAddr; // Frm_addr9[31:0]
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV");

	cmd	= Mp4ENC_GetPhyAddr(g_vlc_hw_ptr)>>3;		// Frm_addr3[31:0], VLC Table set by Fixed Table Address
	OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xC, cmd, "ORSC: Frm_addr3: Start address of VLC table");
	//	cmd = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x28,"ORSC_SHARE: VLC_TABLE_SIZE"); // VLC_table_size[11:0], unit word
	cmd = 128;
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x34, cmd, "ORSC: VSP_SIZE_SET: VLC_table_size");
#endif

	/*cmd = ;	// List0_POC[0][31:0]
	OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR + 0x0, cmd, "ORSC: List0_POC[0]");*/

	/*cmd	= 0;	// Frm_addr28[31:0], default:00
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x70, cmd, "ORSC: Frm_addr28: Data space for OPENRISC");
	cmd	= 0;	// Frm_addr29[31:0], default:00
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x74, cmd, "ORSC: Frm_addr29: Instruction space for OPENRISC");
	cmd	= 0x04400000;	// Frm_addr30[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x78, cmd, "ORSC: Frm_addr30: Bsm_buf0");
	cmd	= 0x04480000;	// Frm_addr31[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x7C, cmd, "ORSC: Frm_addr31: Bsm_buf1");*/

	/*cmd = 0;
	cmd	|= 1 << 2;	// MEA_START[2]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x30, cmd, "ORSC: VSP_START: MEA_START=1");*/
}

void BSM_Init()
{
	uint32 cmd;
	ENC_VOP_MODE_T  *pVop_mode = Mp4Enc_GetVopmode();

#ifdef SIM_IN_WIN
	cmd	= BS_START_ADDR >> 3;	// Bsm_buf0_frm_addr[31:0]
#else
	cmd	= ((uint32)g_enc_vop_mode_ptr->OneFrameBitstream_addr_phy) >> 3;	// Bsm_buf0_frm_addr[31:0]
#endif
	//cmd += (g_enc_vop_mode_ptr->stm_offset >> 3);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x60, cmd, "ORSC: BSM0_FRM_ADDR");

	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x18, V_BIT_27, 0x00000000, "ORSC: BSM_DBG0: BSM_clr enable"); //check bsm is idle
	cmd = 0x04;	// Move data remain in fifo to external memeory
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, cmd, "ORSC: BSM_OPERATE: COUNT_CLR");
	/*OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x18, V_BIT_31, 0x80000000, "ORSC: BSM_DBG0: BSM_RDY enable"); //check bsm is ready
	cmd = 0x04;	// Clear the counter of BSM
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, cmd, "ORSC: BSM_OPERATE: COUNT_CLR");*/
	cmd = 0;
	cmd |= (1 << 31);	// BUFF0_STATUS[31], 1: active
	cmd |= (0 << 30);	// BUFF1_STATUS[30]
	cmd |= (((pVop_mode->OneframeStreamLen&0xfffffffc) & 0x3FFFFFFF) << 0); // BUFFER_SIZE[29:0], unit byte//cmd = g_bsm_reg_ptr->BSM_CFG0 << 0;
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x00, cmd, "ORSC: BSM_CFG0");
	cmd = 0;
	cmd |= (0 << 31);	// DESTUFFING_EN
	cmd |= ((0 & 0x3FFFFFFF) << 0);	// OFFSET_ADDR[29:0], unit word //cmd = g_bsm_reg_ptr->BSM_CFG1 << 0;
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x04, cmd, "ORSC: BSM_CFG1");

	// Write
	/*OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	cmd = (nbits&0x3f) << 24;
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, cmd, "ORSC: BSM_OPERATE: Set OPT_BITS");
	cmd = data;
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x0C, cmd, "ORSC: BSM_WDATA");*/

/*	// UE_V
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_READY_OFF, 1, 1, TIME_OUT_CLK, "BSM_READY: polling bsm rfifo ready");
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_GLO_OPT_OFF, (0x2 << 0) , "BSM_GLO_OPT: configure ue_v() operation");

	// W Start Code
	uint32 cmd = VSP_READ_REG (VSP_BSM_REG_BASE+BSM_CFG1_OFF, "read BSM_CFG0 before modify it");
	cmd &= ~(1<<31); //disable stuffing
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, cmd, "configure bsm buffer size");

	cmd |= (1<<31); //enable stuffing
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG1_OFF, cmd, "configure bsm buffer size");

	// Slice Write finish
	// clear bsm-fifo, polling inactive status reg
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
#if _CMODEL_
	clear_bsm_fifo();
#endif
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");	
	pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;*/
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 

