/*rvld_cbp.c*/
#include "rvld_mode.h"
#include "rvld_global.h"
#include "bsm_global.h"

/*cbp desciptor to cn mapping*/
uint8 g_dsc_to_l0 [108] = 
{
	0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3,
		3, 3, 3, 3,	
};

uint8 g_dsc_to_l1 [108] = 
{
	0, 0, 0, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		2, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 2, 2, 2, 2, 2,
		2, 2, 2, 2,
};

uint8 g_dsc_to_l2 [108] = 
{
	0, 0, 0, 1, 1, 1, 2, 2,
		2, 0, 0, 0, 1, 1, 1, 2,
		2, 2, 0, 0, 0, 1, 1, 1,
		2, 2, 2, 0, 0, 0, 1, 1,
		1, 2, 2, 2, 0, 0, 0, 1,
		1, 1, 2, 2, 2, 0, 0, 0,
		1, 1, 1, 2, 2, 2, 0, 0,
		0, 1, 1, 1, 2, 2, 2, 0,
		0, 0, 1, 1, 1, 2, 2, 2,
		0, 0, 0, 1, 1, 1, 2, 2,
		2, 0, 0, 0, 1, 1, 1, 2,
		2, 2, 0, 0, 0, 1, 1, 1,
		2, 2, 2, 0, 0, 0, 1, 1,
		1, 2, 2, 2,
};

uint8 g_dsc_to_l3 [108] = 
{
	0, 1, 2, 0, 1, 2, 0, 1,
		2, 0, 1, 2, 0, 1, 2, 0,
		1, 2, 0, 1, 2, 0, 1, 2,
		0, 1, 2, 0, 1, 2, 0, 1,
		2, 0, 1, 2, 0, 1, 2, 0,
		1, 2, 0, 1, 2, 0, 1, 2,
		0, 1, 2, 0, 1, 2, 0, 1,
		2, 0, 1, 2, 0, 1, 2, 0,
		1, 2, 0, 1, 2, 0, 1, 2,
		0, 1, 2, 0, 1, 2, 0, 1,
		2, 0, 1, 2, 0, 1, 2, 0,
		1, 2, 0, 1, 2, 0, 1, 2,
		0, 1, 2, 0, 1, 2, 0, 1,
		2, 0, 1, 2,
};

uint32 RvldDecodeCBP (int mb_type, int cbp_type)
{
	int		blk_id;
	uint32	bsm_rdata;
	int		code_len;
	uint32	cbp_dsc;
	int		cx;
	int		base_addr;
	uint32	dsc8x8;
	uint32	cbp = 0;
	uint32	cbp_c;
	int		c0;
	int		c1;
	int		c2;
	int		c3;
	int		cx_offset;

	/*load cbp max_reg*/
	LoadMaxRegArray (mb_type, DSC_CBP, 0, 0, 0, 0);	

	base_addr =	(cbp_type == 2) ? INTRA_CBP1_BASE_ADDR :
				(cbp_type == 1) ? INTRA_CBP0_BASE_ADDR : INTER_CBP_BASE_ADDR;
	
	/*decoder cbp_dsc*/
	cbp_dsc = RvldHuffDecoder (&g_rvld_mode_ptr->c84_reg_arr, base_addr, DSC_CBP, cbp_type);

	FPRINTF (g_rvld_trace_fp, "cbp_dsc: %08x\n", cbp_dsc);

	if (cbp_dsc & 0xf)
	{
		cx = (cbp_dsc & 1) + ((cbp_dsc>>1) & 1) + ((cbp_dsc>>2) & 1) + ((cbp_dsc>>3) & 1) - 1;

		LoadMaxRegArray (mb_type, DSC_8X8, 0, cx, 0, 0);
		
		cx_offset = ((cbp_type == 2) ? (44*4) : 
					(cbp_type == 0) ? INTER_CODE_BASE*4 : 0) 
					+ cx * 44;

		/*decoder cbp for luma*/
		for (blk_id = 0; blk_id < 4; blk_id++)
		{
			int shift_bits;

			if ((cbp_dsc >> (3 - blk_id)) & 1)
			{	
				dsc8x8 = RvldHuffDecoder (&g_rvld_mode_ptr->c84_reg_arr, cx_offset, DSC_8X8, 0);

				shift_bits = ((blk_id >> 1) & 1) * 8 + (blk_id & 1) * 2;
				
				cbp |= ( (((dsc8x8 >> 0) & 1) << 5) | (((dsc8x8 >> 1) & 1) << 4) | (((dsc8x8 >> 2) & 1) << 1) | (((dsc8x8 >> 3) & 1) << 0) ) << shift_bits;	
			}			
		}
	}

	/*decoder cbp for chroma*/
	cbp_c = cbp_dsc >> 4;

	c0 = g_dsc_to_l0[cbp_c];
	c1 = g_dsc_to_l1[cbp_c];
	c2 = g_dsc_to_l2[cbp_c];
	c3 = g_dsc_to_l3[cbp_c];

	bsm_rdata = show_nbits (32);
	code_len  = 0;

	if (c0 == 2)
	{
		cbp |= 0x11 << 16;
	}
	else if (c0 == 1)
	{
		c0 = (bsm_rdata >> 31) & 1;
		cbp |= c0 << 16;
        cbp |= (c0^1) << 20;

		bsm_rdata = bsm_rdata << 1;
		code_len++;
	}

	if (c1 == 2)
	{
		cbp |= 0x11 << 17;
	}
	else if (c1 == 1)
	{
		c1 = (bsm_rdata >> 31) & 1;
		cbp |= c1 << 17;
        cbp |= (c1^1) << 21;
		
		bsm_rdata = bsm_rdata << 1;
		code_len++;
	}

	if (c2 == 2)
	{
		cbp |= 0x11 << 18;
	}
	else if (c2 == 1)
	{
		c2 = (bsm_rdata >> 31) & 1;
		cbp |= c2 << 18;
        cbp |= (c2^1) << 22;
		
		bsm_rdata = bsm_rdata << 1;
		code_len++;
	}

	if (c3 == 2)
	{
		cbp |= 0x11 << 19;
	}
	else if (c3 == 1)
	{
		c3 = (bsm_rdata >> 31) & 1;
		cbp |= c3 << 19;
        cbp |= (c3^1) << 23;
		
		bsm_rdata = bsm_rdata << 1;
		code_len++;
	}

	flush_nbits (code_len);

	FPRINTF (g_cbp_val_fp, "%08x\n", cbp);

	return cbp;
}