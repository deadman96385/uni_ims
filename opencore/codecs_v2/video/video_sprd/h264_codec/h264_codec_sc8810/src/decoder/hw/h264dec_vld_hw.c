/******************************************************************************
 ** File Name:    h264dec_vld.c                                               *
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
#include "sc8810_video_header.h"
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

/**
  * decodes a residual block.
  * @param n block index
  * @param scantable scantable
  * @param max_coeff number of coefficients in the block
  * @return < 0 if an error occurred
  */
int readCoeff4x4_CAVLC_hw (DEC_MB_CACHE_T *mb_cache_ptr, int32 n, int32 max_coeff)
{
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	static const int coeff_token_table_index[17] = {0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3};
	int level[16];
	int zeros_left, coeff_num, coeff_token, total_coeff, i, /*j,*/ trailing_ones, run_before;

	if (n == CHROMA_DC_BLOCK_INDEX)
	{
		int32 idx, n;

		idx = BITSTREAMSHOWBITS(stream, CHROMA_DC_COEFF_TOKEN_VLC_BITS);
		coeff_token = chroma_dc_coeff_token_vlc.table[idx][0];
		n 		    = chroma_dc_coeff_token_vlc.table[idx][1];	
		BITSTREAMFLUSHBITS(stream, ((uint32) n));
		
		total_coeff = coeff_token>>2;
	}else
	{
		if (n == LUMA_DC_BLOCK_INDEX)
		{
			total_coeff = pred_non_zero_count (mb_cache_ptr, 0);
			total_coeff = IClip(0, 16, total_coeff);
			coeff_token = get_vlc_s16(stream, coeff_token_vlc[coeff_token_table_index[total_coeff]].table, COEFF_TOKEN_VLC_BITS);
			total_coeff = coeff_token>>2;
		}else
		{
			total_coeff = pred_non_zero_count (mb_cache_ptr, n);
			total_coeff = IClip(0, 16, total_coeff);
			coeff_token = get_vlc_s16(stream, coeff_token_vlc[coeff_token_table_index[total_coeff]].table, COEFF_TOKEN_VLC_BITS);
			total_coeff = coeff_token>>2;
			mb_cache_ptr->nnz_cache[g_blk_order_map_tbl[n] ] = total_coeff;
		}
	}

	//FIXME set last_non_zero
	if (total_coeff == 0)
		return 0;

	if (total_coeff > max_coeff || total_coeff < 0)
	{
		return -1;
	}

	trailing_ones = coeff_token&3;

	for (i = 0; i < trailing_ones; i++)
	{
		level[i] = 1 - 2*READ_BITS1(stream);
	}

	if (i < total_coeff)
	{
		int level_code, mask;
		int suffix_length = total_coeff > 10 && trailing_ones < 3;
		int prefix = get_level_prefix(stream);

		//first coefficient has suffix_length equal to 0 or 1
		if (prefix < 14)
		{	//FIXME try to build a large unified VLC table for all this
			if (suffix_length)
			{
				level_code = (prefix<<suffix_length) + READ_FLC(stream, suffix_length);	//part
			}else
			{
				level_code = (prefix<<suffix_length);	//part
			}
		}else if (prefix == 14)
		{
			if (suffix_length)
			{
				level_code = (prefix<<suffix_length) + READ_FLC(stream, suffix_length);	//part
			}else
			{
				level_code = prefix + READ_FLC(stream, 4);	//part
			}
		}else if (prefix == 15)
		{
			level_code = (prefix<<suffix_length) + READ_FLC(stream, 12);	//part
			if (suffix_length == 0)	level_code += 15;	//FIXME doesn't make (much) sense
		}else
		{
			return -1;
		}

		if (trailing_ones < 3)	level_code += 2;

		suffix_length = 1;
		if (level_code > 5)
			suffix_length++;
		mask = -(level_code&1);
		level[i] = (((2+level_code)>>1) ^ mask) - mask;
		i++;

		//remaining coefficients have suffix_length >0
		for (; i <total_coeff; i++)
		{
			static const int suffix_limit[7] = {0, 5, 11, 23, 47, 95, (1<<30)};
			prefix = get_level_prefix(stream);
			if (prefix<15)
			{
				level_code = (prefix<<suffix_length) + READ_FLC(stream, suffix_length);
			}else if (prefix ==  15)
			{
				level_code = (prefix<<suffix_length) + READ_FLC(stream, 12);
			}else
			{
				return -1;
			}
			mask = -(level_code&1);
			level[i] = (((2+level_code)>>1) ^ mask) - mask;
			if (level_code > suffix_limit[suffix_length])
				suffix_length++;
		}
	}

	if (total_coeff == max_coeff)
		zeros_left = 0;
	else
	{
		int32 idx, nn;
		if (n == CHROMA_DC_BLOCK_INDEX)
		{
			idx = BITSTREAMSHOWBITS(stream, CHROMA_DC_TOTAL_ZEROS_VLC_BITS);
			zeros_left = chroma_dc_total_zeros_vlc[total_coeff-1].table[idx][0];
			nn 		 = chroma_dc_total_zeros_vlc[total_coeff-1].table[idx][1];
			BITSTREAMFLUSHBITS(stream, ((uint32)nn));
		}
		else
		{
			idx = BITSTREAMSHOWBITS(stream, TOTAL_ZEROS_VLC_BITS);
			zeros_left = total_zeros_vlc[total_coeff-1].table[idx][0];
			nn 		 = total_zeros_vlc[total_coeff-1].table[idx][1];
			BITSTREAMFLUSHBITS(stream, ((uint32)nn));
		}
	}

	coeff_num = zeros_left + total_coeff - 1;
//	j = scantable[coeff_num];
	if (n > 24)
	{
//		block[j] = level[0];
		for (i = 1; i < total_coeff; i++)
		{
			if (zeros_left <= 0)
				run_before = 0;
			else if (zeros_left < 7)
			{
				int32 idx, nn;
				idx = BITSTREAMSHOWBITS(stream, RUN_VLC_BITS);
				run_before = run_vlc[zeros_left-1].table[idx][0];
				nn 		   = run_vlc[zeros_left-1].table[idx][1];
				BITSTREAMFLUSHBITS(stream, ((uint32)nn));
			}else
			{
				run_before = get_vlc_s8(stream, run7_vlc.table, RUN7_VLC_BITS);
			}
			zeros_left -= run_before;
			coeff_num -= 1+run_before;
//			j = scantable[coeff_num];

//			block[j] = level[i];
		}
	}else
	{
//		block[j] = (level[0] * qmul[j] + 32)>>6;
		for (i = 1; i < total_coeff; i++)
		{
			if (zeros_left <= 0)
				run_before = 0;
			else if (zeros_left < 7)
			{
				//run_before = get_vlc_s8(stream, run_vlc[zeros_left-1].table, RUN_VLC_BITS, 1);
				int32 idx, nn;
				idx = BITSTREAMSHOWBITS(stream, RUN_VLC_BITS);
				run_before = run_vlc[zeros_left-1].table[idx][0];
				nn 		   = run_vlc[zeros_left-1].table[idx][1];
				BITSTREAMFLUSHBITS(stream, ((uint32)nn));				
			}else
			{
				run_before = get_vlc_s8(stream, run7_vlc.table, RUN7_VLC_BITS);
			}
			zeros_left -= run_before;
			coeff_num -= 1+run_before;
//			j = scantable[coeff_num];

//			block[j] = (level[i] * qmul[j] + 32)>>6;
		}		
	}

	if (zeros_left < 0)
	{
		return -1;
	}

	return 1;
}

void decode_LUMA_AC_hw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * pMBCache)
{
	int startPos = 0;
	int maxCoeff = 16;
	int blk4x4;
	int numCoeff;
	int cbp = currMB->cbp;
	
	if (currMB->mb_type == I16MB)
	{
		maxCoeff = 15;
		startPos = 1;
	}	

	for (blk4x4 = 0; blk4x4 < 16; blk4x4++)
	{
		if ( cbp & (1 << (blk4x4/4)) )
		{
			numCoeff = readCoeff4x4_CAVLC_hw (pMBCache, blk4x4, maxCoeff);
			if(g_image_ptr->error_flag)
			{
				g_image_ptr->return_pos2 |= (1<<13);
				return;
			}
			pMBCache->cbp_luma_iqt |= (numCoeff<<blk4x4);
		}else
		{
			blk4x4 += 3;  //go to next 8x8 block 
		}
	}
	
	return;
}

void decode_CHROMA_AC_hw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * pMBCache)
{
	int blkIndex;

	for (blkIndex = 0; blkIndex < 8; blkIndex++)
	{
		if (readCoeff4x4_CAVLC_hw (pMBCache, blkIndex+16, 15))
		{
			pMBCache->cbp_uv |= 0x1 << blkIndex;
		}
	}

	return;
}

void decode_mb_cavlc_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	if (mb_info_ptr->mb_type == I16MB)
	{
		readCoeff4x4_CAVLC_hw(mb_cache_ptr, LUMA_DC_BLOCK_INDEX,  16);//luma DC
	}

	decode_LUMA_AC_hw (mb_info_ptr, mb_cache_ptr);

	if (mb_info_ptr->cbp > 15)
	{//CHROMA_DC
		if (readCoeff4x4_CAVLC_hw (mb_cache_ptr, CHROMA_DC_BLOCK_INDEX, 4))
			mb_cache_ptr->cbp_uv |= 0xf;	
		
		if (readCoeff4x4_CAVLC_hw (mb_cache_ptr, CHROMA_DC_BLOCK_INDEX, 4))	
			mb_cache_ptr->cbp_uv |= 0xf0;	
	}

	if (mb_info_ptr->cbp > 31)
	{
		decode_CHROMA_AC_hw (mb_info_ptr, mb_cache_ptr); 
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
