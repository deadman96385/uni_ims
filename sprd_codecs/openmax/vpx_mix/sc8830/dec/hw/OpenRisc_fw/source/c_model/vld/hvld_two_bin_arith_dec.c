/*two_bin_arith_dec.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "bsm_global.h"

void TwoBinArithDec (
					 int		syn_type,			//syntax type for first bin to be decoded
					 uint8	*	ctx_bin0_ptr,		//first bin context to be decoded
					 uint8	*	ctx_bin1_ptr,		//second bin context to be decoded
					 int	*	bin0_ptr,			//returned first binary value
					 int	*	bin1_ptr,			//returned second binary value
					 int		glm_sfx_num
					 )
{
	uint32		bsm_data;

	int			shift_bits_bin0 = 0;
	int			shift_bits_bin1 = 0;
	int			shift_bits;

	int			is_bp_mode;
	int			val_bin0 = 0;
	int			val_bin1 = 0;

	int			bin1_active;

	bsm_data = show_nbits (32);

	is_bp_mode = (syn_type == CABAC_LEV_SIGN) || (syn_type == CABAC_GLM_PRX) || (syn_type == CABAC_GLM_SFX);

	/*instanciation two BiArithDec*/
	val_bin0 = BiArithDec (
							is_bp_mode,	
							&bsm_data,	
							&g_range,	 
							&g_offset,	
							ctx_bin0_ptr, 
							&shift_bits_bin0
							);

	/*********************************************************************************
	bsm_data, g_range, g_offset should use the forwarding value from instance0
	**********************************************************************************/
	is_bp_mode = 1;
	if (syn_type == CABAC_CODED_FLAG)
	{
		bin1_active = 0;
	}
	else if (syn_type == CABAC_SIG_LAST)
	{
		bin1_active = val_bin0;
		is_bp_mode  = 0;
	}
	else if (syn_type == CABAC_LEV_BIN0)
	{
		//if val_bin0 is 0, decode sign, else decode other bin
		bin1_active = 1;
		is_bp_mode  = (val_bin0 == 0) ? 1 : 0;	
	}
	else if (syn_type == CABAC_LEV_BIN_OTH)
	{
		bin1_active = 1;
		*ctx_bin1_ptr = *ctx_bin0_ptr;			//forward status of first bin engine to second engine
		is_bp_mode  = (val_bin0 == 0) ? 1 : 0;
	}
	else if (syn_type == CABAC_GLM_PRX)
	{
		bin1_active = val_bin0;
		is_bp_mode  = 1;
	}
	else if (syn_type == CABAC_GLM_SFX)
	{
		bin1_active = (glm_sfx_num > 1) ? 1 : 0;
		is_bp_mode  = 1;
	}
	else if (syn_type == CABAC_LEV_SIGN)
	{
		bin1_active = 0;
		is_bp_mode  = 0;
	}
	
	if (bin1_active)
	{
		val_bin1 = BiArithDec (
								is_bp_mode,	
								&bsm_data,	
								&g_range,	 
								&g_offset,	
								ctx_bin1_ptr, 
								&shift_bits_bin1
							);	
	}

	shift_bits = shift_bits_bin0 + shift_bits_bin1;

	flush_nbits(shift_bits);

	*bin0_ptr = val_bin0;
	*bin1_ptr = val_bin1;	
}