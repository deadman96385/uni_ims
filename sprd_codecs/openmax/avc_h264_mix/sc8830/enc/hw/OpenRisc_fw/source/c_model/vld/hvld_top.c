/*hvld_top.c*/
#include "video_common.h"
#include "hvld_mode.h"
#include "hvld_global.h"
#include "hdbk_global.h"


void InitVldTrace ();

void init_hvld ()
{
#if defined(H264_DEC)
	/**/
// 	g_hvld_reg_ptr = (HVLD_REG_T *)malloc (sizeof(HVLD_REG_T));
// 	g_dbk_reg_ptr  = (HDBK_MODE_T *)malloc (sizeof(HDBK_MODE_T));
	
	/*init */
//	memcpy (g_hvld_huff_tab, g_huff_tab_token, 69*sizeof(uint32));

//	g_vsp_mbc_out_bfr = (uint32 *)malloc(172*sizeof(uint32));
//	g_vsp_dbk_out_bfr = (uint32 *)malloc(172*sizeof(uint32));
//
//	memset (g_vsp_mbc_out_bfr, 0xff, 172*sizeof(uint32));
//	memset (g_vsp_dbk_out_bfr, 0xff, 172*sizeof(uint32));
	
	memset (g_dbk_line_buf, 0x00, 1024*sizeof(uint32));

// 	InitDbkTrace ();

	InitVldTrace ();

	/*init test vector for vld module verification*/
//	HvldTestVecInit ();
	/*init bsm for vld module verification*/
// 	BsmInitCmd ();

// 	HdbkTestVecInit ();
#endif
}