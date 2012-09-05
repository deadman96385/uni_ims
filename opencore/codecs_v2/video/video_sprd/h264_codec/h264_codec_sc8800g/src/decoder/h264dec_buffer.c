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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void H264Dec_init_img_buffer (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 mb_num_x = img_ptr->frame_width_in_mbs;
	int32 total_mb_num = mb_num_x * img_ptr->frame_height_in_mbs;
	DEC_MB_INFO_T *mb_info;
	int32 i;

	img_ptr->ipred_top_line_buffer = (uint8 *)H264Dec_ExtraMemAlloc_64WordAlign (mb_num_x*32);
#ifndef _VSP_LINUX_	
	img_ptr->mb_info = (DEC_MB_INFO_T *)H264Dec_ExtraMemAlloc ((uint32)total_mb_num*sizeof(DEC_MB_INFO_T));
#else
	H264Dec_ExtraMemCacheAlloc(256);
	img_ptr->mb_info = (DEC_MB_INFO_T *)H264Dec_ExtraMemCacheAlloc ((uint32)total_mb_num*sizeof(DEC_MB_INFO_T));
#endif
	//for fmo
	mb_info = img_ptr->mb_info;
	for(i = 0; i < total_mb_num; i++)
	{
		mb_info->slice_nr = -1;
		mb_info++;
	}

	return;
}

PUBLIC void	H264Dec_init_dpb (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 i;
	int32 frm_size = img_ptr->width * img_ptr->height;
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;

	dpb_ptr->used_size = 0;
	dpb_ptr->ref_frames_in_buffer = 0;
	dpb_ptr->ltref_frames_in_buffer = 0;

	if (dpb_ptr->fs == PNULL)
	{
#ifndef _VSP_LINUX_	
		dpb_ptr->fs = (DEC_FRAME_STORE_T **)H264Dec_ExtraMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));
#else
		dpb_ptr->fs = (DEC_FRAME_STORE_T **)H264Dec_ExtraMemCacheAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));
#endif
        if (img_ptr->error_flag)    return;
	}
	
	if (dpb_ptr->fs_ref == PNULL)
	{
#ifndef _VSP_LINUX_	
		dpb_ptr->fs_ref = (DEC_FRAME_STORE_T **)H264Dec_ExtraMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));
#else
		dpb_ptr->fs_ref = (DEC_FRAME_STORE_T **)H264Dec_ExtraMemCacheAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));
#endif
        if (img_ptr->error_flag)    return;
	}
	
	if (dpb_ptr->fs_ltref == PNULL)
	{
#ifndef _VSP_LINUX_	
		dpb_ptr->fs_ltref = (DEC_FRAME_STORE_T **)H264Dec_ExtraMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));
#else
		dpb_ptr->fs_ltref = (DEC_FRAME_STORE_T **)H264Dec_ExtraMemCacheAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));
#endif
        if (img_ptr->error_flag)    return;
	}

	for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		//each storable_picture buffer is bonding to a frame store
		if (dpb_ptr->fs[i] == PNULL)
		{
#ifndef _VSP_LINUX_		
			dpb_ptr->fs[i] = (DEC_FRAME_STORE_T *)H264Dec_ExtraMemAlloc (sizeof(DEC_FRAME_STORE_T));
#else
			dpb_ptr->fs[i] = (DEC_FRAME_STORE_T *)H264Dec_ExtraMemCacheAlloc (sizeof(DEC_FRAME_STORE_T));
#endif
            		if (img_ptr->error_flag)    return;
#ifndef _VSP_LINUX_					
			dpb_ptr->fs[i]->frame = (DEC_STORABLE_PICTURE_T *)H264Dec_ExtraMemAlloc(sizeof(DEC_STORABLE_PICTURE_T));
#else
			dpb_ptr->fs[i]->frame = (DEC_STORABLE_PICTURE_T *)H264Dec_ExtraMemCacheAlloc(sizeof(DEC_STORABLE_PICTURE_T));
#endif
			if (img_ptr->error_flag)    return;
#ifdef _VSP_LINUX_			
			dpb_ptr->fs[i]->frame->imgY = PNULL;
			dpb_ptr->fs[i]->frame->imgU = PNULL;
			dpb_ptr->fs[i]->frame->pBufferHeader = PNULL;
#else
			dpb_ptr->fs[i]->frame->imgY = (uint8 *)H264Dec_ExtraMemAlloc_64WordAlign(frm_size);
			dpb_ptr->fs[i]->frame->imgU = (uint8 *)H264Dec_ExtraMemAlloc_64WordAlign(frm_size/2);
#endif
			dpb_ptr->fs[i]->frame->imgV = PNULL;
              	if (img_ptr->error_flag)    return;
			dpb_ptr->fs[i]->frame->imgYAddr = (uint32)dpb_ptr->fs[i]->frame->imgY>>8;  //y;
			dpb_ptr->fs[i]->frame->imgUAddr = (uint32)dpb_ptr->fs[i]->frame->imgU>>8;  //u;
			dpb_ptr->fs[i]->frame->imgVAddr = (uint32)dpb_ptr->fs[i]->frame->imgV>>8;  //v;
		}

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
	}

#if _CMODEL_ //for RTL simulation.
	dpb_ptr->fs[0]->frame->imgYAddr = H264DEC_FRAME0_Y_ADDR>>8;
	dpb_ptr->fs[1]->frame->imgYAddr = H264DEC_FRAME1_Y_ADDR>>8;
	dpb_ptr->fs[2]->frame->imgYAddr = H264DEC_FRAME2_Y_ADDR>>8;
	dpb_ptr->fs[3]->frame->imgYAddr = H264DEC_FRAME3_Y_ADDR>>8;
	dpb_ptr->fs[4]->frame->imgYAddr = H264DEC_FRAME4_Y_ADDR>>8;
	dpb_ptr->fs[5]->frame->imgYAddr = H264DEC_FRAME5_Y_ADDR>>8;
	dpb_ptr->fs[6]->frame->imgYAddr = H264DEC_FRAME6_Y_ADDR>>8;
	dpb_ptr->fs[7]->frame->imgYAddr = H264DEC_FRAME7_Y_ADDR>>8;
	dpb_ptr->fs[8]->frame->imgYAddr = H264DEC_FRAME8_Y_ADDR>>8;
	dpb_ptr->fs[9]->frame->imgYAddr = H264DEC_FRAME9_Y_ADDR>>8;
	dpb_ptr->fs[10]->frame->imgYAddr = H264DEC_FRAME10_Y_ADDR>>8;
	dpb_ptr->fs[11]->frame->imgYAddr = H264DEC_FRAME11_Y_ADDR>>8;
	dpb_ptr->fs[12]->frame->imgYAddr = H264DEC_FRAME12_Y_ADDR>>8;
	dpb_ptr->fs[13]->frame->imgYAddr = H264DEC_FRAME13_Y_ADDR>>8;
	dpb_ptr->fs[14]->frame->imgYAddr = H264DEC_FRAME14_Y_ADDR>>8;
	dpb_ptr->fs[15]->frame->imgYAddr = H264DEC_FRAME15_Y_ADDR>>8;
	dpb_ptr->fs[16]->frame->imgYAddr = H264DEC_FRAME16_Y_ADDR>>8;
#endif

	return;
}

LOCAL void H264Dec_unmark_for_reference (DEC_FRAME_STORE_T *fs_ptr)
{
#ifdef _VSP_LINUX_
	if(fs_ptr->frame->pBufferHeader!=NULL)
	{	
		if(fs_ptr->is_reference)
		{
			(*VSP_unbindCb)(g_user_data,fs_ptr->frame->pBufferHeader);
		}
		fs_ptr->frame->pBufferHeader = NULL;
	}
#endif
	fs_ptr->is_reference = 0;
	fs_ptr->is_long_term = 0;
	fs_ptr->is_short_term = 0;
	return;
}

void H264Dec_flush_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
{
	int32 i;
	int32 disp_num = 0;
	DEC_FRAME_STORE_T *tmp_fs_ptr;

	//mark all frame unused
	for (i = 0; i < dpb_ptr->used_size; i++)
	{
		H264Dec_unmark_for_reference (dpb_ptr->fs[i]);

		if (dpb_ptr->fs[i]->disp_status)
		{
			tmp_fs_ptr = dpb_ptr->fs[i];
			dpb_ptr->fs[i] = dpb_ptr->fs[disp_num];
			dpb_ptr->fs[disp_num] = tmp_fs_ptr;
			disp_num++;
		}
	}

	dpb_ptr->used_size = disp_num;

	return;
}

LOCAL void H264Dec_idr_memory_management (DEC_IMAGE_PARAMS_T *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
	if (picture_ptr->no_output_of_prior_pics_flag)
	{
		//nothing
	}else
	{
		H264Dec_flush_dpb (dpb_ptr);
	}

	if (img_ptr->long_term_reference_flag)
	{
		picture_ptr->is_long_term = 1;
		picture_ptr->long_term_frame_idx = 0;
		picture_ptr->long_term_pic_num = 0;
	}
	
	return;
}

LOCAL int32 H264Dec_get_pic_num_x (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1)
{
	int32 curr_pic_num = picture_ptr->frame_num;

	return (curr_pic_num - (difference_of_pic_nums_minus1 + 1));
}

LOCAL void H264Dec_mm_unmark_short_term_for_reference (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1)
{
	int32 pic_num_x;
	int32 i;

	pic_num_x = H264Dec_get_pic_num_x (picture_ptr, difference_of_pic_nums_minus1);

	for (i = 0; i < g_dpb_ptr->ref_frames_in_buffer; i++)
	{
		if ((g_dpb_ptr->fs_ref[i]->is_reference) && (g_dpb_ptr->fs_ref[i]->is_long_term == 0))
		{
			if (g_dpb_ptr->fs_ref[i]->frame->pic_num == pic_num_x)
			{
				H264Dec_unmark_for_reference(g_dpb_ptr->fs_ref[i]);

				return;
			}
		}
	}

	return;
}

LOCAL void H264Dec_mm_unmark_long_term_for_reference (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_pic_num)
{
	int32 i;

	for (i = 0; i < g_dpb_ptr->ltref_frames_in_buffer; i++)
	{
		if ((g_dpb_ptr->fs_ltref[i]->is_reference) && (g_dpb_ptr->fs_ltref[i]->is_long_term))
		{
			if (g_dpb_ptr->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num)
			{
				H264Dec_unmark_for_reference (g_dpb_ptr->fs_ltref[i]);
				return;
			}
		}
	}

	return;
}

LOCAL void H264Dec_unmark_long_term_for_reference_by_frame_idx (int32 long_term_frame_idx)
{
	int32 i;

	for (i = 0; i < g_dpb_ptr->ltref_frames_in_buffer;i++)
	{
		if (g_dpb_ptr->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx)
		{
			H264Dec_unmark_for_reference (g_dpb_ptr->fs_ltref[i]);
		}
	}
	
	return;
}

LOCAL void H264Dec_mark_pic_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx, int32 pic_num_x)
{
	int32 i;

	for (i = 0; i < g_dpb_ptr->ref_frames_in_buffer; i++)
	{
		if (g_dpb_ptr->fs_ref[i]->is_reference)
		{
			if ((!g_dpb_ptr->fs_ref[i]->frame->is_long_term) && (g_dpb_ptr->fs_ref[i]->frame->pic_num == pic_num_x))
			{
				g_dpb_ptr->fs_ref[i]->long_term_frame_idx = long_term_frame_idx;
				g_dpb_ptr->fs_ref[i]->is_long_term = 1;
				g_dpb_ptr->fs_ref[i]->is_short_term = 0;

				g_dpb_ptr->fs_ref[i]->frame->long_term_frame_idx = long_term_frame_idx;
				g_dpb_ptr->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
				g_dpb_ptr->fs_ref[i]->frame->is_long_term = 1;

				return;
			}
		}
	}

	return;
}

LOCAL void H264Dec_mm_assign_long_term_frame_idx (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1, int32 long_term_frame_idx)
{
	int32 pic_num_x;

	pic_num_x = H264Dec_get_pic_num_x (picture_ptr, difference_of_pic_nums_minus1);

	H264Dec_unmark_long_term_for_reference_by_frame_idx (long_term_frame_idx);
	H264Dec_mark_pic_long_term (picture_ptr, long_term_frame_idx, pic_num_x);
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
LOCAL void H264Dec_mm_update_max_long_term_frame_idx (int32 max_long_term_frame_idx_plus1)
{
	int32 i;

	g_dpb_ptr->max_long_term_pic_idx = max_long_term_frame_idx_plus1 -1;

	//check for invalid frames
	for (i = 0; i < g_dpb_ptr->ltref_frames_in_buffer; i++)
	{
		if (g_dpb_ptr->fs_ltref[i]->long_term_frame_idx > g_dpb_ptr->max_long_term_pic_idx)
		{
			H264Dec_unmark_for_reference (g_dpb_ptr->fs_ltref[i]);
		}
	}

	return;
}

//mark all short term reference pictures unused for reference
LOCAL void H264Dec_mm_unmark_all_short_term_for_reference (void)
{
	int32 i;

	for (i = 0; i < g_dpb_ptr->ref_frames_in_buffer; i++)
	{
		H264Dec_unmark_for_reference(g_dpb_ptr->fs_ref[i]);
	}

	H264Dec_update_ref_list (g_dpb_ptr);

	return;
}

LOCAL void H264Dec_mm_unmark_all_long_term_for_reference (void)
{
	H264Dec_mm_update_max_long_term_frame_idx(0);
}

//mark the current picture used for long term reference
LOCAL void H264Dec_mm_mark_current_picture_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx)
{
	//remove long term pictures with same long_term_frame_idx
	H264Dec_unmark_long_term_for_reference_by_frame_idx (long_term_frame_idx);

	picture_ptr->is_long_term = 1;
	picture_ptr->long_term_frame_idx = long_term_frame_idx;
	picture_ptr->long_term_pic_num = long_term_frame_idx;

	return;
}

LOCAL void H264Dec_adaptive_memory_management (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
	DEC_DEC_REF_PIC_MARKING_T *tmp_drpm_ptr;
	int32 i = 0;

	g_image_ptr->last_has_mmco_5 = 0;

	while (i < g_dec_ref_pic_marking_buffer_size)
	{
		tmp_drpm_ptr = &(picture_ptr->dec_ref_pic_marking_buffer[i]);

		switch (tmp_drpm_ptr->memory_management_control_operation)
		{
		case 0:
			if (i != g_dec_ref_pic_marking_buffer_size-1)
			{
				PRINTF("memory_management_control_operation = 0 not last operation buffer");
			}
			break;
		case 1:
			H264Dec_mm_unmark_short_term_for_reference (picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1);
			H264Dec_update_ref_list(g_dpb_ptr);
			break;
		case 2:
			H264Dec_mm_unmark_long_term_for_reference (picture_ptr, tmp_drpm_ptr->long_term_pic_num);
			H264Dec_update_ltref_list (g_dpb_ptr);
			break;
		case 3:
			H264Dec_mm_assign_long_term_frame_idx (picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1, tmp_drpm_ptr->long_term_frame_idx);
			H264Dec_update_ref_list (g_dpb_ptr);
			H264Dec_update_ltref_list(g_dpb_ptr);
			break;
		case 4:
			H264Dec_mm_update_max_long_term_frame_idx (tmp_drpm_ptr->max_long_term_frame_idx_plus1);
			H264Dec_update_ltref_list (g_dpb_ptr);
			break;
		case 5:
			H264Dec_mm_unmark_all_short_term_for_reference ();
			H264Dec_mm_unmark_all_long_term_for_reference ();
			g_image_ptr->last_has_mmco_5 = 1;
			break;
		case 6:
			H264Dec_mm_mark_current_picture_long_term (picture_ptr, tmp_drpm_ptr->long_term_frame_idx);
			if((int32)(g_dpb_ptr->ltref_frames_in_buffer +g_dpb_ptr->ref_frames_in_buffer)>(mmax(1, g_dpb_ptr->num_ref_frames)))
			{
				PRINTF ("max.number of reference frame exceed. invalid stream.");
			}
			break;
		default:
			PRINTF ("invalid memory_management_control_operation in buffer");
		}
		i++;
	}

	if(g_image_ptr->last_has_mmco_5)
	{
		picture_ptr->pic_num = picture_ptr->frame_num = 0;
		H264Dec_flush_dpb(g_dpb_ptr);
	}

	return;
}

LOCAL void H264Dec_insert_picture_in_display_list (DEC_FRAME_STORE_T *fs_ptr)
{
	//TBD

	return;
}

//mark the oldest short term reference to unref
LOCAL void H264Dec_sliding_window_memory_management (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
	int32 i;

	if (dpb_ptr->ref_frames_in_buffer == (dpb_ptr->num_ref_frames - dpb_ptr->ltref_frames_in_buffer))
	{
		for (i = 0; i < dpb_ptr->used_size; i++)
		{
			if ((dpb_ptr->fs[i]->is_reference) && (dpb_ptr->fs[i]->is_short_term))
			{
				H264Dec_unmark_for_reference (dpb_ptr->fs[i]);
				break;
			}
		}
	}

	picture_ptr->is_long_term = 0;

	return;
}

//if current frame is used as reference frame, put it to first unused store frame in dpb
LOCAL void H264Dec_insert_picture_in_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
	int32 used_size = dpb_ptr->used_size;
	DEC_FRAME_STORE_T *curr_fs_ptr = dpb_ptr->fs[MAX_REF_FRAME_NUMBER];
	DEC_FRAME_STORE_T *tmp_fs_ptr;
	
	if (picture_ptr->used_for_reference)
	{
		curr_fs_ptr->is_reference = 1;
#ifdef _VSP_LINUX_
		if(curr_fs_ptr->frame->pBufferHeader!=NULL)
			(*VSP_bindCb)(g_user_data,curr_fs_ptr->frame->pBufferHeader);
#endif
		if (picture_ptr->is_long_term)
		{
			curr_fs_ptr->is_long_term = 1;
			curr_fs_ptr->is_short_term = 0;
			curr_fs_ptr->long_term_frame_idx = picture_ptr->long_term_frame_idx;
		}else
		{
			curr_fs_ptr->is_short_term = 1;
			curr_fs_ptr->is_long_term = 0;
		}
	}else
	{
		curr_fs_ptr->is_short_term = 0;
		curr_fs_ptr->is_long_term = 0;
	}

	curr_fs_ptr->frame = picture_ptr;
	curr_fs_ptr->frame_num = picture_ptr->frame_num;
	curr_fs_ptr->poc = picture_ptr->poc;
	
	//put the current frame store to the first unused frame store of dpb
	tmp_fs_ptr = dpb_ptr->fs[used_size];
	dpb_ptr->fs[used_size] = curr_fs_ptr;
	curr_fs_ptr = tmp_fs_ptr;
	
	dpb_ptr->fs[MAX_REF_FRAME_NUMBER] = tmp_fs_ptr;

	if (!picture_ptr->non_existing)
	{
	// 	dpb_ptr->fs[used_size]->disp_status = 1;
		H264Dec_insert_picture_in_display_list (dpb_ptr->fs[used_size]); //insert picture in display list
	}

	dpb_ptr->used_size++;

	return;
}

PUBLIC void H264Dec_store_picture_in_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;

	img_ptr->last_has_mmco_5 = 0;

	if (picture_ptr->idr_flag)
	{
		H264Dec_idr_memory_management (img_ptr, dpb_ptr, picture_ptr);
	}else
	{
		if (picture_ptr->used_for_reference && (picture_ptr->adaptive_ref_pic_buffering_flag))
		{
			H264Dec_adaptive_memory_management (dpb_ptr, picture_ptr);
		}
	}

	if ((!picture_ptr->idr_flag) && (picture_ptr->used_for_reference && (!picture_ptr->adaptive_ref_pic_buffering_flag)))
	{
		H264Dec_sliding_window_memory_management (dpb_ptr, picture_ptr);
	}

	H264Dec_insert_picture_in_dpb (dpb_ptr, picture_ptr);

	H264Dec_update_ref_list (dpb_ptr);

	H264Dec_update_ltref_list (dpb_ptr);

	if (img_ptr->last_has_mmco_5)
	{
		img_ptr->pre_frame_num = 0;
	}

	return;
}

LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_short_term_pic (int32 pic_num)
{
	int32 i;

	for (i = 0; i < (g_dpb_ptr->ref_frames_in_buffer); i++)
	{
		if (g_dpb_ptr->fs_ref[i] == NULL)
		{
			g_image_ptr->error_flag |= ER_GET_SHORT_REF_ID;
            g_image_ptr->return_pos |= (1<<2);
            H264Dec_get_HW_status(g_image_ptr);
			return NULL;
		}
		if (g_dpb_ptr->fs_ref[i]->is_reference)
		{
			if (g_dpb_ptr->fs_ref[i]->frame == NULL)
			{
				g_image_ptr->error_flag |= ER_GET_SHORT_REF_ID;
                g_image_ptr->return_pos |= (1<<3);
                H264Dec_get_HW_status(g_image_ptr);
				return NULL;
			}
			if ((!g_dpb_ptr->fs_ref[i]->frame->is_long_term) && (g_dpb_ptr->fs_ref[i]->frame->pic_num == pic_num))
			{
				return g_dpb_ptr->fs_ref[i]->frame;
			}
		}
	}

	return g_no_reference_picture_ptr;
}

LOCAL void H264Dec_reorder_short_term (DEC_STORABLE_PICTURE_T **ref_picture_listX_ptr, int32 num_ref_idx_lX_active_minus1, int32 pic_num_lx, int32 *ref_idx_lx)
{
	int32 c_idx, n_idx;
	DEC_STORABLE_PICTURE_T *pic_lx_ptr;

	pic_lx_ptr = H264Dec_get_short_term_pic (pic_num_lx);

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
			if ((ref_picture_listX_ptr[c_idx]->is_long_term) || (ref_picture_listX_ptr[c_idx]->pic_num != pic_num_lx))
			{
				ref_picture_listX_ptr[n_idx++] = ref_picture_listX_ptr[c_idx];
			}
		}
	}

	return;
}

LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_long_term_pic (int32 long_term_pic_num)
{
	int32 i;

	for (i = 0; i < (g_dpb_ptr->ltref_frames_in_buffer); i++)
	{
		if (g_dpb_ptr->fs_ltref[i]->is_reference)
		{
			if ((g_dpb_ptr->fs_ltref[i]->frame->is_long_term) && (g_dpb_ptr->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num))
			{
				return g_dpb_ptr->fs_ltref[i]->frame;
			}
		}
	}

	return NULL;
}

LOCAL void H264Dec_reorder_long_term (DEC_STORABLE_PICTURE_T **ref_picture_listX_ptr, int32 num_ref_idx_lX_active_minus1, 
									  int32 long_term_pic_num, int32 *ref_idx_lx)
{
	int32 c_idx, n_idx;
	DEC_STORABLE_PICTURE_T *pic_lx_ptr;

	pic_lx_ptr = H264Dec_get_long_term_pic (long_term_pic_num);

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

LOCAL void H264Dec_reorder_ref_pic_list (DEC_STORABLE_PICTURE_T **picture_list_ptr, int32 num_ref_idx_lX_active_minus1,
										 int32 *remapping_of_pic_nums_idc, int32 *abs_diff_pic_num_minus1, int32 *long_term_pic_idx)
{
	int32 i;
	int32 max_pic_num, curr_pic_num, pic_num_lx_no_wrap, pic_num_lx_pred, pic_num_lx;
	int32 ref_idx_lx = 0;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;

	max_pic_num = img_ptr->max_frame_num;
	curr_pic_num = img_ptr->frame_num;

	pic_num_lx_pred = curr_pic_num;

	for (i = 0; remapping_of_pic_nums_idc[i] != 3; i++)
	{
		if (remapping_of_pic_nums_idc[i]>3)
		{
			PRINTF ("Invalid remapping_of_pic_nums_idc command");
			img_ptr->error_flag = TRUE;
            img_ptr->return_pos |= (1<<4);
            H264Dec_get_HW_status(img_ptr);
			return;
		}

		if (remapping_of_pic_nums_idc[i]<2)
		{
			if (remapping_of_pic_nums_idc[i] == 0)
			{
				if ((pic_num_lx_pred-(abs_diff_pic_num_minus1[i]+1))<0)
				{
					pic_num_lx_no_wrap = pic_num_lx_pred - (abs_diff_pic_num_minus1[i]+1) + max_pic_num;
				}else
				{
					pic_num_lx_no_wrap = pic_num_lx_pred - (abs_diff_pic_num_minus1[i]+1);
				}
			}else //(remapping_of_pic_nums_idc[i]==1)
			{
				if ((pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1)) >= max_pic_num)
				{
					pic_num_lx_no_wrap = pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1) - max_pic_num;
				}else
				{
					pic_num_lx_no_wrap = pic_num_lx_pred + (abs_diff_pic_num_minus1[i]+1);
				}
			}
		
			pic_num_lx_pred = pic_num_lx_no_wrap;

			if (pic_num_lx_no_wrap > curr_pic_num)
			{
				pic_num_lx = pic_num_lx_no_wrap - max_pic_num;
			}else
			{
				pic_num_lx = pic_num_lx_no_wrap;
			}

			H264Dec_reorder_short_term (picture_list_ptr, num_ref_idx_lX_active_minus1, pic_num_lx, &ref_idx_lx);
		}else //(remapping_of_pic_nums_idc[i]==2)
		{
			H264Dec_reorder_long_term(picture_list_ptr, num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &ref_idx_lx);
		}
	}

	return;
}

PUBLIC void H264Dec_reorder_list (void)
{
	if (g_image_ptr->type != I_SLICE)
	{
		if (g_curr_slice_ptr->ref_pic_list_reordering_flag_l0)
		{
			H264Dec_reorder_ref_pic_list (g_list0, g_image_ptr->num_ref_idx_l0_active-1,
				g_curr_slice_ptr->remapping_of_pic_nums_idc_l0,
				g_curr_slice_ptr->abs_diff_pic_num_minus1_l0,
				g_curr_slice_ptr->long_term_pic_idx_l0);
		}
	}

	return;
}

PUBLIC void H264Dec_init_list (DEC_IMAGE_PARAMS_T *img_ptr, int32 curr_slice_type)
{
	int32 i;
	int32 list0_idx = 0;
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
	DEC_STORABLE_PICTURE_T **list = g_list0;
	int32 max_frame_num = (1<<(g_active_sps_ptr->log2_max_frame_num_minus4+4));

	for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
	{
 		if ((dpb_ptr->fs_ref[i]->frame->used_for_reference)&&(!dpb_ptr->fs_ref[i]->frame->is_long_term))
		{
			if (dpb_ptr->fs_ref[i]->frame_num > img_ptr->frame_num)
			{
				dpb_ptr->fs_ref[i]->frame_num_wrap = dpb_ptr->fs_ref[i]->frame_num - max_frame_num;
			}else
			{
				dpb_ptr->fs_ref[i]->frame_num_wrap = dpb_ptr->fs_ref[i]->frame_num;
			}
		}

		dpb_ptr->fs_ref[i]->frame->pic_num = dpb_ptr->fs_ref[i]->frame_num_wrap;
	}

	//update long_term_pic_num
	for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
	{
		if (dpb_ptr->fs_ltref[i]->frame->is_long_term)
		{
			dpb_ptr->fs_ltref[i]->frame->long_term_pic_num = dpb_ptr->fs_ltref[i]->frame->long_term_frame_idx;
		}
	}

	if (curr_slice_type == I_SLICE)
	{
		g_list_size = 0;

		return;
	}

	/*p slice, put short and long term reference frame into list
	short term is in descending mode, long term is in ascending mode*/
	for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
	{
		list[i] = dpb_ptr->fs_ref[dpb_ptr->ref_frames_in_buffer-1-i]->frame;
		list[i]->mc_ref_pic_num = dpb_ptr->ref_frames_in_buffer-1-i;
		list0_idx++;
	}

	//start of long_term list
	for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
	{
		list[list0_idx+i] = dpb_ptr->fs_ltref[i]->frame;
		list[list0_idx+i]->mc_ref_pic_num = list0_idx+i;
	}

	g_list_size = dpb_ptr->ref_frames_in_buffer + dpb_ptr->ltref_frames_in_buffer;

	for (i = g_list_size; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		list[i] = g_no_reference_picture_ptr;
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
