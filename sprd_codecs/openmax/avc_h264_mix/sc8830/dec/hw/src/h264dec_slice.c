/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
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

PUBLIC int32 H264Dec_Process_slice (H264DecObject *vo)
{
    DEC_NALU_T *nalu_ptr = vo->g_nalu_ptr;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    int32 curr_header;
    int32 new_picture;
    img_ptr->idr_flag = (nalu_ptr->nal_unit_type == NALU_TYPE_IDR);
    img_ptr->nal_reference_idc = (nalu_ptr->nal_reference_idc);

    H264Dec_FirstPartOfSliceHeader (vo);

    /*if picture parameter set id changed, FMO will change, and neighbour 4x4 block
    position infomation(available, position) will change*/
    if (vo->g_old_pps_id != curr_slice_ptr->pic_parameter_set_id)
    {
        vo->g_old_pps_id = curr_slice_ptr->pic_parameter_set_id;

        //use PPS and SPS
        H264Dec_use_parameter_set (vo, curr_slice_ptr->pic_parameter_set_id);
    }
    if(curr_slice_ptr->start_mb_nr>=img_ptr->frame_size_in_mbs)//for error
    {
        vo->error_flag = TRUE;
        return -1;
    }

    H264Dec_RestSliceHeader (vo);
#if _MVC_
    if(curr_slice_ptr->view_id >= 0)
    {
        curr_slice_ptr->p_Dpb = vo->g_dpb_layer[curr_slice_ptr->view_id];
    }
#endif

    if (vo->error_flag == TRUE)
    {
        return -1;
    }

    new_picture = H264Dec_is_new_picture (vo);
    //new_picture |=  (g_dec_picture_ptr==NULL);  //for error stream//weihu

    if(vo->g_dec_picture_ptr==NULL)//for error stream//weihu
        new_picture=TRUE;
    else if((img_ptr->curr_mb_nr <= (int)curr_slice_ptr->start_mb_nr)&&(curr_slice_ptr->start_mb_nr>0))//&&(img_ptr->curr_mb_nr>0)
        new_picture=FALSE;

    img_ptr->is_new_pic = new_picture;

    if (new_picture)
    {
        H264Dec_init_picture (vo);
        curr_header = SOP;
    } else
    {
        curr_header = SOS;
    }

    H264Dec_init_list (vo, img_ptr->type);
#if _MVC_
    if(curr_slice_ptr->svc_extension_flag == 0 || curr_slice_ptr->svc_extension_flag == 1)
        H264Dec_reorder_list_mvc (vo);
    else
#endif
        H264Dec_reorder_list (vo);

    if (vo->error_flag == TRUE)
        return -1;

    //configure ref_list_buf[24]
    {
        int i;

        //ref_list_buf[32]=0;
        memset(vo->ref_list_buf, 0, 33*sizeof(int));

        for (i = 0; i < img_ptr->num_ref_idx_l0_active; i++)//g_list_size[0]//weihu
        {
            vo->ref_list_buf[32] |= ((vo->g_list0[i]->is_long_term)<<i);
        }

        for (i = img_ptr->num_ref_idx_l0_active; i < MAX_REF_FRAME_NUMBER; i++)//g_list_size[0]
        {
            vo->ref_list_buf[32] |= (1<<i);
        }

        for (i = 0; i < vo->g_list_size[1]; i++)
        {
            vo->ref_list_buf[32] |= ((vo->g_list1[i]->is_long_term)<<(i+16));
        }

        for (i = vo->g_list_size[1]; i < MAX_REF_FRAME_NUMBER; i++)
        {
            vo->ref_list_buf[32] |= (1<<(i+16));
        }

        for(i = 0; i < img_ptr->num_ref_idx_l0_active; i++) //g_list_size[0]
        {
            vo->ref_list_buf[i] = vo->g_list0[i]->poc;
        }
        for(i = 0; i < vo->g_list_size[1]; i++)
        {
            vo->ref_list_buf[i+16] = vo->g_list1[i]->poc;
        }
    }

    img_ptr->curr_mb_nr = curr_slice_ptr->start_mb_nr;

    return curr_header;
}

LOCAL void H264Dec_exit_slice (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = vo->g_old_slice_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;

    old_slice_ptr->frame_num = img_ptr->frame_num;
    old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
    old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
    old_slice_ptr->idr_flag = img_ptr->idr_flag;

    if (img_ptr->idr_flag)
    {
        old_slice_ptr->idr_pic_id = img_ptr->idr_pic_id;
    }

    if (vo->g_active_sps_ptr->pic_order_cnt_type == 0)
    {
        old_slice_ptr->pic_order_cnt_lsb = img_ptr->pic_order_cnt_lsb;
        old_slice_ptr->delta_pic_order_cnt_bottom = img_ptr->delta_pic_order_cnt_bottom;
    }

    if (vo->g_active_sps_ptr->pic_order_cnt_type == 1)
    {
        old_slice_ptr->delta_pic_order_cnt[0] = img_ptr->delta_pic_order_cnt[0];
        old_slice_ptr->delta_pic_order_cnt[1] = img_ptr->delta_pic_order_cnt[1];
    }

#if _MVC_
    old_slice_ptr->view_id = curr_slice_ptr->view_id;
    old_slice_ptr->inter_view_flag = curr_slice_ptr->inter_view_flag;
    old_slice_ptr->anchor_pic_flag = curr_slice_ptr->anchor_pic_flag;
#endif

    return;
}

void H264Dec_Cfg_ScalingMatix (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    int i, j;
    uint32 cmd;

    if(vo->g_active_pps_ptr->pic_scaling_matrix_present_flag || vo->g_active_sps_ptr->seq_scaling_matrix_present_flag)
    {
        if(vo->g_active_sps_ptr->seq_scaling_matrix_present_flag) // check sps first
        {
            for(i=0; i<8; i++)
            {
                if(i<6)
                {
                    if(!vo->g_active_sps_ptr->seq_scaling_list_present_flag[i]) // fall-back rule A
                    {
                        if(i==0)
                            SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                        else if(i==3)
                            SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
                        else
                            SCI_MEMCPY(vo->weightscale4x4[i], vo->weightscale4x4[i-1], 16*sizeof(char));
                    } else
                    {
                        if(vo->g_active_sps_ptr->UseDefaultScalingMatrix4x4Flag[i])
                        {
                            if (i<3)
                            {
                                SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                            } else
                            {
                                SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
                            }
                        } else
                            SCI_MEMCPY(vo->weightscale4x4[i], vo->g_active_sps_ptr->ScalingList4x4[i], 16*sizeof(char));
                    }
                } else
                {
                    if(!vo->g_active_sps_ptr->seq_scaling_list_present_flag[i]) // fall-back rule A
                    {
                        if(i==6)
                            SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                        else
                            SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                    } else
                    {
                        if(vo->g_active_sps_ptr->UseDefaultScalingMatrix8x8Flag[i-6])
                        {
                            if (i==6)
                            {
                                SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                            } else
                            {
                                SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                            }
                        } else
                            SCI_MEMCPY(vo->weightscale8x8[i-6], vo->g_active_sps_ptr->ScalingList8x8[i-6], 64*sizeof(char));
                    }
                }
            }
        }

        if(vo->g_active_pps_ptr->pic_scaling_matrix_present_flag) // then check pps
        {
            for(i=0; i<8; i++)
            {
                if(i<6)
                {
                    if(!vo->g_active_pps_ptr->pic_scaling_list_present_flag[i]) // fall-back rule B
                    {
                        if (i==0)
                        {
                            if(!vo->g_active_sps_ptr->seq_scaling_matrix_present_flag)
                                SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                        } else if (i==3)
                        {
                            if(!vo->g_active_sps_ptr->seq_scaling_matrix_present_flag)
                                SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
                        } else
                            SCI_MEMCPY(vo->weightscale4x4[i], vo->weightscale4x4[i-1], 16*sizeof(char));
                    } else
                    {
                        if(vo->g_active_pps_ptr->UseDefaultScalingMatrix4x4Flag[i])
                        {
                            if (i<3)
                            {
                                SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_intra_default, 16*sizeof(char));
                            } else
                            {
                                SCI_MEMCPY(vo->weightscale4x4[i], weightscale4x4_inter_default, 16*sizeof(char));
                            }
                        } else
                            SCI_MEMCPY(vo->weightscale4x4[i], vo->g_active_pps_ptr->ScalingList4x4[i], 16*sizeof(char));
                    }
                }	else
                {
                    if(!vo->g_active_pps_ptr->pic_scaling_list_present_flag[i]) // fall-back rule B
                    {
                        if (i==6)
                        {
                            if(!vo->g_active_sps_ptr->seq_scaling_matrix_present_flag)
                                SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                        } else
                        {
                            if(!vo->g_active_sps_ptr->seq_scaling_matrix_present_flag)
                                SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                        }
                    } else
                    {
                        if(vo->g_active_pps_ptr->UseDefaultScalingMatrix8x8Flag[i-6])
                        {
                            if (i==6)
                            {
                                SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_intra_default, 64*sizeof(char));
                            } else
                            {
                                SCI_MEMCPY(vo->weightscale8x8[i-6], weightscale8x8_inter_default, 64*sizeof(char));
                            }
                        } else
                            SCI_MEMCPY(vo->weightscale8x8[i-6], vo->g_active_pps_ptr->ScalingList8x8[i-6], 64*sizeof(char));
                    }
                }
            }
        }
    }

    if (img_ptr->apply_weights)
    {
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < 16; j++)
            {
                cmd = (vo->g_wp_weight[i][j][2]<<18)|((vo->g_wp_weight[i][j][1]&0x1ff)<<9)|(vo->g_wp_weight[i][j][0]&0x1ff);
                VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+136+(2*j+32*i)*4,cmd,"g_wp_weight");
                
                cmd = (vo->g_wp_offset[i][j][2]<<16)|((vo->g_wp_offset[i][j][1]&0xff)<<8)|(vo->g_wp_offset[i][j][0]&0xff);
                VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+140+(2*j+32*i)*4,cmd,"g_wp_offset");
            }
        }
    }//weihu

    if (vo->g_active_pps_ptr->pic_scaling_matrix_present_flag || vo->g_active_sps_ptr->seq_scaling_matrix_present_flag)
    {
        for(i = 3; i < 6; i++)//inter
        {
            for(j = 0; j < 4; j++)
            {
                cmd=((vo->weightscale4x4[i][j][3]&0xff)<<24)|
                    ((vo->weightscale4x4[i][j][2]&0xff)<<16)|
                    ((vo->weightscale4x4[i][j][1]&0xff)<<8)|
                    ((vo->weightscale4x4[i][j][0]&0xff)<<0);
                VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER4x4Y_OFF+(i-3)*16+j*4,cmd,"weightscale inter4x4");
            }
        }

        for (i = 0; i < 3; i++)//intra
        {
            for(j = 0; j < 4; j++)
            {
                cmd=((vo->weightscale4x4[i][j][3]&0xff)<<24)|
                    ((vo->weightscale4x4[i][j][2]&0xff)<<16)|
                    ((vo->weightscale4x4[i][j][1]&0xff)<<8)|
                    ((vo->weightscale4x4[i][j][0]&0xff)<<0);
                VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA4x4Y_OFF+i*16+j*4,cmd,"weightscale intra4x4");
            }
        }

        for (j = 0; j < 8; j++)
        {
            cmd=((vo->weightscale8x8[1][j][3]&0xff)<<24)|
                ((vo->weightscale8x8[1][j][2]&0xff)<<16)|
                ((vo->weightscale8x8[1][j][1]&0xff)<<8)|
                ((vo->weightscale8x8[1][j][0]&0xff)<<0);
            VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+2*j*4,cmd,"weightscale inter8x8");

            cmd=((vo->weightscale8x8[1][j][7]&0xff)<<24)|
                ((vo->weightscale8x8[1][j][6]&0xff)<<16)|
                ((vo->weightscale8x8[1][j][5]&0xff)<<8)|
                ((vo->weightscale8x8[1][j][4]&0xff)<<0);
            VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTER8x8_OFF+(2*j+1)*4,cmd,"weightscale inter8x8");
        }

        for (j = 0; j < 8; j++)
        {
            cmd=((vo->weightscale8x8[0][j][3]&0xff)<<24)|
                ((vo->weightscale8x8[0][j][2]&0xff)<<16)|
                ((vo->weightscale8x8[0][j][1]&0xff)<<8)|
                ((vo->weightscale8x8[0][j][0]&0xff)<<0);
            VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+2*j*4,cmd,"weightscale intra8x8");

            cmd=((vo->weightscale8x8[0][j][7]&0xff)<<24)|
                ((vo->weightscale8x8[0][j][6]&0xff)<<16)|
                ((vo->weightscale8x8[0][j][5]&0xff)<<8)|
                ((vo->weightscale8x8[0][j][4]&0xff)<<0);
            VSP_WRITE_REG(DCT_IQW_TABLE_BASE_ADDR+INTRA8x8_OFF+(2*j+1)*4,cmd,"weightscale intra8x8");
        }
    }
}

LOCAL void H264Dec_output_one_frame (H264DecObject *vo, DEC_IMAGE_PARAMS_T *img_ptr, MMDecOutput * dec_out)
{
    DEC_VUI_T *vui_seq_parameters_ptr = vo->g_sps_ptr->vui_seq_parameters;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr=  vo->g_dpb_layer[0];	
    DEC_STORABLE_PICTURE_T *prev = dpb_ptr->delayed_pic_ptr;
    DEC_STORABLE_PICTURE_T *cur = vo->g_dec_picture_ptr;
    DEC_STORABLE_PICTURE_T *out = cur;
    int i, pics, cross_idr, out_of_order, out_idx;

    if(vui_seq_parameters_ptr->bitstream_restriction_flag && (img_ptr->has_b_frames < vui_seq_parameters_ptr->num_reorder_frames))
    {
        img_ptr->has_b_frames = vui_seq_parameters_ptr->num_reorder_frames;
//		s->low_delay = 0;
    }

	//SCI_TRACE_LOW("dec poc: %d\t", cur->poc);

    dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = cur;

    for (i = 0; i < dpb_ptr->used_size; i++)
    {
        if (cur == dpb_ptr->fs[i]->frame)
        {
            if(dpb_ptr->fs[i]->is_reference == 0)
            {
                dpb_ptr->fs[i]->is_reference = DELAYED_PIC_REF;

                if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
                {
	            (*(vo->avcHandle->VSP_bindCb))(vo->avcHandle->userdata,dpb_ptr->fs[i]->frame->pBufferHeader);			
                }
            }
        }
    }
        
    cross_idr = 0;
    for(i = 0; i < dpb_ptr->delayed_pic_num; i++)
    {
        if(dpb_ptr->delayed_pic[i]->idr_flag || (dpb_ptr->delayed_pic[i]->poc == 0))
        {
            cross_idr = 1;
        }
    }

    //find the smallest POC frame in dpb buffer
    out = dpb_ptr->delayed_pic[0];
    out_idx = 0;
    for(i = 1; (i < MAX_DELAYED_PIC_NUM) && dpb_ptr->delayed_pic[i] && !dpb_ptr->delayed_pic[i]->idr_flag; i++)
    {
        if(dpb_ptr->delayed_pic[i]->poc < out->poc)
        {
            out = dpb_ptr->delayed_pic[i];
            out_idx = i;
        }
    }

    pics = dpb_ptr->delayed_pic_num;
    out_of_order = !cross_idr && prev && (out->poc < prev->poc);
    if(vui_seq_parameters_ptr->bitstream_restriction_flag && img_ptr->has_b_frames >= vui_seq_parameters_ptr->num_reorder_frames)
    {
    }else if(prev && pics <= img_ptr->has_b_frames)
    {
        out = prev;
    }else if((out_of_order && (pics-1) == img_ptr->has_b_frames && pics < 15/*why 15?, xwluo@20120316 */)  ||
        ((vo->g_sps_ptr->profile_idc != 0x42/*!bp*/)&&(img_ptr->low_delay) && ((!cross_idr && prev && out->poc > (prev->poc + 2)) || cur->slice_type == B_SLICE)))
    {
        img_ptr->low_delay = 0;
        img_ptr->has_b_frames++;
        out = prev;
    } else if(out_of_order)
    {
        out = prev;
    }

    if (out != cur)
    {
        dpb_ptr->delayed_pic_ptr = out;
        dec_out->frameEffective = (prev == out) ? 0 : 1;
        
        //flush one frame from dpb and re-organize the delayed_pic buffer
        if(/*out_of_order ||*/ pics > img_ptr->has_b_frames || dec_out->frameEffective)
        {
            for(i = out_idx; dpb_ptr->delayed_pic[i]; i++)
            {
                dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
            }
            dpb_ptr->delayed_pic_num--;
        }
            
    }

    dec_out->reqNewBuf = 1;				
    if (dec_out->frameEffective)
    {
        dec_out->frame_width = vo->width;
        dec_out->frame_height = vo->height;

        dec_out->pOutFrameY = out->imgY;
        dec_out->pOutFrameU = out->imgU;
        dec_out->pOutFrameV = out->imgV;
        dec_out->pBufferHeader = out->pBufferHeader;
	dec_out->mPicId = out->mPicId;	

        for (i = 0; i < /*dpb_ptr->used_size*/(MAX_REF_FRAME_NUMBER+1); i++)
        {
            if (out == dpb_ptr->fs[i]->frame)
            {
                if(dpb_ptr->fs[i]->is_reference == DELAYED_PIC_REF)
                {
                    dpb_ptr->fs[i]->is_reference = 0;

                    if(dpb_ptr->fs[i]->frame->pBufferHeader!=NULL)
                    {
			(*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->fs[i]->frame->pBufferHeader);				
                        dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
                    }
                }
            }
        }
    }   

    SCI_TRACE_LOW("out poc: %d, effective: %d\t", out->poc, dec_out->frameEffective);

    return;
}



//extern BiContextType context[308];
PUBLIC MMDecRet H264Dec_decode_one_slice_data (H264DecObject *vo, MMDecOutput *dec_output_ptr)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    DEC_PPS_T	*active_pps_ptr = vo->g_active_pps_ptr;
    int *slice_info_ptr = vo->slice_info;
    int8  *list0_map_addr_ptr = vo->g_list0_map_addr;
    int8  *list1_map_addr_ptr = vo->g_list1_map_addr;

    MMDecRet ret = MMDEC_ERROR;
    uint32 cmd;

    uint16 Slice_first_mb_x, Slice_first_mb_y;
    uint8  slice_type = curr_slice_ptr->picture_type;
    uint8  weighted_en=(active_pps_ptr->weighted_bipred_idc>0)&&(slice_type==B_SLICE)||active_pps_ptr->weighted_pred_flag&&(slice_type==P_SLICE);
    int	   i,j;

    if (active_pps_ptr->entropy_coding_mode_flag)
    {
        init_contexts (vo);
    }

    Slice_first_mb_x = img_ptr->curr_mb_nr%img_ptr->frame_width_in_mbs;
    Slice_first_mb_y = img_ptr->curr_mb_nr/img_ptr->frame_width_in_mbs;

    if ((img_ptr->type == B_SLICE) && active_pps_ptr->weighted_bipred_idc == 2)
    {
        img_ptr->luma_log2_weight_denom = 5;
        img_ptr->chroma_log2_weight_denom = 5;
        img_ptr->wp_round_luma = 16;
        img_ptr->wp_round_chroma = 16;
    }//weihu

    //configure the slice info buf
    slice_info_ptr[0] = slice_type;
    slice_info_ptr[1] = img_ptr->slice_nr;
    slice_info_ptr[2] = active_pps_ptr->constrained_intra_pred_flag;
    slice_info_ptr[3] = img_ptr->direct_type;
    slice_info_ptr[4] = vo->g_active_sps_ptr->direct_8x8_reference_flag;
    slice_info_ptr[5] = curr_slice_ptr->qp;
    slice_info_ptr[6] = active_pps_ptr->chroma_qp_index_offset;
    slice_info_ptr[7] = active_pps_ptr->second_chroma_qp_index_offset;
    slice_info_ptr[8] = curr_slice_ptr->LFDisableIdc;
    slice_info_ptr[9] = active_pps_ptr->weighted_bipred_idc;
    slice_info_ptr[10] = img_ptr->framepoc;
    slice_info_ptr[11] = img_ptr->num_ref_idx_l0_active;
    slice_info_ptr[12] = mmin(slice_info_ptr[11], vo->g_list_size[0]);
    slice_info_ptr[13] = mmin(img_ptr->num_ref_idx_l1_active, vo->g_list_size[1]);

    //for(i=0;i<16;i++)
    //  slice_info[14+i]=g_list1_map_list0[i];//weihu

    slice_info_ptr[30] = vo->g_active_sps_ptr->seq_scaling_matrix_present_flag;
    slice_info_ptr[31] = active_pps_ptr->pic_scaling_matrix_present_flag;
    slice_info_ptr[32] = active_pps_ptr->weighted_pred_flag;//weihu
    slice_info_ptr[33] = img_ptr->luma_log2_weight_denom;
    slice_info_ptr[34] = img_ptr->chroma_log2_weight_denom;
    slice_info_ptr[35] = img_ptr->DPB_addr_index;
    slice_info_ptr[36] = curr_slice_ptr->LFAlphaC0Offset;
    slice_info_ptr[37] = curr_slice_ptr->LFBetaOffset;
    slice_info_ptr[38] = Slice_first_mb_x;
    slice_info_ptr[39] = Slice_first_mb_y;
    slice_info_ptr[40] = active_pps_ptr->transform_8x8_mode_flag;
    slice_info_ptr[41] = active_pps_ptr->entropy_coding_mode_flag;
    slice_info_ptr[42] = img_ptr->num_ref_idx_l1_active;
    slice_info_ptr[43] = (vo->g_active_sps_ptr->profile_idc!=0x42);
    slice_info_ptr[44] = weighted_en;
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+IMG_SIZE_OFF, ((img_ptr->frame_height_in_mbs&0xff)<<8)|img_ptr->frame_width_in_mbs&0xff,"IMG_SIZE");

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG0_OFF, ((0x1)<<31)|((curr_slice_ptr->qp&0x3f)<<25)|((img_ptr->slice_nr&0x1ff)<<16)|((img_ptr->frame_size_in_mbs&0x1fff)<<3)|slice_type&0x7,"VSP_CFG0");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, vo->g_nalu_ptr->len&0xfffff,"VSP_CFG1");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG2_OFF,((vo->g_active_sps_ptr->profile_idc!=0x42)<<31)|(((slice_info_ptr[30]||slice_info_ptr[31])&0x1)<<29)|((img_ptr->curr_mb_nr&0x1fff)<<16) |((Slice_first_mb_y&0x7f)<<8)|Slice_first_mb_x&0x7f,"VSP_CFG2");

    cmd=((slice_info_ptr[8]&0x3)<<19)|
        ((slice_info_ptr[9]&0x3)<<17)|
        ((slice_info_ptr[32]&0x1)<<16)|
        ((weighted_en&0x1)<<15)|
        ((slice_info_ptr[41]&0x1)<<14)|
        ((slice_info_ptr[40]&0x1)<<13)|
        ((slice_info_ptr[4]&0x1)<<12)|
        ((slice_info_ptr[3]&0x1)<<11)|
        ((slice_info_ptr[2]&0x1)<<10) |
        ((slice_info_ptr[37]&0x1f)<<5)|
        (slice_info_ptr[36]&0x1f);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG3_OFF,cmd,"VSP_CFG3");

    cmd=((slice_info_ptr[13]&0x1f)<<25)|
        ((slice_info_ptr[12]&0x1f)<<20)|
        (((slice_info_ptr[42]-1)&0x1f)<<15)|
        (((slice_info_ptr[11]-1)&0x1f)<<10)|
        ((slice_info_ptr[7]&0x1f)<<5)|
        ((slice_info_ptr[6]&0x1f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG4_OFF,cmd,"VSP_CFG4");

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG5_OFF,img_ptr->framepoc,"VSP_CFG5 cur_POC");

    cmd=((list0_map_addr_ptr[15]&0x3)<<30)|
        ((list0_map_addr_ptr[4]&0x3f)<<24)|
        ((list0_map_addr_ptr[3]&0x3f)<<18)|
        ((list0_map_addr_ptr[2]&0x3f)<<12)|
        ((list0_map_addr_ptr[1]&0x3f)<<6)|
        ((list0_map_addr_ptr[0]&0x3f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX0_OFF,cmd,"g_list0_map_addr 0");//next 376

    cmd=(((list0_map_addr_ptr[15]>>2)&0x3)<<30)|
        ((list0_map_addr_ptr[9]&0x3f)<<24)|
        ((list0_map_addr_ptr[8]&0x3f)<<18)|
        ((list0_map_addr_ptr[7]&0x3f)<<12)|
        ((list0_map_addr_ptr[6]&0x3f)<<6)|
        ((list0_map_addr_ptr[5]&0x3f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX1_OFF,cmd,"g_list0_map_addr 1");//next 376

    cmd=(((list0_map_addr_ptr[15]>>4)&0x3)<<30)|
        ((list0_map_addr_ptr[14]&0x3f)<<24)|
        ((list0_map_addr_ptr[13]&0x3f)<<18)|
        ((list0_map_addr_ptr[12]&0x3f)<<12)|
        ((list0_map_addr_ptr[11]&0x3f)<<6)|
        ((list0_map_addr_ptr[10]&0x3f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX2_OFF,cmd,"g_list0_map_addr 2");//next 376

    cmd=((list1_map_addr_ptr[15]&0x3)<<30)|
        ((list1_map_addr_ptr[4]&0x3f)<<24)|
        ((list1_map_addr_ptr[3]&0x3f)<<18)|
        ((list1_map_addr_ptr[2]&0x3f)<<12)|
        ((list1_map_addr_ptr[1]&0x3f)<<6)|
        ((list1_map_addr_ptr[0]&0x3f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX3_OFF,cmd,"g_list1_map_addr 0");//next 376

    cmd=(((list1_map_addr_ptr[15]>>2)&0x3)<<30)|
        ((list1_map_addr_ptr[9]&0x3f)<<24)|
        ((list1_map_addr_ptr[8]&0x3f)<<18)|
        ((list1_map_addr_ptr[7]&0x3f)<<12)|
        ((list1_map_addr_ptr[6]&0x3f)<<6)|
        ((list1_map_addr_ptr[5]&0x3f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX4_OFF,cmd,"g_list1_map_addr 1");//next 376

    cmd=(((list1_map_addr_ptr[15]>>4)&0x3)<<30)|
        ((list1_map_addr_ptr[14]&0x3f)<<24)|
        ((list1_map_addr_ptr[13]&0x3f)<<18)|
        ((list1_map_addr_ptr[12]&0x3f)<<12)|
        ((list1_map_addr_ptr[11]&0x3f)<<6)|
        ((list1_map_addr_ptr[10]&0x3f)<<0);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_ADDRIDX5_OFF,cmd,"g_list1_map_addr 2");//next 376

    //configure scaling matrix
    H264Dec_Cfg_ScalingMatix(vo);

    for (i = 0; i < 33; i++)
    {
        VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+i*4,vo->ref_list_buf[i],"ref_list_buf");
    }

    cmd = (img_ptr->chroma_log2_weight_denom<<8)|(img_ptr->luma_log2_weight_denom&0xff);
    VSP_WRITE_REG(PPA_SLICE_INFO_BASE_ADDR+132,cmd,"chroma_log2_weight_denom,luma_log2_weight_denom");

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_CLR_OFF, 0xfFF,"clear BSM_frame done int");

#define USE_INTERRUPT
#define NALU_LEN_VSP (0xFFFFF) // 20 bit. for 1M
#define NALU_LEN_MAX (0x600000) // 786 KB.

    if( vo->g_nalu_ptr->len >NALU_LEN_VSP && !active_pps_ptr->entropy_coding_mode_flag)
    {
	VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_CFG1_OFF, NALU_LEN_VSP,"VSP_CFG1");
    }

#ifdef USE_INTERRUPT
    VSP_WRITE_REG(VSP_REG_BASE_ADDR+ARM_INT_MASK_OFF,V_BIT_2,"ARM_INT_MASK, only enable VSP ACC init");//enable int //
#endif
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_INT_MASK_OFF,V_BIT_2 | V_BIT_4 | V_BIT_5,"VSP_INT_MASK, enable mbw_slice_done, vld_err, time_out");//enable int //frame done/error/timeout
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 1,"RAM_ACC_SEL");//change ram access to vsp hw
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_START_OFF,0xa|vo->is_need_init_vsp_hufftab,"VSP_START");//start vsp   vld/vld_table//load_vld_table_en

    if( vo->g_nalu_ptr->len >NALU_LEN_VSP && !active_pps_ptr->entropy_coding_mode_flag)
    {
	VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+TOTAL_BITS_OFF,NALU_LEN_MAX ,NALU_LEN_MAX, TIME_OUT_CLK, "Poll NALU_LEN_MAX");
	VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF,V_BIT_2, "clear bitcnt" );
    }	

#ifdef USE_INTERRUPT
    cmd = VSP_POLL_COMPLETE((VSPObject *)vo);
    SCI_TRACE_LOW("%s, %d, tmp: %0x", __FUNCTION__, __LINE__, cmd);
#else
    tmp = VSP_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
    while ((tmp&0x34)==0) //weihu tmp
    {
        tmp = VSP_READ_REG(GLB_REG_BASE_ADDR+VSP_INT_RAW_OFF, "check interrupt type");
    }
    //VSP_WRITE_REG(VSP_REG_BASE_ADDR+0x18,0x4,"VSP_INT_CLR");//enable int //frame done/error/timeout
#endif

    if(cmd&0x30)
    {
        vo->error_flag=1;
    } else if((cmd&0x00000004)==0x00000004)
    {
        vo->error_flag=0;
    }
    SCI_TRACE_LOW("%s, %d, error_flag: %d", __FUNCTION__, __LINE__, vo->error_flag);
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL");
    vo->is_need_init_vsp_hufftab = FALSE;

    H264Dec_exit_slice(vo);//g_image_ptr);

    img_ptr->slice_nr++;

    if (vo->error_flag)
    {
        return MMDEC_ERROR;
    }

    //if(end of picture)
    cmd = VSP_READ_REG(GLB_REG_BASE_ADDR+VSP_DBG_STS0_OFF, "check mb_x mb_y number");//weihu tmp
    SCI_TRACE_LOW("%s, %d, tmp: %0x", __FUNCTION__, __LINE__, cmd);
    if((((cmd>>8)&0xff)==(uint)(img_ptr->frame_width_in_mbs-1))&&((cmd&0xff)==(uint)(img_ptr->frame_height_in_mbs-1)))//weihu tmp
    {
        ret =MMDEC_OK;
        H264Dec_exit_picture (vo);		
		SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);
	H264Dec_output_one_frame(vo,img_ptr,dec_output_ptr);	

        vo->frame_dec_finish = TRUE;
        vo->g_dec_picture_ptr = NULL;//weihu for output
        vo->g_nFrame_dec_h264++;

    }

    return ret;
}

PUBLIC MMDecRet H264DecDecode_NALU(H264DecObject *vo, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    int i;
    MMDecRet ret = MMDEC_ERROR;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;

    curr_slice_ptr->next_header = -1;
    vo->error_flag = FALSE;

    if((dec_input_ptr->expected_IVOP) && (img_ptr->curr_mb_nr == 0))
    {
        vo->g_searching_IDR_pic = TRUE;
    }

    ret = H264Dec_Read_SPS_PPS_SliceHeader (vo);//g_nalu_ptr->buf, g_nalu_ptr->len);//weihu

    if (vo->error_flag == TRUE)
    {
        return MMDEC_ERROR;
    }

    if (vo->g_ready_to_decode_slice)
    {
        DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = curr_slice_ptr->p_Dpb;
        DEC_STORABLE_PICTURE_T *pframe;

        if (img_ptr->is_new_pic)
        {
            if (vo->is_need_init_vsp_hufftab && img_ptr->is_new_pic)//分不分cabac和cavlc
            {
                uint32 vld_table_addr;

                vld_table_addr = (uint32)H264Dec_InterMem_V2P(vo, vo->g_cavlc_tbl_ptr);
                //load_vld_table_en=1;

                VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0xc, vld_table_addr/8,"ddr vlc table start addr");
                VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_SIZE_SET_OFF, 0x45,"ddr VLC table size");
                //vd->is_need_init_vsp_hufftab = FALSE;
            }
        }

        //配置frame start address ram
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR, (vo->g_dec_picture_ptr->imgYAddr)>>3,"current Y addr");//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->imgYAddr
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+4, (vo->g_dec_picture_ptr->imgUAddr)>>3,"current UV addr");//0x480000+//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->imgUAddr
        VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+8, (vo->g_dec_picture_ptr->direct_mb_info_Addr)>>3,"current info addr");//0x6a0000+//g_dpb_layer[g_curr_slice_ptr->view_id]->fs[img_ptr->DPB_addr_index-17*g_curr_slice_ptr->view_id]->frame->direct_mb_info

        for(i = 0; i < img_ptr->num_ref_idx_l0_active; i++) //16
        {
            pframe = vo->g_list0[i];
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x80+i*4,(pframe->imgYAddr)>>3,"ref L0 Y addr");//g_dpb_layer[0]->fs[g_list0_map_addr[i]]->frame->imgYAddr
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x100+i*4,(pframe->imgUAddr)>>3,"ref L0 UV addr");//0x480000+
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x180+i*4,(pframe->direct_mb_info_Addr)>>3,"ref L0 info addr");//0x6a0000+
        }

        for(i = 0; i < img_ptr->num_ref_idx_l1_active; i++) //16
        {
            //pframe = dpb_ptr->fs[g_list1_map_addr[i]]->frame;
            pframe = vo->g_list1[i];
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0xc0+i*4,(pframe->imgYAddr)>>3,"ref L1 Y addr");
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x140+i*4,(pframe->imgUAddr)>>3,"ref L1 UV addr");//0x480000+
            VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR+0x1c0+i*4,(pframe->direct_mb_info_Addr)>>3,"ref L1 info addr");//0x6a0000+
        }

        ret = H264Dec_decode_one_slice_data (vo, dec_output_ptr);
    }

    //H264Dec_flush_left_byte (vo);
SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);
    //need IVOP but not found IDR,then return seek ivop
    if(dec_input_ptr->expected_IVOP && vo->g_searching_IDR_pic)
    {
        return MMDEC_FRAME_SEEK_IVOP;
    }

    if (vo->error_flag)
    {
        return MMDEC_ERROR;
    } else
    {
        return MMDEC_OK;
    }
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

