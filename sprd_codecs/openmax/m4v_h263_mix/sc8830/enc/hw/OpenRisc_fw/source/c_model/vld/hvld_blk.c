/*hvld_blk.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "bsm_global.h"

int		g_run_coeff_reg[15];


int CoeffTokenDec (
					int			nc, 
					int		*	trailing_one_ptr, 
					int		*	total_coeff_ptr
					)
{
	uint32		bsm_token_data;
	uint32		token_tbuf_addr;
	int			code_len;
	int			token_error = 0;
	int			offset;
	int			trailing_one;
	int			total_coeff;
	uint32		tbuf_token_data;
	
	
	bsm_token_data = show_nbits (32);

	if(g_trace_enable_flag&TRACE_ENABLE_VLD)
	{
//		VLD_FPRINTF (g_hvld_trace_fp, "next_32bits: 0x%08x, ", bsm_token_data);
	}
	
	if (nc == -1)
	{
		
		/*combinational logic to derive coeff_token*/
		if (bsm_token_data & 0xf0000000)			//prefix0 number < 4 
		{
			if (bsm_token_data & 0x80000000)
			{
				trailing_one = 1;
				total_coeff  = 1;
				code_len	 = 1;
			}
			else if (bsm_token_data & 0x40000000)
			{
				trailing_one = 0;
				total_coeff  = 0;
				code_len	 = 2;
			}
			else if (bsm_token_data & 0x20000000)
			{
				trailing_one = 2;
				total_coeff  = 2;
				code_len	 = 3;
			}
			else
			{
				int suffix_2bits;
				
				code_len	 = 6;

				suffix_2bits = (bsm_token_data >> 26) & 0x3;

				if (suffix_2bits == 3)
				{
					trailing_one = 0;
					total_coeff  = 1;
				}
				else if (suffix_2bits == 0)
				{
					trailing_one = 0;
					total_coeff  = 2;
				}
				else if (suffix_2bits == 2)
				{
					trailing_one = 1;
					total_coeff  = 2;
				}
				else
				{					
					trailing_one = 3;
					total_coeff	 = 3;
				}
			}
		}
		else
		{
			if (bsm_token_data & 0x08000000) 
			{
				code_len	 = 6;

				if ((bsm_token_data >> 26) & 1)
				{
					trailing_one = 0;
					total_coeff  = 3;
				}
				else
				{
					trailing_one = 0;
					total_coeff  = 4;
				}
			}
			else if (bsm_token_data & 0x04000000) 
			{
				code_len	 = 7;
					
				if ((bsm_token_data >> 25) & 1)
				{
					trailing_one = 1;
					total_coeff  = 3;
				}
				else
				{
					trailing_one = 2;
					total_coeff  = 3;
				}
			}
			else if (bsm_token_data & 0x02000000) 
			{
				code_len	 = 8;

				if ((bsm_token_data >> 24) & 1)
				{
					trailing_one = 1;
					total_coeff  = 4;
				}
				else
				{
					trailing_one = 2;
					total_coeff  = 4;
				}
			}
			else
			{
				trailing_one = 3;
				total_coeff  = 4;
				code_len	 = 7;
			}
		}		
	}
	else if (nc <4)
	{
		if (nc < 2)
		{
			if (bsm_token_data & 0xff000000)   //0 prefix number < 8
			{
				if (bsm_token_data & 0x80000000)  //0 prefix_0
				{
					token_tbuf_addr = 0;
					code_len		= 1;
				}
				else if (bsm_token_data & 0x40000000)  //1 prefix_0
				{
					token_tbuf_addr = 1;
					code_len		= 2;
				}
				else if (bsm_token_data & 0x20000000)  //2 prefix_0
				{
					token_tbuf_addr = 2;
					code_len		= 3;
				}
				else if (bsm_token_data & 0x10000000)  //3 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x3;
					token_tbuf_addr = 4 | offset;
					code_len		= (bsm_token_data & 0x08000000) ? 5 : 6;				
				}
				else if (bsm_token_data & 0x08000000)  //4 prefix_0
				{
					offset			= (bsm_token_data >> 25) & 0x3;
					token_tbuf_addr = 8 | offset;
					code_len		= (bsm_token_data & 0x04000000) ? 6 : 7;
				}
				else if (bsm_token_data & 0x04000000)  //5 prefix_0
				{
					offset			= (bsm_token_data >> 24) & 0x3;
					token_tbuf_addr = 12 | offset;
					code_len		= 8;				
				}
				else if (bsm_token_data & 0x02000000)  //6 prefix_0
				{
					offset			= (bsm_token_data >> 23) & 0x3;
					token_tbuf_addr = 16 | offset;
					code_len		= 9;				
				}
				else
				{
					offset			= (bsm_token_data >> 22) & 0x3;
					token_tbuf_addr = 20 | offset;
					code_len		= 10;				
				}
			}
			else
			{
				if (bsm_token_data & 0x00800000)    //8 prefix_0
				{
					offset			= (bsm_token_data >> 21) & 0x3;
					token_tbuf_addr = 24 | offset;
					code_len		= 11;				
				}
				else if (bsm_token_data & 0x00400000)      //9 prefix_0 
				{
					offset			= (bsm_token_data >> 19) & 0x7;
					token_tbuf_addr = 32 | offset;
					code_len		= 13;				
				}
				else if (bsm_token_data & 0x00200000)    //10 prefix_0
				{
					offset			= (bsm_token_data >> 18) & 0x7;
					token_tbuf_addr = 40 | offset;
					code_len		= 14;				
				}
				else if (bsm_token_data & 0x00100000)    //11 prefix_0
				{
					offset			= (bsm_token_data >> 17) & 0x7;
					token_tbuf_addr = 48 | offset;
					code_len		= 15;				
				}
				else if (bsm_token_data & 0x00080000)    //12 prefix_0
				{
					offset			= (bsm_token_data >> 16) & 0x7;
					token_tbuf_addr = 56 | offset;
					code_len		= 16;				
				}
				else if (bsm_token_data & 0x00040000)    //13 prefix_0
				{
					offset			= (bsm_token_data >> 16) & 0x3;
					token_tbuf_addr = 64 | offset;
					code_len		= 16;				
				}
				else if (bsm_token_data & 0x00020000)    //14 prefix_0
				{
					token_tbuf_addr = 68;
					code_len		= 15;				
				}
				else
				{
					token_error		= 1;
					token_tbuf_addr = 0;
					code_len		= 0;					
				}
			}				
		}
		else
		{
			if (bsm_token_data & 0xff000000)   //0 prefix number < 8
			{
				if (bsm_token_data & 0x80000000)  //0 prefix_0
				{
					offset			= (bsm_token_data >> 30) & 0x1;
					token_tbuf_addr = 0 | offset;
					code_len		= 2;
				}
				else if (bsm_token_data & 0x40000000)  //1 prefix_0
				{
					offset			= (bsm_token_data >> 28) & 0x3;
					token_tbuf_addr = 4 | offset;
					code_len		= (bsm_token_data & 0x20000000) ? 3 : 4;
				}
				else if (bsm_token_data & 0x20000000)  //2 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x7;
					token_tbuf_addr = 8 | offset;
					code_len		= (bsm_token_data & 0x10000000) ? 5 : 6;
				}
				else if (bsm_token_data & 0x10000000)	 //3 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x3;
					token_tbuf_addr = 16 | offset;
					code_len		= 6;
				}
				else if (bsm_token_data & 0x08000000)	 //4 prefix_0
				{
					offset			= (bsm_token_data >> 25) & 0x3;
					token_tbuf_addr = 20 | offset;
					code_len		= 7;
				}
				else if (bsm_token_data & 0x04000000)	 //5 prefix_0
				{
					offset			= (bsm_token_data >> 24) & 0x3;
					token_tbuf_addr = 24 | offset;
					code_len		= 8;
				}
				else if (bsm_token_data & 0x02000000)	 //6 prefix_0
				{
					offset			= (bsm_token_data >> 23) & 0x3;
					token_tbuf_addr = 28 | offset;
					code_len		= 9;
				}
				else if (bsm_token_data & 0x01000000)	 //7 prefix_0
				{
					offset			= (bsm_token_data >> 21) & 0x7;
					token_tbuf_addr = 32 | offset;
					code_len		= 11;
				}
				
			}
			else
			{
				if (bsm_token_data & 0x00800000)	 //8 prefix_0
				{
					offset			= (bsm_token_data >> 20) & 0x7;
					token_tbuf_addr = 40 | offset;
					code_len		= 12;
				}
				else if (bsm_token_data & 0x00400000)	 //9 prefix_0
				{
					offset			= (bsm_token_data >> 19) & 0x7;
					token_tbuf_addr = 48 | offset;
					code_len		= 13;
				}
				else if (bsm_token_data & 0x00200000)	 //10 prefix_0
				{
					offset			= (bsm_token_data >> 18) & 0x7;
					token_tbuf_addr = 56 | offset;
					code_len		= (bsm_token_data & 0x00100000) ? 13 : 14;
				}
				else if (bsm_token_data & 0x00100000)	 //11 prefix_0
				{
					offset			= (bsm_token_data >> 18) & 0x3;
					token_tbuf_addr = 64 | offset;
					code_len		= 14;
				}
				else if (bsm_token_data & 0x00080000)	 //12 prefix_0
				{
					token_tbuf_addr = 68;
					code_len		= 13;
				}
				else
				{
					token_error		= 1;
					token_tbuf_addr = 0;
					code_len		= 0;					
				}
			}			
		}
	}
	else
	{
		if (nc < 8)
		{
			if (bsm_token_data & 0xff000000)   //0 prefix number < 8
			{
				if (bsm_token_data & 0x80000000)  //0 prefix_0
				{
					offset			= (bsm_token_data >> 28) & 0x7;
					token_tbuf_addr = 0 | offset;
					code_len		= 4;
				}
				else if (bsm_token_data & 0x40000000)  //1 prefix_0
				{
					offset			= (bsm_token_data >> 27) & 0x7;
					token_tbuf_addr = 8 | offset;
					code_len		= 5;
				}
				else if (bsm_token_data & 0x20000000)  //2 prefix_0
				{
					offset			= (bsm_token_data >> 26) & 0x7;
					token_tbuf_addr = 16 | offset;
					code_len		= 6;
				}
				else if (bsm_token_data & 0x10000000)  //3 prefix_0
				{
					offset			= (bsm_token_data >> 25) & 0x7;
					token_tbuf_addr = 24 | offset;
					code_len		= 7;
				}
				else if (bsm_token_data & 0x08000000)  //4 prefix_0
				{
					offset			= (bsm_token_data >> 24) & 0x7;
					token_tbuf_addr = 32 | offset;
					code_len		= 8;
				}
				else if (bsm_token_data & 0x04000000)  //5 prefix_0
				{
					offset			= (bsm_token_data >> 23) & 0x7;
					token_tbuf_addr = 40 | offset;
					code_len		= 9;
				}
				else if (bsm_token_data & 0x02000000)  //6 prefix_0
				{
					offset			= (bsm_token_data >> 22) & 0x7;
					token_tbuf_addr = 48 | offset;
					code_len		= (((bsm_token_data >> 23) & 0x3) == 0x3) ? 9 : 10;
				}
				else  //7 prefix_0
				{
					offset			= (bsm_token_data >> 22) & 0x3;
					token_tbuf_addr = 56 | offset;
					code_len		= 10;
				}
			}
			else
			{
				if (bsm_token_data & 0x00800000)  //8 prefix_0
				{
					offset			= (bsm_token_data >> 22) & 0x1;
					token_tbuf_addr = 60 | offset;
					code_len		= 10;
				}
				else if (bsm_token_data & 0x00400000)  //7 prefix_0
				{
					token_tbuf_addr = 62;
					code_len		= 10;
				} 
				else
				{
					token_error		= 1;
					token_tbuf_addr = 0;
					code_len		= 0;					
				}
			}
		}
		else
		{
			uint32		next_6bits;

			next_6bits = (bsm_token_data >> 26) & 0x3f;

			if ((next_6bits == 2) | (next_6bits == 7))
			{
				token_error		= 1;
				token_tbuf_addr = 0;
				code_len		= 0;
			}
			else
			{
				token_tbuf_addr = next_6bits;
				code_len		= 6;
			}
		}
	}	

	if (token_error)
		return 1;

	/*look up table*/
	if (nc != -1)
	{
		int token;

		tbuf_token_data = g_hvld_huff_tab [token_tbuf_addr];

		if (nc < 2)
		{
			token = (tbuf_token_data >> 24) & 0xff;
		}
		else if (nc < 4)
		{
			token = (tbuf_token_data >> 16) & 0xff;
		}
		else if (nc < 8)
		{
			token = (tbuf_token_data >> 8) & 0xff;
		}
		else
		{
			token = (tbuf_token_data >> 0) & 0xff;
		}

		trailing_one = (token >> 6) & 3;
		total_coeff  = token & 0x1f;
	}

	flush_nbits(code_len);

	*trailing_one_ptr	= trailing_one;
	*total_coeff_ptr	= total_coeff;

	PrintfCoeffToken (nc, bsm_token_data, trailing_one, total_coeff, code_len);

	return 0;
}

void InitRunReg ()
{
	int i;
	for (i = 0; i < 15; i++)
	{
		g_run_coeff_reg[i] = 0;
	}
}

void H264VldBlk (
				 /*input*/
				 int	blk_type,
				 int	blk_id,
				 int	start_pos,
				 int	max_coeff_num,
				 int	lmb_avail,
				 int	tmb_avail,

				 /*output*/
				 int *	err_ptr
				 )
{
	int			i;
	int			nc;		//for choose huffman table
	int			trailing_one;
	int			total_coeff;
	uint32		nz_flag;		//16 bits register
	int			level;
	int			run;
	int			hvld_dbuf_addr;
	int			blk_coeff_base_addr;
	int			blk_nzf_base_addr;
	int			offset_coeff;
	int			offset_nzf;
	int			blk_tbuf_addr;
	int			index;				//register
	int			position;	

	int			coded_flag_a;
	int			coded_flag_b;

	int			mb_type = g_hvld_reg_ptr->mb_info & 3;

	/*nz_flag is reset to 0 at the signal of block start*/
	nz_flag = 0;				

	GetNeighborCodedInfo (
					mb_type,
					blk_type, 
					blk_id, 
					lmb_avail, 
					tmb_avail, 

					&coded_flag_a,
					&coded_flag_b,
					&nc
					);

	*err_ptr = CoeffTokenDec (nc, &trailing_one, &total_coeff);

	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{
		VLD_FPRINTF (g_hvld_trace_fp, "nc: %d, trailing_one: %d, total_coeff: %d\n", nc, trailing_one, total_coeff);
	}
	
	if (*err_ptr)
	{
		printf ("coeff token error!\n");
		return;
	}

	if (total_coeff == 0)
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


	*err_ptr = H264VldLevDec (trailing_one, total_coeff);
	if (*err_ptr)
	{
		printf ("level error!\n");
		return;
	}

	InitRunReg ();

	*err_ptr = H264VldRunDec (total_coeff, blk_type, max_coeff_num);


	if (*err_ptr)
	{
		printf ("run error!\n");
		return;
	}

	/*derive the block base address, and coefficient start position*/
	if (blk_type == LUMA_DC)
	{
		offset_coeff		= 0;
		offset_nzf			= 0;
		blk_coeff_base_addr = COEFF_LUMA_DC_BASE;
		blk_nzf_base_addr   = NZFLAG_LUMA_DC_BASE;
	}
	else if ((blk_type == LUMA_AC) || (blk_type == LUMA_AC_I16))
	{
		offset_coeff		= blk_id * 8;
		offset_nzf			= blk_id;
		blk_coeff_base_addr = COEFF_LUMA_AC_BASE;
		blk_nzf_base_addr	= NZFLAG_LUMA_AC_BASE;
	}
	else if (blk_type == CHROMA_DC)
	{
		offset_coeff		= blk_id * 2;
		offset_nzf			= blk_id;
		blk_coeff_base_addr = COEFF_CHROMA_DC_BASE;
		blk_nzf_base_addr   = NZFLAG_CHROMA_DC_BASE;
	}
	else
	{
		offset_coeff		= blk_id * 8;
		offset_nzf			= blk_id;
		blk_coeff_base_addr = COEFF_CHROMA_AC_BASE;
		blk_nzf_base_addr   = NZFLAG_CHROMA_AC_BASE;
	}

	blk_coeff_base_addr += offset_coeff;
	blk_nzf_base_addr   += offset_nzf;
	
	/*assemble the coefficient block, including trailing_one and other coefficient*/
	index = start_pos - 1;

	for (i = 0; i < total_coeff; i++)
	{
		blk_tbuf_addr = LEV_TBUF_BASE_ADDR + i;
		level = g_hvld_huff_tab[blk_tbuf_addr];

		run = (i == 15) ? 0 : g_run_coeff_reg[i];

		index += run + 1;

		position = (blk_type == CHROMA_DC) ? index : g_is_dctorder[index];
		
		hvld_dbuf_addr = blk_coeff_base_addr*2 + position;

		((int16 *)vsp_dct_io_0)[hvld_dbuf_addr] = (int16)level;

		nz_flag |= (1 << position);
	}

	/*write non-zero coeff flag into dct/io buffer*/
	hvld_dbuf_addr = blk_nzf_base_addr;

	vsp_dct_io_0[hvld_dbuf_addr] = nz_flag;

	/*write back the total coeff number of AC into register array*/
	if ((blk_type == LUMA_AC) | (blk_type == LUMA_AC_I16) | (blk_type == CHROMA_AC))
	{
		WriteBackTotalCoeff (blk_type, blk_id, total_coeff);
	}
}