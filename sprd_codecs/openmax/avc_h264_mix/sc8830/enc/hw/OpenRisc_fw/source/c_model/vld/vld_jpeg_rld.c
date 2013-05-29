/******************************************************************************
 ** File Name:    vld_jpeg_rld.c	   										  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/19/2008                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:   c model of bsmr module in mpeg4 decoder                    *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

#include "sc6800x_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
extern const uint8 g_DC_offset [17];

#if defined(JPEG_DEC)

uint8 huff_decode_DC (int bChroma)
{
	int i = 0;
	uint32 min_code = 0;
	uint32 max_code = 0;
	int base_addr = 0;
	uint16 code = 0;
	int val = 0;
	int codeLen = 0;
	uint8 huffVal = 0;
	int active, prevActive;
	uint32 activeReg = 0;
	volatile uint32 * pLut = NULL;

	pLut = bChroma ? g_vld_reg_ptr->Chrom_DC_LUT : g_vld_reg_ptr->Luma_DC_LUT;
	activeReg = (bChroma ? g_vld_reg_ptr->DC_VALID : (g_vld_reg_ptr->DC_VALID >> 16)) & 0xffff;

	for (i = 1; i <= 16; i++)
	{
		max_code = pLut [i-1];
		code = (uint16)read_nbits(1); 
		val = (val << 1) | code;
		active = (activeReg >> (16-i)) & 1;

		if (active)
		{
			if ((uint32)val <= max_code)
			{
				codeLen = i;

				if (codeLen > 1)
				{
					prevActive = (activeReg >> (15-(i-2))) & 1;

					min_code = (prevActive + (pLut [i-2] & 0xffff)) * 2;
				}
				else
				{
					min_code = 0;
				}

				base_addr = g_DC_offset [codeLen];

				break;
			}
		}		
	}

	huffVal = (uint8)((g_huffTab[base_addr + val - min_code] >> (16+bChroma*8)) & 0xff);

	return huffVal;
}

uint8 huff_decode_AC (int bChroma)
{
	int i = 0;
	uint32 min_code = 0;
	uint32 max_code = 0;
	int base_addr = 0;
	uint32 max_base = 0;
	uint16 code = 0;
	int val = 0;
	int codeLen = 0;
	uint8 huffVal = 0;
	int active = 0, prevActive = 0;
	uint32 activeReg = 0;
	volatile uint32 * pLut = NULL;
	
	pLut = bChroma ? g_vld_reg_ptr->Chroma_AC_LUT : g_vld_reg_ptr->Luma_AC_LUT;
	activeReg = (bChroma ? g_vld_reg_ptr->AC_VALID : (g_vld_reg_ptr->AC_VALID >> 16)) & 0xffff;
	
	for (i = 1; i <= 16; i++)
	{
		max_base = pLut [i-1];
		code = (uint16)read_nbits(1);
		val = (val << 1) | code;
		active = (activeReg >> (16-i)) & 1;
		
		if (active)
		{
			max_code = (max_base >> 8) & 0xffff;
			base_addr = max_base & 0xff;

			if ((uint32)val <= max_code)
			{
				codeLen = i;
				
				if (codeLen > 1)
				{
					prevActive = (activeReg >> (15-(i-2))) & 1;
					
					min_code = (prevActive + ((pLut [i-2]>>8) & 0xffff)) * 2;
				}
				else
				{
					min_code = 0;
					
				}			
				
				break;
			}
		}		
	}
	
	huffVal = (uint8)((g_huffTab[base_addr + val - min_code] >> (bChroma*8)) & 0xff);

	return huffVal;	
}

int32 huff_extend(int32 r, int32 s)
{
	int32 tmp;

	if(r < (1 << ((s)-1)))
	{
		tmp = r + (-1 << s) + 1;
	}else
	{
		tmp = r;
	}

	return tmp;
}

void jpeg_vld_Block(
					int16 * pBlk_asic,
					HUFF_TBL_T 	*dc_tbl,
					HUFF_TBL_T 	*ac_tbl,
					const uint8 *quant,
					int blkId,
					short DCPred,
					int8 bChroma
					)
{
	int run = 0;
	int32 s = 0, k = 0, r = 0, n = 0;
	int index = 0;
	int coeffNum = 0;
	int16 block[64];
	uint32 noneZeroFlag[2];
	
	memset((void *) block, 0, 64*sizeof(int16));

	s = huff_decode_DC (bChroma);
	r = read_nbits((uint8)s);

	block[0] = (int16)huff_extend(r, (uint8)s);

	for (k = 1; k < 64; k++)
	{
		r = huff_decode_AC(bChroma);
		
		s = r & 15;
		n = r >> 4;

		if (s)
		{
			k = k + n;
			run += n;

			r = read_nbits((uint8)s);
			index = jpeg_fw_zigzag_order [k];

			if (k<64)
			{
				block[index] = ((int16)huff_extend(r, s));
				run = 0;

				coeffNum++;
			}
			else
			{
				JPEG_ASSERT(0);
				break;
			}
		}
		else
		{
			if (n != 15) 
			{
				if (n != 0)
				{
					JPEG_ASSERT(0);
				}
				break;
			}
			k += 15;
			
			index = jpeg_fw_zigzag_order [k];

			run += 15;
		}
	}

	blk_to_asicOrder(block, pBlk_asic, noneZeroFlag);
	
	configure_noneZeroFlag_blk (noneZeroFlag, (uint32 *)(vsp_dct_io_1 + 192 + blkId*4));	
}
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 