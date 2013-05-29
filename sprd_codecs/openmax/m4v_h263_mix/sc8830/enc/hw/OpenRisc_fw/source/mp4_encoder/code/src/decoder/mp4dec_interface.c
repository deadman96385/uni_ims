/******************************************************************************
 ** File Name:    mp4dec_interface.c  		                                  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         08/25/2008                                                  *
 ** Copyright:    2008 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
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

#define    DECODE_BUFFER_SIZE    480 * 1024  //@Zhemin.Lin, CR49137

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecInit(MMCodecBuffer *pBuffer, MMDecVideoFormat *video_format_ptr)
{
	MMDecRet is_init_success = MMDEC_OK;
	DEC_VOP_MODE_T *vop_mode_ptr = NULL;
	H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr = NULL;

	Mp4Dec_InitInterMem(pBuffer);
	
	Mp4Dec_InitGlobal();

	vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_VOP_MODE_T));
	SCI_ASSERT(NULL != vop_mode_ptr);	
	Mp4Dec_SetVopmode(vop_mode_ptr);

	g_dec_is_first_frame = TRUE;
	g_dec_is_stop_decode_vol = FALSE;
	g_dec_is_changed_format = FALSE;	
	vop_mode_ptr->error_flag = FALSE;	

	//for H263 plus header
	h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(sizeof(H263_PLUS_HEAD_INFO_T));
	SCI_ASSERT(NULL != h263_plus_head_info_ptr);
	Mp4Dec_SetH263PlusHeadInfo(h263_plus_head_info_ptr);
	
	Mp4Dec_InitDecoderPara(vop_mode_ptr);	

	vop_mode_ptr->video_std = video_format_ptr->video_std;
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std != MPEG4)
	{
		PRINTF("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{
		uint32 cmd;
		
		VSP_Reset ();

		/*clear time_out int_raw flag, if timeout occurs*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");

		/*init dcam command*/
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3) , "DCAM_CFG: configure DCAM register");
		
		/*init vsp command*/	
		cmd = (0<<16) | (1<<15) |(0<<14)|(1<<12) | (VSP_MPEG4<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: init the global register");

		cmd = (0 << 16) |((uint32)0xffff);
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

		if(video_format_ptr->i_extra > 0)
		{
			PRINTF("\nIt is MPEG-4 bitstream!\n");
			
			Mp4Dec_InitBitstream(video_format_ptr->p_extra, video_format_ptr->i_extra);
			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);
		}

	#if 0 //removed to MP4DecDecode() function.
		if(MMDEC_OK == is_init_success)
		{
			Mp4Dec_InitSessionDecode(vop_mode_ptr);
			vop_mode_ptr->bInitSuceess = TRUE;
		}
	#endif	
	}

	return is_init_success;
}

void PrintfRecFrame (DEC_VOP_MODE_T * vop_mode_ptr)
{
#if defined(WIN32)
	int			i;
	uint32		val;
	uint32 *	p_frame_ptr;
	int			frame_size;

	int			frame_width  = vop_mode_ptr->FrameWidth;
	int			frame_height = vop_mode_ptr->FrameHeight;

	//printf Y
	frame_size = (frame_width/4) * frame_height;
	p_frame_ptr = (uint32 *)vop_mode_ptr->pCurRecFrame->pDecFrame->imgY;
	for (i = 0; i < frame_size; i++)
	{
		val = p_frame_ptr[i];
		fprintf (g_fp_rec_frm_tv, "%08x\n", val);
	}

	//printf UV
	frame_size = (frame_width/4) * (frame_height/2);
	p_frame_ptr = (uint32 *)vop_mode_ptr->pCurRecFrame->pDecFrame->imgU;
	for (i = 0; i < frame_size; i++)
	{
		val = p_frame_ptr[i];
		fprintf (g_fp_rec_frm_tv, "%08x\n", val);
	}
#endif
}
/*****************************************************************************/
//  Description:   Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	uint32 cmd;
	MMDecRet ret = MMDEC_ERROR;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

	VSP_Reset();

	/*clear time_out int_raw flag, if timeout occurs*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");
	
	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

	/*init vsp command*/
	cmd = (0<<16) |(1<<15) | (0<<14) |(1<<12) | ((vop_mode_ptr->video_std)<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0");

	cmd = (0 << 16) |((uint32)0xffff);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

	Mp4Dec_InitBitstream(dec_input_ptr->pStream, dec_input_ptr->dataLen);

	if(ITU_H263 == vop_mode_ptr->video_std)
	{
		ret = Mp4Dec_DecH263Header(vop_mode_ptr);
	}else if(MPEG4 == vop_mode_ptr->video_std)
	{
		ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
	}else
	{
		ret = Mp4Dec_FlvH263PicHeader(vop_mode_ptr);
	}

	if(ret != MMDEC_OK)
	{
		return MMDEC_ERROR;
	}

	if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
	{
		g_nFrame_dec++;
		return MMDEC_FRAME_SEEK_IVOP;
	}

	dec_output_ptr->frameEffective = FALSE;
	dec_output_ptr->pOutFrameY = PNULL;
	dec_output_ptr->pOutFrameU = PNULL;
	dec_output_ptr->pOutFrameV = PNULL;
	
	if(!vop_mode_ptr->bInitSuceess)
	{
		Mp4Dec_InitSessionDecode(vop_mode_ptr);
		vop_mode_ptr->bInitSuceess = TRUE;
	}

	if(vop_mode_ptr->VopPredType != NVOP)
	{
		if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
		{
			return MMDEC_OUTPUT_BUFFER_OVERFLOW;
		}	
		
		if (vop_mode_ptr->post_filter_en)
		{
			if(dec_input_ptr->beDisplayed)
			{
				if(!Mp4Dec_GetCurDispFrameBfr(vop_mode_ptr))
				{
					return MMDEC_OUTPUT_BUFFER_OVERFLOW;
				}
			}
		}
	}else //NVOP
	{
		PRINTF ("frame not coded!\n");
		
	#if defined(_FPGA_AUTO_VERIFICATION_) 
		waiting_n_ms(60);
		while(!read_pc_finished_dec_one_frame_msg())
		{
			;
		}
	#endif //_FPGA_AUTO_VERIFICATION_
		return MMDEC_OK;
	}

#if _DEBUG_
	if(g_dispFrmNum == 30)
	{
		foo();
	}

	if(g_nFrame_dec == 46)
	{
		foo();
	}
#endif //	

	Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);

	if(IVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t I VOP\t%d\n", dec_input_ptr->dataLen); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: IVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		if(!vop_mode_ptr->bDataPartitioning)
		{
			Mp4Dec_DecIVOP(vop_mode_ptr); 
		}
	#ifdef DATA_PARTITION	
		else
		{
			Mp4Dec_DecIVOPErrResDataPartitioning(vop_mode_ptr);
		}
	#endif //DATA_PARTITION	

		if (dec_input_ptr->err_pkt_num)
		{
     	 	Mp4Dec_EC_IVOP(vop_mode_ptr);
		}
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t P VOP\t%d\n", dec_input_ptr->dataLen); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: PVOP\n", g_nFrame_dec);
	#endif //_TRACE_	

		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");

		if(!vop_mode_ptr->bDataPartitioning)
		{
			Mp4Dec_DecPVOP(vop_mode_ptr) ;
		}
	#ifdef DATA_PARTITION	
		else
		{
			Mp4Dec_DecPVOPErrResDataPartitioning(vop_mode_ptr);
		}
	#endif //DATA_PARTITION	

		if (dec_input_ptr->err_pkt_num)
		{
		    VSP_Reset();

        	/*clear time_out int_raw flag, if timeout occurs*/
        	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");
        	
        	/*init dcam command*/
        	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

        	/*init vsp command*/
        	cmd = (0<<16) |(1<<15) | (0<<14) |(0<<12) | ((vop_mode_ptr->video_std)<<8) | (1<<7) | (0<<6) | (0<<3) | (1<<2) | (0<<1) | (0<<0);
        	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0, disable dbk/mbc out");

        	cmd = (0 << 16) |((uint32)0xffff);
        	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");
	
	        Mp4Dec_VspFrameInit(vop_mode_ptr);
	        
	        VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
	
   	 		Mp4Dec_EC_PVOP(vop_mode_ptr);
		}
	}else if(SVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t S VOP, Don't SUPPORTED!\n"); 
//		PRINTF ("\t S VOP!\n"); 
	#if _TRACE_		 	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: SVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		
//		Mp4Dec_DecPVOP(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
	}else if(NVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("frame not coded!\n");
		return MMDEC_OK;
	}else
	{
		PRINTF ("\t B VOP\t%d\n", dec_input_ptr->dataLen);
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: BVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
		vop_mode_ptr->err_MB_num = 0;
		Mp4Dec_DecBVOP(vop_mode_ptr);
	}
	PrintfRecFrame (vop_mode_ptr);
	
	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;

	if(READ_REG_POLL(VSP_DBK_REG_BASE+DBK_CTR1_OFF, V_BIT_0, 0, TIME_OUT_CLK, "DBK_CTR1: polling DBK_CFG_FLAG flag = 0"))
	{
		vop_mode_ptr->error_flag = TRUE;
		return MMDEC_HW_ERROR;
	}
		
	if(READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
		return MMDEC_HW_ERROR;
	}
	
	VSP_WRITE_REG(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec, "VSP_TST: configure frame_cnt to debug register for end of picture");

#if defined(_FPGA_AUTO_VERIFICATION_) 
	waiting_n_ms(60);
	while(!read_pc_finished_dec_one_frame_msg())
	{
		;
	}
		
	{
		uint32 yDataLen = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
		uint32 uvDataLen = yDataLen/4;
			
		send_one_frm_data(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY, vop_mode_ptr->pCurDispFrame->pDecFrame->imgU,
						vop_mode_ptr->pCurDispFrame->pDecFrame->imgV, yDataLen, uvDataLen);
	}
#endif //_FPGA_AUTO_VERIFICATION_	

	if(vop_mode_ptr->error_flag)
	{
		PRINTF ("decode vop error!\n");
// 		g_nFrame_dec++;
	
// 		return MMDEC_ERROR;
	}

// 	if(MMDEC_OK == ret)
	{	
		Mp4DecStorablePic *pic = PNULL;
		
		/*output frame for display*/
		if(vop_mode_ptr->VopPredType != BVOP)
		{
			if(g_nFrame_dec > 0)
			{
				/*send backward reference (the lastest reference frame) frame's YUV to display*/
				pic = vop_mode_ptr->pBckRefFrame;
			}		
		}else
		{
			/*send current B frame's YUV to display*/
			pic = vop_mode_ptr->pCurRecFrame;
		}

		if (vop_mode_ptr->post_filter_en)
		{
			DEC_FRM_BFR *display_frame = PNULL;

			/*get display frame from display queue*/
			display_frame = Mp4Dec_GetDispFrameBfr(pic);
			if(PNULL != display_frame)
			{
				dec_output_ptr->pOutFrameY = display_frame->imgY;
				dec_output_ptr->pOutFrameU = display_frame->imgU;
 				dec_output_ptr->pOutFrameV = display_frame->imgV;
				dec_output_ptr->is_transposed = 0;
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
				dec_output_ptr->frameEffective = 1;
				dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;

//ONLY FOR AUTO FPGA VERIFICATION, BEGIN
		#if _CMODEL_&& defined(_FPGA_AUTO_VERIFICATION_)	
				MPEG4_DecReleaseDispBfr(dec_output_ptr->pOutFrameY); //only for AUTO FPGA VERIFICATION
		#endif
//ONLY FOR AUTO FPGA VERIFICATION, END		

			}else
			{
				dec_output_ptr->frameEffective = 0;
			}
		}else
		{
			if(pic != PNULL)
			{
				dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
				dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
				dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
				dec_output_ptr->is_transposed = 0;
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
				dec_output_ptr->frameEffective = 1;
				dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;
				pic->pDecFrame->bDisp = TRUE; 
			}else
			{
				dec_output_ptr->frameEffective = 0;
			}
		}
		
//ONLY FOR AUTO FPGA VERIFICATION, BEGIN
#if _CMODEL_&& defined(_FPGA_AUTO_VERIFICATION_)		
		dec_output_ptr->pOutFrameY = vop_mode_ptr->pCurDispFrame->pDecFrame->imgY;
		dec_output_ptr->pOutFrameU = vop_mode_ptr->pCurDispFrame->pDecFrame->imgU;
		dec_output_ptr->pOutFrameV = vop_mode_ptr->pCurDispFrame->pDecFrame->imgV;
		dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
		dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
		dec_output_ptr->frameEffective = 1;
#endif
//ONLY FOR AUTO FPGA VERIFICATION, END		

		Mp4Dec_exit_picture(vop_mode_ptr);
	}

	g_nFrame_dec++;

	return ret;
}

/*****************************************************************************/
//  Description: for display, return one frame for display
//	Global resource dependence: 
//  Author:        
//	Note:  the transposed type is passed from MMI "req_transposed"
//         req_transposed£º 1£ºtranposed  0: normal    
/*****************************************************************************/
PUBLIC void mpeg4dec_GetOneDspFrm(MMDecOutput *pOutput, int req_transposed, int is_last_frame)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	Mp4DecStorablePic *pic;

	if(is_last_frame)
	{
		//notes: if the last frame is bvop, then the last display frame should be it's latest reference frame(pBckRefFrame).
		//else, the last display frame should be last decoded I or P vop. In MP4DecDecode function, Mp4Dec_exit_picture has 
		//exchanged pCurRecFrame to pBckRefFrame. so here use pBckRefFrame is correct.
		pic = vop_mode_ptr->pBckRefFrame;
	}else
	{
		pic = vop_mode_ptr->pFrdRefFrame;
	}

	if(PNULL != pic)
	{
		pic->pDecFrame->bDisp = TRUE; 
		pOutput->frame_width = vop_mode_ptr->FrameWidth;
		pOutput->frame_height = vop_mode_ptr->FrameHeight;
		pOutput->frameEffective = 1;
		pOutput->is_transposed = 0;
		pOutput->pOutFrameY = pic->pDecFrame->imgY;
		pOutput->pOutFrameU = pic->pDecFrame->imgU;
		pOutput->pOutFrameV = pic->pDecFrame->imgV;

		return;
	}else
	{
		pOutput->frameEffective = 0;

		return;
	}

	return;
}

/*****************************************************************************/
//  Description: check whether VSP can used for video decoding or not
//	Global resource dependence: 
//  Author:        
//	Note: return VSP status:
//        1: dcam is idle and can be used for vsp   0: dcam is used by isp           
/*****************************************************************************/
BOOLEAN MPEG4DEC_VSP_Available(void)
{
	int dcam_cfg;

	dcam_cfg = VSP_READ_REG(VSP_DCAM_BASE+DCAM_CFG_OFF, "DCAM_CFG: read dcam configure register");

	if (((dcam_cfg >> 3) & 1) == 0) //bit3: VSP_EB, xiaowei.luo@20090907
		return TRUE;
	else
		return FALSE;
}

MMDecRet MP4DecRelease(void)
{
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	return MMDEC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
