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
#include "sc8800g_video_header.h"
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
PUBLIC int32 H264Dec_is_new_picture (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 reslt = 0;
	DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = g_old_slice_ptr;

	reslt |= (old_slice_ptr->pps_id != img_ptr->curr_slice_ptr->pic_parameter_set_id) ? 1 : 0;
	reslt |= (old_slice_ptr->frame_num != img_ptr->frame_num) ? 1 : 0;
	reslt |= ((old_slice_ptr->nal_ref_idc != img_ptr->nal_reference_idc) && ((old_slice_ptr->nal_ref_idc == 0) || (img_ptr->nal_reference_idc == 0))) ? 1 : 0;
	reslt |= (old_slice_ptr->idr_flag != img_ptr->idr_flag) ? 1 : 0;

	if (img_ptr->idr_flag && old_slice_ptr->idr_flag)
	{
		reslt |= (old_slice_ptr->idr_pic_id != img_ptr->idr_pic_id) ? 1 : 0;
	}

	if (g_active_sps_ptr->pic_order_cnt_type == 0)
	{
		reslt |= (old_slice_ptr->pic_order_cnt_lsb != img_ptr->pic_order_cnt_lsb) ? 1 : 0;
		reslt |= (old_slice_ptr->delta_pic_order_cnt_bottom != img_ptr->delta_pic_order_cnt_bottom) ? 1: 0;
	}else if (g_active_sps_ptr->pic_order_cnt_type == 1)
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

LOCAL void h264Dec_remove_frame_from_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int32 pos)
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

LOCAL int32 h264Dec_remove_unused_frame_from_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
	int32 i;
	int32 has_free_bfr = FALSE;

	for (i = 0; i < dpb_ptr->used_size; i++)
	{
		if ((!dpb_ptr->fs[i]->is_reference) && (!dpb_ptr->fs[i]->disp_status))
		{
			h264Dec_remove_frame_from_dpb(dpb_ptr, i);

			has_free_bfr = TRUE;
			break;
		}
	}

	return has_free_bfr;
}

LOCAL DEC_FRAME_STORE_T *H264Dec_get_one_free_pic_buffer (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
	while(dpb_ptr->used_size == (MAX_REF_FRAME_NUMBER+1))
	{
		if (!h264Dec_remove_unused_frame_from_dpb(dpb_ptr))
		{
			//wait for display free buffer
			g_image_ptr->error_flag = TRUE;
            g_image_ptr->return_pos |= (1<<14);
            H264Dec_get_HW_status(g_image_ptr);
			return NULL;
		}
	}

	return dpb_ptr->fs[MAX_REF_FRAME_NUMBER];
}

LOCAL void H264Dec_fill_frame_num_gap (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 curr_frame_num;
	int32 unused_short_term_frm_num;

	PRINTF("a gap in frame number is found, try to fill it.\n");
	SCI_TRACE_LOW("a gap in frame number is found, try to fill it.\n");
	unused_short_term_frm_num = H264Dec_Divide ((img_ptr->pre_frame_num+1), img_ptr->max_frame_num);
	curr_frame_num = img_ptr->frame_num;

	while (curr_frame_num != unused_short_term_frm_num)
	{
		DEC_STORABLE_PICTURE_T *picture_ptr;
		DEC_FRAME_STORE_T *frame_store_ptr = H264Dec_get_one_free_pic_buffer (g_dpb_ptr);

		if (img_ptr->error_flag)
		{
	        img_ptr->return_pos |= (1<<15);
            H264Dec_get_HW_status(img_ptr);
			return;
		}

		frame_store_ptr->disp_status = 0;

		picture_ptr = frame_store_ptr->frame;
		picture_ptr->pic_num = unused_short_term_frm_num;
		picture_ptr->frame_num = unused_short_term_frm_num;
		picture_ptr->non_existing = 1;
		picture_ptr->used_for_reference = 1;
		picture_ptr->is_long_term = 0;
		picture_ptr->idr_flag = 0;
		picture_ptr->adaptive_ref_pic_buffering_flag = 0;
#ifdef _VSP_LINUX_
		picture_ptr->imgY = NULL;
		picture_ptr->imgU = NULL;
		picture_ptr->imgV = NULL;

		picture_ptr->imgYAddr = g_rec_buf.imgYAddr;;
		picture_ptr->imgUAddr = NULL;
		picture_ptr->imgVAddr = NULL;

		picture_ptr->pBufferHeader= NULL;
#endif		

		img_ptr->frame_num = unused_short_term_frm_num;

		H264Dec_store_picture_in_dpb (img_ptr, picture_ptr);

		img_ptr->pre_frame_num = unused_short_term_frm_num;
		unused_short_term_frm_num = H264Dec_Divide (unused_short_term_frm_num+1, img_ptr->max_frame_num);
	}

	img_ptr->frame_num = curr_frame_num;
}

PUBLIC void H264Dec_init_picture (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
	DEC_STORABLE_PICTURE_T *dec_picture_ptr = NULL;
	DEC_FRAME_STORE_T *fs = NULL;
 	int32 i;
	
	//init the slice nr;
	for(i = 0; i < img_ptr->frame_size_in_mbs; i++)
	{
		(img_ptr->mb_info+i)->slice_nr = -1;
	}

	if ((img_ptr->frame_num != img_ptr->pre_frame_num) && (img_ptr->frame_num != H264Dec_Divide((img_ptr->pre_frame_num+1), img_ptr->max_frame_num)))
	{
		if (g_active_sps_ptr->gaps_in_frame_num_value_allowed_flag == 0)
		{
			/*advanced error concealment would be called here to combat unitentional loss of pictures*/
			SCI_TRACE_LOW("an unintentional loss of picture occures!\n");
			img_ptr->error_flag = 1;
			return;
		}
		H264Dec_fill_frame_num_gap(img_ptr);
	}

	fs = H264Dec_get_one_free_pic_buffer(dpb_ptr);

	if (img_ptr->error_flag)
	{
        img_ptr->return_pos |= (1<<16);
        H264Dec_get_HW_status(img_ptr);    
		return;
	}

	g_dec_picture_ptr = fs->frame;

	img_ptr->pre_frame_num = img_ptr->frame_num;
	img_ptr->num_dec_mb = 0;
	img_ptr->constrained_intra_pred_flag = g_active_pps_ptr->constrained_intra_pred_flag;
	img_ptr->mb_x = 0;
	img_ptr->mb_y = 0;
	img_ptr->chroma_qp_offset = g_active_pps_ptr->chroma_qp_index_offset;
	img_ptr->slice_nr = 0;
	img_ptr->curr_mb_nr = 0;
	img_ptr->error_flag = FALSE;//for error concealment
    img_ptr->return_pos = 0;
    img_ptr->return_pos2 = 0;
    
	dec_picture_ptr = g_dec_picture_ptr;
	dec_picture_ptr->dec_ref_pic_marking_buffer = img_ptr->dec_ref_pic_marking_buffer;
	dec_picture_ptr->slice_type = img_ptr->type;
	dec_picture_ptr->used_for_reference = (img_ptr->nal_reference_idc != 0);
	dec_picture_ptr->frame_num = img_ptr->frame_num;
// 	dec_picture_ptr->pic_num = img_ptr->frame_num;
	dec_picture_ptr->idr_flag = img_ptr->idr_flag;
	dec_picture_ptr->adaptive_ref_pic_buffering_flag = img_ptr->adaptive_ref_pic_buffering_flag;
	dec_picture_ptr->no_output_of_prior_pics_flag = img_ptr->no_output_of_prior_pics_flag;
	dec_picture_ptr->is_long_term = 0;
	dec_picture_ptr->non_existing = 0;
#ifdef _VSP_LINUX_
	dec_picture_ptr->imgY = g_rec_buf.imgY;
	dec_picture_ptr->imgU = g_rec_buf.imgU;
	dec_picture_ptr->imgV = g_rec_buf.imgV;

	dec_picture_ptr->imgYAddr= g_rec_buf.imgYAddr;
	dec_picture_ptr->imgUAddr = g_rec_buf.imgUAddr;
	dec_picture_ptr->imgVAddr = g_rec_buf.imgVAddr;

	dec_picture_ptr->pBufferHeader= g_rec_buf.pBufferHeader;
#endif		

	return;
}

PUBLIC void H264Dec_exit_picture (DEC_IMAGE_PARAMS_T *img_ptr)
{
	DEC_STORABLE_PICTURE_T *dec_picture_ptr = g_dec_picture_ptr;

	//reference frame store update
	H264Dec_store_picture_in_dpb(img_ptr, dec_picture_ptr);
	if (img_ptr->last_has_mmco_5)
	{
		img_ptr->pre_frame_num = 0;
	}

#if _CMODEL_
	PRINTF ("\ndecode frame: %3d\t%s\t", g_nFrame_dec_h264, (g_curr_slice_ptr->picture_type == I_SLICE)?"I_SLICE":"P_SLICE");
#endif

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