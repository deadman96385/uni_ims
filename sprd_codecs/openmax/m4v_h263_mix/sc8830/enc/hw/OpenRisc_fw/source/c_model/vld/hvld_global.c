/*hvld_global.c*/
#include "video_common.h"
#include "hvld_mode.h"

// HVLD_REG_T	 *	g_hvld_reg_ptr;

// int32			g_dct_io_buf[256];
/**********************************************************************
cavlc:
	  68 words are used for coeff_token decoder, from 176 start to store level

cabac: 
	  0~4 words: for coded clock flag of 5 block categories, each word contain 
				4 context model according to different neihbor coded information
	  5~38 words: for sig_map of 5 block categories. 2 words for chroma_dc,
				 and 8 words for other categories
	  need to modify since there are 8x8 transfrom
	  0~5 words: for coded block flag of 6 block categories, each word contain 
				4 context model according to different neihbor coded information
**********************************************************************/
uint32			g_hvld_huff_tab[196];//由于8x8变换的存在，这个数据结构也需要改进，VLC的时候不需要改进，因为还是按照4x4来存的，
											

char g_inverse_scan[16] = 
{
	0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15, 
};

char g_is_dctorder [16] =
{
//	0, 8, 2, 1, 10, 4, 12, 6, 9, 3, 11, 5, 14, 13, 7, 15
	0, 2, 8, 4, 10, 1, 3, 9, 6, 12, 14, 5, 11, 7, 13, 15

};

int				g_hvld_dec_status = 0;			//
FILE	*		g_hvld_trace_fp;

