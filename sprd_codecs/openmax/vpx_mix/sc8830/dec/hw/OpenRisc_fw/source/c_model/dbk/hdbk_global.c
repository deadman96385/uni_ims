/*hvld_global.c*/
#include "video_common.h"
#include "hdbk_mode.h"
#include "hdbk_global.h"

// VSP_DBK_REG_T	*	g_dbk_reg_ptr;	

//uint32	*		g_vsp_mbc_out_bfr;
//uint32	*		g_vsp_dbk_out_bfr;


uint32			g_dbk_line_buf[4096];	//20120803_derek

uint32	*		g_frame_y_ptr;
uint32	*		g_frame_c_ptr;

uint32	*		g_frame_y_dsp_ptr;
uint32	*		g_frame_c_dsp_ptr;

int				g_blk_id;

int				g_mb_cnt = 0;
