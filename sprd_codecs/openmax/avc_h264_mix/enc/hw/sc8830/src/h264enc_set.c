/******************************************************************************
 ** File Name:    h264enc_set.c											      *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

LOCAL void h264enc_vui_init (ENC_SPS_T *sps, ENC_VUI_T *vui_seq_parameters_ptr)
{
    vui_seq_parameters_ptr->aspect_ratio_info_present_flag = 0;
    vui_seq_parameters_ptr->overscan_info_present_flag     = 0;
    vui_seq_parameters_ptr->video_signal_type_present_flag = 0;
    vui_seq_parameters_ptr->chroma_location_info_present_flag = 0;
    vui_seq_parameters_ptr->timing_info_present_flag          = 0;
    vui_seq_parameters_ptr->nal_hrd_parameters_present_flag   = 0;
    vui_seq_parameters_ptr->vcl_hrd_parameters_present_flag   = 0;
    vui_seq_parameters_ptr->pic_struct_present_flag          =  0;

    vui_seq_parameters_ptr->bitstream_restriction_flag       =  1;
    if (vui_seq_parameters_ptr->bitstream_restriction_flag)
    {
        vui_seq_parameters_ptr->motion_vectors_over_pic_boundaries_flag =  0;
        vui_seq_parameters_ptr->max_bytes_per_pic_denom                =  0;
        vui_seq_parameters_ptr->max_bits_per_mb_denom                  =  0;
        vui_seq_parameters_ptr->log2_max_mv_length_horizontal        =  16;
        vui_seq_parameters_ptr->log2_max_mv_length_vertical            =  16;
        vui_seq_parameters_ptr->num_reorder_frames                      =  0;
        vui_seq_parameters_ptr->max_dec_frame_buffering              =  sps->i_num_ref_frames;
    }
    else
    {
        vui_seq_parameters_ptr->num_reorder_frames=0;
    }

    return;
}

void h264enc_sps_init (ENC_IMAGE_PARAMS_T *img_ptr)
{
    ENC_SPS_T *sps;

    sps = img_ptr->sps = &img_ptr->sps_array[0];

    sps->i_id = img_ptr->i_sps_id;
    sps->i_profile_idc = img_ptr->cabac_enable ? PROFILE_MAIN : PROFILE_BASELINE;
    sps->i_level_idc = img_ptr->i_level_idc;
    sps->b_constraint_set0 = (sps->i_profile_idc == PROFILE_BASELINE);
    sps->b_constraint_set1 = (sps->i_profile_idc <= PROFILE_MAIN);
    sps->b_constraint_set2 = 0;

    sps->i_log2_max_frame_num = 4; //at least 4
    while( ( 1 << sps->i_log2_max_frame_num) <= img_ptr->i_keyint_max)
    {
        sps->i_log2_max_frame_num++;
    }

    sps->i_poc_type = 0;
    if (sps->i_poc_type == 0)
    {
        sps->i_log2_max_poc_lsb = sps->i_log2_max_frame_num + 1; //max poc = 2 * frame_num
    } else if (sps->i_poc_type == 1)
    {
        sps->b_delta_pic_order_always_zero = 1;
        sps->i_offset_for_non_ref_pic = 0;
        sps->i_offset_for_top_to_bottom_field = 0;
        sps->i_num_ref_frames_in_poc_cycle = 0;
    }

    sps->b_vui = 1;

    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->i_mb_width = img_ptr->frame_width_in_mbs;
    sps->i_mb_height = img_ptr->frame_height_in_mbs;
    sps->b_frame_mbs_only = 1;
    sps->b_mb_adaptive_frame_field = 0;
    sps->b_direct8x8_inference = 0;

    if ((img_ptr->orig_height & 0xf) || (img_ptr->orig_width & 0xf))
    {
        sps->b_crop = 1;

        sps->frame_crop_left_offset = 0;
        sps->frame_crop_right_offset = (img_ptr->width - img_ptr->orig_width)/2;
        sps->frame_crop_top_offset = 0;
        sps->frame_crop_bottom_offset = (img_ptr->height - img_ptr->orig_height)/2;
    } else
    {
        sps->b_crop = 0;

        sps->frame_crop_left_offset = 0;
        sps->frame_crop_right_offset = 0;
        sps->frame_crop_top_offset = 0;
        sps->frame_crop_bottom_offset = 0;
    }

    sps->i_num_ref_frames = 1;
    sps->vui_parameters_present_flag = 1;
    if (sps->vui_parameters_present_flag)
    {
        h264enc_vui_init (sps, &sps->vui_seq_parameters);
    }

    SPRD_CODEC_LOGD ("%s, %d, orig_height: %d, height: %d, b_crop: %d, frame_crop_left_offset: %d, frame_crop_right_offset: %d, frame_crop_top_offset: %d, frame_crop_bottom_offset: %d",
                     __FUNCTION__, __LINE__, img_ptr->orig_height, img_ptr->height, sps->b_crop, sps->frame_crop_left_offset, sps->frame_crop_right_offset, sps->frame_crop_top_offset, sps->frame_crop_bottom_offset);
}

const uint8 x264_cqm_flat16[64] =
{
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16
};

void h264enc_pps_init (ENC_IMAGE_PARAMS_T *img_ptr)
{
    ENC_PPS_T *pps;

    pps = img_ptr->pps = &img_ptr->pps_array[0];

    pps->i_id = img_ptr->sps->i_id;
    pps->i_sps_id = img_ptr->sps->i_id;

    pps->b_entropy_coding_mode_flag = img_ptr->cabac_enable;
    pps->b_pic_order = 0;
    pps->i_num_slice_groups = 1;

    pps->i_num_ref_idx_l0_active = 1;
    pps->i_num_ref_idx_l1_active = 1;

    pps->i_pic_init_qp =  26;
    pps->i_pic_init_qs = 26;

    pps->i_chroma_qp_index_offset = img_ptr->i_chroma_qp_offset;
    pps->b_deblocking_filter_control = 1;
    pps->b_constrained_intra_pred = 0;
    pps->b_redundant_pic_cnt = 0;
}

LOCAL void h264enc_vui_write (H264EncObject *vo, ENC_VUI_T *vui_ptr)
{
    H264Enc_OutputBits (vo, vui_ptr->aspect_ratio_info_present_flag, 1);
    if (vui_ptr->aspect_ratio_info_present_flag)
    {
        H264Enc_OutputBits (vo, vui_ptr->aspect_ratio_idc, 8);
        if (255 == vui_ptr->aspect_ratio_idc)
        {
            H264Enc_OutputBits (vo, vui_ptr->sar_width, 16);
            H264Enc_OutputBits (vo, vui_ptr->sar_height, 16);
        }
    }

    H264Enc_OutputBits (vo, vui_ptr->overscan_info_present_flag, 1);
    if (vui_ptr->overscan_info_present_flag)
    {
        H264Enc_OutputBits (vo, vui_ptr->overscan_appropriate_flag, 1);
    }

    H264Enc_OutputBits (vo, vui_ptr->video_signal_type_present_flag, 1);
    if (vui_ptr->video_signal_type_present_flag)
    {
        H264Enc_OutputBits (vo, vui_ptr->video_format, 3);
        H264Enc_OutputBits (vo, vui_ptr->video_full_range_flag, 1);
        H264Enc_OutputBits (vo, vui_ptr->colour_description_present_flag, 1);

        if(vui_ptr->colour_description_present_flag)
        {
            H264Enc_OutputBits (vo, vui_ptr->colour_primaries, 8);
            H264Enc_OutputBits (vo, vui_ptr->transfer_characteristics, 8);
            H264Enc_OutputBits (vo, vui_ptr->matrix_coefficients, 8);
        }
    }

    H264Enc_OutputBits (vo, vui_ptr->chroma_location_info_present_flag, 1);
    if(vui_ptr->chroma_location_info_present_flag)
    {
        WRITE_UE_V (vui_ptr->chroma_sample_loc_type_top_field);
        WRITE_UE_V (vui_ptr->chroma_sample_loc_type_bottom_field);
    }

    H264Enc_OutputBits (vo, vui_ptr->timing_info_present_flag, 1);
    if (vui_ptr->timing_info_present_flag)
    {
        H264Enc_OutputBits (vo, vui_ptr->num_units_in_tick, 32);
        H264Enc_OutputBits (vo, vui_ptr->time_scale, 32);
        H264Enc_OutputBits (vo, vui_ptr->fixed_frame_rate_flag, 1);
    }

    H264Enc_OutputBits (vo, vui_ptr->nal_hrd_parameters_present_flag, 1);
    H264Enc_OutputBits (vo, vui_ptr->vcl_hrd_parameters_present_flag, 1);
    H264Enc_OutputBits (vo, vui_ptr->pic_struct_present_flag, 1);

    H264Enc_OutputBits (vo, vui_ptr->bitstream_restriction_flag, 1);
    if (vui_ptr->bitstream_restriction_flag)
    {
        H264Enc_OutputBits (vo, vui_ptr->motion_vectors_over_pic_boundaries_flag, 1);

        WRITE_UE_V (vui_ptr->max_bytes_per_pic_denom);
        WRITE_UE_V (vui_ptr->max_bits_per_mb_denom);
        WRITE_UE_V (vui_ptr->log2_max_mv_length_horizontal);
        WRITE_UE_V (vui_ptr->log2_max_mv_length_vertical);
        WRITE_UE_V (vui_ptr->num_reorder_frames);
        WRITE_UE_V (vui_ptr->max_dec_frame_buffering);
    }

    return;
}

void h264enc_sps_write (H264EncObject *vo, ENC_SPS_T *sps)
{
    uint32 nal_header;

    H264Enc_write_nal_start_code(vo);

    /* nal header, ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type; */
    nal_header = ( 0x00 << 7 ) | ( NAL_PRIORITY_HIGHEST << 5 ) | NAL_SPS;
    H264Enc_OutputBits (vo, nal_header, 8);

    H264Enc_OutputBits (vo, sps->i_profile_idc, 8);
    H264Enc_OutputBits (vo, sps->b_constraint_set0, 1);
    H264Enc_OutputBits (vo, sps->b_constraint_set1, 1);
    H264Enc_OutputBits (vo, sps->b_constraint_set2, 1);

    H264Enc_OutputBits (vo, 0, 5);	//reserved

    H264Enc_OutputBits (vo, sps->i_level_idc, 8);

    WRITE_UE_V (sps->i_id);

    WRITE_UE_V (sps->i_log2_max_frame_num - 4);
    WRITE_UE_V (sps->i_poc_type);
    if (sps->i_poc_type == 0 )
    {
        WRITE_UE_V (sps->i_log2_max_poc_lsb -4);
    } else if (sps->i_poc_type == 1)
    {
        //
    }

    WRITE_UE_V (sps->i_num_ref_frames);
    H264Enc_OutputBits (vo, sps->b_gaps_in_frame_num_value_allowed, 1);
    WRITE_UE_V (sps->i_mb_width -1);
    WRITE_UE_V (sps->i_mb_height - 1);
    H264Enc_OutputBits (vo, sps->b_frame_mbs_only, 1);
    H264Enc_OutputBits (vo, sps->b_direct8x8_inference, 1);
    H264Enc_OutputBits (vo, sps->b_crop, 1);
    if(sps->b_crop)
    {
        WRITE_UE_V (sps->frame_crop_left_offset);
        WRITE_UE_V (sps->frame_crop_right_offset);
        WRITE_UE_V (sps->frame_crop_top_offset);
        WRITE_UE_V (sps->frame_crop_bottom_offset);
    }

    H264Enc_OutputBits (vo, sps->b_vui, 1);
    if (sps->b_vui)
    {
        h264enc_vui_write(vo, &sps->vui_seq_parameters);
    }

    H264Enc_rbsp_trailing (vo);
}

void h264enc_pps_write (H264EncObject *vo, ENC_PPS_T *pps)
{
    uint32 nal_header;

    H264Enc_write_nal_start_code(vo);

    /* nal header, ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type; */
    nal_header = ( 0x00 << 7 ) | ( NAL_PRIORITY_HIGHEST << 5 ) | NAL_PPS;
    H264Enc_OutputBits (vo, nal_header, 8);

    WRITE_UE_V (pps->i_id);
    WRITE_UE_V (pps->i_sps_id);

    H264Enc_OutputBits (vo,pps->b_entropy_coding_mode_flag, 1); // cabac encoding
    H264Enc_OutputBits (vo, pps->b_pic_order, 1);
    WRITE_UE_V(pps->i_num_slice_groups-1);

    WRITE_UE_V(pps->i_num_ref_idx_l0_active-1);
    WRITE_UE_V(pps->i_num_ref_idx_l1_active-1);
    H264Enc_OutputBits (vo, 0/*pps->b_weighted_pred*/, 1);
    H264Enc_OutputBits (vo, 0/*pps->b_weighted_bipred*/, 2);

    WRITE_SE_V (pps->i_pic_init_qp - 26);
    WRITE_SE_V (pps->i_pic_init_qs - 26);
    WRITE_SE_V (pps->i_chroma_qp_index_offset);

    H264Enc_OutputBits (vo, pps->b_deblocking_filter_control, 1);
    H264Enc_OutputBits (vo, pps->b_constrained_intra_pred, 1);
    H264Enc_OutputBits (vo, pps->b_redundant_pic_cnt, 1);

    H264Enc_rbsp_trailing (vo);
}

#if 0
void h264enc_sei_version_write(H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr)
{
    int32 i;
    //random ID number generated according to ISO-11578
    const uint8 uuid[16] =
    {
        0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
        0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef
    };
    char version[] = "h264 encoder, Copyright (C) Spreadtrum, Inc.";
    int32 length;

    uint32 nal_header;

    H264Enc_write_nal_start_code (vo);

    /* nal header */
    nal_header = ( 0x00 << 7 ) | ( NAL_PRIORITY_DISPOSABLE << 5 ) | NAL_SEI;
    H264Enc_OutputBits (vo, nal_header, 8);

    length = strlen(version)+1+16;

    H264Enc_OutputBits (vo, 0x5, 8);	//payload_type = user_data_unregistered
    //payload_size
    for(i = 0; i <= length-255; i+=255)
    {
        H264Enc_OutputBits(vo, 255, 8);
    }
    H264Enc_OutputBits(vo, length-i, 8);

    for(i = 0; i < 16; i++)
    {
        H264Enc_OutputBits(vo, uuid[i], 8);
    }

    for(i = 0; i < length-16; i++)
    {
        H264Enc_OutputBits(vo, version[i], 8);
    }

    H264Enc_rbsp_trailing (vo);
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

