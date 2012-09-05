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
#include "sc8800g_video_header.h"
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

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy>>8;
	g_rec_buf.imgUAddr =  (uint32)( pFrameY_phy+size_y)>>8;
	g_rec_buf.imgVAddr =  (uint32)(pFrameY_phy+size_y+size_c)>>8;

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

uint32 g_stream_buffer_v_addr = 0;
uint32 g_stream_buffer_p_addr = 0;
uint32 H264Dec_GetStreamPhyAddr(uint32 vaddr)
{
	return vaddr - g_stream_buffer_v_addr + g_stream_buffer_p_addr;
}
#endif

MMDecRet H264DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
	MMDecRet ret = MMDEC_ERROR;
	if((NULL == buffer_ptr)||(NULL == pVideoFormat))
	{
		return MMDEC_PARAM_ERROR;
	}

	H264Dec_InitInterMem (buffer_ptr);

	g_is_avc1_es = FALSE;
	
	g_ready_to_decode_slice = FALSE;

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
#ifdef _VSP_LINUX_
		g_stream_buffer_v_addr = (uint32)pVideoFormat->p_extra;
		g_stream_buffer_p_addr = (uint32)pVideoFormat->p_extra_phy;
#endif
		H264Dec_VSPInit ();

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
			
			//
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
			
			//
			H264Dec_Read_SPS_PPS_SliceHeader(bfr_ptr, pictureParameterSetLength);
			
			bfr_ptr += pictureParameterSetLength;
		}

		g_is_avc1_es = TRUE;
	}

	if (g_image_ptr->error_flag)
	{
        g_image_ptr->return_pos |= (1<<17);
        H264Dec_get_HW_status(g_image_ptr);     
		return MMDEC_ERROR;
	}
	return MMDEC_OK;
}

PUBLIC MMDecRet H264DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
	MMDecRet ret = MMDEC_ERROR;
	DEC_SLICE_T *curr_slice_ptr = g_curr_slice_ptr;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	int32 slice_unit_len = 0;
	uint8 *pInStream = dec_input_ptr->pStream;
	int32 last_slice = 0;
	int32 slice_len_sum = 0;

	if((NULL == dec_input_ptr)||(NULL == dec_output_ptr))
	{
		return MMDEC_PARAM_ERROR;
	}
	
	curr_slice_ptr->next_header = -1;
	img_ptr->error_flag = FALSE;
    img_ptr->return_pos = 0;
    img_ptr->return_pos2 = 0;

#ifdef _VSP_LINUX_
	g_stream_buffer_v_addr = (uint32)dec_input_ptr->pStream;
	g_stream_buffer_p_addr = (uint32)dec_input_ptr->pStream_phy;
#endif
	
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
			last_slice = get_unit (pInStream, dec_input_ptr->dataLen, &slice_unit_len);
			pInStream += slice_unit_len;
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
			
			pInStream += (slice_unit_len /*- g_lengthSizeMinusOne*/);
			
			slice_len_sum += (slice_unit_len+g_lengthSizeMinusOne);
			
			if (slice_len_sum >= (int)dec_input_ptr->dataLen)
			{
				last_slice = 1;
			}
		}

		if (slice_unit_len < 0)
		{
			img_ptr->error_flag = TRUE;
			return MMDEC_STREAM_ERROR;
		}

		if ((img_ptr->curr_mb_nr == 0) || (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)||g_searching_IDR_pic)//jgdu for streaming seek
		{
			H264Dec_VSPInit ();
		}

		ret = H264Dec_Read_SPS_PPS_SliceHeader (g_nalu_ptr->buf, g_nalu_ptr->len);
		
		if (img_ptr->error_flag)
		{
	        img_ptr->return_pos |= (1<<18);
            H264Dec_get_HW_status(img_ptr);     
			return MMDEC_ERROR;
		}
	
		if (g_ready_to_decode_slice)
		{
			if (g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
			{
				img_ptr->error_flag |= ER_PICTURE_NULL_ID;
                img_ptr->return_pos |= (1<<19);
                H264Dec_get_HW_status(img_ptr);     
				return MMDEC_ERROR;
			}

//			if (img_ptr->is_new_pic)
			{
				configure_huff_tab (g_huff_tab_token, 69);
			}

			ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);
		}
	}

	//need IVOP but not found IDR,then return seek ivop
	if(dec_input_ptr->expected_IVOP && g_searching_IDR_pic)
	{
	    g_old_slice_ptr->frame_num = -1;
		img_ptr->curr_mb_nr = 0;
	    H264Dec_get_HW_status(img_ptr);
		return MMDEC_FRAME_SEEK_IVOP;
	}

	if (img_ptr->error_flag)
	{
        g_old_slice_ptr->frame_num = -1;
		img_ptr->curr_mb_nr = 0;
        img_ptr->return_pos |= (1<<20);
        H264Dec_get_HW_status(img_ptr);     
		return MMDEC_ERROR;
	}else
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
#if _CMODEL_
	VSP_Delete_CModel();
#endif

	H264Dec_FreeMem();

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


