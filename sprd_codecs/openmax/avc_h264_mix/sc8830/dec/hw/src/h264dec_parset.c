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
#include "sc8810_video_header.h"
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
#if _MVC_
typedef enum {
  FREXT_CAVLC444 = 44,       //!< YUV 4:4:4/14 "CAVLC 4:4:4"
  BASELINE       = 66,       //!< YUV 4:2:0/8  "Baseline"
  MAIN           = 77,       //!< YUV 4:2:0/8  "Main"
  EXTENDED       = 88,       //!< YUV 4:2:0/8  "Extended"
  FREXT_HP       = 100,      //!< YUV 4:2:0/8  "High"
  FREXT_Hi10P    = 110,      //!< YUV 4:2:0/10 "High 10"
  FREXT_Hi422    = 122,      //!< YUV 4:2:2/10 "High 4:2:2"
  FREXT_Hi444    = 244,      //!< YUV 4:4:4/14 "High 4:4:4"
  MVC_HIGH       = 118,      //!< YUV 4:2:0/8  "Multiview High"
  STEREO_HIGH    = 128       //!< YUV 4:2:0/8  "Stereo High"
} ProfileIDC;

//replace H264Dec_more_rbsp_data() function. same as !H264Dec_more_rbsp_data 
PUBLIC uint32 uvlc_startcode_follows (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 tmp;
	int32 has_no_zero = 0;
	int32 byte_offset;
	int32 bit_offset;	
	uint32 nDecTotalBits;
	OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");
	nDecTotalBits =OR1200_READ_REG(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF,"BSM flushed bits cnt");
	
	byte_offset = nDecTotalBits/8;
	bit_offset = nDecTotalBits&0x7;
	
	if (byte_offset < g_nalu_ptr->len -1)
	{
		return FALSE;
	}

	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
    OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, ((8-bit_offset)<<24),"BSM_rd n bits");
	tmp = SHOW_FLC(NULL, (8-bit_offset));
	if (tmp == (1<<(7-bit_offset)))
	{
		return TRUE;
	}else
	{
		return FALSE;
	}	
}



static int8 is_BL_profile(uint8 profile_idc) 
{
  return ( profile_idc == FREXT_CAVLC444 || profile_idc == BASELINE || profile_idc == MAIN || profile_idc == EXTENDED ||
           profile_idc == FREXT_HP || profile_idc == FREXT_Hi10P || profile_idc == FREXT_Hi422 || profile_idc == FREXT_Hi444);
}
static int8 is_MVC_profile(uint8 profile_idc)
{
  return ((profile_idc == MVC_HIGH) || (profile_idc == STEREO_HIGH));
}
#endif
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

//                if(!b_video_buffer_malloced && g_sps_ptr->pic_height_in_map_units_minus1/* && g_sps_ptr->profile_idc != 0x42*/)
                if (VSP_mallocCb)                               
                {
                    // Malloc direct mb info buffers
                	uint32 malloc_buffer_num;
                	uint32 malloc_buffer_size;

                	malloc_buffer_num = 17;
                	malloc_buffer_size =  ((g_sps_ptr->pic_height_in_map_units_minus1+1) * (g_sps_ptr->pic_width_in_mbs_minus1+1))  * 80;

                   	 ALOGE("%s, %d", __FUNCTION__, __LINE__);

                	VSP_mallocCb (g_user_data, direct_mb_info_addr, malloc_buffer_num, malloc_buffer_size);

//                	b_video_buffer_malloced = 1;
                }

		img_ptr->max_frame_num = (1<<(sps_ptr->log2_max_frame_num_minus4+4));
		img_ptr->frame_width_in_mbs = (sps_ptr->pic_width_in_mbs_minus1+1);
		img_ptr->pic_height_in_map_units = (sps_ptr->pic_height_in_map_units_minus1+1);
		img_ptr->frame_height_in_mbs = (2-(uint8)sps_ptr->frame_mbs_only_flag)*img_ptr->pic_height_in_map_units;
		img_ptr->frame_size_in_mbs = img_ptr->frame_width_in_mbs*img_ptr->frame_height_in_mbs;
		img_ptr->width = img_ptr->frame_width_in_mbs * MB_SIZE;
		img_ptr->height = img_ptr->frame_height_in_mbs * MB_SIZE;

		//H264Dec_SeqLevelConfig (img_ptr);// empty function

		//reset memory alloc
		H264Dec_FreeExtraMem();

		H264Dec_init_img_buffer (img_ptr);
#if _MVC_
		if ((last_profile_idc != g_active_sps_ptr->profile_idc 
		  && is_BL_profile(g_active_sps_ptr->profile_idc) 
		  && !g_dpb_layer[0]->init_done))
		{
			//init_global_buffers(p_Vid, 0); ??

			if (!img_ptr->no_output_of_prior_pics_flag)
			{
				H264Dec_flush_dpb(g_dpb_layer[0]);
				H264Dec_flush_dpb(g_dpb_layer[1]);
			}
			H264Dec_init_dpb(img_ptr, g_dpb_layer[0], 1);
			H264Dec_init_dpb(img_ptr, g_dpb_layer[1], 2);
		}
		else if( (last_profile_idc != g_active_sps_ptr->profile_idc)
			&& ((is_MVC_profile(last_profile_idc)) || (is_MVC_profile(g_active_sps_ptr->profile_idc)))
			&& (!g_dpb_layer[1]->init_done))
		{
			//assert(g_dpb_layer[0]->init_done);//for OR debug

			if(g_dpb_layer[0]->init_done)
			{
				//*free_dpb(p_Vid->p_Dpb_layer[0]);//james:TODO
				//H264Dec_init_dpb(img_ptr, g_dpb_layer[0], 1);
			}
			//init_global_buffers(p_Vid, 1); ??
			// for now lets re_init both buffers. Later, we should only re_init appropriate one
			// Note that we seem to be doing this for every frame which seems not good.

			//H264Dec_init_dpb(img_ptr, g_dpb_layer[1], 2);//james:initial at first time

		}
		last_pic_width_in_mbs_minus1 = g_active_sps_ptr->pic_width_in_mbs_minus1;  
		last_pic_height_in_map_units_minus1 = g_active_sps_ptr->pic_height_in_map_units_minus1;
		//*last_max_dec_frame_buffering = GetMaxDecFrameBuffering(p_Vid);
		last_profile_idc = g_active_sps_ptr->profile_idc;
		g_curr_slice_ptr->p_Dpb->num_ref_frames = g_active_sps_ptr->num_ref_frames;
#else	
		if (!img_ptr->no_output_of_prior_pics_flag)
		{
			H264Dec_flush_dpb();
		}
		
		H264Dec_init_dpb (img_ptr);
		g_dpb_ptr->num_ref_frames = g_active_sps_ptr->num_ref_frames;
#endif
		
	}

	return;
}

LOCAL void H264Dec_active_pps (DEC_PPS_T *pps_ptr)
{
	if (g_active_pps_ptr != pps_ptr)
	{
		if (g_dec_picture_ptr)//weihu
		{
			// this may only happen on slice loss
			H264Dec_exit_picture (g_image_ptr);
			
			//		H264Dec_display_control ();			
			//dec_output_ptr->frameEffective = TRUE;
			//dec_output_ptr->frame_width = img_ptr->width;
			//dec_output_ptr->frame_height = img_ptr->height;
			//dec_output_ptr->pOutFrameY = g_dec_picture_ptr->imgY;
			//dec_output_ptr->pOutFrameU = g_dec_picture_ptr->imgU;
			//dec_output_ptr->pOutFrameV = g_dec_picture_ptr->imgV;
			
			g_dec_picture_ptr = NULL;//weihu for output
			g_nFrame_dec_h264++;
		}
		g_active_pps_ptr = pps_ptr;
	}
}

PUBLIC void H264Dec_use_parameter_set (int32 pps_id)
{
	int PicParsetId = g_curr_slice_ptr->pic_parameter_set_id;  
	DEC_PPS_T *pps_ptr = &(g_pps_array_ptr[pps_id]);
	DEC_SPS_T *sps_ptr = &(g_sps_array_ptr[pps_ptr->seq_parameter_set_id]);

#if _MVC_
  if((g_curr_slice_ptr->svc_extension_flag == -1))
  {
    if (sps_ptr->valid != TRUE)
	{
      PRINTF ("PicParset %d references an invalid (uninitialized) Sequence Parameter Set with ID %d, expect the unexpected...\n", PicParsetId, (int) pps_ptr->seq_parameter_set_id);
      g_image_ptr->error_flag=TRUE;
	  return;
	}
  }
  else
  {
    // Set SPS to the subset SPS parameters
    g_active_subset_sps = g_SubsetSeqParSet + pps_ptr->seq_parameter_set_id;
    sps_ptr = &(g_active_subset_sps->sps);
    if (g_SubsetSeqParSet[pps_ptr->seq_parameter_set_id].Valid != TRUE)
	{
      PRINTF ("PicParset %d references an invalid (uninitialized) Subset Sequence Parameter Set with ID %d, expect the unexpected...\n",  PicParsetId, (int) pps_ptr->seq_parameter_set_id);
	  g_image_ptr->error_flag=TRUE;
	  return;
	}

  }
#endif

	H264Dec_active_sps (sps_ptr);
	H264Dec_active_pps (pps_ptr);

	if (!(sps_ptr->valid && pps_ptr->valid))
	{
		g_ready_to_decode_slice = FALSE;
	}

	//if (!pps_ptr->entropy_coding_mode_flag) //UVLC
	//{
	//	nal_startcode_follows = uvlc_startcode_follows;
	//	readRefFrame = H264Dec_get_te;
	//	readMVD_xy = readMVD_xy_cavlc;
	//}else
	//{
	//	nal_startcode_follows = biari_decode_final;
	//	readRefFrame = readRefFrame_CABAC;
	//	readMVD_xy = readMVD_xy_cabac;
	//}
	return;
}

PUBLIC void H264Dec_InterpretSEIMessage (void)
{
	//TBD
	return;
}

/*LOCAL void H264Dec_ReadVUI (DEC_SPS_T *sps_ptr)
{
	//TBD
	return;
}*/
LOCAL int32 H264Dec_ReadHRDParameters(DEC_HRD_PARAM_T *hrd_ptr)
{	
	uint32 SchedSelIdx;
	DEC_BS_T *stream = g_image_ptr->bitstrm_ptr;

  	hrd_ptr->cpb_cnt_minus1 = READ_UE_V (stream);
  	hrd_ptr->bit_rate_scale = READ_FLC (stream, 4);
  	hrd_ptr->cpb_size_scale = READ_FLC (stream, 4);

  	for( SchedSelIdx = 0; SchedSelIdx <= hrd_ptr->cpb_cnt_minus1; SchedSelIdx++ )
  	{
    		hrd_ptr->bit_rate_value_minus1[ SchedSelIdx ]  = READ_UE_V (stream);
    		hrd_ptr->cpb_size_value_minus1[ SchedSelIdx ] = H264Dec_Long_UEV (stream);
    		hrd_ptr->cbr_flag[ SchedSelIdx ]                         = READ_FLC  (stream,1);
  	}

  	hrd_ptr->initial_cpb_removal_delay_length_minus1  = READ_FLC (stream, 5);
  	hrd_ptr->cpb_removal_delay_length_minus1            = READ_FLC (stream, 5);
 	hrd_ptr->dpb_output_delay_length_minus1              = READ_FLC (stream, 5);
  	hrd_ptr->time_offset_length                                 = READ_FLC (stream,5);

  	return 0;
}

LOCAL void H264Dec_ReadVUI (DEC_VUI_T *vui_seq_parameters_ptr)
{	
	DEC_BS_T *stream = g_image_ptr->bitstrm_ptr;
   	vui_seq_parameters_ptr->aspect_ratio_info_present_flag = READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->aspect_ratio_info_present_flag)
    	{
      		vui_seq_parameters_ptr->aspect_ratio_idc = READ_FLC  (stream, 8);
      		if (255 == vui_seq_parameters_ptr->aspect_ratio_idc)
      		{
        		vui_seq_parameters_ptr->sar_width = READ_FLC  (stream,16);
        		vui_seq_parameters_ptr->sar_height = READ_FLC  (stream,16);
      		}
  	}

    	vui_seq_parameters_ptr->overscan_info_present_flag     = READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->overscan_info_present_flag)
    	{
      		vui_seq_parameters_ptr->overscan_appropriate_flag    = READ_FLC  (stream,1);
    	}

    	vui_seq_parameters_ptr->video_signal_type_present_flag = READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->video_signal_type_present_flag)
    	{
      		vui_seq_parameters_ptr->video_format                    = READ_FLC  (stream,3);
      		vui_seq_parameters_ptr->video_full_range_flag           = READ_FLC  (stream,1);
      		vui_seq_parameters_ptr->colour_description_present_flag = READ_FLC  (stream,1);
      		if(vui_seq_parameters_ptr->colour_description_present_flag)
      		{
        		vui_seq_parameters_ptr->colour_primaries              = READ_FLC  (stream,8);
        		vui_seq_parameters_ptr->transfer_characteristics      = READ_FLC  (stream,8);
        		vui_seq_parameters_ptr->matrix_coefficients           = READ_FLC  (stream,8);
      		}
    	}
		
    	vui_seq_parameters_ptr->chroma_location_info_present_flag = READ_FLC  (stream,1);
    	if(vui_seq_parameters_ptr->chroma_location_info_present_flag)
    	{
      		vui_seq_parameters_ptr->chroma_sample_loc_type_top_field     = READ_UE_V  (stream);
      		vui_seq_parameters_ptr->chroma_sample_loc_type_bottom_field  = READ_UE_V  (stream);
    	}
		
    	vui_seq_parameters_ptr->timing_info_present_flag          = READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->timing_info_present_flag)
    	{
      		vui_seq_parameters_ptr->num_units_in_tick               = READ_FLC  (stream, 32);
      		vui_seq_parameters_ptr->time_scale                      = READ_FLC  (stream, 32);
      		vui_seq_parameters_ptr->fixed_frame_rate_flag           = READ_FLC  (stream,1);
    	}
		
    	vui_seq_parameters_ptr->nal_hrd_parameters_present_flag   = READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->nal_hrd_parameters_present_flag)
    	{
      		H264Dec_ReadHRDParameters (&(vui_seq_parameters_ptr->nal_hrd_parameters));
    	}
		
    	vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag   = READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag)
    	{
      		H264Dec_ReadHRDParameters (&(vui_seq_parameters_ptr->vcl_hrd_parameters));
    	}
		
    	if (vui_seq_parameters_ptr->nal_hrd_parameters_present_flag || vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag)
    	{
      		vui_seq_parameters_ptr->low_delay_hrd_flag             =  READ_FLC  (stream,1);
    	}
		
    	vui_seq_parameters_ptr->pic_struct_present_flag          =  READ_FLC  (stream,1);
    	vui_seq_parameters_ptr->bitstream_restriction_flag       =  READ_FLC  (stream,1);
    	if (vui_seq_parameters_ptr->bitstream_restriction_flag)
    	{
      		vui_seq_parameters_ptr->motion_vectors_over_pic_boundaries_flag =  READ_FLC  (stream,1);
      		vui_seq_parameters_ptr->max_bytes_per_pic_denom                 =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->max_bits_per_mb_denom                   =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->log2_max_mv_length_horizontal           =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->log2_max_mv_length_vertical             =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->num_reorder_frames                      =  READ_UE_V (stream);
      		vui_seq_parameters_ptr->max_dec_frame_buffering                 =  READ_UE_V (stream);
    	}
		else
		{
			vui_seq_parameters_ptr->num_reorder_frames=0;
		}

  	return;	
}

const int8 ZZ_SCAN[16]  =
{  0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
};

const int8 ZZ_SCAN8[64] =
{  0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

LOCAL void Scaling_List(int8 *scalingList, int8 sizeOfScalingList, int8 *UseDefaultScalingMatrix, DEC_BS_T *s)
{
  int j, scanj;
  int delta_scale, lastScale, nextScale;

  lastScale      = 8;
  nextScale      = 8;

  for(j=0; j<sizeOfScalingList; j++)
  {
    scanj = (sizeOfScalingList==16) ? ZZ_SCAN[j]:ZZ_SCAN8[j];

    if(nextScale!=0)
    {
      delta_scale = READ_SE_V (s);
      nextScale =IClip(-128,127, (lastScale + delta_scale + 256) % 256);//weihu
      *UseDefaultScalingMatrix = (scanj==0 && nextScale==0);
    }

    scalingList[scanj] = (nextScale==0) ? lastScale:nextScale;
    lastScale = scalingList[scanj];
  }
}

LOCAL void H264Dec_interpret_sps (DEC_SPS_T *sps_ptr)
{
	int32 reserved_zero;
	DEC_BS_T *stream = g_image_ptr->bitstrm_ptr;


	sps_ptr->profile_idc = READ_FLC(stream, 8);
	if ((sps_ptr->profile_idc != 0x42) &&
		(sps_ptr->profile_idc != 0x4d) &&
		(sps_ptr->profile_idc != 0x64)
#if _MVC_
		&& (sps_ptr->profile_idc != 0x76)
		&& (sps_ptr->profile_idc != 0x80)
#endif
		)//0x42: baseline profile, 0x4d: main profile
	{
		g_image_ptr->error_flag = TRUE;
	}
	sps_ptr->constrained_set0_flag = READ_FLC(stream, 1);
	sps_ptr->constrained_set1_flag = READ_FLC(stream, 1);
	sps_ptr->constrained_set2_flag = READ_FLC(stream, 1);
	sps_ptr->constrained_set3_flag = READ_FLC(stream, 1);
#if _MVC_
	sps_ptr->constrained_set4_flag = READ_FLC(stream, 1);
	sps_ptr->constrained_set5_flag = READ_FLC(stream, 1);
	reserved_zero = READ_FLC(stream, 2);
#else
	reserved_zero = READ_FLC(stream, 4);
#endif
	sps_ptr->level_idc = READ_FLC(stream, 8);

	sps_ptr->seq_parameter_set_id = READ_UE_V(stream);
	if(sps_ptr->seq_parameter_set_id>31)
		{
		g_image_ptr->error_flag = TRUE;
		}
	if ((sps_ptr->profile_idc == 0x64)
#if _MVC_
		|| (sps_ptr->profile_idc == 0x76)
		|| (sps_ptr->profile_idc == 0x80)
#endif
		)
	{
		sps_ptr->chroma_format_idc = READ_UE_V(stream);
		if ((sps_ptr->chroma_format_idc > 1))//0x42: baseline profile, 0x4d: main profile //!=
		{
			g_image_ptr->error_flag = TRUE;
		}
		sps_ptr->bit_depth_luma_minus8 = READ_UE_V(stream);
		sps_ptr->bit_depth_chroma_minus8 = READ_UE_V(stream);
		sps_ptr->qpprime_y_zero_transform_bypass_flag = READ_FLC(stream, 1);
		sps_ptr->seq_scaling_matrix_present_flag = READ_FLC(stream, 1);
		if(sps_ptr->seq_scaling_matrix_present_flag)
		{
		  int i;
		  int n_ScalingList = 8;
		  for(i=0; i<n_ScalingList; i++)
		  {
			sps_ptr->seq_scaling_list_present_flag[i] = READ_FLC(stream, 1);
			if(sps_ptr->seq_scaling_list_present_flag[i])
			{
			  if(i<6)
				Scaling_List(sps_ptr->ScalingList4x4[i], 16, &sps_ptr->UseDefaultScalingMatrix4x4Flag[i], stream);
			  else
				Scaling_List(sps_ptr->ScalingList8x8[i-6], 64, &sps_ptr->UseDefaultScalingMatrix8x8Flag[i-6], stream);
			}
		  }
		}
	}
	else
	{
		sps_ptr->seq_scaling_matrix_present_flag=0;//weihu
		sps_ptr->chroma_format_idc=1;
	}
	sps_ptr->log2_max_frame_num_minus4 = READ_UE_V(stream);
	sps_ptr->pic_order_cnt_type = READ_UE_V(stream);

	if (sps_ptr->pic_order_cnt_type == 0)
	{
		sps_ptr->log2_max_pic_order_cnt_lsb_minus4 = READ_UE_V(stream);
		sps_ptr->delta_pic_order_always_zero_flag=0;//weihu
		sps_ptr->offset_for_non_ref_pic=0;
		sps_ptr->offset_for_top_to_bottom_field=0;
	}else if (sps_ptr->pic_order_cnt_type == 1)
	{
		int32 i;

		sps_ptr->delta_pic_order_always_zero_flag = READ_FLC(stream, 1);
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
		g_image_ptr->error_flag = TRUE;
		PRINTF ("sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER");
	}

	sps_ptr->gaps_in_frame_num_value_allowed_flag = READ_FLC(stream, 1);
	sps_ptr->pic_width_in_mbs_minus1 = READ_UE_V(stream);
	sps_ptr->pic_height_in_map_units_minus1 = READ_UE_V(stream);

	
	//if(!video_buffer_malloced)//¶¯Ì¬·ÖÅäÄÚ´æ
	//   frame_buf_size=(g_sps_ptr->pic_height_in_map_units_minus1+1)*(g_sps_ptr->pic_width_in_mbs_minus1+1)*256;
	//OR1200_WRITE_REG(GLB_REG_BASE_ADDR+IMG_SIZE_OFF, (((g_sps_ptr->pic_height_in_map_units_minus1+1)&0xff)<<8)|(g_sps_ptr->pic_width_in_mbs_minus1+1)&0xff,"IMG_SIZE");
	sps_ptr->frame_mbs_only_flag = READ_FLC(stream, 1);

	if (!sps_ptr->frame_mbs_only_flag)
	{
		g_image_ptr->error_flag = TRUE;
		sps_ptr->mb_adaptive_frame_field_flag = READ_FLC(stream, 1);
		if (sps_ptr->mb_adaptive_frame_field_flag)
		{
				PRINTF("MBAFF is not supported!\n");
		}
	}

	sps_ptr->direct_8x8_reference_flag = READ_FLC(stream, 1);
	sps_ptr->frame_cropping_flag = READ_FLC(stream, 1);

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

	sps_ptr->vui_parameters_present_flag = READ_FLC(stream, 1);
  sps_ptr->vui_seq_parameters->num_reorder_frames=0;
	if (sps_ptr->vui_parameters_present_flag)
	{
	    //	H264Dec_ReadVUI (sps_ptr);
		H264Dec_ReadVUI (sps_ptr->vui_seq_parameters);
	}

	if(g_image_ptr->error_flag)
	{
		sps_ptr->valid = FALSE;	
		return;
	}

	sps_ptr->valid = TRUE;

	return;
}

// fill subset_sps with content of p
#if _MVC_
static void InterpretSubsetSPS (DEC_SPS_T *sps_ptr, int *curr_seq_set_id)
{
  subset_seq_parameter_set_rbsp_t *subset_sps;
  unsigned int additional_extension2_flag;
  DEC_BS_T *stream = g_image_ptr->bitstrm_ptr;
  DEC_SPS_T *sps;
  //alloc sps 
 

  if ((sps=(DEC_SPS_T *)H264Dec_InterMemAlloc(1*sizeof (DEC_SPS_T)))== NULL)//calloc (1, sizeof (DEC_SPS_T))) == NULL)
  {
     PRINTF("InterpretSubsetSPS: alloc sps failed!");//no_mem_exit ("AllocSPS: SPS");
  }


  H264Dec_interpret_sps (sps);
  get_max_dec_frame_buf_size(sps);

 

  *curr_seq_set_id = sps->seq_parameter_set_id;

  if(sps->seq_parameter_set_id >= MAX_SPS)
  {
	  g_image_ptr->error_flag=TRUE;//for error
	  return;
  }


  subset_sps = g_SubsetSeqParSet + sps->seq_parameter_set_id;
  
  if(subset_sps->Valid || subset_sps->num_views_minus1>=0)
  {
    //if(memcmp(&subset_sps->sps, sps, sizeof (DEC_SPS_T)-sizeof(int)))//for OR debug
	  //assert(0);//for OR debug	
    reset_subset_sps(subset_sps);
  }
  SCI_MEMCPY (&subset_sps->sps, sps, sizeof (DEC_SPS_T));
 
  //free(sps)
  H264Dec_InterMemFree(1*sizeof (DEC_SPS_T));
  sps=NULL;

  //assert (subset_sps != NULL);//for OR debug
  subset_sps->Valid = FALSE;

  

  if( (subset_sps->sps.profile_idc==0x76) || (subset_sps->sps.profile_idc==0x80))
  {
    subset_sps->bit_equal_to_one = READ_FLC(stream, 1);

    if(subset_sps->bit_equal_to_one !=1 )
    {
      PRINTF("\nbit_equal_to_one is not equal to 1!\n");
	  g_image_ptr->error_flag=TRUE;//for error
	  return;// error exit
    }

    seq_parameter_set_mvc_extension(subset_sps, stream);

    subset_sps->mvc_vui_parameters_present_flag = READ_FLC(stream, 1);
    if(subset_sps->mvc_vui_parameters_present_flag)
      mvc_vui_parameters_extension(&(subset_sps->MVCVUIParams), stream);
  }

  additional_extension2_flag = READ_FLC(stream, 1);
  if(additional_extension2_flag)
  {
	  PRINTF("James::additional_extension2_flag is true!!!");
	  //james:need to implement?
    //while (more_rbsp_data(s->streamBuffer, s->frame_bitoffset, s->bitstream_length))
      //additional_extension2_flag = READ_FLC(stream, 1);
  }

  if (subset_sps->sps.valid)
    subset_sps->Valid = TRUE;

  

  return;
}
#endif

LOCAL void H264Dec_make_sps_availabe (int32 sps_id, DEC_SPS_T *sps_ptr)
{
	int8 *src_ptr = (int8 *)sps_ptr;
	int8 *dst_ptr = (int8 *)(&g_sps_array_ptr[sps_id]);
	int32 i;

	for (i = 0; (uint32)i < sizeof(DEC_SPS_T); i++)
	{
		*dst_ptr++ = *src_ptr++;
	}

	return;
}

PUBLIC void H264Dec_ProcessSPS (void)
{
	H264Dec_interpret_sps (g_sps_ptr);
#if _MVC_
	get_max_dec_frame_buf_size(g_sps_ptr);
#endif
	if(g_sps_ptr->seq_parameter_set_id<MAX_SPS)//weihu
	    H264Dec_make_sps_availabe (g_sps_ptr->seq_parameter_set_id, g_sps_ptr);
	else
	{
		g_image_ptr->error_flag = TRUE;
		return;
	}
		

	if (g_active_sps_ptr && (g_sps_ptr->seq_parameter_set_id == g_active_sps_ptr->seq_parameter_set_id))
	{
		g_old_pps_id = -1;
	}
#if _MVC_
	if(g_image_ptr->profile_idc < (int)g_sps_ptr->profile_idc)
		g_image_ptr->profile_idc = g_sps_ptr->profile_idc;
#else
	g_image_ptr->profile_idc = g_sps_ptr->profile_idc;
#endif
	return;
}

#if _MVC_
void ProcessSubsetSPS (void)
{
  subset_seq_parameter_set_rbsp_t *subset_sps;
  int curr_seq_set_id;

  InterpretSubsetSPS (g_sps_ptr, &curr_seq_set_id);
  if(curr_seq_set_id >= MAX_SPS)//weihu
	  return;

  subset_sps = g_SubsetSeqParSet + curr_seq_set_id;
  get_max_dec_frame_buf_size(&(subset_sps->sps));
  //check capability;
  if(subset_sps->num_views_minus1>1)
  {
    PRINTF("Warning: num_views:%d is greater than 2, only decode baselayer!\n", subset_sps->num_views_minus1+1);
    subset_sps->Valid = 0;
    subset_sps->sps.valid = 0;
    g_input->DecodeAllLayers = 0;
  }
  //else if(subset_sps->num_views_minus1==1 && (subset_sps->view_id[0]!=0 || subset_sps->view_id[1]!=1))
  //{
  //}

  if (subset_sps->Valid)
  {
    // SubsetSPSConsistencyCheck (subset_sps);
    g_image_ptr->profile_idc = subset_sps->sps.profile_idc;
    /*p_Vid->separate_colour_plane_flag = subset_sps->sps.separate_colour_plane_flag;
    if( p_Vid->separate_colour_plane_flag )
    {
      p_Vid->ChromaArrayType = 0;
    }
    else
    {
      p_Vid->ChromaArrayType = subset_sps->sps.chroma_format_idc;
    }*/ //james:not consider separate_colour_plane?
  }
}
#endif

LOCAL void H264Dec_interpret_pps (DEC_PPS_T *pps_ptr)
{
	int32 i;
	DEC_BS_T *stream = g_image_ptr->bitstrm_ptr;
	
	pps_ptr->pic_parameter_set_id = READ_UE_V(stream);
	pps_ptr->seq_parameter_set_id = READ_UE_V(stream);
	pps_ptr->entropy_coding_mode_flag = READ_FLC(stream, 1);
	pps_ptr->pic_order_present_flag = READ_FLC(stream, 1);
	pps_ptr->num_slice_groups_minus1 = READ_UE_V(stream);

	if ((pps_ptr->pic_parameter_set_id > 255)||(pps_ptr->seq_parameter_set_id>31))
	{
		g_image_ptr->error_flag=TRUE;
		PRINTF ("pic_parameter_set_id > 255 or pps_ptr->seq_parameter_set_id >31 !\n");
		pps_ptr->valid = FALSE;
		return;
	}//weihu

	//fmo parsing
	if (pps_ptr->num_slice_groups_minus1 > 0)
	{
		g_image_ptr->error_flag=TRUE;
		PRINTF ("FMO used!\n");
		pps_ptr->valid = FALSE;
		return;
/*
		g_image_ptr->fmo_used = TRUE;
		
		

		pps_ptr->slice_group_map_type	= (int8)READ_UE_V(stream);

		if (pps_ptr->slice_group_map_type == 6)
		{
			SCI_ASSERT(NULL != (pps_ptr->slice_group_id = (uint *)H264Dec_InterMemAlloc(SIZE_SLICE_GROUP_ID*sizeof(uint))));//(uint8)
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
			pps_ptr->slice_group_change_direction_flag = READ_FLC(stream, 1);
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
*/
	}else
	{
		g_image_ptr->fmo_used = FALSE; //FALSE;
	}

	//ONLY FOR FMO COMFORMANCE TEST
//	g_image_ptr->fmo_used = TRUE; //FALSE;
	pps_ptr->num_ref_idx_l0_active_minus1 = READ_UE_V(stream);
	if ((pps_ptr->num_ref_idx_l0_active_minus1+1) > MAX_REF_FRAME_NUMBER)
	{
		g_image_ptr->error_flag = TRUE;
		PRINTF ("too many l0_active not supported!\n");
		pps_ptr->valid = FALSE;
		return;
	}
	pps_ptr->num_ref_idx_l1_active_minus1 = READ_UE_V(stream);
	if ((pps_ptr->num_ref_idx_l1_active_minus1+1) > MAX_REF_FRAME_NUMBER)
	{
		g_image_ptr->error_flag = TRUE;
		PRINTF ("too many l0_active not supported!\n");
		pps_ptr->valid = FALSE;
		return;
	}
	pps_ptr->weighted_pred_flag = READ_FLC(stream, 1);
	pps_ptr->weighted_bipred_idc = READ_FLC(stream, 2);
	pps_ptr->pic_init_qp_minus26 = READ_SE_V(stream);
	pps_ptr->pic_init_qs_minus26 = READ_SE_V(stream);
	pps_ptr->chroma_qp_index_offset = READ_SE_V(stream);

	pps_ptr->deblocking_filter_control_present_flag = READ_FLC(stream, 1);
	pps_ptr->constrained_intra_pred_flag = READ_FLC(stream, 1);
	pps_ptr->redundant_pic_cnt_present_flag = READ_FLC(stream, 1);

	if (!uvlc_startcode_follows(g_image_ptr))
	{
		pps_ptr->transform_8x8_mode_flag           = READ_FLC(stream, 1);
		pps_ptr->pic_scaling_matrix_present_flag   = READ_FLC(stream, 1);

		if(pps_ptr->pic_scaling_matrix_present_flag)
		{
		  int n_ScalingList = 6 + 2 * pps_ptr->transform_8x8_mode_flag;
		  for(i=0; i<n_ScalingList; i++)
		  {
			pps_ptr->pic_scaling_list_present_flag[i]= READ_FLC(stream, 1);

			if(pps_ptr->pic_scaling_list_present_flag[i])
			{
			  if(i<6)
				Scaling_List(pps_ptr->ScalingList4x4[i], 16, &pps_ptr->UseDefaultScalingMatrix4x4Flag[i], stream);
			  else
				Scaling_List(pps_ptr->ScalingList8x8[i-6], 64, &pps_ptr->UseDefaultScalingMatrix8x8Flag[i-6], stream);
			}
		  }
		}

		pps_ptr->second_chroma_qp_index_offset = READ_SE_V(stream);
	}
	else
	{
		pps_ptr->second_chroma_qp_index_offset = pps_ptr->chroma_qp_index_offset;
		pps_ptr->transform_8x8_mode_flag           = 0;//weihu
		pps_ptr->pic_scaling_matrix_present_flag   = 0;//weihu
	}

	pps_ptr->valid = TRUE;

	return;
}

LOCAL void H264Dec_make_pps_available (int32 pps_id, DEC_PPS_T *pps_ptr)
{
	int8 *src_ptr = (int8 *)pps_ptr;
	int8 *dst_ptr = (int8 *)(&g_pps_array_ptr[pps_id]);
	int32 i;

	for (i = 0; (uint32)i < sizeof(DEC_PPS_T); i++)
	{
		*dst_ptr++ = *src_ptr++;
	}

	return;
}

PUBLIC void H264Dec_ProcessPPS (void)
{
	DEC_PPS_T *pps_ptr = g_pps_ptr;
	
	H264Dec_interpret_pps (pps_ptr);
	if ((pps_ptr->pic_parameter_set_id > 255)||(pps_ptr->seq_parameter_set_id>31))
	{
		g_image_ptr->error_flag=TRUE;
		return;
	}//weihu
	else
	    H264Dec_make_pps_available (g_pps_ptr->pic_parameter_set_id, pps_ptr);

	if (g_active_pps_ptr && (pps_ptr->pic_parameter_set_id == g_active_pps_ptr->pic_parameter_set_id))
	{
		g_old_pps_id = -1;
	}

	return;
}

#if _MVC_
static void free_pointer(void *pointer)
{
  if (pointer != NULL)
  {
    pointer = NULL;
  }
}
void init_subset_sps_list(subset_seq_parameter_set_rbsp_t *subset_sps_list, int iSize)
{
  int i;
  SCI_MEMSET(subset_sps_list, 0, iSize*sizeof(subset_sps_list[0]));
  for(i=0; i<iSize; i++)
  {
    subset_sps_list[i].sps.seq_parameter_set_id = (unsigned int) -1;
    subset_sps_list[i].num_views_minus1 = -1;
    subset_sps_list[i].num_level_values_signalled_minus1 = -1;
    subset_sps_list[i].MVCVUIParams.num_ops_minus1 = -1;
  }
}
void seq_parameter_set_mvc_extension(subset_seq_parameter_set_rbsp_t *subset_sps, DEC_BS_T *stream)//james: need to modify
{
  int i, j, num_views;

  subset_sps->num_views_minus1 = READ_UE_V(stream);
  num_views = 1+subset_sps->num_views_minus1;
  if(( num_views >0)||(num_views <=1024))
  {
    //if ((subset_sps->view_id = (int*)calloc(num_views, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->view_id");
	subset_sps->view_id = (int*)H264Dec_InterMemAlloc(num_views*sizeof(int));
    //if ((subset_sps->num_anchor_refs_l0 = (int*) calloc(num_views, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->num_anchor_refs_l0");
	subset_sps->num_anchor_refs_l0 = (int*)H264Dec_InterMemAlloc(num_views*sizeof(int));
    //if ((subset_sps->num_anchor_refs_l1 = (int*) calloc(num_views, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->num_anchor_refs_l1");
	subset_sps->num_anchor_refs_l1 = (int*)H264Dec_InterMemAlloc(num_views*sizeof(int));
    //if ((subset_sps->anchor_ref_l0 = (int**) calloc(num_views, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->anchor_ref_l0");
	subset_sps->anchor_ref_l0 =(int**)H264Dec_InterMemAlloc(num_views*sizeof(int*));
    //if ((subset_sps->anchor_ref_l1 = (int**) calloc(num_views, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->anchor_ref_l1");
    subset_sps->anchor_ref_l1 =(int**)H264Dec_InterMemAlloc(num_views*sizeof(int*));
    //if ((subset_sps->num_non_anchor_refs_l0 = (int*) calloc(num_views, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->num_non_anchor_refs_l0");
	subset_sps->num_non_anchor_refs_l0 = (int*)H264Dec_InterMemAlloc(num_views*sizeof(int));
    //if ((subset_sps->num_non_anchor_refs_l1 = (int*) calloc(num_views, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->num_non_anchor_refs_l1");
	subset_sps->num_non_anchor_refs_l1 = (int*)H264Dec_InterMemAlloc(num_views*sizeof(int));
    //if ((subset_sps->non_anchor_ref_l0 = (int**) calloc(num_views, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->non_anchor_ref_l0");
	subset_sps->non_anchor_ref_l0 = (int**)H264Dec_InterMemAlloc(num_views*sizeof(int*));
    //if ((subset_sps->non_anchor_ref_l1 = (int**) calloc(num_views, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->non_anchor_ref_l1");
	subset_sps->non_anchor_ref_l1 = (int**)H264Dec_InterMemAlloc(num_views*sizeof(int*));
  }
  else
  {
       g_image_ptr->error_flag=TRUE;//for error
	   return;
  }
  for(i=0; i<num_views; i++)
  {
    subset_sps->view_id[i] = READ_UE_V(stream);
	if(( subset_sps->view_id[i] <0)||(subset_sps->view_id[i] >1023))
	{
		g_image_ptr->error_flag=TRUE;//for error
		return;
	}
  }
  for(i=1; i<num_views; i++)
  {
    subset_sps->num_anchor_refs_l0[i] = READ_UE_V(stream);
	if(( subset_sps->num_anchor_refs_l0[i] <0)||(subset_sps->num_anchor_refs_l0[i] >=mmin(16,num_views)))
	{
		g_image_ptr->error_flag=TRUE;//for error
		return;
	}
    if(subset_sps->num_anchor_refs_l0[i]>0)
    {
      //if ((subset_sps->anchor_ref_l0[i] = (int*) calloc(subset_sps->num_anchor_refs_l0[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->anchor_ref_l0[i]");
      subset_sps->anchor_ref_l0[i] = (int*)H264Dec_InterMemAlloc(subset_sps->num_anchor_refs_l0[i]*sizeof(int));
      for(j=0; j<subset_sps->num_anchor_refs_l0[i]; j++)
        subset_sps->anchor_ref_l0[i][j] = READ_UE_V(stream);
    }

    subset_sps->num_anchor_refs_l1[i] = READ_UE_V(stream);
	if(( subset_sps->num_anchor_refs_l1[i] <0)||(subset_sps->num_anchor_refs_l1[i] >=mmin(16,num_views)))
	{
		g_image_ptr->error_flag=TRUE;//for error
		return;
	}
    if(subset_sps->num_anchor_refs_l1[i]>0)
    {
      //if ((subset_sps->anchor_ref_l1[i] = (int*) calloc(subset_sps->num_anchor_refs_l1[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->anchor_ref_l1[i]");
	  subset_sps->anchor_ref_l1[i] = (int*)H264Dec_InterMemAlloc(subset_sps->num_anchor_refs_l1[i]*sizeof(int));
      for(j=0; j<subset_sps->num_anchor_refs_l1[i]; j++)
        subset_sps->anchor_ref_l1[i][j] = READ_UE_V(stream);
    }
  }
  for(i=1; i<num_views; i++)
  {
    subset_sps->num_non_anchor_refs_l0[i] = READ_UE_V(stream);
	if(( subset_sps->num_non_anchor_refs_l0[i] <0)||(subset_sps->num_non_anchor_refs_l0[i] >=mmin(16,num_views)))
	{
		g_image_ptr->error_flag=TRUE;//for error
		return;
	}
    if(subset_sps->num_non_anchor_refs_l0[i]>0)
    {
      //if ((subset_sps->non_anchor_ref_l0[i] = (int*) calloc(subset_sps->num_non_anchor_refs_l0[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->non_anchor_ref_l0[i]");
	  subset_sps->non_anchor_ref_l0[i] = (int*)H264Dec_InterMemAlloc(subset_sps->num_non_anchor_refs_l0[i]*sizeof(int));
      for(j=0; j<subset_sps->num_non_anchor_refs_l0[i]; j++)
        subset_sps->non_anchor_ref_l0[i][j] = READ_UE_V(stream);
    }
    subset_sps->num_non_anchor_refs_l1[i] = READ_UE_V(stream);
	if(( subset_sps->num_non_anchor_refs_l1[i] <0)||(subset_sps->num_non_anchor_refs_l1[i] >=mmin(16,num_views)))
	{
		g_image_ptr->error_flag=TRUE;//for error
		return;
	}
    if(subset_sps->num_non_anchor_refs_l1[i]>0)
    {
      //if ((subset_sps->non_anchor_ref_l1[i] = (int*) calloc(subset_sps->num_non_anchor_refs_l1[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->non_anchor_ref_l1[i]");
	  subset_sps->non_anchor_ref_l1[i] = (int*)	H264Dec_InterMemAlloc(subset_sps->num_non_anchor_refs_l1[i]*sizeof(int));
      for(j=0; j<subset_sps->num_non_anchor_refs_l1[i]; j++)
        subset_sps->non_anchor_ref_l1[i][j] = READ_UE_V(stream);
    }
  }
  subset_sps->num_level_values_signalled_minus1 = READ_UE_V(stream);
  if((subset_sps->num_level_values_signalled_minus1 >=0)||(subset_sps->num_level_values_signalled_minus1<64))
  {
    i = 1+ subset_sps->num_level_values_signalled_minus1;
    //if ((subset_sps->level_idc = (int*) calloc(i, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->level_idc");
	subset_sps->level_idc = (int*)H264Dec_InterMemAlloc(i*sizeof(int));
    //if ((subset_sps->num_applicable_ops_minus1 = (int*) calloc(i, sizeof(int))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->num_applicable_ops_minus1");
	subset_sps->num_applicable_ops_minus1 = (int*)H264Dec_InterMemAlloc(i*sizeof(int));
    //if ((subset_sps->applicable_op_temporal_id = (int**) calloc(i, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_temporal_id");
	subset_sps->applicable_op_temporal_id = (int**)H264Dec_InterMemAlloc(i*sizeof(int*));
    //if ((subset_sps->applicable_op_num_target_views_minus1 = (int**) calloc(i, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_num_target_views_minus1");
	subset_sps->applicable_op_num_target_views_minus1 = (int**)H264Dec_InterMemAlloc(i*sizeof(int*));
    //if ((subset_sps->applicable_op_target_view_id = (int***) calloc(i, sizeof(int**))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_target_view_id");
    subset_sps->applicable_op_target_view_id = (int***)H264Dec_InterMemAlloc(i*sizeof(int**));
    //if ((subset_sps->applicable_op_num_views_minus1 = (int**) calloc(i, sizeof(int*))) == NULL)
    //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_num_views_minus1");
	subset_sps->applicable_op_num_views_minus1 = (int**)H264Dec_InterMemAlloc(i*sizeof(int*));
  }
  else
  {
	  g_image_ptr->error_flag=TRUE;//for error
	  return;
  }
  for(i=0; i<=subset_sps->num_level_values_signalled_minus1; i++)
  {
    subset_sps->level_idc[i] = READ_FLC(stream, 8);
    subset_sps->num_applicable_ops_minus1[i] = READ_UE_V(stream);
    if((subset_sps->num_applicable_ops_minus1[i]>=0)||(subset_sps->num_applicable_ops_minus1[i]<1024))
    {
      //if ((subset_sps->applicable_op_temporal_id[i] = (int*) calloc(1+subset_sps->num_applicable_ops_minus1[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_temporal_id[i]");
	  subset_sps->applicable_op_temporal_id[i] = (int*)H264Dec_InterMemAlloc((1+subset_sps->num_applicable_ops_minus1[i])*sizeof(int));
      //if ((subset_sps->applicable_op_num_target_views_minus1[i] = (int*) calloc(1+subset_sps->num_applicable_ops_minus1[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_num_target_views_minus1[i]");
      subset_sps->applicable_op_num_target_views_minus1[i] = (int*)H264Dec_InterMemAlloc((1+subset_sps->num_applicable_ops_minus1[i])*sizeof(int));
      //if ((subset_sps->applicable_op_target_view_id[i] = (int**) calloc(1+subset_sps->num_applicable_ops_minus1[i], sizeof(int *))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_target_view_id[i]");
	  subset_sps->applicable_op_target_view_id[i] = (int**)H264Dec_InterMemAlloc((1+subset_sps->num_applicable_ops_minus1[i])*sizeof(int*));
      //if ((subset_sps->applicable_op_num_views_minus1[i] = (int*) calloc(1+subset_sps->num_applicable_ops_minus1[i], sizeof(int))) == NULL)
      //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_num_views_minus1[i]");
      subset_sps->applicable_op_num_views_minus1[i] = (int*)H264Dec_InterMemAlloc((1+subset_sps->num_applicable_ops_minus1[i])*sizeof(int));
      for(j=0; j<=subset_sps->num_applicable_ops_minus1[i]; j++)
      {
        int k;
        subset_sps->applicable_op_temporal_id[i][j] = READ_FLC(stream, 3);
        subset_sps->applicable_op_num_target_views_minus1[i][j] = READ_UE_V(stream);
        if(subset_sps->applicable_op_num_target_views_minus1[i][j]>=0)
        {
          //if ((subset_sps->applicable_op_target_view_id[i][j] = (int*) calloc(1+subset_sps->applicable_op_num_target_views_minus1[i][j], sizeof(int))) == NULL)
          //  no_mem_exit("init_subset_seq_parameter_set: subset_sps->applicable_op_target_view_id[i][j]");
		  subset_sps->applicable_op_target_view_id[i][j] = (int*)H264Dec_InterMemAlloc((1+subset_sps->applicable_op_num_target_views_minus1[i][j])*sizeof(int));
          for(k = 0; k <= subset_sps->applicable_op_num_target_views_minus1[i][j]; k++)
            subset_sps->applicable_op_target_view_id[i][j][k] = READ_UE_V(stream);
        }
        subset_sps->applicable_op_num_views_minus1[i][j] = READ_UE_V(stream);
      }
    }
	else
	{
		g_image_ptr->error_flag=TRUE;//for error
		return;
	}
  }
}
int MemAlloc1D(void** ppBuf, int iEleSize, int iNum)
{
  if(iEleSize*iNum <=0)
    return 1;

  //*ppBuf = calloc(iNum, iEleSize);
  *ppBuf=H264Dec_InterMemAlloc(iNum*iEleSize);
  return (*ppBuf == NULL);
}
void hrd_parameters(MVCVUI_t *pMVCVUI, DEC_BS_T *stream)
{
  int i;

  pMVCVUI->cpb_cnt_minus1 = (char) READ_UE_V(stream);
  //assert(pMVCVUI->cpb_cnt_minus1<=31);//for OR debug
  pMVCVUI->bit_rate_scale = (char) READ_FLC(stream, 4);
  pMVCVUI->cpb_size_scale = (char) READ_FLC(stream, 4);
  for(i=0; i<=pMVCVUI->cpb_cnt_minus1; i++)
  {
    pMVCVUI->bit_rate_value_minus1[i] = READ_UE_V(stream);
    pMVCVUI->cpb_size_value_minus1[i] = READ_UE_V(stream);
    pMVCVUI->cbr_flag[i]              = (char) READ_FLC(stream, 1);
  }
  pMVCVUI->initial_cpb_removal_delay_length_minus1 = (char) READ_FLC(stream, 5);
  pMVCVUI->cpb_removal_delay_length_minus1         = (char) READ_FLC(stream, 5);
  pMVCVUI->dpb_output_delay_length_minus1          = (char) READ_FLC(stream, 5);
  pMVCVUI->time_offset_length                      = (char) READ_FLC(stream, 5);

}
void mvc_vui_parameters_extension(MVCVUI_t *pMVCVUI, DEC_BS_T *stream)
{
  int i, j, iNumOps;

  pMVCVUI->num_ops_minus1 = READ_UE_V(stream);
  iNumOps = 1+ pMVCVUI->num_ops_minus1;
  if(iNumOps > 0)
  {
    MemAlloc1D((void **)&(pMVCVUI->temporal_id), sizeof(pMVCVUI->temporal_id[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->num_target_output_views_minus1), sizeof(pMVCVUI->num_target_output_views_minus1[0]), iNumOps);
    //if ((pMVCVUI->view_id = (int**) calloc(iNumOps, sizeof(int*))) == NULL)
    //  no_mem_exit("mvc_vui_parameters_extension: pMVCVUI->view_id");
	pMVCVUI->view_id = (int**)H264Dec_InterMemAlloc(iNumOps*sizeof(int*));
    MemAlloc1D((void **)&(pMVCVUI->timing_info_present_flag), sizeof(pMVCVUI->timing_info_present_flag[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->num_units_in_tick), sizeof(pMVCVUI->num_units_in_tick[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->time_scale), sizeof(pMVCVUI->time_scale[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->fixed_frame_rate_flag), sizeof(pMVCVUI->fixed_frame_rate_flag[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->nal_hrd_parameters_present_flag), sizeof(pMVCVUI->nal_hrd_parameters_present_flag[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->vcl_hrd_parameters_present_flag), sizeof(pMVCVUI->vcl_hrd_parameters_present_flag[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->low_delay_hrd_flag), sizeof(pMVCVUI->low_delay_hrd_flag[0]), iNumOps);
    MemAlloc1D((void **)&(pMVCVUI->pic_struct_present_flag), sizeof(pMVCVUI->pic_struct_present_flag[0]), iNumOps);

    for(i=0; i<iNumOps; i++)
    {
      pMVCVUI->temporal_id[i] = (char) READ_FLC(stream, 3);
      pMVCVUI->num_target_output_views_minus1[i] = READ_UE_V(stream);
      if(pMVCVUI->num_target_output_views_minus1[i] >= 0)
        MemAlloc1D((void **)&(pMVCVUI->view_id[i]), sizeof(pMVCVUI->view_id[0][0]), pMVCVUI->num_target_output_views_minus1[i]+1);
      for(j=0; j<=pMVCVUI->num_target_output_views_minus1[i]; j++)
        pMVCVUI->view_id[i][j] = READ_UE_V(stream);
      pMVCVUI->timing_info_present_flag[i] = (char) READ_FLC(stream, 1);
      if(pMVCVUI->timing_info_present_flag[i])
      {
        pMVCVUI->num_units_in_tick[i]     = READ_FLC(stream, 32);
        pMVCVUI->time_scale[i]            = READ_FLC(stream, 32);
        pMVCVUI->fixed_frame_rate_flag[i] = (char) READ_FLC(stream, 1);
      }
      pMVCVUI->nal_hrd_parameters_present_flag[i] = (char) READ_FLC(stream, 1);
      if(pMVCVUI->nal_hrd_parameters_present_flag[i])
        hrd_parameters(pMVCVUI, stream);
      pMVCVUI->vcl_hrd_parameters_present_flag[i] = (char) READ_FLC(stream, 1);
      if(pMVCVUI->vcl_hrd_parameters_present_flag[i])
        hrd_parameters(pMVCVUI, stream);
      if(pMVCVUI->nal_hrd_parameters_present_flag[i]||pMVCVUI->vcl_hrd_parameters_present_flag[i])
        pMVCVUI->low_delay_hrd_flag[i]    = (char) READ_FLC(stream, 1);
      pMVCVUI->pic_struct_present_flag[i] = (char) READ_FLC(stream, 1);
    }
  }
}

void reset_subset_sps(subset_seq_parameter_set_rbsp_t *subset_sps)
{
  int i, j;

  if(subset_sps && subset_sps->num_views_minus1>=0)
  {
    subset_sps->sps.seq_parameter_set_id = (unsigned int) -1;

    free_pointer(subset_sps->view_id);
    for(i=0; i<=subset_sps->num_views_minus1; i++)
    {
      free_pointer(subset_sps->anchor_ref_l0[i]);
      free_pointer(subset_sps->anchor_ref_l1[i]);
    }
    free_pointer(subset_sps->anchor_ref_l0);
    free_pointer(subset_sps->anchor_ref_l1);
    free_pointer(subset_sps->num_anchor_refs_l0);
    free_pointer(subset_sps->num_anchor_refs_l1);

    for(i=0; i<=subset_sps->num_views_minus1; i++)
    {
      free_pointer(subset_sps->non_anchor_ref_l0[i]);
      free_pointer(subset_sps->non_anchor_ref_l1[i]);
    }
    free_pointer(subset_sps->non_anchor_ref_l0);
    free_pointer(subset_sps->non_anchor_ref_l1);
    free_pointer(subset_sps->num_non_anchor_refs_l0);
    free_pointer(subset_sps->num_non_anchor_refs_l1);

    if(subset_sps->num_level_values_signalled_minus1 >= 0)
    {
      free_pointer(subset_sps->level_idc);
      for(i=0; i<=subset_sps->num_level_values_signalled_minus1; i++)
      {
        for(j=0; j<=subset_sps->num_applicable_ops_minus1[i]; j++)
        {
          free_pointer(subset_sps->applicable_op_target_view_id[i][j]);
        }
        free_pointer(subset_sps->applicable_op_target_view_id[i]);
        free_pointer(subset_sps->applicable_op_temporal_id[i]);
        free_pointer(subset_sps->applicable_op_num_target_views_minus1[i]);
        free_pointer(subset_sps->applicable_op_num_views_minus1[i]);
      }
      free_pointer(subset_sps->applicable_op_target_view_id);
      free_pointer(subset_sps->applicable_op_temporal_id);
      free_pointer(subset_sps->applicable_op_num_target_views_minus1);
      free_pointer(subset_sps->applicable_op_num_views_minus1);      
      free_pointer(subset_sps->num_applicable_ops_minus1);

      subset_sps->num_level_values_signalled_minus1 = -1;
    }

    //end;
    subset_sps->num_views_minus1 = -1;
  }

  if(subset_sps && subset_sps->mvc_vui_parameters_present_flag)
  {
    MVCVUI_t *pMVCVUI = &(subset_sps->MVCVUIParams);
    if(pMVCVUI->num_ops_minus1 >=0)
    {
      free_pointer(pMVCVUI->temporal_id);
      free_pointer(pMVCVUI->num_target_output_views_minus1);
      for(i=0; i<=pMVCVUI->num_ops_minus1; i++)
        free_pointer(pMVCVUI->view_id[i]);
      free_pointer(pMVCVUI->view_id);
      free_pointer(pMVCVUI->timing_info_present_flag);
      free_pointer(pMVCVUI->num_units_in_tick);
      free_pointer(pMVCVUI->time_scale);
      free_pointer(pMVCVUI->fixed_frame_rate_flag);
      free_pointer(pMVCVUI->nal_hrd_parameters_present_flag);
      free_pointer(pMVCVUI->vcl_hrd_parameters_present_flag);
      free_pointer(pMVCVUI->low_delay_hrd_flag);
      free_pointer(pMVCVUI->pic_struct_present_flag);

      pMVCVUI->num_ops_minus1 = -1;
    }
    subset_sps->mvc_vui_parameters_present_flag = 0;
  }
}

int GetBaseViewId(subset_seq_parameter_set_rbsp_t **subset_sps)
{
  subset_seq_parameter_set_rbsp_t *curr_subset_sps;
  int i, iBaseViewId=0; //-1;

  *subset_sps = NULL;
  curr_subset_sps = g_SubsetSeqParSet;
  for(i=0; i<MAXSPS; i++)
  {
    if(curr_subset_sps->num_views_minus1>=0 && curr_subset_sps->sps.valid) // && curr_subset_sps->sps.seq_parameter_set_id < MAXSPS)
    {
      iBaseViewId = curr_subset_sps->view_id[0];
      break;
    }
    curr_subset_sps++;
  }

  if(i<MAXSPS)
    *subset_sps = curr_subset_sps;
  return iBaseViewId;
}

void get_max_dec_frame_buf_size(DEC_SPS_T *sps)
{
  int pic_size = (sps->pic_width_in_mbs_minus1 + 1) * (sps->pic_height_in_map_units_minus1 + 1) * (sps->frame_mbs_only_flag?1:2) * 384;

  int size = 0;

  switch (sps->level_idc)
  {
  case 9:
    size = 152064;
    break;
  case 10:
    size = 152064;
    break;
  case 11:
    if (!(sps->profile_idc >= 0x64 || sps->profile_idc == 0x2c) && (sps->constrained_set3_flag == 1))
      size = 152064;
    else
      size = 345600;
    break;
  case 12:
    size = 912384;
    break;
  case 13:
    size = 912384;
    break;
  case 20:
    size = 912384;
    break;
  case 21:
    size = 1824768;
    break;
  case 22:
    size = 3110400;
    break;
  case 30:
    size = 3110400;
    break;
  case 31:
    size = 6912000;
    break;
  case 32:
    size = 7864320;
    break;
  case 40:
    size = 12582912;
    break;
  case 41:
    size = 12582912;
    break;
  case 42:
    size = 13369344;
    break;
  case 50:
    size = 42393600;
    break;
  case 51:
    size = 70778880;
    break;
  default:
	PRINTF("undefined level");
	g_image_ptr->error_flag = TRUE;
	size = 70778880;//weihu
	//EXIT(500);//weihu
    break;
  }

  size /= pic_size;
  size = (size < 16) ? size : 16;
  sps->max_dec_frame_buffering = size;
}
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
