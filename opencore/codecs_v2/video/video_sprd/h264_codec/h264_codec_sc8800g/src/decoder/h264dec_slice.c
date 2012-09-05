/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
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

#if defined(RUN_IN_PLATFORM)
PUBLIC int32 get_unit (uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len)
{
	int32 len = 0;
	uint8 *ptr;
	uint8 data;
	static int32 declen = 0;
	static int32 s_bFisrtUnit = TRUE;
	int32 zero_num = 0;
	int32 startCode_len = 0;
	int32 stuffing_num = 0;
 	g_nalu_ptr->buf = pInStream;

	ptr = pInStream;

 // 	if (s_bFisrtUnit)
	{
		while ((data = *ptr++) == 0x00)
		{
			len++;
		}

 		*slice_unit_len = (len+1);
 	 	declen += (len+1);
        g_nalu_ptr->buf += (len+1);

	//	(uint8*)p_video_format->p_extra += g_stream_offset;

 		s_bFisrtUnit = FALSE;
	}

	len = 0;

	//read til next start code, and remove the third start code code emulation byte
	while (declen < frm_bs_len)
	{
		data = *ptr++;len++;
		declen++;

		if (zero_num < 2)
		{
			zero_num++;
			if(data != 0)
			{
				zero_num = 0;
			}
		}else
		{
			if ((zero_num == 2) && (data == 0x03))
			{
				zero_num = 0;
				stuffing_num++;
				continue;
			}
							
			if ((zero_num == 2) && (data == 0x1))
			{
				startCode_len = 3;
				break;
			}

			if ((zero_num == 3) && (data == 0x1))
			{
				startCode_len = 4;
				break;
			}
			if ((zero_num == 4) && (data == 0x1))
			{
				startCode_len = 5;
				break;
			}
			if ((zero_num == 5) && (data == 0x1))
			{
				startCode_len = 6;
				break;
			}
			if ((zero_num == 6) && (data == 0x1))
			{
				startCode_len = 7;
				break;
			}
			if ((zero_num == 7) && (data == 0x1))
			{
				startCode_len = 8;
				break;
			}

			if (data == 0)
			{
				zero_num++;
			}else
			{
				zero_num = 0;
			}
		}
	}

	*slice_unit_len += (len - startCode_len /*+ stuffing_num*/);
	
 	g_nalu_ptr->len = len - startCode_len - stuffing_num;
	declen -= startCode_len;

	if (declen >= frm_bs_len)
	{
		declen = 0;
		s_bFisrtUnit = TRUE;
		return 1;
	}

	return 0;
}
#endif

//#define DUMP_H264_ES

PUBLIC int32 get_unit_avc1 (uint8 *pInStream, int32 slice_unit_len)
{
	int32 len = 0;
	uint8 *ptr;
	uint8 data;
//	static int32 declen = 0;
//	static int32 s_bFisrtUnit = TRUE;
	int32 zero_num = 0;
	int32 startCode_len = 0;
	int32 stuffing_num = 0;
 	g_nalu_ptr->buf = pInStream;

	ptr = pInStream;

	//read til next start code, and remove the third start code code emulation byte
	while (len < slice_unit_len)
	{
		data = *ptr++;len++;

		if (zero_num < 2)
		{
			zero_num++;
			if(data != 0)
			{
				zero_num = 0;
			}
		}else
		{
#ifndef DUMP_H264_ES
			if ((zero_num == 2) && (data == 0x03))
			{
				zero_num = 0;
				stuffing_num++;
				continue;
			}
#endif
										
			if ((zero_num == 2) && (data == 0x1))
			{
				startCode_len = 3;
				break;
			}

			if ((zero_num == 3) && (data == 0x1))
			{
				startCode_len = 4;
				break;
			}
			if ((zero_num == 4) && (data == 0x1))
			{
				startCode_len = 5;
				break;
			}
			if ((zero_num == 5) && (data == 0x1))
			{
				startCode_len = 6;
				break;
			}
			if ((zero_num == 6) && (data == 0x1))
			{
				startCode_len = 7;
				break;
			}
			if ((zero_num == 7) && (data == 0x1))
			{
				startCode_len = 8;
				break;
			}

			if (data == 0)
			{
				zero_num++;
			}else
			{
				zero_num = 0;
			}
		}
	}

 	g_nalu_ptr->len = len -  stuffing_num;

	return 0;
}

PUBLIC void H264Dec_get_HW_status(DEC_IMAGE_PARAMS_T *img_ptr)
{
	img_ptr->ahb_state = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, "AHB: AHB status");
   	img_ptr->vsp_state = VSP_READ_REG(VSP_GLB_REG_BASE+VSP_DBG_OFF, "VSP GLOABL: VSP GLOBAL status");
    img_ptr->dbk_state = VSP_READ_REG(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, "DBK: DBK status");
  	img_ptr->mbc_state = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC: mbc status");
    img_ptr->vld_state = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, "VLD: vld status");
}

PUBLIC int32 H264Dec_Process_slice (DEC_NALU_T *nalu_ptr)
{
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 curr_header;
	int32 new_picture;

	img_ptr->idr_flag = (nalu_ptr->nal_unit_type == NALU_TYPE_IDR);
	img_ptr->nal_reference_idc = (nalu_ptr->nal_reference_idc);

	H264Dec_FirstPartOfSliceHeader (curr_slice_ptr, img_ptr);

	/*if picture parameter set id changed, FMO will change, and neighbour 4x4 block
	position infomation(available, position) will change*/
	if (g_old_pps_id != curr_slice_ptr->pic_parameter_set_id)
	{
		g_old_pps_id = curr_slice_ptr->pic_parameter_set_id;

		//use PPS and SPS
		H264Dec_use_parameter_set (curr_slice_ptr->pic_parameter_set_id);
#ifdef _VSP_LINUX_
		if(img_ptr->error_flag)
			return -1;
#endif
	}

	H264Dec_RestSliceHeader (img_ptr, curr_slice_ptr);

	if (H264Dec_FMO_init(img_ptr) == FALSE || img_ptr->error_flag)
	{
		img_ptr->error_flag = TRUE;
        img_ptr->return_pos2 |= (1<<0);
        H264Dec_get_HW_status(img_ptr);        
		return -1;
	}

	new_picture = H264Dec_is_new_picture (img_ptr);
	img_ptr->is_new_pic = new_picture;

	if (new_picture)
	{
		H264Dec_init_picture (img_ptr);
		if(img_ptr->error_flag )
			return -1;
		curr_header = SOP;
	}else
	{
		curr_header = SOS;
	}

	H264Dec_init_list (img_ptr, img_ptr->type);
	H264Dec_reorder_list ();

	h264Dec_PicLevelSendRefAddressCommmand (img_ptr);

	img_ptr->curr_mb_nr = curr_slice_ptr->start_mb_nr;

	return curr_header;
}

LOCAL void H264Dec_exit_slice (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = g_old_slice_ptr;

	old_slice_ptr->frame_num = img_ptr->frame_num;
	old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
	old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
	old_slice_ptr->idr_flag = img_ptr->idr_flag;

    if (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)
    {
        old_slice_ptr->frame_num = -1;
    }

	if (img_ptr->idr_flag)
	{
		old_slice_ptr->idr_pic_id = img_ptr->idr_pic_id;
	}

	if (g_active_sps_ptr->pic_order_cnt_type == 0)
	{
		old_slice_ptr->pic_order_cnt_lsb = img_ptr->pic_order_cnt_lsb;
		old_slice_ptr->delta_pic_order_cnt_bottom = img_ptr->delta_pic_order_cnt_bottom;
	}

	if (g_active_sps_ptr->pic_order_cnt_type == 1)
	{
		old_slice_ptr->delta_pic_order_cnt[0] = img_ptr->delta_pic_order_cnt[0];
		old_slice_ptr->delta_pic_order_cnt[1] = img_ptr->delta_pic_order_cnt[1];
	}

	return;
}

LOCAL void H264Dec_decode_one_slice_I (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = NULL;

	g_pre_mb_is_intra4 = 0;

	while (!end_of_slice)
	{
		img_ptr->mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
		img_ptr->mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;
		curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_ISlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	

		if (img_ptr->error_flag)
		{
            img_ptr->return_pos2 |= (1<<1);
            H264Dec_get_HW_status(img_ptr);            
			return;
		}
		
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (curr_mb_info_ptr, mb_cache_ptr);

		//dbk
		if (!img_ptr->fmo_used)
		{
			H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}else
		{
			VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, (img_ptr->mb_y << 16) | img_ptr->mb_x, "config dbk x and y id");
			VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");
		}

		H264Dec_mb_level_sync (img_ptr);
        if (img_ptr->error_flag)
		{
    		img_ptr->return_pos2 |= (1<<2);
            H264Dec_get_HW_status(img_ptr);         
			return;
		}
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr);
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

LOCAL void H264Dec_decode_one_slice_P (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = NULL;

	img_ptr->cod_counter = -1;

	while (!end_of_slice)
	{
		img_ptr->mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
		img_ptr->mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;
		curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_PSlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        
        if (img_ptr->error_flag)
        {
            img_ptr->return_pos2 |= (1<<3);
            H264Dec_get_HW_status(img_ptr);        
            return;
        }

		//mvp, stand here for let MCA launch ahead.
		if (!curr_mb_info_ptr->is_intra)
		{
			H264Dec_mv_prediction (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}else
		{
 	 		H264Dec_fill_mv_and_ref (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}

		//vld
		if (curr_mb_info_ptr->is_skipped)
		{
			H264Dec_set_nnz_to_zero (curr_mb_info_ptr, mb_cache_ptr);
		}else
		{
			H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		}
		
		if (img_ptr->error_flag)
		{
    		img_ptr->return_pos2 |= (1<<4);
            H264Dec_get_HW_status(img_ptr);       
			return;
		}
		
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (curr_mb_info_ptr, mb_cache_ptr);	
	
		//dbk
		if (!img_ptr->fmo_used)
		{
			H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}else
		{
			VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, (img_ptr->mb_y << 16) | img_ptr->mb_x, "config dbk x and y id");
			VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");
		}		

		H264Dec_mb_level_sync (img_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr);
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = g_curr_slice_ptr;
	int32 cmd;
	MMDecRet ret = MMDEC_ERROR;

	curr_slice_ptr->next_header = -1;	
	
	/*frame size, YUV format, max_X, max_Y*/
	cmd = (JPEG_FW_YUV420 << 24) | (img_ptr->frame_height_in_mbs << 12) | img_ptr->frame_width_in_mbs;
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");

	//config dbk fmo mode or not
	cmd = VSP_READ_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, "read dbk configure");
	cmd |= ((img_ptr->fmo_used)<<3);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, cmd, "configure dbk free run mode and enable filter");

	if (curr_slice_ptr->picture_type == I_SLICE)
	{
		H264Dec_decode_one_slice_I (img_ptr);
	}else if (curr_slice_ptr->picture_type == P_SLICE)
	{
		H264Dec_decode_one_slice_P (img_ptr);
	}else
	{
		img_ptr->error_flag = TRUE;
        img_ptr->return_pos2 = (1<<5);
        H264Dec_get_HW_status(img_ptr);        
		PRINTF ("the other picture type is not supported!\n");
	}

	if (img_ptr->error_flag)
	{
        img_ptr->return_pos2 = (1<<6);
        H264Dec_get_HW_status(img_ptr);

		g_searching_IDR_pic = 1;
		return MMDEC_ERROR;
	}

	img_ptr->slice_nr++;

	if (SOP == curr_slice_ptr->next_header) //end of picture
	{
		ret = H264Dec_Picture_Level_Sync (img_ptr);

		if (img_ptr->fmo_used)
		{
			H264Dec_VSPInit ();
			
			cmd = VSP_READ_REG (VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "read out Endian Info") & 0xf;
			cmd |= ((img_ptr->width*img_ptr->height/4) << 4);
			VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");

			VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, img_ptr->width, "configure frame width");

			cmd = (JPEG_FW_YUV420 << 24) | (img_ptr->frame_height_in_mbs << 12) | img_ptr->frame_width_in_mbs;
			VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");

  	  	 	H264Dec_deblock_one_frame (img_ptr);

			READ_REG_POLL(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, V_BIT_16, V_BIT_16, TIME_OUT_CLK, "DBK_DEBUG: polling Frame idle status");
			READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status");

			VSP_WRITE_REG(VSP_GLB_REG_BASE+VSP_TST_OFF, g_nFrame_dec_h264, "VSP_TST: configure frame_cnt to debug register for end of picture");
		}

		H264Dec_exit_picture (img_ptr);

		dec_output_ptr->frameEffective = 1;
		dec_output_ptr->frame_width = img_ptr->width;
		dec_output_ptr->frame_height = img_ptr->height;
		dec_output_ptr->pOutFrameY = g_dec_picture_ptr->imgY;
		dec_output_ptr->pOutFrameU = g_dec_picture_ptr->imgU;
		dec_output_ptr->pOutFrameV = g_dec_picture_ptr->imgV;
#ifdef _VSP_LINUX_
		dec_output_ptr->pBufferHeader = g_dec_picture_ptr->pBufferHeader;
		dec_output_ptr->reqNewBuf = 1;
#endif
			
		g_dec_picture_ptr = NULL;
		g_nFrame_dec_h264++;
	}

	img_ptr->ahb_state = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, "AHB: AHB status");
   	img_ptr->vsp_state = VSP_READ_REG(VSP_GLB_REG_BASE+VSP_DBG_OFF, "VSP GLOABL: VSP GLOBAL status");
    img_ptr->dbk_state = VSP_READ_REG(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, "DBK: DBK status");
	img_ptr->mbc_state = VSP_READ_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, "MBC: mbc status");

	return ret;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 