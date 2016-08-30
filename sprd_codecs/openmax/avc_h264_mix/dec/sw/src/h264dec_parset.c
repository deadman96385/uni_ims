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
#include "h264dec_video_header.h"
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

uint32 H264Dec_CalculateMemSize (H264DecContext *vo)
{
    int32 Frm_width_align = ((vo->width + 15) & (~15));
    int32 Frm_height_align = ((vo->height + 15) & (~15));
    int32 mb_num_x = Frm_width_align/16;
    int32 mb_num_y = Frm_height_align/16;
    int32 mb_num_total = mb_num_x * mb_num_y;
    uint32 size_extra;
    uint32 frame_num = 2;

    if (vo->yuv_format == YUV420SP_NV12 || vo->yuv_format == YUV420SP_NV21) {
        frame_num = MAX_REF_FRAME_NUMBER+1;
    }

    size_extra = 24*16*2 /*g_halfPixTemp*/
                 + (2+mb_num_y)*mb_num_x*8 /*MB_INFO*/
                 + (mb_num_total*16) /*i4x4pred_mode_ptr*/
                 + (mb_num_total*16) /*direct_ptr*/
                 + (mb_num_total*24) /*nnz_ptr*/
                 + (mb_num_total*2*16*2*2) /*mvd*/
                 + (mb_num_total*4) /*slice_nr_ptr*/
                 + 3*4*(MAX_REF_FRAME_NUMBER+1) /*fs, fs_ref, fs_ltref*/
                 + (MAX_REF_FRAME_NUMBER+1)*(7*4+(27+150*2*17)*4+mb_num_total*16*(2*2*2 + 1 + 1 + 4 + 4)) /*dpb_ptr*/
                 + frame_num *((mb_num_x*16+48)*(mb_num_y*16+48)*3/2)
                 + mb_num_total /*g_MbToSliceGroupMap*/
                 + 10*1024; //rsv

    return size_extra;
}

/*
if sps_id is changed, size of frame may be changed, so the buffer for dpb and img
need to be re-allocate, and the parameter of img need to be re-computed
*/
LOCAL void H264Dec_active_sps (DEC_SPS_T *sps_ptr, H264DecContext *vo)
{
    vo->max_frame_num = (1<<(sps_ptr->log2_max_frame_num_minus4+4));
    vo->frame_width_in_mbs = (sps_ptr->pic_width_in_mbs_minus1+1);
    vo->pic_height_in_map_units = (sps_ptr->pic_height_in_map_units_minus1+1);
    vo->frame_height_in_mbs = (2-(uint8)sps_ptr->frame_mbs_only_flag)*vo->pic_height_in_map_units;
    vo->frame_size_in_mbs = vo->frame_width_in_mbs*vo->frame_height_in_mbs;
    vo->width = vo->frame_width_in_mbs * MB_SIZE;
    vo->height = vo->frame_height_in_mbs * MB_SIZE;
    vo->b4_pitch = (vo->frame_width_in_mbs << 2);

    {
        DEC_MB_CACHE_T *mb_cache_ptr = vo->g_mb_cache_ptr;
        int32 blk8x8Idx, blk4x4Idx = 0;
        int32 offset;

        vo->ext_width  = vo->width + Y_EXTEND_SIZE * 2;
        vo->ext_height = vo->height + Y_EXTEND_SIZE * 2;
        vo->start_in_frameY = vo->ext_width * Y_EXTEND_SIZE + Y_EXTEND_SIZE;
        vo->start_in_frameUV = (vo->ext_width>>1) * UV_EXTEND_SIZE + UV_EXTEND_SIZE;

        //y
        for (blk8x8Idx = 0; blk8x8Idx < 4; blk8x8Idx++) {
            offset = (((blk8x8Idx >> 1) *vo->ext_width+ (blk8x8Idx & 1) )<<3);
            mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset;
            mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset + 4;
            mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset + 4 * vo->ext_width;
            mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = offset + 4 + 4 * vo->ext_width;
        }

        //uv
        mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 0;
        mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 4;
        mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 4 * vo->ext_width/2;
        mb_cache_ptr->blk4x4_offset[blk4x4Idx++] = 4 + 4 * vo->ext_width/2;
    }

    if (vo->g_active_sps_ptr != sps_ptr) {
        vo->g_active_sps_ptr = sps_ptr;
        vo->g_MbToSliceGroupMap = NULL;

        if(vo->avcHandle->VSP_extMemCb) {
            uint32 size_extra;
            int ret;

            size_extra = H264Dec_CalculateMemSize(vo);
            ret = (*(vo->avcHandle->VSP_extMemCb))(vo->avcHandle->userdata, size_extra);
            if(ret < 0) {
#if _H264_PROTECT_ & _LEVEL_LOW_
                vo->error_flag |= ER_EXTRA_MEMO_ID;
                vo->return_pos1 |= (1<<13);
#endif
                return;
            }
        } else {
#if _H264_PROTECT_ & _LEVEL_LOW_
            vo->error_flag |= ER_BSM_ID;
            vo->return_pos1 |= (1<<14);
#endif
            return;
        }

        //Added for bug333874
        H264Dec_clear_delayed_buffer(vo);
        if (!vo->no_output_of_prior_pics_flag) {
            H264Dec_flush_dpb(vo, vo->g_dpb_ptr);
        }

        //reset memory alloc
        H264Dec_FreeExtraMem(vo);

        if (H264Dec_init_img_buffer (vo) != MMDEC_OK) {
            vo->error_flag |= ER_EXTRA_MEMO_ID;
            vo->return_pos2 |= (1<<27);
            return;
        }

        if (H264Dec_init_dpb (vo) != MMDEC_OK) {
            vo->error_flag |= ER_EXTRA_MEMO_ID;
            vo->return_pos2 |= (1<<28);
            return;
        }

        vo->g_dpb_ptr->num_ref_frames = vo->g_active_sps_ptr->num_ref_frames;
    }

    return;
}

LOCAL void H264Dec_active_pps (H264DecContext *vo, DEC_PPS_T *pps_ptr)
{
    if (vo->g_active_pps_ptr != pps_ptr) {
        vo->g_active_pps_ptr = pps_ptr;
    }
}

PUBLIC void H264Dec_use_parameter_set (H264DecContext *vo, int32 pps_id)
{
    DEC_PPS_T *pps_ptr = &(vo->g_pps_array_ptr[pps_id]);
    DEC_SPS_T *sps_ptr = &(vo->g_sps_array_ptr[pps_ptr->seq_parameter_set_id]);

    if (sps_ptr->valid && pps_ptr->valid) {
        H264Dec_active_sps (sps_ptr, vo);

#if _H264_PROTECT_ & _LEVEL_LOW_
        if(vo->error_flag) {
            vo->return_pos1 |= (1<<15);
            return;
        }
#endif
        H264Dec_active_pps (vo, pps_ptr);
    } else {
#if _H264_PROTECT_ & _LEVEL_LOW_
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<16);
#endif
        vo->g_ready_to_decode_slice = FALSE;
    }

    if (!vo->is_cabac) {//UVLC
        vo->nal_startcode_follows = uvlc_startcode_follows;
        vo->readRefFrame = H264Dec_get_te;
        vo->decode_mvd_xy = decode_cavlc_mb_mvd;
        vo->read_mb_type = decode_cavlc_mb_type;
        vo->read_b8mode = readB8_typeInfo_cavlc;
        vo->sw_vld_mb= decode_mb_cavlc;
    } else {
        vo->nal_startcode_follows = get_cabac_terminate;
        vo->readRefFrame = decode_cabac_mb_ref;
        vo->decode_mvd_xy = decode_cabac_mb_mvd;
        vo->read_mb_type = decode_cabac_mb_type;
        vo->read_b8mode = decode_cabac_mb_sub_type;
        vo->sw_vld_mb= decode_mb_cabac;
    }

    return;
}

PUBLIC void H264Dec_interpret_sei_message (void)
{
    //TBD
    return;
}

LOCAL int32 H264Dec_ReadHRDParameters(DEC_BS_T *bs_ptr, DEC_HRD_PARAM_T *hrd_ptr)
{
    uint32 SchedSelIdx;

    hrd_ptr->cpb_cnt_minus1 = UE_V();
    hrd_ptr->bit_rate_scale = READ_FLC(4);
    hrd_ptr->cpb_size_scale = READ_FLC(4);

    for(SchedSelIdx = 0; SchedSelIdx <= hrd_ptr->cpb_cnt_minus1; SchedSelIdx++) {
        hrd_ptr->bit_rate_value_minus1[ SchedSelIdx ]  = UE_V();
        hrd_ptr->cpb_size_value_minus1[ SchedSelIdx ] = Long_UE_V();
        hrd_ptr->cbr_flag[ SchedSelIdx ]                         = READ_FLAG();
    }

    hrd_ptr->initial_cpb_removal_delay_length_minus1  = READ_FLC(5);
    hrd_ptr->cpb_removal_delay_length_minus1            = READ_FLC(5);
    hrd_ptr->dpb_output_delay_length_minus1              = READ_FLC(5);
    hrd_ptr->time_offset_length                                 = READ_FLC(5);

    return 0;
}

LOCAL void H264Dec_ReadVUI (DEC_VUI_T *vui_seq_parameters_ptr, DEC_BS_T *bs_ptr)
{
    vui_seq_parameters_ptr->aspect_ratio_info_present_flag = READ_FLAG();
    if (vui_seq_parameters_ptr->aspect_ratio_info_present_flag) {
        vui_seq_parameters_ptr->aspect_ratio_idc = READ_FLC(8);
        if (255 == vui_seq_parameters_ptr->aspect_ratio_idc) {
            vui_seq_parameters_ptr->sar_width = READ_FLC(16);
            vui_seq_parameters_ptr->sar_height = READ_FLC(16);
        }
    }

    vui_seq_parameters_ptr->overscan_info_present_flag     = READ_FLAG();
    if (vui_seq_parameters_ptr->overscan_info_present_flag) {
        vui_seq_parameters_ptr->overscan_appropriate_flag    = READ_FLAG();
    }

    vui_seq_parameters_ptr->video_signal_type_present_flag = READ_FLAG();
    if (vui_seq_parameters_ptr->video_signal_type_present_flag) {
        vui_seq_parameters_ptr->video_format                    = READ_FLC(3);
        vui_seq_parameters_ptr->video_full_range_flag           = READ_FLAG ();
        vui_seq_parameters_ptr->colour_description_present_flag = READ_FLAG ();
        if(vui_seq_parameters_ptr->colour_description_present_flag) {
            vui_seq_parameters_ptr->colour_primaries              = READ_FLC(8);
            vui_seq_parameters_ptr->transfer_characteristics      = READ_FLC(8);
            vui_seq_parameters_ptr->matrix_coefficients           = READ_FLC(8);
        }
    }

    vui_seq_parameters_ptr->chroma_location_info_present_flag = READ_FLAG ();
    if(vui_seq_parameters_ptr->chroma_location_info_present_flag) {
        vui_seq_parameters_ptr->chroma_sample_loc_type_top_field     = UE_V();
        vui_seq_parameters_ptr->chroma_sample_loc_type_bottom_field  = UE_V();
    }

    vui_seq_parameters_ptr->timing_info_present_flag          = READ_FLAG();
    if (vui_seq_parameters_ptr->timing_info_present_flag) {
        vui_seq_parameters_ptr->num_units_in_tick               = READ_FLC(32);
        vui_seq_parameters_ptr->time_scale                      = READ_FLC(32);
        vui_seq_parameters_ptr->fixed_frame_rate_flag           = READ_FLAG ();
    }

    vui_seq_parameters_ptr->nal_hrd_parameters_present_flag   = READ_FLAG ();
    if (vui_seq_parameters_ptr->nal_hrd_parameters_present_flag) {
        H264Dec_ReadHRDParameters(bs_ptr, &(vui_seq_parameters_ptr->nal_hrd_parameters));
    }

    vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag   = READ_FLAG ();
    if (vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag) {
        H264Dec_ReadHRDParameters(bs_ptr, &(vui_seq_parameters_ptr->vcl_hrd_parameters));
    }

    if (vui_seq_parameters_ptr->nal_hrd_parameters_present_flag || vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag) {
        vui_seq_parameters_ptr->low_delay_hrd_flag             =  READ_FLAG ();
    }

    vui_seq_parameters_ptr->pic_struct_present_flag          =  READ_FLAG ();
    vui_seq_parameters_ptr->bitstream_restriction_flag       =  READ_FLAG ();
    if (vui_seq_parameters_ptr->bitstream_restriction_flag) {
        vui_seq_parameters_ptr->motion_vectors_over_pic_boundaries_flag =  READ_FLAG ();
        vui_seq_parameters_ptr->max_bytes_per_pic_denom                 =  UE_V();
        vui_seq_parameters_ptr->max_bits_per_mb_denom                   =  UE_V();
        vui_seq_parameters_ptr->log2_max_mv_length_horizontal           =  UE_V();
        vui_seq_parameters_ptr->log2_max_mv_length_vertical             =  UE_V();
        vui_seq_parameters_ptr->num_reorder_frames                      =  UE_V();
        vui_seq_parameters_ptr->max_dec_frame_buffering                 =  UE_V();
    }

    return;
}

LOCAL void decode_scaling_list(DEC_BS_T *bs_ptr, uint8 *factors, int size,
                               const uint8 *jvt_list, const uint8 *fallback_list) {
    int i, last = 8, next = 8;
    const uint8 *scan = size == 16 ? g_inverse_zigzag_tbl : g_inverse_8x8_zigzag_tbl;
    if(!READ_FLC(1)) {/* matrix not written, we use the predicted one */
        memcpy(factors, fallback_list, size*sizeof(uint8));
    } else {
        for(i=0; i<size; i++) {
            if(next) {
                next = (last + SE_V()) & 0xff;
            }

            if(!i && !next) { /* matrix not written, we use the preset one */
                memcpy(factors, jvt_list, size*sizeof(uint8));
                break;
            }
            last = factors[scan[i]] = next ? next : last;
        }
    }
}

LOCAL void decode_scaling_matrices(DEC_BS_T *bs_ptr, DEC_SPS_T *sps, DEC_PPS_T *pps, int is_sps,
                                   uint8 (*scaling_matrix4)[16], uint8 (*scaling_matrix8)[64]) {
    int fallback_sps = !is_sps && sps->seq_scaling_matrix_present_flag;
    const uint8 *fallback[4] = {
        fallback_sps ? sps->ScalingList4x4[0] : weightscale4x4_intra_default,
        fallback_sps ? sps->ScalingList4x4[3] : weightscale4x4_inter_default,
        fallback_sps ? sps->ScalingList8x8[0] : weightscale8x8_intra_default,
        fallback_sps ? sps->ScalingList8x8[1] : weightscale8x8_inter_default
    };

    decode_scaling_list(bs_ptr, scaling_matrix4[0], 16, weightscale4x4_intra_default, fallback[0]); // Intra, Y
    decode_scaling_list(bs_ptr, scaling_matrix4[1], 16, weightscale4x4_intra_default, scaling_matrix4[0]); // Intra, Cr
    decode_scaling_list(bs_ptr, scaling_matrix4[2], 16, weightscale4x4_intra_default, scaling_matrix4[1]); // Intra, Cb
    decode_scaling_list(bs_ptr, scaling_matrix4[3], 16, weightscale4x4_inter_default, fallback[1]); // Inter, Y
    decode_scaling_list(bs_ptr, scaling_matrix4[4], 16, weightscale4x4_inter_default, scaling_matrix4[3]); // Inter, Cr
    decode_scaling_list(bs_ptr, scaling_matrix4[5], 16, weightscale4x4_inter_default, scaling_matrix4[4]); // Inter, Cb
    if(is_sps || pps->transform_8x8_mode_flag) {
        decode_scaling_list(bs_ptr, scaling_matrix8[0], 64, weightscale8x8_intra_default,fallback[2]);  // Intra, Y
        decode_scaling_list(bs_ptr, scaling_matrix8[1], 64, weightscale8x8_inter_default,fallback[3]);  // Inter, Y
    }
}

LOCAL MMDecRet H264Dec_interpret_sps (DEC_SPS_T *sps_ptr, H264DecContext *vo)
{
    int32 reserved_zero;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    sps_ptr->profile_idc = READ_FLC(8);

    sps_ptr->constrained_set0_flag = READ_FLAG();
    sps_ptr->constrained_set1_flag = READ_FLAG();
    sps_ptr->constrained_set2_flag = READ_FLAG();
    sps_ptr->constrained_set3_flag = READ_FLAG();

    reserved_zero = READ_FLC(4);

    sps_ptr->level_idc = READ_FLC(8);
    sps_ptr->seq_parameter_set_id = UE_V();

    if (sps_ptr->profile_idc == 0x64) {//hp
        sps_ptr->chroma_format_idc = UE_V();
        if ((sps_ptr->chroma_format_idc != 1)) {
            vo->error_flag |= ER_BSM_ID;
        }

        sps_ptr->bit_depth_luma_minus8 = UE_V();
        sps_ptr->bit_depth_chroma_minus8 = UE_V();
        sps_ptr->qpprime_y_zero_transform_bypass_flag = READ_FLC(1);
        sps_ptr->seq_scaling_matrix_present_flag =   READ_FLAG();
        if(sps_ptr->seq_scaling_matrix_present_flag) {
            decode_scaling_matrices(bs_ptr, sps_ptr, NULL, 1, sps_ptr->ScalingList4x4,sps_ptr->ScalingList8x8);
        }
    }

    sps_ptr->log2_max_frame_num_minus4 = UE_V();
    sps_ptr->pic_order_cnt_type = UE_V();

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (sps_ptr->seq_parameter_set_id < 0 || sps_ptr->log2_max_frame_num_minus4 < 0 ||
            sps_ptr->log2_max_frame_num_minus4 > 12 || sps_ptr->pic_order_cnt_type < 0 ||
            sps_ptr->pic_order_cnt_type > 2) {
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<18);
        return MMDEC_STREAM_ERROR;
    }
#endif

    if (sps_ptr->pic_order_cnt_type == 0) {
        sps_ptr->log2_max_pic_order_cnt_lsb_minus4 = UE_V();
    } else if (sps_ptr->pic_order_cnt_type == 1) {
        int32 i;

        sps_ptr->delta_pic_order_always_zero_flag = READ_FLAG();
        sps_ptr->offset_for_non_ref_pic = Long_SE_V();
        sps_ptr->offset_for_top_to_bottom_field = Long_SE_V();
        sps_ptr->num_ref_frames_in_pic_order_cnt_cycle = UE_V();

#if _H264_PROTECT_ & _LEVEL_LOW_
        if (sps_ptr->num_ref_frames_in_pic_order_cnt_cycle > 255) {
            vo->error_flag |= ER_BSM_ID;
            vo->return_pos1 |= (1<<19);
            return MMDEC_STREAM_ERROR;
        }
#endif
        for (i = 0; i < (int32)(sps_ptr->num_ref_frames_in_pic_order_cnt_cycle); i++) {
            sps_ptr->offset_for_ref_frame[i] = Long_SE_V();
        }
    }

    sps_ptr->num_ref_frames = UE_V();
#if _H264_PROTECT_ & _LEVEL_LOW_
    if (sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER) {
        vo->error_flag |= ER_REF_FRM_ID;
        vo->return_pos1 |= (1<<20);
        PRINTF ("sps_ptr->num_ref_frames > MAX_REF_FRAME_NUMBER");
    }
#endif

    sps_ptr->gaps_in_frame_num_value_allowed_flag = READ_FLAG();
    sps_ptr->pic_width_in_mbs_minus1 = UE_V();
    sps_ptr->pic_height_in_map_units_minus1 = UE_V();

    if ((sps_ptr->profile_idc != 0x42) && (sps_ptr->profile_idc != 0x4d) && (sps_ptr->profile_idc != 0x64)) {//0x42: bp, 0x4d: mp, 0x64: hp
        vo->not_supported = TRUE;
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<17);
        return MMDEC_STREAM_ERROR;
    } else {
        vo->b8_mv_pred_func[4] = H264Dec_mv_prediction_P8x8_8x8;
        vo->b8_mv_pred_func[5] = H264Dec_mv_prediction_P8x8_8x4;
        vo->b8_mv_pred_func[6] = H264Dec_mv_prediction_P8x8_4x8;
        vo->b8_mv_pred_func[7] = H264Dec_mv_prediction_P8x8_4x4;

        H264Dec_setClipTab ();
        H264Dec_init_mc_function ();
        H264Dec_init_intra4x4Pred_function ();
        H264Dec_init_intra8x8Pred_function();
        H264Dec_init_intra16x16Pred_function ();
        H264Dec_init_intraChromaPred_function ();
    }

    sps_ptr->frame_mbs_only_flag = READ_FLAG();

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (!sps_ptr->frame_mbs_only_flag) {
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<22);
        sps_ptr->mb_adaptive_frame_field_flag = READ_FLAG();
        if (sps_ptr->mb_adaptive_frame_field_flag) {
            PRINTF("MBAFF is not supported!\n");
        }
        return MMDEC_STREAM_ERROR;
    }
#endif

    sps_ptr->direct_8x8_inference_flag = READ_FLAG();
    sps_ptr->frame_cropping_flag = READ_FLAG();

    if (sps_ptr->frame_cropping_flag == 1) {
        sps_ptr->frame_crop_left_offset = UE_V();
        sps_ptr->frame_crop_right_offset = UE_V();
        sps_ptr->frame_crop_top_offset = UE_V();
        sps_ptr->frame_crop_bottom_offset = UE_V();
    } else {
        sps_ptr->frame_crop_left_offset = 0;
        sps_ptr->frame_crop_right_offset = 0;
        sps_ptr->frame_crop_top_offset = 0;
        sps_ptr->frame_crop_bottom_offset = 0;
    }

    sps_ptr->vui_parameters_present_flag = READ_FLAG();
    if (sps_ptr->vui_parameters_present_flag) {
        H264Dec_ReadVUI (sps_ptr->vui_seq_parameters, bs_ptr);
    }

    sps_ptr->valid = TRUE;

    return MMDEC_OK;
}

LOCAL void H264Dec_make_sps_availabe (H264DecContext *vo, int32 sps_id, DEC_SPS_T *sps_ptr)
{
    int32 *src_ptr = (int32 *)sps_ptr;
    int32 *dst_ptr = (int32 *)(&vo->g_sps_array_ptr[sps_id]);
    uint32 sps_size = sizeof(DEC_SPS_T)/4;
#if 0
    int32 i;
    for (i = 0; i < sps_size; i++) {
        *dst_ptr++ = *src_ptr++;
    }
#else
    memcpy(dst_ptr, src_ptr, sizeof(DEC_SPS_T));
#endif

    return;
}

PUBLIC MMDecRet H264Dec_process_sps (H264DecContext *vo)
{
    DEC_SPS_T *sps_ptr = vo->g_sps_ptr;
    MMDecRet ret;

    memset(sps_ptr, 0, sizeof(DEC_SPS_T)-(6*16+2*64+4));
    memset(sps_ptr->ScalingList4x4,16,6*16);
    memset(sps_ptr->ScalingList8x8,16,2*64);

    ret = H264Dec_interpret_sps (sps_ptr, vo);
    if (ret != MMDEC_OK) {
        return ret;
    }

    H264Dec_make_sps_availabe (vo, sps_ptr->seq_parameter_set_id, sps_ptr);

    if (vo->g_active_sps_ptr && (sps_ptr->seq_parameter_set_id == vo->g_active_sps_ptr->seq_parameter_set_id)) {
        vo->g_old_pps_id = -1;
    }
    vo->g_active_sps_ptr = NULL;

    vo->profile_idc = sps_ptr->profile_idc;
    vo->low_delay = 1;
    vo->has_b_frames = !vo->low_delay;

    return MMDEC_OK;
}

LOCAL MMDecRet H264Dec_interpret_pps (DEC_PPS_T *pps_ptr, H264DecContext *vo)
{
    int32 i;
    int32 NumberBitsPerSliceGroupId;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    pps_ptr->pic_parameter_set_id = UE_V();
    pps_ptr->seq_parameter_set_id = UE_V();
    pps_ptr->entropy_coding_mode_flag = vo->is_cabac = READ_FLAG();
    pps_ptr->pic_order_present_flag = READ_FLAG();
    pps_ptr->num_slice_groups_minus1 = UE_V();

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (pps_ptr->num_slice_groups_minus1 >=MAX_NUM_SLICE_GROUPS_MINUS1) {
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<23);
        return MMDEC_STREAM_ERROR;
    }
#endif

    //fmo parsing
    if (pps_ptr->num_slice_groups_minus1 > 0) {
        vo->fmo_used = TRUE;

        PRINTF ("FMO used!\n");
        pps_ptr->slice_group_map_type	= (int8)UE_V();

        if (pps_ptr->slice_group_map_type == 6) {
            pps_ptr->slice_group_id = (uint8 *)H264Dec_MemAlloc(vo, SIZE_SLICE_GROUP_ID*sizeof(uint8), 4, INTER_MEM);
            CHECK_MALLOC(pps_ptr->slice_group_id, "pps_ptr->slice_group_id");
        }

        if (pps_ptr->slice_group_map_type == 0) {
            for (i = 0; i <= (int32)pps_ptr->num_slice_groups_minus1; i++) {
                pps_ptr->run_length_minus1[i] = (uint16)UE_V();
            }
        } else if (pps_ptr->slice_group_map_type == 2) {
            for (i = 0; i < pps_ptr->num_slice_groups_minus1; i++) {
                pps_ptr->top_left[i] = UE_V();
                pps_ptr->bottom_right[i] =  UE_V();
            }
        } else if ((pps_ptr->slice_group_map_type == 3) || (pps_ptr->slice_group_map_type == 4) ||
                   (pps_ptr->slice_group_map_type == 5)) {
            pps_ptr->slice_group_change_direction_flag = READ_FLAG();
            pps_ptr->slice_group_change_rate_minus1 = UE_V();
        } else if (pps_ptr->slice_group_map_type == 6) {
            if ((pps_ptr->num_slice_groups_minus1+1) > 4) {
                NumberBitsPerSliceGroupId = 3;
            } else if ((pps_ptr->num_slice_groups_minus1+1) > 2) {
                NumberBitsPerSliceGroupId = 2;
            } else {
                NumberBitsPerSliceGroupId = 1;
            }

            //! JVT-F078, exlicitly signal number of MBs in the map
            pps_ptr->num_slice_group_map_units_minus1 = UE_V();
            for (i = 0; i <= (int32)pps_ptr->num_slice_group_map_units_minus1; i++) {
                pps_ptr->slice_group_id[i] = READ_FLC(NumberBitsPerSliceGroupId);
            }
        }
    } else {
        vo->fmo_used = FALSE; //FALSE;
    }

    //ONLY FOR FMO COMFORMANCE TEST
    pps_ptr->num_ref_idx_l0_active_minus1 = UE_V();

#if _H264_PROTECT_ & _LEVEL_LOW_
    if ((pps_ptr->num_ref_idx_l0_active_minus1+1) > MAX_REF_FRAME_NUMBER) {
        vo->error_flag |= ER_REF_FRM_ID;
        vo->return_pos1 |= (1<<24);     //lint !e648
        PRINTF ("too many l0_active not supported!\n");
        return MMDEC_STREAM_ERROR;
    }
#endif

    pps_ptr->num_ref_idx_l1_active_minus1 = UE_V();
    pps_ptr->weighted_pred_flag = READ_FLAG();
    pps_ptr->weighted_bipred_idc = READ_FLC(2);
    pps_ptr->pic_init_qp_minus26 = SE_V();
    pps_ptr->pic_init_qs_minus26 = SE_V();
    pps_ptr->chroma_qp_index_offset = SE_V();

    pps_ptr->deblocking_filter_control_present_flag = READ_FLAG();
    pps_ptr->constrained_intra_pred_flag = READ_FLAG();
    pps_ptr->redundant_pic_cnt_present_flag = READ_FLAG();

    if (!uvlc_startcode_follows((void *)vo)) {
        pps_ptr->transform_8x8_mode_flag = READ_FLAG();
        pps_ptr->pic_scaling_matrix_present_flag = READ_FLAG();
        if(pps_ptr->pic_scaling_matrix_present_flag) {
            decode_scaling_matrices(bs_ptr, &vo->g_sps_array_ptr[pps_ptr->seq_parameter_set_id], pps_ptr, 0, pps_ptr->ScalingList4x4, pps_ptr->ScalingList8x8);
        }

        pps_ptr->second_chroma_qp_index_offset = SE_V();
    } else {
        pps_ptr->second_chroma_qp_index_offset = pps_ptr->chroma_qp_index_offset;
    }

    pps_ptr->valid = TRUE;

    return MMDEC_OK;
}

LOCAL void H264Dec_make_pps_available (H264DecContext *vo, int32 pps_id, DEC_PPS_T *pps_ptr)
{
    int32 *src_ptr = (int32 *)pps_ptr;
    int32 *dst_ptr = (int32 *)(&vo->g_pps_array_ptr[pps_id]);
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

PUBLIC MMDecRet H264Dec_process_pps (H264DecContext *vo)
{
    DEC_PPS_T *pps_ptr = vo->g_pps_ptr;
    MMDecRet ret;

    memset(vo->g_pps_ptr, 0, sizeof(DEC_PPS_T)-4);
    memset(vo->g_pps_ptr->ScalingList4x4,16,6*16);
    memset(vo->g_pps_ptr->ScalingList8x8,16,2*64);

    ret = H264Dec_interpret_pps (pps_ptr, vo);
    if (ret != MMDEC_OK) {
        return ret;
    }
    H264Dec_make_pps_available (vo, pps_ptr->pic_parameter_set_id, pps_ptr);

    if (vo->g_active_pps_ptr && (pps_ptr->pic_parameter_set_id == vo->g_active_pps_ptr->pic_parameter_set_id)) {
        vo->g_old_pps_id = -1;
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
