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

void h264enc_sps_init (ENC_IMAGE_PARAMS_T *img_ptr)
{
    ENC_SPS_T *sps;

    sps = img_ptr->sps = &img_ptr->sps_array[0];

    sps->i_id = img_ptr->i_sps_id;
    sps->i_profile_idc = PROFILE_BASELINE;
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

    sps->b_vui = 0;

    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->i_mb_width = img_ptr->frame_width_in_mbs;
    sps->i_mb_height = img_ptr->frame_height_in_mbs;
    sps->b_frame_mbs_only = 1;
    sps->b_mb_adaptive_frame_field = 0;
    sps->b_direct8x8_inference = 0;

    if ((img_ptr->orig_height & 0xf) != 0) //only consider 1920x1080 and 960x540
    {
        sps->b_crop = 1;

        sps->frame_crop_left_offset = 0;
        sps->frame_crop_right_offset = 0;
        sps->frame_crop_top_offset = 0;
        sps->frame_crop_bottom_offset = (img_ptr->height - img_ptr->orig_height)/2;
    }else
    {
        sps->b_crop = 0;

        sps->frame_crop_left_offset = 0;
        sps->frame_crop_right_offset = 0;
        sps->frame_crop_top_offset = 0;
        sps->frame_crop_bottom_offset = 0;
    }

    SCI_TRACE_LOW("%s, %d, orig_height: %d, height: %d, b_crop: %d, frame_crop_left_offset: %d, frame_crop_right_offset: %d, frame_crop_top_offset: %d, frame_crop_bottom_offset: %d",
            __FUNCTION__, __LINE__, img_ptr->orig_height, img_ptr->height, sps->b_crop, sps->frame_crop_left_offset, sps->frame_crop_right_offset, sps->frame_crop_top_offset, sps->frame_crop_bottom_offset);
    sps->i_num_ref_frames = 1;
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

void h264enc_pps_init (H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr)
{
    ENC_PPS_T *pps;

    pps = img_ptr->pps = &img_ptr->pps_array[0];

    pps->i_id = img_ptr->sps->i_id;
    pps->i_sps_id = img_ptr->sps->i_id;

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

    H264Enc_OutputBits (vo, 0/*pps->b_cabac*/, 1);
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

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

