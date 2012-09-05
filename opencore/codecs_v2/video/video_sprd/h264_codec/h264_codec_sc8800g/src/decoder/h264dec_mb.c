/******************************************************************************
 ** File Name:    h264dec_mb.c			                                      *
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

PUBLIC void H264Dec_get_mb_avail (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_x = img_ptr->mb_x;
	int32 mb_y = img_ptr->mb_y;
	int32 frame_width_in_mbs = img_ptr->frame_width_in_mbs;
	int32 curr_mb_slice_nr = mb_info_ptr->slice_nr;	

	mb_cache_ptr->mb_avail_a = ((mb_x > 0) && (curr_mb_slice_nr == (mb_info_ptr - 1)->slice_nr)); //left
	mb_cache_ptr->mb_avail_b = ((mb_y > 0) && (curr_mb_slice_nr == (mb_info_ptr - frame_width_in_mbs)->slice_nr)); //top
	mb_cache_ptr->mb_avail_c = ((mb_y > 0) && 
		(mb_x < (img_ptr->frame_width_in_mbs -1)) && (curr_mb_slice_nr == (mb_info_ptr - frame_width_in_mbs+1)->slice_nr)); //top right
	mb_cache_ptr->mb_avail_d = ((mb_x > 0) && 
		(mb_y > 0) && (curr_mb_slice_nr == (mb_info_ptr - frame_width_in_mbs-1)->slice_nr)); //top left

	return;
}

PUBLIC void H264Dec_start_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->slice_nr = img_ptr->slice_nr;
	mb_info_ptr->is_intra = FALSE;
	mb_info_ptr->is_skipped = FALSE;
	mb_info_ptr->qp = img_ptr->qp;
	((int32 *)(mb_info_ptr->ref_pic))[0] = (int32)0xffffffff;
	
	mb_info_ptr->LFDisableIdc = img_ptr->curr_slice_ptr->LFDisableIdc;
	mb_info_ptr->LFAlphaC0Offset = img_ptr->curr_slice_ptr->LFAlphaC0Offset;
	mb_info_ptr->LFBetaOffset = img_ptr->curr_slice_ptr->LFBetaOffset;

	mb_cache_ptr->cbp = 0;
	mb_cache_ptr->cbp_uv = 0;
	
 	H264Dec_get_mb_avail(img_ptr, mb_info_ptr, mb_cache_ptr);

	VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, (img_ptr->mb_y << 8) | (img_ptr->mb_x << 0), "vsp global reg: configure current MB position");

	return;
}

LOCAL void H264Dec_interpret_mb_mode_I (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_mode = mb_info_ptr->mb_type;

	if (mb_mode == 0)
	{
		mb_info_ptr->mb_type = I4MB_264;
	}else if (mb_mode == 25)
	{
		mb_info_ptr->mb_type = IPCM;
	}else
	{
		mb_info_ptr->mb_type = I16MB;
		mb_cache_ptr->cbp = g_ICBP_TBL[(mb_mode-1)>>2];
		mb_cache_ptr->i16mode = ((mb_mode-1)&0x3);
	}

	return;
}

LOCAL void H264Dec_interpret_mb_mode_P (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_mode = mb_info_ptr->mb_type;

	if (mb_mode < 4)
	{
		mb_info_ptr->mb_type = mb_mode;
		mb_cache_ptr->all_zero_ref = 0;
	}else if ((mb_mode == 4) || (mb_mode == 5))
	{
		mb_info_ptr->mb_type = P8X8;
		mb_cache_ptr->all_zero_ref = ((mb_mode == 5) ? 1 : 0);
	}else if (mb_mode == 6)
	{
		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->mb_type = I4MB_264;
	}else if (mb_mode == 31)
	{
		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->mb_type = IPCM;
	}else
	{
		mb_mode -= 7;
		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->mb_type = I16MB;
		mb_cache_ptr->cbp = g_ICBP_TBL[mb_mode>>2];
		mb_cache_ptr->i16mode = (mb_mode&0x3);
	}

	return;
}

LOCAL void H264Dec_fill_intraMB_context (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 top_mb_avail, left_mb_avail;
	DEC_MB_INFO_T *left_mb_info_ptr, *top_mb_info_ptr;
	int8 *i4_pred_ref_dst_ptr, *i4_pred_ref_ptr;
	int8 *nnz_ref_dst_ptr, *nnz_ref_ptr;
	int32 is_constained_intra = img_ptr->constrained_intra_pred_flag;
	int32 i;

	i4_pred_ref_dst_ptr = mb_info_ptr->i4x4pred_mode_ref;

	//top ipred mde reference
	top_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs;
	top_mb_avail = mb_cache_ptr->mb_avail_b;

	if (top_mb_avail && top_mb_info_ptr->is_intra)
	{
		i4_pred_ref_ptr = top_mb_info_ptr->i4x4pred_mode_ref+4*CONTEXT_CACHE_WIDTH;

		((int32 *)i4_pred_ref_dst_ptr)[0] = ((int32 *)i4_pred_ref_ptr)[0];
		i4_pred_ref_dst_ptr[4] = i4_pred_ref_ptr[4];
	}else
	{
		if (top_mb_avail && !is_constained_intra)
		{
			((int32 *)i4_pred_ref_dst_ptr)[0] = 0x02020202;
			i4_pred_ref_dst_ptr[4] = 0x02;
		}else
		{
			((int32 *)i4_pred_ref_dst_ptr)[0] = (int32)0xfefefefe;
			i4_pred_ref_dst_ptr[4] = -2;
		}
	}

	//left ipred mode reference
	left_mb_info_ptr = mb_info_ptr-1;
	left_mb_avail = mb_cache_ptr->mb_avail_a;

	if (left_mb_avail && left_mb_info_ptr->is_intra)
	{
		i4_pred_ref_ptr = left_mb_info_ptr->i4x4pred_mode_ref;
		i4_pred_ref_dst_ptr[1*CONTEXT_CACHE_WIDTH] = i4_pred_ref_ptr[1*CONTEXT_CACHE_WIDTH+4];
		i4_pred_ref_dst_ptr[2*CONTEXT_CACHE_WIDTH] = i4_pred_ref_ptr[2*CONTEXT_CACHE_WIDTH+4];
		i4_pred_ref_dst_ptr[3*CONTEXT_CACHE_WIDTH] = i4_pred_ref_ptr[3*CONTEXT_CACHE_WIDTH+4];
		i4_pred_ref_dst_ptr[4*CONTEXT_CACHE_WIDTH] = i4_pred_ref_ptr[4*CONTEXT_CACHE_WIDTH+4];
	}else
	{
		int8 i4Pred = -2;

		if (left_mb_avail && !is_constained_intra)
		{
			i4Pred = 2;
		}

		i4_pred_ref_dst_ptr[1*CONTEXT_CACHE_WIDTH] = i4Pred;
		i4_pred_ref_dst_ptr[2*CONTEXT_CACHE_WIDTH] = i4Pred;
		i4_pred_ref_dst_ptr[3*CONTEXT_CACHE_WIDTH] = i4Pred;
		i4_pred_ref_dst_ptr[4*CONTEXT_CACHE_WIDTH] = i4Pred;
	}

	//top nnz reference
	if (top_mb_avail)
	{
		nnz_ref_ptr = top_mb_info_ptr->nnz_ref;
		nnz_ref_dst_ptr = mb_info_ptr->nnz_ref;

		//y
		((int32*)nnz_ref_dst_ptr)[0] = ((int32*)nnz_ref_ptr)[6];
		nnz_ref_dst_ptr[4] = nnz_ref_ptr[4*CONTEXT_CACHE_WIDTH+4];

		//cb, cr
		nnz_ref_dst_ptr[5*CONTEXT_CACHE_WIDTH+1] = nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH+1];
		((int32*)nnz_ref_dst_ptr)[8] = ((int32*)nnz_ref_ptr)[11];
	}else
	{
		nnz_ref_dst_ptr = mb_info_ptr->nnz_ref;

		//y
		((int32*)nnz_ref_dst_ptr)[0] = 0;
		nnz_ref_dst_ptr[4] = 0;

		//cb, cr
		nnz_ref_dst_ptr[5*CONTEXT_CACHE_WIDTH+1] = 0;
		((int32*)nnz_ref_dst_ptr)[8] = 0;
	}
	
	//left nnz ref
	if (left_mb_avail) //baseline do not consider constrainedIntra
	{
		nnz_ref_ptr = left_mb_info_ptr->nnz_ref;
		nnz_ref_dst_ptr = mb_info_ptr->nnz_ref;

		//y
		nnz_ref_dst_ptr[1*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[1*CONTEXT_CACHE_WIDTH+4];
		nnz_ref_dst_ptr[2*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[2*CONTEXT_CACHE_WIDTH+4];
		nnz_ref_dst_ptr[3*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[3*CONTEXT_CACHE_WIDTH+4];
		nnz_ref_dst_ptr[4*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[4*CONTEXT_CACHE_WIDTH+4];

		//cb
		nnz_ref_dst_ptr[6*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[6*CONTEXT_CACHE_WIDTH+2];
		nnz_ref_dst_ptr[7*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH+2];
		//cr
		nnz_ref_dst_ptr[6*CONTEXT_CACHE_WIDTH+3] = nnz_ref_ptr[6*CONTEXT_CACHE_WIDTH+5];
		nnz_ref_dst_ptr[7*CONTEXT_CACHE_WIDTH+3] = nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH+5];
	}else
	{
		nnz_ref_dst_ptr = mb_info_ptr->nnz_ref;

		//y
		nnz_ref_dst_ptr[1*CONTEXT_CACHE_WIDTH] = 0;
		nnz_ref_dst_ptr[2*CONTEXT_CACHE_WIDTH] = 0;
		nnz_ref_dst_ptr[3*CONTEXT_CACHE_WIDTH] = 0;
		nnz_ref_dst_ptr[4*CONTEXT_CACHE_WIDTH] = 0;

		//cb
		nnz_ref_dst_ptr[6*CONTEXT_CACHE_WIDTH] = 0;
		nnz_ref_dst_ptr[7*CONTEXT_CACHE_WIDTH] = 0;
		//cr
		nnz_ref_dst_ptr[6*CONTEXT_CACHE_WIDTH+3] = 0;
		nnz_ref_dst_ptr[7*CONTEXT_CACHE_WIDTH+3] = 0;
	}

	//y
	for (i = 1; i < 5; i++)
	{
		nnz_ref_dst_ptr = mb_info_ptr->nnz_ref+(i*CONTEXT_CACHE_WIDTH);
		nnz_ref_dst_ptr[1] = 0;
		nnz_ref_dst_ptr[2] = 0;
		nnz_ref_dst_ptr[3] = 0;
		nnz_ref_dst_ptr[4] = 0;
	}
	//u
	nnz_ref_dst_ptr = mb_info_ptr->nnz_ref+(6*CONTEXT_CACHE_WIDTH);
	nnz_ref_dst_ptr[1] = 0;
	nnz_ref_dst_ptr[2] = 0;
	nnz_ref_dst_ptr[4] = 0;
	nnz_ref_dst_ptr[5] = 0;

	//v
	nnz_ref_dst_ptr = mb_info_ptr->nnz_ref+(7*CONTEXT_CACHE_WIDTH);
	nnz_ref_dst_ptr[1] = 0;
	nnz_ref_dst_ptr[2] = 0;
	nnz_ref_dst_ptr[4] = 0;
	nnz_ref_dst_ptr[5] = 0;

	return;	
}

LOCAL void H264Dec_fill_interMB_context (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 left_mb_avail, top_mb_avail, top_right_mb_avail, top_left_mb_avail;
	DEC_MB_INFO_T *left_mb_info_ptr, *top_mb_info_ptr, *top_right_mb_info_ptr;
	int8 *ref_idx_ptr, *ref_idx_dst_ptr;
	int16 *ref_mv_ptr, *ref_mv_dst_ptr;
	int8 *nnz_ref_dst_ptr, *nnz_ref_ptr;
//	int32 is_constained_intra = img_ptr->constrained_intra_pred_flag;
	int32 mb_x = img_ptr->mb_x;

	ref_idx_dst_ptr = mb_info_ptr->ref_idx_cache;
	ref_mv_dst_ptr = mb_info_ptr->mv_cache;
	nnz_ref_dst_ptr = mb_info_ptr->nnz_ref;

	memset(ref_idx_dst_ptr, -2, sizeof(int8)*(6*5+2));
	memset(nnz_ref_dst_ptr, 0, sizeof(int8)*(6*8));
	memset(ref_mv_dst_ptr, 0,  sizeof(int16)*(6*5*2));

	//fill mb_cache->
	ref_idx_ptr = mb_info_ptr->ref_idx_cache+CONTEXT_CACHE_WIDTH+1;
	ref_idx_ptr[0] = ref_idx_ptr[1] = ref_idx_ptr[2] = ref_idx_ptr[3] = 0; ref_idx_ptr += CONTEXT_CACHE_WIDTH;
	ref_idx_ptr[0] = ref_idx_ptr[1] = ref_idx_ptr[2] = ref_idx_ptr[3] = 0; ref_idx_ptr += CONTEXT_CACHE_WIDTH;
	ref_idx_ptr[0] = ref_idx_ptr[1] = ref_idx_ptr[2] = ref_idx_ptr[3] = 0; ref_idx_ptr += CONTEXT_CACHE_WIDTH;
	ref_idx_ptr[0] = ref_idx_ptr[1] = ref_idx_ptr[2] = ref_idx_ptr[3] = 0; 

	//left mv and reference frame index ref
	left_mb_info_ptr = mb_info_ptr-1;
	left_mb_avail = mb_cache_ptr->mb_avail_a;

	//left nnz ref
	nnz_ref_ptr = left_mb_info_ptr->nnz_ref;
	nnz_ref_dst_ptr[1*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[1*CONTEXT_CACHE_WIDTH+4];
	nnz_ref_dst_ptr[2*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[2*CONTEXT_CACHE_WIDTH+4];
	nnz_ref_dst_ptr[3*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[3*CONTEXT_CACHE_WIDTH+4];
	nnz_ref_dst_ptr[4*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[4*CONTEXT_CACHE_WIDTH+4];
	nnz_ref_dst_ptr[6*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[6*CONTEXT_CACHE_WIDTH+2];
	nnz_ref_dst_ptr[7*CONTEXT_CACHE_WIDTH] = nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH+2];
	nnz_ref_dst_ptr[6*CONTEXT_CACHE_WIDTH+3] = nnz_ref_ptr[6*CONTEXT_CACHE_WIDTH+5];
	nnz_ref_dst_ptr[7*CONTEXT_CACHE_WIDTH+3] = nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH+5];

	if (left_mb_avail)
	{
		//left reference frame index
	// 	if (!left_mb_info_ptr->is_skipped)
		{
			ref_idx_ptr = left_mb_info_ptr->ref_idx_cache+4;

			ref_idx_dst_ptr[1*CONTEXT_CACHE_WIDTH] = ref_idx_ptr[1*CONTEXT_CACHE_WIDTH];
			ref_idx_dst_ptr[2*CONTEXT_CACHE_WIDTH] = ref_idx_ptr[2*CONTEXT_CACHE_WIDTH];
			ref_idx_dst_ptr[3*CONTEXT_CACHE_WIDTH] = ref_idx_ptr[3*CONTEXT_CACHE_WIDTH];
			ref_idx_dst_ptr[4*CONTEXT_CACHE_WIDTH] = ref_idx_ptr[4*CONTEXT_CACHE_WIDTH];	
		}

		//left mv ref
		ref_mv_ptr = left_mb_info_ptr->mv_cache+4*2;

		((int32 *)ref_mv_dst_ptr)[1*CONTEXT_CACHE_WIDTH] = ((int32 *)ref_mv_ptr)[1*CONTEXT_CACHE_WIDTH];
		((int32 *)ref_mv_dst_ptr)[2*CONTEXT_CACHE_WIDTH] = ((int32 *)ref_mv_ptr)[2*CONTEXT_CACHE_WIDTH];
		((int32 *)ref_mv_dst_ptr)[3*CONTEXT_CACHE_WIDTH] = ((int32 *)ref_mv_ptr)[3*CONTEXT_CACHE_WIDTH];
		((int32 *)ref_mv_dst_ptr)[4*CONTEXT_CACHE_WIDTH] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH];
	}else
	{
		if (mb_x > 0)
		{
			ref_mv_ptr = left_mb_info_ptr->mv_cache+4*2;
			((int32 *)ref_mv_dst_ptr)[1*CONTEXT_CACHE_WIDTH+5] = ((int32 *)ref_mv_ptr)[1*CONTEXT_CACHE_WIDTH];
			((int32 *)ref_mv_dst_ptr)[2*CONTEXT_CACHE_WIDTH+5] = ((int32 *)ref_mv_ptr)[2*CONTEXT_CACHE_WIDTH];
			((int32 *)ref_mv_dst_ptr)[3*CONTEXT_CACHE_WIDTH+5] = ((int32 *)ref_mv_ptr)[3*CONTEXT_CACHE_WIDTH];
			((int32 *)ref_mv_dst_ptr)[4*CONTEXT_CACHE_WIDTH+5] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH];
		}		
	}

	//top left or top right mv ref and reference frame index
	//top right
	top_right_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs+1;
	top_right_mb_avail = mb_cache_ptr->mb_avail_c;

	if (top_right_mb_avail)
	{
		ref_idx_ptr= top_right_mb_info_ptr->ref_idx_cache;
		ref_idx_dst_ptr[5] = ref_idx_ptr[4*CONTEXT_CACHE_WIDTH+1];
		ref_mv_ptr = top_right_mb_info_ptr->mv_cache;
		((int32 *)ref_mv_dst_ptr)[5] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH+1];
	}else
	{
		//set top right is not available
		ref_idx_dst_ptr[5] = -2;
		((int32 *)ref_mv_dst_ptr)[5] = 0;
	}

	//init top left reference
	top_left_mb_avail = mb_cache_ptr->mb_avail_d;

	if (top_left_mb_avail)
	{
		DEC_MB_INFO_T *top_left_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs-1;

		ref_idx_ptr = top_left_mb_info_ptr->ref_idx_cache;
		ref_idx_dst_ptr[0] = ref_idx_ptr[4*CONTEXT_CACHE_WIDTH+4];

		ref_mv_ptr = top_left_mb_info_ptr->mv_cache;
		((int32 *)ref_mv_dst_ptr)[0] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH+4];
	}

	//top mv and reference frame index ref
	top_mb_info_ptr = mb_info_ptr - img_ptr->frame_width_in_mbs;
	top_mb_avail = mb_cache_ptr->mb_avail_b;

	if (top_mb_avail)
	{
		//fill top reference frame index ref and nnz ref
		//nnz ref
		nnz_ref_ptr = top_mb_info_ptr->nnz_ref;
		((int32*)nnz_ref_dst_ptr)[0] = ((int32*)nnz_ref_ptr)[6];
		nnz_ref_dst_ptr[4] = nnz_ref_ptr[28];
		nnz_ref_dst_ptr[31] = nnz_ref_ptr[43];
		((int32*)nnz_ref_dst_ptr)[8] = ((int32*)nnz_ref_ptr)[11];

		//ref index
		ref_idx_ptr = top_mb_info_ptr->ref_idx_cache;
		ref_idx_dst_ptr[1] = ref_idx_ptr[25];
		ref_idx_dst_ptr[2] = ref_idx_ptr[26];
		ref_idx_dst_ptr[3] = ref_idx_ptr[27];
		ref_idx_dst_ptr[4] = ref_idx_ptr[28];
		
		//fill top mv ref
		ref_mv_ptr = top_mb_info_ptr->mv_cache;
		((int32 *)ref_mv_dst_ptr)[1] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH+1];
		((int32 *)ref_mv_dst_ptr)[2] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH+2];
		((int32 *)ref_mv_dst_ptr)[3] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH+3];
		((int32 *)ref_mv_dst_ptr)[4] = ((int32 *)ref_mv_ptr)[4*CONTEXT_CACHE_WIDTH+4];
	}

	return;
}

PUBLIC void H264Dec_fill_mv_and_ref (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8  *ref_idx_ptr, *ref_idx_dst_ptr;
	int16 *ref_mv_ptr, *ref_mv_dst_ptr;
//	int32 mb_x = img_ptr->mb_x;
	int32 i;

	ref_idx_dst_ptr = mb_info_ptr->ref_idx_cache;
	ref_mv_dst_ptr = mb_info_ptr->mv_cache;

	if (img_ptr->mb_y)
	{
 		ref_idx_ptr = (mb_info_ptr-img_ptr->frame_width_in_mbs)->ref_idx_cache+1;
		ref_idx_dst_ptr[1] = ref_idx_ptr[0];	ref_idx_dst_ptr[2] = ref_idx_ptr[1];
		ref_idx_dst_ptr[3] = ref_idx_ptr[2];	ref_idx_dst_ptr[4] = ref_idx_ptr[3];
		
		ref_mv_ptr = (mb_info_ptr-img_ptr->frame_width_in_mbs)->mv_cache+1*2;
		((int32 *)(ref_mv_dst_ptr))[1] = ((int32 *)ref_mv_ptr)[0];
		((int32 *)(ref_mv_dst_ptr))[2] = ((int32 *)ref_mv_ptr)[1];
		((int32 *)(ref_mv_dst_ptr))[3] = ((int32 *)ref_mv_ptr)[2];
		((int32 *)(ref_mv_dst_ptr))[4] = ((int32 *)ref_mv_ptr)[3];
	}

	for (i = 1; i <= 4; i++)
	{
		ref_idx_dst_ptr += CONTEXT_CACHE_WIDTH;
		ref_idx_dst_ptr[1] = -1;	ref_idx_dst_ptr[2] = -1;
		ref_idx_dst_ptr[3] = -1;	ref_idx_dst_ptr[4] = -1;

		ref_mv_dst_ptr += (2*CONTEXT_CACHE_WIDTH);
		((int32 *)(ref_mv_dst_ptr))[1] = 0x0;
		((int32 *)(ref_mv_dst_ptr))[2] = 0x0;
		((int32 *)(ref_mv_dst_ptr))[3] = 0x0;
		((int32 *)(ref_mv_dst_ptr))[4] = 0x0;
	}

	return;
}

LOCAL void H264Dec_read_and_derive_ipred_modes(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 blk4x4Idx;
	int32 val;

	//get 16 block4x4 intra prediction mode, luma
	if (mb_info_ptr->mb_type == I4MB_264)
	{
		if ((g_curr_slice_ptr->picture_type != I_SLICE) || (!g_pre_mb_is_intra4))
		{
			int32 blk4x4StrIdx;
			int32 pred_mode;
			int32 up_ipred_mode;
			int32 left_ipred_mode;
			int32 most_probable_ipred_mode;
			int8 *i4_pred_mode_ref_ptr = mb_info_ptr->i4x4pred_mode_ref;
			uint8 *blk_order_map_tbl_ptr = g_blk_order_map_tbl;

			for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
			{
				READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
					"polling bsm fifo depth >= 8 words for h264 header");

				val = SHOW_FLC(4);
				if(val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!, noted by xwluo@20100618
				{
					pred_mode = -1;
					READ_FLC(1);
				}else
				{
					pred_mode = val & 0x7; //remaining_mode_selector
					READ_FLC(4);
				}
				
				blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
				left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
				up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-CONTEXT_CACHE_WIDTH];
				most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

				if (most_probable_ipred_mode < 0)
				{
					most_probable_ipred_mode = DC_PRED;
				}

				pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + ((pred_mode >= most_probable_ipred_mode) ? 1 : 0) ));
				//must! because BITSTRM may be error
				if ((pred_mode >9) || (pred_mode < 0))
				{
					img_ptr->error_flag |= ER_BSM_ID;
                    img_ptr->return_pos |= (1<<21);
                    H264Dec_get_HW_status(img_ptr);    
					return;
				}

				i4_pred_mode_ref_ptr[blk4x4StrIdx] = pred_mode;
			}
		}
	}else
	{
		int32 *int32_i4x4_pred_mode_ref_ptr = (int32 *)(mb_info_ptr->i4x4pred_mode_ref);

		int32_i4x4_pred_mode_ref_ptr[2] = 0x02020202;
		int32_i4x4_pred_mode_ref_ptr[4] = 0x02020202;
		int32_i4x4_pred_mode_ref_ptr[5] = 0x02020202;
		int32_i4x4_pred_mode_ref_ptr[6] = 0x02020202;
		int32_i4x4_pred_mode_ref_ptr[7] = 0x02020202;
	}

	if ((g_curr_slice_ptr->picture_type != I_SLICE) || (!g_pre_mb_is_intra4))
	{
		//get chroma intra prediction mode
		mb_cache_ptr->c_ipred_mode = READ_UE_V();
	}

	return;
}

LOCAL void H264Dec_read_CBPandDeltaQp(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 val;

	if (mb_info_ptr->mb_type != I16MB)
	{
		val = READ_UE_V();
		if(mb_info_ptr->is_intra)
		{
			mb_cache_ptr->cbp = g_cbp_intra_tbl[val];
		}else
		{
			mb_cache_ptr->cbp = g_cbp_inter_tbl[val];
		}
	}

	if ((mb_cache_ptr->cbp > 0) || (mb_info_ptr->mb_type == I16MB))
	{
		int32 delta_q;
		delta_q = READ_SE_V();
		img_ptr->qp = mb_info_ptr->qp = (img_ptr->qp+delta_q+52)%52;
	}

	return;
}

PUBLIC void H264Dec_read_one_macroblock_ISlice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->is_intra = TRUE;

	if (g_pre_mb_is_intra4)
	{
		mb_info_ptr->mb_type = 0;
	}else
	{
		mb_info_ptr->mb_type = READ_UE_V();
	}

	H264Dec_interpret_mb_mode_I (mb_info_ptr, mb_cache_ptr);

	H264Dec_fill_intraMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);
	
	if (mb_info_ptr->mb_type != IPCM)
	{
		H264Dec_read_and_derive_ipred_modes (img_ptr, mb_info_ptr, mb_cache_ptr);
		H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		int8 *i4_pred_ptr = mb_info_ptr->i4x4pred_mode_ref+CONTEXT_CACHE_WIDTH+1;
		
		i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02;	i4_pred_ptr += CONTEXT_CACHE_WIDTH;
		i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02;	i4_pred_ptr += CONTEXT_CACHE_WIDTH;
		i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02;	i4_pred_ptr += CONTEXT_CACHE_WIDTH;
		i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02;	
	
		mb_info_ptr->qp = 0;
	}

	mb_info_ptr->qp_c = g_QP_SCALER_CR_TBL[IClip(0, 51, mb_info_ptr->qp+img_ptr->chroma_qp_offset)];

	return;
}

PUBLIC void H264Dec_read_one_macroblock_PSlice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 val;

	//cod counter
	if (img_ptr->cod_counter == -1)
	{
		img_ptr->cod_counter = READ_UE_V();
	}

	if (img_ptr->cod_counter == 0)
	{
		val = READ_UE_V();
		mb_info_ptr->mb_type = val+1;
	}else
	{
		mb_info_ptr->is_skipped = TRUE;
		mb_info_ptr->mb_type = 0;
	}
	img_ptr->cod_counter--;

	H264Dec_interpret_mb_mode_P(mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_fill_intraMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);

		if (mb_info_ptr->mb_type != IPCM)
		{
			H264Dec_read_and_derive_ipred_modes (img_ptr, mb_info_ptr, mb_cache_ptr);
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			int8 *i4_pred_ptr = mb_info_ptr->i4x4pred_mode_ref + CONTEXT_CACHE_WIDTH + 1;

			mb_info_ptr->qp = 0;
			i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02; i4_pred_ptr += CONTEXT_CACHE_WIDTH;
			i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02; i4_pred_ptr += CONTEXT_CACHE_WIDTH;
			i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02; i4_pred_ptr += CONTEXT_CACHE_WIDTH;
			i4_pred_ptr[0] = 0x02;	i4_pred_ptr[1] = 0x02;	i4_pred_ptr[2] = 0x02;	i4_pred_ptr[3] = 0x02; 
		}
	}else
	{
		int32 ref_pic_num;

		if (g_list0[0] == NULL)
		{
			img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos |= (1<<22);
            H264Dec_get_HW_status(img_ptr);        
			return;
		}

		ref_pic_num = g_list0[0]->mc_ref_pic_num;
		((int32 *)(mb_info_ptr->ref_pic))[0] = ref_pic_num * 0x01010101;//ref_pic_num | (ref_pic_num<<8) | (ref_pic_num<<16) | (ref_pic_num<<24);

		H264Dec_fill_interMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);

		if (!mb_info_ptr->is_skipped)
		{
			if (mb_info_ptr->mb_type == P8x8)
			{
				int32 sub_mb_mode;
				int32 blk8x8Nr;

				for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
				{
					sub_mb_mode = READ_UE_V();
                    if((sub_mb_mode < 0) || (sub_mb_mode >= 4))
                    {
                        g_image_ptr->error_flag |= ER_BSM_ID;
                        img_ptr->return_pos |= (1<<23);
                        H264Dec_get_HW_status(img_ptr);      
                        return;
                    }
					mb_cache_ptr->b8mode[blk8x8Nr] = sub_mb_mode + 4;
				}
			}

			H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		}
	}

	mb_info_ptr->qp_c = g_QP_SCALER_CR_TBL[IClip(0, 51, mb_info_ptr->qp+img_ptr->chroma_qp_offset)];
	
	return;
}

LOCAL void H264Dec_read_nnz_from_vld (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 tc0, tc1, tc2, tc3, tc4, tc5;
	int8* nnz_ref_ptr = mb_info_ptr->nnz_ref;

	mb_cache_ptr->cbp_uv = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_CBP_UV_OFFSET, "read out cbp_uv");

	//read back the tc from vld
	tc0 = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_NNZ_BLK_0_OFFSET, "read out block0 nnz of current MB from vld");
	tc1 = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_NNZ_BLK_1_OFFSET, "read out block1 nnz of current MB from vld");
	tc2 = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_NNZ_BLK_2_OFFSET, "read out block2 nnz of current MB from vld");
	tc3 = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_NNZ_BLK_3_OFFSET, "read out block3 nnz of current MB from vld");
	tc4 = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_NNZ_BLK_4_OFFSET, "read out block4 nnz of current MB from vld");
	tc5 = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_NNZ_BLK_5_OFFSET, "read out block5 nnz of current MB from vld");

	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*1 + 1] = (tc0 >> 24) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*1 + 2] = (tc0 >> 16) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*2 + 1] = (tc0 >> 8 ) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*2 + 2] = (tc0 >> 0 ) & 0xff;

	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*1 + 3] = (tc1 >> 24) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*1 + 4] = (tc1 >> 16) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*2 + 3] = (tc1 >> 8 ) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*2 + 4] = (tc1 >> 0 ) & 0xff;

	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*3 + 1] = (tc2 >> 24) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*3 + 2] = (tc2 >> 16) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*4 + 1] = (tc2 >> 8 ) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*4 + 2] = (tc2 >> 0 ) & 0xff;

	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*3 + 3] = (tc3 >> 24) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*3 + 4] = (tc3 >> 16) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*4 + 3] = (tc3 >> 8 ) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*4 + 4] = (tc3 >> 0 ) & 0xff;

	//cb
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*6 + 1] = (tc4 >> 24) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*6 + 2] = (tc4 >> 16) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*7 + 1] = (tc4 >> 8 ) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*7 + 2] = (tc4 >> 0 ) & 0xff;
	
	//cr
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*6 + 4] = (tc5 >> 24) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*6 + 5] = (tc5 >> 16) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*7 + 4] = (tc5 >> 8 ) & 0xff;
	nnz_ref_ptr [CONTEXT_CACHE_WIDTH*7 + 5] = (tc5 >> 0 ) & 0xff;

	return;
}

PUBLIC void H264Dec_start_vld_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 up_nnz_y, left_nnz_y;
	uint32 nnz_cb, nnz_cr;
	int32 vsp_mb_type, mb_type = mb_info_ptr->mb_type;
	int32 left_mb_avail, top_mb_avail;
	int32 cbp = mb_cache_ptr->cbp;
	int8 *nnz_ref_ptr = mb_info_ptr->nnz_ref;
	uint32 cmd;

	vsp_mb_type	= (mb_type == IPCM) ? H264_IPCM : ((mb_type == I16MB) ? H264_IMB16X16 : H264_OTHER_TYPE);	

	up_nnz_y	= (nnz_ref_ptr[1]<<24) | (nnz_ref_ptr[2]<<16) | (nnz_ref_ptr[3]<<8) | (nnz_ref_ptr[4]);
	left_nnz_y	= (nnz_ref_ptr[  CONTEXT_CACHE_WIDTH] << 24) | (nnz_ref_ptr[2*CONTEXT_CACHE_WIDTH]<<16) |
			      (nnz_ref_ptr[3*CONTEXT_CACHE_WIDTH] << 8 ) | (nnz_ref_ptr[4*CONTEXT_CACHE_WIDTH]);
	nnz_cb		= (nnz_ref_ptr[5*CONTEXT_CACHE_WIDTH+1] << 24) | (nnz_ref_ptr[5*CONTEXT_CACHE_WIDTH+2]<<16)|
				  (nnz_ref_ptr[6*CONTEXT_CACHE_WIDTH  ] << 8)  | (nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH]);
	nnz_cr		= (nnz_ref_ptr[5*CONTEXT_CACHE_WIDTH+4]<<24)| (nnz_ref_ptr[5*CONTEXT_CACHE_WIDTH+5]<<16)|
				  (nnz_ref_ptr[6*CONTEXT_CACHE_WIDTH+3]<<8) | (nnz_ref_ptr[7*CONTEXT_CACHE_WIDTH+3]);

	//configure mb information
	left_mb_avail = mb_cache_ptr->mb_avail_a;
	top_mb_avail = mb_cache_ptr->mb_avail_b;

	cmd = ((top_mb_avail<<17) | (left_mb_avail << 16) | (cbp << 8) | (vsp_mb_type << 0));
	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_MB_INFO_OFFSET, cmd, "configure mb information");

	//configure nnz
	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_TOP_NNZ_Y_OFFSET,	up_nnz_y, "configure up_nnz_y");
	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_LEFT_NNZ_Y_OFFSET,	left_nnz_y, "configure left_nnz_y");
	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_TL_NNZ_CB_OFFSET,	nnz_cb, "configure nnz_cb");
	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_TL_NNZ_CR_OFFSET,	nnz_cr, "configure nnz_cr");

	VSP_WRITE_REG(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, 1, "start vld one mb");

#if _CMODEL_
 	H264VldMBCtr ();
#endif

    if(READ_REG_POLL(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, V_BIT_1, V_BIT_1, TIME_OUT_CLK,"VLD: polling VLD one MB status"))
	{
		img_ptr->error_flag |= ER_POLL_VLD_ID;
        img_ptr->return_pos |= (1<<24);
        H264Dec_get_HW_status(img_ptr);        
		return;
	}
    
	if(((VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, "VLD: check vld error flag")>>2)&0x01))
	{
		img_ptr->error_flag |= ER_VLD_ID;
        img_ptr->return_pos |= (1<<25);
        H264Dec_get_HW_status(img_ptr);        
		return;
	}

	if ((mb_type != IPCM) && (!mb_info_ptr->is_skipped))
	{
		H264Dec_read_nnz_from_vld (mb_info_ptr, mb_cache_ptr);
	}else
	{
		if (mb_type == IPCM)
		{
	 		H264Dec_set_IPCM_nnz (mb_info_ptr, mb_cache_ptr);
		}
	}

	return;
}

PUBLIC void H264Dec_set_IPCM_nnz (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8 *nnz_ref_ptr = mb_info_ptr->nnz_ref+CONTEXT_CACHE_WIDTH+1;

	nnz_ref_ptr[0] = 0x10; nnz_ref_ptr[1] = 0x10; nnz_ref_ptr[2] = 0x10; nnz_ref_ptr[3] = 0x10; nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = 0x10; nnz_ref_ptr[1] = 0x10; nnz_ref_ptr[2] = 0x10; nnz_ref_ptr[3] = 0x10; nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = 0x10; nnz_ref_ptr[1] = 0x10; nnz_ref_ptr[2] = 0x10; nnz_ref_ptr[3] = 0x10; nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = 0x10; nnz_ref_ptr[1] = 0x10; nnz_ref_ptr[2] = 0x10; nnz_ref_ptr[3] = 0x10; 

	//for cb
	nnz_ref_ptr += (2*CONTEXT_CACHE_WIDTH);
	nnz_ref_ptr[0] = 0x10; nnz_ref_ptr[1] = 0x10; nnz_ref_ptr[3] = 0x10; nnz_ref_ptr[4] = 0x10; 

	//for cr
	nnz_ref_ptr += (CONTEXT_CACHE_WIDTH);
	nnz_ref_ptr[0] = 0x10; nnz_ref_ptr[1] = 0x10; nnz_ref_ptr[3] = 0x10; nnz_ref_ptr[4] = 0x10; 

	return;
}

PUBLIC void H264Dec_set_nnz_to_zero (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int8 *nnz_ref_ptr = mb_info_ptr->nnz_ref+CONTEXT_CACHE_WIDTH+1;

	//luma
	nnz_ref_ptr[0] = nnz_ref_ptr[1] = nnz_ref_ptr[2] = nnz_ref_ptr[3] = 0; nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = nnz_ref_ptr[1] = nnz_ref_ptr[2] = nnz_ref_ptr[3] = 0; nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = nnz_ref_ptr[1] = nnz_ref_ptr[2] = nnz_ref_ptr[3] = 0; nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = nnz_ref_ptr[1] = nnz_ref_ptr[2] = nnz_ref_ptr[3] = 0; 

	//for cb
	nnz_ref_ptr += (2*CONTEXT_CACHE_WIDTH);
	nnz_ref_ptr[0] = nnz_ref_ptr[1] = nnz_ref_ptr[2] = nnz_ref_ptr[3] = 0; 

	//for cr
	nnz_ref_ptr += CONTEXT_CACHE_WIDTH;
	nnz_ref_ptr[0] = nnz_ref_ptr[1] = nnz_ref_ptr[2] = nnz_ref_ptr[3] = 0; 

	return;
}

PUBLIC void H264Dec_start_iqt_macroblock (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_info_cmd;
	int32 qp_info_cmd;
	uint32 qp_per,qp_rem,qp_per_c,qp_rem_c;
	int32 cbp26_vld;	
	int32 mb_type	= mb_info_ptr->mb_type;
//	int32 cbp		= mb_cache_ptr->cbp;
//	int32 cbp_uv	= mb_cache_ptr->cbp_uv;

	if (mb_type == I16MB)
	{
		mb_info_cmd = (0 << 28);
	}else if (mb_type == IPCM)
	{
		mb_info_cmd = (2 << 28);
	}else
	{
		if (mb_info_ptr->is_skipped)
		{
			mb_info_cmd = (1<<28);
		}else
		{
			mb_info_cmd = (3 << 28);
		}
	}

	if ((mb_type != IPCM) && (!mb_info_ptr->is_skipped))
	{
		cbp26_vld = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_CBP_IQT_OFFSET, "read cbp for iqt from vld");
		mb_info_cmd |= cbp26_vld;
		
		qp_per = g_qpPerRem_tbl[mb_info_ptr->qp][0];
		qp_rem = g_qpPerRem_tbl[mb_info_ptr->qp][1];
		qp_per_c = g_qpPerRem_tbl[mb_info_ptr->qp_c][0];
		qp_rem_c = g_qpPerRem_tbl[mb_info_ptr->qp_c][1];
		qp_info_cmd = (qp_per<<12) | (qp_per_c<<8) | (qp_rem<<4) | (qp_rem_c<<0);
		VSP_WRITE_REG(VSP_DCT_REG_BASE+IICT_CFG1_OFF, qp_info_cmd, "configure qp information");
	}else//IPCM or skip_mb
	{
		cbp26_vld = 0;
	}
	
	//intra and inter(with non-zero coeff) should start idct, otherwise MUST NOT
	if (!mb_info_ptr->is_intra && (cbp26_vld == 0))
	{
		//MUST!! don't start IICT
		return;
	}	
	
	VSP_WRITE_REG(VSP_DCT_REG_BASE+IICT_CFG0_OFF, mb_info_cmd, "configure mb_type and cbp");
	
	VSP_WRITE_REG(VSP_DCT_REG_BASE+IICT_CTRL_OFF, 1, "start iqt");
	
#if _CMODEL_
	iqt_module();
#endif
	
	if(READ_REG_POLL(VSP_DCT_REG_BASE+IICT_CTRL_OFF, V_BIT_1, V_BIT_1, TIME_OUT_CLK,"iict: polling iict one MB status"))
	{
		g_image_ptr->error_flag |= ER_DCT_ID;
        g_image_ptr->return_pos |= (1<<26);    
         H264Dec_get_HW_status(g_image_ptr);       
	}
	
	return;
}

LOCAL int32 H264Dec_more_rbsp_data (void)
{
	int32 tmp;
//	int32 has_no_zero = 0;
	int32 byte_offset;
	int32 bit_offset;
	uint32 nDecTotalBits = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");

	byte_offset = nDecTotalBits/8;
	bit_offset = nDecTotalBits&0x7;

	if (byte_offset < g_nalu_ptr->len -1)
	{
		return TRUE;
	}

	tmp = SHOW_FLC(8-bit_offset);
	if (tmp == (1<<(7-bit_offset)))
	{
		return FALSE;
	}else
	{
		return TRUE;
	}	
}

PUBLIC int32 H264Dec_exit_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr)
{
	img_ptr->num_dec_mb++;

	if (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)
	{
		g_curr_slice_ptr->next_header = SOP;
		return TRUE;
	}else
	{
		img_ptr->curr_mb_nr = H264Dec_Fmo_get_next_mb_num(img_ptr->curr_mb_nr, img_ptr);

		if (img_ptr->curr_mb_nr == -1)
		{
			H264Dec_more_rbsp_data();
			return TRUE;
		}

		if (H264Dec_more_rbsp_data())
		{
			return FALSE;
		}

		if (img_ptr->cod_counter <= 0)
		{
			return TRUE;
		}

		return FALSE;
	}
}

PUBLIC void H264Dec_start_mbc_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
 	int32 cbp24_vld = 0;
	int32 mbc_mb_mode = 0;
	uint32 cmd;

	if (mb_info_ptr->mb_type != IPCM)
	{
		if (!mb_info_ptr->is_skipped)
		{
			cbp24_vld = VSP_READ_REG(VSP_VLD_REG_BASE+HVLD_CBP_MBC_OFFSET, "read cbp for mbc from vld");
		}else
		{
			cbp24_vld = 0;
		}	
	}else
	{
		mb_info_ptr->is_intra = TRUE;
		mbc_mb_mode = MBC_IPCM; //i_pcm 
	}
	
	if (mb_info_ptr->is_intra)
	{
		int32 mb_avail_a = mb_cache_ptr->mb_avail_a;
		int32 mb_avail_b = mb_cache_ptr->mb_avail_b;
		int32 mb_avail_c = mb_cache_ptr->mb_avail_c;
		int32 mb_avail_d = mb_cache_ptr->mb_avail_d;
		uint32 ipred_cmd0 = 0, ipred_cmd1 = 0;

		switch(mb_info_ptr->mb_type) 
		{
		case I4MB_264:
			mbc_mb_mode = MBC_I4x4; //i4x4
			break;
		case I16MB:
			mbc_mb_mode = MBC_I16x16; 
			break;
		case IPCM:
			mbc_mb_mode = MBC_IPCM; //ipcm
			break;
		default:
			break;
		}

		if (mb_info_ptr->mb_type == I4MB_264)
		{
			int8 *i4x4_pred_mode_ptr = mb_info_ptr->i4x4pred_mode_ref + CONTEXT_CACHE_WIDTH+1;
			int32 blk4x4Idx;
			int32 i, j;

			for (i = 0; i < 4; i++)
			{
				for (j = 0; j < 4; j++)
				{
					blk4x4Idx = i*4+j;

					if (i < 2)
					{
						ipred_cmd0 |= (i4x4_pred_mode_ptr[i*CONTEXT_CACHE_WIDTH+j] <<(4*(7-blk4x4Idx)));
					}else
					{
						ipred_cmd1 |= (i4x4_pred_mode_ptr[i*CONTEXT_CACHE_WIDTH+j] <<(4*(15-blk4x4Idx)));
					}
				}			
			}
		}else if (mb_info_ptr->mb_type == I16MB)
		{
			ipred_cmd0 = ipred_cmd1 = mb_cache_ptr->i16mode;
			//must! because BITSTRM may be error
			if ((mb_cache_ptr->i16mode > 4) || (mb_cache_ptr->i16mode < 0))
			{
				img_ptr->error_flag |= ER_BSM_ID;
                img_ptr->return_pos |= (1<<27);
                H264Dec_get_HW_status(img_ptr);         
				return;
			}
		}

		if (img_ptr->constrained_intra_pred_flag)
		{
//			int32 mb_x = img_ptr->mb_x;
			int32 mb_num_x = img_ptr->frame_width_in_mbs;

			if (mb_avail_a)
			{
				mb_avail_a = (mb_info_ptr-1)->is_intra;
			}

			if (mb_avail_b)
			{
				mb_avail_b = (mb_info_ptr-mb_num_x)->is_intra;
			}

			if (mb_avail_c)
			{
				mb_avail_c = (mb_info_ptr-mb_num_x+1)->is_intra;
			}

			if (mb_avail_d)
			{
				mb_avail_d = (mb_info_ptr-mb_num_x-1)->is_intra;
			}
		}

		cmd = (MBC_INTRA_MB<<30) |	((mbc_mb_mode & 0x3) << 28) | ((mb_avail_d & 0x1) << 27) | ((mb_avail_a & 0x1) << 26) | 
			((mb_avail_b & 0x1) << 25) | ((mb_avail_c & 0x1) << 24) | (cbp24_vld & 0xffffff);
		VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD0_OFF, cmd, "configure mb information.");

		VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD1_OFF, ipred_cmd0, "configure mb intra prediction mode, luma sub-block 0-7");
		VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD2_OFF, ipred_cmd1, "configure mb intra prediction mode, luma sub-block 8-15");

		VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD3_OFF, (mb_cache_ptr->c_ipred_mode & 0x3), "configure mb intra prediction mode, block 0-7");
	}else //inter mb
	{
		cmd = (MBC_INTER_MB<<30) |	(cbp24_vld & 0xffffff);
		VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD0_OFF, cmd, "configure mb information.");
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