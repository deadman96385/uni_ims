/******************************************************************************
 ** File Name:    h264dec_buffer.c                                            *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 03/29/2010    Xiaowei Luo     Create.                                     *
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

PUBLIC void H264Dec_clear_delayed_buffer(H264DecObject *vo)
{
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr =  vo->g_dpb_layer[0];
    int32 i;

    if (dpb_ptr->delayed_pic_ptr)
    {
        if (dpb_ptr->delayed_pic_ptr->pBufferHeader)
        {
            (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->delayed_pic_ptr->pBufferHeader);
            dpb_ptr->delayed_pic_ptr->pBufferHeader = NULL;
        }
        dpb_ptr->delayed_pic_ptr = NULL;
    }

    for (i = 0; dpb_ptr->delayed_pic[i]; i++)
    {
        int32 j;

        for (j = 0; j <  (MAX_REF_FRAME_NUMBER+1); j++)
        {
            if (dpb_ptr->delayed_pic[i] == dpb_ptr->fs[j]->frame)
            {
                if(dpb_ptr->fs[j]->is_reference == DELAYED_PIC_REF)
                {
                    dpb_ptr->fs[j]->is_reference = 0;

                    if(dpb_ptr->fs[j]->frame->pBufferHeader!=NULL)
                    {
                        (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->fs[j]->frame->pBufferHeader);
                        dpb_ptr->fs[j]->frame->pBufferHeader = NULL;
                    }
                }
            }
        }

        dpb_ptr->delayed_pic[i] = NULL;
        dpb_ptr->delayed_pic_num --;
    }

    if( 0 != dpb_ptr->delayed_pic_num )
    {
        SCI_TRACE_LOW("%s: delayed_pic_num is %d\n", __FUNCTION__, dpb_ptr->delayed_pic_num);
    }
}

PUBLIC MMDecRet H264Dec_init_img_buffer (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    int32 mb_num_x = img_ptr->frame_width_in_mbs;
    int32 total_mb_num = mb_num_x * img_ptr->frame_height_in_mbs;
    uint8 *direct_mb_info_v;
    uint32 buffer_num, buffer_size, i;

    // Malloc direct mb info buffers
    buffer_num = MAX_REF_FRAME_NUMBER+1;
    buffer_size =  total_mb_num  * 80;

    for(i =0; i < buffer_num; i++)
    {
        direct_mb_info_v = H264Dec_MemAlloc (vo, buffer_size, 8, HW_NO_CACHABLE);
        CHECK_MALLOC(direct_mb_info_v, "direct_mb_info_v");
        vo->direct_mb_info_addr[i] = (uint32)H264Dec_MemV2P(vo, direct_mb_info_v, HW_NO_CACHABLE);
    }

    vo->g_cavlc_tbl_ptr = (uint32 *)H264Dec_MemAlloc(vo, sizeof(uint32)*69, 8, HW_NO_CACHABLE);
    CHECK_MALLOC(vo->g_cavlc_tbl_ptr, "vo->g_cavlc_tbl_ptr");

    //copy cavlc tbl to phy addr of g_cavlc_tbl_ptr.
    memcpy(vo->g_cavlc_tbl_ptr, g_huff_tab_token, sizeof(uint32)*69);

    return MMDEC_OK;
}

PUBLIC MMDecRet H264Dec_init_dpb (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int type)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    int32 i;
    int32 frm_size = vo->width * vo->height;
    int32 frm_size_in_blk = (frm_size >> 4);

    dpb_ptr->used_size = 0;
    dpb_ptr->ref_frames_in_buffer = 0;
    dpb_ptr->ltref_frames_in_buffer = 0;

    dpb_ptr->num_ref_frames = vo->g_active_sps_ptr->num_ref_frames;

    dpb_ptr->fs = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1), 4, INTER_MEM);
    CHECK_MALLOC(dpb_ptr->fs, "dpb_ptr->fs");

    dpb_ptr->fs_ref = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1), 4, INTER_MEM);
    CHECK_MALLOC(dpb_ptr->fs_ref, "dpb_ptr->fs_ref");

    dpb_ptr->fs_ltref = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1), 4, INTER_MEM);
    CHECK_MALLOC(dpb_ptr->fs_ltref, "dpb_ptr->fs_ltref");

    dpb_ptr->fs_ilref = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*(1), 4, INTER_MEM);
    CHECK_MALLOC(dpb_ptr->fs_ilref, "dpb_ptr->fs_ilref");

    for (i = 0; i < (MAX_REF_FRAME_NUMBER+1); i++)
    {
        //each storable_picture buffer is bonding to a frame store
        dpb_ptr->fs[i] = (DEC_FRAME_STORE_T *)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T), 4, INTER_MEM);
        CHECK_MALLOC(dpb_ptr->fs[i], "dpb_ptr->fs[i]");

        dpb_ptr->fs[i]->frame = (DEC_STORABLE_PICTURE_T *)H264Dec_MemAlloc(vo, sizeof(DEC_STORABLE_PICTURE_T), 4, INTER_MEM);
        CHECK_MALLOC(dpb_ptr->fs[i]->frame, "dpb_ptr->fs[i]->frame");

        dpb_ptr->fs[i]->frame->direct_mb_info =PNULL;//(int32 *)H264Dec_ExtraMemAlloc_64WordAlign((frm_size_in_blk>>4) * 20*sizeof(int32));//weihu
        dpb_ptr->fs[i]->frame->imgY = PNULL;
        dpb_ptr->fs[i]->frame->imgU = PNULL;
        dpb_ptr->fs[i]->frame->imgV = PNULL;
        dpb_ptr->fs[i]->frame->imgYAddr = (uint32)dpb_ptr->fs[i]->frame->imgY>>8;  //y;
        dpb_ptr->fs[i]->frame->imgUAddr = (uint32)dpb_ptr->fs[i]->frame->imgU>>8;  //u;
        dpb_ptr->fs[i]->frame->imgVAddr = (uint32)dpb_ptr->fs[i]->frame->imgV>>8;  //v;
#if _MVC_
        dpb_ptr->fs[i]->view_id = -1;//MVC_INIT_VIEW_ID;
        dpb_ptr->fs[i]->inter_view_flag[0] = dpb_ptr->fs[i]->inter_view_flag[1] = 0;
        dpb_ptr->fs[i]->anchor_pic_flag[0] = dpb_ptr->fs[i]->anchor_pic_flag[1] = 0;
#endif

        dpb_ptr->fs[i]->frame->dec_ref_pic_marking_buffer = NULL;

        dpb_ptr->fs_ref[i] = NULL;
        dpb_ptr->fs_ltref[i] = NULL;

        //init
        dpb_ptr->fs[i]->is_reference = 0;
        dpb_ptr->fs[i]->is_long_term = 0;
        dpb_ptr->fs[i]->is_short_term = 0;
        dpb_ptr->fs[i]->long_term_frame_idx = 0;
        dpb_ptr->fs[i]->disp_status = 0;
        dpb_ptr->fs[i]->frame_num = 0;
        dpb_ptr->fs[i]->poc = 0;
        dpb_ptr->fs[i]->pic_num =0;
        dpb_ptr->fs[i]->frame_num_wrap=0;
        dpb_ptr->fs[i]->view_id=0;
    }

#if _MVC_
    if (type == 2)
    {
        dpb_ptr->fs_ilref[0] = (DEC_FRAME_STORE_T *)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T), 4, INTER_MEM);
        CHECK_MALLOC(dpb_ptr->fs_ilref[0], "dpb_ptr->fs_ilref[0]");

        // These may need some cleanups
        dpb_ptr->fs_ilref[0]->view_id = -1;//MVC_INIT_VIEW_ID;
        dpb_ptr->fs_ilref[0]->inter_view_flag[0] = dpb_ptr->fs_ilref[0]->inter_view_flag[1] = 0;
        dpb_ptr->fs_ilref[0]->anchor_pic_flag[0] = dpb_ptr->fs_ilref[0]->anchor_pic_flag[1] = 0;
        // given that this is in a different buffer, do we even need proc_flag anymore?
    } else
    {
        dpb_ptr->fs_ilref[0] = NULL;
    }
#endif

    for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
    {
        dpb_ptr->fs[i]->frame->DPB_addr_index = i + (type==1 ? 0 : MAX_REF_FRAME_NUMBER+1);//weihu
        dpb_ptr->fs[i]->frame->direct_mb_info_Addr = vo->direct_mb_info_addr[i];
    }

#if _MVC_
    dpb_ptr->last_output_view_id = -1;
    dpb_ptr->init_done = 1;
#endif
    dpb_ptr->size = MAX_REF_FRAME_NUMBER;

    for(i = 0; i < MAX_DELAYED_PIC_NUM; i++)
    {
        dpb_ptr->delayed_pic[i] = NULL;
    }
    dpb_ptr->delayed_pic_ptr= NULL;
    dpb_ptr->delayed_pic_num = 0;

    return MMDEC_OK;
}

LOCAL void H264Dec_unmark_for_reference (H264DecObject *vo, DEC_FRAME_STORE_T *fs_ptr)
{
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr =  vo->g_dpb_layer[0];
    int32 i;

    fs_ptr->is_reference = 0;

    for (i = 0; dpb_ptr->delayed_pic[i]; i++)
    {
        if (fs_ptr->frame == dpb_ptr->delayed_pic[i])
        {
            fs_ptr->is_reference = DELAYED_PIC_REF;
        }
    }

    if(fs_ptr->frame->pBufferHeader!=NULL)
    {
        if(!fs_ptr->is_reference)
        {
            (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,fs_ptr->frame->pBufferHeader);
            fs_ptr->frame->pBufferHeader = NULL;
        }
    }

    fs_ptr->is_long_term = 0;
    fs_ptr->is_short_term = 0;

    return;
}

LOCAL void H264Dec_get_smallest_poc (H264DecObject *vo, int32 *poc, int32 * pos, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;

    if (dpb_ptr->used_size<1)
    {
        SCI_TRACE_LOW ("Cannot determine smallest POC, DPB empty.");
        vo->error_flag |= ER_REF_FRM_ID;
        return;
    }

    *pos=-1;
    *poc = SINT_MAX;
    for (i=0; i<dpb_ptr->used_size; i++)
    {
        if (*poc > dpb_ptr->fs[i]->poc)
        {
            *poc = dpb_ptr->fs[i]->poc;
            *pos=i;
        }
    }
    return;
}

LOCAL void H264Dec_output_one_frame_from_dpb (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 poc, pos;
    DEC_FRAME_STORE_T **fs = dpb_ptr->fs;
    DEC_STORABLE_PICTURE_T *frame;

    H264Dec_get_smallest_poc(vo, &poc, &pos, dpb_ptr);
    if (pos<0)
    {
        vo->error_flag |= ER_REF_FRM_ID;
        return;
    }

    frame = fs[pos]->frame;

    if (!fs[pos]->is_reference)
    {
        h264Dec_remove_frame_from_dpb(vo,dpb_ptr, pos);
    }
}

PUBLIC void H264Dec_flush_dpb (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    int32 disp_num = 0;
    DEC_FRAME_STORE_T *tmp_fs_ptr;

    //mark all frame unused
    for (i = 0; i < dpb_ptr->used_size; i++)
    {
        H264Dec_unmark_for_reference (vo, dpb_ptr->fs[i]);

        if ((dpb_ptr->fs[i]->is_reference != DELAYED_PIC_REF))
        {
            tmp_fs_ptr = dpb_ptr->fs[i];
            dpb_ptr->fs[i] = dpb_ptr->fs[disp_num];
            dpb_ptr->fs[disp_num] = tmp_fs_ptr;
            disp_num++;
        }
    }

    dpb_ptr->used_size = disp_num;

    while(dpb_ptr->used_size)
    {
        H264Dec_output_one_frame_from_dpb(vo, dpb_ptr);
    }

    return;
}

LOCAL void H264Dec_idr_memory_management (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;

    if (picture_ptr->no_output_of_prior_pics_flag)
    {
        //nothing
    } else
    {
        H264Dec_flush_dpb (vo, dpb_ptr);
    }

    if (img_ptr->long_term_reference_flag)
    {
        picture_ptr->is_long_term = 1;
        picture_ptr->long_term_frame_idx = 0;
        picture_ptr->long_term_pic_num = 0;
    }
#if _MVC_
    dpb_ptr->last_output_view_id = -1;
#endif

    return;
}

LOCAL int32 H264Dec_get_pic_num_x (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1)
{
    int32 curr_pic_num = picture_ptr->frame_num;

    return (curr_pic_num - (difference_of_pic_nums_minus1 + 1));
}

LOCAL void H264Dec_mm_unmark_short_term_for_reference (H264DecObject *vo,DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 pic_num_x;
    int32 i;

    pic_num_x = H264Dec_get_pic_num_x (picture_ptr, difference_of_pic_nums_minus1);
    for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
    {
        if ((dpb_ptr->fs_ref[i]->is_reference) && (dpb_ptr->fs_ref[i]->is_long_term == 0))
        {
            if (dpb_ptr->fs_ref[i]->frame->pic_num == pic_num_x)
            {
                H264Dec_unmark_for_reference(vo, dpb_ptr->fs_ref[i]);
                return;
            }
        }
    }

    return;
}

LOCAL void H264Dec_mm_unmark_long_term_for_reference (H264DecObject *vo, DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_pic_num, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
    {
        if ((dpb_ptr->fs_ltref[i]->is_reference) && (dpb_ptr->fs_ltref[i]->is_long_term))
        {
            if (dpb_ptr->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num)
            {
                H264Dec_unmark_for_reference (vo, dpb_ptr->fs_ltref[i]);
                return;
            }
        }
    }
    return;
}
LOCAL void H264Dec_unmark_long_term_for_reference_by_frame_idx (H264DecObject *vo,int32 long_term_frame_idx, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
    {
        if (dpb_ptr->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx)
        {
            H264Dec_unmark_for_reference (vo, dpb_ptr->fs_ltref[i]);
        }
    }

    return;
}

LOCAL void H264Dec_mark_pic_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx, int32 pic_num_x, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
    {
        if (dpb_ptr->fs_ref[i]->is_reference)
        {
            if ((!dpb_ptr->fs_ref[i]->frame->is_long_term) && (dpb_ptr->fs_ref[i]->frame->pic_num == pic_num_x))
            {
                dpb_ptr->fs_ref[i]->long_term_frame_idx = long_term_frame_idx;
                dpb_ptr->fs_ref[i]->is_long_term = 1;
                dpb_ptr->fs_ref[i]->is_short_term = 0;

                dpb_ptr->fs_ref[i]->frame->long_term_frame_idx = long_term_frame_idx;
                dpb_ptr->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
                dpb_ptr->fs_ref[i]->frame->is_long_term = 1;

                return;
            }
        }
    }

    return;
}

LOCAL void H264Dec_mm_assign_long_term_frame_idx (H264DecObject *vo, DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1, int32 long_term_frame_idx, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 pic_num_x;

    pic_num_x = H264Dec_get_pic_num_x (picture_ptr, difference_of_pic_nums_minus1);
    H264Dec_unmark_long_term_for_reference_by_frame_idx (vo, long_term_frame_idx, dpb_ptr);
    H264Dec_mark_pic_long_term (picture_ptr, long_term_frame_idx, pic_num_x, dpb_ptr);
}

LOCAL void H264Dec_update_ref_list (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i,j;
    int32 max_pos;
    DEC_FRAME_STORE_T *tmp_frame_ptr;

    for (i = 0, j = 0; i < dpb_ptr->used_size; i++)
    {
        if (dpb_ptr->fs[i]->is_reference && dpb_ptr->fs[i]->is_short_term)
        {
            dpb_ptr->fs_ref[j++] = dpb_ptr->fs[i];
        }
    }

    dpb_ptr->ref_frames_in_buffer = j;

    //sort
    for (j = 0; j < dpb_ptr->ref_frames_in_buffer; j++)
    {
        max_pos = j;

        for (i = j+1; i < dpb_ptr->ref_frames_in_buffer; i++)
        {
            if (dpb_ptr->fs_ref[i]->pic_num > dpb_ptr->fs_ref[max_pos]->pic_num)
            {
                max_pos = i;
            }
        }

        //exchange
        tmp_frame_ptr = dpb_ptr->fs_ref[j];
        dpb_ptr->fs_ref[j] = dpb_ptr->fs_ref[max_pos];
        dpb_ptr->fs_ref[max_pos] = tmp_frame_ptr;
    }

    return;
}

LOCAL void H264Dec_update_ltref_list (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i,j;
    int32 min_pos;
    DEC_FRAME_STORE_T *tmp_frame_ptr;

    for (i = 0, j = 0; i < dpb_ptr->used_size; i++)
    {
        if (dpb_ptr->fs[i]->is_reference && dpb_ptr->fs[i]->is_long_term)
        {
            dpb_ptr->fs_ltref[j++] = dpb_ptr->fs[i];
        }
    }

    dpb_ptr->ltref_frames_in_buffer = j;

    //sort
    for (j = 0; j < dpb_ptr->ltref_frames_in_buffer; j++)
    {
        min_pos = j;

        for (i = j+1; i < dpb_ptr->ltref_frames_in_buffer; i++)
        {
            if (dpb_ptr->fs_ltref[i]->long_term_frame_idx < dpb_ptr->fs_ltref[min_pos]->long_term_frame_idx)
            {
                min_pos = i;
            }
        }

        //exchange
        tmp_frame_ptr = dpb_ptr->fs_ltref[j];
        dpb_ptr->fs_ltref[j] = dpb_ptr->fs_ltref[min_pos];
        dpb_ptr->fs_ltref[min_pos] = tmp_frame_ptr;
    }

    return;
}

//set new max long_term_frame_idx
LOCAL void H264Dec_mm_update_max_long_term_frame_idx (H264DecObject *vo, int32 max_long_term_frame_idx_plus1, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    dpb_ptr->max_long_term_pic_idx = max_long_term_frame_idx_plus1 -1;

    //check for invalid frames
    for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
    {
        if (dpb_ptr->fs_ltref[i]->long_term_frame_idx > dpb_ptr->max_long_term_pic_idx)
        {
            H264Dec_unmark_for_reference (vo, dpb_ptr->fs_ltref[i]);
        }
    }

    return;
}

//mark all short term reference pictures unused for reference
LOCAL void H264Dec_mm_unmark_all_short_term_for_reference (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;
    for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
    {
        H264Dec_unmark_for_reference(vo, dpb_ptr->fs_ref[i]);
    }

    H264Dec_update_ref_list (dpb_ptr);

    return;
}

LOCAL void H264Dec_mm_unmark_all_long_term_for_reference (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    H264Dec_mm_update_max_long_term_frame_idx(vo, 0, dpb_ptr);
}

//mark the current picture used for long term reference
LOCAL void H264Dec_mm_mark_current_picture_long_term (H264DecObject *vo,DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    //remove long term pictures with same long_term_frame_idx
    H264Dec_unmark_long_term_for_reference_by_frame_idx (vo, long_term_frame_idx, dpb_ptr);

    picture_ptr->is_long_term = 1;
    picture_ptr->long_term_frame_idx = long_term_frame_idx;
    picture_ptr->long_term_pic_num = long_term_frame_idx;

    return;
}

LOCAL void H264Dec_adaptive_memory_management (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;
    int32 i = 0;

    img_ptr->last_has_mmco_5 = 0;

    while (i < vo->g_dec_ref_pic_marking_buffer_size)
    {
        tmp_drpm_ptr = &(picture_ptr->dec_ref_pic_marking_buffer[i]);

        switch (tmp_drpm_ptr->memory_management_control_operation)
        {
        case 0:
            if (i != vo->g_dec_ref_pic_marking_buffer_size-1)
            {
                vo->error_flag |= ER_REF_FRM_ID;
                SCI_TRACE_LOW("memory_management_control_operation = 0 not last operation buffer");
                return;
            }
            break;
        case 1:
            H264Dec_mm_unmark_short_term_for_reference (vo, picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1, dpb_ptr);
            H264Dec_update_ref_list(dpb_ptr);
            break;
        case 2:
            H264Dec_mm_unmark_long_term_for_reference (vo, picture_ptr, tmp_drpm_ptr->long_term_pic_num, dpb_ptr);
            H264Dec_update_ltref_list (dpb_ptr);
            break;
        case 3:
            H264Dec_mm_assign_long_term_frame_idx (vo, picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1, tmp_drpm_ptr->long_term_frame_idx, dpb_ptr);
            H264Dec_update_ref_list (dpb_ptr);
            H264Dec_update_ltref_list(dpb_ptr);
            break;
        case 4:
            H264Dec_mm_update_max_long_term_frame_idx (vo, tmp_drpm_ptr->max_long_term_frame_idx_plus1, dpb_ptr);
            H264Dec_update_ltref_list (dpb_ptr);
            break;
        case 5:
            H264Dec_mm_unmark_all_short_term_for_reference (vo, dpb_ptr);
            H264Dec_mm_unmark_all_long_term_for_reference (vo, dpb_ptr);
            img_ptr->last_has_mmco_5 = 1;
            break;
        case 6:
            H264Dec_mm_mark_current_picture_long_term (vo, picture_ptr, tmp_drpm_ptr->long_term_frame_idx, dpb_ptr);
            if((int32)(dpb_ptr->ltref_frames_in_buffer +dpb_ptr->ref_frames_in_buffer)>(mmax(1, dpb_ptr->num_ref_frames)))
            {
                vo->error_flag |= ER_REF_FRM_ID;
                SCI_TRACE_LOW ("max.number of reference frame exceed. invalid stream.");
                return;
            }
            break;
        default:
        {
            vo->error_flag |= ER_REF_FRM_ID;
            SCI_TRACE_LOW ("invalid memory_management_control_operation in buffer");
            return;
        }
        }
        i++;
    }

    if(img_ptr->last_has_mmco_5)
    {
        picture_ptr->pic_num = picture_ptr->frame_num = 0;
        img_ptr->toppoc -= picture_ptr->poc;
        img_ptr->bottompoc -= picture_ptr->poc;
        picture_ptr->poc -= picture_ptr->poc;
        img_ptr->framepoc = picture_ptr->poc;
        img_ptr->ThisPOC = picture_ptr->poc;
        H264Dec_flush_dpb(vo, dpb_ptr);
    }

    return;
}

LOCAL void H264Dec_insert_picture_in_display_list (DEC_FRAME_STORE_T *fs_ptr)
{
    //TBD

    return;
}

//mark the oldest short term reference to unref
LOCAL void H264Dec_sliding_window_memory_management (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
    int32 i;

    if (dpb_ptr->ref_frames_in_buffer >= (dpb_ptr->num_ref_frames - dpb_ptr->ltref_frames_in_buffer))//for error ==
    {
        for (i = 0; i < dpb_ptr->used_size; i++)
        {
            if ((dpb_ptr->fs[i]->is_reference) && (dpb_ptr->fs[i]->is_short_term))
            {
                H264Dec_unmark_for_reference (vo, dpb_ptr->fs[i]);
                break;
            }
        }
    }

    picture_ptr->is_long_term = 0;

    return;
}

//if current frame is used as reference frame, put it to first unused store frame in dpb
LOCAL void H264Dec_insert_picture_in_dpb (H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_FRAME_STORE_T *curr_fs_ptr, DEC_STORABLE_PICTURE_T *picture_ptr, int8 put_to_dpb)
{
    int32 used_size = dpb_ptr->used_size;
    DEC_FRAME_STORE_T *tmp_fs_ptr;



    if (picture_ptr->used_for_reference)
    {
        curr_fs_ptr->is_reference = 1;

        if(curr_fs_ptr->frame->pBufferHeader!=NULL)
        {
            (*(vo->avcHandle->VSP_bindCb))(vo->avcHandle->userdata,curr_fs_ptr->frame->pBufferHeader);
        }

        if (picture_ptr->is_long_term)
        {
            curr_fs_ptr->is_long_term = 1;
            curr_fs_ptr->is_short_term = 0;
            curr_fs_ptr->long_term_frame_idx = picture_ptr->long_term_frame_idx;
        } else
        {
            curr_fs_ptr->is_short_term = 1;
            curr_fs_ptr->is_long_term = 0;
        }
    } else
    {
        curr_fs_ptr->is_short_term = 0;
        curr_fs_ptr->is_long_term = 0;
    }

#if _MVC_
    curr_fs_ptr->view_id = picture_ptr->view_id;
    curr_fs_ptr->inter_view_flag[0] = curr_fs_ptr->inter_view_flag[1] = picture_ptr->inter_view_flag;
    curr_fs_ptr->anchor_pic_flag[0] = curr_fs_ptr->anchor_pic_flag[1] = picture_ptr->anchor_pic_flag;
#endif

    curr_fs_ptr->frame = picture_ptr;
    curr_fs_ptr->frame_num = picture_ptr->frame_num;
    curr_fs_ptr->poc = picture_ptr->poc;
    if(put_to_dpb)//james
    {
        //put the current frame store to the first unused frame store of dpb
        tmp_fs_ptr = dpb_ptr->fs[used_size];
        dpb_ptr->fs[used_size] = curr_fs_ptr;
        curr_fs_ptr = tmp_fs_ptr;

        dpb_ptr->fs[MAX_REF_FRAME_NUMBER] = tmp_fs_ptr;

        if (!picture_ptr->non_existing)
        {
            // 	dpb_ptr->fs[used_size]->disp_status = 1;
            //	H264Dec_insert_picture_in_display_list (dpb_ptr->fs[used_size]); //insert picture in display list
        }

        dpb_ptr->used_size++;
    }

    return;
}

#if _MVC_
void store_proc_picture_in_dpb(H264DecObject *vo, DEC_DECODED_PICTURE_BUFFER_T *p_Dpb, DEC_STORABLE_PICTURE_T *p)
{
    int8 put_to_dpb = 0;
    DEC_FRAME_STORE_T *fs = p_Dpb->fs_ilref[0];
    {
        fs->frame = NULL;
        fs->is_reference = 0;
    }

    H264Dec_insert_picture_in_dpb(vo, p_Dpb, fs, p, put_to_dpb);

}
#endif

LOCAL void dump_dpb(DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
#if 0//DUMP_DPB
    unsigned i;
    PRINTF ("\n");
    for (i=0; i < (dpb_ptr->used_size); i++)
    {
        PRINTF("(");
        PRINTF("fn=%d  ", dpb_ptr->fs[i]->frame_num);
        PRINTF("F: poc=%d  ", dpb_ptr->fs[i]->frame->poc);
        PRINTF("G: poc=%d)  ", dpb_ptr->fs[i]->poc);

        if (dpb_ptr->fs[i]->is_reference) PRINTF ("ref (%d) ", dpb_ptr->fs[i]->is_reference);

        if (dpb_ptr->fs[i]->is_long_term) PRINTF ("lt_ref (%d) ", dpb_ptr->fs[i]->is_reference);

        if (dpb_ptr->fs[i]->disp_status) PRINTF ("out ");

        if (dpb_ptr->fs[i]->frame->non_existing) PRINTF ("non_existing  ");

        PRINTF ("\n");
    }
#endif
}

PUBLIC void H264Dec_store_picture_in_dpb (H264DecObject *vo, DEC_STORABLE_PICTURE_T *picture_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    int8 put_to_dpb = 1;
    int try_cnt = 0;

    img_ptr->last_has_mmco_5 = 0;

    if (picture_ptr->idr_flag)
    {
        H264Dec_idr_memory_management (vo, dpb_ptr, picture_ptr);
    } else
    {
        if (picture_ptr->used_for_reference && (picture_ptr->adaptive_ref_pic_buffering_flag))
        {
            H264Dec_adaptive_memory_management (vo, dpb_ptr, picture_ptr);
        }
    }

    if (vo->error_flag)
    {
        return;
    }

    if ((!picture_ptr->idr_flag) && (picture_ptr->used_for_reference && (!picture_ptr->adaptive_ref_pic_buffering_flag)))
    {
        H264Dec_sliding_window_memory_management (vo, dpb_ptr, picture_ptr);
    }
    SCI_TRACE_LOW_DPB("%s, %d, %d used vs total %d", __FUNCTION__, __LINE__,dpb_ptr->used_size ,  dpb_ptr->size);

    if (dpb_ptr->used_size >= dpb_ptr->size)
    {
        // first try to remove unused frames
        if(!H264Dec_remove_unused_frame_from_dpb(vo, dpb_ptr))
            H264Dec_remove_delayed_frame_from_dpb(vo, dpb_ptr);
    }
    SCI_TRACE_LOW_DPB("%s, %d, %d used vs total %d", __FUNCTION__, __LINE__,dpb_ptr->used_size ,  dpb_ptr->size);


    H264Dec_insert_picture_in_dpb (vo, dpb_ptr, dpb_ptr->fs[MAX_REF_FRAME_NUMBER], picture_ptr, put_to_dpb);


    H264Dec_update_ref_list (dpb_ptr);

    H264Dec_update_ltref_list (dpb_ptr);

    if (img_ptr->last_has_mmco_5)
    {
        img_ptr->pre_frame_num = 0;
    }

//	dump_dpb();

    return;
}

LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_short_term_pic (H264DecObject *vo, int32 pic_num, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
    int32 i;

    for (i = 0; i < (dpb_ptr->ref_frames_in_buffer); i++)
    {
        if (dpb_ptr->fs_ref[i] == NULL)
        {
            vo->error_flag |= ER_REF_FRM_ID;
            return NULL;//weihu//return g_no_reference_picture_ptr
        }

        if (dpb_ptr->fs_ref[i]->is_reference)
        {
            if (dpb_ptr->fs_ref[i]->frame == NULL)
            {
                vo->error_flag |= ER_REF_FRM_ID;
                return NULL;//weihu
            }

            if ((!dpb_ptr->fs_ref[i]->frame->is_long_term) && (dpb_ptr->fs_ref[i]->frame->pic_num == pic_num))
            {
                return dpb_ptr->fs_ref[i]->frame;
            }
        }
    }

    return vo->g_no_reference_picture_ptr;
}

LOCAL void H264Dec_reorder_short_term (H264DecObject *vo, DEC_STORABLE_PICTURE_T **ref_picture_listX_ptr, int32 num_ref_idx_lX_active_minus1, int32 pic_num_lx, int32 *ref_idx_lx)
{
    int32 c_idx, n_idx;
    DEC_STORABLE_PICTURE_T *pic_lx_ptr;

    pic_lx_ptr = H264Dec_get_short_term_pic (vo, pic_num_lx, vo->g_curr_slice_ptr->p_Dpb);

    for (c_idx = (num_ref_idx_lX_active_minus1+1); c_idx > *ref_idx_lx; c_idx--)//先把ref_idx位置开始的左右参考帧图像往后移
    {
        ref_picture_listX_ptr[c_idx] = ref_picture_listX_ptr[c_idx-1];
    }

    ref_picture_listX_ptr[(*ref_idx_lx)++] = pic_lx_ptr;//然后把算出来的那个参考帧放在这里

    n_idx = *ref_idx_lx;

    for (c_idx = (*ref_idx_lx); c_idx <= (num_ref_idx_lX_active_minus1+1); c_idx++)//把那个算出来的参考帧从它原来的位置移出去
    {
        // 	if (ref_picture_listX_ptr[c_idx])
        {
            if ((ref_picture_listX_ptr[c_idx]->is_long_term) || (ref_picture_listX_ptr[c_idx]->pic_num != pic_num_lx))
            {
                ref_picture_listX_ptr[n_idx++] = ref_picture_listX_ptr[c_idx];
            }
        }
    }

    return;
}

LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_long_term_pic (H264DecObject *vo, int32 long_term_pic_num)
{
    int32 i;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    DEC_DECODED_PICTURE_BUFFER_T *p_Dpb = curr_slice_ptr->p_Dpb;

    for (i = 0; i < (p_Dpb->ltref_frames_in_buffer); i++)
    {
        if (p_Dpb->fs_ltref[i] == NULL)
        {
            vo->error_flag |= ER_REF_FRM_ID;
            return NULL;
        }

        if (p_Dpb->fs_ltref[i]->is_reference)
        {
            if (p_Dpb->fs_ltref[i]->frame == NULL)
            {
                vo->error_flag |= ER_REF_FRM_ID;
                return NULL;
            }

            if ((p_Dpb->fs_ltref[i]->frame->is_long_term) && (p_Dpb->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num))
            {
                return p_Dpb->fs_ltref[i]->frame;
            }
        }
    }

    return vo->g_no_reference_picture_ptr;//NULL;//weihu
}

LOCAL void H264Dec_reorder_long_term (H264DecObject *vo,
                                      DEC_STORABLE_PICTURE_T **ref_picture_listX_ptr,
                                      int32 num_ref_idx_lX_active_minus1,
                                      int32 long_term_pic_num,
                                      int32 *ref_idx_lx)
{
    int32 c_idx, n_idx;
    DEC_STORABLE_PICTURE_T *pic_lx_ptr;

    pic_lx_ptr = H264Dec_get_long_term_pic (vo, long_term_pic_num);

    for (c_idx = (num_ref_idx_lX_active_minus1+1); c_idx > *ref_idx_lx; c_idx--)
    {
        ref_picture_listX_ptr[c_idx] = ref_picture_listX_ptr[c_idx-1];
    }

    ref_picture_listX_ptr[(*ref_idx_lx)++] = pic_lx_ptr;

    n_idx = *ref_idx_lx;

    for (c_idx = (*ref_idx_lx); c_idx <= (num_ref_idx_lX_active_minus1+1); c_idx++)
    {
        // 	if (ref_picture_listX_ptr[c_idx])
        {
            if ((!ref_picture_listX_ptr[c_idx]->is_long_term) || (ref_picture_listX_ptr[c_idx]->long_term_pic_num != long_term_pic_num))
            {
                ref_picture_listX_ptr[n_idx++] = ref_picture_listX_ptr[c_idx];
            }
        }
    }

    return;
}

#if _MVC_
LOCAL DEC_STORABLE_PICTURE_T *get_inter_view_pic(H264DecObject *vo, int targetViewID, int currPOC, int listidx)
{
    unsigned i;
    unsigned int listinterview_size;
    DEC_FRAME_STORE_T **fs_listinterview;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;

    if (listidx == 0)
    {
        fs_listinterview = curr_slice_ptr->fs_listinterview0;
        listinterview_size = curr_slice_ptr->listinterviewidx0;
    }
    else
    {
        fs_listinterview = curr_slice_ptr->fs_listinterview1;
        listinterview_size = curr_slice_ptr->listinterviewidx1;
    }

    for(i=0; i<listinterview_size; i++)
    {
        if (fs_listinterview[i]->layer_id == GetVOIdx(vo,  targetViewID ))
        {
            return fs_listinterview[i]->frame;
        }
    }

    return NULL;
}

LOCAL void reorder_interview(H264DecObject *vo,
                             DEC_STORABLE_PICTURE_T **RefPicListX,
                             int32 num_ref_idx_lX_active_minus1,
                             int32 *refIdxLX,
                             int8 targetViewID,
                             int32 currPOC,
                             int listidx)
{
    int cIdx, nIdx;
    DEC_STORABLE_PICTURE_T *picLX;

    picLX = get_inter_view_pic(vo, targetViewID, currPOC, listidx);

    if (picLX)
    {
        for( cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx-- )
            RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];

        RefPicListX[ (*refIdxLX)++ ] = picLX;

        nIdx = *refIdxLX;

        for( cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++ )
        {
            if((GetViewIdx(vo, RefPicListX[cIdx]->view_id ) != targetViewID) || (RefPicListX[cIdx]->poc != currPOC))
                RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];
        }
    }
}

LOCAL void H264Dec_reorder_ref_pic_list_mvc (H264DecObject *vo,
        DEC_STORABLE_PICTURE_T **picture_list_ptr,
        int32 num_ref_idx_lX_active_minus1,
        int32 *remapping_of_pic_nums_idc,
        int32 *abs_diff_pic_num_minus1,
        int32 *long_term_pic_idx,
        int8 listNO,
        int32 **anchor_ref,
        int32 **non_anchor_ref,
        int8 view_id,
        int8 anchor_pic_flag,
        int32 currPOC)
{
    int32 i;
    int32 max_pic_num, curr_pic_num, pic_num_lx_no_wrap, pic_num_lx_pred, pic_num_lx;
    int8 picViewIdxLX, targetViewID;
    int32 ref_idx_lx = 0;
    int8 maxViewIdx =0;
    int8 curr_VOIdx = -1;
    int8 picViewIdxLXPred=-1;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    int *abs_diff_view_idx_minus1 = curr_slice_ptr->abs_diff_view_idx_minus1[listNO];

    if(curr_slice_ptr->svc_extension_flag==0)
    {
        curr_VOIdx = view_id;
        maxViewIdx = get_maxViewIdx(vo, view_id, anchor_pic_flag, 0);
        picViewIdxLXPred=-1;
    }

    max_pic_num = img_ptr->max_frame_num;
    curr_pic_num = img_ptr->frame_num;

    pic_num_lx_pred = curr_pic_num;

    for (i = 0; remapping_of_pic_nums_idc[i] != 3; i++)
    {
        if (remapping_of_pic_nums_idc[i]>5)
        {
            SCI_TRACE_LOW ("Invalid remapping_of_pic_nums_idc command");
            vo->error_flag |= ER_REF_FRM_ID;
            return;
        }

        if (remapping_of_pic_nums_idc[i]<2)
        {
            if (remapping_of_pic_nums_idc[i] == 0)
            {
                if ((pic_num_lx_pred-(abs_diff_pic_num_minus1[i]+1))<0)
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred - (abs_diff_pic_num_minus1[i]+1) + max_pic_num;
                } else
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred - (abs_diff_pic_num_minus1[i]+1);
                }
            } else //(remapping_of_pic_nums_idc[i]==1)
            {
                if ((pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1)) >= max_pic_num)
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1) - max_pic_num;
                } else
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1);
                }
            }

            pic_num_lx_pred = pic_num_lx_no_wrap;

            if (pic_num_lx_no_wrap > curr_pic_num)
            {
                pic_num_lx = pic_num_lx_no_wrap - max_pic_num;
            } else
            {
                pic_num_lx = pic_num_lx_no_wrap;
            }

            H264Dec_reorder_short_term (vo, picture_list_ptr, num_ref_idx_lX_active_minus1, pic_num_lx, &ref_idx_lx);
        } else if (remapping_of_pic_nums_idc[i] == 2) //(modification_of_pic_nums_idc[i] == 2)
        {
            H264Dec_reorder_long_term(vo, picture_list_ptr, num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &ref_idx_lx);
        } else
        {
            if(remapping_of_pic_nums_idc[i] == 4) //(modification_of_pic_nums_idc[i] == 4)
            {
                picViewIdxLX = picViewIdxLXPred - (abs_diff_view_idx_minus1[i] + 1);
                if( picViewIdxLX <0)
                    picViewIdxLX += maxViewIdx;
            } else //(modification_of_pic_nums_idc[i] == 5)
            {
                picViewIdxLX = picViewIdxLXPred + (abs_diff_view_idx_minus1[i] + 1);
                if( picViewIdxLX >= maxViewIdx)
                    picViewIdxLX -= maxViewIdx;
            }
            picViewIdxLXPred = picViewIdxLX;

            if (anchor_pic_flag)
                targetViewID = anchor_ref[curr_VOIdx][picViewIdxLX];
            else
                targetViewID = non_anchor_ref[curr_VOIdx][picViewIdxLX];

            if(listNO == 0)
                reorder_interview(vo, vo->g_list0, num_ref_idx_lX_active_minus1, &ref_idx_lx, targetViewID, currPOC, listNO);
            else
                reorder_interview(vo, vo->g_list1, num_ref_idx_lX_active_minus1, &ref_idx_lx, targetViewID, currPOC, listNO);
        }
    }

    return;
}
#endif

LOCAL void H264Dec_reorder_ref_pic_list (H264DecObject *vo,
        DEC_STORABLE_PICTURE_T **picture_list_ptr,
        int32 num_ref_idx_lX_active_minus1,
        int32 *remapping_of_pic_nums_idc,
        int32 *abs_diff_pic_num_minus1,
        int32 *long_term_pic_idx)
{
    int32 i;
    int32 max_pic_num, curr_pic_num, pic_num_lx_no_wrap, pic_num_lx_pred, pic_num_lx;
    int32 ref_idx_lx = 0;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;

    max_pic_num = img_ptr->max_frame_num;
    curr_pic_num = img_ptr->frame_num;

    pic_num_lx_pred = curr_pic_num;

    for (i = 0; remapping_of_pic_nums_idc[i] != 3; i++)
    {
        if (remapping_of_pic_nums_idc[i]>3)
        {
            PRINTF ("Invalid remapping_of_pic_nums_idc command");
            vo->error_flag |= ER_REF_FRM_ID;
            return;
        }

        if (remapping_of_pic_nums_idc[i]<2)
        {
            if (remapping_of_pic_nums_idc[i] == 0)
            {
                if ((pic_num_lx_pred-(abs_diff_pic_num_minus1[i]+1))<0)
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred - (abs_diff_pic_num_minus1[i]+1) + max_pic_num;
                } else
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred - (abs_diff_pic_num_minus1[i]+1);
                }
            } else //(remapping_of_pic_nums_idc[i]==1)
            {
                if ((pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1)) >= max_pic_num)
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1) - max_pic_num;
                } else
                {
                    pic_num_lx_no_wrap = pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1);
                }
            }

            pic_num_lx_pred = pic_num_lx_no_wrap;

            if (pic_num_lx_no_wrap > curr_pic_num)
            {
                pic_num_lx = pic_num_lx_no_wrap - max_pic_num;
            } else
            {
                pic_num_lx = pic_num_lx_no_wrap;
            }

            H264Dec_reorder_short_term (vo, picture_list_ptr, num_ref_idx_lX_active_minus1, pic_num_lx, &ref_idx_lx);
        } else //(remapping_of_pic_nums_idc[i]==2)
        {
            H264Dec_reorder_long_term(vo, picture_list_ptr, num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &ref_idx_lx);
        }
    }

    return;
}

//xwluo@20110607
//the reference idx in list1 are mapped into list0, for MCA hardware module with only one reference frame list.
LOCAL void H264Dec_map_list1(H264DecObject *vo)
{
    int32 list0_size = vo->g_list_size[0];
    int32 *map_ptr = vo->g_list1_map_list0;
    int32 i, j;

    for(i = 0; i < vo->g_list_size[1]; i++)
    {
        for(j = 0; j < list0_size; j++)
        {
            if (vo->g_list1[i]->imgYAddr == vo->g_list0[j]->imgYAddr)
            {
                break;
            }
        }

        //not found
        if (j == list0_size)
        {
            vo->g_list0[list0_size] = vo->g_list1[i];
            list0_size++;
        }

        map_ptr[i] = j;
    }

    //set the remain with a invalid num
    for (i = vo->g_list_size[1]; i < MAX_REF_FRAME_NUMBER; i++)
    {
        map_ptr[i] = 0x3f;//weihu0730
    }

    vo->g_list_size[0] = list0_size;

    if((vo->g_list_size[0] == 0) && (vo->g_list_size[1] == 0))//for error // 不考虑对B/P slice 全Intra MB 情况支持
    {
        vo->error_flag |= ER_REF_FRM_ID;
        return;
    }

    for(i = 0; i < vo->g_list_size[0]; i++)
    {
        vo->g_list0_map_addr[i] = vo->g_list0[i]->DPB_addr_index;
    }

    for (i = vo->g_list_size[0]; i < (2*MAX_REF_FRAME_NUMBER); i++)
    {
        vo->g_list0_map_addr[i] = 0x3f;
    }

    for(i = 0; i < vo->g_list_size[1]; i++)
    {
        vo->g_list1_map_addr[i] = vo->g_list1[i]->DPB_addr_index;
    }

    for (i = vo->g_list_size[1]; i < MAX_REF_FRAME_NUMBER; i++)
    {
        vo->g_list1_map_addr[i] = 0x3f;
    }
}

PUBLIC void H264Dec_reorder_list (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T	*curr_slice_ptr = vo->g_curr_slice_ptr;
    int32 currSliceType = img_ptr->type;
    int i;

    if (currSliceType != I_SLICE)
    {
        if (curr_slice_ptr->ref_pic_list_reordering_flag_l0)
        {
            H264Dec_reorder_ref_pic_list (vo, vo->g_list0, img_ptr->num_ref_idx_l0_active-1,
                                          curr_slice_ptr->remapping_of_pic_nums_idc_l0,
                                          curr_slice_ptr->abs_diff_pic_num_minus1_l0,
                                          curr_slice_ptr->long_term_pic_idx_l0);
            vo->g_list_size[0] = img_ptr->num_ref_idx_l0_active;
        }

        if (currSliceType == B_SLICE)
        {
            if (curr_slice_ptr->ref_pic_list_reordering_flag_l1)
            {
                H264Dec_reorder_ref_pic_list (vo, vo->g_list1, img_ptr->num_ref_idx_l1_active-1,
                                              curr_slice_ptr->remapping_of_pic_nums_idc_l1,
                                              curr_slice_ptr->abs_diff_pic_num_minus1_l1,
                                              curr_slice_ptr->long_term_pic_idx_l1);
                vo->g_list_size[1] = img_ptr->num_ref_idx_l1_active;
            }
        }

        H264Dec_map_list1(vo);
    }

    return;
}

#if _MVC_
PUBLIC void H264Dec_reorder_list_mvc (H264DecObject *vo)
{
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    subset_seq_parameter_set_rbsp_t *active_subset_sps = vo->g_active_subset_sps;
    int32 currPOC = img_ptr->framepoc;
    int32 currSliceType = img_ptr->type;
    int i;
    int8 listNO;

    if (currSliceType != I_SLICE)
    {
        if (curr_slice_ptr->ref_pic_list_reordering_flag_l0)
        {
            listNO = 0;
            H264Dec_reorder_ref_pic_list_mvc (vo, vo->g_list0, img_ptr->num_ref_idx_l0_active-1,
                                              curr_slice_ptr->remapping_of_pic_nums_idc_l0,
                                              curr_slice_ptr->abs_diff_pic_num_minus1_l0,
                                              curr_slice_ptr->long_term_pic_idx_l0,
                                              listNO,
                                              active_subset_sps->anchor_ref_l0,
                                              active_subset_sps->non_anchor_ref_l0,
                                              curr_slice_ptr->view_id,
                                              curr_slice_ptr->anchor_pic_flag,
                                              currPOC);
            vo->g_list_size[0] = img_ptr->num_ref_idx_l0_active;
        }

        if (currSliceType == B_SLICE)
        {
            if (curr_slice_ptr->ref_pic_list_reordering_flag_l1)
            {
                listNO = 1;
                H264Dec_reorder_ref_pic_list_mvc (vo, vo->g_list1, img_ptr->num_ref_idx_l1_active-1,
                                                  curr_slice_ptr->remapping_of_pic_nums_idc_l1,
                                                  curr_slice_ptr->abs_diff_pic_num_minus1_l1,
                                                  curr_slice_ptr->long_term_pic_idx_l1,
                                                  listNO,
                                                  active_subset_sps->anchor_ref_l1,
                                                  active_subset_sps->non_anchor_ref_l1,
                                                  curr_slice_ptr->view_id,
                                                  curr_slice_ptr->anchor_pic_flag,
                                                  currPOC);
                vo->g_list_size[1] = img_ptr->num_ref_idx_l1_active;
            }
        }

        H264Dec_map_list1(vo);
    }

    return;
}
#endif

#define SYS_QSORT	0//1 //for or 

#if SYS_QSORT
/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by picture number for qsort in descending order
 *
 ************************************************************************
 */
static int compare_pic_by_pic_num_desc( const void *arg1, const void *arg2 )
{
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->pic_num < (*(DEC_STORABLE_PICTURE_T**)arg2)->pic_num)
        return 1;
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->pic_num > (*(DEC_STORABLE_PICTURE_T**)arg2)->pic_num)
        return -1;
    else
        return 0;
}

/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by picture number for qsort in descending order
 *
 ************************************************************************
 */
static int compare_pic_by_lt_pic_num_asc( const void *arg1, const void *arg2 )
{
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->long_term_pic_num < (*(DEC_STORABLE_PICTURE_T**)arg2)->long_term_pic_num)
        return -1;
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->long_term_pic_num > (*(DEC_STORABLE_PICTURE_T**)arg2)->long_term_pic_num)
        return 1;
    else
        return 0;
}

/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by poc for qsort in ascending order
 *
 ************************************************************************
 */
static int compare_pic_by_poc_asc( const void *arg1, const void *arg2 )
{
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->poc < (*(DEC_STORABLE_PICTURE_T**)arg2)->poc)
        return -1;
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->poc > (*(DEC_STORABLE_PICTURE_T**)arg2)->poc)
        return 1;
    else
        return 0;
}

/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by poc for qsort in descending order
 *
 ************************************************************************
 */
static int compare_pic_by_poc_desc( const void *arg1, const void *arg2 )
{
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->poc < (*(DEC_STORABLE_PICTURE_T**)arg2)->poc)
        return 1;
    if ( (*(DEC_STORABLE_PICTURE_T**)arg1)->poc > (*(DEC_STORABLE_PICTURE_T**)arg2)->poc)
        return -1;
    else
        return 0;
}
#else

/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by picture number for qsort in descending order
 *
 ************************************************************************
 */
static int compare_pic_by_pic_num_desc(DEC_STORABLE_PICTURE_T *list_ptr[], int len)
{
    int16 i, j;

    for (i = 0; i < len; i++)
    {
        for(j = i+1; j < len; j++)
        {
            int32 t1, t2;
            t1 = list_ptr[i]->pic_num;
            t2 = list_ptr[j]->pic_num;
            if (t1 < t2)
            {
                DEC_STORABLE_PICTURE_T *tmp_s;
                tmp_s = list_ptr[i];
                list_ptr[i] = list_ptr[j];
                list_ptr[j] = tmp_s;
            }
        }
    }
    return 0;

}

/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by picture number for qsort in descending order
 *
 ************************************************************************
 */
static int compare_pic_by_lt_pic_num_asc(DEC_STORABLE_PICTURE_T *list_ptr[], int len)
{
    int16 i, j;

    for (i = 0; i < len; i++)
    {
        for(j = i+1; j < len; j++)
        {
            int32 t1, t2;
            t1 = list_ptr[i]->long_term_pic_num;
            t2 = list_ptr[j]->long_term_pic_num;
            if (t1 > t2)
            {
                DEC_STORABLE_PICTURE_T *tmp_s;
                tmp_s = list_ptr[i];
                list_ptr[i] = list_ptr[j];
                list_ptr[j] = tmp_s;
            }
        }
    }
    return 0;

}

/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by poc for qsort in ascending order
 *
 ************************************************************************
 */
static int compare_pic_by_poc_asc(DEC_STORABLE_PICTURE_T *list_ptr[], int len)
{
    int16 i, j;

    for (i = 0; i < len; i++)
    {
        for(j = i+1; j < len; j++)
        {
            int32 t1, t2;
            t1 = list_ptr[i]->poc;
            t2 = list_ptr[j]->poc;
            if (t1 > t2)
            {
                DEC_STORABLE_PICTURE_T *tmp_s;
                tmp_s = list_ptr[i];
                list_ptr[i] = list_ptr[j];
                list_ptr[j] = tmp_s;
            }
        }
    }
    return 0;

}


/*!
 ************************************************************************
 * \brief
 *    compares two stored pictures by poc for qsort in descending order
 *
 ************************************************************************
 */
static int compare_pic_by_poc_desc(DEC_STORABLE_PICTURE_T *list_ptr[], int len)
{
    int16 i, j;

    for (i = 0; i < len; i++)
    {
        for(j = i+1; j < len; j++)
        {
            int32 t1, t2;
            t1 = list_ptr[i]->poc;
            t2 = list_ptr[j]->poc;
            if (t1 < t2)
            {
                DEC_STORABLE_PICTURE_T *tmp_s;
                tmp_s = list_ptr[i];
                list_ptr[i] = list_ptr[j];
                list_ptr[j] = tmp_s;
            }
        }
    }
    return 0;

}
#endif

#if _MVC_
static int is_view_id_in_ref_view_list(int view_id, int *ref_view_id, int num_ref_views)
{
    int i;
    for(i=0; i<num_ref_views; i++)
    {
        if(view_id == ref_view_id[i])
            break;
    }

    return (num_ref_views && (i<num_ref_views));
}
void append_interview_list(H264DecObject *vo,
                           DEC_DECODED_PICTURE_BUFFER_T *p_Dpb,
                           int8 list_idx,
                           DEC_STORABLE_PICTURE_T **list,
                           int32 *listXsize,
                           int32 currPOC,
                           int curr_view_id,
                           int8 anchor_pic_flag)
{
    subset_seq_parameter_set_rbsp_t *active_subset_sps = vo->g_active_subset_sps;
    int8 iVOIdx = curr_view_id;
    int8 pic_avail;
    int32 poc = 0;
    int8 fld_idx;
    int32 num_ref_views, *ref_view_id;
    DEC_FRAME_STORE_T *fs = p_Dpb->fs_ilref[0];

    if(iVOIdx <0)
    {
        PRINTF("Error: iVOIdx: %d is not less than 0\n", iVOIdx);
    }

    if(anchor_pic_flag)
    {
        num_ref_views = list_idx ? active_subset_sps->num_anchor_refs_l1[iVOIdx] : active_subset_sps->num_anchor_refs_l0[iVOIdx];
        ref_view_id   = list_idx ? active_subset_sps->anchor_ref_l1[iVOIdx] : active_subset_sps->anchor_ref_l0[iVOIdx];
    } else
    {
        num_ref_views = list_idx ? active_subset_sps->num_non_anchor_refs_l1[iVOIdx] : active_subset_sps->num_non_anchor_refs_l0[iVOIdx];
        ref_view_id = list_idx ? active_subset_sps->non_anchor_ref_l1[iVOIdx] : active_subset_sps->non_anchor_ref_l0[iVOIdx];
    }

    //  if(num_ref_views <= 0)
    //    PRINTF("Error: iNumOfRefViews: %d is not larger than 0\n", num_ref_views);

    /*if(currPicStructure == BOTTOM_FIELD)
    	fld_idx = 1;
    else*/
    fld_idx = 0;

    //james
    pic_avail = 1;
    poc = fs->frame->frame_poc;

    /*if(currPicStructure==FRAME)
    {
    	pic_avail = (fs->is_used == 3);
    	if (pic_avail)
    		poc = fs->frame->poc;
    }
    else if(currPicStructure==TOP_FIELD)
    {
    	pic_avail = fs->is_used & 1;
    	if (pic_avail)
    		poc = fs->top_field->poc;
    }
    else if(currPicStructure==BOTTOM_FIELD)
    {
    	pic_avail = fs->is_used & 2;
    	if (pic_avail)
    		poc = fs->bottom_field->poc;
    }
    else
    	pic_avail =0;*/

    if(pic_avail /*&& fs->inter_view_flag[fld_idx]*/)
    {
        if(poc == currPOC)
        {
            if(is_view_id_in_ref_view_list(fs->view_id, ref_view_id, num_ref_views))
            {
                //add one inter-view reference;
                list[*listXsize] = (DEC_STORABLE_PICTURE_T *)fs;
                //next;
                (*listXsize)++;
            }
        }
    }
}
#endif

/*!
 ************************************************************************
 * \brief
 *    Initialize listX[0] and list[1] depending on current picture type
 *
 ************************************************************************
 */
PUBLIC MMDecRet H264Dec_init_list (H264DecObject *vo, int32 curr_slice_type)
{
    int32 i;
    int32 list0idx = 0;
    int32 list0idx_1 = 0;
    int32 listltidx = 0;
    DEC_IMAGE_PARAMS_T *img_ptr = vo->g_image_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->g_curr_slice_ptr;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = curr_slice_ptr->p_Dpb;
    DEC_STORABLE_PICTURE_T **list = vo->g_list0;
    int32 max_frame_num = (1<<(vo->g_active_sps_ptr->log2_max_frame_num_minus4+4));

#if _MVC_
    curr_slice_ptr->listinterviewidx0 = 0;
    curr_slice_ptr->listinterviewidx1 = 0;
#endif

    for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
    {
        DEC_FRAME_STORE_T *fs_ref = dpb_ptr->fs_ref[i];

        if ((fs_ref->frame->used_for_reference)&&(!fs_ref->frame->is_long_term))
        {
            if (fs_ref->frame_num > img_ptr->frame_num)
            {
                fs_ref->frame_num_wrap = fs_ref->frame_num - max_frame_num;
            } else
            {
                fs_ref->frame_num_wrap = fs_ref->frame_num;
            }
        }

        fs_ref->frame->pic_num = fs_ref->frame_num_wrap;
    }

    //update long_term_pic_num
    for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
    {
        DEC_STORABLE_PICTURE_T *frame = dpb_ptr->fs_ltref[i]->frame;

        if (frame->is_long_term)
        {
            frame->long_term_pic_num = frame->long_term_frame_idx;
        }
    }

    if (curr_slice_type == I_SLICE)
    {
        vo->g_list_size[0] = 0;
        vo->g_list_size[1] = 0;
        return MMDEC_OK;
    }

    if (curr_slice_type == P_SLICE)
    {
        /*p slice, put short and long term reference frame into list
        short term is in descending mode, long term is in ascending mode*/
        for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
        {
            if ((dpb_ptr->fs_ref[i]->frame->used_for_reference) && (!dpb_ptr->fs_ref[i]->frame->is_long_term))
            {
                list[list0idx] = dpb_ptr->fs_ref[i]->frame;
                list0idx++;
            }
        }
        // order list 0 by PicNum
#if SYS_QSORT
        qsort((void*)list, list0idx, sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_pic_num_desc);
#else
        compare_pic_by_pic_num_desc(list, list0idx);
#endif
        vo->g_list_size[0] = list0idx;

        //long term handling
        for(i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
        {
            if (dpb_ptr->fs_ltref[i]->frame->is_long_term)
            {
                dpb_ptr->fs_ltref[i]->frame->long_term_pic_num = dpb_ptr->fs_ltref[i]->frame->long_term_frame_idx;
                list[list0idx] = dpb_ptr->fs_ltref[i]->frame;
                list0idx++;
            }
        }
#if SYS_QSORT
        qsort((void*)&list[vo->g_list_size[0]], list0idx-vo->g_list_size[0], sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_lt_pic_num_asc);
#else
        compare_pic_by_lt_pic_num_asc(&list[vo->g_list_size[0]], list0idx-vo->g_list_size[0]);
#endif
#if _MVC_
        if (curr_slice_ptr->svc_extension_flag == 0 && curr_slice_ptr->view_id != 0)
        {
            int curr_view_id = curr_slice_ptr->layer_id;
            curr_slice_ptr->fs_listinterview0 = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*dpb_ptr->size, 4, INTER_MEM);//weihu//calloc(dpb_ptr->size, sizeof (DEC_FRAME_STORE_T*));
            CHECK_MALLOC(curr_slice_ptr->fs_listinterview0, "curr_slice_ptr->fs_listinterview0");

            list0idx = vo->g_list_size[0];

            append_interview_list(vo, vo->g_dpb_layer[1],
                                  0,
                                  (DEC_STORABLE_PICTURE_T **)(curr_slice_ptr->fs_listinterview0),
                                  &(curr_slice_ptr->listinterviewidx0),
                                  img_ptr->framepoc,
                                  curr_view_id,
                                  curr_slice_ptr->anchor_pic_flag);
            for (i=0; i<(unsigned int)(curr_slice_ptr->listinterviewidx0); i++)
            {
                vo->g_list0[list0idx++] = curr_slice_ptr->fs_listinterview0[i]->frame;
            }
            //currSlice->listXsize[0] = (char) list0idx;
        }
#endif
        vo->g_list_size[0] = list0idx;
        vo->g_list_size[1] = 0;

        for (i = vo->g_list_size[0]; i < MAX_REF_FRAME_NUMBER+1; i++)
        {
            list[i] = vo->g_no_reference_picture_ptr;
        }
    } else	//B-slice
    {
        for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
        {
            DEC_FRAME_STORE_T *fs_ref = dpb_ptr->fs_ref[i];

            if ((fs_ref->frame->used_for_reference)&&(!fs_ref->frame->is_long_term))
            {
                if (img_ptr->framepoc > fs_ref->frame->poc)
                {
                    list[list0idx] = dpb_ptr->fs_ref[i]->frame;
                    list0idx++;
                }
            }
        }
#if SYS_QSORT
        qsort((void*)list, list0idx, sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_poc_desc);
#else
        compare_pic_by_poc_desc(list, list0idx);
#endif
        list0idx_1 = list0idx;

        for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
        {
            if ((dpb_ptr->fs_ref[i]->frame->used_for_reference) && (!dpb_ptr->fs_ref[i]->frame->is_long_term))
            {
                if (img_ptr->framepoc < dpb_ptr->fs_ref[i]->frame->poc)
                {
                    list[list0idx] = dpb_ptr->fs_ref[i]->frame;
                    list0idx++;
                }
            }
        }
#if SYS_QSORT
        qsort((void*)&list[list0idx_1], list0idx-list0idx_1, sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_poc_asc);
#else
        compare_pic_by_poc_asc(&list[list0idx_1], list0idx-list0idx_1);
#endif

        for (i = 0; i < list0idx_1; i++)
        {
            vo->g_list1[list0idx-list0idx_1+i] = vo->g_list0[i];
        }
        for (i = list0idx_1; i < list0idx; i++)
        {
            vo->g_list1[i-list0idx_1] = vo->g_list0[i];
        }
        vo->g_list_size[0] = vo->g_list_size[1] = list0idx;

        //long term handling
        for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
        {
            if (dpb_ptr->fs_ltref[i]->frame->is_long_term)
            {
                dpb_ptr->fs_ltref[i]->frame->long_term_pic_num = dpb_ptr->fs_ltref[i]->frame->long_term_frame_idx;

                vo->g_list0[list0idx] = dpb_ptr->fs_ltref[i]->frame;
                vo->g_list1[list0idx++] = dpb_ptr->fs_ltref[i]->frame;
            }
        }
#if SYS_QSORT
        qsort((void *)&(vo->g_list0)[vo->g_list_size[0]], list0idx-vo->g_list_size[0], sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_lt_pic_num_asc);
#else
        compare_pic_by_lt_pic_num_asc(&(vo->g_list0)[vo->g_list_size[0]], list0idx-vo->g_list_size[0]);
#endif

#if SYS_QSORT
        qsort((void *)&(vo->g_list1)[vo->g_list_size[0]], list0idx-vo->g_list_size[0], sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_lt_pic_num_asc);
#else
        compare_pic_by_lt_pic_num_asc(&(vo->g_list1)[vo->g_list_size[0]], list0idx-vo->g_list_size[0]);
#endif
        vo->g_list_size[0] = vo->g_list_size[1] = list0idx;

#if _MVC_
        if (curr_slice_ptr->svc_extension_flag == 0)
        {
            int curr_view_id = curr_slice_ptr->view_id;
            // B-Slice
            curr_slice_ptr->fs_listinterview0 = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*dpb_ptr->size, 4, INTER_MEM);
            CHECK_MALLOC(curr_slice_ptr->fs_listinterview0, "curr_slice_ptr->fs_listinterview0");

            curr_slice_ptr->fs_listinterview1 = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (vo, sizeof(DEC_FRAME_STORE_T*)*dpb_ptr->size, 4, INTER_MEM);
            CHECK_MALLOC(curr_slice_ptr->fs_listinterview1, "curr_slice_ptr->fs_listinterview1");

            list0idx = vo->g_list_size[0];

            append_interview_list(vo, vo->g_dpb_layer[1],
                                  0,
                                  (DEC_STORABLE_PICTURE_T **)(curr_slice_ptr->fs_listinterview0),
                                  &(curr_slice_ptr->listinterviewidx0),
                                  img_ptr->framepoc,
                                  curr_view_id,
                                  curr_slice_ptr->anchor_pic_flag);

            append_interview_list(vo, vo->g_dpb_layer[1], 0, (DEC_STORABLE_PICTURE_T **)(curr_slice_ptr->fs_listinterview0),
                                  &(curr_slice_ptr->listinterviewidx0), img_ptr->framepoc, curr_view_id, curr_slice_ptr->anchor_pic_flag);
            append_interview_list(vo, vo->g_dpb_layer[1], 1, (DEC_STORABLE_PICTURE_T **)(curr_slice_ptr->fs_listinterview1),
                                  &(curr_slice_ptr->listinterviewidx1), img_ptr->framepoc, curr_view_id, curr_slice_ptr->anchor_pic_flag);
            for (i=0; i<(unsigned int)(curr_slice_ptr->listinterviewidx0); i++)
            {
                vo->g_list0[list0idx++] = curr_slice_ptr->fs_listinterview0[i]->frame;
            }
            vo->g_list_size[0] = (char) list0idx;
            list0idx = vo->g_list_size[1];
            for (i=0; i<(unsigned int)(curr_slice_ptr->listinterviewidx1); i++)
            {
                vo->g_list1[list0idx++] = curr_slice_ptr->fs_listinterview1[i]->frame;
            }
            vo->g_list_size[1] = (char) list0idx;
        }
#endif
    }

    if ((vo->g_list_size[0] == vo->g_list_size[1]) && (vo->g_list_size[0] > 1))
    {
        // check if lists are identical, if yes swap first two elements of listX[1]	//???
        int32 diff = 0;
        for (i = 0; i < vo->g_list_size[0]; i++)
        {
            if (vo->g_list0[i] != vo->g_list1[i])
            {
                diff = 1;
            }
        }
        if (!diff)
        {
            DEC_STORABLE_PICTURE_T *tmp_s;
            tmp_s = vo->g_list1[0];
            vo->g_list1[0] = vo->g_list1[1];
            vo->g_list1[1] = tmp_s;
        }
    }
    //set max size
    vo->g_list_size[0] = mmin(vo->g_list_size[0], img_ptr->num_ref_idx_l0_active);
    vo->g_list_size[1] = mmin(vo->g_list_size[1], img_ptr->num_ref_idx_l1_active);

    //set the unsed list entries to NULL
    for (i = vo->g_list_size[0]; i < MAX_REF_FRAME_NUMBER+1; i++)
    {
        vo->g_list0[i] = vo->g_no_reference_picture_ptr;
    }

    for (i = vo->g_list_size[1]; i < MAX_REF_FRAME_NUMBER+1; i++)
    {
        vo->g_list1[i] = vo->g_no_reference_picture_ptr;
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
