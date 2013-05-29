/******************************************************************************
 ** File Name:    dct_top.c		                                              *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
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

const uint8 g_ASIC_DCT_Matrix[64] =
{
	0,	32,	16,	48,	8,	40,	24,	56,
		4,	36,	20,	52,	12,	44,	28,	60,
		2,	34,	18,	50,	10,	42,	26,	58,
		6,	38,	22,	54,	14,	46,	30,	62,
		1,	33,	17,	49,	 9,	41,	25,	57,
		5,	37,	21,	53,	13,	45,	29,	61,
		3,	35,	19,	51,	11,	43,	27,	59,
		7,	39,	23,	55,	15,	47,	31,	63,
};		

void AsicOrderBlock (int16 * pBlk, int16 * pBlkDst)
{
	int i, j;
	int16 a, b;
	int16 blkTmp [64];
	
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
			blkTmp [i*8 + j] = pBlk [i*8 + j];
	}
	
	for (i = 0; i < 8; i++)
	{
		a = blkTmp [8*i+4];
		b = blkTmp [8*i+1];
		blkTmp [8*i+1] = a;
		blkTmp [8*i+4] = b;
		
		a = blkTmp [8*i+6];
		b = blkTmp [8*i+3];
		blkTmp [8*i+3] = a;
		blkTmp [8*i+6] = b;
	}
	
	/*x and y transpose*/
	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			pBlkDst [i*8 + j] = blkTmp [j*8 + i];
		}
	}
	
	/*adjust order*/
	/*horizontal order adjust to "0 4 2 6 1 5 3 7"*/
	for (i = 0; i < 8; i++)
	{
		a = pBlkDst [8*i+4];
		b = pBlkDst [8*i+1];
		pBlkDst [8*i+1] = a;
		pBlkDst [8*i+4] = b;
		
		a = pBlkDst [8*i+6];
		b = pBlkDst [8*i+3];
		pBlkDst [8*i+3] = a;
		pBlkDst [8*i+6] = b;
	}	
}

void dct_module()
{
	int32 standard;
	int32 blk_num;

	standard = (g_glb_reg_ptr->VSP_CFG0>>8)&0x7;

	if(standard != VSP_JPEG)
	{
		blk_num = 6;
	}else
	{
		blk_num = g_block_num_in_one_mcu;
		g_dct_reg_ptr->IDCT_IN_CBP = 0x3f;
	}

// 	if(standard != VSP_JPEG)
	{
		int32 F_trans_en = (g_dct_reg_ptr->DCT_CONFIG >> 0) & 0x01;
		int32 dct_quant_en = (g_dct_reg_ptr->DCT_CONFIG >> 8) & 0x01;
		int32 Inter_mode_en = (g_dct_reg_ptr->DCT_CONFIG >> 5 ) & 0x01;
		int32 blk_idx;

		if(F_trans_en) //dct
		{
			int16 *pSrc;
			int16 *pDst;
			int16 dctAsic[64];
			int8 *pint8Src; //for jpeg encoder
			
			g_dct_reg_ptr->DCT_OUT_CBP = 0;

			pSrc = (int16 *)vsp_dct_io_0;
			pDst = (int16 *)vsp_dct_io_1;
			pint8Src = (int8 *)vsp_dct_io_0 ;
				
			for(blk_idx = 0; blk_idx < blk_num; blk_idx++)
			{	
				if(standard != VSP_JPEG)
				{
					if(!Inter_mode_en)//intra block
					{
						Mp4Enc_Asic_Dct_Intra((int8 *)(pSrc+blk_idx*64), pDst+blk_idx*64, 0);
					}else//inter block
					{
						Mp4Enc_Asic_Dct_Inter(pSrc+blk_idx*64, pDst+blk_idx*64);
					}
				}else
				{
					JpegEnc_Asic_Dct_Intra((int8 *)(pint8Src+blk_idx*64), pDst+blk_idx*64, 0);
					(pDst+blk_idx*64)[0] -= 1024;//DC_DIFF;
				}
					
				if(dct_quant_en)
				{
					quant(pDst+blk_idx*64, Inter_mode_en, standard, blk_idx);
				}
				
				AsicOrderBlock(pDst+blk_idx*64, dctAsic);
				memcpy(pDst+blk_idx*64, dctAsic, 64*sizeof(int16));
	
				printf_DCTCoeff_block(g_fp_dct_tv, pDst+blk_idx*64);
			}

			printf_nonezeroFlag_enc(blk_num);
		}else //idct
		{
			int16 dctNormal[64] = {0};
			int16 *pSrc;
			int16 *pDst;

			printf_mb_coeff_flag(vsp_dct_io_1);

			pSrc = (int16 *)vsp_dct_io_1;
			pDst = (int16 *)vsp_dct_io_0;
			
			for(blk_idx = 0; blk_idx < blk_num; blk_idx++)
			{
				memset(dctNormal, 0, 64*sizeof(int16));
			 	AsicOrderBlock(pSrc+blk_idx*64, dctNormal);
			 	memcpy(pSrc+blk_idx*64, dctNormal, 64*sizeof(int16));			
				
				if(dct_quant_en)
				{
					iquant(pSrc+blk_idx*64, Inter_mode_en, blk_idx);
				}
				
				Mp4_Asic_idct1(pSrc+blk_idx*64);
			}
			
			printf_mb_idct(g_fp_idct_tv, vsp_dct_io_1, g_dct_reg_ptr->IDCT_IN_CBP);
		}//if(F_trans_en) //dct
	}//if(standard != VSP_JPEG)		
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
