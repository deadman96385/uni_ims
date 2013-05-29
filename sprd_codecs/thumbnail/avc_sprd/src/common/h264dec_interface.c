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

//DEC_STORABLE_PICTURE_T g_rec_buf;
PUBLIC void H264Dec_SetCurRecPic(VideoDecControls *decCtrl, uint8	*pFrameY, int32 picId)
{
        H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	  
	img_ptr->g_rec_buf.imgY =  pFrameY;

	img_ptr->g_rec_buf.pBufferHeader = NULL;//pBufferHeader;
	img_ptr->g_rec_buf.mPicId = picId;	
}

//FunctionType_Bind_CB avc_bindCb = NULL;
//FunctionType_UnBind_CB avc_unbindCb = NULL;
//void *avc_user_data = NULL;
void H264Dec_RegBufferCB(VideoDecControls *decCtrl, FunctionType_Bind_CB bindCb,FunctionType_UnBind_CB unbindCb,void *userdata)
{
        H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
    
	img_ptr->avc_bindCb = bindCb;
	img_ptr->avc_unbindCb = unbindCb;
	img_ptr->avc_user_data = userdata;
}
extern void H264Dec_flush_dpb (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr);

void H264Dec_ReleaseRefBuffers(VideoDecControls *decCtrl)
{
        H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;
	int32 i;

//	SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, E\n");
	
	if(dpb_ptr)
	{
		if (dpb_ptr->delayed_pic_ptr)
		{
			if (dpb_ptr->delayed_pic_ptr->pBufferHeader)
			{
				(*img_ptr->avc_unbindCb)(img_ptr->avc_user_data, dpb_ptr->delayed_pic_ptr->pBufferHeader);
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
							(*img_ptr->avc_unbindCb)(img_ptr->avc_user_data,dpb_ptr->fs[j]->frame->pBufferHeader);
							dpb_ptr->fs[j]->frame->pBufferHeader = NULL;
						}
#endif
					}
				}
			}

			dpb_ptr->delayed_pic[i] = NULL;
		}
		H264Dec_flush_dpb(img_ptr, dpb_ptr);
	}

	for (i = 0; i < 16; i++)
	{
		if (dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame && dpb_ptr->fs[i]->frame->pBufferHeader)
		{
			(*img_ptr->avc_unbindCb)(img_ptr->avc_user_data, dpb_ptr->fs[i]->frame->pBufferHeader);
			dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
		//	dpb_ptr->fs[i]->frame->need_unbind = 0;

//			SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, unbind\n");			
		}
	}

	img_ptr->g_searching_IDR_pic = TRUE;
	if(img_ptr->g_old_slice_ptr)
	        img_ptr->g_old_slice_ptr->frame_num = -1;	

	

#ifdef _VSP_DEC_
//        SCI_TRACE_LOW("---H264Dec_ReleaseRefBuffers, 1,%d",img_ptr->is_previous_cmd_done);
        if(img_ptr->VSP_used &&    !img_ptr->is_previous_cmd_done)
        {
            VSP_START_CQM();
            img_ptr->is_previous_cmd_done = TRUE;
        }
//        SCI_TRACE_LOW("---H264Dec_ReleaseRefBuffers, 2,%d",img_ptr->is_previous_cmd_done);
#endif
//	SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, X\n");
}

MMDecRet H264Dec_GetLastDspFrm(VideoDecControls *decCtrl, uint8 **pOutput, int32 *picId)
{
	int32 i;
        H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;

	//for multi-slice case, we push the current decoding picture into dpb->delayed_pic queue whether it has been finished or not.
	if (img_ptr->g_dec_picture_ptr != NULL)
	{
		dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = img_ptr->g_dec_picture_ptr;
		img_ptr->g_dec_picture_ptr = NULL;
	}	

	//pop one picture from delayed picture queue.
	if (dpb_ptr->delayed_pic_num)
	{
		*pOutput = dpb_ptr->delayed_pic[0]->imgY;
                *picId = dpb_ptr->delayed_pic[0]->mPicId;
		
		for(i =0; i < dpb_ptr->delayed_pic_num; i++)
		{
			dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
		}
		dpb_ptr->delayed_pic_num--;

		return MMDEC_OK;
	}else
	{
		*pOutput = NULL;
		return MMDEC_ERROR;
	}
    
}

//FunctionType_SPS VSP_spsCb = NULL;
void H264Dec_RegSPSCB(VideoDecControls *decCtrl, FunctionType_SPS spsCb,void *userdata)
{
        H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;

//LOGI("%s, %d", __FUNCTION__, __LINE__);

	img_ptr->VSP_spsCb = spsCb;
        img_ptr->avc_user_data = userdata;
}
#endif

MMDecRet H264DecGetNALType(VideoDecControls *decCtrl, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc)
{
    int forbidden_zero_bit;

//    SCI_TRACE_LOW("%s, %d, data: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x", __FUNCTION__, __LINE__, 
//		bitstream[0],bitstream[1],bitstream[2],bitstream[3],bitstream[4],bitstream[5],bitstream[6],bitstream[7],bitstream[8]);

    if (size > 0)
    {
        forbidden_zero_bit = bitstream[0] >> 7;
        if (forbidden_zero_bit != 0)
            return MMDEC_ERROR;
        *nal_ref_idc = (bitstream[0] & 0x60) >> 5;
        *nal_type = bitstream[0] & 0x1F;
        return MMDEC_OK;
    }

    return MMDEC_ERROR;
}

PUBLIC void H264GetBufferDimensions(VideoDecControls *decCtrl, int32 *aligned_width, int32 *aligned_height) 
{
    	H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	DEC_SPS_T *sps_ptr = &(img_ptr->g_sps_array_ptr[0]);
	int32 crop_left, crop_right, crop_top, crop_bottom;

//	SCI_TRACE_LOW("%s, %d, sps_ptr: %0x", __FUNCTION__, __LINE__, sps_ptr);
	
	int32 width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
        int32 height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;

	if (sps_ptr->frame_cropping_flag)
	{
		crop_left = 2 * sps_ptr->frame_crop_left_offset;
		crop_right =  width - (2 * sps_ptr->frame_crop_right_offset + 1);

		if (sps_ptr->frame_mbs_only_flag)
		{
			crop_top = 2 * sps_ptr->frame_crop_top_offset;
                        crop_bottom =   height -  (2 * sps_ptr->frame_crop_bottom_offset + 1);
		}
		else
		{
                        crop_top = 4 * sps_ptr->frame_crop_top_offset;
                        crop_bottom =  height - (4 * sps_ptr->frame_crop_bottom_offset + 1);
		}
	} else {
		crop_bottom = height - 1;
		crop_right = width - 1;
		crop_top = crop_left = 0;
	}

	*aligned_width = (crop_right - crop_left + 1 + 15) & ~15;
	*aligned_height = (crop_bottom - crop_top + 1 + 15) & ~15;
}

void H264DecCroppingParams(H264DecContext *img_ptr, uint32 *croppingFlag,
    uint32 *leftOffset, uint32 *width, uint32 *topOffset, uint32 *height)
{
	DEC_SPS_T *sps_ptr = &(img_ptr->g_sps_array_ptr[0]);
    	int32 aligned_width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
        int32 aligned_height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;


//    if (pStorage->activeSps && pStorage->activeSps->frameCroppingFlag)
    if (sps_ptr->frame_cropping_flag)
    {
        *croppingFlag = 1;
        *leftOffset = 2 * sps_ptr->frame_crop_left_offset;
        *width = aligned_width -
                 2 * (sps_ptr->frame_crop_left_offset +
                      sps_ptr->frame_crop_right_offset);
        *topOffset = 2 * sps_ptr->frame_crop_top_offset;
        *height = aligned_height -
                  2 * (sps_ptr->frame_crop_top_offset +
                       sps_ptr->frame_crop_bottom_offset);
    }
    else
    {
        *croppingFlag = 0;
        *leftOffset = 0;
        *width = 0;
        *topOffset = 0;
        *height = 0;
    }

}

MMDecRet H264DecGetInfo(VideoDecControls *decCtrl, H264SwDecInfo *pDecInfo)
{
    	H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	DEC_SPS_T *sps_ptr = &(img_ptr->g_sps_array_ptr[0]);

//    storage_t *pStorage;

//    SCI_TRACE_LOW("H264DecGetInfo#");

    if (/*decInst == NULL ||*/ pDecInfo == NULL)
    {
        SCI_TRACE_LOW("H264SwDecGetInfo# ERROR: decInst or pDecInfo is NULL");
        return(MMDEC_PARAM_ERROR);
    }


//    pStorage = &(((decContainer_t *)decInst)->storage);

//    LOGI("%s, %d, g_active_sps_ptr: %0x, g_active_pps_ptr: %0x", __FUNCTION__, __LINE__, g_active_sps_ptr, g_active_pps_ptr);

//    if (pStorage->activeSps == NULL || pStorage->activePps == NULL)
    if (sps_ptr == NULL /*|| g_active_pps_ptr == NULL*/)
    {
        SCI_TRACE_LOW("H264SwDecGetInfo# ERROR: Headers not decoded yet");
        return(MMDEC_ERROR);
    }

#ifdef H264DEC_TRACE
    sprintf(((decContainer_t*)decInst)->str,
        "H264SwDecGetInfo# decInst %p  pDecInfo %p", decInst, (void*)pDecInfo);
    SCI_TRACE_LOW(((decContainer_t*)decInst)->str);
#endif

    /* h264bsdPicWidth and -Height return dimensions in macroblock units,
     * picWidth and -Height in pixels */
    pDecInfo->picWidth        = /*h264bsdPicWidth(pStorage)*/(sps_ptr->pic_width_in_mbs_minus1+1) << 4;
    pDecInfo->picHeight       = /*h264bsdPicHeight(pStorage)*/ (sps_ptr->pic_height_in_map_units_minus1+1)<< 4;
//    pDecInfo->videoRange      = h264bsdVideoRange(pStorage);
//    pDecInfo->matrixCoefficients = h264bsdMatrixCoefficients(pStorage);

#if 1
    H264DecCroppingParams(img_ptr,
        &pDecInfo->croppingFlag,
        &pDecInfo->cropParams.cropLeftOffset,
        &pDecInfo->cropParams.cropOutWidth,
        &pDecInfo->cropParams.cropTopOffset,
        &pDecInfo->cropParams.cropOutHeight);

    /* sample aspect ratio */
//    h264bsdSampleAspectRatio(pStorage,
//                             &pDecInfo->parWidth,
//                             &pDecInfo->parHeight);

    //added for bug#154484 and bug#154498
    if ((pDecInfo->cropParams.cropLeftOffset + pDecInfo->cropParams.cropOutWidth 
        > pDecInfo->picWidth) ||
        (pDecInfo->cropParams.cropTopOffset + pDecInfo->cropParams.cropOutHeight 
        > pDecInfo->picHeight))
    {
        return(MMDEC_ERROR);
    }
    
#endif
    /* profile */
    pDecInfo->profile = sps_ptr->profile_idc;//h264bsdProfile(pStorage);

//    SCI_TRACE_LOW("H264DecGetInfo# OK");

    return(MMDEC_OK);

}

MMDecRet H264DecInit(VideoDecControls *decCtrl, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    	H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	MMDecRet ret = MMDEC_ERROR;
	MMCodecBuffer tmp_buffer;
	
	SCI_ASSERT(NULL != buffer_ptr);
	SCI_ASSERT(NULL != pVideoFormat);

//    LOGI("%s, %d", __FUNCTION__, __LINE__);

	memset(decCtrl, 0, sizeof(VideoDecControls)); 

    	img_ptr = (H264DecContext *) buffer_ptr->int_buffer_ptr;
	decCtrl->videoDecoderData = (void *) img_ptr;

#ifdef _VSP_DEC_
	if (VSP_OPEN_Dev() < 0)
	{
		return ret;
	}
#endif	
	
	tmp_buffer.int_buffer_ptr = buffer_ptr->int_buffer_ptr + sizeof(H264DecContext);
	tmp_buffer.int_size = buffer_ptr->int_size - sizeof(H264DecContext);

	H264Dec_InitInterMem (img_ptr, &tmp_buffer);

	img_ptr->g_is_avc1_es = FALSE;
	img_ptr->g_ready_to_decode_slice = FALSE;

	H264Dec_init_global_para (img_ptr);
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
			
			return H264DecDecode(decCtrl, &input, &output);
		}
		configurationVersion = *bfr_ptr++;
		AVCProfileIndication = *bfr_ptr++;
		profile_compatibility = *bfr_ptr++;
		AVCLevelIndication = *bfr_ptr++;
		data = *bfr_ptr++;
		img_ptr->g_lengthSizeMinusOne = (data & 0x3)+1;
		data = *bfr_ptr++;
		numOfSequenceParameterSets = data & 0x1f;
		
		for (i = 0; i < numOfSequenceParameterSets; i++)
		{
			uint16 sequenceParameterSetLength;
			uint8 tmp = *bfr_ptr++;
			data = *bfr_ptr++;
			
			sequenceParameterSetLength = (tmp<<8)|data;
			 ret = H264Dec_Read_SPS_PPS_SliceHeader(img_ptr, bfr_ptr, sequenceParameterSetLength, NULL);
			 
			 bfr_ptr += sequenceParameterSetLength;
		}
		
		numoPictureParameterSets = *bfr_ptr++;
		for (i = 0; i < numoPictureParameterSets; i++)
		{
			uint16 pictureParameterSetLength;
			
			uint8 tmp = *bfr_ptr++;
			data = *bfr_ptr++;
			
			pictureParameterSetLength = (tmp<<8)|data;
			H264Dec_Read_SPS_PPS_SliceHeader(img_ptr, bfr_ptr, pictureParameterSetLength, NULL);
			
			bfr_ptr += pictureParameterSetLength;
		}

//LOGI("%s, %d", __FUNCTION__, __LINE__);
		img_ptr->g_is_avc1_es = TRUE;
	}

#if _H264_PROTECT_ & _LEVEL_LOW_
	if (img_ptr->error_flag)
	{
		img_ptr->return_pos |= (1<<19);
		return MMDEC_ERROR;
	}
#endif

	return MMDEC_OK;
}

PUBLIC MMDecRet H264DecDecode(VideoDecControls *decCtrl, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
//	VIDEO_DATA_T *img_ptr = (VIDEO_DATA_T *) decCtrl->videoDecoderData;
	MMDecRet ret = MMDEC_ERROR;
	H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 slice_unit_len = 0;
	uint8 *pInStream = dec_input_ptr->pStream;
	int32 last_slice = 0;
	int32 slice_len_sum = 0, rbsp_len = 0;

	SCI_ASSERT(NULL != dec_input_ptr);
	SCI_ASSERT(NULL != dec_output_ptr);

        if (dec_input_ptr->dataLen <= 0)
        {
            return MMDEC_ERROR;
        }
	
	curr_slice_ptr->next_header = -1;
	img_ptr->error_flag = FALSE;
        img_ptr->return_pos = 0;
 	img_ptr->return_pos1 = 0;	
   	img_ptr->return_pos2 = 0;
	
 	if((dec_input_ptr->expected_IVOP) && (img_ptr->curr_mb_nr == 0))
	{
		img_ptr->g_searching_IDR_pic = TRUE;
	}

	dec_output_ptr->frameEffective = 0;
#ifdef _VSP_LINUX_	
	dec_output_ptr->reqNewBuf = 0;
#endif

	while (!last_slice)
	{
//	LOGI("%s, %d, g_is_avc1_es: %d", __FUNCTION__, __LINE__, g_is_avc1_es);
		if (!img_ptr->g_is_avc1_es)
		{
			//slice_unit_len = (int32)img_ptr->g_nalu_ptr;
			last_slice = get_unit (img_ptr, pInStream, dec_input_ptr->dataLen, &slice_unit_len);
			dec_input_ptr->dataLen -= slice_unit_len;
//		LOGI("%s, %d, slice_unit_len: %d, last_slice: %d", __FUNCTION__, __LINE__, slice_unit_len, last_slice);
		}else
		{
		#if 0
			if (img_ptr->g_lengthSizeMinusOne == 1)
			{
				slice_unit_len = *pInStream++;
			}else if (img_ptr->g_lengthSizeMinusOne == 2)
			{
				uint8 len1, len2;
				
				len1 =  *pInStream++;
				len2 =  *pInStream++;
				
				slice_unit_len = (len1 <<8) | (len2);
			}else if (img_ptr->g_lengthSizeMinusOne == 4)
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
						
			slice_len_sum += (slice_unit_len+img_ptr->g_lengthSizeMinusOne);
			
			if (slice_len_sum >= (int)dec_input_ptr->dataLen)
			{
				last_slice = 1;
			}
		#endif
		}
		
		ret = H264Dec_Read_SPS_PPS_SliceHeader (img_ptr, img_ptr->g_nalu_ptr->buf, img_ptr->g_nalu_ptr->len, dec_output_ptr);
		
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
		
		if (img_ptr->g_ready_to_decode_slice)
		{
		#if _H264_PROTECT_ & _LEVEL_LOW_	
			if (img_ptr->g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
			{
				img_ptr->error_flag |= ER_PICTURE_NULL_ID;
                		img_ptr->return_pos |= (1<<21);

				return MMDEC_ERROR;
			}
		#endif
		
#ifdef _VSP_DEC_
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
					data = (((data0>>shift0)<<shift0)) | ((data1>>shift1)&g_h264_msk[shift0]);
					*stm++ = data;

					for (ii = 0; ii < (rbsp_len - (4-remain_bytes)); ii += 4)
					{
						data0 = data1;
						data1 = *buf++;
						data = (data1 >> shift1) | ((data0 & g_h264_msk[shift1])<<shift0);
						*stm++ = data;
					}
				}else
				{
					memcpy(img_ptr->frame_bistrm_ptr+g_stream_offset, g_nalu_ptr->buf, (((rbsp_len+3)>>2)<<2));						
				}
					
				H264Dec_InitBitstream(/*((void *)H264Dec_ExtraMem_V2P((img_ptr->frame_bistrm_ptr+g_stream_offset), HW_CACHABLE)),*/ rbsp_len);
				g_stream_offset += rbsp_len;
			}
#endif
//            LOGI("%s, %d", __FUNCTION__, __LINE__);
			ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);
		}

		pInStream += slice_unit_len;

		if (img_ptr->g_need_back_last_word)
		{
			uint32 byte_rest;
			int32 *word_align_pIn;
			byte_rest = (uint32)pInStream;
			byte_rest = ((byte_rest)>>2)<<2;
			word_align_pIn = (int32 *)(byte_rest);

			*word_align_pIn = img_ptr->g_back_last_word;
		}
	}

	//need IVOP but not found IDR,then return seek ivop
	if(dec_input_ptr->expected_IVOP && img_ptr->g_searching_IDR_pic)
	{
		SCI_TRACE_LOW("H264DecDecode: need IVOP\n");
		return MMDEC_FRAME_SEEK_IVOP;
	}

#if _H264_PROTECT_ & _LEVEL_LOW_
	if (img_ptr->error_flag)
	{
    		img_ptr->g_old_slice_ptr->frame_num = -1;
		img_ptr->curr_mb_nr = 0;
        	img_ptr->return_pos |= (1<<22);
		return MMDEC_ERROR;
	}else
#endif	
	{
		return MMDEC_OK;
	}
}

PUBLIC MMDecRet H264_DecReleaseDispBfr(VideoDecControls *decCtrl, uint8 *pBfrAddr)
{
	return MMDEC_OK;
}

MMDecRet H264DecRelease(VideoDecControls *decCtrl)
{
	H264DecContext *img_ptr = (H264DecContext *) decCtrl->videoDecoderData;

	H264Dec_FreeMem(img_ptr);

#ifdef _VSP_DEC_
	VSP_CLOSE_Dev();
#endif
	
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
