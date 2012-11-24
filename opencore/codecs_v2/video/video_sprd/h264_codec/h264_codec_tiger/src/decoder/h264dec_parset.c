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
#include "tiger_video_header.h"
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
	if (g_active_sps_ptr != sps_ptr)
	{
		g_active_sps_ptr = sps_ptr;

		img_ptr->max_frame_num = (1<<(sps_ptr->log2_max_frame_num_minus4+4));
		img_ptr->frame_width_in_mbs = (sps_ptr->pic_width_in_mbs_minus1+1);
		img_ptr->pic_height_in_map_units = (sps_ptr->pic_height_in_map_units_minus1+1);
		img_ptr->frame_height_in_mbs = (2-(uint8)sps_ptr->frame_mbs_only_flag)*img_ptr->pic_height_in_map_units;
		img_ptr->frame_size_in_mbs = img_ptr->frame_width_in_mbs*img_ptr->frame_height_in_mbs;
		img_ptr->width = img_ptr->frame_width_in_mbs * MB_SIZE;
		img_ptr->height = img_ptr->frame_height_in_mbs * MB_SIZE;	
		img_ptr->b4_pitch = (img_ptr->frame_width_in_mbs << 2);
#ifdef _VSP_LINUX_
//		SCI_TRACE_LOW("H264Dec_active_sps 2\n");

	    	g_MbToSliceGroupMap = NULL;

		if(VSP_spsCb)
		{
			int ret = (*VSP_spsCb)(g_user_data,img_ptr->width,img_ptr->height,sps_ptr->num_ref_frames);
			if(!ret)
			{
				img_ptr->error_flag = TRUE;
				return;
			}
		}else
		{
			img_ptr->error_flag = TRUE;
			return;
		}
#endif

		H264Dec_SeqLevelConfig (img_ptr);

		//reset memory alloc
		H264Dec_FreeExtraMem();

		H264Dec_init_img_buffer (img_ptr);
		
		if (!img_ptr->no_output_of_prior_pics_flag)
		{
			H264Dec_flush_dpb(g_dpb_ptr);
		}

		H264Dec_init_dpb (img_ptr);

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

	H264Dec_active_sps (sps_ptr, img_ptr);
#ifdef _VSP_LINUX_
	if(g_image_ptr->error_flag)
		return;
#endif
	H264Dec_active_pps (pps_ptr);

	if (!(sps_ptr->valid && pps_ptr->valid))
	{
		g_ready_to_decode_slice = FALSE;
	}

	if (!img_ptr->is_cabac) //UVLC
	{
		nal_startcode_follows = uvlc_startcode_follows;
		readRefFrame = H264Dec_get_te;
		readMVD_xy = readMVD_xy_cavlc;
		sw_vld_mb= decode_mb_cavlc;
		read_mb_type_ISlice = read_mb_type_ISlice_cavlc;
		read_mb_type_PBSlice = read_mb_type_PBSlice_cavlc;
		read_b8mode = readB8_typeInfo_cavlc;
	}else
	{
		nal_startcode_follows = biari_decode_final;
		readRefFrame = readRefFrame_CABAC;
		readMVD_xy = readMVD_xy_cabac;
		sw_vld_mb= decode_mb_cabac;	
		read_mb_type_ISlice = read_mb_type_ISlice_cabac;
		read_mb_type_PBSlice = read_mb_type_PBSlice_cabac;
		read_b8mode = readB8_typeInfo_CABAC;
	}
	
	return;
}

PUBLIC void H264Dec_interpret_sei_message (void)
{
	//TBD
	return;
}

LOCAL void H264Dec_ReadVUI (DEC_SPS_T *sps_ptr)
{
	//TBD
	return;
}

LOCAL void H264Dec_interpret_sps (DEC_SPS_T *sps_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 reserved_zero;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	sps_ptr->profile_idc = READ_FLC(stream, 8);
	if ((sps_ptr->profile_idc != 0x42) && (sps_ptr->profile_idc != 0x4d))//0x42: baseline profile, 0x4d: main profile
	{
		img_ptr->not_supported = TRUE;
		img_ptr->error_flag = TRUE;
	}
	sps_ptr->constrained_set0_flag = READ_BITS1(stream);
	sps_ptr->constrained_set1_flag = READ_BITS1(stream);
	sps_ptr->constrained_set2_flag = READ_BITS1(stream);
	sps_ptr->constrained_set3_flag = READ_BITS1(stream);

	reserved_zero = READ_FLC(stream, 4);

	sps_ptr->level_idc = READ_FLC(stream, 8);
	sps_ptr->seq_parameter_set_id = READ_UE_V(stream);
	sps_ptr->log2_max_frame_num_minus4 = READ_UE_V(stream);
	sps_ptr->pic_order_cnt_type = READ_UE_V(stream);

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

		for (i = 0; i < (int32)(sps_ptr->num_ref_frames_in_pic_order_cnt_cycle); i++)
		{
			sps_ptr->offset_for_ref_frame[i] = H264Dec_Long_SEV(stream);
		}
	}

	sps_ptr->num_ref_frames = READ_UE_V(stream);	
	if (sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER)
	{
		img_ptr->error_flag = TRUE;
		PRINTF ("sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER");
	}

	sps_ptr->gaps_in_frame_num_value_allowed_flag = READ_BITS1(stream);
	sps_ptr->pic_width_in_mbs_minus1 = READ_UE_V(stream);
	sps_ptr->pic_height_in_map_units_minus1 = READ_UE_V(stream);

	sps_ptr->frame_mbs_only_flag = READ_BITS1(stream);

	if (!sps_ptr->frame_mbs_only_flag)
	{
		img_ptr->error_flag = TRUE;
		sps_ptr->mb_adaptive_frame_field_flag = READ_BITS1(stream);
		if (sps_ptr->mb_adaptive_frame_field_flag)
		{
			PRINTF("MBAFF is not supported!\n");
		}
	}

	sps_ptr->direct_8x8_inference_flag = READ_BITS1(stream);
	sps_ptr->frame_cropping_flag = READ_BITS1(stream);

	if (sps_ptr->frame_cropping_flag == 1)
	{
		uint32 frame_crop_left_offset;
		uint32 frame_crop_right_offset;
		uint32 frame_crop_top_offset;
		uint32 frame_crop_bottom_offset;

		frame_crop_left_offset = READ_UE_V(stream);
		frame_crop_right_offset = READ_UE_V(stream);
		frame_crop_top_offset = READ_UE_V(stream);
		frame_crop_bottom_offset = READ_UE_V(stream);	
	}

	sps_ptr->vui_parameters_present_flag = READ_BITS1(stream);

	if (sps_ptr->vui_parameters_present_flag)
	{
		H264Dec_ReadVUI (sps_ptr);
	}

	sps_ptr->valid = TRUE;

	return;
}

LOCAL void H264Dec_make_sps_availabe (int32 sps_id, DEC_SPS_T *sps_ptr)
{
	int32 *src_ptr = (int32 *)sps_ptr;
	int32 *dst_ptr = (int32 *)(&g_sps_array_ptr[sps_id]);
	uint32 sps_size = sizeof(DEC_SPS_T)/4;
	int32 i;

	for (i = 0; i < sps_size; i++)
	{
		*dst_ptr++ = *src_ptr++;
	}

	return;
}

PUBLIC void H264Dec_process_sps (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_SPS_T *sps_ptr = g_sps_ptr;

	H264Dec_interpret_sps (sps_ptr, img_ptr);
	H264Dec_make_sps_availabe (sps_ptr->seq_parameter_set_id, sps_ptr);

	if (g_active_sps_ptr && (sps_ptr->seq_parameter_set_id == g_active_sps_ptr->seq_parameter_set_id))
	{
		g_old_pps_id = -1;
	}

	img_ptr->profile_idc = sps_ptr->profile_idc;

	return;
}

LOCAL void H264Dec_interpret_pps (DEC_PPS_T *pps_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 i;
	int32 NumberBitsPerSliceGroupId;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	
	pps_ptr->pic_parameter_set_id = READ_UE_V(stream);
	pps_ptr->seq_parameter_set_id = READ_UE_V(stream);
	pps_ptr->entropy_coding_mode_flag = img_ptr->is_cabac = READ_BITS1(stream);
	pps_ptr->pic_order_present_flag = READ_BITS1(stream);
	pps_ptr->num_slice_groups_minus1 = READ_UE_V(stream);

	//fmo parsing
	if (pps_ptr->num_slice_groups_minus1 > 0)
	{
		g_image_ptr->fmo_used = TRUE;
		
		PRINTF ("FMO used!\n");
		pps_ptr->slice_group_map_type	= (int8)READ_UE_V(stream);

		if (pps_ptr->slice_group_map_type == 6)
		{
			SCI_ASSERT(NULL != (pps_ptr->slice_group_id = (uint8 *)H264Dec_InterMemAlloc(SIZE_SLICE_GROUP_ID*sizeof(uint8))));
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

	if ((pps_ptr->num_ref_idx_l0_active_minus1+1) > MAX_REF_FRAME_NUMBER)
	{
		g_image_ptr->error_flag = TRUE;
		PRINTF ("too many l0_active not supported!\n");
	}

	pps_ptr->num_ref_idx_l1_active_minus1 = READ_UE_V(stream);
	pps_ptr->weighted_pred_flag = READ_BITS1(stream);
	pps_ptr->weighted_bipred_idc = READ_FLC(stream, 2);
	pps_ptr->pic_init_qp_minus26 = READ_SE_V(stream);
	pps_ptr->pic_init_qs_minus26 = READ_SE_V(stream);
	pps_ptr->chroma_qp_index_offset = READ_SE_V(stream);

	pps_ptr->deblocking_filter_control_present_flag = READ_BITS1(stream);
	pps_ptr->constrained_intra_pred_flag = READ_BITS1(stream);
	pps_ptr->redundant_pic_cnt_present_flag = READ_BITS1(stream);

	pps_ptr->second_chroma_qp_index_offset = pps_ptr->chroma_qp_index_offset;

	pps_ptr->valid = TRUE;

	return;
}

LOCAL void H264Dec_make_pps_available (int32 pps_id, DEC_PPS_T *pps_ptr)
{
	int32 *src_ptr = (int32 *)pps_ptr;
	int32 *dst_ptr = (int32 *)(&g_pps_array_ptr[pps_id]);
	uint32 pps_size = sizeof(DEC_PPS_T)/4;
	uint32 i;

	for (i = 0; i < pps_size; i++)
	{
		*dst_ptr++ = *src_ptr++;
	}

	return;
}

PUBLIC void H264Dec_process_pps (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_PPS_T *pps_ptr = g_pps_ptr;
	
	H264Dec_interpret_pps (pps_ptr, img_ptr);
	H264Dec_make_pps_available (pps_ptr->pic_parameter_set_id, pps_ptr);

	if (g_active_pps_ptr && (pps_ptr->pic_parameter_set_id == g_active_pps_ptr->pic_parameter_set_id))
	{
		g_old_pps_id = -1;
	}

	return;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
