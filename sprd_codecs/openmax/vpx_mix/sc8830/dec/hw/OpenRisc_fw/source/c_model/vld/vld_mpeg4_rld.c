/******************************************************************************
 ** File Name:    vld_mpeg4_rld.c   										  *
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

const uint8 s_standard_zigzag[BLOCK_SQUARE_SIZE] = 
{
     0,  1,  8, 16,  9,  2,  3, 10, 
	17, 24, 32, 25, 18, 11,  4,  5, 
	12, 19, 26, 33, 40, 48, 41, 34, 
	27, 20, 13,  6,  7, 14, 21, 28, 
	35, 42, 49, 56, 57, 50, 43, 36, 
	29, 22, 15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61, 54, 47, 55, 62, 63
};

const uint8 s_dec_horizontal_scan[BLOCK_SQUARE_SIZE] = 
{
	 0,  1,  2,  3,  8,  9, 16, 17, 
	10, 11,  4,  5,  6,  7, 15, 14, 
	13, 12, 19, 18, 24, 25, 32, 33, 
	26, 27, 20, 21, 22, 23, 28, 29, 
	30, 31, 34, 35, 40, 41, 48, 49, 
	42, 43, 36, 37, 38, 39, 44, 45, 
	46, 47, 50, 51, 56, 57, 58, 59, 
	52, 53, 54, 55, 60, 61, 62, 63
};

const uint8 s_dec_vertical_scan[BLOCK_SQUARE_SIZE] = 
{
    0 ,  8, 16, 24,  1,  9,  2, 10, 
	17, 25, 32, 40, 48, 56, 57, 49, 
	41, 33, 26, 18,  3, 11,  4, 12, 
	19, 27, 34, 42, 50, 58, 35, 43, 
	51, 59, 20, 28,  5, 13,  6, 14, 
	21, 29, 36, 44, 52, 60, 37, 45, 
	53, 61, 22, 30,  7, 15, 23, 31, 
	38, 46, 54, 62, 39, 47, 55, 63
};


int g_postrans [8] = 
{
	0, 4, 2, 6, 1, 5, 3, 7
};

const char intra_max_level[40] =
{
	27, 10,  5,  4,  3,  3,  3,  3,    //last = 0
		2,  2,  1,  1,  1,  1,  1,  0,
		
		8,  3,  2,  2,  2,  2,  2,  1,    //last = 1
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  0,  0,  0,
};


const char intra_max_run [44] = 
{
	127, 14,  9,  7,  3,  2,  1,		//intra_max_run0 [32]
		1,  1,  1,  1,  0,  0,  0, 
		0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,
		0, 0, 0, 0,   //stuffing four byte zero for fast addressing
		
		127, 20,  6,					//intra_max_run1 [12]
		1,  0,  0,  
		0,  0,  0, 
		0, 0, 0,    //stuffing 3 byte zero
};


const char inter_max_level[128] = 
{
	12,  6,  4,  3,  3,  3,  3,  2, 
		2,  2,  2,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		
		3,  2,  1,  1,  1,  1,  1,  1, 
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  1,  1,  1,  1,  1,  1,  1,
		1,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0,
		0,  0,  0,  0,  0,  0,  0,  0
};


char inter_max_run [20] =
{
	127, 26, 10,  6,  2, 1, 1, 0,	//inter_max_run0
		0,  0,  0,  0,  0, 0, 0, 0, 
		
		127, 40,  1,  0     //inter_max_run1
};

#define OFFSET_PRX0	0x0         //offset of prefix of 0 consecutive 0
#define OFFSET_PRX1	0x8			//offset of prefix of 1 consecutive 0
#define OFFSET_PRX2	0x18		//offset of prefix of 2 consecutive 0
#define OFFSET_PRX3	0x28		//offset of prefix of 3 consecutive 0
#define OFFSET_PRX4	0x48		//offset of prefix of 4 consecutive 0
#define OFFSET_PRX5	0x68		//offset of prefix of 5 consecutive 0
#define OFFSET_PRX6	0x88		//offset of prefix of 6 consecutive 0
#define OFFSET_PRX7	0x90		//offset of prefix of 7 consecutive 0
#define OFFSET_PRX8	0x94		//offset of prefix of 8 consecutive 0

void blk_to_asicOrder(int16 * pblock, int16 * pblk_asic, uint32 * pNZFlag)
{
	int i;
	int indexAsic;
	int16 val;
	uint32 nzflag0 = 0;
	uint32 nzflag1 = 0;

	for (i = 0; i < 64; i++)
	{
		indexAsic = g_postrans [i&7] * 8 + g_postrans [(i>>3)];

		val = pblock[i];
		pblk_asic [indexAsic] = val;

		if (val != 0)
		{
			if (indexAsic < 32)
			{
				nzflag0 = nzflag0 | (1 << indexAsic);
			}
			else
			{
				nzflag1 = nzflag1 | (1 << (indexAsic-32));

			}
		}
	}

	pNZFlag[0] = nzflag0;
	pNZFlag[1] = nzflag1;
}


int NZFlagAcdcGen (int nz_flag_acdc, int indexAsic)
{
	switch (indexAsic)
	{
	case 0:
		nz_flag_acdc |= 1<<0;
		break;
	case 1:
		nz_flag_acdc |= 1<<1;
		break;
	case 2:
		nz_flag_acdc |= 1<<2;
		break;
	case 3:
		nz_flag_acdc |= 1<<3;
		break;
	case 4:
		nz_flag_acdc |= 1<<4;
		break;
	case 5:
		nz_flag_acdc |= 1<<5;
		break;
	case 6:
		nz_flag_acdc |= 1<<6;
		break;
	case 7:
		nz_flag_acdc |= 1<<7;
		break;
	case 8:
		nz_flag_acdc |= 1<<8;
		break;
	case 16:
		nz_flag_acdc |= 1<<9;
		break;
	case 24:
		nz_flag_acdc |= 1<<10;
		break;
	case 32:
		nz_flag_acdc |= 1<<11;
		break;
	case 40:
		nz_flag_acdc |= 1<<12;
		break;
	case 48:
		nz_flag_acdc |= 1<<13;
		break;
	case 56:
		nz_flag_acdc |= 1<<14;
		break;
	default:
		break;
	}

	return nz_flag_acdc;
}

void configure_noneZeroFlag_blk (uint32 * pNZFlag, uint32 * pNoneZeroFlag)
{
// 	uint32 noneZeroWritten;
	uint32 noneZeroFlag16;

	noneZeroFlag16 = (uint32)((pNZFlag[0]>>0) & 0xffff);
	//noneZeroWritten = ((noneZeroFlag16 >> 12) << 16) | (noneZeroFlag16 & 0xfff);
	pNoneZeroFlag [0] = noneZeroFlag16;
	
	noneZeroFlag16 = (uint32)((pNZFlag[0]>>16) & 0xffff);
	//noneZeroWritten = ((noneZeroFlag16 >> 12) << 16) | (noneZeroFlag16 & 0xfff);
	pNoneZeroFlag [1] = noneZeroFlag16;
	
	noneZeroFlag16 = (uint32)((pNZFlag[1]>>0) & 0xffff);
	//noneZeroWritten = ((noneZeroFlag16 >> 12) << 16) | (noneZeroFlag16 & 0xfff);
	pNoneZeroFlag [2] = noneZeroFlag16;
	
	noneZeroFlag16 = (uint32)((pNZFlag[1]>>16) & 0xffff);
	//noneZeroWritten = ((noneZeroFlag16 >> 12) << 16) | (noneZeroFlag16 & 0xfff);
	pNoneZeroFlag [3] = noneZeroFlag16;
}


int decodeOneBlockTCOEF_mpeg4 (
								int mbtype,
								int iBlk, 
								int iCoefStart,
								int zigzag_mode,
								int rotation_ena
								)
{
	int index;
	int indexAsic;
	int escapeMode;
	uint32 tmp;
	uint32 code;
	uint32 val;
	const char * pScan;
	int lev_adj, run_adj;
	int run, level, len, sign;
	int last = 0;
	int CoeffNum = 0;
	int flushLen = 0;
	int i = iCoefStart;
	int16 pBlk [64];
	uint32 codeword;
	uint32 noneZeroFlag[2];
	int nz_flag_acdc = 0;
	const uint8 * pMax_level, * pMax_run;
	const uint32 * pHuffTab = vsp_huff_dcac_tab;
	int16 * pBlk_asic;

	if (rotation_ena)
	{
		if (iBlk == 1)
			iBlk = 2;
		else if (iBlk == 2)
			iBlk = 1;
	}

	pBlk_asic =  (int16*)(vsp_dct_io_1 + 32*iBlk);
	
	memset (pBlk, 0, 64 * sizeof (short));

	if (zigzag_mode == VSP_STD_ZIGZAG)
		pScan = s_standard_zigzag;
	else if (zigzag_mode == VSP_HOR_ZIGZAG)
		pScan = s_dec_horizontal_scan;
	else 
		pScan = s_dec_vertical_scan;

	if (mbtype == VSP_INTRA)
	{
		pMax_level = intra_max_level;
		pMax_run   = intra_max_run;
	}
	else
	{
		pMax_level = inter_max_level;
		pMax_run   = inter_max_run;
	}

	while (!last)
	{
		if ((CoeffNum == 0) && (iBlk == 3))
			printf ("");

		code = show_nbits (32);

		VLD_FPRINTF(g_pfRunLevel_mpeg4dec, "bsm_out: 0x%08x, ", code);

		/*judge whether escape and escape mode*/
		escapeMode = 0;
		flushLen = 0;
		codeword = code;

		if ((code >> 24) == 6) //first escape mode
		{
			escapeMode = 1;
			flushLen += 8;
		}
		else if ((code >> 23) == 0xe)	//second escape mode
		{
			escapeMode = 2;
			flushLen += 9;
		}
		else if ((code >> 23) == 0xf)	//third escape mode
		{
			escapeMode = 3;
			flushLen += 9;
		}	

		code = code << flushLen;

		VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "escapemode: %d, ", escapeMode);
		
		if (escapeMode == 3)
		{			
			/*fix length decoding*/
			val = code >> 11;
			flushLen += 21;

			last = val >> 20;
			run = (val >> 14) & 0x3f;
			level = (val >> 1) & 0xfff;

			if (level > 2048)
			{
				sign = 1;
				level = 4096 - level;
			}
			else if ((level == 2048) || (level == 0))
			{
				printf ("wrong bit stream!\n");
				g_vld_reg_ptr->VLD_CTL = V_BIT_30;
				return 0;
			}
			else
			{
				sign = 0;
			}			
		}
		else
		{
			int maxLen;
			int suffix;
			int offset;

			/*compare the prefix*/
			if ((code >> 31) == 1)			//1
			{
				suffix = 3;
				maxLen = 4;
				offset = OFFSET_PRX0;
			}
			else if ((code >> 30) == 1)		//01
			{
				suffix = 4;
				maxLen = 6;
				offset = OFFSET_PRX1;
			}
			else if ((code >> 29) == 1)		//001
			{		
				suffix = 4;
				maxLen = 7;
				offset = OFFSET_PRX2;
			}
			else if ((code >> 28) == 1)		//0001
			{
				suffix = 5;
				maxLen = 9;
				offset = OFFSET_PRX3;
			}
			else if ((code >> 27) == 1)		//0000_1
			{
				suffix = 5;
				maxLen = 10;
				offset = OFFSET_PRX4;
			}
			else if ((code >> 26) == 1)		//0000_01
			{
				suffix = 5;
				maxLen = 12;
				offset = OFFSET_PRX5;
			}
			else if ((code >> 25) == 1)		//0000_001
			{
				suffix = 3;
				maxLen = 10;
				offset = OFFSET_PRX6;
			}
			else if ((code >> 24) == 1)		//0000_0001
			{
				suffix = 2;
				maxLen = 10;
				offset = OFFSET_PRX7;
			}
			else if ((code >> 23) == 1)		//0000_0000_1
			{
				suffix = 2;
				maxLen = 11;
				offset = OFFSET_PRX8;
			}
			else
			{
				printf ("wrong bit stream!\n");

				g_vld_reg_ptr->VLD_CTL = V_BIT_30;
				return 0;
			//	exit (-1);
			}

			tmp = (code >> (32 - maxLen)) & ((1 << suffix) - 1);
			val = pHuffTab [offset + tmp];

			if (mbtype == VSP_INTRA)
				val = val >> 16;
			else
				val = val & 0xffff;

			last = val >> 15;
			run = (val >> 9) & 0x3f;
			level = (val >> 4) & 0x1f;
			len = val & 0xf;

			lev_adj = 0; 
			run_adj = 0;

			if (escapeMode == 1)
			{
				int offset = (mbtype == VSP_INTRA) ? 16 : 64;
				lev_adj = pMax_level [last*offset  + run];
			}
			else if (escapeMode == 2)
			{
				int offset = (mbtype == VSP_INTRA) ? 32 : 16;
				run_adj = pMax_run [last*offset + level] + 1;
			}

			level += lev_adj;
			run   += run_adj;
			sign = (code << (len-1)) >> 31;

			flushLen += len;			
		}

		codeword = codeword >> (32-flushLen);

		printf_codeWordInfo (codeword, flushLen, g_pfRunLevel_mpeg4dec);
		i += run;

		if (i > 63)
		{
			g_vld_reg_ptr->VLD_CTL = V_BIT_30;
			printf ("vld error\n");
			return 0;
		}
		
		index = pScan [i];		
		if (rotation_ena)
		{
			index = (index >> 3) + (index&7) * 8;
		}
		
		indexAsic = g_postrans [index&7] * 8 + g_postrans [(index>>3)];
		nz_flag_acdc = NZFlagAcdcGen (nz_flag_acdc, indexAsic);

		VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "last: %d, run: %d, level: %d, sign: %d, asic_index: %d\n", 
			last, run, level, sign, indexAsic);

		if (sign) 
			level = -level;

	//	printf_halfWord_hex (g_pfDCTCoeff_mpeg4dec, (short)level);
	
		pBlk [index] = level;

		i += 1;
		CoeffNum++;
		flush_nbits (flushLen);
	}

	/*reorder coefficient to ASIC order and compute none-zero flag*/
	FprintfOneBlock (pBlk, g_mpegdec_vld_no_acdc_fp);

	blk_to_asicOrder(pBlk, pBlk_asic, noneZeroFlag);

	configure_noneZeroFlag_blk (noneZeroFlag, (uint32 *)(vsp_dct_io_1 + 192 + iBlk*4));	

	return nz_flag_acdc;
}

void decodeOneBlockTCOEF_h263 (
							   int iBlk, 
							   int iCoefStart, 
							   int zigzag_mode, 
							   int rotation_ena,
							   int is_flv_v1
							   )
{
	int index;
	int indexAsic;
	uint32 tmp;
	uint32 code;
	uint32 val;
	const char * pScan;
	int bEscape;
	int run, level, len, sign;
	int last = 0;
	int flushLen = 0;
	int i = iCoefStart;
	int16 pBlk [64];
	uint32 noneZeroFlag[2];
	const uint32 * pHuffTab = vsp_huff_dcac_tab;
	int16 * pBlk_asic;

	if (rotation_ena)
	{
		if (iBlk == 1)
			iBlk = 2;
		else if (iBlk == 2)
			iBlk = 1;
	}

	pBlk_asic =  (int16*)(vsp_dct_io_1 + 32*iBlk);

	memset (pBlk, 0, 64 * sizeof (short));

	if (zigzag_mode == VSP_STD_ZIGZAG)
		pScan = s_standard_zigzag;
	else if (zigzag_mode == VSP_HOR_ZIGZAG)
		pScan = s_dec_horizontal_scan;
	else 
		pScan = s_dec_vertical_scan;

	while (!last)
	{
		code = show_nbits (32);

		VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "bsm_out: 0x%08x, ", code);

		/*judge whether escape and escape mode*/
		bEscape = 0;
		flushLen = 0;

		if ((code >> 25) == 3)	//be escape mode
		{
			bEscape = 1;
			flushLen += 7;
		}

		code = code << flushLen;

		if (bEscape)
		{
			int32 run_level_last;
		
			if (!is_flv_v1)
			{
				run_level_last = code >> 17;	
				flushLen += 15;			
				
				last = run_level_last >> 14;
				run = (run_level_last >> 8) & 0x3f;
				level = run_level_last & 0xff;

				VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "escape mode, ");
				
				if (level > 128)
				{ 
					sign = 1; 
					level = 256 - level; 
				} 
				else if ((level == 128) || (level == 0))
				{ 
					g_vld_reg_ptr->VLD_CTL = V_BIT_30;
					PRINTF ("Illegal LEVEL for ESCAPE mode 4: 128\n");	
					return;
				}
				else
				{
					sign = 0;
				}
			}
			else
			{
				int is_long_lev;

				is_long_lev = (code >> 31) & 1;
				last = (code >> 30) & 1;
				run = (code >> 24) & 0x3f;

				if (is_long_lev)
				{
					VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "long escape,  ");

					level = (code >> 13) & 0x7ff;
					if (level > 1024)
					{
						sign = 1;
						level = 2048-level;
					}
					else if ((level == 1024) || (level == 0))
					{
						g_vld_reg_ptr->VLD_CTL = V_BIT_30;
						PRINTF ("Illegal LEVEL for ESCAPE mode 4: 0\n");
						return;
					}
					else
					{
						sign = 0;
					}

					flushLen += 19;	
				}
				else
				{
					VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "not long escape,  ");

					level = (code >> 17) & 0x7f;
					if (level > 64)
					{
						sign = 1;
						level = 128 - level;
					}
					else if ((level == 64) || (level == 0))
					{
						g_vld_reg_ptr->VLD_CTL = V_BIT_30;
						PRINTF ("Illegal LEVEL for ESCAPE mode 4: 0\n");
						return;
					}
					else
					{
						sign = 0;
					}

					flushLen += 15;	
				}
			}
		}
		else
		{
			int maxLen;
			int suffix;
			int offset;
			
			/*compare the prefix*/
			if ((code >> 31) == 1)			///1
			{
				suffix = 3;
				maxLen = 4;
				offset = OFFSET_PRX0;
			}
			else if ((code >> 30) == 1)		//01
			{
				suffix = 4;
				maxLen = 6;
				offset = OFFSET_PRX1;
			}
			else if ((code >> 29) == 1)		//001
			{		
				suffix = 4;
				maxLen = 7;
				offset = OFFSET_PRX2;
			}
			else if ((code >> 28) == 1)		//0001
			{
				suffix = 5;
				maxLen = 9;
				offset = OFFSET_PRX3;
			}
			else if ((code >> 27) == 1)		//0000_1
			{
				suffix = 5;
				maxLen = 10;
				offset = OFFSET_PRX4;
			}
			else if ((code >> 26) == 1)		//0000_01
			{
				suffix = 5;
				maxLen = 12;
				offset = OFFSET_PRX5;
			}
			else if ((code >> 25) == 1)		//0000_001
			{
				suffix = 3;
				maxLen = 10;
				offset = OFFSET_PRX6;
			}
			else if ((code >> 24) == 1)		//0000_0001
			{
				suffix = 2;
				maxLen = 10;
				offset = OFFSET_PRX7;
			}
			else if ((code >> 23) == 1)		//0000_0000_1
			{
				suffix = 2;
				maxLen = 11;
				offset = OFFSET_PRX8;
			}
			else
			{
				printf ("wrong bit stream!\n");
				g_vld_reg_ptr->VLD_CTL = V_BIT_30;
				return;
			}

			
			tmp = (code >> (32 - maxLen)) & ((1 << suffix) - 1);
			val = pHuffTab [offset + tmp];

			val = val & 0xffff;
			
			last = val >> 15;
			run = (val >> 9) & 0x3f;
			level = (val >> 4) & 0x1f;
			len = val & 0xf;
			
			sign = (code << (len-1)) >> 31;
			
			flushLen += len;			
		}
		
		i += run;
		if (i > 63)
		{
			g_vld_reg_ptr->VLD_CTL = V_BIT_30;
			return;
		}

		index = pScan [i];	
		if (rotation_ena)
		{
			index = (index >> 3) + (index&7) * 8;
		}

		indexAsic = g_postrans [index&7] * 8 + g_postrans [(index>>3)];

		VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "last: %d, run: %d, level: %d, sign: %d, asic_index: %d\n", 
			last, run, level, sign, indexAsic);
		
		if (sign) 
			level = -level;

//		printf_halfWord_hex (g_pfDCTCoeff_mpeg4dec, (short)level);
	
		pBlk [index] = level;

		i += 1;
		flush_nbits (flushLen);
	}

	FprintfOneBlock (pBlk, g_mpegdec_vld_no_acdc_fp);

	blk_to_asicOrder(pBlk, pBlk_asic, noneZeroFlag);
	
	configure_noneZeroFlag_blk (noneZeroFlag, (uint32 *)(vsp_dct_io_1 + 192 + iBlk*4));	
}

/*********************************************************************************
interface with vld_top, register file

input:
interface for both intra and inter 
standard
mb_type
rotation

only for inter
cbp

only for intra
start_position
zigzag_mode
block_id
*********************************************************************************/

int mpeg4dec_vld (
				   int		standard,				// mpeg4 or h.263
   				   int		is_rvlc,				//is rvlc or not
				   int		mb_type,				//intra or inter
				   int		cbp,					//only for inter mb
				   int		start_position,			//0 or 1
				   int		zigzag_mode,			//standard, horizontal, vertical
				   int		blk_id,					//for storing address in dct/io buffer, only for intra mb
				   int		rotation_ena
				   )
{
	int blk_cnt;
	int nz_flag_acdc = 0;

	if (mb_type == VSP_INTRA)
	{
		//decode only one block
		if ((standard == ITU_H263) || (standard == FLV_H263))
		{
			int is_flv_v1 = (standard == FLV_H263) ? 1 :0;
			decodeOneBlockTCOEF_h263 (
										blk_id, 
										start_position, 
										zigzag_mode, 
										rotation_ena,
										is_flv_v1
										);
		}
		else
		{
			if (!is_rvlc)
			{
				nz_flag_acdc = decodeOneBlockTCOEF_mpeg4 (
													mb_type, 
													blk_id, 
													start_position, 
													zigzag_mode, 
													rotation_ena
													);
			}
			else
			{
				nz_flag_acdc = RvlcOneBlock (
											(mb_type == VSP_INTRA),
											zigzag_mode,
											start_position,
											blk_id
										);
				
			}
		}
	}
	else
	{
		int transpose_blkId;

		if (cbp == 0)
			return 0;

		for (blk_cnt = 0; blk_cnt < 6; blk_cnt++)
		{
			VLD_FPRINTF (g_mpeg4dec_vld_trace_fp, "blk_cnt: %d\n", blk_cnt);
			VLD_FPRINTF (g_pfRunLevel_mpeg4dec, "block: %d\n", blk_cnt);

			transpose_blkId = blk_cnt;
			if (rotation_ena)
			{
				if (blk_cnt == 1)
					transpose_blkId = 2;
				else if (blk_cnt == 2)
					transpose_blkId = 1;		
			}

			
			if ((1<<(5-blk_cnt)) & cbp)
			{
				if ((standard == ITU_H263) || (standard == FLV_H263))
				{
					int is_flv_v1 = (standard == FLV_H263) ? 1 : 0;
					decodeOneBlockTCOEF_h263 (
						blk_cnt, 
						0, 
						zigzag_mode, 
						rotation_ena,
						is_flv_v1
						);
				}
				else
				{
					if (!is_rvlc)
					{
						decodeOneBlockTCOEF_mpeg4 (
							mb_type, 
							blk_cnt, 
							0, 
							zigzag_mode, 
							rotation_ena
							);
					}
					else
					{
						RvlcOneBlock (
										(mb_type == VSP_INTRA),
										zigzag_mode,
										start_position,
										blk_cnt
								);
					}
				}
			}	
			
			FprintfOneBlock ((int16 *)(vsp_dct_io_1 + transpose_blkId*32), g_mpeg4dec_vld_trace_fp);
		}		
	}

	return nz_flag_acdc;
}
			  
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 