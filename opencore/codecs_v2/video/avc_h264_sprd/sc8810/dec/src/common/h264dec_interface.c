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

DEC_STORABLE_PICTURE_T g_rec_buf;
PUBLIC void H264Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader/*, int32 iDecH264WasSw*/)
{
//    int32 size_y, size_c;

//    if (iDecH264WasSw)
//    {
//#ifdef YUV_THREE_PLANE
//	size_y = ((g_image_ptr->width+Y_EXTEND_SIZE*2+ 15) / MB_SIZE) * ((g_image_ptr->height+Y_EXTEND_SIZE*2+ 15) / MB_SIZE)*256;	
//#else
//	size_y = ((g_image_ptr->width+ 15) / MB_SIZE) * ((g_image_ptr->height+ 15) / MB_SIZE)*256;	
//#endif
//     }else
//    {
//        size_y = ((g_image_ptr->width+ 15) / MB_SIZE) * ((g_image_ptr->height+ 15) / MB_SIZE)*256;	
//    }
 //      size_c = size_y >> 2;
	  
	g_rec_buf.imgY =  pFrameY;
//	g_rec_buf.imgU =  pFrameY+size_y;
//	g_rec_buf.imgV =  pFrameY+size_y+size_c;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy>>8;
//	g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y)>>8;
//	g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c)>>8;

	g_rec_buf.pBufferHeader = pBufferHeader;
	
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
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
	int32 i;

//	SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, E\n");
	
	if(dpb_ptr)
	{
		if (dpb_ptr->delayed_pic_ptr)
		{
			if (dpb_ptr->delayed_pic_ptr->pBufferHeader)
			{
				(*VSP_unbindCb)(g_user_data, dpb_ptr->delayed_pic_ptr->pBufferHeader);
				dpb_ptr->delayed_pic_ptr->pBufferHeader = NULL;
			}
			dpb_ptr->delayed_pic_ptr = NULL;
		}

		for (i = 0; dpb_ptr->delayed_pic[i]; i++)
		{
			int32 j;
			
			for (j = 0; j < /*dpb_ptr->used_size*/16; j++)
			{
				if (dpb_ptr->delayed_pic[j] == dpb_ptr->fs[j]->frame)
				{
					if(dpb_ptr->fs[j]->is_reference == DELAYED_PIC_REF)
					{
						dpb_ptr->fs[j]->is_reference = 0;

#ifdef _VSP_LINUX_
						if(dpb_ptr->fs[j]->frame->pBufferHeader!=NULL)
						{
//							SCI_TRACE_LOW("unbind in H264Dec_ReleaseRefBuffers\t");
							(*VSP_unbindCb)(g_user_data,dpb_ptr->fs[j]->frame->pBufferHeader);
							dpb_ptr->fs[j]->frame->pBufferHeader = NULL;
						}
#endif
					}
				}
			}

			dpb_ptr->delayed_pic[i] = NULL;
		}
		H264Dec_flush_dpb(dpb_ptr);
	}

	for (i = 0; i < 16; i++)
	{
		if (dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame && dpb_ptr->fs[i]->frame->pBufferHeader)
		{
			(*VSP_unbindCb)(g_user_data, dpb_ptr->fs[i]->frame->pBufferHeader);
			dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
		//	dpb_ptr->fs[i]->frame->need_unbind = 0;

//			SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, unbind\n");			
		}
	}

	g_searching_IDR_pic = TRUE;
	if(g_old_slice_ptr)
	        g_old_slice_ptr->frame_num = -1;	

	

#if 1
        SCI_TRACE_LOW("---H264Dec_ReleaseRefBuffers, 1,%d",g_image_ptr->is_previous_cmd_done);
        if(g_image_ptr->VSP_used &&    !g_image_ptr->is_previous_cmd_done)
        {
            VSP_START_CQM();
            g_image_ptr->is_previous_cmd_done = TRUE;
        }
        SCI_TRACE_LOW("---H264Dec_ReleaseRefBuffers, 2,%d",g_image_ptr->is_previous_cmd_done);
#endif
//	SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, X\n");
}

int H264Dec_GetLastDspFrm(void **pOutput)
{
	int32 i;
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;

	//for multi-slice case, we push the current decoding picture into dpb->delayed_pic queue whether it has been finished or not.
	if (g_dec_picture_ptr != NULL)
	{
		dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = g_dec_picture_ptr;
		g_dec_picture_ptr = NULL;
	}	

	//pop one picture from delayed picture queue.
	if (dpb_ptr->delayed_pic_num)
	{
		*pOutput = dpb_ptr->delayed_pic[0]->pBufferHeader;

        if(dpb_ptr->delayed_pic[0]->pBufferHeader !=NULL)
        {
        	(*VSP_unbindCb)(g_user_data,dpb_ptr->delayed_pic[0]->pBufferHeader);
            dpb_ptr->delayed_pic[0]->pBufferHeader = NULL;
        }
		
		for(i =0; i < dpb_ptr->delayed_pic_num; i++)
		{
			dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
		}
		dpb_ptr->delayed_pic_num--;

		return TRUE;
	}else
	{
		*pOutput = NULL;
		return FALSE;
	}
}

FunctionType_SPS VSP_spsCb = NULL;
void H264Dec_RegSPSCB(FunctionType_SPS spsCb)
{
	VSP_spsCb = spsCb;
}

FunctionType_FlushCache VSP_fluchCacheCb = NULL;
void  H264Dec_RegFlushCacheCB( FunctionType_FlushCache fluchCacheCb)
{
   	VSP_fluchCacheCb = fluchCacheCb;   
}
#endif

MMDecRet H264DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
	MMDecRet ret = MMDEC_ERROR;
	SCI_ASSERT(NULL != buffer_ptr);
	SCI_ASSERT(NULL != pVideoFormat);

	if (VSP_OPEN_Dev() < 0)
	{
		return ret;
	}
	
	H264Dec_InitInterMem (buffer_ptr);

	g_is_avc1_es = FALSE;
	g_ready_to_decode_slice = FALSE;

	H264Dec_init_global_para ();
	H264Dec_init_vld_table ();

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
			 ret = H264Dec_Read_SPS_PPS_SliceHeader(bfr_ptr, sequenceParameterSetLength,NULL);
			 
			 bfr_ptr += sequenceParameterSetLength;
		}
		
		numoPictureParameterSets = *bfr_ptr++;
		for (i = 0; i < numoPictureParameterSets; i++)
		{
			uint16 pictureParameterSetLength;
			
			uint8 tmp = *bfr_ptr++;
			data = *bfr_ptr++;
			
			pictureParameterSetLength = (tmp<<8)|data;
			H264Dec_Read_SPS_PPS_SliceHeader(bfr_ptr, pictureParameterSetLength, NULL);
			
			bfr_ptr += pictureParameterSetLength;
		}

		g_is_avc1_es = TRUE;
	}

#if _H264_PROTECT_ & _LEVEL_LOW_
	if (g_image_ptr->error_flag)
	{
		g_image_ptr->return_pos |= (1<<19);
		return MMDEC_ERROR;
	}
#endif

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

#ifdef _DEBUG_TIME_
	gettimeofday(&tpstart,NULL);
#endif

	SCI_ASSERT(NULL != dec_input_ptr);
	SCI_ASSERT(NULL != dec_output_ptr);

//	SCI_TRACE_LOW("H264DecDecode 0\n");
	
	curr_slice_ptr->next_header = -1;
	img_ptr->error_flag = FALSE;
    img_ptr->return_pos = 0;
 	img_ptr->return_pos1 = 0;	
   	img_ptr->return_pos2 = 0;
	
 	if((dec_input_ptr->expected_IVOP) && (img_ptr->curr_mb_nr == 0))
	{
		g_searching_IDR_pic = TRUE;
	}

	dec_output_ptr->frameEffective = 0;
#ifdef _VSP_LINUX_	
	dec_output_ptr->reqNewBuf = 0;
#endif

	while (!last_slice)
	{
		if (!g_is_avc1_es)
		{
			slice_unit_len = (int32)g_nalu_ptr;
			last_slice = get_unit (pInStream, dec_input_ptr->dataLen, &slice_unit_len, &start_code_len);
			dec_input_ptr->dataLen -= slice_unit_len;
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
        
		ret = H264Dec_Read_SPS_PPS_SliceHeader (g_nalu_ptr->buf, g_nalu_ptr->len, dec_output_ptr);
		
	#if _H264_PROTECT_ & _LEVEL_LOW_
		if (img_ptr->error_flag)
		{
		 	img_ptr->return_pos |= (1<<20);

			if (img_ptr->not_supported)
			{
				return MMDEC_NOT_SUPPORTED;
			}

			return MMDEC_ERROR;
		}
	#endif	
		
		if (g_ready_to_decode_slice)
		{
		#if _H264_PROTECT_ & _LEVEL_LOW_	
			if (g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
			{
				img_ptr->error_flag |= ER_PICTURE_NULL_ID;
                		img_ptr->return_pos |= (1<<21);

				return MMDEC_ERROR;
			}
		#endif
		
			if (img_ptr->VSP_used)
			{
				uint32 remain_bytes;
				
				H264Dec_init_slice_VSP(img_ptr, curr_slice_ptr);

				rbsp_len = (slice_unit_len - start_code_len);
				remain_bytes = g_stream_offset & 0x3;
				
				if (remain_bytes)
				{
					uint32 shift0 = ((4-remain_bytes)*8);
					uint32 shift1 = (remain_bytes*8);
					uint32 *stm = ((uint32 *)(img_ptr->frame_bistrm_ptr+g_stream_offset - remain_bytes));
					uint32 *buf=((uint32 *)(g_nalu_ptr->buf));
					uint32 data0, data1, data, ii;

					data0 = stm[0];
					data1 = *buf++;
					data = (((data0>>shift0)<<shift0)) | ((data1>>shift1)&g_msk[shift0]);
					*stm++ = data;

					for (ii = 0; ii < (rbsp_len - (4-remain_bytes)); ii += 4)
					{
						data0 = data1;
						data1 = *buf++;
						data = (data1 >> shift1) | ((data0 & g_msk[shift1])<<shift0);
						*stm++ = data;
					}
				}else
				{
					memcpy(img_ptr->frame_bistrm_ptr+g_stream_offset, g_nalu_ptr->buf, (((rbsp_len+3)>>2)<<2));						
				}
					
				H264Dec_InitBitstream(/*((void *)H264Dec_ExtraMem_V2P((img_ptr->frame_bistrm_ptr+g_stream_offset), HW_CACHABLE)),*/ rbsp_len);
				g_stream_offset += rbsp_len;
			}

			ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);
		}

		pInStream += slice_unit_len;

		if (g_need_back_last_word)
		{
			uint32 byte_rest;
			int32 *word_align_pIn;
			byte_rest = (uint32)pInStream;
			byte_rest = ((byte_rest)>>2)<<2;
			word_align_pIn = (int32 *)(byte_rest);

			*word_align_pIn = g_back_last_word;
		}
	}

	//need IVOP but not found IDR,then return seek ivop
	if(dec_input_ptr->expected_IVOP && g_searching_IDR_pic)
	{
		SCI_TRACE_LOW("H264DecDecode: need IVOP\n");
		return MMDEC_FRAME_SEEK_IVOP;
	}

#if _H264_PROTECT_ & _LEVEL_LOW_
	if (img_ptr->error_flag)
	{
    		g_old_slice_ptr->frame_num = -1;
		img_ptr->curr_mb_nr = 0;
        	img_ptr->return_pos |= (1<<22);
		return MMDEC_ERROR;
	}else
#endif	
	{
		return MMDEC_OK;
	}
}

PUBLIC MMDecRet H264_DecReleaseDispBfr(uint8 *pBfrAddr)
{
	return MMDEC_OK;
}

MMDecRet H264DecRelease(void)
{
	H264Dec_FreeMem();

	VSP_CLOSE_Dev();
	
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	
	return MMDEC_OK;
}

BOOLEAN H264DEC_VSP_Available(void)
{
	int dcam_cfg;

	dcam_cfg = VSP_READ_REG(VSP_DCAM_BASE+DCAM_CFG_OFF, "DCAM_CFG: read dcam configure register");

	if (((dcam_cfg >> 3) & 1) == 0) //bit3: VSP_EB, xiaowei.luo@20090907
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
