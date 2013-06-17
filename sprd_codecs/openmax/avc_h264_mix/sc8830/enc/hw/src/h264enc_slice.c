
#include "sc6800x_video_header.h"
#include "video_common.h"
#ifdef SIM_IN_WIN
	#include "hmea_global.h"
	#include "tv_mea.h"
	#include "hvlc_tv.h"
#endif

PUBLIC int32 h264enc_slicetype_decide(ENC_IMAGE_PARAMS_T *img_ptr)
{
 //	return SLICE_TYPE_I;

	if((g_nFrame_enc % INTRA_PERIOD) == 0x0)
	{
		return SLICE_TYPE_I;
	}else
	{
		return SLICE_TYPE_P;
	}
}
//fill "default" values
LOCAL void h264enc_slice_header_init (ENC_IMAGE_PARAMS_T *img_ptr, ENC_SLICE_HEADER_T *sh, ENC_SPS_T *sps, 
						ENC_PPS_T *pps, int32 i_type, int32 i_idr_pic_id, int32 i_frame, int32 i_qp)
{
	//first we fill all field
	sh->sps = sps;
	sh->pps = pps;

	sh->i_type = i_type;
	//sh->i_first_mb = 0;
	sh->i_last_mb = IClip(0, img_ptr->frame_size_in_mbs-1, (int32)(sh->i_first_mb+img_ptr->slice_mb-1));//img_ptr->sps->i_mb_width*img_ptr->sps->i_mb_height;
	sh->i_pps_id = pps->i_id;

	sh->i_frame_num = i_frame;

	sh->i_idr_pic_id = i_idr_pic_id;

	//poc stuff, fixed later
	sh->i_poc_lsb = 0;
	sh->i_delta_poc[0] = 0;
	sh->i_delta_poc[1] = 0;

	sh->i_redundant_pic_cnt = 0;

	sh->b_num_ref_idx_override = 0;
	sh->i_num_ref_idx_l0_active = 1;
	sh->i_num_ref_idx_l1_active = 1;

	sh->b_ref_pic_list_reordering_l0 = 0;
//	sh->b_ref_pic_list_reordering_l1 = 0;

	sh->i_qp = i_qp;
	sh->i_qp_delta = i_qp - pps->i_pic_init_qp;

	sh->i_alpha_c0_offset	= 0;//3*2;	// -6 ~ 6, div2
	sh->i_beta_offset		= 0;//-3*2;	// -6 ~ 6, div2

	//if effective qp <= 15, deblocking would have no effect anyway
	/*if (pps->b_deblocking_filter_control
		&& (15 < i_qp + 2 * MAX(sh->i_alpha_c0_offset, sh->i_beta_offset)))
	{
		sh->i_disable_deblocking_filter_idc = 0;
	}else
	{
		sh->i_disable_deblocking_filter_idc = 1;
	}*/
	sh->i_disable_deblocking_filter_idc = 0; //tmp disable dbk
}

LOCAL void h264enc_slice_header_write (ENC_IMAGE_PARAMS_T *img_ptr)
{
	int32 i;
	uint32 nal_header;
	ENC_SLICE_HEADER_T *sh = &(img_ptr->sh);

	//slice		
	H264Enc_write_nal_start_code();

	/* nal header, ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type; */
	nal_header = ( 0x00 << 7 ) | ( img_ptr->i_nal_ref_idc << 5 ) | img_ptr->i_nal_type;
	H264Enc_OutputBits (nal_header, 8);

	WRITE_UE_V (sh->i_first_mb);
	WRITE_UE_V (sh->i_type+5); //same type things
	WRITE_UE_V (sh->i_pps_id);
	H264Enc_OutputBits (sh->i_frame_num, sh->sps->i_log2_max_frame_num);

	if (sh->i_idr_pic_id >= 0) //NAL IDR
	{
		WRITE_UE_V (sh->i_idr_pic_id);
	}

	if (sh->sps->i_poc_type == 0)
	{
		H264Enc_OutputBits (sh->i_poc_lsb, sh->sps->i_log2_max_poc_lsb);
	}

	if (sh->pps->b_redundant_pic_cnt)
	{
		WRITE_UE_V (sh->i_redundant_pic_cnt);
	}

	if (sh->i_type == SLICE_TYPE_P)
	{
		H264Enc_OutputBits (sh->b_num_ref_idx_override, 1);

		if (sh->b_num_ref_idx_override)
		{
			WRITE_UE_V (sh->i_num_ref_idx_l0_active -1);
		}
	}

	//ref pic list reordering
	if (sh->i_type != SLICE_TYPE_I)
	{
		H264Enc_OutputBits (sh->b_ref_pic_list_reordering_l0, 1);

		if (sh->b_ref_pic_list_reordering_l0)
		{
			for (i = 0; i < sh->i_num_ref_idx_l0_active; i++)
			{
				//WRITE_UE_V (sh->ref_pic_list_order[0][i].idc);
				//WRITE_UE_V (sh->ref_pic_list_order[0][i].arg);
			}
			WRITE_UE_V (3);
		}
	}

	if (img_ptr->i_nal_ref_idc != 0)
	{
		if(sh->i_idr_pic_id >= 0)
		{
			H264Enc_OutputBits (0, 1); //no output of prior pics flag
			H264Enc_OutputBits (0, 1); //long term reference flag
		}else
		{
			H264Enc_OutputBits (0, 1); //adaptive_ref_pic_marking_mode_flag
		}
	}

	WRITE_SE_V (sh->i_qp_delta); //slice qp delta

	if (sh->pps->b_deblocking_filter_control)
	{
		WRITE_UE_V(sh->i_disable_deblocking_filter_idc);

		if (sh->i_disable_deblocking_filter_idc != 1)
		{
			WRITE_SE_V (sh->i_alpha_c0_offset>>1);
			WRITE_SE_V (sh->i_beta_offset>>1);
		}
	}
}

PUBLIC void h264enc_slice_init (ENC_IMAGE_PARAMS_T *img_ptr, int32 nal_type, int32 slice_type, int32 global_qp)
{
	//create slice header
	if (nal_type == NAL_SLICE_IDR)
	{
		h264enc_slice_header_init (img_ptr, &img_ptr->sh, img_ptr->sps, img_ptr->pps, slice_type, 
			img_ptr->i_idr_pic_id, img_ptr->frame_num, global_qp);

		//increment id
		//img_ptr->i_idr_pic_id = (img_ptr->i_idr_pic_id + 1) % 65536;
	}else
	{
		h264enc_slice_header_init (img_ptr, &img_ptr->sh, img_ptr->sps, img_ptr->pps, slice_type, -1, img_ptr->frame_num, global_qp);

		//always set the real higher num of ref frame used
		img_ptr->sh.b_num_ref_idx_override = 1;
		img_ptr->sh.i_num_ref_idx_l0_active = 1/*(h->i_ref0 <= 0) ? 1 : h->i_ref0*/;
	//	g_slice_header_ptr->i_num_ref_idx_l1_active = (h->i_ref1 <= 0) ? 1 : img_ptr->i_ref1;
	}

	if (img_ptr->sps->i_poc_type == 0)
	{
		img_ptr->sh.i_poc_lsb = img_ptr->pYUVRecFrame->i_poc & ( ( 1<< img_ptr->sps->i_log2_max_poc_lsb) - 1);
		img_ptr->sh.i_delta_poc_bottom = 0; //won't work for field
	}else if (img_ptr->sps->i_poc_type == 1)
	{
		//nothing to do
	}else
	{
		//nothing to do
	}	

//	h264enc_macroblock_slice_init (h);
}

#ifdef SIM_IN_WIN
PUBLIC void H264Enc_VspFrameInit(ENC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 cmd;	
	uint32 frm_offset = 0;

	if (!g_is_yuv_frm_malloced)
	{
		//now, for uv_interleaved
		cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "Read out Endian Info") & 0xf;
	#if _CMODEL_ //for RTL simulation
		cmd |= (((g_input->pic_height*g_input->pic_width)>>2)<<4); //word unit	
	#else
		cmd |= (((img_ptr->pYUVSrcFrame->imgUV - img_ptr->pYUVSrcFrame->imgY) >> 2)<<4); //word unit
	#endif
		VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");

		g_is_yuv_frm_malloced = TRUE;
 	}

#if _CMODEL_ //for RTL simulation
	frm_offset = (g_input->pic_width*g_input->pic_height*3*g_nFrame_enc)>>(8+1);
#endif

//	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CFG_OFF, MBC_RUN_AUTO_MODE, "MBC_CFG: mpeg4 use auto_mode");
	/*init AHBM, current frame, forward reference frame, backward reference frame*/	
	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 0, img_ptr->pYUVRecFrame->imgYAddr,"configure reconstruct frame Y");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 4, img_ptr->pYUVRefFrame->imgYAddr,"configure reference frame Y");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 16, img_ptr->pYUVSrcFrame->imgYAddr+frm_offset,"configure source frame Y");
	close_vsp_iram();

	cmd = (JPEG_FW_YUV420 << 24) | (img_ptr->frame_height_in_mbs << 12) | img_ptr->frame_width_in_mbs;
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");

	cmd = (img_ptr->height << 16) | (img_ptr->width);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, cmd, "DCAM_DCAM_SRC_SZIE: configure frame width and height");

// 	configure_huff_tab(g_mp4_enc_huff_tbl, 128);

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_RAW_OFF, 0, "DCAM_INT_RAW: clear int_raw flag");

	//dbk.
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, ((img_ptr->sh.i_disable_deblocking_filter_idc==0)<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: disable/enable post-filter and free_run_mode");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_0, "DBK_VDB_BUF_ST: init ping buffer to be available");

	//mea register configure
// 	Mp4Enc_init_MEA(vop_mode_ptr);	

	//vlc
//	cmd = ((vop_mode_ptr->MBNumX * vop_mode_ptr->MBNumY)  & 0x1ffff);
//	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CFG_OFF, cmd, "VLC_CFG_OFF: total mcu number");

	//init with not availabe (for top right dx = 7, 15
	memset(g_mb_cache_ptr->ref, -2, 6*8*sizeof(int8));

	g_mea_fetch_ptr->src_frame[0] = (uint32)(g_enc_image_ptr->pYUVSrcFrame->imgY);
	g_mea_fetch_ptr->src_frame[1] = (uint32)(g_enc_image_ptr->pYUVSrcFrame->imgUV);
	g_mea_fetch_ptr->ref_frame[0] = (uint32)(g_enc_image_ptr->pYUVRefFrame->imgY);
	g_mea_fetch_ptr->ref_frame[1] = (uint32)(g_enc_image_ptr->pYUVRefFrame->imgUV);

//	PrintfSrcFrame ((uint32 *)(g_mea_fetch_ptr->src_frame[0]), (uint32 *)(g_mea_fetch_ptr->src_frame[1]), (int)(img_ptr->width/4), (int)(img_ptr->height));

//	PrintfCmd (1, MEA_REG_BASE+MEA_FRM_TYPE_OFF, !(g_enc_image_ptr->sh.i_type == SLICE_TYPE_I), 0);
}



void prepare_slice_info(ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, ENC_MB_CACHE_T *mb_cache_ptr, int *slice_info)
{
	int i, j;
	ENC_MB_MODE_T * lmb_info_ptr  = mb_info_ptr-1;
	ENC_MB_MODE_T * tmb_info_ptr  = mb_info_ptr - img_ptr->frame_width_in_mbs;
	uint8 zScanOrder[16] =
	{
		0,  1,  4,  5,
		2,  3,  6,  7,
		8,  9, 12, 13,
		10, 11, 14, 15
	};

	slice_info[0] = img_ptr->mb_x;//7b cur_mb_x
	slice_info[1] = img_ptr->mb_y;//7b cur_mb_y
	slice_info[2] = mb_cache_ptr->mb_avail_a;//1b
	slice_info[3] = mb_cache_ptr->mb_avail_b;//1b
	slice_info[4] = mb_cache_ptr->mb_avail_c;//1b
	slice_info[5] = mb_cache_ptr->mb_avail_d;//1b
	slice_info[6] = img_ptr->sh.i_disable_deblocking_filter_idc;
	slice_info[107] = IS_INTRA(mb_info_ptr->type);	// is_intra
	slice_info[110] = mb_info_ptr->qp;	// QP_Y
	slice_info[113] = img_ptr->i_chroma_qp_offset;
	slice_info[114] = img_ptr->sh.i_alpha_c0_offset;
	slice_info[115] = img_ptr->sh.i_beta_offset;
	slice_info[116] = (mb_info_ptr->type == I_4x4);

	// slice_info[7] ~ slice_info[31], ref_idx is beyond boundary
	// slice_info[32] ~ slice_info[81], mv[x][y]
	// slice_info[82] ~ slice_info[106], cbp
	// slice_info[7], slice_info[32], slice_info[33], slice_info[82] are not used
	if( img_ptr->mb_x > 0 )	// left available
	{
		slice_info[12] = slice_info[17] = lmb_info_ptr->ref[1];
		slice_info[22] = slice_info[27] = lmb_info_ptr->ref[3];
		slice_info[108] = IS_INTRA(lmb_info_ptr->type);	// is_left_intra
		slice_info[111] = lmb_info_ptr->qp;	// left_QP_Y
		for(i=0; i<4; i++)
		{
			slice_info[42+10*i] = lmb_info_ptr->mv[3+i*4][0];
			slice_info[43+10*i] = lmb_info_ptr->mv[3+i*4][1];
			slice_info[87+i*5] = (lmb_info_ptr->nnz[zScanOrder[3+4*i]]!=0);
		}
	}
	if( img_ptr->mb_y > 0 )	// top available
	{
		slice_info[8] = slice_info[9] = tmb_info_ptr->ref[2];
		slice_info[10] = slice_info[11] = tmb_info_ptr->ref[3];
		slice_info[109] = IS_INTRA(tmb_info_ptr->type);	// is_top_intra
		slice_info[112] = tmb_info_ptr->qp;	// top_QP_Y
		for(i=0; i<4; i++)
		{
			slice_info[34+2*i] = tmb_info_ptr->mv[12+i][0];
			slice_info[35+2*i] = tmb_info_ptr->mv[12+i][1];
			slice_info[83+i] = (tmb_info_ptr->nnz[zScanOrder[12+i]]!=0);
		}
	}

	for(i=0; i<4; i++)
	{
		for(j=0; j<4; j++)
		{
			slice_info[13+j+i*5] = mb_cache_ptr->ref[x264_scan8[zScanOrder[i*4+j]]];
			slice_info[44+2*j+i*10] = mb_cache_ptr->mv[x264_scan8[zScanOrder[i*4+j]]][0];
			slice_info[45+2*j+i*10] = mb_cache_ptr->mv[x264_scan8[zScanOrder[i*4+j]]][1];
			slice_info[88+j+i*5] = mb_cache_ptr->nnz[x264_scan8[zScanOrder[i*4+j]]];
		}
	}
}



extern FILE * stat_out;
#endif	// SIM_IN_WIN

#ifdef TV_OUT
short dct_out_buf[432];
short idct_in_buf[432];
#endif
PUBLIC int32 h264enc_slice_write (ENC_IMAGE_PARAMS_T *img_ptr)
{
#ifdef SIM_IN_WIN
	int32 mb_xy;
	int32 i_skip;
	int32 i_mb_x, i_mb_y;
	int32 start_mb_x, start_mb_y;
	int32 skip_mb_num = 0;
	int32 is_i_frame = (g_enc_image_ptr->sh.i_type == SLICE_TYPE_I)?1:0;
	ENC_MB_MODE_T *mb_info_ptr;
	ENC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	int slice_info[120] = {0};
#endif	
	int32 i_frame_size;
	uint32 slice_bits;

	img_ptr->slice_end = 0;
#ifdef SIM_IN_WIN
	slice_bits = g_bsm_reg_ptr->TOTAL_BITS;
#else
	slice_bits = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS");
#endif

	//slice header
	h264enc_slice_header_write(img_ptr);

	img_ptr->qp = img_ptr->sh.i_qp;
//	img_ptr->i_last_dqp = 0;

#ifdef SIM_IN_WIN
	i_skip = 0;

	if (img_ptr->sh.i_first_mb==0)
		H264Enc_VspFrameInit (img_ptr);

	FPRINTF_ME (g_me_trace_fp, "\n\nfrmae: %d\n", img_ptr->frame_num);

	start_mb_x = img_ptr->sh.i_first_mb % img_ptr->frame_width_in_mbs;
	start_mb_y = img_ptr->sh.i_first_mb / img_ptr->frame_width_in_mbs;
#endif

	//OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	//OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
	//OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory

#define USE_INTERRUPT

#ifdef USE_INTERRUPT	
	OR1200_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
#endif
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+VSP_INT_MASK_OFF,V_BIT_1 | V_BIT_5,"VSP_INT_MASK, enable vlc_slice_done, time_out");//enable int //frame done/timeout

	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x30, 0x5|((img_ptr->sh.i_first_mb==0)<<3), "ORSC: VSP_START: ENCODE_START=1");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
//	OR1200_WRITE_REG(ORSC_VSP_GLB_OFF + 0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");
	//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));//MCU_SLEEP


	{
#ifdef USE_INTERRUPT
        uint32 tmp = VSP_POLL_COMPLETE();
        SCI_TRACE_LOW("%s, %d, tmp1: %0x", __FUNCTION__, __LINE__, tmp);
#else	
		//ÖÐ¶Ï»½ÐÑ
		uint32 tmp = OR1200_READ_REG(ORSC_VSP_GLB_OFF+0x0C, "ORSC: Check VSP_INT_RAW");
		while ((tmp&0x32)==0)	// not (VLC_FRM_DONE|VLC_ERR|TIME_OUT)
			tmp = OR1200_READ_REG(ORSC_VSP_GLB_OFF+0x0C, "ORSC: Check VSP_INT_RAW");
#endif
		if(tmp&0x30)	// (VLC_ERR|TIME_OUT)
		{
                        img_ptr->error_flag=1;
//			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x08, 0x2e,"ORSC: VSP_INT_CLR: clear BSM_frame error int"); // (VLC_FRM_DONE|MBW_FMR_DONE|PPA_FRM_DONE|TIME_OUT)
		}
		else if((tmp&V_BIT_1)==V_BIT_1)	// VLC_FRM_DONE
		{
			img_ptr->error_flag=0;
//			OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x08, V_BIT_1,"ORSC: VSP_INT_CLR: clear VLC_FRM_DONE");
		}
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF+0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF+0x1C, V_BIT_1, 0x0, "ORSC: Polling AXIM_STS: not Axim_wch_busy"); //check all data has written to DDR
		OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF+0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling MBW_FMR_DONE"); //check MBW is done
		OR1200_WRITE_REG(ORSC_VSP_GLB_OFF+0x08, V_BIT_2,"ORSC: VSP_INT_CLR: clear MBW_FMR_DONE");
		i_frame_size = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS");

		if( (img_ptr->sh.i_last_mb + 1) < img_ptr->frame_size_in_mbs)
			img_ptr->sh.i_first_mb = img_ptr->sh.i_last_mb + 1;
		else
			img_ptr->sh.i_first_mb = 0;
	}

#ifdef RC_BU
	if(g_h264_enc_config->RateCtrlEnable)
		BU_bit_stat = (i_frame_size - slice_bits);
#endif

	return (i_frame_size-slice_bits);
}
