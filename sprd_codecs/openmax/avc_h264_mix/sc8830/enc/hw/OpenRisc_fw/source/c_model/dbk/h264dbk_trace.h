/*h264dbk_trace.h*/

#ifndef _H264DBK_TRACE_H_
#define _H264DBK_TRACE_H_

#include <stdio.h>

extern FILE * g_dbk_trace_fp;

void InitDbkTrace ();


void DBK_TraceMBInfor (
					   int		cur_qp, 
					   int		left_qp, 
					   int		top_qp, 
					   int		chroma_qp_offset, 
					   int		alpha_offset, 
					   int		beta_offset
					   );

void DBK_TraceMBBs (
					uint32	bs_h0,
					uint32	bs_h1,
					uint32	bs_v0,
					uint32	bs_v1
					);

void DBK_PrintOneMB (int mb_y, int mb_x);

void DBK_PrintFilterOneMB (int mb_y, int mb_x);

#endif

