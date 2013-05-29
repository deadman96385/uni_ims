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
PUBLIC void MP4DecSetPostFilter(MP4Handle *mp4Handle, int en)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;

	vop_mode_ptr->post_filter_en = en;
}

PUBLIC void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	  
	vd->g_rec_buf.imgY =  pFrameY;
//	g_rec_buf.imgU =  pFrameY+size_y;
//	g_rec_buf.imgV =  pFrameY+size_y+size_c;

	vd->g_rec_buf.imgYAddr = (uint32)pFrameY_phy>>8;
//	vd->g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y)>>8;
//	vd->g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c)>>8;

	vd->g_rec_buf.pBufferHeader = pBufferHeader;

	SCI_TRACE_LOW("%s: %s :  %x,%x,%x\n",  __FILE__, __FUNCTION__, pFrameY,pBufferHeader,pFrameY_phy);
}

//void MP4DecRegBufferCB(VideoDecControls *decCtrl, FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata)
//{
//    	VIDEO_DATA_T *vd = (VIDEO_DATA_T *) decCtrl->videoDecoderData;
//
//	vd->VSP_bindCb = bindCb;
//	vd->VSP_unbindCb = unbindCb;
//	vd->g_user_data = userdata;
//}

//void Mp4DecRegMemAllocCB (VideoDecControls *decCtrl, void *userdata, FunctionType_MemAllocCB extMemCb)
//{
//    VIDEO_DATA_T *vd = (VIDEO_DATA_T *) decCtrl->videoDecoderData;
    
//    vd->g_user_data = userdata;
//    vd->VSP_extMemCb = extMemCb;
//}

void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	int i;
	
	if(!vop_mode_ptr)
		return;
	if (!vop_mode_ptr->post_filter_en)
	{
		for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
		{
			if(vd->g_FrmYUVBfr[i].bRef)
			{
				vd->g_FrmYUVBfr[i].bRef = FALSE;
				if((*mp4Handle->VSP_unbindCb) && vd->g_FrmYUVBfr[i].pBufferHeader!=NULL){
				 	(*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,vd->g_FrmYUVBfr[i].pBufferHeader,1);
				 	//g_FrmYUVBfr[i].pBufferHeader = NULL;
				}
			}
		}
	}else
	{
		for(i = 0; i < DISP_YUV_BUFFER_NUM; i++)
		{
			if(vd->g_DispFrmYUVBfr[i].bRef)
			{
				vd->g_DispFrmYUVBfr[i].bRef = FALSE;
				vd->g_DispFrmYUVBfr[i].bDisp = FALSE;
				if((*mp4Handle->VSP_unbindCb) && vd->g_DispFrmYUVBfr[i].pBufferHeader!=NULL){
	                     	(*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,vd->g_DispFrmYUVBfr[i].pBufferHeader,1);
					//g_DispFrmYUVBfr[i].pBufferHeader = NULL;
				}
			}
		}
	}
	if (!vop_mode_ptr->post_filter_en)
	{
		vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
	        vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
	}
	vop_mode_ptr->g_nFrame_dec = 0;

#if 1
        SCI_TRACE_LOW("---MP4DecReleaseRefBuffers, 1,%d",vop_mode_ptr->is_previous_cmd_done);
        if(vop_mode_ptr->VSP_used &&  !vop_mode_ptr->is_previous_cmd_done)
        {
//            VSP_START_CQM();
	    vop_mode_ptr->is_previous_cmd_done = 1;
        }
        SCI_TRACE_LOW("---MP4DecReleaseRefBuffers, 2,%d",vop_mode_ptr->is_previous_cmd_done);
#endif

}

int MP4DecGetLastDspFrm(MP4Handle *mp4Handle, void **pOutput)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
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
				if(vd->g_DispFrmYUVBfr[i].bDisp && (rec_imgY == vd->g_DispFrmYUVBfr[i].rec_imgY))
				{
					disp_frm = &vd->g_DispFrmYUVBfr[i];
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

PUBLIC void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;

	 *display_width = vop_mode_ptr->OrgFrameWidth;
	 *display_height = vop_mode_ptr->OrgFrameHeight;

        SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *display_width, *display_height);
}

PUBLIC void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height) 
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;

	vop_mode_ptr->FrameWidth  = (int16)(((vop_mode_ptr->OrgFrameWidth  + 15) >> 4) << 4);
	vop_mode_ptr->FrameHeight = (int16)(((vop_mode_ptr->OrgFrameHeight + 15) >> 4) << 4);

    	vop_mode_ptr->FrameExtendWidth  = vop_mode_ptr->FrameWidth  + 2 * YEXTENTION_SIZE;
	vop_mode_ptr->FrameExtendHeight = vop_mode_ptr->FrameHeight + 2 * YEXTENTION_SIZE;

#ifndef YUV_THREE_PLANE
        *width = vop_mode_ptr->FrameWidth;
    	*height = vop_mode_ptr->FrameHeight;
#else
    	*width = vop_mode_ptr->FrameExtendWidth;
    	*height = vop_mode_ptr->FrameExtendHeight;
#endif        

        SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *width, *height);
}

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
PUBLIC MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = NULL;
	MMCodecBuffer *pBuffer_tmp = pBuffer;

    SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

//	memset(decCtrl, 0, sizeof(VideoDecControls)); 

//	if (VSP_OPEN_Dev() < 0)
//	{
//		return MMDEC_ERROR;
//	};
	
	vd = (MP4DecObject *) pBuffer_tmp->int_buffer_ptr;
        memset(vd, 0, sizeof(MP4DecObject)); 
	mp4Handle->videoDecoderData = (void *) vd;
	
	pBuffer_tmp->int_buffer_ptr += sizeof(MP4DecObject);
	pBuffer_tmp->int_size -= sizeof(MP4DecObject);

	Mp4Dec_InitInterMem(vd, pBuffer_tmp);
	
	Mp4Dec_InitGlobal();
        vd->mp4Handle = mp4Handle;
	
	vd->vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_InterMemAlloc(vd, sizeof(DEC_VOP_MODE_T));
	vop_mode_ptr = vd->vop_mode_ptr;

	//for H263 plus header
	vd->h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(vd, sizeof(H263_PLUS_HEAD_INFO_T));
	SCI_ASSERT(NULL != vd->h263_plus_head_info_ptr);

	vop_mode_ptr->bitstrm_ptr = (DEC_BS_T *)Mp4Dec_InterMemAlloc(vd, sizeof(DEC_BS_T));	
	SCI_ASSERT(NULL != vop_mode_ptr->bitstrm_ptr);
	
	vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_InterMemAlloc(vd, sizeof(DEC_MB_MODE_T));
	SCI_ASSERT(NULL != vop_mode_ptr->pMbMode_B);

	 vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_InterMemAlloc(vd, sizeof(DEC_MB_BFR_T));
	SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr);

 	vop_mode_ptr->IntraQuantizerMatrix = (uint8 *)Mp4Dec_InterMemAlloc(vd, sizeof(uint8)*BLOCK_SQUARE_SIZE);
	SCI_ASSERT(NULL != vop_mode_ptr->IntraQuantizerMatrix);

	vop_mode_ptr->InterQuantizerMatrix = (uint8 *)Mp4Dec_InterMemAlloc(vd, sizeof(uint8)*BLOCK_SQUARE_SIZE);
	SCI_ASSERT(NULL != vop_mode_ptr->InterQuantizerMatrix);
    	
	Mp4Dec_InitDecoderPara(vd);

        return MMDEC_OK;
}

PUBLIC MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr)
{
    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
        DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
        MMDecRet ret = MMDEC_OK;

	SCI_TRACE_LOW("%s, test", __FUNCTION__);
            
	vop_mode_ptr->uv_interleaved = video_format_ptr->uv_interleaved = 1;
	vop_mode_ptr->video_std = video_format_ptr->video_std;
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std != MPEG4)
	{
		SCI_TRACE_LOW ("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{		
		if(video_format_ptr->i_extra > 0)
		{
			Mp4Dec_InitBitstream_sw(vop_mode_ptr->bitstrm_ptr, video_format_ptr->p_extra, video_format_ptr->i_extra);

			ret = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);

			if(MMDEC_OK == ret)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;

                                vop_mode_ptr->FrameWidth =  ((vop_mode_ptr->OrgFrameWidth  + 15)>>4)<<4;
	                        vop_mode_ptr->FrameHeight  = ((vop_mode_ptr->OrgFrameHeight + 15) >>4)<<4;

                                vop_mode_ptr->FrameExtendWidth  = vop_mode_ptr->FrameWidth  + 2 * YEXTENTION_SIZE;
	                        vop_mode_ptr->FrameExtendHeight = vop_mode_ptr->FrameHeight + 2 * YEXTENTION_SIZE;

//                                SCI_TRACE_LOW("%s, %d, org_width: %d, org_height: %d, width: %d, height: %d, ext_width: %d, ext_height: %d", __FUNCTION__, __LINE__, vop_mode_ptr->OrgFrameWidth, vop_mode_ptr->OrgFrameHeight,
//                                    vop_mode_ptr->FrameWidth, vop_mode_ptr->FrameHeight, vop_mode_ptr->FrameExtendWidth, vop_mode_ptr->FrameExtendHeight);

SCI_TRACE_LOW("%s, test", __FUNCTION__);
SCI_TRACE_LOW("%s, test", __FUNCTION__);

			}
		}
	}

	return ret;
}

/*****************************************************************************/
//  Description:   Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/

static int32 s_hw_error_flag;
PUBLIC MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
	MMDecRet ret = MMDEC_ERROR;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;

	mp4Handle->g_mpeg4_dec_err_flag = 0;

	SCI_TRACE_LOW("MP4DecDecode: E");

    if(!Mp4Dec_GetCurRecFrameBfr(vd, vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;
		mp4Handle->g_mpeg4_dec_err_flag |= 1;
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

#if 0
	if (vop_mode_ptr->VSP_used || vop_mode_ptr->VT_used)
	{
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
#endif	
	Mp4Dec_InitBitstream_sw(vop_mode_ptr->bitstrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);

	if(MPEG4 == vop_mode_ptr->video_std)
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
		if(ITU_H263 == vop_mode_ptr->video_std)
	    {
	    	uint32 first_32bits = Mp4Dec_Show32Bits(vop_mode_ptr->bitstrm_ptr);
	        if((first_32bits>>11) == 0x10)
	        {
	        	if((first_32bits>>10) == 0x20)
	            	vop_mode_ptr->video_std = ITU_H263;
	            else
	            	vop_mode_ptr->video_std = FLV_V1;
	        }
	    }

		if(ITU_H263 == vop_mode_ptr->video_std)
		{
			ret = Mp4Dec_DecH263Header(vd, vop_mode_ptr);
		}else
		{	
			ret = Mp4Dec_FlvH263PicHeader(vd, vop_mode_ptr);
		}				
	}

	SCI_TRACE_LOW("%s, %d, ret: %d, pic_type: %d", __FUNCTION__, __LINE__, ret, vop_mode_ptr->VopPredType);
	if(ret != MMDEC_OK)
	{
		//modified by xwluo, 20100511
		mp4Handle->g_mpeg4_dec_err_flag |= 1<<1;		
		//return ret;
		goto MPEG4_DEC_CQM_ERROR;
	}else if (!vop_mode_ptr->is_work_mode_set)
	{
		MP4Dec_JudgeDecMode(vop_mode_ptr);
		vop_mode_ptr->is_work_mode_set = TRUE;
	}

	if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
	{
        if (vop_mode_ptr->g_nFrame_dec)
	    {
			vop_mode_ptr->g_nFrame_dec++;
	    }
		mp4Handle->g_mpeg4_dec_err_flag |= 1<<2;			
		//return MMDEC_FRAME_SEEK_IVOP;
		ret = MMDEC_FRAME_SEEK_IVOP;
		goto MPEG4_DEC_CQM_ERROR;		
	}

	dec_output_ptr->frameEffective = FALSE;
	dec_output_ptr->pOutFrameY = PNULL;
	dec_output_ptr->pOutFrameU = PNULL;
	dec_output_ptr->pOutFrameV = PNULL;
	
	if(!vop_mode_ptr->bInitSuceess)
	{
		if( MMDEC_OK != Mp4Dec_InitSessionDecode(mp4Handle, vd, vop_mode_ptr) )
		{
			mp4Handle->g_mpeg4_dec_err_flag |= 1<<3;		        
			//return MMDEC_MEMORY_ERROR;
			ret = MMDEC_MEMORY_ERROR;
			goto MPEG4_DEC_CQM_ERROR;
		}
		vop_mode_ptr->bInitSuceess = TRUE;
                return MMDEC_MEMORY_ALLOCED;
	}

	if(vop_mode_ptr->VopPredType != NVOP)
	{
		//sorenson H263 and itu h.263 dont support B frame.
		if((BVOP == vop_mode_ptr->VopPredType) &&  (MPEG4 != vop_mode_ptr->video_std) )
		{
#ifdef _VSP_LINUX_		
			dec_output_ptr->VopPredType = NVOP;
#endif		
			//return MMDEC_OK;
			ret = MMDEC_OK;
			goto MPEG4_DEC_CQM_ERROR;
		}
		
		if(!Mp4Dec_GetCurRecFrameBfr(vd, vop_mode_ptr))
		{
			mp4Handle->g_mpeg4_dec_err_flag |= 1<<4;			
			//return MMDEC_OUTPUT_BUFFER_OVERFLOW;
			ret = MMDEC_OUTPUT_BUFFER_OVERFLOW;
			goto MPEG4_DEC_CQM_ERROR;
		}
		
		if (vop_mode_ptr->post_filter_en)
		{
			if(dec_input_ptr->beDisplayed)
			{
				if(!Mp4Dec_GetCurDispFrameBfr(vd, vop_mode_ptr))
				{
					mp4Handle->g_mpeg4_dec_err_flag |= 1<<5;					
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
		uint32 ByteConsumed = Mp4Dec_ByteConsumed(vop_mode_ptr);
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
	ret = Mp4Dec_InitVop(vd, vop_mode_ptr, dec_input_ptr);

	if(ret != MMDEC_OK)
	{
		mp4Handle->g_mpeg4_dec_err_flag |= 1<<6;		
		//return ret;
		goto MPEG4_DEC_CQM_ERROR;
	}
		

	if(IVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t I VOP\t%d frame_num %d\n", dec_input_ptr->dataLen,vop_mode_ptr->g_nFrame_dec); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: IVOP\n", vop_mode_ptr->g_nFrame_dec);
	#endif //_TRACE_	

		ret = vd->g_Mp4Dec_IVOP(vd, vop_mode_ptr); 	
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t P VOP\t%d frame_num %d\n", dec_input_ptr->dataLen,vop_mode_ptr->g_nFrame_dec); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: PVOP\n", vop_mode_ptr->g_nFrame_dec);
	#endif //_TRACE_	
	
		if (vop_mode_ptr->post_filter_en)
		{ //jgdu
			if (vop_mode_ptr->pBckRefFrame->pDecFrame == PNULL)
			{
				if( (PNULL != vop_mode_ptr->pCurDispFrame) && 
					(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame) && 
					(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame->imgY)
				  )
				{
					MPEG4_DecReleaseDispBfr(vd, vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
				}
				mp4Handle->g_mpeg4_dec_err_flag |= 1<<7;	
//				SCI_TRACE_LOW("leon %s,%d",__FUNCTION__,__LINE__);
				//return MMDEC_ERROR;
				ret = MMDEC_ERROR;
				goto MPEG4_DEC_CQM_ERROR;
			}
		}		

		ret = vd->g_Mp4Dec_PVOP(vd, vop_mode_ptr); 	
	}else if(SVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t S VOP, Don't SUPPORTED!\n"); 

	#if _TRACE_		 	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: SVOP\n", vop_mode_ptr->g_nFrame_dec);
	#endif //_TRACE_	
//		vop_mode_ptr->be_gmc_wrap = TRUE;
//		Mp4Dec_DecPVOP_sw(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
	}else if(NVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("frame not coded!\n");
       	MPEG4_DecReleaseDispBfr(vd, vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
		//return MMDEC_OK;
		ret = MMDEC_OK;
		goto MPEG4_DEC_CQM_ERROR;
	}else
	{
		SCI_TRACE_LOW ("\t B VOP\t%d\n", dec_input_ptr->dataLen);
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: BVOP\n", vop_mode_ptr->g_nFrame_dec);
	#endif //_TRACE_	
		vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
		vop_mode_ptr->err_MB_num = 0;
		ret = vd->g_Mp4Dec_BVOP(vd, vop_mode_ptr); 
	}

	if (vop_mode_ptr->VSP_used)
	{
#if 0	
		/*frame end synchrolization*/
		VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 16, 1, 1, "DBK_CTR1: polling dbk frame idle");
		VSP_WRITE_CMD_INFO( (VSP_DBK << 29) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

		VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "AHBM_STS: polling AHB idle status");
		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, vop_mode_ptr->g_nFrame_dec, "VSP_TST: configure frame_cnt to debug register for end of picture");
		VSP_WRITE_CMD_INFO( (VSP_GLB << 29) | (2<<24) | (VSP_TST_WOFF<<8) | ((1<<7)|VSP_DBG_WOFF));

		VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, 0x12345678, "VSP_TST: finished one frame");
		VSP_WRITE_CMD_INFO(0x12345678);

	//	ret = g_Mp4Dec_Frm_Level_Sync (dec_input_ptr, dec_output_ptr, vop_mode_ptr);
		
	//	ret = Mp4Dec_frm_level_sync_hw_sw_normal (dec_input_ptr, dec_output_ptr, vop_mode_ptr);
		ret = Mp4Dec_frm_level_sync_hw_sw_pipeline (vd, dec_input_ptr, dec_output_ptr, vop_mode_ptr);
#endif	
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
			ret = MMDEC_OK;
		}
		
		Mp4Dec_output_one_frame (vd, dec_output_ptr, vop_mode_ptr);	
	}
	
	Mp4Dec_exit_picture(vd, vop_mode_ptr);

	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;
	vop_mode_ptr->g_nFrame_dec++;

#ifdef _VSP_LINUX_
{
	uint32 ByteConsumed = Mp4Dec_ByteConsumed(vop_mode_ptr);

        SCI_TRACE_LOW ("%s, %d, ByteConsumed: %d, dec_input_ptr->dataLen:%d", __FUNCTION__, __LINE__, ByteConsumed, dec_input_ptr->dataLen);

	if(ByteConsumed > dec_input_ptr->dataLen)
		dec_input_ptr->dataLen = 0;
	else
		dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

}
#endif

	SCI_TRACE_LOW("MP4DecDecode: X");
	
	if(ret != MMDEC_OK)
	{
		mp4Handle->g_mpeg4_dec_err_flag |= 1<<10;		
		goto MPEG4_DEC_CQM_ERROR;
	}
	return ret;


MPEG4_DEC_CQM_ERROR:
#if 1
        SCI_TRACE_LOW("---MP4DecDecode, 1,%d",vop_mode_ptr->is_previous_cmd_done);
        if(vop_mode_ptr->VSP_used &&  !vop_mode_ptr->is_previous_cmd_done)
        {
//            VSP_START_CQM();
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
PUBLIC void mpeg4dec_GetOneDspFrm(MP4Handle *mp4Handle, MMDecOutput *pOutput, int req_transposed, int is_last_frame)
{
	MP4DecObject *vd = mp4Handle->videoDecoderData;
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
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

//	dcam_cfg = VSP_READ_REG(VSP_DCAM_BASE+DCAM_CFG_OFF, "DCAM_CFG: read dcam configure register");

	if (((dcam_cfg >> 3) & 1) == 0) //bit3: VSP_EB, xiaowei.luo@20090907
		return TRUE;
	else
		return FALSE;
}

MMDecRet MP4DecRelease(MP4Handle *mp4Handle)
{
	MP4DecObject *vd = mp4Handle->videoDecoderData;

	Mp4Dec_FreeMem(vd);
	
//	VSP_CLOSE_Dev();
	
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
