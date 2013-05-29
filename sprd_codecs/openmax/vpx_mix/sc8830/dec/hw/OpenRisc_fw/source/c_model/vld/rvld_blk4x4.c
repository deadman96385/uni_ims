/*rvld_blk4x4.c*/
//#include "rvdec_basic.h"
#include "rvld_mode.h"
#include "rvld_global.h"
#include "buffer_global.h"

int g_dsc4x4_idx;
int g_access_times[6][27];

int ComputeDsc4x4Idx (int svlc_mbtype, int is_chroma, int is_intra)
{
	int idx;
	
	if (!is_chroma)
	{		
		idx =	(svlc_mbtype == 3)	? 2 :
				(svlc_mbtype == 2)	? 1 :
				(svlc_mbtype == 1)	? 0 : 3;
	}
	else
	{		
		idx = is_intra ? 4 : 5;
	}

	return idx;	
}

void RvldDecodeBlk4x4 (
					   int	mb_type, 
					   int	blk_type,
					   int	blk_id
					   )
{
	int		is_dc_y;
	int		is_ac_y;
	int		is_chroma;
	int		is_intra;
	int		is_intra16;
	int		is_inter16;
	int		is_blk16;	
	int		svlc_mbtype;
	int		dsc4x4_base_addr;
	int		dsc2x2_base_addr0;
	int		dsc2x2_base_addr1;
	int		base_addr;
	int		dsc4x4;
	int		dsc2x2;
	int		blk2x2_id;
	int		cbp_blk2x2;
	int		coeff_num;
	int		dsc_lev;
	int		level;
	int		max_lev;
	int		sign;
	int		coeff;
	int		extra_bits;
	MAX_REG_ARR_T * max_reg_ptr;

	int		coeff_idx;
	uint32	nz_flag = 0;
	uint32	blk_base_addr;
	uint32	nzf_base_addr;
	uint32	rvld_dbuf_addr;

	int		dct_order_idx;

	int		dct_io_map[4] = {0, 2, 1, 3};	

	is_dc_y   = (blk_type == LUMA_DC_BLK)   ? 1 : 0;
	is_ac_y   = (blk_type == LUMA_AC_BLK)   ? 1 : 0;
	is_chroma = (blk_type == CHROMA_AC_BLK) ? 1 : 0;

	is_intra	= (mb_type == INTRA16X16) || (mb_type == INTRA4X4);
	is_intra16	= (mb_type == INTRA16X16);
	is_inter16	= (mb_type == INTER16X16);

	is_blk16 = (is_intra16 || is_inter16) ? 1 : 0;

	svlc_mbtype	=	is_dc_y						? 3 :
					(is_intra16 || is_inter16)	? 2 : 
					is_intra					? 1 : 0;

	blk_base_addr = (blk_type == LUMA_DC_BLK) ? COEFF_LUMA_DC_BASE : 
					(blk_type == LUMA_AC_BLK) ? COEFF_LUMA_AC_BASE + blk_id*8 : COEFF_CHROMA_AC_BASE + blk_id*8;

	nzf_base_addr = (blk_type == LUMA_DC_BLK) ? NZFLAG_LUMA_DC_BASE : 
					(blk_type == LUMA_AC_BLK) ? NZFLAG_LUMA_AC_BASE + blk_id : NZFLAG_CHROMA_AC_BASE + blk_id;

	/*load max_reg*/

	if (!is_chroma)
	{
	
		dsc4x4_base_addr =	(svlc_mbtype == 3)	? INTRA_L4X4DSC2_BASE_ADDR :
							(svlc_mbtype == 2)	? INTRA_L4X4DSC1_BASE_ADDR :
							(svlc_mbtype == 1)	? INTRA_L4X4DSC0_BASE_ADDR : INTER_L4X4DSC_BASE_ADDR;
	}
	else
	{		
		dsc4x4_base_addr = is_intra ? INTRA_C4X4DSC_BASE_ADDR : INTER_C4X4DSC_BASE_ADDR;
	}

	if (!is_chroma)
	{	
		dsc2x2_base_addr0 = (svlc_mbtype != 0) ? INTRA_L2X2DSC0_BASE_ADDR : INTER_L2X2DSC0_BASE_ADDR;
		dsc2x2_base_addr1 = (svlc_mbtype != 0) ? INTRA_L2X2DSC1_BASE_ADDR : INTER_L2X2DSC1_BASE_ADDR;
	}
	else
	{	
		dsc2x2_base_addr0 = is_intra ? INTRA_C2X2DSC0_BASE_ADDR : INTER_C2X2DSC0_BASE_ADDR;
		dsc2x2_base_addr1 = is_intra ? INTRA_C2X2DSC1_BASE_ADDR : INTER_C2X2DSC1_BASE_ADDR;
	}

	g_dsc4x4_idx = ComputeDsc4x4Idx (svlc_mbtype, is_chroma, is_intra);

	/*decoding blk4x4*/
	dsc4x4 = RvldHuffDecoder (&g_rvld_mode_ptr->c84_reg_arr, dsc4x4_base_addr, DSC_4X4, 0);

	FPRINTF (g_rvld_trace_fp, "dsc_4x4: %0x\n", dsc4x4);

	for (blk2x2_id = 0; blk2x2_id < 4; blk2x2_id++)
	{

		cbp_blk2x2 = (blk2x2_id == 0) ? (dsc4x4 >> 3) : 
					 (blk2x2_id == 1) ? (dsc4x4 >> 2) & 1 : 
					 (blk2x2_id == 2) ? (dsc4x4 >> 1) & 1 : (dsc4x4 & 1);

//		if ((g_mb_y == 4) && (g_mb_x == 1) && (blk_id == 8) && (blk2x2_id == 0))
//			printf ("");
	
		if (cbp_blk2x2)
		{
			if (blk2x2_id == 0)
			{
				dsc2x2 = dsc4x4 >> 3;
			}
			else
			{
				max_reg_ptr = (blk2x2_id == 3) ? &g_rvld_mode_ptr->dsc2x2_reg_arr1 : &g_rvld_mode_ptr->dsc2x2_reg_arr0;
				base_addr	= (blk2x2_id == 3) ? dsc2x2_base_addr1 : dsc2x2_base_addr0;
			
				dsc2x2 = RvldHuffDecoder (max_reg_ptr, base_addr, DSC_2X2, 0);			
			}

			FPRINTF (g_rvld_trace_fp, "blk2x2_id: %d, dsc_2x2: %0x\n", blk2x2_id, dsc2x2);
		
			/*decoder blk2x2*/			
			if (is_chroma)
			{
				max_reg_ptr = is_intra ? &g_rvld_mode_ptr->intra_lev_reg_arr : &g_rvld_mode_ptr->inter_lev_reg_arr;
			}
			else
			{
				max_reg_ptr = svlc_mbtype ? &g_rvld_mode_ptr->intra_lev_reg_arr : &g_rvld_mode_ptr->inter_lev_reg_arr;
			}
				
			for (coeff_num = 0; coeff_num < 4; coeff_num++)
			{
				dsc_lev =	(coeff_num == 0) ? g_dsc_to_l0[dsc2x2] : 
							(coeff_num == 1) ? g_dsc_to_l1[dsc2x2] : 
							(coeff_num == 2) ? g_dsc_to_l2[dsc2x2] : g_dsc_to_l3[dsc2x2];
		
				max_lev  = (coeff_num == 0) ? 3 : 2;

				if ((blk_id == 4) && (blk2x2_id == 0) && (coeff_num == 2))
					printf ("");

		
				if (dsc_lev)
				{
					coeff = dsc_lev;

					coeff_idx = (blk2x2_id >> 1) * 8 + (blk2x2_id & 1) * 2 +			//block base
								((coeff_num < 2) ? coeff_num : (coeff_num + 2));		//coeff offset

					if (blk2x2_id == 2)
					{
						coeff_idx = (coeff_idx == 9)  ? 12 :
									(coeff_idx == 12) ? 9  : coeff_idx;
					}

					if (dsc_lev == max_lev)
					{
						base_addr = ((svlc_mbtype == 0) ? (INTER_CODE_BASE + INTER_LEV_CODE_BASE) : (INTRA_LEV_CODE_BASE)) * 4;
						level = RvldHuffDecoder (max_reg_ptr, base_addr, DSC_LEV, 0);

						if (level == 25)
							printf ("");
					//	if (level >= MAX_EXTRA_LEVEL - 1)
						{
							extra_bits = level - (MAX_EXTRA_LEVEL - 1);
							if(extra_bits > 1)
							{
								level = read_nbits (extra_bits - 1);
								level += ((1<<(extra_bits-1))+ MAX_EXTRA_LEVEL - 1);
							}
						}

						coeff += level;
					}				
					
					/*get sign*/
					sign = read_nbits (1);

					coeff = sign ? -coeff : coeff;	

					dct_order_idx = (dct_io_map[(coeff_idx & 0xc)>>2] << 2) + dct_io_map[coeff_idx & 0x3];
					
					rvld_dbuf_addr = blk_base_addr*2 + dct_order_idx;

					if (rvld_dbuf_addr == 1)
						printf ("");

					((int16 *)vsp_dct_io_0)[rvld_dbuf_addr] = coeff;

					FPRINTF (g_rvld_trace_fp, "coeff: %0x\n", coeff);

					nz_flag = nz_flag | (1 << dct_order_idx);
				}		
			}
		}
	}

	rvld_dbuf_addr = nzf_base_addr;

	vsp_dct_io_0[rvld_dbuf_addr] = nz_flag;	
}