/*sig_map_decoder.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"

/*each 15 bits for significant map and last coefficient flag*/
int		g_sig_map_reg;

uint32	sigctx_reg_dec;
uint32	sigctx_reg_wb;

/************************************************************************
decode significant and last coefficient flag
one cycle for one pair, 

need to update and read out the context model
for signle port ram 
at even clock, read out two contex model
at odd clock, write back previous two contex model
************************************************************************/
void ArithSigMapDecoder (int blk_cat, int blk_id)
{
	int			max_coeff_num;
	int			index;
	int			coeff_pos;
	int			start_pos;
//	uint32		two_ctx_model;
	uint32	*	ctx_model_ptr;
	uint16		ctx_sig_last;
	uint8		sig_ctx;
	uint8		last_ctx;
	int			is_sig;
	int			is_last;
	int			syn_type;
	int			cat_offset;

	/*reset the register before one block decoding*/
	g_sig_map_reg = 0;

//	max_coeff_num = (blk_cat == CHROMA_DC) ? 4 :
//					((blk_cat == LUMA_DC) || (blk_cat == LUMA_AC)) ? 16 : 15;
	max_coeff_num = (blk_cat == CHROMA_DC) ? 4 : 16;

//	start_pos = (max_coeff_num == 15) ? 1 : 0;
	start_pos = ((blk_cat == LUMA_AC_I16) || (blk_cat == CHROMA_AC)) ? 1 : 0;

	cat_offset = (blk_cat == 0) ? 0 :
				 (blk_cat == 1) ? 8 : 
				 (blk_cat == 2) ? 16 :
				 (blk_cat == 3) ? 24 : 26;
				 
	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		VLD_FPRINTF (g_hvld_trace_fp, "sig_map decoding:\n");
	}

	/*the 15th coeff don't need significant indicator*/
	for (index = 0; index < max_coeff_num-1; index++)
	{
		coeff_pos = index + start_pos;

		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			VLD_FPRINTF (g_hvld_trace_fp, "sig_last pair, pos: %d\n", coeff_pos);
		}

		if ((blk_id == 1) && (index == 14))
			printf ("");

		//at even clock, read out two context model
		if ((index & 1) == 0)
		{
			ctx_model_ptr  = (g_hvld_huff_tab + SIG_MAP_CTX_BASE) + cat_offset + (index >> 1);
			sigctx_reg_dec = *ctx_model_ptr;
		}

		ctx_sig_last = (uint16)((index & 1) ? (sigctx_reg_dec >> 16) : (sigctx_reg_dec & 0xffff));

		sig_ctx  = ctx_sig_last & 0xff;
		last_ctx = (ctx_sig_last >> 8) & 0xff;

		syn_type = CABAC_SIG_LAST;
		TwoBinArithDec (syn_type, &sig_ctx, &last_ctx, &is_sig, &is_last, 0);

// 		VLD_FPRINTF  (g_hvld_trace_fp, "sig: %d, last: %d\n", is_sig, is_last);

		/*update significant register*/
		g_sig_map_reg = g_sig_map_reg | (is_sig << coeff_pos);		

		ctx_sig_last = (last_ctx << 8) | sig_ctx;

		/*store the context into register sigctx_reg_dec*/
		if ((index & 1) == 0)
		{
			sigctx_reg_dec = (sigctx_reg_dec & 0xffff0000) | ctx_sig_last;
		}
		else
		{
			//at odd clock, write back two context mode to memory
			sigctx_reg_wb = (sigctx_reg_dec & 0xffff) | (ctx_sig_last << 16);
			*ctx_model_ptr = sigctx_reg_wb;			
		}

		if ((is_last) || (coeff_pos == max_coeff_num-2))
		{
			if ((index & 1) == 0 )
			{
				*ctx_model_ptr = sigctx_reg_dec;			
			}

			break;
		}
	}	

	/*--- last coefficient must be significant if no last symbol was received ---*/
	if (!is_last)
	{
		g_sig_map_reg = g_sig_map_reg | (1 << (max_coeff_num-1));			
	}

}
