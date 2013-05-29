#include "sc6800x_video_header.h"

void SharedRAM_Init()
{
	uint32 cmd;

	//OUTPUT_FW_CMD_VECTOR(1, reg_addr, value, pstring); // 0:Read, 1:Write, 2:Read Polling
	cmd = 0;
	cmd |= 0 << 0;	// boot_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 0 << 1;	// Share_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 1 << 2;	// Glb_regs_access_sw, 0:ARM, 1:IMCU
//	OR1200_WRITE_REG(AHB_BASE_ADDR + 0, cmd, "AHB: ARM_ACCESS_CTRL: Shared_RAM = IMCU, VSP_Glb_Reg = IMCU");
	//OR1200_READ_REG(AHB_BASE_ADDR + 4, "AHB: ARM_ACCESS_STATUS: Read Settings");

	cmd = 0;
	cmd |= 0 << 0;	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, cmd, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

	cmd = 0;
	cmd |= H264 << 0;	// VSP_STAND[3:0], 0x0100:H264, 0x0010:VP8
	cmd |= 1 << 4;		// WORK_MODE[4], 0:decode, 1:encode
	cmd |= 1 << 8;		// input_buffer_update[8], for enc: YUV buffer
	cmd |= 1 << 16;		// video_buffer_malloced[16], 1:video buffer malloced for VSP
	cmd |= 0 << 29;		// not_first_reset[29]
	cmd |= 0 << 30;		// cpu_will_be_reset[30]
	cmd |= 1 << 31;		// VSP_RUN[31]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0, cmd, "ORSC_SHARE: MODE_CFG: H264 Encode");

	cmd = 0;
	cmd |= 352/*g_enc_image_ptr->frame_width_in_mbs*/ << 0;	// MB_X_MAX[11:0]
	cmd |= 288/*g_enc_image_ptr->frame_height_in_mbs*/ << 12;	// MB_Y_MAX[23:12]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x4, cmd, "AHB: IMG_SIZE: Set MB_X_MAX & MB_Y_MAX");

	cmd = INTER_MALLOC_MEM_START_ADDR;	// VSP_MEM0_ST_ADDR[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x8, cmd, "ORSC_SHARE: VSP_MEM0_ST_ADDR: Start address of code run (heap & stack ) memory space");
	cmd = TOTAL_INTER_MALLOC_SIZE;		// CODE_RUN_SIZE[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0xc, cmd, "ORSC_SHARE: CODE_RUN_SIZE: Size for code run, start from VSP_MEM0_ST_ADDR");

	cmd = EXTRA_MALLOC_MEM_START_ADDR;	// VSP_MEM1_ST_ADDR[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x10, cmd, "ORSC_SHARE: VSP_MEM1_ST_ADDR: Start address of frame and mb_info memory space");
	cmd = TOTAL_EXTRA_MALLOC_SIZE;		// VSP_MEM1_SIZE[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x14, cmd, "ORSC_SHARE: VSP_MEM1_SIZE: Size of frame and mb_info memory space(byte)");

	cmd = BS_START_ADDR;	// STREAM_BUF_ADDR[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x18, cmd, "ORSC_SHARE: STREAM_BUF_ADDR: Bit stream address. Input for decoder, output for encoder");
	cmd = BS_BUF_SIZE;		// STREAM_BUF_SIZE[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x1c, cmd, "ORSC_SHARE: STREAM_BUF_SIZE: Indicate bit stream buffer size (byte)");

	cmd = 0;				// stream_len[31:0]
//	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x20, cmd, "AHB: stream_len: Length of used size in stream buffer");

	cmd = VLC_TABLE_BFR_ADDR;	// VLC_TABLE_ST_ADDR[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x24, cmd, "ORSC_SHARE: VLC_TABLE_ST_ADDR");
	cmd = VLC_TABLE_BFR_SIZE;	// VLC_TABLE_SIZE[15:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x28, cmd, "ORSC_SHARE: VLC_TABLE_SIZE");

	cmd = FRAME_BUF_SIZE*8;	// UV_FRAME_BUF_SIZE[15:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x34, cmd, "ORSC_SHARE: UV_FRAME_BUF_SIZE");

	cmd = 0;
	cmd |= TARGET_BITRATE&0x7fffffff;	// Target_rate[30:0]
	cmd |= (1 << 31);	// RATE_CONTROL_EN[31]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x38, cmd, "ORSC_SHARE: RATE_CONTROL");

	cmd = 0;
	cmd |= 0 << 0;	// boot_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 1 << 1;	// Share_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 1 << 2;	// Glb_regs_access_sw, 0:ARM, 1:IMCU
//	OR1200_WRITE_REG(AHB_BASE_ADDR + 0, cmd, "AHB: ARM_ACCESS_CTRL: Shared_RAM = IMCU, VSP_Glb_Reg = IMCU");
	//OR1200_READ_REG(AHB_BASE_ADDR + 4, "ARM_ACCESS_STATUS: Read Settings");

	// Can clock and start be set to 1 simultaneously ?
	// Reset ?
	cmd = 0;
	cmd |= 1 << 0;		// IMCU_EB, 1: enable the clock to open-risc, posedge
	cmd |= 0 << 1;		// IMCU_WAKE, Wake up open-risc, posedge of this signal will generate IRQ to Open-RISC
//	OR1200_WRITE_REG(AHB_BASE_ADDR + 8, cmd, "AHB: MCU_CONTROL_SET: Start IMCU");
}



#ifndef SIM_IN_WIN
	#define MEA_SKIP_THRESHOLD	(1800>>3) // derek_2012-08-24, total 11-bit, but only config [10:3]
	#define MEA_SEARCH_ROUND_16			8	// 0=unlimited, or lesst than 10
	#define MEA_SEARCH_ROUND_8			3	// 0=unlimited, or lesst than 4
#endif
void ORSC_Init()
{
	uint32 cmd;
	
	//OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF + 0x80, V_BIT_0, 0x00000001, "ORSC: IMCU_STS: Polling IMCU_START");
	//OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x84, 0x1, "ORSC: CLR_START: Set CLR_START=1 to clear IMCU_START");
/*
	cmd = 0;
	cmd |= H264 << 0;	// VSP_standard[3:0], 4:H264, 2:VP8
	cmd |= 1 << 4;		// Work_mode[4], 1:encode, 0:decode
	cmd |= 0 << 5;		// Manual_mode[5], 1:enable manual mode
	cmd |= 1 << 6;		// VLD_table_mode[6], 1: software update the table, 0: hardware auto update table
	//OR1200_READ_REG(ORSC_SHARERAM_OFF + 0, "ORSC: VSP_MODE: Read from share ram");
	//OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x20, cmd, "ORSC: VSP_MODE: Set standard and work mode");

	cmd = 0;
	cmd |= 0 << 0;	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
	//OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, cmd, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
*/

	cmd = 0;
	cmd |= g_enc_image_ptr->frame_width_in_mbs << 0;	// MB_X_MAX[7:0]
	cmd |= g_enc_image_ptr->frame_height_in_mbs << 8;	// MB_Y_MAX[15:8]
//	cmd |= (g_input->ori_width >> 3) << 16;				// CUR_IMG_WIDTH[24:16], unit 8-pixels
	cmd |= (g_enc_image_ptr->width>> 3) << 16;				// CUR_IMG_WIDTH[24:16], unit 8-pixels
	//OR1200_READ_REG(ORSC_SHARERAM_OFF + 4, "ORSC: IMG_SIZE: Read from share ram");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x24, cmd, "ORSC: IMG_SIZE: Set MB_X_MAX & MB_Y_MAX & CUR_IMG_WIDTH");

	cmd = 0;
	cmd |= ((g_enc_image_ptr->pYUVSrcFrame->i_type==SLICE_TYPE_I) ? 0 :
		    (g_enc_image_ptr->pYUVSrcFrame->i_type==SLICE_TYPE_P) ? 1:2) << 0;	// FRM_TYPE[2:0], 0:I, 1:P, 2:B
	cmd	|= (g_enc_image_ptr->frame_size_in_mbs&0x1fff) << 3;	// Max_mb_num[15:3]
	/*if(g_enc_image_ptr->sh.i_first_mb != 0)
#ifdef SIM_IN_WIN
		cmd |= (((g_enc_image_ptr->mb_info+(g_enc_image_ptr->sh.i_first_mb-1))->slice_nr+1)&0x1ff) << 16; // Slice_num[24:16]
#else
		cmd |= ((g_enc_image_ptr->sh.i_first_mb / g_enc_image_ptr->slice_mb)&0x1ff) << 16; // Slice_num[24:16]
#endif
	else
		cmd	|= 0 << 16;	// Slice_num[24:16]*/
	cmd |= g_enc_image_ptr->slice_nr << 16;	// Slice_num[24:16]
	cmd	|= g_enc_image_ptr->sh.i_qp << 25;	// SliceQP[30:25]
	//cmd	|= (!g_enc_image_ptr->sh.i_disable_deblocking_filter_idc) << 31;	// Deblocking_eb[31]
	cmd	|= (1) << 31;	// Deblocking_eb[31]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x3C, cmd, "ORSC: VSP_CFG0");

	cmd = 0;
	cmd	|= (MEA_SKIP_THRESHOLD & 0xff) << 0;	// Skip_threshold[7:0]
	cmd	|= (MEA_SEARCH_ROUND_16) << 8;	// Ime_16X16_max[11:8], less than 8
	cmd	|= (MEA_SEARCH_ROUND_8) << 12;	// Ime_8X8_max[15:12], less than 3
//	cmd	|= (MEA_SEARCH_ROUND_16 - (g_nFrame_enc%3)) << 8;	// Ime_16X16_max[11:8], less than 8
//	cmd	|= (MEA_SEARCH_ROUND_8 + (g_nFrame_enc%3)) << 12;	// Ime_8X8_max[15:12], less than 3
	cmd	|= 0x1FF << 16;	// Ipred_mode_cfg[24:16], IEA prediction mode setting, all 9 modes are on
	cmd |= (g_enc_image_ptr->sh.i_qp & 0x3f) << 25; // MB_Qp[30:25]
	cmd |= 1 << 31; // Bsm_bytealign_mode[31], let PPA do stop bit and byte align
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x40, cmd, "ORSC: VSP_CFG1");

	cmd = 0;
	cmd	|= (g_enc_image_ptr->sh.i_last_mb / g_enc_image_ptr->frame_width_in_mbs) << 0;	// last_mb_y[6:0]
	cmd	|= (g_enc_image_ptr->sh.i_first_mb / g_enc_image_ptr->frame_width_in_mbs) << 8;	// First_mb_y[14:8]
	cmd	|= (g_enc_image_ptr->crop_x>>1) << 16;	// CUR_IMG_ST_X[25:16]
	cmd	|= 0 << 29;	// Dct_h264_scale_en[29]
	cmd	|= 0 << 30;	// MCA_rounding_type[30], For MCA only
	cmd	|= 0 << 31;	// Ppa_info_vdb_eb[31], 1: PPA need write MB info to DDR
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x44, cmd, "ORSC: VSP_CFG2");

	cmd = 0;
	cmd	|= (g_enc_image_ptr->sh.i_alpha_c0_offset & 0x1f) << 0;	// ALPHA_OFFSET[4:0]
	cmd	|= (g_enc_image_ptr->sh.i_beta_offset & 0x1f) << 5;		// BETA_OFFSET[9:5]
	cmd	|= g_enc_image_ptr->pps->b_constrained_intra_pred << 10;// constrained_intra_pred_flag[10]
	cmd	|= 0 << 11;	// direct_spatial_mv_pred_flag[11], used in B slice
	cmd	|= g_enc_image_ptr->sps->b_direct8x8_inference << 12;	// direct_8x8_inference_flag[12]
	cmd	|= 0 << 13;	// transform_8x8_mode_flag[13]
	cmd	|= 0 << 14;	// Entropy_coding_mode_flag[14], 0:CAVLC, 1:CABAC
	cmd |= 0 << 15; // MCA_weighted_en[15]
	cmd	|= 0 << 16;	// Weighted_pred_flag[16]
	cmd	|= 0 << 17;	// Weighted_bipred_idc[18:17]
	cmd	|= /*g_enc_image_ptr->sh.i_disable_deblocking_filter_idc*/0 << 19;	// Disable_deblocking_filter_idc[20:19]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x48, cmd, "ORSC: VSP_CFG3");

	cmd = 0;
	cmd	|= (g_enc_image_ptr->pps->i_chroma_qp_index_offset & 0x1f) << 0;	// Chroma_qp_index_offset[4:0]
	cmd	|= (0 & 0x1f) << 5;	// Second_Chroma_qp_index_offset[9:5]
	cmd	|= ((g_enc_image_ptr->pps->i_num_ref_idx_l0_active-1) & 0x1f) << 10;// Num_refidx_l0_active_minus1[14:10]
	cmd	|= ((g_enc_image_ptr->pps->i_num_ref_idx_l1_active-1) & 0x1f) << 15;// Num_refidx_l1_active_minus1[19:15]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x4C, cmd, "ORSC: VSP_CFG4");

	cmd	= g_enc_image_ptr->sh.i_poc_lsb << 0;	// Cur_poc[31:0]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x50, cmd, "ORSC: VSP_CFG5");

//	cmd	= 0x04680000;		// Bsm_buf1_frm_addr[31:0]
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x64, cmd, "ORSC: BSM1_FRM_ADDR");

	//cmd	= (g_enc_image_ptr->pYUVRefFrame->addr_idx & 0x3f);	// ADDR_IDX_CFG0[31:0]
	cmd	= (0 & 0x3f);	// ADDR_IDX_CFG0[31:0], fixed to 0
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x68, cmd, "ORSC: ADDR_IDX_CFG0");	

	cmd = g_enc_image_ptr->pYUVRefFrame->i_poc;	// List0_POC[0][31:0]
	OR1200_WRITE_REG(ORSC_PPA_SINFO_OFF + 0x0, cmd, "ORSC: List0_POC[0]");

#ifdef SIM_IN_WIN
	// 0x0200000 - 0x03FFFFF	Current Y
	// 0x2400000 - 0x24FFFFF	Current UV
	// 0x0400000 - 0x05FFFFF	Reconstruct Y
	// 0x2500000 - 0x25FFFFF	Reconstruct UV
	// 0x0600000 - 0x07FFFFF	Reference Y
	// 0x2600000 - 0x26FFFFF	Reference UV
	cmd	= 0x00200000>>3;	// Frm_addr8[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x20, cmd, "ORSC: Frm_addr8: Start address of current frame Y");
	cmd	= 0x02400000>>3;	// Frm_addr9[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x24, cmd, "ORSC: Frm_addr9: Start address of current frame UV");

	if((g_nFrame_enc%2) == 0)
	{
		cmd	= 0x0400000>>3;	// Frm_addr0[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
		cmd	= 0x2500000>>3;	// Frm_addr1[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
		cmd	= 0x0600000>>3;	// Frm_addr32[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y");
		cmd	= 0x2600000>>3;	// Frm_addr64[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV");
	}
	else
	{
		cmd	= 0x0600000>>3;	// Frm_addr0[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
		cmd	= 0x2600000>>3;	// Frm_addr1[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
		cmd	= 0x0400000>>3;	// Frm_addr32[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y");
		cmd	= 0x2500000>>3;	// Frm_addr64[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV");
	}

	cmd	= 0x0180000>>3;		// Frm_addr3[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0xC, cmd, "ORSC: Frm_addr3: Start address of VLC table");
	cmd = (0x12c & 0xfff) << 0;	// VLC_table_size[11:0], unit word
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x34, cmd, "ORSC: VSP_SIZE_SET: VLC_table_size");

	cmd	= 0x00030800;		// Frm_addr4[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x10, cmd, "ORSC: Frm_addr4: Start address of Partition info");
#else


	cmd	= g_enc_image_ptr->pYUVSrcFrame->imgYAddr;	// Frm_addr8[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x20, cmd, "ORSC: Frm_addr8: Start address of current frame Y");
	cmd	= g_enc_image_ptr->pYUVSrcFrame->imgUVAddr;	// Frm_addr9[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x24, cmd, "ORSC: Frm_addr9: Start address of current frame UV");
	cmd	= g_enc_image_ptr->pYUVRecFrame->imgYAddr;	// Frm_addr0[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
	cmd	= g_enc_image_ptr->pYUVRecFrame->imgUVAddr;	// Frm_addr1[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
	cmd	= g_enc_image_ptr->pYUVRefFrame->imgYAddr;	// Frm_addr32[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y");
	cmd	= g_enc_image_ptr->pYUVRefFrame->imgUVAddr;	// Frm_addr64[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV");

	cmd	= ((uint32)g_vlc_hw_tbl+or_addr_offset)>>3;		// Frm_addr3[31:0], VLC Table set by Fixed Table Address
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0xC, cmd, "ORSC: Frm_addr3: Start address of VLC table");
//	cmd = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x28,"ORSC_SHARE: VLC_TABLE_SIZE"); // VLC_table_size[11:0], unit word
	cmd = 300;
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x34, cmd, "ORSC: VSP_SIZE_SET: VLC_table_size");
#endif
	/*cmd	= 0;	// Frm_addr28[31:0], default:00
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x70, cmd, "ORSC: Frm_addr28: Data space for OPENRISC");
	cmd	= 0;	// Frm_addr29[31:0], default:00
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x74, cmd, "ORSC: Frm_addr29: Instruction space for OPENRISC");
	cmd	= 0x04400000;	// Frm_addr30[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x78, cmd, "ORSC: Frm_addr30: Bsm_buf0");
	cmd	= 0x04480000;	// Frm_addr31[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x7C, cmd, "ORSC: Frm_addr31: Bsm_buf1");*/

	/*cmd = 0;
	cmd	|= 1 << 0;	// VLD_TABLE_START[0]
	cmd	|= 0 << 1;	// DECODE_START[1]
	cmd	|= 1 << 2;	// ENCODE_START[2]
	cmd	|= 1 << 3;	// CACHE_INI_START[3]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x30, cmd, "ORSC: VSP_START");*/
}


void BSM_Init()
{
	uint32 cmd;

#ifdef SIM_IN_WIN
	cmd	= BS_START_ADDR >> 3;	// Bsm_buf0_frm_addr[31:0]
#else
	cmd	= ((uint32)g_enc_image_ptr->pOneFrameBitstream) >> 3;	// Bsm_buf0_frm_addr[31:0]
#endif
	cmd += (g_enc_image_ptr->stm_offset >> 3);
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x60, cmd, "ORSC: BSM0_FRM_ADDR");

	//OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, V_BIT_0, "ORSC: Polling BSM_RDY");
//	OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
	cmd = 0x04;	// Move data remain in fifo to external memeory
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, cmd, "ORSC: BSM_OPERATE: COUNT_CLR");

	cmd = 0;
	cmd |= (1 << 31);	// BUFF0_STATUS[31], 1: active
	cmd |= (0 << 30);	// BUFF1_STATUS[30]
	cmd |= (((g_enc_image_ptr->OneframeStreamLen&0xfffffffc) & 0x3FFFFFFF) << 0); // BUFFER_SIZE[29:0], unit byte//cmd = g_bsm_reg_ptr->BSM_CFG0 << 0;
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x00, cmd, "ORSC: BSM_CFG0");
	cmd = 0;
	cmd |= (0 << 31);	// DESTUFFING_EN
	cmd |= ((0 & 0x3FFFFFFF) << 0);	// OFFSET_ADDR[29:0], unit word //cmd = g_bsm_reg_ptr->BSM_CFG1 << 0;
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x04, cmd, "ORSC: BSM_CFG1");

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






