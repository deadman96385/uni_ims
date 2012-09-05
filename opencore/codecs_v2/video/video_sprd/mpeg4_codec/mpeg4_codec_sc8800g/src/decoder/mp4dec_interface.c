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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define    DECODE_BUFFER_SIZE    (480 * 1024)  //@Zhemin.Lin, CR49137


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
	int32 size_y = ((vop_mode_ptr->OrgFrameWidth  + 15) / MB_SIZE) * ((vop_mode_ptr->OrgFrameHeight+ 15) / MB_SIZE)*256;	
       int32 size_c = size_y >> 2;
	  
	g_rec_buf.imgY =  pFrameY;
	g_rec_buf.imgU =  pFrameY+size_y;
	g_rec_buf.imgV =  pFrameY+size_y+size_c;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy>>8;
	g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y)>>8;
	g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c)>>8;

	g_rec_buf.pBufferHeader = pBufferHeader;
	
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

void MP4DecReleaseRefBuffers()
{
	int i;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	if(!vop_mode_ptr)
		return;
	if (!vop_mode_ptr->post_filter_en)
	{
		for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
		{
			if(g_FrmYUVBfr[i].bRef)
			{
				g_FrmYUVBfr[i].bRef = FALSE;
				if(g_FrmYUVBfr[i].pBufferHeader!=NULL){
				 	(*VSP_unbindCb)(g_user_data,g_FrmYUVBfr[i].pBufferHeader,1);
				 	//g_FrmYUVBfr[i].pBufferHeader = NULL;
				}
			}
		}
	}else
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
	if (!vop_mode_ptr->post_filter_en)
	{
		vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
	        vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
	}
	g_nFrame_dec = 0;
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

	Mp4Dec_InitInterMem(pBuffer);
	
	Mp4Dec_InitGlobal();

	vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_VOP_MODE_T));
	if(NULL == vop_mode_ptr)
	{
		return MMDEC_MEMORY_ERROR;
	}
	Mp4Dec_SetVopmode(vop_mode_ptr);

	g_dec_is_first_frame = TRUE;
	g_dec_is_stop_decode_vol = FALSE;
	g_dec_is_changed_format = FALSE;	
	vop_mode_ptr->error_flag = FALSE;	

	//for H263 plus header
	h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(sizeof(H263_PLUS_HEAD_INFO_T));
	if(NULL == h263_plus_head_info_ptr)
	{
		return MMDEC_MEMORY_ERROR;
	}
	Mp4Dec_SetH263PlusHeadInfo(h263_plus_head_info_ptr);

	vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_MODE_T)); 
    if(NULL == vop_mode_ptr->pMbMode_B)
    {
		return MMDEC_MEMORY_ERROR;
	}	

    vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_BFR_T));
	if(NULL == vop_mode_ptr->mb_cache_ptr)
    {
		return MMDEC_MEMORY_ERROR;
	}
    	
	Mp4Dec_InitDecoderPara(vop_mode_ptr);	

	vop_mode_ptr->uv_interleaved = video_format_ptr->uv_interleaved;
	vop_mode_ptr->video_std = video_format_ptr->video_std;
	vop_mode_ptr->size_decode_flag = FALSE;
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std != MPEG4)
	{
		PRINTF("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{
		Mp4Dec_Reset(vop_mode_ptr);
		
		if(video_format_ptr->i_extra > 0)
		{
			PRINTF("\nIt is MPEG-4 bitstream!\n");
#ifdef  _VSP_LINUX_
			Mp4Dec_InitBitstream(video_format_ptr->p_extra_phy, video_format_ptr->i_extra);
#else
			Mp4Dec_InitBitstream(video_format_ptr->p_extra, video_format_ptr->i_extra);
#endif
			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);
#ifdef  _VSP_LINUX_
			if(MMDEC_OK == is_init_success)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
			}
#endif			
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
    if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;
g_mpeg4_dec_err_flag |= 1;
        return MMDEC_OUTPUT_BUFFER_OVERFLOW;
    }

	vop_mode_ptr->sw_vld_flag = FALSE;

FLV_RE_DEC:
	vop_mode_ptr->error_flag = FALSE;

	Mp4Dec_VerifyBitstrm(dec_input_ptr->pStream, dec_input_ptr->dataLen);

	if (vop_mode_ptr->error_flag)
	{
		vop_mode_ptr->error_flag = FALSE;
g_mpeg4_dec_err_flag |= 1<<1;		
		return MMDEC_STREAM_ERROR;
	}

	Mp4Dec_Reset(vop_mode_ptr);
#ifdef  _VSP_LINUX_
	Mp4Dec_InitBitstream(dec_input_ptr->pStream_phy, dec_input_ptr->dataLen);
#else
	Mp4Dec_InitBitstream(dec_input_ptr->pStream, dec_input_ptr->dataLen);
#endif
	if(ITU_H263 == vop_mode_ptr->video_std)
	{
		ret = Mp4Dec_DecH263Header(vop_mode_ptr);
	}else if(MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->find_vop_header  = 0;
		ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
		if(!vop_mode_ptr->find_vop_header){
			dec_output_ptr->VopPredType = NVOP;
			return MMDEC_OK;
		}
	}else
	{
		ret = Mp4Dec_FlvH263PicHeader(vop_mode_ptr);
	}

	if(ret != MMDEC_OK)
	{
		//modified by xwluo, 20100511
g_mpeg4_dec_err_flag |= 1<<2;			
		return ret;//MMDEC_ERROR;
	}

	if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
	{
		g_nFrame_dec++;
g_mpeg4_dec_err_flag |= 1<<3;		
		return MMDEC_FRAME_SEEK_IVOP;
	}

	dec_output_ptr->frameEffective = FALSE;
	dec_output_ptr->pOutFrameY = PNULL;
	dec_output_ptr->pOutFrameU = PNULL;
	dec_output_ptr->pOutFrameV = PNULL;
	
    if(!vop_mode_ptr->bInitSuceess)
    {
        if( MMDEC_OK != Mp4Dec_InitSessionDecode(vop_mode_ptr) )
        {
g_mpeg4_dec_err_flag |= 1<<4;	        
            return MMDEC_MEMORY_ERROR;
        }
        vop_mode_ptr->bInitSuceess = TRUE;
    }

	if(vop_mode_ptr->VopPredType != NVOP)
	{
		//sorenson H263 and itu h.263 dont support B frame.
		if((BVOP == vop_mode_ptr->VopPredType) && 
		   (MPEG4 != vop_mode_ptr->video_std) )
		{
#ifdef _VSP_LINUX_		
			dec_output_ptr->VopPredType = NVOP;
#endif
			return MMDEC_OK;
		}
		
		if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
		{
g_mpeg4_dec_err_flag |= 1<<5;		
			return MMDEC_OUTPUT_BUFFER_OVERFLOW;
		}	
		
		if (vop_mode_ptr->post_filter_en)
		{
			if(dec_input_ptr->beDisplayed)
			{
				if(!Mp4Dec_GetCurDispFrameBfr(vop_mode_ptr))
				{
g_mpeg4_dec_err_flag |= 1<<6;					
					return MMDEC_OUTPUT_BUFFER_OVERFLOW;
				}
			}
		}else //for fpga verification
		{
			vop_mode_ptr->pCurDispFrame = vop_mode_ptr->pCurRecFrame;
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
	//SCI_TRACE_LOW("MP4DecDecode Mp4Dec_InitVop %d,%d,%d\n",vop_mode_ptr->VopPredType,vop_mode_ptr->bDataPartitioning,dec_input_ptr->err_pkt_num);
	s_hw_error_flag = 0;
	ret = Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);
	if (ret != MMDEC_OK)
	{
g_mpeg4_dec_err_flag |= 1<<7;		
		return ret;
	}	

	if(IVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t I VOP\t%d\t", dec_input_ptr->dataLen); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: IVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		if(!vop_mode_ptr->bDataPartitioning)
		{
			ret = Mp4Dec_DecIVOP(vop_mode_ptr); 
			
			if (vop_mode_ptr->error_flag && vop_mode_ptr->video_std == FLV_H263)
			{
				if (!vop_mode_ptr->sw_vld_flag)
				{
					MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
					vop_mode_ptr->sw_vld_flag = TRUE;
					
				}
				goto FLV_RE_DEC;
			}
		}
	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			ret = Mp4Dec_DecIVOPErrResDataPartitioning(vop_mode_ptr);
		}
	#endif //_MP4CODEC_DATA_PARTITION_	

if (vop_mode_ptr->post_filter_en){ //jgdu
	//	if (dec_input_ptr->err_pkt_num)
		if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
		{
			s_hw_error_flag = 1;
						
			Mp4Dec_Reset(vop_mode_ptr);
			ret =Mp4Dec_VspFrameInit(vop_mode_ptr);
	        VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
     	 	Mp4Dec_EC_IVOP(vop_mode_ptr);
#ifndef _VSP_LINUX_			
            MMU_InvalideDCACHE();
#endif
		}
}		
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t P VOP\t%d\t", dec_input_ptr->dataLen); 
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: PVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
	
if (vop_mode_ptr->post_filter_en){ //jgdu
		if (vop_mode_ptr->pBckRefFrame->pDecFrame == PNULL)
		{
			if( (PNULL != vop_mode_ptr->pCurDispFrame) && 
				(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame) && 
				(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame->imgY)
			  )
			{
				MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
			}
g_mpeg4_dec_err_flag |= 1<<8;				
			return MMDEC_ERROR;
		}
}
		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");

		if(!vop_mode_ptr->bDataPartitioning)
		{
			ret = Mp4Dec_DecPVOP(vop_mode_ptr) ;
			
			if (vop_mode_ptr->error_flag && vop_mode_ptr->video_std == FLV_H263)
			{
				if (!vop_mode_ptr->sw_vld_flag)
				{
					MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
					vop_mode_ptr->sw_vld_flag = TRUE;
				}
				goto FLV_RE_DEC;
			}
		}
	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			ret = Mp4Dec_DecPVOPErrResDataPartitioning(vop_mode_ptr);
		}
	#endif //_MP4CODEC_DATA_PARTITION_	


if (vop_mode_ptr->post_filter_en){ //jgdu
	//	if (dec_input_ptr->err_pkt_num)
		if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
		{
			s_hw_error_flag=  1;
	
			Mp4Dec_Reset(vop_mode_ptr);
	        ret = Mp4Dec_VspFrameInit(vop_mode_ptr);
	        VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
		 	Mp4Dec_EC_PVOP(vop_mode_ptr);
#ifndef _VSP_LINUX_			
            MMU_InvalideDCACHE();
#endif
		}
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
        MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
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
	
	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;

	
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
	if(s_hw_error_flag || READ_REG_POLL(VSP_DBK_REG_BASE+DBK_CTR1_OFF, V_BIT_0, 0, TIME_OUT_CLK, "DBK_CTR1: polling DBK_CFG_FLAG flag = 0"))
	{
g_mpeg4_dec_err_flag |= 1<<9;		
		return MMDEC_HW_ERROR;
	}

	if(s_hw_error_flag || READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status"))
	{
		PRINTF("TIME OUT!\n");
g_mpeg4_dec_err_flag |= 1<<10;			
		return MMDEC_HW_ERROR;
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
#ifdef _VSP_LINUX_
				dec_output_ptr->pBufferHeader = display_frame->pBufferHeader;
#endif
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
			if ( (PNULL != pic) && (PNULL != pic->pDecFrame))
			{
				dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
				dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
				dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
				dec_output_ptr->is_transposed = 0;
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
				dec_output_ptr->frameEffective = 1;
				dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;
#ifndef _VSP_LINUX_				
				pic->pDecFrame->bDisp = TRUE;
#endif
#ifdef _VSP_LINUX_
				dec_output_ptr->pBufferHeader = pic->pDecFrame->pBufferHeader;
#endif
			}else
			{
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
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


//	SCI_TRACE_LOW("MP4DecDecode frameEffective %d,%d,%d\n",dec_output_ptr->frameEffective,ret,Mp4Dec_ByteConsumed());
#ifdef _VSP_LINUX_
{
	uint32 ByteConsumed = Mp4Dec_ByteConsumed();
	if(ByteConsumed > dec_input_ptr->dataLen)
		dec_input_ptr->dataLen = 0;
	else
		dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;
}
#endif
g_mpeg4_dec_err_flag = (1<<31)|ret;	
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
