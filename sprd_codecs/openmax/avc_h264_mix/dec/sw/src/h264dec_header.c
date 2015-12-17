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
#include "h264dec_video_header.h"
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

PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader(H264DecContext *img_ptr, uint8 *bitstrm_ptr, uint32 bitstrm_len, MMDecOutput *dec_output_ptr)
{
    uint32 tmpVar;
    int32 ret = 0;
    DEC_NALU_T *nal_ptr = img_ptr->g_nalu_ptr;
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;

    H264Dec_InitBitstream(bs_ptr, bitstrm_ptr, bitstrm_len);

    tmpVar = READ_FLC(8);
    nal_ptr->nal_unit_type = tmpVar & 0x1f;
    nal_ptr->nal_reference_idc = ((tmpVar>>5)&0x3);
    nal_ptr->frobidden_bit = ((tmpVar>>7)&0x1);

    /*jump to corresponding NALU type decode*/
    img_ptr->g_ready_to_decode_slice = FALSE;
    switch(nal_ptr->nal_unit_type) {
    case NALU_TYPE_IDR:
        if (img_ptr->sawSPS && img_ptr->sawPPS) {
            img_ptr->g_ready_to_decode_slice = TRUE;
            ret = H264Dec_process_slice (img_ptr, nal_ptr);

            if ((img_ptr->g_curr_slice_ptr->start_mb_nr == 0) && (!img_ptr->error_flag)) {
                img_ptr->g_searching_IDR_pic = FALSE;
            }
        } else {
            img_ptr->error_flag |= ER_BSM_ID;
        }
        break;
    case NALU_TYPE_SLICE:
        if (img_ptr->sawSPS && img_ptr->sawPPS) {
#if 0
            if (!g_searching_IDR_pic)
            {
                img_ptr->g_ready_to_decode_slice = TRUE;
                ret = H264Dec_process_slice (img_ptr, nal_ptr);
            }
#else
            ret = H264Dec_process_slice (img_ptr, nal_ptr);
            if (!img_ptr->g_searching_IDR_pic || (img_ptr->type == I_SLICE)) {
                img_ptr->g_ready_to_decode_slice = TRUE;

                if ((img_ptr->g_curr_slice_ptr->start_mb_nr == 0) && (!img_ptr->error_flag)) {
                    img_ptr->g_searching_IDR_pic = FALSE;
                }
            }
#endif
        } else {
            img_ptr->error_flag |= ER_BSM_ID;
        }
        break;
    case NALU_TYPE_PPS:
        img_ptr->sawPPS = TRUE;
        H264Dec_process_pps (img_ptr);
        break;
    case NALU_TYPE_SPS:
        img_ptr->sawSPS = TRUE;
        H264Dec_process_sps (img_ptr);
        if(dec_output_ptr) {
            dec_output_ptr->frame_width = (img_ptr->g_sps_ptr->pic_width_in_mbs_minus1+1) * 16;
            dec_output_ptr->frame_height= (img_ptr->g_sps_ptr->pic_height_in_map_units_minus1+1) *(2-(uint8)img_ptr->g_sps_ptr->frame_mbs_only_flag) *16;
        }
        break;
    case NALU_TYPE_SEI:
        H264Dec_interpret_sei_message ();
        break;
    default:
        PRINTF ("nalu type error!\n");
    }

    return ret;
}

LOCAL MMDecRet H264Dec_dec_ref_pic_marking (H264DecContext *img_ptr)
{
    int32 val;
    DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;

    if (img_ptr->idr_flag) {
        img_ptr->no_output_of_prior_pics_flag = READ_FLAG();
        img_ptr->long_term_reference_flag = READ_FLAG();
    } else {
        img_ptr->adaptive_ref_pic_buffering_flag = READ_FLAG();

        if (img_ptr->adaptive_ref_pic_buffering_flag) {
            int32 i = 0;

            //read memory management control operation
            img_ptr->g_dec_ref_pic_marking_buffer_size = 0;

            do {
                tmp_drpm_ptr = img_ptr->g_dec_ref_pic_marking_buffer+img_ptr->g_dec_ref_pic_marking_buffer_size;
                img_ptr->g_dec_ref_pic_marking_buffer_size++;

                val = tmp_drpm_ptr->memory_management_control_operation = UE_V();

                if ((val==1) || (val==3)) {
                    tmp_drpm_ptr->difference_of_pic_nums_minus1 = UE_V();
                } else if (val ==2) {
                    tmp_drpm_ptr->long_term_pic_num = UE_V();
                }

                if ((val==3)||(val==6)) {
                    tmp_drpm_ptr->long_term_frame_idx = UE_V();
                } else if (val == 4) {
                    tmp_drpm_ptr->max_long_term_frame_idx_plus1 = UE_V();
                }

                if ((i++) >= 50) {
                    img_ptr->error_flag |= ER_BSM_ID;
                    img_ptr->return_pos |= (1<<10);
                    return MMDEC_STREAM_ERROR;
                }
            } while(val != 0);
        }
    }

    return MMDEC_OK;
}

PUBLIC MMDecRet H264Dec_FirstPartOfSliceHeader (DEC_SLICE_T *curr_slice_ptr, H264DecContext *img_ptr)
{
    uint32 tmp;
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;

    curr_slice_ptr->start_mb_nr = UE_V();

    tmp = UE_V();
    img_ptr->type = curr_slice_ptr->picture_type = ((tmp > 4) ? (tmp - 5) : tmp);
//	if (img_ptr->type == P_SLICE || img_ptr->type == B_SLICE)
//	{
//		H264Dec_register_deblock_func (img_ptr);
//	}

//	if (img_ptr->is_cabac)
//	{
//		H264Dec_register_readMB_type_func (img_ptr);
//	}

    curr_slice_ptr->pic_parameter_set_id = UE_V();

    return MMDEC_OK;
}

LOCAL void H264Dec_ref_pic_list_reordering (H264DecContext *img_ptr)
{
    uint32 tmp;
    int32 val;
    int32 i;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;

    if (img_ptr->type != I_SLICE) {
        val = curr_slice_ptr->ref_pic_list_reordering_flag_l0 = READ_FLAG();

        if (val) {
            i = 0;

            do {
                val = curr_slice_ptr->remapping_of_pic_nums_idc_l0[i] = UE_V();

                if ((val == 0) || (val == 1)) {
                    tmp = SHOW_FLC(16);

                    if (tmp == 0) {
                        READ_FLC(16);
                        curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = 0xffff;
                        READ_FLC(17);
                    } else {
                        curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = UE_V();
                    }
                } else {
                    if (val == 2) {
                        tmp = SHOW_FLC(16);

                        if (tmp == 0) {
                            READ_FLC(16);
                            curr_slice_ptr->long_term_pic_idx_l0[i] = 0xffff;
                            READ_FLC(17);
                        } else {
                            curr_slice_ptr->long_term_pic_idx_l0[i] = UE_V();
                        }
                    }
                }
                i++;

                if ((val > 3) || (val < 0)) {
                    PRINTF ("Invalid remapping_of_pic_nums_idc command");
                    img_ptr->error_flag |= ER_BSM_ID;
                    img_ptr->return_pos |= (1<<11);
                    return;
                }
            } while(val != 3);
        }
    }

    if (img_ptr->type == B_SLICE) {
        val = curr_slice_ptr->ref_pic_list_reordering_flag_l1 = READ_FLAG();

        if (val) {
            i = 0;

            do {
                val = curr_slice_ptr->remapping_of_pic_nums_idc_l1[i] = UE_V();

                if ((val == 0) || (val == 1)) {
                    tmp = SHOW_FLC(16);

                    if (tmp == 0) {
                        READ_FLC(16);
                        curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = 0xffff;
                        READ_FLC(17);
                    } else {
                        curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = UE_V();
                    }
                } else {
                    if (val == 2) {
                        tmp = SHOW_FLC(16);

                        if (tmp == 0) {
                            READ_FLC(16);
                            curr_slice_ptr->long_term_pic_idx_l1[i] = 0xffff;
                            READ_FLC(17);
                        } else {
                            curr_slice_ptr->long_term_pic_idx_l1[i] = UE_V();
                        }
                    }
                }
                i++;

                if ((val > 3) || (val < 0)) {
                    PRINTF ("Invalid remapping_of_pic_nums_idc command");
                    img_ptr->error_flag |= ER_BSM_ID;
                    img_ptr->return_pos |= (1<<12);
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

    while (tmp != 0) {
        tmp >>= 1;
        ret++;
    }

    return ret;
}

LOCAL void H264Dec_pred_weight_table (H264DecContext *img_ptr)
{
    int32 i, j,list;
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;

    img_ptr->luma_log2_weight_denom = UE_V();
    img_ptr->wp_round_luma = img_ptr->luma_log2_weight_denom ? (1 << (img_ptr->luma_log2_weight_denom - 1) ) : 0;

    img_ptr->chroma_log2_weight_denom = UE_V();
    img_ptr->wp_round_chroma = img_ptr->chroma_log2_weight_denom ? (1 << (img_ptr->chroma_log2_weight_denom - 1)) : 0;

    //reset
    for (i = 0; i < MAX_REF_FRAME_NUMBER; i++) {
        int32 comp;
        int32 log_weight_denom;

        for (comp = 0; comp < 3; comp++) {
            log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
            img_ptr->g_wp_weight[0][i][comp] = (1 << log_weight_denom);
            img_ptr->g_wp_weight[1][i][comp] = (1 << log_weight_denom);
        }
    }

    for (list = 0; list < 2; list++) {
        for (i = 0; i < img_ptr->ref_count[list]; i++) {
            int32 luma_weight_flag;
            int32 chroma_weight_flag;

            luma_weight_flag = READ_FLAG();
            if (luma_weight_flag) {
                img_ptr->g_wp_weight[list][i][0] = SE_V();
                img_ptr->g_wp_offset[list][i][0] = SE_V();
            } else {
                img_ptr->g_wp_weight[list][i][0] = 1 << img_ptr->luma_log2_weight_denom;
                img_ptr->g_wp_offset[list][i][0] = 0;
            }

            chroma_weight_flag = READ_FLAG();
            for (j = 1; j < 3; j++) {
                if (chroma_weight_flag) {
                    img_ptr->g_wp_weight[list][i][j] = SE_V();
                    img_ptr->g_wp_offset[list][i][j] = SE_V();
                } else {
                    img_ptr->g_wp_weight[list][i][j] = 1 << img_ptr->chroma_log2_weight_denom;
                    img_ptr->g_wp_offset[list][i][j] = 0;
                }
            }
        }
    }
}

PUBLIC MMDecRet H264Dec_RestSliceHeader (H264DecContext *img_ptr, DEC_SLICE_T *curr_slice_ptr)
{
    uint32 tmp;
    DEC_SPS_T *active_sps_ptr = img_ptr->g_active_sps_ptr;
    DEC_PPS_T *active_pps_ptr = img_ptr->g_active_pps_ptr;
    DEC_BS_T *bs_ptr = img_ptr->bitstrm_ptr;
    MMDecRet ret;

    img_ptr->frame_num = READ_FLC (active_sps_ptr->log2_max_frame_num_minus4+4);

    if (active_sps_ptr->frame_mbs_only_flag) {
    } else {
        int32 field_pic_flag = READ_FLAG();

        if (field_pic_flag) {
            img_ptr->error_flag |= ER_BSM_ID;
            img_ptr->return_pos |= (1<<13);

            PRINTF("field is not supported!\n");
            return MMDEC_STREAM_ERROR;
        }
    }

    if (img_ptr->idr_flag) {
        img_ptr->pre_frame_num = 0;

        tmp = SHOW_FLC (16);

        if (tmp == 0) {
            READ_FLC (16);
            img_ptr->idr_pic_id = 0xffff;
            READ_FLC (17);
        } else {
            img_ptr->idr_pic_id = UE_V();
        }
    }

    if (active_sps_ptr->pic_order_cnt_type == 0) {
        img_ptr->pic_order_cnt_lsb = READ_FLC(active_sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4);

        if (active_pps_ptr->pic_order_present_flag) {
            img_ptr->delta_pic_order_cnt_bottom = Long_SE_V();
        } else {
            img_ptr->delta_pic_order_cnt_bottom = 0;
        }
    } else if (active_sps_ptr->pic_order_cnt_type == 1) {
        if (!(active_sps_ptr->delta_pic_order_always_zero_flag)) {
            img_ptr->delta_pic_order_cnt[0] = Long_SE_V();

            if (active_pps_ptr->pic_order_present_flag) {
                img_ptr->delta_pic_order_cnt[1] = Long_SE_V();
            }
        } else {
            img_ptr->delta_pic_order_cnt[0] = 0;
            img_ptr->delta_pic_order_cnt[1] = 0;
        }
    }

    if (active_pps_ptr->redundant_pic_cnt_present_flag) {
        /*img_ptr->redundant_pic_cnt =*/ UE_V();
    }

    if(img_ptr->type==B_SLICE) {
        img_ptr->direct_spatial_mv_pred_flag = READ_FLAG(); //u_1 ("SH: direct_spatial_mv_pred_flag", currStream);
    }

    if (img_ptr->direct_spatial_mv_pred_flag == 1) {
        img_ptr->direct_mv = H264Dec_direct_mv_spatial;
        img_ptr->pred_skip_bslice = H264Dec_mv_prediction_skipped_bslice_spatial;
        img_ptr->b8_mv_pred_func[0] = H264Dec_MC8x8_direct_spatial;
    } else {
        img_ptr->direct_mv = H264Dec_direct_mv_temporal;
        img_ptr->pred_skip_bslice = H264Dec_mv_prediction_skipped_bslice_temporal;
        img_ptr->b8_mv_pred_func[0] = H264Dec_MC8x8_direct_temporal;
    }

    img_ptr->ref_count[0] = active_pps_ptr->num_ref_idx_l0_active_minus1 + 1;
    img_ptr->ref_count[1] = active_pps_ptr->num_ref_idx_l1_active_minus1 + 1;

    if ((P_SLICE == img_ptr->type) || (B_SLICE == img_ptr->type))  {
        uint32 num_ref_idx_active_override_flag  = READ_FLAG();

        if (num_ref_idx_active_override_flag) {
            img_ptr->ref_count[0] = (int8)(1+UE_V());

            if(B_SLICE == img_ptr->type) {
                img_ptr->ref_count[1] = (int8)(1+UE_V());///1 + ue_v ("SH: num_ref_idx_l1_active_minus1", currStream);
            }
        }

        if ((img_ptr->ref_count[0] > MAX_REF_FRAME_NUMBER) || (img_ptr->ref_count[1] > MAX_REF_FRAME_NUMBER)) {
            img_ptr->error_flag |= ER_BSM_ID;
            img_ptr->return_pos |= (1<<14);
            PRINTF ("too many l0_active not supported!\n");
            return MMDEC_STREAM_ERROR;
        }
    }

    if (img_ptr->type != B_SLICE) {
        img_ptr->ref_count[1] = 0;
    }

    H264Dec_ref_pic_list_reordering(img_ptr);

    if(img_ptr->error_flag) {
        img_ptr->return_pos |= (1<<15);
        return MMDEC_STREAM_ERROR;
    }

    if ((active_pps_ptr->weighted_pred_flag&&(img_ptr->type==P_SLICE))||(active_pps_ptr->weighted_bipred_idc==1 && (img_ptr->type==B_SLICE))) {
        H264Dec_pred_weight_table (img_ptr);
    }

    if (img_ptr->nal_reference_idc) {
        ret = H264Dec_dec_ref_pic_marking (img_ptr);
        if (ret != MMDEC_OK) {
            return ret;
        }
    }

    if (img_ptr->is_cabac && img_ptr->type!=I_SLICE) {
        img_ptr->model_number = UE_V();//ue_v("SH: cabac_init_idc", currStream);
    } else {
        img_ptr->model_number = 0;
    }

    tmp = SE_V();
    curr_slice_ptr->slice_qp_delta = tmp;
    curr_slice_ptr->qp = img_ptr->qp = active_pps_ptr->pic_init_qp_minus26+26+tmp;

    if (active_pps_ptr->deblocking_filter_control_present_flag) {
        curr_slice_ptr->LFDisableIdc = UE_V();

        if (curr_slice_ptr->LFDisableIdc != 1) {
            curr_slice_ptr->LFAlphaC0Offset = 2*SE_V();
            curr_slice_ptr->LFBetaOffset = 2*SE_V();
        } else {
            curr_slice_ptr->LFAlphaC0Offset = curr_slice_ptr->LFBetaOffset = 0;
        }
    } else {
        curr_slice_ptr->LFDisableIdc = curr_slice_ptr->LFAlphaC0Offset = curr_slice_ptr->LFBetaOffset = 0;
    }

    if ((active_pps_ptr->num_slice_groups_minus1 > 0) && (active_pps_ptr->slice_group_map_type >= 3) &&
            (active_pps_ptr->slice_group_map_type <= 5)) {
        uint32 len;

        len = (active_sps_ptr->pic_height_in_map_units_minus1+1)*(active_sps_ptr->pic_width_in_mbs_minus1+1)/
              ((int32)active_pps_ptr->slice_group_change_rate_minus1+1);

        if (((active_sps_ptr->pic_height_in_map_units_minus1+1)*(active_sps_ptr->pic_width_in_mbs_minus1+1))%
                ((int32)active_pps_ptr->slice_group_change_rate_minus1+1)) {
            len += 1;
        }

        len = CeilLog2(len+1);

        img_ptr->slice_group_change_cycle = READ_FLC(len);
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
