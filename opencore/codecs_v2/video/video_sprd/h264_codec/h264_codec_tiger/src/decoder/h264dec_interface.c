/******************************************************************************
 ** File Name:    h264dec_interface.c                                         *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 ** 06/26/2012   Leon Li             Modify.                                                                                       *
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

#ifdef _VSP_LINUX_

DEC_STORABLE_PICTURE_T g_rec_buf;
PUBLIC void H264Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	int32 size_y = ((g_image_ptr->width+ 15) / MB_SIZE) * ((g_image_ptr->height+ 15) / MB_SIZE)*256;	
       int32 size_c = size_y >> 2;
	  
	g_rec_buf.imgY =  pFrameY;
	g_rec_buf.imgU =  pFrameY+size_y;
	g_rec_buf.imgV =  pFrameY+size_y+size_c;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy;//>>8;
	g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y);//>>8;
	g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c);//>>8;

	g_rec_buf.pBufferHeader = pBufferHeader;

	SCI_TRACE_LOW("SetCurRecPic pFrameY %x, pFrameY_phy %x pBufferHeader %x",pFrameY,pFrameY_phy, pBufferHeader);
}
FunctionType_BufCB VSP_bindCb = NULL;
FunctionType_BufCB VSP_unbindCb = NULL;
void *g_user_data = NULL;
void H264Dec_RegBufferCB(FunctionType_BufCB bindCb,FunctionType_BufCB unbindCb,void *userdata)
{
	VSP_bindCb = bindCb;
	VSP_unbindCb = unbindCb;
	g_user_data = userdata;
}
extern void H264Dec_flush_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr);
void H264Dec_ReleaseRefBuffers()
{
	if(g_dpb_ptr)
		H264Dec_flush_dpb(g_dpb_ptr);
	g_searching_IDR_pic = TRUE;
	if(g_old_slice_ptr)
	        g_old_slice_ptr->frame_num = -1;	
}

int H264Dec_GetLastDspFrm(void **pOutput)
{

	return FALSE;
}

FunctionType_SPS VSP_spsCb = NULL;
void H264Dec_RegSPSCB(FunctionType_SPS spsCb)
{
	VSP_spsCb = spsCb;
}
#endif

MMDecRet H264DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
	MMDecRet ret = MMDEC_ERROR;
	SCI_ASSERT(NULL != buffer_ptr);
	SCI_ASSERT(NULL != pVideoFormat);

	H264Dec_InitInterMem (buffer_ptr);

	g_is_avc1_es = FALSE;
	g_ready_to_decode_slice = FALSE;

	H264Dec_init_vld_table ();
	H264Dec_init_global_para ();
	
	if (pVideoFormat->i_extra > 0)
	{
		uint8 *bfr_ptr = (uint8*)pVideoFormat->p_extra;
		uint8 configurationVersion = 1;
		uint8 AVCProfileIndication;
		uint8 profile_compatibility;
		uint8 AVCLevelIndication;
//		uint8 reserved = 0x3f;
		//uint8 lengthSizeMinusOne;
//		uint8 resrved2 = 0x3;
		uint8 numOfSequenceParameterSets;
		uint8 numoPictureParameterSets;
		uint32 i;
		uint8 data;
		
		if (bfr_ptr[0] == 0)
		{
			MMDecInput input = {0};
			MMDecOutput output = {0};
			input.pStream = pVideoFormat->p_extra;
			input.dataLen = pVideoFormat->i_extra;
			
			return H264DecDecode(&input, &output);
		}
		configurationVersion = *bfr_ptr++;
		AVCProfileIndication = *bfr_ptr++;
		profile_compatibility = *bfr_ptr++;
		AVCLevelIndication = *bfr_ptr++;
		data = *bfr_ptr++;
		g_lengthSizeMinusOne = (data & 0x3)+1;
		data = *bfr_ptr++;
		numOfSequenceParameterSets = data & 0x1f;
		
		for (i = 0; i < numOfSequenceParameterSets; i++)
		{
			uint16 sequenceParameterSetLength;
			uint8 tmp = *bfr_ptr++;
			data = *bfr_ptr++;
			
			sequenceParameterSetLength = (tmp<<8)|data;
			 ret = H264Dec_Read_SPS_PPS_SliceHeader(bfr_ptr, sequenceParameterSetLength);
			 
			 bfr_ptr += sequenceParameterSetLength;
		}
		
		numoPictureParameterSets = *bfr_ptr++;
		for (i = 0; i < numoPictureParameterSets; i++)
		{
			uint16 pictureParameterSetLength;
			
			uint8 tmp = *bfr_ptr++;
			data = *bfr_ptr++;
			
			pictureParameterSetLength = (tmp<<8)|data;
			H264Dec_Read_SPS_PPS_SliceHeader(bfr_ptr, pictureParameterSetLength);
			
			bfr_ptr += pictureParameterSetLength;
		}

		g_is_avc1_es = TRUE;
	}

	if (g_image_ptr->error_flag)
	{
		SCI_TRACE_LOW("g_image_ptr->error_flag %s,%d\n",__FUNCTION__,__LINE__);
		return MMDEC_ERROR;
	}
//SCI_TRACE_LOW("out %s,%d\n",__FUNCTION__,__LINE__);	
	return MMDEC_OK;
}

PUBLIC MMDecRet H264DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret = MMDEC_ERROR;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 slice_unit_len = 0;
	uint8 *pInStream = dec_input_ptr->pStream;
	int32 last_slice = 0;
	int32 start_code_len = 0, slice_len_sum = 0, rbsp_len = 0;
	
//SCI_TRACE_LOW("in %s,%d\n",__FUNCTION__,__LINE__);
	SCI_ASSERT(NULL != dec_input_ptr);
	SCI_ASSERT(NULL != dec_output_ptr);

//	SCI_TRACE_LOW("H264DecDecode 0\n");
	
	curr_slice_ptr->next_header = -1;
	img_ptr->error_flag = FALSE;
	
 	if((dec_input_ptr->expected_IVOP) && (img_ptr->curr_mb_nr == 0))
	{
		g_searching_IDR_pic = TRUE;
	}

	dec_output_ptr->frameEffective = 0;
	dec_output_ptr->reqNewBuf = 0;

	while (!last_slice)
	{
		if (!g_is_avc1_es)
		{
			last_slice = get_unit (pInStream, dec_input_ptr->dataLen, &slice_unit_len, &start_code_len);
		}else
		{
			if (g_lengthSizeMinusOne == 1)
			{
				slice_unit_len = *pInStream++;
			}else if (g_lengthSizeMinusOne == 2)
			{
				uint8 len1, len2;
				
				len1 =  *pInStream++;
				len2 =  *pInStream++;
				
				slice_unit_len = (len1 <<8) | (len2);
			}else if (g_lengthSizeMinusOne == 4)
			{
				uint8 len1, len2, len3, len4;
				
				len1 =  *pInStream++;
				len2 =  *pInStream++;
				len3 =  *pInStream++;
				len4 =  *pInStream++;
				
				slice_unit_len = (len1 <<24) | (len2<<16) | (len3 << 8) | (len4);
			}else
			{
				//
			}
			
			get_unit_avc1 (pInStream, slice_unit_len);
						
			slice_len_sum += (slice_unit_len+g_lengthSizeMinusOne);
			
			if (slice_len_sum >= (int)dec_input_ptr->dataLen)
			{
				last_slice = 1;
			}
		}
		
		ret = H264Dec_Read_SPS_PPS_SliceHeader (g_nalu_ptr->buf, g_nalu_ptr->len);
		
		if (img_ptr->error_flag == TRUE)
		{
			if (img_ptr->not_supported)
			{
				SCI_TRACE_LOW("img_ptr->not_supported %s,%d\n",__FUNCTION__,__LINE__);
				return MMDEC_NOT_SUPPORTED;
			}
			SCI_TRACE_LOW("img_ptr->error_flag %s,%d\n",__FUNCTION__,__LINE__);
			return MMDEC_ERROR;
		}
		
		if (g_ready_to_decode_slice)
		{
			if (g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
			{
				img_ptr->error_flag = TRUE;
				SCI_TRACE_LOW("img_ptr->error_flag %s,%d\n",__FUNCTION__,__LINE__);
				return MMDEC_ERROR;
			}

		#if 0
			if ((img_ptr->curr_mb_nr == 0) || (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs))
		#else
			if (img_ptr->is_new_pic)
		#endif
			{
				uint32 cmd;
				
				g_stream_offset = 0;
				g_cmd_data_ptr = g_cmd_data_base;
				g_cmd_info_ptr = g_cmd_info_base;
   
				/*init vsp command*/
				cmd = (1<<19)|((img_ptr->is_cabac)<<18) |(1<<17) |(0<<16) |(0<<15) | (0<<14) |(1<<12) | (H264<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
				VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0");
				VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+VSP_TIME_OUT_VAL, TIME_OUT_CLK, "GLB_TIMEOUT_VALUE: timeout value");
				
				/*frame size, YUV format, max_X, max_Y*/
				cmd = (JPEG_FW_YUV420 << 24) | (img_ptr->frame_height_in_mbs << 12) | img_ptr->frame_width_in_mbs;
				VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");			
				VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (3<<24) | (GLB_CFG1_WOFF<<16) | (VSP_TIME_OUT_VAL_WOFF<<8) | (GLB_CFG0_WOFF));
				
				//config dbk fmo mode or not				
				VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CFG_OFF, ((img_ptr->fmo_used)<<3)|6, "configure dbk free run mode and enable filter, fmo mode or not");
				VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, 1, "configure dbk ping-pang buffer0 enable");
				VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1, "configure dbk control flag");
				VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT)| (3<<24) |(DBK_CTR1_WOFF<<16) | (DBK_VDB_BUF_ST_WOFF<<8) | (DBK_CFG_WOFF));

				VSP_WRITE_REG(VSP_MEMO10_ADDR + 0, MAPaddr(g_dec_picture_ptr->imgYAddr), "configure reconstruct frame Y");
SCI_TRACE_LOW("imgY phy-addr %x,reg_value%x",g_dec_picture_ptr->imgYAddr,VSP_READ_REG(VSP_MEMO10_ADDR,""));
//SCI_TRACE_LOW("imgU phy-addr %x,reg_value%x",g_dec_picture_ptr->imgUAddr,VSP_READ_REG(VSP_MEMO10_ADDR,""));
//SCI_TRACE_LOW("imgV phy-addr %x,reg_value%x",g_dec_picture_ptr->imgVAddr,VSP_READ_REG(VSP_MEMO10_ADDR,""));
			#if _CMODEL_ //for RTL simulation
				VSP_WRITE_REG(VSP_MEMO10_ADDR + 4, ((uint32)INTRA_PREDICTION_ADDR)>>2, "Top MB bottom data block first line pixel data for IPRED prediction");
				VSP_WRITE_REG(VSP_MEMO10_ADDR + 4*3, ((uint32)VLD_CABAC_TBL_ADDR)>>2, "vld cabac buffer address");
			#else
				#ifdef _VSP_LINUX_
				VSP_WRITE_REG(VSP_MEMO10_ADDR + 4, ((uint32)H264Dec_ExtraMem_V2Phy(img_ptr->ipred_top_line_buffer))>>2, "Top MB bottom data block first line pixel data for IPRED prediction");
				VSP_WRITE_REG(VSP_MEMO10_ADDR + 4*3, ((uint32)H264Dec_ExtraMem_V2Phy(img_ptr->vld_cabac_table_ptr))>>2, "vld cabac buffer address");
				#else
				VSP_WRITE_REG(VSP_MEMO10_ADDR + 4, ((uint32)img_ptr->ipred_top_line_buffer)>>2, "Top MB bottom data block first line pixel data for IPRED prediction");
				VSP_WRITE_REG(VSP_MEMO10_ADDR + 4*3, ((uint32)img_ptr->vld_cabac_table_ptr)>>2, "vld cabac buffer address");
				#endif
			#endif
				
			}	
			
			if ((curr_slice_ptr->picture_type == P_SLICE) || (curr_slice_ptr->picture_type == B_SLICE))
			{	
				int32 i;	
				uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;

				for (i = 0; i < g_list_size[0]/*16*/; i++)
				{
					VSP_WRITE_REG_CQM(pTableAddr+ (4+i)*4, MAPaddr(g_list[0][i]->imgYAddr), "configure reference picture");			
					VSP_WRITE_CMD_INFO((VSP_RAM10 << CQM_SHIFT_BIT) | (1<<24) | (4+i));
				}
			}
		
		#if _CMODEL_|_FW_TEST_VECTOR_
			VSP_WRITE_REG_CQM(VSP_MEMO10_ADDR+ 8, MAPaddr(BIT_STREAM_DEC_0),"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
		#else //actual envioronment
			VSP_WRITE_REG_CQM(VSP_MEMO10_ADDR+ 8, ((uint32)(H264Dec_ExtraMem_V2Phy(img_ptr->frame_bistrm_ptr+g_stream_offset))>>2), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
		#endif
			VSP_WRITE_CMD_INFO((VSP_RAM10 << CQM_SHIFT_BIT) | (1<<24) | 2);
//SCI_TRACE_LOW("bitstream phy-addr %x,reg_value%x",H264Dec_ExtraMem_V2Phy(img_ptr->frame_bistrm_ptr+g_stream_offset),VSP_READ_REG(VSP_MEMO10_ADDR+8,""));
			//xweiluo@20110517, hw parsing using dec_input_ptr->pStream, which include the rawstream (NOT de-stuffed ).
			rbsp_len = (slice_unit_len - start_code_len);
			memcpy(img_ptr->frame_bistrm_ptr+g_stream_offset, pInStream+start_code_len, rbsp_len);
			H264Dec_InitBitstream(((void *)H264Dec_ExtraMem_V2Phy(img_ptr->frame_bistrm_ptr+g_stream_offset)), rbsp_len);
			g_stream_offset += rbsp_len;
			
			ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);
		}

		pInStream += slice_unit_len;
	}

	//need IVOP but not found IDR,then return seek ivop
	if(dec_input_ptr->expected_IVOP && g_searching_IDR_pic)
	{
		SCI_TRACE_LOW("H264DecDecode: need IVOP\n");
		return MMDEC_FRAME_SEEK_IVOP;
	}

	if (img_ptr->error_flag)
	{
		img_ptr->curr_mb_nr = 0;
		return MMDEC_ERROR;
	}else
	{
		return MMDEC_OK;
	}
}

PUBLIC MMDecRet H264_DecReleaseDispBfr(uint8 *pBfrAddr)
{
	return MMDEC_OK;
	//SCI_TRACE_LOW(" %s,%d\n",__FUNCTION__,__LINE__);
}

MMDecRet H264DecRelease(void)
{
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	H264Dec_FreeMem();
//SCI_TRACE_LOW(" %s,%d\n",__FUNCTION__,__LINE__);
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
