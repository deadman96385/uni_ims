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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//extern int32  video_buffer_malloced;

PUBLIC void H264Dec_init_img_buffer (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 mb_num_x = img_ptr->frame_width_in_mbs;
	int32 total_mb_num = mb_num_x * img_ptr->frame_height_in_mbs;
#if SIM_IN_WIN
	DEC_MB_INFO_T *mb_info;
	int32 i;
#endif
	//img_ptr->vld_cabac_table_ptr = (uint32 *)H264Dec_ExtraMemAlloc_64WordAlign (100*sizeof(uint32));//100*40//weihu
	img_ptr->vld_cabac_table_ptr = (uint32 *)H264Dec_InterMemAlloc (100*sizeof(uint32));//100*40//weihu	
#if SIM_IN_WIN
	img_ptr->ipred_top_line_buffer = (uint8 *)H264Dec_ExtraMemAlloc_64WordAlign (mb_num_x*32);
	H264Dec_ExtraMemAlloc (2*mb_num_x*sizeof(DEC_MB_INFO_T)); //for posy = -1 mb.
	img_ptr->mb_info = (DEC_MB_INFO_T *)H264Dec_ExtraMemAlloc ((uint32)total_mb_num*sizeof(DEC_MB_INFO_T));

	//for fmo
	mb_info = img_ptr->mb_info;
	for(i = 0; i < total_mb_num; i++)
	{
		mb_info->slice_nr = -1;
		mb_info++;
	}
#endif
	return;
}
#if _MVC_
PUBLIC void	H264Dec_init_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, int type)
#else
PUBLIC void	H264Dec_init_dpb (DEC_IMAGE_PARAMS_T *img_ptr)
#endif
{
	int32 i;
	int32 frm_size = img_ptr->width * img_ptr->height;
	int32 frm_size_in_blk = (frm_size >> 4);
#if (!_MVC_)
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif

	dpb_ptr->used_size = 0;
	dpb_ptr->ref_frames_in_buffer = 0;
	dpb_ptr->ltref_frames_in_buffer = 0;

#if _MVC_
	dpb_ptr->num_ref_frames = g_active_sps_ptr->num_ref_frames;
#endif

	//if (dpb_ptr->fs == PNULL)
	{
		dpb_ptr->fs = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));//H264Dec_ExtraMemAlloc
	}
	
	//if (dpb_ptr->fs_ref == PNULL)
	{
		dpb_ptr->fs_ref = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));//H264Dec_ExtraMemAlloc
	}
	
	//if (dpb_ptr->fs_ltref == PNULL)
	{
		dpb_ptr->fs_ltref = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1));//H264Dec_ExtraMemAlloc
	}
#if _MVC_
	//if (dpb_ptr->fs_ilref == PNULL)
	{
		dpb_ptr->fs_ilref = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*(1));//H264Dec_ExtraMemAlloc
	}
#endif

	for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		//each storable_picture buffer is bonding to a frame store
		//if (dpb_ptr->fs[i] == PNULL)
		{
			dpb_ptr->fs[i] = (DEC_FRAME_STORE_T *)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T));//H264Dec_ExtraMemAlloc
			dpb_ptr->fs[i]->frame = (DEC_STORABLE_PICTURE_T *)H264Dec_InterMemAlloc(sizeof(DEC_STORABLE_PICTURE_T));//H264Dec_ExtraMemAlloc
#if SIM_IN_WIN
			dpb_ptr->fs[i]->frame->mv[0] = (int16 *)H264Dec_ExtraMemAlloc((frm_size_in_blk << 1) * sizeof(int16));
			dpb_ptr->fs[i]->frame->mv[1] = (int16 *)H264Dec_ExtraMemAlloc((frm_size_in_blk << 1) * sizeof(int16));
			dpb_ptr->fs[i]->frame->ref_idx[0] = (int8 *)H264Dec_ExtraMemAlloc(frm_size_in_blk);
			dpb_ptr->fs[i]->frame->ref_idx[1] = (int8 *)H264Dec_ExtraMemAlloc(frm_size_in_blk);
			dpb_ptr->fs[i]->frame->ref_pic_id[0] = (int32 *)H264Dec_ExtraMemAlloc(frm_size_in_blk * sizeof(int32));
			dpb_ptr->fs[i]->frame->ref_pic_id[1] = (int32 *)H264Dec_ExtraMemAlloc(frm_size_in_blk * sizeof(int32));
#else
            dpb_ptr->fs[i]->frame->direct_mb_info =PNULL;//(int32 *)H264Dec_ExtraMemAlloc_64WordAlign((frm_size_in_blk>>4) * 20*sizeof(int32));//weihu
#endif
			dpb_ptr->fs[i]->frame->imgY = PNULL;//(uint8 *)H264Dec_ExtraMemAlloc_64WordAlign(frm_size);
			dpb_ptr->fs[i]->frame->imgU = PNULL;//(uint8 *)H264Dec_ExtraMemAlloc_64WordAlign(frm_size/2);
			dpb_ptr->fs[i]->frame->imgV = PNULL;
			dpb_ptr->fs[i]->frame->imgYAddr = (uint32)dpb_ptr->fs[i]->frame->imgY>>8;  //y;
			dpb_ptr->fs[i]->frame->imgUAddr = (uint32)dpb_ptr->fs[i]->frame->imgU>>8;  //u;
			dpb_ptr->fs[i]->frame->imgVAddr = (uint32)dpb_ptr->fs[i]->frame->imgV>>8;  //v;
#if _MVC_
		    dpb_ptr->fs[i]->view_id = -1;//MVC_INIT_VIEW_ID;
			dpb_ptr->fs[i]->inter_view_flag[0] = dpb_ptr->fs[i]->inter_view_flag[1] = 0;
			dpb_ptr->fs[i]->anchor_pic_flag[0] = dpb_ptr->fs[i]->anchor_pic_flag[1] = 0;
#endif
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
		dpb_ptr->fs[i]->pic_num =0;
		dpb_ptr->fs[i]->frame_num_wrap=0;
		dpb_ptr->fs[i]->view_id=0;

	}
#if _MVC_
	if (type == 2)
	{
		dpb_ptr->fs_ilref[0] = (DEC_FRAME_STORE_T *)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T));//H264Dec_ExtraMemAlloc
		// These may need some cleanups
		dpb_ptr->fs_ilref[0]->view_id = -1;//MVC_INIT_VIEW_ID;
		dpb_ptr->fs_ilref[0]->inter_view_flag[0] = dpb_ptr->fs_ilref[0]->inter_view_flag[1] = 0;
		dpb_ptr->fs_ilref[0]->anchor_pic_flag[0] = dpb_ptr->fs_ilref[0]->anchor_pic_flag[1] = 0;
		// given that this is in a different buffer, do we even need proc_flag anymore?   
	}
	else
		dpb_ptr->fs_ilref[0] = NULL;
#endif

#if SIM_IN_WIN//for RTL simulation.
/*
	dpb_ptr->fs[0]->frame->imgYAddr = H264DEC_FRAME0_Y_ADDR>>8;//need extend to 2M Byte//weihu
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
*/
#endif
	
	for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		dpb_ptr->fs[i]->frame->DPB_addr_index = i+(type==1 ? 0 : MAX_REF_FRAME_NUMBER+1);//weihu
		dpb_ptr->fs[i]->frame->direct_mb_info_Addr = direct_mb_info_addr[i];
	}
    
#if _MVC_
	dpb_ptr->last_output_view_id = -1;
	dpb_ptr->init_done = 1;
#endif
	dpb_ptr->size = MAX_REF_FRAME_NUMBER;

	return;
}

LOCAL void H264Dec_unmark_for_reference (DEC_FRAME_STORE_T *fs_ptr)
{

	fs_ptr->is_reference = 0;
	fs_ptr->is_long_term = 0;
	fs_ptr->is_short_term = 0;

/*	if(fs_ptr->frame->pBufferHeader!=NULL)
	{	
		if(!fs_ptr->is_reference)
		{
			OR_VSP_UNBIND(fs_ptr->frame->pBufferHeader);
			fs_ptr->frame->pBufferHeader = NULL;
		}
	}*/


	return;
}
#if _MVC_
LOCAL void H264Dec_get_smallest_poc (int32 *poc,int32 * pos, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_get_smallest_poc (int32 *poc,int32 * pos)
#endif
{
	int32 i;
#if (!_MVC_)
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif

	if (dpb_ptr->used_size<1)
	{
		g_image_ptr->error_flag = TRUE;
		return;
    //error("Cannot determine smallest POC, DPB empty.",150);
	}

	*pos=-1;
	*poc = SINT_MAX;
	for (i=0; i<dpb_ptr->used_size; i++)
	{
		if ((*poc > dpb_ptr->fs[i]->poc)&&(!dpb_ptr->fs[i]->disp_status))
		{
			*poc = dpb_ptr->fs[i]->poc;
			*pos=i;
		}
	}
	return;
}
#if _MVC_
LOCAL void H264Dec_output_one_frame_from_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_output_one_frame_from_dpb ()
#endif
{
	int32 poc, pos;
#if (!_MVC_)
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif
	DEC_FRAME_STORE_T **fs = dpb_ptr->fs;
	DEC_STORABLE_PICTURE_T *frame;
#if _MVC_
	H264Dec_get_smallest_poc(&poc, &pos, dpb_ptr);
#else
	H264Dec_get_smallest_poc(&poc, &pos);
#endif
	if (pos<0)
	{
		g_image_ptr->error_flag = TRUE;
		return;
		
	}

	frame = fs[pos]->frame;
  display_array_BH[display_array_len]=frame->pBufferHeader;
	display_array_Y[display_array_len] = frame->imgY;
	display_array_UV[display_array_len++] = frame->imgU;
 
 //	PRINTF ("output frame with frame_num #%d, poc %d (dpb_ptr.size=%d, dpb.used_size=%d), total frame num %d\n", dpb_ptr->fs[pos]->frame_num, dpb_ptr->fs[pos]->frame->poc, dpb_ptr->size, dpb_ptr->used_size, g_nFrame_dec_h264);
#if SIM_IN_WIN
#if FPGA_AUTO_VERIFICATION
#else
	PRINTF ("\ndisplay array: %3d\t frame poc %d\t display_array_len %d\t addr_index %d\t", g_dispFrmNum, poc, display_array_len, frame->DPB_addr_index);
    
	write_out_frame(frame);//weihu true output
#endif
#endif
	fs[pos]->disp_status = 1;

	if (!fs[pos]->is_reference)
 	{
		h264Dec_remove_frame_from_dpb(dpb_ptr, pos);
	}
}
#if _MVC_
PUBLIC void H264Dec_flush_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
PUBLIC void H264Dec_flush_dpb ()
#endif
{
	int32 i;
	int32 disp_num = 0;
#if (!_MVC_)
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif
	DEC_FRAME_STORE_T *tmp_fs_ptr;

	//mark all frame unused
	for (i = 0; i < dpb_ptr->used_size; i++)
	{
		H264Dec_unmark_for_reference (dpb_ptr->fs[i]);

		if (!dpb_ptr->fs[i]->disp_status)
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
#if _MVC_
		H264Dec_output_one_frame_from_dpb(dpb_ptr);
#else
		H264Dec_output_one_frame_from_dpb();
#endif
	}

	return;
}

LOCAL void H264Dec_idr_memory_management (DEC_IMAGE_PARAMS_T *img_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
{
	if (picture_ptr->no_output_of_prior_pics_flag)
	{
		//nothing
	}else
	{
#if _MVC_
		H264Dec_flush_dpb (dpb_ptr);
#else
		H264Dec_flush_dpb ();
#endif
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
#if _MVC_
LOCAL void H264Dec_mm_unmark_short_term_for_reference (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_unmark_short_term_for_reference (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1)
#endif
{
	int32 pic_num_x;
	int32 i;

	pic_num_x = H264Dec_get_pic_num_x (picture_ptr, difference_of_pic_nums_minus1);
#if _MVC_
	for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
	{
		if ((dpb_ptr->fs_ref[i]->is_reference) && (dpb_ptr->fs_ref[i]->is_long_term == 0))
		{
			if (dpb_ptr->fs_ref[i]->frame->pic_num == pic_num_x)
			{
				H264Dec_unmark_for_reference(dpb_ptr->fs_ref[i]);
				return;
			}
		}
	}
#else
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
#endif

	return;
}
#if _MVC_
LOCAL void H264Dec_mm_unmark_long_term_for_reference (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_pic_num, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_unmark_long_term_for_reference (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_pic_num)
#endif
{
	int32 i;
#if _MVC_
	for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
	{
		if ((dpb_ptr->fs_ltref[i]->is_reference) && (dpb_ptr->fs_ltref[i]->is_long_term))
		{
			if (dpb_ptr->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num)
			{
				H264Dec_unmark_for_reference (dpb_ptr->fs_ltref[i]);
				return;
			}
		}
	}
#else
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
#endif
	return;
}
#if _MVC_
LOCAL void H264Dec_unmark_long_term_for_reference_by_frame_idx (int32 long_term_frame_idx, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_unmark_long_term_for_reference_by_frame_idx (int32 long_term_frame_idx)
#endif
{
	int32 i;
#if _MVC_
	for (i = 0; i < dpb_ptr->ltref_frames_in_buffer;i++)
	{
		if (dpb_ptr->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx)
		{
			H264Dec_unmark_for_reference (dpb_ptr->fs_ltref[i]);
		}
	}
#else
	for (i = 0; i < g_dpb_ptr->ltref_frames_in_buffer;i++)
	{
		if (g_dpb_ptr->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx)
		{
			H264Dec_unmark_for_reference (g_dpb_ptr->fs_ltref[i]);
		}
	}
#endif
	
	return;
}
#if _MVC_
LOCAL void H264Dec_mark_pic_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx, int32 pic_num_x, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mark_pic_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx, int32 pic_num_x)
#endif
{
	int32 i;
#if _MVC_
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
#else
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
#endif

	return;
}
#if _MVC_
LOCAL void H264Dec_mm_assign_long_term_frame_idx (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1, int32 long_term_frame_idx, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_assign_long_term_frame_idx (DEC_STORABLE_PICTURE_T *picture_ptr, int32 difference_of_pic_nums_minus1, int32 long_term_frame_idx)
#endif
{
	int32 pic_num_x;

	pic_num_x = H264Dec_get_pic_num_x (picture_ptr, difference_of_pic_nums_minus1);
#if _MVC_
	H264Dec_unmark_long_term_for_reference_by_frame_idx (long_term_frame_idx, dpb_ptr);
	H264Dec_mark_pic_long_term (picture_ptr, long_term_frame_idx, pic_num_x, dpb_ptr);
#else
	H264Dec_unmark_long_term_for_reference_by_frame_idx (long_term_frame_idx);
	H264Dec_mark_pic_long_term (picture_ptr, long_term_frame_idx, pic_num_x);
#endif
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
#if _MVC_
LOCAL void H264Dec_mm_update_max_long_term_frame_idx (int32 max_long_term_frame_idx_plus1, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_update_max_long_term_frame_idx (int32 max_long_term_frame_idx_plus1)
#endif
{
	int32 i;
#if _MVC_
	dpb_ptr->max_long_term_pic_idx = max_long_term_frame_idx_plus1 -1;

	//check for invalid frames
	for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
	{
		if (dpb_ptr->fs_ltref[i]->long_term_frame_idx > dpb_ptr->max_long_term_pic_idx)
		{
			H264Dec_unmark_for_reference (dpb_ptr->fs_ltref[i]);
		}
	}
#else
	g_dpb_ptr->max_long_term_pic_idx = max_long_term_frame_idx_plus1 -1;

	//check for invalid frames
	for (i = 0; i < g_dpb_ptr->ltref_frames_in_buffer; i++)
	{
		if (g_dpb_ptr->fs_ltref[i]->long_term_frame_idx > g_dpb_ptr->max_long_term_pic_idx)
		{
			H264Dec_unmark_for_reference (g_dpb_ptr->fs_ltref[i]);
		}
	}
#endif

	return;
}

//mark all short term reference pictures unused for reference
#if _MVC_
LOCAL void H264Dec_mm_unmark_all_short_term_for_reference (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_unmark_all_short_term_for_reference (void)
#endif
{
	int32 i;
#if _MVC_
	for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
	{
		H264Dec_unmark_for_reference(dpb_ptr->fs_ref[i]);
	}

	H264Dec_update_ref_list (dpb_ptr);
#else
	for (i = 0; i < g_dpb_ptr->ref_frames_in_buffer; i++)
	{
		H264Dec_unmark_for_reference(g_dpb_ptr->fs_ref[i]);
	}

	H264Dec_update_ref_list (g_dpb_ptr);
#endif

	return;
}
#if _MVC_
LOCAL void H264Dec_mm_unmark_all_long_term_for_reference (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_unmark_all_long_term_for_reference (void)
#endif
{
#if _MVC_
	H264Dec_mm_update_max_long_term_frame_idx(0, dpb_ptr);
#else
	H264Dec_mm_update_max_long_term_frame_idx(0);
#endif
}

//mark the current picture used for long term reference
#if _MVC_
LOCAL void H264Dec_mm_mark_current_picture_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void H264Dec_mm_mark_current_picture_long_term (DEC_STORABLE_PICTURE_T *picture_ptr, int32 long_term_frame_idx)
#endif
{
	//remove long term pictures with same long_term_frame_idx
#if _MVC_
	H264Dec_unmark_long_term_for_reference_by_frame_idx (long_term_frame_idx, dpb_ptr);
#else
	H264Dec_unmark_long_term_for_reference_by_frame_idx (long_term_frame_idx);
#endif

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
				g_image_ptr->error_flag=TRUE;
				PRINTF("memory_management_control_operation = 0 not last operation buffer");
			}
			break;
		case 1:
#if _MVC_
			H264Dec_mm_unmark_short_term_for_reference (picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1, dpb_ptr);
			H264Dec_update_ref_list(dpb_ptr);
#else
			H264Dec_mm_unmark_short_term_for_reference (picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1);
			H264Dec_update_ref_list(g_dpb_ptr);
#endif
			break;
		case 2:
#if _MVC_
			H264Dec_mm_unmark_long_term_for_reference (picture_ptr, tmp_drpm_ptr->long_term_pic_num, dpb_ptr);
			H264Dec_update_ltref_list (dpb_ptr);
#else
			H264Dec_mm_unmark_long_term_for_reference (picture_ptr, tmp_drpm_ptr->long_term_pic_num);
			H264Dec_update_ltref_list (g_dpb_ptr);
#endif
			break;
		case 3:
#if _MVC_
			H264Dec_mm_assign_long_term_frame_idx (picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1, tmp_drpm_ptr->long_term_frame_idx, dpb_ptr);
			H264Dec_update_ref_list (dpb_ptr);
			H264Dec_update_ltref_list(dpb_ptr);
#else
			H264Dec_mm_assign_long_term_frame_idx (picture_ptr, tmp_drpm_ptr->difference_of_pic_nums_minus1, tmp_drpm_ptr->long_term_frame_idx);
			H264Dec_update_ref_list (g_dpb_ptr);
			H264Dec_update_ltref_list(g_dpb_ptr);
#endif
			break;
		case 4:
#if _MVC_
			H264Dec_mm_update_max_long_term_frame_idx (tmp_drpm_ptr->max_long_term_frame_idx_plus1, dpb_ptr);
			H264Dec_update_ltref_list (dpb_ptr);
#else
			H264Dec_mm_update_max_long_term_frame_idx (tmp_drpm_ptr->max_long_term_frame_idx_plus1);
			H264Dec_update_ltref_list (g_dpb_ptr);
#endif
			break;
		case 5:
#if _MVC_
			H264Dec_mm_unmark_all_short_term_for_reference (dpb_ptr);
			H264Dec_mm_unmark_all_long_term_for_reference (dpb_ptr);
#else
			H264Dec_mm_unmark_all_short_term_for_reference ();
			H264Dec_mm_unmark_all_long_term_for_reference ();
#endif
			g_image_ptr->last_has_mmco_5 = 1;
			break;
		case 6:
#if _MVC_
			H264Dec_mm_mark_current_picture_long_term (picture_ptr, tmp_drpm_ptr->long_term_frame_idx, dpb_ptr);
			if((int32)(dpb_ptr->ltref_frames_in_buffer +dpb_ptr->ref_frames_in_buffer)>(mmax(1, dpb_ptr->num_ref_frames)))
			{
				g_image_ptr->error_flag=TRUE;
				PRINTF ("max.number of reference frame exceed. invalid stream.");
			}
#else
			H264Dec_mm_mark_current_picture_long_term (picture_ptr, tmp_drpm_ptr->long_term_frame_idx);
			if((int32)(g_dpb_ptr->ltref_frames_in_buffer +g_dpb_ptr->ref_frames_in_buffer)>(mmax(1, g_dpb_ptr->num_ref_frames)))
			{
				g_image_ptr->error_flag=TRUE;
				PRINTF ("max.number of reference frame exceed. invalid stream.");
			}
#endif
			break;
		default:
			{			
			   PRINTF ("invalid memory_management_control_operation in buffer");
			   g_image_ptr->error_flag=TRUE;
			}
		}
		i++;
	}

	if(g_image_ptr->last_has_mmco_5)
	{
		picture_ptr->pic_num = picture_ptr->frame_num = 0;
		g_image_ptr->toppoc -= picture_ptr->poc;
		g_image_ptr->bottompoc -= picture_ptr->poc;
		picture_ptr->poc -= picture_ptr->poc;
		g_image_ptr->framepoc = picture_ptr->poc;
		g_image_ptr->ThisPOC = picture_ptr->poc;
		H264Dec_flush_dpb(dpb_ptr);
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

	if (dpb_ptr->ref_frames_in_buffer >= (dpb_ptr->num_ref_frames - dpb_ptr->ltref_frames_in_buffer))//for error ==
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
LOCAL void H264Dec_insert_picture_in_dpb (DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr, DEC_FRAME_STORE_T *curr_fs_ptr, DEC_STORABLE_PICTURE_T *picture_ptr, int8 put_to_dpb)
{
	int32 used_size = dpb_ptr->used_size;
	//DEC_FRAME_STORE_T *curr_fs_ptr;
	DEC_FRAME_STORE_T *tmp_fs_ptr;

	
    if(curr_fs_ptr->frame->pBufferHeader!=NULL)
		OR_VSP_BIND(curr_fs_ptr->frame->pBufferHeader);

	if (picture_ptr->used_for_reference)
	{
		curr_fs_ptr->is_reference = 1;
		
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

#if _MVC_
    curr_fs_ptr->view_id = picture_ptr->view_id;
    curr_fs_ptr->inter_view_flag[0] = curr_fs_ptr->inter_view_flag[1] = picture_ptr->inter_view_flag;
    curr_fs_ptr->anchor_pic_flag[0] = curr_fs_ptr->anchor_pic_flag[1] = picture_ptr->anchor_pic_flag;
#endif

	curr_fs_ptr->frame = picture_ptr;
	curr_fs_ptr->frame_num = picture_ptr->frame_num;
	curr_fs_ptr->poc = picture_ptr->poc;
//	curr_fs_ptr->disp_status = 0;
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
void store_proc_picture_in_dpb(DEC_DECODED_PICTURE_BUFFER_T *p_Dpb, DEC_STORABLE_PICTURE_T *p)
{
	int8 put_to_dpb = 0;
	DEC_FRAME_STORE_T *fs = p_Dpb->fs_ilref[0];
	{
		fs->frame = NULL;
		fs->is_reference = 0;
	}

	H264Dec_insert_picture_in_dpb(p_Dpb, fs, p, put_to_dpb);

}
#endif
#if _MVC_
LOCAL void dump_dpb(DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL void dump_dpb(void)
#endif
{
#if (!_MVC_)
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;	
#endif
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
#if _MVC_
PUBLIC void H264Dec_store_picture_in_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T *picture_ptr, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
PUBLIC void H264Dec_store_picture_in_dpb (DEC_IMAGE_PARAMS_T *img_ptr, DEC_STORABLE_PICTURE_T *picture_ptr)
#endif
{
#if (!_MVC_)
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif
	int8 put_to_dpb = 1;
	int display_delay_frame = 2;

	if(display_delay_frame>16)
		display_delay_frame = 16;

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
//jin.zhou
	// first try to remove unused frames
//	  if (dpb_ptr->used_size == dpb_ptr->size)
  	if (dpb_ptr->used_size >= display_delay_frame)
	  {
	    	H264Dec_remove_unused_frame_from_dpb(dpb_ptr);
	  }
	//  while (dpb_ptr->used_size == dpb_ptr->size)
	if( dpb_ptr->used_size >= display_delay_frame)
	  {
	    // flush a frame
#if _MVC_
	    	H264Dec_output_one_frame_from_dpb(dpb_ptr);
#else
			H264Dec_output_one_frame_from_dpb(img_ptr);
#endif
			if (g_image_ptr->error_flag)//for error
			{
			//	break;			
			}
	  }
	H264Dec_insert_picture_in_dpb (dpb_ptr, dpb_ptr->fs[MAX_REF_FRAME_NUMBER], picture_ptr, put_to_dpb);


	H264Dec_update_ref_list (dpb_ptr);

	H264Dec_update_ltref_list (dpb_ptr);

	if (img_ptr->last_has_mmco_5)
	{
		img_ptr->pre_frame_num = 0;
	}

//	dump_dpb();

	return;
}
#if _MVC_
LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_short_term_pic (int32 pic_num, DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr)
#else
LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_short_term_pic (int32 pic_num)
#endif
{
	int32 i;
#if _MVC_
	for (i = 0; i < (dpb_ptr->ref_frames_in_buffer); i++)
	{
		if (dpb_ptr->fs_ref[i] == NULL)
		{
			g_image_ptr->error_flag = TRUE;
			return NULL;//weihu//return g_no_reference_picture_ptr
		}
		if (dpb_ptr->fs_ref[i]->is_reference)
		{
			if (dpb_ptr->fs_ref[i]->frame == NULL)
			{
				g_image_ptr->error_flag = TRUE;
				return NULL;//weihu
			}
			if ((!dpb_ptr->fs_ref[i]->frame->is_long_term) && (dpb_ptr->fs_ref[i]->frame->pic_num == pic_num))
			{
				return dpb_ptr->fs_ref[i]->frame;
			}
		}
	}
#else
	for (i = 0; i < (g_dpb_ptr->ref_frames_in_buffer); i++)
	{
		if (g_dpb_ptr->fs_ref[i] == NULL)
		{
			g_image_ptr->error_flag = TRUE;
			return NULL;
		}
		if (g_dpb_ptr->fs_ref[i]->is_reference)
		{
			if (g_dpb_ptr->fs_ref[i]->frame == NULL)
			{
				g_image_ptr->error_flag = TRUE;
				return NULL;
			}
			if ((!g_dpb_ptr->fs_ref[i]->frame->is_long_term) && (g_dpb_ptr->fs_ref[i]->frame->pic_num == pic_num))
			{
				return g_dpb_ptr->fs_ref[i]->frame;
			}
		}
	}
#endif

	return g_no_reference_picture_ptr;
}

LOCAL void H264Dec_reorder_short_term (DEC_STORABLE_PICTURE_T **ref_picture_listX_ptr, int32 num_ref_idx_lX_active_minus1, int32 pic_num_lx, int32 *ref_idx_lx)
{
	int32 c_idx, n_idx;
	DEC_STORABLE_PICTURE_T *pic_lx_ptr;

	pic_lx_ptr = H264Dec_get_short_term_pic (pic_num_lx, g_curr_slice_ptr->p_Dpb);

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

LOCAL DEC_STORABLE_PICTURE_T *H264Dec_get_long_term_pic (int32 long_term_pic_num)
{
	int32 i;

	for (i = 0; i < (g_curr_slice_ptr->p_Dpb->ltref_frames_in_buffer); i++)
	{
		if (g_curr_slice_ptr->p_Dpb->fs_ltref[i] == NULL)
		{
			g_image_ptr->error_flag = TRUE;
			return NULL;
		}
		if (g_curr_slice_ptr->p_Dpb->fs_ltref[i]->is_reference)
		{
			if (g_curr_slice_ptr->p_Dpb->fs_ltref[i]->frame == NULL)
			{
				g_image_ptr->error_flag = TRUE;
				return NULL;
			}
			if ((g_curr_slice_ptr->p_Dpb->fs_ltref[i]->frame->is_long_term) 
			 && (g_curr_slice_ptr->p_Dpb->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num))
			{
				return g_curr_slice_ptr->p_Dpb->fs_ltref[i]->frame;
			}
		}
	}

	return g_no_reference_picture_ptr;//NULL;//weihu
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
#if _MVC_
LOCAL DEC_STORABLE_PICTURE_T *get_inter_view_pic(int targetViewID, int currPOC, int listidx)
{
  unsigned i;
  unsigned int listinterview_size;
  struct frame_store_tag **fs_listinterview;

  if (listidx == 0)
  {
    fs_listinterview = g_curr_slice_ptr->fs_listinterview0;
    listinterview_size = g_curr_slice_ptr->listinterviewidx0; 
  }
  else
  {
    fs_listinterview = g_curr_slice_ptr->fs_listinterview1;
    listinterview_size = g_curr_slice_ptr->listinterviewidx1; 
  }

  for(i=0; i<listinterview_size; i++)
  {
    if (fs_listinterview[i]->layer_id == GetVOIdx( targetViewID ))
    {
        return fs_listinterview[i]->frame;
    }
  }

  return NULL;
}

LOCAL void reorder_interview(DEC_STORABLE_PICTURE_T **RefPicListX, int32 num_ref_idx_lX_active_minus1, 
							 int32 *refIdxLX, int8 targetViewID, int32 currPOC, int listidx)
{
  int cIdx, nIdx;
  DEC_STORABLE_PICTURE_T *picLX;

  picLX = get_inter_view_pic(targetViewID, currPOC, listidx);

  if (picLX)
  {
    for( cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx-- )
      RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];

    RefPicListX[ (*refIdxLX)++ ] = picLX;

    nIdx = *refIdxLX;

    for( cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++ )
    {
      if((GetViewIdx( RefPicListX[cIdx]->view_id ) != targetViewID) || (RefPicListX[cIdx]->poc != currPOC))
        RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];
    }
  }
}
LOCAL void H264Dec_reorder_ref_pic_list_mvc (DEC_STORABLE_PICTURE_T **picture_list_ptr,
										 int32 num_ref_idx_lX_active_minus1,
										 int32 *remapping_of_pic_nums_idc,
										 int32 *abs_diff_pic_num_minus1,
										 int32 *long_term_pic_idx,
										 int8 listNO,
										 int32 **anchor_ref,
										 int32 **non_anchor_ref,
										 int8 view_id,
										 int8 anchor_pic_flag,
										 int32 currPOC
										 )
{
	int32 i;
	int32 max_pic_num, curr_pic_num, pic_num_lx_no_wrap, pic_num_lx_pred, pic_num_lx;
	int32 ref_idx_lx = 0;
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	int8 picViewIdxLX, targetViewID;
	int8 maxViewIdx =0;
	int8 curr_VOIdx = -1;
	int8 picViewIdxLXPred=-1;
	int *abs_diff_view_idx_minus1 = g_curr_slice_ptr->abs_diff_view_idx_minus1[listNO];

	if(g_curr_slice_ptr->svc_extension_flag==0)
	{
		curr_VOIdx = view_id;
		maxViewIdx = get_maxViewIdx(view_id, anchor_pic_flag, 0);
		picViewIdxLXPred=-1;
	}

	max_pic_num = img_ptr->max_frame_num;
	curr_pic_num = img_ptr->frame_num;

	pic_num_lx_pred = curr_pic_num;

	for (i = 0; remapping_of_pic_nums_idc[i] != 3; i++)
	{
		if (remapping_of_pic_nums_idc[i]>5)
		{
			PRINTF ("Invalid remapping_of_pic_nums_idc command");
			img_ptr->error_flag = TRUE;
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
		}
		else if (remapping_of_pic_nums_idc[i] == 2) //(modification_of_pic_nums_idc[i] == 2)
		{
			H264Dec_reorder_long_term(picture_list_ptr, num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &ref_idx_lx);
		}
		else 
		{
			if(remapping_of_pic_nums_idc[i] == 4) //(modification_of_pic_nums_idc[i] == 4)
			{
				picViewIdxLX = picViewIdxLXPred - (abs_diff_view_idx_minus1[i] + 1);
				if( picViewIdxLX <0)
					picViewIdxLX += maxViewIdx;
			}
			else //(modification_of_pic_nums_idc[i] == 5)
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
				reorder_interview(g_list0, num_ref_idx_lX_active_minus1, &ref_idx_lx, targetViewID, currPOC, listNO);
			else
				reorder_interview(g_list1, num_ref_idx_lX_active_minus1, &ref_idx_lx, targetViewID, currPOC, listNO);
		}
	}

	return;
}
#endif
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

//xwluo@20110607
//the reference idx in list1 are mapped into list0, for MCA hardware module with only one reference frame list.
LOCAL void H264Dec_map_list1(void)
{
	int32 list0_size = g_list_size[0];
	int32 *map_ptr = g_list1_map_list0;
	int32 i, j;

	for(i = 0; i < g_list_size[1]; i++)
	{
		for(j = 0; j < list0_size; j++)
		{
			if (g_list1[i]->imgYAddr == g_list0[j]->imgYAddr)
			{
				break;
			}
		}

		//not found
		if (j == list0_size)
		{
			g_list0[list0_size] = g_list1[i];
			list0_size++;
		}	

		map_ptr[i] = j;
	}

	//set the remain with a invalid num
	for (i = g_list_size[1]; i < MAX_REF_FRAME_NUMBER; i++)
	{
		map_ptr[i] = 0x3f;//weihu0730
	}
			
	g_list_size[0] = list0_size;
}

PUBLIC void H264Dec_reorder_list (void)
{
	DEC_SLICE_T	*currSlice = g_curr_slice_ptr;

	int32 currSliceType = g_image_ptr->type;
	int i;
	
	if (currSliceType != I_SLICE)
	{
		if (g_curr_slice_ptr->ref_pic_list_reordering_flag_l0)
		{
			H264Dec_reorder_ref_pic_list (g_list0, g_image_ptr->num_ref_idx_l0_active-1,
				currSlice->remapping_of_pic_nums_idc_l0,
				currSlice->abs_diff_pic_num_minus1_l0,
				currSlice->long_term_pic_idx_l0);
			g_list_size[0] = g_image_ptr->num_ref_idx_l0_active;
		}

		if (currSliceType == B_SLICE)
		{
			if (g_curr_slice_ptr->ref_pic_list_reordering_flag_l1)
			{
				H264Dec_reorder_ref_pic_list (g_list1, g_image_ptr->num_ref_idx_l1_active-1,
					currSlice->remapping_of_pic_nums_idc_l1,
					currSlice->abs_diff_pic_num_minus1_l1,
					currSlice->long_term_pic_idx_l1);
				g_list_size[1] = g_image_ptr->num_ref_idx_l1_active;
			}
		}

		H264Dec_map_list1();

		if((g_list_size[0]==0)&&(g_list_size[1]==0))//for error // 不考虑对B/P slice 全Intra MB 情况支持
		{
			g_image_ptr->error_flag=TRUE;			
		}
		for(i = 0; i < g_list_size[0]; i++)
			g_list0_map_addr[i] = g_list0[i]->DPB_addr_index;
		for (i = g_list_size[0]; i < 32; i++)
		{
			g_list0_map_addr[i] = 0x3f;
		};//weihu
		for(i = 0; i < g_list_size[1]; i++)
			g_list1_map_addr[i] = g_list1[i]->DPB_addr_index;
		for (i = g_list_size[1]; i < MAX_REF_FRAME_NUMBER; i++)
		{
			g_list1_map_addr[i] = 0x3f;//weihu
		};//weihu
#if SIM_IN_WIN		
		MCAPRINTF (g_fp_reflist_adr_tv, "frame_cnt=%d,slice_type=%d\n",g_nFrame_dec_h264,g_image_ptr->type);	
		for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
			MCAPRINTF (g_fp_reflist_adr_tv, "%02x" , g_list0_map_addr[i]);
		MCAPRINTF (g_fp_reflist_adr_tv,"\n");
		for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
			MCAPRINTF (g_fp_reflist_adr_tv, "%02x", g_list1_map_addr[i]);
		MCAPRINTF (g_fp_reflist_adr_tv,"\n");
#endif
		
	}

	return;
}
#if _MVC_
PUBLIC void H264Dec_reorder_list_mvc (void)
{
	DEC_SLICE_T	*currSlice = g_curr_slice_ptr;
	int32 currPOC = g_image_ptr->framepoc;

	int32 currSliceType = g_image_ptr->type;
	int i;
	int8 listNO;
	
	if (currSliceType != I_SLICE)
	{
		if (g_curr_slice_ptr->ref_pic_list_reordering_flag_l0)
		{
			listNO = 0;
			H264Dec_reorder_ref_pic_list_mvc (g_list0, g_image_ptr->num_ref_idx_l0_active-1,
				currSlice->remapping_of_pic_nums_idc_l0,
				currSlice->abs_diff_pic_num_minus1_l0,
				currSlice->long_term_pic_idx_l0,
				listNO,
				g_active_subset_sps->anchor_ref_l0,
				g_active_subset_sps->non_anchor_ref_l0,
				g_curr_slice_ptr->view_id,
				g_curr_slice_ptr->anchor_pic_flag,
				currPOC
				);
			g_list_size[0] = g_image_ptr->num_ref_idx_l0_active;
		}

		if (currSliceType == B_SLICE)
		{
			if (g_curr_slice_ptr->ref_pic_list_reordering_flag_l1)
			{
				listNO = 1;
				H264Dec_reorder_ref_pic_list_mvc (g_list1, g_image_ptr->num_ref_idx_l1_active-1,
					currSlice->remapping_of_pic_nums_idc_l1,
					currSlice->abs_diff_pic_num_minus1_l1,
					currSlice->long_term_pic_idx_l1,
					listNO,
					g_active_subset_sps->anchor_ref_l1,
					g_active_subset_sps->non_anchor_ref_l1,
					g_curr_slice_ptr->view_id,
					g_curr_slice_ptr->anchor_pic_flag,
					currPOC
					);
				g_list_size[1] = g_image_ptr->num_ref_idx_l1_active;
			}
		}

		H264Dec_map_list1();

		if((g_list_size[0]==0)&&(g_list_size[1]==0))//for error // 不考虑对B/P slice 全Intra MB 情况支持
		{
			g_image_ptr->error_flag=TRUE;			
		}
		for(i = 0; i < g_list_size[0]; i++)
			g_list0_map_addr[i] = g_list0[i]->DPB_addr_index;
		for (i = g_list_size[0]; i < 32; i++)
		{
			g_list0_map_addr[i] = 0x3f;
		};//weihu
		for(i = 0; i < g_list_size[1]; i++)
			g_list1_map_addr[i] = g_list1[i]->DPB_addr_index;
		for (i = g_list_size[1]; i < MAX_REF_FRAME_NUMBER; i++)
		{
			g_list1_map_addr[i] = 0x3f;//weihu
		};//weihu
		
	}

	return;
}
#endif

#define SYS_QSORT	0//1//for or 

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
void append_interview_list(DEC_DECODED_PICTURE_BUFFER_T *p_Dpb, 
                           int8 list_idx, 
                           DEC_STORABLE_PICTURE_T **list,
                           int32 *listXsize, 
                           int32 currPOC, 
                           int curr_view_id, 
                           int8 anchor_pic_flag)
{
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
		num_ref_views = list_idx? g_active_subset_sps->num_anchor_refs_l1[iVOIdx] : g_active_subset_sps->num_anchor_refs_l0[iVOIdx];
		ref_view_id   = list_idx? g_active_subset_sps->anchor_ref_l1[iVOIdx] : g_active_subset_sps->anchor_ref_l0[iVOIdx];
	}
	else
	{
		num_ref_views = list_idx? g_active_subset_sps->num_non_anchor_refs_l1[iVOIdx] : g_active_subset_sps->num_non_anchor_refs_l0[iVOIdx];
		ref_view_id = list_idx? g_active_subset_sps->non_anchor_ref_l1[iVOIdx] : g_active_subset_sps->non_anchor_ref_l0[iVOIdx];
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
				list[*listXsize] = fs; 
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
PUBLIC void H264Dec_init_list (DEC_IMAGE_PARAMS_T *img_ptr, int32 curr_slice_type)
{
	int32 i;
	int32 list0idx = 0;
	int32 list0idx_1 = 0;
	int32 listltidx = 0;
#if _MVC_
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_curr_slice_ptr->p_Dpb;
#else
	DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = g_dpb_ptr;
#endif
	DEC_STORABLE_PICTURE_T **list = g_list0;
	int32 max_frame_num = (1<<(g_active_sps_ptr->log2_max_frame_num_minus4+4));
#if _MVC_
	g_curr_slice_ptr->listinterviewidx0 = 0;
	g_curr_slice_ptr->listinterviewidx1 = 0;
#endif

	for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
	{
		DEC_FRAME_STORE_T *fs_ref = dpb_ptr->fs_ref[i];
		
 		if ((fs_ref->frame->used_for_reference)&&(!fs_ref->frame->is_long_term))
		{
			if (fs_ref->frame_num > img_ptr->frame_num)
			{
				fs_ref->frame_num_wrap = fs_ref->frame_num - max_frame_num;
			}else
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
		g_list_size[0] = 0;
		g_list_size[1] = 0;
		return;
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
#if SIM_IN_WIN
				list[list0idx++]->mc_ref_pic_num = i;
#else
                list0idx++;
#endif
			}
		}
		// order list 0 by PicNum
#if SYS_QSORT	
		qsort((void*)list, list0idx, sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_pic_num_desc);
#else
		compare_pic_by_pic_num_desc(list, list0idx);
#endif
		g_list_size[0] = list0idx;

		//long term handling
		for(i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
		{
			if (dpb_ptr->fs_ltref[i]->frame->is_long_term)
			{
				dpb_ptr->fs_ltref[i]->frame->long_term_pic_num = dpb_ptr->fs_ltref[i]->frame->long_term_frame_idx;
				list[list0idx] = dpb_ptr->fs_ltref[i]->frame;
#if SIM_IN_WIN
				list[list0idx]->mc_ref_pic_num = list0idx+i;
#endif
				list0idx++;
			}
		}
#if SYS_QSORT	
		qsort((void*)&list[g_list_size[0]], list0idx-g_list_size[0], sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_lt_pic_num_asc);
#else
		compare_pic_by_lt_pic_num_asc(&list[g_list_size[0]], list0idx-g_list_size[0]);
#endif
#if _MVC_
		if (g_curr_slice_ptr->svc_extension_flag == 0 && g_curr_slice_ptr->view_id != 0)
		{        
			int curr_view_id = g_curr_slice_ptr->layer_id;
			//DEC_STORABLE_PICTURE_T *test;
			g_curr_slice_ptr->fs_listinterview0 = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*dpb_ptr->size);//weihu//calloc(dpb_ptr->size, sizeof (DEC_FRAME_STORE_T*));
				
			if (NULL==g_curr_slice_ptr->fs_listinterview0)
			{
				PRINTF("[mem alloc faile]init_lists: fs_listinterview0");
			}
			list0idx = g_list_size[0];
		
			append_interview_list(g_dpb_layer[1],
								0, 
								g_curr_slice_ptr->fs_listinterview0, 
								&g_curr_slice_ptr->listinterviewidx0, 
								img_ptr->framepoc,
								curr_view_id, 
								g_curr_slice_ptr->anchor_pic_flag);
			for (i=0; i<(unsigned int)g_curr_slice_ptr->listinterviewidx0; i++)
			{
				g_list0[list0idx++] = g_curr_slice_ptr->fs_listinterview0[i]->frame;
			}
			//currSlice->listXsize[0] = (char) list0idx;
		}
#endif
		g_list_size[0] = list0idx;
		g_list_size[1] = 0;

		for (i = g_list_size[0]; i < MAX_REF_FRAME_NUMBER+1; i++)
		{
			list[i] = g_no_reference_picture_ptr;
		}
	}else	//B-slice
	{
		for (i = 0; i < dpb_ptr->ref_frames_in_buffer; i++)
		{
			DEC_FRAME_STORE_T *fs_ref = dpb_ptr->fs_ref[i];
		
 			if ((fs_ref->frame->used_for_reference)&&(!fs_ref->frame->is_long_term))
			{
				if (img_ptr->framepoc > fs_ref->frame->poc)
				{
					list[list0idx] = dpb_ptr->fs_ref[i]->frame;
#if SIM_IN_WIN
					list[list0idx++]->mc_ref_pic_num = i;
#else
					list0idx++;
#endif
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
#if SIM_IN_WIN
					list[list0idx++]->mc_ref_pic_num = list0idx_1+i;
#else
                    list0idx++;
#endif
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
			g_list1[list0idx-list0idx_1+i] = g_list0[i];
		}
		for (i = list0idx_1; i < list0idx; i++)
		{
			g_list1[i-list0idx_1] = g_list0[i];
		}
		g_list_size[0] = g_list_size[1] = list0idx;

		//long term handling
		for (i = 0; i < dpb_ptr->ltref_frames_in_buffer; i++)
		{
			if (dpb_ptr->fs_ltref[i]->frame->is_long_term)
			{
				dpb_ptr->fs_ltref[i]->frame->long_term_pic_num = dpb_ptr->fs_ltref[i]->frame->long_term_frame_idx;

				g_list0[list0idx] = dpb_ptr->fs_ltref[i]->frame;
				g_list1[list0idx++] = dpb_ptr->fs_ltref[i]->frame;
			}
		}
#if SYS_QSORT	
		qsort((void *)&g_list0[g_list_size[0]], list0idx-g_list_size[0], sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_lt_pic_num_asc);
#else
		compare_pic_by_lt_pic_num_asc(&g_list0[g_list_size[0]], list0idx-g_list_size[0]);
#endif

#if SYS_QSORT
		qsort((void *)&g_list1[g_list_size[0]], list0idx-g_list_size[0], sizeof(DEC_STORABLE_PICTURE_T*), compare_pic_by_lt_pic_num_asc);		
#else
		compare_pic_by_lt_pic_num_asc(&g_list1[g_list_size[0]], list0idx-g_list_size[0]);
#endif
		g_list_size[0] = g_list_size[1] = list0idx;

#if _MVC_
		if (g_curr_slice_ptr->svc_extension_flag == 0)
		{
			int curr_view_id = g_curr_slice_ptr->view_id;
			// B-Slice
			g_curr_slice_ptr->fs_listinterview0 = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*dpb_ptr->size);//weihu//calloc(dpb_ptr->size, sizeof (DEC_FRAME_STORE_T*));
			//if (NULL==g_curr_slice_ptr->fs_listinterview0)
			//	no_mem_exit("init_lists: fs_listinterview0");
			g_curr_slice_ptr->fs_listinterview1 = (DEC_FRAME_STORE_T **)H264Dec_InterMemAlloc (sizeof(DEC_FRAME_STORE_T*)*dpb_ptr->size);//weihu// calloc(dpb_ptr->size, sizeof (DEC_FRAME_STORE_T*));
			//if (NULL==g_curr_slice_ptr->fs_listinterview1)
			//	no_mem_exit("init_lists: fs_listinterview1");
			list0idx = g_list_size[0];

			append_interview_list(g_dpb_layer[1],
								0, 
								g_curr_slice_ptr->fs_listinterview0, 
								&g_curr_slice_ptr->listinterviewidx0, 
								img_ptr->framepoc,
								curr_view_id, 
								g_curr_slice_ptr->anchor_pic_flag);

			append_interview_list(g_dpb_layer[1], 0, g_curr_slice_ptr->fs_listinterview0, &g_curr_slice_ptr->listinterviewidx0, 
									img_ptr->framepoc, curr_view_id, g_curr_slice_ptr->anchor_pic_flag);
			append_interview_list(g_dpb_layer[1], 1, g_curr_slice_ptr->fs_listinterview1, &g_curr_slice_ptr->listinterviewidx1, 
									img_ptr->framepoc, curr_view_id, g_curr_slice_ptr->anchor_pic_flag);
			for (i=0; i<(unsigned int)g_curr_slice_ptr->listinterviewidx0; i++)
			{
				g_list0[list0idx++]=g_curr_slice_ptr->fs_listinterview0[i]->frame;
			}
			g_list_size[0] = (char) list0idx;
			list0idx = g_list_size[1];
			for (i=0; i<(unsigned int)g_curr_slice_ptr->listinterviewidx1; i++)
			{
				g_list1[list0idx++] = g_curr_slice_ptr->fs_listinterview1[i]->frame;
			}
			g_list_size[1] = (char) list0idx;  
		}
#endif
	}

	if ((g_list_size[0] == g_list_size[1]) && (g_list_size[0] > 1))
	{
		 // check if lists are identical, if yes swap first two elements of listX[1]	//???
		 int32 diff = 0;
		 for (i = 0; i < g_list_size[0]; i++)
		 {
		 	if (g_list0[i] != g_list1[i])
		 	{
		 		diff = 1;
		 	}
		 }
		 if (!diff)
		 {
		 	DEC_STORABLE_PICTURE_T *tmp_s;
		 	tmp_s = g_list1[0];
		 	g_list1[0] = g_list1[1];
		 	g_list1[1] = tmp_s;
		 }
	}
	//set max size
	g_list_size[0] = mmin(g_list_size[0], img_ptr->num_ref_idx_l0_active);
	g_list_size[1] = mmin(g_list_size[1], img_ptr->num_ref_idx_l1_active);


	//set the unsed list entries to NULL
	for (i = g_list_size[0]; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		g_list0[i] = g_no_reference_picture_ptr;
	}

	for (i = g_list_size[1]; i < MAX_REF_FRAME_NUMBER+1; i++)
	{
		g_list1[i] = g_no_reference_picture_ptr;
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





















