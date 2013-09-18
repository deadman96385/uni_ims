/******************************************************************************
 ** File Name:    vp8_init.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void vp8_create_common(VP8_COMMON *oci)
{
    vp8_default_coef_probs(oci);
    vp8_init_mbmode_probs(oci);
    vp8_default_bmode_probs(oci->fc.bmode_prob);

    oci->mb_no_coeff_skip = 1;
    oci->no_lpf = 0;
    oci->simpler_lpf = 0;
    oci->use_bilinear_mc_filter = 0;
    oci->full_pixel = 0;
    oci->multi_token_partition = ONE_PARTITION;
    oci->clr_type = REG_YUV;
    oci->clamp_type = RECON_CLAMP_REQUIRED;
    oci->buffer_count = 0;

    // Initialise reference frame sign bias structure to defaults
    vpx_memset(oci->ref_frame_sign_bias, 0, sizeof(oci->ref_frame_sign_bias));

    // Default disable buffer to buffer copying
    oci->copy_buffer_to_gf = 0;
    oci->copy_buffer_to_arf = 0;
}

void Vp8Dec_InitVSP(VPXDecObject *vo)
{
    VSP_FH_REG_T *fh_reg_ptr = vo->g_fh_reg_ptr;
    VP8_COMMON *const pc = & vo->common;
    vp8_reader *const bc = & vo->bc;
    MACROBLOCKD *const xd  = & vo->mb;
    uint32 cmd;
    int z;
    int QIndex[4];
    int baseline_filter_level[4];

    cmd = (pc->mb_cols&0xff) << 0;	// MB_X_MAX[7:0]
    cmd |= (pc->mb_rows&0xff) << 8;	// MB_Y_MAX[15:8]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + IMG_SIZE_OFF, cmd, "IMG_SIZE: Set MB_X_MAX & MB_Y_MAX");

    cmd = ((pc->frame_type == KEY_FRAME) ? 0 : 1) << 0;	// FRM_TYPE[2:0], 0:I, 1:P, 2:B
    cmd |= (pc->MBs&0x1fff) << 3;	// Max_mb_num[15:3]
    cmd |= 0 << 16;	// Slice_num[24:16]
    cmd |= 0 << 25;	// SliceQP[30:25]
    cmd |= (pc->filter_level>0) << 31;	// Deblocking_eb[31]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG0_OFF, cmd, "VSP_CFG0");

    cmd = (0 & 0xfffff) << 0;	// Nalu_length[19:0], unit byte
    cmd |= (0 & 0x1ff) << 20;	// Num_MB_in_GOB[28:20]
    cmd |= (0 & 0x7) << 29;		// Num_MBline_in_GOB[31:29]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG1_OFF, cmd, "VSP_CFG1");

    cmd = (0) << 0;	// first_mb_x[6:0]
    cmd |= (0) << 8;	// first_mb_y[14:8]
    cmd |= (pc->use_bilinear_mc_filter) << 15;	// mca_vp8_biliner_en[15]
    cmd |= (0) << 16;	// first_mb_num[28:16]
    cmd |= (0) << 29;	// dct_h264_scale_en[29]
    cmd |= (0) << 30;	// MCA_rounding_type[30], For MCA only
    cmd |= (0) << 31;	// ppa_info_vdb_eb[31], 1: PPA need write MB info to DDR
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG2_OFF, cmd, "VSP_CFG2");

    cmd = xd->update_mb_segmentation_map << 0;	// update_mb_segment_map[0]
    cmd |= pc->mb_no_coeff_skip << 1;			// mb_no_skip_coeff[1]
    cmd |= (pc->ref_frame_sign_bias[GOLDEN_FRAME]) << 4;	// ref_frame_sign_bias[5:2]
    cmd |= (pc->ref_frame_sign_bias[ALTREF_FRAME]) << 5;	// ref_frame_sign_bias[5:2]
    cmd |= (1 << pc->multi_token_partition) << 6;	// nbr_dct_partitions[9:6]
    cmd |= (fh_reg_ptr->FH_CFG11) << 10; // dct_part_offset[31:10]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG3_OFF, cmd, "VSP_CFG3");

    cmd = bc->range << 0;			// bd_range[7:0]
    cmd |= bc->count << 8;			// bd_bitcnt[11:8]
    cmd |= pc->filter_type << 12;	// filter_type[12]
    cmd |= pc->sharpness_level<<13;	// sharpness_level[15:13]
    cmd |= bc->value << 16;			// bd_value[31:16]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG4_OFF, cmd, "VSP_CFG4");

    cmd = ((fh_reg_ptr->FH_CFG8>>8) & 0xff) << 0;	// Prob_intra[7:0]
    cmd |= ((fh_reg_ptr->FH_CFG8>>16) & 0xff) << 8;	// Prob_last[15:8]
    cmd |= ((fh_reg_ptr->FH_CFG8>>24) & 0xff) << 16;	// Prob_gf[23:16]
    cmd |= (fh_reg_ptr->FH_CFG8 & 0xff) << 24;		// Prob_skip_false[31:24]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG5_OFF, cmd, "VSP_CFG5");

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
        } else
        {
            QIndex[z] = pc->base_qindex;
            baseline_filter_level[z] = pc->filter_level;
        }
    }

    cmd = (xd->mode_ref_lf_delta_enabled) << 0;	// mode_ref_lf_delta_enabled[0]
    cmd |= (baseline_filter_level[0] & 0x3f) << 1;	// filter_level_0[6:1]
    cmd |= (baseline_filter_level[1] & 0x3f) << 7;	// filter_level_1[12:7]
    cmd |= (baseline_filter_level[2] & 0x3f) << 13;	// filter_level_2[18:13]
    cmd |= (baseline_filter_level[3] & 0x3f) << 19;	// filter_level_3[24:19]
    cmd |= (pc->full_pixel) << 25;	// uv_full_pixel[25]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG6_OFF, cmd, "VSP_CFG6");

    cmd = (xd->ref_lf_deltas[0]&0xff) << 0;	// ref_lf_delta0[7:0], ref_frame 0 1 2 3
    cmd |= (xd->ref_lf_deltas[1]&0xff) << 8;	// ref_lf_delta1[15:8]
    cmd |= (xd->ref_lf_deltas[2]&0xff) << 16;	// ref_lf_delta2[23:16]
    cmd |= (xd->ref_lf_deltas[3]&0xff) << 24;	// ref_lf_delta3[31:24]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG7_OFF, cmd, "VSP_CFG7");

    cmd = (xd->mode_lf_deltas[0]&0xff) << 0;	// mode_lf_deltas0[7:0], mbmi->mode 0 1 2 3
    cmd |= (xd->mode_lf_deltas[1]&0xff) << 8;	// mode_lf_deltas1[15:8]
    cmd |= (xd->mode_lf_deltas[2]&0xff) << 16;	// mode_lf_deltas2[23:16]
    cmd |= (xd->mode_lf_deltas[3]&0xff) << 24;	// mode_lf_deltas3[31:24]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG8_OFF, cmd, "VSP_CFG8");

    cmd = (pc->Y1dequant[ QIndex[0] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_0[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[0] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_0[17:9]
    cmd |= ((pc->UVdequant[ QIndex[0] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_0[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX0_OFF, cmd, "ADDR_IDX_CFG0, QP_DC_0");

    cmd = (pc->Y1dequant[ QIndex[0] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_0[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[0] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_0[17:9]
    cmd |= ((pc->UVdequant[ QIndex[0] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_0[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX1_OFF, cmd, "ADDR_IDX_CFG1, QP_AC_0");

    cmd = (pc->Y1dequant[ QIndex[1] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_1[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[1] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_1[17:9]
    cmd |= ((pc->UVdequant[ QIndex[1] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_1[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX2_OFF, cmd, "ADDR_IDX_CFG2, QP_DC_1");

    cmd = (pc->Y1dequant[ QIndex[1] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_1[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[1] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_1[17:9]
    cmd |= ((pc->UVdequant[ QIndex[1] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_1[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX3_OFF, cmd, "ADDR_IDX_CFG3, QP_AC_1");

    cmd = (pc->Y1dequant[ QIndex[2] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_2[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[2] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_2[17:9]
    cmd |= ((pc->UVdequant[ QIndex[2] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_2[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX4_OFF, cmd, "ADDR_IDX_CFG4, QP_DC_2");

    cmd = (pc->Y1dequant[ QIndex[2] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_2[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[2] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_2[17:9]
    cmd |= ((pc->UVdequant[ QIndex[2] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_2[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX5_OFF, cmd, "ADDR_IDX_CFG5, QP_AC_2");

    cmd = (pc->Y1dequant[ QIndex[3] ][0][0] & 0x1ff) << 0;		// QP_Y1_DC_3[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[3] ][0][0] & 0x1ff) << 9);	// QP_Y2_DC_3[17:9]
    cmd |= ((pc->UVdequant[ QIndex[3] ][0][0] & 0x1ff) << 18);	// QP_UV_DC_3[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG9_OFF, cmd, "VSP_CFG9, QP_DC_3");

    cmd = (pc->Y1dequant[ QIndex[3] ][0][1] & 0x1ff) << 0;		// QP_Y1_AC_3[8:0]
    cmd |= ((pc->Y2dequant[ QIndex[3] ][0][1] & 0x1ff) << 9);	// QP_Y2_AC_3[17:9]
    cmd |= ((pc->UVdequant[ QIndex[3] ][0][1] & 0x1ff) << 18);	// QP_UV_AC_3[26:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG10_OFF, cmd, "VSP_CFG10, QP_AC_3");

    cmd = 0;	// List0_POC[0][31:0]
    VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR + 0x0, cmd, "List0_POC[0]");

    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x0, ((uint32)pc->new_frame.y_buffer) >> 3, "Frm_addr0: Start address of reconstruct frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x4, ((uint32)pc->new_frame.u_buffer) >> 3, "Frm_addr1: Start address of reconstruct frame UV");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x10, ((uint32)Vp8Dec_ExtraMem_V2P(vo, pc->FRAME_ADDR_4, EXTRA_MEM)) >> 3, "Frm_addr4: Partition info for MB Header");
    if(pc->frame_type != KEY_FRAME)
    {
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80, ((uint32)pc->last_frame.y_buffer) >> 3, "Frm_addr32: Start address of Reference list0 frame Y(last)");
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100, ((uint32)pc->last_frame.u_buffer) >> 3, "Frm_addr64: Start address of Reference list0 frame UV(last)");
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x84, ((uint32)pc->golden_frame.y_buffer) >> 3, "Frm_addr33: Start address of Reference list0 frame Y(golden)");
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x104, ((uint32)pc->golden_frame.u_buffer) >> 3, "Frm_addr65: Start address of Reference list0 frame UV(golden)");
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x88, ((uint32)pc->alt_ref_frame.y_buffer) >> 3, "Frm_addr34: Start address of Reference list0 frame Y(alt_ref)");
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x108, ((uint32)pc->alt_ref_frame.u_buffer) >> 3, "Frm_addr66: Start address of Reference list0 frame UV(alt_ref)");
    }
}

uint32 BitstreamReadBits (VPXDecObject *vo, uint32 nbits)
{
    uint32 cmd;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_RDY_OFF, V_BIT_0, V_BIT_0, TIME_OUT_CLK, "Polling BSM_RDY");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, ((nbits)&0x3f)<<24, "BSM_OPERATE: Set OPT_BITS");

    cmd = (uint32)VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + BSM_RDATA_OFF, "BSM_RDATA");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, (((nbits)&0x3f)<<24)|0x1, "BSM_OPERATE: BSM_FLUSH n bits");

    return cmd;
}

void Write_tbuf_Probs(VPXDecObject *vo)
{
    int i, j, k;
    uint32 cmd = 0;
    VP8_COMMON *const pc = & vo->common;
    MACROBLOCKD *const xd  = & vo->mb;
    vp8_prob *mvc = (vp8_prob *)pc->fc.mvc;
    uint32 word_addr = 0;

    cmd = (xd->mb_segment_tree_probs[2]<<16) | (xd->mb_segment_tree_probs[1]<<8) | xd->mb_segment_tree_probs[0];
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    for(i=0; i<6; i++)
    {
        cmd = 0;
        for(j=0; j<4; j++)
        {
            cmd |= ((vp8_mode_contexts[i][j]&0xff)<<(j*8));
        }
        VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
    }

    cmd = (vp8_mbsplit_probs[2]<<16) | (vp8_mbsplit_probs[1]<<8) | vp8_mbsplit_probs[0];
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    for(i=0; i<5; i++)
    {
        cmd = (vp8_sub_mv_ref_prob2[i][2]<<16) | (vp8_sub_mv_ref_prob2[i][1]<<8) | vp8_sub_mv_ref_prob2[i][0];
        VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
    }

    for(i=0; i<2; i++)
    {
        mvc = (vp8_prob *)(&(pc->fc.mvc[i]));
        for(j=0; j<4; j++)
        {
            cmd = (mvc[3]<<24) | (mvc[2]<<16) | (mvc[1]<<8) | mvc[0];
            VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
            mvc+=4;
        }
        cmd = (mvc[2]<<16) | (mvc[1]<<8) | mvc[0];
        VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
    }

    cmd = (pc->kf_ymode_prob[3]<<24) | (pc->kf_ymode_prob[2]<<16) | (pc->kf_ymode_prob[1]<<8) | (pc->kf_ymode_prob[0]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    cmd = (pc->fc.ymode_prob[3]<<24) | (pc->fc.ymode_prob[2]<<16) | (pc->fc.ymode_prob[1]<<8) | (pc->fc.ymode_prob[0]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    cmd = (pc->kf_uv_mode_prob[2]<<16) | (pc->kf_uv_mode_prob[1]<<8) | (pc->kf_uv_mode_prob[0]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    cmd = (pc->fc.uv_mode_prob[2]<<16) | (pc->fc.uv_mode_prob[1]<<8) | (pc->fc.uv_mode_prob[0]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    for(i=0; i<10; i++)
    {
        for(j=0; j<10; j++)
        {
            cmd = (pc->kf_bmode_prob[i][j][3]<<24) | (pc->kf_bmode_prob[i][j][2]<<16) | (pc->kf_bmode_prob[i][j][1]<<8) | (pc->kf_bmode_prob[i][j][0]);
            VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

            cmd = (pc->kf_bmode_prob[i][j][7]<<24) | (pc->kf_bmode_prob[i][j][6]<<16) | (pc->kf_bmode_prob[i][j][5]<<8) | (pc->kf_bmode_prob[i][j][4]);
            VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

            cmd = (pc->kf_bmode_prob[i][j][8]);
            VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
        }
    }

    cmd = (pc->fc.bmode_prob[3]<<24) | (pc->fc.bmode_prob[2]<<16) | (pc->fc.bmode_prob[1]<<8) | (pc->fc.bmode_prob[0]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    cmd = (pc->fc.bmode_prob[7]<<24) | (pc->fc.bmode_prob[6]<<16) | (pc->fc.bmode_prob[5]<<8) | (pc->fc.bmode_prob[4]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    cmd = (pc->fc.bmode_prob[8]);
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");

    for(k=0; k<4; k++)
    {
        for(i=0; i<8; i++)
        {
            for(j=0; j<3; j++)
            {
                cmd = (pc->fc.coef_probs[k][i][j][3]<<24) | (pc->fc.coef_probs[k][i][j][2]<<16) | (pc->fc.coef_probs[k][i][j][1]<<8) | (pc->fc.coef_probs[k][i][j][0]);
                VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
                cmd = (pc->fc.coef_probs[k][i][j][7]<<24) | (pc->fc.coef_probs[k][i][j][6]<<16) | (pc->fc.coef_probs[k][i][j][5]<<8) | (pc->fc.coef_probs[k][i][j][4]);
                VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
                cmd = (pc->fc.coef_probs[k][i][j][10]<<16) | (pc->fc.coef_probs[k][i][j][9]<<8) | (pc->fc.coef_probs[k][i][j][8]);
                VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, cmd, "Write VLC table0");
            }
        }
    }

    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x0000009F), "Write VLC table0");	// Pcat1
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x000091A5), "Write VLC table0");	// Pcat2
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x008C94AD), "Write VLC table0");	// Pcat3
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x878C9BB0), "Write VLC table0");	// Pcat4
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x00000000), "Write VLC table0");	// Pcat4
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x868D9DB4), "Write VLC table0");	// Pcat5
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x00000082), "Write VLC table0");	// Pcat5
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0xE6F3FEFE), "Write VLC table0");	// Pcat6
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x8C99B1C4), "Write VLC table0");	// Pcat6
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (0x00818285), "Write VLC table0");	// Pcat6
    VSP_WRITE_REG(VLC_TABLE0_BASE_ADDR + 4*word_addr++, (vo->g_fh_reg_ptr->FH_CFG8), "Write VLC table0");
}

void Vp8Dec_create_decompressor(VPXDecObject *vo)
{
    vpx_memset(&vo->mb, 0, sizeof(MACROBLOCKD));
    vo->common.new_frame.buffer_alloc = 0;
    vo->common.new_frame.addr_idx = 0;
    vo->common.new_frame.pBufferHeader = NULL;
    vo->common.last_frame.buffer_alloc = 0;
    vo->common.last_frame.addr_idx = 0;
    vo->common.last_frame.pBufferHeader = NULL;
    vo->common.golden_frame.buffer_alloc = 0;
    vo->common.golden_frame.addr_idx = 0;
    vo->common.golden_frame.pBufferHeader = NULL;
    vo->common.alt_ref_frame.buffer_alloc = 0;
    vo->common.alt_ref_frame.addr_idx = 0;
    vo->common.alt_ref_frame.pBufferHeader = NULL;
    vo->common.FRAME_ADDR_4 = 0;
    vo->common.ref_count[0] = 0;
    vo->common.ref_count[1] = 0;
    vo->common.ref_count[2] = 0;
    vo->common.ref_count[3] = 0;

    vo->common.buffer_pool [0] =NULL;
    vo->common.buffer_pool [1] =NULL;
    vo->common.buffer_pool [2] =NULL;
    vo->common.buffer_pool [3] =NULL;

    vo->common.refresh_golden_frame = 0;
    vo->common.refresh_alt_ref_frame = 0;
    vo->common.y1dc_delta_q = 0;
    vo->common.y2dc_delta_q = 0;
    vo->common.uvdc_delta_q = 0;
    vo->common.y2ac_delta_q = 0;
    vo->common.uvac_delta_q = 0;
    vo->common.Width = 0;
    vo->common.Height = 0;

    vp8_create_common(&vo->common);

    vp8cx_init_de_quantizer(vo);
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

