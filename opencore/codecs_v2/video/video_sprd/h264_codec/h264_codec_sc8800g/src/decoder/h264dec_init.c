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
#include "sc8800g_video_header.h"
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

#if _CMODEL_
	g_stream_offset = 0;
	VSP_Init_CModel();
#endif //_CMODEL_

	//SCI_ASSERT(NULL != (g_sps_array_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T) * MAX_SPS)));
	g_sps_array_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T) * MAX_SPS);
	//SCI_ASSERT(NULL != (g_pps_array_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T) * MAX_PPS)));
        g_pps_array_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T) * MAX_PPS);
	//SCI_ASSERT(NULL != (g_sps_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T))));
	g_sps_ptr = (DEC_SPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_SPS_T));
	//SCI_ASSERT(NULL != (g_pps_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T))));
	g_pps_ptr = (DEC_PPS_T *)H264Dec_InterMemAlloc (sizeof(DEC_PPS_T));
	//SCI_ASSERT(NULL != (g_dpb_ptr = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc (sizeof(DEC_DECODED_PICTURE_BUFFER_T))));
	g_dpb_ptr = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_InterMemAlloc (sizeof(DEC_DECODED_PICTURE_BUFFER_T));
#ifdef _VSP_LINUX_
	g_dpb_ptr->used_size = 0;
#endif
	g_active_sps_ptr = NULL;
	g_active_pps_ptr = NULL;

	//SCI_ASSERT(NULL != (g_old_slice_ptr = (DEC_OLD_SLICE_PARAMS_T *)H264Dec_InterMemAlloc (sizeof(DEC_OLD_SLICE_PARAMS_T))));
	g_old_slice_ptr = (DEC_OLD_SLICE_PARAMS_T *)H264Dec_InterMemAlloc (sizeof(DEC_OLD_SLICE_PARAMS_T));
	//SCI_ASSERT(NULL != (g_nalu_ptr = (DEC_NALU_T *)H264Dec_InterMemAlloc (sizeof(DEC_NALU_T))));
	g_nalu_ptr = (DEC_NALU_T *)H264Dec_InterMemAlloc (sizeof(DEC_NALU_T));
// 	SCI_ASSERT(NULL != (g_nalu_ptr->buf = (uint8 *)H264Dec_InterMemAlloc (sizeof(uint8)*NALU_BUFFER_SIZE)));

	//SCI_ASSERT(NULL != (g_image_ptr = (DEC_IMAGE_PARAMS_T *)H264Dec_InterMemAlloc(sizeof(DEC_IMAGE_PARAMS_T))));
	g_image_ptr = (DEC_IMAGE_PARAMS_T *)H264Dec_InterMemAlloc(sizeof(DEC_IMAGE_PARAMS_T));
	//SCI_ASSERT(NULL != (g_curr_slice_ptr = (DEC_SLICE_T *)H264Dec_InterMemAlloc(sizeof(DEC_SLICE_T)))); 
	g_curr_slice_ptr = (DEC_SLICE_T *)H264Dec_InterMemAlloc(sizeof(DEC_SLICE_T));

	//SCI_ASSERT(NULL != (g_no_reference_picture_ptr = (DEC_STORABLE_PICTURE_T *)H264Dec_InterMemAlloc(sizeof(DEC_STORABLE_PICTURE_T))));
	g_no_reference_picture_ptr = (DEC_STORABLE_PICTURE_T *)H264Dec_InterMemAlloc(sizeof(DEC_STORABLE_PICTURE_T));

	//SCI_ASSERT(NULL != (g_mb_cache_ptr = (DEC_MB_CACHE_T *)H264Dec_InterMemAlloc(sizeof(DEC_MB_CACHE_T))));
	g_mb_cache_ptr = (DEC_MB_CACHE_T *)H264Dec_InterMemAlloc(sizeof(DEC_MB_CACHE_T));

	g_mb_cache_ptr->b8_mv_pred_func[0] = H264Dec_mv_prediction_P8x8_8x8;
	g_mb_cache_ptr->b8_mv_pred_func[1] = H264Dec_mv_prediction_P8x8_8x4;
	g_mb_cache_ptr->b8_mv_pred_func[2] = H264Dec_mv_prediction_P8x8_4x8;
	g_mb_cache_ptr->b8_mv_pred_func[3] = H264Dec_mv_prediction_P8x8_4x4;

	//init global vars
	g_dec_ref_pic_marking_buffer_size = 0;
	g_ready_to_decode_slice = FALSE;
	g_searching_IDR_pic = 1;
	g_nFrame_dec_h264 = 0;
	g_firstBsm_init_h264 = TRUE;
	g_old_pps_id = -1; //initialize to a impossible value

	for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
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
    g_image_ptr->return_pos = 0;
    g_image_ptr->return_pos2 = 0;    
	g_image_ptr->size_decode_flag = FALSE;
	
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
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, (1<<12), "DCAM_INT_CLR: clear time_out int_raw flag");
	
	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");
	
	/*init vsp command*/
	cmd = (0<<16) |(1<<15) | (0<<14) |(1<<12) | (H264<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0");
	
	cmd = (0 << 16) |((uint32)0xffff);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");
	
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, 6, "configure dbk free run mode and enable filter");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, 1, "configure dbk ping-pang buffer0 enable");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1, "configure dbk control flag");

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