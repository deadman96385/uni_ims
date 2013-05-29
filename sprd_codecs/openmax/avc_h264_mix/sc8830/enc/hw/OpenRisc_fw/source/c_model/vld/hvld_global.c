/*hvld_global.c*/
#include "video_common.h"
#include "hvld_mode.h"

// HVLD_REG_T	 *	g_hvld_reg_ptr;

// int32			g_dct_io_buf[256];
uint32			g_hvld_huff_tab[196];		//only 68 word is used for coeff_token

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

