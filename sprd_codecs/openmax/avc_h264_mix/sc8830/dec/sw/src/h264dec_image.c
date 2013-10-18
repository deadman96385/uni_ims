/******************************************************************************
 ** File Name:    h264dec_image.c                                             *
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
PUBLIC int32 H264Dec_is_new_picture (H264DecContext *img_ptr)
{
    int32 reslt = 0;
    DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = img_ptr->g_old_slice_ptr;

    reslt |= (old_slice_ptr->pps_id != img_ptr->curr_slice_ptr->pic_parameter_set_id) ? 1 : 0;
    reslt |= (old_slice_ptr->frame_num != img_ptr->frame_num) ? 1 : 0;
    reslt |= ((old_slice_ptr->nal_ref_idc != img_ptr->nal_reference_idc) && ((old_slice_ptr->nal_ref_idc == 0) || (img_ptr->nal_reference_idc == 0))) ? 1 : 0;
    reslt |= (old_slice_ptr->idr_flag != img_ptr->idr_flag) ? 1 : 0;

    if (img_ptr->idr_flag && old_slice_ptr->idr_flag)
    {
        reslt |= (old_slice_ptr->idr_pic_id != img_ptr->idr_pic_id) ? 1 : 0;
    }

    if (img_ptr->g_active_sps_ptr->pic_order_cnt_type == 0)
    {
        reslt |= (old_slice_ptr->pic_order_cnt_lsb != img_ptr->pic_order_cnt_lsb) ? 1 : 0;
        reslt |= (old_slice_ptr->delta_pic_order_cnt_bottom != img_ptr->delta_pic_order_cnt_bottom) ? 1 : 0;
    } else if (img_ptr->g_active_sps_ptr->pic_order_cnt_type == 1)
    {
        reslt |= (old_slice_ptr->delta_pic_order_cnt[0] != img_ptr->delta_pic_order_cnt[0]) ? 1 : 0;
        reslt |= (old_slice_ptr->delta_pic_order_cnt[1] != img_ptr->delta_pic_order_cnt[1]) ? 1 : 0;
    }

    return reslt;
}

LOCAL int32 H264Dec_Divide (uint32 dividend, uint32 divisor) //quotient = divident/divisor
{
    while (dividend >= divisor)
    {
        dividend -= divisor;
    }

    return dividend; //remainder
}

PUBLIC void h264Dec_remove_frame_from_dpb (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int32 pos)
{
    int32 i;
    DEC_FRAME_STORE_T *tmp_fs_ptr;

    tmp_fs_ptr = dpb_ptr->fs[pos];

    //move empty fs to the end of list
    for (i = pos; i < dpb_ptr->used_size-1; i++)
    {
        dpb_ptr->fs[i] = dpb_ptr->fs[i+1];
    }

    //initialize the frame store
    tmp_fs_ptr->is_reference = FALSE;
    tmp_fs_ptr->is_long_term = FALSE;
    tmp_fs_ptr->is_short_term = FALSE;
    dpb_ptr->fs[dpb_ptr->used_size-1] = tmp_fs_ptr;

    dpb_ptr->used_size--;

    return;
}

PUBLIC int32 H264Dec_remove_unused_frame_from_dpb (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    int32 has_free_bfr = FALSE;

    for (i = 0; i < dpb_ptr->used_size; i++)
    {
        if ((!dpb_ptr->fs[i]->is_reference) && (dpb_ptr->fs[i]->disp_status))
        {
            h264Dec_remove_frame_from_dpb(img_ptr, dpb_ptr, i);
            has_free_bfr = TRUE;
            break;
        }
    }

    return has_free_bfr;
}

LOCAL DEC_FRAME_STORE_T *H264Dec_get_one_free_pic_buffer (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    while(dpb_ptr->used_size == (MAX_REF_FRAME_NUMBER+1))
    {
        if (!H264Dec_remove_unused_frame_from_dpb(img_ptr, dpb_ptr))
        {
            //wait for display free buffer
#if _H264_PROTECT_ & _LEVEL_LOW_
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos |= (1<<16);
#endif
            return NULL;
        }
    }

    return dpb_ptr->fs[MAX_REF_FRAME_NUMBER];
}

/*!
 ************************************************************************
 * \brief
 *    To calculate the poc values
 *        based upon JVT-F100d2
 *  POC200301: Until Jan 2003, this function will calculate the correct POC
 *    values, but the management of POCs in buffered pictures may need more work.
 * \return
 *    none
 ************************************************************************
 */
LOCAL void H264Dec_POC(H264DecContext *img_ptr)
{
    int32 i;
    DEC_SPS_T	*sps_ptr = img_ptr->g_active_sps_ptr;
    uint32 MaxPicOrderCntLsb = (1<<(sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4));

    switch (sps_ptr->pic_order_cnt_type)
    {
    case 0:	//poc mode 0
        ///1 st
        if (img_ptr->idr_flag)
        {
            img_ptr->PrevPicOrderCntMsb = 0;
            img_ptr->PrevPicOrderCntLsb = 0;
        } else
        {
            if (img_ptr->last_has_mmco_5)
            {
                if (0/*img_ptr->last_pic_bottom_field*/)
                {
                    img_ptr->PrevPicOrderCntMsb = 0;
                    img_ptr->PrevPicOrderCntLsb = 0;
                } else
                {
                    img_ptr->PrevPicOrderCntMsb = 0;
                    img_ptr->PrevPicOrderCntLsb = img_ptr->toppoc;
                }
            }
        }

        //calculate the MSBs of current picture
        if (img_ptr->pic_order_cnt_lsb < img_ptr->PrevPicOrderCntLsb &&
                (img_ptr->PrevPicOrderCntLsb - img_ptr->pic_order_cnt_lsb ) >= (MaxPicOrderCntLsb /2))
        {
            img_ptr->PicOrderCntMsb = img_ptr->PrevPicOrderCntMsb + MaxPicOrderCntLsb;
        } else if (img_ptr->pic_order_cnt_lsb > img_ptr->PrevPicOrderCntLsb &&
                   (img_ptr->pic_order_cnt_lsb - img_ptr->PrevPicOrderCntLsb) > (MaxPicOrderCntLsb /2))
        {
            img_ptr->PicOrderCntMsb = img_ptr->PrevPicOrderCntMsb - MaxPicOrderCntLsb;
        } else
        {
            img_ptr->PicOrderCntMsb = img_ptr->PrevPicOrderCntMsb;
        }

        ///2nd
        if(img_ptr->field_pic_flag==0)
        {   //frame pix
            img_ptr->toppoc = img_ptr->PicOrderCntMsb + img_ptr->pic_order_cnt_lsb;
            img_ptr->bottompoc = img_ptr->toppoc + img_ptr->delta_pic_order_cnt_bottom;
            img_ptr->ThisPOC = img_ptr->framepoc = (img_ptr->toppoc < img_ptr->bottompoc)? img_ptr->toppoc : img_ptr->bottompoc; // POC200301
        }
        else if (img_ptr->bottom_field_flag==0)
        {   //top field
            img_ptr->ThisPOC= img_ptr->toppoc = img_ptr->PicOrderCntMsb + img_ptr->pic_order_cnt_lsb;
        }
        else
        {   //bottom field
            img_ptr->ThisPOC= img_ptr->bottompoc = img_ptr->PicOrderCntMsb + img_ptr->pic_order_cnt_lsb;
        }
        img_ptr->framepoc=img_ptr->ThisPOC;

        if ( img_ptr->frame_num!=img_ptr->PreviousFrameNum)
            img_ptr->PreviousFrameNum=img_ptr->frame_num;

        if(img_ptr->nal_reference_idc)
        {
            img_ptr->PrevPicOrderCntLsb = img_ptr->pic_order_cnt_lsb;
            img_ptr->PrevPicOrderCntMsb = img_ptr->PicOrderCntMsb;
        }

        break;
    case 1:	//poc mode 1
        ///1 st
        if (img_ptr->idr_flag)
        {
            img_ptr->FrameNumOffset = 0;	//  first pix of IDRGOP,
            img_ptr->delta_pic_order_cnt[0] = 0;	//ignore first delta
            if (img_ptr->frame_num)
            {
                PRINTF("frame_num != 0 in idr pix");
                img_ptr->error_flag |= ER_BSM_ID;
            }
        } else
        {
            if (img_ptr->last_has_mmco_5)
            {
                img_ptr->PreviousFrameNumOffset = 0;
                img_ptr->PreviousFrameNum = 0;
            }

            if (img_ptr->frame_num < img_ptr->PreviousFrameNum)
            {
                //not first pix of IDRGOP
                img_ptr->FrameNumOffset = img_ptr->PreviousFrameNumOffset+ img_ptr->max_frame_num;
            } else
            {
                img_ptr->FrameNumOffset = img_ptr->PreviousFrameNumOffset;
            }
        }

        ///2nd
        if (sps_ptr->num_ref_frames_in_pic_order_cnt_cycle)
        {
            img_ptr->AbsFrameNum = img_ptr->FrameNumOffset + img_ptr->frame_num;
        } else
        {
            img_ptr->AbsFrameNum = 0;
        }

        if (!(img_ptr->nal_reference_idc) && img_ptr->AbsFrameNum > 0)
        {
            img_ptr->AbsFrameNum--;
        }

        ///3rd
        img_ptr->ExpectedDeltaPerPicOrderCntCycle = 0;

        if (sps_ptr->num_ref_frames_in_pic_order_cnt_cycle)
        {
            for (i = 0; i < (int32)sps_ptr->num_ref_frames_in_pic_order_cnt_cycle; i++)
            {
                img_ptr->ExpectedDeltaPerPicOrderCntCycle += sps_ptr->offset_for_ref_frame[i];
            }
        }

        if (img_ptr->AbsFrameNum)
        {
            img_ptr->PicOrderCntCycleCnt = (img_ptr->AbsFrameNum-1)/sps_ptr->num_ref_frames_in_pic_order_cnt_cycle;
            img_ptr->FrameNumInPicOrderCntCycle = (img_ptr->AbsFrameNum-1)%sps_ptr->num_ref_frames_in_pic_order_cnt_cycle;
            img_ptr->ExpectedPicOrderCnt = img_ptr->PicOrderCntCycleCnt*img_ptr->ExpectedDeltaPerPicOrderCntCycle;

            for (i = 0; i <= img_ptr->FrameNumInPicOrderCntCycle; i++)
            {
                img_ptr->ExpectedPicOrderCnt += sps_ptr->offset_for_ref_frame[i];
            }
        } else
        {
            img_ptr->ExpectedPicOrderCnt = 0;
        }

        if (!img_ptr->nal_reference_idc)
        {
            img_ptr->ExpectedPicOrderCnt += sps_ptr->offset_for_non_ref_pic;
        }

        if (img_ptr->field_pic_flag == 0)
        {
            //frame pix
            img_ptr->toppoc = img_ptr->ExpectedPicOrderCnt + img_ptr->delta_pic_order_cnt[0];
            img_ptr->bottompoc = img_ptr->toppoc + sps_ptr->offset_for_top_to_bottom_field + img_ptr->delta_pic_order_cnt[1];
            img_ptr->ThisPOC = img_ptr->framepoc = (img_ptr->toppoc < img_ptr->bottompoc) ? img_ptr->toppoc : img_ptr->bottompoc;	//poc 200301
        } else	if (img_ptr->bottom_field_flag == 0)
        {
            //top field
        } else
        {   //bottom field
        }
        img_ptr->framepoc = img_ptr->ThisPOC;

        img_ptr->PreviousFrameNum = img_ptr->frame_num;
        img_ptr->PreviousFrameNumOffset = img_ptr->FrameNumOffset;

        break;
    case 2: //poc mode 2
        if (img_ptr->idr_flag) 	//idr picture
        {
            img_ptr->FrameNumOffset = 0; //first pix of IDRGOP
            img_ptr->ThisPOC = img_ptr->framepoc = img_ptr->toppoc = img_ptr->bottompoc = 0;
            if (img_ptr->frame_num)
            {
                PRINTF("frame_num != 0 in idr pix");
                img_ptr->error_flag |= ER_BSM_ID;
            }
        } else
        {
            if (img_ptr->last_has_mmco_5)
            {
                img_ptr->PreviousFrameNum = 0;
                img_ptr->PreviousFrameNumOffset = 0;
            }

            if (img_ptr->frame_num < img_ptr->PreviousFrameNum)
            {
                img_ptr->FrameNumOffset = img_ptr->PreviousFrameNumOffset + img_ptr->max_frame_num;
            } else
            {
                img_ptr->FrameNumOffset = img_ptr->PreviousFrameNumOffset;
            }

            img_ptr->AbsFrameNum = img_ptr->FrameNumOffset + img_ptr->frame_num;
            if (!img_ptr->nal_reference_idc)
            {
                img_ptr->ThisPOC = (2*img_ptr->AbsFrameNum - 1);
            } else
            {
                img_ptr->ThisPOC = (2*img_ptr->AbsFrameNum);
            }

            if (img_ptr->field_pic_flag == 0)	//frame pix
            {
                img_ptr->toppoc = img_ptr->bottompoc = img_ptr->framepoc = img_ptr->ThisPOC;
            } else if (img_ptr->bottom_field_flag == 0)	//top field
            {
            } else	//bottom field
            {
            }
        }

        img_ptr->PreviousFrameNum = img_ptr->frame_num;
        img_ptr->PreviousFrameNumOffset = img_ptr->FrameNumOffset;
    }
}

LOCAL void H264Dec_fill_frame_num_gap (H264DecContext *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 curr_frame_num;
    int32 unused_short_term_frm_num;
    int tmp1 = img_ptr->delta_pic_order_cnt[0];
    int tmp2 = img_ptr->delta_pic_order_cnt[1];

    PRINTF("a gap in frame number is found, try to fill it.\n");

    unused_short_term_frm_num = H264Dec_Divide ((img_ptr->pre_frame_num+1), img_ptr->max_frame_num);
    curr_frame_num = img_ptr->frame_num;

    while (curr_frame_num != unused_short_term_frm_num)
    {
        DEC_STORABLE_PICTURE_T *picture_ptr;
        DEC_FRAME_STORE_T *frame_store_ptr = H264Dec_get_one_free_pic_buffer (img_ptr, dpb_ptr);

#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (frame_store_ptr == PNULL || frame_store_ptr->frame == PNULL)
        {
            img_ptr->error_flag |= ER_BSM_ID;
        }

        if (img_ptr->error_flag)
        {
            img_ptr->return_pos |= (1<<17);
            return;
        }
#endif

        frame_store_ptr->disp_status = 1;

        picture_ptr = frame_store_ptr->frame;
        picture_ptr->pic_num = unused_short_term_frm_num;
        picture_ptr->frame_num = unused_short_term_frm_num;
        picture_ptr->non_existing = 1;
        picture_ptr->used_for_reference = 1;
        picture_ptr->is_long_term = 0;
        picture_ptr->idr_flag = 0;
        picture_ptr->adaptive_ref_pic_buffering_flag = 0;

        picture_ptr->imgY = NULL;
        picture_ptr->imgU = NULL;
        picture_ptr->imgV = NULL;
        picture_ptr->imgYAddr = img_ptr->g_rec_buf.imgYAddr;
        picture_ptr->imgUAddr = NULL;
        picture_ptr->imgVAddr = NULL;
        picture_ptr->pBufferHeader= NULL;

        img_ptr->frame_num = unused_short_term_frm_num;
        if (img_ptr->g_active_sps_ptr->pic_order_cnt_type!=0)
        {
            H264Dec_POC(img_ptr);
        }
        picture_ptr->frame_poc=img_ptr->framepoc;
        picture_ptr->poc=img_ptr->framepoc;

        H264Dec_store_picture_in_dpb (img_ptr, dpb_ptr, picture_ptr);

        img_ptr->pre_frame_num = unused_short_term_frm_num;
        unused_short_term_frm_num = H264Dec_Divide (unused_short_term_frm_num+1, img_ptr->max_frame_num);
    }

    img_ptr->frame_num = curr_frame_num;
    img_ptr->delta_pic_order_cnt[0] = tmp1;
    img_ptr->delta_pic_order_cnt[1] = tmp2;

}

LOCAL int dumppoc(H264DecContext *img)
{
#if 0
    PRINTF ("\nPOC locals...\n");
    PRINTF ("toppoc                                %d\n", img->toppoc);
    PRINTF ("bottompoc                             %d\n", img->bottompoc);
    PRINTF ("frame_num                             %d\n", img->frame_num);
    PRINTF ("field_pic_flag                        %d\n", img->field_pic_flag);
    PRINTF ("bottom_field_flag                     %d\n", img->bottom_field_flag);
    PRINTF ("POC SPS\n");
    PRINTF ("log2_max_frame_num_minus4             %d\n", g_active_sps_ptr->log2_max_frame_num_minus4);         // POC200301
    PRINTF ("log2_max_pic_order_cnt_lsb_minus4     %d\n", g_active_sps_ptr->log2_max_pic_order_cnt_lsb_minus4);
    PRINTF ("pic_order_cnt_type                    %d\n", g_active_sps_ptr->pic_order_cnt_type);
    PRINTF ("num_ref_frames_in_pic_order_cnt_cycle %d\n", g_active_sps_ptr->num_ref_frames_in_pic_order_cnt_cycle);
    PRINTF ("delta_pic_order_always_zero_flag      %d\n", g_active_sps_ptr->delta_pic_order_always_zero_flag);
    PRINTF ("offset_for_non_ref_pic                %d\n", g_active_sps_ptr->offset_for_non_ref_pic);
    PRINTF ("offset_for_top_to_bottom_field        %d\n", g_active_sps_ptr->offset_for_top_to_bottom_field);
    PRINTF ("offset_for_ref_frame[0]               %d\n", g_active_sps_ptr->offset_for_ref_frame[0]);
    PRINTF ("offset_for_ref_frame[1]               %d\n", g_active_sps_ptr->offset_for_ref_frame[1]);
    PRINTF ("POC in SLice Header\n");
    PRINTF ("pic_order_present_flag                %d\n", g_active_pps_ptr->pic_order_present_flag);
    PRINTF ("delta_pic_order_cnt[0]                %d\n", img->delta_pic_order_cnt[0]);
    PRINTF ("delta_pic_order_cnt[1]                %d\n", img->delta_pic_order_cnt[1]);
    PRINTF ("delta_pic_order_cnt[2]                %d\n", img->delta_pic_order_cnt[2]);
    PRINTF ("idr_flag                              %d\n", img->idr_flag);
    PRINTF ("MaxFrameNum                           %d\n", img->max_frame_num);
#endif
    return 0;
}
PUBLIC void H264Dec_init_picture (H264DecContext *img_ptr)
{
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;
    DEC_STORABLE_PICTURE_T *dec_picture_ptr = NULL;
    DEC_FRAME_STORE_T *fs = NULL;
    int32 i;

    if (img_ptr->fmo_used)
    {
        memset(img_ptr->slice_nr_ptr, -1, img_ptr->frame_width_in_mbs * img_ptr->frame_height_in_mbs);
    }

    if ((img_ptr->frame_num != img_ptr->pre_frame_num) && (img_ptr->frame_num != H264Dec_Divide((img_ptr->pre_frame_num+1), img_ptr->max_frame_num)))
    {
        if (img_ptr->g_active_sps_ptr->gaps_in_frame_num_value_allowed_flag == 0)
        {
            /*advanced error concealment would be called here to combat unitentional loss of pictures*/
            SCI_TRACE_LOW("an unintentional loss of picture occures!\n");
            //	img_ptr->error_flag |= ER_BSM_ID;
            //	return;
        }
        //H264Dec_fill_frame_num_gap(img_ptr, dpb_ptr);
        H264Dec_clear_delayed_buffer(img_ptr);
    }

    fs = H264Dec_get_one_free_pic_buffer(img_ptr, dpb_ptr);

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (img_ptr->error_flag)
    {
        img_ptr->return_pos |= (1<<18);
        return;
    }

    if (!fs || fs->frame == PNULL)
    {
        img_ptr->return_pos1 |= (1<<0);
        return;
    }
#endif

    if(fs->is_reference == DELAYED_PIC_REF)
    {
        int32 out_idx;

        fs->is_reference = 0;

        if ((fs->frame->pBufferHeader!=NULL) && ((*(img_ptr->avcHandle->VSP_unbindCb)) != NULL))
        {
            (*(img_ptr->avcHandle->VSP_unbindCb))(img_ptr->avcHandle->userdata,fs->frame->pBufferHeader);
            fs->frame->pBufferHeader = NULL;
        }

        for (i = 0; i < img_ptr->g_dpb_ptr->delayed_pic_num; i++)
        {
            //printf("%d,", dpb_ptr->delayed_pic[i]->poc);
            if (fs->frame == img_ptr->g_dpb_ptr->delayed_pic[i])
            {
                out_idx = i;
                break;
            }
        }

        for(i = out_idx; dpb_ptr->delayed_pic[i]; i++)
        {
            dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
        }
        dpb_ptr->delayed_pic_num--;
    }

    fs->disp_status = 0;
    img_ptr->g_dec_picture_ptr = fs->frame;
    if (fs->frame->imgYUV[0] == 0)
    {
        int32 ext_frm_size = img_ptr->ext_width * img_ptr->ext_height;

        fs->frame->imgYUV[0] = (uint8 *)H264Dec_MemAlloc(img_ptr, ext_frm_size, 256, SW_CACHABLE);
        fs->frame->imgYUV[1] = (uint8 *)H264Dec_MemAlloc(img_ptr, ext_frm_size>>2, 256, SW_CACHABLE);
        fs->frame->imgYUV[2] = (uint8 *)H264Dec_MemAlloc(img_ptr, ext_frm_size>>2, 256, SW_CACHABLE);
    }

    img_ptr->pre_frame_num = img_ptr->frame_num;
    img_ptr->num_dec_mb = 0;

    //calculate POC
    H264Dec_POC(img_ptr);
//	dumppoc (img_ptr);

    img_ptr->constrained_intra_pred_flag = img_ptr->g_active_pps_ptr->constrained_intra_pred_flag;
    img_ptr->mb_x = 0;
    img_ptr->mb_y = 0;
    img_ptr->chroma_qp_offset = img_ptr->g_active_pps_ptr->chroma_qp_index_offset;
    img_ptr->second_chroma_qp_index_offset = img_ptr->g_active_pps_ptr->second_chroma_qp_index_offset;
    img_ptr->slice_nr = 0;
    img_ptr->curr_mb_nr = 0;
    img_ptr->error_flag = FALSE;//for error concealment
    img_ptr->return_pos = 0;
    img_ptr->return_pos1 = 0;
    img_ptr->return_pos2 = 0;

    dec_picture_ptr = img_ptr->g_dec_picture_ptr;
    dec_picture_ptr->dec_ref_pic_marking_buffer = img_ptr->dec_ref_pic_marking_buffer;
    dec_picture_ptr->slice_type = img_ptr->type;
    dec_picture_ptr->used_for_reference = (img_ptr->nal_reference_idc != 0);
    dec_picture_ptr->frame_num = img_ptr->frame_num;
    dec_picture_ptr->poc=img_ptr->framepoc;
    dec_picture_ptr->frame_poc=img_ptr->framepoc;
// 	dec_picture_ptr->pic_num = img_ptr->frame_num;
    dec_picture_ptr->idr_flag = img_ptr->idr_flag;
    dec_picture_ptr->adaptive_ref_pic_buffering_flag = img_ptr->adaptive_ref_pic_buffering_flag;
    dec_picture_ptr->no_output_of_prior_pics_flag = img_ptr->no_output_of_prior_pics_flag;
    dec_picture_ptr->is_long_term = 0;
    dec_picture_ptr->non_existing = 0;
    dec_picture_ptr->pBufferHeader= img_ptr->g_rec_buf.pBufferHeader;
    dec_picture_ptr->mPicId = img_ptr->g_rec_buf.mPicId;

    {
        int32 size_y = img_ptr->width * img_ptr->height;

        dec_picture_ptr->imgY = img_ptr->g_rec_buf.imgY;
        dec_picture_ptr->imgU = dec_picture_ptr->imgY + size_y; //g_rec_buf.imgU;
        dec_picture_ptr->imgV = dec_picture_ptr->imgU + (size_y>>2);//g_rec_buf.imgV;

        img_ptr->g_mb_cache_ptr->mb_addr[0] = dec_picture_ptr->imgYUV[0] + img_ptr->start_in_frameY;
        img_ptr->g_mb_cache_ptr->mb_addr[1] = dec_picture_ptr->imgYUV[1] + img_ptr->start_in_frameUV;
        img_ptr->g_mb_cache_ptr->mb_addr[2] = dec_picture_ptr->imgYUV[2] + img_ptr->start_in_frameUV;
    }

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (dec_picture_ptr->ref_idx_ptr[0] == NULL || dec_picture_ptr->ref_idx_ptr[1] == NULL)
    {
        img_ptr->error_flag |= ER_BSM_ID;
        img_ptr->return_pos1 |= (1<<31);
        return;
    }
#endif

#ifndef _NEON_OPT_
    memset(dec_picture_ptr->ref_idx_ptr[0], -1, (img_ptr->frame_size_in_mbs << 4) * sizeof(char));
    memset(dec_picture_ptr->ref_idx_ptr[1], -1, (img_ptr->frame_size_in_mbs << 4) * sizeof(char));
#else
    memset_refIdx_negOne(dec_picture_ptr->ref_idx_ptr[0], dec_picture_ptr->ref_idx_ptr[1], img_ptr->frame_size_in_mbs);
#endif

    return;
}

PUBLIC void H264Dec_exit_picture (H264DecContext *img_ptr)
{
    DEC_STORABLE_PICTURE_T *dec_picture_ptr = img_ptr->g_dec_picture_ptr;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;

    if (dec_picture_ptr->used_for_reference)
    {
        H264Dec_extent_frame (img_ptr, dec_picture_ptr);
    }
    H264Dec_write_disp_frame (img_ptr, dec_picture_ptr);

    //reference frame store update
    H264Dec_store_picture_in_dpb(img_ptr, dpb_ptr, dec_picture_ptr);
    if (img_ptr->last_has_mmco_5)
    {
        img_ptr->pre_frame_num = 0;
    }

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
