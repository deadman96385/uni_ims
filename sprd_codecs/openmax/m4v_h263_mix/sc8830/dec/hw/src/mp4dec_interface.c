/******************************************************************************
 ** File Name:    mp4dec_interface.c										  *
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

FunctionType_BufCB VSP_bindCb = NULL;
FunctionType_BufCB VSP_unbindCb = NULL;
void *g_user_data = NULL;
FunctionType_MemAllocCB VSP_mallocCb = NULL;

uint32 video_size_get = 0;
int g_mpeg4_dec_err_flag = 0;
SLICEINFO SliceInfo;

/*************************************/
/* functions needed for android platform */
/*************************************/
PUBLIC void OR_VSP_BIND(void *pHeader)
{
	 (*VSP_bindCb)(g_user_data,pHeader,0);
}

PUBLIC void OR_VSP_UNBIND(void *pHeader)
{
   	(*VSP_unbindCb)(g_user_data,pHeader,0);
}




PUBLIC void MP4DecSetPostFilter(MP4Handle *mp4Handle, int en)
{
	// Shark VSP DO NOT support post filter for MPEG4 decoder.
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	vop_mode_ptr->post_filter_en = 0;
}

PUBLIC void MP4DecSetCurRecPic(MP4Handle *mp4Handle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	g_rec_buf.imgY =  pFrameY;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy;

	g_rec_buf.pBufferHeader = pBufferHeader;	
	
}

void MP4DecReleaseRefBuffers(MP4Handle *mp4Handle)
{

	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	int i;
	
	if(!vop_mode_ptr)
		return;

	for(i = 0; i < DEC_YUV_BUFFER_NUM; i++)
	{
		if(g_FrmYUVBfr[i].bRef)
		{
			g_FrmYUVBfr[i].bRef = FALSE;
			if((*mp4Handle->VSP_unbindCb) &&g_FrmYUVBfr[i].pBufferHeader!=NULL){
			 	(*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,g_FrmYUVBfr[i].pBufferHeader,0);
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
				if((*mp4Handle->VSP_unbindCb) && g_DispFrmYUVBfr[i].pBufferHeader!=NULL){
	                     	(*mp4Handle->VSP_unbindCb)(mp4Handle->userdata,g_DispFrmYUVBfr[i].pBufferHeader,0);
					//g_DispFrmYUVBfr[i].pBufferHeader = NULL;
				}
			}
		}
	}
#if 0 //bug127759    
	if (!vop_mode_ptr->post_filter_en)
#endif     
	{
		vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
	        vop_mode_ptr->pBckRefFrame->pDecFrame = NULL;
	}
	g_nFrame_dec = 0;
}

int MP4DecGetLastDspFrm(MP4Handle *mp4Handle,void **pOutput)
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


PUBLIC void Mp4GetVideoDimensions(MP4Handle *mp4Handle, int32 *display_width, int32 *display_height)
{
//    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
//	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

	 *display_width = vop_mode_ptr->OrgFrameWidth;
	 *display_height =  vop_mode_ptr->OrgFrameHeight;

        SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *display_width, *display_height);
}

PUBLIC void Mp4GetBufferDimensions(MP4Handle *mp4Handle, int32 *width, int32 *height) 
{
//    	MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
//	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	
    	*width =( (vop_mode_ptr->OrgFrameWidth + 15) >>4) <<4;
    	*height = ( (vop_mode_ptr->OrgFrameHeight + 15) >>4) <<4;

        SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *width, *height);
}


MMDecRet MP4DecInit(MP4Handle *mp4Handle, MMCodecBuffer * buffer_ptr)
{
	MMDecRet is_init_success = MMDEC_OK;
	DEC_VOP_MODE_T *vop_mode_ptr = NULL;
	H263_PLUS_HEAD_INFO_T *h263_plus_head_info_ptr = NULL;


	VSP_mallocCb = mp4Handle->VSP_extMemCb;
	VSP_bindCb = mp4Handle->VSP_bindCb;
	VSP_unbindCb = mp4Handle->VSP_unbindCb;
	g_user_data = mp4Handle->userdata;

#ifndef _FPGA_TEST_
	// Open VSP device
	if(VSP_OPEN_Dev()<0)
	{
		return MMDEC_HW_ERROR;
	}	
#else
	TEST_VSP_ENABLE();
#endif

	Mp4Dec_InitInterMem(buffer_ptr);

	vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_VOP_MODE_T));
	SCI_ASSERT(NULL != vop_mode_ptr);	
    	Mp4Dec_SetVopmode(vop_mode_ptr);

	g_nFrame_dec = 0;

	g_dec_is_first_frame = TRUE;
	g_dec_is_stop_decode_vol = FALSE;
	g_dec_is_changed_format = FALSE;	
	g_dec_pre_vop_format = IVOP;

	g_is_need_init_vsp_hufftab = TRUE;
	g_is_need_init_vsp_dcttab= TRUE;
	
	vop_mode_ptr->error_flag = FALSE;	
	


	//for H263 plus header
	h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_InterMemAlloc(sizeof(H263_PLUS_HEAD_INFO_T));
	SCI_ASSERT(NULL != h263_plus_head_info_ptr);
	Mp4Dec_SetH263PlusHeadInfo(h263_plus_head_info_ptr);
#if SIM_IN_WIN
	vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_MODE_T)); 
    SCI_ASSERT(NULL != vop_mode_ptr->pMbMode_B);
    vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_InterMemAlloc(sizeof(DEC_MB_BFR_T));
	SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr); 	

#else
	Mp4Dec_InitDecoderPara(vop_mode_ptr);	

	
    vop_mode_ptr->uv_interleaved = /*video_format_ptr->uv_interleaved = */1;
	//vop_mode_ptr->video_std = video_format_ptr->video_std;
	vop_mode_ptr->intra_acdc_pred_disable = TRUE;
	vop_mode_ptr->QuantizerType = Q_H263;
	vop_mode_ptr->bDataPartitioning = FALSE;
	vop_mode_ptr->bReversibleVlc = FALSE;
	vop_mode_ptr->bResyncMarkerDisable = TRUE;
	vop_mode_ptr->bAlternateScan = FALSE;
	vop_mode_ptr->bQuarter_pel = FALSE;
	vop_mode_ptr->bInterlace = FALSE;
	vop_mode_ptr->bInitSuceess = FALSE;	
	memset(vop_mode_ptr->InterQuantizerMatrix,0,64);
    memset(vop_mode_ptr->IntraQuantizerMatrix,0,64);
	vop_mode_ptr->NumGobInVop=0;
    vop_mode_ptr->NumMBInGob=0;
	vop_mode_ptr->num_mbline_gob=0;
    vop_mode_ptr->last_non_b_time=0;
    vop_mode_ptr->last_time_base=0;
	vop_mode_ptr->time_base=0;
	vop_mode_ptr->time=0;
	vop_mode_ptr->time_pp=0;
    vop_mode_ptr->time_bp=0;
	vop_mode_ptr->mvInfoForward.FCode=0;
    vop_mode_ptr->mvInfoBckward.FCode=0;
	vop_mode_ptr->QuantPrecision = 5;
	vop_mode_ptr->bCoded = TRUE;
	vop_mode_ptr->RoundingControl = 0;
	vop_mode_ptr->IntraDcSwitchThr = 0;

	g_rvlc_tbl_ptr = (uint32*)Mp4Dec_InterMemAlloc(sizeof(uint32)*146);
	g_huff_tbl_ptr= (uint32*)Mp4Dec_InterMemAlloc(sizeof(uint32)*152);	

	memcpy(g_rvlc_tbl_ptr, g_rvlc_huff_tab,(sizeof(uint32)*146));
	memcpy(g_huff_tbl_ptr, g_mp4_dec_huff_tbl,(sizeof(uint32)*152));

#endif

#if 0
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std !=VSP_MPEG4)
	{
		PRINTF("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{
		#if SIM_IN_WIN
		Mp4Dec_Reset(vop_mode_ptr);
		#endif
		if(video_format_ptr->i_extra > 0)
		{
			#if SIM_IN_WIN
			PRINTF("\nIt is MPEG-4 bitstream!\n");
			Mp4Dec_InitBitstream(video_format_ptr->p_extra, video_format_ptr->i_extra);
		   #endif
			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);

		 	if(MMDEC_OK == is_init_success)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
			}
		}

	#if 0 //removed to MP4DecDecode() function.
		if(MMDEC_OK == is_init_success)
		{
			Mp4Dec_InitSessionDecode(vop_mode_ptr);
			vop_mode_ptr->bInitSuceess = TRUE;
		}
	#endif	
	}
#endif    

	return is_init_success;
}


PUBLIC MMDecRet MP4DecVolHeader(MP4Handle *mp4Handle, MMDecVideoFormat *video_format_ptr)
{
        MMDecRet ret = MMDEC_OK;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

	vop_mode_ptr->video_std = video_format_ptr->video_std;	
	
	/*judge h.263 or mpeg4*/
	if(video_format_ptr->video_std != STREAM_ID_MPEG4)
	{
		ALOGE ("\nH263(ITU or Sorenson format) is detected!\n");
	}else 
	{				
		if(video_format_ptr->i_extra > 0)
		{
			MMDecRet is_init_success;
			uint32 bs_buffer_length, bs_start_addr;			

		       // Bitstream.
	                bs_start_addr=((uint32)video_format_ptr->p_extra_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned. 
	                bs_buffer_length= video_format_ptr->i_extra;
			g_stream_offset = 0;		
			
			if(ARM_VSP_RST()<0)
			{
				return MMDEC_HW_ERROR;
		        }
			SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
			
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_MPEG4,"VSP_MODE");

			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

			SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
			is_init_success = Mp4Dec_DecMp4Header(vop_mode_ptr, video_format_ptr->i_extra);
			SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
		 	if(MMDEC_OK == is_init_success)
			{
				video_format_ptr->frame_width = vop_mode_ptr->OrgFrameWidth;
				video_format_ptr->frame_height= vop_mode_ptr->OrgFrameHeight;
			}

			VSP_RELEASE_Dev();
			
			video_size_get = 1;

           		 mp4Handle->VSP_extMemCb(mp4Handle->userdata,  video_format_ptr->frame_width, video_format_ptr->frame_height);
		}
	}
	return ret;
}

/*****************************************************************************/
//  Description:   firware Decode one vop	
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/

PUBLIC void mpeg4_decode_vop(DEC_VOP_MODE_T *vop_mode_ptr)	
{
	int tmp;
	int pic_end=0;
	int VopPredType=vop_mode_ptr->VopPredType;
    int resyn_bits=VopPredType?vop_mode_ptr->mvInfoForward.FCode - 1:0;


	while(!pic_end)
	{	
		tmp=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		if(tmp&0x30)
		{
			vop_mode_ptr->error_flag=1;

			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame done int");
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
			pic_end=1;//weihu
		}
		else if((tmp&0x00000004)==0x00000004)
		{
		    
			vop_mode_ptr->error_flag=0;
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame done int");
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
			pic_end=0;
            
			tmp=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_DBG_STS0_OFF, "read mbx mby");
			
			vop_mode_ptr->mb_y=tmp&0xff;
           		vop_mode_ptr->mb_x=(tmp>>8)&0xff;

			if(vop_mode_ptr->mb_y==(vop_mode_ptr->MBNumY-1)&&vop_mode_ptr->mb_x==(vop_mode_ptr->MBNumX-1))
			{
				pic_end=1;
			}else
			{

				if(vop_mode_ptr->video_std!=ITU_H263)
				{	
					if(!vop_mode_ptr->bResyncMarkerDisable)
					{
						if(Mp4Dec_CheckResyncMarker(resyn_bits))
						{	
							Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,&SliceInfo, resyn_bits);
						}
					}
				}else if(VopPredType!=BVOP)
				{
					SliceInfo.GobNum++;
					Mp4Dec_DecGobHeader(vop_mode_ptr,&SliceInfo);
				}
			}

		
		}
	}	
}


int16 MP4DecDecodeShortHeader(mp4StreamType *psBits,
                         int32 *width,
                         int32 *height,
                         int32 *display_width,
                         int32 *display_height);



PUBLIC MMDecRet MP4DecDecode(MP4Handle *mp4Handle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
  	MMDecRet ret = MMDEC_ERROR;
 	int32 i;
   	uint32 bs_buffer_length, bs_start_addr;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();


	if(!video_size_get)
	{
		int32 width, height, display_width, display_height;
		mp4StreamType sBits;	
		sBits.data = dec_input_ptr->pStream;
		sBits.numBytes = dec_input_ptr->dataLen;
		sBits.bitBuf = 0;
		sBits.bitPos = 32;
		sBits.bytePos = 0;
		sBits.dataBitPos = 0;
		
		MP4DecDecodeShortHeader(&sBits,&width,&height,&display_width,&display_height);

		vop_mode_ptr->OrgFrameWidth =display_width ;
	 	vop_mode_ptr->OrgFrameHeight =display_height ;
		vop_mode_ptr->FrameWidth =width ;
	 	vop_mode_ptr->FrameHeight =height ;

		video_size_get = 1; 		
		//tmp
	        mp4Handle->VSP_extMemCb(mp4Handle->userdata,  width, height);

                return MMDEC_MEMORY_ALLOCED;	
	}

        if(ARM_VSP_RST()<0)
        {
        	return MMDEC_HW_ERROR;
         }
        SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
		
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, vop_mode_ptr->video_std,"VSP_MODE");



	// Bitstream.

	g_stream_offset =0;		// bitstream offset
	bs_start_addr=((uint32)dec_input_ptr->pStream_phy);	// bistream start address		
	bs_buffer_length= dec_input_ptr->dataLen;//bitstream length .
				
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset),"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.


	vop_mode_ptr->error_flag = FALSE;

	g_mpeg4_dec_err_flag = 0;


#if SIM_IN_WIN
     PPAParaBUF *PPAParaBuf;//=NULL;
	assert((PPAParaBuf=malloc(sizeof(PPAParaBUF)))!=NULL);
	memset(PPAParaBuf,0,sizeof(PPAParaBUF));
	 if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;

        return MMDEC_OUTPUT_BUFFER_OVERFLOW;
    }
    
	if(LENTH==NULL)
	{
		LENTH=fopen("..\\..\\test_vectors\\bs_offset.txt","w+");
		assert(LENTH!=NULL);
	}
	
	if (vop_mode_ptr->error_flag)
	{
		vop_mode_ptr->error_flag = FALSE;
		return MMDEC_STREAM_ERROR;
	}

	Mp4Dec_Reset(vop_mode_ptr);

	Mp4Dec_InitBitstream(dec_input_ptr->pStream, dec_input_ptr->dataLen);//I think this is c model @2012_10_6

#else
    if(!Mp4Dec_GetCurRecFrameBfr(vop_mode_ptr))
    {
        memset(dec_output_ptr,0,sizeof(MMDecOutput));
        dec_output_ptr->frameEffective = 0;
	g_mpeg4_dec_err_flag |= 1;	
        return MMDEC_OUTPUT_BUFFER_OVERFLOW;
    }	
 
#endif



	if(ITU_H263 == vop_mode_ptr->video_std)
	{
		ret = Mp4Dec_DecH263Header(vop_mode_ptr,SliceInfo);
	}else if(VSP_MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->find_vop_header  = 0;
		ret = Mp4Dec_DecMp4Header(vop_mode_ptr, dec_input_ptr->dataLen);
		if(!vop_mode_ptr->find_vop_header)
		{
#ifdef _VSP_LINUX_							
			dec_output_ptr->VopPredType = NVOP;
#endif
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
			dec_output_ptr->VopPredType = NVOP;
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
   


	dec_output_ptr->frameEffective = FALSE;//weihu	
	dec_output_ptr->pOutFrameY = PNULL;
	dec_output_ptr->pOutFrameU = PNULL;
	dec_output_ptr->pOutFrameV = PNULL;
	
# if 1 //SIM_IN_WIN	
    if(!vop_mode_ptr->bInitSuceess)
    {
        if( MMDEC_OK != Mp4Dec_InitSessionDecode(vop_mode_ptr) )
        {
			
            return MMDEC_MEMORY_ERROR;
        }
        vop_mode_ptr->bInitSuceess = TRUE;
    }
	
	memset(&SliceInfo,0,sizeof(SLICEINFO));
#else
	{
		if(!vop_mode_ptr->bInitSuceess)
		{
			vop_mode_ptr->MBNumX = (vop_mode_ptr->OrgFrameWidth  + 15) / MB_SIZE;
			vop_mode_ptr->MBNumY = (vop_mode_ptr->OrgFrameHeight + 15) / MB_SIZE;
			vop_mode_ptr->MBNum  = (int16)(vop_mode_ptr->MBNumX  * vop_mode_ptr->MBNumY);
			vop_mode_ptr->FrameWidth  = (int16)(vop_mode_ptr->MBNumX  * MB_SIZE);
			vop_mode_ptr->FrameHeight = (int16)(vop_mode_ptr->MBNumY  * MB_SIZE);
			
			vop_mode_ptr->MB_in_VOP_length = Mp4Dec_Compute_log2(vop_mode_ptr->MBNum);
			
			if(VSP_MPEG4 == vop_mode_ptr->video_std)
			{
				vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Dec_Compute_log2(vop_mode_ptr->time_inc_resolution);
			}
			
			vop_mode_ptr->post_filter_en = TRUE;//FALSE;
            vop_mode_ptr->sliceNumber=0;
			
			Mp4Dec_InitHuffmanTable(vop_mode_ptr);

			 g_is_need_init_vsp_hufftab=TRUE;
		}
		vop_mode_ptr->bInitSuceess = TRUE;
		
	}
#endif
     
	

	SliceInfo.video_std=vop_mode_ptr->video_std;
	SliceInfo.DataPartition=vop_mode_ptr->bDataPartitioning;
    SliceInfo.VOPCodingType=vop_mode_ptr->VopPredType;
	SliceInfo.ShortHeader=(ITU_H263 == vop_mode_ptr->video_std)?1:0;
	SliceInfo.Max_MBX=vop_mode_ptr->MBNumX;
	SliceInfo.Max_MBy=vop_mode_ptr->MBNumY;
	SliceInfo.IsRvlc=vop_mode_ptr->bReversibleVlc;;
	SliceInfo.QuantType=vop_mode_ptr->QuantizerType;
	SliceInfo.PicHeight=vop_mode_ptr->FrameHeight;
	SliceInfo.PicWidth=vop_mode_ptr->FrameWidth;
    SliceInfo.NumMbsInGob=vop_mode_ptr->NumMBInGob;
	SliceInfo.NumMbLineInGob=vop_mode_ptr->num_mbline_gob;
	SliceInfo.VopQuant=vop_mode_ptr->StepSize;
    SliceInfo.VOPFcodeFwd=vop_mode_ptr->mvInfoForward.FCode;
    SliceInfo.VOPFcodeBck=vop_mode_ptr->mvInfoBckward.FCode;
	SliceInfo.FirstMBx=0;
    SliceInfo.FirstMBy=0;
	SliceInfo.SliceNum=0;
   #if SIM_IN_WIN
	SliceInfo.pMbMode=vop_mode_ptr->pMbMode;
  #endif
    SliceInfo.VopRoundingType=(SliceInfo.VOPCodingType==PVOP)?vop_mode_ptr->RoundingControl:0;
    SliceInfo.IntraDCThr=vop_mode_ptr->IntraDcSwitchThr;


	
	if(vop_mode_ptr->VopPredType != NVOP)
	{
		//sorenson H263 and itu h.263 dont support B frame.
		if((BVOP == vop_mode_ptr->VopPredType) &&  (VSP_MPEG4 != vop_mode_ptr->video_std) )
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
		PRINTF ("frame not coded!\n");
#ifdef _VSP_LINUX_
	{
		uint32 BitConsumed,ByteConsumed;
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
		BitConsumed = OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "Read out BitConsumed. ") ;
		ByteConsumed = (BitConsumed + 7) >>3;
		
		if(ByteConsumed > dec_input_ptr->dataLen)
			dec_input_ptr->dataLen = 0;
		else
			dec_input_ptr->dataLen = dec_input_ptr->dataLen - ByteConsumed;

		dec_output_ptr->VopPredType = NVOP;
	}
#endif			
		return MMDEC_OK;
	}

////////////////////////////before is display ////////////////////////////////////
	
	Mp4Dec_InitVop(vop_mode_ptr, dec_input_ptr);

	{		
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG0_OFF, ((0x0)<<31)|((SliceInfo.VopQuant&0x3f)<<25)|((0&0x1ff)<<16)|((SliceInfo.Max_MBX*SliceInfo.Max_MBy)<<3)|SliceInfo.VOPCodingType&0x7,"VSP_CFG0");
       		 OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, (SliceInfo.NumMbsInGob&0x1ff)<<20|(SliceInfo.NumMbLineInGob&0x7)<<29,"VSP_CFG1");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF,(BVOP == vop_mode_ptr->VopPredType?0:1)<<31|(1-SliceInfo.VopRoundingType)<<30|0,"VSP_CFG2");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,(SliceInfo.ShortHeader|SliceInfo.DataPartition<<1|((SliceInfo.IsRvlc&(SliceInfo.VOPCodingType!=BVOP))<<2)|
			SliceInfo.IntraDCThr<<3|SliceInfo.VOPFcodeFwd<<6|SliceInfo.VOPFcodeBck<<9|0<<12|SliceInfo.QuantType<<13),"VSP_CFG3");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF,((vop_mode_ptr->time_pp&0xffff)<<16|(vop_mode_ptr->time_bp&0xffff)),"VSP_CFG4");	
		//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,vop_mode_ptr->time_pp&0xffff,"VSP_CFG3");
		if(vop_mode_ptr->time_pp==0)
		{
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,0,"VSP_CFG5");
		}
		else
		{
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,((1<<29)/vop_mode_ptr->time_pp),"VSP_CFG5");
		}

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,0x0,"VSP_INT_MASK");//enable int
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 1,"RAM_ACC_SEL");//change ram access to vsp hw
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_START_OFF,0xa|g_is_need_init_vsp_hufftab,"VSP_START");//start vsp   vld/vld_table//load_vld_table_en
	//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+MCU_SLEEP_OFF,0x1,"MCU_SLEEP");//MCU_SLEEP
	#if SIM_IN_WIN
		FPRINTF(g_fp_global_tv,"//***********************************frame num=%d slice id=%d\n",g_nFrame_dec,0);
		OR1200_Vaild=0;
	#endif
		g_is_need_init_vsp_hufftab = TRUE;
		g_is_need_init_vsp_dcttab= FALSE;
	}	


#if SIM_IN_WIN	
		if(IVOP == vop_mode_ptr->VopPredType)
		{
			PRINTF ("\t I VOP\t%d\n", dec_input_ptr->dataLen); 
	
			if(!vop_mode_ptr->bDataPartitioning)
			{
				ret = Mp4Dec_DecIVOP(vop_mode_ptr,PPAParaBuf,&SliceInfo); 
			}
 //#ifdef _MP4CODEC_DATA_PARTITION_	
			else
			{
				ret = Mp4Dec_DecIVOPErrResDataPartitioning(vop_mode_ptr,PPAParaBuf,&SliceInfo);
			}
		
			if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
			{
				
				Mp4Dec_Reset(vop_mode_ptr);
				ret =Mp4Dec_VspFrameInit(vop_mode_ptr);
				VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
				Mp4Dec_EC_IVOP(vop_mode_ptr);
			}
//#endif //_MP4CODEC_DATA_PARTITION_


	
	}else if(PVOP == vop_mode_ptr->VopPredType)
	{

		PRINTF ("\t P VOP\t%d\n", dec_input_ptr->dataLen); 

		if (vop_mode_ptr->pBckRefFrame->pDecFrame == PNULL)
		{
			if( (PNULL != vop_mode_ptr->pCurDispFrame) && 
				(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame) && 
				(PNULL != vop_mode_ptr->pCurDispFrame->pDecFrame->imgY)
			  )
			{
				MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
			}
			return MMDEC_ERROR;
		}

		VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");

		if(!vop_mode_ptr->bDataPartitioning)
		{
			ret = Mp4Dec_DecPVOP(vop_mode_ptr,PPAParaBuf,&SliceInfo) ;
		}
//	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			ret = Mp4Dec_DecPVOPErrResDataPartitioning(vop_mode_ptr,PPAParaBuf,&SliceInfo);
		}
//	#endif //_MP4CODEC_DATA_PARTITION_	

		if (dec_input_ptr->err_pkt_num || vop_mode_ptr->err_MB_num)
		{
			
			Mp4Dec_Reset(vop_mode_ptr);
			ret = Mp4Dec_VspFrameInit(vop_mode_ptr);
			VSP_WRITE_REG(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
			Mp4Dec_EC_PVOP(vop_mode_ptr);
		}
			

	}else if(SVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("\t S VOP, Don't SUPPORTED!\n"); 
//		Mp4Dec_DecPVOP(vop_mode_ptr);  //removed by Xiaowei.Luo, because SC6800H don't support GMC
	}else if(NVOP == vop_mode_ptr->VopPredType)
	{
		PRINTF ("frame not coded!\n");
        MPEG4_DecReleaseDispBfr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY);
		return MMDEC_OK;
	}else
	{
		PRINTF ("\t B VOP\t%d\n", dec_input_ptr->dataLen);
		vop_mode_ptr->RoundingControl = 0; //Notes: roundingctrol is 0 in B-VOP.
		vop_mode_ptr->err_MB_num = 0;

		Mp4Dec_DecBVOP(vop_mode_ptr,PPAParaBuf, &SliceInfo);
	}

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
/////////////////////////end
#ifdef MD5_CHK
	{
	uint32 yDataLen = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
	uint32 uvDataLen = yDataLen/2;
	Get_MD5_Code(vop_mode_ptr->pCurDispFrame->pDecFrame->imgY,  
				yDataLen, dec_output_ptr->ycode);
	Get_MD5_Code(vop_mode_ptr->pCurDispFrame->pDecFrame->imgU,
				uvDataLen, dec_output_ptr->uvcode);

#if defined(_FPGA_AUTO_VERIFICATION_) 
	waiting_n_ms(60);
	while(!read_pc_finished_dec_one_frame_msg())
	{
		;
	}
		

	send_one_frm_data(dec_output_ptr->ycode, dec_output_ptr->uvcode, 0
					16,8);

#endif //_FPGA_AUTO_VERIFICATION_	
	}
#endif
#else
			mpeg4_decode_vop(vop_mode_ptr);//wait hw return
#endif	



#if SIM_IN_WIN
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


		
		{
			int32 i,j;

			uint32 *img=(uint32*)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgY);

			FPRINTF (g_fp_rec_frm_tv,"//frame_no=%d\n",g_nFrame_dec);
			for(i=0;i<vop_mode_ptr->FrameHeight;i++)
			{
				for(j=0;j<(vop_mode_ptr->FrameWidth/8);j++)
				{
					FPRINTF (g_fp_rec_frm_tv, "%08x%08x\n",*(img+2*j+1+i*vop_mode_ptr->FrameWidth/4),*(img+2*j+i*vop_mode_ptr->FrameWidth/4));
				}
			}
			
			img=(uint32*)(vop_mode_ptr->pCurRecFrame->pDecFrame->imgU);
			for(i=0;i<(vop_mode_ptr->FrameHeight/2);i++)
			{
				for(j=0;j<(vop_mode_ptr->FrameWidth/8);j++)
				{
					FPRINTF (g_fp_rec_frm_tv, "%08x%08x\n",*(img+2*j+1+i*vop_mode_ptr->FrameWidth/4),*(img+2*j+i*vop_mode_ptr->FrameWidth/4));
				}
			}
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

		//		pic->pDecFrame->bDisp = TRUE; 
			}else
			{
				dec_output_ptr->frame_width = vop_mode_ptr->FrameWidth;
				dec_output_ptr->frame_height = vop_mode_ptr->FrameHeight;
				dec_output_ptr->frameEffective = 0;
			}
		}

		dec_output_ptr->pOutFrameY = vop_mode_ptr->pCurDispFrame->pDecFrame->imgY;
		dec_output_ptr->pOutFrameU = vop_mode_ptr->pCurDispFrame->pDecFrame->imgU;
		dec_output_ptr->pOutFrameV = vop_mode_ptr->pCurDispFrame->pDecFrame->imgV;
//ONLY FOR AUTO FPGA VERIFICATION, END		
           free(PPAParaBuf);	
           Mp4Dec_exit_picture(vop_mode_ptr);
	}

#else
	Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);	
	
	
	Mp4Dec_exit_picture(vop_mode_ptr);

#endif
	vop_mode_ptr->pre_vop_type = vop_mode_ptr->VopPredType;
	g_nFrame_dec++;
	
        VSP_RELEASE_Dev();

	// Return output.		
	return ret;
}



MMDecRet MP4DecRelease(MP4Handle *mp4Handle)
{
#ifndef _FPGA_TEST_
	VSP_CLOSE_Dev();
#endif
	return MMDEC_OK;
}


static const uint32 mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};


int16 ShowBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,          /* nr of bits to read */
    uint32 *pulOutData      /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos;

    uint i;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3; /* Byte Aligned Position */
        bitPos = dataBitPos & 7; /* update bit position */
        if (dataBytePos > pStream->numBytes - 4)
        {
            pStream->bitBuf = 0;
            for (i = 0; i < pStream->numBytes - dataBytePos; i++)
            {
                pStream->bitBuf |= pStream->data[dataBytePos+i];
                pStream->bitBuf <<= 8;
            }
            pStream->bitBuf <<= 8 * (3 - i);
        }
        else
        {
            bits = &pStream->data[dataBytePos];
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        pStream->bitPos = bitPos;
    }

    bitPos += ucNBits;

    *pulOutData = (pStream->bitBuf >> (32 - bitPos)) & mask[(uint16)ucNBits];


    return 0;
}

int16 FlushBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits                      /* number of bits to flush */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos, byteLeft;


    if ((dataBitPos + ucNBits) > (uint32)(pStream->numBytes << 3))
        return (-2); // Buffer over run

    dataBitPos += ucNBits;
    bitPos     += ucNBits;

    if (bitPos > 32)
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        byteLeft = pStream->numBytes - dataBytePos; // Byte Lefts
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        if (byteLeft > 3)
        {
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        else
        {
            uint32 lShift = 24;
            uint32 tmpBuff = 0;
            while (byteLeft)
            {
                tmpBuff |= (*bits << lShift);
                bits++;
                byteLeft--;
                lShift -= 8;
            }
            pStream->bitBuf = tmpBuff;
        }
    }

    pStream->dataBitPos = dataBitPos;
    pStream->bitPos     = bitPos;

    return 0;
}

int16 ReadBits(
    mp4StreamType *pStream,           /* Input Stream */
    uint8 ucNBits,                     /* nr of bits to read */
    uint32 *pulOutData                 /* output target */
)
{
    uint8 *bits;
    uint32 dataBitPos = pStream->dataBitPos;
    uint32 bitPos = pStream->bitPos;
    uint32 dataBytePos, byteLeft;

    if ((dataBitPos + ucNBits) > (pStream->numBytes << 3))
    {
        *pulOutData = 0;
        return (-2); // Buffer over run
    }

    //  dataBitPos += ucNBits;

    if (ucNBits > (32 - bitPos))    /* not enough bits */
    {
        dataBytePos = dataBitPos >> 3;    /* Byte Aligned Position */
        byteLeft = pStream->numBytes - dataBytePos; // Byte Lefts
        bitPos = dataBitPos & 7; /* update bit position */
        bits = &pStream->data[dataBytePos];
        if (byteLeft > 3)
        {
            pStream->bitBuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
        }
        else
        {
            uint32 lShift = 24;
            uint32 tmpBuff = 0;
            while (byteLeft)
            {
                tmpBuff |= (*bits << lShift);
                bits++;
                byteLeft--;
                lShift -= 8;
            }
            pStream->bitBuf = tmpBuff;
        }
    }

    pStream->dataBitPos += ucNBits;
    pStream->bitPos      = (unsigned char)(bitPos + ucNBits);

    *pulOutData = (pStream->bitBuf >> (32 - pStream->bitPos)) & mask[(uint16)ucNBits];

    return 0;
}


int16 MP4DecDecodeShortHeader(mp4StreamType *psBits,
                         int32 *width,
                         int32 *height,
                         int32 *display_width,
                         int32 *display_height)
{
    uint32 codeword;
    int32   extended_PTYPE = 0;
    int32 UFEP = 0;
    int32 custom_PFMT = 0;

ALOGI("%s, %d", __FUNCTION__, __LINE__);

    ShowBits(psBits, 22, &codeword);

    if (codeword !=  0x20)
    {
        return MP4_INVALID_VOL_PARAM;
    }
    FlushBits(psBits, 22);
    ReadBits(psBits, 8, &codeword);

    ReadBits(psBits, 1, &codeword);
    if (codeword == 0) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    ReadBits(psBits, 1, &codeword);
    if (codeword == 1) return MP4_INVALID_VOL_PARAM;

    /* source format */
    ReadBits(psBits, 3, &codeword);
    switch (codeword)
    {
        case 1:
            *width = 128;
            *height = 96;
            break;

        case 2:
            *width = 176;
            *height = 144;
            break;

        case 3:
            *width = 352;
            *height = 288;
            break;

        case 4:
            *width = 704;
            *height = 576;
            break;

        case 5:
            *width = 1408;
            *height = 1152;
            break;

        case 7:
            extended_PTYPE = 1;
            break;
        default:
            /* Msg("H.263 source format not legal\n"); */
            return MP4_INVALID_VOL_PARAM;
    }

    if (extended_PTYPE == 0)
    {
        *display_width = *width;
        *display_height = *height;
        return 0;
    }
    /* source format */
    ReadBits(psBits, 3, &codeword);
    UFEP = codeword;
    if (UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        switch (codeword)
        {
            case 1:
                *width = 128;
                *height = 96;
                break;

            case 2:
                *width = 176;
                *height = 144;
                break;

            case 3:
                *width = 352;
                *height = 288;
                break;

            case 4:
                *width = 704;
                *height = 576;
                break;

            case 5:
                *width = 1408;
                *height = 1152;
                break;

            case 6:
                custom_PFMT = 1;
                break;
            default:
                /* Msg("H.263 source format not legal\n"); */
                return MP4_INVALID_VOL_PARAM;
        }
        if (custom_PFMT == 0)
        {
            *display_width = *width;
            *display_height = *height;
            return 0;
        }
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 3, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;             /* RPS, ISD, AIV */
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 4, &codeword);
        if (codeword != 8) return MP4_INVALID_VOL_PARAM;
    }
    if (UFEP == 0 || UFEP == 1)
    {
        ReadBits(psBits, 3, &codeword);
        if (codeword > 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        if (codeword) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 1, &codeword);
        ReadBits(psBits, 3, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
    }
    else
    {
        return MP4_INVALID_VOL_PARAM;
    }
    ReadBits(psBits, 1, &codeword);
    if (codeword) return MP4_INVALID_VOL_PARAM; /* CPM */
    if (custom_PFMT == 1 && UFEP == 1)
    {
	uint32 CP_PAR_code;

        ReadBits(psBits, 4, &CP_PAR_code);
        if (CP_PAR_code == 0) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 9, &codeword);
        *display_width = (codeword + 1) << 2;
        *width = (*display_width + 15) & -16;
        ReadBits(psBits, 1, &codeword);
        if (codeword != 1) return MP4_INVALID_VOL_PARAM;
        ReadBits(psBits, 9, &codeword);
        if (codeword == 0) return MP4_INVALID_VOL_PARAM;
        *display_height = codeword << 2;
        *height = (*display_height + 15) & -16;

	if (CP_PAR_code == 0xf)
        {
            ReadBits(psBits, 8, &codeword);
            ReadBits(psBits, 8, &codeword);
        }
    }

    return 0;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
