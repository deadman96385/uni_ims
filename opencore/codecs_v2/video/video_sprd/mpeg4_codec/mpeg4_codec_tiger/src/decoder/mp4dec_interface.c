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
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define    DECODE_BUFFER_SIZE    (480 * 1024)  //@Zhemin.Lin, CR49137
/*
void dump_file1( uint8* pBuffer,uint32 aInBufSize)
{
	FILE *fp = fopen("/data/tmp1.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}

void dump_file2( uint8* pBuffer,uint32 aInBufSize)
{
	FILE *fp = fopen("/data/tmp2.yuv","ab");
	fwrite(pBuffer,1,aInBufSize,fp);
	fclose(fp);
}
*/
 void dump_cmd_info()
{	
  	FILE* info = fopen("/data/info.txt","wb");
	FILE* data = fopen("/data/data.txt","wb");  

	FILE* info_vsp= fopen("/data/info_vsp.txt","wb");
	FILE* data_vsp = fopen("/data/data_vsp.txt","wb");  
	
        int length = 0;
	int i = 0;

	length = g_cmd_info_ptr - g_cmd_info_base;

	SCI_TRACE_LOW("dump info lenght %d",length);
       	if(NULL != info)
  		fwrite(g_cmd_info_base,1,length,info);

	 if(NULL != info_vsp)
  		fwrite(g_vsp_cmd_info_base,1,length,info_vsp);

	length = g_cmd_data_ptr - g_cmd_data_base;

	SCI_TRACE_LOW("dump data lenght %d",length);
	
	if(NULL != data)
  		fwrite(g_cmd_data_base,1,length,data);

	if(NULL != data_vsp)
  		fwrite(g_vsp_cmd_data_base,1,length,data_vsp);
	
	if(NULL != info)fclose(info);
	if(NULL != data)fclose(data);
	if(NULL != info_vsp)fclose(info_vsp);
	if(NULL != data_vsp)fclose(data_vsp);
}

FILE* m4v_output_yuv = NULL;
void dump_output_yuv()
{
	
        int length = 0;
	int i = 0;
        int width = g_dec_vop_mode_ptr->FrameWidth;
	int height = g_dec_vop_mode_ptr->FrameHeight;
	unsigned char * src_y = g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgY;
	unsigned char * src_u= src_y+width*height;
	unsigned char * src_v = src_u+width*height/4;
	
	SCI_TRACE_LOW("dump yuv frame_num %d,current imgY %x, PhyAddr %x",g_nFrame_dec,g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgY,\
		g_dec_vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr);
	if(g_nFrame_dec == 0 || g_nFrame_dec == 247)
	{
		m4v_output_yuv = fopen("/data/m4v_out.yuv","wb");  
	}

	SCI_TRACE_LOW("dump_output_yuv width %d,height %d,src_y  %x %x %x",width,height,src_y,src_u,src_v);
	if(m4v_output_yuv != NULL )
	{
        	fwrite(src_y,1,width*height,m4v_output_yuv);
		fwrite(src_u,1,width*height/4,m4v_output_yuv);
		fwrite(src_v,1,width*height/4,m4v_output_yuv);
	}
		
	if(g_nFrame_dec== 60)
	{
		fclose(m4v_output_yuv );
	}
}
#ifdef _VSP_LINUX_
PUBLIC void MP4DecSetPostFilter(int en)
{
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	vop_mode_ptr->post_filter_en = en;
	SCI_TRACE_LOW("post_filter_en %d",vop_mode_ptr->post_filter_en);
//	vop_mode_ptr->post_filter_en = 0;
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

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy;//>>8;
	g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y);//>>8;
	g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c);//>>8;

	g_rec_buf.pBufferHeader = pBufferHeader;
#if _DEBUG_
	SCI_TRACE_LOW("MP4DecSetCurRecPic imgY %x,imgYAddr %x",g_rec_buf.imgY,g_rec_buf.imgYAddr);
#endif	
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
	SCI_ASSERT(NULL != vop_mode_ptr);	
	Mp4Dec_SetVopmode(vop_mode_ptr);

	g_dec_is_first_frame = TRUE;
	g_dec_is_stop_decode_vol = FALSE;
	g_dec_is_changed_format = FALSE;	
	vop_mode_ptr->error_flag = FALSE;	
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;

	//for H263 plus header
	h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(sizeof(H263_PLUS_HEAD_INFO_T));
	SCI_ASSERT(NULL != h263_plus_head_info_ptr);
	Mp4Dec_SetH263PlusHeadInfo(h263_plus_head_info_ptr);

	vop_mode_ptr->bitstrm_ptr = (DEC_BS_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_BS_T));	
	SCI_ASSERT(NULL != vop_mode_ptr->bitstrm_ptr);
	
	vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_MODE_T));
	SCI_ASSERT(NULL != vop_mode_ptr->pMbMode_B);

	 vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_BFR_T));
	SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr);

 	vop_mode_ptr->IntraQuantizerMatrix = (uint8 *)Mp4Dec_InterMemAlloc(sizeof(uint8)*BLOCK_SQUARE_SIZE);
	SCI_ASSERT(NULL != vop_mode_ptr->IntraQuantizerMatrix);

	 vop_mode_ptr->InterQuantizerMatrix = (uint8 *)Mp4Dec_InterMemAlloc(sizeof(uint8)*BLOCK_SQUARE_SIZE);
	SCI_ASSERT(NULL != vop_mode_ptr->InterQuantizerMatrix);
    	
	Mp4Dec_InitDecoderPara(vop_mode_ptr);

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

void PrintfRecFrame (DEC_VOP_MODE_T * vop_mode_ptr)
{
#if defined(WIN32)&&(!defined(_LIB))
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


#if  _CMODEL_&&(!defined(MPEG4DEC_LIB)) 
void write_out_rec_tmp(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int i, j;
	uint8 *p_frame;
	int width;
	int height;
	uint8 *pFrameUV;
	DEC_FRM_BFR *rec_frm;
#ifndef _ARM_	
	FILE * pf_seq = s_pDec_recon_yuv_file;
#endif	
	rec_frm = vop_mode_ptr->pCurRecFrame->pDecFrame;

	width = vop_mode_ptr->FrameWidth;
	height = vop_mode_ptr->FrameHeight;
	
	/*write Y frame*/	
	p_frame = rec_frm->imgY;

	for (i = 0; i < height; i++)
	{
#ifndef _ARM_
		fwrite (p_frame, 1, width, pf_seq);
#endif
		p_frame += width;
	}

	pFrameUV = rec_frm->imgU;
	width = width;
	height = height / 2;
	
	/*write UV frame*/
	for (i = 0; i < height; i++)
	{
#ifndef _ARM_
		fwrite (pFrameUV, 1, width, pf_seq);
#endif
		pFrameUV += width;
	}


}

#endif
LOCAL void Mp4Dec_output_one_frame (MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
	Mp4DecStorablePic *pic = PNULL;
	VOP_PRED_TYPE_E VopPredType = vop_mode_ptr->VopPredType;//pre_vop_type;
		
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
#if _DEBUG_	
        SCI_TRACE_LOW("output_one_frame : pic %x",pic);
#endif
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
#if _DEBUG_		
			if (vop_mode_ptr->pFrdRefFrame->pDecFrame)
			{
				SCI_TRACE_LOW(" Frd->imgY: %0x, \t", vop_mode_ptr->pFrdRefFrame->pDecFrame->imgY);
			}

			if (vop_mode_ptr->pBckRefFrame->pDecFrame)
			{
				SCI_TRACE_LOW(" Bck->imgY: %0x\t", vop_mode_ptr->pBckRefFrame->pDecFrame->imgY);
			}

			if (vop_mode_ptr->pCurRecFrame->pDecFrame)
			{
				SCI_TRACE_LOW("Cur->imgY: %0x\t", vop_mode_ptr->pCurRecFrame->pDecFrame->imgY);
			}

			SCI_TRACE_LOW("pic->imgY: %0x\n", pic->pDecFrame->imgY);		
#endif			
			dec_output_ptr->pOutFrameY = pic->pDecFrame->imgY;
			dec_output_ptr->pOutFrameU = pic->pDecFrame->imgU;
			dec_output_ptr->pOutFrameV = pic->pDecFrame->imgV;
			dec_output_ptr->pBufferHeader = pic->pDecFrame->pBufferHeader;
			dec_output_ptr->is_transposed = 0;
			dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
			dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
			dec_output_ptr->frameEffective = 1;
			dec_output_ptr->err_MB_num = vop_mode_ptr->err_MB_num;
#ifndef _VSP_LINUX_				
			pic->pDecFrame->bDisp = TRUE; 
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
#if 1
LOCAL MMDecRet Mp4Dec_frm_level_sync_hw_sw_pipeline (MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;
	MMDecRet ret = MMDEC_OK;

#if _DEBUG_			
	SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_pipeline: frm_num%d\n", g_nFrame_dec); 
#endif

	if (g_nFrame_dec == 0)
	{
		g_cmd_done_init = 1;
	}else
	{
		ret = VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), (TIME_OUT_CLK), "DCAM_INT_STS_OFF: polling CMD DONE initerupt");

		if (ret)
		{
			VSP_Reset();
			SCI_TRACE_LOW("H264Dec_Picture_Level_Sync, cmd NOT done");
		}else
		{
			uint32 disp_y_addr;
			uint32 uv_offset;
			
			g_cmd_done_init = 1;//!ret;

			Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);
		}
	}

	if(g_cmd_done_init)
	{
		g_cmd_done_init = 0;

		memcpy (vop_mode_ptr->frame_bistrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);
		
		/*clear time_out int_raw flag, if timeout occurs*/
	    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");

		Mp4Dec_Reset(vop_mode_ptr);
		VSP_Write_CMD_by_BSM(((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_info_base)), ((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_data_base)));
		ret = Mp4Dec_VspFrameInit(vop_mode_ptr, ((uint32)(Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->frame_bistrm_ptr))));

		cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
		cmd = cmd & (~0x10);
		cmd = cmd & (~0x200);
		cmd |= (1<<8);
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	
	}

	return ret;
}

LOCAL MMDecRet Mp4Dec_frm_level_sync_hw_sw_normal (MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;
	MMDecRet ret = MMDEC_OK;
        uint32 length= 0;
#if _DEBUG_		
	SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_normal: frm_num%d\n", g_nFrame_dec); 
#endif

	Mp4Dec_Reset(vop_mode_ptr);
#if 0
	length = (uint32)g_cmd_info_ptr - (uint32)g_cmd_info_base;
	memcpy(g_vsp_cmd_info_base,g_cmd_info_base,length);
	length = (uint32)g_cmd_data_ptr - (uint32)g_cmd_data_base;
	memcpy(g_vsp_cmd_data_base,g_cmd_data_base,length);
#endif
	//if(0 == g_nFrame_dec)
	  // 	dump_cmd_info();
	ret = Mp4Dec_VspFrameInit(vop_mode_ptr, dec_input_ptr->pStream_phy);
	SCI_TRACE_LOW("g_nFrame_dec %d ,pCurRecFrame %x",g_nFrame_dec,vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr);

	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	cmd = cmd & (~0x10);
	cmd = cmd & (~0x200);
	cmd |= (1<<8);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	

#if	1//_CMODEL_
	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), (TIME_OUT_CLK), "DCAM_INT_STS_OFF: polling CMD DONE initerupt");
#if 1//_DEBUG_		
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, "");
	SCI_TRACE_LOW("Mp4Dec_frm_level_sync_hw_sw_normal, frame_num %d,cmd done, %d\n", g_nFrame_dec,cmd);

     //    if( 61 >  g_nFrame_dec)
    //     {
    //    	dump_output_yuv();
    //     }
#endif
#else
		do{
		}while(!g_cmd_done_init);

		g_cmd_done_init = 0;		
#endif	

   	 /*clear time_out int_raw flag, if timeout occurs*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");

	//VSP_WRITE_REG (VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, ((1<<26) |(1<<25)), "'STOP AXIM");

	Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);	

	return MMDEC_OK;
}

#endif
/*****************************************************************************/
//  Description:   Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/

int32 s_hw_error_flag;
int g_mpeg4_dec_err_flag;
PUBLIC MMDecRet MP4DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret = MMDEC_ERROR;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	uint32 cmd;
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
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;

	Mp4Dec_VerifyBitstrm(dec_input_ptr->pStream, dec_input_ptr->dataLen);

	if (vop_mode_ptr->error_flag)
	{
		vop_mode_ptr->error_flag = FALSE;
		vop_mode_ptr->return_pos2 |= (1<<5);
		return MMDEC_STREAM_ERROR;
	}
	Mp4Dec_InitBitstream_sw(g_dec_vop_mode_ptr->bitstrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);

	if(ITU_H263 == vop_mode_ptr->video_std)
	{
		ret = Mp4Dec_DecH263Header(vop_mode_ptr);
	}else if(MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->find_vop_header  = 0;
		ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
		if(!vop_mode_ptr->find_vop_header)
		{
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
		g_mpeg4_dec_err_flag |= 1<<1;		

		if (vop_mode_ptr->VopPredType == BVOP)
		{
			//here, error occur may be NO refereance frame for BVOP, so we skip this B frame.
			return MMDEC_OK;
		}else
		{
			return ret;
		}
	}

	if(dec_input_ptr->expected_IVOP && (vop_mode_ptr->VopPredType != IVOP))
	{
        if (g_nFrame_dec)
	    {
			g_nFrame_dec++;
	    }
		g_mpeg4_dec_err_flag |= 1<<2;			
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
			g_mpeg4_dec_err_flag |= 1<<3;		        
            return MMDEC_MEMORY_ERROR;
        }
        vop_mode_ptr->bInitSuceess = TRUE;
    }
	dec_output_ptr->vop_type=(vop_mode_ptr->VopPredType == NVOP)?1:0;
	
	if(vop_mode_ptr->VopPredType != NVOP)
	{
		//sorenson H263 and itu h.263 dont support B frame.
		if((BVOP == vop_mode_ptr->VopPredType) &&  (MPEG4 != vop_mode_ptr->video_std) )
		{
#ifdef _VSP_LINUX_		
			dec_output_ptr->VopPredType = NVOP;
#endif		
			return MMDEC_OK;
		}
		
		if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
		{
			g_mpeg4_dec_err_flag |= 1<<4;			
			return MMDEC_OUTPUT_BUFFER_OVERFLOW;
		}	
		
		if (vop_mode_ptr->post_filter_en)
		{
			if(dec_input_ptr->beDisplayed)
			{
				if(!Mp4Dec_GetCurDispFrameBfr(vop_mode_ptr))
				{
					g_mpeg4_dec_err_flag |= 1<<5;					
					return MMDEC_OUTPUT_BUFFER_OVERFLOW;
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

	if(g_nFrame_dec == 13)
	{
		foo();
	}
#endif //
	
#if !_CMODEL_
	g_cmd_data_ptr = g_cmd_data_base;
	g_cmd_info_ptr = g_cmd_info_base;
#endif	

        s_hw_error_flag = 0;
	Mp4Dec_Reset (vop_mode_ptr);
	Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);
	
//	cmd = (jpeg_fw_codec->YUV_Info_0.v_data_ptr == NULL)?0:1;//00: uv_interleaved, two plane, 1: three plane
	cmd = vop_mode_ptr->uv_interleaved ?0:1;
	cmd = (cmd<<27);
	cmd  |= 0x1ff;
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd, "configure AHB register: BURST_GAP, and frame_mode");


	if(2 ==vop_mode_ptr->uv_interleaved )//vu_interleaved
	{
		cmd |= ((1<<21)|(1<<18));
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
	}

	/*init vsp command*/
   	 cmd = (1<<17) |(0<<16) |(0<<15) | (0<<14) |(1<<12) | ((vop_mode_ptr->video_std)<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
     	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0, disable dbk/mbc out");

	/*frame size, YUV format, max_X, max_Y*/
	cmd = (JPEG_FW_YUV420 << 24) | (vop_mode_ptr->MBNumY << 12) | vop_mode_ptr->MBNumX;
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");
	VSP_WRITE_CMD_INFO((VSP_GLB << 29) | (2<<24) | (GLB_CFG1_WOFF<<8) | (GLB_CFG0_WOFF));

	//dbk.
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CFG_OFF, (vop_mode_ptr->post_filter_en<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: enable/disable post-filter and free_run_mode");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, 0x33333333, "configure bs h1");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, 0x33333333, "configure bs v1");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_0, "DBK_VDB_BUF_ST: init ping-pong buffer to be available");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");
	VSP_WRITE_CMD_INFO( (VSP_DBK << 29)| (5<<24) |(HDBK_BS_V1_WOFF<<16) | (HDBK_BS_H1_WOFF<<8) | (DBK_CFG_WOFF));
	VSP_WRITE_CMD_INFO( (DBK_CTR1_WOFF<<8) | DBK_VDB_BUF_ST_WOFF);

	//Mp4Dec_InitBitstream(dec_input_ptr->pStream, dec_input_ptr->dataLen);//leon to be modify??
	Mp4Dec_InitBitstream(((void *)Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->frame_bistrm_ptr)), dec_input_ptr->dataLen);
#if _CMODEL_
	Mp4Dec_Reset (vop_mode_ptr);
	ret = Mp4Dec_VspFrameInit(vop_mode_ptr, (uint32)(dec_input_ptr->pStream));
#endif

	if(IVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t I VOP\t%d\n", dec_input_ptr->dataLen); 

	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: IVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		if(!vop_mode_ptr->bDataPartitioning)
		{
			ret = Mp4Dec_DecIVOP(vop_mode_ptr); 
		}
	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			ret = Mp4Dec_DecIVOPErrResDataPartitioning(vop_mode_ptr);
		}
	#endif //_MP4CODEC_DATA_PARTITION_	

	#if 0
	//	if (dec_input_ptr->err_pkt_num)
		if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
		{
			s_hw_error_flag = 1;
						
			Mp4Dec_Reset(vop_mode_ptr);
			Mp4Dec_VspFrameInit(vop_mode_ptr,vop_mode_ptr->frame_bistrm_ptr);
	        	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
     	 		Mp4Dec_EC_IVOP(vop_mode_ptr);
		}
	#endif	
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t P VOP\t%d  frame: %d\n", dec_input_ptr->dataLen, g_nFrame_dec); 
		
	#if _TRACE_		 
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: PVOP\n", g_nFrame_dec);
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
					MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
				}
				g_mpeg4_dec_err_flag |= 1<<7;				
				return MMDEC_ERROR;
			}
		}		

		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
		VSP_WRITE_CMD_INFO((VSP_MCA<< 29) | (1<<24) | MCA_CFG_WOFF);

		if(!vop_mode_ptr->bDataPartitioning)
		{
			ret = Mp4Dec_DecPVOP(vop_mode_ptr) ;
		}
	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			ret = Mp4Dec_DecPVOPErrResDataPartitioning(vop_mode_ptr);
		}
	#endif //_MP4CODEC_DATA_PARTITION_	

	#if 0
	//	if (dec_input_ptr->err_pkt_num)
		if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
		{
			s_hw_error_flag=  1;
	
			Mp4Dec_Reset(vop_mode_ptr);
	        	Mp4Dec_VspFrameInit(vop_mode_ptr,vop_mode_ptr->frame_bistrm_ptr);
	        	VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
		 	Mp4Dec_EC_PVOP(vop_mode_ptr);
		}
	#endif	
	}else if(SVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("\t S VOP, Don't SUPPORTED!\n");  
//		PRINTF ("\t S VOP!\n"); 
	#if _TRACE_		 	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: SVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		
//		Mp4Dec_DecPVOP(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
	}else if(NVOP == vop_mode_ptr->VopPredType)
	{
		SCI_TRACE_LOW ("frame not coded!\n");
       		MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
		return MMDEC_OK;
	}else
	{
		SCI_TRACE_LOW ("\t B VOP\t%d %d\n", dec_input_ptr->dataLen,g_nFrame_dec);
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\nnframe: %d, frame type: BVOP\n", g_nFrame_dec);
	#endif //_TRACE_	
		vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
		vop_mode_ptr->err_MB_num = 0;
		Mp4Dec_DecBVOP(vop_mode_ptr);
	}
//	PrintfRecFrame (vop_mode_ptr);
	
	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;

	/*frame end synchrolization*/
	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 16, 1, 1, "DBK_CTR1: polling dbk frame idle");
	VSP_WRITE_CMD_INFO( (VSP_DBK << 29) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

	VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "AHBM_STS: polling AHB idle status");
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec, "VSP_TST: configure frame_cnt to debug register for end of picture");
	VSP_WRITE_CMD_INFO( (VSP_GLB << 29) | (2<<24) | (VSP_TST_WOFF<<8) | ((1<<7)|VSP_DBG_WOFF));

	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TST_OFF, 0x12345678, "VSP_TST: finished one frame");
	VSP_WRITE_CMD_INFO(0x12345678);
	
	if(vop_mode_ptr->err_MB_num)//@simon.wang 2012.2.18
	{	
		SCI_TRACE_LOW ("parsing vop syntax error!mb_x: %d, mb_y: %d, err_mb_num: %d, return_pos: %x, return_pos1:%x, return_pos2: %x, dec_err_flag: %d\n", 
			vop_mode_ptr->mb_x, vop_mode_ptr->mb_y, vop_mode_ptr->err_MB_num, vop_mode_ptr->return_pos, vop_mode_ptr->return_pos1, vop_mode_ptr->return_pos2, g_mpeg4_dec_err_flag);
 		return MMDEC_STREAM_ERROR;
	}
#if 0	
	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	cmd = cmd & (~0x10);
	cmd = cmd& (~0x200);
	cmd |= (1<<8);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	

#if _CMODEL_
	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), (TIME_OUT_CLK), "DCAM_INT_STS_OFF: polling CMD DONE initerupt");
#else
	do{
	}while(!g_cmd_done_init);

	g_cmd_done_init = 0;		
#endif	

   	 /*clear time_out int_raw flag, if timeout occurs*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");

    /*cleared above,so this time should not poll*/
//	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), TIME_OUT_CLK, "DCAM_INT_STS_OFF: polling CMD DONE initerupt");
    
	VSP_WRITE_REG (VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, ((1<<26) |(1<<25)), "'STOP AXIM");
#endif //if 0
	

// 	if(MMDEC_OK == ret)
	{	
		
        	ret = Mp4Dec_frm_level_sync_hw_sw_normal (dec_input_ptr, dec_output_ptr, vop_mode_ptr);
		Mp4Dec_exit_picture(vop_mode_ptr);
	}

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
