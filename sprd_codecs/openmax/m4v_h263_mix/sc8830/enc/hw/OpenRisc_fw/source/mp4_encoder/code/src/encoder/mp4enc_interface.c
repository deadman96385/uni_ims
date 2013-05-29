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
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pVol_mode);
	SCI_ASSERT(NULL != pVop_mode);
#ifdef _LIB
	memset(pVop_mode->pOneFrameBitstream, 0, pVop_mode->OneframeStreamLen);
#endif
	if(!pVop_mode->short_video_header)   //MPEG-4 case
	{	
		NumBits = Mp4Enc_EncSequenceHeader(pVol_mode, pVop_mode);
		NumBits += Mp4Enc_EncVOHeader(pVop_mode);	
		NumBits += Mp4Enc_EncVOLHeader(pVol_mode, pVop_mode);
	//	NumBits += Mp4Enc_OutputLeftBits();	
#ifdef SIM_IN_WIN
		pVop_mode->stm_offset += ((g_bsm_reg_ptr->TOTAL_BITS + 7) >> 3);		
		//clear bsm-fifo, polling inactive status reg
		VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
	#if _CMODEL_
		clear_bsm_fifo();
	#endif
		READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");
		{
//			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR, V_BIT_1, V_BIT_1, "ORSC: Polling VLC_FRM_DONE "); //check vlc frame done
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		}
		pOutput->strmSize = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read total bits") + 7 ) >>3;
		pOutput->strmSize = (pOutput->strmSize+7)&0xfffffff8; // DWORD aligned
#else
		{
//			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR, V_BIT_1, V_BIT_1, "ORSC: Polling VLC_FRM_DONE "); //check vlc frame done
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		}
		//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+0x08, V_BIT_1, "clear bsm-fifo");
		//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x00, V_BIT_31, V_BIT_31, "polling BSMW inactive status");
		//pVop_mode->stm_offset = (OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC_SHARE: read total bits") + 7) >> 3;
		//pVop_mode->stm_offset += OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x2c,"ORSC: DSTUF_NUM");
		//pVop_mode->stm_offset = (pVop_mode->stm_offset+7)&0xfffffff8; // DWORD aligned

		pOutput->strmSize = (OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC_SHARE: read total bits") + 7) >> 3;
#endif
		
	}else
	{
		pOutput->strmSize = 0;
	}
#ifdef OUTPUT_TEST_VECTOR
	if (!pVop_mode->short_video_header)
	{
		if(g_fp_bsm_total_bits_tv != NULL)
			fprintf(g_fp_bsm_total_bits_tv, "%08x\n", NumBits);
	}
#endif

	pOutput->pOutBuf = pVop_mode->pOneFrameBitstream;

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
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pConf);
	SCI_ASSERT(NULL != pVol_mode);
	SCI_ASSERT(NULL != pVop_mode);

	pVol_mode->short_video_header	= pConf->h263En;
	pVol_mode->ProfileAndLevel		= pConf->profileAndLevel;
	
	pVop_mode->FrameRate			= pConf->FrameRate;	
	pVop_mode->targetBitRate		= pConf->targetBitRate;
	pVop_mode->RateCtrlEnable		= pConf->RateCtrlEnable;
	
	pVop_mode->StepI				= pConf->QP_IVOP;
	pVop_mode->StepP				= pConf->QP_PVOP;

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
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();

	SCI_ASSERT(NULL != pConf);
	SCI_ASSERT(NULL != pVol_mode);
	SCI_ASSERT(NULL != pVop_mode);

	pConf->QP_IVOP = pVop_mode->StepI;
	pConf->QP_PVOP = pVop_mode->StepP;
	
	pConf->h263En = pVol_mode->short_video_header;
	pConf->profileAndLevel = pVol_mode->ProfileAndLevel;

	pConf->targetBitRate = pVop_mode->targetBitRate;
	pConf->FrameRate = pVop_mode->FrameRate;	
	pConf->RateCtrlEnable = pVop_mode->RateCtrlEnable; 
	
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
MMEncRet MP4EncInit(MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat)
{
	uint16 frame_width = pVideoFormat->frame_width;
	uint16 frame_height = pVideoFormat->frame_height;
	VOL_MODE_T  *vol_mode_ptr = NULL;
	ENC_VOP_MODE_T  *vop_mode_ptr = NULL;
	MMEncRet init_return;


	Mp4Enc_InitMem(pInterMemBfr, pExtaMemBfr);
	vol_mode_ptr = (VOL_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(VOL_MODE_T));	
	Mp4Enc_SetVolmode(vol_mode_ptr);
	vop_mode_ptr = (ENC_VOP_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_VOP_MODE_T)); 
	Mp4Enc_SetVopmode(vop_mode_ptr);
	
	vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header  = pVideoFormat->is_h263;
	vop_mode_ptr->uv_interleaved = pVideoFormat->uv_interleaved;
	
	vol_mode_ptr->VolWidth = frame_width;
	vol_mode_ptr->VolHeight = frame_height;

	g_enc_last_modula_time_base = 0;
	g_enc_tr = 0;
	g_enc_is_prev_frame_encoded_success = FALSE;
	g_enc_p_frame_count = 0;	
	g_nFrame_enc = 0;

	Mp4Enc_InitVolVopPara(vol_mode_ptr, vop_mode_ptr, pVideoFormat->time_scale);
	Mp4Enc_InitSession(vol_mode_ptr, vop_mode_ptr); 
	vop_mode_ptr->pOneFrameBitstream = pBitstreamBfr->common_buffer_ptr;
	vop_mode_ptr->OneframeStreamLen = pBitstreamBfr->size;
	return MMENC_OK;
}
#if defined(_LIB)
void Mp4_WriteRecFrame_LIB(uint8 *pRec_Y, uint8 *pRec_U, uint8 *pRec_V, uint32 frame_width, uint32 frame_height)
{
	uint32 size = frame_width * frame_height;
	FILE *rec_file = fopen("D:\\james.chen\\rec_file.yuv", "ab");
	fwrite(pRec_Y,1,size, rec_file);
	
	/*if (!g_uv_interleaved)
	{
		size = size>>2;
		fwrite(pRec_U,1,size, rec_file);
		fwrite(pRec_V,1,size, rec_file);
	}else*/
	{
		size = size>>1;
		fwrite(pRec_U,1,size, rec_file);
	}
	
	fclose(rec_file);
	return;
}
#endif
/*****************************************************************************/
//  Description:   Encode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMEncRet MP4EncStrmEncode(MMEncIn *pInput, MMEncOut *pOutput)
{
	int32 ret;
	VOP_PRED_TYPE_E frame_type;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();
	BOOLEAN *pIs_prev_frame_success = &g_enc_is_prev_frame_encoded_success;
	uint32 frame_width = pVop_mode->FrameWidth;
	uint32 frame_height = pVop_mode->FrameHeight;
	uint32 frame_size = frame_width * frame_height;
	BOOLEAN frame_skip = FALSE;


	pVop_mode->mbline_num_slice	= 5;//pVop_mode->FrameHeight/MB_SIZE;
	pVop_mode->intra_mb_dis		= 30;	

	if(!pVop_mode->bInitRCSuceess)
	{
		Mp4Enc_InitRateCtrl(&g_rc_par, &g_stat_rc);
		pVop_mode->bInitRCSuceess = TRUE;
	}

	if(pInput->vopType == IVOP)
	{
		g_rc_par.p_count = 0;
	}

	frame_type 				= Mp4Enc_JudgeFrameType (&g_rc_par, &g_stat_rc);
	pVop_mode->VopPredType	= (pInput->vopType == IVOP)?IVOP:frame_type;
	frame_skip				= (frame_type == NVOP) ? 1 : 0;

	pVop_mode->pYUVSrcFrame->imgY = pInput->p_src_y;
	pVop_mode->pYUVSrcFrame->imgYAddr = (uint32)pVop_mode->pYUVSrcFrame->imgY / 8;
	pVop_mode->pYUVSrcFrame->imgU = pInput->p_src_u;
	pVop_mode->pYUVSrcFrame->imgUAddr = (uint32)pVop_mode->pYUVSrcFrame->imgU / 8;			
	
	PRINTF("%d ", g_nFrame_enc);
	g_nFrame_enc++;
	
	if(!frame_skip)
	{	
		g_stat_rc.be_re_enc		= FALSE;
		g_stat_rc.be_skip_frame = FALSE;

#ifdef RE_ENC_SHEME
FRAME_ENC:
#endif
		Mp4Enc_InitRCFrame (&g_rc_par);



		if(IVOP == frame_type)
		{

			Mp4Enc_InitRCGOP (&g_rc_par);
			ret = Mp4Enc_EncIVOP(pVop_mode, pInput->time_stamp);
		    g_enc_p_frame_count = pVol_mode->PbetweenI;
		}
		else
		{

			g_enc_p_frame_count--;

			ret = Mp4Enc_EncPVOP(pVop_mode, pInput->time_stamp);
		}

		(*pIs_prev_frame_success) = ret;

		//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR, V_BIT_1, V_BIT_1, "ORSC: Polling VLC_FRM_DONE "); //check vlc frame done
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
		

		//pVop_mode->stm_offset += (OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC_SHARE: read total bits") + 7)>>3;

		//pVop_mode->stm_offset += pOutput->strmSize;

		//pVop_mode->stm_offset += OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x2c,"ORSC: DSTUF_NUM");
		//pVop_mode->stm_offset = (pVop_mode->stm_offset+7)&0xfffffff8; // DWORD aligned
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x20, pVop_mode->stm_offset,"shareRAM 0x20 stream_len");
		//g_rc_par.nbits_total = pVop_mode->stm_offset<<3;

		pOutput->strmSize = (OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC_SHARE: read total bits") + 7)>>3;

		pOutput->pOutBuf = pVop_mode->pOneFrameBitstream;

		g_rc_par.sad = 10000;

		g_rc_par.nbits_total = pOutput->strmSize<<3;

		if (pVop_mode->RateCtrlEnable)
		{
			int32 Ec_Q8 = (g_rc_par.sad/pVop_mode->MBNum);

#ifdef RE_ENC_SHEME
			Mp4Enc_AnalyzeEncResult(&g_stat_rc, g_rc_par.nbits_total, frame_type, Ec_Q8);

			if (g_stat_rc.be_re_enc)
			{
				if (g_stat_rc.be_scene_cut)
				{
					frame_type = IVOP;
					g_rc_par.p_count = g_rc_par.p_between_i;
				}

				PRINTF("\t bisTotalCur: %d, frame re-encoded\n", g_rc_par.nbits_total);

				g_re_enc_frame_number++;

				goto FRAME_ENC;
			}

			if (g_stat_rc.be_skip_frame)
			{
				pOutput->strmSize = 0;
				PRINTF("\t bitsTotalCur: %d, frame skipped\n", g_rc_par.nbits_total);
				return MMENC_OK;
			}
#endif

			if (IVOP == frame_type)
			{
				Mp4Enc_ResetRCModel (pVop_mode, &g_stat_rc, &g_rc_par);
			}
			else
			{						
				Mp4Enc_UpdateRCModel (pVop_mode, &g_stat_rc, &g_rc_par, Ec_Q8);
			}
		}

	
		//update ref frame
		Mp4Enc_UpdateRefFrame(pVop_mode);
	}else
	{
		pOutput->strmSize = 0;		
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

	return TRUE;

}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
