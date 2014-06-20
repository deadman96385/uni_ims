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
#include "h264dec_video_header.h"
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
#if _MVC_
    old_slice_ptr->view_id=(int8)0xff;//weihu
    old_slice_ptr->inter_view_flag=(int8)0xff;
    old_slice_ptr->anchor_pic_flag=(int8)0xff;
#endif
    return;
}

PUBLIC MMDecRet H264Dec_init_global_para (H264DecObject*vo)
{
    int32 i,j;

    vo->g_sps_array_ptr = (DEC_SPS_T *)H264Dec_MemAlloc (vo, sizeof(DEC_SPS_T) * MAX_SPS, 4, INTER_MEM);
    CHECK_MALLOC(vo->g_sps_array_ptr, "vo->g_sps_array_ptr");

    vo->g_pps_array_ptr = (DEC_PPS_T *)H264Dec_MemAlloc (vo, sizeof(DEC_PPS_T) * MAX_PPS, 4, INTER_MEM);
    CHECK_MALLOC(vo->g_pps_array_ptr, "vo->g_pps_array_ptr");

    vo->g_sps_ptr = (DEC_SPS_T *)H264Dec_MemAlloc (vo, sizeof(DEC_SPS_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_sps_ptr, "vo->g_sps_ptr");

    vo->g_sps_ptr->vui_seq_parameters = (DEC_VUI_T *)H264Dec_MemAlloc (vo, sizeof(DEC_VUI_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_sps_ptr->vui_seq_parameters, "vo->g_sps_ptr->vui_seq_parameters");

    vo->g_pps_ptr = (DEC_PPS_T *)H264Dec_MemAlloc (vo, sizeof(DEC_PPS_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_pps_ptr, "vo->g_pps_ptr");

#if _MVC_
    for (i = 0; i < 2; i++)
    {
        vo->g_dpb_layer[i] = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_MemAlloc (vo, sizeof(DEC_DECODED_PICTURE_BUFFER_T), 4, INTER_MEM);
        CHECK_MALLOC(vo->g_dpb_layer[i], "vo->g_dpb_layer[i]");

        vo->g_dpb_layer[i]->init_done=0;
        vo->g_dpb_layer[i]->used_size=0;
    }
#else
    g_dpb_ptr = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_MemAlloc (vo, sizeof(DEC_DECODED_PICTURE_BUFFER_T), 4, INTER_MEM);
    CHECK_MALLOC(g_dpb_ptr, "g_dpb_ptr");
#endif
    vo->g_active_sps_ptr = NULL;
    vo->g_active_pps_ptr = NULL;

    vo->g_old_slice_ptr = (DEC_OLD_SLICE_PARAMS_T *)H264Dec_MemAlloc (vo, sizeof(DEC_OLD_SLICE_PARAMS_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_old_slice_ptr, "vo->g_old_slice_ptr");

    vo->g_nalu_ptr = (DEC_NALU_T *)H264Dec_MemAlloc (vo, sizeof(DEC_NALU_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_nalu_ptr, "vo->g_nalu_ptr");

    vo->g_image_ptr = (DEC_IMAGE_PARAMS_T *)H264Dec_MemAlloc(vo, sizeof(DEC_IMAGE_PARAMS_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_image_ptr, "vo->g_image_ptr");

    vo->g_curr_slice_ptr = (DEC_SLICE_T *)H264Dec_MemAlloc(vo, sizeof(DEC_SLICE_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_curr_slice_ptr, "vo->g_curr_slice_ptr");

    vo->g_curr_slice_ptr->mot_ctx = (MotionInfoContexts *)H264Dec_MemAlloc(vo, sizeof(MotionInfoContexts), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_curr_slice_ptr->mot_ctx, "vo->g_curr_slice_ptr->mot_ctx");

    vo->g_curr_slice_ptr->tex_ctx = (TextureInfoContexts *)H264Dec_MemAlloc(vo, sizeof(TextureInfoContexts), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_curr_slice_ptr->tex_ctx, "vo->g_curr_slice_ptr->tex_ctx");

#if _MVC_
    vo->g_curr_slice_ptr->p_Dpb = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_MemAlloc(vo, sizeof(DEC_DECODED_PICTURE_BUFFER_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_curr_slice_ptr->p_Dpb, "vo->g_curr_slice_ptr->p_Dpb");
#endif

    vo->g_no_reference_picture_ptr = (DEC_STORABLE_PICTURE_T *)H264Dec_MemAlloc(vo, sizeof(DEC_STORABLE_PICTURE_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_no_reference_picture_ptr, "vo->g_no_reference_picture_ptr");

    //init global vars
    vo->memory_error = 0;
    vo->g_dec_ref_pic_marking_buffer_size = 0;
    vo->g_ready_to_decode_slice = FALSE;
    vo->g_searching_IDR_pic = 1;
    vo->g_nFrame_dec_h264 = 0;
    vo->g_old_pps_id = -1; //initialize to a impossible value
#if _MVC_
    vo->last_profile_idc=0;
#endif

    for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
    {
        vo->g_list0_map_addr[i] = 0;
        vo->g_list1_map_addr[i] = 0;
    }

    for (i=0; i<2; i++)
    {
        for (j=0; j<16; j++)
        {
            VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+136+(2*j+32*i)*4,0,"clr g_wp_weight");
            VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+140+(2*j+32*i)*4,0,"clr g_wp_offset");

        }
    }
    for(i=3; i<6; i++) //inter
    {
        for(j=0; j<4; j++)
        {
            VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER4x4Y_OFF+(i-3)*16+j*4,0,"clr weightscale inter4x4");
        }
    }

    for(i=0; i<3; i++) //intra
    {
        for(j=0; j<4; j++)
        {
            VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA4x4Y_OFF+i*16+j*4,0,"clr weightscale intra4x4");
        }
    }

    for(j=0; j<8; j++)
    {
        VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+2*j*4,0," clr weightscale inter8x8");
        VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+(2*j+1)*4,0," clr weightscale inter8x8");
        VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+2*j*4,0," clr weightscale inter8x8");
        VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+(2*j+1)*4,0," clr weightscale inter8x8");
    }

    vo->g_no_reference_picture_ptr->used_for_reference = 0;
    vo->g_no_reference_picture_ptr->is_long_term = 0;
    vo->g_no_reference_picture_ptr->frame_num = 0;
    vo->g_no_reference_picture_ptr->pic_num = 0;
    vo->g_no_reference_picture_ptr->long_term_pic_num = 0;
    vo->g_no_reference_picture_ptr->long_term_frame_idx = 0;
    vo->g_no_reference_picture_ptr->poc=0;
    vo->g_no_reference_picture_ptr->imgYAddr=0;
    vo->g_no_reference_picture_ptr->imgUAddr=0;
    vo->g_no_reference_picture_ptr->imgVAddr=0;
    vo->g_no_reference_picture_ptr->DPB_addr_index=0x3f;

    for (i = 0; i < (MAX_REF_FRAME_NUMBER+MAX_REF_FRAME_NUMBER); i++)//weihu//+1
    {
        vo->g_list0[i] = vo->g_no_reference_picture_ptr;
    }

    vo->g_image_ptr->dec_ref_pic_marking_buffer = vo->g_dec_ref_pic_marking_buffer;

    for (i = 0; i < MAX_SPS; i++)
    {
        vo->g_sps_array_ptr[i].valid = FALSE;
    }

    for (i = 0; i < MAX_PPS; i++)
    {
        vo->g_pps_array_ptr[i].valid = FALSE;
    }

    vo->g_image_ptr->low_delay = 1;
    vo->g_image_ptr->has_b_frames =  !vo->g_image_ptr->low_delay;
    vo->g_image_ptr->curr_slice_ptr = vo->g_curr_slice_ptr;
    vo->g_image_ptr->curr_mb_nr = 0;
    vo->error_flag = 0;
    vo->g_image_ptr->no_output_of_prior_pics_flag=0;//weihu
    vo->g_image_ptr->profile_idc=0;
    vo->is_need_init_vsp_hufftab = TRUE;

    vo->g_active_pps_ptr = NULL;
    vo->g_active_sps_ptr = NULL;
    vo->g_dec_picture_ptr = NULL;

    vo->g_MbToSliceGroupMap = NULL;

    //init old slice
    H264Dec_init_old_slice (vo->g_old_slice_ptr);

#if _MVC_
    vo->g_active_subset_sps = NULL;
    vo->g_curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU=NULL;//weihu
    vo->g_curr_slice_ptr->inter_view_flag= NULL;
    vo->g_curr_slice_ptr->anchor_pic_flag= NULL;
    init_subset_sps_list(vo->g_SubsetSeqParSet, MAX_SPS);

    vo->DecodeAllLayers = 0;
#endif

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
