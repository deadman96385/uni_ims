#include <memory.h>
#include "sci_types.h"
#include "video_common.h"
#include "iict_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "hvld_mode.h"

int16 s_quantMatrix[6][16] = 
{
	{160, 208, 160, 208, 208, 256, 208, 256,	160, 208, 160, 208,	208,256, 208, 256, },
	{176, 224, 176, 224, 224, 288, 224, 288,	176, 224, 176, 224,	224, 288, 224, 288,},
	{208, 256, 208, 256, 256, 320, 256, 320,	208, 256, 208, 256,	256, 320, 256, 320,},
	{224, 288, 224, 288, 288, 368, 288, 368,	224, 288, 224, 288,	288, 368, 288, 368,},
	{256, 320, 256, 320, 320, 400, 320, 400,	256, 320, 256, 320,	320, 400, 320, 400,},
	{288, 368, 288, 368, 368, 464, 368, 464,	288, 368, 288, 368,	368, 464, 368, 464,},
};

uint8 g_blKIndex[16] = 
{
	 0,   1,  4,  5 , 
     2,   3,  6,  7 , 
     8,   9, 12,  13, 
     10, 11, 14,  15, 
};

// LOCAL int32 g_postrans [4] = {0, 2, 1, 3};
static int32 g_postrans[16] =
	{	0, 2, 1, 3, 
		8, 10, 9,11,
		4, 6, 5, 7,
		12, 14,13, 15
	};

int16 inverse_quant (int16 coeff, int32 asic_index, int32 is_luma)
{
	int32 qp_per;
	int32 qp_rem;
	int32 shift;
	int16 *quant_ptr = NULL;
	int32 normal_idx;

	normal_idx = g_postrans[asic_index];/*g_postrans [asic_index&3] * 4 + g_postrans [(asic_index>>2)];*/

	if (is_luma)
	{
		qp_per = (g_dct_reg_ptr->iict_cfg1>>12)&0xf;
		qp_rem = (g_dct_reg_ptr->iict_cfg1>>4)&0xf;
	}else
	{
		qp_per = (g_dct_reg_ptr->iict_cfg1>>8)&0xf;
		qp_rem = (g_dct_reg_ptr->iict_cfg1>>0)&0xf;
	}

	quant_ptr = s_quantMatrix[qp_rem];

	if (normal_idx == 0) //first 
	{
		if (is_luma)
		{
			int32 mb_type = (g_dct_reg_ptr->iict_cfg0 >> 28) & 0x3;

			if (mb_type == 0) //i16x16, dc
			{
				if (qp_per < 6)
				{
					int32 qp_const = (1<<(5-qp_per));

					shift = 6 - qp_per;

					coeff = ((coeff * quant_ptr[normal_idx] +qp_const)>>shift);
				}else
				{
					shift = qp_per - 6;
					coeff = ((coeff * quant_ptr[normal_idx]) << shift);
				}

				return coeff;
			}
		}else //chroma dc
		{
			coeff = (((coeff * quant_ptr[normal_idx])<<qp_per)>>5);

			return coeff;
		}	
	}
	
	if (qp_per < 4)
	{
		int32 qp_const = (1<<(3-qp_per));

		shift = 4 - qp_per;

		coeff = ((coeff * quant_ptr[normal_idx] +qp_const)>>shift);
	}else
	{
		shift = qp_per - 4;
		coeff = ((coeff * quant_ptr[normal_idx]) << shift);
	}
	
	return coeff;
}

int16 get_asic_coeff (int16 *asic_blk, int32 *pNzflag, int32 asic_index)
{
	int16 coeff;
// 	int32 asic_index;
	int32 coeff_not_zero;

// 	asic_index = g_postrans [normal_idx&3] * 4 + g_postrans [(normal_idx>>2)];

	coeff_not_zero = (*pNzflag >> asic_index)&0x1;

	if (coeff_not_zero)
	{
		coeff = asic_blk[asic_index];		
	}else
	{
		coeff = 0;
	}

	return coeff;
}
int16 get_one_coeff_from_asic_order_bfr(int16 *asic_blk, 
										int32 *pNzflag, 
										int32 asic_index,  //coeff position index
										int32 blk4x4Idx,   //block4x4 index
										int32 is_hadamard_itrans, 
										int32 is_luma)
{
	int16 coeff;
	int32 need_iq = (is_hadamard_itrans ? 0 : 1);
	int32 mb_type = (g_dct_reg_ptr->iict_cfg0 >> 28) & 0x3;

	//the first coeff
	if (asic_index == 0)
	{
		if (is_luma)
		{
			if ((mb_type == 0) && (!is_hadamard_itrans)) 
			{
				int16 *dc4x4_normal_blk_ptr = (int16 *)(vsp_dct_io_0 + COEFF_LUMA_DC_BASE);
				coeff = dc4x4_normal_blk_ptr[g_blKIndex[blk4x4Idx]];
			}else
			{
				coeff = get_asic_coeff (asic_blk, pNzflag, asic_index);
			}
		}else //chroma
		{
			int16 *dc2x2_normal_blk_ptr = (int16 *)(vsp_dct_io_0 + COEFF_CHROMA_DC_BASE);
			coeff = dc2x2_normal_blk_ptr[blk4x4Idx];		
		}
	}else //other coeff
	{
		coeff = get_asic_coeff (asic_blk, pNzflag, asic_index);
	}
	
	if (need_iq)
	{
		coeff = inverse_quant (coeff, asic_index, is_luma);
	}
	
	return coeff;
}

void itrans4x4 (int32 is_hadamard_itrans, int32 blk4x4Idx, int32 cbp26, int32 is_luma)
{
	int16 *asic_blk4x4_ptr = NULL; 
	int32 *pNzflag = NULL;
	int16 blk_tmp[16];
	int32 m0, m1, m2, m3;
	int32 n0, n1, n2, n3;
	int32 i;

	memset(blk_tmp, 0, sizeof(int16)*16);

	if (is_hadamard_itrans)
	{
		asic_blk4x4_ptr = (int16 *)(vsp_dct_io_0 + COEFF_LUMA_DC_BASE);
		pNzflag = vsp_dct_io_0 + NZFLAG_LUMA_DC_BASE;
	}else
	{
		if (is_luma)
		{
			asic_blk4x4_ptr = (int16 *)(vsp_dct_io_0 + COEFF_LUMA_AC_BASE + 8 * blk4x4Idx);
			pNzflag = vsp_dct_io_0 + NZFLAG_LUMA_AC_BASE + blk4x4Idx;
		}else
		{
			asic_blk4x4_ptr = (int16 *)(vsp_dct_io_0 + COEFF_CHROMA_AC_BASE + 8 * blk4x4Idx);
			pNzflag = vsp_dct_io_0 + NZFLAG_CHROMA_AC_BASE + blk4x4Idx;
		}		
	}

	//first trans
	for (i = 0; i < 4; i++)
	{
		m0 = get_one_coeff_from_asic_order_bfr(asic_blk4x4_ptr, pNzflag, i*4+0, blk4x4Idx, is_hadamard_itrans, is_luma);

		if ((!is_hadamard_itrans) && (!(cbp26 & (1<<(blk4x4Idx+(is_luma?0:16))))))
		{
			m2 = 0;
			m1 = 0;
			m3 = 0;
		}else
		{
			m2 = get_one_coeff_from_asic_order_bfr(asic_blk4x4_ptr, pNzflag, i*4+1, blk4x4Idx, is_hadamard_itrans, is_luma);
			m1 = get_one_coeff_from_asic_order_bfr(asic_blk4x4_ptr, pNzflag, i*4+2, blk4x4Idx, is_hadamard_itrans, is_luma);
			m3 = get_one_coeff_from_asic_order_bfr(asic_blk4x4_ptr, pNzflag, i*4+3, blk4x4Idx, is_hadamard_itrans, is_luma);
		}
		
		n0 = m0 + m2;
		n1 = m0 - m2;
		if (is_hadamard_itrans)
		{
			n2 = m1 - m3;
			n3 = m1 + m3;
		}else
		{
			n2 = (m1>>1) - m3;
			n3 = m1 + (m3>>1);
		}

		blk_tmp[i*4+0] = n0 + n3;
		blk_tmp[i*4+1] = n1 + n2;
		blk_tmp[i*4+2] = n1 - n2;
		blk_tmp[i*4+3] = n0 - n3;
	}

	//second itrans
	for (i = 0; i < 4; i++)
	{
		m0 = blk_tmp[0*4+i];
		m1 = blk_tmp[2*4+i];
		m2 = blk_tmp[1*4+i];
		m3 = blk_tmp[3*4+i];

		n0 = m0 + m2;
		n1 = m0 - m2;
		if (is_hadamard_itrans)
		{
			n2 = m1 - m3;
			n3 = m1 + m3;
		}else
		{
			n2 = (m1>>1) - m3;
			n3 = m1 + (m3>>1);
		}

		m0 = n0 + n3;
		m1 = n1 + n2;
		m2 = n1 - n2;
		m3 = n0 - n3;

		if(is_hadamard_itrans)
		{
			asic_blk4x4_ptr[0*4+i] = (int16)(m0);
			asic_blk4x4_ptr[1*4+i] = (int16)(m1);
			asic_blk4x4_ptr[2*4+i] = (int16)(m2);
			asic_blk4x4_ptr[3*4+i] = (int16)(m3);
		}else
		{
			asic_blk4x4_ptr[0*4+i] = (int16)((m0+(1<<5))>>6);
			asic_blk4x4_ptr[1*4+i] = (int16)((m1+(1<<5))>>6);
			asic_blk4x4_ptr[2*4+i] = (int16)((m2+(1<<5))>>6);
			asic_blk4x4_ptr[3*4+i] = (int16)((m3+(1<<5))>>6);
		}		
	}

	return;
}

//2x2 chroma dc is in normal order.
int16 get_one_coeff_from_asic_order_bfr2x2 (int16 *asic_blk, int32 *pNzflag, int32 normal_idx)
{
	int32 coeff_not_zero;
	int32 asic_index = normal_idx;
	int16 coeff;
	
	coeff_not_zero = (*pNzflag >> asic_index)&0x1;;

	if (coeff_not_zero)
	{
		coeff = asic_blk[asic_index];
	}else
	{
		coeff = 0;
	}

	return coeff;
}

void itrans2x2 (int32 is_v)
{
	int16 *asic_blk2x2_ptr = (int16 *)(vsp_dct_io_0 + COEFF_CHROMA_DC_BASE + is_v*2);
	int32 *pNzflag = vsp_dct_io_0 + NZFLAG_CHROMA_DC_BASE + is_v;
	int32 m0, m1, m2, m3;
	int32 n0, n1, n2, n3;

	m0 = get_one_coeff_from_asic_order_bfr2x2 (asic_blk2x2_ptr, pNzflag, 0);
	m1 = get_one_coeff_from_asic_order_bfr2x2 (asic_blk2x2_ptr, pNzflag, 1);
	m2 = get_one_coeff_from_asic_order_bfr2x2 (asic_blk2x2_ptr, pNzflag, 2);
	m3 = get_one_coeff_from_asic_order_bfr2x2 (asic_blk2x2_ptr, pNzflag, 3);

	n0 = m0 + m1;
	n1 = m2 + m3;
	n2 = m0 - m1;
	n3 = m2 - m3;

	asic_blk2x2_ptr[0] = (int16)(n0 + n1);
	asic_blk2x2_ptr[1] = (int16)(n2 + n3);
	asic_blk2x2_ptr[2] = (int16)(n0 - n1);
	asic_blk2x2_ptr[3] = (int16)(n2 - n3);

	return;
}
