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

FunctionType_BufCB VSP_bindCb = NULL;
FunctionType_BufCB VSP_unbindCb = NULL;
void *g_user_data = NULL;
FunctionType_MallocCB VSP_mallocCb = NULL;



/*************************************/
/* functions needed for android platform */
/*************************************/

PUBLIC void OR_VSP_BIND(void *pHeader)
{
	 (*VSP_bindCb)(g_user_data,pHeader);
}

PUBLIC void OR_VSP_UNBIND(void *pHeader)
{
   	(*VSP_unbindCb)(g_user_data,pHeader);
}


void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle)
{
    int32 i;

    H264Dec_flush_dpb(g_dpb_layer[0]);

    SCI_TRACE_LOW("%s, %d, display_array_len: %d", __FUNCTION__, __LINE__, display_array_len);
    for(i =0;i<display_array_len; i++)
    {
		OR_VSP_UNBIND(display_array_BH[i]);
	}

    display_array_len = 0;
}

MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, uint8 **pOutput, int32 *picId)
{
	* pOutput = NULL;         
	return MMDEC_ERROR;

}




PUBLIC void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId)
{
	 int i; 
	g_rec_buf.imgY =  pFrameY;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy;

	g_rec_buf.pBufferHeader = pBufferHeader;	

	 g_rec_buf.mPicId = picId;
}

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc)
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



MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo)
{
	DEC_SPS_T *sps_ptr = &(g_sps_array_ptr[0]);
    	int32 aligned_width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
        int32 aligned_height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;

    if ( pDecInfo == NULL)
    {
  //      ALOGI("H264SwDecGetInfo# ERROR: decInst or pDecInfo is NULL");
        return(MMDEC_PARAM_ERROR);
    }

    pDecInfo->picWidth        = aligned_width;
    pDecInfo->picHeight       = aligned_height;

    if (sps_ptr->frame_cropping_flag)
    {
        pDecInfo->croppingFlag = 1;
        pDecInfo->cropParams.cropLeftOffset = 2 * sps_ptr->frame_crop_left_offset;
        pDecInfo->cropParams.cropOutWidth = aligned_width -
                 2 * (sps_ptr->frame_crop_left_offset +
                      sps_ptr->frame_crop_right_offset);
         pDecInfo->cropParams.cropTopOffset = 2 * sps_ptr->frame_crop_top_offset;
         pDecInfo->cropParams.cropOutHeight= aligned_height -
                  2 * (sps_ptr->frame_crop_top_offset +
                       sps_ptr->frame_crop_bottom_offset);
    }
    else
    {
        pDecInfo->croppingFlag = 0;
        pDecInfo->cropParams.cropLeftOffset = 0;
         pDecInfo->cropParams.cropOutWidth  = 0;
        pDecInfo->cropParams.cropTopOffset = 0;
         pDecInfo->cropParams.cropOutHeight= 0;
    }


    return(MMDEC_OK);

}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{

SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);	
	MMDecRet ret = MMDEC_ERROR;
	SCI_ASSERT(NULL != buffer_ptr);
	SCI_ASSERT(NULL != pVideoFormat);

	VSP_mallocCb = avcHandle->VSP_extMemCb;
	VSP_bindCb = avcHandle->VSP_bindCb;
	VSP_unbindCb = avcHandle->VSP_unbindCb;
	g_user_data = avcHandle->userdata;

	if (VSP_OPEN_Dev() < 0)
	{
		return ret;
	}

	// Physical memory as internal memory.
	H264Dec_InitInterMem (buffer_ptr);
	H264Dec_init_vld_table ();
	H264Dec_init_global_para ();

	//copy cavlc tbl to phy addr of g_cavlc_tbl_ptr.
	memcpy(g_cavlc_tbl_ptr, g_huff_tab_token, sizeof(uint32)*69);
	
	if (g_image_ptr->error_flag)
	{
		return MMDEC_ERROR;
	}
	return MMDEC_OK;
}





PUBLIC MMDecRet H264DecDecode_NALU(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)

{
	int i;		
	MMDecRet ret = MMDEC_ERROR;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	DEC_SLICE_T *curr_slice_ptr = g_curr_slice_ptr;
	


	
	
  
	curr_slice_ptr->next_header = -1;
	img_ptr->error_flag = FALSE;

 	if((dec_input_ptr->expected_IVOP) && (img_ptr->curr_mb_nr == 0))
	{
		g_searching_IDR_pic = TRUE;
	}	
	


	ret = H264Dec_Read_SPS_PPS_SliceHeader ();//g_nalu_ptr->buf, g_nalu_ptr->len);//weihu
	
	if (img_ptr->error_flag == TRUE)
	{
//		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xe8, 0x77777777, "FPGA test");

		return MMDEC_ERROR;
	}
	
	if (g_ready_to_decode_slice)
	{

#if _MVC_
		DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_curr_slice_ptr->p_Dpb;
#else
		DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif
		DEC_STORABLE_PICTURE_T *pframe;
	#if 0
		if ((img_ptr->curr_mb_nr == 0) || (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs))
	#else
		if (img_ptr->is_new_pic)
	#endif
		{
			//H264Dec_VSPInit ();			
			
			if (img_ptr->is_need_init_vsp_hufftab && img_ptr->is_new_pic)//分不分cabac和cavlc
			{			
				uint32 vld_table_addr;

				vld_table_addr = H264Dec_GetPhyAddr(g_cavlc_tbl_ptr);	
				//load_vld_table_en=1;
				
				OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0xc, vld_table_addr/8,"ddr vlc table start addr");
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_SIZE_SET_OFF, 0x45,"ddr VLC table size");
				//img_ptr->is_need_init_vsp_hufftab = FALSE;
			}	


		}			

		//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr
		//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0x80000000|(g_stream_offset),"BSM_cfg1 stream buffer offset");//byte align
		//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(g_slice_datalen+128),"BSM_cfg0 stream buffer size");//打开BSM load data//真实sps/pps/slice nalu size+16dw,注意word对齐
	
        //配置frame start address ram
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR, (g_dec_picture_ptr->imgYAddr)>>3,"current Y addr");//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->imgYAddr
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+4, (g_dec_picture_ptr->imgUAddr)>>3,"current UV addr");//0x480000+//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->imgUAddr
		OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+8, (g_dec_picture_ptr->direct_mb_info_Addr)>>3,"current info addr");//0x6a0000+//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->direct_mb_info
		for(i=0;i<g_image_ptr->num_ref_idx_l0_active;i++)//16
			//for(i=0;i<32;i++)
		{
//			pframe = dpb_ptr->fs[g_list0_map_addr[i]]->frame;
			pframe = g_list0[i];
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x80+i*4,(pframe->imgYAddr)>>3,"ref L0 Y addr");//g_dpb_layer[0]->fs[g_list0_map_addr[i]]->frame->imgYAddr
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x100+i*4,(pframe->imgUAddr)>>3,"ref L0 UV addr");//0x480000+
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x180+i*4,(pframe->direct_mb_info_Addr)>>3,"ref L0 info addr");//0x6a0000+
			
		}
		for(i=0;i<g_image_ptr->num_ref_idx_l1_active;i++)//16
			//for(i=0;i<32;i++)
		{
			//pframe = dpb_ptr->fs[g_list1_map_addr[i]]->frame;
			pframe = g_list1[i];
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0xc0+i*4,(pframe->imgYAddr)>>3,"ref L1 Y addr");
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x140+i*4,(pframe->imgUAddr)>>3,"ref L1 UV addr");//0x480000+
			OR1200_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x1c0+i*4,(pframe->direct_mb_info_Addr)>>3,"ref L1 info addr");//0x6a0000+
			
		}
	
		ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);

		
	}

	H264Dec_flush_left_byte ();

	//need IVOP but not found IDR,then return seek ivop
	if(dec_input_ptr->expected_IVOP && g_searching_IDR_pic)
	{
		return MMDEC_FRAME_SEEK_IVOP;
	}

	if (img_ptr->error_flag)
	{
//		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR + 0xe8, 0x7777ffff, "FPGA test");
		return MMDEC_ERROR;
	}else
	{
		return MMDEC_OK;
	}
}

int32 b_video_buffer_malloced = 0;

PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret;
    int32 i;
    uint32 bs_buffer_length, bs_start_addr;

	frame_dec_finish=0;	

    if(ARM_VSP_RST()<0)
    {
	return MMDEC_HW_ERROR;
    }
SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");	
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_H264,"VSP_MODE");
	g_image_ptr->is_need_init_vsp_hufftab = TRUE;				


   	 // Bitstream.
   	 bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
  	 bs_buffer_length=dec_input_ptr->dataLen;
  	 g_stream_offset=0;

   

	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM

			
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset|0xc0000000),"BSM_cfg1 check startcode");//byte align
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (0x80000000|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data
  	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, 0x00000004,0x00000004,"startcode found");//check bsm is idle	

	//Get start code length of first NALU.
	g_slice_datalen=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");			
	g_stream_offset+=g_slice_datalen;
	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");
	while(g_stream_offset<bs_buffer_length)
	{
		// Find the next start code and get length of NALU.
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (g_stream_offset|0xc0000000),"BSM_cfg1 check startcode");//byte align
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (0x80000000|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, 0x00000004,0x00000004,"startcode found");//check bsm is idle	
		// Get length of NALU and net bitstream length.
		g_slice_datalen=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");
		g_nalu_ptr->len=OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_DATA_LEN,"get NAL_DATA_LEN");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");
        

		// Configure BSM for decoding.
        OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0x80000000|(g_stream_offset),"BSM_cfg1 stream buffer offset");//point to the start of NALU.
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

		ret = H264DecDecode_NALU(dec_input_ptr, dec_output_ptr);
					
		g_stream_offset += g_slice_datalen;//dec_input_ptr->dataLen;




					

		if( (MMDEC_ERROR ==ret) ||frame_dec_finish)//dec_output.frameEffective
		{ 
									


	//		ALOGE("%s, %d", __FUNCTION__, __LINE__);
	//		ALOGE("pic_width : %d, pic_height :%d",dec_output_ptr->frame_width,dec_output_ptr->frame_height);				
	
			break;	//break loop.					
		}
					

	}



    if(!b_video_buffer_malloced && g_sps_ptr->pic_height_in_map_units_minus1/* && g_sps_ptr->profile_idc != 0x42*/)
    {
        // Malloc direct mb info buffers
	uint32 malloc_buffer_num;
	uint32 malloc_buffer_size;

	malloc_buffer_num = 17;
	malloc_buffer_size =  ((g_sps_ptr->pic_height_in_map_units_minus1+1) * (g_sps_ptr->pic_width_in_mbs_minus1+1))  * 80;

   	 ALOGE("%s, %d", __FUNCTION__, __LINE__);

	VSP_mallocCb (g_user_data, direct_mb_info_addr, malloc_buffer_num, malloc_buffer_size);

	b_video_buffer_malloced = 1;
    }

          VSP_RELEASE_Dev();
 
    // Return output.		
    return ret;
}




PUBLIC MMDecRet H264_DecReleaseDispBfr(AVCHandle *avcHandle, uint8 *pBfrAddr)
{
    return MMDEC_OK;
}

MMDecRet H264DecRelease(AVCHandle *avcHandle)
{
#ifndef _FPGA_TEST_
    VSP_CLOSE_Dev();
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
