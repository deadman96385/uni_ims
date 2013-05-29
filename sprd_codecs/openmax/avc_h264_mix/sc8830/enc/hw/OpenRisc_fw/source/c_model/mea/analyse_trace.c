/*analyse_trace.c*/
#include <stdio.h>
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"

FILE *	g_me_trace_fp;
//FILE *	g_me_intpl_fp;
FILE *  g_me_pred_mv_fp;

//FILE *  g_src_out_fp;		//for debugging fetch and pre-filter  
//FILE *  g_src_out_txt_fp;

//uint32 * g_src_out_frm[3];	

void TraceInit ()
{
	g_me_trace_fp = fopen ("..\\..\\trace\\me_trace.txt", "w");
	g_me_pred_mv_fp = fopen ("..\\..\\trace\\me_pred_mv_trace.txt", "w");
//	g_me_intpl_fp = fopen("..\\..\\trace\\me_interpolation_trace.txt", "w");

//	g_src_out_fp  = fopen ("..\\..\\trace\\src_frame_out.yuv", "wb");
//	g_src_out_txt_fp = fopen ("..\\..\\trace\\src_frame_out.txt", "w");

//	g_src_out_frm[0] = (uint32 *)malloc(g_input->pic_height*g_input->pic_width/4*sizeof(uint32));
//	g_src_out_frm[1] = (uint32 *)malloc(g_input->pic_height*g_input->pic_width/4/4*sizeof(uint32));
//	g_src_out_frm[2] = (uint32 *)malloc(g_input->pic_height*g_input->pic_width/4/4*sizeof(uint32));
}

// void SmallDiamondTrace (						
// 						int	rd_cost_0,
// 						int	rd_cost_1, 
// 						int	rd_cost_2, 
// 						int	rd_cost_3, 
// 						int	rd_cost_4
// 						)
// {	
// 	FPRINTF_ME (g_me_trace_fp, "small diamond search:\n");
// 	FPRINTF_ME (g_me_trace_fp, "\t      %04x\n",			rd_cost_1);
// 	FPRINTF_ME (g_me_trace_fp, "\t%04x  %04x  %04x\n",		rd_cost_2, rd_cost_0, rd_cost_3);
// 	FPRINTF_ME (g_me_trace_fp, "\t      %04x\n",			rd_cost_4);
// }

#if 0
void OutSrcMBToFrame (uint32 * mea_src_buf, int mb_x, int mb_y, int frame_width)
{
	int			i;
	int			j;
	int			frm_offset;
	uint32	*	blk_ptr;
	uint32	*	frm_ptr;

	/*out y block*/
	blk_ptr		= mea_src_buf;
	frm_offset	= mb_y*16*frame_width + mb_x*4;
	frm_ptr		= g_src_out_frm[0] + frm_offset;
	for (j = 0; j < 16; j++)
	{
		for (i = 0; i < 4; i++)
		{
			frm_ptr[i] = *blk_ptr++;
			FPRINTF_ME (g_src_out_txt_fp, "%08x\n", frm_ptr[i]);
		}

		frm_ptr += frame_width;
	}
	
	frame_width = frame_width / 2;

	/*out u block*/
	blk_ptr		= mea_src_buf + 64;
	frm_offset	= mb_y*8*frame_width + mb_x*2;
	frm_ptr		= g_src_out_frm[1] + frm_offset;
	for (j = 0; j < 8; j++)
	{
		for (i = 0; i < 2; i++)
		{
			frm_ptr[i] = *blk_ptr++;
			FPRINTF_ME (g_src_out_txt_fp, "%08x\n", frm_ptr[i]);
		}
		
		frm_ptr += frame_width;
	}
	
	/*out v block*/
	blk_ptr		= mea_src_buf + 64 + 16;
	frm_offset	= mb_y*8*frame_width + mb_x*2;
	frm_ptr		= g_src_out_frm[2] + frm_offset;
	for (j = 0; j < 8; j++)
	{
		for (i = 0; i < 2; i++)
		{
			frm_ptr[i] = *blk_ptr++;
			FPRINTF_ME (g_src_out_txt_fp, "%08x\n", frm_ptr[i]);
		}
		
		frm_ptr += frame_width;
	}
}
#endif