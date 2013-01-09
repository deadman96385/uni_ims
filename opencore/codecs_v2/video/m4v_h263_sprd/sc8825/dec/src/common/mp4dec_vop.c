/******************************************************************************
 ** File Name:    mp4dec_vop.c                                             *
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
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_exit_picture(DEC_VOP_MODE_T *vop_mode_ptr)
{	
	/*reorder reference frame list*/
	if(vop_mode_ptr->VopPredType != BVOP)
	{
		Mp4DecStorablePic * pframetmp;

		vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;
#ifdef _VSP_LINUX_
		if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader!=NULL))
		{
			(*VSP_bindCb)(g_user_data,vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader,0);
		}
#endif

		/*i frame, clean reference frame buf point*/
		if(vop_mode_ptr->VopPredType == IVOP)
		{
			if(vop_mode_ptr->pBckRefFrame->pDecFrame != NULL)
			{
#ifdef _VSP_LINUX_
				if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pBckRefFrame->pDecFrame->pBufferHeader!=NULL))
				{
					(*VSP_unbindCb)(g_user_data,vop_mode_ptr->pBckRefFrame->pDecFrame->pBufferHeader,0);
					vop_mode_ptr->pBckRefFrame->pDecFrame->pBufferHeader = NULL;
				}
#endif		
				vop_mode_ptr->pBckRefFrame->pDecFrame->bRef = FALSE;
				vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
			}		
		}


		/*the buffer for forward reference frame will no longer be a reference frame*/		
		if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
		{
#ifdef _VSP_LINUX_
			if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader!=NULL))
			{
				(*VSP_unbindCb)(g_user_data,vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader,0);
				vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader = NULL;
			}
#endif		

			vop_mode_ptr->pFrdRefFrame->pDecFrame->bRef = FALSE;
			vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
		}
		
		//exchange buffer address. bck->frd, current->bck, frd->current
		pframetmp = vop_mode_ptr->pFrdRefFrame;
		vop_mode_ptr->pFrdRefFrame = vop_mode_ptr->pBckRefFrame;
		vop_mode_ptr->pBckRefFrame = vop_mode_ptr->pCurRecFrame;	
		vop_mode_ptr->pCurRecFrame = pframetmp;	

		if (!vop_mode_ptr->VSP_used && vop_mode_ptr->VopPredType != BVOP)
		{ 
			Mp4Dec_ExtendFrame(vop_mode_ptr); 
		}
	}
}

PUBLIC MMDecRet Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
	MMDecRet ret = MMDEC_OK;

	vop_mode_ptr->sliceNumber	= 0;
	vop_mode_ptr->stop_decoding = FALSE;
	vop_mode_ptr->mbnumDec		= 0;
	vop_mode_ptr->error_flag	= FALSE;
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;
	vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
	vop_mode_ptr->err_num		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_left		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_pos_ptr	= dec_input_ptr->err_pkt_pos;
	vop_mode_ptr->err_MB_num	= vop_mode_ptr->MBNum;

	if(IVOP != vop_mode_ptr->VopPredType)
	{
		if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
		{	
			ret =  MMDEC_ERROR;
		}
		
		if(BVOP == vop_mode_ptr->VopPredType)
		{
			if (vop_mode_ptr->pFrdRefFrame->pDecFrame == NULL)
			{
				ret =  MMDEC_ERROR;

			}
		}
	}
	
	if (vop_mode_ptr->VSP_used)
	{
		 g_cmd_data_base = vop_mode_ptr->cmd_data_buf[vop_mode_ptr->cmd_buf_idx];
		g_cmd_info_base =vop_mode_ptr->cmd_info_buf[vop_mode_ptr->cmd_buf_idx];
		vop_mode_ptr->cmd_buf_idx = 1 - vop_mode_ptr->cmd_buf_idx;
		
		 Mp4Dec_init_frame_VSP(vop_mode_ptr, dec_input_ptr);
		
		//set all mb in one vop to not-decoded
		memset (vop_mode_ptr->mbdec_stat_ptr, 0, vop_mode_ptr->MBNum);

#if 0	//disable it for avoid green block effect in real environment.
{
		int size = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
//		if (dec_input_ptr->err_pkt_num != 0) //for error frame, only for debug
		{
			memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgY, 0xff, size);
			if (vop_mode_ptr->uv_interleaved)
			{
				memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, 0xff, size/2);
			}else
			{
				memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, 0xff, size/4);
				memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgV, 0xff, size/4);
			}
		}
}
#endif
	}else
	{
		//init mc function according to rounding_control
		g_mp4dec_mc_16x16[0] = mc_xyfull_16x16;
		g_mp4dec_mc_8x8[0] = mc_xyfull_8x8;
		if (vop_mode_ptr->RoundingControl == 0)
		{
			g_mp4dec_mc_16x16[1] = mc_xfullyhalf_16x16_rnd0;
			g_mp4dec_mc_16x16[2] = mc_xhalfyfull_16x16_rnd0;
			g_mp4dec_mc_16x16[3] = mc_xyhalf_16x16_rnd0;
	  
			g_mp4dec_mc_8x8[1] = mc_xfullyhalf_8x8_rnd0;
			g_mp4dec_mc_8x8[2] = mc_xhalfyfull_8x8_rnd0;
			g_mp4dec_mc_8x8[3] = mc_xyhalf_8x8_rnd0;
		}else
		{
			g_mp4dec_mc_16x16[1] = mc_xfullyhalf_16x16_rnd1;
			g_mp4dec_mc_16x16[2] = mc_xhalfyfull_16x16_rnd1;
			g_mp4dec_mc_16x16[3] = mc_xyhalf_16x16_rnd1;
	  
			g_mp4dec_mc_8x8[1] = mc_xfullyhalf_8x8_rnd1;
			g_mp4dec_mc_8x8[2] = mc_xhalfyfull_8x8_rnd1;
			g_mp4dec_mc_8x8[3] = mc_xyhalf_8x8_rnd1;
		}
	}

#if _CMODEL_
	Mp4Dec_VspFrameInit(vop_mode_ptr, ((uint32)(dec_input_ptr->pStream)));
#endif

	return ret;
}

PUBLIC void MP4Dec_JudgeDecMode (DEC_VOP_MODE_T * vop_mode_ptr)
{
	int32 aligned_frm_width = (((vop_mode_ptr->OrgFrameWidth + 15)>>4)<<4);
	int32 aligned_frm_height = (((vop_mode_ptr->OrgFrameHeight + 15)>>4)<<4);

	if ((aligned_frm_width <= 320 && aligned_frm_height <= 240) ||
		(aligned_frm_width <= 240 && aligned_frm_height <= 320))
	{
		vop_mode_ptr->VSP_used = 0;
		vop_mode_ptr->VT_used = 1;
	}
#ifndef MP4CODEC_NO_PMEM
	else if ((aligned_frm_width <= 1280 && aligned_frm_height <=720) ||
		(aligned_frm_width <= 720 && aligned_frm_height <= 1280))
	{
		vop_mode_ptr->VSP_used = 1;
		vop_mode_ptr->VT_used = 0;		
	}
#endif
	else 
	{
		vop_mode_ptr->VSP_used = 0;
		vop_mode_ptr->VT_used = 0;
	}

	SCI_TRACE_LOW("%s, vsp_used: %d, VT_used: %d", __FUNCTION__, vop_mode_ptr->VSP_used, vop_mode_ptr->VT_used);
}

PUBLIC void Mp4Dec_ExchangeMBMode (DEC_VOP_MODE_T * vop_mode_ptr)
{
	DEC_MB_MODE_T *mb_mode_tmp_ptr;

	mb_mode_tmp_ptr				= vop_mode_ptr->pMbMode;
	vop_mode_ptr->pMbMode		= vop_mode_ptr->pMbMode_prev;
	vop_mode_ptr->pMbMode_prev	= mb_mode_tmp_ptr;
}

PUBLIC void Mp4Dec_output_one_frame (MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
	Mp4DecStorablePic *pic = PNULL;
	VOP_PRED_TYPE_E VopPredType = vop_mode_ptr->VopPredType;//pre_vop_type;

	if(!vop_mode_ptr->VSP_used)
	{
		if(vop_mode_ptr->VT_used )
		{
			if(vop_mode_ptr->post_filter_en)
			{
				write_display_frame( vop_mode_ptr, vop_mode_ptr->pCurDispFrame->pDecFrame);
			}
			else
			{
				write_display_frame(vop_mode_ptr,vop_mode_ptr->pCurRecFrame->pDecFrame);
			}
		}
		else
		{
#ifndef YUV_THREE_PLANE
			if(VopPredType != BVOP)
			{
				write_display_frame(vop_mode_ptr,vop_mode_ptr->pCurRecFrame->pDecFrame);
			}
#endif
		}
	}
		
	/*output frame for display*/
	if(VopPredType != BVOP)
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
#ifdef _VSP_LINUX_
			dec_output_ptr->pBufferHeader = display_frame->pBufferHeader;
#endif	
		}else
		{
			dec_output_ptr->frameEffective = 0;
		}
	}else
	{
		if(pic != PNULL)
		{
			if (!vop_mode_ptr->VSP_used && !vop_mode_ptr->VT_used)
			{
#ifndef YUV_THREE_PLANE
				if (VopPredType == BVOP && (vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL))
				{
					int32 size_y = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

					memcpy (pic->pDecFrame->imgY, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgY, size_y);
					memcpy (pic->pDecFrame->imgU, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgU, size_y>>1);

				}
#else
				if (VopPredType == BVOP && (vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL))
				{
					int32 size_y = vop_mode_ptr->FrameExtendWidth * vop_mode_ptr->FrameExtendHeigth;

					memcpy (pic->pDecFrame->imgY, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgY, size_y);
					memcpy (pic->pDecFrame->imgU, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgU, size_y>>2);
					memcpy (pic->pDecFrame->imgV, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgV, size_y>>2);
				}
#endif
			}
			dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
			dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
			dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
#ifdef _VSP_LINUX_			
			dec_output_ptr->pBufferHeader = pic->pDecFrame->pBufferHeader;
#endif
			dec_output_ptr->is_transposed = 0;
		#ifndef YUV_THREE_PLANE
			dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
		#else
			dec_output_ptr->frame_width = vop_mode_ptr->FrameExtendWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameExtendHeigth;
		#endif
			dec_output_ptr->frameEffective = 1;
			dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;
#ifndef _VSP_LINUX_				
//			pic->pDecFrame->bDisp = TRUE; 
#endif
		}else
		{
#if _DEBUG_				
			SCI_TRACE_LOW("OUTPUT pic is NULL\n");
#endif
			dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
			dec_output_ptr->frameEffective = 0;
		}
	}		
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
