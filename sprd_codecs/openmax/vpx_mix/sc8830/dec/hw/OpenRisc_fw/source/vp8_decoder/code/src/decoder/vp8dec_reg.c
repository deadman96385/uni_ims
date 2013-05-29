#include "sc8810_video_header.h"

#include "vp8_mode_count.h"
#include "vp8_entropy_mode.h"

VSP_FH_REG_T   * g_fh_reg_ptr;

// VSP INPUT OUT ADDR ?
void SharedRAM_Init()
{
	uint32 cmd;

	// may not be available when used
//	int mb_row_max = (g_glb_reg_ptr->VSP_CFG1 >> 12) & 0x1FF;
//	int mb_col_max = (g_glb_reg_ptr->VSP_CFG1 >> 0) & 0x1FF;
	
	//OUTPUT_FW_CMD_VECTOR(1, reg_addr, value, pstring); // 0:Read, 1:Write, 2:Read Polling
	cmd = 0;
	cmd |= 0 << 0;	// boot_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 0 << 8;	// Share_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 1 << 16;	// Glb_regs_access_sw, 0:ARM, 1:IMCU
//	OR1200_WRITE_REG(AHB_BASE_ADDR + 0, cmd, "AHB: ARM_ACCESS_CTRL: Shared_RAM = ARM, VSP_Glb_Reg = IMCU");
	//OR1200_READ_REG(AHB_BASE_ADDR + 4, "AHB: ARM_ACCESS_STATUS: Read Settings");
	
	cmd = 0;
	cmd |= 0 << 0;	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, cmd, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");
	
	cmd = 0;
	cmd |= 0x0010 << 0;	// VSP_STAND[3:0], 0x0100:H264, 0x0010:VP8
	cmd |= 0 << 4;		// WORK_MODE[4], 0:decode, 1:encode
	cmd |= 1 << 8;		// input_buffer_update[8], for enc: YUV buffer
	cmd |= 0 << 16;		// video_buffer_malloced[16], 1:video buffer malloced for VSP
	cmd |= 0 << 29;		// not_first_reset[29]
	cmd |= 0 << 30;		// cpu_will_be_reset[30]
	cmd |= 1 << 31;		// VSP_RUN[31]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0, cmd, "ORSC_SHARE: MODE_CFG: VP8 Decode");
	
	/*cmd = 0;
	cmd |= (mb_col_max*16) << 0;	// MB_X_MAX[11:0]
	cmd |= (mb_row_max*16) << 12;	// MB_Y_MAX[23:12]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x4, cmd, "ORSC_SHARE: IMG_SIZE: Set MB_X_MAX & MB_Y_MAX");*/
	
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
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x20, cmd, "ORSC_SHARE: stream_len: Length of used size in stream buffer");
	
	cmd = VLD_TABLE_ADDR;	// VLC_TABLE_ST_ADDR[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x24, cmd, "ORSC_SHARE: VLC_TABLE_ST_ADDR");
	cmd = 69;				// VLC_TABLE_SIZE[31:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x28, cmd, "ORSC_SHARE: VLC_TABLE_SIZE");

	cmd = FRAME_BUF_SIZE*8;	// UV_FRAME_BUF_SIZE[15:0]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x34, cmd, "ORSC_SHARE: UV_FRAME_BUF_SIZE");

	/*cmd = 0;
	cmd |= TARGET_BITRATE&0x7fffffff;	// Target_rate[30:0]
	cmd |= (1 << 31);	// RATE_CONTROL_EN[31]
	OR1200_WRITE_REG(ORSC_SHARERAM_OFF + 0x38, cmd, "ORSC_SHARE: RATE_CONTROL");*/
	
	cmd = 0;
	cmd |= 0 << 0;	// boot_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 1 << 1;	// Share_ram_access_sw, 0:ARM, 1:IMCU
	cmd |= 1 << 2;	// Glb_regs_access_sw, 0:ARM, 1:IMCU
//	OR1200_WRITE_REG(AHB_BASE_ADDR + 0, cmd, "ORSC_SHARE: ARM_ACCESS_CTRL: Shared_RAM = IMCU, VSP_Glb_Reg = IMCU");
	//OR1200_READ_REG(AHB_BASE_ADDR + 4, "ARM_ACCESS_STATUS: Read Settings");
	
	// Can clock and start be set to 1 simultaneously ?
	// Reset ?
	cmd = 0;
	cmd |= 1 << 0;		// IMCU_EB, 1: enable the clock to open-risc, posedge
	cmd |= 0 << 1;		// IMCU_WAKE, Wake up open-risc, posedge of this signal will generate IRQ to Open-RISC
//	OR1200_WRITE_REG(AHB_BASE_ADDR + 8, cmd, "ORSC_SHARE: MCU_CONTROL_SET: Start IMCU");
}


#ifdef SIM_IN_WIN
// 0x0400000 - 0x05FFFFF	Reconstruct Y
// 0x2500000 - 0x25FFFFF	Reconstruct UV
// 0x0600000 - 0x07FFFFF	Reference Y(last)
// 0x2600000 - 0x26FFFFF	Reference UV(last)
// 0x0800000 - 0x09FFFFF	Reference Y(golden)
// 0x2700000 - 0x27FFFFF	Reference UV(golden)
// 0x0A00000 - 0x0BFFFFF	Reference Y(alt_ref)
// 0x2800000 - 0x28FFFFF	Reference UV(alt_ref)
uint32 FRM_ADDR[4][2] =
{
	{0x0400000>>3, 0x2500000>>3},
	{0x0600000>>3, 0x2600000>>3},
	{0x0800000>>3, 0x2700000>>3},
	{0x0A00000>>3, 0x2800000>>3},
};
#endif

void ORSC_Init(VP8D_COMP *pbi)
{
	VP8_COMMON *const pc = & pbi->common;
    vp8_reader *const bc = & pbi->bc;
	MACROBLOCKD *const xd  = & pbi->mb;
	uint32 cmd;
	int z;
	int QIndex[4];
	int baseline_filter_level[4];
	
//	OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF + 0x80, V_BIT_0, 0x00000001, "ORSC: IMCU_STS: Polling IMCU_START");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x84, 0x1, "ORSC: CLR_START: Set CLR_START=1 to clear IMCU_START");

//	cmd = 0;
//	cmd |= 2 << 0;		// VSP_standard[3:0], 4:H264, 2:VP8
//	cmd |= 0 << 4;		// Work_mode[4], 1:encode, 0:decode
//	cmd |= 0 << 5;		// Manual_mode[5], 1:enable manual mode
//	cmd |= 1 << 6;		// VLD_table_mode[6], 1: software update the table, 0: hardware auto update table
	//OR1200_READ_REG(ORSC_SHARERAM_OFF + 0, "ORSC: VSP_MODE: Read from share ram");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x20, cmd, "ORSC: VSP_MODE: Set standard, work mode and manual mode");

	cmd = 0;
	cmd |= (pc->mb_cols&0xff) << 0;	// MB_X_MAX[7:0]
	cmd |= (pc->mb_rows&0xff) << 8;	// MB_Y_MAX[15:8]
	//OR1200_READ_REG(ORSC_SHARERAM_OFF + 4, "ORSC: IMG_SIZE: Read from share ram");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x24, cmd, "ORSC: IMG_SIZE: Set MB_X_MAX & MB_Y_MAX");

//	cmd = 0;
//	cmd |= 0 << 0;	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, cmd, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

	cmd = 0;
	cmd |= ((pc->frame_type == KEY_FRAME) ? 0 : 1) << 0;	// FRM_TYPE[2:0], 0:I, 1:P, 2:B
	cmd	|= (pc->MBs&0x1fff) << 3;	// Max_mb_num[15:3]
	cmd	|= 0 << 16;	// Slice_num[24:16]
	cmd	|= 0 << 25;	// SliceQP[30:25]
	//cmd	|= (!g_enc_image_ptr->sh.i_disable_deblocking_filter_idc) << 31;	// Deblocking_eb[31]
	cmd	|= (pc->filter_level>0) << 31;	// Deblocking_eb[31]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x3C, cmd, "ORSC: VSP_CFG0");

	cmd = 0;
	cmd	|= (0 & 0xfffff) << 0;	// Nalu_length[19:0], unit byte
	cmd	|= (0 & 0x1ff) << 20;	// Num_MB_in_GOB[28:20]
	cmd	|= (0 & 0x7) << 29;		// Num_MBline_in_GOB[31:29]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x40, cmd, "ORSC: VSP_CFG1");

	cmd = 0;
	cmd	|= (0) << 0;	// first_mb_x[6:0]
	cmd	|= (0) << 8;	// first_mb_y[14:8]
	cmd	|= (pc->use_bilinear_mc_filter) << 15;	// mca_vp8_biliner_en[15]
	cmd	|= (0) << 16;	// first_mb_num[28:16]
	cmd	|= (0) << 29;	// dct_h264_scale_en[29]
	cmd	|= (0) << 30;	// MCA_rounding_type[30], For MCA only
	cmd	|= (0) << 31;	// ppa_info_vdb_eb[31], 1: PPA need write MB info to DDR
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x44, cmd, "ORSC: VSP_CFG2");

	cmd = 0;
	cmd	|= xd->update_mb_segmentation_map << 0;	// update_mb_segment_map[0]
	cmd	|= pc->mb_no_coeff_skip << 1;			// mb_no_skip_coeff[1]
	cmd	|= (pc->ref_frame_sign_bias[GOLDEN_FRAME]) << 4;	// ref_frame_sign_bias[5:2]
	cmd	|= (pc->ref_frame_sign_bias[ALTREF_FRAME]) << 5;	// ref_frame_sign_bias[5:2]
	cmd	|= (1 << pc->multi_token_partition) << 6;	// nbr_dct_partitions[9:6]
#ifdef SIM_IN_WIN
	cmd	|= (g_fh_reg_ptr->FH_CFG11+ ((bs_start_addr-BS_START_ADDR)&0x7)) << 10; // dct_part_offset[31:10]
#else
	//cmd	|= (g_fh_reg_ptr->FH_CFG11+ ((bs_start_addr+g_stream_offset-12-pbi->source_sz)&0x7)) << 10; // dct_part_offset[31:10]
	cmd	|= (g_fh_reg_ptr->FH_CFG11) << 10; // dct_part_offset[31:10]
#endif
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x48, cmd, "ORSC: VSP_CFG3");

	cmd = 0;
	cmd	|= bc->range << 0;			// bd_range[7:0]
	cmd	|= bc->count << 8;			// bd_bitcnt[11:8]
	cmd	|= pc->filter_type << 12;	// filter_type[12]
	cmd	|= pc->sharpness_level<<13;	// sharpness_level[15:13]
	cmd	|= bc->value << 16;			// bd_value[31:16]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x4C, cmd, "ORSC: VSP_CFG4");

	cmd	= 0;
	cmd	|= ((g_fh_reg_ptr->FH_CFG8>>8) & 0xff) << 0;	// Prob_intra[7:0]
	cmd	|= ((g_fh_reg_ptr->FH_CFG8>>16) & 0xff) << 8;	// Prob_last[15:8]
	cmd	|= ((g_fh_reg_ptr->FH_CFG8>>24) & 0xff) << 16;	// Prob_gf[23:16]
	cmd	|= (g_fh_reg_ptr->FH_CFG8 & 0xff) << 24;		// Prob_skip_false[31:24]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x50, cmd, "ORSC: VSP_CFG5");

	for(z=0; z<MAX_MB_SEGMENTS; z++)	// For 4 segment_id
	{
		if (xd->segmentation_enabled)
		{
			// Abs Value
			if (xd->mb_segement_abs_delta == SEGMENT_ABSDATA)
			{
				QIndex[z] = xd->segment_feature_data[MB_LVL_ALT_Q][z];
				baseline_filter_level[z] = xd->segment_feature_data[MB_LVL_ALT_LF][z];
			}
			// Delta Value
			else
			{
				QIndex[z] = pc->base_qindex + xd->segment_feature_data[MB_LVL_ALT_Q][z];
				QIndex[z] = (QIndex[z] >= 0) ? ((QIndex[z] <= MAXQ) ? QIndex[z] : MAXQ) : 0;    // Clamp to valid range
				baseline_filter_level[z] = pc->filter_level + xd->segment_feature_data[MB_LVL_ALT_LF][z];
				baseline_filter_level[z] = (baseline_filter_level[z] >= 0) ? ((baseline_filter_level[z] <= MAX_LOOP_FILTER) ? baseline_filter_level[z] : MAX_LOOP_FILTER) : 0;  // Clamp to valid range
			}
		}
		else
		{
			QIndex[z] = pc->base_qindex;
			baseline_filter_level[z] = pc->filter_level;
		}
	}

	cmd	= 0;
	cmd	|= (xd->mode_ref_lf_delta_enabled) << 0;	// mode_ref_lf_delta_enabled[0]
	cmd	|= (baseline_filter_level[0] & 0x3f) << 1;	// filter_level_0[6:1]
	cmd	|= (baseline_filter_level[1] & 0x3f) << 7;	// filter_level_1[12:7]
	cmd	|= (baseline_filter_level[2] & 0x3f) << 13;	// filter_level_2[18:13]
	cmd	|= (baseline_filter_level[3] & 0x3f) << 19;	// filter_level_3[24:19]
	cmd	|= (pc->full_pixel) << 25;	// uv_full_pixel[25]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x54, cmd, "ORSC: VSP_CFG6");

	cmd	= 0;
	cmd	|= (xd->ref_lf_deltas[0]&0xff) << 0;	// ref_lf_delta0[7:0], ref_frame 0 1 2 3
	cmd	|= (xd->ref_lf_deltas[1]&0xff) << 8;	// ref_lf_delta1[15:8]
	cmd	|= (xd->ref_lf_deltas[2]&0xff) << 16;	// ref_lf_delta2[23:16]
	cmd	|= (xd->ref_lf_deltas[3]&0xff) << 24;	// ref_lf_delta3[31:24]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x58, cmd, "ORSC: VSP_CFG7");

	cmd	= 0;
	cmd	|= (xd->mode_lf_deltas[0]&0xff) << 0;	// mode_lf_deltas0[7:0], mbmi->mode 0 1 2 3
	cmd	|= (xd->mode_lf_deltas[1]&0xff) << 8;	// mode_lf_deltas1[15:8]
	cmd	|= (xd->mode_lf_deltas[2]&0xff) << 16;	// mode_lf_deltas2[23:16]
	cmd	|= (xd->mode_lf_deltas[3]&0xff) << 24;	// mode_lf_deltas3[31:24]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x5C, cmd, "ORSC: VSP_CFG8");

	/*cmd	= BS_START_ADDR;	// Bsm_buf0_frm_addr[31:0], 64-bit aligned ?
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x60, cmd, "ORSC: BSM0_FRM_ADDR");

	cmd	= 0x04680000;	// Bsm_buf1_frm_addr[31:0]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x64, cmd, "ORSC: BSM1_FRM_ADDR");*/

	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[0] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_0[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[0] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_0[17:9]
	cmd |= ((pc->UVdequant[ QIndex[0] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_0[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x68, cmd, "ORSC: ADDR_IDX_CFG0, QP_DC_0");
	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[0] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_0[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[0] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_0[17:9]
	cmd |= ((pc->UVdequant[ QIndex[0] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_0[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x6C, cmd, "ORSC: ADDR_IDX_CFG1, QP_AC_0");

	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[1] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_1[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[1] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_1[17:9]
	cmd |= ((pc->UVdequant[ QIndex[1] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_1[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x70, cmd, "ORSC: ADDR_IDX_CFG2, QP_DC_1");
	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[1] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_1[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[1] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_1[17:9]
	cmd |= ((pc->UVdequant[ QIndex[1] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_1[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x74, cmd, "ORSC: ADDR_IDX_CFG3, QP_AC_1");

	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[2] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_2[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[2] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_2[17:9]
	cmd |= ((pc->UVdequant[ QIndex[2] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_2[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x78, cmd, "ORSC: ADDR_IDX_CFG4, QP_DC_2");
	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[2] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_2[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[2] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_2[17:9]
	cmd |= ((pc->UVdequant[ QIndex[2] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_2[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x7C, cmd, "ORSC: ADDR_IDX_CFG5, QP_AC_2");

	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[3] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_3[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[3] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_3[17:9]
	cmd |= ((pc->UVdequant[ QIndex[3] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_3[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x84, cmd, "ORSC: VSP_CFG9, QP_DC_3");
	cmd	= 0;
	cmd |= (pc->Y1dequant[ QIndex[3] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_3[8:0]
	cmd |= ((pc->Y2dequant[ QIndex[3] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_3[17:9]
	cmd |= ((pc->UVdequant[ QIndex[3] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_3[26:18]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x88, cmd, "ORSC: VSP_CFG10, QP_AC_3");

	cmd = 0;	// List0_POC[0][31:0]
	OR1200_WRITE_REG(ORSC_PPA_SINFO_OFF + 0x0, cmd, "ORSC: List0_POC[0]");

#ifdef SIM_IN_WIN
	// 0x0200000 - 0x03FFFFF	Current Y
	// 0x2400000 - 0x24FFFFF	Current UV
	// 0x0400000 - 0x05FFFFF	Reconstruct Y
	// 0x2500000 - 0x25FFFFF	Reconstruct UV
	// 0x0600000 - 0x07FFFFF	Reference Y(last)
	// 0x2600000 - 0x26FFFFF	Reference UV(last)
	// 0x0800000 - 0x09FFFFF	Reference Y(golden)
	// 0x2700000 - 0x27FFFFF	Reference UV(golden)
	// 0x0A00000 - 0x0BFFFFF	Reference Y(alt_ref)
	// 0x2800000 - 0x28FFFFF	Reference UV(alt_ref)
	//cmd	= 0x0200000>>3;	// Frm_addr8[31:0]
	//OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x20, cmd, "ORSC: Frm_addr8: Start address of current frame Y");	// encode only
	//cmd	= 0x2400000>>3;	// Frm_addr9[31:0]
	//OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x24, cmd, "ORSC: Frm_addr9: Start address of current frame UV");

	cmd	= FRM_ADDR[pc->new_frame.addr_idx][0];	// Frm_addr0[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
	cmd	= FRM_ADDR[pc->new_frame.addr_idx][1];	// Frm_addr1[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
	if(pc->frame_type != KEY_FRAME)
	{
		cmd	= FRM_ADDR[pc->last_frame.addr_idx][0];	// Frm_addr32[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y(last)");
		cmd	= FRM_ADDR[pc->last_frame.addr_idx][1];	// Frm_addr64[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV(last)");
		cmd	= FRM_ADDR[pc->golden_frame.addr_idx][0];	// Frm_addr33[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x84, cmd, "ORSC: Frm_addr33: Start address of Reference list0 frame Y(golden)");
		cmd	= FRM_ADDR[pc->golden_frame.addr_idx][1];	// Frm_addr64[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x104, cmd, "ORSC: Frm_addr65: Start address of Reference list0 frame UV(golden)");
		cmd	= FRM_ADDR[pc->alt_ref_frame.addr_idx][0];	// Frm_addr34[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x88, cmd, "ORSC: Frm_addr34: Start address of Reference list0 frame Y(alt_ref)");
		cmd	= FRM_ADDR[pc->alt_ref_frame.addr_idx][1];	// Frm_addr64[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x108, cmd, "ORSC: Frm_addr66: Start address of Reference list0 frame UV(alt_ref)");
	}

	cmd	= 0x0180000>>3;		// Frm_addr3[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0xC, cmd, "ORSC: Frm_addr3: Start address of VLC table");
	cmd = (0x400 & 0xfff) << 0;	// VLC_table_size[11:0], unit word
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x34, cmd, "ORSC: VSP_SIZE_SET: VLC_table_size");

	cmd	= 0x00030800;		// Frm_addr4[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x10, cmd, "ORSC: Frm_addr4: Start address of Partition info");
#else
	cmd	= ((uint32)pc->new_frame.y_buffer) >> 3;	// Frm_addr0[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x0, cmd, "ORSC: Frm_addr0: Start address of reconstruct frame Y");
	cmd	= ((uint32)pc->new_frame.u_buffer) >> 3;	// Frm_addr1[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x4, cmd, "ORSC: Frm_addr1: Start address of reconstruct frame UV");
	cmd	= ((uint32)pc->FRAME_ADDR_4 + or_addr_offset) >> 3;	// Frm_addr4[31:0]
	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x10, cmd, "ORSC: Frm_addr4: Partition info for MB Header");
	if(pc->frame_type != KEY_FRAME)
	{
		cmd	= ((uint32)pc->last_frame.y_buffer) >> 3;	// Frm_addr32[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x80, cmd, "ORSC: Frm_addr32: Start address of Reference list0 frame Y(last)");
		cmd	= ((uint32)pc->last_frame.u_buffer) >> 3;	// Frm_addr64[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x100, cmd, "ORSC: Frm_addr64: Start address of Reference list0 frame UV(last)");
		cmd	= ((uint32)pc->golden_frame.y_buffer) >> 3;	// Frm_addr33[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x84, cmd, "ORSC: Frm_addr33: Start address of Reference list0 frame Y(golden)");
		cmd	= ((uint32)pc->golden_frame.u_buffer) >> 3;	// Frm_addr65[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x104, cmd, "ORSC: Frm_addr65: Start address of Reference list0 frame UV(golden)");
		cmd	= ((uint32)pc->alt_ref_frame.y_buffer) >> 3;	// Frm_addr34[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x88, cmd, "ORSC: Frm_addr34: Start address of Reference list0 frame Y(alt_ref)");
		cmd	= ((uint32)pc->alt_ref_frame.u_buffer) >> 3;	// Frm_addr66[31:0]
		OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0x108, cmd, "ORSC: Frm_addr66: Start address of Reference list0 frame UV(alt_ref)");
	}
	
//	cmd	= vld_table_addr >> 3;		// Frm_addr3[31:0]
//	OR1200_WRITE_REG(ORSC_FMADD_TBL_OFF + 0xC, cmd, "ORSC: Frm_addr3: Start address of VLC table");
//	cmd = OR1200_READ_REG(ORSC_SHARERAM_OFF+0x28,"ORSC_SHARE: VLC_TABLE_SIZE"); // VLC_table_size[11:0], unit word
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x34, cmd, "ORSC: VSP_SIZE_SET: VLC_table_size");
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
	cmd	|= 0 << 0;	// VLD_TABLE_START[0]
	cmd	|= 1 << 1;	// DECODE_START[1]
	cmd	|= 0 << 2;	// ENCODE_START[2]
	cmd	|= 1 << 3;	// CACHE_INI_START[3]
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x30, cmd, "ORSC: VSP_START");*/
}


void BSM_Init(unsigned long size)
{
	uint32 cmd;

	cmd	= BS_START_ADDR;	// Bsm_buf0_frm_addr[31:0], 64-bit aligned ?
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x60, cmd, "ORSC: BSM0_FRM_ADDR");
//	cmd	= 0x04680000;	// Bsm_buf1_frm_addr[31:0]
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x64, cmd, "ORSC: BSM1_FRM_ADDR");

	OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
	//OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	cmd = 0x06;	// Move data remain in fifo to external memeory
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, cmd, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR");
	cmd = 0;
	cmd |= (1 << 31);	// BUFF0_STATUS[31]
	cmd |= (0 << 30);	// BUFF1_STATUS[30]
	cmd |= (((g_stream_offset & 0xfffffffc) & 0x3FFFFFFF) << 0); // BUFFER_SIZE[29:0], unit byte//cmd = g_bsm_reg_ptr->BSM_CFG0 << 0;
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x00, cmd, "ORSC: BSM_CFG0");
	cmd = 0;
	cmd |= (0 << 31);	// DESTUFFING_EN
	cmd |= (((g_stream_offset-size) & 0x3FFFFFFF) << 0);	// OFFSET_ADDR[29:0], unit word //cmd = g_bsm_reg_ptr->BSM_CFG1 << 0;
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x04, cmd, "ORSC: BSM_CFG1");

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


uint32 BitstreamReadBits (uint32 nbits)
{
	uint32 temp;
	
	OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, ((nbits)&0x3f)<<24, "ORSC: BSM_OPERATE: Set OPT_BITS");
	temp = (uint32)OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x10, "ORSC: BSM_RDATA");
	OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, (((nbits)&0x3f)<<24)|0x1, "ORSC: BSM_OPERATE: BSM_FLUSH n bits");
#ifdef SIM_IN_WIN
	g_bsm_reg_ptr->TOTAL_BITS += nbits;
#endif
	return temp;	
}

uint32 BITSTREAMSHOWBYTE(vp8_reader *stream, uint32 nbytes)
{
	uint i;
	uint32 tmp = 0;;
	
	for(i=0; i<nbytes; i++)
	{
		tmp |= (*(stream->read_ptr + i) << (8*(3-i)));
	}
	return tmp;
}

void Write_tbuf_Probs(VP8D_COMP *pbi)
{
	int i, j, k;
	uint32 temp = 0;
	VP8_COMMON *const pc = & pbi->common;
	MACROBLOCKD *const xd  = & pbi->mb;
	vp8_prob *mvc = (vp8_prob *)pc->fc.mvc;
	uint32 word_addr = 0;

	temp = (xd->mb_segment_tree_probs[2]<<16) | (xd->mb_segment_tree_probs[1]<<8) | xd->mb_segment_tree_probs[0];
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");

	for(i=0; i<6; i++)
	{
		temp = 0;
		for(j=0; j<4; j++)
			temp |= ((vp8_mode_contexts[i][j]&0xff)<<(j*8));
		OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	}
	
	temp = (vp8_mbsplit_probs[2]<<16) | (vp8_mbsplit_probs[1]<<8) | vp8_mbsplit_probs[0];
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	
	for(i=0; i<5; i++)
	{
		temp = (vp8_sub_mv_ref_prob2[i][2]<<16) | (vp8_sub_mv_ref_prob2[i][1]<<8) | vp8_sub_mv_ref_prob2[i][0];
		OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	}
	
	for(i=0; i<2; i++)
	{
		mvc = (vp8_prob *)(&(pc->fc.mvc[i]));
		for(j=0; j<4; j++)
		{
			temp = (mvc[3]<<24) | (mvc[2]<<16) | (mvc[1]<<8) | mvc[0];
			OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
			mvc+=4;
		}
		temp = (mvc[2]<<16) | (mvc[1]<<8) | mvc[0];
		OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	}

	temp = (pc->kf_ymode_prob[3]<<24) | (pc->kf_ymode_prob[2]<<16) | (pc->kf_ymode_prob[1]<<8) | (pc->kf_ymode_prob[0]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	temp = (pc->fc.ymode_prob[3]<<24) | (pc->fc.ymode_prob[2]<<16) | (pc->fc.ymode_prob[1]<<8) | (pc->fc.ymode_prob[0]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	temp = (pc->kf_uv_mode_prob[2]<<16) | (pc->kf_uv_mode_prob[1]<<8) | (pc->kf_uv_mode_prob[0]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	temp = (pc->fc.uv_mode_prob[2]<<16) | (pc->fc.uv_mode_prob[1]<<8) | (pc->fc.uv_mode_prob[0]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");

	for(i=0; i<10; i++)
		for(j=0; j<10; j++)
		{
			temp = (pc->kf_bmode_prob[i][j][3]<<24) | (pc->kf_bmode_prob[i][j][2]<<16) | (pc->kf_bmode_prob[i][j][1]<<8) | (pc->kf_bmode_prob[i][j][0]);
			OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
			temp = (pc->kf_bmode_prob[i][j][7]<<24) | (pc->kf_bmode_prob[i][j][6]<<16) | (pc->kf_bmode_prob[i][j][5]<<8) | (pc->kf_bmode_prob[i][j][4]);
			OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
			temp = (pc->kf_bmode_prob[i][j][8]);
			OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
		}
	temp = (pc->fc.bmode_prob[3]<<24) | (pc->fc.bmode_prob[2]<<16) | (pc->fc.bmode_prob[1]<<8) | (pc->fc.bmode_prob[0]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	temp = (pc->fc.bmode_prob[7]<<24) | (pc->fc.bmode_prob[6]<<16) | (pc->fc.bmode_prob[5]<<8) | (pc->fc.bmode_prob[4]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
	temp = (pc->fc.bmode_prob[8]);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
		
	for(k=0; k<4; k++)
		for(i=0; i<8; i++)
			for(j=0; j<3; j++)
			{
				temp = (pc->fc.coef_probs[k][i][j][3]<<24) | (pc->fc.coef_probs[k][i][j][2]<<16) | (pc->fc.coef_probs[k][i][j][1]<<8) | (pc->fc.coef_probs[k][i][j][0]);
				OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
				temp = (pc->fc.coef_probs[k][i][j][7]<<24) | (pc->fc.coef_probs[k][i][j][6]<<16) | (pc->fc.coef_probs[k][i][j][5]<<8) | (pc->fc.coef_probs[k][i][j][4]);
				OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
				temp = (pc->fc.coef_probs[k][i][j][10]<<16) | (pc->fc.coef_probs[k][i][j][9]<<8) | (pc->fc.coef_probs[k][i][j][8]);
				OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, temp, "ORSC: Write VLC table0");
			}
				
	//FPRINTF_VLD (g_fp_vp8_prob_tv, "%08x\n", (*((uint32*)vp8d_token_extra_bits2[DCT_VAL_CATEGORY1].Probs))&0x0000ffff);
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x0000009F), "ORSC: Write VLC table0");	// Pcat1
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x000091A5), "ORSC: Write VLC table0");	// Pcat2
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x008C94AD), "ORSC: Write VLC table0");	// Pcat3
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x878C9BB0), "ORSC: Write VLC table0");	// Pcat4
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x00000000), "ORSC: Write VLC table0");	// Pcat4
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x868D9DB4), "ORSC: Write VLC table0");	// Pcat5
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x00000082), "ORSC: Write VLC table0");	// Pcat5
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0xE6F3FEFE), "ORSC: Write VLC table0");	// Pcat6
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x8C99B1C4), "ORSC: Write VLC table0");	// Pcat6
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (0x00818285), "ORSC: Write VLC table0");	// Pcat6
	OR1200_WRITE_REG(ORSC_VLC0_TBL_OFF + 4*word_addr++, (g_fh_reg_ptr->FH_CFG8), "ORSC: Write VLC table0");

	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xf8, 0xeeeeeeee, "FPGA test");
}