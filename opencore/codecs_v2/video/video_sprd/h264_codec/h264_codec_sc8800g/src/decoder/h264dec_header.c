/******************************************************************************
 ** File Name:    h264dec_header.c                                            *
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

#define PRINTF_HEAD_INFO	//printf

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader(uint8 *bitstrm_ptr, uint32 bitstrm_len)
{
	uint32 tmpVar;
	int32 ret = 0;
	DEC_NALU_T	*nal_ptr = g_nalu_ptr;

	H264Dec_InitBitstream(bitstrm_ptr, bitstrm_len);

	READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
		"polling bsm fifo depth >= 8 words for h264 header");

	tmpVar = READ_FLC(8);

	nal_ptr->nal_unit_type = tmpVar & 0x1f;
	nal_ptr->nal_reference_idc = ((tmpVar>>5)&0x3);
	nal_ptr->frobidden_bit = ((tmpVar>>7)&0x1);
	
	/*jump to corresponding NALU type decode*/
	g_ready_to_decode_slice = FALSE;
	switch(nal_ptr->nal_unit_type)
	{
	case NALU_TYPE_IDR:
		g_ready_to_decode_slice = TRUE;
		ret = H264Dec_Process_slice (nal_ptr);

		if ((g_curr_slice_ptr->start_mb_nr == 0) && (!g_image_ptr->error_flag))
		{
			g_searching_IDR_pic = FALSE;
		}
		break;
	case NALU_TYPE_SLICE:
		if (!g_searching_IDR_pic)
		{
			g_ready_to_decode_slice = TRUE;
			ret = H264Dec_Process_slice (nal_ptr);
		}
		break;
	case NALU_TYPE_PPS:
		H264Dec_ProcessPPS ();
		break;
	case NALU_TYPE_SPS:
		H264Dec_ProcessSPS ();
		break;
	case NALU_TYPE_SEI:
		H264Dec_InterpretSEIMessage ();
		break;
	default:
		PRINTF ("nalu type error!\n");
	}

	return ret;
}

LOCAL void H264Dec_dec_ref_pic_marking (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 val;
	DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;

	if (img_ptr->idr_flag)
	{
		img_ptr->no_output_of_prior_pics_flag = READ_FLC(1);
		img_ptr->long_term_reference_flag = READ_FLC(1);
	}else
	{
		img_ptr->adaptive_ref_pic_buffering_flag = READ_FLC(1);

		if (img_ptr->adaptive_ref_pic_buffering_flag)
		{
			//read memory management control operation
			g_dec_ref_pic_marking_buffer_size = 0;

			do 
			{
				tmp_drpm_ptr = g_dec_ref_pic_marking_buffer+g_dec_ref_pic_marking_buffer_size;
				g_dec_ref_pic_marking_buffer_size++;
				
				val = tmp_drpm_ptr->memory_management_control_operation = READ_UE_V();

				if ((val==1) || (val==3))
				{
					tmp_drpm_ptr->difference_of_pic_nums_minus1 = READ_UE_V();
				}

				if (val ==2)
				{
					tmp_drpm_ptr->long_term_pic_num = READ_UE_V();
				}

				if ((val==3)||(val==6))
				{
					tmp_drpm_ptr->long_term_frame_idx = READ_UE_V();
				}
				if (val == 4)
				{
					tmp_drpm_ptr->max_long_term_frame_idx_plus1 = READ_UE_V();
				}
			} while(val != 0);
		}
	}

	return;
}

PUBLIC void H264Dec_FirstPartOfSliceHeader (DEC_SLICE_T *curr_slice_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 tmp;

	curr_slice_ptr->start_mb_nr = READ_UE_V ();
	
	tmp = READ_UE_V ();

	if (tmp > 4)
	{
		tmp -= 5;
	}

	img_ptr->type = curr_slice_ptr->picture_type = tmp;

	curr_slice_ptr->pic_parameter_set_id = READ_UE_V ();
}

LOCAL void H264Dec_ref_pic_list_reordering (DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 tmp;
	int32 val;
	int32 i;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

	if (img_ptr->type != I_SLICE)
	{
		val = curr_slice_ptr->ref_pic_list_reordering_flag_l0 = READ_FLC(1);

		if (val)
		{
			i = 0;

			do 
			{
				READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
					"polling bsm fifo depth >= 8 words for h264 header");

				val = curr_slice_ptr->remapping_of_pic_nums_idc_l0[i] = READ_UE_V();
				
				if ((val == 0) || (val == 1))
				{
					tmp = SHOW_FLC(16);

					if (tmp == 0)
					{
						READ_FLC(16);
						curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = 0xffff;
						READ_FLC(17);
					}else
					{
						curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = READ_UE_V();
					}
				}else
				{
					if (val == 2)
					{
						tmp = SHOW_FLC(16);

						if (tmp == 0)
						{
							READ_FLC(16);
							curr_slice_ptr->long_term_pic_idx_l0[i] = 0xffff;
							READ_FLC(17);
						}else
						{
							curr_slice_ptr->long_term_pic_idx_l0[i] = READ_UE_V();
						}
					}
				}
				i++;

				if ((val > 3) || (val < 0))
				{
					PRINTF ("Invalid remapping_of_pic_nums_idc command");
					img_ptr->error_flag |= ER_BSM_ID;
                    img_ptr->return_pos |= (1<<12);
                    H264Dec_get_HW_status(img_ptr);
    				return;
				}
			} while(val != 3);
		}
	}

	return;
}

LOCAL uint32 CeilLog2 (uint32 val)
{
	uint32 tmp = val - 1;
	uint32 ret = 0;

	while (tmp != 0)
	{
		tmp >>= 1;
		ret++;
	}

	return ret;
}

PUBLIC void H264Dec_RestSliceHeader (DEC_IMAGE_PARAMS_T *img_ptr, DEC_SLICE_T *curr_slice_ptr)
{
	uint32 tmp;
	DEC_SPS_T *active_sps_ptr = g_active_sps_ptr;
	DEC_PPS_T *active_pps_ptr = g_active_pps_ptr;

	READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
		"polling bsm fifo depth >= 8 words for h264 header");

	img_ptr->frame_num = READ_FLC (active_sps_ptr->log2_max_frame_num_minus4+4);

	if (img_ptr->idr_flag)
	{
		img_ptr->pre_frame_num = 0;

		tmp = SHOW_FLC (16);

		if (tmp == 0)
		{
			READ_FLC (16);
			img_ptr->idr_pic_id = 0xffff;
			READ_FLC (17);
		}else
		{
			img_ptr->idr_pic_id = READ_UE_V ();
		}
	}

	if (active_sps_ptr->pic_order_cnt_type == 0)
	{
		img_ptr->pic_order_cnt_lsb = READ_FLC (active_sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4);

		if (active_pps_ptr->pic_order_present_flag)
		{
			img_ptr->delta_pic_order_cnt_bottom = H264Dec_Long_SEV ();
		}else
		{
			img_ptr->delta_pic_order_cnt_bottom = 0;
		}
	}

	if ((active_sps_ptr->pic_order_cnt_type == 1) && (!(active_sps_ptr->delta_pic_order_always_zero_flag)))
	{
		img_ptr->delta_pic_order_cnt[0] = H264Dec_Long_SEV ();

		if (active_pps_ptr->pic_order_present_flag)
		{
			img_ptr->delta_pic_order_cnt[1] = H264Dec_Long_SEV ();
		}
	}else
	{
		if (active_sps_ptr->pic_order_cnt_type == 1)
		{
			img_ptr->delta_pic_order_cnt[0] = 0;
			img_ptr->delta_pic_order_cnt[1] = 0;
		}
	}

	if (active_pps_ptr->redundant_pic_cnt_present_flag)
	{
		img_ptr->redundant_pic_cnt = READ_UE_V ();
	}

	img_ptr->num_ref_idx_l0_active = active_pps_ptr->num_ref_idx_l0_active_minus1+1;

	if (P_SLICE == img_ptr->type)
	{
		tmp = READ_FLC (1);

		if (tmp == 1)
		{
			img_ptr->num_ref_idx_l0_active = (int8)(1+READ_UE_V());
		}

		if (img_ptr->num_ref_idx_l0_active > MAX_REF_FRAME_NUMBER)
		{
			PRINTF ("too many l0_active not supported!\n");
		    img_ptr->error_flag = TRUE;
            img_ptr->return_pos2 |= (1<<7);
            H264Dec_get_HW_status(img_ptr);
            return;
		}
	}

	H264Dec_ref_pic_list_reordering(img_ptr);
    
    if(img_ptr->error_flag)
    {
        img_ptr->return_pos |= (1<<13);
        H264Dec_get_HW_status(img_ptr);
        return;
    }

	if (img_ptr->nal_reference_idc)
	{
		H264Dec_dec_ref_pic_marking (img_ptr);
	}

	tmp = READ_SE_V();
	curr_slice_ptr->slice_qp_delta = tmp;
	curr_slice_ptr->qp = img_ptr->qp = active_pps_ptr->pic_init_qp_minus26+26+(int8)tmp;

	if (active_pps_ptr->deblocking_filter_control_present_flag)
	{
		curr_slice_ptr->LFDisableIdc = READ_UE_V();

		if (curr_slice_ptr->LFDisableIdc != 1)
		{
			curr_slice_ptr->LFAlphaC0Offset = 2*READ_SE_V();
			curr_slice_ptr->LFBetaOffset = 2* READ_SE_V();
		}else
		{
			curr_slice_ptr->LFAlphaC0Offset = curr_slice_ptr->LFBetaOffset = 0;
		}
	}else
	{
		curr_slice_ptr->LFDisableIdc = curr_slice_ptr->LFAlphaC0Offset = curr_slice_ptr->LFBetaOffset = 0;
	}

	if ((active_pps_ptr->num_slice_groups_minus1 > 0) && (active_pps_ptr->slice_group_map_type >= 3) &&
		(active_pps_ptr->slice_group_map_type <= 5))
	{
		uint32 len;

		len = ((int32)active_sps_ptr->pic_height_in_map_units_minus1+1)*((int32)active_sps_ptr->pic_width_in_mbs_minus1+1)/
			((int32)active_pps_ptr->slice_group_change_rate_minus1+1);

		if ((((int32)active_sps_ptr->pic_height_in_map_units_minus1+1)*((int32)active_sps_ptr->pic_width_in_mbs_minus1+1))%
			((int32)active_pps_ptr->slice_group_change_rate_minus1+1))
		{
			len += 1;
		}

		len = CeilLog2(len+1);

		img_ptr->slice_group_change_cycle = READ_FLC(len);
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