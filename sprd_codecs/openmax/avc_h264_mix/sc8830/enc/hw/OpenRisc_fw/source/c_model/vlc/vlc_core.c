/******************************************************************************
 ** File Name:    vlc_core.c												  *
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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/

const char grgiStandardZigzag [64] = {
	0, 1, 8, 16, 9, 2, 3, 10, 
		17, 24, 32, 25, 18, 11, 4, 5, 
		12, 19, 26, 33, 40, 48, 41, 34, 
		27, 20, 13, 6, 7, 14, 21, 28, 
		35, 42, 49, 56, 57, 50, 43, 36, 
		29, 22, 15, 23, 30, 37, 44, 51, 
		58, 59, 52, 45, 38, 31, 39, 46, 
		53, 60, 61, 54, 47, 55, 62, 63
};

void GetNzFlagInScanOrder (int blk_id, uint32 flag_in_scan_order[2])
{
	int i;
	int index;
	int is_nz;
	uint32 nz_flag[4];
	uint32 nz_flag_in_raster[2];

	flag_in_scan_order[0] = 0;
	flag_in_scan_order[1] = 0;
	

	nz_flag[0] = vsp_dct_io_1[192 + blk_id*4];
	nz_flag[1] = vsp_dct_io_1[192 + blk_id*4 + 1];
	nz_flag[2] = vsp_dct_io_1[192 + blk_id*4 + 2];
	nz_flag[3] = vsp_dct_io_1[192 + blk_id*4 + 3];

	nz_flag_in_raster[0] = 	( (nz_flag[1] & 0xffff) << 16 ) | ( (nz_flag[0] & 0xffff) << 0  );
	nz_flag_in_raster[1] = 	( (nz_flag[3] & 0xffff) << 16 ) | ( (nz_flag[2] & 0xffff) << 0  );

//	nz_flag_in_raster[0] = 	( (((nz_flag[1] >> 4) & 0xf000) | (nz_flag[1] & 0xfff)) << 16 ) |
//							( (((nz_flag[0] >> 4) & 0xf000) | (nz_flag[0] & 0xfff)) << 0  );
//
//	nz_flag_in_raster[1] =  ( (((nz_flag[3] >> 4) & 0xf000) | (nz_flag[3] & 0xfff)) << 16 ) |
//						    ( (((nz_flag[2] >> 4) & 0xf000) | (nz_flag[2] & 0xfff)) << 0  );

	for (i = 0; i < 64; i++)
	{
		index = grgiStandardZigzag [i];	

		if (index < 32)
		{
			is_nz = (nz_flag_in_raster[0] >> index) & 1;
		}
		else 
		{
			is_nz = (nz_flag_in_raster[1] >> (index - 32) ) & 1;
		}

		if (i < 32 )
		{
			flag_in_scan_order [0] |= is_nz << i;
		}
		else
		{
			flag_in_scan_order [1] |= is_nz << (i - 32); 
		}
	}
}

int GetRun (uint16 flag_16coeff)
{
	int run;
	
	if (flag_16coeff == 0)   //real run >= 16
	{
		run = 16;
	}
	else if (flag_16coeff & 0x00ff)    //run <8
	{
		if (flag_16coeff & 0x000f)    //run < 4
		{
			if (flag_16coeff & 0x0001)
			{
				run = 0;
			}
			else if (flag_16coeff & 0x0002)
			{
				run = 1;
			}
			else if (flag_16coeff & 0x0004)
			{
				run = 2;
			}
			else
			{
				run = 3;
			}
		}
		else			// 4 =< run < 8
		{
			if (flag_16coeff & 0x0010)
			{
				run = 4;
			}
			else if (flag_16coeff & 0x0020)
			{
				run = 5;
			}
			else if (flag_16coeff & 0x0040)
			{
				run = 6;
			}
			else
			{
				run = 7;
			}
		}
	}
	else								//  8 =< run <16
	{
		if (flag_16coeff & 0x0f00)
		{
			if (flag_16coeff & 0x0100)
			{
				run = 8;
			}
			else if (flag_16coeff & 0x0200)
			{
				run = 9;
			}
			else if (flag_16coeff & 0x0400)
			{
				run = 10;
			}
			else
			{
				run = 11;
			}
		}
		else
		{
			if (flag_16coeff & 0x1000)
			{
				run = 12;
			}
			else if (flag_16coeff & 0x2000)
			{
				run = 13;
			}
			else if (flag_16coeff & 0x4000)
			{
				run = 14;
			}
			else
			{
				run = 15;
			}
		}
	}
	
	return run;
}

void rlc_blk (
			  /**/
			  int		blk_id,
			  int		standard,
			  int		start_position,

			  /**/
			  int		is_intra,
			  int		is_short_header
			  )
{
	int		is_dc;
	int		last;
	int		run;
	int16	level;
	int		is_nz_coeff;

	/*register definition*/
	int		run_acc;			//only for mpeg4, it is true run
	int		coeff_index;		//for get coeff from dbuf

	uint32	flag_in_scan_order[2];

	uint16	flag_16coeff;

	int		vlc_dbuf_addr;
	int		vlc_dbuf_rdata;

	int		index_in_blk;
	int		asic_index;

	uint32	tmp;
	uint32	shift_bits;

	int		sign;
	int		abs_level;
	static  int coeff_num = 0;
	int		map[8] = {0, 4, 2, 6, 1, 5, 3, 7};

#if defined(MPEG4_ENC)
	if ((g_nFrame_enc == 1) & (g_enc_vop_mode_ptr->mb_x == 3) && (g_enc_vop_mode_ptr->mb_y == 0) && (blk_id == 3))
		printf ("");
#endif

	run_acc		= 0;
	coeff_index = start_position;

	/*first, load none-zero flag, and re-order it into zigzag scan order*/
	GetNzFlagInScanOrder (blk_id, flag_in_scan_order);

	/*encode DC coeff*/
	if (standard == JPEG)
	{
		is_dc = 1;
		run = 0;
		level = (int16)((vsp_dct_io_1[32*blk_id] << 16) >> 16);
		jpeg_vlc(blk_id, is_dc, run, level);
		
		coeff_index++;
		is_dc = 0;
	}
	

	if (coeff_index == 1)
	{	
		tmp = flag_in_scan_order[1] & 1;

		flag_in_scan_order[0] = (flag_in_scan_order[0] >> 1) | (tmp << 31);

		flag_in_scan_order[1] = flag_in_scan_order[1] >> 1;
	}
	
	FPRINTF (g_fp_trace_vlc, "None-zero falg of the block:\n");
	fprintf_oneWord_hex (g_fp_trace_vlc, flag_in_scan_order[0]);
	fprintf_oneWord_hex (g_fp_trace_vlc, flag_in_scan_order[1]);

	if ((flag_in_scan_order[0] == 0) && (flag_in_scan_order[1] == 0))
	{
		last = 1;
	}
	else 
	{
		last = 0;
	}

	while (!last)
	{
		if (coeff_num == 8)
			printf ("");

		/*get run according to none_zero flag*/
		flag_16coeff = (uint16)(flag_in_scan_order[0] & 0xffff);

		run = GetRun (flag_16coeff);

		run_acc += run;
		
		if (run == 16)
		{
			is_nz_coeff = 0;
			level = 0;
		}
		else
		{
			is_nz_coeff = 1;
		}

		if ((standard == JPEG) && (run == 16))
			run = 15;

		/*get level from dbuf, if none-zero coeff found*/
		if (is_nz_coeff)
		{
			coeff_index		+= run_acc;
 			index_in_blk	= grgiStandardZigzag[coeff_index];
			asic_index      = map[index_in_blk&7] * 8 + map[index_in_blk >> 3];
			vlc_dbuf_addr	= blk_id*32 + (asic_index >> 1);
			vlc_dbuf_rdata	= vsp_dct_io_1[vlc_dbuf_addr];
			level			= (asic_index & 1) ? (int16)(vlc_dbuf_rdata >> 16) : 
							  (int16)((vlc_dbuf_rdata<<16) >> 16);
			coeff_index++;
		}

		/*update none-zero flag*/
		shift_bits = is_nz_coeff ? (run + 1) : 16;
		tmp = flag_in_scan_order[1] << (32-shift_bits);
		flag_in_scan_order[0] = (flag_in_scan_order[0] >> shift_bits) | tmp;
		flag_in_scan_order[1] = flag_in_scan_order[1] >> shift_bits;


		/*judge whether the last coeff*/
		if ((flag_in_scan_order[0] == 0) && (flag_in_scan_order[1] == 0))
		{
			last = 1;
		}

		if (level > 0)
		{
			sign = 0;
			abs_level = level;
		}
		else
		{
			sign = 1;
			abs_level = -level;
		}

//		if (coeff_num == 0x2d5)
	//		fclose (g_fp_trace_vlc);

		/*encode one event*/
		if (standard == JPEG)
		{
			jpeg_vlc (blk_id, is_dc, run, level);

			if(is_nz_coeff)
			{
				run_acc = 0;
			}
		}
		else
		{
			if (is_nz_coeff)
			{
				if (abs_level == 18)
					printf ("");

				FPRINTF (g_fp_trace_vlc, "last: %d, run: %2d, level: %3d, sign: %d, ", last, run_acc, abs_level, sign);
				mpeg4_vlc (is_short_header, is_intra, run_acc, level, last);

				PrintRlcInf (run_acc, level, last);

				run_acc = 0;
			}
		}

		FFLUSH(g_fp_trace_vlc);
		coeff_num++;
	}
	
	/*if jpeg, encode "eob" 0/0 if the 64th coeff is zero*/
	if (standard == JPEG)
	{
		if (coeff_index < 64)
		{
			jpeg_vlc(blk_id, is_dc, 0, 0);
		}
	}
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
