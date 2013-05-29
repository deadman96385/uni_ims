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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define PRINTF_HEAD_INFO	//PRINTF

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

#if _MVC_
void nal_unit_header_mvc_extension(NALUnitHeaderMVCExt_t *NaluHeaderMVCExt, DEC_BS_T *s)
{ 
	int tmp;
  NaluHeaderMVCExt->non_idr_flag     = READ_FLC(s, 1);
  NaluHeaderMVCExt->priority_id      = READ_FLC(s, 6);
  tmp= READ_FLC(s, 10);
  if((tmp<0)||(tmp>127))//for error //0~1023//weihu
  {
	   g_image_ptr->error_flag = TRUE;//weihu
  }
  NaluHeaderMVCExt->view_id          =tmp;
  NaluHeaderMVCExt->temporal_id      = READ_FLC(s, 3);
  NaluHeaderMVCExt->anchor_pic_flag  = READ_FLC(s, 1);
  NaluHeaderMVCExt->inter_view_flag  = READ_FLC(s, 1);
  NaluHeaderMVCExt->reserved_one_bit = READ_FLC(s, 1);
  if(NaluHeaderMVCExt->reserved_one_bit != 1)
  {
    g_image_ptr->error_flag = TRUE;//weihu
    PRINTF("Nalu Header MVC Extension: reserved_one_bit is not 1!\n");
  }
}
#endif

PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader()//uint8 *bitstrm_ptr, uint32 bitstrm_len)//weihu
{
	uint32 tmpVar;
	int32 ret = 0;
	DEC_NALU_T	*nal_ptr = g_nalu_ptr;
#if SIM_IN_WIN
	DEC_BS_T *stream = g_image_ptr->bitstrm_ptr;
	uint8 *bitstrm_ptr=g_nalu_ptr->buf; 
	uint32 bitstrm_len=g_nalu_ptr->len;


	H264Dec_InitBitstream_sw(stream, bitstrm_ptr, bitstrm_len);
	
	if ((g_nFrame_dec_h264 == 45) )//(g_image_ptr->mb_x == 2) && (g_image_ptr->mb_y == 3))
	{
		PRINTF("");
	}



	tmpVar = READ_FLC(stream, 8);
#else
    tmpVar = READ_FLC(NULL, 8);
#endif

	nal_ptr->nal_unit_type = tmpVar & 0x1f;
	nal_ptr->nal_reference_idc = ((tmpVar>>5)&0x3);
	nal_ptr->frobidden_bit = ((tmpVar>>7)&0x1);

#if _MVC_
	g_curr_slice_ptr->svc_extension_flag = -1;
#if FPGA_AUTO_VERIFICATION
	if( nal_ptr->nal_unit_type == NALU_TYPE_PREFIX || nal_ptr->nal_unit_type == NALU_TYPE_SLC_EXT)
#else
	if(g_input->DecodeAllLayers == 1 && nal_ptr->nal_unit_type == NALU_TYPE_PREFIX || nal_ptr->nal_unit_type == NALU_TYPE_SLC_EXT)
#endif
	{
#if SIM_IN_WIN
		g_curr_slice_ptr->svc_extension_flag = READ_FLC(stream, 1);
#else
		g_curr_slice_ptr->svc_extension_flag = READ_FLC(NULL, 1);
#endif

		if (!g_curr_slice_ptr->svc_extension_flag)//MVC
		{
#if SIM_IN_WIN
			nal_unit_header_mvc_extension(&g_curr_slice_ptr->NaluHeaderMVCExt, stream);
#else
            nal_unit_header_mvc_extension(&g_curr_slice_ptr->NaluHeaderMVCExt, NULL);
#endif
			g_curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU = (nal_ptr->nal_unit_type == NALU_TYPE_PREFIX);
		}
	}
	if(nal_ptr->nal_unit_type == NALU_TYPE_SLC_EXT)
	{
		if(g_curr_slice_ptr->svc_extension_flag)
		{
			//to be implemented for Annex G;
			g_image_ptr->error_flag=TRUE;			
		}
		else
			nal_ptr->nal_unit_type = NALU_TYPE_SLICE;
	}
	if (g_image_ptr->error_flag) 
	{
		return -1;
	}
#endif
	
//    OR1200_WRITE_REG(0x180014, (0x12340000|nal_ptr->nal_unit_type),"for FPGA debug 5");

	/*jump to corresponding NALU type decode*/
	g_ready_to_decode_slice = FALSE;
	switch(nal_ptr->nal_unit_type)
	{
	case NALU_TYPE_IDR:
	case NALU_TYPE_SLICE:
#if _MVC_
		if(g_curr_slice_ptr->svc_extension_flag == 0)
		{  //MVC
			g_curr_slice_ptr->view_id = g_curr_slice_ptr->NaluHeaderMVCExt.view_id;
			g_curr_slice_ptr->inter_view_flag = g_curr_slice_ptr->NaluHeaderMVCExt.inter_view_flag;
			g_curr_slice_ptr->anchor_pic_flag = g_curr_slice_ptr->NaluHeaderMVCExt.anchor_pic_flag;
		}
		else if(g_curr_slice_ptr->svc_extension_flag == -1) //SVC and the normal AVC;
		{
			if(g_active_subset_sps == NULL)
			{
				//g_curr_slice_ptr->view_id = GetBaseViewId(g_active_subset_sps);
				g_curr_slice_ptr->view_id = 0;//TOFIX
				if(g_curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU >0)
				{
					//assert(g_curr_slice_ptr->view_id == g_curr_slice_ptr->NaluHeaderMVCExt.view_id); //for OR debug
					g_curr_slice_ptr->inter_view_flag = g_curr_slice_ptr->NaluHeaderMVCExt.inter_view_flag;
					g_curr_slice_ptr->anchor_pic_flag = g_curr_slice_ptr->NaluHeaderMVCExt.anchor_pic_flag;
				}
				else
				{
					//g_curr_slice_ptr->inter_view_flag = 1;
					//g_curr_slice_ptr->anchor_pic_flag = g_image_ptr->idr_flag;
				}
			}
			else
			{
				//assert(g_active_subset_sps->num_views_minus1 >=0);////for OR debug
				// prefix NALU available
				if(g_curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU >0)
				{
					g_curr_slice_ptr->view_id = g_curr_slice_ptr->NaluHeaderMVCExt.view_id;
					g_curr_slice_ptr->inter_view_flag = g_curr_slice_ptr->NaluHeaderMVCExt.inter_view_flag;
					g_curr_slice_ptr->anchor_pic_flag = g_curr_slice_ptr->NaluHeaderMVCExt.anchor_pic_flag;
				}
				else
				{ //no prefix NALU;
					g_curr_slice_ptr->view_id = g_active_subset_sps->view_id[0];
					g_curr_slice_ptr->inter_view_flag = 1;
					g_curr_slice_ptr->anchor_pic_flag = g_image_ptr->idr_flag;
				}
			}
		}
		g_curr_slice_ptr->layer_id = g_curr_slice_ptr->view_id = GetVOIdx( g_curr_slice_ptr->view_id );
#endif
		g_ready_to_decode_slice = TRUE;
		
		ret = H264Dec_Process_slice (nal_ptr);

		if ((g_curr_slice_ptr->start_mb_nr == 0) && (!g_image_ptr->error_flag))
		{
			g_searching_IDR_pic = FALSE;
		}
		
		break;
	/*case NALU_TYPE_SLICE:
		if (!g_searching_IDR_pic)
		{
			g_ready_to_decode_slice = TRUE;
			ret = H264Dec_Process_slice (nal_ptr);
		}
		break;*/
	case NALU_TYPE_PPS:
		H264Dec_ProcessPPS ();
		break;
	case NALU_TYPE_SPS:
		H264Dec_ProcessSPS ();
		break;
	case NALU_TYPE_SUB_SPS:
      //PRINTF ("Found NALU_TYPE_SUB_SPS\n");
#if FPGA_AUTO_VERIFICATION
      if(1) 
#else
      if (g_input->DecodeAllLayers== 1)
#endif
      {
        ProcessSubsetSPS();
      }
      else
      {
          PRINTF ("Found Subsequence SPS NALU. Ignoring.\n");
      }
      break;
	case NALU_TYPE_SEI:
		H264Dec_InterpretSEIMessage ();
		break;
	case NALU_TYPE_PREFIX:
		//do something for SVC
		break;
	default:
		PRINTF ("nalu type=%d error!\n",nal_ptr->nal_unit_type);
	}

	return ret;
}

LOCAL void H264Dec_dec_ref_pic_marking (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 val;
	DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
#if _MVC_
	if (img_ptr->idr_flag ||  (g_curr_slice_ptr->svc_extension_flag == 0 && g_curr_slice_ptr->NaluHeaderMVCExt.non_idr_flag == 0) )
#else
	if (img_ptr->idr_flag)
#endif
	{
		img_ptr->no_output_of_prior_pics_flag = READ_FLC(stream, 1);
		img_ptr->long_term_reference_flag = READ_FLC(stream, 1);
	}else
	{
		img_ptr->adaptive_ref_pic_buffering_flag = READ_FLC(stream, 1);

		if (img_ptr->adaptive_ref_pic_buffering_flag)
		{
			//read memory management control operation
			g_dec_ref_pic_marking_buffer_size = 0;

			do 
			{
				tmp_drpm_ptr = g_dec_ref_pic_marking_buffer+g_dec_ref_pic_marking_buffer_size;
				g_dec_ref_pic_marking_buffer_size++;
				
				val = tmp_drpm_ptr->memory_management_control_operation = READ_UE_V(stream);

				if ((val==1) || (val==3))
				{
					tmp_drpm_ptr->difference_of_pic_nums_minus1 = READ_UE_V(stream);
				}

				if (val ==2)
				{
					tmp_drpm_ptr->long_term_pic_num = READ_UE_V(stream);
				}

				if ((val==3)||(val==6))
				{
					tmp_drpm_ptr->long_term_frame_idx = READ_UE_V(stream);
				}
				if (val == 4)
				{
					tmp_drpm_ptr->max_long_term_frame_idx_plus1 = READ_UE_V(stream);
				}
			} while(val != 0);
		}
	}

	return;
}

PUBLIC void H264Dec_FirstPartOfSliceHeader (DEC_SLICE_T *curr_slice_ptr, DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 tmp;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	curr_slice_ptr->start_mb_nr = READ_UE_V (stream);


	tmp = READ_UE_V (stream);

    
	if (tmp > 4)
	{
		tmp -= 5;
	}

	img_ptr->type = curr_slice_ptr->picture_type = (tmp<2 ? tmp+1 : 0);//tmp;//weihu

	curr_slice_ptr->pic_parameter_set_id = READ_UE_V (stream);

	if((tmp>9)||(curr_slice_ptr->pic_parameter_set_id>255))//for error
	{
		g_image_ptr->error_flag = TRUE;
		return;
	}
}
#if _MVC_
LOCAL void H264Dec_ref_pic_list_mvc_reordering(DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 tmp;
	int32 val;
	int32 i;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	if (img_ptr->type != I_SLICE)
	{
		val = curr_slice_ptr->ref_pic_list_reordering_flag_l0 = READ_FLC(stream, 1);

		if (val)
		{
			i=0;
			do
			{
				val = curr_slice_ptr->remapping_of_pic_nums_idc_l0[i] = READ_UE_V(stream);
				if (val==0 || val==1)
				{
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
					tmp = SHOW_FLC(stream, 16);

					if (tmp == 0)
					{
						READ_FLC(stream, 16);
						curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = 0xffff;
						READ_FLC(stream, 17);
					}else
					{
						curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = READ_UE_V(stream);
					}
				}
				else
				{
					if (val == 2)
					{
						OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
						OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
						tmp = SHOW_FLC(stream, 16);

						if (tmp == 0)
						{
							READ_FLC(stream, 16);
							curr_slice_ptr->long_term_pic_idx_l0[i] = 0xffff;
							READ_FLC(stream, 17);
						}else
						{
							curr_slice_ptr->long_term_pic_idx_l0[i] = READ_UE_V(stream);
						}
					}
					else if (val==4 || val==5)
					{
						curr_slice_ptr->abs_diff_view_idx_minus1[0][i] = READ_UE_V(stream);
					}
				}
				i++;
				/*if ((val > 3) || (val < 0))//james:
				{
					PRINTF ("Invalid remapping_of_pic_nums_idc command");
					img_ptr->error_flag = TRUE;
					return;
				}*/
			} while (val != 3);
		}
	}

	if (img_ptr->type == B_SLICE)
	{
		val = curr_slice_ptr->ref_pic_list_reordering_flag_l1 = READ_FLC(stream, 1);

		if (val)
		{
			i = 0;

			do 
			{
				val = curr_slice_ptr->remapping_of_pic_nums_idc_l1[i] = READ_UE_V(stream);
				
				if ((val == 0) || (val == 1))
				{
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
					tmp = SHOW_FLC(stream, 16);

					if (tmp == 0)
					{
						READ_FLC(stream, 16);
						curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = 0xffff;
						READ_FLC(stream, 17);
					}else
					{
						curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = READ_UE_V(stream);
					}
				}else
				{
					if (val == 2)
					{
						OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
						OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
						tmp = SHOW_FLC(stream, 16);

						if (tmp == 0)
						{
							READ_FLC(stream, 16);
							curr_slice_ptr->long_term_pic_idx_l1[i] = 0xffff;
							READ_FLC(stream, 17);
						}else
						{
							curr_slice_ptr->long_term_pic_idx_l1[i] = READ_UE_V(stream);
						}
					}
					else if (val==4 || val==5)
					{
						curr_slice_ptr->abs_diff_view_idx_minus1[1][i] = READ_UE_V(stream);
					}
				}
				i++;

				/*if ((val > 3) || (val < 0))
				{
					PRINTF ("Invalid remapping_of_pic_nums_idc command");
					img_ptr->error_flag = TRUE;
					return;
				}*/
			} while(val != 3);
		}
	}

	return;
}
#endif
LOCAL void H264Dec_ref_pic_list_reordering (DEC_IMAGE_PARAMS_T *img_ptr)
{
	uint32 tmp;
	uint32 val;
	int32 i;
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	if (img_ptr->type != I_SLICE)
	{
		val = curr_slice_ptr->ref_pic_list_reordering_flag_l0 = READ_FLC(stream, 1);

		if (val)
		{
			i = 0;

			do 
			{
				val = curr_slice_ptr->remapping_of_pic_nums_idc_l0[i] = READ_UE_V(stream);
				
				if ((val == 0) || (val == 1))
				{
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
					tmp = SHOW_FLC(stream, 16);

					if (tmp == 0)
					{
						READ_FLC(stream, 16);
						curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = 0xffff;
						READ_FLC(stream, 17);
					}else
					{
						curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = READ_UE_V(stream);
					}
				}else
				{
					if (val == 2)
					{
						OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
						OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
						tmp = SHOW_FLC(stream, 16);

						if (tmp == 0)
						{
							READ_FLC(stream, 16);
							curr_slice_ptr->long_term_pic_idx_l0[i] = 0xffff;
							READ_FLC(stream, 17);
						}else
						{
							curr_slice_ptr->long_term_pic_idx_l0[i] = READ_UE_V(stream);
						}
					}
				}
				i++;

				if ((val > 3) || (val < 0) ||(i>img_ptr->num_ref_idx_l0_active)&&(val != 3))//for error
				{
					PRINTF ("Invalid remapping_of_pic_nums_idc command");
					img_ptr->error_flag = TRUE;
					return;
				}
			} while(val != 3);
		}
	}

	if (img_ptr->type == B_SLICE)
	{
		val = curr_slice_ptr->ref_pic_list_reordering_flag_l1 = READ_FLC(stream, 1);

		if (val)
		{
			i = 0;

			do 
			{
				val = curr_slice_ptr->remapping_of_pic_nums_idc_l1[i] = READ_UE_V(stream);
				
				if ((val == 0) || (val == 1))
				{
					OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
					OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
					tmp = SHOW_FLC(stream, 16);

					if (tmp == 0)
					{
						READ_FLC(stream, 16);
						curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = 0xffff;
						READ_FLC(stream, 17);
					}else
					{
						curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = READ_UE_V(stream);
					}
				}else
				{
					if (val == 2)
					{
						OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
						OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
						tmp = SHOW_FLC(stream, 16);

						if (tmp == 0)
						{
							READ_FLC(stream, 16);
							curr_slice_ptr->long_term_pic_idx_l1[i] = 0xffff;
							READ_FLC(stream, 17);
						}else
						{
							curr_slice_ptr->long_term_pic_idx_l1[i] = READ_UE_V(stream);
						}
					}
				}
				i++;

				if ((val > 3) || (val < 0)||(i>img_ptr->num_ref_idx_l1_active)&&(val != 3))//for error
				{
					PRINTF ("Invalid remapping_of_pic_nums_idc command");
					img_ptr->error_flag = TRUE;
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

LOCAL void pred_weight_table(DEC_IMAGE_PARAMS_T *img_ptr, DEC_SLICE_T *curr_slice_ptr)
{
	int32 i, j;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	img_ptr->luma_log2_weight_denom = READ_UE_V(stream);
	img_ptr->wp_round_luma = img_ptr->luma_log2_weight_denom ? (1 << (img_ptr->luma_log2_weight_denom - 1) ) : 0;
    if(g_active_sps_ptr->chroma_format_idc)
	   img_ptr->chroma_log2_weight_denom = READ_UE_V(stream);
	else
	   img_ptr->chroma_log2_weight_denom = 0;
	img_ptr->wp_round_chroma = img_ptr->chroma_log2_weight_denom ? (1 << (img_ptr->chroma_log2_weight_denom - 1)) : 0;

	//reset
	for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
	{
		int32 comp;
		int32 log_weight_denom;

		for (comp = 0; comp < 3; comp++)
		{
			log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
			g_wp_weight[0][i][comp] = (1 << log_weight_denom);
			g_wp_weight[1][i][comp] = (1 << log_weight_denom);
		}
	}

	for (i = 0; i < img_ptr->num_ref_idx_l0_active; i++)
	{
		int32 luma_weight_flag_l0;
		int32 chroma_weight_flag_l0;

		luma_weight_flag_l0 = READ_FLC(stream, 1);

		if (luma_weight_flag_l0)
		{
			g_wp_weight[0][i][0] = READ_SE_V(stream);
			g_wp_offset[0][i][0] = READ_SE_V(stream);
		}else
		{
			g_wp_weight[0][i][0] = 1 << img_ptr->luma_log2_weight_denom;
			g_wp_offset[0][i][0] = 0;
		}

		if(g_active_sps_ptr->chroma_format_idc)//weihu
			chroma_weight_flag_l0 = READ_FLC(stream, 1);
		else
			chroma_weight_flag_l0 =0;

		for (j = 1; j < 3; j++)
		{
			if (chroma_weight_flag_l0)
			{
				g_wp_weight[0][i][j] = READ_SE_V(stream);
				g_wp_offset[0][i][j] = READ_SE_V(stream);
			}else
			{
				g_wp_weight[0][i][j] = 1 << img_ptr->chroma_log2_weight_denom;
				g_wp_offset[0][i][j] = 0;
			}
		}
		
	}

	if ((img_ptr->type == B_SLICE) && g_active_pps_ptr->weighted_bipred_idc == 1)
	{
		int32 luma_weight_flag_l1;
		int32 chroma_weight_flag_l1;
		
		for (i = 0; i < img_ptr->num_ref_idx_l1_active; i++)
		{
			luma_weight_flag_l1 = READ_FLC(stream, 1);

			if (luma_weight_flag_l1)
			{
				g_wp_weight[1][i][0] = READ_SE_V(stream);
				g_wp_offset[1][i][0] = READ_SE_V(stream);
			}else
			{
				g_wp_weight[1][i][0] = 1 << img_ptr->luma_log2_weight_denom;
				g_wp_offset[1][i][0] = 0;
			}

			if(g_active_sps_ptr->chroma_format_idc)//weihu
				chroma_weight_flag_l1 = READ_FLC(stream, 1);
			else
				chroma_weight_flag_l1 = 0;

			for (j = 1; j < 3; j++)
			{
				if (chroma_weight_flag_l1)
				{
					g_wp_weight[1][i][j] = READ_SE_V(stream);
					g_wp_offset[1][i][j] = READ_SE_V(stream);					
				}else
				{
					g_wp_weight[1][i][j] = 1 << img_ptr->chroma_log2_weight_denom;
					g_wp_offset[1][i][j] = 0;					
				}
			}
			
		}
	}
	
}

PUBLIC void H264Dec_RestSliceHeader (DEC_IMAGE_PARAMS_T *img_ptr, DEC_SLICE_T *curr_slice_ptr)
{
	uint32 tmp;
	DEC_SPS_T *active_sps_ptr = g_active_sps_ptr;
	DEC_PPS_T *active_pps_ptr = g_active_pps_ptr;
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	img_ptr->frame_num = READ_FLC (stream, active_sps_ptr->log2_max_frame_num_minus4+4);
	

	if (active_sps_ptr->frame_mbs_only_flag)
	{

	}else
	{
		int32 field_pic_flag = READ_FLC(stream, 1);

		if (field_pic_flag)
		{
			img_ptr->error_flag = TRUE;
			PRINTF("field is not supported!\n");
		//	exit(-1);
		}
	}

	if (img_ptr->idr_flag)
	{
		img_ptr->pre_frame_num = 0;

		OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_RDY_OFF, 0x00000001,0x00000001,"BSM_rdy");	
		OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, (16<<24),"BSM_rd n bits");
		tmp = SHOW_FLC (stream, 16);

		if (tmp == 0)
		{
			READ_FLC (stream, 16);
			img_ptr->idr_pic_id = 0xffff;
			READ_FLC (stream, 17);
		}else
		{
			img_ptr->idr_pic_id = READ_UE_V (stream);
		}
	}
#if _MVC_
	else if ( g_curr_slice_ptr->svc_extension_flag == 0 && g_curr_slice_ptr->NaluHeaderMVCExt.non_idr_flag == 0 )
	{
		img_ptr->idr_pic_id = READ_UE_V (stream);
	}
#endif

	if (active_sps_ptr->pic_order_cnt_type == 0)
	{
		img_ptr->pic_order_cnt_lsb = READ_FLC (stream, active_sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4);

		if (active_pps_ptr->pic_order_present_flag)
		{
			img_ptr->delta_pic_order_cnt_bottom = H264Dec_Long_SEV (stream);
		}else
		{
			img_ptr->delta_pic_order_cnt_bottom = 0;
		}
	}

	if ((active_sps_ptr->pic_order_cnt_type == 1) && (!(active_sps_ptr->delta_pic_order_always_zero_flag)))
	{
		img_ptr->delta_pic_order_cnt[0] = H264Dec_Long_SEV (stream);

		if (active_pps_ptr->pic_order_present_flag)
		{
			img_ptr->delta_pic_order_cnt[1] = H264Dec_Long_SEV (stream);
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
		img_ptr->redundant_pic_cnt = READ_UE_V (stream);
	}

	if(img_ptr->type==B_SLICE)
  	{
    		img_ptr->direct_type = READ_FLC(stream, 1); //u_1 ("SH: direct_spatial_mv_pred_flag", currStream);
	}
	else
	{
		img_ptr->direct_type =0;
	}

	/*if (img_ptr->direct_type == 1)
	{
		direct_mv = H264Dec_direct_mv_spatial;
		pred_skip_bslice = H264Dec_mv_prediction_skipped_bslice_spatial;
		MC8x8_direct = H264Dec_MC8x8_direct_spatial;
	}else
	{
		direct_mv = H264Dec_direct_mv_temporal;
		pred_skip_bslice = H264Dec_mv_prediction_skipped_bslice_temporal;
		MC8x8_direct = H264Dec_MC8x8_direct_temporal;
	}*/

	img_ptr->num_ref_idx_l0_active = active_pps_ptr->num_ref_idx_l0_active_minus1 + 1;
	img_ptr->num_ref_idx_l1_active = active_pps_ptr->num_ref_idx_l1_active_minus1 + 1;


	if ((P_SLICE == img_ptr->type) || (B_SLICE == img_ptr->type))
	{
		tmp = READ_FLC (stream, 1);

		if (tmp)
		{
			img_ptr->num_ref_idx_l0_active = (1+READ_UE_V(stream));
			
			if(B_SLICE == img_ptr->type)
		    {
				img_ptr->num_ref_idx_l1_active = (1+READ_UE_V(stream));///1 + ue_v ("SH: num_ref_idx_l1_active_minus1", currStream);
				if (img_ptr->num_ref_idx_l1_active > MAX_REF_FRAME_NUMBER)
				{
					img_ptr->error_flag = TRUE;
					PRINTF ("too many l0_active not supported!\n");
				}
			}

		}


		if (img_ptr->num_ref_idx_l0_active > MAX_REF_FRAME_NUMBER)
		{
			img_ptr->error_flag = TRUE;
			PRINTF ("too many l0_active not supported!\n");
		}
	}

	if (img_ptr->type != B_SLICE)
	{
	    img_ptr->num_ref_idx_l1_active = 0;
	}
#if _MVC_
	if(g_curr_slice_ptr->svc_extension_flag == 0 || g_curr_slice_ptr->svc_extension_flag == 1)
		H264Dec_ref_pic_list_mvc_reordering(img_ptr);
	else
		H264Dec_ref_pic_list_reordering(img_ptr);
#else
	H264Dec_ref_pic_list_reordering(img_ptr);
#endif
    if(img_ptr->error_flag)
    {
        return;
    }

	img_ptr->apply_weights = ((active_pps_ptr->weighted_pred_flag && (curr_slice_ptr->picture_type == P_SLICE ) )
          || ((active_pps_ptr->weighted_bipred_idc > 0 ) && (curr_slice_ptr->picture_type == B_SLICE)));

	if ((active_pps_ptr->weighted_pred_flag&&(img_ptr->type==P_SLICE))||
      (active_pps_ptr->weighted_bipred_idc==1 && (img_ptr->type==B_SLICE)))
	{
		pred_weight_table(img_ptr, curr_slice_ptr);
	}
	else
	{
		img_ptr->luma_log2_weight_denom=0;
		img_ptr->chroma_log2_weight_denom=0;
	}

	if (img_ptr->nal_reference_idc)
	{
		H264Dec_dec_ref_pic_marking (img_ptr);
	}

	if (active_pps_ptr->entropy_coding_mode_flag && img_ptr->type!=I_SLICE)
	{
		img_ptr->model_number = READ_UE_V(stream);//ue_v("SH: cabac_init_idc", currStream);
	}
	else 
	{
		img_ptr->model_number = 0;
	}

	tmp = READ_SE_V(stream);
	curr_slice_ptr->slice_qp_delta = tmp;
	curr_slice_ptr->qp = img_ptr->qp = active_pps_ptr->pic_init_qp_minus26+26+tmp;

	if (active_pps_ptr->deblocking_filter_control_present_flag)
	{
		curr_slice_ptr->LFDisableIdc = READ_UE_V(stream);

		if (curr_slice_ptr->LFDisableIdc != 1)
		{
			curr_slice_ptr->LFAlphaC0Offset = 2*READ_SE_V(stream);
			curr_slice_ptr->LFBetaOffset = 2* READ_SE_V(stream);
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

		len = (active_sps_ptr->pic_height_in_map_units_minus1+1)*(active_sps_ptr->pic_width_in_mbs_minus1+1)/
			((int32)active_pps_ptr->slice_group_change_rate_minus1+1);

		if (((active_sps_ptr->pic_height_in_map_units_minus1+1)*(active_sps_ptr->pic_width_in_mbs_minus1+1))%
			((int32)active_pps_ptr->slice_group_change_rate_minus1+1))
		{
			len += 1;
		}

		len = CeilLog2(len+1);

		img_ptr->slice_group_change_cycle = READ_FLC(stream, len);
	}
	
	return;
}
#if _MVC_
int GetVOIdx(int iViewId)
{
  int iVOIdx = -1;
  int *piViewIdMap;
  if(g_active_subset_sps)
  {
    piViewIdMap = g_active_subset_sps->view_id;
    for(iVOIdx = g_active_subset_sps->num_views_minus1; iVOIdx>=0; iVOIdx--)
      if(piViewIdMap[iVOIdx] == iViewId)
        break;
  }
  else
  {
    subset_seq_parameter_set_rbsp_t *curr_subset_sps;
    int i;

    curr_subset_sps = g_SubsetSeqParSet;
    for(i=0; i<MAXSPS; i++)
    {
      if(curr_subset_sps->num_views_minus1>=0 && curr_subset_sps->sps.valid)
      {
        break;
      }
      curr_subset_sps++;
    }

    if( i < MAXSPS )
    {
      g_active_subset_sps = curr_subset_sps;

      piViewIdMap = g_active_subset_sps->view_id;
      for(iVOIdx = g_active_subset_sps->num_views_minus1; iVOIdx>=0; iVOIdx--)
        if(piViewIdMap[iVOIdx] == iViewId)
          break;

      return iVOIdx;
    }
    else
    {
      iVOIdx = 0;
    }
  }

  return iVOIdx;
}

int GetViewIdx(int iVOIdx)
{
  int iViewIdx = -1;
  int *piViewIdMap;

  if( g_active_subset_sps )
  {
    //assert( g_active_subset_sps->num_views_minus1 >= iVOIdx && iVOIdx >= 0 );//for OR debug
    piViewIdMap = g_active_subset_sps->view_id;
    iViewIdx = piViewIdMap[iVOIdx];    
  }

  return iViewIdx;
}
int get_maxViewIdx (int view_id, int anchor_pic_flag, int listidx)
{
  int VOIdx;
  int maxViewIdx = 0;

  VOIdx = view_id; 
  if(VOIdx >= 0)
  {
    if(anchor_pic_flag)
      maxViewIdx = listidx? g_active_subset_sps->num_anchor_refs_l1[VOIdx] : g_active_subset_sps->num_anchor_refs_l0[VOIdx];
    else
      maxViewIdx = listidx? g_active_subset_sps->num_non_anchor_refs_l1[VOIdx] : g_active_subset_sps->num_non_anchor_refs_l0[VOIdx];
  }

  return maxViewIdx;
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





















