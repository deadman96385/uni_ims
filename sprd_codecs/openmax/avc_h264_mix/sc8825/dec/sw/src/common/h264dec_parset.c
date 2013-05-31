/******************************************************************************
 ** File Name:    h264dec_parset.c                                             *
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
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

/*
if sps_id is changed, size of frame may be changed, so the buffer for dpb and img
need to be re-allocate, and the parameter of img need to be re-computed
*/
LOCAL void H264Dec_active_sps (DEC_SPS_T *sps_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	img_ptr->max_frame_num = (1<<(sps_ptr->log2_max_frame_num_minus4+4));
	img_ptr->frame_width_in_mbs = (sps_ptr->pic_width_in_mbs_minus1+1);
	img_ptr->pic_height_in_map_units = (sps_ptr->pic_height_in_map_units_minus1+1);
	img_ptr->frame_height_in_mbs = (2-(uint8)sps_ptr->frame_mbs_only_flag)*img_ptr->pic_height_in_map_units;
	img_ptr->frame_size_in_mbs = img_ptr->frame_width_in_mbs*img_ptr->frame_height_in_mbs;
	img_ptr->width = img_ptr->frame_width_in_mbs * MB_SIZE;
	img_ptr->height = img_ptr->frame_height_in_mbs * MB_SIZE;	
	img_ptr->b4_pitch = (img_ptr->frame_width_in_mbs << 2);
	
	img_ptr->ext_width  = img_ptr->width + Y_EXTEND_SIZE * 2;
	img_ptr->ext_height = img_ptr->height + Y_EXTEND_SIZE * 2;
	img_ptr->start_in_frameY = img_ptr->ext_width * Y_EXTEND_SIZE + Y_EXTEND_SIZE;
	img_ptr->start_in_frameUV = (img_ptr->ext_width>>1) * UV_EXTEND_SIZE + UV_EXTEND_SIZE;

	if (!img_ptr->VSP_used)
	{
		DEC_MB_CACHE_T *mb_cache_ptr = g_mb_cache_ptr;
		int32 blk8x8Idx, blk4x4Idx = 0;
		int32 offset;

		//y
		for (blk8x8Idx = 0; blk8x8Idx < 4; blk8x8Idx++)
		{
			offset = (((blk8x8Idx >> 1) *img_ptr->ext_width+ (blk8x8Idx & 1) )<<3);
			mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset;
			mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset + 4;
			mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset + 4 * img_ptr->ext_width;
			mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset + 4 + 4 * img_ptr->ext_width;
		}

		//uv
		mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 0;
		mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 4;
		mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 4 * img_ptr->ext_width/2;
		mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 4 + 4 * img_ptr->ext_width/2;
	}else
	{
#if 0 //_H264_PROTECT_ & _LEVEL_HIGH_
		if ((sps_ptr->pic_width_in_mbs_minus1 < 3 || sps_ptr->pic_height_in_map_units_minus1 < 3) /*64x64*/
		||(sps_ptr->pic_width_in_mbs_minus1 > 79 || sps_ptr->pic_height_in_map_units_minus1 >  44) /*1280x720*/)  
		{
			img_ptr->error_flag |= ER_BSM_ID;
			img_ptr->return_pos1 |= (1<<12);
			return;
		}
#endif
	}
		
	if (g_active_sps_ptr != sps_ptr)
	{
		g_active_sps_ptr = sps_ptr;
		
#ifdef _VSP_LINUX_
	    	g_MbToSliceGroupMap = NULL;
		if(VSP_spsCb)
		{
			int ret = (*VSP_spsCb)(g_user_data,img_ptr->width,img_ptr->height,sps_ptr->num_ref_frames);
			if(!ret)
			{
			#if _H264_PROTECT_ & _LEVEL_LOW_
				img_ptr->error_flag |= ER_BSM_ID;
				img_ptr->return_pos1 |= (1<<13);
			#endif	
				return;
			}
		}else
		{
		#if _H264_PROTECT_ & _LEVEL_LOW_
			img_ptr->error_flag |= ER_BSM_ID;
			img_ptr->return_pos1 |= (1<<14);
		#endif	
			return;
		}
#endif

//		H264Dec_SeqLevelConfig (img_ptr);

		//reset memory alloc
		H264Dec_FreeExtraMem();

		H264Dec_init_img_buffer (img_ptr);
		
		if (!img_ptr->no_output_of_prior_pics_flag)
		{
			H264Dec_flush_dpb(g_dpb_ptr);
		}

		H264Dec_init_dpb (img_ptr, sps_ptr);

		g_dpb_ptr->num_ref_frames = g_active_sps_ptr->num_ref_frames;
	}

	return;
}

LOCAL void H264Dec_active_pps (DEC_PPS_T *pps_ptr)
{
	if (g_active_pps_ptr != pps_ptr)
	{
		g_active_pps_ptr = pps_ptr;
	}
}

PUBLIC void H264Dec_use_parameter_set (DEC_IMAGE_PARAMS_T *img_ptr, int32 pps_id)
{
	DEC_PPS_T *pps_ptr = &(g_pps_array_ptr[pps_id]);
	DEC_SPS_T *sps_ptr = &(g_sps_array_ptr[pps_ptr->seq_parameter_set_id]);

	if (sps_ptr->valid && pps_ptr->valid)
	{
		H264Dec_active_sps (sps_ptr, img_ptr);
#if _H264_PROTECT_ & _LEVEL_LOW_
		if(g_image_ptr->error_flag)
		{
			img_ptr->return_pos1 |= (1<<15);
			return;
		}
#endif
		H264Dec_active_pps (pps_ptr);
	}else
	{
#if _H264_PROTECT_ & _LEVEL_LOW_
		img_ptr->error_flag |= ER_BSM_ID;
		img_ptr->return_pos1 |= (1<<16);
#endif		
		g_ready_to_decode_slice = FALSE;
	}

	if (!img_ptr->is_cabac) //UVLC
	{
		nal_startcode_follows = uvlc_startcode_follows;
		readRefFrame = H264Dec_get_te;
		decode_mvd_xy = decode_cavlc_mb_mvd;
		read_mb_type = decode_cavlc_mb_type;
		read_b8mode = readB8_typeInfo_cavlc;
		if (img_ptr->VSP_used)
		{
		#if 0
			sw_vld_mb= decode_mb_cavlc_hw;
		#endif
		}else
		{
			sw_vld_mb= decode_mb_cavlc_sw;		
		}
	}else
	{
		nal_startcode_follows = get_cabac_terminate;
		readRefFrame = decode_cabac_mb_ref;
		decode_mvd_xy = decode_cabac_mb_mvd;
		read_mb_type = decode_cabac_mb_type;
		read_b8mode = decode_cabac_mb_sub_type;
		if (img_ptr->VSP_used)
		{
		#if 0
			sw_vld_mb= decode_mb_cabac_hw;	
        #endif        
		}else
		{
			sw_vld_mb= decode_mb_cabac_sw;	
		}
	}
	
	return;
}

PUBLIC void H264Dec_interpret_sei_message (void)
{
	//TBD
	return;
}

LOCAL int32 H264Dec_ReadHRDParameters(DEC_BS_T *stream, DEC_HRD_PARAM_T *hrd_ptr)
{	
	uint32 SchedSelIdx;

  	hrd_ptr->cpb_cnt_minus1 = READ_UE_V (stream);
  	hrd_ptr->bit_rate_scale = READ_FLC (stream, 4);
  	hrd_ptr->cpb_size_scale = READ_FLC (stream, 4);

  	for( SchedSelIdx = 0; SchedSelIdx <= hrd_ptr->cpb_cnt_minus1; SchedSelIdx++ )
  	{
    		hrd_ptr->bit_rate_value_minus1[ SchedSelIdx ]  = READ_UE_V (stream);
    		hrd_ptr->cpb_size_value_minus1[ SchedSelIdx ] = READ_UE_V (stream);
    		hrd_ptr->cbr_flag[ SchedSelIdx ]                         = READ_BITS1 (stream);
  	}

  	hrd_ptr->initial_cpb_removal_delay_length_minus1  = READ_FLC (stream, 5);
  	hrd_ptr->cpb_removal_delay_length_minus1            = READ_FLC (stream, 5);
 	hrd_ptr->dpb_output_delay_length_minus1              = READ_FLC (stream, 5);
  	hrd_ptr->time_offset_length                                 = READ_FLC (stream,5);

  	return 0;
}

LOCAL void H264Dec_ReadVUI (DEC_VUI_T *vui_seq_parameters_ptr, DEC_BS_T *stream)
{	
    	vui_seq_parameters_ptr->aspect_ratio_info_present_flag = READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->aspect_ratio_info_present_flag)
    	{
      		vui_seq_parameters_ptr->aspect_ratio_idc = READ_FLC  (stream, 8);
      		if (255 == vui_seq_parameters_ptr->aspect_ratio_idc)
      		{
        		vui_seq_parameters_ptr->sar_width = READ_FLC  (stream,16);
        		vui_seq_parameters_ptr->sar_height = READ_FLC  (stream,16);
      		}
  	}

    	vui_seq_parameters_ptr->overscan_info_present_flag     = READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->overscan_info_present_flag)
    	{
      		vui_seq_parameters_ptr->overscan_appropriate_flag    = READ_BITS1  (stream);
    	}

    	vui_seq_parameters_ptr->video_signal_type_present_flag = READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->video_signal_type_present_flag)
    	{
      		vui_seq_parameters_ptr->video_format                    = READ_FLC  (stream,3);
      		vui_seq_parameters_ptr->video_full_range_flag           = READ_BITS1  (stream);
      		vui_seq_parameters_ptr->colour_description_present_flag = READ_BITS1  (stream);
      		if(vui_seq_parameters_ptr->colour_description_present_flag)
      		{
        		vui_seq_parameters_ptr->colour_primaries              = READ_FLC  (stream,8);
        		vui_seq_parameters_ptr->transfer_characteristics      = READ_FLC  (stream,8);
        		vui_seq_parameters_ptr->matrix_coefficients           = READ_FLC  (stream,8);
      		}
    	}
		
    	vui_seq_parameters_ptr->chroma_location_info_present_flag = READ_BITS1  (stream);
    	if(vui_seq_parameters_ptr->chroma_location_info_present_flag)
    	{
      		vui_seq_parameters_ptr->chroma_sample_loc_type_top_field     = READ_UE_V  (stream);
      		vui_seq_parameters_ptr->chroma_sample_loc_type_bottom_field  = READ_UE_V  (stream);
    	}
		
    	vui_seq_parameters_ptr->timing_info_present_flag          = READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->timing_info_present_flag)
    	{
      		vui_seq_parameters_ptr->num_units_in_tick               = READ_FLC  (stream, 32);
      		vui_seq_parameters_ptr->time_scale                      = READ_FLC  (stream, 32);
      		vui_seq_parameters_ptr->fixed_frame_rate_flag           = READ_BITS1  (stream);
    	}
		
    	vui_seq_parameters_ptr->nal_hrd_parameters_present_flag   = READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->nal_hrd_parameters_present_flag)
    	{
      		H264Dec_ReadHRDParameters (stream, &(vui_seq_parameters_ptr->nal_hrd_parameters));
    	}
		
    	vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag   = READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag)
    	{
      		H264Dec_ReadHRDParameters (stream, &(vui_seq_parameters_ptr->vcl_hrd_parameters));
    	}
		
    	if (vui_seq_parameters_ptr->nal_hrd_parameters_present_flag || vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag)
    	{
      		vui_seq_parameters_ptr->low_delay_hrd_flag             =  READ_BITS1  (stream);
    	}
		
    	vui_seq_parameters_ptr->pic_struct_present_flag          =  READ_BITS1  (stream);
    	vui_seq_parameters_ptr->bitstream_restriction_flag       =  READ_BITS1  (stream);
    	if (vui_seq_parameters_ptr->bitstream_restriction_flag)
    	{
      		vui_seq_parameters_ptr->motion_vectors_over_pic_boundaries_flag =  READ_BITS1  (stream);
      		vui_seq_parameters_ptr->max_bytes_per_pic_denom                 =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->max_bits_per_mb_denom                   =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->log2_max_mv_length_horizontal           =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->log2_max_mv_length_vertical             =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->num_reorder_frames                      =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->max_dec_frame_buffering                 =  READ_UE_V (stream);
    	}

  	return;	
}

LOCAL void decode_scaling_list(DEC_BS_T *stream, uint8 *factors, int size,
                                const uint8 *jvt_list, const uint8 *fallback_list){
    int i, last = 8, next = 8;
    const uint8 *scan = size == 16 ? g_inverse_zigzag_tbl : g_inverse_8x8_zigzag_tbl;
    if(!READ_FLC(stream, 1)) /* matrix not written, we use the predicted one */
        memcpy(factors, fallback_list, size*sizeof(uint8));
    else
    for(i=0;i<size;i++){
        if(next)
            next = (last +  READ_SE_V (stream)) & 0xff;
        if(!i && !next){ /* matrix not written, we use the preset one */
            memcpy(factors, jvt_list, size*sizeof(uint8));
            break;
        }
        last = factors[scan[i]] = next ? next : last;
    }
}

LOCAL void decode_scaling_matrices(DEC_BS_T *stream, DEC_SPS_T *sps, DEC_PPS_T *pps, int is_sps,
                                   uint8 (*scaling_matrix4)[16], uint8 (*scaling_matrix8)[64]){
    int fallback_sps = !is_sps && sps->seq_scaling_matrix_present_flag;
    const uint8 *fallback[4] = {
        fallback_sps ? sps->ScalingList4x4[0] : weightscale4x4_intra_default,
        fallback_sps ? sps->ScalingList4x4[3] : weightscale4x4_inter_default,
        fallback_sps ? sps->ScalingList8x8[0] : weightscale8x8_intra_default,
        fallback_sps ? sps->ScalingList8x8[1] : weightscale8x8_inter_default
    };

       decode_scaling_list(stream,scaling_matrix4[0],16,weightscale4x4_intra_default,fallback[0]); // Intra, Y
       decode_scaling_list(stream,scaling_matrix4[1],16,weightscale4x4_intra_default,scaling_matrix4[0]); // Intra, Cr
       decode_scaling_list(stream,scaling_matrix4[2],16,weightscale4x4_intra_default,scaling_matrix4[1]); // Intra, Cb
       decode_scaling_list(stream,scaling_matrix4[3],16,weightscale4x4_inter_default,fallback[1]); // Inter, Y
       decode_scaling_list(stream,scaling_matrix4[4],16,weightscale4x4_inter_default,scaling_matrix4[3]); // Inter, Cr
       decode_scaling_list(stream,scaling_matrix4[5],16,weightscale4x4_inter_default,scaling_matrix4[4]); // Inter, Cb
       if(is_sps || pps->transform_8x8_mode_flag){
           decode_scaling_list(stream,scaling_matrix8[0],64,weightscale8x8_intra_default,fallback[2]);  // Intra, Y
           decode_scaling_list(stream,scaling_matrix8[1],64,weightscale8x8_inter_default,fallback[3]);  // Inter, Y
       }
}

LOCAL MMDecRet H264Dec_interpret_sps (DEC_SPS_T *sps_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 reserved_zero;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	sps_ptr->profile_idc = READ_FLC(stream, 8);

	sps_ptr->constrained_set0_flag = READ_BITS1(stream);
	sps_ptr->constrained_set1_flag = READ_BITS1(stream);
	sps_ptr->constrained_set2_flag = READ_BITS1(stream);
	sps_ptr->constrained_set3_flag = READ_BITS1(stream);

	reserved_zero = READ_FLC(stream, 4);

	sps_ptr->level_idc = READ_FLC(stream, 8);
	sps_ptr->seq_parameter_set_id = READ_UE_V(stream);

	if (sps_ptr->profile_idc == 0x64) //hp
	{
		sps_ptr->chroma_format_idc = READ_UE_V(stream);
		if ((sps_ptr->chroma_format_idc != 1))
		{
			g_image_ptr->error_flag |= ER_BSM_ID;
		}

		sps_ptr->bit_depth_luma_minus8 = READ_UE_V(stream);
		sps_ptr->bit_depth_chroma_minus8 = READ_UE_V(stream);
		sps_ptr->qpprime_y_zero_transform_bypass_flag = READ_FLC(stream, 1);
		sps_ptr->seq_scaling_matrix_present_flag =   READ_BITS1(stream);
		if(sps_ptr->seq_scaling_matrix_present_flag)
		{
			decode_scaling_matrices(stream, sps_ptr, NULL, 1, sps_ptr->ScalingList4x4,sps_ptr->ScalingList8x8);		
		}
	}
	
	sps_ptr->log2_max_frame_num_minus4 = READ_UE_V(stream);
	sps_ptr->pic_order_cnt_type = READ_UE_V(stream);
	
#if _H264_PROTECT_ & _LEVEL_HIGH_
	if (sps_ptr->seq_parameter_set_id < 0 || sps_ptr->log2_max_frame_num_minus4 < 0 ||
		sps_ptr->log2_max_frame_num_minus4 > 12 || sps_ptr->pic_order_cnt_type < 0 ||
		sps_ptr->pic_order_cnt_type > 2)
	{
		img_ptr->error_flag |= ER_BSM_ID;
		img_ptr->return_pos1 |= (1<<18);
		return MMDEC_STREAM_ERROR;
	}
#endif

	if (sps_ptr->pic_order_cnt_type == 0)
	{
		sps_ptr->log2_max_pic_order_cnt_lsb_minus4 = READ_UE_V(stream);
	}else if (sps_ptr->pic_order_cnt_type == 1)
	{
		int32 i;

		sps_ptr->delta_pic_order_always_zero_flag = READ_BITS1(stream);
		sps_ptr->offset_for_non_ref_pic = H264Dec_Long_SEV(stream);
		sps_ptr->offset_for_top_to_bottom_field = H264Dec_Long_SEV(stream);
		sps_ptr->num_ref_frames_in_pic_order_cnt_cycle = READ_UE_V(stream);

#if _H264_PROTECT_ & _LEVEL_LOW_
		if (sps_ptr->num_ref_frames_in_pic_order_cnt_cycle > 255)
		{
			img_ptr->error_flag |= ER_BSM_ID;
			img_ptr->return_pos1 |= (1<<19);
			return MMDEC_STREAM_ERROR;
		}
#endif
		for (i = 0; i < (int32)(sps_ptr->num_ref_frames_in_pic_order_cnt_cycle); i++)
		{
			sps_ptr->offset_for_ref_frame[i] = H264Dec_Long_SEV(stream);
		}
	}

	sps_ptr->num_ref_frames = READ_UE_V(stream);	
#if _H264_PROTECT_ & _LEVEL_LOW_	
	if (sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER)
	{
		img_ptr->error_flag |= ER_REF_FRM_ID;
        	img_ptr->return_pos1 |= (1<<20);
		PRINTF ("sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER");
	}
#endif	

	sps_ptr->gaps_in_frame_num_value_allowed_flag = READ_BITS1(stream);
	sps_ptr->pic_width_in_mbs_minus1 = READ_UE_V(stream);
	sps_ptr->pic_height_in_map_units_minus1 = READ_UE_V(stream);

    if ((sps_ptr->profile_idc != 0x42) && (sps_ptr->profile_idc != 0x4d) && (sps_ptr->profile_idc != 0x64))//0x42: bp, 0x4d: mp, 0x64: hp
    {
    	img_ptr->not_supported = TRUE;
        img_ptr->error_flag |= ER_BSM_ID;
        img_ptr->return_pos1 |= (1<<17);
	    SCI_TRACE_LOW("MMDEC_STREAM_ERROR");
        return MMDEC_STREAM_ERROR;
    }else
#if 0    
//    if (sps_ptr->profile_idc == 0x64/*hp*/ /*|| sps_ptr->profile_idc == 0x4d*//*mp*/
//		|| sps_ptr->pic_width_in_mbs_minus1 > 79 || sps_ptr->pic_height_in_map_units_minus1 > 44)	/*D1*/
#endif		
    {
        img_ptr->VSP_used = 0;

        img_ptr->b8_mv_pred_func[4] = H264Dec_mv_prediction_P8x8_8x8_sw;
        img_ptr->b8_mv_pred_func[5] = H264Dec_mv_prediction_P8x8_8x4_sw;
        img_ptr->b8_mv_pred_func[6] = H264Dec_mv_prediction_P8x8_4x8_sw;
        img_ptr->b8_mv_pred_func[7] = H264Dec_mv_prediction_P8x8_4x4_sw;

        H264Dec_setClipTab ();
        H264Dec_init_mc_function ();
        H264Dec_init_intra4x4Pred_function ();
        H264Dec_init_intra8x8Pred_function();
        H264Dec_init_intra16x16Pred_function ();
        H264Dec_init_intraChromaPred_function ();
    }
#if 0
    else {
		img_ptr->VSP_used = 1;

		img_ptr->b8_mv_pred_func[4] = H264Dec_mv_prediction_P8x8_8x8_hw;
		img_ptr->b8_mv_pred_func[5] = H264Dec_mv_prediction_P8x8_8x4_hw;
		img_ptr->b8_mv_pred_func[6] = H264Dec_mv_prediction_P8x8_4x8_hw;
		img_ptr->b8_mv_pred_func[7] = H264Dec_mv_prediction_P8x8_4x4_hw;
		
#if _H264_PROTECT_ & _LEVEL_HIGH_
	if ((sps_ptr->pic_width_in_mbs_minus1 < 3 || sps_ptr->pic_height_in_map_units_minus1 < 3) /*64x64*/
/*	||(sps_ptr->pic_width_in_mbs_minus1 > 44 || sps_ptr->pic_height_in_map_units_minus1 >  35)*/ /*768x576*/) 
	{
		img_ptr->error_flag |= ER_BSM_ID;
		img_ptr->return_pos1 |= (1<<21);
		return MMDEC_STREAM_ERROR;
	}
#endif	

	}
#endif

	sps_ptr->frame_mbs_only_flag = READ_BITS1(stream);

#if _H264_PROTECT_ & _LEVEL_LOW_
	if (!sps_ptr->frame_mbs_only_flag)
	{
		img_ptr->error_flag |= ER_BSM_ID;
        	img_ptr->return_pos1 |= (1<<22); 
		sps_ptr->mb_adaptive_frame_field_flag = READ_BITS1(stream);
		if (sps_ptr->mb_adaptive_frame_field_flag)
		{
			PRINTF("MBAFF is not supported!\n");
		}
		return MMDEC_STREAM_ERROR;
	}
#endif	

	sps_ptr->direct_8x8_inference_flag = READ_BITS1(stream);
	sps_ptr->frame_cropping_flag = READ_BITS1(stream);

	if (sps_ptr->frame_cropping_flag == 1)
	{
	//	uint32 frame_crop_left_offset;
	//	uint32 frame_crop_right_offset;
	//	uint32 frame_crop_top_offset;
	//	uint32 frame_crop_bottom_offset;

		sps_ptr->frame_crop_left_offset = READ_UE_V(stream);
		sps_ptr->frame_crop_right_offset = READ_UE_V(stream);
		sps_ptr->frame_crop_top_offset = READ_UE_V(stream);
		sps_ptr->frame_crop_bottom_offset = READ_UE_V(stream);	
	}else
	{
		sps_ptr->frame_crop_left_offset = 0;
		sps_ptr->frame_crop_right_offset = 0;
		sps_ptr->frame_crop_top_offset = 0;
		sps_ptr->frame_crop_bottom_offset = 0;	
	}

	sps_ptr->vui_parameters_present_flag = READ_BITS1(stream);

	if (sps_ptr->vui_parameters_present_flag)
	{
		H264Dec_ReadVUI (sps_ptr->vui_seq_parameters, stream);
	}

	sps_ptr->valid = TRUE;

	return MMDEC_OK;
}

LOCAL void H264Dec_make_sps_availabe (int32 sps_id, DEC_SPS_T *sps_ptr)
{
	int32 *src_ptr = (int32 *)sps_ptr;
	int32 *dst_ptr = (int32 *)(&g_sps_array_ptr[sps_id]);
	uint32 sps_size = sizeof(DEC_SPS_T)/4;
#if 0
	int32 i;
	for (i = 0; i < sps_size; i++)
	{
		*dst_ptr++ = *src_ptr++;
	}
#else
	memcpy(dst_ptr, src_ptr, sizeof(DEC_SPS_T));
#endif

	return;
}

PUBLIC MMDecRet H264Dec_process_sps (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SPS_T *sps_ptr = g_sps_ptr;
	MMDecRet ret;

	memset(sps_ptr, 0, sizeof(DEC_SPS_T)-(6*16+2*64+4));
	memset(sps_ptr->ScalingList4x4,16,6*16);
	memset(sps_ptr->ScalingList8x8,16,2*64);

	ret = H264Dec_interpret_sps (sps_ptr, img_ptr);
	if (ret != MMDEC_OK)
	{
		return ret;
	}
	
	H264Dec_make_sps_availabe (sps_ptr->seq_parameter_set_id, sps_ptr);

	if (g_active_sps_ptr && (sps_ptr->seq_parameter_set_id == g_active_sps_ptr->seq_parameter_set_id))
	{
		g_old_pps_id = -1;
	}

	img_ptr->profile_idc = sps_ptr->profile_idc;
	img_ptr->low_delay = 1;
	img_ptr->has_b_frames = !img_ptr->low_delay;

	return MMDEC_OK;
}

LOCAL MMDecRet H264Dec_interpret_pps (DEC_PPS_T *pps_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 i;
	int32 NumberBitsPerSliceGroupId;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	
	pps_ptr->pic_parameter_set_id = READ_UE_V(stream);
	pps_ptr->seq_parameter_set_id = READ_UE_V(stream);
	pps_ptr->entropy_coding_mode_flag = img_ptr->is_cabac = READ_BITS1(stream);
	pps_ptr->pic_order_present_flag = READ_BITS1(stream);
	pps_ptr->num_slice_groups_minus1 = READ_UE_V(stream);

#if _H264_PROTECT_ & _LEVEL_LOW_
	if (pps_ptr->num_slice_groups_minus1 >MAX_NUM_SLICE_GROUPS_MINUS1)
	{
		img_ptr->error_flag |= ER_BSM_ID;
		img_ptr->return_pos1 |= (1<<23);
		return MMDEC_STREAM_ERROR;
	}
#endif	

	//fmo parsing
	if (pps_ptr->num_slice_groups_minus1 > 0)
	{
		img_ptr->fmo_used = TRUE;
		
		PRINTF ("FMO used!\n");
		pps_ptr->slice_group_map_type	= (int8)READ_UE_V(stream);

		if (pps_ptr->slice_group_map_type == 6)
		{
			SCI_ASSERT(NULL != (pps_ptr->slice_group_id = (uint8 *)H264Dec_InterMemAlloc(SIZE_SLICE_GROUP_ID*sizeof(uint8), 4)));
		}

		if (pps_ptr->slice_group_map_type == 0)
		{
			for (i = 0; i <= (int32)pps_ptr->num_slice_groups_minus1; i++)
			{
				pps_ptr->run_length_minus1[i] = (uint16)READ_UE_V(stream);
			}
		}else if (pps_ptr->slice_group_map_type == 2)
		{
			for (i = 0; i < pps_ptr->num_slice_groups_minus1; i++)
			{
				pps_ptr->top_left[i] = READ_UE_V(stream);
				pps_ptr->bottom_right[i] =  READ_UE_V(stream);
			}
		}else if ((pps_ptr->slice_group_map_type == 3) || (pps_ptr->slice_group_map_type == 4) || 
			(pps_ptr->slice_group_map_type == 5))
		{
			pps_ptr->slice_group_change_direction_flag = READ_BITS1(stream);
			pps_ptr->slice_group_change_rate_minus1 = READ_UE_V(stream);
		}else if (pps_ptr->slice_group_map_type == 6)
		{
			if ((pps_ptr->num_slice_groups_minus1+1) > 4)
			{
				NumberBitsPerSliceGroupId = 3;
			}else if ((pps_ptr->num_slice_groups_minus1+1) > 2)
			{
				NumberBitsPerSliceGroupId = 2;
			}else
			{
				NumberBitsPerSliceGroupId = 1;
			}
			//! JVT-F078, exlicitly signal number of MBs in the map
			pps_ptr->num_slice_group_map_units_minus1 = READ_UE_V(stream);

			for (i = 0; i <= (int32)pps_ptr->num_slice_group_map_units_minus1; i++)
			{
				pps_ptr->slice_group_id[i] = READ_FLC(stream, NumberBitsPerSliceGroupId);
			}
		}
	}else
	{
		img_ptr->fmo_used = FALSE; //FALSE;
	}

	//ONLY FOR FMO COMFORMANCE TEST
//	g_image_ptr->fmo_used = TRUE; //FALSE;
	pps_ptr->num_ref_idx_l0_active_minus1 = READ_UE_V(stream);

#if _H264_PROTECT_ & _LEVEL_LOW_
	if ((pps_ptr->num_ref_idx_l0_active_minus1+1) > MAX_REF_FRAME_NUMBER)
	{
		img_ptr->error_flag |= ER_REF_FRM_ID;
        	img_ptr->return_pos1 |= (1<<24);     //lint !e648
		PRINTF ("too many l0_active not supported!\n");
		return MMDEC_STREAM_ERROR;
	}
#endif	

	pps_ptr->num_ref_idx_l1_active_minus1 = READ_UE_V(stream);
	pps_ptr->weighted_pred_flag = READ_BITS1(stream);
	pps_ptr->weighted_bipred_idc = READ_FLC(stream, 2);
	pps_ptr->pic_init_qp_minus26 = READ_SE_V(stream);
	pps_ptr->pic_init_qs_minus26 = READ_SE_V(stream);
	pps_ptr->chroma_qp_index_offset = READ_SE_V(stream);

	pps_ptr->deblocking_filter_control_present_flag = READ_BITS1(stream);
	pps_ptr->constrained_intra_pred_flag = READ_BITS1(stream);
	pps_ptr->redundant_pic_cnt_present_flag = READ_BITS1(stream);

	if (!uvlc_startcode_follows(img_ptr))
	{
		pps_ptr->transform_8x8_mode_flag = READ_BITS1(stream);
		pps_ptr->pic_scaling_matrix_present_flag = READ_BITS1(stream);
		if(pps_ptr->pic_scaling_matrix_present_flag)
		{
        		decode_scaling_matrices(stream, &g_sps_array_ptr[pps_ptr->seq_parameter_set_id], pps_ptr, 0, pps_ptr->ScalingList4x4, pps_ptr->ScalingList8x8);
		}

		pps_ptr->second_chroma_qp_index_offset = READ_SE_V(stream);
	}else
	{
		pps_ptr->second_chroma_qp_index_offset = pps_ptr->chroma_qp_index_offset;
	}

	pps_ptr->valid = TRUE;

	return MMDEC_OK;
}

LOCAL void H264Dec_make_pps_available (int32 pps_id, DEC_PPS_T *pps_ptr)
{
	int32 *src_ptr = (int32 *)pps_ptr;
	int32 *dst_ptr = (int32 *)(&g_pps_array_ptr[pps_id]);
	uint32 pps_size = sizeof(DEC_PPS_T)/4;
#if 0
	uint32 i;
	for (i = 0; i < pps_size; i++)
	{
		*dst_ptr++ = *src_ptr++;
	}
#else
	memcpy (dst_ptr, src_ptr, sizeof(DEC_PPS_T));
#endif

	return;
}

PUBLIC MMDecRet H264Dec_process_pps (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_PPS_T *pps_ptr = g_pps_ptr;
	MMDecRet ret;

	memset(g_pps_ptr, 0, sizeof(DEC_PPS_T)-4);
	memset(g_pps_ptr->ScalingList4x4,16,6*16);
	memset(g_pps_ptr->ScalingList8x8,16,2*64);
	
	ret = H264Dec_interpret_pps (pps_ptr, img_ptr);
	if (ret != MMDEC_OK)
	{
		return ret;
	}
	H264Dec_make_pps_available (pps_ptr->pic_parameter_set_id, pps_ptr);

	if (g_active_pps_ptr && (pps_ptr->pic_parameter_set_id == g_active_pps_ptr->pic_parameter_set_id))
	{
		g_old_pps_id = -1;
	}

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
