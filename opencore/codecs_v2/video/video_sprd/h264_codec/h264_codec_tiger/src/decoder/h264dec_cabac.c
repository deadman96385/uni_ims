/******************************************************************************
 ** File Name:    h264dec_cabac.c                                             *
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
  ** 06/26/2012   Leon Li             Modify.                                                                                      *
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

void cabac_new_slice ()
{
  last_dquant=0;
}

LOCAL int32 readMB_typeInfo_CABAC_ISLICE (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	MotionInfoContexts *ctx = img_ptr->curr_slice_ptr->mot_ctx;
	int32 act_ctx;
	int32 act_sym;

	act_ctx = 0;
	if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->mb_type != I4MB_264)
	{
		act_ctx++;
	}

	if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->mb_type != I4MB_264)
	{
		act_ctx++;
	}
		
	if (biari_decode_symbol(img_ptr, ctx->mb_type_contexts[0] + act_ctx) == 0)
	{
		return 0; ///4x4 intra
	}

	if (biari_decode_final (img_ptr) )
	{
		return 25; //IPCM
	}

	act_sym = 1; //I16x16
	act_sym += (12 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[0]+4)); //decoding of AC/no AC
	//decoding of cbp: 0, 1, 2
	if (biari_decode_symbol(img_ptr, ctx->mb_type_contexts[0]+5))
	{
		act_sym += (4 + 4 * (biari_decode_symbol(img_ptr, ctx->mb_type_contexts[0]+6)));
	}

	//decoding of I pred-mode: 0, 1, 2, 3
	act_sym += 2 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[0]+7);
	act_sym += biari_decode_symbol(img_ptr, ctx->mb_type_contexts[0]+8);

	return act_sym;
}

LOCAL int32 readMB_typeInfo_CABAC_PSLICE (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	MotionInfoContexts *ctx = img_ptr->curr_slice_ptr->mot_ctx;
	int32 act_sym;

	if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[1][4]))
	{
		if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[1][7]))	act_sym = 7;
		else	act_sym = 6;
	}else
	{
		if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[1][5]))
		{
			if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[1][7]))	act_sym = 2;
			else act_sym = 3;
		}else
		{
			if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[1][6]))	act_sym = 4;
			else	act_sym = 1;
		}
	}

	if (act_sym <= 6)
	{
		return act_sym;
	}else// additional info for 16x16 Intra-mode
	{ 
		if (biari_decode_final(img_ptr))
		{
			return 31;
		}else
		{
			act_sym += 12 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 8); //decoding of AC/no AC

			//decoding of cbp: 0, 1, 2
			if (biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 9))
			{
				act_sym += (4 + 4 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 9));
			}

			// decoding of I pred-mode: 0,1,2,3
			act_sym += 2 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 10);
			act_sym += biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 10);
			return act_sym;
		}
	}
}

LOCAL int32 readMB_typeInfo_CABAC_BSLICE (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	MotionInfoContexts *ctx = img_ptr->curr_slice_ptr->mot_ctx;
	int32 act_ctx;
	int32 act_sym;

	act_ctx = 0;
	if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->mb_type != 0)
	{
		act_ctx++;
	}

	if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->mb_type != 0)
	{
		act_ctx++;
	}

	if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][act_ctx]))
	{
		if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][4]))
		{
			if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][5]))
			{
				act_sym = 12;

				if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=8;
				if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=4;
				if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=2;

				if (act_sym == 24)	act_sym = 11;
				else if (act_sym == 26)	act_sym = 22;
				else
				{
					if (act_sym == 22)	act_sym = 23;
					if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6])) act_sym += 1;
				}
			}else
			{
				act_sym = 3;

				if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=4;
				if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=2;
				if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym +=1;
			}
		}else
		{
			if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][6]))	act_sym =2;
			else	act_sym = 1;
		}
	}else
	{
		act_sym = 0;
	}

	if ( act_sym <= 23)
	{
		return act_sym;
	}else// additional info for 16x16 Intra-mode
	{ 
		if (biari_decode_final(img_ptr))
		{
			return 48;
		}else
		{
			act_sym += 12 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 8); //decoding of AC/no AC

			//decoding of cbp: 0, 1, 2
			if (biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 9))
			{
				act_sym += (4 + 4 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 9));
			}

			// decoding of I pred-mode: 0,1,2,3
			act_sym += 2 * biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 10);
			act_sym += biari_decode_symbol(img_ptr, ctx->mb_type_contexts[1] + 10);
			return act_sym;
		}
	}
}

PUBLIC void H264Dec_register_readMB_type_func (DEC_IMAGE_PARAMS_T *img_ptr)
{
	if (img_ptr->type == I_SLICE)
		readMB_typeInfo_CABAC = readMB_typeInfo_CABAC_ISLICE;
	else if (img_ptr->type == P_SLICE)
		readMB_typeInfo_CABAC = readMB_typeInfo_CABAC_PSLICE;
	else //B_SLICE
		readMB_typeInfo_CABAC = readMB_typeInfo_CABAC_BSLICE;	
}

int32 readIntraPredMode_CABAC (DEC_IMAGE_PARAMS_T *img_ptr)
{
  	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;
	int act_sym;
	int32 pred;

  	// use_most_probable_mode
  	act_sym = biari_decode_symbol(img_ptr, ctx->ipr_contexts);

  	// remaining_mode_selector
	if (act_sym == 1)
	{
		pred = -1;
	}else
	{
	    pred  = 0;
	    pred |= (biari_decode_symbol(img_ptr, ctx->ipr_contexts+1)     );
	    pred |= (biari_decode_symbol(img_ptr, ctx->ipr_contexts+1) << 1);
	    pred |= (biari_decode_symbol(img_ptr, ctx->ipr_contexts+1) << 2);
	}

	return pred;
}

int32 readCIPredMode_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;
	int32 act_ctx = 0;

	if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->c_ipred_mode != 0)
	{ 
		act_ctx++;
	}

	if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->c_ipred_mode != 0)
	{
		act_ctx++;
	}

	if (biari_decode_symbol(img_ptr, ctx->cipr_contexts + act_ctx ) == 0)
	{
		return 0;
	}

	if (biari_decode_symbol(img_ptr, ctx->cipr_contexts + 3 ) == 0)
	{
		return 1;
	}

	if (biari_decode_symbol(img_ptr, ctx->cipr_contexts + 3 ) == 0)
	{
		return 2;
	}else
	{
		return 3;
	}	
}

int32 readCBP_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	TextureInfoContexts *ctx = curr_slice_ptr->tex_ctx;
	int32 mb_x, mb_y;
	int32 a, b;
	int32 curr_cbp_ctx, curr_cbp_idx;
	int32 cbp = 0;
	int32 cbp_bit;
	int32 msk;

	//  coding of luma part (bit by bit)
	for (mb_y = 0; mb_y < 4; mb_y += 2)
	{
		for (mb_x = 0; mb_x < 4; mb_x += 2)
		{
			if (mb_y == 0)
			{
				if (!mb_cache_ptr->mb_avail_b)
				{
					b = 0;
				}else
				{
					DEC_MB_INFO_T *top_mb_info_ptr = img_ptr->abv_mb_info;
				//	if (top_mb_info_ptr->mb_type == IPCM)
				//	{
				//		b = 0;
				//	}else
					{
						b = ((((top_mb_info_ptr->cbp) & (1 << (2 + mb_x/2))) == 0)? 1: 0);
					}					
				}
			}else
			{
				b = (((cbp & (1<<(mb_x/2))) == 0) ? 1: 0);
			}

			if (mb_x == 0)
			{
				if (mb_cache_ptr->mb_avail_a)
				{
					DEC_MB_INFO_T *left_mb_info_ptr = curr_mb_ptr - 1;
				//	if (left_mb_info_ptr->mb_type == IPCM)
				//	{
				//		a = 0;
				//	}else
					{
						a = (((left_mb_info_ptr->cbp & (1<<(mb_y+1))) == 0)? 1: 0);
					}
				}else
				{
					a = 0;
				}
			}else
			{
				a = (((cbp & (1<<mb_y)) == 0) ? 1 : 0);
			}
		
			curr_cbp_ctx = a + 2*b;
			msk = (1<<(mb_y+mb_x/2));
			cbp_bit = biari_decode_symbol(img_ptr, ctx->cbp_contexts[0]+curr_cbp_ctx);
			if (cbp_bit) cbp += msk;
		}
	}

	if (curr_mb_ptr->is_intra)
	{
		curr_cbp_idx = 0;
	}else
	{
		curr_cbp_idx = 1;
	}

	 // coding of chroma part
	 // CABAC decoding for BinIdx 0
	 b = 0;
	 if (mb_cache_ptr->mb_avail_b)
	 {
		DEC_MB_INFO_T *top_mb_info_ptr = img_ptr->abv_mb_info;
	 //	if (top_mb_info_ptr->mb_type == IPCM)
	 //	{
	 //		b = 1;
	 //	}else
	 	{
	 		b = ((top_mb_info_ptr->cbp) > 15)?1: 0;
	 	}
	 }

	 a = 0;
	 if (mb_cache_ptr->mb_avail_a)
	 {
	 	DEC_MB_INFO_T *left_mb_info_ptr = curr_mb_ptr - 1;
	//	if (left_mb_info_ptr->mb_type == IPCM)
	//	{
	//		a = 1;
	//	}else
		{
			a = ((left_mb_info_ptr->cbp) > 15)?1:0;
		}
	 }

	 curr_cbp_ctx = a + 2*b;
	 cbp_bit = biari_decode_symbol(img_ptr,  ctx->cbp_contexts[1]+curr_cbp_ctx);
	 
	 // CABAC decoding for BinIdx 1 
	 if (cbp_bit) // set the chroma bits
	 {
	 	b = 0;
	 	if (mb_cache_ptr->mb_avail_b)
	 	{
			DEC_MB_INFO_T *top_mb_info_ptr = img_ptr->abv_mb_info;
		 	if (top_mb_info_ptr->mb_type == IPCM)
		 	{
		 		b = 1;
		 	}else
		 	{
		 		if ((top_mb_info_ptr->cbp) > 15)
		 		{
		 			b = (( (top_mb_info_ptr->cbp >> 4) == 2) ? 1 : 0);
		 		}
		 	}
		 }

	 	a = 0;
	 	if (mb_cache_ptr->mb_avail_a)
		 {
		 	DEC_MB_INFO_T *left_mb_info_ptr = curr_mb_ptr - 1;
			if (left_mb_info_ptr->mb_type == IPCM)
			{
				a = 1;
			}else
			{
				if (left_mb_info_ptr->cbp > 15)
				{
          			a = (( (left_mb_info_ptr->cbp >> 4) == 2) ? 1 : 0);
				}
			}
		 }

	 	curr_cbp_ctx = a + 2*b;
	 	cbp_bit = biari_decode_symbol(img_ptr, ctx->cbp_contexts[2] + curr_cbp_ctx);
	 	cbp += (cbp_bit == 1) ? 32:16;
	 }

	if (!cbp)
	{
		last_dquant = 0;
	}

	return cbp;
}

int32 readDquant_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr)
{
  	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int32 act_ctx;
	int32 act_sym;
	int32 dquant;

	act_ctx = ((last_dquant != 0) ? 1 : 0);

	act_sym = biari_decode_symbol(img_ptr, ctx->delta_qp_contexts + act_ctx);
	if (act_sym != 0)
	{
		act_ctx = 2;
		act_sym = unary_bin_decode(img_ptr, ctx->delta_qp_contexts + act_ctx, 1);
		act_sym++;
	}

	dquant = (act_sym+1)/2;
	if ((act_sym & 0x01) == 0)	// lsb is signed bit
	{
		dquant = -dquant;
	}

	last_dquant = dquant;

	return dquant;
}

int32 readMB_skip_flagInfo_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	MotionInfoContexts *ctx = img_ptr->curr_slice_ptr->mot_ctx;
	int32 act_ctx;
	int32 bframe=(img_ptr->type==B_SLICE);
	int32 curr_mb_type;

	act_ctx = 0;
	if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->skip_flag)
		act_ctx++;
	if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->skip_flag)
		act_ctx++;
		
	if (bframe)
	{
		act_ctx += 7;

		if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[2][act_ctx]) == 1)
		{
			curr_mb_type = 0;
			img_ptr->cod_counter=0;
		}else
		{
			curr_mb_type = 1;
		}

		curr_mb_ptr->cbp = curr_mb_type;
	}else
	{
		if (biari_decode_symbol(img_ptr, &ctx->mb_type_contexts[1][act_ctx]) == 1)
		{
			curr_mb_type = 0;
		}else
		{
			curr_mb_type = 1;
		}
	}

	if (!curr_mb_type)
	{
		last_dquant=0;
	}

	curr_mb_ptr->skip_flag = curr_mb_type;

	return curr_mb_type;
}

uint32 readB8_typeInfo_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int32 bframe=(img_ptr->type==B_SLICE);
	int32 b8_type;

	if (!bframe)
	{
		if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[0][1]))
		{
			b8_type = 0;
		}else
		{
			if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[0][3]))
			{
				if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[0][4])) b8_type = 2;
				else	b8_type = 3;
			}else
			{
				b8_type = 1;
			}
		}
	}else //B frame
	{
		if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][0]))
		{
			if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][1]))
			{
				if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][2]))
				{
					if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))
					{
						b8_type = 10;
						if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))	b8_type++;
					}else
					{
						b8_type = 6;
						if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))	b8_type += 2;
						if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))	b8_type++;
					}
				}else
				{
					b8_type = 2;
					if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))	b8_type += 2;
					if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))	b8_type+=1;
				}
			}else
			{
				if (biari_decode_symbol(img_ptr, &ctx->b8_type_contexts[1][3]))	b8_type = 1;
				else	b8_type = 0;
			}
			b8_type++;
		}else
		{
			b8_type = 0;
		}
	}

	return b8_type;
}

int32 readRefFrame_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int8  *ref_idx_ptr = mb_cache_ptr->ref_idx_cache[list] + g_blk_order_map_tbl[blk_id*4];
	int8 *direct_ptr = mb_cache_ptr->direct_cache +g_blk_order_map_tbl[blk_id*4];

	int32 addctx = 0;
	int32 a, b;
	int32 act_ctx;
	int32 act_sym;

	if (img_ptr->type == P_SLICE)
	{
		a = (ref_idx_ptr[-1] > 0) ? 1 : 0;
		b = (ref_idx_ptr[-CTX_CACHE_WIDTH] > 0) ? 1 : 0;		
	}else
	{
		a = (ref_idx_ptr[-1] > 0) ? (!direct_ptr[-1]) : 0;
		b = (ref_idx_ptr[-CTX_CACHE_WIDTH] > 0) ? (!direct_ptr[-CTX_CACHE_WIDTH] ) : 0;
	}

	act_ctx = a + 2*b;
	act_sym = biari_decode_symbol(img_ptr, ctx->ref_no_contexts[addctx]+act_ctx);

	if (act_sym != 0)
	{
		act_sym = 1+unary_bin_decode(img_ptr, ctx->ref_no_contexts[addctx]+4, 1);
	}

	return act_sym;
}
	
int32 readMVD_CABAC (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	MotionInfoContexts *ctx = curr_slice_ptr->mot_ctx;
	int32 a, b;
	int32 act_ctx;
 	int32 mvd_x, mvd_y;
	int32 mv_sign;
	int32 mv_local_err;
	int16 *mvd_cache = mb_cache_ptr->mvd_cache[list];

	//mvd_x
	b = ABS(mvd_cache[(sub_blk_id - CTX_CACHE_WIDTH)*2 + 0]);
	a = ABS(mvd_cache[(sub_blk_id-1)*2 + 0]);

	mv_local_err = a+b;
	if (mv_local_err < 3) act_ctx = 5*0;
	else
	{
		if (mv_local_err > 32)	act_ctx = 5*0+3;
		else	act_ctx = 5*0+2;
	}

	mvd_x = biari_decode_symbol(img_ptr, &ctx->mv_res_contexts[0][act_ctx]);

	if (mvd_x != 0)
	{
		act_ctx = 5*0;
		mvd_x = unary_exp_golomb_mv_decode(img_ptr, ctx->mv_res_contexts[1]+act_ctx, 3);
		mvd_x++;
		mv_sign = biari_decode_symbol_eq_prob(img_ptr);

		if (mv_sign)	mvd_x = -mvd_x;
	}

	//mvd_y
	b = ABS(mvd_cache[(sub_blk_id - CTX_CACHE_WIDTH)*2 + 1]);
	a = ABS(mvd_cache[(sub_blk_id-1)*2 + 1]);

	mv_local_err = a+b;
	if (mv_local_err < 3) act_ctx = 5*1;
	else
	{
		if (mv_local_err > 32)	act_ctx = 5*1+3;
		else	act_ctx = 5*1+2;
	}

	mvd_y = biari_decode_symbol(img_ptr, &ctx->mv_res_contexts[0][act_ctx]);

	if (mvd_y != 0)
	{
		act_ctx = 5*1;
		mvd_y = unary_exp_golomb_mv_decode(img_ptr, ctx->mv_res_contexts[1]+act_ctx, 3);
		mvd_y++;
		mv_sign = biari_decode_symbol_eq_prob(img_ptr);

		if (mv_sign)	mvd_y = -mvd_y;
	}

#if defined(H264_BIG_ENDIAN)
	return  (mvd_x << 16) | (mvd_y & 0xffff);
#else
	return (mvd_y << 16) | (mvd_x & 0xffff);
#endif
}

static const int maxpos       [] = {16, 15, 64, 32, 32, 16,  4, 15};
static const int c1isdc       [] = { 1,  0,  1,  1,  1,  1,  1,  0};

static const int type2ctx_bcbp[] = { 0,  1,  2,  2,  3,  4,  5,  6}; // 7
static const int type2ctx_map [] = { 0,  1,  2,  3,  4,  5,  6,  7}; // 8
static const int type2ctx_last[] = { 0,  1,  2,  3,  4,  5,  6,  7}; // 8
static const int type2ctx_one [] = { 0,  1,  2,  3,  3,  4,  5,  6}; // 7
static const int type2ctx_abs [] = { 0,  1,  2,  3,  3,  4,  5,  6}; // 7

//--- zig-zag scan ----
static const int  pos2ctx_map8x8 [] = { 0,  1,  2,  3,  4,  5,  5,  4,  4,  3,  3,  4,  4,  4,  5,  5,
                                        4,  4,  4,  4,  3,  3,  6,  7,  7,  7,  8,  9, 10,  9,  8,  7,
                                        7,  6, 11, 12, 13, 11,  6,  7,  8,  9, 14, 10,  9,  8,  6, 11,
                                       12, 13, 11,  6,  9, 14, 10,  9, 11, 12, 13, 11 ,14, 10, 12, 14}; // 15 CTX
static const int  pos2ctx_map8x4 [] = { 0,  1,  2,  3,  4,  5,  7,  8,  9, 10, 11,  9,  8,  6,  7,  8,
                                        9, 10, 11,  9,  8,  6, 12,  8,  9, 10, 11,  9, 13, 13, 14, 14}; // 15 CTX
static const int  pos2ctx_map4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 14}; // 15 CTX
static const int* pos2ctx_map    [] = {pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map8x8, pos2ctx_map8x4,
                                       pos2ctx_map8x4, pos2ctx_map4x4, pos2ctx_map4x4, pos2ctx_map4x4};

//===== position -> ctx for LAST =====
static const int  pos2ctx_last8x8 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
                                         2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
                                         3,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  4,
                                         5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8}; //  9 CTX
static const int  pos2ctx_last8x4 [] = { 0,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,
                                         3,  3,  3,  3,  4,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8}; //  9 CTX

static const int  pos2ctx_last4x4 [] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15}; // 15 CTX
static const int* pos2ctx_last    [] = {pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last8x8, pos2ctx_last8x4,
                                        pos2ctx_last8x4, pos2ctx_last4x4, pos2ctx_last4x4, pos2ctx_last4x4};


/*!
 ************************************************************************
 * \brief
 *    Read Significance MAP
 ************************************************************************
 */
int read_significance_map (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr,  int type,  int coeff[])
{
  int   i, sig;
  int   coeff_ctr = 0;
  int   i0        = 0;
  int   i1        = maxpos[type]-1;
  BiContextType  *map_ctx   =img_ptr->curr_slice_ptr->tex_ctx->map_contexts[type2ctx_map [type]];
  BiContextType  *last_ctx  =   img_ptr->curr_slice_ptr->tex_ctx->last_contexts[type2ctx_last[type]];

  if (!c1isdc[type])
  {
    i0++; i1++; coeff--;
  }

  for (i=i0; i<i1; i++) // if last coeff is reached, it has to be significant
  {
    //--- read significance symbol ---
      sig = biari_decode_symbol   (img_ptr, map_ctx + pos2ctx_map[type][i]);
	
    if (sig)
    {
      coeff[i] = 1;
      coeff_ctr++;
      //--- read last coefficient symbol ---
      if (biari_decode_symbol (img_ptr, last_ctx + pos2ctx_last[type][i]))
      {
        for (i++; i<i1+1; i++) coeff[i] = 0;
      }
    }
    else
    {
      coeff[i] = 0;
    }
  }
  //--- last coefficient must be significant if no last symbol was received ---
  if (i<i1+1)
  {
    coeff[i] = 1;
    coeff_ctr++;
  }

  return coeff_ctr;
}

/*!
 ************************************************************************
 * \brief
 *    Exp-Golomb decoding for LEVELS
 ***********************************************************************
 */
unsigned int unary_exp_golomb_level_decode(DEC_IMAGE_PARAMS_T *img_ptr, BiContextType *bi_ctx_ptr)
{
  unsigned int l,k;
  unsigned int symbol;
  unsigned int exp_start = 13;

  symbol = biari_decode_symbol(img_ptr, bi_ctx_ptr );

  if (symbol==0)
    return 0;
  else
  {
    symbol=0;
    k=1;
    do
    {
      l=biari_decode_symbol(img_ptr, bi_ctx_ptr);
      symbol++;
      k++;
    }
    while((l!=0) && (k!=exp_start));
    if (l!=0)
      symbol += exp_golomb_decode_eq_prob(img_ptr,0)+1;
    return symbol;
  }
}

/*!
 ************************************************************************
 * \brief
 *    Read Levels
 ************************************************************************
 */
void read_significant_coefficients (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr,  int type,  int coeff[])
{
  int   i, ctx;
  int   c1 = 1;
  int   c2 = 0;

  for (i=maxpos[type]-1; i>=0; i--)
  {
    if (coeff[i]!=0)
    {
      ctx = mmin (c1,4);
      coeff[i] += biari_decode_symbol (img_ptr, img_ptr->curr_slice_ptr->tex_ctx->one_contexts[type2ctx_one[type]] + ctx);
      if (coeff[i]==2)
      {
        ctx = mmin (c2,4);
        coeff[i] += unary_exp_golomb_level_decode (img_ptr, img_ptr->curr_slice_ptr->tex_ctx->abs_contexts[type2ctx_abs[type]]+ctx);
        c1=0;
        c2++;
      }
      else if (c1)
      {
        c1++;
      }
      if (biari_decode_symbol_eq_prob(img_ptr))
      {
        coeff[i] *= -1;
      }
    }
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
