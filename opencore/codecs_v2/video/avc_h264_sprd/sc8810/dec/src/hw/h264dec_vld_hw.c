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

#define DECODE_SIGNIFICANCE( coefs, sig_off, last_off ) \
        for(last= 0; last < coefs; last++) { \
            uint8 *sig_ctx = significant_coeff_ctx_base + sig_off; \
            if( get_cabac( CC, sig_ctx )) { \
                uint8 *last_ctx = last_coeff_ctx_base + last_off; \
                index[coeff_count++] = last; \
                if( get_cabac( CC, last_ctx ) ) { \
                    last= max_coeff; \
                    break; \
                } \
            } \
        }\
        if( last == max_coeff -1 ) {\
            index[coeff_count++] = last;\
        }
		
static int32 readCoeff4x4_CABAC_hw(DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T *mb_cache_ptr, int cat, int32 n, int32 max_coeff)
{
    DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
    int index[64];
    int last;
    int k, coeff_count = 0;
    int node_ctx = 0;

    uint8 *significant_coeff_ctx_base;
    uint8 *last_coeff_ctx_base;
    uint8 *abs_level_m1_ctx_base;
    int8 *pNnzRef = mb_cache_ptr->nnz_cache;

#define CC &cc
    CABACContext cc;
    cc.range     = img_ptr->cabac.range;
    cc.low       = img_ptr->cabac.low;

    /* cat: 0-> DC 16x16  n = 0
     *      1-> AC 16x16  n = luma4x4idx
     *      2-> Luma4x4   n = luma4x4idx
     *      3-> DC Chroma n = iCbCr
     *      4-> AC Chroma n = 4 * iCbCr + chroma4x4idx
     *      5-> Luma8x8   n = 4 * luma8x8idx
     */

    /* read coded block flag */
    if( cat != 5 ) 
    {
	if( get_cabac( CC, &img_ptr->cabac_state[85 + get_cabac_cbf_ctx( currMB, mb_cache_ptr, cat, n ) ] ) == 0 ) 
	{
            if( cat == 1 || cat == 2 || cat == 4)
		pNnzRef [g_blk_order_map_tbl[n]] = 0;
	    img_ptr->cabac.range     = cc.range;
	    img_ptr->cabac.low       = cc.low;
	    return 0;
	}
    }

    significant_coeff_ctx_base = img_ptr->cabac_state + significant_coeff_flag_offset[cat];
    last_coeff_ctx_base = img_ptr->cabac_state + last_coeff_flag_offset[cat];
    abs_level_m1_ctx_base = img_ptr->cabac_state + coeff_abs_level_m1_offset[cat];

    if( cat == 5 ) 
    {
	const uint8 *sig_off = significant_coeff_flag_offset_8x8;
	DECODE_SIGNIFICANCE( 63, sig_off[last], last_coeff_flag_offset_8x8[last] );

    	//	n = n << 2; //start of 8x8 block
	pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
	pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
	pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
	pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
    }else 
    {
	DECODE_SIGNIFICANCE( max_coeff - 1, last, last );

        if( cat == 0 )
        {
    	    mb_cache_ptr->vld_dc_coded_flag |=  (1 << 8);
        }else if( cat == 1 || cat == 2 )
        {
    	    pNnzRef [g_blk_order_map_tbl[n]] = coeff_count;
        }else if( cat == 3 )
        {
    	    mb_cache_ptr->vld_dc_coded_flag |=  (1 << (9+n));
        }else //if( cat == 4 )
        {
            //assert( cat == 4 );
    	    pNnzRef [g_blk_order_map_tbl[n]] = coeff_count;
        }
    }
		
    k = coeff_count;
    for( k--; k >= 0; k-- ) 
    {
	uint8 *ctx = coeff_abs_level1_ctx[node_ctx] + abs_level_m1_ctx_base;

       	if( get_cabac( CC, ctx ) == 0 ) 
	{
    	    node_ctx = coeff_abs_level_transition[0][node_ctx];
            /*block[j] =*/get_cabac_bypass_sign( CC, -1);
        } else 
        {
	    int coeff_abs = 2;
	    ctx = coeff_abs_levelgt1_ctx[node_ctx] + abs_level_m1_ctx_base;
	    node_ctx = coeff_abs_level_transition[1][node_ctx];

	    while( coeff_abs < 15 && get_cabac( CC, ctx ) ) 
	    {
		coeff_abs++;
	    }

	    if( coeff_abs >= 15 ) 
	    {
		int j = 0;
		while( get_cabac_bypass( CC ) ) 
		{
		    j++;
                }

                //coeff_abs=1;
		while( j-- ) 
		{
		    /*coeff_abs += coeff_abs +*/ get_cabac_bypass( CC );
		}
		//	coeff_abs+= 14;
	    }

            /*block[j] =*/ get_cabac_bypass_sign( CC, -1);
        }
    }

    img_ptr->cabac.range = cc.range;
    img_ptr->cabac.low     = cc.low;

    return coeff_count;
}

void decode_mb_cabac_hw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{	
    int32 uv;
    int32 maxCoeff, numCoeff;
    int32 blk4x4, blk8x8;
    int32 blkIndex;
    int32 blk_type;
    int32 cbp;

    mb_cache_ptr->vld_dc_coded_flag = (mb_cache_ptr->vld_dc_coded_flag & 0xff);

    if (mb_info_ptr->mb_type == I16MB)
    {
        //luma dc
        readCoeff4x4_CABAC_hw (mb_info_ptr, mb_cache_ptr, LUMA_DC, 0, 16);

        //for luma ac
	blk_type = LUMA_AC_I16;
	maxCoeff = 15;    
    }else
    {
        blk_type = LUMA_AC;
	maxCoeff = 16;
    }

    cbp = mb_info_ptr->cbp;
    
    // luma ac
    for (blkIndex = 0, blk8x8= 0; blk8x8 < 4; blk8x8++)
    {
	if (cbp & (1 << blk8x8))
	{
	    for (blk4x4 = 0; blk4x4 < 4; blk4x4++, blkIndex++)
	    {					
		if (readCoeff4x4_CABAC_hw (mb_info_ptr, mb_cache_ptr, blk_type, blkIndex, maxCoeff))
                {
		    mb_cache_ptr->cbp_luma_iqt |= (1<< blkIndex);
                }
	    }
	}else
        {
            blkIndex += 4;
        }
    }

    if (cbp > 15)
    {
        //chroma dc
        for (uv = 0; uv < 2; uv++)
        {			
            if (readCoeff4x4_CABAC_hw (mb_info_ptr, mb_cache_ptr,  CHROMA_DC, uv, 4))
            {
                mb_cache_ptr->cbp_uv |= 0xf << (4*uv);	
            }
        }
    }

    if (cbp > 31)
    {
        //chroma ac
	for (blkIndex = 0, uv = 0; uv < 2; uv++)
	{	
            for (blk4x4 = 0; blk4x4 < 4; blk4x4++, blkIndex++)
            {	
		if (readCoeff4x4_CABAC_hw (mb_info_ptr, mb_cache_ptr, CHROMA_AC, blkIndex + 16, 15))
		{
		    mb_cache_ptr->cbp_uv |= (0x1 << blkIndex);
		}
	    }
	}
    }
		
    mb_info_ptr->dc_coded_flag = (mb_cache_ptr->vld_dc_coded_flag >> 8) & 0x7;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
