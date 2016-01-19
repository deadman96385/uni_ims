/******************************************************************************
 ** File Name:    h264enc_init.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/18/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/18/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define MEA_SKIP_THRESHOLD	    (1800>>3) // derek_2012-08-24, total 11-bit, but only config [10:3]
#define MEA_SEARCH_ROUND_16			8	// 0=unlimited, or lesst than 10
#define MEA_SEARCH_ROUND_8			3	// 0=unlimited, or lesst than 4

PUBLIC MMEncRet H264Enc_InitVSP(H264EncObject *vo)
{
    ENC_IMAGE_PARAMS_T *img_ptr = vo->g_enc_image_ptr;
    ENC_ANTI_SHAKE_T *anti_shark_ptr = &(vo->g_anti_shake);
    uint32 cmd;

//    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, 0x30868,"axim endian set, vu format"); // VSP and OR endian.
    cmd = V_BIT_17|V_BIT_16|V_BIT_11|V_BIT_5|V_BIT_3;
    if (vo->yuv_format == YUV420SP_NV21)  //vu format
    {
        cmd |= V_BIT_6;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, cmd,"axim endian set, vu format"); //VSP and OR endian.
    cmd =  0;
    if((SHARKL == vo->vsp_version) || (SHARKLT8 == vo->vsp_version))
    {
	cmd = 0x180;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR +AXIM_BURST_GAP_OFF,cmd,"axim burst gap"); //VSP and OR endian.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (STREAM_ID_H264|V_BIT_4|((img_ptr->cabac_enable&0x01)<<9)), "VSP_MODE: Set standard and work mode");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

    cmd = img_ptr->frame_width_in_mbs << 0;	// MB_X_MAX[7:0]
    cmd |= img_ptr->frame_height_in_mbs << 8;	// MB_Y_MAX[15:8]
    cmd |= (anti_shark_ptr->enable_anti_shake ? (anti_shark_ptr->input_width>>3) : (img_ptr->width>>3)) << 16;	  // CUR_IMG_WIDTH[24:16], unit 8-pixels
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + IMG_SIZE_OFF, cmd, "IMG_SIZE: Set MB_X_MAX & MB_Y_MAX & CUR_IMG_WIDTH");

    cmd = ((img_ptr->pYUVSrcFrame->i_type == SLICE_TYPE_I) ? 0 :
           ((img_ptr->pYUVSrcFrame->i_type == SLICE_TYPE_P) ? 1: 2)) << 0;	// FRM_TYPE[2:0], 0:I, 1:P, 2:B
    cmd |= ((img_ptr->frame_size_in_mbs&0x1fff) << 3);	// Max_mb_num[15:3]
    cmd |= (img_ptr->slice_nr << 16);	// Slice_num[24:16]
    cmd |= (img_ptr->sh.i_qp << 25);	// SliceQP[30:25]
    cmd |= (1) << 31;	// Deblocking_eb[31]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x3C, cmd, "VSP_CFG0");

    cmd = ((g_skipBlock_QP_table[img_ptr->sh.i_qp] >> 3) & 0xff) << 0;	// Skip_threshold[7:0]
    cmd |= (MEA_SEARCH_ROUND_16) << 8;	// Ime_16X16_max[11:8], less than 8

    if ((1920 == img_ptr->width) && (1088 == img_ptr->height))
    {
        cmd |= (0x1 << 12);	// Ime_8X8_max[15:12], less than 3
        cmd |= (0x1E7 << 16);	// Ipred_mode_cfg[24:16], IEA prediction mode setting, all 9 modes are on
    } else
    {
        cmd |= (MEA_SEARCH_ROUND_8 << 12);	// Ime_8X8_max[15:12], less than 3
        cmd |= (0x1FF << 16);	// Ipred_mode_cfg[24:16], IEA prediction mode setting, all 9 modes are on
    }
    cmd |= ((img_ptr->sh.i_qp & 0x3f) << 25); // MB_Qp[30:25]
    cmd |= (1 << 31); // Bsm_bytealign_mode[31], let PPA do stop bit and byte align
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG1_OFF, cmd, "VSP_CFG1");

    cmd = ((img_ptr->sh.i_last_mb / img_ptr->frame_width_in_mbs) << 0);	// last_mb_y[6:0]
    cmd |= ((img_ptr->sh.i_first_mb / img_ptr->frame_width_in_mbs) << 8);	// First_mb_y[14:8]
    if (anti_shark_ptr->enable_anti_shake)
    {
        cmd |= (anti_shark_ptr->shift_x >>1) << 16;
    }
    cmd |= (0 << 29);	// Dct_h264_scale_en[29]
    cmd |= (0 << 30);	// MCA_rounding_type[30], For MCA only
    cmd |= (0 << 31);	// Ppa_info_vdb_eb[31], 1: PPA need write MB info to DDR
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG2_OFF, cmd, "VSP_CFG2");

    cmd = ((img_ptr->sh.i_alpha_c0_offset & 0x1f) << 0);	// ALPHA_OFFSET[4:0]
    cmd |= ((img_ptr->sh.i_beta_offset & 0x1f) << 5);		// BETA_OFFSET[9:5]
    cmd |= (img_ptr->pps->b_constrained_intra_pred << 10);// constrained_intra_pred_flag[10]
    cmd |= (0 << 11);	// direct_spatial_mv_pred_flag[11], used in B slice
    cmd |= (img_ptr->sps->b_direct8x8_inference << 12);	// direct_8x8_inference_flag[12]
    cmd |= (0 << 13);	// transform_8x8_mode_flag[13]
    cmd	|= img_ptr->pps->b_entropy_coding_mode_flag << 14;	// Entropy_coding_mode_flag[14], 0:CAVLC, 1:CABAC
    cmd |= (0 << 15); // MCA_weighted_en[15]
    cmd |= (0 << 16);	// Weighted_pred_flag[16]
    cmd |= (0 << 17);	// Weighted_bipred_idc[18:17]
    cmd |= (0 << 19);	// Disable_deblocking_filter_idc[20:19]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG3_OFF, cmd, "VSP_CFG3");

    cmd = ((img_ptr->pps->i_chroma_qp_index_offset & 0x1f) << 0);	// Chroma_qp_index_offset[4:0]
    cmd |= ((0 & 0x1f) << 5);	// Second_Chroma_qp_index_offset[9:5]
    cmd |= (((img_ptr->pps->i_num_ref_idx_l0_active-1) & 0x1f) << 10);// Num_refidx_l0_active_minus1[14:10]
    cmd |= (((img_ptr->pps->i_num_ref_idx_l1_active-1) & 0x1f) << 15);// Num_refidx_l1_active_minus1[19:15]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG4_OFF, cmd, "VSP_CFG4");

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG5_OFF, img_ptr->sh.i_poc_lsb, "VSP_CFG5, Cur_poc[31:0]");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_ADDRIDX0_OFF, (0 & 0x3f), "ADDR_IDX_CFG0");
    VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR + 0x0, img_ptr->pYUVRefFrame->i_poc, "List0_POC[0]");
    if(anti_shark_ptr->enable_anti_shake)
    {
        img_ptr->pYUVSrcFrame->imgY += (anti_shark_ptr->shift_y*anti_shark_ptr->input_width );
        img_ptr->pYUVSrcFrame->imgYAddr = (uint_32or64)img_ptr->pYUVSrcFrame->imgY >> 3;
        img_ptr->pYUVSrcFrame->imgUV += (anti_shark_ptr->shift_y*anti_shark_ptr->input_width )/2;
        img_ptr->pYUVSrcFrame->imgUVAddr = (uint_32or64)img_ptr->pYUVSrcFrame->imgUV >> 3;
    }
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x20, img_ptr->pYUVSrcFrame->imgYAddr, "Frm_addr8: Start address of current frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x24, img_ptr->pYUVSrcFrame->imgUVAddr, "Frm_addr9: Start address of current frame UV");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x0, img_ptr->pYUVRecFrame->imgYAddr, "Frm_addr0: Start address of reconstruct frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x4, img_ptr->pYUVRecFrame->imgUVAddr, "Frm_addr1: Start address of reconstruct frame UV");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80, img_ptr->pYUVRefFrame->imgYAddr, "Frm_addr32: Start address of Reference list0 frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100, img_ptr->pYUVRefFrame->imgUVAddr, "Frm_addr64: Start address of Reference list0 frame UV");

    cmd	= (uint_32or64)H264Enc_ExtraMem_V2P(vo, (uint8 *)vo->g_vlc_hw_ptr, EXTRA_MEM)>>3;		// Frm_addr3[31:0], VLC Table set by Fixed Table Address
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xC, cmd, "Frm_addr3: Start address of VLC table");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_SIZE_SET_OFF, 300, "VSP_SIZE_SET: VLC_table_size");

    return MMENC_OK;
}

PUBLIC void H264Enc_InitBSM(H264EncObject *vo)
{
    ENC_IMAGE_PARAMS_T *img_ptr = vo->g_enc_image_ptr;
    uint32 cmd;

    cmd = (((uint32)img_ptr->OneFrameBitstream_addr_phy) >> 3);	// Bsm_buf0_frm_addr[31:0]
    cmd += (img_ptr->stm_offset >> 3);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + BSM0_FRM_ADDR_OFF, cmd, "BSM0_FRM_ADDR");

    cmd = 0x04;	// Move data remain in fifo to external memeory
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, cmd, "BSM_OPERATE: COUNT_CLR");

    cmd = (1 << 31);	// BUFF0_STATUS[31], 1: active
    cmd |= (0 << 30);	// BUFF1_STATUS[30]
    cmd |= ((img_ptr->OneframeStreamLen&(~0x3)) & (V_BIT_30 -1)); // BUFFER_SIZE[29:0], unit byte//cmd = g_bsm_reg_ptr->BSM_CFG0 << 0;
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_CFG0_OFF, cmd, "BSM_CFG0");

    cmd = (0 << 31);	// DESTUFFING_EN
    cmd |= ((0 & 0x3FFFFFFF) << 0);	// OFFSET_ADDR[29:0], unit word //cmd = g_bsm_reg_ptr->BSM_CFG1 << 0;
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_CFG1_OFF, cmd, "BSM_CFG1");
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

