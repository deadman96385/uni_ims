/******************************************************************************
 ** File Name:    dct_trace.c	    										  *
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

#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

void printf_nonezeroFlag_enc (int32 blk_num)
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int i;
	
	for (i = 0; i < blk_num; i++)
	{
		fprintf_oneWord_hex (g_fp_dct_tv, *(vsp_dct_io_1 + 192+i*4));
		fprintf_oneWord_hex (g_fp_dct_tv, *(vsp_dct_io_1 + 192+i*4+1));
		fprintf_oneWord_hex (g_fp_dct_tv, *(vsp_dct_io_1 + 192+i*4+2));
		fprintf_oneWord_hex (g_fp_dct_tv, *(vsp_dct_io_1 + 192+i*4+3));
	}
}
}

//for dct, not for idct.20090703
void printf_DCTCoeff_block (FILE *pf, int16 * pBlock)
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int i;
	//int32 * pIntCoeff;
	int16 * pIntCoeff;

	uint32 code;
	/*
	pIntCoeff = (int32 *)pBlock;//dctAsic;
	
	for (i = 0; i < 32; i++)
	{
		code = *pIntCoeff++;	
		fprintf_oneWord_hex (pf, code);
	}
	*///weihu
	pIntCoeff = pBlock;
	for (i = 0; i < 8; i++)
	{
		code = *pIntCoeff++;	
	    fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pf, "%0.4x ", code&0xffff);
		fprintf (pf, "\n");
	}
    fprintf (pf, "---------------------------\n");
}
}

void copy_int16_to_uint8(int16 *pSrc, uint8 *pDst)
{
	int32 i;
	uint8 *mpDst = pDst;
	int16 *mpSrc = pSrc;
	for(i = 0; i < 64; i++)
	{
		mpDst[i] = (uint8)(IClip(0, 255, mpSrc[i]+128));
	}
}

void printf_one_block_uint8(FILE *pf, uint8 *pBlock)
{
	int i;
	uint8 *p8Coeff;
	uint32 code;

	p8Coeff = pBlock;

	for(i = 0; i < 32; i++)
	{
		code = (p8Coeff[0]) | ((p8Coeff[1]) << 16);
		
		fprintf_oneWord_hex(pf, code);

		p8Coeff+=2;
	}
}

void printf_mb_idct(FILE *pfIDCT, int32 *pDctIOBuf, int cbp)
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int i;
	int iBlk;
	int val;
	int32 * pBlk = pDctIOBuf;
	int32 blk_num = 6;
	int32 vid_standard	= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;

	if(VSP_JPEG == vid_standard)
	{
		uint8 tmp_blk[64];

		blk_num = g_block_num_in_one_mcu;
		
		for (iBlk = 0; iBlk < blk_num; iBlk++)
		{
			pBlk = pDctIOBuf + iBlk*32;

			copy_int16_to_uint8((int16 *)pBlk, tmp_blk)	;
			printf_one_block_uint8(pfIDCT, tmp_blk);
		}
	}else
	{
		for (iBlk = 0; iBlk < blk_num; iBlk++)
		{
			pBlk = pDctIOBuf + iBlk*32;

			//if (cbp & (32>>iBlk))//weihu
			{
				for (i = 0; i < 32; i++)
				{
					val = *pBlk++;

					fprintf_oneWord_hex (pfIDCT, val);
				}
			}
		}
	}
}
}

void printf_IDCTCoeff_block (FILE *pfIDCT, int16 * pBlock)//weihu
{
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int i;
	//int32 * pIntCoeff;
	int16 * pIntCoeff;

	uint32 code;
	/*
	pIntCoeff = (int32 *)pBlock;//dctAsic;
	
	for (i = 0; i < 32; i++)
	{
		code = *pIntCoeff++;	
		fprintf_oneWord_hex (pf, code);
	}
	*///weihu
	pIntCoeff = pBlock;
	for (i = 0; i < 8; i++)
	{
		code = *pIntCoeff++;	
	    fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		code = *pIntCoeff++;	
		fprintf (pfIDCT, "%0.4x ", code&0xffff);
		fprintf (pfIDCT, "\n");
	}
    fprintf (pfIDCT, "---------------------------\n");
}
}

void printf_mb_coeff_flag (int32 * pDCTIOBuf)
{
#if !defined(MPEG4_LIB)
if(g_vector_enable_flag&VECTOR_ENABLE_DCT)
{
	int i;
	int iblk;
	int twoCoeff;
	int32 * pBlk;
	int32 * pNZFlag;
	int32 blk_num = 6;
	int32 vid_standard	= (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0xf;

	if(VSP_JPEG == vid_standard)
	{
		blk_num = g_block_num_in_one_mcu;
	}	

	for (iblk = 0; iblk < blk_num; iblk++)
	{
		pBlk = pDCTIOBuf + 32*iblk;

		for (i = 0; i < 32; i++)
		{
			twoCoeff = pBlk[i];

			fprintf_oneWord_hex (g_fp_vld_tv, twoCoeff);
		}
	}

	/*printf four word none-zero flag*/
	pNZFlag = pDCTIOBuf + 192;
	for (i = 0; i < (4*blk_num); i++)
	{
		fprintf_oneWord_hex (g_fp_vld_tv, pNZFlag[i]);
	}
}
#endif
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 




















// End 