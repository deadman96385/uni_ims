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
#include "tiger_video_header.h"
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
//for mv and mvd
LOCAL void fill_cache_templet_int16 (int32 *src_ptr, int32 *dst_cache, DEC_MB_CACHE_T *mb_cache_ptr, int32 b4_pitch)
{
	//clear by 0.
	dst_cache[16] = dst_cache[17] = dst_cache[18] = dst_cache[19] =  
	dst_cache[28] = dst_cache[29] = dst_cache[30] = dst_cache[31] = 
	dst_cache[40] = dst_cache[41] = dst_cache[42] = dst_cache[43] = 
	dst_cache[52] = dst_cache[53] = dst_cache[54] = dst_cache[55] = 0;
		
	//fill top
	if (mb_cache_ptr->mb_avail_b)
	{
		int32 *top_src_ptr = src_ptr - b4_pitch;
		dst_cache[4] = top_src_ptr[0];
		dst_cache[5] = top_src_ptr[1];
		dst_cache[6] = top_src_ptr[2];
		dst_cache[7] = top_src_ptr[3];
	}else
	{
		dst_cache[4] = dst_cache[5] = dst_cache[6] = dst_cache[7] = 0; 
	}
		
	//fill left
	if (mb_cache_ptr->mb_avail_a)
	{
		dst_cache[15] = src_ptr[b4_pitch*0 - 1];	
		dst_cache[27] = src_ptr[b4_pitch*1 - 1];	
		dst_cache[39] = src_ptr[b4_pitch*2 - 1];	
		dst_cache[51] = src_ptr[b4_pitch*3 - 1];				
	}else
	{
		dst_cache[15] = dst_cache[27] = 
		dst_cache[39] = dst_cache[51] = 0;
	}	
}

//for direct, ref_idx
LOCAL void fill_cache_templet_int8 (int8 *src_ptr, int8 *dst_cache, DEC_MB_CACHE_T *mb_cache_ptr, int32 b4_pitch)
{
	if (!mb_cache_ptr->is_direct)
	{
		//clear by 0.
		((int32 *)dst_cache)[4]  = 
		((int32 *)dst_cache)[7]  = 
		((int32 *)dst_cache)[10] = 
		((int32 *)dst_cache)[13] = 0; 
	}
	
	//fill top
	if (mb_cache_ptr->mb_avail_b)
	{
		((int32 *)dst_cache)[1] = ((int32 *)(src_ptr - b4_pitch))[0];
	}else
	{
		((int32 *)dst_cache)[1] = 0x0;
	}
	
	//fill left
	if (mb_cache_ptr->mb_avail_a)
	{
		dst_cache[CTX_CACHE_WIDTH_X1 + 3] = src_ptr[b4_pitch*0 - 1];	
		dst_cache[CTX_CACHE_WIDTH_X2 + 3] = src_ptr[b4_pitch*1 - 1];	
		dst_cache[CTX_CACHE_WIDTH_X3 + 3] = src_ptr[b4_pitch*2 - 1];	
		dst_cache[CTX_CACHE_WIDTH_X4 + 3] = src_ptr[b4_pitch*3 - 1];				
	}else
	{
		dst_cache[CTX_CACHE_WIDTH_X1 + 3] = 
		dst_cache[CTX_CACHE_WIDTH_X2 + 3] = 
		dst_cache[CTX_CACHE_WIDTH_X3 + 3] = 
		dst_cache[CTX_CACHE_WIDTH_X4 + 3] = 0;
	}	
}

LOCAL void fill_cache (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 list;		
	int32 b4_pitch = img_ptr->b4_pitch;
	int8 *nnz_ptr, *nnz_cache;
	int32 *mv_ptr, *mv_cache;
	int32 *mvd_ptr, *mvd_cache;
	int8 *ref_idx_ptr, *ref_idx_cache;
				
	//nnz_cache 	
	nnz_ptr = img_ptr->nnz_ptr + img_ptr->b4_offset_nnz;
	nnz_cache = mb_cache_ptr->nnz_cache;
	((int32 *)nnz_cache)[4]  = ((int32 *)nnz_cache)[7]  = 
	((int32 *)nnz_cache)[10] = ((int32 *)nnz_cache)[13] = 0;//y	
	((int16 *)nnz_cache)[38] = ((int16 *)nnz_cache)[44] = 0;//u
	((int16 *)nnz_cache)[40] = ((int16 *)nnz_cache)[46] = 0;//v
		 
	//top nnz reference
	if (mb_cache_ptr->mb_avail_b)
	{
		((int32 *)nnz_cache)[1]  = ((int32 *)(nnz_ptr - b4_pitch*3))[0];//y
		((int16 *)nnz_cache)[32] = ((int16 *)(nnz_ptr - b4_pitch))[0]; //cb
		((int16 *)nnz_cache)[34] = ((int16 *)(nnz_ptr - b4_pitch))[1]; //cr
	}else/*set nnz_ref not available*/	
	{
		((int32 *)nnz_cache)[1]	= 0x40404040;//y
		((int16 *)nnz_cache)[32] = 0x4040;	//cb
		((int16 *)nnz_cache)[34] = 0x4040;	//cr
	}
	
	//left nnz ref
	if (mb_cache_ptr->mb_avail_a) 
	{	
		nnz_cache[CTX_CACHE_WIDTH_X1 + 3] = nnz_ptr[b4_pitch*0 - 1];//y
		nnz_cache[CTX_CACHE_WIDTH_X2 + 3] = nnz_ptr[b4_pitch*1 - 1];
		nnz_cache[CTX_CACHE_WIDTH_X3 + 3] = nnz_ptr[b4_pitch*2 - 1];
		nnz_cache[CTX_CACHE_WIDTH_X4 + 3] = nnz_ptr[b4_pitch*3 - 1];
		
		nnz_cache[CTX_CACHE_WIDTH_X6 + 3] = nnz_ptr[b4_pitch*4 - 3];//cb
		nnz_cache[CTX_CACHE_WIDTH_X7 + 3] = nnz_ptr[b4_pitch*5 - 3];
		
		nnz_cache[CTX_CACHE_WIDTH_X6 + 7] = nnz_ptr[b4_pitch*4 - 1];//cr
		nnz_cache[CTX_CACHE_WIDTH_X7 + 7] = nnz_ptr[b4_pitch*5 - 1];
	}else/*set nnz_ref not available*/	
	{
		nnz_cache[CTX_CACHE_WIDTH_X1 + 3] = 	//Y
		nnz_cache[CTX_CACHE_WIDTH_X2 + 3] = 
		nnz_cache[CTX_CACHE_WIDTH_X3 + 3] = 
		nnz_cache[CTX_CACHE_WIDTH_X4 + 3] = 0x40;
		
		nnz_cache[CTX_CACHE_WIDTH_X6 + 3] = 0x40;//cb
		nnz_cache[CTX_CACHE_WIDTH_X7 + 3] = 0x40;
				
		nnz_cache[CTX_CACHE_WIDTH_X6 + 7] = 0x40;//cr
		nnz_cache[CTX_CACHE_WIDTH_X7 + 7] = 0x40;
	}

	for (list = 0; list < img_ptr->list_count; list++)
	{	
		mv_ptr = (int32 *)(g_dec_picture_ptr->mv_ptr[list]) + img_ptr->b4_offset;
		mv_cache = (int32 *)(mb_cache_ptr->mv_cache[list]);
		fill_cache_templet_int16(mv_ptr, mv_cache, mb_cache_ptr, b4_pitch);
		//fill top-right
		if (mb_cache_ptr->mb_avail_c)
		{
			mv_cache[8] = mv_ptr[4 - b4_pitch];
		}else
		{
			mv_cache[8] = 0;
		}
		//fill top-left
		if (mb_cache_ptr->mb_avail_d)
		{
			mv_cache[3] = mv_ptr[-1 - b4_pitch];
		}else
		{
			mv_cache[3] = 0;
		}

		mvd_ptr = (int32 *)(img_ptr->mvd_ptr[list]) + img_ptr->b4_offset;
		mvd_cache = (int32 *)(mb_cache_ptr->mvd_cache[list]);
		fill_cache_templet_int16(mvd_ptr, mvd_cache, mb_cache_ptr, b4_pitch);
		
		ref_idx_ptr = g_dec_picture_ptr->ref_idx_ptr[list] + img_ptr->b4_offset;
		ref_idx_cache = mb_cache_ptr->ref_idx_cache[list];
	//	fill_cache_templet_int8(ref_idx_ptr, ref_idx_cache, mb_cache_ptr, b4_pitch);
		if (mb_info_ptr->is_intra)
		{	//clear by -1.
			((int32 *)ref_idx_cache)[4]  = ((int32 *)ref_idx_cache)[7]  = 
			((int32 *)ref_idx_cache)[10] = ((int32 *)ref_idx_cache)[13] = -1; 
			
		}else
		{	//clear by 0.
			((int32 *)ref_idx_cache)[4]  = ((int32 *)ref_idx_cache)[7]  = 
			((int32 *)ref_idx_cache)[10] = ((int32 *)ref_idx_cache)[13] = 0; 
		}
		
		//fill top
		if (mb_cache_ptr->mb_avail_b)
		{
			((int32 *)ref_idx_cache)[1] = ((int32 *)(ref_idx_ptr - b4_pitch))[0];
		}else
		{
			((int32 *)ref_idx_cache)[1] = 0xfefefefe;
		}
		
		//fill left
		if (mb_cache_ptr->mb_avail_a)
		{
			ref_idx_cache[CTX_CACHE_WIDTH_X1 + 3] = ref_idx_ptr[b4_pitch*0 - 1];	
			ref_idx_cache[CTX_CACHE_WIDTH_X2 + 3] = ref_idx_ptr[b4_pitch*1 - 1];	
			ref_idx_cache[CTX_CACHE_WIDTH_X3 + 3] = ref_idx_ptr[b4_pitch*2 - 1];	
			ref_idx_cache[CTX_CACHE_WIDTH_X4 + 3] = ref_idx_ptr[b4_pitch*3 - 1];				
		}else
		{
			ref_idx_cache[CTX_CACHE_WIDTH_X1 + 3] = 
			ref_idx_cache[CTX_CACHE_WIDTH_X2 + 3] = 
			ref_idx_cache[CTX_CACHE_WIDTH_X3 + 3] = 
			ref_idx_cache[CTX_CACHE_WIDTH_X4 + 3] = -2;
		}

		//fill top-right
		if (mb_cache_ptr->mb_avail_c)
		{
			ref_idx_cache[8] = ref_idx_ptr[4 - b4_pitch];
		}else
		{
			ref_idx_cache[8] = -2;
		}

		//fill top-left
		if (mb_cache_ptr->mb_avail_d)
		{
			ref_idx_cache[3] = ref_idx_ptr[-1 - b4_pitch];
		}else
		{
			ref_idx_cache[3] = -2;
		}

		//fill right
		ref_idx_cache[20] = 
		ref_idx_cache[32] = 
		ref_idx_cache[44] = 
		ref_idx_cache[56] = -2;
	}
				
	if ( img_ptr->is_cabac)
	{
		//direct cache
		if (img_ptr->type == B_SLICE)
		{
			int8 *direct_ptr = img_ptr->direct_ptr + img_ptr->b4_offset;
			int8 *direct_cache = mb_cache_ptr->direct_cache;
			fill_cache_templet_int8(direct_ptr, direct_cache, mb_cache_ptr, b4_pitch);
		}
	
		//mvd
		for (list = 0; list < img_ptr->list_count; list++)
		{
			int32 *mvd_ptr = (int32*)(img_ptr->mvd_ptr[list]) + img_ptr->b4_offset;
			int32 *mvd_cache = (int32*)(mb_cache_ptr->mvd_cache[list]);	
			fill_cache_templet_int16(mvd_ptr, mvd_cache, mb_cache_ptr, b4_pitch);
		}
	}		
}

LOCAL void H264Dec_store_mc_info (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_STORABLE_PICTURE_T *pic = g_dec_picture_ptr;
	int32 b4_offset= img_ptr->b4_offset;
	int32 b4_pitch = img_ptr->b4_pitch;
	int32 list, row;
	
	for (list = 0; list < img_ptr->list_count; list++)
	{
		int8  *ref_idx_cache = mb_cache_ptr->ref_idx_cache[list] + CTX_CACHE_WIDTH_PLUS4;
		int32 *mv_cache = (int32*)(mb_cache_ptr->mv_cache[list]) + CTX_CACHE_WIDTH_PLUS4;
		int32 *src_ref_pic_num_ptr = pic->pic_num_ptr[img_ptr->slice_nr][list];

		int8 *ref_idx_ptr = pic->ref_idx_ptr[list] + b4_offset;
		int32 *ref_pic_id_ptr = pic->ref_pic_id_ptr[list] + b4_offset;
		int32 *mv_ptr = (int32*)(pic->mv_ptr[list]) + b4_offset;

		for (row = 4; row > 0; row--)
		{
			((int32 *)ref_idx_ptr)[0] = ((int32 *)ref_idx_cache)[0];
			
			ref_pic_id_ptr[0] = src_ref_pic_num_ptr[ref_idx_cache[0]];
			ref_pic_id_ptr[1] = src_ref_pic_num_ptr[ref_idx_cache[1]];
			ref_pic_id_ptr[2] = src_ref_pic_num_ptr[ref_idx_cache[2]];
			ref_pic_id_ptr[3] = src_ref_pic_num_ptr[ref_idx_cache[3]];

			mv_ptr[0] = mv_cache[0];
			mv_ptr[1] = mv_cache[1];
			mv_ptr[2] = mv_cache[2];
			mv_ptr[3] = mv_cache[3];

			ref_idx_ptr += b4_pitch;
			ref_pic_id_ptr += b4_pitch;
			mv_ptr += b4_pitch;
			ref_idx_cache += CTX_CACHE_WIDTH;
			mv_cache += CTX_CACHE_WIDTH;
		}	
	}
}

void write_back_cache (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 i, list;
	int32 b4_pitch = img_ptr->b4_pitch;
	int32 pitch = b4_pitch >> 2;
	int8 *i4_pred_mode_ptr, *i4_pred_mode_cache;
	int8 *nnz_ptr, *nnz_cache;

	//nnz cache
	nnz_ptr = img_ptr->nnz_ptr + img_ptr->b4_offset_nnz;
	nnz_cache = mb_cache_ptr->nnz_cache;
	((int32 *)nnz_ptr)[0*pitch] = ((int32 *)nnz_cache)[4];
	((int32 *)nnz_ptr)[1*pitch] = ((int32 *)nnz_cache)[7];
	((int32 *)nnz_ptr)[2*pitch] = ((int32 *)nnz_cache)[10];
	((int32 *)nnz_ptr)[3*pitch] = ((int32 *)nnz_cache)[13]; //y
	nnz_ptr += b4_pitch*4;
	((int16 *)nnz_ptr)[0] = ((int16 *)nnz_cache)[38];
	((int16 *)nnz_ptr)[1] = ((int16 *)nnz_cache)[40];
	nnz_ptr += b4_pitch;
	((int16 *)nnz_ptr)[0] = ((int16 *)nnz_cache)[44];
	((int16 *)nnz_ptr)[1] = ((int16 *)nnz_cache)[46];

	//i4x4_pred_mode_cache
	if (mb_info_ptr->is_intra)
	{
		i4_pred_mode_ptr = img_ptr->i4x4pred_mode_ptr + img_ptr->b4_offset;
		i4_pred_mode_cache = mb_cache_ptr->i4x4pred_mode_cache;

		((int32 *)i4_pred_mode_ptr)[0*pitch] = ((int32 *)i4_pred_mode_cache)[4];
		((int32 *)i4_pred_mode_ptr)[1*pitch] = ((int32 *)i4_pred_mode_cache)[7];
		((int32 *)i4_pred_mode_ptr)[2*pitch] = ((int32 *)i4_pred_mode_cache)[10];
		((int32 *)i4_pred_mode_ptr)[3*pitch] = ((int32 *)i4_pred_mode_cache)[13];
	}

	//lsw for direct mode
	H264Dec_store_mc_info(img_ptr, mb_cache_ptr);

	if (img_ptr->is_cabac)
	{
		//direct cache
		if (img_ptr->type == B_SLICE)
		{
			int8 *direct_ptr = img_ptr->direct_ptr + img_ptr->b4_offset;
			int8 *direct_cache = mb_cache_ptr->direct_cache;

			((int32 *)direct_ptr)[0*pitch] = ((int32 *)direct_cache)[4];
			((int32 *)direct_ptr)[1*pitch] = ((int32 *)direct_cache)[7];
			((int32 *)direct_ptr)[2*pitch] = ((int32 *)direct_cache)[10];
			((int32 *)direct_ptr)[3*pitch] = ((int32 *)direct_cache)[13];
		}

		//mvd cache
		for (list = 0; list < img_ptr->list_count; list++)
		{
			int32 *mvd_ptr = (int32 *)(img_ptr->mvd_ptr[list]) + img_ptr->b4_offset;
			int32 *mvd_cache = (int32 *)(mb_cache_ptr->mvd_cache[list]) + CTX_CACHE_WIDTH_PLUS4;

			for (i = 4; i > 0; i--)
			{
				mvd_ptr[0] = mvd_cache[0]; 	mvd_ptr[1] = mvd_cache[1];
				mvd_ptr[2] = mvd_cache[2]; 	mvd_ptr[3] = mvd_cache[3];
				mvd_ptr += b4_pitch; mvd_cache += CTX_CACHE_WIDTH;
			}
		}
	}

	return;
}

PUBLIC void H264Dec_start_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{	
	int32 mb_x = img_ptr->mb_x;
	int32 mb_y = img_ptr->mb_y;
	int32 frame_width_in_mbs = img_ptr->frame_width_in_mbs;
	int32 curr_mb_slice_nr;	
	
	VSP_WRITE_REG_CQM (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, (mb_y << 8) | (mb_x << 0), "vsp global reg: configure current MB position");
	VSP_WRITE_CMD_INFO((VSP_GLB << CQM_SHIFT_BIT) | (1<<24) | GLB_CTRL0_WOFF);

	mb_info_ptr->slice_nr = img_ptr->slice_nr;
	mb_info_ptr->is_intra = FALSE;
	mb_info_ptr->qp = img_ptr->qp;
	mb_info_ptr->skip_flag = 0;
	mb_info_ptr->c_ipred_mode = 0;
	mb_info_ptr->cbp = 0;

	mb_cache_ptr->is_skipped = FALSE;
	((int32 *)(mb_cache_ptr->b8mode))[0] = 0;
	((int32 *)(mb_cache_ptr->b8pdir))[0] = 0;
	((int32 *)(mb_cache_ptr->mbc_b8pdir))[0] = 0;
	
	mb_cache_ptr->cbp_uv = 0;
	mb_cache_ptr->is_direct = 0;

#if _DEBUG_
	if ((img_ptr->mb_x == 0) && (img_ptr->mb_y == 1) && (g_nFrame_dec_h264 == 0))
	{
		foo2();
	}
#endif	

	img_ptr->abv_mb_info = mb_info_ptr - frame_width_in_mbs;
	curr_mb_slice_nr = mb_info_ptr->slice_nr;	
	mb_cache_ptr->mb_avail_a = ((mb_x > 0) && (curr_mb_slice_nr == (mb_info_ptr - 1)->slice_nr)); //left
	mb_cache_ptr->mb_avail_b = ((mb_y > 0) && (curr_mb_slice_nr == img_ptr->abv_mb_info->slice_nr)); //top
	mb_cache_ptr->mb_avail_c = ((mb_y > 0) && 
		(mb_x < (frame_width_in_mbs -1)) && (curr_mb_slice_nr == (img_ptr->abv_mb_info+1)->slice_nr)); //top right
	mb_cache_ptr->mb_avail_d = ((mb_x > 0) && (mb_y > 0) && (curr_mb_slice_nr == (img_ptr->abv_mb_info-1)->slice_nr)); //top left

	img_ptr->b4_offset = (mb_y<<2)* img_ptr->b4_pitch+ (mb_x<<2);
	img_ptr->b4_offset_nnz = (mb_y*6)* img_ptr->b4_pitch + (mb_x<<2);

	if (img_ptr->type == B_SLICE)
	{
		int32 i, j, ii, jj;
		int32 blk_map_8x8[4] = {0, 0, 3, 3};
		int32 blk_map_normal[4] = {0, 1, 2, 3};
		int32 *blk_map;
		int32 blk_x = (mb_x << 2);
		int32 blk_y = (mb_y << 2);
		int32 b4_pitch = img_ptr->b4_pitch;

		if (g_active_sps_ptr->direct_8x8_inference_flag == 1)
		{
			blk_map = blk_map_8x8;
		}else
		{
			blk_map = blk_map_normal;
		}

		for (j = 0; j < 4; j++)
		{
			jj = blk_y + blk_map[j];
			for (i = 0; i < 4; i++)
			{
				ii = blk_x + blk_map[i];
				img_ptr->corner_map[j*4+i] = jj*b4_pitch + ii;
			}
		}
	}
		
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
		mb_info_ptr->cbp = g_ICBP_TBL[(mb_mode-1)>>2];
		mb_cache_ptr->i16mode = ((mb_mode-1)&0x3);
	}

	return;
}

LOCAL void H264Dec_interpret_mb_mode_P (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 mb_mode = mb_info_ptr->mb_type;
	switch (mb_mode)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		mb_cache_ptr->all_zero_ref = 0;
		break;
	case 4:
	case 5:
		mb_info_ptr->mb_type = P8x8;
		mb_cache_ptr->all_zero_ref = mb_mode - 4;
	{
		uint32 sub_mb_mode;
		int32 blk8x8Nr;

		for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
		{
			sub_mb_mode = read_b8mode (img_ptr, mb_cache_ptr, blk8x8Nr);					
           	if((sub_mb_mode < 0) || (sub_mb_mode >= 4))
		 	{
				img_ptr->error_flag = 1;
				return;
			}
			mb_cache_ptr->b8mode[blk8x8Nr] = sub_mb_mode + 4;
			mb_cache_ptr->b8pdir[blk8x8Nr] = 0;
		}
	}
		break;
	case 6:
		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->mb_type = I4MB_264;
		break;
	case 31:	
		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->mb_type = IPCM;
		break;
	default:
		mb_mode -= 7;
		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->mb_type = I16MB;
		mb_info_ptr->cbp = g_ICBP_TBL[mb_mode>>2];
		mb_cache_ptr->i16mode = (mb_mode&0x3);			
	}

	return;
}

static const int b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, 11};
static const int b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

static const int32 offset2pdir16x16[12]   = {0, 0, 1, 2, 0,0,0,0,0,0,0,0};
static const int32 offset2pdir16x8[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},{1,0},
                                            {0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2},{0,0}};
static const int32 offset2pdir8x16[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},
                                             {1,0},{0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2}};

LOCAL void H264Dec_interpret_mb_mode_B (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 i, mbmode;
	int32 mbtype = mb_info_ptr->mb_type;
	int8 *b8mode = mb_cache_ptr->b8mode;
	int8 *b8pdir = mb_cache_ptr->b8pdir;

	//set mbtype, b8type, and b8pdir
	if (mbtype== 0) //B_Skip,B_Direct_16*16
	{
		int8 *direct_cache = mb_cache_ptr->direct_cache;
		mbmode = 0;
		((int32 *)b8mode)[0] = 0;
		((int32 *)b8pdir)[0] = 0x02020202;

		((int32*)direct_cache)[4] = 	
		((int32*)direct_cache)[7] = 	
		((int32*)direct_cache)[10] = 	
		((int32*)direct_cache)[13] = 0x01010101;	

		mb_cache_ptr->is_direct = 1;
	}else if (mbtype == 23) //intra4x4
	{
		mbmode = I4MB_264;
		((int32 *)b8mode)[0] = 0x0b0b0b0b;
		((int32 *)b8pdir)[0] = 0xffffffff;
		mb_info_ptr->is_intra = TRUE;
	}else if ((mbtype>23) && (mbtype < 48)) //intra16x16
	{
		mbmode = I16MB; 
		((int32 *)b8mode)[0] = 0x00000000;
		((int32 *)b8pdir)[0] = 0xffffffff;

		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->cbp = g_ICBP_TBL[(mbtype-24)>>2];
		mb_cache_ptr->i16mode = (mbtype-24)&0x3;
	}else if (mbtype == 22) //8x8(+split)
	{
		DEC_BS_T *stream = img_ptr->bitstrm_ptr;
		uint32 sub_mb_mode;
		int32 blk8x8Nr;

		//b8mode and pdir is transmitted in additional codewords
		for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
		{
			int32 offset = (blk8x8Nr>>1)*CTX_CACHE_WIDTH_X2 + (blk8x8Nr&0x1)*2;
			int8 *direct_dst_ptr = mb_cache_ptr->direct_cache + CTX_CACHE_WIDTH_PLUS4+ offset;
			
			sub_mb_mode = read_b8mode (img_ptr, mb_cache_ptr, blk8x8Nr);
			if (!sub_mb_mode)
			{
				((int16*)direct_dst_ptr)[0] = 0x0101; direct_dst_ptr += CTX_CACHE_WIDTH;
				((int16*)direct_dst_ptr)[0] = 0x0101;	
				
				mb_cache_ptr->is_direct = 1;
			}else
			{
				((int16*)direct_dst_ptr)[0] = 0x0; direct_dst_ptr += CTX_CACHE_WIDTH;
				((int16*)direct_dst_ptr)[0] = 0x0;	
			}
					
			b8mode[blk8x8Nr] = b_v2b8[sub_mb_mode];
			b8pdir[blk8x8Nr] = b_v2pd[sub_mb_mode];				
		}
		mbmode = P8x8;	
	}else if (mbtype < 4) //16x16
	{
		mbmode = 1;
		((int32 *)b8mode)[0] = 0x01010101;
		((int32 *)b8pdir)[0] = 0x01010101 * offset2pdir16x16[mbtype];
	}else if (mbtype == 48)
	{
		mbmode = IPCM;
		((int32 *)b8mode)[0] = 0x00000000;
		((int32 *)b8pdir)[0] = 0xffffffff;

		mb_info_ptr->is_intra = TRUE;
		mb_info_ptr->cbp = -1;
		mb_cache_ptr->i16mode = 0;
	}else if (mbtype%2==0) //16x8
	{
		mbmode = 2;
		((int32 *)b8mode)[0] = 0x02020202;
		b8pdir[0] = b8pdir[1] = offset2pdir16x8[mbtype][0];
		b8pdir[2] = b8pdir[3] = offset2pdir16x8[mbtype][1];
	}else
	{
		mbmode = 3;
		((int32 *)b8mode)[0] = 0x03030303;
		b8pdir[0] = b8pdir[2] = offset2pdir8x16[mbtype][0];
		b8pdir[1] = b8pdir[3] = offset2pdir8x16[mbtype][1];
	}
	mb_info_ptr->mb_type = mbmode;

	return;
}

LOCAL void H264Dec_fill_intraMB_context (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 top_mb_avail, left_mb_avail;
	DEC_MB_INFO_T *left_mb_info_ptr;
	int32 is_constained_intra = img_ptr->constrained_intra_pred_flag;
	int32 b4_pitch = img_ptr->b4_pitch;

	int8 *i4x4_pred_mode_cache = mb_cache_ptr->i4x4pred_mode_cache;
	int8 *i4x4_pred_mode_ptr = img_ptr->i4x4pred_mode_ptr + img_ptr->b4_offset;

	//top ipred mde reference
	top_mb_avail = mb_cache_ptr->mb_avail_b;
	if (top_mb_avail && img_ptr->abv_mb_info->is_intra)
	{
		((int32 *)i4x4_pred_mode_cache)[1] = ((int32 *)(i4x4_pred_mode_ptr - b4_pitch))[0];
	}else
	{
		if (top_mb_avail && !is_constained_intra)
		{
			((int32 *)i4x4_pred_mode_cache)[1] = 0x02020202;
		}else
		{
			((int32 *)i4x4_pred_mode_cache)[1] = (int32)0xfefefefe;
		}
	}

	//left ipred mode reference
	left_mb_info_ptr = mb_info_ptr-1;
	left_mb_avail = mb_cache_ptr->mb_avail_a;
	if (left_mb_avail && left_mb_info_ptr->is_intra)
	{
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X1 + 3] = i4x4_pred_mode_ptr[b4_pitch*0 - 1];
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X2 + 3] = i4x4_pred_mode_ptr[b4_pitch*1 - 1];
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X3 + 3] = i4x4_pred_mode_ptr[b4_pitch*2 - 1];
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X4 + 3] = i4x4_pred_mode_ptr[b4_pitch*3 - 1];
	}else
	{
		int8 i4Pred;

		i4Pred = (left_mb_avail && !is_constained_intra) ? 2 : -2;

		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X1 + 3] = 
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X2 + 3] = 
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X3 + 3] = 
		i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X4 + 3] = i4Pred;
	}

	fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

	return;	
}

LOCAL void H264Dec_read_and_derive_ipred_modes(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	//get 16 block4x4 intra prediction mode, luma
	if (mb_info_ptr->mb_type == I4MB_264)
	{
		int32 blk4x4StrIdx, blk4x4Idx;
		int32 pred_mode;
		int32 up_ipred_mode;
		int32 left_ipred_mode;
		int32 most_probable_ipred_mode;
		int8 *i4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;
		uint8 *blk_order_map_tbl_ptr = g_blk_order_map_tbl;
		DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;
		int32 val;

		int32 i, j;
		uint32 mbc_id_map[8] = {3, 1, 2, 0, 7, 5, 6, 4};

		for (blk4x4Idx = 0, i = 0; i < 8; i++)
		{	
			uint32 maped_id = mbc_id_map[i];
			mb_cache_ptr->mbc_ipred_cmd[maped_id]  = 0;
			for (j = 1; j >= 0; j--)
			{
				if (!img_ptr->is_cabac)
				{
					val = SHOW_FLC(bitstrm, 4);
					if(val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!, noted by xwluo@20100618
					{
						pred_mode = -1;
						READ_BITS1(bitstrm);
					}else
					{
						pred_mode = val & 0x7; //remaining_mode_selector
						READ_FLC(bitstrm, 4);
					}
				}else
				{
					pred_mode = readIntraPredMode_CABAC (img_ptr);
				}
				
				blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
				left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
				up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-CTX_CACHE_WIDTH];
				most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

				if (most_probable_ipred_mode < 0)
				{
					most_probable_ipred_mode = DC_PRED;
				}

				pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + (pred_mode >= most_probable_ipred_mode)));
					
				//must! because BITSTRM may be error
				if ((pred_mode >9) || (pred_mode < 0))
				{
					img_ptr->error_flag = TRUE;
					return;
				}

				i4_pred_mode_ref_ptr[blk4x4StrIdx] = pred_mode;
				mb_cache_ptr->mbc_ipred_cmd[maped_id] |= (pred_mode << (4*j));
				blk4x4Idx++;
			}
		}
		
		mb_cache_ptr->mbc_mb_mode = MBC_I4x4;
	}else
	{
		int8 *i4x4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;

		((int32*)i4x4_pred_mode_ref_ptr)[4] = 
		((int32*)i4x4_pred_mode_ref_ptr)[7] = 
		((int32*)i4x4_pred_mode_ref_ptr)[10] = 
		((int32*)i4x4_pred_mode_ref_ptr)[13] = 0x02020202;	

		mb_cache_ptr->mbc_mb_mode = MBC_I16x16;

		((uint32*)(mb_cache_ptr->mbc_ipred_cmd))[0] = ((uint32*)(mb_cache_ptr->mbc_ipred_cmd))[1] = mb_cache_ptr->i16mode;
	}

	if (!img_ptr->is_cabac)
	{
		DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;

		//get chroma intra prediction mode
		mb_info_ptr->c_ipred_mode = READ_UE_V(bitstrm);
	}else
	{
		mb_info_ptr->c_ipred_mode = readCIPredMode_CABAC(img_ptr, mb_info_ptr, mb_cache_ptr);
	}

	return;
}

LOCAL void H264Dec_read_CBPandDeltaQp(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	if (mb_info_ptr->mb_type != I16MB)
	{
		if (!img_ptr->is_cabac)
		{
			int32 val = READ_UE_V(stream);
			if(mb_info_ptr->is_intra)
			{
				mb_info_ptr->cbp = g_cbp_intra_tbl[val];
			}else
			{
				mb_info_ptr->cbp = g_cbp_inter_tbl[val];
			}
		}else
		{
			mb_info_ptr->cbp = readCBP_CABAC (img_ptr,  mb_info_ptr, mb_cache_ptr);
		}
	}

	if ((mb_info_ptr->cbp > 0) || (mb_info_ptr->mb_type == I16MB))
	{
		int32 delta_q;
		if (!img_ptr->is_cabac)
		{
			delta_q = READ_SE_V(stream);
		}else
		{
			delta_q = readDquant_CABAC (img_ptr,  mb_info_ptr);
		}
		img_ptr->qp = mb_info_ptr->qp = (img_ptr->qp+delta_q+52)%52;
	}

	return;
}

/* set IPCM NNZ                                                         */
/************************************************************************/
LOCAL void H264Dec_set_IPCM_nnz(DEC_MB_CACHE_T *mb_cache_ptr)
{
#if 0
	int8 *pnnzRef = mb_cache_ptr->nnz_cache + CTX_CACHE_WIDTH_PLUS4 ;
	
   	((int32 *)pnnzRef)[0] = 0x10101010;	 pnnzRef += CTX_CACHE_WIDTH;
   	((int32 *)pnnzRef)[0] = 0x10101010;	 pnnzRef += CTX_CACHE_WIDTH;
    ((int32 *)pnnzRef)[0] = 0x10101010;	 pnnzRef += CTX_CACHE_WIDTH; 
   	((int32 *)pnnzRef)[0] = 0x10101010;	 
   
   	/*for CB*/
	pnnzRef +=  CTX_CACHE_WIDTH_X2;
	((int16 *)pnnzRef)[0] = 0x1010;	((int16 *)pnnzRef)[2] = 0x1010;

	 /*for CR*/
	pnnzRef += ( CTX_CACHE_WIDTH);
   	((int16 *)pnnzRef)[0] = 0x1010;	((int16 *)pnnzRef)[2] = 0x1010;
#else
	int8 *nnz_cache = mb_cache_ptr->nnz_cache;

	((int32 *)nnz_cache)[4]  = ((int32 *)nnz_cache)[7]  = 
	((int32 *)nnz_cache)[10] = ((int32 *)nnz_cache)[13] = 0x10101010;//y	
	((int16 *)nnz_cache)[38] = ((int16 *)nnz_cache)[44] = 0x1010;//u
	((int16 *)nnz_cache)[40] = ((int16 *)nnz_cache)[46] = 0x1010;//v
#endif
}

/************************************************************************/
/* decode I_PCM data                                                    */
/************************************************************************/
LOCAL void H264Dec_decode_IPCM_MB(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, DEC_BS_T * bitstrm)
{
	int32 i,j;
	int32 bit_offset;

	bit_offset  = (bitstrm->bitsLeft & 0x7);
	
	if(bit_offset)
	{
		READ_FLC(bitstrm, bit_offset);
	}

	//Y
	for(j = 0; j < 16; j++)
	{
		for(i = 0; i < 16; i += 4)
		{
			READ_FLC(bitstrm, 32);
		}
	}

	//U
	for(j = 0; j < 8; j++)
	{
		for(i = 0; i < 8; i += 4)
		{
			READ_FLC(bitstrm, 32);
		}
	}	
	//V
	for(j = 0; j < 8; j++)
	{
		for(i = 0; i < 8; i += 4)
		{
			READ_FLC(bitstrm, 32);
			
		}
	}		
}

LOCAL void H264Dec_read_intraMB_context (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	H264Dec_fill_intraMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);
	
	if (mb_info_ptr->mb_type != IPCM)
	{
		H264Dec_read_and_derive_ipred_modes (img_ptr, mb_info_ptr, mb_cache_ptr);
		H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		int32 *i4x4pred_mode_cache = (int32 *)(mb_cache_ptr->i4x4pred_mode_cache);
		
		i4x4pred_mode_cache[4] = i4x4pred_mode_cache[7] =
		i4x4pred_mode_cache[10] = i4x4pred_mode_cache[13] = 0x02020202;	

		mb_cache_ptr->mbc_mb_mode = MBC_IPCM;
		mb_info_ptr->qp = 0;
	}
}

PUBLIC uint32 readB8_typeInfo_cavlc (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr)
{
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	uint32 b8mode;

	b8mode = READ_UE_V(stream);

	return b8mode;
}

PUBLIC void read_mb_type_ISlice_cabac (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->mb_type = readMB_typeInfo_CABAC (img_ptr, mb_info_ptr, mb_cache_ptr);
}

PUBLIC void read_mb_type_PBSlice_cabac (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->mb_type = readMB_skip_flagInfo_CABAC (img_ptr, mb_info_ptr, mb_cache_ptr);
	
	// read MB type
	if (mb_info_ptr->mb_type != 0)
	{
		mb_info_ptr->mb_type = readMB_typeInfo_CABAC (img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		mb_cache_ptr->is_skipped = TRUE;
		mb_info_ptr->dc_coded_flag = 0;
	}
}

PUBLIC void read_mb_type_ISlice_cavlc (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;
	mb_info_ptr->mb_type = READ_UE_V(stream);
}

PUBLIC void read_mb_type_PBSlice_cavlc (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_BS_T *stream = img_ptr->bitstrm_ptr;

	//cod counter
	if (img_ptr->cod_counter == -1)
	{
		img_ptr->cod_counter = READ_UE_V(stream);
	}
	
	if (img_ptr->cod_counter == 0)
	{
		mb_info_ptr->mb_type = READ_UE_V(stream) + (img_ptr->type == P_SLICE);
		mb_info_ptr->skip_flag = 0;
	}else
	{
		mb_info_ptr->mb_type = 0;
		mb_cache_ptr->is_skipped = TRUE;
		mb_info_ptr->skip_flag = 1; 
	}
	img_ptr->cod_counter--;
}

PUBLIC void H264Dec_read_one_macroblock_ISlice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	mb_info_ptr->is_intra = TRUE;

	read_mb_type_ISlice (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_I (mb_info_ptr, mb_cache_ptr);	
	H264Dec_read_intraMB_context(img_ptr, mb_info_ptr, mb_cache_ptr);

	return;
}

PUBLIC void H264Dec_read_one_macroblock_PSlice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	read_mb_type_PBSlice (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_P(img_ptr, mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_read_intraMB_context(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		if (g_list[0][0] == NULL)
		{
			img_ptr->error_flag = TRUE;
			return;
		}

		mb_cache_ptr->read_ref_id_flag[0] =( (img_ptr->ref_count[0] > 1) && (!mb_cache_ptr->all_zero_ref));
		mb_cache_ptr->read_ref_id_flag[1] = ((img_ptr->ref_count[1] > 1) && (!mb_cache_ptr->all_zero_ref));

		fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

		if (!mb_cache_ptr->is_skipped)
		{
			H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			mb_cache_ptr->cbp_iqt = 0;
			mb_cache_ptr->cbp_mbc = 0;
		}
	}
	
	return;
}


PUBLIC void H264Dec_read_one_macroblock_BSlice (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	read_mb_type_PBSlice (img_ptr, mb_info_ptr, mb_cache_ptr);
	H264Dec_interpret_mb_mode_B(img_ptr, mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->is_intra)
	{
		H264Dec_read_intraMB_context(img_ptr, mb_info_ptr, mb_cache_ptr);
	}else
	{
		if (g_list[0][0] == NULL)
		{
			img_ptr->error_flag = TRUE;
			return;
		}

		mb_cache_ptr->read_ref_id_flag[0] = (img_ptr->ref_count[0] > 1);
		mb_cache_ptr->read_ref_id_flag[1] = (img_ptr->ref_count[1] > 1);

		fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);
		
		if (!mb_cache_ptr->is_skipped)
		{
#if 0
			if (IS_INTERMV(mb_info_ptr))
#else
			if (mb_info_ptr->mb_type != 0)
#endif
			{
				H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
			}
			H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
		}else
		{
			mb_cache_ptr->cbp_iqt = 0;
			mb_cache_ptr->cbp_mbc = 0;
		}

		if ( mb_cache_ptr->is_direct)
		{
			direct_mv (img_ptr, mb_info_ptr, mb_cache_ptr);
		}
	}
	
	return;
}

uint32 vsp_mbtype_map[15] = {	H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, H264_INTERMB, 
								H264_INTERMB, H264_INTERMB, H264_IMB4X4, H264_IMB16X16, H264_INTERMB, H264_INTERMB, H264_INTERMB, 
								H264_IPCM
							};
							
PUBLIC void H264Dec_start_vld_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	uint32 up_nnz_y, left_nnz_y;
	uint32 nnz_cb, nnz_cr;
	int32 vsp_mb_type, mb_type = mb_info_ptr->mb_type;
	int32 left_mb_avail, top_mb_avail;
	int32 cbp = mb_info_ptr->cbp;
	int8 *nnz_ref_ptr = mb_cache_ptr->nnz_cache;
	uint32 cmd;
	int	left_mb_type;
	int top_mb_type;
	int vsp_lmb_type;
	int vsp_tmb_type;
	uint32 flush_bits = img_ptr->bitstrm_ptr->bitcnt - img_ptr->bitstrm_ptr->bitcnt_before_vld;

	{
		uint32 nWords = flush_bits/32;
		int32 i = 0;

		flush_bits -= nWords*32;
		cmd = (flush_bits<<24) | 1;
		VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush N bits");	
	#if _CMODEL_
		flush_nbits(flush_bits);
	#endif
		VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);


		if (nWords)
		{
			cmd = (32<<24) | 1;

			for (i = 0; i < nWords; i++)
			{
				VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");
				VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one word");	
		#if _CMODEL_
				flush_nbits(32);
		#endif
				
				VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (2<<24) | (BSM_CFG2_WOFF<<8) | ((1<<7)|BSM_DEBUG_WOFF));
			}
		}
	}

	if (img_ptr->is_cabac)
	{
		DecodingEnvironment* dep = &img_ptr->de_cabac;
		cmd = ((dep->Drange << 16) | (dep->Dvalue & 0x1ff)); 

		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_ARTHI_BS_STATE_OFFSET, cmd, "configure bistream status");

		cmd = ( img_ptr->abv_mb_info->dc_coded_flag <<4) |((mb_info_ptr - 1)->dc_coded_flag);
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CODED_DC_FLAG_OFFSET, cmd, "configure decode_dc_flag");
		mb_cache_ptr->vld_dc_coded_flag = cmd;

		VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (2<<24) |(HVLD_CODED_DC_FLAG_WOFF<<8) |HVLD_ARTHI_BS_STATE_WOFF);
	}
	
	left_mb_type = (mb_info_ptr-1)->mb_type;
	top_mb_type  = img_ptr->abv_mb_info->mb_type;
	vsp_mb_type	=	vsp_mbtype_map[mb_type];
	vsp_lmb_type	=	vsp_mbtype_map[left_mb_type];
	vsp_tmb_type	=	vsp_mbtype_map[top_mb_type];

	up_nnz_y	= (nnz_ref_ptr[4]<<24) | (nnz_ref_ptr[5]<<16) | (nnz_ref_ptr[6]<<8) | (nnz_ref_ptr[7]);
	left_nnz_y	= 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X1 + 3] << 24)  | 
					(nnz_ref_ptr[CTX_CACHE_WIDTH_X2 + 3] << 16)  |
			     		(nnz_ref_ptr[CTX_CACHE_WIDTH_X3 + 3] << 8  )  |
			     		(nnz_ref_ptr[CTX_CACHE_WIDTH_X4 + 3]);
	nnz_cb		= 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 4] << 24) |
					(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 5] << 16) |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X6 + 3] << 8)   |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X7 + 3]);
	nnz_cr		= 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 8] << 24) |
					(nnz_ref_ptr[CTX_CACHE_WIDTH_X5 + 9] << 16) |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X6 + 7] << 8)   |
				 	(nnz_ref_ptr[CTX_CACHE_WIDTH_X7 + 7]);

	//configure mb information
	left_mb_avail = mb_cache_ptr->mb_avail_a;
	top_mb_avail = mb_cache_ptr->mb_avail_b;

	//require to configure entropy decoding type, and left/top MB type
	cmd = (img_ptr->is_cabac << 31) | (vsp_lmb_type << 20) | (vsp_tmb_type << 22) |
		  ((top_mb_avail<<17) | (left_mb_avail << 16) | (cbp << 8) | (vsp_mb_type << 0));
	
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_MB_INFO_OFFSET, cmd, "configure mb information");

	//configure nnz	
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_TOP_NNZ_Y_OFFSET,	up_nnz_y, "configure up_nnz_y");
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_LEFT_NNZ_Y_OFFSET,	left_nnz_y, "configure left_nnz_y");
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_TL_NNZ_CB_OFFSET,	nnz_cb, "configure nnz_cb");
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_TL_NNZ_CR_OFFSET,	nnz_cr, "configure nnz_cr");
	
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, 1, "start vld one mb");

#if _CMODEL_
 	H264VldMBCtr ();
#endif

	VSP_READ_REG_POLL_CQM(VSP_VLD_REG_BASE+HVLD_CTRL_OFFSET, 1, 1, 1,"VLD: polling VLD one MB status");
	
	VSP_WRITE_CMD_INFO((VSP_VLD << CQM_SHIFT_BIT) | (7<<24) | (HVLD_LEFT_NNZ_Y_WOFF<<16)|(HVLD_TOP_NNZ_Y_WOFF<<8) |HVLD_MB_INFO_WOFF);
	VSP_WRITE_CMD_INFO((((1<<7)|HVLD_CTRL_WOFF)<<24) | (HVLD_CTRL_WOFF<<16) | (HVLD_TL_NNZ_CR_WOFF<<8) |HVLD_TL_NNZ_CB_WOFF);

//SW VLD
	if (mb_info_ptr->mb_type != IPCM)
	{
		mb_cache_ptr->cbp_luma_iqt = 0;

		sw_vld_mb (mb_info_ptr, mb_cache_ptr);
		H264Dec_ComputeCBPIqtMbc (mb_info_ptr, mb_cache_ptr);

	}else
	{
		mb_cache_ptr->cbp_iqt = 0;
		mb_cache_ptr->cbp_mbc = 0xffffff;

		//for cabac
		mb_info_ptr->c_ipred_mode = 0;
		mb_info_ptr->cbp = 0x3f;
	
		H264Dec_decode_IPCM_MB(img_ptr, mb_info_ptr, mb_cache_ptr, img_ptr->bitstrm_ptr);
		H264Dec_set_IPCM_nnz(mb_cache_ptr);

		mb_info_ptr->skip_flag = 1;
		//For CABAC decoding of Dquant
		if (img_ptr->is_cabac)
		{
			uint32 nStuffedBits;

			last_dquant=0;
			mb_info_ptr->dc_coded_flag = 7;
			nStuffedBits = arideco_start_decoding(img_ptr);

			if (nStuffedBits)
			{
				uint32 cmd = (nStuffedBits<<24) | 1;

				VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush N bits");	
			#if _CMODEL_
				flush_nbits(nStuffedBits);
			#endif
				VSP_WRITE_CMD_INFO((VSP_BSM << CQM_SHIFT_BIT) | (1<<24) | BSM_CFG2_WOFF);
			}
		}
	}

	img_ptr->bitstrm_ptr->bitcnt_before_vld = img_ptr->bitstrm_ptr->bitcnt;

	return;
}

uint32 s_iqt_mb_type[] = {	IQT_OTHER,	IQT_OTHER,	IQT_OTHER,	IQT_OTHER,	IQT_OTHER, 	IQT_OTHER,	IQT_OTHER,	IQT_OTHER, 
							IQT_OTHER,	IQT_OTHER,	IQT_I16,	IQT_OTHER,	IQT_OTHER,	IQT_OTHER,	IQT_PCM,	IQT_OTHER 
					};

PUBLIC void H264Dec_start_iqt_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 cbp26, mb_info_cmd, qp_info_cmd;
	int32 mb_type	= mb_info_ptr->mb_type;
	int32 cbp		= mb_info_ptr->cbp;
	int32 cbp_uv	= mb_cache_ptr->cbp_uv;
	
	if ((mb_type == IPCM) || (mb_cache_ptr->is_skipped))//IPCM or skip_mb
	{
		cbp26 = 0;
		mb_info_cmd = 0;
	}else
	{
		int32 qp_c;
		
		cbp26 = mb_cache_ptr->cbp_iqt;
		mb_info_cmd = cbp26;
		qp_c = g_QP_SCALER_CR_TBL[IClip(0, 51, mb_info_ptr->qp+img_ptr->chroma_qp_offset)];
		qp_info_cmd = (g_qpPerRem_tbl[mb_info_ptr->qp]<<4) | g_qpPerRem_tbl[qp_c];
		VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+IICT_CFG1_OFF, qp_info_cmd, "configure qp information"); 
		VSP_WRITE_CMD_INFO((VSP_DCT << CQM_SHIFT_BIT) | (1<<24) |IICT_CFG1_WOFF);
	}
	
	//intra and inter(with non-zero coeff) should start idct, otherwise MUST NOT
	if (!mb_info_ptr->is_intra && (cbp26 == 0))
	{
		//MUST!! don't start IICT
		return;
	}	

	mb_info_cmd |= s_iqt_mb_type[mb_type];
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+IICT_CFG0_OFF, mb_info_cmd, "configure mb_type and cbp");
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+IICT_CTRL_OFF, 1, "start iqt");
		
#if _CMODEL_
	iqt_module();
#endif
	
	VSP_READ_REG_POLL_CQM(VSP_DCT_REG_BASE+IICT_CTRL_OFF, 1, 1, 1,"iict: polling iict one MB status");	
	VSP_WRITE_CMD_INFO((VSP_DCT << CQM_SHIFT_BIT) | (3<<24) |(((1<<7)|IICT_CTRL_WOFF)<<16)|(IICT_CTRL_WOFF<<8)|IICT_CFG0_WOFF);
	
	return;
}

//replace H264Dec_more_rbsp_data() function. same as !H264Dec_more_rbsp_data 
PUBLIC uint32 uvlc_startcode_follows (DEC_IMAGE_PARAMS_T *img_ptr)
{
	int32 tmp;
	int32 has_no_zero = 0;
	int32 byte_offset;
	int32 bit_offset;
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	uint32 nDecTotalBits = stream->bitcnt;

	byte_offset = nDecTotalBits/8;
	bit_offset = nDecTotalBits&0x7;

	if (byte_offset < g_nalu_ptr->len -1)
	{
		return FALSE;
	}

	tmp = SHOW_FLC(stream, (8-bit_offset));
	if (tmp == (1<<(7-bit_offset)))
	{
		return TRUE;
	}else
	{
		return FALSE;
	}	
}

PUBLIC int32 H264Dec_exit_macroblock (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	write_back_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

	img_ptr->num_dec_mb++;

	img_ptr->mb_x++;
	if (img_ptr->mb_x == img_ptr->frame_width_in_mbs)
	{
		img_ptr->mb_x = 0;
		img_ptr->mb_y ++;
	}

	if (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)
	{
		img_ptr->curr_slice_ptr->next_header = SOP;
		return TRUE;
	}else
	{
		img_ptr->curr_mb_nr = H264Dec_Fmo_get_next_mb_num(img_ptr->curr_mb_nr, img_ptr);

		if (img_ptr->curr_mb_nr == -1)
		{
			nal_startcode_follows(img_ptr);
			return TRUE;
		}

		if (!nal_startcode_follows(img_ptr))
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
	uint32 cmd;

	if (mb_info_ptr->is_intra)
	{
		int32 mb_avail_a = mb_cache_ptr->mb_avail_a;
		int32 mb_avail_b = mb_cache_ptr->mb_avail_b;
		int32 mb_avail_c = mb_cache_ptr->mb_avail_c;
		int32 mb_avail_d = mb_cache_ptr->mb_avail_d;
		uint32 ipred_cmd0, ipred_cmd1;

		ipred_cmd0 = ((uint32 *)(mb_cache_ptr->mbc_ipred_cmd))[0];
		ipred_cmd1 = ((uint32 *)(mb_cache_ptr->mbc_ipred_cmd))[1];
		
		if (img_ptr->constrained_intra_pred_flag)
		{
			if (mb_avail_a)
			{
				mb_avail_a = (mb_info_ptr-1)->is_intra;
			}

			if (mb_avail_b)
			{
				mb_avail_b = img_ptr->abv_mb_info->is_intra;
			}

			if (mb_avail_c)
			{
				mb_avail_c = (img_ptr->abv_mb_info+1)->is_intra;
			}

			if (mb_avail_d)
			{
				mb_avail_d = (img_ptr->abv_mb_info-1)->is_intra;
			}
		}

		cmd = (MBC_INTRA_MB<<30) |	((mb_cache_ptr->mbc_mb_mode & 0x3) << 28) | (mb_avail_d << 27) | (mb_avail_a << 26) | 
			(mb_avail_b << 25) | (mb_avail_c << 24) | (mb_cache_ptr->cbp_mbc & 0xffffff);
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, cmd, "configure mb information.");

		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD1_OFF, ipred_cmd0, "configure mb intra prediction mode, luma sub-block 0-7");
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD2_OFF, ipred_cmd1, "configure mb intra prediction mode, luma sub-block 8-15");

		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD3_OFF, (mb_info_ptr->c_ipred_mode & 0x3), "configure mb intra prediction mode, block 0-7");

		VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (4<<24) |(MBC_CMD2_WOFF<<16)|(MBC_CMD1_WOFF<<8)|MBC_CMD0_WOFF);
		VSP_WRITE_CMD_INFO(MBC_CMD3_WOFF);
	}else //inter mb
	{
		cmd = (MBC_INTER_MB<<30) |	(mb_cache_ptr->cbp_mbc & 0xffffff);
		VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF, cmd, "configure mb information.");
		VSP_WRITE_CMD_INFO((VSP_MBC << CQM_SHIFT_BIT) | (1<<24) |MBC_CMD0_WOFF);
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
