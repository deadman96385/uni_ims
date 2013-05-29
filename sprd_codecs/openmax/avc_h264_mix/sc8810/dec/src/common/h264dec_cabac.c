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

void cabac_new_slice ()
{
	last_dquant=0;
}

int8 decode_cabac_mb_transform_size (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 a, b;
	int32 act_ctx;
	int32 lmb_avail = mb_cache_ptr->mb_avail_a;
	int32 tmb_avail = mb_cache_ptr->mb_avail_b;

	if (!tmb_avail)
	{
		b = 0;
	}else
	{
		b = img_ptr->abv_mb_info->transform_size_8x8_flag;
	}

	if (!lmb_avail)
	{
		a = 0;
	}else
	{
		a = (curr_mb_ptr - 1)->transform_size_8x8_flag;
	}

	act_ctx = a + b;
	return get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[399+act_ctx] );
}

int32 decode_cabac_intra_mb_type(DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 ctx_base, int32 intra_slice)
{
	uint8 *state = &img_ptr->cabac_state[ctx_base];
	int32 mb_type;
	int32 act_ctx;

	if (intra_slice)
	{
		act_ctx = 0;
		if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->mb_type != I4MB_264)
		{
			act_ctx++;
		}

		if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->mb_type != I4MB_264)
		{
			act_ctx++;
		}
			
		if (get_cabac(&img_ptr->cabac, &state[act_ctx]) == 0)
		{
			return 0; ///4x4 intra
		}

		state += 2;
	}else
	{
		if (get_cabac(&img_ptr->cabac, &state[0]) == 0)
		{
			return 0; /*I4X4*/
		}
	}

	if (get_cabac_terminate(img_ptr))
	{
		return 25;	/*PCM*/
	}

	mb_type = 1; /* I16x16 */
    mb_type += 12 * get_cabac( &img_ptr->cabac, &state[1] ); /* cbp_luma != 0 */
    if( get_cabac( &img_ptr->cabac, &state[2] ) ) /* cbp_chroma */
    	mb_type += 4 + 4 * get_cabac( &img_ptr->cabac, &state[2+intra_slice] );
   	mb_type += 2 * get_cabac( &img_ptr->cabac, &state[3+intra_slice] );
   	mb_type += 1 * get_cabac( &img_ptr->cabac, &state[3+2*intra_slice] );

    return mb_type;
}

int32 decode_cabac_mb_intra4x4_pred_mode (DEC_IMAGE_PARAMS_T *img_ptr)
{
  	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int act_sym;
	int32 pred;

  	// use_most_probable_mode
  	act_sym = get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[68] );

  	// remaining_mode_selector
	if (act_sym == 1)
	{
		pred = -1;
	}else
	{
	    pred  = 0;
	    pred |= (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[69])     );
	    pred |= (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[69]) << 1);
	    pred |= (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[69]) << 2);
	}

	return pred;
}

int32 decode_cabac_mb_chroma_pre_mode (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int32 act_ctx = 0;

	if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->c_ipred_mode != 0)
	{ 
		act_ctx++;
	}

	if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->c_ipred_mode != 0)
	{
		act_ctx++;
	}

	if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[64+act_ctx] ) == 0)
	{
		return 0;
	}

	if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[64+3] ) == 0)
	{
		return 1;
	}

	if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[64+3] ) == 0)
	{
		return 2;
	}else
	{
		return 3;
	}	
}

int32 decode_cabac_mb_cbp (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
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
			cbp_bit = get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[73 + curr_cbp_ctx]);
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
	 cbp_bit = get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[77 + curr_cbp_ctx]);
	 
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

	 	curr_cbp_ctx = 4+ a + 2*b;
	 	cbp_bit = get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[77 + curr_cbp_ctx]);
	 	cbp += (cbp_bit == 1) ? 32:16;
	 }

	if (!cbp)
	{
		last_dquant = 0;
	}

	return cbp;
}

int32 decode_cabac_mb_dqp (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_INFO_T *curr_mb_ptr)
{
	int32 act_ctx;
	int32 act_sym = 0;
	int32 dquant;

	act_ctx = ((last_dquant != 0) ? 1 : 0);

	while( get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[60 + act_ctx] ) ) 
	{
	        if( act_ctx < 2 )
	            act_ctx = 2;
	        else
	            act_ctx = 3;
	        act_sym++;
	        if(act_sym > 102) //prevent infinite loop
	            return (-1<<30);
    }

    if( act_sym&0x01 )
        dquant = (act_sym + 1)/2;
    else
        dquant = -(act_sym + 1)/2;
	
	last_dquant = dquant;

	return dquant;
}

uint32 decode_cabac_mb_sub_type (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr)
{
	if (img_ptr->type != B_SLICE)
	{
		if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[21]))
		{
			return 0;	/*8x8*/
		}
		
		if (!get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[22]))
		{
			return 1;	/*8x4*/
		}

		if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[23]))
		{
			return 2;	/*8x4*/
		}

		return 3;	/*4x4*/
	}else
	{
		int32 b8_type;

		if (!get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[36]))
		{
			return 0;	/* B_Direct_8x8 */
		}

		if (!get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[37]))
		{
			return 1 + get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[39]);	/* B_L0_8x8, B_L1_8x8 */
		}

		b8_type = 3;
		if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[38]))
		{
			if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[39]))
			{
				return 11 + get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[39]);	/* B_L1_4x4, B_Bi_4x4 */
			}
			b8_type += 4;
		}

		b8_type += 2*get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[39]);
		b8_type += get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[39]);

		return b8_type;
	}
}

int32 decode_cabac_mb_ref (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list)
{
	DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
	int8  *ref_idx_ptr = mb_cache_ptr->ref_idx_cache[list] + g_blk_order_map_tbl[blk_id*4];
	int8 *direct_ptr = mb_cache_ptr->direct_cache +g_blk_order_map_tbl[blk_id*4];

	int32 a, b;
	int32 act_ctx;
	int32 act_sym = 0;

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
	while(get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[54+act_ctx]))
	{
		act_sym ++;
		if (act_ctx < 4)
		{
			act_ctx = 4;
		}else
		{
			act_ctx = 5;
		}

		if(act_sym >= 32 /*h->ref_list[list]*/)
		{
            		return 0; //FIXME we should return -1 and check the return everywhere
        }
	}

	return act_sym;
}
	
int32 decode_cabac_mb_mvd (DEC_IMAGE_PARAMS_T *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list)
{
	int16 *mvd_cache = mb_cache_ptr->mvd_cache[list];
	int32 a, b;
	int32 mv_local_err;
	int32 act_ctx;
	uint32 uv;
	int32 ctxbase = 40;
	int32 mvd[2];

	for (uv = 0; uv < 2; uv++)
	{
		b = ABS(mvd_cache[(sub_blk_id - CTX_CACHE_WIDTH)*2 + uv]);
		a = ABS(mvd_cache[(sub_blk_id-1)*2 + uv]);
	
		mv_local_err = a+b;
		if (mv_local_err < 3)
		{
			act_ctx = 0;
		}else if (mv_local_err > 32)
		{
			act_ctx = 2;
		}else
		{
			act_ctx = 1;
		}

		if (!get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[ctxbase+act_ctx]))
		{
			mvd[uv] = 0;
		}else
		{
			mvd[uv] = 1;
			act_ctx = 3;
			while (mvd[uv] < 9 && get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[ctxbase+act_ctx]))
			{
				mvd[uv]++;
				if (act_ctx < 6)
				{
					act_ctx++;
				}
			}

			if (mvd[uv] >= 9)
			{
				int32 k = 3;
				while(get_cabac_bypass(&img_ptr->cabac))
				{
					mvd[uv] += 1<<k;
					k++;
					if (k > 24)
					{
						img_ptr->error_flag |= ER_BSM_ID;
						return -(1<<30);
					}
				}

				while(k--)
				{
					if (get_cabac_bypass(&img_ptr->cabac))
					{
						mvd[uv] += (1<<k);
					}
				}
			}
		
			mvd[uv] = get_cabac_bypass_sign(&img_ptr->cabac, -mvd[uv]);
		}

		ctxbase = 47;
	}

#if defined(H264_BIG_ENDIAN)
	return  (mvd[0] << 16) | (mvd[1] & 0xffff);
#else
	return (mvd[1] << 16) | (mvd[0] & 0xffff);
#endif
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
