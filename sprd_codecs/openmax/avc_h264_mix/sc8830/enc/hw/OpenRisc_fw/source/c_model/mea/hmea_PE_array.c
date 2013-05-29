/*pe_array.c*/
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"


/*****************************************************************
pe array:
				pe0
			pe1		pe2
				pe3

position:
				1
			2	0	3
				4

bestpoint

				1
			2		0
				3
*****************************************************************/

#define bilinear(p0, p1) IClip(0, 255, ((p0+p1)/2))
int satd_val[4];
int		g_min_pos;
int		g_min_cost;
uint32	ref_word;			//register for reference pixel
uint32	ref_word_d1;
uint32	ref_word_d2;
uint32	ref_word_d3;
uint32	ref_word_d4;
uint32	ref_word_d5;
uint32	ref_word_d6;
uint32	ref_word_d7;
uint32	ref_word_d8;
uint32	ref_word_d9;
uint32	ref_word_d10;
uint32	ref_word_d11;

uint32 src_word_00;			//register for source pixels
uint32 src_word_01;
uint32 src_word_02;
uint32 src_word_03;

uint32 src_word_10;
uint32 src_word_11;
uint32 src_word_12;
uint32 src_word_13;

uint32 src_word_20;
uint32 src_word_21;
uint32 src_word_22;
uint32 src_word_23;

uint32 src_word_30;
uint32 src_word_31;
uint32 src_word_32;
uint32 src_word_33;

int		g_is_sp;
int		g_print_pix;

void PEReset ()
{
	g_sad_blk0_pe0 = 0;
	g_sad_blk1_pe0 = 0;
	g_sad_blk2_pe0 = 0;
	g_sad_blk3_pe0 = 0;

	g_sad_blk0_pe1 = 0;
	g_sad_blk1_pe1 = 0;
	g_sad_blk2_pe1 = 0;
	g_sad_blk3_pe1 = 0;

	g_sad_blk0_pe2 = 0;
	g_sad_blk1_pe2 = 0;
	g_sad_blk2_pe2 = 0;
	g_sad_blk3_pe2 = 0;

	g_sad_blk0_pe3 = 0;
	g_sad_blk1_pe3 = 0;
	g_sad_blk2_pe3 = 0;
	g_sad_blk3_pe3 = 0;
	satd_val[0] = satd_val[1] = satd_val[2] = satd_val[3] =0;

	ref_word_d7 = 0;
	ref_word_d6 = 0;
	ref_word_d5 = 0;
	ref_word_d4 = 0;
	ref_word_d3 = 0;
	ref_word_d2 = 0;
	ref_word_d1 = 0;
	ref_word_d8 = 0;
	ref_word_d9 = 0;
	ref_word_d10 = 0;
	ref_word_d11 = 0;	
}

/*address generation for the search window is 128x96 */
void RefAddrGen (
				 int	x_ref, 
				 int	y_offset, 
				 int	mb_y,
				 int *	x_ref_wind_ptr, 
				 int *	y_ref_wind_ptr,
				 int	frame_width,
				 int	frame_height
				 )
{
	int x_ref_clip;
	int y_ref_clip;	
	int max_width;
	int max_height;
	int y_ref;

	max_width = (frame_width >> 2) - 1;
	max_height = frame_height - 1;

	/*coordinate clip*/
	x_ref_clip = x_ref;
	if (x_ref < 0)
		x_ref_clip = 0;
	else if (x_ref > max_width)
		x_ref_clip = max_width;

	y_ref_clip = y_offset;
	y_ref = y_offset + mb_y*MB_SIZE;
	if (y_ref < 0)
		y_ref_clip = -mb_y*MB_SIZE;
	else if(y_ref > max_height)
		y_ref_clip = max_height - mb_y*MB_SIZE;

	/*cooordiante mapping from frame to search window*/
	*x_ref_wind_ptr = (x_ref_clip + SEARCH_RANGE_X/4) & 0x1f;

	*y_ref_wind_ptr = y_ref_clip + SEARCH_RANGE_Y;
}

uint32 PixPadding (
				   uint32	ref_rdata, 
				   int		int_x_ref, 
				   int		frame_width, 
				   int		frame_height
				   )
{
	uint8	ref_pix0;
	//	uint8	ref_pix1;
	//	uint8	ref_pix2;
	uint8	ref_pix3;
	uint32	ref_padding;

	ref_pix0 = (ref_rdata >>  0) & 0xff;
	//	ref_pix1 = (ref_rdata >>  8) & 0xff;
	//	ref_pix2 = (ref_rdata >> 16) & 0xff;
	ref_pix3 = (ref_rdata >> 24) & 0xff;

	ref_padding = ref_rdata;

	if (int_x_ref < 0)
	{
		ref_padding = (ref_pix0 << 24) | (ref_pix0 << 16) | (ref_pix0 << 8) | (ref_pix0 << 0);
	}
	else if(int_x_ref >= (frame_width>>2))
	{
		ref_padding = (ref_pix3 << 24) | (ref_pix3 << 16) | (ref_pix3 << 8) | (ref_pix3 << 0);
	}

	return ref_padding;
}


void UpdateMinSadReg (int min_pos)
{
	if ((min_pos == 0) || (min_pos == 1))
	{
		g_min_cost_blk0 = g_sad_blk0_pe0;
		g_min_cost_blk1 = g_sad_blk1_pe0;
		g_min_cost_blk2 = g_sad_blk2_pe0;
		g_min_cost_blk3 = g_sad_blk3_pe0;
	}
	else if (min_pos == 2)
	{
		g_min_cost_blk0 = g_sad_blk0_pe1;
		g_min_cost_blk1 = g_sad_blk1_pe1;
		g_min_cost_blk2 = g_sad_blk2_pe1;
		g_min_cost_blk3 = g_sad_blk3_pe1;
	}
	else if (min_pos == 3)
	{
		g_min_cost_blk0 = g_sad_blk0_pe2;
		g_min_cost_blk1 = g_sad_blk1_pe2;
		g_min_cost_blk2 = g_sad_blk2_pe2;
		g_min_cost_blk3 = g_sad_blk3_pe2;
	}
	else if (min_pos == 4)
	{
		g_min_cost_blk0 = g_sad_blk0_pe3;
		g_min_cost_blk1 = g_sad_blk1_pe3;
		g_min_cost_blk2 = g_sad_blk2_pe3;
		g_min_cost_blk3 = g_sad_blk3_pe3;
	}
}

void ComputeSATD()
{
	int i,j;
	int m[16];
	int *p = m;
	int k, satd = 0;
	int d[16];

	for(i = 0; i <4; i++)
	{
		for(j =0; j <16; j++)
		{
			satd = 0;
			m[ 0] = diff[ i][j][0] + diff[ i][j][12];
			m[ 1] = diff[ i][j][ 1] + diff[ i][j][13];
			m[ 2] = diff[ i][j][ 2] + diff[ i][j][14];
			m[ 3] = diff[ i][j][ 3] + diff[ i][j][15];
			m[ 4] = diff[ i][j][ 4] + diff[ i][j][ 8];
			m[ 5] = diff[ i][j][ 5] + diff[ i][j][ 9];
			m[ 6] = diff[ i][j][ 6] + diff[ i][j][10];
			m[ 7] = diff[ i][j][ 7] + diff[ i][j][11];
			m[ 8] = diff[ i][j][ 4] - diff[ i][j][ 8];
			m[ 9] = diff[ i][j][ 5] - diff[ i][j][ 9];
			m[10] = diff[ i][j][ 6] - diff[ i][j][10];
			m[11] = diff[ i][j][ 7] - diff[ i][j][11];
			m[12] = diff[ i][j][ 0] - diff[ i][j][12];
			m[13] = diff[ i][j][ 1] - diff[ i][j][13];
			m[14] = diff[ i][j][ 2] - diff[ i][j][14];
			m[15] = diff[ i][j][ 3] - diff[ i][j][15];

			d[ 0] = m[ 0] + m[ 4];
			d[ 1] = m[ 1] + m[ 5];
			d[ 2] = m[ 2] + m[ 6];
			d[ 3] = m[ 3] + m[ 7];
			d[ 4] = m[ 8] + m[12];
			d[ 5] = m[ 9] + m[13];
			d[ 6] = m[10] + m[14];
			d[ 7] = m[11] + m[15];
			d[ 8] = m[ 0] - m[ 4];
			d[ 9] = m[ 1] - m[ 5];
			d[10] = m[ 2] - m[ 6];
			d[11] = m[ 3] - m[ 7];
			d[12] = m[12] - m[ 8];
			d[13] = m[13] - m[ 9];
			d[14] = m[14] - m[10];
			d[15] = m[15] - m[11];

			m[ 0] = d[ 0] + d[ 3];
			m[ 1] = d[ 1] + d[ 2];
			m[ 2] = d[ 1] - d[ 2];
			m[ 3] = d[ 0] - d[ 3];
			m[ 4] = d[ 4] + d[ 7];
			m[ 5] = d[ 5] + d[ 6];
			m[ 6] = d[ 5] - d[ 6];
			m[ 7] = d[ 4] - d[ 7];
			m[ 8] = d[ 8] + d[11];
			m[ 9] = d[ 9] + d[10];
			m[10] = d[ 9] - d[10];
			m[11] = d[ 8] - d[11];
			m[12] = d[12] + d[15];
			m[13] = d[13] + d[14];
			m[14] = d[13] - d[14];
			m[15] = d[12] - d[15];

			d[ 0] = m[ 0] + m[ 1];
			d[ 1] = m[ 0] - m[ 1];
			d[ 2] = m[ 2] + m[ 3];
			d[ 3] = m[ 3] - m[ 2];
			d[ 4] = m[ 4] + m[ 5];
			d[ 5] = m[ 4] - m[ 5];
			d[ 6] = m[ 6] + m[ 7];
			d[ 7] = m[ 7] - m[ 6];
			d[ 8] = m[ 8] + m[ 9];
			d[ 9] = m[ 8] - m[ 9];
			d[10] = m[10] + m[11];
			d[11] = m[11] - m[10];
			d[12] = m[12] + m[13];
			d[13] = m[12] - m[13];
			d[14] = m[14] + m[15];
			d[15] = m[15] - m[14];

			//===== sum up =====
			// Table lookup is faster than abs macro
			for (k=0; k<16; ++k)
			{
			satd += (d [k] > 0 ? d[k] : (0 - d[k]));
			}
		satd_val[i] += ((satd +1)>>1);
		}
	}
}

void PEArray(
			 int				mb_x, 
			 int				mb_y, 
			 int				blk_width, 
			 int				blk_height, 
			 int				blk8_id,		
			 MOTION_VECTOR_T *	cent_mv_ptr,	//mv for single point, or center point for small diamond, in quarter pixel precision
			 int				step_len,		//step length for small diamond, 4: for integer  2: for half pixel  1: for quarter pixel
			 int				is_sp			//is single point or small diamond pattern
			 )
{
	int		i;
	int		j;
	int		pe0_en;
	int		pe1_en;
	int		pe2_en;
	int		pe3_en;
	int		x_ref;
	int		y_ref;
	int		int_x_ref;
	int		int_y_ref;
	int		ref_width;			//word number needed in horizontal 
	int		add_width;
	int		add_line;
	int		line_num;			//line number needed in vertical
	int		x_ref_wind;			//word alignment address
	int		y_ref_wind;
	int		ref_addr;
	uint32	ref_rdata;
	uint32	ref_padding;		//four reference pixel stuffing

	int		src_col_cnt = 0;	//for reading source buffer
	int		src_row_cnt = 0;

	int		p0_x_cnt = 0;			//for judging PE active or not
	int		p0_y_cnt = 0;
	int		p1_x_cnt = 0;
	int		p1_y_cnt = 0;	
	int		p2_x_cnt = 0;
	int		p2_y_cnt = 0;
	int		p3_x_cnt = 0;
	int		p3_y_cnt = 0;
	int		pe0_active;
	int		pe1_active;
	int		pe2_active;
	int		pe3_active;

	int		pe0_hor_eff;
	int		pe0_ver_eff;
	int		pe1_hor_eff;
	int		pe1_ver_eff;
	int		pe2_hor_eff;
	int		pe2_ver_eff;
	int		pe3_hor_eff;
	int		pe3_ver_eff;

	int		pe0_cal;
	int		pe1_cal;
	int		pe2_cal;
	int		pe3_cal;

	//src buffer interface
	int		src_buf_rd;
	int		src_buf_addr;
	uint32	src_rdata;

	//reference data for four PE
	uint32	ref_top0;
	uint32	ref_top1 = 0;
	uint32	ref_bot0 = 0;
	uint32	ref_bot1 = 0;

	//source data for four PE
	uint32	src_pe0;
	uint32	src_pe1;
	uint32	src_pe2;
	uint32	src_pe3;

	uint32	src_00_temp;
	uint32	src_01_temp;
	uint32	src_02_temp;
	uint32	src_03_temp;

	int		dx_pe0;
	int		dy_pe0;
	int		dx_pe1;
	int		dy_pe1;
	int		dx_pe2;
	int		dy_pe2;
	int		dx_pe3;
	int		dy_pe3;

	int		byte_oft_pe0;
	int		byte_oft_pe1;
	int		byte_oft_pe2;
	int		byte_oft_pe3;

	int		is_blk16x16;
	int		is_blk8x8;
	int		is_blk16x8;
	int		is_blk8x16;

	int		pe1_start_y;
	int		pe3_start_y; 

	int		dist_y_pe01;		//to judge whether the two point is located in the same integer line, can be 0, 1
	int		dist_y_pe03;		//to judge its integer distance, it can be 0, 1, 2, to determine how to ge the source 

	int		dist_x_pe10;		//to determine how to get the source
	int		dist_x_pe12;

	int		src_x_offset;
	int		src_y_offset;
	
	uint8	src_pix0;
	uint8	src_pix1;
	uint8	src_pix2;
	uint8	src_pix3;

	int		fst_word_invalid;

	uint8	pe0_src_col_cnt = 0;
	uint8	pe0_src_row_cnt = 0;
	uint8	pe1_src_col_cnt = 0;
	uint8	pe1_src_row_cnt = 0;
	uint8	pe2_src_col_cnt = 0;
	uint8	pe2_src_row_cnt = 0;
	uint8	pe3_src_col_cnt = 0;
	uint8	pe3_src_row_cnt = 0;

	int		frame_width  = g_mb_num_x * MB_SIZE;
	int		frame_height = g_mb_num_y * MB_SIZE;

	PEReset ();


	g_is_sp = is_sp;

	if ((mb_x == 1) && (mb_y == 0) && (blk8_id == 2) && (blk_width == 8) && (blk_height == 8) && !is_sp && (step_len == 1))
		g_print_pix = 1;
	else
		g_print_pix = 0;
	if(!is_sp)
		printf("");

	pe0_en = 1;
	pe1_en = is_sp ? 0 : 1;
	pe2_en = is_sp ? 0 : 1;
	pe3_en = is_sp ? 0 : 1;

	is_blk16x16 = ((blk_width == 16) && (blk_height == 16)) ? 1 : 0;
	is_blk8x8   = ((blk_width ==  8) && (blk_height ==  8)) ? 1 : 0;
	is_blk16x8  = ((blk_width == 16) && (blk_height ==  8)) ? 1 : 0;
	is_blk8x16  = ((blk_width ==  8) && (blk_height == 16)) ? 1 : 0;

	/*x and y absolute start address in frame*/
	x_ref = mb_x * MB_SIZE * 4 + (blk8_id &  1) * 8 * 4 + cent_mv_ptr->x;		// 1/4pel position
//	y_ref = mb_y * MB_SIZE * 4 + (blk8_id >> 1) * 8 * 4 + cent_mv_ptr->y;
	y_ref = (blk8_id >> 1) * 8 * 4 + cent_mv_ptr->y;   //relative address in search window

	//for single point, same with diamond with step_len 0// Move to left top 
	x_ref = is_sp ? x_ref : x_ref - step_len ;					
	y_ref = is_sp ? y_ref : y_ref - step_len;

	src_x_offset = (blk8_id &  1) * 2;							//word position
	src_y_offset = (blk8_id >> 1) * 8;
	
	byte_oft_pe0 = ((x_ref+step_len) >> 2) & 0x3;				//byte position offset
	byte_oft_pe1 = (x_ref >> 2) & 0x3;
	byte_oft_pe2 = ((x_ref+2*step_len) >> 2) & 0x3;
	byte_oft_pe3 = byte_oft_pe0;

	dx_pe0 = (x_ref+step_len) & 0x3;
	dx_pe1 = x_ref & 0x3;
	dx_pe2 = (x_ref+2*step_len) & 0x3;
	dx_pe3 = dx_pe0;

	dy_pe0 = y_ref & 0x3;
	dy_pe1 = (y_ref + step_len) & 0x3;
	dy_pe2 = dy_pe1;
	dy_pe3 = (y_ref + 2*step_len) & 0x3;
	
	dist_y_pe01 = ((y_ref & 0x3) + step_len) >> 2;				//0 or 1, 0 - no source reading needed, 1 need to read a new pixel of left top
	dist_y_pe03 = ((y_ref & 0x3) + step_len*2) >> 2;

//	dist_x_pe10 = ((x_ref & 0x3) + step_len) >> 2;
//	dist_x_pe12 = ((x_ref & 0x3) + step_len*2) >> 2;
	dist_x_pe10 = ((x_ref & 0xf) + step_len) >> 4;
	dist_x_pe12 = ((x_ref & 0xf) + step_len*2) >> 4;	

	//**** intergel position SAD ******//
	/*determine the width and height to be read for reference block*/
	add_width = ((x_ref & 0xf) + step_len*2 ) >= 16 ? 2 : 1;
	ref_width = blk_width/4 + add_width;

	add_line  = ((y_ref & 0x3) + step_len*2 + 4) >> 2;
	line_num  = blk_height + add_line;
	
	for (j = 0; j < line_num; j++)
	{
		for (i = 0; i < ref_width; i++)
		{
			if ((j == 16) && (i == 0))
				printf ("");

			int_x_ref = (x_ref>>4) + i;			//word position
			int_y_ref = (y_ref>>2) + j;

			RefAddrGen (int_x_ref, int_y_ref, mb_y, &x_ref_wind, &y_ref_wind, frame_width, frame_height);

			ref_addr  = y_ref_wind*WIND_W + x_ref_wind;
			ref_rdata = mea_window_buf[ref_addr];

			ref_padding = PixPadding (ref_rdata, int_x_ref, frame_width, frame_height);

			ref_word = ref_padding;
			
			/*control signal for pe0*/
			pe0_hor_eff = 1;
			fst_word_invalid = (((x_ref & 0xf) + step_len) >= 16);
			if ((i == 0) || (pe0_src_col_cnt >= blk_width/4) || (fst_word_invalid && (i == 1)) )
			{
				pe0_hor_eff = 0;
			}			

			pe0_ver_eff = 0;
			if ((j > 0) && (j < blk_height+1))
			{
				pe0_ver_eff = 1;
			}

			/*control signal for pe1*/
			pe1_hor_eff = 0;
			if ((i >= 1) && (i < blk_width/4+1))
				pe1_hor_eff = 1;

			pe1_ver_eff = 0;
			//pe1_start_y = (((y_ref & 0x3) + step_len) >> 1) ? 1 : 0;
			pe1_start_y = ((y_ref & 0x3) + step_len) >> 2;
			if ((j > pe1_start_y) && (j < pe1_start_y + blk_height + 1))
			{
				pe1_ver_eff = 1;
			}

			/*control signal for pe2*/
			pe2_hor_eff = 1;
			fst_word_invalid = (((x_ref & 0xf) + step_len*2) >= 16);
			if ((i == 0) || (pe2_src_col_cnt >= blk_width/4) || (fst_word_invalid && (i == 1)))
			{
				pe2_hor_eff = 0;
			}

			pe2_ver_eff = pe1_ver_eff;

			/*control signal for pe3*/
			pe3_hor_eff = pe0_hor_eff;
			pe3_start_y = ((y_ref & 0x3) + step_len*2) >> 2;
			pe3_ver_eff = ((j > pe3_start_y) && (j < pe3_start_y+blk_height+1)) ? 1 : 0;

			/*which pe is enable*/
			pe0_cal = pe0_hor_eff && pe0_ver_eff;
			pe1_cal = pe1_hor_eff && pe1_ver_eff;
			pe2_cal = pe2_hor_eff && pe2_ver_eff;
			pe3_cal = pe3_hor_eff && pe3_ver_eff;

			pe0_active = pe0_cal && pe0_en;
			pe1_active = pe1_cal && pe1_en;
			pe2_active = pe2_cal && pe2_en;
			pe3_active = pe3_cal && pe3_en;
			if(pe0_active)
				printf("");

			/*source buffer access signal*/
			src_buf_rd = (dist_y_pe01 >= 1) ? pe0_cal : pe1_cal;// == 1
			if (src_buf_rd)
			{
				src_buf_addr = (src_row_cnt + src_y_offset) * 4 + src_col_cnt + src_x_offset;
				src_rdata    = mea_src_buf0[src_buf_addr];
			}
			else
			{
				src_rdata = 0;
			}

			/*generating source pixel for four pe*///* now we need 4 rows of source registers/
			/*source data for pe0*/
			if (dist_y_pe01 == 0)
			{
				if (dist_x_pe10 == 0)
					src_pe0 = src_rdata;
				else
					src_pe0 = (pe0_src_col_cnt == 0) ? src_word_10 : 
							  (pe0_src_col_cnt == 1) ? src_word_11 : 
							  (pe0_src_col_cnt == 2) ? src_word_12 :  src_word_13;
			}
			else
			{		
				src_pe0 = src_rdata;
			}

			/*source data for pe1*/
			if (dist_y_pe01 == 0)
			{
				src_pe1 = src_rdata;
			}
			else
			{
				if(dist_y_pe01 == 1)
				{
					src_pe1 = (pe1_src_col_cnt == 0) ? src_word_00 : 
							  (pe1_src_col_cnt == 1) ? src_word_01 : 
							  (pe1_src_col_cnt == 2) ? src_word_02 :  src_word_03;	
				}
				else
				{
					src_pe1 = (pe1_src_col_cnt == 0) ? src_word_20 : 
							  (pe1_src_col_cnt == 1) ? src_word_21 : 
							  (pe1_src_col_cnt == 2) ? src_word_22 :  src_word_23;
				}
			}
			
			/*source data for pe2*/
			if (dist_y_pe01 == 0)
			{
				if (dist_x_pe12 == 0)
				{
					src_pe2 = src_rdata;
				}
				else
				{
					src_pe2 = (pe2_src_col_cnt == 0) ? src_word_10 : 
							  (pe2_src_col_cnt == 1) ? src_word_11 : 
							  (pe2_src_col_cnt == 2) ? src_word_12 :  src_word_13;
				}
			}
			else
			{
				if(dist_y_pe01 == 1)
				{
					src_pe2 = (pe2_src_col_cnt == 0) ? src_word_00 : 
							  (pe2_src_col_cnt == 1) ? src_word_01 : 
							  (pe2_src_col_cnt == 2) ? src_word_02 :  src_word_03;	
				}
				else
				{
					src_pe2 = (pe2_src_col_cnt == 0) ? src_word_20 : 
							  (pe2_src_col_cnt == 1) ? src_word_21 : 
							  (pe2_src_col_cnt == 2) ? src_word_22 :  src_word_23;
				}

			}

			/*source data for pe3*/
			if (dist_y_pe03 == 0)
			{
				src_pe3 = src_pe0;
			}
			else if (dist_y_pe03 == 1)
			{
				src_pe3 = (pe3_src_col_cnt == 0) ? src_word_00 : 
						  (pe3_src_col_cnt == 1) ? src_word_01 : 
						  (pe3_src_col_cnt == 2) ? src_word_02 :  src_word_03;
			}
			else if(dist_y_pe03 == 2)
			{
				src_pe3 = (pe3_src_col_cnt == 0) ? src_word_20 : 
						  (pe3_src_col_cnt == 1) ? src_word_21 : 
						  (pe3_src_col_cnt == 2) ? src_word_22 :  src_word_23;
			}
			else
			{
				src_pe3 = (pe3_src_col_cnt == 0) ? src_word_10 : 
						 (pe3_src_col_cnt == 1) ? src_word_11 : 
						(pe3_src_col_cnt == 2) ? src_word_12 :  src_word_13;
			}

			ref_bot1 = ref_word;
			ref_bot0 = ref_word_d1;			
			
			ref_top0 = (blk_width == 16) ? ((ref_width == 6) ? ref_word_d7 : ref_word_d6) : 
										   ((ref_width == 4) ? ref_word_d5 : ref_word_d4);
			ref_top1 = (blk_width == 16) ? ((ref_width == 6) ? ref_word_d6 : ref_word_d5) :
									   ((ref_width == 4) ? ref_word_d4 : ref_word_d3);
							
			/*enable each pe*/

										   /*first compute mean for MBY, second compute sum of abosolute mean difference*/
			if (src_buf_rd)
			{
				src_pix0 = (src_rdata >>  0) & 0xff;
				src_pix1 = (src_rdata >>  8) & 0xff;
				src_pix2 = (src_rdata >> 16) & 0xff;
				src_pix3 = (src_rdata >> 24) & 0xff;
				if (!g_mean_cal_done)
				{
					g_mb_mean_y += src_pix0 + src_pix1 + src_pix2 + src_pix3;
				}

				if (g_mean_cal_done && !g_dev_cal_done)
				{
					g_mb_dev += ABS((int)(src_pix0-g_mb_mean_y)) + ABS((int)(src_pix1-g_mb_mean_y)) + 
								ABS((int)(src_pix2-g_mb_mean_y)) + ABS((int)(src_pix3-g_mb_mean_y));
				}
			}

			/*pe0 instantiation*/
			ProcessElement (
							ref_top0,
							ref_top1,
							ref_bot0,
							ref_bot1,
							src_pe0,
							byte_oft_pe0,
							dx_pe0,			
							dy_pe0,
							pe0_src_col_cnt,
							pe0_src_row_cnt,
							0,
							pe0_active
							);

			/*pe1 instantiation*/
			ProcessElement (
							ref_top0,
							ref_top1,
							ref_bot0,
							ref_bot1,
							src_pe1,
							byte_oft_pe1,
							dx_pe1,			
							dy_pe1,
							pe1_src_col_cnt,
							pe1_src_row_cnt,
							1,
							pe1_active
							);

			/*pe2 instantiation*/
			ProcessElement (
							ref_top0,
							ref_top1,
							ref_bot0,
							ref_bot1,
							src_pe2,
							byte_oft_pe2,
							dx_pe2,			
							dy_pe2,
							pe2_src_col_cnt,
							pe2_src_row_cnt,
							2,
							pe2_active
							);

			/*pe3 instantiation*/
			ProcessElement (
							ref_top0,
							ref_top1,
							ref_bot0,
							ref_bot1,
							src_pe3,
							byte_oft_pe3,
							dx_pe3,	
							dy_pe3,
							pe3_src_col_cnt,
							pe3_src_row_cnt,
							3,
							pe3_active
							);
			
			/*update src register*/
			if (src_buf_rd)
			{
				if (src_col_cnt == 0)
					src_word_10 = src_rdata;
				else if (src_col_cnt == 1)
					src_word_11 = src_rdata;			
				else if (src_col_cnt == 2)
					src_word_12 = src_rdata;			
				else if (src_col_cnt == 3)
					src_word_13 = src_rdata;
			}	
			
			/*update reference register*/
			ref_word_d7 = ref_word_d6;
			ref_word_d6 = ref_word_d5;
			ref_word_d5 = ref_word_d4;
			ref_word_d4 = ref_word_d3;
			ref_word_d3 = ref_word_d2;
			ref_word_d2 = ref_word_d1;
			ref_word_d1 = ref_word;			

			if (src_buf_rd)
				src_col_cnt++;
			
			pe0_src_col_cnt = pe0_hor_eff ? pe0_src_col_cnt+1 : pe0_src_col_cnt;
			pe1_src_col_cnt = pe1_hor_eff ? pe1_src_col_cnt+1 : pe1_src_col_cnt;
			pe2_src_col_cnt = pe2_hor_eff ? pe2_src_col_cnt+1 : pe2_src_col_cnt;
			pe3_src_col_cnt = pe3_hor_eff ? pe3_src_col_cnt+1 : pe3_src_col_cnt;
		}

		if (pe0_ver_eff)
			src_row_cnt++;

		src_col_cnt = 0;
		pe0_src_col_cnt = 0;
		pe1_src_col_cnt = 0;
		pe2_src_col_cnt = 0;
		pe3_src_col_cnt = 0;

		/*source pixel register array update*/
		src_00_temp = src_word_00;
		src_01_temp = src_word_01;
		src_02_temp = src_word_02;
		src_03_temp = src_word_03;

		src_word_00 = src_word_10;
		src_word_01 = src_word_11;
		src_word_02 = src_word_12;
		src_word_03 = src_word_13;
			
		src_word_10 = src_word_30;
		src_word_11 = src_word_31;
		src_word_12 = src_word_32;
		src_word_13 = src_word_33;

		src_word_30 = src_word_20;
		src_word_31 = src_word_21;
		src_word_32 = src_word_22;
		src_word_33 = src_word_23;

		src_word_20 = src_00_temp;
		src_word_21 = src_01_temp;
		src_word_22 = src_02_temp;
		src_word_23 = src_03_temp;

		pe0_src_row_cnt = pe0_ver_eff ? pe0_src_row_cnt+1 : pe0_src_row_cnt;
		pe1_src_row_cnt = pe1_ver_eff ? pe1_src_row_cnt+1 : pe1_src_row_cnt;
		pe2_src_row_cnt = pe2_ver_eff ? pe2_src_row_cnt+1 : pe2_src_row_cnt;
		pe3_src_row_cnt = pe3_ver_eff ? pe3_src_row_cnt+1 : pe3_src_row_cnt;
	}
	
	if (is_blk16x16)
	{
		g_sad_pe0 = g_sad_blk0_pe0 + g_sad_blk1_pe0 + g_sad_blk2_pe0 + g_sad_blk3_pe0;
		g_sad_pe1 = g_sad_blk0_pe1 + g_sad_blk1_pe1 + g_sad_blk2_pe1 + g_sad_blk3_pe1;
		g_sad_pe2 = g_sad_blk0_pe2 + g_sad_blk1_pe2 + g_sad_blk2_pe2 + g_sad_blk3_pe2;
		g_sad_pe3 = g_sad_blk0_pe3 + g_sad_blk1_pe3 + g_sad_blk2_pe3 + g_sad_blk3_pe3;
	}
	else if (is_blk16x8)
	{
		g_sad_pe0 = g_sad_blk0_pe0 + g_sad_blk1_pe0;
		g_sad_pe1 = g_sad_blk0_pe1 + g_sad_blk1_pe1;
		g_sad_pe2 = g_sad_blk0_pe2 + g_sad_blk1_pe2;
		g_sad_pe3 = g_sad_blk0_pe3 + g_sad_blk1_pe3;
	}
	else if (is_blk8x16)
	{
		g_sad_pe0 = g_sad_blk0_pe0 + g_sad_blk2_pe0;
		g_sad_pe1 = g_sad_blk0_pe1 + g_sad_blk2_pe1;
		g_sad_pe2 = g_sad_blk0_pe2 + g_sad_blk2_pe2;
		g_sad_pe3 = g_sad_blk0_pe3 + g_sad_blk2_pe3;
	}
	else
	{
		g_sad_pe0 = g_sad_blk0_pe0;
		g_sad_pe1 = g_sad_blk0_pe1;
		g_sad_pe2 = g_sad_blk0_pe2;
		g_sad_pe3 = g_sad_blk0_pe3;
	}
	
	g_sad_pe1 = is_sp ? 0xffffffff : g_sad_pe1;
	g_sad_pe2 = is_sp ? 0xffffffff : g_sad_pe2;
	g_sad_pe3 = is_sp ? 0xffffffff : g_sad_pe3;
	
//#endif
	/*PE will be started for block16x16 at least for two times, first compute the mean, and the second compute derivation*/
	if (g_mean_cal_done && !g_dev_cal_done)
	{
		g_dev_cal_done = 1;
	}
	
	if (!g_mean_cal_done)
	{
		g_mb_mean_y = (g_mb_mean_y+128) >> 8;
		g_mean_cal_done = 1;
	}
}


