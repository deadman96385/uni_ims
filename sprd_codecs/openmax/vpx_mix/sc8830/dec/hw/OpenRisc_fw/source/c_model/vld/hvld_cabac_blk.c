/*cabac_blk.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "common_global.h"
#include "bsm_global.h"

int ReadBlkCodedFlag (int mb_type, int blk_cat, int blk_id)
{
	int		is_bp_mode;
	uint32	bsm_data;
	int		shift_bits;

	int		coded_flag_a;
	int		coded_flag_b;
	int		nc;

	int		lmb_avail;
	int		tmb_avail;

	int		cbf;

	int		ctx_idx;
	int		ctx_idx_inc;
	int		ctx_idx_offset;
	uint8 * context_ptr;	

	lmb_avail	= (g_hvld_reg_ptr->mb_info >> 16) & 1;
	tmb_avail	= (g_hvld_reg_ptr->mb_info >> 17) & 1;

	GetNeighborCodedInfo (
						/*input*/
						mb_type,
						blk_cat, 
						blk_id, 
						lmb_avail, 
						tmb_avail, 
						
						/*output*/
						&coded_flag_a,
						&coded_flag_b,
						&nc
					);	

	/*compute ctx_idx_inc*/
	ctx_idx_inc		= coded_flag_a + 2*coded_flag_b;
	ctx_idx_offset	= blk_cat*4;
	ctx_idx			= ctx_idx_offset + ctx_idx_inc;

	context_ptr		= (uint8 *)(g_hvld_huff_tab+CODED_FLAG_CTX_BASE) + ctx_idx;
	
	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		VLD_FPRINTF (g_hvld_trace_fp, "coded_blk_flag, ctx_idx_inc: %d\n", ctx_idx_inc);
	}

	/*arithmetic decoding*/
	is_bp_mode	= 0;
	bsm_data	= show_nbits (32);
	
	cbf = BiArithDec (
				is_bp_mode,	
				&bsm_data,	
				&g_range,	 
				&g_offset,	
				context_ptr, 
				&shift_bits
			);

	flush_nbits (shift_bits);

	WriteBackTotalCoeff (blk_cat, blk_id, cbf);

	return cbf;
}

void CabacBlk (int mb_type, int blk_type, int blk_id)
{
	int		coded_blk_flag;

	if (blk_id == 1)
		printf ("");

	/*decode coded_block_flag*/
	coded_blk_flag = ReadBlkCodedFlag (mb_type, blk_type, blk_id);

	if (coded_blk_flag == 0)
	{
		return;
	}
	
	if (blk_type == CHROMA_DC)
	{
		if (blk_id == 0)
			g_hvld_reg_ptr->cbp_uv = g_hvld_reg_ptr->cbp_uv | 0xf;
		else 
			g_hvld_reg_ptr->cbp_uv = g_hvld_reg_ptr->cbp_uv | 0xf0;
	}
	else if (blk_type == CHROMA_AC)
	{
		g_hvld_reg_ptr->cbp_uv = g_hvld_reg_ptr->cbp_uv | (1 << blk_id);
	}	

	/*if block coded, decode significant map*/
	ArithSigMapDecoder (blk_type, blk_id);
	
	/*decode level information*/
	ArithLevInforDec (blk_type, blk_id);
}
