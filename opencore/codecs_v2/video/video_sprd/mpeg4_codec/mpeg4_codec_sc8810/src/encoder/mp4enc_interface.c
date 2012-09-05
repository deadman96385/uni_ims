/******************************************************************************
 ** File Name:    mp4enc_interface.c										  *
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

/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncGenHeader(MMEncOut *pOutput)
{
	uint32 NumBits = 0;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != vol_mode_ptr);
	SCI_ASSERT(NULL != vop_mode_ptr);
	
	if(!vop_mode_ptr->short_video_header)   //MPEG-4 case
	{	
		NumBits = Mp4Enc_EncSequenceHeader(vol_mode_ptr, vop_mode_ptr);		
		NumBits += Mp4Enc_EncVOHeader(vop_mode_ptr);	
		NumBits += Mp4Enc_EncVOLHeader(vol_mode_ptr, vop_mode_ptr);
		
		//clear bsm-fifo, polling inactive status reg
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
	#if _CMODEL_
		clear_bsm_fifo();
	#endif
		VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");

		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;	
	}else
	{
		pOutput->strmSize = 0;
	}

	pOutput->pOutBuf = vop_mode_ptr->pOneFrameBitstream;

	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncSetConf(MMEncConfig *pConf)
{
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pConf);
	SCI_ASSERT(NULL != vol_mode_ptr);
	SCI_ASSERT(NULL != vop_mode_ptr);

	vol_mode_ptr->short_video_header	= pConf->h263En;
	vol_mode_ptr->ProfileAndLevel		= pConf->profileAndLevel;
	
	vop_mode_ptr->FrameRate			= pConf->FrameRate;	
	vop_mode_ptr->targetBitRate		= pConf->targetBitRate;
	vop_mode_ptr->RateCtrlEnable		= pConf->RateCtrlEnable;
	
	vop_mode_ptr->StepI				= pConf->QP_IVOP;
	vop_mode_ptr->StepP				= pConf->QP_PVOP;

	vop_mode_ptr->vbv_buf_size			= pConf->vbv_buf_size;

	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncGetConf(MMEncConfig *pConf)
{
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pConf);
	SCI_ASSERT(NULL != vol_mode_ptr);
	SCI_ASSERT(NULL != vop_mode_ptr);

	pConf->QP_IVOP = vop_mode_ptr->StepI;
	pConf->QP_PVOP = vop_mode_ptr->StepP;
	
	pConf->h263En = vol_mode_ptr->short_video_header;
	pConf->profileAndLevel = vol_mode_ptr->ProfileAndLevel;

	pConf->targetBitRate = vop_mode_ptr->targetBitRate;
	pConf->FrameRate = vop_mode_ptr->FrameRate;	
	pConf->RateCtrlEnable = vop_mode_ptr->RateCtrlEnable; 
	
	return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Close mpeg4 encoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncRelease(void)
{
// 	Mp4_CommonMemFree();
	Mp4Enc_MemFree();

// 	g_bVspInited = FALSE;
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	return MMENC_OK;
}

void MPEG4Enc_close(void)
{
	return;
}

/*****************************************************************************/
//  Description:   Init mpeg4 encoder 
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncInit(MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr, MMEncVideoInfo *pVideoFormat)
{
	uint16 frame_width = pVideoFormat->frame_width;
	uint16 frame_height = pVideoFormat->frame_height;
	VOL_MODE_T  *vol_mode_ptr = NULL;
	ENC_VOP_MODE_T  *vop_mode_ptr = NULL;
	MMEncRet init_return;
	
#ifndef _VSP_LINUX_
	if(pExtaMemBfr->size < 200 * 1024)
	{
		return 1;
	}
#endif
	
#if _CMODEL_
	VSP_Init_CModel();
#endif //_CMODEL_

	Mp4Enc_InitMem(pInterMemBfr, pExtaMemBfr);

	vol_mode_ptr = (VOL_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(VOL_MODE_T));	
	Mp4Enc_SetVolmode(vol_mode_ptr);

	vop_mode_ptr = (ENC_VOP_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_VOP_MODE_T)); 
	Mp4Enc_SetVopmode(vop_mode_ptr);
	
	vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header  = pVideoFormat->is_h263;
	vop_mode_ptr->uv_interleaved = pVideoFormat->uv_interleaved = 1; //modified by xw, 20100511
	
	vol_mode_ptr->VolWidth = frame_width;
	vol_mode_ptr->VolHeight = frame_height;

	if (frame_width > 352 && frame_height > 288)
	{
		vop_mode_ptr->big_size_flag = TRUE;
	}else
	{
		vop_mode_ptr->big_size_flag = FALSE;
	}

	//init global param
	g_enc_last_modula_time_base = 0;
	g_enc_tr = 0;
	g_is_yuv_frm_malloced = FALSE;
	g_enc_is_prev_frame_encoded_success = FALSE;
	g_enc_p_frame_count = 0;	

	Mp4Enc_InitVolVopPara(vol_mode_ptr, vop_mode_ptr, pVideoFormat->time_scale);		

	Mp4Enc_InitSession(vol_mode_ptr, vop_mode_ptr); 

	if(!pVideoFormat->is_h263)
	{
		uint32 cmd;
		
		VSP_Reset ();

		/*clear time_out int_raw flag, if timeout occurs*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");

		/*init dcam command*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register");
		
		/*init vsp command*/	
		cmd  = (1<<15) |(1<<14)|(1<<13)|(1<<12);
		cmd |= ( (!vop_mode_ptr->short_video_header) ? (1<<8) : 0 ); 
		cmd |= ( (1<<7) | (0<<6) | (1<<5) | (1<<4) |
		         (0<<3) | (1<<2) | (1<<1) | (1<<0) );
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: init the global register, little endian");

		cmd = (0 << 16) |((uint32)0xffff);
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

		cmd = (vop_mode_ptr->uv_interleaved)?1:0;     /// 1:uv_interleaved, two plane, 0: three plane
		cmd = (cmd<<8)/*|0xff*/;	//NEED NO GAP @xweluo@20120209
		VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_BURST_GAP_OFFSET, cmd, "configure AHB register: BURST_GAP, and frame_mode");

		Mp4Enc_InitBitStream(vop_mode_ptr);
	}

	init_return = MMENC_OK;
	
	return init_return;
}

/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncStrmEncode(MMEncIn *pInput, MMEncOut *pOutput)
{
	uint32 cmd;
	int32 ret;
	VOP_PRED_TYPE_E frame_type;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();
	BOOLEAN *pIs_prev_frame_success = &g_enc_is_prev_frame_encoded_success;
	BOOLEAN frame_skip = FALSE;

	if (vop_mode_ptr->big_size_flag)
	{
		vop_mode_ptr->RateCtrlEnable = FALSE;
	}
		
	VSP_Reset();

	/*clear time_out int_raw flag, if timeout occurs*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");
	
	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

	/*init vsp command*/
	cmd  = (1<<15) |(1<<14)|(1<<13)|(1<<12);
	cmd |= ( (!vop_mode_ptr->short_video_header) ? (1<<8): 0 );
	cmd |= ( (1<<7) | (0<<6) | (1<<5) | (1<<4) |
	         (0<<3) | (1<<2) | (1<<1) | (1<<0) );
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: little endian");

	cmd = (0 << 16) |((uint32)0xffff);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

	cmd = (vop_mode_ptr->uv_interleaved)?1:0;///1: uv_interleaved, two plane, 0: three plane
	cmd = (cmd<<8)/*|0xff*/;
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_BURST_GAP_OFFSET, cmd, "configure AHB register: BURST_GAP, and frame_mode");

	vop_mode_ptr->mbline_num_slice	= 1;
	vop_mode_ptr->intra_mb_dis		= 30;	
	vop_mode_ptr->sliceNumber = 0;	
	
	if(!vop_mode_ptr->bInitRCSuceess)
	{
		Mp4Enc_InitRateCtrl(&g_rc_par, &g_stat_rc);
		vop_mode_ptr->bInitRCSuceess = TRUE;
        g_nFrame_enc = 0;
		g_enc_last_modula_time_base = pInput->time_stamp / (int32)vol_mode_ptr->ClockRate; //the first frame
	}
    g_nFrame_enc++;

	if(pInput->vopType == IVOP)
	{
		g_rc_par.p_count = 0;
	}

	
	g_stat_rc.B				= pInput->bs_remain_len;
    
	frame_type 				= Mp4Enc_JudgeFrameType (&g_rc_par, &g_stat_rc);
	vop_mode_ptr->VopPredType	= (pInput->vopType == IVOP)?IVOP:frame_type;
	frame_skip				=  (vop_mode_ptr->VopPredType == NVOP) ? 1 : 0;
	
	PRINTF("%d ", g_nFrame_enc);
	if(!frame_skip)
	{	
		g_stat_rc.be_re_enc		= FALSE;
		g_stat_rc.be_skip_frame = FALSE;

#ifdef RE_ENC_SHEME
FRAME_ENC:
#endif
		Mp4Enc_InitRCFrame (&g_rc_par);
		
		Mp4Enc_InitBitStream(vop_mode_ptr);

#if defined(_SIMULATION_) 
	{
		uint32 frame_width = vop_mode_ptr->FrameWidth;
		uint32 frame_height = vop_mode_ptr->FrameHeight;
		uint32 frame_size = frame_width * frame_height;

		memcpy(vop_mode_ptr->pYUVSrcFrame->imgY, pInput->p_src_y, frame_size);
		if (!vop_mode_ptr->uv_interleaved)
		{
			memcpy(vop_mode_ptr->pYUVSrcFrame->imgU, pInput->p_src_y+frame_size, frame_size/4);
			memcpy(vop_mode_ptr->pYUVSrcFrame->imgV, pInput->p_src_y+frame_size*5/4, frame_size/4);
		}else
		{
			memcpy(vop_mode_ptr->pYUVSrcFrame->imgU, pInput->p_src_y+frame_size, frame_size/2);
		}
	}
#else
        //--y,u,v address should be 256 bytes aligned. YUV420.
        vop_mode_ptr->pYUVSrcFrame->imgY = pInput->p_src_y;
        vop_mode_ptr->pYUVSrcFrame->imgU = pInput->p_src_u;
        vop_mode_ptr->pYUVSrcFrame->imgV = pInput->p_src_v;

#ifdef _VSP_LINUX_
        vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint32)pInput->p_src_y_phy>> 8;
        vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint32)pInput->p_src_u_phy>> 8;
        vop_mode_ptr->pYUVSrcFrame->imgVAddr = (uint32)pInput->p_src_v_phy>> 8;
#else
        vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgY >> 8;
        vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgU >> 8;
        vop_mode_ptr->pYUVSrcFrame->imgVAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgV >> 8;
#endif

#endif

	 //optimize for android 4.0 8810 encoder can not meet the requirement of D1 30Hz 
	 if(vop_mode_ptr->big_size_flag){
	 	if((IVOP != vop_mode_ptr->VopPredType)&&(0==g_nFrame_enc%3)){
			vop_mode_ptr->VopPredType = NVOP;
	 	}
	 }
	 //optimize for android 4.0 8810 encoder end

		if(IVOP == vop_mode_ptr->VopPredType)
		{
		#if defined(SIM_IN_WIN)
			FPRINTF(g_rgstat_fp, "\nNo.%d Frame:\t I VOP\n", g_nFrame_enc);
		//	FPRINTF(g_fp_trace_fw,"\t I VOP\n");
		#endif
			PRINTF ("No.%d Frame:\t IVOP\n", g_nFrame_enc);
			Mp4Enc_InitRCGOP (&g_rc_par);
			ret = Mp4Enc_EncIVOP(vop_mode_ptr, pInput->time_stamp);				
		    g_enc_p_frame_count = vol_mode_ptr->PbetweenI;
		}else if (PVOP == vop_mode_ptr->VopPredType)
		{
		#if defined(SIM_IN_WIN)
			FPRINTF(g_rgstat_fp, "\nNo.%d Frame:\t P VOP\n", g_nFrame_enc);
//			FPRINTF(g_fp_trace_fw,"\t P VOP\n");
		#endif
			PRINTF ("No.%d Frame:\t PVOP\n", g_nFrame_enc);

			g_enc_p_frame_count--;

			ret = g_EncPVOP(vop_mode_ptr, pInput->time_stamp);
		}else
		{
			g_enc_p_frame_count--;

			ret = Mp4Enc_EncNVOP(vop_mode_ptr, pInput->time_stamp);
		}

		(*pIs_prev_frame_success) = ret;
	
		//clear bsm-fifo, polling inactive status reg
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
	#if _CMODEL_
		clear_bsm_fifo();
	#endif
		VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");

		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
		pOutput->pOutBuf = vop_mode_ptr->pOneFrameBitstream;

		g_rc_par.nbits_total = pOutput->strmSize<<3;

		if (vop_mode_ptr->RateCtrlEnable)
		{
			int32 Ec_Q8 = (g_rc_par.sad/vop_mode_ptr->MBNum);	

			if (Ec_Q8 < 256)  //to avoid devided by zero
				Ec_Q8 = 256;

#ifdef RE_ENC_SHEME
			if(vop_mode_ptr->FrameWidth<=176)
			{//jgdu
				Mp4Enc_AnalyzeEncResult(&g_stat_rc, g_rc_par.nbits_total, vop_mode_ptr->VopPredType, Ec_Q8);

				if (g_stat_rc.be_re_enc)
				{
					if (g_stat_rc.be_scene_cut)
					{
						vop_mode_ptr->VopPredType = IVOP;
						g_rc_par.p_count = g_rc_par.p_between_i;
					}

					PRINTF("\t bisTotalCur: %d, frame re-encoded\n", g_rc_par.nbits_total);

					g_re_enc_frame_number++;

					goto FRAME_ENC;
				}

				if (g_stat_rc.be_skip_frame)
				{
					pOutput->strmSize = 0;
					g_stat_rc.is_pfrm_skipped = TRUE;
					PRINTF("\t bitsTotalCur: %d, frame skipped\n", g_rc_par.nbits_total);
					return MMENC_OK;
				}
			}
#endif
			
			g_stat_rc.is_pfrm_skipped = FALSE;


			if (IVOP == vop_mode_ptr->VopPredType)
			{
				Mp4Enc_ResetRCModel (vop_mode_ptr, &g_stat_rc, &g_rc_par);
			}
			else
			{						
				Mp4Enc_UpdateRCModel (vop_mode_ptr, &g_stat_rc, &g_rc_par, Ec_Q8);
			}
		}

#if 0//_DEBUG_
#if defined(SIM_IN_WIN) && defined(MPEG4_ENC)
//		Mp4_WriteRecFrame(
//						vop_mode_ptr->pYUVRecFrame->imgY, 
//						vop_mode_ptr->pYUVRecFrame->imgU,
//						vop_mode_ptr->pYUVRecFrame->imgV, 
//						s_pEnc_output_recon_file,
//						vop_mode_ptr->FrameWidth,
//						vop_mode_ptr->FrameHeight
//						);

		//write source frame out for computing psnr in decoder
//		Mp4_WriteRecFrame (
//						vop_mode_ptr->pYUVSrcFrame->imgY, 
//						vop_mode_ptr->pYUVSrcFrame->imgU,
//						vop_mode_ptr->pYUVSrcFrame->imgV, 
//						s_fp_src_enc,
//						vop_mode_ptr->FrameWidth,
//						vop_mode_ptr->FrameHeight
//						);

// 		Mp4Enc_ComputeSNR(vop_mode_ptr->pYUVSrcFrame, vop_mode_ptr->pYUVRecFrame);
#endif
#endif
		
		//update ref frame
		Mp4Enc_UpdateRefFrame(vop_mode_ptr);
	}else
	{
		g_stat_rc.is_pfrm_skipped = TRUE;
		pOutput->strmSize = 0;
		PRINTF ("\nNo. %d Frame:\t skipped\n\n", g_nFrame_enc);

	#if defined(SIM_IN_WIN)
		FPRINTF (g_rgstat_fp, "\nNo. %d Frame:\t skipped\n\n", g_nFrame_enc);
	#endif
	}
		
	return MMENC_OK;
}

/*****************************************************************************/
//  Description: check whether VSP can used for video encoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN MPEG4ENC_VSP_Available(void)
{
	int dcam_cfg;

	dcam_cfg = VSP_READ_REG(VSP_DCAM_BASE+DCAM_CFG_OFF, "DCAM_CFG: read dcam configure register");

	if (((dcam_cfg >> 3) & 1) == 0)
		return TRUE;
	else
		return FALSE;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
