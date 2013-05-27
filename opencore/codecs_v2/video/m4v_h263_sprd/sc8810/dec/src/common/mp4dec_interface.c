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

#ifdef _VSP_LINUX_
PUBLIC void MP4DecSetPostFilter(int en)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	vop_mode_ptr->post_filter_en = en;
}

DEC_FRM_BFR g_rec_buf;
PUBLIC void MP4DecSetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
//#ifdef MP4VDEC_SW_SUPPORT_YUV420SP
//	int32 size_y = ((vop_mode_ptr->OrgFrameWidth  + 15) >>4) * ((vop_mode_ptr->OrgFrameHeight+ 15) >>4)<<8;	
//#else
//	int32 size_y = ((vop_mode_ptr->OrgFrameWidth  + 15 + 32) >>4) * ((vop_mode_ptr->OrgFrameHeight+ 15 + 32) >>4)<<8;	
//#endif
//       int32 size_c = size_y >> 2;
	  
	g_rec_buf.imgY =  pFrameY;
//	g_rec_buf.imgU =  pFrameY+size_y;
//	g_rec_buf.imgV =  pFrameY+size_y+size_c;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy>>8;
//	g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y)>>8;
//	g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c)>>8;

	g_rec_buf.pBufferHeader = pBufferHeader;

	SCI_TRACE_LOW("VSP DPB::Mpeg4Decoder_OMX::Mp4DecodeVideo set cur pic  %x,%x,%x\n", pFrameY,pBufferHeader,pFrameY_phy);
}

FunctionType_BufCB VSP_bindCb = NULL;
FunctionType_BufCB VSP_unbindCb = NULL;
void *g_user_data = NULL;
void MP4DecRegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata)
{
	VSP_bindCb = bindCb;
	VSP_unbindCb = unbindCb;
	g_user_data = userdata;
}

FunctionType_FlushCache VSP_fluchCacheCb = NULL;
void  MP4Dec_RegFlushCacheCB( FunctionType_FlushCache fluchCacheCb)
{
   	VSP_fluchCacheCb = fluchCacheCb;   
}

void MP4DecReleaseRefBuffers()
{
	int i;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	if(!vop_mode_ptr)
		return;

	for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
	{
		if(g_FrmYUVBfr[i].bRef)
		{
			g_FrmYUVBfr[i].bRef = FALSE;
			if(g_FrmYUVBfr[i].pBufferHeader!=NULL)
			{
			 	(*VSP_unbindCb)(g_user_data,g_FrmYUVBfr[i].pBufferHeader,1);
			 	//g_FrmYUVBfr[i].pBufferHeader = NULL;
			}
		}
	}

    if (vop_mode_ptr->post_filter_en)
	{
		for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
		{
			if(g_DispFrmYUVBfr[i].bRef)
			{
				g_DispFrmYUVBfr[i].bRef = FALSE;
				g_DispFrmYUVBfr[i].bDisp = FALSE;
				if(g_DispFrmYUVBfr[i].pBufferHeader!=NULL){
	                     	(*VSP_unbindCb)(g_user_data,g_DispFrmYUVBfr[i].pBufferHeader,1);
					//g_DispFrmYUVBfr[i].pBufferHeader = NULL;
				}
			}
		}
	}
//	if (!vop_mode_ptr->post_filter_en)  // for TDFAE00046008.
	{
		vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
	        vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
	}
	g_nFrame_dec = 0;

#if 1
        SCI_TRACE_LOW("---MP4DecReleaseRefBuffers, 1,%d",vop_mode_ptr->is_previous_cmd_done);
        if(vop_mode_ptr->VSP_used &&  !vop_mode_ptr->is_previous_cmd_done)
        {
            VSP_START_CQM();
	    vop_mode_ptr->is_previous_cmd_done = 1;
        }
        SCI_TRACE_LOW("---MP4DecReleaseRefBuffers, 2,%d",vop_mode_ptr->is_previous_cmd_done);
#endif

}

int MP4DecGetLastDspFrm(void **pOutput)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	DEC_FRM_BFR *frm_bfr;
	DEC_FRM_BFR *disp_frm = PNULL;
	uint8 *rec_imgY = PNULL;
	int i;

	*pOutput = NULL;
	if(!vop_mode_ptr)
		return FALSE;
	if(!vop_mode_ptr->pBckRefFrame)
		return FALSE;		
	frm_bfr = vop_mode_ptr->pBckRefFrame->pDecFrame;
	vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
	if(NULL != frm_bfr)
	{	
		if (!vop_mode_ptr->post_filter_en)
		{
			*pOutput = frm_bfr->pBufferHeader;
			
		}else
		{
			rec_imgY = frm_bfr->imgY;

			for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
			{
				if(g_DispFrmYUVBfr[i].bDisp && (rec_imgY == g_DispFrmYUVBfr[i].rec_imgY))
				{
					disp_frm = &g_DispFrmYUVBfr[i];
					break;
				}
			}
			if(disp_frm)
			{
				*pOutput = disp_frm->pBufferHeader;
				disp_frm->bDisp = FALSE;
			}else
			{
				return FALSE;
			}
		}
		return TRUE;
	}else
	{
		return FALSE;
	}
}

#endif

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

	if (VSP_OPEN_Dev() < 0)
	{
		return MMDEC_ERROR;
	};
	
	Mp4Dec_InitInterMem(pBuffer);
	
	Mp4Dec_InitGlobal();

	vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_VOP_MODE_T), 4);
	SCI_ASSERT(NULL != vop_mode_ptr);	
	Mp4Dec_SetVopmode(vop_mode_ptr);

	g_dec_is_first_frame = TRUE;
	g_dec_is_stop_decode_vol = FALSE;
	g_dec_is_changed_format = FALSE;	
	vop_mode_ptr->error_flag = FALSE;	
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;
	vop_mode_ptr->VSP_used = 0;
	vop_mode_ptr->VT_used = 0;

	//for H263 plus header
	h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(sizeof(H263_PLUS_HEAD_INFO_T), 4);
	SCI_ASSERT(NULL != h263_plus_head_info_ptr);
	Mp4Dec_SetH263PlusHeadInfo(h263_plus_head_info_ptr);

	vop_mode_ptr->bitstrm_ptr = (DEC_BS_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_BS_T), 4);	
	SCI_ASSERT(NULL != vop_mode_ptr->bitstrm_ptr);
	
	vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_MODE_T), 4);
	SCI_ASSERT(NULL != vop_mode_ptr->pMbMode_B);

	 vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_BFR_T), 4);
	SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr);

 	vop_mode_ptr->IntraQuantizerMatrix = (uint8 *)Mp4Dec_InterMemAlloc(sizeof(uint8)*BLOCK_SQUARE_SIZE, 4);
	SCI_ASSERT(NULL != vop_mode_ptr->IntraQuantizerMatrix);

	 vop_mode_ptr->InterQuantizerMatrix = (uint8 *)Mp4Dec_InterMemAlloc(sizeof(uint8)*BLOCK_SQUARE_SIZE, 4);
	SCI_ASSERT(NULL != vop_mode_ptr->InterQuantizerMatrix);

	vop_mode_ptr->dbk_para = (DBK_PARA_T *)Mp4Dec_InterMemAlloc(sizeof(DBK_PARA_T), 4);
		
	Mp4Dec_InitDecoderPara(vop_mode_ptr);

	vop_mode_ptr->uv_interleaved = video_format_ptr->uv_interleaved = 1;
	vop_mode_ptr->video_std = video_format_ptr->video_std;
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std != VSP_MPEG4)
	{
		SCI_TRACE_LOW ("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{		
		if(video_format_ptr->i_extra > 0)
		{
			Mp4Dec_InitBitstream_sw(vop_mode_ptr->bitstrm_ptr, video_format_ptr->p_extra, video_format_ptr->i_extra);

			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);

			if(MMDEC_OK == is_init_success)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
			}
		}
	}

	return is_init_success;
}

/*****************************************************************************/
//  Description:   Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/

static int32 s_hw_error_flag;
int g_mpeg4_dec_err_flag;
PUBLIC MMDecRet MP4DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret = MMDEC_ERROR;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

	g_mpeg4_dec_err_flag = 0;

        
	SCI_TRACE_LOW("MP4DecDecode: E");

    if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;
		g_mpeg4_dec_err_flag |= 1;
        //return MMDEC_OUTPUT_BUFFER_OVERFLOW;
        ret = MMDEC_OUTPUT_BUFFER_OVERFLOW;
	goto MPEG4_DEC_CQM_ERROR;	
    }

	vop_mode_ptr->sw_vld_flag = FALSE;

FLV_RE_DEC:
	vop_mode_ptr->error_flag = FALSE;
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;

//	if (vop_mode_ptr->VSP_used || vop_mode_ptr->VT_used)
	{
    		if(VSP_MPEG4 != vop_mode_ptr->video_std)
                {
                    uint8 *pStream = dec_input_ptr->pStream;

    	    	    uint32 first_32bits = (pStream[0]<<24) | (pStream[1]<<16) | (pStream[2]<<8) | (pStream[3]);
    	            if((first_32bits>>11) == 0x10)
    	            {
    	        	if((first_32bits>>10) == 0x20)
    	            	    vop_mode_ptr->video_std = VSP_ITU_H263;
    	                else
    	            	    vop_mode_ptr->video_std = VSP_FLV_V1;
    	            }
    	        }

		Mp4Dec_VerifyBitstrm(dec_input_ptr->pStream, dec_input_ptr->dataLen);

		if (vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->error_flag = FALSE;
			vop_mode_ptr->return_pos2 |= (1<<5);
			g_mpeg4_dec_err_flag |= (1<<8);
			//return MMDEC_STREAM_ERROR;
			ret = MMDEC_STREAM_ERROR;
			goto MPEG4_DEC_CQM_ERROR;			
		}
	}
	Mp4Dec_InitBitstream_sw(vop_mode_ptr->bitstrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);

	if(VSP_MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->find_vop_header  = 0;
		ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
		if(!vop_mode_ptr->find_vop_header)
		{
#ifdef _VSP_LINUX_							
			dec_output_ptr->VopPredType = NVOP;
#endif
			//return MMDEC_OK;
			ret = MMDEC_OK;
			goto MPEG4_DEC_CQM_ERROR;
		}
	}else
	{
		if(VSP_ITU_H263 == vop_mode_ptr->video_std)
		{
			ret = Mp4Dec_DecH263Header(vop_mode_ptr);
		}else
		{	
			ret = Mp4Dec_FlvH263PicHeader(vop_mode_ptr);
		}				
	}

	if(ret != MMDEC_OK)
	{
		//modified by xwluo, 20100511
		g_mpeg4_dec_err_flag |= 1<<1;		
		//return ret;
		goto MPEG4_DEC_CQM_ERROR;
	}else if (!vop_mode_ptr->is_work_mode_set)
	{
		MP4Dec_JudgeDecMode(vop_mode_ptr);
		vop_mode_ptr->is_work_mode_set = TRUE;
	}
/*
	if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
	{
        if (g_nFrame_dec)
	    {
			g_nFrame_dec++;
	    }
		g_mpeg4_dec_err_flag |= 1<<2;			
		//return MMDEC_FRAME_SEEK_IVOP;
		ret = MMDEC_FRAME_SEEK_IVOP;
		goto MPEG4_DEC_CQM_ERROR;		
	}
*/
	vop_mode_ptr->is_expect_IVOP  = dec_input_ptr->expected_IVOP;
	dec_output_ptr->frameEffective = FALSE;
	dec_output_ptr->pOutFrameY = PNULL;
	dec_output_ptr->pOutFrameU = PNULL;
	dec_output_ptr->pOutFrameV = PNULL;

	if(!vop_mode_ptr->bInitSuceess)
	{
		if( MMDEC_OK != Mp4Dec_InitSessionDecode(vop_mode_ptr) )
		{
			g_mpeg4_dec_err_flag |= 1<<3;		        
			//return MMDEC_MEMORY_ERROR;
			ret = MMDEC_MEMORY_ERROR;
			goto MPEG4_DEC_CQM_ERROR;
		}
		vop_mode_ptr->bInitSuceess = TRUE;
	}

	if(vop_mode_ptr->VopPredType != NVOP)
	{
		//sorenson H263 and itu h.263 dont support B frame.
		if((BVOP == vop_mode_ptr->VopPredType) &&  (VSP_MPEG4 != vop_mode_ptr->video_std) )
		{
#ifdef _VSP_LINUX_		
			dec_output_ptr->VopPredType = NVOP;
#endif		
			//return MMDEC_OK;
			ret = MMDEC_OK;
			goto MPEG4_DEC_CQM_ERROR;
		}
		
		if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
		{
			g_mpeg4_dec_err_flag |= 1<<4;
			//return MMDEC_OUTPUT_BUFFER_OVERFLOW;
			ret = MMDEC_OUTPUT_BUFFER_OVERFLOW;
			goto MPEG4_DEC_CQM_ERROR;
		}
		
		if (vop_mode_ptr->post_filter_en)
		{
			if(dec_input_ptr->beDisplayed)
			{
				if(!Mp4Dec_GetCurDispFrameBfr(vop_mode_ptr))
				{
					g_mpeg4_dec_err_flag |= 1<<5;					
					//return MMDEC_OUTPUT_BUFFER_OVERFLOW;
					ret = MMDEC_OUTPUT_BUFFER_OVERFLOW;
					goto MPEG4_DEC_CQM_ERROR;
				}
			}
		}else //for fpga verification
		{
			vop_mode_ptr->pCurDispFrame = vop_mode_ptr->pCurRecFrame;
		}
	}else //NVOP
	{
		SCI_TRACE_LOW ("MP4DecDecode: frame not coded!\n");
		
#ifdef _VSP_LINUX_
	{
		uint32 ByteConsumed = Mp4Dec_ByteConsumed();
		if(ByteConsumed > dec_input_ptr->dataLen)
			dec_input_ptr->dataLen = 0;
		else
			dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;
		dec_output_ptr->VopPredType = NVOP;
	}
#endif	
		
		//return MMDEC_OK;
		ret = MMDEC_OK;
		goto MPEG4_DEC_CQM_ERROR;
	}

	s_hw_error_flag = 0;
	ret = Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);

	if(ret != MMDEC_OK)
	{
		g_mpeg4_dec_err_flag |= 1<<6;		
		//return ret;
		goto MPEG4_DEC_CQM_ERROR;
	}
		
	if(IVOP == vop_mode_ptr->VopPredType)
	{
//		SCI_TRACE_LOW ("\t I VOP\t%d frame_num %d\n", dec_input_ptr->dataLen,g_nFrame_dec); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: IVOP\n", g_nFrame_dec);
	#endif //_TRACE_	

		ret = g_Mp4Dec_IVOP(vop_mode_ptr); 	
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{
//		SCI_TRACE_LOW ("\t P VOP\t%d frame_num %d\n", dec_input_ptr->dataLen,g_nFrame_dec); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: PVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
	
		if (vop_mode_ptr->post_filter_en  && vop_mode_ptr->is_expect_IVOP  == FALSE)
		{ //jgdu
		
			if (vop_mode_ptr->pBckRefFrame->pDecFrame == PNULL)
			{
				if( (PNULL != vop_mode_ptr->pCurDispFrame) && 
					(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame) && 
					(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame->imgY)
				  )
				{
					MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
				}
				g_mpeg4_dec_err_flag |= 1<<7;	
//				SCI_TRACE_LOW("leon %s,%d",__FUNCTION__,__LINE__);
				//return MMDEC_ERROR;
				ret = MMDEC_ERROR;
				goto MPEG4_DEC_CQM_ERROR;
			}
		}		

		ret = g_Mp4Dec_PVOP(vop_mode_ptr); 
	}else if(SVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t S VOP, Don't SUPPORTED!\n"); 

	#if _TRACE_		 	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: SVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
//		vop_mode_ptr->be_gmc_wrap = TRUE;
//		Mp4Dec_DecPVOP_sw(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
	}else if(NVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("frame not coded!\n");
       	MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
		//return MMDEC_OK;
		ret = MMDEC_OK;
		goto MPEG4_DEC_CQM_ERROR;
	}else
	{
//		SCI_TRACE_LOW ("\t B VOP\t%d\n", dec_input_ptr->dataLen);
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: BVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
		vop_mode_ptr->err_MB_num = 0;
		ret = g_Mp4Dec_BVOP(vop_mode_ptr); 
	}

	if (vop_mode_ptr->VSP_used)
	{
		/*frame end synchrolization*/
		VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 16, 1, 1, "DBK_CTR1: polling dbk frame idle");
		VSP_WRITE_CMD_INFO( (VSP_DBK << 29) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

		VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "AHBM_STS: polling AHB idle status");
		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec, "VSP_TST: configure frame_cnt to debug register for end of picture");
		VSP_WRITE_CMD_INFO( (VSP_GLB << 29) | (2<<24) | (VSP_TST_WOFF<<8) | ((1<<7)|VSP_DBG_WOFF));

		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, 0x12345678, "VSP_TST: finished one frame");
		VSP_WRITE_CMD_INFO(0x12345678);

	//	ret = g_Mp4Dec_Frm_Level_Sync (dec_input_ptr, dec_output_ptr, vop_mode_ptr);
		
	//	ret = Mp4Dec_frm_level_sync_hw_sw_normal (dec_input_ptr, dec_output_ptr, vop_mode_ptr);
		ret = Mp4Dec_frm_level_sync_hw_sw_pipeline (dec_input_ptr, dec_output_ptr, vop_mode_ptr);
	}else
	{
		if(vop_mode_ptr->VT_used)
		{
			if(vop_mode_ptr->err_MB_num)
			{
				SCI_TRACE_LOW ("MP4DecDecode: Detect error bitstream, try to conceal!\n");
				if(IVOP == vop_mode_ptr->VopPredType)
				{
					Mp4Dec_EC_IVOP_vt(vop_mode_ptr);
				}
				else if(PVOP == vop_mode_ptr->VopPredType)
				{
					Mp4Dec_EC_PVOP_vt(vop_mode_ptr);
				}			
			}
			if(vop_mode_ptr->post_filter_en)
			{
				Mp4Dec_Deblock_vop( vop_mode_ptr);
			}
			
			ret = MMDEC_OK;
		}
		
		Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);	
	}
	
	Mp4Dec_exit_picture(vop_mode_ptr);

	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;
	g_nFrame_dec++;

#ifdef _VSP_LINUX_
{
	uint32 ByteConsumed = Mp4Dec_ByteConsumed();

	if(ByteConsumed > dec_input_ptr->dataLen)
		dec_input_ptr->dataLen = 0;
	else
		dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

}
#endif

	SCI_TRACE_LOW("MP4DecDecode: X");
	
	if(ret != MMDEC_OK)
	{
		g_mpeg4_dec_err_flag |= 1<<10;		
		goto MPEG4_DEC_CQM_ERROR;
	}
	return ret;


MPEG4_DEC_CQM_ERROR:
#if 1
        SCI_TRACE_LOW("---MP4DecDecode, 1,%d",vop_mode_ptr->is_previous_cmd_done);
        if(vop_mode_ptr->VSP_used &&  !vop_mode_ptr->is_previous_cmd_done)
        {
            VSP_START_CQM();
	    vop_mode_ptr->is_previous_cmd_done = 1;
        }
        SCI_TRACE_LOW("---MP4DecDecode, 2,%d",vop_mode_ptr->is_previous_cmd_done);
	return ret;	
#endif	
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
	}else
	{
		pOutput->frameEffective = 0;
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
	Mp4Dec_FreeMem();
	
	VSP_CLOSE_Dev();
	
#if _CMODEL_
	VSP_Delete_CModel();
#endif
//	SCI_TRACE_LOW("enter MP4DecRelease------------- \n");
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
