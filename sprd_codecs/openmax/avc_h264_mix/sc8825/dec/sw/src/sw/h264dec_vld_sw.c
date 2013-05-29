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
int readCoeff4x4_CAVLC_sw (DEC_MB_CACHE_T *mb_cache_ptr, int16 *block, int32 n, const uint8 *scantable, const uint32 *qmul, int32 max_coeff)
{
	DEC_BS_T * stream = g_image_ptr->bitstrm_ptr;
	static const int coeff_token_table_index[17] = {0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3};
	int level[16];
	int zeros_left, coeff_num, coeff_token, total_coeff, i, j, trailing_ones, run_before;
	uint32 code;

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
	if (total_coeff <= 0)
		return 0;

	if (total_coeff > max_coeff)
	{
		return -1;
	}

	trailing_ones = coeff_token&3;

	code = READ_FLC(stream, trailing_ones);
	code = code << (32 - trailing_ones);
	for (i = 0; i < trailing_ones; i++)
	{
	//	level[i] = 1 - 2*READ_BITS1(stream);
		level[i] = 1 - 2*((code>>31) & 0x1);
		code <<= 1;
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
	{
		zeros_left = 0;
	}else
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
	j = scantable[coeff_num];
	if (n > 24)
	{
		block[j] = level[0];
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
			j = scantable[coeff_num];

			block[j] = level[i];
		}
	}else
	{
		block[j] = (level[0] * qmul[j] + 32)>>6;
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
			j = scantable[coeff_num];

			block[j] = (level[i] * qmul[j] + 32)>>6;
		}		
	}

	if (zeros_left < 0)
	{
		return -1;
	}

	return total_coeff;
}

void decode_LUMA_DC (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int32 qp)
{
	int16 DC[16];
	int32 total_coeff;
	const int8 * inverse_zigZag = g_inverse_zigzag_tbl;

#ifndef _NEON_OPT_
	memset (DC, 0, 16*sizeof(int16));
#else
	memset_8words_zero(DC);
#endif

	total_coeff = readCoeff4x4_CAVLC_sw(mb_cache_ptr,DC, LUMA_DC_BLOCK_INDEX, inverse_zigZag, NULL, 16);
	if (total_coeff)
	{
		itrans_lumaDC (DC, mb_cache_ptr->coff_Y, qp);
	}

	return;
}

void decode_LUMA_AC_sw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp )
{
	int32 numCoeff, maxCoeff;
	int32 blk4x4,blk8x8;
	int16 * coeff = mb_cache_ptr->coff_Y;
	int32 cbp = currMB->cbp;
	const int8 * inverse_zigZag;   
	const int32 * quant_mat;

	if(currMB->transform_size_8x8_flag)
	{
		inverse_zigZag = g_inverse_8x8_zigzag_tbl_cavlc;
		quant_mat= (currMB->is_intra) ? g_image_ptr->dequant8_buffer[0][qp] : g_image_ptr->dequant8_buffer[0][qp];
		
		for(blk8x8 = 0; blk8x8 < 4; blk8x8++)
		{
			if(cbp & (1 << blk8x8))
			{
				int8 *nnz;

			#ifndef _NEON_OPT_
				memset(coeff, 0, 64*sizeof(int16));
			#else
				clear_Nx16words (coeff, 2);
			#endif

				for(blk4x4 = 0; blk4x4 < 4; blk4x4++)
				{
					numCoeff = readCoeff4x4_CAVLC_sw(mb_cache_ptr, coeff, ((blk8x8<<2) + blk4x4), inverse_zigZag+16*blk4x4, quant_mat, 16);
				}
				nnz = &(mb_cache_ptr->nnz_cache[g_blk_order_map_tbl[(blk8x8<<2)+0]]);
				nnz[0] += (nnz[1] + nnz[12] + nnz[13]);
			}
			coeff += 16*4;
		}
	}else
	{
		inverse_zigZag = g_inverse_zigzag_tbl;
		quant_mat = (currMB->is_intra) ? g_image_ptr->dequant4_buffer[0][qp] : g_image_ptr->dequant4_buffer[3][qp];
		
		if (currMB->mb_type == I16MB)
		{
			maxCoeff = 15;
			inverse_zigZag += 1;
		}else
		{
			maxCoeff = 16;
		}
		
		for (blk4x4 = 0; blk4x4 < 16; blk4x4++)
		{
			if ( cbp & (1 << (blk4x4/4)) )
			{
				if (/*(numCoeff > 0) &&*/ (currMB->mb_type != I16MB))  
				{
				#ifndef _NEON_OPT_
					memset (coeff, 0, 16*sizeof(int16));
				#else
					memset_8words_zero(coeff);
				#endif
				}
				numCoeff = readCoeff4x4_CAVLC_sw(mb_cache_ptr, coeff, blk4x4, inverse_zigZag, quant_mat, maxCoeff);

				coeff += 16;
			}else
			{
				blk4x4 += 3;  //go to next 8x8 block 
				coeff  += 16*4;
			}
		}
	}
	
	return;
}

void decode_CHROMA_DC (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp_uv[2])
{
	int uv;
	int16 DC[4];
	int total_coeff;
	int m0, m1, m2, m3;
	int n0, n1, n2, n3;
	int32 quantizer;
	int16 * pCoeff;
	const int8 inverse_zigZag[4] = {0, 1, 2, 3};
	
	/*dc coeff for uv*/
	for (uv = 0; uv < 2; uv++)
	{
		((int32 *)DC) [0] = ((int32 *)DC) [1] = 0;
	
		total_coeff = readCoeff4x4_CAVLC_sw(mb_cache_ptr,DC, CHROMA_DC_BLOCK_INDEX, inverse_zigZag, NULL, 4);
		if (total_coeff)
		{
			mb_cache_ptr->cbp_uv |= 0xf << (4*uv);
		}else
		{
			continue;
		}
			
		/*inverse transform and inverse quantization and put to DC position*/
		m0 = DC[0];    m1 = DC[1];    m2 = DC[2];    m3 = DC[3];
		n0 = m0 + m1;  n1 = m2 + m3;  n2 = m0 - m1;  n3 = m2 - m3;

		quantizer = (currMB->is_intra)? g_image_ptr->dequant4_buffer[uv+1][qp_uv[uv]][0]: g_image_ptr->dequant4_buffer[uv+4][qp_uv[uv]][0];	
		m0 = (n0 + n1) * quantizer;
		m1 = (n2 + n3) * quantizer;
		m2 = (n0 - n1) * quantizer;
		m3 = (n2 - n3) * quantizer;
 
		pCoeff = mb_cache_ptr->coff_UV[uv];	
		pCoeff[ 0] = m0 >> 7;
		pCoeff[16] = m1 >> 7;
		pCoeff[32] = m2 >> 7;
		pCoeff[48] = m3 >> 7;
	}

	return;
}

void decode_CHROMA_AC_sw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp_uv[2])
{
	int uv;
	int numCoeff;
	int blkIndex;
	int blk4x4;
	int16 * coeff;
	const int32 * quant_mat;
	const int8 * inverse_zigZag = g_inverse_zigzag_tbl + 1;
	
	for (uv = 0; uv < 2; uv++)
	{
		quant_mat = (currMB->is_intra)? g_image_ptr->dequant4_buffer[uv+1][qp_uv[uv]]: g_image_ptr->dequant4_buffer[uv+4][qp_uv[uv]];
		coeff = mb_cache_ptr->coff_UV[uv];
		
		for (blk4x4 = 0; blk4x4 < 4; blk4x4++)
		{	
			blkIndex = uv * 4 + blk4x4 + 16;
			numCoeff = readCoeff4x4_CAVLC_sw(mb_cache_ptr, coeff, blkIndex, inverse_zigZag, quant_mat, 15);
			if (numCoeff)
			{
				mb_cache_ptr->cbp_uv |= 0x1 << (uv*4 + blk4x4);
			}else
			{
				coeff += 16;		
				continue;
			}

			coeff += 16;				
		}
	}

	return;
}

void decode_mb_cavlc_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 qp_uv, qp_c[2];
	int32 qp;
#ifdef _NEON_OPT_
	int32 i;
	int16 *coeff;
	int16x8_t v16x8 = vmovq_n_s16(0);
#endif

	qp = mb_info_ptr->qp;
	

	if (mb_info_ptr->mb_type == I16MB)
	{
	#ifndef _NEON_OPT_
		memset (mb_cache_ptr->coff_Y, 0, 16*16*sizeof(int16));
	#else
		clear_Nx16words(mb_cache_ptr->coff_Y, 8);
	#endif
		decode_LUMA_DC (mb_info_ptr,  mb_cache_ptr, qp);	//luma DC
	}

	decode_LUMA_AC_sw (mb_info_ptr, mb_cache_ptr, qp);

	qp_uv = qp + g_image_ptr->chroma_qp_offset;
	qp_uv = IClip(0, 51, qp_uv);
	qp_c[0] = g_QP_SCALER_CR_TBL[qp_uv];

	qp_uv = qp + g_image_ptr->second_chroma_qp_index_offset;
	qp_uv = IClip(0, 51, qp_uv);
	qp_c[1] = g_QP_SCALER_CR_TBL[qp_uv];
	
	if (mb_info_ptr->cbp > 15)
	{
	#ifndef _NEON_OPT_
		memset (mb_cache_ptr->coff_UV, 0, 2*4*16*sizeof(int16));
	#else
		clear_Nx16words(mb_cache_ptr->coff_UV, 4);
	#endif
		decode_CHROMA_DC(mb_info_ptr, mb_cache_ptr, qp_c);
	}

	if (mb_info_ptr->cbp > 31)
	{
		decode_CHROMA_AC_sw (mb_info_ptr, mb_cache_ptr, qp_c); 
	}
}

#define	LUMA_DC			0
#define LUMA_AC_I16		1
#define LUMA_AC			2
#define CHROMA_DC		3
#define CHROMA_AC		4

/******************************************************************
			derive nc according to block type, block id
*******************************************************************/

	static const int significant_coeff_flag_offset[2][6] = 
	{
		{ 105+0, 105+15, 105+29, 105+44, 105+47, 402 },
		{ 277+0, 277+15, 277+29, 277+44, 277+47, 436 }
    	};

	static const int last_coeff_flag_offset[2][6] = 
	{
		{ 166+0, 166+15, 166+29, 166+44, 166+47, 417 },
		{ 338+0, 338+15, 338+29, 338+44, 338+47, 451 }
    	};

	static const int coeff_abs_level_m1_offset[6] = 
	{
		227+0, 227+10, 227+20, 227+30, 227+39, 426
	};

	static const uint8 significant_coeff_flag_offset_8x8[2][63] = 
	{
		{ 
			0, 1, 2, 3, 4, 5, 5, 4, 4, 3, 3, 4, 4, 4, 5, 5,
			4, 4, 4, 4, 3, 3, 6, 7, 7, 7, 8, 9,10, 9, 8, 7,
			7, 6,11,12,13,11, 6, 7, 8, 9,14,10, 9, 8, 6,11,
			12,13,11, 6, 9,14,10, 9,11,12,13,11,14,10,12 
		},
      		{ 
      			0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 7, 7, 8, 4, 5,
			6, 9,10,10, 8,11,12,11, 9, 9,10,10, 8,11,12,11,
			9, 9,10,10, 8,11,12,11, 9, 9,10,10, 8,13,13, 9,
			9,10,10, 8,13,13, 9, 9,10,10,14,14,14,14,14 
		}
	};

	const uint8 last_coeff_flag_offset_8x8[63] = 
	{
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	    	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	    	3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	    	5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8
	};
	
    /* node ctx: 0..3: abslevel1 (with abslevelgt1 == 0).
     * 4..7: abslevelgt1 + 3 (and abslevel1 doesn't matter).
     * map node ctx => cabac ctx for level=1 */
	static const uint8 coeff_abs_level1_ctx[8] = { 1, 2, 3, 4, 0, 0, 0, 0 };
	
    /* map node ctx => cabac ctx for level>1 */
	static const uint8 coeff_abs_levelgt1_ctx[8] = { 5, 5, 5, 5, 6, 7, 8, 9 };
	
	static const uint8 coeff_abs_level_transition[2][8] = 
	{
    /* update node ctx after decoding a level=1 */
		{ 1, 2, 3, 3, 4, 5, 6, 7 },
    /* update node ctx after decoding a level>1 */
		{ 4, 4, 4, 4, 5, 6, 7, 7 }
	};

#define MB_FIELD	0

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
		
static int32 readCoeff4x4_CABAC_sw(DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T *mb_cache_ptr, int16 *block, int cat, int32 n, const uint8 *scantable, const uint32 *qmul, int32 max_coeff)
{
	DEC_IMAGE_PARAMS_T *img_ptr = g_image_ptr;
	int index[64];
    int last;
	int coeff_count = 0;
	int node_ctx = 0;

    uint8 *significant_coeff_ctx_base;
    uint8 *last_coeff_ctx_base;
    uint8 *abs_level_m1_ctx_base;
	int8 *pNnzRef = mb_cache_ptr->nnz_cache;

//#ifndef ARCH_X86
//#define CABAC_ON_STACK
//#endif
//#ifdef CABAC_ON_STACK
#define CC &cc
	CABACContext cc;
    cc.range     = img_ptr->cabac.range;
    cc.low       = img_ptr->cabac.low;
   // 	cc.bytestream= img_ptr->cabac.bytestream;
//#else
//#define CC &h->cabac
//#endif


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
		//	else if( cat == 4 )
		// 		pNnzRef [g_blk_order_map_tbl[n]] = 0;
//#ifdef CABAC_ON_STACK
			img_ptr->cabac.range     = cc.range;
			img_ptr->cabac.low       = cc.low;
	//		img_ptr->cabac.bytestream = cc.bytestream;
//#endif
			return 0;
		}
	}

	significant_coeff_ctx_base = img_ptr->cabac_state + significant_coeff_flag_offset[MB_FIELD][cat];
	last_coeff_ctx_base = img_ptr->cabac_state + last_coeff_flag_offset[MB_FIELD][cat];
	abs_level_m1_ctx_base = img_ptr->cabac_state + coeff_abs_level_m1_offset[cat];

    if( cat == 5 ) 
	{
		const uint8 *sig_off = significant_coeff_flag_offset_8x8[MB_FIELD];

		DECODE_SIGNIFICANCE( 63, sig_off[last], last_coeff_flag_offset_8x8[last] );
    } else 
    {
		DECODE_SIGNIFICANCE( max_coeff - 1, last, last );
	}
		
    if( cat == 0 )
    {
		//h->cbp_table[h->mb_xy] |= 0x100;
		mb_cache_ptr->vld_dc_coded_flag |=  (1 << 8);
    }else if( cat == 1 || cat == 2 )
    {
		pNnzRef [g_blk_order_map_tbl[n]] = coeff_count;
    }else if( cat == 3 )
    {
		//h->cbp_table[h->mb_xy] |= 0x40 << n;
		mb_cache_ptr->vld_dc_coded_flag |=  (1 << (9+n));
    }else if( cat == 4 )
    {
		pNnzRef [g_blk_order_map_tbl[n]] = coeff_count;
    }else
    {
       	//assert( cat == 5 );
       	//fill_rectangle(&h->non_zero_count_cache[scan8[n]], 2, 2, 8, coeff_count, 1);

	//	n = n << 2; //start of 8x8 block
		pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
		pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
		pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
		pNnzRef [g_blk_order_map_tbl[n++]] = coeff_count;
    }

    for( coeff_count--; coeff_count >= 0; coeff_count-- ) 
	{
		uint8 *ctx = coeff_abs_level1_ctx[node_ctx] + abs_level_m1_ctx_base;

		int j= scantable[index[coeff_count]];

       	if( get_cabac( CC, ctx ) == 0 ) 
		{
			node_ctx = coeff_abs_level_transition[0][node_ctx];
            if( !qmul ) 
			{
              	block[j] = get_cabac_bypass_sign( CC, -1);
            }else
            {
             	block[j] = (get_cabac_bypass_sign( CC, -qmul[j]) + 32) >> 6;
            }
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

				coeff_abs=1;
				while( j-- ) 
				{
					coeff_abs += coeff_abs + get_cabac_bypass( CC );
				}
				coeff_abs+= 14;
			}

			if( !qmul ) 
			{
				if( get_cabac_bypass( CC ) ) block[j] = -coeff_abs;
				else                                block[j] =  coeff_abs;
			}else
			{
				if( get_cabac_bypass( CC ) ) block[j] = (-coeff_abs * qmul[j] + 32) >> 6;
				else                                block[j] = ( coeff_abs * qmul[j] + 32) >> 6;
			}
		}
	}

//#ifdef CABAC_ON_STACK
	img_ptr->cabac.range     = cc.range     ;
	img_ptr->cabac.low       = cc.low       ;
//	img_ptr->cabac.bytestream= cc.bytestream;
//#endif

	return coeff_count;
}

void decode_LUMA_DC_cabac_sw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp)
{
	int16 DC[16];
	int32 total_coeff;
	const int8 * inverse_zigZag = g_inverse_zigzag_tbl;

#ifndef _NEON_OPT_
	memset (DC, 0, 16*sizeof(int16));
#else
	memset_8words_zero(DC);
#endif

	//total_coeff = readCoeff4x4_CAVLC_sw(mb_cache_ptr,DC, LUMA_DC_BLOCK_INDEX, inverse_zigZag, NULL, 16);
	total_coeff = readCoeff4x4_CABAC_sw (currMB, mb_cache_ptr, DC, LUMA_DC, 0, inverse_zigZag, NULL, 16);

	if (total_coeff)
	{
		itrans_lumaDC (DC, mb_cache_ptr->coff_Y, qp);
	}

	return;
}

void decode_LUMA_AC_cabac_sw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp)
{
	int32 maxCoeff;
	int32 blk4x4, blk8x8;
	int16 * coeff = mb_cache_ptr->coff_Y;
	int32 cbp = currMB->cbp;
	const int8 * inverse_zigZag;
	const int32 *quant_mat ;
	int32 blk_type;

	if (currMB->transform_size_8x8_flag)
	{
		inverse_zigZag = g_inverse_8x8_zigzag_tbl;
		maxCoeff = 64;
		quant_mat= (currMB->is_intra) ? g_image_ptr->dequant8_buffer[0][qp] : g_image_ptr->dequant8_buffer[0][qp];

		for (blk8x8= 0; blk8x8 < 4; blk8x8++)
		{
			if (cbp & (1 << blk8x8))
			{
			#ifndef _NEON_OPT_
				memset(coeff, 0, 64*sizeof(int16));
			#else
				clear_Nx16words(coeff, 2);
			#endif
				readCoeff4x4_CABAC_sw (currMB, mb_cache_ptr, coeff, 5, (blk8x8<<2), inverse_zigZag, quant_mat, maxCoeff);
			}

			coeff += 16*4;
		}
	}else
	{
		quant_mat = (currMB->is_intra) ? g_image_ptr->dequant4_buffer[0][qp] : g_image_ptr->dequant4_buffer[3][qp];
		
		if (currMB->mb_type == I16MB)
		{
			inverse_zigZag = g_inverse_zigzag_tbl + 1; //g_inverse_zigzag_cabac_I16_ac_tbl;
			blk_type = LUMA_AC_I16;
			maxCoeff = 15;
		}else
		{
			inverse_zigZag = g_inverse_zigzag_tbl;
			blk_type = LUMA_AC;
		//	mb_type = MB_TYPE_LUMA_4x4;
			maxCoeff = 16;
		}

		for (blk8x8= 0; blk8x8 < 4; blk8x8++)
		{
			if (cbp & (1 << blk8x8))
			{
				for (blk4x4 = 0; blk4x4 < 4; blk4x4++)
				{
					if (/*(numCoeff > 0) &&*/ (currMB->mb_type != I16MB))
					{
					#ifndef _NEON_OPT_
						memset(coeff, 0, 16*sizeof(int16));
					#else
						memset_8words_zero(coeff);
					#endif
					}
					
					readCoeff4x4_CABAC_sw (currMB, mb_cache_ptr, coeff, blk_type, ((blk8x8<<2) + blk4x4), inverse_zigZag, quant_mat, maxCoeff);	
					coeff += 16;
				}
			}else
			{
				coeff += 16*4;
			}
		}
	}

	return;
}

void decode_CHROMA_DC_cabac_sw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp_uv[2])
{
//	int k;
	int uv;
	int16 DC[4];
	int total_coeff;
	int m0, m1,m2, m3;
	int n0, n1, n2, n3;
	int32 quantizer;
	int16 * pCoeff;
	const int8 inverse_zigZag[4] = {0, 1, 2, 3};

	/*dc coeff for uv*/
	for (uv = 0; uv < 2; uv++)
	{		
		((int32 *)DC) [0] = ((int32 *)DC) [1] = 0;
		
		total_coeff = readCoeff4x4_CABAC_sw (currMB, mb_cache_ptr, DC, CHROMA_DC, uv, inverse_zigZag, NULL, 4);

		if (total_coeff)
		{
			mb_cache_ptr->cbp_uv |= 0xf << (4*uv);	
		}else
		{
			continue;
		}

		/*inverse transform and inverse quantization and put to DC position*/
		m0 = DC[0];    m1 = DC[1];    m2 = DC[2];    m3 = DC[3];
		n0 = m0 + m1;  n1 = m2 + m3;  n2 = m0 - m1;  n3 = m2 - m3;

		quantizer = (currMB->is_intra)? g_image_ptr->dequant4_buffer[uv+1][qp_uv[uv]][0]: g_image_ptr->dequant4_buffer[uv+4][qp_uv[uv]][0];	
		m0 = (n0 + n1) * quantizer;
		m1 = (n2 + n3) * quantizer;
		m2 = (n0 - n1) * quantizer;
		m3 = (n2 - n3) * quantizer;

		pCoeff = mb_cache_ptr->coff_UV[uv];
		pCoeff[ 0] = m0 >> 7;
		pCoeff[16] = m1 >> 7;
		pCoeff[32] = m2 >> 7;
		pCoeff[48] = m3 >> 7;
	}

	return;
}

void decode_CHROMA_AC_cabac_sw (DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, int qp_uv[2])
{
//	int k;
	int uv;
	int numCoeff;
	int blkIndex;
//	int level;
//	int index;
	int blk4x4;
	int16 * coeff;
	const int32 *quant_mat;
	const int8 * inverse_zigZag = g_inverse_zigzag_tbl + 1;//g_inverse_zigzag_cabac_I16_ac_tbl;

	for (uv = 0; uv < 2; uv++)
	{
		quant_mat = (currMB->is_intra)? g_image_ptr->dequant4_buffer[uv+1][qp_uv[uv]]: g_image_ptr->dequant4_buffer[uv+4][qp_uv[uv]];
		coeff = mb_cache_ptr->coff_UV[uv];
		
		for (blk4x4 = 0; blk4x4 < 4; blk4x4++)
		{	
			blkIndex = uv * 4 + blk4x4 + 16;

		#if 0
			numCoeff = readCoeff4x4_CABAC_sw (currMB, mb_cache_ptr, MB_TYPE_CHROMA_AC, CHROMA_AC, blkIndex) ;
		#else
			numCoeff = readCoeff4x4_CABAC_sw (currMB, mb_cache_ptr, coeff, CHROMA_AC, blkIndex, inverse_zigZag, quant_mat, 15);
		#endif
			if (numCoeff)
			{
				mb_cache_ptr->cbp_uv |= 0x1 << (uv*4 + blk4x4);
			}else
			{
				coeff += 16;				
				continue;
			}
			coeff += 16;			
		}
	}

	return;
}

void decode_mb_cabac_sw (DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
	int32 qp_uv, qp_c[2];
	int32 qp;
#ifdef _NEON_OPT_
	int32 i;
	int16 *coeff;
	int16x8_t v16x8 = vmovq_n_s16(0);
#endif

	qp = mb_info_ptr->qp;
	
	mb_cache_ptr->vld_dc_coded_flag = (mb_cache_ptr->vld_dc_coded_flag & 0xff);

	if (mb_info_ptr->mb_type == I16MB)
	{
	#ifndef _NEON_OPT_
		memset (mb_cache_ptr->coff_Y, 0, 16*16*sizeof(int16));
	#else
		clear_Nx16words(mb_cache_ptr->coff_Y, 8);
	#endif
		decode_LUMA_DC_cabac_sw(mb_info_ptr, mb_cache_ptr, qp);
	}

	decode_LUMA_AC_cabac_sw(mb_info_ptr, mb_cache_ptr, qp);

	qp_uv = qp + g_image_ptr->chroma_qp_offset;
	qp_uv = IClip(0, 51, qp_uv);
	qp_c[0] = g_QP_SCALER_CR_TBL[qp_uv];


	qp_uv = qp + g_image_ptr->second_chroma_qp_index_offset;
	qp_uv = IClip(0, 51, qp_uv);
	qp_c[1] = g_QP_SCALER_CR_TBL[qp_uv];
	
	if (mb_info_ptr->cbp > 15)
	{
	#ifndef _NEON_OPT_
		memset (mb_cache_ptr->coff_UV, 0, 2*4*16*sizeof(int16));
	#else
		clear_Nx16words(mb_cache_ptr->coff_UV, 4);
	#endif
		decode_CHROMA_DC_cabac_sw(mb_info_ptr, mb_cache_ptr, qp_c);
	}

	if (mb_info_ptr->cbp > 31)
	{
		decode_CHROMA_AC_cabac_sw(mb_info_ptr, mb_cache_ptr, qp_c);
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
