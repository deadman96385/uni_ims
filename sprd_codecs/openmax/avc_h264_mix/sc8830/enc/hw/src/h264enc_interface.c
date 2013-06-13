#include "sc6800x_video_header.h"
#ifdef SIM_IN_WIN
	#include "tv_mea.h"
#endif

/*****************************************************************************/
//  Description:   Init h264 encoder 
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/

MMEncRet H264EncInit(MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat)
{
	MMEncRet is_init_success = MMENC_OK;
	ENC_IMAGE_PARAMS_T *img_ptr;
	uint32 frame_buf_size;

	SCI_ASSERT(NULL != pInterMemBfr);
	SCI_ASSERT(NULL != pExtaMemBfr);
	SCI_ASSERT(NULL != pVideoFormat);

	if (VSP_OPEN_Dev() < 0)
	{
		return MMENC_ERROR;
	}

#ifdef SIM_IN_WIN
//	HVlcTestVectorInit ();
#endif

	h264enc_mem_init(pInterMemBfr, pExtaMemBfr);

#if _CMODEL_
	VSP_Init_CModel();
#endif //_CMODEL_

	g_is_yuv_frm_malloced = FALSE;
	g_nFrame_enc = 0;

	img_ptr = g_enc_image_ptr = (ENC_IMAGE_PARAMS_T *)h264enc_inter_mem_alloc(sizeof(ENC_IMAGE_PARAMS_T));

	g_h264_enc_config = (MMEncConfig *)h264enc_inter_mem_alloc(sizeof(MMEncConfig));

	g_vlc_hw_ptr = (uint8 *)h264enc_inter_mem_alloc(406*8);

	memcpy(g_vlc_hw_ptr, g_vlc_hw_tbl, (406*8));
#ifdef SIM_IN_WIN
	g_mb_cache_ptr = (ENC_MB_CACHE_T *)h264enc_extra_mem_alloc(sizeof(ENC_MB_CACHE_T));
#endif

	img_ptr->width = pVideoFormat->frame_width;
	img_ptr->height = pVideoFormat->frame_height;
// 	img_ptr->i_frame = 0;
	img_ptr->frame_num = 0;
	img_ptr->i_idr_pic_id = 0;
	img_ptr->i_sps_id = 0;
	img_ptr->i_level_idc = 51;	//as close to "unresticted as we can get
//	img_ptr->i_max_ref0 = img_ptr->i_frame_reference_num;
//	img_ptr->i_max_dpb = 2;	//h->sps->vui.i_max_dec_frame_buffering + 1;
// 	h->fdec->i_poc = 0; //tmp add, 20100512
	img_ptr->i_keyint_max	= 15;
//	img_ptr->i_last_idr	= -img_ptr->i_keyint_max;
	img_ptr->i_chroma_qp_offset = 0;
 	img_ptr->i_ref0 = 0;
	img_ptr->sh.i_first_mb = 0;
	img_ptr->pic_sz = 0;
	img_ptr->stm_offset = 0;
	img_ptr->slice_nr = 0;
	img_ptr->prev_slice_bits = 0;
	img_ptr->crop_x = 0;
	img_ptr->crop_y = 0;
	img_ptr->frame_width_in_mbs = (img_ptr->width + 15) >> 4;
	img_ptr->frame_height_in_mbs = (img_ptr->height + 15) >>4;
	img_ptr->frame_size_in_mbs = img_ptr->frame_width_in_mbs * img_ptr->frame_height_in_mbs;
	img_ptr->slice_mb = (img_ptr->frame_height_in_mbs / SLICE_MB)*img_ptr->frame_width_in_mbs;

#ifdef SIM_IN_WIN
	img_ptr->mb_info = (ENC_MB_MODE_T *)h264enc_extra_mem_alloc(img_ptr->frame_size_in_mbs*sizeof(ENC_MB_MODE_T));
	img_ptr->ipred_top_line_buffer = (uint8 *)H264Enc_ExtraMemAlloc_64WordAlign (img_ptr->frame_width_in_mbs*32);
#endif

	//init frames
	img_ptr->pYUVSrcFrame = (H264EncStorablePic *)h264enc_inter_mem_alloc(sizeof(H264EncStorablePic));
	img_ptr->pYUVRecFrame = (H264EncStorablePic *)h264enc_inter_mem_alloc(sizeof(H264EncStorablePic));
	img_ptr->pYUVRefFrame = (H264EncStorablePic *)h264enc_inter_mem_alloc(sizeof(H264EncStorablePic));
	img_ptr->pYUVSrcFrame->i_poc = 0;
	img_ptr->pYUVRecFrame->i_poc = 0;
	img_ptr->pYUVRefFrame->i_poc = 0;
	img_ptr->pYUVSrcFrame->addr_idx = 0;
	img_ptr->pYUVRecFrame->addr_idx = 1;
	img_ptr->pYUVRefFrame->addr_idx = 2;

	// init RC parameters
	rc_gop_paras.rem_bits = 0;
	rc_gop_paras.curr_buf_full = 0;

#ifdef SIM_IN_WIN
	H264Enc_InitYUVBfr(img_ptr);
#endif

	/* rate control */
	/////	
#ifdef SIM_IN_WIN
	//initialize the one frame bitstream buffer.
	img_ptr->pOneFrameBitstream = (uint8 *)H264Enc_ExtraMemAlloc_64WordAlign(ONEFRAME_BITSTREAM_BFR_SIZE);  //allocate BITSTREAM_BFR_SIZE for one frame encoded bitstream.
	img_ptr->OneframeStreamLen = ONEFRAME_BITSTREAM_BFR_SIZE;
#else
	img_ptr->pOneFrameBitstream = pBitstreamBfr->common_buffer_ptr;
	img_ptr->OneFrameBitstream_addr_phy = (uint32 )pBitstreamBfr->common_buffer_ptr_phy;
	img_ptr->OneframeStreamLen = pBitstreamBfr->size;

	frame_buf_size = img_ptr->width * img_ptr->height;
	
	img_ptr->pYUVRecFrame->imgY = (uint8 *)h264enc_extra_mem_alloc(frame_buf_size);
	img_ptr->pYUVRecFrame->imgYAddr = (uint32)img_ptr->pYUVRecFrame->imgY >> 3;	// DWORD
	img_ptr->pYUVRecFrame->imgUV = (uint8 *)h264enc_extra_mem_alloc(frame_buf_size/2);
	img_ptr->pYUVRecFrame->imgUVAddr = (uint32)img_ptr->pYUVRecFrame->imgUV >> 3;	// DWORD
	img_ptr->pYUVRefFrame->imgY = (uint8 *)h264enc_extra_mem_alloc(frame_buf_size);
	img_ptr->pYUVRefFrame->imgYAddr = (uint32)img_ptr->pYUVRefFrame->imgY >> 3;	// DWORD
	img_ptr->pYUVRefFrame->imgUV = (uint8 *)h264enc_extra_mem_alloc(frame_buf_size/2);
	img_ptr->pYUVRefFrame->imgUVAddr = (uint32)img_ptr->pYUVRefFrame->imgUV >> 3;	// DWORD
	g_is_yuv_frm_malloced = TRUE;


#endif

	h264enc_sps_init (img_ptr);
	h264enc_pps_init (img_ptr);

	return is_init_success;
}

MMEncRet H264EncSetConf(MMEncConfig *pConf)
{
	SCI_ASSERT(NULL != pConf);
	
	g_h264_enc_config->FrameRate		= pConf->FrameRate;	
	g_h264_enc_config->targetBitRate		= pConf->targetBitRate;
	g_h264_enc_config->RateCtrlEnable	= pConf->RateCtrlEnable;
	
	g_h264_enc_config->QP_IVOP		= pConf->QP_IVOP;
	g_h264_enc_config->QP_PVOP		= pConf->QP_PVOP;	
	
	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Get H264 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncGetConf(MMEncConfig *pConf)
{
	SCI_ASSERT(NULL != pConf);

	pConf->QP_IVOP					 =g_h264_enc_config->QP_IVOP;
	pConf->QP_PVOP					 = g_h264_enc_config->QP_PVOP ;
	
	pConf->targetBitRate 				= g_h264_enc_config->targetBitRate;
	pConf->FrameRate 				= g_h264_enc_config->FrameRate;	
	pConf->RateCtrlEnable 			= g_h264_enc_config->RateCtrlEnable; 
	
	return MMENC_OK;
}


/*****************************************************************************/
//  Description:   Close mpeg4 encoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncRelease(void)
{
#ifndef _FPGA_TEST_
	VSP_CLOSE_Dev();
#endif
	return MMENC_OK;
}

void H264Enc_close(void)
{
	return;
}



#ifdef SIM_IN_WIN
void WriteRecFrame(uint8 *pRec_Y, uint8 *pRec_UV, FILE *pf_recon_file, uint32 frame_width, uint32 frame_height)
{
#if CROP_1080P
	uint32 size = frame_width * IClip(0, 1080, frame_height);
#else
	uint32 size = frame_width * frame_height;
#endif

 	fwrite(pRec_Y,1,size, pf_recon_file);
	size = size>>1;
	fwrite(pRec_UV,1,size, pf_recon_file);
}

uint32 H264Enc_WriteOutStream (MMEncOut *enc_output_ptr)
{
	uint32 prevention_count = 0;
	if (enc_output_ptr->strmSize)
	{
		//static int sps_pps_passed = 0;
		int32 offset = 0;
		uint8 prevention = 0x03;
		uint32 zero_count = 0;
		//int start_code;
		int start_code_count = 0;
		uint32 left_bytes;
		
		/*if( (enc_frame_num==0) && (sps_pps_passed==0))
		{
			start_code = 2;
			sps_pps_passed = 1;
		}
		else
			start_code = 0;*/
		
		while(offset < enc_output_ptr->strmSize)
		{
			if(zero_count==2)
			{
				if( (enc_output_ptr->pOutBuf[offset] == 0x00) ||
					(enc_output_ptr->pOutBuf[offset] == 0x01) ||
					(enc_output_ptr->pOutBuf[offset] == 0x02) ||
					(enc_output_ptr->pOutBuf[offset] == 0x03) )
				{
					if(start_code_count >= 0/*start_code*/)
					{
#ifdef _LIB
						one_frame_stream[off_frame_strm++] = prevention;
						//one_frame_stream[off_frame_strm++] = enc_output_ptr->pOutBuf[offset++];
#else
						fwrite (&prevention, 1, 1, s_pEnc_output_bs_file);
						//fwrite (&enc_output_ptr->pOutBuf[offset++], 1, 1, s_pEnc_output_bs_file);
#endif
						prevention_count++;
					}
					else
					{
						start_code_count++;
#ifdef _LIB
						one_frame_stream[off_frame_strm++] = enc_output_ptr->pOutBuf[offset++];
#else
						fwrite (&enc_output_ptr->pOutBuf[offset++], 1, 1, s_pEnc_output_bs_file);
#endif
					}
				}
				else
#ifdef _LIB
					one_frame_stream[off_frame_strm++] = enc_output_ptr->pOutBuf[offset++];
#else
					fwrite (&enc_output_ptr->pOutBuf[offset++], 1, 1, s_pEnc_output_bs_file);
#endif
				zero_count = 0;
			}
			else
			{
				if(enc_output_ptr->pOutBuf[offset] == 0)
					zero_count++;
				else
					zero_count = 0;
#ifdef _LIB
				one_frame_stream[off_frame_strm++] = enc_output_ptr->pOutBuf[offset++];
#else
				fwrite (&enc_output_ptr->pOutBuf[offset++], 1, 1, s_pEnc_output_bs_file);
#endif
			}				
		}
		left_bytes = (8 - ((enc_output_ptr->strmSize+prevention_count) & 0x7)) & 0x7;
		prevention = 0;
		while(left_bytes-- > 0)
#ifdef _LIB
			one_frame_stream[off_frame_strm++] = prevention;
#else
			fwrite (&prevention, 1, 1, s_pEnc_output_bs_file);
#endif
		//fwrite (enc_output_ptr->pOutBuf, 1, enc_output_ptr->strmSize, s_pEnc_output_bs_file);
	}
	return prevention_count;
}
extern FILE * stat_out;
#endif	// SIM_IN_WIN

#define SPS_EN    0
#define PPS_EN   0



/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet H264EncStrmEncode(MMEncIn *pInput, MMEncOut *pOutput)
{
	int32 i_slice_type;
	int32 i_nal_type;
	int32 i_nal_ref_idc;
 	int32 i_global_qp;
	ENC_IMAGE_PARAMS_T *img_ptr = g_enc_image_ptr;
	uint32 rate_control_en = g_h264_enc_config->RateCtrlEnable;

	if(ARM_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}
	SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");	

#ifdef SIM_IN_WIN
	uint32 frame_width = g_input->ori_width;
	uint32 frame_height = g_input->ori_height;
	uint32 frame_size = frame_width * frame_height;
	uint32 cmd;

	if(img_ptr->sh.i_first_mb == 0)
	{
		VSP_Reset();
		/*clear time_out int_raw flag, if timeout occurs*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");
	
		/*init dcam command*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

		/*init vsp command*/
		cmd = (0<<16) |(1<<15) | (1<<14) |(1<<12) | (H264<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0");

		cmd = (0 << 16) |((uint32)0xffff);
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

		// Frame Crop
		if( (g_input->ori_width != g_input->pic_width) || (g_input->ori_height != g_input->pic_height) )
		{
			int32 i;
			uint8 *dst, *src;

			img_ptr->crop_x = Clip3(0, (g_input->ori_width-g_input->pic_width), img_ptr->crop_x+2*1);
			img_ptr->crop_y = Clip3(0, (g_input->ori_height-g_input->pic_height), img_ptr->crop_y+2*1);
			
			dst = img_ptr->pYUVSrcFrame->imgY;
			src = pInput->p_src_y + g_input->ori_width * img_ptr->crop_y + img_ptr->crop_x;
			for(i=0; i<img_ptr->height; i++)
			{
				memcpy(dst, src, img_ptr->width);
				src += g_input->ori_width;
				dst += img_ptr->width;
			}
			dst = img_ptr->pYUVSrcFrame->imgUV;
			src = pInput->p_src_y + frame_size + (g_input->ori_width * (img_ptr->crop_y>>1) + img_ptr->crop_x);
			for(i=0; i<(img_ptr->height>>1); i++)
			{
				memcpy(dst, src, img_ptr->width);
				src += g_input->ori_width;
				dst += img_ptr->width;
			}
		}
		else
		{
			memcpy(img_ptr->pYUVSrcFrame->imgY, pInput->p_src_y, frame_size);
			memcpy(img_ptr->pYUVSrcFrame->imgUV, pInput->p_src_y+frame_size, frame_size/2);
		}
	}
#endif // SIM_IN_WIN

	img_ptr->pYUVSrcFrame->i_frame = g_nFrame_enc;

#ifdef VLC_TV_SPLIT
	VLCSplitInit(img_ptr);
#endif
	
	img_ptr->pYUVSrcFrame->i_type = i_slice_type =( pInput->vopType == 0)? SLICE_TYPE_I : SLICE_TYPE_P/* = (int32)h264enc_slicetype_decide(img_ptr)*/;

	img_ptr->stm_offset = 0;
	img_ptr->pYUVSrcFrame->imgY =  pInput->p_src_y_phy;
	img_ptr->pYUVSrcFrame->imgUV =  pInput->p_src_u_phy;

				
	img_ptr->pYUVSrcFrame->imgYAddr = (uint32)img_ptr->pYUVSrcFrame->imgY >> 3;
	img_ptr->pYUVSrcFrame->imgUVAddr = (uint32)img_ptr->pYUVSrcFrame->imgUV >> 3;

	if( i_slice_type == SLICE_TYPE_I )
	{
		//img_ptr->i_last_idr = img_ptr->pYUVSrcFrame->i_frame;

		//rate_control
		if(img_ptr->sh.i_first_mb==0)
		{
			i_global_qp = rc_pic_paras.curr_pic_qp = rc_init_GOP(&(rc_gop_paras)); // 20
			rc_init_pict(&(rc_gop_paras), &(rc_pic_paras));
#ifdef TV_OUT
#ifdef ONE_FRAME
			if(img_ptr->frame_num>FRAME_X)
#endif
			{
				Print_SrcRef_Frame(img_ptr);
				Print_MEA_CFG(img_ptr, i_global_qp);	// with QP Fixed
			}
#endif
		}
		else
#ifdef NO_BU_CHANGE
			i_global_qp = rc_pic_paras.curr_pic_qp = rc_init_slice(img_ptr, &(rc_pic_paras));
#else
			i_global_qp = rc_pic_paras.curr_pic_qp;
#endif

#if defined(SIM_IN_WIN) && !defined(_LIB)
		FPRINTF(g_fp_trace_fw, "No.%d Frame:\t IDR_SLICE", g_nFrame_enc);
		PRINTF("No.%d Frame:\t IDR_SLICE\t QP=%d", g_nFrame_enc, i_global_qp);
		fprintf(stat_out, "No.%d Frame:\t IDR_SLICE\t QP=%d", g_nFrame_enc, i_global_qp);
#endif
		if( img_ptr->frame_num == 0 )
			i_nal_type = NAL_SLICE_IDR;
		else
			i_nal_type = NAL_SLICE;
		i_nal_ref_idc = NAL_PRIORITY_HIGHEST;
	}
	else if (i_slice_type == SLICE_TYPE_P)
	{
		//rate_control
		if(img_ptr->sh.i_first_mb==0)
		{
			g_h264_enc_config->QP_PVOP = rc_init_pict(&(rc_gop_paras), &(rc_pic_paras));
			rc_pic_paras.curr_pic_qp = g_h264_enc_config->QP_PVOP;
			i_global_qp = g_h264_enc_config->QP_PVOP;
			h264enc_reference_update(img_ptr);
#ifdef TV_OUT
#ifdef ONE_FRAME
			if(img_ptr->frame_num>FRAME_X)
#endif
			{
				Print_SrcRef_Frame(img_ptr);
				Print_MEA_CFG(img_ptr, i_global_qp);	// with QP Fixed
			}
#endif
		}
		else
		{
#ifdef NO_BU_CHANGE
			g_h264_enc_config->QP_PVOP = rc_pic_paras.curr_pic_qp = rc_init_slice(img_ptr, &(rc_pic_paras));
			i_global_qp = g_h264_enc_config->QP_PVOP;
#else
			g_h264_enc_config->QP_PVOP = rc_pic_paras.curr_pic_qp;
			i_global_qp = g_h264_enc_config->QP_PVOP;
#endif
		}

#if defined(SIM_IN_WIN) && !defined(_LIB)
		FPRINTF(g_fp_trace_fw, "No.%d Frame:\t P_SLICE", g_nFrame_enc);
		PRINTF("No.%d Frame:\t P_SLICE\t QP=%d", g_nFrame_enc, g_input->QP_PVOP);
		fprintf(stat_out, "No.%d Frame:\t P_SLICE\t QP=%d", g_nFrame_enc, g_input->QP_PVOP);
#endif
		i_nal_type = NAL_SLICE;
		i_nal_ref_idc = NAL_PRIORITY_HIGH;
	}
	prev_qp = i_global_qp;	// MUST HAVE prev_qp updated!!

	img_ptr->pYUVRecFrame->i_poc =
	img_ptr->pYUVSrcFrame->i_poc = 2 * img_ptr->frame_num;
	img_ptr->pYUVRecFrame->i_type = img_ptr->pYUVSrcFrame->i_type;
	img_ptr->pYUVRecFrame->i_frame = img_ptr->pYUVSrcFrame->i_frame;
	img_ptr->pYUVSrcFrame->b_kept_as_ref =
	img_ptr->pYUVRecFrame->b_kept_as_ref = i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE;

	//init
 	h264enc_reference_build_list (img_ptr, img_ptr->pYUVRecFrame->i_poc, i_slice_type);



	/* ------------------------ Create slice header  ----------------------- */
	h264enc_slice_init( img_ptr, i_nal_type, i_slice_type, i_global_qp );

	// OpenRISC Init
	ORSC_Init();


#ifdef TV_OUT
#ifdef ONE_FRAME
	if(img_ptr->frame_num>FRAME_X)
#endif
		Print_PPA_CFG(img_ptr, i_global_qp);
#endif

	/* ---------------------- Write the bitstream -------------------------- */
	/* Init bitstream context */
#ifdef SIM_IN_WIN
	H264Enc_InitBitStream (img_ptr);
#endif
	img_ptr->i_nal_type = i_nal_type;
	img_ptr->i_nal_ref_idc = i_nal_ref_idc;

	//if(img_ptr->sh.i_first_mb==0)	// Flush each frame, not each slice
	BSM_Init();

	//write SPS and PPS
	if ((i_nal_type == NAL_SLICE_IDR) && (img_ptr->sh.i_first_mb==0))
	{
		img_ptr->pYUVRecFrame->i_poc = 0;

#if SPS_EN
		/*if (g_nFrame_enc == 0)
		{
			h264enc_sei_version_write(img_ptr);
		}*/
#ifdef RC_BU
		{
		int bits_1;//, bits_0;
		/*if(rate_control_en)
		{
#ifdef SIM_IN_WIN
			bits_0 = g_bsm_reg_ptr->TOTAL_BITS;
#else
			bits_0 = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS");
#endif
		}*/
#endif
		//generate sequence parameters
		h264enc_sps_write(img_ptr->sps);
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF+0x1C, V_BIT_1, 0x0, "ORSC: Polling AXIM_STS: not Axim_wch_busy"); //check all data has written to DDR
#ifdef RC_BU
		if(rate_control_en)
		{
#ifdef SIM_IN_WIN
			bits_1 = g_bsm_reg_ptr->TOTAL_BITS;
#else
			bits_1 = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS");
#endif
			rc_pic_paras.target_bits -= (bits_1);
		}
#endif
		
#ifdef SIM_IN_WIN
		img_ptr->stm_offset += (g_bsm_reg_ptr->TOTAL_BITS >> 3);
		OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");
	#ifdef TV_OUT
		FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
		PrintBSMOut(img_ptr);
	#endif
		clear_bsm_fifo();
		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
		pOutput->pOutBuf = img_ptr->pOneFrameBitstream;
//		if( *((uint32*)img_ptr->pOneFrameBitstream) == 0xffffffff )
//			*((uint32*)img_ptr->pOneFrameBitstream) = 0x01000000;
		img_ptr->stm_offset += H264Enc_WriteOutStream(pOutput);
		H264Enc_InitBitStream (img_ptr);
#else
		{
			uint32 * start_ptr;
			start_ptr = (uint32 *)(img_ptr->pOneFrameBitstream +img_ptr->stm_offset );
			* start_ptr = 0x01000000;
			//*((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset-or_addr_offset])) = 0x01000000;
		}
		img_ptr->stm_offset += ( OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS") >> 3);
		img_ptr->stm_offset += OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");
#endif
		img_ptr->stm_offset = (img_ptr->stm_offset+7)&0xfffffff8; // DWORD aligned
		BSM_Init();
#endif

#if PPS_EN
		//generate picture parameters
		h264enc_pps_write(img_ptr->pps);
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF+0x1C, V_BIT_1, 0x0, "ORSC: Polling AXIM_STS: not Axim_wch_busy"); //check all data has written to DDR
#ifdef RC_BU
		if(rate_control_en)
		{
#ifdef SIM_IN_WIN
			bits_1 = g_bsm_reg_ptr->TOTAL_BITS;
#else
			bits_1 = OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS");
#endif
			rc_pic_paras.target_bits -= (bits_1);
		}
		}
#endif
#ifdef SIM_IN_WIN
		img_ptr->stm_offset += (g_bsm_reg_ptr->TOTAL_BITS >> 3);
		OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");
	#ifdef TV_OUT
		FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
		PrintBSMOut(img_ptr);
	#endif
		clear_bsm_fifo();
		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
		pOutput->pOutBuf = img_ptr->pOneFrameBitstream;
//		if( *((uint32*)img_ptr->pOneFrameBitstream) == 0xffffffff )
//			*((uint32*)img_ptr->pOneFrameBitstream) = 0x01000000;
		img_ptr->stm_offset += H264Enc_WriteOutStream(pOutput);
		H264Enc_InitBitStream (img_ptr);
#else
		{
			uint32 * start_ptr;
			start_ptr = (uint32 *)(img_ptr->pOneFrameBitstream +img_ptr->stm_offset );
			* start_ptr = 0x01000000;
			//*((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset-or_addr_offset])) = 0x01000000;		
		}
		img_ptr->stm_offset += (OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS") >> 3);
		img_ptr->stm_offset += OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");
#endif
		img_ptr->prev_slice_bits = ((8-(img_ptr->stm_offset & 0x7))&0x7)<<3;
		img_ptr->stm_offset = (img_ptr->stm_offset+7)&0xfffffff8; // DWORD aligned
		BSM_Init();
 #endif       
	}


	//write frame
	img_ptr->slice_sz[img_ptr->slice_nr] = h264enc_slice_write(img_ptr);




#ifdef SIM_IN_WIN
	OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");

	//clear bsm-fifo, polling inactive status reg
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
	#if _CMODEL_
		clear_bsm_fifo();
	#endif
	READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");
	
	pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
	pOutput->pOutBuf = img_ptr->pOneFrameBitstream;
	
//	if( *((uint32*)img_ptr->pOneFrameBitstream) == 0xffffffff )
//		*((uint32*)img_ptr->pOneFrameBitstream) = 0x01000000;
	img_ptr->slice_sz[img_ptr->slice_nr] += (H264Enc_WriteOutStream(pOutput) << 3);
#else
	{
		uint32 * start_ptr;
			start_ptr = (uint32 *)(img_ptr->pOneFrameBitstream +img_ptr->stm_offset );
			* start_ptr = 0x01000000;
		//*((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset-or_addr_offset])) = 0x01000000;
	}
	img_ptr->slice_sz[img_ptr->slice_nr] += (OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM") << 3);
#endif

	// calculate slice size and stream offset
	{
		uint32 tmp = (64 - (img_ptr->slice_sz[img_ptr->slice_nr] & 0x3f)) & 0x3f;
		img_ptr->slice_sz[img_ptr->slice_nr] = (img_ptr->slice_sz[img_ptr->slice_nr]+63)&0xffffffc0;
		img_ptr->pic_sz += img_ptr->slice_sz[img_ptr->slice_nr];
		img_ptr->stm_offset += (img_ptr->slice_sz[img_ptr->slice_nr] >> 3);
		//img_ptr->stm_offset = (img_ptr->stm_offset+7)&0xfffffff8; // DWORD aligned
		img_ptr->slice_sz[img_ptr->slice_nr] -= tmp;
		img_ptr->slice_sz[img_ptr->slice_nr] += img_ptr->prev_slice_bits;
		img_ptr->prev_slice_bits = tmp;
		img_ptr->slice_nr++;
	}

	//rate_control
	if(img_ptr->sh.i_first_mb==0)
	{
	 	rc_update_pict(img_ptr->pic_sz, &(rc_gop_paras));
		img_ptr->pic_sz = 0;
		img_ptr->slice_nr = 0;
#ifdef _LIB
		pOutput->pOutBuf = one_frame_stream;
		pOutput->strmSize = off_frame_strm;
		off_frame_strm = 0;
#endif
		pOutput->strmSize = img_ptr->stm_offset;
		pOutput->pOutBuf = img_ptr->pOneFrameBitstream;
	}

#if defined(SIM_IN_WIN) && defined(H264_ENC) && !defined(_LIB)
	if(img_ptr->sh.i_first_mb == 0)
	{
#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_enc==FRAME_X)
#endif
		Print_DBK_Frame(img_ptr);
#endif
		WriteRecFrame(
					img_ptr->pYUVRecFrame->imgY, 
					img_ptr->pYUVRecFrame->imgUV,
				//	img_ptr->pYUVRecFrame->imgV, 
					s_pEnc_output_recon_file,
					img_ptr->width,
					img_ptr->height
					);
#ifdef VLC_TV_SPLIT
		VLCSplitDeinit(img_ptr);
#endif
	}
#endif
	//increase frame count
	if(img_ptr->sh.i_first_mb == 0)
	{
		g_nFrame_enc++;
		if(img_ptr->pYUVRecFrame->b_kept_as_ref)
		{
			img_ptr->frame_num++;
		}
		if(img_ptr->i_nal_type == NAL_SLICE_IDR)
			img_ptr->i_idr_pic_id = (img_ptr->i_idr_pic_id + 1) % 65536;
	}

	 VSP_RELEASE_Dev();
	
	return MMENC_OK;
}


/*****************************************************************************/
//  Description:   generate sps or pps header
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/

#if 1// !SPS_EN
MMEncRet H264EncGenHeader(MMEncOut *pOutput, int is_sps)
{
	ENC_IMAGE_PARAMS_T *img_ptr = g_enc_image_ptr;

	if(ARM_VSP_RST()<0)
	{
		return MMDEC_HW_ERROR;
	}

	SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");	

	
		img_ptr->stm_offset = 0;

		BSM_Init();
        
		/*if (g_nFrame_enc == 0)
		{
			h264enc_sei_version_write(img_ptr);
		}*/

        if (is_sps)
        {
		//generate sequence parameters
		h264enc_sps_write(img_ptr->sps);
            }else
           {
                h264enc_pps_write(img_ptr->pps);
            }
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(ORSC_BSM_CTRL_OFF + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
		OR1200_READ_REG_POLL(ORSC_BSM_CTRL_OFF+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		OR1200_READ_REG_POLL(ORSC_VSP_GLB_OFF+0x1C, V_BIT_1, 0x0, "ORSC: Polling AXIM_STS: not Axim_wch_busy"); //check all data has written to DDR
		
#ifdef SIM_IN_WIN
		img_ptr->stm_offset += (g_bsm_reg_ptr->TOTAL_BITS >> 3);
		OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");
	#ifdef TV_OUT
		FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
		PrintBSMOut(img_ptr);
	#endif
		clear_bsm_fifo();
		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
		pOutput->pOutBuf = img_ptr->pOneFrameBitstream;
//		if( *((uint32*)img_ptr->pOneFrameBitstream) == 0xffffffff )
//			*((uint32*)img_ptr->pOneFrameBitstream) = 0x01000000;
		img_ptr->stm_offset += H264Enc_WriteOutStream(pOutput);
		H264Enc_InitBitStream (img_ptr);
#else
		{
					uint32 * start_ptr;
			start_ptr = (uint32 *)(img_ptr->pOneFrameBitstream +img_ptr->stm_offset );
			* start_ptr = 0x01000000;
			//*((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset-or_addr_offset])) = 0x01000000;			
		}
		img_ptr->stm_offset = ( OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x14,"ORSC: TOTAL_BITS") >> 3);
		img_ptr->stm_offset += OR1200_READ_REG(ORSC_BSM_CTRL_OFF+0x2c,"ORSC: DSTUF_NUM");
#endif
		img_ptr->stm_offset = (img_ptr->stm_offset+7)&0xfffffff8; // DWORD aligned

        		pOutput->strmSize = img_ptr->stm_offset;
			pOutput->pOutBuf = img_ptr->pOneFrameBitstream;	


	 VSP_RELEASE_Dev();

    return MMENC_OK;

}
#endif

