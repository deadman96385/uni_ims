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

PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader(H264DecContext *vo, uint8 *bitstrm_ptr, uint32 bitstrm_len, MMDecOutput *dec_output_ptr)
{
    uint32 tmpVar;
    int32 ret = 0;
    DEC_NALU_T *nal_ptr = vo->g_nalu_ptr;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    H264Dec_InitBitstream(bs_ptr, bitstrm_ptr, bitstrm_len);

    tmpVar = READ_FLC(8);
    nal_ptr->nal_unit_type = tmpVar & 0x1f;
    nal_ptr->nal_reference_idc = ((tmpVar>>5)&0x3);
    nal_ptr->frobidden_bit = ((tmpVar>>7)&0x1);

    /*jump to corresponding NALU type decode*/
    vo->g_ready_to_decode_slice = FALSE;
    switch(nal_ptr->nal_unit_type) {
    case NALU_TYPE_IDR:
        if (vo->sawSPS && vo->sawPPS) {
            vo->g_ready_to_decode_slice = TRUE;
            ret = H264Dec_process_slice (vo, nal_ptr);

            if ((vo->g_curr_slice_ptr->start_mb_nr == 0) && (!vo->error_flag)) {
                vo->g_searching_IDR_pic = FALSE;
            }
        } else {
            vo->error_flag |= ER_BSM_ID;
        }
        break;
    case NALU_TYPE_SLICE:
        if (vo->sawSPS && vo->sawPPS) {
#if 0
            if (!g_searching_IDR_pic)
            {
                img_ptr->g_ready_to_decode_slice = TRUE;
                ret = H264Dec_process_slice (img_ptr, nal_ptr);
            }
#else
            ret = H264Dec_process_slice (vo, nal_ptr);
            if (!vo->g_searching_IDR_pic || (vo->type == I_SLICE)) {
                vo->g_ready_to_decode_slice = TRUE;

                if ((vo->g_curr_slice_ptr->start_mb_nr == 0) && (!vo->error_flag)) {
                    vo->g_searching_IDR_pic = FALSE;
                }
            }
#endif
        } else {
            vo->error_flag |= ER_BSM_ID;
        }
        break;
    case NALU_TYPE_PPS:
        if (vo->sawPPS != TRUE) {
            H264Dec_process_pps (vo);
        }
        vo->sawPPS = TRUE;
        break;
    case NALU_TYPE_SPS:
        if (vo->sawSPS != TRUE) {
            H264Dec_process_sps (vo);
            if(dec_output_ptr) {
                dec_output_ptr->frame_width = (vo->g_sps_ptr->pic_width_in_mbs_minus1+1) * 16;
                dec_output_ptr->frame_height= (vo->g_sps_ptr->pic_height_in_map_units_minus1+1) *(2-(uint8)vo->g_sps_ptr->frame_mbs_only_flag) *16;
            }
        }
        vo->sawSPS = TRUE;
        break;
    case NALU_TYPE_SEI:
        H264Dec_interpret_sei_message ();
        break;
    default:
        PRINTF ("nalu type error!\n");
    }

    return ret;
}

LOCAL MMDecRet H264Dec_dec_ref_pic_marking (H264DecContext *vo)
{
    int32 val;
    DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    if (vo->idr_flag) {
        vo->no_output_of_prior_pics_flag = READ_FLAG();
        vo->long_term_reference_flag = READ_FLAG();
    } else {
        vo->adaptive_ref_pic_buffering_flag = READ_FLAG();

        if (vo->adaptive_ref_pic_buffering_flag) {
            int32 i = 0;

            //read memory management control operation
            vo->g_dec_ref_pic_marking_buffer_size = 0;

            do {
                tmp_drpm_ptr = vo->g_dec_ref_pic_marking_buffer+vo->g_dec_ref_pic_marking_buffer_size;
                vo->g_dec_ref_pic_marking_buffer_size++;

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
                    vo->error_flag |= ER_BSM_ID;
                    vo->return_pos |= (1<<10);
                    return MMDEC_STREAM_ERROR;
                }
            } while(val != 0);
        }
    }

    return MMDEC_OK;
}

PUBLIC MMDecRet H264Dec_FirstPartOfSliceHeader (DEC_SLICE_T *curr_slice_ptr, H264DecContext *vo)
{
    uint32 tmp;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    curr_slice_ptr->start_mb_nr = UE_V();

    tmp = UE_V();
    vo->type = curr_slice_ptr->picture_type = ((tmp > 4) ? (tmp - 5) : tmp);
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

LOCAL void H264Dec_ref_pic_list_reordering (H264DecContext *vo)
{
    uint32 tmp;
    int32 val;
    int32 i;
    DEC_SLICE_T *curr_slice_ptr = vo->curr_slice_ptr;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    if (vo->type != I_SLICE) {
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
                    vo->error_flag |= ER_BSM_ID;
                    vo->return_pos |= (1<<11);
                    return;
                }
            } while(val != 3);
        }
    }

    if (vo->type == B_SLICE) {
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
                    vo->error_flag |= ER_BSM_ID;
                    vo->return_pos |= (1<<12);
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

LOCAL void H264Dec_pred_weight_table (H264DecContext *vo)
{
    int32 i, j,list;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    vo->luma_log2_weight_denom = UE_V();
    vo->wp_round_luma = vo->luma_log2_weight_denom ? (1 << (vo->luma_log2_weight_denom - 1) ) : 0;

    vo->chroma_log2_weight_denom = UE_V();
    vo->wp_round_chroma = vo->chroma_log2_weight_denom ? (1 << (vo->chroma_log2_weight_denom - 1)) : 0;

    //reset
    for (i = 0; i < MAX_REF_FRAME_NUMBER; i++) {
        int32 comp;
        int32 log_weight_denom;

        for (comp = 0; comp < 3; comp++) {
            log_weight_denom = (comp == 0) ? vo->luma_log2_weight_denom : vo->chroma_log2_weight_denom;
            vo->g_wp_weight[0][i][comp] = (1 << log_weight_denom);
            vo->g_wp_weight[1][i][comp] = (1 << log_weight_denom);
        }
    }

    for (list = 0; list < 2; list++) {
        for (i = 0; i < vo->ref_count[list]; i++) {
            int32 luma_weight_flag;
            int32 chroma_weight_flag;

            luma_weight_flag = READ_FLAG();
            if (luma_weight_flag) {
                vo->g_wp_weight[list][i][0] = SE_V();
                vo->g_wp_offset[list][i][0] = SE_V();
            } else {
                vo->g_wp_weight[list][i][0] = 1 << vo->luma_log2_weight_denom;
                vo->g_wp_offset[list][i][0] = 0;
            }

            chroma_weight_flag = READ_FLAG();
            for (j = 1; j < 3; j++) {
                if (chroma_weight_flag) {
                    vo->g_wp_weight[list][i][j] = SE_V();
                    vo->g_wp_offset[list][i][j] = SE_V();
                } else {
                    vo->g_wp_weight[list][i][j] = 1 << vo->chroma_log2_weight_denom;
                    vo->g_wp_offset[list][i][j] = 0;
                }
            }
        }
    }
}

PUBLIC MMDecRet H264Dec_RestSliceHeader (H264DecContext *vo, DEC_SLICE_T *curr_slice_ptr)
{
    uint32 tmp;
    DEC_SPS_T *active_sps_ptr = vo->g_active_sps_ptr;
    DEC_PPS_T *active_pps_ptr = vo->g_active_pps_ptr;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;
    MMDecRet ret;

    vo->frame_num = READ_FLC (active_sps_ptr->log2_max_frame_num_minus4+4);

    if (active_sps_ptr->frame_mbs_only_flag) {
    } else {
        int32 field_pic_flag = READ_FLAG();

        if (field_pic_flag) {
            vo->error_flag |= ER_BSM_ID;
            vo->return_pos |= (1<<13);

            PRINTF("field is not supported!\n");
            return MMDEC_STREAM_ERROR;
        }
    }

    if (vo->idr_flag) {
        vo->pre_frame_num = 0;

        tmp = SHOW_FLC (16);

        if (tmp == 0) {
            READ_FLC (16);
            vo->idr_pic_id = 0xffff;
            READ_FLC (17);
        } else {
            vo->idr_pic_id = UE_V();
        }
    }

    if (active_sps_ptr->pic_order_cnt_type == 0) {
        vo->pic_order_cnt_lsb = READ_FLC(active_sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4);

        if (active_pps_ptr->pic_order_present_flag) {
            vo->delta_pic_order_cnt_bottom = Long_SE_V();
        } else {
            vo->delta_pic_order_cnt_bottom = 0;
        }
    } else if (active_sps_ptr->pic_order_cnt_type == 1) {
        if (!(active_sps_ptr->delta_pic_order_always_zero_flag)) {
            vo->delta_pic_order_cnt[0] = Long_SE_V();

            if (active_pps_ptr->pic_order_present_flag) {
                vo->delta_pic_order_cnt[1] = Long_SE_V();
            }
        } else {
            vo->delta_pic_order_cnt[0] = 0;
            vo->delta_pic_order_cnt[1] = 0;
        }
    }

    if (active_pps_ptr->redundant_pic_cnt_present_flag) {
        /*img_ptr->redundant_pic_cnt =*/ UE_V();
    }

    if(vo->type==B_SLICE) {
        vo->direct_spatial_mv_pred_flag = READ_FLAG(); //u_1 ("SH: direct_spatial_mv_pred_flag", currStream);
    }

    if (vo->direct_spatial_mv_pred_flag == 1) {
        vo->direct_mv = H264Dec_direct_mv_spatial;
        vo->pred_skip_bslice = H264Dec_mv_prediction_skipped_bslice_spatial;
        vo->b8_mv_pred_func[0] = H264Dec_MC8x8_direct_spatial;
    } else {
        vo->direct_mv = H264Dec_direct_mv_temporal;
        vo->pred_skip_bslice = H264Dec_mv_prediction_skipped_bslice_temporal;
        vo->b8_mv_pred_func[0] = H264Dec_MC8x8_direct_temporal;
    }

    vo->ref_count[0] = active_pps_ptr->num_ref_idx_l0_active_minus1 + 1;
    vo->ref_count[1] = active_pps_ptr->num_ref_idx_l1_active_minus1 + 1;

    if ((P_SLICE == vo->type) || (B_SLICE == vo->type))  {
        uint32 num_ref_idx_active_override_flag  = READ_FLAG();

        if (num_ref_idx_active_override_flag) {
            vo->ref_count[0] = (int8)(1+UE_V());

            if(B_SLICE == vo->type) {
                vo->ref_count[1] = (int8)(1+UE_V());///1 + ue_v ("SH: num_ref_idx_l1_active_minus1", currStream);
            }
        }

        if ((vo->ref_count[0] > MAX_REF_FRAME_NUMBER) || (vo->ref_count[1] > MAX_REF_FRAME_NUMBER)) {
            vo->error_flag |= ER_BSM_ID;
            vo->return_pos |= (1<<14);
            PRINTF ("too many l0_active not supported!\n");
            return MMDEC_STREAM_ERROR;
        }
    }

    if (vo->type != B_SLICE) {
        vo->ref_count[1] = 0;
    }

    H264Dec_ref_pic_list_reordering(vo);

    if(vo->error_flag) {
        vo->return_pos |= (1<<15);
        return MMDEC_STREAM_ERROR;
    }

    if ((active_pps_ptr->weighted_pred_flag&&(vo->type==P_SLICE))||(active_pps_ptr->weighted_bipred_idc==1 && (vo->type==B_SLICE))) {
        H264Dec_pred_weight_table (vo);
    }

    if (vo->nal_reference_idc) {
        ret = H264Dec_dec_ref_pic_marking (vo);
        if (ret != MMDEC_OK) {
            return ret;
        }
    }

    if (vo->is_cabac && vo->type!=I_SLICE) {
        vo->model_number = UE_V();//ue_v("SH: cabac_init_idc", currStream);
    } else {
        vo->model_number = 0;
    }

    tmp = SE_V();
    curr_slice_ptr->slice_qp_delta = tmp;
    curr_slice_ptr->qp = vo->qp = active_pps_ptr->pic_init_qp_minus26+26+tmp;

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

        vo->slice_group_change_cycle = READ_FLC(len);
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
