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


PUBLIC void H264Dec_SetCurRecPic(uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
	 int i; 
	g_rec_buf.imgY =  pFrameY;

	g_rec_buf.imgYAddr = (uint32)pFrameY_phy;

	g_rec_buf.pBufferHeader = pBufferHeader;	

	
}

void H264DecCroppingParams(uint32 *croppingFlag,
    uint32 *leftOffset, uint32 *width, uint32 *topOffset, uint32 *height)
{
	DEC_SPS_T *sps_ptr = &(g_sps_array_ptr[0]);
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

MMDecRet H264DecInit(MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
	MMDecRet ret = MMDEC_ERROR;
	SCI_ASSERT(NULL != buffer_ptr);
	SCI_ASSERT(NULL != pVideoFormat);

	H264Dec_InitInterMem (buffer_ptr);
	H264Dec_init_vld_table ();
	H264Dec_init_global_para ();
	if (g_image_ptr->error_flag)
	{
		return MMDEC_ERROR;
	}
	return MMDEC_OK;
}



PUBLIC MMDecRet H264DecDecode(MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
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

                		vld_table_addr= (unsigned int)(&g_huff_tab_token)+or_addr_offset;//硬件绝对地址，要加上openrisc基地址

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
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End
