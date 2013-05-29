/*hvld_reg.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "common_global.h"

void NnzRegInit ()
{
	g_hvld_reg_ptr->nnz_blk_0 = 0;
	g_hvld_reg_ptr->nnz_blk_1 = 0;
	g_hvld_reg_ptr->nnz_blk_2 = 0;
	g_hvld_reg_ptr->nnz_blk_3 = 0;
	g_hvld_reg_ptr->nnz_blk_4 = 0;
	g_hvld_reg_ptr->nnz_blk_5 = 0;

	g_hvld_reg_ptr->coded_dc_flag = (g_hvld_reg_ptr->coded_dc_flag & 0xff);

	g_hvld_reg_ptr->cbp_uv	  = 0;
}

/******************************************************************
			derive nc according to block type, block id
*******************************************************************/
void GetNeighborCodedInfo (
					 /*input*/
					 int		mb_type,
					 int		blk_type, 
					 int		blk_id, 
					 int		lmb_avail, 
					 int		tmb_avail, 

					 /*output*/
					 int	*	coded_flag_a_ptr,
					 int	*	coded_flag_b_ptr,
					 int	*	nc_ptr
					 )
{
	int		na;				//left block's nnz
	int		nb;				//top block's nnz
	int		nc;
	int		lblk_avail;		//left block available
	int		tblk_avail;		//top block available
	
	uint32	neighbor_nnz;
	uint32	cur_nnz;

	int		coded_flag_a;
	int		coded_flag_b;

	int		is_intra;
	int		default_value;

	is_intra = (mb_type < H264_INTERMB) ? 1 : 0;
	default_value = is_intra ? 1 : 0;

	if (blk_type == CHROMA_DC)
	{
		int shift_bits;

		nc = -1;

		if (lmb_avail)
		{
			shift_bits = (blk_id == 0) ? 1 : 2;
			coded_flag_a = (g_hvld_reg_ptr->coded_dc_flag >> shift_bits) & 1;
		}
		else
		{
			coded_flag_a = default_value;
		}

		if (tmb_avail)
		{
			shift_bits = (blk_id == 0) ? 5 : 6;
			coded_flag_b = (g_hvld_reg_ptr->coded_dc_flag >> shift_bits) & 1;
		}
		else
		{
			coded_flag_b = default_value;
		}
	}
	else if (blk_type == CHROMA_AC)
	{
		neighbor_nnz = (blk_id < 4) ? g_hvld_reg_ptr->tl_nnz_cb : g_hvld_reg_ptr->tl_nnz_cr;
		cur_nnz		 = (blk_id < 4) ? g_hvld_reg_ptr->nnz_blk_4 : g_hvld_reg_ptr->nnz_blk_5;

		blk_id = blk_id & 3;
		
		if(blk_id == 0)
		{
			na = (neighbor_nnz >> 8) & 0x1f;
			nb = (neighbor_nnz >> 24) & 0x1f;
			lblk_avail = lmb_avail;
			tblk_avail = tmb_avail;
		}
		else if (blk_id == 1)
		{
			na = (cur_nnz >> 24) & 0x1f;
			nb = (neighbor_nnz >> 16) & 0x1f;
			lblk_avail = 1;
			tblk_avail = tmb_avail;
		}
		else if (blk_id == 2)
		{
			na = (neighbor_nnz >> 0) & 0x1f;
			nb = (cur_nnz >> 24) & 0x1f;
			lblk_avail = lmb_avail;
			tblk_avail = 1;
		}
		else
		{
			na = (cur_nnz >> 8) & 0x1f;
			nb = (cur_nnz >> 16) & 0x1f;
			lblk_avail = 1;
			tblk_avail = 1;
		}

		coded_flag_a = lblk_avail ? (na != 0) : default_value;
		coded_flag_b = tblk_avail ? (nb != 0) : default_value;
	}
	else
	{
		if (blk_type == LUMA_DC)
			blk_id = 0;

		tblk_avail = ((blk_id == 0) | (blk_id == 1) | (blk_id == 4) | (blk_id == 5)) ? tmb_avail : 1;
		lblk_avail = ((blk_id == 0) | (blk_id == 2) | (blk_id == 8) | (blk_id == 10)) ? lmb_avail : 1;
		
		if (blk_id < 8)
		{
			if (blk_id < 4)
			{
				if (blk_id == 0)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 24) & 0x1f;
				}
				else if (blk_id == 1)
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 16) & 0x1f;
				}
				else if (blk_id == 2)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 16) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 16) & 0x1f;
				}
			}
			else
			{
				if (blk_id == 4)
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 16) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 8) & 0x1f;
				}
				else if (blk_id == 5)
				{
					na = (g_hvld_reg_ptr->nnz_blk_1 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->top_nnz_y >> 0) & 0x1f;
				}
				else if (blk_id == 6)
				{
					na = (g_hvld_reg_ptr->nnz_blk_0 >> 0) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_1 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 16) & 0x1f;
				}				
			}
		}
		else
		{
			if (blk_id < 12)
			{
				if (blk_id == 8)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 8) & 0x1f;
				}
				else if (blk_id == 9)
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_0 >> 0) & 0x1f;
				}
				else if (blk_id == 10)
				{
					na = (g_hvld_reg_ptr->left_nnz_y >> 0) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_2 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_2 >> 16) & 0x1f;
				}
			}
			else
			{
				if (blk_id == 12)
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 16) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 8) & 0x1f;
				}
				else if (blk_id == 13)
				{
					na = (g_hvld_reg_ptr->nnz_blk_3 >> 24) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_1 >> 0) & 0x1f;
				}
				else if (blk_id == 14)
				{
					na = (g_hvld_reg_ptr->nnz_blk_2 >> 0) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_3 >> 24) & 0x1f;
				}
				else
				{
					na = (g_hvld_reg_ptr->nnz_blk_3 >> 8) & 0x1f;
					nb = (g_hvld_reg_ptr->nnz_blk_3 >> 16) & 0x1f;
				}				
			}
		}
		
		if (blk_type == LUMA_DC)
		{
			if (lmb_avail)
			{
				coded_flag_a = (g_hvld_reg_ptr->coded_dc_flag >> 0) & 1;
			}
			else
			{
				coded_flag_a = default_value;
			}
			
			if (tmb_avail)
			{
				coded_flag_b = (g_hvld_reg_ptr->coded_dc_flag >> 4) & 1;
			}
			else
			{
				coded_flag_b = default_value;
			}
		}
		else
		{
			coded_flag_a = lblk_avail ? (na != 0) : default_value;
			coded_flag_b = tblk_avail ? (nb != 0) : default_value;
		}
	}

	if (blk_type != CHROMA_DC)
	{
		if (tblk_avail & lblk_avail)
		{
			nc = (na + nb + 1) >> 1;
		}
		else if (lblk_avail)
		{
			nc = na;
		}
		else if (tblk_avail)
		{
			nc = nb;
		}
		else
		{
			nc = 0;
		}
	}
	else
	{
		nc = -1;
	}

	*nc_ptr = nc;
	*coded_flag_a_ptr = coded_flag_a;
	*coded_flag_b_ptr = coded_flag_b;
}

void WriteBackTotalCoeff (
						  int	blk_type, 
						  int	blk_id, 
						  int	total_coeff
						  )
{
	int nnz_4blk;

	if (blk_type == LUMA_DC)
	{
		g_hvld_reg_ptr->coded_dc_flag = g_hvld_reg_ptr->coded_dc_flag | ((total_coeff != 0) << 8);
	}
	else if (blk_type == CHROMA_DC)
	{
		g_hvld_reg_ptr->coded_dc_flag = g_hvld_reg_ptr->coded_dc_flag | ((total_coeff != 0) << (9+blk_id));
	}
	else if ((blk_type == LUMA_AC) | (blk_type == LUMA_AC_I16))
	{
		if (blk_id < 8)
		{
			if (blk_id < 4)
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_0;

				if (blk_id == 0)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 1)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 2)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}

				g_hvld_reg_ptr->nnz_blk_0 = nnz_4blk;
			}
			else
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_1;

				if (blk_id == 4)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 5)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 6)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				g_hvld_reg_ptr->nnz_blk_1 = nnz_4blk;
			}
		}
		else
		{
			if (blk_id < 12)
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_2;
				
				if (blk_id == 8)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 9)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 10)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				g_hvld_reg_ptr->nnz_blk_2 = nnz_4blk;
			}
			else
			{
				nnz_4blk = g_hvld_reg_ptr->nnz_blk_3;
				
				if (blk_id == 12)
				{
					nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
				}
				else if (blk_id == 13)
				{
					nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
				}
				else if (blk_id == 14)
				{
					nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
				}
				else
				{
					nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
				}
				
				g_hvld_reg_ptr->nnz_blk_3 = nnz_4blk;
			}
		}
	}
	else
	{
		if (blk_id < 4)
		{
			nnz_4blk = g_hvld_reg_ptr->nnz_blk_4;
			
			if (blk_id == 0)
			{
				nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
			}
			else if (blk_id == 1)
			{
				nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
			}
			else if (blk_id == 2)
			{
				nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
			}
			else
			{
				nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
			}
			
			g_hvld_reg_ptr->nnz_blk_4 = nnz_4blk;
		}
		else
		{
			
			nnz_4blk = g_hvld_reg_ptr->nnz_blk_5;
			
			if (blk_id == 4)
			{
				nnz_4blk = (nnz_4blk & 0xffffff) | (total_coeff << 24); 
			}
			else if (blk_id == 5)
			{
				nnz_4blk = (nnz_4blk & 0xff00ffff) | (total_coeff << 16); 
			}
			else if (blk_id == 6)
			{
				nnz_4blk = (nnz_4blk & 0xffff00ff) | (total_coeff << 8); 
			}
			else
			{
				nnz_4blk = (nnz_4blk & 0xffffff00) | (total_coeff << 0); 
			}
			
			g_hvld_reg_ptr->nnz_blk_5 = nnz_4blk;
		}
	}
}

int GetBin0Ctx (int blk_cat, int ctx_bin0_inc)
{
	int ctx_bin0;
	int ctx_4inc;
	int ctx_inc4_4cat;

	if (ctx_bin0_inc == 4)
	{
		if (blk_cat == 4)
		{
			ctx_bin0 = g_hvld_reg_ptr->ctx_bin0_inc4_cat4;
		}
#ifdef LUMA_8x8_CABAC
		else if (blk_cat == 5)
		{
			ctx_bin0 = (g_hvld_reg_ptr->ctx_bin0_binoth_inc4_cat5 & 0x7f);
		}
#endif
		else
		{
			ctx_inc4_4cat = g_hvld_reg_ptr->ctx_bin0_inc4_cat0_3;
			
			ctx_bin0 = (blk_cat == 0) ? ((ctx_inc4_4cat >>  0) & 0x7f) : 
					   (blk_cat == 1) ? ((ctx_inc4_4cat >>  8) & 0x7f) : 
					   (blk_cat == 2) ? ((ctx_inc4_4cat >> 16) & 0x7f) : ((ctx_inc4_4cat >> 24) & 0x7f);
		}
	}
	else
	{
		if (blk_cat == 0)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_bin0_cat0;
		}
		else if (blk_cat == 1)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_bin0_cat1;
		}
		else if (blk_cat == 2)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_bin0_cat2;
		}
		else if (blk_cat == 3)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_bin0_cat3;
		}
#ifdef LUMA_8x8_CABAC
		else if (blk_cat == 5)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_bin0_cat5;
		}
#endif
		else
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_bin0_cat4;
		}

		ctx_bin0 = (ctx_bin0_inc == 0) ? ((ctx_4inc >>  0) & 0x7f) : 
				   (ctx_bin0_inc == 1) ? ((ctx_4inc >>  8) & 0x7f) : 
				   (ctx_bin0_inc == 2) ? ((ctx_4inc >> 16) & 0x7f) : ((ctx_4inc >> 24) & 0x7f);
	}

	return ctx_bin0;
}


int GetBinothCtx (int blk_cat, int ctx_binoth_inc)
{
	int ctx_binoth;
	int ctx_4inc;
	int ctx_inc4_4cat;
	
	if (ctx_binoth_inc == 4)
	{
#ifdef LUMA_8x8_CABAC
		if (blk_cat == 5)
		{
			ctx_binoth = ((g_hvld_reg_ptr->ctx_bin0_binoth_inc4_cat5 >> 8) & 0x7f);
		}
		else
		{
			ctx_inc4_4cat = g_hvld_reg_ptr->ctx_binoth_inc4;
			
			ctx_binoth = (blk_cat == 0) ? ((ctx_inc4_4cat >>  0) & 0x7f) : 
						 (blk_cat == 1) ? ((ctx_inc4_4cat >>  8) & 0x7f) : 
						 (blk_cat == 2) ? ((ctx_inc4_4cat >> 16) & 0x7f) : ((ctx_inc4_4cat >> 24) & 0x7f);
		}
#else
		ctx_inc4_4cat = g_hvld_reg_ptr->ctx_binoth_inc4;
			
		ctx_binoth = (blk_cat == 0) ? ((ctx_inc4_4cat >>  0) & 0x7f) : 
					 (blk_cat == 1) ? ((ctx_inc4_4cat >>  8) & 0x7f) : 
					 (blk_cat == 2) ? ((ctx_inc4_4cat >> 16) & 0x7f) : ((ctx_inc4_4cat >> 24) & 0x7f);
#endif 
	}
	else
	{
		if (blk_cat == 0)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_binoth_cat0;
		}
		else if (blk_cat == 1)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_binoth_cat1;
		}
		else if (blk_cat == 2)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_binoth_cat2;
		}
		else if (blk_cat == 3)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_binoth_cat3;
		}
#ifdef LUMA_8x8_CABAC
		else if (blk_cat == 5)
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_binoth_cat5;
		}
#endif
		else
		{
			ctx_4inc = g_hvld_reg_ptr->ctx_binoth_cat4;
		}
		
		ctx_binoth = (ctx_binoth_inc == 0) ? ((ctx_4inc >>  0) & 0x7f) : 
					 (ctx_binoth_inc == 1) ? ((ctx_4inc >>  8) & 0x7f) : 
					 (ctx_binoth_inc == 2) ? ((ctx_4inc >> 16) & 0x7f) : ((ctx_4inc >> 24) & 0x7f);
	}
	
	return ctx_binoth;
}

void UpdateBin0Ctx (int blk_cat, int ctx_bin0_inc, int ctx_bin0_upt)
{
// 	int			ctx_bin0;
	int			ctx_4inc;	
	int			ctx_inc4_4cat;
	uint32	*	ctx_ptr;	
	
	if (ctx_bin0_inc == 4)
	{
		if (blk_cat == 4)
		{
			g_hvld_reg_ptr->ctx_bin0_inc4_cat4 = ctx_bin0_upt;
		}
#ifdef LUMA_8x8_CABAC
		else if (blk_cat == 5)
		{
			ctx_inc4_4cat = g_hvld_reg_ptr->ctx_bin0_binoth_inc4_cat5;
			
			ctx_inc4_4cat = (ctx_bin0_upt << 0) | (ctx_inc4_4cat & 0xffffff00);
									
			g_hvld_reg_ptr->ctx_bin0_binoth_inc4_cat5 = ctx_inc4_4cat;
		}
#endif
		else
		{
			ctx_inc4_4cat = g_hvld_reg_ptr->ctx_bin0_inc4_cat0_3;
			
			ctx_inc4_4cat = (blk_cat == 0) ? ((ctx_bin0_upt << 0)  | (ctx_inc4_4cat & 0xffffff00)) : 
							(blk_cat == 1) ? ((ctx_bin0_upt << 8)  | (ctx_inc4_4cat & 0xffff00ff)) : 
							(blk_cat == 2) ? ((ctx_bin0_upt << 16) | (ctx_inc4_4cat & 0xff00ffff)) : 
							((ctx_bin0_upt << 24) | (ctx_inc4_4cat & 0x00ffffff));
			
			g_hvld_reg_ptr->ctx_bin0_inc4_cat0_3 = ctx_inc4_4cat;
		}
	}
	else
	{
		if (blk_cat == 0)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_bin0_cat0);
		}
		else if (blk_cat == 1)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_bin0_cat1);
		}		
		else if (blk_cat == 2)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_bin0_cat2);
		}
		else if (blk_cat == 3)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_bin0_cat3);
		}
#ifdef LUMA_8x8_CABAC
		else if (blk_cat == 5)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_bin0_cat5);
		}
#endif
		else
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_bin0_cat4);
		}
		
		ctx_4inc = *ctx_ptr;
		
		ctx_4inc = (ctx_bin0_inc == 0) ? ((ctx_bin0_upt << 0)  | (ctx_4inc & 0xffffff00)) : 
				   (ctx_bin0_inc == 1) ? ((ctx_bin0_upt << 8)  | (ctx_4inc & 0xffff00ff)) : 
				   (ctx_bin0_inc == 2) ? ((ctx_bin0_upt << 16) | (ctx_4inc & 0xff00ffff)) : 
				   ((ctx_bin0_upt << 24) | (ctx_4inc & 0x00ffffff));
		
		*ctx_ptr = ctx_4inc;
	}
}

void UpdateBinothCtx (int blk_cat, int ctx_binoth_inc, int ctx_binoth_upt)
{
// 	int			ctx_bin0;
	int			ctx_4inc;	
	int			ctx_inc4_4cat;
	uint32	*	ctx_ptr;
	
	
	if (ctx_binoth_inc == 4)
	{
#ifdef LUMA_8x8_CABAC
		if (blk_cat == 5)
		{
			ctx_inc4_4cat = g_hvld_reg_ptr->ctx_bin0_binoth_inc4_cat5;
		
			ctx_inc4_4cat = ((ctx_binoth_upt << 8)  | (ctx_inc4_4cat & 0xffff00ff));
				
			g_hvld_reg_ptr->ctx_bin0_binoth_inc4_cat5 = ctx_inc4_4cat;
		}
		else
		{
			ctx_inc4_4cat = g_hvld_reg_ptr->ctx_binoth_inc4;
		
			ctx_inc4_4cat = (blk_cat == 0) ? ((ctx_binoth_upt << 0)  | (ctx_inc4_4cat & 0xffffff00)) : 
							(blk_cat == 1) ? ((ctx_binoth_upt << 8)  | (ctx_inc4_4cat & 0xffff00ff)) : 
							(blk_cat == 2) ? ((ctx_binoth_upt << 16) | (ctx_inc4_4cat & 0xff00ffff)) : 
							((ctx_binoth_upt << 24) | (ctx_inc4_4cat & 0x00ffffff));
				
			g_hvld_reg_ptr->ctx_binoth_inc4 = ctx_inc4_4cat;
		}
#else
		ctx_inc4_4cat = g_hvld_reg_ptr->ctx_binoth_inc4;
		
		ctx_inc4_4cat = (blk_cat == 0) ? ((ctx_binoth_upt << 0)  | (ctx_inc4_4cat & 0xffffff00)) : 
						(blk_cat == 1) ? ((ctx_binoth_upt << 8)  | (ctx_inc4_4cat & 0xffff00ff)) : 
						(blk_cat == 2) ? ((ctx_binoth_upt << 16) | (ctx_inc4_4cat & 0xff00ffff)) : 
						((ctx_binoth_upt << 24) | (ctx_inc4_4cat & 0x00ffffff));
			
		g_hvld_reg_ptr->ctx_binoth_inc4 = ctx_inc4_4cat;
#endif		
	}
	else
	{
		if (blk_cat == 0)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_binoth_cat0);
		}
		else if (blk_cat == 1)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_binoth_cat1);
		}		
		else if (blk_cat == 2)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_binoth_cat2);
		}
		else if (blk_cat == 3)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_binoth_cat3);
		}
#ifdef LUMA_8x8_CABAC
		else if (blk_cat == 5)
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_binoth_cat5);
		}
#endif
		else
		{
			ctx_ptr = &(g_hvld_reg_ptr->ctx_binoth_cat4);
		}
		
		ctx_4inc = *ctx_ptr;
		
		ctx_4inc =  (ctx_binoth_inc == 0) ? ((ctx_binoth_upt << 0)  | (ctx_4inc & 0xffffff00)) : 
					(ctx_binoth_inc == 1) ? ((ctx_binoth_upt << 8)  | (ctx_4inc & 0xffff00ff)) : 
					(ctx_binoth_inc == 2) ? ((ctx_binoth_upt << 16) | (ctx_4inc & 0xff00ffff)) : 
					((ctx_binoth_upt << 24) | (ctx_4inc & 0x00ffffff));
		
		*ctx_ptr = ctx_4inc;
	}
}
