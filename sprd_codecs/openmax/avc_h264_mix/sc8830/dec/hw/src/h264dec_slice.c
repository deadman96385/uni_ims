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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

int ref_list_buf[33];
int slice_info[50];

#if 0// defined(RUN_IN_PLATFORM)
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
 	uint8 *bfr = g_nalu_ptr->buf = pInStream;

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

/*
PUBLIC int32 get_unit_avc1 (uint8 *pInStream, int32 slice_unit_len)
{
	int32 len = 0;
	uint8 *ptr;
	uint8 data;
	static int32 declen = 0;
	static int32 s_bFisrtUnit = TRUE;
	int32 zero_num = 0;
	int32 startCode_len = 0;
	int32 stuffing_num = 0;
 	uint8 *bfr = g_nalu_ptr->buf;// = pInStream;

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
*/
PUBLIC int32 H264Dec_Process_slice (DEC_NALU_T *nalu_ptr)
{
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 curr_header;
	int32 new_picture;
#if SIM_IN_WIN
	if ((g_nFrame_dec_h264 == 205) && (img_ptr->slice_nr==0))
	{
		g_nFrame_dec_h264 = 205;
	}//weihu
#endif
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
	}
	if(curr_slice_ptr->start_mb_nr>=img_ptr->frame_size_in_mbs)//for error
	{
		g_image_ptr->error_flag = TRUE;
		return -1;
	}

	H264Dec_RestSliceHeader (img_ptr, curr_slice_ptr);
#if _MVC_
	if(g_curr_slice_ptr->view_id >= 0)
	{
		g_curr_slice_ptr->p_Dpb = g_dpb_layer[g_curr_slice_ptr->view_id];
	}
#endif
//#if SIM_IN_WIN
//	if (H264Dec_FMO_init(img_ptr) == FALSE || img_ptr->error_flag == 1)
//#else
    if (img_ptr->error_flag == TRUE)
//#endif
	{
		//img_ptr->error_flag = TRUE;
		return -1;
	}

	new_picture = H264Dec_is_new_picture (img_ptr);
    //new_picture |=  (g_dec_picture_ptr==NULL);  //for error stream//weihu

	if(g_dec_picture_ptr==NULL)//for error stream//weihu
        new_picture=TRUE;
	else if((img_ptr->curr_mb_nr <= (int)curr_slice_ptr->start_mb_nr)&&(curr_slice_ptr->start_mb_nr>0))//&&(img_ptr->curr_mb_nr>0)
		new_picture=FALSE;

	img_ptr->is_new_pic = new_picture;

	if (new_picture)
	{
		H264Dec_init_picture (img_ptr);
		curr_header = SOP;
	}else
	{
		curr_header = SOS;
	}

	H264Dec_init_list (img_ptr, img_ptr->type);
#if _MVC_
	if(g_curr_slice_ptr->svc_extension_flag == 0 || g_curr_slice_ptr->svc_extension_flag == 1)
		H264Dec_reorder_list_mvc ();
	else
#endif
 		H264Dec_reorder_list ();
	if (img_ptr->error_flag == TRUE)
        return -1;
/*
#if SIM_IN_WIN	

	//if((g_nFrame_dec_h264 == 9)&&(img_ptr->slice_nr==3))
	{
		uint8	*p;//jzy
		int		i,j,k;
		int			frame_size;
		uint32		val;
		
		int			frame_width  = img_ptr->width;
		int			frame_height = img_ptr->height;		


		MCAPRINTF (g_fp_ref_list_tv,"frame_cnt=%d,slice_type=%d,frame_width=%d,frame_height=%d\n",g_nFrame_dec_h264,g_image_ptr->type,frame_width,frame_height);	

		for (k=0; k<img_ptr->num_ref_idx_l0_active; k++)
		{			
			frame_size =(frame_width/8) * frame_height;
			//luma
			p = g_list0[k]->imgY;
			if(p==0)
			{			
				for (i = 0; i <frame_size; i++)				
					MCAPRINTF (g_fp_ref_list_tv, "%016x\n", 0);
			}
			else
			{
				for (i = 0; i < frame_size; i++)
				{
					for (j = 0; j<8; j++)
					{
					val = p[8*i+7-j];		
					MCAPRINTF (g_fp_ref_list_tv, "%02x", val);
					}
					MCAPRINTF (g_fp_ref_list_tv, "\n");

				}
			}

			frame_size =(frame_width/8) * (frame_height/2);
			p = g_list0[k]->imgU;
			if(p==0)
			{			
				for (i = 0; i <frame_size; i++)				
					MCAPRINTF (g_fp_ref_list_tv, "%016x\n", 0);
			}
			else
			{
				for (i = 0; i < frame_size; i++)
				{
					for (j = 0; j<8; j++)
					{
					val = p[8*i+7-j];		
					MCAPRINTF (g_fp_ref_list_tv, "%02x", val);
					}
					MCAPRINTF (g_fp_ref_list_tv, "\n");

				}
			}
		}
		for (k=img_ptr->num_ref_idx_l0_active; k<MAX_REF_FRAME_NUMBER; k++)
		{			
			frame_size =(frame_width/8) * (frame_height/2)*3;
			for (i = 0; i < frame_size; i++)
			{		
				MCAPRINTF (g_fp_ref_list_tv, "%016x\n", 0);			
			}		
		}
		//list1

		for (k=0; k<img_ptr->num_ref_idx_l1_active; k++)
		{
			frame_size =(frame_width/8) * frame_height;
			//luma
			p = g_list1[k]->imgY;
			if(p==0)
			{			
				for (i = 0; i <frame_size; i++)				
					MCAPRINTF (g_fp_ref_list_tv, "%016x\n", 0);
			}
			else
			{
				for (i = 0; i < frame_size; i++)
				{
					for (j = 0; j<8; j++)
					{
					val = p[8*i+7-j];		
					MCAPRINTF (g_fp_ref_list_tv, "%02x", val);
					}
					MCAPRINTF (g_fp_ref_list_tv, "\n");

				}
			}

			frame_size =(frame_width/8) * (frame_height/2);
			p = g_list1[k]->imgU;
			if(p==0)
			{			
				for (i = 0; i <frame_size; i++)				
					MCAPRINTF (g_fp_ref_list_tv, "%016x\n", 0);
			}
			else
			{
				for (i = 0; i < frame_size; i++)
				{
					for (j = 0; j<8; j++)
					{
					val = p[8*i+7-j];		
					MCAPRINTF (g_fp_ref_list_tv, "%02x", val);
					}
					MCAPRINTF (g_fp_ref_list_tv, "\n");

				}
			}
		}
		for (k=img_ptr->num_ref_idx_l1_active; k<MAX_REF_FRAME_NUMBER; k++)
		{
			frame_size =(frame_width/8) * (frame_height/2)*3;
			for (i = 0; i < frame_size; i++)
			{		
				MCAPRINTF (g_fp_ref_list_tv, "%016x\n", 0);			
			}		
		}

	}	
#endif
*/
	//configure ref_list_buf[24]
	{
		int i;

#if SIM_IN_WIN		
		memset(ref_list_buf, 0, 33*sizeof(int));
#else
        //ref_list_buf[32]=0;
		memset(ref_list_buf, 0, 33*sizeof(int));
#endif

		for (i=0; i<img_ptr->num_ref_idx_l0_active; i++)//g_list_size[0]//weihu
		{
			ref_list_buf[32] |= ((g_list0[i]->is_long_term)<<i);
		}
		for (i=img_ptr->num_ref_idx_l0_active; i<MAX_REF_FRAME_NUMBER; i++)//g_list_size[0]
		{
			ref_list_buf[32] |= (1<<i);
		}
		for (i=0; i<g_list_size[1]; i++)
		{
			ref_list_buf[32] |= ((g_list1[i]->is_long_term)<<(i+16));
		}
		for (i=g_list_size[1]; i<MAX_REF_FRAME_NUMBER; i++)
		{
			ref_list_buf[32] |= (1<<(i+16));
		}

		for(i=0;i<img_ptr->num_ref_idx_l0_active;i++)//g_list_size[0]
		{
		   ref_list_buf[i] = g_list0[i]->poc;
		}
		for(i=0;i<g_list_size[1];i++)
		{
			ref_list_buf[i+16] = g_list1[i]->poc;
		}
	}

	/*if (g_active_pps_ptr->entropy_coding_mode_flag)
	{
		arideco_start_decoding (img_ptr);
	}*///移到slice_data处理程序中
#if SIM_IN_WIN
	if (new_picture)
	{
		h264Dec_PicLevelSendRefAddressCommmand (img_ptr);
	}
#endif
	img_ptr->curr_mb_nr = curr_slice_ptr->start_mb_nr;

	return curr_header;
}
/*
LOCAL void H264Dec_exit_slice (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = g_old_slice_ptr;

	old_slice_ptr->frame_num = img_ptr->frame_num;
	old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
	old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
	old_slice_ptr->idr_flag = img_ptr->idr_flag;

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

	VSP_READ_REG_POLL(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 17, 1, 1, "DBK_CTR1: polling dbk slice idle");
	VSP_WRITE_CMD_INFO((VSP_DBK << MID_SHIFT_BIT) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

	VSP_READ_REG_POLL(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "VSP_DBG: polling AHB idle");
	VSP_WRITE_CMD_INFO((VSP_GLB << MID_SHIFT_BIT) | (1<<24) |((1<<7)|VSP_DBG_WOFF));

	return;
}*/
LOCAL void H264Dec_exit_slice (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = g_old_slice_ptr;
	
	old_slice_ptr->frame_num = img_ptr->frame_num;
	old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
	old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
	old_slice_ptr->idr_flag = img_ptr->idr_flag;
	
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
	
#if _MVC_
	old_slice_ptr->view_id = g_curr_slice_ptr->view_id;
	old_slice_ptr->inter_view_flag = g_curr_slice_ptr->inter_view_flag;
	old_slice_ptr->anchor_pic_flag = g_curr_slice_ptr->anchor_pic_flag;
#endif
	
	return;
}

void foo2(void)
{

}
/*
LOCAL void set_ref_pic_num(DEC_IMAGE_PARAMS_T *img_ptr)
{
  int i;

  int slice_id=img_ptr->slice_nr;

  for (i=0;i<g_list_size[0];i++)
  {
    g_dec_picture_ptr->ref_pic_num        [slice_id][0][i] = g_list0[i]->poc * 2 ;
  }

  for (i=0;i<g_list_size[1];i++)
  {
    g_dec_picture_ptr->ref_pic_num        [slice_id][1][i] = g_list1[i]->poc * 2 ;
  }
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

 //	  	PRINTF("%d, %d\n", img_ptr->mb_x, img_ptr->mb_y);
 #if _DEBUG_
		if ((img_ptr->mb_x == 4) && (img_ptr->mb_y == 0) && (g_nFrame_dec_h264 == 0))
		{
			foo2();
		}
#endif
		FPRINTF (g_fp_trace_mbc, "frame: %d, mb_y: %d, mb_x: %d\n", g_nFrame_dec_h264, img_ptr->mb_y, img_ptr->mb_x);//weihu
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_ISlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	

		if (img_ptr->error_flag)
		{
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
			VSP_WRITE_CMD_INFO((VSP_DBK << MID_SHIFT_BIT) | (2<<24) |(HDBK_CFG_FINISH_WOFF<<8)|DBK_SID_CFG_WOFF);
		}

		H264Dec_mb_level_sync (img_ptr);
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

	set_ref_pic_num(img_ptr);

	while (!end_of_slice)
	{
		img_ptr->mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
		img_ptr->mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;
		curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	#if _DEBUG_
		if ((img_ptr->mb_x == 4) && (img_ptr->mb_y == 4) && (g_nFrame_dec_h264 == 5))//weihu
		{
			foo2();
		}
	#endif	
		FPRINTF (g_fp_trace_mbc, "frame: %d, mb_y: %d, mb_x: %d\n", g_nFrame_dec_h264, img_ptr->mb_y, img_ptr->mb_x);//weihu
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

		H264Dec_read_one_macroblock_PSlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        
        if (img_ptr->error_flag)
        {
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
			return;
		}
		

		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (curr_mb_info_ptr, mb_cache_ptr);	
	
		//dbk
		if (!img_ptr->fmo_used)
		{
  			H264Dec_BS_and_Para_PSLICE (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		}else
		{
			VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, (img_ptr->mb_y << 16) | img_ptr->mb_x, "config dbk x and y id");
			VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");
			VSP_WRITE_CMD_INFO((VSP_DBK << MID_SHIFT_BIT) | (2<<24) |(HDBK_CFG_FINISH_WOFF<<8)|DBK_SID_CFG_WOFF);
		}		

		H264Dec_mb_level_sync (img_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr);
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

LOCAL void H264Dec_decode_one_slice_B (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = NULL;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

	while (!end_of_slice)
	{
		img_ptr->mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
		img_ptr->mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;
		curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	#if _DEBUG_
		if ((img_ptr->mb_x == 21) && (img_ptr->mb_y == 9) && (g_nFrame_dec_h264 == 5))
		{
			foo2();
		}
	#endif
		FPRINTF (g_fp_trace_mbc, "frame: %d, mb_y: %d, mb_x: %d\n", g_nFrame_dec_h264, img_ptr->mb_y, img_ptr->mb_x);//weihu
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

		H264Dec_read_one_macroblock_BSlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        
        if (img_ptr->error_flag)
        {
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
		if (curr_mb_info_ptr->is_skipped )
		{
			H264Dec_set_nnz_to_zero (curr_mb_info_ptr, mb_cache_ptr);
		}else
		{
			H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		}
		
		if (img_ptr->error_flag)
		{
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
			VSP_WRITE_CMD_INFO((VSP_DBK << MID_SHIFT_BIT) | (2<<24) |(HDBK_CFG_FINISH_WOFF<<8)|DBK_SID_CFG_WOFF);
		}		

		H264Dec_mb_level_sync (img_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr);
	}

	H264Dec_exit_slice (img_ptr);

	return;
}
*/
void fill_wp_params(DEC_IMAGE_PARAMS_T *img_ptr)
{
  int i, j/*, k*/;
  int comp;
  int log_weight_denom;
  //int tb, td;
  int bframe = (img_ptr->type==B_SLICE);
  int max_bwd_ref, max_fwd_ref;
  //int tx,DistScaleFactor;

  max_fwd_ref = img_ptr->num_ref_idx_l0_active;
  max_bwd_ref = img_ptr->num_ref_idx_l1_active;

  if (g_active_pps_ptr->weighted_bipred_idc == 2 && bframe)
  {
    img_ptr->luma_log2_weight_denom = 5;
    img_ptr->chroma_log2_weight_denom = 5;
    img_ptr->wp_round_luma = 16;
    img_ptr->wp_round_chroma = 16;

    for (i=0; i<MAX_REF_FRAME_NUMBER; i++)
    {
      /*for (comp=0; comp<3; comp++)
      {
        log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
        g_wp_weight[0][i][comp] = 1<<log_weight_denom;
        g_wp_weight[1][i][comp] = 1<<log_weight_denom;
        g_wp_offset[0][i][comp] = 0;
        g_wp_offset[1][i][comp] = 0;
      }*/
    }
  }

  if (bframe)
  {
    for (i=0; i<max_fwd_ref; i++)
    {
      for (j=0; j<max_bwd_ref; j++)
      {
        for (comp = 0; comp<3; comp++)
        {
          log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
          if (g_active_pps_ptr->weighted_bipred_idc == 1)
          {
            g_wbp_weight[0][i][j][comp] =  g_wp_weight[0][i][comp];
            g_wbp_weight[1][i][j][comp] =  g_wp_weight[1][j][comp];
          }
          /*else if (g_active_pps_ptr->weighted_bipred_idc == 2)
          {
            td = Clip3(-128,127,g_list1[j]->poc - g_list0[i]->poc);
            if (td == 0 || g_list1[j]->is_long_term || g_list0[i]->is_long_term)
            {
              g_wbp_weight[0][i][j][comp] =   32;
              g_wbp_weight[1][i][j][comp] =   32;
            }
            else
            {
              tb = Clip3(-128,127,img_ptr->ThisPOC - g_list0[i]->poc);

              tx = (16384 + ABS(td/2))/td;
              DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);

              g_wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
              g_wbp_weight[0][i][j][comp] = 64 - g_wbp_weight[1][i][j][comp];
              if (g_wbp_weight[1][i][j][comp] < -64 || g_wbp_weight[1][i][j][comp] > 128)
              {
                g_wbp_weight[0][i][j][comp] = 32;
                g_wbp_weight[1][i][j][comp] = 32;
                g_wp_offset[0][i][comp] = 0;
                g_wp_offset[1][j][comp] = 0;
              }
            }
          }*/
        }
      }
   }
 }
}
//extern BiContextType context[308];
extern int *context;
int tmp1;
PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = g_curr_slice_ptr;
	DEC_PPS_T	*active_pps_ptr = g_active_pps_ptr;

	MMDecRet ret = MMDEC_ERROR;
	unsigned int tmp;

	uint16 Slice_first_mb_x, Slice_first_mb_y;
	uint8  slice_type = curr_slice_ptr->picture_type;
	//uint32 range, value;
	uint8  weighted_en=(g_active_pps_ptr->weighted_bipred_idc>0)&&(slice_type==B_slice)||g_active_pps_ptr->weighted_pred_flag&&(slice_type==P_slice);
	
	int	   i,j,tmp2;
	
	
	//DEC_BS_STORE temp_strm_stat;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	
//    OR1200_WRITE_REG(0x18001c, 0x12345670,"for FPGA debug 7");

	if (active_pps_ptr->entropy_coding_mode_flag)
	{
#if SIM_IN_WIN
		CONTEXT_FPRINTF(g_fp_context_tv,"frame_cnt=%d,slice_cnt=%d\n",g_nFrame_dec_h264,img_ptr->slice_nr);
#endif
		init_contexts (img_ptr);		
		//cabac_new_slice();
		last_dquant=0;
	}

	Slice_first_mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
	Slice_first_mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;
	
#if SIM_IN_WIN
	 if ( (active_pps_ptr->weighted_bipred_idc > 0  && (img_ptr->type == B_SLICE)) || (active_pps_ptr->weighted_pred_flag && img_ptr->type !=I_SLICE))
	{
	    fill_wp_params(img_ptr);
	}
	 if (img_ptr->apply_weights)
	 {
		 img_ptr->mbc_cfg_cmd = (img_ptr->luma_log2_weight_denom<<4) | (img_ptr->chroma_log2_weight_denom<<1) | 1;
	 }else
	 {
		 img_ptr->mbc_cfg_cmd = 0;
	 }
	 curr_slice_ptr->next_header = -1;


		
	

	
	




	//以下三个函数的内部，然后写将参数写到一个buffer里头去，然后再将正常的解码流程解出的东西输入到buffer里头去，比较两个buffer内容是否相同。
	//{
		
	
		

		g_par_reg_ptr->PARSER_CFG0 = 0;

		vldLineBuf = (LINE_BUF_T *)safe_malloc(sizeof(LINE_BUF_T) * (img_ptr->frame_width_in_mbs));
		currMBBuf  = (MB_BUF_T   *)safe_malloc(sizeof(MB_BUF_T  ));
		leftMBBuf  = (MB_BUF_T   *)safe_malloc(sizeof(MB_BUF_T  ));

		g_par_reg_ptr->PARSER_CFG0 |= (slice_type << 15) | (g_active_pps_ptr->transform_8x8_mode_flag << 14) | (Slice_first_mb_y << 7)  | (Slice_first_mb_x); //configure the transform_8x8_mode_flag
		g_par_reg_ptr->PARSER_CFG0 |= ((img_ptr->num_ref_idx_l0_active-1)<<17) | ((img_ptr->num_ref_idx_l1_active-1)<<23);
#else
		if ((img_ptr->type == B_SLICE) && g_active_pps_ptr->weighted_bipred_idc == 2)
		{
			img_ptr->luma_log2_weight_denom = 5;
			img_ptr->chroma_log2_weight_denom = 5;
			img_ptr->wp_round_luma = 16;
			img_ptr->wp_round_chroma = 16;
		}//weihu
#endif
		//store the bitstream state
		/*temp_strm_stat.bitcnt = stream->bitcnt;
		temp_strm_stat.bitcnt_before_vld = stream->bitcnt_before_vld;
		temp_strm_stat.bitsLeft = stream->bitsLeft;
		temp_strm_stat.bufa = stream->bufa;
		temp_strm_stat.bufb = stream->bufb;
		temp_strm_stat.p_stream = stream->p_stream;
		temp_strm_stat.rdptr = stream->rdptr;
		temp_strm_stat.stream_len = stream->stream_len;
		temp_strm_stat.stream_len_left = stream->stream_len_left;
		memcpy(temp_strm_stat.rdbfr, stream->rdbfr,  BITSTREAM_BFR_SIZE * sizeof (int32));
		if (g_active_pps_ptr->entropy_coding_mode_flag)
		{
			range = img_ptr->de_cabac.Drange;
			value = img_ptr->de_cabac.Dvalue;
		}*/	

		//configure the slice info buf
		slice_info[0] = slice_type;
		slice_info[1] = img_ptr->slice_nr;
		slice_info[2] = g_active_pps_ptr->constrained_intra_pred_flag;
		slice_info[3] = img_ptr->direct_type;
		slice_info[4] = g_active_sps_ptr->direct_8x8_reference_flag;
		slice_info[5] = curr_slice_ptr->qp;
		slice_info[6] = g_active_pps_ptr->chroma_qp_index_offset;
		slice_info[7] = g_active_pps_ptr->second_chroma_qp_index_offset;
		slice_info[8] = curr_slice_ptr->LFDisableIdc;
		slice_info[9] = g_active_pps_ptr->weighted_bipred_idc;
		slice_info[10] = img_ptr->framepoc;
		slice_info[11] = img_ptr->num_ref_idx_l0_active;
		slice_info[12] = mmin(slice_info[11], g_list_size[0]);
		slice_info[13] = mmin(img_ptr->num_ref_idx_l1_active, g_list_size[1]);

		//for(i=0;i<16;i++)
		//  slice_info[14+i]=g_list1_map_list0[i];//weihu
		
		slice_info[30] = g_active_sps_ptr->seq_scaling_matrix_present_flag;
		slice_info[31] = g_active_pps_ptr->pic_scaling_matrix_present_flag;
		slice_info[32] = g_active_pps_ptr->weighted_pred_flag;//weihu
		slice_info[33] = g_image_ptr->luma_log2_weight_denom;
		slice_info[34] = g_image_ptr->chroma_log2_weight_denom;
		slice_info[35] = img_ptr->DPB_addr_index;
        slice_info[36] = curr_slice_ptr->LFAlphaC0Offset;
        slice_info[37] = curr_slice_ptr->LFBetaOffset;
		slice_info[38] = Slice_first_mb_x;
        slice_info[39] = Slice_first_mb_y;
		slice_info[40] = g_active_pps_ptr->transform_8x8_mode_flag;
		slice_info[41] = g_active_pps_ptr->entropy_coding_mode_flag;
		slice_info[42] = img_ptr->num_ref_idx_l1_active;
		slice_info[43] = (g_active_sps_ptr->profile_idc!=0x42);
		slice_info[44] = weighted_en;
		//chroma_log2_weight_denom, 
		//Luma_log2_weight_denom
		// OR1200_WRITE_REG(0x180020, 0x12000000,"for FPGA debug 8");
#if SIM_IN_WIN
		PARSER_FPRINTF(g_fp_global, "work_mode = 0\n");//weihu
		PARSER_FPRINTF(g_fp_global, "standard = 4\n");//weihu
		PARSER_FPRINTF(g_fp_global, "Slice_first_mb_x = %x\n",Slice_first_mb_x);//weihu
		PARSER_FPRINTF(g_fp_global, "Slice_first_mb_y = %x\n",Slice_first_mb_y);//weihu
		PARSER_FPRINTF(g_fp_global, "Picwidth_in_mb = %x\n",img_ptr->frame_width_in_mbs);//weihu
		PARSER_FPRINTF(g_fp_global, "Picheight_in_mb = %x\n",img_ptr->frame_height_in_mbs);//weihu
		PARSER_FPRINTF(g_fp_global, "Slice_type = %x\n",slice_type);//weihu
		PARSER_FPRINTF(g_fp_global, "Slice_num = %x\n",img_ptr->slice_nr);//weihu
	//	PARSER_FPRINTF(g_fp_global, "Nalu_bytes = %x\n",g_nalu_ptr->len);//weihu
		PARSER_FPRINTF(g_fp_global, "constrained_intra_pred_flag = %x\n",g_active_pps_ptr->constrained_intra_pred_flag);//weihu
	    PARSER_FPRINTF(g_fp_global, "direct_spatial_mv_pred_flag = %x\n",img_ptr->direct_type);//weihu
		//	PARSER_FPRINTF(g_fp_global, "entropy_coding_mode_flag = %x\n",active_pps_ptr->entropy_coding_mode_flag);//weihu
        PARSER_FPRINTF(g_fp_global, "direct_8x8_inference_flag = %x\n",g_active_sps_ptr->direct_8x8_reference_flag);//weihu
        //PARSER_FPRINTF(g_fp_global, "transform_8x8_mode_flag = %x\n",g_active_pps_ptr->transform_8x8_mode_flag);//weihu
        //PARSER_FPRINTF(g_fp_global, "num_ref_idx_l0_active_minus1 = %x\n",img_ptr->num_ref_idx_l0_active-1);//weihu
		//PARSER_FPRINTF(g_fp_global, "num_ref_idx_l1_active_minus1 = %x\n",img_ptr->num_ref_idx_l1_active-1);//weihu
        PARSER_FPRINTF(g_fp_global, "chroma_qp_index_offset = %x\n",g_active_pps_ptr->chroma_qp_index_offset);
		PARSER_FPRINTF(g_fp_global, "second_chroma_qp_index_offset = %x\n",g_active_pps_ptr->second_chroma_qp_index_offset);
		PARSER_FPRINTF(g_fp_global, "SliceQPy = %x\n",curr_slice_ptr->qp);//weihu
		PARSER_FPRINTF(g_fp_global, "disable_deblocking_filter_idc = %x\n",curr_slice_ptr->LFDisableIdc);//weihu
		PARSER_FPRINTF(g_fp_global, "weighted_bipred_idc = %x\n",g_active_pps_ptr->weighted_bipred_idc);//weihu
		PARSER_FPRINTF(g_fp_global, "Curr_POC = %x\n",img_ptr->framepoc);//weihu
		PARSER_FPRINTF(g_fp_global, "ppa_info_vdb_en = %x\n",(g_active_sps_ptr->profile_idc!=0x42));//weihu
		//for(i=0;i<16;i++)
		//	PARSER_FPRINTF(g_fp_global, "maplist1_to_list0[%x] = %x\n",i,g_list1_map_list0[i]);//weihu


		PARSER_FPRINTF(g_fp_vld_global, "work_mode = 0\n");//weihu
		PARSER_FPRINTF(g_fp_vld_global, "standard = 4\n");//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Slice_first_mb_x = %x\n",Slice_first_mb_x);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Slice_first_mb_y = %x\n",Slice_first_mb_y);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Picwidth_in_mb = %x\n",img_ptr->frame_width_in_mbs);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Picheight_in_mb = %x\n",img_ptr->frame_height_in_mbs);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Slice_type = %x\n",slice_type);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Slice_num = %x\n",img_ptr->slice_nr);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "Nalu_bytes = %x\n",g_nalu_ptr->len);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "entropy_coding_mode_flag = %x\n",active_pps_ptr->entropy_coding_mode_flag);//weihu
        PARSER_FPRINTF(g_fp_vld_global, "direct_8x8_inference_flag = %x\n",g_active_sps_ptr->direct_8x8_reference_flag);//weihu
        PARSER_FPRINTF(g_fp_vld_global, "transform_8x8_mode_flag = %x\n",g_active_pps_ptr->transform_8x8_mode_flag);//weihu
        PARSER_FPRINTF(g_fp_vld_global, "num_ref_idx_l0_active_minus1 = %x\n",img_ptr->num_ref_idx_l0_active-1);//weihu
		PARSER_FPRINTF(g_fp_vld_global, "num_ref_idx_l1_active_minus1 = %x\n",img_ptr->num_ref_idx_l1_active-1);//weihu

		PARSER_FPRINTF(g_fp_global, "list_size0 = %x\n",slice_info[12]);//weihu
		PARSER_FPRINTF(g_fp_global, "list_size1 = %x\n",slice_info[13]);//weihu	
#endif       
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+IMG_SIZE_OFF, ((img_ptr->frame_height_in_mbs&0xff)<<8)|img_ptr->frame_width_in_mbs&0xff,"IMG_SIZE");

		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG0_OFF, ((0x1)<<31)|((curr_slice_ptr->qp&0x3f)<<25)|((img_ptr->slice_nr&0x1ff)<<16)|((img_ptr->frame_size_in_mbs&0x1fff)<<3)|slice_type&0x7,"VSP_CFG0");
        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, g_nalu_ptr->len&0xfffff,"VSP_CFG1");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF,((g_active_sps_ptr->profile_idc!=0x42)<<31)|(((slice_info[30]||slice_info[31])&0x1)<<29)|((img_ptr->curr_mb_nr&0x1fff)<<16) |((Slice_first_mb_y&0x7f)<<8)|Slice_first_mb_x&0x7f,"VSP_CFG2");
		
       		 tmp=((slice_info[8]&0x3)<<19)|((slice_info[9]&0x3)<<17)|((slice_info[32]&0x1)<<16)|((weighted_en&0x1)<<15)|((slice_info[41]&0x1)<<14)|((slice_info[40]&0x1)<<13)|((slice_info[4]&0x1)<<12)|((slice_info[3]&0x1)<<11)|((slice_info[2]&0x1)<<10) |((slice_info[37]&0x1f)<<5)|(slice_info[36]&0x1f);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,tmp,"VSP_CFG3");
		tmp=((slice_info[13]&0x1f)<<25)|((slice_info[12]&0x1f)<<20)|(((slice_info[42]-1)&0x1f)<<15)|(((slice_info[11]-1)&0x1f)<<10)|((slice_info[7]&0x1f)<<5)|((slice_info[6]&0x1f)<<0);		
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF,tmp,"VSP_CFG4");
        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,img_ptr->framepoc,"VSP_CFG5 cur_POC");
		
		
	//	tmp=((g_list1_map_list0[7]&0xf)<<28)|((g_list1_map_list0[6]&0xf)<<24)|((g_list1_map_list0[5]&0xf)<<20)|((g_list1_map_list0[4]&0xf)<<16)|((g_list1_map_list0[3]&0xf)<<12)|((g_list1_map_list0[2]&0xf)<<8)|((g_list1_map_list0[1]&0xf)<<4)|(g_list1_map_list0[0]&0xf);
	//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG6_OFF,tmp,"list1_map_list0 0");

	//	PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
		
	//	tmp=((g_list1_map_list0[15]&0xf)<<28)|((g_list1_map_list0[14]&0xf)<<24)|((g_list1_map_list0[13]&0xf)<<20)|((g_list1_map_list0[12]&0xf)<<16)|((g_list1_map_list0[11]&0xf)<<12)|((g_list1_map_list0[10]&0xf)<<8)|((g_list1_map_list0[9]&0xf)<<4)|(g_list1_map_list0[8]&0xf);
	//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG7_OFF,tmp,"list1_map_list0 1");
		
	//	PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu

		tmp=((g_list0_map_addr[15]&0x3)<<30)|((g_list0_map_addr[4]&0x3f)<<24)|((g_list0_map_addr[3]&0x3f)<<18)|((g_list0_map_addr[2]&0x3f)<<12)|((g_list0_map_addr[1]&0x3f)<<6)|((g_list0_map_addr[0]&0x3f)<<0);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX0_OFF,tmp,"g_list0_map_addr 0");//next 376 
#if SIM_IN_WIN
		PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
#endif
		tmp=(((g_list0_map_addr[15]>>2)&0x3)<<30)|((g_list0_map_addr[9]&0x3f)<<24)|((g_list0_map_addr[8]&0x3f)<<18)|((g_list0_map_addr[7]&0x3f)<<12)|((g_list0_map_addr[6]&0x3f)<<6)|((g_list0_map_addr[5]&0x3f)<<0);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX1_OFF,tmp,"g_list0_map_addr 1");//next 376 
#if SIM_IN_WIN
		PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
#endif
		tmp=(((g_list0_map_addr[15]>>4)&0x3)<<30)|((g_list0_map_addr[14]&0x3f)<<24)|((g_list0_map_addr[13]&0x3f)<<18)|((g_list0_map_addr[12]&0x3f)<<12)|((g_list0_map_addr[11]&0x3f)<<6)|((g_list0_map_addr[10]&0x3f)<<0);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX2_OFF,tmp,"g_list0_map_addr 2");//next 376 
#if SIM_IN_WIN
		PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
#endif		
		tmp=((g_list1_map_addr[15]&0x3)<<30)|((g_list1_map_addr[4]&0x3f)<<24)|((g_list1_map_addr[3]&0x3f)<<18)|((g_list1_map_addr[2]&0x3f)<<12)|((g_list1_map_addr[1]&0x3f)<<6)|((g_list1_map_addr[0]&0x3f)<<0);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX3_OFF,tmp,"g_list1_map_addr 0");//next 376 
#if SIM_IN_WIN
		PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
#endif		
		tmp=(((g_list1_map_addr[15]>>2)&0x3)<<30)|((g_list1_map_addr[9]&0x3f)<<24)|((g_list1_map_addr[8]&0x3f)<<18)|((g_list1_map_addr[7]&0x3f)<<12)|((g_list1_map_addr[6]&0x3f)<<6)|((g_list1_map_addr[5]&0x3f)<<0);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX4_OFF,tmp,"g_list1_map_addr 1");//next 376 
#if SIM_IN_WIN
		PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
#endif
		
		tmp=(((g_list1_map_addr[15]>>4)&0x3)<<30)|((g_list1_map_addr[14]&0x3f)<<24)|((g_list1_map_addr[13]&0x3f)<<18)|((g_list1_map_addr[12]&0x3f)<<12)|((g_list1_map_addr[11]&0x3f)<<6)|((g_list1_map_addr[10]&0x3f)<<0);
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX5_OFF,tmp,"g_list1_map_addr 2");//next 376
#if SIM_IN_WIN		
		PARSER_FPRINTF(g_fp_global, "%x\n",tmp);//weihu
#endif
		
	//	PARSER_FPRINTF(g_fp_global, "seq_scaling_matrix_present_flag = %x\n",g_active_sps_ptr->seq_scaling_matrix_present_flag);//weihu
     //   PARSER_FPRINTF(g_fp_global, "pic_scaling_matrix_present_flag = %x\n",g_active_pps_ptr->pic_scaling_matrix_present_flag);//weihu
    // OR1200_WRITE_REG(0x180020, 0x02300000,"for FPGA debug 8");   
		//configure scaling matrix
		{
		  int i;

		  if(g_active_pps_ptr->pic_scaling_matrix_present_flag || g_active_sps_ptr->seq_scaling_matrix_present_flag)
		  {
			if(g_active_sps_ptr->seq_scaling_matrix_present_flag) // check sps first
			{
			  for(i=0; i<8; i++)
			  {
				if(i<6)
				{
				  if(!g_active_sps_ptr->seq_scaling_list_present_flag[i]) // fall-back rule A
				  {
					if(i==0)
					  SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
					else if(i==3)
					  SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
					else
					  SCI_MEMCPY(weightscale4x4[i], weightscale4x4[i-1], 16*sizeof(char));
				  }
				  else
				  {
					if(g_active_sps_ptr->UseDefaultScalingMatrix4x4Flag[i])
					{
						if (i<3)
						{
							SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
						}
						else
						{
							SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
						}
					}
					else
					  SCI_MEMCPY(weightscale4x4[i], g_active_sps_ptr->ScalingList4x4[i], 16*sizeof(char));
				  }
				}
				else
				{
				  if(!g_active_sps_ptr->seq_scaling_list_present_flag[i]) // fall-back rule A
				  {
					if(i==6)
					  SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
					else
					  SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
				  }
				  else
				  {
					if(g_active_sps_ptr->UseDefaultScalingMatrix8x8Flag[i-6])
					{
						if (i==6)
						{
							SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
						}
						else
						{
							SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
						}
					}
					else
					  SCI_MEMCPY(weightscale8x8[i-6], g_active_sps_ptr->ScalingList8x8[i-6], 64*sizeof(char));
				  }
				}
			  }
			}

			if(g_active_pps_ptr->pic_scaling_matrix_present_flag) // then check pps
			{
			  for(i=0; i<8; i++)
			  {
				if(i<6)
				{
				  if(!g_active_pps_ptr->pic_scaling_list_present_flag[i]) // fall-back rule B
				  {
					if (i==0)
					{
					  if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
						SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
					}
					else if (i==3)
					{
					  if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
						SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
					}
					else
					  SCI_MEMCPY(weightscale4x4[i], weightscale4x4[i-1], 16*sizeof(char));
				  }
				  else
				  {
					if(g_active_pps_ptr->UseDefaultScalingMatrix4x4Flag[i])
					{
						if (i<3)
						{
							SCI_MEMCPY(weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
						}
						else
						{
							SCI_MEMCPY(weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
						}
					}
					else
					  SCI_MEMCPY(weightscale4x4[i], g_active_pps_ptr->ScalingList4x4[i], 16*sizeof(char));
				  }
				}
				else
				{
				  if(!g_active_pps_ptr->pic_scaling_list_present_flag[i]) // fall-back rule B
				  {
					if (i==6)
					{
					  if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
						SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
					}
					else
					{
					  if(!g_active_sps_ptr->seq_scaling_matrix_present_flag)
						SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
					}
				  }
				  else
				  {
					if(g_active_pps_ptr->UseDefaultScalingMatrix8x8Flag[i-6])
					{
						if (i==6)
						{
							SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
						}
						else
						{
							SCI_MEMCPY(weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
						}
					}
					else
					  SCI_MEMCPY(weightscale8x8[i-6], g_active_pps_ptr->ScalingList8x8[i-6], 64*sizeof(char));
				  }
				}
			  }
			}
		  }
		}
    //OR1200_WRITE_REG(0x180020, 0x00340000,"for FPGA debug 8");

		for(i=0;i<33;i++)
		{
			OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+i*4,ref_list_buf[i],"ref_list_buf");
		}
		
		tmp=(g_image_ptr->chroma_log2_weight_denom<<8)|(g_image_ptr->luma_log2_weight_denom&0xff);
		OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+132,tmp,"chroma_log2_weight_denom,luma_log2_weight_denom");
		
		if (img_ptr->apply_weights)
		{		
			for (i=0; i<2; i++)
			{
				for (j=0; j<16; j++)
				{      	
					tmp=(g_wp_weight[i][j][2]<<18)|((g_wp_weight[i][j][1]&0x1ff)<<9)|(g_wp_weight[i][j][0]&0x1ff);
					OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+136+(2*j+32*i)*4,tmp,"g_wp_weight");				
					tmp=(g_wp_offset[i][j][2]<<16)|((g_wp_offset[i][j][1]&0xff)<<8)|(g_wp_offset[i][j][0]&0xff);
					OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+140+(2*j+32*i)*4,tmp,"g_wp_offset");
					
				}
			}
		}//weihu
	
	  //OR1200_WRITE_REG(0x180020, 0x00005600,"for FPGA debug 8");
		if(g_active_pps_ptr->pic_scaling_matrix_present_flag || g_active_sps_ptr->seq_scaling_matrix_present_flag)
		{
		
			for(i=3;i<6;i++)//inter
			{		
				for(j=0;j<4;j++)
				{
					tmp=((weightscale4x4[i][j][3]&0xff)<<24)|((weightscale4x4[i][j][2]&0xff)<<16)|((weightscale4x4[i][j][1]&0xff)<<8)|((weightscale4x4[i][j][0]&0xff)<<0);
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER4x4Y_OFF+(i-3)*16+j*4,tmp,"weightscale inter4x4");
				}
			}
			
			for(i=0;i<3;i++)//intra
			{		
				for(j=0;j<4;j++)
				{
					tmp=((weightscale4x4[i][j][3]&0xff)<<24)|((weightscale4x4[i][j][2]&0xff)<<16)|((weightscale4x4[i][j][1]&0xff)<<8)|((weightscale4x4[i][j][0]&0xff)<<0);
					OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA4x4Y_OFF+i*16+j*4,tmp,"weightscale intra4x4");
				}
			}
					
			for(j=0;j<8;j++)
			{
				tmp=((weightscale8x8[1][j][3]&0xff)<<24)|((weightscale8x8[1][j][2]&0xff)<<16)|((weightscale8x8[1][j][1]&0xff)<<8)|((weightscale8x8[1][j][0]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+2*j*4,tmp,"weightscale inter8x8");
				tmp=((weightscale8x8[1][j][7]&0xff)<<24)|((weightscale8x8[1][j][6]&0xff)<<16)|((weightscale8x8[1][j][5]&0xff)<<8)|((weightscale8x8[1][j][4]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+(2*j+1)*4,tmp,"weightscale inter8x8");
			}
			for(j=0;j<8;j++)
			{
				tmp=((weightscale8x8[0][j][3]&0xff)<<24)|((weightscale8x8[0][j][2]&0xff)<<16)|((weightscale8x8[0][j][1]&0xff)<<8)|((weightscale8x8[0][j][0]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+2*j*4,tmp,"weightscale intra8x8");
				tmp=((weightscale8x8[0][j][7]&0xff)<<24)|((weightscale8x8[0][j][6]&0xff)<<16)|((weightscale8x8[0][j][5]&0xff)<<8)|((weightscale8x8[0][j][4]&0xff)<<0);
				OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+(2*j+1)*4,tmp,"weightscale intra8x8");
			}
	
		}
		//OR1200_WRITE_REG(0x180020, 0x00000078,"for FPGA debug 8");
        /*
		PARSER_FPRINTF(g_fp_slicedata_offset, " offset_bits = %d\n", stream->bitcnt);//weihu
		FFLUSH(g_fp_slicedata_offset);
		tmp=stream->bitcnt>>3;
		for(i=0;i<tmp;i++)
		{	
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");//check bsm_rdy to rd	
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (8<<24)|0x1,"BSM_OP");//flush stuff bits to slice head 
		}
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");//check bsm_rdy to rd	
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, ((stream->bitcnt&7)<<24)|0x1,"BSM_OP");//flush stuff bits to slice head 
        *///解slice header不用BSM硬件
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x7c, (0x00000000),"display en");
		//tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		//if(tmp1&0x34)
		//       OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x7c, (0x99990000|tmp1),"display en");
		//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0xfFF,"clear BSM_frame done int"); 
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0xfFF,"clear BSM_frame done int");  
		//tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		//if(tmp1)
		//       OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x64, (0x99990000|tmp1),"display en");    
		tmp1=0;
		
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x74, 0x55,"display en");
		tmp2=1;
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x60, 0x55,"display en");
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, (0x99999999),"display en"); 
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x78, (0x99999999),"display en"); 
		//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,0x34,"VSP_INT_MASK");//enable int //frame done/error/timeout
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,0x0,"VSP_INT_MASK");//enable int //frame done/error/timeout
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 1,"RAM_ACC_SEL");//change ram access to vsp hw
//		OR1200_WRITE_REG(0x180020, 0x55555550|(0xa|img_ptr->is_need_init_vsp_hufftab),"for FPGA debug 9");
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_START_OFF,0xa|img_ptr->is_need_init_vsp_hufftab,"VSP_START");//start vsp   vld/vld_table//load_vld_table_en
		//load_vld_table_en=0;//do not load table again	
		//while(tmp2<0x10000)
		//      tmp2++;
		    
		
		//OR1200_WRITE_REG(0x180024, 0x66666666,"for FPGA debug 9");
		/*OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x66667777,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x66000000,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x00770000,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x00008800,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x00000099,"display en");*/
		//img_ptr->is_need_init_vsp_hufftab = FALSE;
		////OR1200_WRITE_REG(GLB_REG_BASE_ADDR+MCU_SLEEP_OFF,0x1,"MCU_SLEEP");
		//asm("l.mtspr\t\t%0,%1,0": : "r" (SPR_PMR), "r" (SPR_PMR_SME));//MCU_SLEEP
		//OR1200_WRITE_REG(0x180024, 0x66667777,"for FPGA debug 9");
		
    
	  
	
		
		//OR1200_WRITE_REG(0x180020, 0x12345670,"for FPGA debug 8");
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x60, 0x12345670,"display en");
		/*OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x77776666,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x60000000,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x00700000,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x00008000,"display en");
		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, 0x00000090,"display en");*/
		//OR1200_WRITE_REG(0x180024, 0xffffffff,"for FPGA debug 9");
#if SIM_IN_WIN
		FPRINTF(g_fp_global_tv,"//***********************************frame num=%d slice id=%d\n",g_nFrame_dec_h264,slice_info[1]);


#if FPGA_AUTO_VERIFICATION
#else
		sw_wr_bsm=0;
#endif
 		H264Dec_parse_slice_data();
#if FPGA_AUTO_VERIFICATION
#else
		sw_wr_bsm=1;
#endif
		//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+VSP_INT_SYS_OFF, 0x00000004,0x00000004,"BSM_frame done int");//check HW int	
		OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, 0x00000004,0x00000004,"BSM_frame done int");//check HW int	
		OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x4,"clear BSM_frame done int");
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
		
#else
		//中断唤醒
		//tmp=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_SYS_OFF, "check interrupt type");//MCU_SLEEP
		
		//OR1200_WRITE_REG(0x180024, 0x77777777,"for FPGA debug 9");
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x60, 0x77777777,"display en");
		tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x60, 0x88888888,"display en");
		//OR1200_WRITE_REG(0x180024, 0x88888888,"for FPGA debug 9");
		while ((tmp1&0x34)==0) //weihu tmp
		{
			//tmp=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_SYS_OFF, "check interrupt type");//MCU_SLEEP
			tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		//	OR1200_WRITE_REG(0x180024, (((tmp2++)&0xffff)<<16)|(tmp1&0xff),"for FPGA debug 9");
		//	OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x74, (((tmp2++)&0xffff)<<16)|(tmp1&0xff),"display en");
		}

    //OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x70, (0x99990000|tmp1),"display en");   
    
		if(tmp1&0x30)
		{
            img_ptr->error_flag=1;
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame error int");//weihu// 0x34
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame error int");//weihu// 0x34
			//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x74, 0x12345600|(tmp1&0x30),"display en");
			//OR1200_WRITE_REG(0x180024, 0x12345600|(tmp1&0x30),"for FPGA debug 9");
		}
		else if((tmp1&0x00000004)==0x00000004)
		{
			img_ptr->error_flag=0;
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame done int");//0x4
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0x1ff,"clear BSM_frame done int");//0x4
			//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x74, 0x12345600|(tmp1&0x04),"display en");
			//OR1200_WRITE_REG(0x180024, 0x12345600|(tmp1&0x04),"for FPGA debug 9");
		}
		//tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x78, (0x99990000|tmp1),"display en");

		//while(tmp2<0x8000)
		//      tmp2++;
		//tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
		//OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x68, (0x99990000|tmp1),"display en");
		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,"BSM_clr enable");//check bsm is idle	
#endif
        OR1200_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
		img_ptr->is_need_init_vsp_hufftab = FALSE;
		H264Dec_exit_slice(img_ptr);//g_image_ptr);
		//reset the bitstream state
	    /*		
		memcpy(stream->rdbfr, temp_strm_stat.rdbfr,  BITSTREAM_BFR_SIZE * sizeof (int32));
		stream->bitcnt = temp_strm_stat.bitcnt;
		stream->bitcnt_before_vld =  temp_strm_stat.bitcnt_before_vld;
		stream->bitsLeft = temp_strm_stat.bitsLeft;
		stream->bufa = temp_strm_stat.bufa;
		stream->bufb = temp_strm_stat.bufb;
		stream->p_stream = temp_strm_stat.p_stream;
		stream->rdptr = temp_strm_stat.rdptr;
		stream->stream_len = temp_strm_stat.stream_len;
		stream->stream_len_left = temp_strm_stat.stream_len_left;
		
		if (g_active_pps_ptr->entropy_coding_mode_flag)
		{
			img_ptr->de_cabac.Drange = range;
			img_ptr->de_cabac.Dvalue = value;
			init_contexts (img_ptr);
		}*/
	//}

/*	if (curr_slice_ptr->picture_type == I_SLICE)
	{
		H264Dec_decode_one_slice_I (img_ptr);
	}else if (curr_slice_ptr->picture_type == P_SLICE)
	{
		img_ptr->list_count = 1;
		H264Dec_decode_one_slice_P (img_ptr);
	}else if (curr_slice_ptr->picture_type == B_SLICE)
	{
		img_ptr->list_count = 2;
		H264Dec_decode_one_slice_B (img_ptr);
	}else
	{
		PRINTF ("the other picture type is not supported!\n");
	}*///weihu2
//	PRINTF ("\tend at mb_x:%d, mb_y:%d, stuffing:%d", img_ptr->mb_x, img_ptr->mb_y, g_destuffing_cnt); 

    img_ptr->slice_nr++;
//	OR1200_WRITE_REG(0x180028, (0x12340000+img_ptr->slice_nr),"for FPGA debug 10");

	if (img_ptr->error_flag)
 	{
		return MMDEC_ERROR;
	}

	
#if SIM_IN_WIN
	if (SOP == curr_slice_ptr->next_header) //end of picture
	{

		ret = H264Dec_Picture_Level_Sync (img_ptr);

		//if (img_ptr->fmo_used)
		//{
  	  	 //	H264Dec_deblock_one_frame (img_ptr);
		//}
		PrintfRecFrame (img_ptr, g_dec_picture_ptr);
#else
	//if(end of picture)
	tmp1=OR1200_READ_REG(GLB_REG_BASE_ADDR+VSP_DBG_STS0_OFF, "check mb_x mb_y number");//weihu tmp
	if((((tmp1>>8)&0xff)==(uint)(img_ptr->frame_width_in_mbs-1))&&((tmp1&0xff)==(uint)(img_ptr->frame_height_in_mbs-1)))//weihu tmp
	{
		ret =MMDEC_OK;
		//配置状态寄存器，帧号等
//		OR1200_WRITE_REG(SHARE_RAM_BASE_ADDR+0x80, g_nFrame_dec_h264,"shareRAM 0x80 frame_num");//
#endif

		H264Dec_exit_picture (img_ptr);

//		H264Dec_display_control ();
    frame_dec_finish=TRUE;

		if(display_array_len>0)
		{
			g_dispFrmNum++;
			dec_output_ptr->frameEffective = TRUE;
			dec_output_ptr->pOutFrameY = display_array_Y[0];
			dec_output_ptr->pOutFrameU = display_array_UV[0];
			dec_output_ptr->pBufferHeader = display_array_BH[0];
			dec_output_ptr->mPicId = display_array_mPicId[0];
			//dec_output_ptr->pOutFrameV = display_array_UV[0];//g_dec_picture_ptr->imgV;
			OR_VSP_UNBIND(display_array_BH[0]);
			display_array_len--;
			for(i =0;i<display_array_len; i++)
			{
				display_array_BH[i]=display_array_BH[i+1];
				display_array_Y[i] = display_array_Y[i+1];
				display_array_UV[i] = display_array_UV[i+1];
				display_array_mPicId[i] = display_array_mPicId[i+1];
			}
		}

		
		dec_output_ptr->frame_width = img_ptr->width;
		dec_output_ptr->frame_height = img_ptr->height;
		

		
			
		g_dec_picture_ptr = NULL;//weihu for output
		g_nFrame_dec_h264++;

	}

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




















































































