#include "sc6800x_video_header.h"

#define OFFSET_TYPE1_LEN5		0
#define OFFSET_TYPE1_LEN6		2
#define OFFSET_TYPE1_LEN7		6
#define OFFSET_TYPE1_LEN8		12	
#define OFFSET_TYPE1_LEN9		20	
#define OFFSET_TYPE1_LEN10		30	
#define OFFSET_TYPE1_LEN11		42
#define OFFSET_TYPE1_LEN12		56
#define OFFSET_TYPE1_LEN13		72
#define OFFSET_TYPE1_LEN14		90
#define OFFSET_TYPE1_LEN15		110
#define OFFSET_TYPE1_LEN16		132

#define TABLE_SIZE_TYPE1		146	

const uint8 g_dec_standard_zigzag[BLOCK_SQUARE_SIZE] = 
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

const uint8 g_dec_horizontal_scan[BLOCK_SQUARE_SIZE] = 
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

const uint8 g_dec_vertical_scan[BLOCK_SQUARE_SIZE] = 
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

/*rvlc huffman table for first bit is '0'*/


/*huffman decoding for first bit '1', use combinational logic,
24 words will be used if use look-up table*/

/*run level decoding of type0 is derived by combinational logic*/

extern  uint8 g_standard_zigzag[BLOCK_SQUARE_SIZE];
int		NZFlagAcdcGen (int nz_flag_acdc, int indexAsic);
void	blk_to_asicOrder_mpeg4 (int16 * pblock, int16 * pblk_asic, uint32 * pNZFlag);
void	configure_noneZeroFlag_blk (uint32 * pNZFlag, uint32 * pNoneZeroFlag);

int RvlcOneBlock (
				   int		is_intra,
				   int		zigzag_mode,
				   int		start_pos,
				   int		blk_id
				   )
{
	int			i = start_pos;
	int			index;
	int			index_asic;

	int			last = 0;
	int			run;
	int			level;
	int			sign;

	uint32		code;  

	int			code_type;
	int			flushLen;

	int			code_len_type0;
	int			error_len_type0 = 0;
//	int			lut_addr_type0;
	int			offset_type0;
	int			sign_type0;

	int			code_len_type1;
	uint16		code_snd0;
	uint16		code_thd0;

	int			snd0_pos;
	int			thd0_pos;
	int			sign_type1;
	int			offset_thd0;
	int			offset_len_type1;
//	int			offset_snd0;

	int			error_snd0;
	int			error_thd0;
	int			error_codelen_type1;

	int			offset_type1;

	uint32		last_run_lev_0;
	uint32		last_run_lev_1;
	uint32		last_run_lev;

	uint32		tbuf_addr;
	uint32		tbuf_rdata;

	int			nz_flag_acdc = 0;

	const uint8	*	pScan;
	int			coeff;
	int16	*	pBlk_asic;
	int16		pBlk [64];
	uint32		noneZeroFlag[2];
	int			postrans [8] = {0, 4, 2, 6, 1, 5, 3, 7};

	int			coeff_num = 0;

	pBlk_asic = (int16 *)vsp_dct_io_1 + blk_id * 64;

	if (zigzag_mode == VSP_STD_ZIGZAG)
	{
		pScan = g_dec_standard_zigzag;
	}
	else if (zigzag_mode == VSP_HOR_ZIGZAG)
	{
		pScan = g_dec_horizontal_scan;
	}
	else
	{
		pScan = g_dec_vertical_scan;
	}

	memset (pBlk, 0, 64 * sizeof (short));
 
	while (!last)
	{
		if (coeff_num == 5)
			printf ("");

		//show next 32 bits from bit stream
		code = show_nbits (32);

#if _TRACE_
		FPRINTF (g_pfRunLevel_mpeg4dec, "bsm_out: 0x%08x, ", code);
		if (code == 0x0ed80182)
			printf ("");
#endif

		/*********************************************************************
		determine the code type of the code, escape type, 
		or first bit is '1' or '0', otherwise
		*********************************************************************/
		if ((code >> 28) == 0)
		{
			code_type = 2;
		}
		else if ((code >> 31) == 1)
		{
			code_type = 0;
		}
		else
		{
			code_type = 1;
		}


		/*********************************************************************
			determine the code length of first type, first bit is '1'
		*********************************************************************/
		//get second '1' position		
		if (code_type == 0)
		{
			if ((code >> 30) & 1)
			{
				code_len_type0	= 4;
				offset_type0	= (code >> 29) & 1;
				sign_type0		= (code >> 28) & 1;
			}
			else if ((code >> 29) & 1)
			{
				code_len_type0	= 5;
				offset_type0	= (code >> 28) & 1;
				sign_type0		= (code >> 27) & 1;
			}
			else if ((code >> 28) & 1)
			{
				code_len_type0	= 6;
				offset_type0	= (code >> 27) & 1;
				sign_type0		= (code >> 26) & 1;
			}
			else if ((code >> 27) & 1)
			{
				code_len_type0	= 7;
				offset_type0	= (code >> 26) & 1;
				sign_type0		= (code >> 25) & 1;
			}
			else if ((code >> 26) & 1)
			{
				code_len_type0	= 8;
				offset_type0	= (code >> 25) & 1;
				sign_type0		= (code >> 24) & 1;
			}
			else if ((code >> 25) & 1)
			{
				code_len_type0	= 9;
				offset_type0	= (code >> 24) & 1;
				sign_type0		= (code >> 23) & 1;
			}
			else if ((code >> 24) & 1)
			{
				code_len_type0	= 10;
				offset_type0	= (code >> 23) & 1;
				sign_type0		= (code >> 22) & 1;
			}
			else if ((code >> 23) & 1)
			{
				code_len_type0	= 11;
				offset_type0	= (code >> 22) & 1;
				sign_type0		= (code >> 21) & 1;
			}
			else if ((code >> 22) & 1)
			{
				code_len_type0	= 12;
				offset_type0	= (code >> 21) & 1;
				sign_type0		= (code >> 20) & 1;
			}
			else if ((code >> 21) & 1)
			{
				code_len_type0	= 13;
				offset_type0	= (code >> 20) & 1;
				sign_type0		= (code >> 19) & 1;
			}
			else if ((code >> 20) & 1)
			{
				code_len_type0	= 14;
				offset_type0	= (code >> 19) & 1;
				sign_type0		= (code >> 18) & 1;
			}
			else if ((code >> 19) & 1)
			{
				code_len_type0	= 15;
				offset_type0	= (code >> 18) & 1;
				sign_type0		= (code >> 17) & 1;
			}
			else
			{
				code_len_type0  = 16;
				offset_type0	= 0;
				sign_type0		= 0;
			}
		}

		error_len_type0 = (code_len_type0 == 16) ? 1 : 0;

		/*********************************************************************
			determine the code length of second type, first bit is '0'
		*********************************************************************/
		if (code_type == 1)
		{
			code_snd0 = (uint16)((code << 1) >> 16);
			
			//to find second '0' position
			if ((code_snd0 & 0xff00) != 0xff00)
			{
				if ((code_snd0 & 0xf000) != 0xf000)
				{
					if ((code_snd0 & 0xc000) != 0xc000)
					{
						if ((code_snd0 & 0x8000) != 0x8000)
						{
							snd0_pos	= 0;
							code_thd0	= code_snd0 << 1;
						}
						else
						{
							snd0_pos	= 1;
							code_thd0	= code_snd0 << 2;	
						}
					}
					else
					{
						if ((code_snd0 & 0x2000) != 0x2000)
						{
							snd0_pos	= 2;
							code_thd0	= code_snd0 << 3;
						}
						else
						{
							snd0_pos	= 3;
							code_thd0	= code_snd0 << 4;
						}
					}
				}
				else
				{				
					if ((code_snd0 & 0xc00) != 0xc00)
					{
						if ((code_snd0 & 0x800) != 0x800)
						{
							snd0_pos	= 4;
							code_thd0	= code_snd0 << 5;
						}
						else
						{
							snd0_pos	= 5;
							code_thd0	= code_snd0 << 6;
						}
					}
					else
					{
						if ((code_snd0 & 0x200) != 0x200)
						{
							snd0_pos	= 6;
							code_thd0	= code_snd0 << 7;
						}
						else
						{
							snd0_pos	= 7;
							code_thd0	= code_snd0 << 8;
						}
					}
				}
			}
			else
			{
				if ((code_snd0 & 0xf0) != 0xf0)
				{
					if ((code_snd0 & 0xc0) != 0xc0)
					{
						if ((code_snd0 & 0x80) != 0x80)
						{
							snd0_pos	= 8;
							code_thd0	= code_snd0 << 9;
						}
						else
						{
							snd0_pos	= 9;
							code_thd0	= code_snd0 << 10;
						}
					}
					else
					{
						if ((code_snd0 & 0x20) != 0x20)
						{
							snd0_pos	= 10;
							code_thd0	= code_snd0 << 11;
						}
						else
						{
							snd0_pos	= 15;
							code_thd0	= 0;
						}
					}
				}
				else
				{
					snd0_pos	= 15;
					code_thd0	= 0;
				}
			}
			
			error_snd0 = (snd0_pos == 15) ? 1 : 0;
			
			//third '0' position, offset_thd0, and sign 
			if ((code_thd0 & 0xff00) != 0xff00)
			{
				if ((code_thd0 & 0xf000) != 0xf000)
				{
					if ((code_thd0 & 0xc000) != 0xc000)
					{
						if ((code_thd0 & 0x8000) != 0x8000)
						{
							thd0_pos	= 0;
							offset_thd0 = (code_thd0 >> 14) & 1;
							sign_type1	= (code_thd0 >> 13) & 1;
						}
						else
						{
							thd0_pos	= 1;
							offset_thd0 = (code_thd0 >> 13) & 1;
							sign_type1	= (code_thd0 >> 12) & 1;
						}
					}
					else
					{
						if ((code_thd0 & 0x2000) != 0x2000)
						{
							thd0_pos	= 2;
							offset_thd0 = (code_thd0 >> 12) & 1;
							sign_type1	= (code_thd0 >> 11) & 1;
						}
						else
						{
							thd0_pos	= 3;
							offset_thd0 = (code_thd0 >> 11) & 1;
							sign_type1	= (code_thd0 >> 10) & 1;
						}
					}
				}
				else
				{
					
					if ((code_thd0 & 0xc00) != 0xc00)
					{
						if ((code_thd0 & 0x800) != 0x800)
						{
							thd0_pos	= 4;
							offset_thd0 = (code_thd0 >> 10) & 1;
							sign_type1	= (code_thd0 >> 9) & 1;
						}
						else
						{
							thd0_pos	= 5;
							offset_thd0 = (code_thd0 >> 9) & 1;
							sign_type1	= (code_thd0 >> 8) & 1;
						}
					}
					else
					{
						if ((code_thd0 & 0x200) != 0x200)
						{
							thd0_pos	= 6;
							offset_thd0 = (code_thd0 >> 8) & 1;
							sign_type1	= (code_thd0 >> 7) & 1;
						}
						else
						{
							thd0_pos	= 7;
							offset_thd0 = (code_thd0 >> 7) & 1;
							sign_type1	= (code_thd0 >> 6) & 1;
						}
					}
				}
			}
			else
			{
				if ((code_thd0 & 0xf0) != 0xf0)
				{
					if ((code_thd0 & 0xc0) != 0xc0)
					{
						if ((code_thd0 & 0x80) != 0x80)
						{
							thd0_pos	= 8;
							offset_thd0 = (code_thd0 >> 6) & 1;
							sign_type1	= (code_thd0 >> 5) & 1;					
						}
						else
						{
							thd0_pos	= 9;
							offset_thd0 = (code_thd0 >> 5) & 1;
							sign_type1	= (code_thd0 >> 4) & 1;	
						}
					}
					else
					{
						if ((code_thd0 & 0x20) != 0x20)
						{
							thd0_pos	= 10;
							offset_thd0 = (code_thd0 >> 4) & 1;
							sign_type1	= (code_thd0 >> 3) & 1;
						}
						else
						{
							thd0_pos	= 11;
							offset_thd0 = (code_thd0 >> 3) & 1;
							sign_type1	= (code_thd0 >> 2) & 1;
						}
					}
				}
				else
				{				
					thd0_pos	= 15;
					offset_thd0 = 0;
					sign_type1	= 0;
				}
			}
			
			error_thd0			= (thd0_pos == 15) ? 1 : 0;
			
			code_len_type1		= 1 + (snd0_pos + 1) + (thd0_pos + 1) + 2;
			
			error_codelen_type1 = (code_len_type1 > 16) ? 1 : 0;
			
			//get the look-up table address of second type
			switch(code_len_type1)
			{
			case 5:
				offset_len_type1 = OFFSET_TYPE1_LEN5;
				break;
			case 6:
				offset_len_type1 = OFFSET_TYPE1_LEN6;
				break;
			case 7:
				offset_len_type1 = OFFSET_TYPE1_LEN7;
				break;
			case 8:
				offset_len_type1 = OFFSET_TYPE1_LEN8;
				break;
			case 9:
				offset_len_type1 = OFFSET_TYPE1_LEN9;
				break;
			case 10:
				offset_len_type1 = OFFSET_TYPE1_LEN10;
				break;
			case 11:
				offset_len_type1 = OFFSET_TYPE1_LEN11;
				break;
			case 12:
				offset_len_type1 = OFFSET_TYPE1_LEN12;
				break;
			case 13:
				offset_len_type1 = OFFSET_TYPE1_LEN13;
				break;
			case 14:
				offset_len_type1 = OFFSET_TYPE1_LEN14;
				break;
			case 15:
				offset_len_type1 = OFFSET_TYPE1_LEN15;
				break;
			case 16:
				offset_len_type1 = OFFSET_TYPE1_LEN16;
				break;
			default:
				offset_len_type1 = 0;
			}
			
			offset_type1 = offset_len_type1 + snd0_pos*2 + offset_thd0;
		}


		/*********************************************************************
			1. fix length decoding if escape type, 
			2. combinational logic to get run level for type0
			3. look up huffman table to get run level for type1
		*********************************************************************/
		if (code_type == 2)
		{
			last	= (code >> 26) & 1;
			run		= (code >> 20) & 0x3f;
			level	= (code >> 8) & 0x7ff;
			sign	= (code >> 2) & 1;
		}
		else
		{
			/*for type0, use combinational logic to get run level and last*/ 
			if (code_type == 0)
			{
				switch(code_len_type0)
				{
				case 4:
					last_run_lev_0 = ((0<<31) | ( 0<<24) | (1<<16)) | ((0<<15) | ( 0<<8) | (1<<0));
					last_run_lev_1 = ((0<<31) | ( 0<<24) | (2<<16)) | ((0<<15) | ( 1<<8) | (1<<0));
					break;
				case 5:				
					last_run_lev_0 = ((0<<31) | ( 0<<24) | (3<<16)) | ((0<<15) | ( 2<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | ( 0<<24) | (1<<16)) | ((1<<15) | ( 0<<8) | (1<<0));
					break;
				case 6:
					last_run_lev_0 = ((1<<31) | ( 1<<24) | (1<<16)) | ((1<<15) | ( 1<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | ( 2<<24) | (1<<16)) | ((1<<15) | ( 2<<8) | (1<<0));
					break;
				case 7:
					last_run_lev_0 = ((1<<31) | ( 5<<24) | (1<<16)) | ((1<<15) | ( 5<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | ( 6<<24) | (1<<16)) | ((1<<15) | ( 6<<8) | (1<<0));
					break;
				case 8:
					last_run_lev_0 = ((1<<31) | (10<<24) | (1<<16)) | ((1<<15) | (10<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (11<<24) | (1<<16)) | ((1<<15) | (11<<8) | (1<<0));
					break;
				case 9:
					last_run_lev_0 = ((1<<31) | (13<<24) | (1<<16)) | ((1<<15) | (13<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (14<<24) | (1<<16)) | ((1<<15) | (14<<8) | (1<<0));
					break;
				case 10:
					last_run_lev_0 = ((1<<31) | (19<<24) | (1<<16)) | ((1<<15) | (19<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (20<<24) | (1<<16)) | ((1<<15) | (20<<8) | (1<<0));
					break;
				case 11:
					last_run_lev_0 = ((1<<31) | (24<<24) | (1<<16)) | ((1<<15) | (24<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (25<<24) | (1<<16)) | ((1<<15) | (25<<8) | (1<<0));
					break;
				case 12:
					last_run_lev_0 = ((1<<31) | (27<<24) | (1<<16)) | ((1<<15) | (27<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (28<<24) | (1<<16)) | ((1<<15) | (28<<8) | (1<<0));
					break;
				case 13:
					last_run_lev_0 = ((1<<31) | (34<<24) | (1<<16)) | ((1<<15) | (34<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (35<<24) | (1<<16)) | ((1<<15) | (35<<8) | (1<<0));
					break;
				case 14:
					last_run_lev_0 = ((1<<31) | (36<<24) | (1<<16)) | ((1<<15) | (36<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (37<<24) | (1<<16)) | ((1<<15) | (37<<8) | (1<<0));
					break;
				case 15:
					last_run_lev_0 = ((1<<31) | (39<<24) | (1<<16)) | ((1<<15) | (39<<8) | (1<<0));
					last_run_lev_1 = ((1<<31) | (40<<24) | (1<<16)) | ((1<<15) | (40<<8) | (1<<0));
					break;
				default:
					last_run_lev_0 = 0;
					last_run_lev_1 = 0;
					break;
				}
				
				if (offset_type0 == 0)
				{
					last_run_lev = is_intra ? (last_run_lev_0 >> 16) : (last_run_lev_0 & 0xffff);
				}
				else
				{
					last_run_lev = is_intra ? (last_run_lev_1 >> 16) : (last_run_lev_1 & 0xffff);
				}
			}
			else
			{
				/*for type1, look up huffman table to get run level and las*/
				tbuf_addr		= offset_type1;
				tbuf_rdata		= vsp_huff_dcac_tab[tbuf_addr];
				last_run_lev	= is_intra ? (tbuf_rdata >> 16) : (tbuf_rdata & 0xffff);
			}

			last	= (last_run_lev >> 15) & 1;
			run		= (last_run_lev >> 8)  & 0x3f;
			level	= (last_run_lev >> 0)  & 0xff;
			sign	= (code_type == 0) ? sign_type0 : sign_type1;
		}

		/*********************************************************************
			update coefficient position
		*********************************************************************/

		i = i + run;
		if (i > 63)
		{
			printf ("rvld ac coeff error!\n");
		}

		index				= pScan[i];

		coeff				= sign ? -level : level;

		index_asic			= postrans [index&7] * 8 + postrans [(index>>3)];

		//blk_ptr[index_asic]	= coeff;
		pBlk[index]		= coeff;	

		nz_flag_acdc		= NZFlagAcdcGen (nz_flag_acdc, index_asic);
		
		i++;

		flushLen = (code_type == 2) ? 30 :
				   (code_type == 0) ? code_len_type0 : code_len_type1;

#if _TRACE_
		FPRINTF (g_pfRunLevel_mpeg4dec, "last: %d, run: %d, level: %d, sign: %d, index: %d, asic_index: %d, code_len: %d\n", 
			last, run, level, sign, i, index_asic, flushLen);
#endif

		flush_nbits (flushLen);

		coeff_num++;
	}

	blk_to_asicOrder (pBlk, pBlk_asic, noneZeroFlag);

	configure_noneZeroFlag_blk (noneZeroFlag, (uint32 *)(vsp_dct_io_1 + 192 + blk_id*4));	

	return nz_flag_acdc;
}
