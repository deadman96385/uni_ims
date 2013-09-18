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

#define PRINTF_HEAD_INFO	//PRINTF

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

#if _MVC_
void nal_unit_header_mvc_extension(H264DecObject *vo, NALUnitHeaderMVCExt_t *NaluHeaderMVCExt)
{
    int tmp;
    NaluHeaderMVCExt->non_idr_flag     = READ_FLC(1);
    NaluHeaderMVCExt->priority_id      = READ_FLC(6);
    tmp= READ_FLC(10);
    if((tmp<0)||(tmp>127))//for error //0~1023//weihu
    {
        vo->error_flag |= ER_SREAM_ID;
        return;
    }
    NaluHeaderMVCExt->view_id          =tmp;
    NaluHeaderMVCExt->temporal_id      = READ_FLC(3);
    NaluHeaderMVCExt->anchor_pic_flag  = READ_FLC(1);
    NaluHeaderMVCExt->inter_view_flag  = READ_FLC(1);
    NaluHeaderMVCExt->reserved_one_bit = READ_FLC(1);
    if(NaluHeaderMVCExt->reserved_one_bit != 1)
    {
        vo->error_flag |= ER_SREAM_ID;
        SCI_TRACE_LOW ("Nalu Header MVC Extension: reserved_one_bit is not 1!\n");
        return;
    }
}
#endif

void dump_bs( uint8* pBuffer,int32 aInBufSize)
{
    FILE *fp = fopen("/data/video_es.m4v","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

PUBLIC int32 H264Dec_Read_SPS_PPS_SliceHeader(H264DecObject *vo)//uint8 *bitstrm_ptr, uint32 bitstrm_len)//weihu
{
    uint32 tmpVar;
    int32 ret = 0;
    DEC_NALU_T *nal_ptr = vo->g_nalu_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;

    tmpVar = READ_FLC(8);

    SCI_TRACE_LOW("%s, %d, the first byte: %d.", __FUNCTION__, __LINE__, tmpVar);

    nal_ptr->nal_unit_type = tmpVar & 0x1f;
    nal_ptr->nal_reference_idc = ((tmpVar>>5)&0x3);
    nal_ptr->frobidden_bit = ((tmpVar>>7)&0x1);

#if _MVC_
    curr_slice_ptr->svc_extension_flag = -1;

    if(vo->DecodeAllLayers == 1 && nal_ptr->nal_unit_type == NALU_TYPE_PREFIX || nal_ptr->nal_unit_type == NALU_TYPE_SLC_EXT)
    {
        curr_slice_ptr->svc_extension_flag = READ_FLC(1);
        if (!curr_slice_ptr->svc_extension_flag)//MVC
        {
            nal_unit_header_mvc_extension(vo, &curr_slice_ptr->NaluHeaderMVCExt);
            curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU = (nal_ptr->nal_unit_type == NALU_TYPE_PREFIX);
        }
    }
    
    if(nal_ptr->nal_unit_type == NALU_TYPE_SLC_EXT)
    {
        if(curr_slice_ptr->svc_extension_flag)
        {
            //to be implemented for Annex G;
            vo->error_flag |= ER_SREAM_ID;
        } else
        {
            nal_ptr->nal_unit_type = NALU_TYPE_SLICE;
        }
    }
    
    if (vo->error_flag)
    {
        return -1;
    }
#endif

    /*jump to corresponding NALU type decode*/
    vo->g_ready_to_decode_slice = FALSE;
    switch(nal_ptr->nal_unit_type)
    {
    case NALU_TYPE_IDR:
    case NALU_TYPE_SLICE:
#if _MVC_
        if(curr_slice_ptr->svc_extension_flag == 0)
        {   //MVC
            curr_slice_ptr->view_id = curr_slice_ptr->NaluHeaderMVCExt.view_id;
            curr_slice_ptr->inter_view_flag = curr_slice_ptr->NaluHeaderMVCExt.inter_view_flag;
            curr_slice_ptr->anchor_pic_flag = curr_slice_ptr->NaluHeaderMVCExt.anchor_pic_flag;
        }
        else if(curr_slice_ptr->svc_extension_flag == -1) //SVC and the normal AVC;
        {
            if(vo->g_active_subset_sps == NULL)
            {
                //g_curr_slice_ptr->view_id = GetBaseViewId(g_active_subset_sps);
                curr_slice_ptr->view_id = 0;//TOFIX
                if(curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU >0)
                {
                    //assert(g_curr_slice_ptr->view_id == g_curr_slice_ptr->NaluHeaderMVCExt.view_id); //for OR debug
                    curr_slice_ptr->inter_view_flag = curr_slice_ptr->NaluHeaderMVCExt.inter_view_flag;
                    curr_slice_ptr->anchor_pic_flag = curr_slice_ptr->NaluHeaderMVCExt.anchor_pic_flag;
                } else
                {
                    //g_curr_slice_ptr->inter_view_flag = 1;
                    //g_curr_slice_ptr->anchor_pic_flag = g_image_ptr->idr_flag;
                }
            } else
            {
                //assert(g_active_subset_sps->num_views_minus1 >=0);////for OR debug
                // prefix NALU available
                if(curr_slice_ptr->NaluHeaderMVCExt.iPrefixNALU >0)
                {
                    curr_slice_ptr->view_id = curr_slice_ptr->NaluHeaderMVCExt.view_id;
                    curr_slice_ptr->inter_view_flag = curr_slice_ptr->NaluHeaderMVCExt.inter_view_flag;
                    curr_slice_ptr->anchor_pic_flag = curr_slice_ptr->NaluHeaderMVCExt.anchor_pic_flag;
                } else
                {   //no prefix NALU;
                    curr_slice_ptr->view_id = vo->g_active_subset_sps->view_id[0];
                    curr_slice_ptr->inter_view_flag = 1;
                    curr_slice_ptr->anchor_pic_flag = vo->g_image_ptr->idr_flag;
                }
            }
        }
        curr_slice_ptr->layer_id = curr_slice_ptr->view_id = GetVOIdx(vo, curr_slice_ptr->view_id );
#endif
        vo->g_ready_to_decode_slice = TRUE;

        ret = H264Dec_Process_slice (vo);

        if ((curr_slice_ptr->start_mb_nr == 0) && (!vo->error_flag))
        {
            vo->g_searching_IDR_pic = FALSE;
        }

        break;
    case NALU_TYPE_PPS:
        H264Dec_ProcessPPS (vo);
        break;
    case NALU_TYPE_SPS:
        H264Dec_ProcessSPS (vo);
        break;
    case NALU_TYPE_SUB_SPS:
        //PRINTF ("Found NALU_TYPE_SUB_SPS\n");
        if (vo->DecodeAllLayers== 1)
        {
            ProcessSubsetSPS(vo);
        } else
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

LOCAL void H264Dec_dec_ref_pic_marking (H264DecObject *vo)
{
    int32 val;
    DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;

#if _MVC_
    if (img_ptr->idr_flag ||  (curr_slice_ptr->svc_extension_flag == 0 && curr_slice_ptr->NaluHeaderMVCExt.non_idr_flag == 0) )
#else
    if (img_ptr->idr_flag)
#endif
    {
        img_ptr->no_output_of_prior_pics_flag = READ_FLC(1);
        img_ptr->long_term_reference_flag = READ_FLC(1);
    } else
    {
        img_ptr->adaptive_ref_pic_buffering_flag = READ_FLC(1);

        if (img_ptr->adaptive_ref_pic_buffering_flag)
        {
            //read memory management control operation
            vo->g_dec_ref_pic_marking_buffer_size = 0;

            do
            {
                tmp_drpm_ptr = vo->g_dec_ref_pic_marking_buffer+vo->g_dec_ref_pic_marking_buffer_size;
                vo->g_dec_ref_pic_marking_buffer_size++;

                val = tmp_drpm_ptr->memory_management_control_operation = UE_V ();

                if ((val==1) || (val==3))
                {
                    tmp_drpm_ptr->difference_of_pic_nums_minus1 = UE_V ();
                }

                if (val ==2)
                {
                    tmp_drpm_ptr->long_term_pic_num = UE_V ();
                }

                if ((val==3)||(val==6))
                {
                    tmp_drpm_ptr->long_term_frame_idx = UE_V ();
                }
                if (val == 4)
                {
                    tmp_drpm_ptr->max_long_term_frame_idx_plus1 = UE_V ();
                }
            } while(val != 0);
        }
    }

    return;
}

PUBLIC void H264Dec_FirstPartOfSliceHeader (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    uint32 tmp;

    curr_slice_ptr->start_mb_nr = UE_V ();

    tmp = UE_V ();

    if (tmp > 4)
    {
        tmp -= 5;
    }

    img_ptr->type = curr_slice_ptr->picture_type = (tmp<2 ? tmp+1 : 0);//tmp;//weihu

    curr_slice_ptr->pic_parameter_set_id = UE_V ();

    if((tmp>9)||(curr_slice_ptr->pic_parameter_set_id>255))//for error
    {
        vo->error_flag |= ER_SREAM_ID;
        return;
    }
}

#if _MVC_
LOCAL void H264Dec_ref_pic_list_mvc_reordering(H264DecObject *vo)
{
    uint32 tmp;
    int32 val;
    int32 i;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

    if (img_ptr->type != I_SLICE)
    {
        val = curr_slice_ptr->ref_pic_list_reordering_flag_l0 = READ_FLC(1);

        if (val)
        {
            i=0;
            do
            {
                val = curr_slice_ptr->remapping_of_pic_nums_idc_l0[i] = UE_V();
                if (val==0 || val==1)
                {
                    tmp = SHOW_FLC(16);

                    if (tmp == 0)
                    {
                        READ_FLC(16);
                        curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = 0xffff;
                        READ_FLC(17);
                    } else
                    {
                        curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = UE_V();
                    }
                } else
                {
                    if (val == 2)
                    {
                        tmp = SHOW_FLC(16);

                        if (tmp == 0)
                        {
                            READ_FLC(16);
                            curr_slice_ptr->long_term_pic_idx_l0[i] = 0xffff;
                            READ_FLC(17);
                        } else
                        {
                            curr_slice_ptr->long_term_pic_idx_l0[i] = UE_V();
                        }
                    } else if (val==4 || val==5)
                    {
                        curr_slice_ptr->abs_diff_view_idx_minus1[0][i] = UE_V();
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
        val = curr_slice_ptr->ref_pic_list_reordering_flag_l1 = READ_FLC(1);

        if (val)
        {
            i = 0;

            do
            {
                val = curr_slice_ptr->remapping_of_pic_nums_idc_l1[i] = UE_V();

                if ((val == 0) || (val == 1))
                {
                    tmp = SHOW_FLC(16);

                    if (tmp == 0)
                    {
                        READ_FLC(16);
                        curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = 0xffff;
                        READ_FLC(17);
                    } else
                    {
                        curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = UE_V();
                    }
                } else
                {
                    if (val == 2)
                    {
                        tmp = SHOW_FLC(16);

                        if (tmp == 0)
                        {
                            READ_FLC(16);
                            curr_slice_ptr->long_term_pic_idx_l1[i] = 0xffff;
                            READ_FLC(17);
                        } else
                        {
                            curr_slice_ptr->long_term_pic_idx_l1[i] = UE_V();
                        }
                    } else if (val==4 || val==5)
                    {
                        curr_slice_ptr->abs_diff_view_idx_minus1[1][i] = UE_V();
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

LOCAL void H264Dec_ref_pic_list_reordering (H264DecObject *vo)
{
    uint32 tmp;
    uint32 val;
    int32 i;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;

    if (img_ptr->type != I_SLICE)
    {
        val = curr_slice_ptr->ref_pic_list_reordering_flag_l0 = READ_FLC(1);

        if (val)
        {
            i = 0;

            do
            {
                val = curr_slice_ptr->remapping_of_pic_nums_idc_l0[i] = UE_V();

                if ((val == 0) || (val == 1))
                {
                    tmp = SHOW_FLC(16);

                    if (tmp == 0)
                    {
                        READ_FLC(16);
                        curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = 0xffff;
                        READ_FLC(17);
                    } else
                    {
                        curr_slice_ptr->abs_diff_pic_num_minus1_l0[i] = UE_V();
                    }
                } else
                {
                    if (val == 2)
                    {
                        tmp = SHOW_FLC(16);

                        if (tmp == 0)
                        {
                            READ_FLC(16);
                            curr_slice_ptr->long_term_pic_idx_l0[i] = 0xffff;
                            READ_FLC(17);
                        } else
                        {
                            curr_slice_ptr->long_term_pic_idx_l0[i] = UE_V();
                        }
                    }
                }
                i++;

                if ((val > 3) || (val < 0) ||(i>img_ptr->num_ref_idx_l0_active)&&(val != 3))//for error
                {
                    SCI_TRACE_LOW ("Invalid remapping_of_pic_nums_idc command");
                    vo->error_flag |= ER_SREAM_ID;
                    return;
                }
            } while(val != 3);
        }
    }

    if (img_ptr->type == B_SLICE)
    {
        val = curr_slice_ptr->ref_pic_list_reordering_flag_l1 = READ_FLC(1);

        if (val)
        {
            i = 0;

            do
            {
                val = curr_slice_ptr->remapping_of_pic_nums_idc_l1[i] = UE_V();

                if ((val == 0) || (val == 1))
                {
                    tmp = SHOW_FLC(16);

                    if (tmp == 0)
                    {
                        READ_FLC(16);
                        curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = 0xffff;
                        READ_FLC(17);
                    } else
                    {
                        curr_slice_ptr->abs_diff_pic_num_minus1_l1[i] = UE_V();
                    }
                } else
                {
                    if (val == 2)
                    {
                        tmp = SHOW_FLC(16);

                        if (tmp == 0)
                        {
                            READ_FLC(16);
                            curr_slice_ptr->long_term_pic_idx_l1[i] = 0xffff;
                            READ_FLC(17);
                        } else
                        {
                            curr_slice_ptr->long_term_pic_idx_l1[i] = UE_V();
                        }
                    }
                }
                i++;

                if ((val > 3) || (val < 0)||(i>img_ptr->num_ref_idx_l1_active)&&(val != 3))//for error
                {
                    SCI_TRACE_LOW ("Invalid remapping_of_pic_nums_idc command");
                    vo->error_flag |= ER_SREAM_ID;
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

LOCAL void pred_weight_table(H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    int32 i, j;

    img_ptr->luma_log2_weight_denom = UE_V();
    img_ptr->wp_round_luma = img_ptr->luma_log2_weight_denom ? (1 << (img_ptr->luma_log2_weight_denom - 1) ) : 0;
    if(vo->g_active_sps_ptr->chroma_format_idc)
        img_ptr->chroma_log2_weight_denom = UE_V();
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
            vo->g_wp_weight[0][i][comp] = (1 << log_weight_denom);
            vo->g_wp_weight[1][i][comp] = (1 << log_weight_denom);
        }
    }

    for (i = 0; i < img_ptr->num_ref_idx_l0_active; i++)
    {
        int32 luma_weight_flag_l0;
        int32 chroma_weight_flag_l0;

        luma_weight_flag_l0 = READ_FLC(1);

        if (luma_weight_flag_l0)
        {
            vo->g_wp_weight[0][i][0] = SE_V();
            vo->g_wp_offset[0][i][0] = SE_V();
        } else
        {
            vo->g_wp_weight[0][i][0] = 1 << img_ptr->luma_log2_weight_denom;
            vo->g_wp_offset[0][i][0] = 0;
        }

        if(vo->g_active_sps_ptr->chroma_format_idc)//weihu
            chroma_weight_flag_l0 = READ_FLC(1);
        else
            chroma_weight_flag_l0 =0;

        for (j = 1; j < 3; j++)
        {
            if (chroma_weight_flag_l0)
            {
                vo->g_wp_weight[0][i][j] = SE_V();
                vo->g_wp_offset[0][i][j] = SE_V();
            } else
            {
                vo->g_wp_weight[0][i][j] = 1 << img_ptr->chroma_log2_weight_denom;
                vo->g_wp_offset[0][i][j] = 0;
            }
        }

    }

    if ((img_ptr->type == B_SLICE) && vo->g_active_pps_ptr->weighted_bipred_idc == 1)
    {
        int32 luma_weight_flag_l1;
        int32 chroma_weight_flag_l1;

        for (i = 0; i < img_ptr->num_ref_idx_l1_active; i++)
        {
            luma_weight_flag_l1 = READ_FLC(1);

            if (luma_weight_flag_l1)
            {
                vo->g_wp_weight[1][i][0] = SE_V();
                vo->g_wp_offset[1][i][0] = SE_V();
            } else
            {
                vo->g_wp_weight[1][i][0] = 1 << img_ptr->luma_log2_weight_denom;
                vo->g_wp_offset[1][i][0] = 0;
            }

            if(vo->g_active_sps_ptr->chroma_format_idc)//weihu
                chroma_weight_flag_l1 = READ_FLC(1);
            else
                chroma_weight_flag_l1 = 0;

            for (j = 1; j < 3; j++)
            {
                if (chroma_weight_flag_l1)
                {
                    vo->g_wp_weight[1][i][j] = SE_V();
                    vo->g_wp_offset[1][i][j] = SE_V();
                } else
                {
                    vo->g_wp_weight[1][i][j] = 1 << img_ptr->chroma_log2_weight_denom;
                    vo->g_wp_offset[1][i][j] = 0;
                }
            }

        }
    }

}

PUBLIC void H264Dec_RestSliceHeader (H264DecObject *vo)
{
    uint32 tmp;
    DEC_SPS_T *active_sps_ptr = vo->g_active_sps_ptr;
    DEC_PPS_T *active_pps_ptr = vo->g_active_pps_ptr;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;

    img_ptr->frame_num = READ_FLC (active_sps_ptr->log2_max_frame_num_minus4+4);

    if (active_sps_ptr->frame_mbs_only_flag)
    {
    } else
    {
        int32 field_pic_flag = READ_FLC(1);

        if (field_pic_flag)
        {
            vo->error_flag |= ER_SREAM_ID;
            SCI_TRACE_LOW("field is not supported!\n");
            return;
        }
    }

    if (img_ptr->idr_flag)
    {
        img_ptr->pre_frame_num = 0;

        tmp = SHOW_FLC (16);

        if (tmp == 0)
        {
            READ_FLC (16);
            img_ptr->idr_pic_id = 0xffff;
            READ_FLC (17);
        } else
        {
            img_ptr->idr_pic_id = UE_V ();
        }
    }
#if _MVC_
    else if (curr_slice_ptr->svc_extension_flag == 0 && curr_slice_ptr->NaluHeaderMVCExt.non_idr_flag == 0 )
    {
        img_ptr->idr_pic_id = UE_V ();
    }
#endif

    if (active_sps_ptr->pic_order_cnt_type == 0)
    {
        img_ptr->pic_order_cnt_lsb = READ_FLC (active_sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4);

        if (active_pps_ptr->pic_order_present_flag)
        {
            img_ptr->delta_pic_order_cnt_bottom = Long_SE_V ();
        } else
        {
            img_ptr->delta_pic_order_cnt_bottom = 0;
        }
    }

    if ((active_sps_ptr->pic_order_cnt_type == 1) && (!(active_sps_ptr->delta_pic_order_always_zero_flag)))
    {
        img_ptr->delta_pic_order_cnt[0] = Long_SE_V ();

        if (active_pps_ptr->pic_order_present_flag)
        {
            img_ptr->delta_pic_order_cnt[1] = Long_SE_V ();
        }
    } else
    {
        if (active_sps_ptr->pic_order_cnt_type == 1)
        {
            img_ptr->delta_pic_order_cnt[0] = 0;
            img_ptr->delta_pic_order_cnt[1] = 0;
        }
    }

    if (active_pps_ptr->redundant_pic_cnt_present_flag)
    {
        img_ptr->redundant_pic_cnt = UE_V ();
    }

    if(img_ptr->type==B_SLICE)
    {
        img_ptr->direct_type = READ_FLC(1); //u_1 ("SH: direct_spatial_mv_pred_flag", currStream);
    } else
    {
        img_ptr->direct_type =0;
    }

    img_ptr->num_ref_idx_l0_active = active_pps_ptr->num_ref_idx_l0_active_minus1 + 1;
    img_ptr->num_ref_idx_l1_active = active_pps_ptr->num_ref_idx_l1_active_minus1 + 1;

    if ((P_SLICE == img_ptr->type) || (B_SLICE == img_ptr->type))
    {
        tmp = READ_FLC (1);

        if (tmp)
        {
            img_ptr->num_ref_idx_l0_active = (1+UE_V());

            if(B_SLICE == img_ptr->type)
            {
                img_ptr->num_ref_idx_l1_active = (1+UE_V());///1 + ue_v ("SH: num_ref_idx_l1_active_minus1", currStream);
                if (img_ptr->num_ref_idx_l1_active > MAX_REF_FRAME_NUMBER)
                {
                    vo->error_flag |= ER_SREAM_ID;
                    SCI_TRACE_LOW ("too many l0_active not supported!\n");
                    return;
                }
            }
        }

        if (img_ptr->num_ref_idx_l0_active > MAX_REF_FRAME_NUMBER)
        {
            vo->error_flag |= ER_REF_FRM_ID;
            SCI_TRACE_LOW ("too many l0_active not supported!\n");
            return;
        }
    }

    if (img_ptr->type != B_SLICE)
    {
        img_ptr->num_ref_idx_l1_active = 0;
    }

#if _MVC_
    if(curr_slice_ptr->svc_extension_flag == 0 || curr_slice_ptr->svc_extension_flag == 1)
        H264Dec_ref_pic_list_mvc_reordering(vo);
    else
        H264Dec_ref_pic_list_reordering(vo);
#else
    H264Dec_ref_pic_list_reordering(vo, img_ptr);
#endif
    if(vo->error_flag)
    {
        return;
    }

    img_ptr->apply_weights = ((active_pps_ptr->weighted_pred_flag && (curr_slice_ptr->picture_type == P_SLICE ) )
                              || ((active_pps_ptr->weighted_bipred_idc > 0 ) && (curr_slice_ptr->picture_type == B_SLICE)));

    if ((active_pps_ptr->weighted_pred_flag&&(img_ptr->type==P_SLICE))||
            (active_pps_ptr->weighted_bipred_idc==1 && (img_ptr->type==B_SLICE)))
    {
        pred_weight_table(vo);
    }
    else
    {
        img_ptr->luma_log2_weight_denom=0;
        img_ptr->chroma_log2_weight_denom=0;
    }

    if (img_ptr->nal_reference_idc)
    {
        H264Dec_dec_ref_pic_marking (vo);
    }

    if (active_pps_ptr->entropy_coding_mode_flag && img_ptr->type!=I_SLICE)
    {
        img_ptr->model_number = UE_V();//ue_v("SH: cabac_init_idc", currStream);
    }
    else
    {
        img_ptr->model_number = 0;
    }

    tmp = SE_V();
    curr_slice_ptr->slice_qp_delta = tmp;
    curr_slice_ptr->qp = img_ptr->qp = active_pps_ptr->pic_init_qp_minus26+26+tmp;

    if (active_pps_ptr->deblocking_filter_control_present_flag)
    {
        curr_slice_ptr->LFDisableIdc = UE_V();

        if (curr_slice_ptr->LFDisableIdc != 1)
        {
            curr_slice_ptr->LFAlphaC0Offset = 2*SE_V();
            curr_slice_ptr->LFBetaOffset = 2* SE_V();
        } else
        {
            curr_slice_ptr->LFAlphaC0Offset = curr_slice_ptr->LFBetaOffset = 0;
        }
    } else
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

        img_ptr->slice_group_change_cycle = READ_FLC(len);
    }

    return;
}

#if _MVC_
int GetVOIdx(H264DecObject *vo, int iViewId)
{
    subset_seq_parameter_set_rbsp_t *active_subset_sps = vo->g_active_subset_sps;
    int iVOIdx = -1;
    int *piViewIdMap;
    if(active_subset_sps)
    {
        piViewIdMap = active_subset_sps->view_id;
        for(iVOIdx = active_subset_sps->num_views_minus1; iVOIdx>=0; iVOIdx--)
            if(piViewIdMap[iVOIdx] == iViewId)
                break;
    }
    else
    {
        subset_seq_parameter_set_rbsp_t *curr_subset_sps;
        int i;

        curr_subset_sps = vo->g_SubsetSeqParSet;
        for(i = 0; i < MAX_SPS; i++)
        {
            if(curr_subset_sps->num_views_minus1>=0 && curr_subset_sps->sps.valid)
            {
                break;
            }
            curr_subset_sps++;
        }

        if( i < MAX_SPS )
        {
            active_subset_sps = vo->g_active_subset_sps = curr_subset_sps;

            piViewIdMap = active_subset_sps->view_id;
            for(iVOIdx = active_subset_sps->num_views_minus1; iVOIdx>=0; iVOIdx--)
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

int GetViewIdx(H264DecObject *vo, int iVOIdx)
{
    subset_seq_parameter_set_rbsp_t *active_subset_sps = vo->g_active_subset_sps;
    int iViewIdx = -1;
    int *piViewIdMap;

    if(active_subset_sps )
    {
        //assert( g_active_subset_sps->num_views_minus1 >= iVOIdx && iVOIdx >= 0 );//for OR debug
        piViewIdMap = active_subset_sps->view_id;
        iViewIdx = piViewIdMap[iVOIdx];
    }

    return iViewIdx;
}
int get_maxViewIdx (H264DecObject *vo, int view_id, int anchor_pic_flag, int listidx)
{
    subset_seq_parameter_set_rbsp_t *active_subset_sps = vo->g_active_subset_sps;
    int VOIdx;
    int maxViewIdx = 0;

    VOIdx = view_id;
    if(VOIdx >= 0)
    {
        if(anchor_pic_flag)
            maxViewIdx = listidx? active_subset_sps->num_anchor_refs_l1[VOIdx] : active_subset_sps->num_anchor_refs_l0[VOIdx];
        else
            maxViewIdx = listidx? active_subset_sps->num_non_anchor_refs_l1[VOIdx] : active_subset_sps->num_non_anchor_refs_l0[VOIdx];
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
