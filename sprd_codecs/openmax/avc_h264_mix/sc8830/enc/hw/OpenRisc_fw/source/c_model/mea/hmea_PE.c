/*process_element.c*/
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"

/*register for storing four block8x8's sad for four process element*/
// uint16		g_sad_blk0_pe0;
// uint16		g_sad_blk1_pe0;
// uint16		g_sad_blk2_pe0;
// uint16		g_sad_blk3_pe0;
// 
// uint16		g_sad_blk0_pe1;
// uint16		g_sad_blk1_pe1;
// uint16		g_sad_blk2_pe1;
// uint16		g_sad_blk3_pe1;
// 
// uint16		g_sad_blk0_pe2;
// uint16		g_sad_blk1_pe2;
// uint16		g_sad_blk2_pe2;
// uint16		g_sad_blk3_pe2;
// 
// uint16		g_sad_blk0_pe3;
// uint16		g_sad_blk1_pe3;
// uint16		g_sad_blk2_pe3;
// uint16		g_sad_blk3_pe3;

extern	int g_is_sp;
extern	int g_print_pix;
extern	uint32 src_word_00;			//register for source pixels
extern	uint32 src_word_01;
extern	uint32 src_word_02;
extern	uint32 src_word_03;

extern	uint32 src_word_10;
extern	uint32 src_word_11;
extern	uint32 src_word_12;
extern	uint32 src_word_13;

uint8 PixSad (
				uint8	ref_top_p0,
				uint8	ref_top_p1,
				uint8	ref_bot_p0,
				uint8	ref_bot_p1,
				uint8	src_pix,
				int		dx,
				int		dy,
				int		single_pos
				)
{
	uint8	ref_pix;
	uint8	sad_pix;

	/*bilinear interpolation*/
/*	ref_pix = (ref_top_p0 * (4 - dx) * (4 - dy)  + 
			   ref_top_p1 * dx * (4 - dy)	 +
			   ref_bot_p0 * (4 - dx) * dy)		+ 
			   ref_bot_p1 * dx * dy +  8) / 16;
*/

	switch(single_pos)
	{
		case 1:
			ref_pix = (ref_top_p0 + ref_bot_p0)/2;
			break;
// 		case 1:
// 			ref_pix = (ref_top_p1 + ref_bot_p1)/2;
// 			break;
		default:
			ref_pix = ref_top_p0;
			break;
	}
	
	/*compute one pixel sad*/ 
	if (src_pix > ref_pix)
		sad_pix = src_pix - ref_pix;
	else
		sad_pix = ref_pix - src_pix;

	return sad_pix;	
}

int PixSatd (
				uint8	ref_top_p0,
				uint8	ref_top_p1,
				uint8	ref_bot_p0,
				uint8	ref_bot_p1,
				uint8	src_pix,
				int		dx,
				int		dy,
				int		single_pos
				)
{
	int	ref_pix;
	int	sad_pix;

	/*bilinear interpolation*/
/*	ref_pix = (ref_top_p0 * (4 - dx) * (4 - dy)  + 
			   ref_top_p1 * dx * (4 - dy)	 +
			   ref_bot_p0 * (4 - dx) * dy)		+ 
			   ref_bot_p1 * dx * dy +  8) / 16;
*/

	switch(single_pos)
	{
		case 1:
			ref_pix = (ref_top_p0 + ref_bot_p0)/2;
			break;
 //		case 1:
 //			ref_pix = (ref_top_p1 + ref_bot_p1)/2;
// 			break;
		default:
			ref_pix = ref_top_p0;
			break;
	}
	
	/*compute one pixel sad*/ 
//	if (src_pix > ref_pix)
		sad_pix = src_pix - ref_pix;
//	else
//		sad_pix = ref_pix - src_pix;

	return sad_pix;	
}

void ProcessElement (
					 uint32		ref_top0,
					 uint32		ref_top1,
					 uint32		ref_bot0,
					 uint32		ref_bot1,
					 uint32		src_word,
					 int		byte_offset,
					 int		dx,			//byte offset and sub-pixel postion
					 int		dy,
					 int		col_cnt,
					 int		row_cnt,
					 int		pe_num,
					 int		pe_active
					 )
{
	uint8	ref_top_p0;
	uint8	ref_top_p1;
	uint8	ref_top_p2;
	uint8	ref_top_p3;
	uint8	ref_top_p4;
	
	uint8	ref_bot_p0;
	uint8	ref_bot_p1;
	uint8	ref_bot_p2;
	uint8	ref_bot_p3;
	uint8	ref_bot_p4;

	uint8	src_p0;
	uint8	src_p1;
	uint8	src_p2;
	uint8	src_p3;

	uint8	sad_p0;
	uint8	sad_p1;
	uint8	sad_p2;
	uint8	sad_p3;
	uint16	sad_word;

	int		is_blk0;
	int		is_blk1;
	int		is_blk2;
	int		is_blk3;
	int		single_pos;

	if (!pe_active)
		return;

	/*bilinear interpolation*/

	if ((row_cnt == 0) && !g_is_sp && (col_cnt == 3) && (pe_num == 0))
	{
		printf ("");
	}
	
	if(dx)
	{
		single_pos = 1;
	}
	else
	{
		single_pos = 0;
	}

	/*multiplex for getting reference pixels according to pixel byte offset*/
	if (byte_offset == 0)
	{
		ref_top_p0 = (ref_top0 >>  0) & 0xff;
		ref_top_p1 = (ref_top0 >>  8) & 0xff;
		ref_top_p2 = (ref_top0 >> 16) & 0xff;
		ref_top_p3 = (ref_top0 >> 24) & 0xff;
		ref_top_p4 = (ref_top1 >> 0) & 0xff;

		ref_bot_p0 = (ref_bot0 >>  0) & 0xff;
		ref_bot_p1 = (ref_bot0 >>  8) & 0xff;
		ref_bot_p2 = (ref_bot0 >> 16) & 0xff;
		ref_bot_p3 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p4 = (ref_bot1 >>  0) & 0xff;
	}
	else if (byte_offset == 1)
	{
		
		ref_top_p0 = (ref_top0 >>  8) & 0xff;
		ref_top_p1 = (ref_top0 >> 16) & 0xff;
		ref_top_p2 = (ref_top0 >> 24) & 0xff;
		ref_top_p3 = (ref_top1 >>  0) & 0xff;
		ref_top_p4 = (ref_top1 >>  8) & 0xff;
		
		ref_bot_p0 = (ref_bot0 >>  8) & 0xff;
		ref_bot_p1 = (ref_bot0 >> 16) & 0xff;
		ref_bot_p2 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p3 = (ref_bot1 >>  0) & 0xff;
		ref_bot_p4 = (ref_bot1 >>  8) & 0xff;
	}	
	else if (byte_offset == 2)
	{
		
		ref_top_p0 = (ref_top0 >> 16) & 0xff;
		ref_top_p1 = (ref_top0 >> 24) & 0xff;
		ref_top_p2 = (ref_top1 >>  0) & 0xff;
		ref_top_p3 = (ref_top1 >>  8) & 0xff;
		ref_top_p4 = (ref_top1 >> 16) & 0xff;
		
		ref_bot_p0 = (ref_bot0 >> 16) & 0xff;
		ref_bot_p1 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p2 = (ref_bot1 >>  0) & 0xff;
		ref_bot_p3 = (ref_bot1 >>  8) & 0xff;
		ref_bot_p4 = (ref_bot1 >> 16) & 0xff;
	}
	else
	{
		
		ref_top_p0 = (ref_top0 >> 24) & 0xff;
		ref_top_p1 = (ref_top1 >>  0) & 0xff;
		ref_top_p2 = (ref_top1 >>  8) & 0xff;
		ref_top_p3 = (ref_top1 >> 16) & 0xff;
		ref_top_p4 = (ref_top1 >> 24) & 0xff;
		
		ref_bot_p0 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p1 = (ref_bot1 >>  0) & 0xff;
		ref_bot_p2 = (ref_bot1 >>  8) & 0xff;
		ref_bot_p3 = (ref_bot1 >> 16) & 0xff;
		ref_bot_p4 = (ref_bot1 >> 24) & 0xff;
	}

	src_p0 = (src_word >>  0) & 0xff;
	src_p1 = (src_word >>  8) & 0xff;
	src_p2 = (src_word >> 16) & 0xff;
	src_p3 = (src_word >> 24) & 0xff;

	//instantiation four sad computation module
	sad_p0 = PixSad (ref_top_p0, ref_top_p1, ref_bot_p0, ref_bot_p1, src_p0, dx, dy, single_pos);
	sad_p1 = PixSad (ref_top_p1, ref_top_p2, ref_bot_p1, ref_bot_p2, src_p1, dx, dy, single_pos);
	sad_p2 = PixSad (ref_top_p2, ref_top_p3, ref_bot_p2, ref_bot_p3, src_p2, dx, dy, single_pos);
	sad_p3 = PixSad (ref_top_p3, ref_top_p4, ref_bot_p3, ref_bot_p4, src_p3, dx, dy, single_pos);

#if 0
	//fprintf (g_me_trace_fp, "\npe0_top0: %d, pe0_src: %d\n", ref_top_p0, src_p0);
	//fprintf (g_me_trace_fp, "\npe1_top0: %d, pe1_src: %d\n", ref_top_p1, src_p1);
	//fprintf (g_me_trace_fp, "\npe1_top0: %d, pe1_src: %d\n", ref_top_p2, src_p2);
	//fprintf (g_me_trace_fp, "\npe1_top0: %d, pe1_src: %d\n", ref_top_p3, src_p3);
	//	fprintf (g_me_trace_fp, "%0x, %0x, %0x, %0x, \n", sad_p0, sad_p1, sad_p2, sad_p3);
#endif

	sad_word = (sad_p0 + sad_p1) + (sad_p2 + sad_p3);

	if (row_cnt < 8)
	{
		is_blk0 = (col_cnt < 2) ? 1 : 0;
		is_blk1 = (col_cnt < 2) ? 0 : 1;
		is_blk2 = 0;
		is_blk3 = 0;
	}
	else
	{
		is_blk0 = 0;
		is_blk1 = 0;
		is_blk2 = (col_cnt < 2) ? 1 : 0;
		is_blk3 = (col_cnt < 2) ? 0 : 1;
	}
	
	if (pe_num == 0)
	{
		g_sad_blk0_pe0 = is_blk0 ? (g_sad_blk0_pe0 + sad_word) : g_sad_blk0_pe0;
		g_sad_blk1_pe0 = is_blk1 ? (g_sad_blk1_pe0 + sad_word) : g_sad_blk1_pe0;
		g_sad_blk2_pe0 = is_blk2 ? (g_sad_blk2_pe0 + sad_word) : g_sad_blk2_pe0;
		g_sad_blk3_pe0 = is_blk3 ? (g_sad_blk3_pe0 + sad_word) : g_sad_blk3_pe0;
	}
	else if (pe_num == 1)
	{
		g_sad_blk0_pe1 = is_blk0 ? (g_sad_blk0_pe1 + sad_word) : g_sad_blk0_pe1;
		g_sad_blk1_pe1 = is_blk1 ? (g_sad_blk1_pe1 + sad_word) : g_sad_blk1_pe1;
		g_sad_blk2_pe1 = is_blk2 ? (g_sad_blk2_pe1 + sad_word) : g_sad_blk2_pe1;
		g_sad_blk3_pe1 = is_blk3 ? (g_sad_blk3_pe1 + sad_word) : g_sad_blk3_pe1;
	}
	else if (pe_num == 2)
	{
		g_sad_blk0_pe2 = is_blk0 ? (g_sad_blk0_pe2 + sad_word) : g_sad_blk0_pe2;
		g_sad_blk1_pe2 = is_blk1 ? (g_sad_blk1_pe2 + sad_word) : g_sad_blk1_pe2;
		g_sad_blk2_pe2 = is_blk2 ? (g_sad_blk2_pe2 + sad_word) : g_sad_blk2_pe2;
		g_sad_blk3_pe2 = is_blk3 ? (g_sad_blk3_pe2 + sad_word) : g_sad_blk3_pe2;
	}
	else
	{
		g_sad_blk0_pe3 = is_blk0 ? (g_sad_blk0_pe3 + sad_word) : g_sad_blk0_pe3;
		g_sad_blk1_pe3 = is_blk1 ? (g_sad_blk1_pe3 + sad_word) : g_sad_blk1_pe3;
		g_sad_blk2_pe3 = is_blk2 ? (g_sad_blk2_pe3 + sad_word) : g_sad_blk2_pe3;
		g_sad_blk3_pe3 = is_blk3 ? (g_sad_blk3_pe3 + sad_word) : g_sad_blk3_pe3;
	}

	if ((row_cnt == 0) && !g_is_sp && (col_cnt == 3) && (pe_num == 0))
	{
		printf ("");
	}
}

void ProcessElement_SATD(
								uint32	ref_top0,
								uint32	ref_top1, 
								uint32	ref_bot0, 
								uint32 	ref_bot1, 
								uint32 	src_word, 
								int		byte_offset, 
								int 		dx, 
								int 		dy, 
								int 		col_cnt, 
								int 		row_cnt, 
								int 		pe_num, 
								int 		pe_active
								)
{
	uint8	ref_top_p0;
	uint8	ref_top_p1;
	uint8	ref_top_p2;
	uint8	ref_top_p3;
	uint8	ref_top_p4;
	
	uint8	ref_bot_p0;
	uint8	ref_bot_p1;
	uint8	ref_bot_p2;
	uint8	ref_bot_p3;
	uint8	ref_bot_p4;

	uint8	src_p0;
	uint8	src_p1;
	uint8	src_p2;
	uint8	src_p3;

	int	sad_p0;
	int	sad_p1;
	int	sad_p2;
	int	sad_p3;

	uint8	blk_num;
	uint8	pix_num;
	int		single_pos;

	if (!pe_active)
		return;

	if ((row_cnt == 0) && !g_is_sp && (col_cnt == 3) && (pe_num == 0))
	{
		printf ("");
	}
	
	if(dx)
	{
		single_pos = 1;
	}
	else
	{
		single_pos = 0;
	}

	/*multiplex for getting reference pixels according to pixel byte offset*/
	if (byte_offset == 0)
	{
		ref_top_p0 = (ref_top0 >>  0) & 0xff;
		ref_top_p1 = (ref_top0 >>  8) & 0xff;
		ref_top_p2 = (ref_top0 >> 16) & 0xff;
		ref_top_p3 = (ref_top0 >> 24) & 0xff;
		ref_top_p4 = (ref_top1 >> 0) & 0xff;

		ref_bot_p0 = (ref_bot0 >>  0) & 0xff;
		ref_bot_p1 = (ref_bot0 >>  8) & 0xff;
		ref_bot_p2 = (ref_bot0 >> 16) & 0xff;
		ref_bot_p3 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p4 = (ref_bot1 >>  0) & 0xff;
	}
	else if (byte_offset == 1)
	{
		
		ref_top_p0 = (ref_top0 >>  8) & 0xff;
		ref_top_p1 = (ref_top0 >> 16) & 0xff;
		ref_top_p2 = (ref_top0 >> 24) & 0xff;
		ref_top_p3 = (ref_top1 >>  0) & 0xff;
		ref_top_p4 = (ref_top1 >>  8) & 0xff;
		
		ref_bot_p0 = (ref_bot0 >>  8) & 0xff;
		ref_bot_p1 = (ref_bot0 >> 16) & 0xff;
		ref_bot_p2 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p3 = (ref_bot1 >>  0) & 0xff;
		ref_bot_p4 = (ref_bot1 >>  8) & 0xff;
	}	
	else if (byte_offset == 2)
	{
		
		ref_top_p0 = (ref_top0 >> 16) & 0xff;
		ref_top_p1 = (ref_top0 >> 24) & 0xff;
		ref_top_p2 = (ref_top1 >>  0) & 0xff;
		ref_top_p3 = (ref_top1 >>  8) & 0xff;
		ref_top_p4 = (ref_top1 >> 16) & 0xff;
		
		ref_bot_p0 = (ref_bot0 >> 16) & 0xff;
		ref_bot_p1 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p2 = (ref_bot1 >>  0) & 0xff;
		ref_bot_p3 = (ref_bot1 >>  8) & 0xff;
		ref_bot_p4 = (ref_bot1 >> 16) & 0xff;
	}
	else
	{
		
		ref_top_p0 = (ref_top0 >> 24) & 0xff;
		ref_top_p1 = (ref_top1 >>  0) & 0xff;
		ref_top_p2 = (ref_top1 >>  8) & 0xff;
		ref_top_p3 = (ref_top1 >> 16) & 0xff;
		ref_top_p4 = (ref_top1 >> 24) & 0xff;
		
		ref_bot_p0 = (ref_bot0 >> 24) & 0xff;
		ref_bot_p1 = (ref_bot1 >>  0) & 0xff;
		ref_bot_p2 = (ref_bot1 >>  8) & 0xff;
		ref_bot_p3 = (ref_bot1 >> 16) & 0xff;
		ref_bot_p4 = (ref_bot1 >> 24) & 0xff;
	}

	src_p0 = (src_word >>  0) & 0xff;
	src_p1 = (src_word >>  8) & 0xff;
	src_p2 = (src_word >> 16) & 0xff;
	src_p3 = (src_word >> 24) & 0xff;

	//instantiation four sad computation module
	sad_p0 = PixSatd (ref_top_p0, ref_top_p1, ref_bot_p0, ref_bot_p1, src_p0, dx, dy, single_pos);
	sad_p1 = PixSatd (ref_top_p1, ref_top_p2, ref_bot_p1, ref_bot_p2, src_p1, dx, dy, single_pos);
	sad_p2 = PixSatd (ref_top_p2, ref_top_p3, ref_bot_p2, ref_bot_p3, src_p2, dx, dy, single_pos);
	sad_p3 = PixSatd (ref_top_p3, ref_top_p4, ref_bot_p3, ref_bot_p4, src_p3, dx, dy, single_pos);

	blk_num = col_cnt + (row_cnt/4)*4;
	pix_num = (row_cnt%4)*4;

	diff[pe_num][blk_num][pix_num++] = sad_p0;
	diff[pe_num][blk_num][pix_num++] = sad_p1;
	diff[pe_num][blk_num][pix_num++] = sad_p2;
	diff[pe_num][blk_num][pix_num] = sad_p3;
	
	if ((row_cnt == 0) && !g_is_sp && (col_cnt == 3) && (pe_num == 0))
	{
		printf ("");
	}
}
