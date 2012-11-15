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

SHORT_VLC_SYMBOL_T coeff_token_vlc[4];
BYTE_VLC_SYMBOL_T chroma_dc_coeff_token_vlc;

BYTE_VLC_SYMBOL_T total_zeros_vlc[15];
BYTE_VLC_SYMBOL_T chroma_dc_total_zeros_vlc[3];

BYTE_VLC_SYMBOL_T run_vlc[6];
BYTE_VLC_SYMBOL_T run7_vlc;

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC void H264Dec_init_vld_table(void)
{
	#define MAKE_COEFF_TOKEN(i)			coeff_token_vlc[i].table = coeff_token_vlc_tbl_##i
	#define MAKE_CHROMA_DC_COEFF_TOKEN	chroma_dc_coeff_token_vlc.table = chroma_dc_coeff_token_vlc_tbl
	#define MAKE_TOTAL_ZEROS(i)				total_zeros_vlc[i].table = total_zeros_vlc_tbl_##i
	#define MAKE_CHROMA_DC_TOTAL_ZEROS(i)	chroma_dc_total_zeros_vlc[i].table = chroma_dc_total_zeros_vlc_tbl_##i
	#define MAKE_RUN_VLC(i)					run_vlc[i].table = run_vlc_tbl_##i
	#define MAKE_RUN7_VLC					run7_vlc.table = run7_vlc_tbl

	MAKE_COEFF_TOKEN(0);	MAKE_COEFF_TOKEN(1);
	MAKE_COEFF_TOKEN(2);	MAKE_COEFF_TOKEN(3);

	MAKE_CHROMA_DC_COEFF_TOKEN;

	MAKE_TOTAL_ZEROS(0);	MAKE_TOTAL_ZEROS(1);	MAKE_TOTAL_ZEROS(2);
	MAKE_TOTAL_ZEROS(3);	MAKE_TOTAL_ZEROS(4);	MAKE_TOTAL_ZEROS(5);
	MAKE_TOTAL_ZEROS(6);	MAKE_TOTAL_ZEROS(7);	MAKE_TOTAL_ZEROS(8);
	MAKE_TOTAL_ZEROS(9);	MAKE_TOTAL_ZEROS(10);	MAKE_TOTAL_ZEROS(11);
	MAKE_TOTAL_ZEROS(12);	MAKE_TOTAL_ZEROS(13);	MAKE_TOTAL_ZEROS(14);

	MAKE_CHROMA_DC_TOTAL_ZEROS(0);
	MAKE_CHROMA_DC_TOTAL_ZEROS(1);
	MAKE_CHROMA_DC_TOTAL_ZEROS(2);

	MAKE_RUN_VLC(0);	MAKE_RUN_VLC(1);	MAKE_RUN_VLC(2);
	MAKE_RUN_VLC(3);	MAKE_RUN_VLC(4);	MAKE_RUN_VLC(5);

	MAKE_RUN7_VLC;

	#undef MAKE_COEFF_TOKEN
	#undef MAKE_CHROMA_DC_COEFF_TOKEN
	#undef MAKE_TOTAL_ZEROS				
	#undef MAKE_CHROMA_DC_TOTAL_ZEROS
	#undef MAKE_RUN_VLC					
	#undef MAKE_RUN7_VLC								
}

//now, bits = 9, max_depth = 2
PUBLIC int32 get_vlc_s16 (DEC_BS_T *stream, const int16 tbl[][2], uint32 bits)
{
	int32 n;
	uint32 idx;
	int32 code;
	int32 val16;
	int32 flush_bits = 0;

	val16 = BITSTREAMSHOWBITS(stream, 30);
	idx = val16 >> (30-bits);
	code = tbl[idx][0];
	n = tbl[idx][1];

	if (n < 0)
	{
		flush_bits = bits;
		val16 <<= (bits);
		idx = (val16 >> (30+n)) & g_msk[-n];
		idx += code;
		code = tbl[idx][0];
		n = tbl[idx][1];
	}

	flush_bits += n;
	BITSTREAMFLUSHBITS(stream, (uint32)flush_bits);

	return code;
}

//now, bits = 9, max_depth = 2
PUBLIC int32 get_vlc_s8 (DEC_BS_T *stream, const int8 tbl[][2], uint32 bits)
{
	int32 n;
	uint32 idx;
	int32 code;
	int32 val16;
	int32 flush_bits = 0;

	val16 = BITSTREAMSHOWBITS(stream, 30);
	idx = val16 >> (30-bits);
	code = tbl[idx][0];
	n = tbl[idx][1];

	if (n < 0)
	{
		flush_bits = bits;
		val16 <<= (bits);
		idx = (val16 >> (30+n)) & g_msk[-n];
		idx += code;
		code = tbl[idx][0];
		n = tbl[idx][1];
	}

	flush_bits += n;
	BITSTREAMFLUSHBITS(stream, (uint32)flush_bits);

	return code;
}


PUBLIC int get_level_prefix (DEC_BS_T *stream)
{
	int32 len;
	int32 val = BITSTREAMSHOWBITS(stream, 32);

	/*get (leading zero number)*/
#ifndef _ARM_CLZ_OPT_
{
	uint32 msk = 0x80000000;
	if (!val)
	{
		g_image_ptr->error_flag |= ER_BSM_ID;
		return 0;
	}
	len = 0;
	while (!((uint32)val & msk))
	{
		len++;
		msk = msk >> 1;
	}
}
#else
	#if defined(__GNUC__)
		__asm__("clz %0, %1":"=&r"(len):"r"(val):"cc");
	#else
		__asm {
			clz len, val
		}
	#endif
#endif
	BITSTREAMFLUSHBITS(stream, (uint32)(len+1));

	return len;
}

/**
  * gets the predicted number of non zero coefficients.
  * @param n block index
  */
PUBLIC int pred_non_zero_count (DEC_MB_CACHE_T *mb_cache_ptr, int blkIndex)
{
	const int blkStrIdx = g_blk_order_map_tbl[blkIndex];
	const int leftNnz = mb_cache_ptr->nnz_cache[blkStrIdx - 1];
	const int topNnz = mb_cache_ptr->nnz_cache[blkStrIdx - CTX_CACHE_WIDTH];
	int pred_nnz = leftNnz + topNnz;

	if (pred_nnz < 64)	pred_nnz = (pred_nnz+1)>>1;

	return pred_nnz&31;
}

#define LUMA_DC			0
#define LUMA_AC_I16		1
#define LUMA_AC			2
#define CHROMA_DC		3
#define CHROMA_AC		4

int32 get_cabac_cbf_ctx(DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T * mb_cache_ptr, int cat,  int blk_id )
{
	int		na;				//left block's nnz
	int		nb;				//top block's nnz
	int		lblk_avail;		//left block available
	int		tblk_avail;		//top block available
	int		coded_flag_a;
	int		coded_flag_b;

	int32 	lmb_avail	= mb_cache_ptr->mb_avail_a;//(g_hvld_info_ptr->mb_info >> 16) & 1;
	int32	tmb_avail	= mb_cache_ptr->mb_avail_b;//(g_hvld_info_ptr->mb_info >> 17) & 1;
	int		default_value = mb_info_ptr->is_intra ? 1 : 0;
	
	int32 	ctx_idx;

	switch (cat)
	{
		case LUMA_DC:
			if (lmb_avail)
			{
				coded_flag_a = (mb_cache_ptr->vld_dc_coded_flag >> 0) & 1;
			}
			else
			{
				coded_flag_a = default_value;
			}
			
			if (tmb_avail)
			{
				coded_flag_b = (mb_cache_ptr->vld_dc_coded_flag >> 4) & 1;
			}
			else
			{
				coded_flag_b = default_value;
			}
			break;

		case LUMA_AC_I16:
		//	break;

		case LUMA_AC:
			{
				int32 map_id = g_blk_order_map_tbl[blk_id];
				int8 *nnz_ref_ptr = mb_cache_ptr->nnz_cache;
				
				na = nnz_ref_ptr[map_id-1];
				nb = nnz_ref_ptr[map_id-CTX_CACHE_WIDTH];
				
				tblk_avail = ((blk_id == 0) || (blk_id == 1) || (blk_id == 4) || (blk_id == 5)) ? tmb_avail : 1;
				lblk_avail = ((blk_id == 0) || (blk_id == 2) || (blk_id == 8) || (blk_id == 10)) ? lmb_avail : 1;

				coded_flag_a = lblk_avail ? (na != 0) : default_value;
				coded_flag_b = tblk_avail ? (nb != 0) : default_value;
			}
			break;
		case CHROMA_DC:
			{
				int shift_bits;

				if (lmb_avail)
				{
					shift_bits = (blk_id == 0) ? 1 : 2;
					coded_flag_a = (mb_cache_ptr->vld_dc_coded_flag >> shift_bits) & 1;
				}else
				{
					coded_flag_a = default_value;
				}

				if (tmb_avail)
				{
					shift_bits = (blk_id == 0) ? 5 : 6;
					coded_flag_b = (mb_cache_ptr->vld_dc_coded_flag >> shift_bits) & 1;
				}else
				{
					coded_flag_b = default_value;
				}
			}
			break;

		case CHROMA_AC:

			{
				int32 map_id = g_blk_order_map_tbl[blk_id];
				int8 *nnz_ref_ptr = mb_cache_ptr->nnz_cache;

				blk_id = blk_id & 3;
				
				na = nnz_ref_ptr[map_id-1];
				nb = nnz_ref_ptr[map_id-CTX_CACHE_WIDTH];
				
				tblk_avail = ((blk_id == 0) || (blk_id == 1) ) ? tmb_avail : 1;
				lblk_avail = ((blk_id == 0) || (blk_id == 2) ) ? lmb_avail : 1;

				coded_flag_a = lblk_avail ? (na != 0) : default_value;
				coded_flag_b = tblk_avail ? (nb != 0) : default_value;
			}
			
			break;
			
	}

	ctx_idx = 2*coded_flag_b+coded_flag_a;

	return ctx_idx + cat*4;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
