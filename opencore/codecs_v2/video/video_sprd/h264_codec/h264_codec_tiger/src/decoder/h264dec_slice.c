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
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#if defined(RUN_IN_PLATFORM)
PUBLIC int32 get_unit (uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len, int32 *start_code_len)
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

//	SCI_TRACE_LOW("get_unit 0, frm_bs_len %d, %0x, %0x, %0x, %0x\n",frm_bs_len,  bfr[0], bfr[1], bfr[2], bfr[3]);

	//start code
	while ((data = *ptr++) == 0x00)
	{
		len++;
	}
	*start_code_len = (len+1);
	*slice_unit_len = (*start_code_len);
	declen += (*start_code_len);
	g_nalu_ptr->buf += (*start_code_len);

	//destuffing
	bfr = g_nalu_ptr->buf;
	len = 0;

	//read til next start code, and remove the third start code code emulation byte
	while (declen < frm_bs_len)
	{
		data = *ptr++;len++;
		declen++;

		if (zero_num < 2)
		{
			*bfr++ = data; 
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
			}else
			{
				*bfr++ = data; 				

				if (data == 0x1)
				{
					if (zero_num >= 2)
					{
						startCode_len = zero_num + 1;
						break;
					}
				}else if (data == 0x00)
				{
					zero_num++;
				}else
				{
					zero_num = 0;
				}
			}
		}
	}

	*slice_unit_len += (len - startCode_len /*+ stuffing_num*/);
	
 	g_nalu_ptr->len = len - startCode_len - stuffing_num;
	declen -= startCode_len;

	SCI_TRACE_LOW("get_unit 1, dec_len %d, slice_unit_len %d, startcode_len %d, stuffing_num %d\n", declen,*slice_unit_len,  startCode_len, stuffing_num);

#if 0
	if (*slice_unit_len == 10476)
	{
		FILE *fp = fopen("/data/frm13bs.dat","ab");
		fwrite(g_nalu_ptr->buf,1,g_nalu_ptr->len,fp);
		fclose(fp);
	}
#endif
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
 	uint8 *bfr = g_nalu_ptr->buf = pInStream;

	ptr = pInStream;

	SCI_TRACE_LOW("get_unit_avc1 0, frm_bs_len %d, %0x, %0x, %0x, %0x\n",slice_unit_len,  bfr[0], bfr[1], bfr[2], bfr[3]);

	//read til next start code, and remove the third start code code emulation byte
	while (len < slice_unit_len)
	{
		data = *ptr++;len++;

		if (zero_num < 2)
		{
			*bfr++ = data; 
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
			}else
#endif
			{
				*bfr++ = data; 				

				if (data == 0x1)
				{
					if (zero_num >= 2)
					{
						startCode_len = zero_num + 1;
						break;
					}
				}else if (data == 0x00)
				{
					zero_num++;
				}else
				{
					zero_num = 0;
				}
			}
		}
	}

	SCI_TRACE_LOW("get_unit_avc1 1, len %d, stuffing_num %d\n", len, stuffing_num);

 	g_nalu_ptr->len = len -  stuffing_num;

	return 0;
}

PUBLIC int32 H264Dec_process_slice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_NALU_T *nalu_ptr)
{
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
		H264Dec_use_parameter_set (img_ptr, curr_slice_ptr->pic_parameter_set_id);
#ifdef _VSP_LINUX_
		if(img_ptr->error_flag)
			return -1;
#endif
	}

	H264Dec_RestSliceHeader (img_ptr, curr_slice_ptr);

	if (H264Dec_FMO_init(img_ptr) == FALSE || img_ptr->error_flag == 1)
	{
		img_ptr->error_flag = TRUE;
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
	H264Dec_reorder_list (img_ptr);

	if (img_ptr->is_cabac)
	{
		arideco_start_decoding (img_ptr);
	}

	if (new_picture)
	{
		h264Dec_PicLevelSendRefAddressCommmand (img_ptr);
	}
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

	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+DBK_DEBUG_OFF, 17, 1, 1, "DBK_CTR1: polling dbk slice idle");
	VSP_WRITE_CMD_INFO((VSP_DBK << CQM_SHIFT_BIT) | (1<<24) |((1<<7)|DBK_DEBUG_WOFF));

	VSP_READ_REG_POLL_CQM(VSP_GLB_REG_BASE+VSP_DBG_OFF, 6, 1, 1, "VSP_DBG: polling AHB idle");
	VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (1<<24) |((1<<7)|VSP_DBG_WOFF));
//SCI_TRACE_LOW ("out H264Dec_exit_slice!\n");	
	return;
}

void foo2(void)
{

}

LOCAL void set_ref_pic_num(DEC_IMAGE_PARAMS_T *img_ptr)
{
	int list ,i/*,j*/;
	int slice_id=img_ptr->slice_nr;

  	for (list = 0; list < img_ptr->list_count; list++)
  	{
		for (i=0;i<g_list_size[list];i++)
	  	{
			g_dec_picture_ptr->pic_num_ptr		  [slice_id][list][i] = g_list[list][i]->poc * 2 ;
	  	}
  	}
}

LOCAL void H264Dec_decode_one_slice_I (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_I: curr_mb_nr: %d, tot_mb_x: %d\n",img_ptr->curr_mb_nr, img_ptr->frame_width_in_mbs);
	while (!end_of_slice)
	{
		H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_read_one_macroblock_ISlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	

		if (img_ptr->error_flag)
		{
			return;
		}
		
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_mb_level_sync (img_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		curr_mb_info_ptr++;
	}

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_I, end: mb_x: %d, mb_y: %d\n", img_ptr->mb_x, img_ptr->mb_y);

	H264Dec_exit_slice (img_ptr);

	return;
}

LOCAL void H264Dec_decode_one_slice_P (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_P: curr_mb_nr: %d, tot_mb_x: %d\n",img_ptr->curr_mb_nr, img_ptr->frame_width_in_mbs);

	while (!end_of_slice)
	{
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
		}

		//vld
		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		}
		
		if (img_ptr->error_flag)
		{
			return;
		}
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
  		H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);		
		H264Dec_mb_level_sync (img_ptr);	
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		curr_mb_info_ptr++;
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

LOCAL void H264Dec_decode_one_slice_B (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 end_of_slice = FALSE;
	DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
	DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

	img_ptr->cod_counter = -1;

	set_ref_pic_num(img_ptr);

//	SCI_TRACE_LOW("H264Dec_decode_one_slice_B: curr_mb_nr: %d, tot_mb_x: %d\n",img_ptr->curr_mb_nr, img_ptr->frame_width_in_mbs);

	while (!end_of_slice)
	{
	#if _DEBUG_
		if ((img_ptr->mb_x == 0) && (img_ptr->mb_y == 2) && (g_nFrame_dec_h264 == 21))
		{
			foo2();
		}
	#endif

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
		}

		//vld
		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_start_vld_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		}
		
		if (img_ptr->error_flag)
		{
			return;
		}
		
		H264Dec_start_mbc_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		H264Dec_start_iqt_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);	
		H264Dec_BS_and_Para (img_ptr, curr_mb_info_ptr, mb_cache_ptr);		
		H264Dec_mb_level_sync (img_ptr);
		end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
		
		curr_mb_info_ptr++; 
	}

	H264Dec_exit_slice (img_ptr);

	return;
}

LOCAL void H264Dec_fill_wp_params (DEC_IMAGE_PARAMS_T *img_ptr, DEC_PPS_T *active_pps_ptr)
{
	int32 i, j/*, k*/;
  	int32 comp;
 	int32 log_weight_denom;
	int32 tb, td;
  	int32 bframe = (img_ptr->type==B_SLICE);
  	int32 max_bwd_ref, max_fwd_ref;
  	int32 tx,DistScaleFactor;
  	
//  if (bframe)
  	{
		if (active_pps_ptr->weighted_bipred_idc == 2)
	  	{
	    	img_ptr->luma_log2_weight_denom = 5;
	    	img_ptr->chroma_log2_weight_denom = 5;
	    	img_ptr->wp_round_luma = 16;
	    	img_ptr->wp_round_chroma = 16;

	    	for (i=0; i<MAX_REF_FRAME_NUMBER; i++)
	    	{
	      		for (comp=0; comp<3; comp++)
	      		{
	        		log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
	        		g_wp_weight[0][i][comp] = 1<<log_weight_denom;
	        		g_wp_weight[1][i][comp] = 1<<log_weight_denom;
	        		g_wp_offset[0][i][comp] = 0;
	        		g_wp_offset[1][i][comp] = 0;
	      		}
	    	}
	  	}
		
	  	max_fwd_ref = img_ptr->ref_count[0];
 		max_bwd_ref = img_ptr->ref_count[1];

    	for (i=0; i<max_fwd_ref; i++)
    	{
      		for (j=0; j<max_bwd_ref; j++)
      		{
        		for (comp = 0; comp<3; comp++)
        		{
        			log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
          			if (active_pps_ptr->weighted_bipred_idc == 1)
          			{
            			g_wbp_weight[0][i][j][comp] =  g_wp_weight[0][i][comp];
            			g_wbp_weight[1][i][j][comp] =  g_wp_weight[1][j][comp];
          			}else if (active_pps_ptr->weighted_bipred_idc == 2)
          			{
            			td = Clip3(-128,127,g_list[1][j]->poc - g_list[0][i]->poc);
            			if (td == 0 || g_list[1][j]->is_long_term || g_list[0][i]->is_long_term)
            			{
              				g_wbp_weight[0][i][j][comp] =   32;
              				g_wbp_weight[1][i][j][comp] =   32;
            			}else
            			{
              				tb = Clip3(-128,127,img_ptr->ThisPOC - g_list[0][i]->poc);
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
          			}
        		}
      		}
   		}
 	}
}

PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_PPS_T	*active_pps_ptr = g_active_pps_ptr;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	MMDecRet ret = MMDEC_ERROR;
	
	if (img_ptr->is_cabac)
	{
		init_contexts (img_ptr);
		cabac_new_slice();
	}

//	if ((active_pps_ptr->weighted_bipred_idc > 0 && (img_ptr->type == B_SLICE)) || (active_pps_ptr->weighted_pred_flag && img_ptr->type != I_SLICE))
	if ((img_ptr->type == B_SLICE) && ((active_pps_ptr->weighted_bipred_idc > 0) || (active_pps_ptr->weighted_pred_flag)))
	{
		H264Dec_fill_wp_params (img_ptr, active_pps_ptr);
	}	
	
	img_ptr->apply_weights = ((active_pps_ptr->weighted_pred_flag && (curr_slice_ptr->picture_type == P_SLICE ) )
          || ((active_pps_ptr->weighted_bipred_idc > 0 ) && (curr_slice_ptr->picture_type == B_SLICE)));

	if (img_ptr->apply_weights)
	{
		img_ptr->mbc_cfg_cmd = (img_ptr->luma_log2_weight_denom<<4) | (img_ptr->chroma_log2_weight_denom<<1) | 1;
	}else
	{
		img_ptr->mbc_cfg_cmd = 0;
	}
		
	if (curr_slice_ptr->picture_type == I_SLICE)
	{
		img_ptr->list_count = 0;
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
		img_ptr->error_flag = TRUE;
		SCI_TRACE_LOW ("the other picture type is not supported!\n");
	}

	if (img_ptr->error_flag)
	{
		SCI_TRACE_LOW("H264Dec_decode_one_slice_data: mb_x: %d, mb_y: %d, bit_cnt: %d\n",img_ptr->mb_x, img_ptr->mb_y, img_ptr->bitstrm_ptr->bitcnt);
		return MMDEC_ERROR;
	}

	img_ptr->slice_nr++;

	if (SOP == curr_slice_ptr->next_header) //end of picture
	{
	
		ret = H264Dec_Picture_Level_Sync (img_ptr);

		if (img_ptr->fmo_used)
		{
  	  	 	H264Dec_deblock_one_frame (img_ptr);
		}

		H264Dec_exit_picture (img_ptr);

		dec_output_ptr->frameEffective = 1;
		dec_output_ptr->frame_width = img_ptr->width;
		dec_output_ptr->frame_height = img_ptr->height;
		dec_output_ptr->pOutFrameY = g_dec_picture_ptr->imgY;
		dec_output_ptr->pOutFrameU = g_dec_picture_ptr->imgU;
		dec_output_ptr->pOutFrameV = g_dec_picture_ptr->imgV;
		dec_output_ptr->pBufferHeader = g_dec_picture_ptr->pBufferHeader;
		dec_output_ptr->reqNewBuf = 1;
#if 0
		if (g_nFrame_dec_h264 < 50)
		{
			FILE *fp = fopen("/data/h264_dec.yuv","ab");
			fwrite((uint8 *)dec_output_ptr->pOutFrameY,1,img_ptr->width*img_ptr->height,fp);
			fwrite((uint8 *)dec_output_ptr->pOutFrameU,1,img_ptr->width*img_ptr->height/2,fp);
			fclose(fp);
		}
#endif
		g_dec_picture_ptr = NULL;
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
