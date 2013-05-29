/*bi_arith_dec.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"

/*arithmetic decoder engine status*/
int g_range;
int g_offset;

int ComputeRLps (int state, int q_range)
{
	uint32	r_lps_4;
	uint8	r_lps;

	switch(state)
	{
	case 0:  r_lps_4 = 0xF0D0B080; break;
	case 1:  r_lps_4 = 0xE3C5A780; break;
	case 2:  r_lps_4 = 0xD8BB9E80; break;
	case 3:  r_lps_4 = 0xCDB2967B; break;
	case 4:  r_lps_4 = 0xC3A98E74; break;
	case 5:  r_lps_4 = 0xB9A0876F; break;
	case 6:  r_lps_4 = 0xAF988069; break;
	case 7:  r_lps_4 = 0xA6907A64; break;
	case 8:  r_lps_4 = 0x9E89745F; break;
	case 9:  r_lps_4 = 0x96826E5A; break;
	case 10: r_lps_4 = 0x8E7B6855; break;
	case 11: r_lps_4 = 0x87756351; break;
	case 12: r_lps_4 = 0x806F5E4D; break;
	case 13: r_lps_4 = 0x7A695949; break;
	case 14: r_lps_4 = 0x74645545; break;
	case 15: r_lps_4 = 0x6E5F5042; break;
	case 16: r_lps_4 = 0x685A4C3E; break;
	case 17: r_lps_4 = 0x6356483B; break;
	case 18: r_lps_4 = 0x5E514538; break;
	case 19: r_lps_4 = 0x594D4135; break;
	case 20: r_lps_4 = 0x55493E33; break;
	case 21: r_lps_4 = 0x50453B30; break;
	case 22: r_lps_4 = 0x4C42382E; break;
	case 23: r_lps_4 = 0x483F352B; break;
	case 24: r_lps_4 = 0x453B3229; break;
	case 25: r_lps_4 = 0x41383027; break;
	case 26: r_lps_4 = 0x3E362D25; break;
	case 27: r_lps_4 = 0x3B332B23; break;
	case 28: r_lps_4 = 0x38302921; break;
	case 29: r_lps_4 = 0x352E2720; break;
	case 30: r_lps_4 = 0x322B251E; break;
	case 31: r_lps_4 = 0x3029231D; break;
	case 32: r_lps_4 = 0x2D27211B; break;
	case 33: r_lps_4 = 0x2B251F1A; break;
	case 34: r_lps_4 = 0x29231E18; break;
	case 35: r_lps_4 = 0x27211C17; break;
	case 36: r_lps_4 = 0x25201B16; break;
	case 37: r_lps_4 = 0x231E1A15; break;
	case 38: r_lps_4 = 0x211D1814; break;
	case 39: r_lps_4 = 0x1F1B1713; break;
	case 40: r_lps_4 = 0x1E1A1612; break;
	case 41: r_lps_4 = 0x1C191511; break;
	case 42: r_lps_4 = 0x1B171410; break;
	case 43: r_lps_4 = 0x1916130F; break;
	case 44: r_lps_4 = 0x1815120E; break;
	case 45: r_lps_4 = 0x1714110E; break;
	case 46: r_lps_4 = 0x1613100D; break;
	case 47: r_lps_4 = 0x15120F0C; break;
	case 48: r_lps_4 = 0x14110E0C; break;
	case 49: r_lps_4 = 0x13100E0B; break;
	case 50: r_lps_4 = 0x120F0D0B; break;
	case 51: r_lps_4 = 0x110F0C0A; break;
	case 52: r_lps_4 = 0x100E0C0A; break;
	case 53: r_lps_4 = 0x0F0D0B09; break;
	case 54: r_lps_4 = 0x0E0C0B09; break;
	case 55: r_lps_4 = 0x0E0C0A08; break;
	case 56: r_lps_4 = 0x0D0B0908; break;
	case 57: r_lps_4 = 0x0C0B0907; break;
	case 58: r_lps_4 = 0x0C0A0907; break;
	case 59: r_lps_4 = 0x0B0A0807; break;
	case 60: r_lps_4 = 0x0B090806; break;
	case 61: r_lps_4 = 0x0A090706; break;
	case 62: r_lps_4 = 0x09080706; break;
	case 63: r_lps_4 = 0x02020202; break;
	default: r_lps_4 = 0;
	}

	r_lps = (uint8)((q_range == 0) ? (r_lps_4 & 0xff) : 
			(q_range == 1) ? ((r_lps_4 >> 8) & 0xff)  : 
			(q_range == 2) ? ((r_lps_4 >> 16) & 0xff) : ((r_lps_4 >> 24) & 0xff));

	return r_lps;
}

int LookupLpsState (int state)
{
	int	nxt_state;

	switch(state)
	{
	case 0:  nxt_state = 0; break;
	case 1:  nxt_state = 0; break;
	case 2:  nxt_state = 1; break;
	case 3:  nxt_state = 2; break;
	case 4:  nxt_state = 2; break;
	case 5:  nxt_state = 4; break;
	case 6:  nxt_state = 4; break;
	case 7:  nxt_state = 5; break;
	case 8:  nxt_state = 6; break;
	case 9:  nxt_state = 7; break;
	case 10: nxt_state = 8; break;
	case 11: nxt_state = 9; break;
	case 12: nxt_state = 9; break;
	case 13: nxt_state = 11; break;
	case 14: nxt_state = 11; break;
	case 15: nxt_state = 12; break;
	case 16: nxt_state = 13; break;
	case 17: nxt_state = 13; break;
	case 18: nxt_state = 15; break;
	case 19: nxt_state = 15; break;
	case 20: nxt_state = 16; break;
	case 21: nxt_state = 16; break;
	case 22: nxt_state = 18; break;
	case 23: nxt_state = 18; break;
	case 24: nxt_state = 19; break;
	case 25: nxt_state = 19; break;
	case 26: nxt_state = 21; break;
	case 27: nxt_state = 21; break;
	case 28: nxt_state = 22; break;
	case 29: nxt_state = 22; break;
	case 30: nxt_state = 23; break;
	case 31: nxt_state = 24; break;
	case 32: nxt_state = 24; break;
	case 33: nxt_state = 25; break;
	case 34: nxt_state = 26; break;
	case 35: nxt_state = 26; break;
	case 36: nxt_state = 27; break;
	case 37: nxt_state = 27; break;
	case 38: nxt_state = 28; break;
	case 39: nxt_state = 29; break;
	case 40: nxt_state = 29; break;
	case 41: nxt_state = 30; break;
	case 42: nxt_state = 30; break;
	case 43: nxt_state = 30; break;
	case 44: nxt_state = 31; break;
	case 45: nxt_state = 32; break;
	case 46: nxt_state = 32; break;
	case 47: nxt_state = 33; break;
	case 48: nxt_state = 33; break;
	case 49: nxt_state = 33; break;
	case 50: nxt_state = 34; break;
	case 51: nxt_state = 34; break;
	case 52: nxt_state = 35; break;
	case 53: nxt_state = 35; break;
	case 54: nxt_state = 35; break;
	case 55: nxt_state = 36; break;
	case 56: nxt_state = 36; break;
	case 57: nxt_state = 36; break;
	case 58: nxt_state = 37; break;
	case 59: nxt_state = 37; break;
	case 60: nxt_state = 37; break;
	case 61: nxt_state = 38; break;
	case 62: nxt_state = 38; break;
	case 63: nxt_state = 63; break;
	default: nxt_state = 0;
	}

	return nxt_state;
}

int BiArithDec (
				int			is_bp_mode,				//is by-pass mode
				uint32	*	bsm_data_ptr,			//next 32 bits in bitstream
				int		*	range_ptr,				//code range of arithmetic engine 
				int		*	offset_ptr,				//offset of arithmetic engine
				uint8	*	context_ptr,			//point to context model status to be decoded
				int		*	shift_bits_ptr			//consumed bits for the bin
				)
{
	int		bit;
	int		state;
	int		q_range;
	int		r_lps;
	int		val_mps;
	int		shift_bits;
	int		range;
	int		offset;
	uint32	bsm_data;
	int		val_bin;

	/*for by-pass mode*/
	int		offset_eq;
	int		bit_eq;
	uint32  bsm_data_eq;

	bsm_data	= *bsm_data_ptr;
	range		= *range_ptr;
	offset		= *offset_ptr;

	/*for by-pass mode*/
	offset_eq	= (offset << 1) | (bsm_data >> 31);
	bsm_data_eq = bsm_data << 1;

	if (offset_eq >= range)
	{
		bit_eq	  = 1;
		offset_eq = offset_eq - range;
	}
	else
	{
		bit_eq = 0;
	}	

	/*get the context model*/
	val_mps	= ((*context_ptr) >> 6) & 1;
	state	= *context_ptr & 0x3f;
	
	if (!is_bp_mode)
	{
		if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
		{
			VLD_FPRINTF (g_hvld_trace_fp, "bs_offset: 0x%0x, bs_range: 0x%0x, mps: %d, state: %d, ", offset, range, val_mps, state);
		}
	}

	q_range = (range >> 6 ) & 3;

	r_lps	= ComputeRLps (state, q_range);

	range	= range - r_lps;

	/*update context model*/
	if (offset < range)		//mps
	{		
		bit		= val_mps;
		state	= state + 1;
		if (state > 62)
		{
			state = 62;
		}
	}
	else					//lps
	{
		offset	= offset - range;
		range	= r_lps;
		bit		= !val_mps;

		if (state == 0)
		{
			val_mps = !val_mps;			
		}

		state = LookupLpsState (state);
	}

	/*re-normanization*/
	if ((range >> 8) == 1)
	{
		shift_bits	= 0;
		range		= range;
		offset		= offset;
		bsm_data	= bsm_data;
	}
	else if ((range >> 7) == 1)
	{
		shift_bits	= 1;
		range		= (range << 1) & 0x1ff;
		offset		= ((offset << 1) | (bsm_data >> 31)) & 0x3ff;
		bsm_data	= bsm_data << 1;
	}
	else if ((range >> 6) == 1)
	{
		shift_bits	= 2;
		range		= (range << 2) & 0x1ff;
		offset		= ((offset << 2) | ((bsm_data >> 30) & 0x3)) & 0x3ff;
		bsm_data	= bsm_data << 2;
	}
	else if ((range >> 5) == 1)
	{
		shift_bits	= 3;
		range		= (range << 3) & 0x1ff;
		offset		= ((offset << 3) | ((bsm_data >> 29) & 0x7)) & 0x3ff;
		bsm_data	= bsm_data << 3;
	}
	else if ((range >> 4) == 1)
	{
		shift_bits	= 4;
		range		= (range << 4) & 0x1ff;
		offset		= ((offset << 4) | ((bsm_data >> 28) & 0xf)) & 0x3ff;
		bsm_data	= bsm_data << 4;
	}
	else if ((range >> 3) == 1)
	{
		shift_bits	= 5;
		range		= (range << 5) & 0x1ff;
		offset		= ((offset << 5) | ((bsm_data >> 27) & 0x1f)) & 0x3ff;
		bsm_data	= bsm_data << 5;
	}
	else if ((range >> 2) == 1)
	{
		shift_bits	= 6;
		range		= (range << 6) & 0x1ff;
		offset		= ((offset << 6) | ((bsm_data >> 26) & 0x3f)) & 0x3ff;
		bsm_data	= bsm_data << 6;
	}	
	else if ((range >> 1) == 1)
	{
		shift_bits	= 7;
		range		= (range << 7) & 0x1ff;
		offset		= ((offset << 7) | ((bsm_data >> 25) & 0x7f)) & 0x3ff;
		bsm_data	= bsm_data << 7;
	}

	*bsm_data_ptr	= is_bp_mode ? bsm_data_eq : bsm_data;
	*range_ptr		= is_bp_mode ? *range_ptr : range;
	*offset_ptr		= is_bp_mode ? offset_eq : offset;
	*shift_bits_ptr = is_bp_mode ? 1 : shift_bits;
	*context_ptr	= is_bp_mode ? *context_ptr : ((val_mps << 6) | state);
	val_bin			= is_bp_mode ? bit_eq : bit;	
	
	if(g_trace_enable_flag&TRACE_ENABLE_VLD)	
	{	
		VLD_FPRINTF (g_hvld_trace_fp, "bin_dec: %d\n", val_bin);
	}
	
	return val_bin;	
}
