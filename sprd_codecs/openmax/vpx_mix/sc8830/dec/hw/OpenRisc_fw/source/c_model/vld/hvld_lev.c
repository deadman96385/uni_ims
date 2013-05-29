/*hvld_lev.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "bsm_global.h"

//register
static int		level_two_or_higher = 0;    
static int		suffix_len = 0;



/************************************************************************/
/* 
   1. trailing_one sign is stored in register, for one cycle decoding all trailing one sign
   2. the other coefficient level is stored in huffman table for saving area
   3. and run_before are stored in register for one cycle to write back one coefficient
                                                                     */
/************************************************************************/

void ReadLevVlc (
				 int	suffix_len, 
				 int *	lev_ptr, 
				 int *	sign_ptr,
				 int *	lev_err_ptr
				 )
{
	int		level_prefix;
	int		bsm_lev_data;
	int		lev_prefix_error = 0;
	uint32	next_16bits;
	int		lev_suffix_size;
	int		level_code;
	int		sign;
	int		level;
	int		level_suffix;
	int		lev_bsm_len;

	bsm_lev_data = show_nbits (32);

	/*prefix length*/
	if ((bsm_lev_data & 0xffff0000) == 0)
	{
		lev_prefix_error = 1;
		next_16bits		 = 0;
	}
	else if (bsm_lev_data & 0xff000000)
	{
		if (bsm_lev_data & 0xf0000000)
		{
			if (bsm_lev_data & 0x80000000)
			{
				level_prefix = 0;
				next_16bits  = (bsm_lev_data >> 15) & 0xffff;
			}
			else if (bsm_lev_data & 0x40000000)
			{
				level_prefix = 1;
				next_16bits  = (bsm_lev_data >> 14) & 0xffff;
			}
			else if (bsm_lev_data & 0x20000000)
			{
				level_prefix = 2;
				next_16bits  = (bsm_lev_data >> 13) & 0xffff;
			}
			else
			{
				level_prefix = 3;
				next_16bits  = (bsm_lev_data >> 12) & 0xffff;
			}
		}
		else
		{
			if (bsm_lev_data & 0x08000000)
			{
				level_prefix = 4;
				next_16bits  = (bsm_lev_data >> 11) & 0xffff;
			}
			else if (bsm_lev_data & 0x04000000)
			{
				level_prefix = 5;
				next_16bits  = (bsm_lev_data >> 10) & 0xffff;
			}
			else if (bsm_lev_data & 0x02000000)
			{
				level_prefix = 6;
				next_16bits  = (bsm_lev_data >> 9) & 0xffff;
			}
			else
			{
				level_prefix = 7;
				next_16bits  = (bsm_lev_data >> 8) & 0xffff;
			}
		}
	}
	else
	{
		if (bsm_lev_data & 0x00f00000)
		{
			if (bsm_lev_data & 0x00800000)
			{
				level_prefix = 8;
				next_16bits  = (bsm_lev_data >> 7) & 0xffff;
			}
			else if (bsm_lev_data & 0x00400000)
			{
				level_prefix = 9;
				next_16bits  = (bsm_lev_data >> 6) & 0xffff;
			}
			else if (bsm_lev_data & 0x00200000)
			{
				level_prefix = 10;
				next_16bits  = (bsm_lev_data >> 5) & 0xffff;
			}
			else
			{
				level_prefix = 11;
				next_16bits  = (bsm_lev_data >> 4) & 0xffff;
			}
		}
		else
		{
			if (bsm_lev_data & 0x00080000)
			{
				level_prefix = 12;
				next_16bits  = (bsm_lev_data >> 3) & 0xffff;
			}
			else if (bsm_lev_data & 0x00040000)
			{
				level_prefix = 13;
				next_16bits  = (bsm_lev_data >> 2) & 0xffff;
			}
			else if (bsm_lev_data & 0x00020000)
			{
				level_prefix = 14;
				next_16bits  = (bsm_lev_data >> 1) & 0xffff;
			}
			else
			{
				level_prefix = 15;
				next_16bits  = (bsm_lev_data >> 0) & 0xffff;
			}
		}
	}

	if ((level_prefix == 14) && (suffix_len == 0))
	{
		lev_suffix_size = 4;
	}
	else if (level_prefix == 15)
	{
		lev_suffix_size = 12;
	}
	else
	{
		lev_suffix_size = suffix_len;
	}

	level_suffix = next_16bits >> (16 - lev_suffix_size);

	level_code  = (level_prefix << suffix_len) + level_suffix; 

	if ((level_prefix == 15) && (suffix_len == 0))
	{
		level_code += 15;
	}

	sign  = level_code & 1;
	level = (level_code + 2) >> 1;

	lev_bsm_len = lev_suffix_size + level_prefix + 1;
	flush_nbits (lev_bsm_len);

	*lev_ptr		= level;
	*sign_ptr		= sign;
	*lev_err_ptr	= lev_prefix_error;

}

int SuffixLenInc (int suffix_len, int level)
{
	int		suffix_len_inc;

	if (suffix_len == 0)
	{
		suffix_len_inc = 1;
	}
	else if (suffix_len == 1)
	{
		suffix_len_inc = (level > 3) ? 1 : 0;
	}
	else if (suffix_len == 2)
	{
		suffix_len_inc = (level > 6) ? 1 : 0;
	}
	else if (suffix_len == 3)
	{
		suffix_len_inc = (level > 12) ? 1 : 0;
	}
	else if (suffix_len == 4)
	{
		suffix_len_inc = (level > 24) ? 1 : 0;
	}
	else if (suffix_len == 5)
	{
		suffix_len_inc = (level > 48) ? 1 : 0;
	}
	else
	{
		suffix_len_inc = 0;
	}

	return suffix_len_inc;	
}


int H264VldLevDec (
					int		trailing_one, 
					int		total_coeff
					)
{
	int		k;
	int		level;
	int		sign;
	uint32	bsm_lev_data;
	int		level_two_or_higher;
	int		suffix_len;
	int		lev_tbuf_addr;
	int		suffix_len_inc;
	int		lev_err;
	int		trailing_one_left;
	int		lev_num = 0;


	/*trailing one sign*/
	trailing_one_left = trailing_one;
	while (trailing_one_left != 0)
	{
		bsm_lev_data = show_nbits (32);
		
		sign = (bsm_lev_data >> 31) & 1;
		
		level = sign ? -1 : 1;

		lev_tbuf_addr = total_coeff - 1 - (trailing_one - trailing_one_left) + LEV_TBUF_BASE_ADDR;
		g_hvld_huff_tab[lev_tbuf_addr] = level;

		flush_nbits (1);

//		PrintfCoeff (level);

		trailing_one_left--;
	}

	if (total_coeff == trailing_one)
		return 0;

	level_two_or_higher = (trailing_one == 3) ? 0 : 1;
	suffix_len			= ((total_coeff > 10) && (trailing_one < 3)) ? 1 : 0;
	

	/*the other coefficient */
	for (k = total_coeff - 1-trailing_one; k >= 0; k--)
	{
		if (lev_num == 2)
			printf ("");
		ReadLevVlc (suffix_len, &level, &sign, &lev_err);

		if (lev_err)
			return 1;

		if (level_two_or_higher)
		{
			level++;
			level_two_or_higher = 0;
		}

		/*suffix lenght update*/
		suffix_len_inc = SuffixLenInc (suffix_len, level);

		suffix_len = suffix_len_inc ? suffix_len + 1 : suffix_len;	

		if ((k == total_coeff - 1-trailing_one) && (level > 3))
		{
			suffix_len = 2;			
		}

		level = sign ? -level : level;

		/*write level to huffman table, from 176 position of huffman table*/
		lev_tbuf_addr = LEV_TBUF_BASE_ADDR + k;
		g_hvld_huff_tab[lev_tbuf_addr] = level;

//		PrintfCoeff (level);
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			VLD_FPRINTF (g_hvld_trace_fp, "level: %d\n", level);
		}

		lev_num++;
	}

	return 0;
}
