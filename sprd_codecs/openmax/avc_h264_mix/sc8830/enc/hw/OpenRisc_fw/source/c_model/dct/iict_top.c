#include <memory.h>
#include "sci_types.h"
#include "video_common.h"
#include "iict_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "hvld_mode.h"

#define ISQT_FPRINT //fprintf

void trace_after_iqt_for_asic_compare ()
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int32 i;
	int16 *iqt_result_ptr = (int16 *)(vsp_dct_io_0 + COEFF_LUMA_AC_BASE);

	for (i = 0; i < (16*24); i++)
	{
		FormatPrintHexNum(iqt_result_ptr[i], g_fp_idct_tv);
	}
}
	return;
}

void PrintfResidual (int is_intra16, int cbp)
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int		i;
	int		val;
	int		blk_id;
	int		blk_cbp;
	int		base_addr;

	//printf inversed transform result of luma dc
	if (is_intra16)
	{
		base_addr = 192;

		for (i = 0; i < 8; i++)
		{
			val = vsp_dct_io_0[base_addr + i];
			FPRINTF (g_fp_idct_tv, "%08x\n", val);
		}
	}

	//printf inversed transform result of chroma dc
	if ((cbp >> 24) & 1)
	{
		base_addr = 200;

		for (i = 0; i < 4; i++)
		{
			val = vsp_dct_io_0[base_addr + i];
			FPRINTF (g_fp_idct_tv, "%08x\n", val);
		}
	}

/**********************************************************************************
	printf 24 4x4 blk with non-zero coefficient
	note: if is_intra16, all luma coeff should be printed
	      if original cbp > 15, all chroma coeff should be printed
************************************************************************************/
	for (blk_id = 0; blk_id < 24; blk_id++)
	{
		//blk_cbp;
		if (blk_id < 16)
		{
			if (is_intra16)
				blk_cbp = 1;
			else
				blk_cbp = (cbp >> blk_id) & 1;
		}
		else
		{
			if ((cbp >> 24) & 1)
				blk_cbp = 1;
			else
				blk_cbp = 0;
		}
		
		if (blk_cbp)
		{	
			base_addr = blk_id * 8;	
			
			for (i = 0; i < 8; i++)
			{
				val = vsp_dct_io_0[base_addr + i];
				FPRINTF (g_fp_idct_tv, "%08x\n", val);
			}			
		}
	}
}
}

void iqt_module ()
{
	int32 mb_type = (g_dct_reg_ptr->iict_cfg0 >> 28) & 0x3;
	int32 cbp26 = (g_dct_reg_ptr->iict_cfg0 >> 0) & 0x3ffffff;
	int32 blk4x4Idx;
	int32 mb_x = ((g_glb_reg_ptr->VSP_CTRL0>>0) & 0xff);
	int32 mb_y = ((g_glb_reg_ptr->VSP_CTRL0>>8) & 0xff);

	if ((mb_y== 3) && (mb_x == 0))
	{
		printf("");
	}

	if ((mb_type == 2/*ipcm*/) || (mb_type == 1/*skip*/))
	{
		return;
	}

	if (mb_type == 0) //i16x16 mode
	{
		itrans4x4 (1, 0, cbp26, 1);
	}

	//luma
	for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++)
	{
		itrans4x4 (0, blk4x4Idx, cbp26, 1);
	}

	//chroma
	if (cbp26 & (1<<24))
	{
		itrans2x2 (0);  //u
		itrans2x2 (1);  //v
	}

	//chroma inverse trans
	for (blk4x4Idx = 0; blk4x4Idx < 8; blk4x4Idx++)//4*2(u and v)
	{
		if (cbp26 & (1<<24)) //if (cbp26 & (1<<(16+blk4x4Idx)))
		{
			itrans4x4(0, blk4x4Idx, cbp26, 0);
		}
	}

	PrintfResidual ((mb_type == 0), cbp26);
//	trace_after_iqt_for_asic_compare();
}
