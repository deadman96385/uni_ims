/*arith_lev_infor_dec.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "common_global.h"
#include "buffer_global.h"

/*the contex model for bin0 and bin1~bin13 are stored in register*/
//CONTEX_MOD_BIN_T	g_ctx_bin0[5];
//CONTEX_MOD_BIN_T	g_ctx_binoth[5];


int GetPrefix0Num (int sig_map_reg)
{
	int prefix0_num;

	if (sig_map_reg & 0x8000)
		prefix0_num = 0;
	else if (sig_map_reg & 0x4000)
		prefix0_num = 1;
	else if (sig_map_reg & 0x2000)
		prefix0_num = 2;
	else if (sig_map_reg & 0x1000)
		prefix0_num = 3;
	else if (sig_map_reg & 0x800)
		prefix0_num = 4;
	else if (sig_map_reg & 0x400)
		prefix0_num = 5;
	else if (sig_map_reg & 0x200)
		prefix0_num = 6;
	else if (sig_map_reg & 0x100)
		prefix0_num = 7;
	else if (sig_map_reg & 0x80)
		prefix0_num = 8;
	else if (sig_map_reg & 0x40)
		prefix0_num = 9;
	else if (sig_map_reg & 0x20)
		prefix0_num = 10;
	else if (sig_map_reg & 0x10)
		prefix0_num = 11;
	else if (sig_map_reg & 0x8)
		prefix0_num = 12;
	else if (sig_map_reg & 0x4)
		prefix0_num = 13;
	else if (sig_map_reg & 0x2)
		prefix0_num = 14;
	else
		prefix0_num = 15;

	return prefix0_num;
}

void UpdateSignMap ( int * sig_map_ptr, int prefix0_num)
{
	int sig_map = *sig_map_ptr;

	switch (prefix0_num)
	{
	case 0: sig_map  = (sig_map <<  1) & 0xffff;	break;
	case 1: sig_map  = (sig_map <<  2) & 0xffff;	break;
	case 2: sig_map  = (sig_map <<  3) & 0xffff;	break;
	case 3: sig_map  = (sig_map <<  4) & 0xffff;	break;
	case 4: sig_map  = (sig_map <<  5) & 0xffff;	break;
	case 5: sig_map  = (sig_map <<  6) & 0xffff;	break;
	case 6: sig_map  = (sig_map <<  7) & 0xffff;	break;
	case 7: sig_map  = (sig_map <<  8) & 0xffff;	break;
	case 8: sig_map  = (sig_map <<  9) & 0xffff;	break;
	case 9: sig_map  = (sig_map << 10) & 0xffff;	break;
	case 10: sig_map = (sig_map << 11) & 0xffff;	break;
	case 11: sig_map = (sig_map << 12) & 0xffff;	break;
	case 12: sig_map = (sig_map << 13) & 0xffff;	break;
	case 13: sig_map = (sig_map << 14) & 0xffff;	break;
	case 14: sig_map = (sig_map << 15) & 0xffff;	break;
	case 15: sig_map = (sig_map << 16) & 0xffff;	break;
	default: sig_map = 0;
	}

	*sig_map_ptr = sig_map;
}

/******************************************************************
bin0 is always decoded in first arith_engine
******************************************************************/
int ArithOneLevDec (
					int blk_cat, 
					int num_t1, 
					int num_lgt1
					)//这个函数应该不需要改了，只需要将那个里头的ctx_bin方面的东西改了就好了。
{
	int		lev_done = 0;
	int		syn_type;
	int		val_bin0;
	int		val_bin1;

	int		glm_prx_num;
	int		glm_sfx_num;

	int		ctx_bin0_inc;
	int		ctx_binoth_inc;

	int		coeff;
	int		sign;
	int		level;
	int		bin_idx = 0;

	uint8	ctx_bin0;
	uint8	ctx_binoth;

	int		has_golomb = 0;

	
	int		symbol = 0;
	int		binary_symbol = 0;
				

	syn_type = CABAC_LEV_BIN0;
	
	/************************************************************************
	decode bin0-sign or bin0-bin1
	if level is 1, decode bin0 in engine0 and sign in engine1
	else, decode bin0 in engine0 and bin1 in engine1
	*************************************************************************/
	ctx_bin0_inc = (num_lgt1 > 0) ? 0 : mmin(4, 1+num_t1);
	ctx_binoth_inc = mmin(4, num_lgt1);

	ctx_bin0   = GetBin0Ctx (blk_cat, ctx_bin0_inc);
	ctx_binoth = GetBinothCtx (blk_cat, ctx_binoth_inc);

	TwoBinArithDec (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);

	
	if (val_bin0 == 0)
	{
		level = 1;
		sign  = val_bin1;
	}
	else
	{
		if (val_bin1 == 0)
		{
			syn_type = CABAC_LEV_SIGN;

			/*read sign, no ctx is updated*/
			TwoBinArithDec (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);

			level = 2;
			sign  = val_bin0;
		}
		else
		{
			level    = 2;
			bin_idx += 2;

			/*unary decodingg*/
			while (1)
			{
				/*decode bin1~bin13, */
				syn_type = CABAC_LEV_BIN_OTH;

				TwoBinArithDec (syn_type, &ctx_binoth, &ctx_binoth, &val_bin0, &val_bin1, 0);

				if (val_bin0 == 0)
				{
					level += 1;
					sign = val_bin1;
					break;
				}
				else if (val_bin1 == 0)
				{
					/*read sign*/
					level += 2;
					syn_type = CABAC_LEV_SIGN;
					TwoBinArithDec (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);					

					sign  = val_bin0;

					break;
				}
				
				bin_idx += 2;
				level += 2;

				if (bin_idx == 14)
				{
					has_golomb = 1;
					break;
				}
			}

			/*exp-golomb decoding*/
			if (has_golomb)
			{
				glm_prx_num = 0;

				/*golomb prefix decoding*/
				while (1)
				{
					syn_type = CABAC_GLM_PRX;
					TwoBinArithDec (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);	

					if (val_bin0 == 0)
					{
						break;
					}
					else if (val_bin1 == 0)
					{
						symbol = symbol + (1 << glm_prx_num);

						glm_prx_num += 1;
						break;
					}
					else
					{
						symbol = symbol + (1 << glm_prx_num) + (1 << (glm_prx_num+1));
						glm_prx_num += 2;
					}
				}

				/*golomb surfix decoding*/
				glm_sfx_num = glm_prx_num;

				while (glm_sfx_num > 0)
				{					
					syn_type = CABAC_GLM_SFX;
					TwoBinArithDec (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, glm_sfx_num);

					if (val_bin0 == 1)
					{
						binary_symbol = binary_symbol | (1 << (glm_sfx_num - 1));
					}

					if (glm_sfx_num > 1)
					{
						if (val_bin1 == 1)
						{
							binary_symbol = binary_symbol | (1 << (glm_sfx_num - 2));
						}
					}						

					/*level update*/

					if (glm_sfx_num < 2)
						break;

					glm_sfx_num -= 2;
				}
				
				level = level + symbol + binary_symbol + 1;

				syn_type = CABAC_LEV_SIGN;
				TwoBinArithDec (syn_type, &ctx_bin0, &ctx_binoth, &val_bin0, &val_bin1, 0);

				sign = val_bin0;
			}
		}
	}	

	coeff = (sign == 1) ? -level : level;
	
	UpdateBin0Ctx (blk_cat, ctx_bin0_inc, ctx_bin0);
	UpdateBinothCtx (blk_cat, ctx_binoth_inc, ctx_binoth);

	return coeff;		
}


void ArithLevInforDec (int blk_type, int blk_id)
{
	int		coeff_pos;
	int		prefix0_num;
	int		coeff;
	int		num_t1;
	int		num_lgt1;
	int		level;
	int		blk_base;
	int		nzf_base;
	uint32  nz_flag;
	int16 * blk_ptr;

	int		start_pos;
	int		asic_order;

	/*coeff is decoded in reversed scan*/
	coeff_pos	= 16;
	num_t1		= 0;
	num_lgt1	= 0;

	nz_flag		= 0;

	start_pos = ((blk_type == LUMA_AC_I16) || (blk_type == CHROMA_AC)) ? 1 : 0;
	
	blk_base = (blk_type == LUMA_DC) ? COEFF_LUMA_DC_BASE :
			   ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16)) ? COEFF_LUMA_AC_BASE :
			   (blk_type == CHROMA_DC) ? COEFF_CHROMA_DC_BASE : COEFF_CHROMA_AC_BASE;
	
	nzf_base = (blk_type == LUMA_DC) ? NZFLAG_LUMA_DC_BASE :
			   ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16)) ? NZFLAG_LUMA_AC_BASE :
			   (blk_type == CHROMA_DC) ? NZFLAG_CHROMA_DC_BASE : NZFLAG_CHROMA_AC_BASE;

	blk_base = blk_base + ((blk_type == CHROMA_DC) ? 2 : 8) * blk_id;
	
	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		VLD_FPRINTF (g_hvld_trace_fp, "lev decoding: \n");
	}

	while (g_sig_map_reg != 0)
	{		
		/*get coefficient position*/
		prefix0_num = GetPrefix0Num (g_sig_map_reg);

		coeff_pos = coeff_pos - prefix0_num - 1;

		if ((blk_type == CHROMA_AC) && (blk_id == 3) && (coeff_pos == 3))
			printf ("");

		/*decode one level information*/
		coeff = ArithOneLevDec (blk_type, num_t1, num_lgt1);

		level = ABS(coeff);

		if (level > 1)
		{
			num_lgt1++;
		}

		if (level == 1)
		{
			num_t1++;
		}

		/*update the sig_map_reg*/
		UpdateSignMap (&g_sig_map_reg, prefix0_num);

		blk_ptr = (int16 *)(vsp_dct_io_0 + blk_base);

		asic_order = (blk_type == CHROMA_DC) ? coeff_pos : g_is_dctorder[coeff_pos];
		
		nz_flag = nz_flag | (1 << asic_order);

		blk_ptr[asic_order] = coeff;

		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			VLD_FPRINTF (g_hvld_trace_fp, "coeff_id: %d, lev: %d\n", coeff_pos-start_pos, coeff);
		}
	}	

	//nzf_base[blk_id] = nz_flag;
	vsp_dct_io_0[nzf_base+blk_id] = nz_flag;
}