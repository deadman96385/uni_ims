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
#include "sc8800g_video_header.h"
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
LOCAL void H264Dec_active_sps (DEC_SPS_T *sps_ptr)
{
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;

	if (g_active_sps_ptr != sps_ptr)
	{
		g_active_sps_ptr = sps_ptr;

		img_ptr->max_frame_num = (1<<(sps_ptr->log2_max_frame_num_minus4+4));
		img_ptr->frame_width_in_mbs = (sps_ptr->pic_width_in_mbs_minus1+1);
		img_ptr->pic_height_in_map_units = (sps_ptr->pic_height_in_map_units_minus1+1);
		img_ptr->frame_height_in_mbs = (2-sps_ptr->frame_mbs_only_flag)*(int8)img_ptr->pic_height_in_map_units;
		img_ptr->frame_size_in_mbs = img_ptr->frame_width_in_mbs*img_ptr->frame_height_in_mbs;
		img_ptr->width = img_ptr->frame_width_in_mbs * MB_SIZE;
		img_ptr->height = img_ptr->frame_height_in_mbs * MB_SIZE;
#ifdef _VSP_LINUX_
	        g_MbToSliceGroupMap = NULL;
		if(VSP_spsCb)
		{
			int ret = (*VSP_spsCb)(g_user_data,img_ptr->width,img_ptr->height,sps_ptr->num_ref_frames);
			if(!ret)
			{
				img_ptr->error_flag = TRUE;
				return;
			}
		}else{
			img_ptr->error_flag = TRUE;
			return;
		}
#endif

		H264Dec_SeqLevelConfig (img_ptr);

		//reset memory alloc
		H264Dec_FreeExtraMem();

		H264Dec_init_img_buffer (img_ptr);
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

PUBLIC void H264Dec_use_parameter_set (int32 pps_id)
{
	DEC_PPS_T *pps_ptr = &(g_pps_array_ptr[pps_id]);
	DEC_SPS_T *sps_ptr = &(g_sps_array_ptr[pps_ptr->seq_parameter_set_id]);

	H264Dec_active_sps (sps_ptr);
#ifdef _VSP_LINUX_
	if(g_image_ptr->error_flag)
		return;
#endif
	H264Dec_active_pps (pps_ptr);

	if (!(sps_ptr->valid && pps_ptr->valid))
	{
		g_ready_to_decode_slice = FALSE;
	}

	return;
}

PUBLIC void H264Dec_InterpretSEIMessage (void)
{
	//TBD
	return;
}

LOCAL void H264Dec_ReadVUI (DEC_SPS_T *sps_ptr)
{
	//TBD
	return;
}

LOCAL void H264Dec_interpret_sps (DEC_SPS_T *sps_ptr)
{
	int32 reserved_zero;

	sps_ptr->profile_idc = READ_FLC(8);
	if (sps_ptr->profile_idc != 0x42) //0x42: baseline profile
	{
		g_image_ptr->error_flag |= ER_BSM_ID;
        g_image_ptr->return_pos |= (1<<28);
        H264Dec_get_HW_status(g_image_ptr);        
	}
	sps_ptr->constrained_set0_flag = READ_FLC(1);
	sps_ptr->constrained_set1_flag = READ_FLC(1);
	sps_ptr->constrained_set2_flag = READ_FLC(1);
	sps_ptr->constrained_set3_flag = READ_FLC(1);

	reserved_zero = READ_FLC(4);

	sps_ptr->level_idc = READ_FLC(8);

	sps_ptr->seq_parameter_set_id = READ_UE_V();
	sps_ptr->log2_max_frame_num_minus4 = READ_UE_V();
	sps_ptr->pic_order_cnt_type = READ_UE_V();

	if (sps_ptr->pic_order_cnt_type == 0)
	{
		sps_ptr->log2_max_pic_order_cnt_lsb_minus4 = READ_UE_V();
	}else if (sps_ptr->pic_order_cnt_type == 1)
	{
		int32 i;

		sps_ptr->delta_pic_order_always_zero_flag = READ_FLC(1);
		sps_ptr->offset_for_non_ref_pic = H264Dec_Long_SEV();
		sps_ptr->offset_for_top_to_bottom_field = H264Dec_Long_SEV();
		sps_ptr->num_ref_frames_in_pic_order_cnt_cycle = READ_UE_V();

		for (i = 0; i < (int32)(sps_ptr->num_ref_frames_in_pic_order_cnt_cycle); i++)
		{
			sps_ptr->offset_for_ref_frame[i] = H264Dec_Long_SEV();
		}
	}

	sps_ptr->num_ref_frames = READ_UE_V();
	
	if (sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER)
	{
		g_image_ptr->error_flag |= ER_REF_FRM_ID;
        g_image_ptr->return_pos |= (1<<29);
        H264Dec_get_HW_status(g_image_ptr);        
		PRINTF ("sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER");
	}

	sps_ptr->gaps_in_frame_num_value_allowed_flag = READ_FLC(1);
	sps_ptr->pic_width_in_mbs_minus1 = READ_UE_V();
	sps_ptr->pic_height_in_map_units_minus1 = READ_UE_V();

	g_image_ptr->size_decode_flag = TRUE;

	sps_ptr->frame_mbs_only_flag = READ_FLC(1);

	if (!sps_ptr->frame_mbs_only_flag)
	{
		g_image_ptr->error_flag |= ER_BSM_ID;
        g_image_ptr->return_pos |= (1<<30); 
        H264Dec_get_HW_status(g_image_ptr);       
		PRINTF("field is not supported!\n");
	}

	sps_ptr->direct_8x8_reference_flag = READ_FLC(1);
	sps_ptr->frame_cropping_flag = READ_FLC(1);

	if (sps_ptr->frame_cropping_flag == 1)
	{
		int32 frame_crop_left_offset;
		int32 frame_crop_right_offset;
		int32 frame_crop_top_offset;
		int32 frame_crop_bottom_offset;

		frame_crop_left_offset = READ_UE_V();
		frame_crop_right_offset = READ_UE_V();
		frame_crop_top_offset = READ_UE_V();
		frame_crop_bottom_offset = READ_UE_V();	
	}

	sps_ptr->vui_parameters_present_flag = READ_FLC(1);

	if (sps_ptr->vui_parameters_present_flag)
	{
		H264Dec_ReadVUI (sps_ptr);
	}

	sps_ptr->valid = TRUE;

	return;
}

LOCAL void H264Dec_make_sps_availabe (int32 sps_id, DEC_SPS_T *sps_ptr)
{
	int8 *src_ptr = (int8 *)sps_ptr;
	int8 *dst_ptr = (int8 *)(&g_sps_array_ptr[sps_id]);
	int32 i;

	for (i = 0; i < (int32)sizeof(DEC_SPS_T); i++)
	{
		*dst_ptr++ = *src_ptr++;
	}

	return;
}

PUBLIC void H264Dec_ProcessSPS (void)
{
	H264Dec_interpret_sps (g_sps_ptr);
	H264Dec_make_sps_availabe (g_sps_ptr->seq_parameter_set_id, g_sps_ptr);

	if (g_active_sps_ptr && (g_sps_ptr->seq_parameter_set_id == g_active_sps_ptr->seq_parameter_set_id))
	{
		g_old_pps_id = -1;
	}

	g_image_ptr->profile_idc = g_sps_ptr->profile_idc;

	return;
}

LOCAL void H264Dec_interpret_pps (DEC_PPS_T *pps_ptr)
{
	int32 i;
	int32 NumberBitsPerSliceGroupId;
	
	pps_ptr->pic_parameter_set_id = READ_UE_V();
	pps_ptr->seq_parameter_set_id = READ_UE_V();
	pps_ptr->entropy_coding_mode_flag = READ_FLC(1);

    if (pps_ptr->entropy_coding_mode_flag) //=1, cabac
    {
        g_image_ptr->return_pos2 |= (1<<16);     //lint !e648
        H264Dec_get_HW_status(g_image_ptr);   
        return;
    }
    
	pps_ptr->pic_order_present_flag = READ_FLC(1);
	pps_ptr->num_slice_groups_minus1 = READ_UE_V();

	//fmo parsing
	if (pps_ptr->num_slice_groups_minus1 > 0)
	{
		g_image_ptr->fmo_used = TRUE;
		
		PRINTF ("FMO used!\n");
		pps_ptr->slice_group_map_type	= (int8)READ_UE_V();

		if (pps_ptr->slice_group_map_type == 0)
		{
			for (i = 0; i <= (int32)pps_ptr->num_slice_groups_minus1; i++)
			{
				pps_ptr->run_length_minus1[i] = (uint16)READ_UE_V();
			}
		}else if (pps_ptr->slice_group_map_type == 2)
		{
			for (i = 0; i < pps_ptr->num_slice_groups_minus1; i++)
			{
				pps_ptr->top_left[i] = READ_UE_V();
				pps_ptr->bottom_right[i] =  READ_UE_V();
			}
		}else if ((pps_ptr->slice_group_map_type == 3) || (pps_ptr->slice_group_map_type == 4) || 
			(pps_ptr->slice_group_map_type == 5))
		{
			pps_ptr->slice_group_change_direction_flag = READ_FLC(1);
			pps_ptr->slice_group_change_rate_minus1 = READ_UE_V();
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
			pps_ptr->num_slice_group_map_units_minus1 = READ_UE_V();

			for (i = 0; i <= (int32)pps_ptr->num_slice_group_map_units_minus1; i++)
			{
				pps_ptr->slice_group_id[i] = READ_FLC(NumberBitsPerSliceGroupId);
			}
		}
	}else
	{
		g_image_ptr->fmo_used = FALSE; //FALSE;
	}

	//ONLY FOR FMO COMFORMANCE TEST
 //	g_image_ptr->fmo_used = TRUE; //FALSE;

	pps_ptr->num_ref_idx_l0_active_minus1 = READ_UE_V();

	if ((pps_ptr->num_ref_idx_l0_active_minus1+1) > MAX_REF_FRAME_NUMBER)
	{
		g_image_ptr->error_flag |= ER_REF_FRM_ID;
        g_image_ptr->return_pos |= (1<<31);     //lint !e648
        H264Dec_get_HW_status(g_image_ptr);        
		PRINTF ("too many l0_active not supported!\n");
	}

	pps_ptr->num_ref_idx_l1_active_minus1 = READ_UE_V();
	pps_ptr->weighted_pred_flag = READ_FLC(1);
	pps_ptr->weighted_bipred_idc = READ_FLC(2);

    //added by xweiluo@20110518,
    if (pps_ptr->num_ref_idx_l1_active_minus1 || pps_ptr->weighted_pred_flag || pps_ptr->weighted_bipred_idc)
    {
        g_image_ptr->error_flag |= ER_BSM_ID;
        g_image_ptr->return_pos2 |= (1<<17);     //lint !e648
        H264Dec_get_HW_status(g_image_ptr);        
		PRINTF (" these three param must be zero in baseline bitstream!\n");
    }
    
	pps_ptr->pic_init_qp_minus26 = READ_SE_V();
	pps_ptr->pic_init_qs_minus26 = READ_SE_V();
	pps_ptr->chroma_qp_index_offset = READ_SE_V();

	pps_ptr->deblocking_filter_control_present_flag = READ_FLC(1);
	pps_ptr->constrained_intra_pred_flag = READ_FLC(1);
	pps_ptr->redundant_pic_cnt_present_flag = READ_FLC(1);

	pps_ptr->second_chroma_qp_index_offset = pps_ptr->chroma_qp_index_offset;

	pps_ptr->valid = TRUE;

	return;
}

LOCAL void H264Dec_make_pps_available (int32 pps_id, DEC_PPS_T *pps_ptr)
{
	int8 *src_ptr = (int8 *)pps_ptr;
	int8 *dst_ptr = (int8 *)(&g_pps_array_ptr[pps_id]);
	int32 i;

	for (i = 0; i < (int32)sizeof(DEC_PPS_T); i++)
	{
		*dst_ptr++ = *src_ptr++;
	}

	return;
}

PUBLIC void H264Dec_ProcessPPS (void)
{
	DEC_PPS_T *pps_ptr = g_pps_ptr;
	
//	SCI_ASSERT(NULL != (pps_ptr->slice_group_id = (uint8 *)H264Dec_InterMemAlloc(SIZE_SLICE_GROUP_ID*sizeof(uint8))));
	if ((pps_ptr->slice_group_map_type == 6) && (g_image_ptr->fmo_used))
	{
		//SCI_ASSERT(NULL != (pps_ptr->slice_group_id = (uint8 *)H264Dec_InterMemAlloc(SIZE_SLICE_GROUP_ID*sizeof(uint8))));
		pps_ptr->slice_group_id = (uint8 *)H264Dec_InterMemAlloc(SIZE_SLICE_GROUP_ID*sizeof(uint8));
	}

	H264Dec_interpret_pps (pps_ptr);
	H264Dec_make_pps_available (g_pps_ptr->pic_parameter_set_id, pps_ptr);

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