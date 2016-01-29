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
PUBLIC int32 H264Dec_is_new_picture (H264DecContext *vo)
{
    int32 reslt = 0;
    DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = vo->g_old_slice_ptr;

    reslt |= (old_slice_ptr->pps_id != vo->curr_slice_ptr->pic_parameter_set_id) ? 1 : 0;
    reslt |= (old_slice_ptr->frame_num != vo->frame_num) ? 1 : 0;
    reslt |= ((old_slice_ptr->nal_ref_idc != vo->nal_reference_idc) && ((old_slice_ptr->nal_ref_idc == 0) || (vo->nal_reference_idc == 0))) ? 1 : 0;
    reslt |= (old_slice_ptr->idr_flag != vo->idr_flag) ? 1 : 0;

    if (vo->idr_flag && old_slice_ptr->idr_flag) {
        reslt |= (old_slice_ptr->idr_pic_id != vo->idr_pic_id) ? 1 : 0;
    }

    if (vo->g_active_sps_ptr->pic_order_cnt_type == 0) {
        reslt |= (old_slice_ptr->pic_order_cnt_lsb != vo->pic_order_cnt_lsb) ? 1 : 0;
        reslt |= (old_slice_ptr->delta_pic_order_cnt_bottom != vo->delta_pic_order_cnt_bottom) ? 1 : 0;
    } else if (vo->g_active_sps_ptr->pic_order_cnt_type == 1) {
        reslt |= (old_slice_ptr->delta_pic_order_cnt[0] != vo->delta_pic_order_cnt[0]) ? 1 : 0;
        reslt |= (old_slice_ptr->delta_pic_order_cnt[1] != vo->delta_pic_order_cnt[1]) ? 1 : 0;
    }

    return reslt;
}

LOCAL int32 H264Dec_Divide (uint32 dividend, uint32 divisor) //quotient = divident/divisor
{
    while (dividend >= divisor) {
        dividend -= divisor;
    }

    return dividend; //remainder
}

PUBLIC void h264Dec_remove_frame_from_dpb (H264DecContext *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int32 pos)
{
    int32 i;
    DEC_FRAME_STORE_T *tmp_fs_ptr;

    tmp_fs_ptr = dpb_ptr->fs[pos];

    //move empty fs to the end of list
    for (i = pos; i < dpb_ptr->used_size-1; i++) {
        dpb_ptr->fs[i] = dpb_ptr->fs[i+1];
    }

    //initialize the frame store
    tmp_fs_ptr->is_reference = FALSE;
    tmp_fs_ptr->is_long_term = FALSE;
    tmp_fs_ptr->is_short_term = FALSE;
    dpb_ptr->fs[dpb_ptr->used_size-1] = tmp_fs_ptr;

    dpb_ptr->used_size--;

    H264DEC_UNBIND_FRAME(vo, tmp_fs_ptr->frame);

    return;
}

PUBLIC int32 H264Dec_remove_unused_frame_from_dpb (H264DecContext *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    int32 has_free_bfr = FALSE;

    for (i = 0; i < dpb_ptr->used_size; i++) {
        SCI_TRACE_LOW_DPB("%s, %d, is_reference %d  pBufferHeader %x", __FUNCTION__, __LINE__,dpb_ptr->fs[i]->is_reference ,  dpb_ptr->fs[i]->frame->pBufferHeader);
        if ((!dpb_ptr->fs[i]->is_reference)) {
            h264Dec_remove_frame_from_dpb(vo, dpb_ptr, i);
            has_free_bfr = TRUE;
            break;
        }
    }

    return has_free_bfr;
}

PUBLIC int32 H264Dec_remove_delayed_frame_from_dpb (H264DecContext *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    uint32 i,j;
    uint32 out_idx;
    int32 has_free_bfr = FALSE;
    DEC_FRAME_STORE_T *fs = NULL;

    for (i = 0; i < (uint32)(dpb_ptr->used_size); i++) {
        SCI_TRACE_LOW_DPB("%s, %d, is_reference %d  pBufferHeader %x", __FUNCTION__, __LINE__,dpb_ptr->fs[i]->is_reference ,  dpb_ptr->fs[i]->frame->pBufferHeader);
        if ((DELAYED_PIC_REF == dpb_ptr->fs[i]->is_reference)) {
            fs = dpb_ptr->fs[i];

            out_idx = dpb_ptr->delayed_pic_num;
            for (j = 0; j < dpb_ptr->delayed_pic_num; j++) {
                if (fs->frame == dpb_ptr->delayed_pic[j]) {
                    out_idx = j;
                    has_free_bfr = TRUE;
                    break;
                }
            }

            if (has_free_bfr == TRUE) {
                for(j = out_idx; dpb_ptr->delayed_pic[j]; j++) {
                    dpb_ptr->delayed_pic[j] = dpb_ptr->delayed_pic[j+1];
                }
                dpb_ptr->delayed_pic_num--;

                h264Dec_remove_frame_from_dpb(vo, dpb_ptr, i);
            }

            break;
        }
    }

    return has_free_bfr;
}

LOCAL DEC_FRAME_STORE_T *H264Dec_get_one_free_pic_buffer (H264DecContext *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    SCI_TRACE_LOW_DPB("%s, %d, %d used vs total %d", __FUNCTION__, __LINE__,dpb_ptr->used_size ,  dpb_ptr->size);

    if (dpb_ptr->used_size == (MAX_REF_FRAME_NUMBER+1)) {
        if (!H264Dec_remove_unused_frame_from_dpb(vo, dpb_ptr)) {
            if(!H264Dec_remove_delayed_frame_from_dpb(vo, dpb_ptr)) {
                //wait for display free buffer
#if _H264_PROTECT_ & _LEVEL_LOW_
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos |= (1<<16);
#endif
                return NULL;
            }
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
LOCAL void H264Dec_POC(H264DecContext *vo)
{
    int32 i;
    DEC_SPS_T *sps_ptr = vo->g_active_sps_ptr;
    uint32 MaxPicOrderCntLsb = (1<<(sps_ptr->log2_max_pic_order_cnt_lsb_minus4+4));

    switch (sps_ptr->pic_order_cnt_type) {
    case 0:	//poc mode 0
        ///1 st
        if (vo->idr_flag) {
            vo->PrevPicOrderCntMsb = 0;
            vo->PrevPicOrderCntLsb = 0;
        } else {
            if (vo->last_has_mmco_5) {
                if (0/*img_ptr->last_pic_bottom_field*/) {
                    vo->PrevPicOrderCntMsb = 0;
                    vo->PrevPicOrderCntLsb = 0;
                } else {
                    vo->PrevPicOrderCntMsb = 0;
                    vo->PrevPicOrderCntLsb = vo->toppoc;
                }
            }
        }

        //calculate the MSBs of current picture
        if (vo->pic_order_cnt_lsb < vo->PrevPicOrderCntLsb &&
                (vo->PrevPicOrderCntLsb - vo->pic_order_cnt_lsb ) >= (MaxPicOrderCntLsb /2)) {
            vo->PicOrderCntMsb = vo->PrevPicOrderCntMsb + MaxPicOrderCntLsb;
        } else if (vo->pic_order_cnt_lsb > vo->PrevPicOrderCntLsb &&
                   (vo->pic_order_cnt_lsb - vo->PrevPicOrderCntLsb) > (MaxPicOrderCntLsb /2)) {
            vo->PicOrderCntMsb = vo->PrevPicOrderCntMsb - MaxPicOrderCntLsb;
        } else {
            vo->PicOrderCntMsb = vo->PrevPicOrderCntMsb;
        }

        ///2nd
        if(vo->field_pic_flag==0) {   //frame pix
            vo->toppoc = vo->PicOrderCntMsb + vo->pic_order_cnt_lsb;
            vo->bottompoc = vo->toppoc + vo->delta_pic_order_cnt_bottom;
            vo->ThisPOC = vo->framepoc = (vo->toppoc < vo->bottompoc)? vo->toppoc : vo->bottompoc; // POC200301
        } else if (vo->bottom_field_flag==0) {   //top field
            vo->ThisPOC= vo->toppoc = vo->PicOrderCntMsb + vo->pic_order_cnt_lsb;
        } else {   //bottom field
            vo->ThisPOC= vo->bottompoc = vo->PicOrderCntMsb + vo->pic_order_cnt_lsb;
        }
        vo->framepoc=vo->ThisPOC;

        if ( vo->frame_num!=vo->PreviousFrameNum) {
            vo->PreviousFrameNum=vo->frame_num;
        }

        if(vo->nal_reference_idc) {
            vo->PrevPicOrderCntLsb = vo->pic_order_cnt_lsb;
            vo->PrevPicOrderCntMsb = vo->PicOrderCntMsb;
        }

        break;
    case 1:	//poc mode 1
        ///1 st
        if (vo->idr_flag) {
            vo->FrameNumOffset = 0;	//  first pix of IDRGOP,
            vo->delta_pic_order_cnt[0] = 0;	//ignore first delta
            if (vo->frame_num) {
                SPRD_CODEC_LOGE("frame_num != 0 in idr pix\n");
                vo->error_flag |= ER_BSM_ID;
            }
        } else {
            if (vo->last_has_mmco_5) {
                vo->PreviousFrameNumOffset = 0;
                vo->PreviousFrameNum = 0;
            }

            if (vo->frame_num < vo->PreviousFrameNum) {
                //not first pix of IDRGOP
                vo->FrameNumOffset = vo->PreviousFrameNumOffset+ vo->max_frame_num;
            } else {
                vo->FrameNumOffset = vo->PreviousFrameNumOffset;
            }
        }

        ///2nd
        if (sps_ptr->num_ref_frames_in_pic_order_cnt_cycle) {
            vo->AbsFrameNum = vo->FrameNumOffset + vo->frame_num;
        } else {
            vo->AbsFrameNum = 0;
        }

        if (!(vo->nal_reference_idc) && vo->AbsFrameNum > 0) {
            vo->AbsFrameNum--;
        }

        ///3rd
        vo->ExpectedDeltaPerPicOrderCntCycle = 0;

        if (sps_ptr->num_ref_frames_in_pic_order_cnt_cycle) {
            for (i = 0; i < (int32)(sps_ptr->num_ref_frames_in_pic_order_cnt_cycle); i++) {
                vo->ExpectedDeltaPerPicOrderCntCycle += sps_ptr->offset_for_ref_frame[i];
            }
        }

        if (vo->AbsFrameNum) {
            vo->PicOrderCntCycleCnt = (vo->AbsFrameNum-1)/sps_ptr->num_ref_frames_in_pic_order_cnt_cycle;
            vo->FrameNumInPicOrderCntCycle = (vo->AbsFrameNum-1)%sps_ptr->num_ref_frames_in_pic_order_cnt_cycle;
            vo->ExpectedPicOrderCnt = vo->PicOrderCntCycleCnt*vo->ExpectedDeltaPerPicOrderCntCycle;

            for (i = 0; i <= vo->FrameNumInPicOrderCntCycle; i++) {
                vo->ExpectedPicOrderCnt += sps_ptr->offset_for_ref_frame[i];
            }
        } else {
            vo->ExpectedPicOrderCnt = 0;
        }

        if (!vo->nal_reference_idc) {
            vo->ExpectedPicOrderCnt += sps_ptr->offset_for_non_ref_pic;
        }

        if (vo->field_pic_flag == 0) {
            //frame pix
            vo->toppoc = vo->ExpectedPicOrderCnt + vo->delta_pic_order_cnt[0];
            vo->bottompoc = vo->toppoc + sps_ptr->offset_for_top_to_bottom_field + vo->delta_pic_order_cnt[1];
            vo->ThisPOC = vo->framepoc = (vo->toppoc < vo->bottompoc) ? vo->toppoc : vo->bottompoc;	//poc 200301
        } else if (vo->bottom_field_flag == 0) {
            //top field
        } else {   //bottom field
        }
        vo->framepoc = vo->ThisPOC;

        vo->PreviousFrameNum = vo->frame_num;
        vo->PreviousFrameNumOffset = vo->FrameNumOffset;

        break;
    case 2: //poc mode 2
        if (vo->idr_flag) {	//idr picture
            vo->FrameNumOffset = 0; //first pix of IDRGOP
            vo->ThisPOC = vo->framepoc = vo->toppoc = vo->bottompoc = 0;
            if (vo->frame_num) {
                SPRD_CODEC_LOGE("frame_num != 0 in idr pix\n");
                vo->error_flag |= ER_BSM_ID;
            }
        } else {
            if (vo->last_has_mmco_5) {
                vo->PreviousFrameNum = 0;
                vo->PreviousFrameNumOffset = 0;
            }

            if (vo->frame_num < vo->PreviousFrameNum) {
                vo->FrameNumOffset = vo->PreviousFrameNumOffset + vo->max_frame_num;
            } else {
                vo->FrameNumOffset = vo->PreviousFrameNumOffset;
            }

            vo->AbsFrameNum = vo->FrameNumOffset + vo->frame_num;
            if (!vo->nal_reference_idc) {
                vo->ThisPOC = (2*vo->AbsFrameNum - 1);
            } else {
                vo->ThisPOC = (2*vo->AbsFrameNum);
            }

            if (vo->field_pic_flag == 0)	{//frame pix
                vo->toppoc = vo->bottompoc = vo->framepoc = vo->ThisPOC;
            } else if (vo->bottom_field_flag == 0) {	//top field
            } else {	//bottom field
            }
        }

        vo->PreviousFrameNum = vo->frame_num;
        vo->PreviousFrameNumOffset = vo->FrameNumOffset;
    }
}

LOCAL void H264Dec_fill_frame_num_gap (H264DecContext *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 curr_frame_num;
    int32 unused_short_term_frm_num;
    int tmp1 = vo->delta_pic_order_cnt[0];
    int tmp2 = vo->delta_pic_order_cnt[1];

    PRINTF("a gap in frame number is found, try to fill it.\n");

    unused_short_term_frm_num = H264Dec_Divide ((vo->pre_frame_num+1), vo->max_frame_num);
    curr_frame_num = vo->frame_num;

    while (curr_frame_num != unused_short_term_frm_num) {
        DEC_STORABLE_PICTURE_T *picture_ptr;
        DEC_FRAME_STORE_T *frame_store_ptr = H264Dec_get_one_free_pic_buffer (vo, dpb_ptr);
        DEC_STORABLE_PICTURE_T *prev = dpb_ptr->delayed_pic_num ? dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num-1] : PNULL;

#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (frame_store_ptr == PNULL || frame_store_ptr->frame == PNULL) {
            vo->error_flag |= ER_BSM_ID;
        }

        if (vo->error_flag) {
            vo->return_pos |= (1<<17);
            return;
        }
#endif

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
        picture_ptr->imgYAddr = vo->g_rec_buf.imgYAddr;
        picture_ptr->imgUAddr = NULL;
        picture_ptr->imgVAddr = NULL;
        picture_ptr->pBufferHeader= NULL;
        if (prev) {
            picture_ptr->imgYUV[0] = prev->imgYUV[0];
            picture_ptr->imgYUV[1] = prev->imgYUV[1];
            picture_ptr->imgYUV[2] = prev->imgYUV[2];
        }
        vo->frame_num = unused_short_term_frm_num;
        if (vo->g_active_sps_ptr->pic_order_cnt_type!=0) {
            H264Dec_POC(vo);
        }
        picture_ptr->frame_poc=vo->framepoc;
        picture_ptr->poc=vo->framepoc;

        H264Dec_store_picture_in_dpb (vo, picture_ptr, dpb_ptr);

        vo->pre_frame_num = unused_short_term_frm_num;
        unused_short_term_frm_num = H264Dec_Divide (unused_short_term_frm_num+1, vo->max_frame_num);
    }

    vo->frame_num = curr_frame_num;
    vo->delta_pic_order_cnt[0] = tmp1;
    vo->delta_pic_order_cnt[1] = tmp2;

}

LOCAL int dumppoc(H264DecContext *vo)
{
#if 0
    PRINTF ("\nPOC locals...\n");
    PRINTF ("toppoc                                %d\n", vo->toppoc);
    PRINTF ("bottompoc                             %d\n", vo->bottompoc);
    PRINTF ("frame_num                             %d\n", vo->frame_num);
    PRINTF ("field_pic_flag                        %d\n", vo->field_pic_flag);
    PRINTF ("bottom_field_flag                     %d\n", vo->bottom_field_flag);
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
    PRINTF ("delta_pic_order_cnt[0]                %d\n", vo->delta_pic_order_cnt[0]);
    PRINTF ("delta_pic_order_cnt[1]                %d\n", vo->delta_pic_order_cnt[1]);
    PRINTF ("delta_pic_order_cnt[2]                %d\n", vo->delta_pic_order_cnt[2]);
    PRINTF ("idr_flag                              %d\n", img->idr_flag);
    PRINTF ("MaxFrameNum                           %d\n", vo->max_frame_num);
#endif
    return 0;
}

PUBLIC MMDecRet H264Dec_init_picture (H264DecContext *vo)
{
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = vo->g_dpb_ptr;
    DEC_STORABLE_PICTURE_T *dec_picture_ptr = NULL;
    DEC_FRAME_STORE_T *fs = NULL;
    int32 i;

    if (vo->fmo_used) {
        memset(vo->slice_nr_ptr, -1, vo->frame_width_in_mbs * vo->frame_height_in_mbs);
    }

    if ((vo->frame_num != vo->pre_frame_num) &&
            (vo->frame_num != H264Dec_Divide((vo->pre_frame_num+1), vo->max_frame_num))) {
        if (vo->g_active_sps_ptr->gaps_in_frame_num_value_allowed_flag == 0) {
            /*advanced error concealment would be called here to combat unitentional loss of pictures*/
            SPRD_CODEC_LOGW("an unintentional loss of picture occures! pre_frame_num: %d, frame_num: %d\n",
                            vo->pre_frame_num, vo->frame_num);
            //	return;
        }
        H264Dec_fill_frame_num_gap(vo, dpb_ptr);
    }

    fs = H264Dec_get_one_free_pic_buffer(vo, dpb_ptr);

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (vo->error_flag) {
        vo->return_pos |= (1<<18);
        return MMDEC_ERROR;
    }

    if (!fs || fs->frame == PNULL) {
        vo->return_pos1 |= (1<<0);
        return MMDEC_ERROR;
    }
#endif

    if(fs->is_reference == DELAYED_PIC_REF) {
        uint32 out_idx = 0;

        fs->is_reference = 0;
        H264DEC_UNBIND_FRAME(vo, fs->frame);

        for (i = 0; i < vo->g_dpb_ptr->delayed_pic_num; i++) {
            if (fs->frame == vo->g_dpb_ptr->delayed_pic[i]) {
                out_idx = i;
                break;
            }
        }

        for(i = out_idx; dpb_ptr->delayed_pic[i]; i++) {
            dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
        }
        dpb_ptr->delayed_pic_num--;
    }

    vo->g_dec_picture_ptr = fs->frame;
    if (fs->frame->imgYUV[0] == 0) {
        int32 ext_frm_size = vo->ext_width * vo->ext_height;

        fs->frame->imgYUV[0] = (uint8 *)H264Dec_MemAlloc(vo, ext_frm_size, 256, SW_CACHABLE);
        CHECK_MALLOC(fs->frame->imgYUV[0], "fs->frame->imgYUV[0]");

        fs->frame->imgYUV[1] = (uint8 *)H264Dec_MemAlloc(vo, ext_frm_size>>2, 256, SW_CACHABLE);
        CHECK_MALLOC(fs->frame->imgYUV[1], "fs->frame->imgYUV[1]");

        fs->frame->imgYUV[2] = (uint8 *)H264Dec_MemAlloc(vo, ext_frm_size>>2, 256, SW_CACHABLE);
        CHECK_MALLOC(fs->frame->imgYUV[2], "fs->frame->imgYUV[2]");
    }

    vo->pre_frame_num = vo->frame_num;
    vo->num_dec_mb = 0;

    //calculate POC
    H264Dec_POC(vo);
//	dumppoc (img_ptr);

    vo->constrained_intra_pred_flag = vo->g_active_pps_ptr->constrained_intra_pred_flag;
    vo->mb_x = 0;
    vo->mb_y = 0;
    vo->chroma_qp_offset = vo->g_active_pps_ptr->chroma_qp_index_offset;
    vo->second_chroma_qp_index_offset = vo->g_active_pps_ptr->second_chroma_qp_index_offset;
    vo->slice_nr = 0;
    vo->curr_mb_nr = 0;
    vo->error_flag = FALSE;//for error concealment
    vo->return_pos = 0;
    vo->return_pos1 = 0;
    vo->return_pos2 = 0;

    dec_picture_ptr = vo->g_dec_picture_ptr;
    dec_picture_ptr->dec_ref_pic_marking_buffer = vo->dec_ref_pic_marking_buffer;
    dec_picture_ptr->slice_type = vo->type;
    dec_picture_ptr->used_for_reference = (vo->nal_reference_idc != 0);
    dec_picture_ptr->frame_num = vo->frame_num;
    dec_picture_ptr->poc=vo->framepoc;
    dec_picture_ptr->frame_poc=vo->framepoc;
// 	dec_picture_ptr->pic_num = img_ptr->frame_num;
    dec_picture_ptr->idr_flag = vo->idr_flag;
    dec_picture_ptr->adaptive_ref_pic_buffering_flag = vo->adaptive_ref_pic_buffering_flag;
    dec_picture_ptr->no_output_of_prior_pics_flag = vo->no_output_of_prior_pics_flag;
    dec_picture_ptr->is_long_term = 0;
    dec_picture_ptr->non_existing = 0;
    dec_picture_ptr->pBufferHeader= vo->g_rec_buf.pBufferHeader;
    dec_picture_ptr->mPicId = vo->g_rec_buf.mPicId;

    {
        int32 size_y = vo->width * vo->height;

        dec_picture_ptr->imgY = vo->g_rec_buf.imgY;
        dec_picture_ptr->imgU = dec_picture_ptr->imgY + size_y; //g_rec_buf.imgU;
        dec_picture_ptr->imgV = dec_picture_ptr->imgU + (size_y>>2);//g_rec_buf.imgV;

        vo->g_mb_cache_ptr->mb_addr[0] = dec_picture_ptr->imgYUV[0] + vo->start_in_frameY;
        vo->g_mb_cache_ptr->mb_addr[1] = dec_picture_ptr->imgYUV[1] + vo->start_in_frameUV;
        vo->g_mb_cache_ptr->mb_addr[2] = dec_picture_ptr->imgYUV[2] + vo->start_in_frameUV;
    }

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (dec_picture_ptr->ref_idx_ptr[0] == NULL || dec_picture_ptr->ref_idx_ptr[1] == NULL) {
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<31);
        return MMDEC_ERROR;
    }
#endif

#ifndef _NEON_OPT_
    memset(dec_picture_ptr->ref_idx_ptr[0], -1, (vo->frame_size_in_mbs << 4) * sizeof(char));
    memset(dec_picture_ptr->ref_idx_ptr[1], -1, (vo->frame_size_in_mbs << 4) * sizeof(char));
#else
    memset_refIdx_negOne(dec_picture_ptr->ref_idx_ptr[0], dec_picture_ptr->ref_idx_ptr[1], vo->frame_size_in_mbs);
#endif

    return MMDEC_OK;
}

PUBLIC void H264Dec_exit_picture (H264DecContext *vo)
{
    DEC_STORABLE_PICTURE_T *dec_picture_ptr = vo->g_dec_picture_ptr;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = vo->g_dpb_ptr;

    if (vo->yuv_format == YUV420SP_NV12 || vo->yuv_format == YUV420SP_NV21) {
        H264Dec_deblock_picture(vo, dec_picture_ptr);
    }

    if (dec_picture_ptr->used_for_reference) {
        H264Dec_extent_frame (vo, dec_picture_ptr);
    }
    H264Dec_write_disp_frame (vo, dec_picture_ptr);

    //reference frame store update
    H264Dec_store_picture_in_dpb(vo, dec_picture_ptr, dpb_ptr);
    if (vo->last_has_mmco_5) {
        vo->pre_frame_num = 0;
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
