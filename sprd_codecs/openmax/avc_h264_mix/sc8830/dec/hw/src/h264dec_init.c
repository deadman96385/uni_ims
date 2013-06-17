/******************************************************************************
 ** File Name:    h264dec_init.c                                             *
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
extern int *context;

LOCAL void H264Dec_init_old_slice (DEC_OLD_SLICE_PARAMS_T *old_slice_ptr)
{
	old_slice_ptr->field_pic_flag = 0;
	old_slice_ptr->pps_id = (uint8)UINT_MAX;
	old_slice_ptr->frame_num = (int32)UINT_MAX;
	old_slice_ptr->nal_ref_idc = (int8)0xff;
	old_slice_ptr->idr_flag = 0;
	old_slice_ptr->pic_order_cnt_lsb = SINT_MAX;
	old_slice_ptr->delta_pic_order_cnt_bottom = SINT_MAX;
	old_slice_ptr->delta_pic_order_cnt[0] = SINT_MAX;
	old_slice_ptr->delta_pic_order_cnt[1] = SINT_MAX;
#if _MVC_	
	old_slice_ptr->view_id=(int8)0xff;//weihu
	old_slice_ptr->inter_view_flag=(int8)0xff;
	old_slice_ptr->anchor_pic_flag=(int8)0xff;
#endif
	return;
}

PUBLIC void H264Dec_init_global_para (void)
{
 	int32 i,j;


	SCI_ASSERT(NULL != (g_sps_array_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T) * MAX_SPS)));
	SCI_ASSERT(NULL != (g_pps_array_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T) * MAX_PPS)));

	SCI_ASSERT(NULL != (g_sps_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T))));
	g_sps_ptr->vui_seq_parameters = (DEC_VUI_T *)H264Dec_InterMemAlloc (sizeof(DEC_VUI_T));//weihu
	SCI_ASSERT(NULL != (g_pps_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T))));
#if _MVC_
	SCI_ASSERT(NULL != (g_dpb_layer[0] = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc (sizeof(DEC_DECODED_PICTURE_BUFFER_T))));
	g_dpb_layer[0]->init_done=0;
	g_dpb_layer[0]->used_size=0;
	SCI_ASSERT(NULL != (g_dpb_layer[1] = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc (sizeof(DEC_DECODED_PICTURE_BUFFER_T))));
    g_dpb_layer[1]->init_done=0;
	g_dpb_layer[1]->used_size=0;
#else
	SCI_ASSERT(NULL != (g_dpb_ptr = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc (sizeof(DEC_DECODED_PICTURE_BUFFER_T))));
#endif
	g_active_sps_ptr = NULL;
	g_active_pps_ptr = NULL;

	SCI_ASSERT(NULL != (g_old_slice_ptr = (DEC_OLD_SLICE_PARAMS_T *)H264Dec_InterMemAlloc (sizeof(DEC_OLD_SLICE_PARAMS_T))));

	SCI_ASSERT(NULL != (g_nalu_ptr = (DEC_NALU_T *)H264Dec_InterMemAlloc (sizeof(DEC_NALU_T))));
//	SCI_ASSERT(NULL != (g_nalu_ptr->buf = (uint8 *)H264Dec_InterMemAlloc (sizeof(uint8)*NALU_BUFFER_SIZE)));
    
	SCI_ASSERT(NULL != (g_image_ptr = (DEC_IMAGE_PARAMS_T *)H264Dec_InterMemAlloc(sizeof(DEC_IMAGE_PARAMS_T))));
    memset(g_image_ptr,0,sizeof(DEC_IMAGE_PARAMS_T));

	SCI_ASSERT(NULL != (g_image_ptr->bitstrm_ptr = (DEC_BS_T *)H264Dec_InterMemAlloc(sizeof(DEC_BS_T))));
	SCI_ASSERT(NULL != (g_curr_slice_ptr = (DEC_SLICE_T *)H264Dec_InterMemAlloc(sizeof(DEC_SLICE_T)))); 
	SCI_ASSERT(NULL != (g_curr_slice_ptr->mot_ctx = (MotionInfoContexts *)H264Dec_InterMemAlloc(sizeof(MotionInfoContexts)))); 
	SCI_ASSERT(NULL != (g_curr_slice_ptr->tex_ctx = (TextureInfoContexts *)H264Dec_InterMemAlloc(sizeof(TextureInfoContexts)))); 
#if _MVC_
	SCI_ASSERT(NULL != (g_curr_slice_ptr->p_Dpb = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc(sizeof(DEC_DECODED_PICTURE_BUFFER_T)))); 
#endif

	SCI_ASSERT(NULL != (g_no_reference_picture_ptr = (DEC_STORABLE_PICTURE_T *)H264Dec_InterMemAlloc(sizeof(DEC_STORABLE_PICTURE_T))));
    //memset1(g_no_reference_picture_ptr,0,sizeof(DEC_STORABLE_PICTURE_T));
//	SCI_ASSERT(NULL != (g_mb_cache_ptr = (DEC_MB_CACHE_T *)H264Dec_InterMemAlloc(sizeof(DEC_MB_CACHE_T))));

	SCI_ASSERT(NULL != (g_cavlc_tbl_ptr = (uint32 *)H264Dec_InterMemAlloc(sizeof(uint32)*69)));

	SCI_ASSERT(NULL != (g_input = (INPUT_PARA_T *)H264Dec_InterMemAlloc(sizeof(INPUT_PARA_T))));
	

	//g_image_ptr->b8_mv_pred_func[0] = H264Dec_mv_prediction_P8x8_8x8;
	//g_image_ptr->b8_mv_pred_func[1] = H264Dec_mv_prediction_P8x8_8x4;
	//g_image_ptr->b8_mv_pred_func[2] = H264Dec_mv_prediction_P8x8_4x8;
	//g_image_ptr->b8_mv_pred_func[3] = H264Dec_mv_prediction_P8x8_4x4;

	//init global vars
	g_dec_ref_pic_marking_buffer_size = 0;
	g_ready_to_decode_slice = FALSE;
	g_searching_IDR_pic = 1;
	g_nFrame_dec_h264 = 0;
//	g_firstBsm_init_h264 = TRUE;
	g_old_pps_id = -1; //initialize to a impossible value
//	last_dquant = 0;
#if _MVC_	
	last_profile_idc=0;
#endif 

//#if SIM_IN_WIN
//#else//?xxxxxxxx
	for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
	{
		g_list0_map_addr[i] = 0;
		g_list1_map_addr[i] = 0;
	}
	
	for (i=0; i<2; i++)
	{
		for (j=0; j<16; j++)
		{      	
			OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+136+(2*j+32*i)*4,0,"clr g_wp_weight");			
			OR1200_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+140+(2*j+32*i)*4,0,"clr g_wp_offset");
			
		}
	}
	for(i=3;i<6;i++)//inter
	{		
		for(j=0;j<4;j++)
		{			
			OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER4x4Y_OFF+(i-3)*16+j*4,0,"clr weightscale inter4x4");
		}
	}
	
	for(i=0;i<3;i++)//intra
	{		
		for(j=0;j<4;j++)
		{
			OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA4x4Y_OFF+i*16+j*4,0,"clr weightscale intra4x4");
		}
	}
	
	for(j=0;j<8;j++)
	{		
		OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+2*j*4,0," clr weightscale inter8x8");		
		OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+(2*j+1)*4,0," clr weightscale inter8x8");
		OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+2*j*4,0," clr weightscale inter8x8");
		OR1200_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+(2*j+1)*4,0," clr weightscale inter8x8");
	}

//#endif

	g_no_reference_picture_ptr->used_for_reference = 0;
	g_no_reference_picture_ptr->is_long_term = 0;
	g_no_reference_picture_ptr->frame_num = 0;
	g_no_reference_picture_ptr->pic_num = 0;
	g_no_reference_picture_ptr->long_term_pic_num = 0;
	g_no_reference_picture_ptr->long_term_frame_idx = 0;
	g_no_reference_picture_ptr->poc=0;
	g_no_reference_picture_ptr->imgYAddr=0;
	g_no_reference_picture_ptr->imgUAddr=0;
	g_no_reference_picture_ptr->imgVAddr=0;
	g_no_reference_picture_ptr->DPB_addr_index=0x3f;
	for (i = 0; i < (MAX_REF_FRAME_NUMBER+MAX_REF_FRAME_NUMBER); i++)//weihu//+1
	{
		g_list0[i] = g_no_reference_picture_ptr;
	}

	g_image_ptr->dec_ref_pic_marking_buffer = g_dec_ref_pic_marking_buffer;

	for (i = 0; i < MAX_SPS; i++)
	{
		g_sps_array_ptr[i].valid = FALSE;
	}

	for (i = 0; i < MAX_PPS; i++)
	{
		g_pps_array_ptr[i].valid = FALSE;
	}

	g_image_ptr->curr_slice_ptr = g_curr_slice_ptr;
	g_image_ptr->curr_mb_nr = 0;
	g_image_ptr->error_flag = FALSE;
	g_image_ptr->is_need_init_vsp_hufftab = TRUE;
	g_image_ptr->no_output_of_prior_pics_flag=0;//weihu
	g_image_ptr->profile_idc=0;
	
	

	g_active_pps_ptr = NULL;
	g_active_sps_ptr = NULL;
    g_dec_picture_ptr = NULL;

	g_MbToSliceGroupMap = NULL;

	//init old slice
	H264Dec_init_old_slice (g_old_slice_ptr);

#if _MVC_
    g_active_subset_sps = NULL;
	g_curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU=NULL;//weihu
	g_curr_slice_ptr->inter_view_flag= NULL;
	g_curr_slice_ptr->anchor_pic_flag= NULL;
    init_subset_sps_list(g_SubsetSeqParSet, MAXSPS);

	g_input->DecodeAllLayers = 0;
#endif

	return;
}

PUBLIC void H264Dec_init_vld_table (void)
{
	//context = (int *)(CABAC_CONTEXT_BASE_ADDR);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
