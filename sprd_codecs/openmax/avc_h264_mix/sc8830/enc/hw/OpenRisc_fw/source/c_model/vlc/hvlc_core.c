#include "sc6800x_video_header.h"

LOCAL void bs_write_vlc( vlc_t v )
{
	write_nbits(v.i_bits, v.i_size, 1);
#ifdef TV_OUT
//	FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
#endif
}

int32 x264_mb_predict_non_zero_code( ENC_IMAGE_PARAMS_T *img_ptr, int32 idx )
{
    int32 za = g_mb_cache_ptr->nnz[x264_scan8[idx] - 1];
    int32 zb = g_mb_cache_ptr->nnz[x264_scan8[idx] - 8];

    int32 i_ret = za + zb;

    if( i_ret < 0x80 )
    {
        i_ret = ( i_ret + 1 ) >> 1;
    }
    return i_ret & 0x7f;
}

/****************************************************************************
 * block_residual_write_cavlc:
 ****************************************************************************/
void block_residual_write_cavlc( ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, int32 i_idx, int16 *coef, int32 i_count )
{
    int32 level[16], run[16];
    int32 i_total, i_trailing;
    int32 i_total_zero;
    int32 i_last;
    int32 i_sign;

    int32 i;
    int32 i_zero_left;
    int32 i_suffix_length;

	
	int32 nC;
//	uint32 code;
//	uint32 len;
	vlc_t  v;

    /* first find i_last */
    i_last = i_count - 1;
    while( i_last >= 0 && coef[i_last] == 0 )
    {
        i_last--;
    }

    i_sign = 0;
    i_total = 0;
    i_trailing = 0;
    i_total_zero = 0;

    if( i_last >= 0 )
    {
        int32 b_trailing = 1;
        int32 idx = 0;

        /* level and run and total */
        while( i_last >= 0 )
        {
            level[idx] = coef[i_last--];

            run[idx] = 0;
            while( i_last >= 0 && coef[i_last] == 0 )
            {
                run[idx]++;
                i_last--;
            }

            i_total++;
            i_total_zero += run[idx];

            if( b_trailing && abs( level[idx] ) == 1 && i_trailing < 3 )
            {
                i_sign <<= 1;
                if( level[idx] < 0 )
                {
                    i_sign |= 0x01;
                }

                i_trailing++;
            }
            else
            {
                b_trailing = 0;
            }

            idx++;
        }
    }
#ifdef VLC_TV_SPLIT
	FPRINTF_VLC (g_vlc_trace_token_fp, "// Frame=%d, mb_x=%d, mb_y=%d, block=%d\n", g_nFrame_enc, img_ptr->mb_x, img_ptr->mb_y, i_idx);
	FPRINTF_VLC (g_vlc_trace_token_fp, "%08x\n", i_total_zero);
	FPRINTF_VLC (g_vlc_trace_token_fp, "%08x\n", i_total);
	FPRINTF_VLC (g_vlc_trace_token_fp, "%08x\n", i_trailing);
#endif

    /* total/trailing */
    if( i_idx == BLOCK_INDEX_CHROMA_DC )
    {
		nC = -1;
		v = x264_coeff_token[4][i_total*4+i_trailing];
        bs_write_vlc( x264_coeff_token[4][i_total*4+i_trailing] );
    }
    else
    {
        /* x264_mb_predict_non_zero_code return 0 <-> (16+16+1)>>1 = 16 */
        int32 ct_index[17] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,3 };

        if( i_idx == BLOCK_INDEX_LUMA_DC )
        {
            nC = x264_mb_predict_non_zero_code( img_ptr, 0 );
        }
        else
        {
            nC = x264_mb_predict_non_zero_code( img_ptr, i_idx );
        }

		v = x264_coeff_token[ct_index[nC]][i_total*4+i_trailing];
        bs_write_vlc( x264_coeff_token[ct_index[nC]][i_total*4+i_trailing] );
    }
#ifdef TV_OUT
	mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
	PrintfCavlcOffset(mb_info_ptr, "coeff_token");
	mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif

	//FPRINTF (g_hvlc_event_fp, "coeff_token  code_word: %0x, code_len: %d\n", v.i_bits, v.i_size);
	FPRINTF (g_hvlc_event_fp, "coeff_token, total_coeff: %d, trailing_one: %d, nc: %d,  code_word: %0x, code_len: %d\n", 
		i_total, i_trailing, nC, v.i_bits, v.i_size);

    if( i_total <= 0 )
    {
#ifdef VLC_TV_SPLIT
		FPRINTF_VLC (g_vlc_trace_level_fp, "// Frame=%d, mb_x=%d, mb_y=%d, block=%d num_level=%d\n", g_nFrame_enc, img_ptr->mb_x, img_ptr->mb_y, i_idx, 0);
		FPRINTF_VLC (g_vlc_trace_run_fp, "// Frame=%d, mb_x=%d, mb_y=%d, block=%d\n", g_nFrame_enc, img_ptr->mb_x, img_ptr->mb_y, i_idx);
		for(i=0; i<16; i++)
		{
			FPRINTF_VLC (g_vlc_trace_level_fp, "%08x\n", 0);
			FPRINTF_VLC (g_vlc_trace_run_fp, "%08x\n", 0);
		}
#endif
        return;
    }

    i_suffix_length = i_total > 10 && i_trailing < 3 ? 1 : 0;
    if( i_trailing > 0 )
    {
        write_nbits( i_sign, i_trailing, 1);
#ifdef TV_OUT
//		FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
		mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
		PrintfCavlcOffset(mb_info_ptr, "trailing_1s");
		mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif
		FPRINTF (g_hvlc_event_fp, "trailing_one  trailing_one_sign; %0x, trailing_one_num: %d\n", i_sign, i_trailing);
    }
#ifdef VLC_TV_SPLIT
	FPRINTF_VLC (g_vlc_trace_level_fp, "// Frame=%d, mb_x=%d, mb_y=%d, block=%d num_level=%d\n", g_nFrame_enc, img_ptr->mb_x, img_ptr->mb_y, i_idx, i_total-i_trailing);
#endif
    for( i = i_trailing; i < i_total; i++ )
    {
        int		i_level_code;
		uint32	code = 0;
		int		code_len;
		uint32	abs_lev_code;
		uint32	mask;

        /* calculate level code */
        if( level[i] < 0 )
        {
            i_level_code = -2*level[i] - 1;
        }
        else /* if( level[i] > 0 ) */
        {
            i_level_code = 2 * level[i] - 2;
        }


        if( i == i_trailing && i_trailing < 3 )
        {
            i_level_code -=2; /* as level[i] can't be 1 for the first one if i_trailing < 3 */
        }
		
		abs_lev_code = i_level_code;
#ifdef VLC_TV_SPLIT
		FPRINTF_VLC (g_vlc_trace_level_fp, "%08x\n", abs_lev_code);
#endif
		
        if( ( i_level_code >> i_suffix_length ) < 14 )
        {
			v = x264_level_prefix[i_level_code >> i_suffix_length];
            bs_write_vlc( x264_level_prefix[i_level_code >> i_suffix_length] );
#ifdef TV_OUT
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_prefix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif

			code	 = v.i_bits;
			code_len = v.i_size;
            if( i_suffix_length > 0 )
            {
                write_nbits( i_level_code, i_suffix_length, 1);
#ifdef TV_OUT
//				FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
				mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
				PrintfCavlcOffset(mb_info_ptr, "level_suffix");
				mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif

				mask = (1 << i_suffix_length) - 1;

				code = (code << i_suffix_length) | (i_level_code & mask);
				code_len = code_len + i_suffix_length;
            }
        }
        else if( i_suffix_length == 0 && i_level_code < 30 )
        {
            bs_write_vlc( x264_level_prefix[14] );
#ifdef TV_OUT
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_prefix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif
            write_nbits( i_level_code - 14, 4,  1);
#ifdef TV_OUT
//			FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_suffix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif
			
			mask	 = 0xf;
			code	 = (x264_level_prefix[14].i_bits << 4) | ((i_level_code - 14) & mask);
			code_len = x264_level_prefix[14].i_size + 4;
        }
        else if( i_suffix_length > 0 && ( i_level_code >> i_suffix_length ) == 14 )
        {
            bs_write_vlc( x264_level_prefix[14] );
#ifdef TV_OUT
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_prefix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif
            write_nbits( i_level_code, i_suffix_length,  1);
#ifdef TV_OUT
//			FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_suffix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif

			mask	 = (1 << i_suffix_length) - 1;
			code	 = (x264_level_prefix[14].i_bits << i_suffix_length) | (i_level_code & mask);
			code_len = x264_level_prefix[14].i_size + i_suffix_length;
        }
        else
        {
            bs_write_vlc(x264_level_prefix[15] );
#ifdef TV_OUT
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_prefix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif
            i_level_code -= 15 << i_suffix_length;

			code = x264_level_prefix[15].i_bits;
			code_len = x264_level_prefix[15].i_size;

            if( i_suffix_length == 0 )
            {
                i_level_code -= 15;
            }

            if( i_level_code >= ( 1 << 12 ) || i_level_code < 0 )
            {
                printf("OVERFLOW levelcode=%d\n", i_level_code );
            }

            write_nbits( i_level_code, 12, 1 );    /* check overflow ?? */
#ifdef TV_OUT
//			FPRINTF_VLC (g_bsm_totalbits_fp, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
			mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
			PrintfCavlcOffset(mb_info_ptr, "level_suffix");
			mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif

			mask = (1<<12) - 1;
			code = (code << 12) | (i_level_code & mask);
			code_len = code_len + 12;
        }

		FPRINTF (g_hvlc_event_fp, "level: %08x code_word: %0x, code_len: %d\n", abs_lev_code, code, code_len);

        if( i_suffix_length == 0 )
        {
            i_suffix_length++;
        }
        if( ABS( level[i] ) > ( 3 << ( i_suffix_length - 1 ) ) && i_suffix_length < 6 )
        {
            i_suffix_length++;
        }
    }
#ifdef VLC_TV_SPLIT
	for(i=0; i<(16-i_total+i_trailing); i++)
		FPRINTF_VLC (g_vlc_trace_level_fp, "%08x\n", 0);
#endif

    if( i_total < i_count )
    {
        if( i_idx == BLOCK_INDEX_CHROMA_DC )
        {
			v = x264_total_zeros_dc[i_total-1][i_total_zero];
            bs_write_vlc( x264_total_zeros_dc[i_total-1][i_total_zero] );
        }
        else
        {
			v = x264_total_zeros[i_total-1][i_total_zero];
            bs_write_vlc( x264_total_zeros[i_total-1][i_total_zero] );
        }
#ifdef TV_OUT
		mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
		PrintfCavlcOffset(mb_info_ptr, "total_zeros");
		mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif
		FPRINTF (g_hvlc_event_fp, "total_zeros: %d, code_word: %0x, code_len: %d\n", i_total_zero, v.i_bits, v.i_size);
    }

#ifdef VLC_TV_SPLIT
	FPRINTF_VLC (g_vlc_trace_run_fp, "// Frame=%d, mb_x=%d, mb_y=%d, block=%d\n", g_nFrame_enc, img_ptr->mb_x, img_ptr->mb_y, i_idx);
#endif
    for( i = 0, i_zero_left = i_total_zero; i < i_total - 1; i++ )
    {
        int i_zl;

        if( i_zero_left <= 0 )
        {
            break;
        }

        i_zl = mmin( i_zero_left - 1, 6 );

#ifdef VLC_TV_SPLIT
		FPRINTF_VLC (g_vlc_trace_run_fp, "%08x\n", run[i]);
#endif
		v = x264_run_before[i_zl][run[i]];
        bs_write_vlc( x264_run_before[i_zl][run[i]] );
#ifdef TV_OUT
		mb_info_ptr->cavlc_end_bits = g_bsm_reg_ptr->TOTAL_BITS;
		PrintfCavlcOffset(mb_info_ptr, "run_before");
		mb_info_ptr->cavlc_start_bits = g_bsm_reg_ptr->TOTAL_BITS;
#endif

		FPRINTF (g_hvlc_event_fp, "run_before: %d, code_word: %0x, code_len: %d\n", run[i], v.i_bits, v.i_size);

        i_zero_left -= run[i];
    }
#ifdef VLC_TV_SPLIT
	for(; i<16; i++)
		FPRINTF_VLC (g_vlc_trace_run_fp, "%08x\n", 0);
#endif
}

void macroblock_luma_write_cavlc( ENC_IMAGE_PARAMS_T *img_ptr, ENC_MB_MODE_T *mb_info_ptr, int32 i8start, int32 i8end )
{
    int32 i8, i4/*, i*/;
 
    for( i8 = i8start; i8 <= i8end; i8++ )
	{
        if( mb_info_ptr->i_cbp_luma & (1 << i8) )
		{
            for( i4 = 0; i4 < 4; i4++ )
			{
				FPRINTF (g_hvlc_event_fp, "luma ac, blk_id: %d\n", (i4+i8*4));
				block_residual_write_cavlc( img_ptr, mb_info_ptr, i4+i8*4, (int16*)(vsp_dct_io_0)+(i4+i8*4)*16, 16 );
			}
		}
	}
}
