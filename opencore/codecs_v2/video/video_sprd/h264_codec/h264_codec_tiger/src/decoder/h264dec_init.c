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
  ** 06/26/2012   Leon Li             Modify.                                                                                      *
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

	return;
}

PUBLIC void H264Dec_init_global_para (void)
{
 	int32 i;

	g_stream_offset = 0;
#if _CMODEL_
	VSP_Init_CModel();
#endif //_CMODEL_

	g_sps_array_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T) * MAX_SPS);
        g_pps_array_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T) * MAX_PPS);
	g_sps_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T));
	g_pps_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T));
	g_dpb_ptr = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc (sizeof(DEC_DECODED_PICTURE_BUFFER_T));
#ifdef _VSP_LINUX_
	g_dpb_ptr->used_size = 0;
#endif
	g_active_sps_ptr = NULL;
	g_active_pps_ptr = NULL;

	g_old_slice_ptr = (DEC_OLD_SLICE_PARAMS_T *)H264Dec_InterMemAlloc (sizeof(DEC_OLD_SLICE_PARAMS_T));
	g_nalu_ptr = (DEC_NALU_T *)H264Dec_InterMemAlloc (sizeof(DEC_NALU_T));

	g_image_ptr = (DEC_IMAGE_PARAMS_T *)H264Dec_InterMemAlloc(sizeof(DEC_IMAGE_PARAMS_T));
	g_image_ptr->bitstrm_ptr = (DEC_BS_T *)H264Dec_InterMemAlloc(sizeof(DEC_BS_T));
	g_curr_slice_ptr = (DEC_SLICE_T *)H264Dec_InterMemAlloc(sizeof(DEC_SLICE_T));
	g_curr_slice_ptr->mot_ctx = (MotionInfoContexts *)H264Dec_InterMemAlloc(sizeof(MotionInfoContexts)); 
	g_curr_slice_ptr->tex_ctx = (TextureInfoContexts *)H264Dec_InterMemAlloc(sizeof(TextureInfoContexts)); 

	g_no_reference_picture_ptr = (DEC_STORABLE_PICTURE_T *)H264Dec_InterMemAlloc(sizeof(DEC_STORABLE_PICTURE_T));

	g_mb_cache_ptr = (DEC_MB_CACHE_T *)H264Dec_InterMemAlloc(sizeof(DEC_MB_CACHE_T));

	g_image_ptr->b8_mv_pred_func[0] = H264Dec_mv_prediction_P8x8_8x8;
	g_image_ptr->b8_mv_pred_func[1] = H264Dec_mv_prediction_P8x8_8x4;
	g_image_ptr->b8_mv_pred_func[2] = H264Dec_mv_prediction_P8x8_4x8;
	g_image_ptr->b8_mv_pred_func[3] = H264Dec_mv_prediction_P8x8_4x4;

	//init global vars
	g_dec_ref_pic_marking_buffer_size = 0;
	g_ready_to_decode_slice = FALSE;
	g_searching_IDR_pic = 1;
	g_nFrame_dec_h264 = 0;
	g_firstBsm_init_h264 = TRUE;
	g_old_pps_id = -1; //initialize to a impossible value
	last_dquant = 0;

	for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		g_list[0][i] = g_no_reference_picture_ptr;
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
	g_image_ptr->not_supported = FALSE;
	g_image_ptr->is_need_init_vsp_hufftab = TRUE;
	
	g_no_reference_picture_ptr->used_for_reference = 0;
	g_no_reference_picture_ptr->is_long_term = 0;
	g_no_reference_picture_ptr->frame_num = 0;
	g_no_reference_picture_ptr->pic_num = 0;
	g_no_reference_picture_ptr->long_term_pic_num = 0;
	g_no_reference_picture_ptr->long_term_frame_idx = 0;

	g_active_pps_ptr = NULL;
	g_active_sps_ptr = NULL;

	g_MbToSliceGroupMap = NULL;

	//init old slice
	H264Dec_init_old_slice (g_old_slice_ptr);

	return;
}

PUBLIC void H264Dec_VSPInit (void)
{
	int cmd;


	VSP_Reset();

   	 /*clear time_out int_raw flag, if timeout occurs*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");
        	
    	/*init dcam command*/
   	 VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

   	 cmd = (1 << 16) |((uint32)0xffff);
   	 VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: enable hardware timer out");
	
	return;
}

PUBLIC void H264Dec_init_vld_table (void)
{
	/*total Zero Chroma DC*/
	g_totZero_Chroma_DC [0] = g_totZero_Chroma_DC1_tbl;
	g_totZero_Chroma_DC [1] = g_totZero_Chroma_DC2_tbl;
	g_totZero_Chroma_DC [2] = g_totZero_Chroma_DC3_tbl;

	/*run before*/
	g_run_zeroLeft [0] = g_run_zeroLeft1_tbl;
	g_run_zeroLeft [1] = g_run_zeroLeft2_tbl;
	g_run_zeroLeft [2] = g_run_zeroLeft3_tbl;
	g_run_zeroLeft [3] = g_run_zeroLeft4_tbl;
	g_run_zeroLeft [4] = g_run_zeroLeft5_tbl;
	g_run_zeroLeft [5] = g_run_zeroLeft6_tbl;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
