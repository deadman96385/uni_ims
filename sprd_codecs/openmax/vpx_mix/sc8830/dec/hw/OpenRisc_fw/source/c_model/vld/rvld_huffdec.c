/*rvld_huffdec.c*/

#include "rvld_mode.h"
#include "rvld_global.h"
#include "common_global.h"
#include "bsm_global.h"

extern int g_dsc4x4_idx;
extern int g_access_times[6][27];

void StatDsc4x4Access (int item_offset)
{
	int base_addr;
	int cache_line;
	

	if (g_dsc4x4_idx == 0)
	{
		base_addr = 88;
	}
	else if (g_dsc4x4_idx == 1)
	{
		base_addr = 432 + 88;
	}	
	else if (g_dsc4x4_idx == 2)
	{
		base_addr = 432*2 + 88;
	}	
	else if (g_dsc4x4_idx == 3)
	{
		base_addr = 1943 + 151 + 44;
	}
	else if (g_dsc4x4_idx == 4)
	{
		base_addr = 1438;
	}
	else
	{
		base_addr = 1943 + 151 + 530;
	}

	cache_line = (item_offset/2 - base_addr) / 16;

//	g_access_times[g_dsc4x4_idx][cache_line] = g_access_times[g_dsc4x4_idx][cache_line] + 1;
}

int GetCodeLength (MAX_REG_ARR_T *	max_reg_arr_ptr, uint32 bsm_rdata)
{
	int code_len;
	
	if (((bsm_rdata >> 31) <= (max_reg_arr_ptr->max_reg_len1 & 0x1)) && ((max_reg_arr_ptr->max_reg_len1 >> 1) & 1))
	{
		code_len = 1;
	}
	else if (((bsm_rdata >> 30) <= (max_reg_arr_ptr->max_reg_len2 & 0x3)) && ((max_reg_arr_ptr->max_reg_len2 >> 2) & 1))
	{
		code_len = 2;
	}
	else if (((bsm_rdata >> 29) <= (max_reg_arr_ptr->max_reg_len3 & 0x7)) && ((max_reg_arr_ptr->max_reg_len3 >> 3) & 1))
	{
		code_len = 3;
	}	
	else if (((bsm_rdata >> 28) <= (max_reg_arr_ptr->max_reg_len4 & 0xf)) && ((max_reg_arr_ptr->max_reg_len4 >> 4) & 1))
	{
		code_len = 4;
	}
	else if (((bsm_rdata >> 27) <= (max_reg_arr_ptr->max_reg_len5 & 0x1f)) && ((max_reg_arr_ptr->max_reg_len5 >> 5) & 1))
	{
		code_len = 5;
	}	
	else if (((bsm_rdata >> 26) <= (max_reg_arr_ptr->max_reg_len6 & 0x3f)) && ((max_reg_arr_ptr->max_reg_len6 >> 6) & 1))
	{
		code_len = 6;
	}
	else if (((bsm_rdata >> 25) <= (max_reg_arr_ptr->max_reg_len7 & 0x7f)) && ((max_reg_arr_ptr->max_reg_len7 >> 7) & 1))
	{
		code_len = 7;
	}
	else if (((bsm_rdata >> 24) <= (max_reg_arr_ptr->max_reg_len8 & 0xff)) && ((max_reg_arr_ptr->max_reg_len8 >> 8) & 1))
	{
		code_len = 8;
	}
	else if (((bsm_rdata >> 23) <= (max_reg_arr_ptr->max_reg_len9 & 0x1ff)) && ((max_reg_arr_ptr->max_reg_len9 >> 9) & 1))
	{
		code_len = 9;
	}	
	else if (((bsm_rdata >> 22) <= (max_reg_arr_ptr->max_reg_len10 & 0x3ff)) && ((max_reg_arr_ptr->max_reg_len10 >> 10) & 1))
	{
		code_len = 10;
	}
	else if (((bsm_rdata >> 21) <= (max_reg_arr_ptr->max_reg_len11 & 0x7ff)) && ((max_reg_arr_ptr->max_reg_len11 >> 11) & 1))
	{
		code_len = 11;
	}
	else if (((bsm_rdata >> 20) <= (max_reg_arr_ptr->max_reg_len12 & 0xfff)) && ((max_reg_arr_ptr->max_reg_len12 >> 12) & 1))
	{
		code_len = 12;
	}
	else if (((bsm_rdata >> 19) <= (max_reg_arr_ptr->max_reg_len13 & 0x1fff)) && ((max_reg_arr_ptr->max_reg_len13 >> 13) & 1))
	{
		code_len = 13;
	}
	else if (((bsm_rdata >> 18) <= (max_reg_arr_ptr->max_reg_len14 & 0x3fff)) && ((max_reg_arr_ptr->max_reg_len14 >> 14) & 1))
	{
		code_len = 14;
	}
	else if (((bsm_rdata >> 17) <= (max_reg_arr_ptr->max_reg_len15 & 0x7fff)) && ((max_reg_arr_ptr->max_reg_len15 >> 15) & 1))
	{
		code_len = 15;
	}
	else
	{
		code_len = 16;
	}

	return code_len;
}

uint32 GetMinValue (MAX_REG_ARR_T *	max_reg_arr_ptr, int code_len)
{
	uint32	min_code;

	if (code_len == 1)
	{
		min_code = 0;
	}
	else if (code_len == 2)
	{
		min_code = (max_reg_arr_ptr->max_reg_len1 & 0x1) + ((max_reg_arr_ptr->max_reg_len1 >> 1) & 1)/**/;
	}	
	else if (code_len == 3)
	{
		min_code = (max_reg_arr_ptr->max_reg_len2 & 0x3) + ((max_reg_arr_ptr->max_reg_len2 >> 2) & 1)/**/;
	}	
	else if (code_len == 4)
	{
		min_code = (max_reg_arr_ptr->max_reg_len3 & 0x7) + ((max_reg_arr_ptr->max_reg_len3 >> 3) & 1)/**/;
	}	
	else if (code_len == 5)
	{
		min_code = (max_reg_arr_ptr->max_reg_len4 & 0xf) + ((max_reg_arr_ptr->max_reg_len4 >> 4) & 1)/**/;
	}	
	else if (code_len == 6)
	{
		min_code = (max_reg_arr_ptr->max_reg_len5 & 0x1f) + ((max_reg_arr_ptr->max_reg_len5 >> 5) & 1)/**/;
	}	
	else if (code_len == 7)
	{
		min_code = (max_reg_arr_ptr->max_reg_len6 & 0x3f) + ((max_reg_arr_ptr->max_reg_len6 >> 6) & 1)/**/;
	}	
	else if (code_len == 8)
	{
		min_code = (max_reg_arr_ptr->max_reg_len7 & 0x7f) + ((max_reg_arr_ptr->max_reg_len7 >> 7) & 1)/**/;
	}	
	else if (code_len == 9)
	{
		min_code = (max_reg_arr_ptr->max_reg_len8 & 0xff) + ((max_reg_arr_ptr->max_reg_len8 >> 8) & 1)/**/;
	}	
	else if (code_len == 10)
	{
		min_code = (max_reg_arr_ptr->max_reg_len9 & 0x1ff) + ((max_reg_arr_ptr->max_reg_len9 >> 9) & 1)/**/;
	}	
	else if (code_len == 11)
	{
		min_code = (max_reg_arr_ptr->max_reg_len10 & 0x3ff) + ((max_reg_arr_ptr->max_reg_len10 >> 10) & 1)/**/;
	}	
	else if (code_len == 12)
	{
		min_code = (max_reg_arr_ptr->max_reg_len11 & 0x7ff) + ((max_reg_arr_ptr->max_reg_len11 >> 11) & 1)/**/;
	}	
	else if (code_len == 13)
	{
		min_code = (max_reg_arr_ptr->max_reg_len12 & 0xfff) + ((max_reg_arr_ptr->max_reg_len12 >> 12) & 1)/**/;
	}	
	else if (code_len == 14)
	{
		min_code = (max_reg_arr_ptr->max_reg_len13 & 0x1fff) + ((max_reg_arr_ptr->max_reg_len13 >> 13) & 1)/**/;
	}	
	else if (code_len == 15)
	{
		min_code = (max_reg_arr_ptr->max_reg_len14 & 0x3fff) + ((max_reg_arr_ptr->max_reg_len14 >> 14) & 1)/**/;
	}
	else
	{
		min_code = (max_reg_arr_ptr->max_reg_len15 & 0x7fff) + ((max_reg_arr_ptr->max_reg_len15 >> 15) & 1)/**/;
	}

	min_code = min_code << 1;

	return min_code;
}

int RvldHuffDecoder (
					  MAX_REG_ARR_T *	max_reg_arr_ptr, 
					  int				base_addr, 
					  int				dsc_type,
					  int				cbp_type
					  )
{
	uint32	code;
	uint32	bsm_rdata;
	int		code_len;
	int		min_code;
	uint32	code_word;
	int		offset;
	int		is_long_code;
	int		len_base_addr;
	uint32	tbuf_rdata;
	uint32	sdram_base;
	int		len_pos;
	

	bsm_rdata = show_nbits (32);

	code_len = GetCodeLength (max_reg_arr_ptr, bsm_rdata);

	min_code = GetMinValue (max_reg_arr_ptr, code_len);

	code_word = bsm_rdata >> (32 - code_len);

	offset   = code_word - min_code;

	len_pos  = code_len - 1;

	if (dsc_type == DSC_LEV)
	{
		len_base_addr = (code_len == 1)  ? DSCLEV_LEN1_BASE : 
						(code_len == 2)  ? DSCLEV_LEN2_BASE : 
						(code_len == 3)  ? DSCLEV_LEN3_BASE : 
						(code_len == 4)  ? DSCLEV_LEN4_BASE : 
						(code_len == 5)  ? DSCLEV_LEN5_BASE : 
						(code_len == 6)  ? DSCLEV_LEN6_BASE : 
						(code_len == 7)  ? DSCLEV_LEN7_BASE : 
						(code_len == 8)  ? DSCLEV_LEN8_BASE : 
						(code_len == 9)  ? DSCLEV_LEN9_BASE : 
						(code_len == 10) ? DSCLEV_LEN10_BASE : 
						(code_len == 11) ? DSCLEV_LEN11_BASE : 
						(code_len == 12) ? DSCLEV_LEN12_BASE : 
						(code_len == 13) ? DSCLEV_LEN13_BASE : 
						(code_len == 14) ? DSCLEV_LEN14_BASE : 
						(code_len == 15) ? DSCLEV_LEN15_BASE : DSCLEV_LEN16_BASE;

		len_base_addr += base_addr;
	}
	else if (dsc_type == DSC_8X8)
	{
		len_base_addr = (code_len == 1)  ? DSC8X8_LEN1_BASE : 
						(code_len == 2)  ? DSC8X8_LEN2_BASE : 
						(code_len == 3)  ? DSC8X8_LEN3_BASE : 
						(code_len == 4)  ? DSC8X8_LEN4_BASE : 
						(code_len == 5)  ? DSC8X8_LEN5_BASE : 
						(code_len == 6)  ? DSC8X8_LEN6_BASE : 
						(code_len == 7)  ? DSC8X8_LEN7_BASE : DSC8X8_LEN8_BASE;

		len_base_addr += base_addr;
	}
	else
	{
		tbuf_rdata = g_rvld_huff_tab[base_addr + len_pos/2];

		len_base_addr = (len_pos & 1) ? (tbuf_rdata >> 16) : (tbuf_rdata & 0xffff);
	}

	is_long_code = ((dsc_type == DSC_CBP) || (dsc_type == DSC_4X4)) ? 1 : 0;

	if (dsc_type == DSC_CBP)
	{
		/*the huffman table is stored in external memory*/
		sdram_base = (cbp_type == 2) ? (g_rvld_reg_ptr->intra_cbp0_addr/* +  MAX_CBP*2*/): 
					 (cbp_type == 1) ? g_rvld_reg_ptr->intra_cbp0_addr : g_rvld_reg_ptr->inter_cbp_addr;

		code = ((uint16 *)sdram_base)[len_base_addr/2+offset];
	}
	else
	{
		if (dsc_type == DSC_4X4)
		{
			int		ele_offset;
			uint32	cache_rdata;

			ele_offset = len_base_addr/2 + offset;

			StatDsc4x4Access (ele_offset);
			
			cache_rdata = GetDsc4x4Code (g_dsc4x4_idx, ele_offset/2);

			code = (ele_offset & 1) ? (cache_rdata >> 16) : (cache_rdata & 0xffff);
		}
		else
		{
			code = is_long_code ? ((uint16*)g_rvld_huff_tab)[len_base_addr/2+offset] : 
								  ((uint8*)g_rvld_huff_tab)[len_base_addr+offset];
		}
	}

//	fprintf (g_rvld_trace_fp, "bsm_rdata: %08x, huff_len: %0d, huff_dec_code: %08x\n", bsm_rdata, code_len, code);

	flush_nbits(code_len);

	FPRINTF (g_huff_dec_pf, "%08x\n", (code_len << 16) | code);
	
	return code;
}