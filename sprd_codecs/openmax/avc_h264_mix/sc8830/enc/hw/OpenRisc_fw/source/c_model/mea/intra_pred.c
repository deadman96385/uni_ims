/*intra_pred.c*/
#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"
#include "tv_mea.h"
#include "common_global.h"
#include "h264enc_mode.h"
#include "h264enc_global.h"


//int		g_bs_imode16[3] = {1, 3, 3};
int		g_bs_imode16[3] = {1, 3, 3};
int		g_bs_imode8[3] = {1, 3, 3};

int HadamardSAD4x4 (int* diff);
int HadamardSAD16x16 (int (*diff16)[16]);


void ReadTopIntraRefPixel (int mb_x, int mb_y, int is_y)
{
	int			i;
	int			offset;
	uint32	*	frame_ptr;
	uint32	*	frame_ptr_v;
	uint32	*	line_buf_ptr;
//	uint16		tmp_reg;
//	uint32		rdata;
//	uint16		two_u_pix;
	int			row_num;

	row_num = is_y ? MB_SIZE : 8;
	
	offset = mb_y * g_mb_num_x * 4 * row_num + mb_x * 4;//g_mb_num_x是从global寄存器获得的宽方向的宏块的个数
	
	if (is_y)
	{
		frame_ptr = (uint32 *)(g_enc_image_ptr->ipred_top_line_buffer + mb_x * MB_SIZE);
	}
	else
	{
		// Source Frame
		//frame_ptr = g_src_frame[1] + offset - g_mb_num_x * 4;
		// Reconstructed Frame
		frame_ptr = (uint32 *)(g_enc_image_ptr->ipred_top_line_buffer + g_mb_num_x * MB_SIZE + mb_x * MB_CHROMA_SIZE);
		frame_ptr_v = (uint32 *)(g_enc_image_ptr->ipred_top_line_buffer + g_mb_num_x * (MB_SIZE+MB_CHROMA_SIZE) + mb_x * MB_CHROMA_SIZE);
	}
// 	frame_ptr = (is_y ? g_src_frame[0] : g_src_frame[1]) + offset - g_mb_num_x * 4;//-g_mb_num_x * 4代表的是当前宏块的上方的一行像素
	
	line_buf_ptr = mea_out_buf0 + (is_y ? 0 : 4);	//取上方的16个像素，不包括左上的点？？

	/*for (i = 0; i < 4; i++)
	{
		rdata = frame_ptr[i];		
		
		if (is_y)
		{
			line_buf_ptr[i] = rdata;
		}
		else
		{
			two_u_pix = ((rdata >> 0) & 0xff) | (rdata >> 8) & 0xff00;

			if ((i&1) == 0)
			{
				tmp_reg =  two_u_pix;
			}			
			
			if ((i & 1) == 1)
			{
				line_buf_ptr [i>>1] = (two_u_pix << 16) | tmp_reg;
			}
		}		
	}*/

	if (is_y)
	{
		for (i = 0; i < 4; i++)
			line_buf_ptr[i] = frame_ptr[i];
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			line_buf_ptr[i] = frame_ptr[i];
			line_buf_ptr[i+2] = frame_ptr_v[i];
		}
	}
}

void IntraModeDecision (
						MODE_DECISION_T *	mode_dcs_ptr, 
						int					top_avail, 
						int					left_avail, 
						int					is_y,
						int				*	min_cost_ptr,
						int				*	mode_ptr,
						int					mb_x,
						int					mb_y,
						IEA_SAD			*	iea_sad
						)
{
	int			i;
	int			j;
	int			dir;
	int			word_num;
	uint32		four_pix;
	uint8		pix0;
	uint8		pix1;
	uint8		pix2;
	uint8		pix3;
	uint16		sum;
	uint16		dc_pred0 = 0;        //used for Y or block0 of U
	uint16		dc_pred1 = 0;		//for block1 of U
	uint16		dc_pred2 = 0;		//for block2 of U
	uint16		dc_pred3 = 0;		//for block3 of U
	int			is_add_to_pred0;
	int			is_add_to_pred1;
	int			is_add_to_pred2;
	int			is_add_to_pred3;
	int			offset;
	int			avail_num;
	int			rnd;
	int			shift;
	int			blk_width;
	int			blk_height;
	int			sad_dc  = 0;
	int			sad_hor = 0;
	int			sad_ver = 0;
#ifdef INTRA_CHROME_UV
	uint16		dc_pred0_v = 0;     //for block0 of V
	uint16		dc_pred1_v = 0;		//for block1 of V
	uint16		dc_pred2_v = 0;		//for block2 of V
	uint16		dc_pred3_v = 0;		//for block3 of V
	int			sad_dc_v  = 0;
	int			sad_hor_v = 0;
	int			sad_ver_v = 0;
	uint8		hor_ref_v;
#endif
#ifdef SATD_COST
	int			diff4x4_dc[16];
	int			diff4x4_hor[16];
	int			diff4x4_ver[16];
	int 		diff16x16_dc[16][16];
	int			diff16x16_hor[16][16];
	int			diff16x16_ver[16][16];
	int			satd_dc  = 0;
	int			satd_hor = 0;
	int			satd_ver = 0;
#endif

	uint8		ver_ref_p0;
	uint8		ver_ref_p1;
	uint8		ver_ref_p2;
	uint8		ver_ref_p3;

	uint8		hor_ref;

	uint8		dc_pred;
	int			lambda;

	uint32  *	src_blk_ptr;
	uint32  *	left_ref_ptr;
	uint32  *	top_ref_ptr;
	uint32	*	line_buf_ptr;
	int			mode_cost;
	int			min_cost;
	int			min_mode;
	uint32	*	intra_ref_base_ptr;	
	int		    min4x4_cost = 0;
	
	if (g_mode_dcs_ptr->top_avail)
	{
		ReadTopIntraRefPixel (mb_x, mb_y, is_y);//取上方的参考值像素
	}	

	offset = INTRA_REF_BASE_ADDR + ((mb_x&1) ? 0 : 8);	// offset=0 or 8 stores two MB's left pixels, ping pong mode
	/*if (is_y)
	{
		intra_ref_base_ptr = (uint32 *)g_luma_left;
	}
	else
	{
		intra_ref_base_ptr = mea_window_buf + offset;
	}*/

	// 4x4's and 16x16's left pixels are all source MB, because reconstructed MB still in pipe
	intra_ref_base_ptr = mea_window_buf + offset;
	
	/*************************************************************************
				4x4 Intra Prediction for luma samples
	**************************************************************************/
	if(is_y)// if is luma component, need to perform both 4x4 and 16x16 intra mode decision and then compare their cost
	{
		uint8  blkIdxInMB;
		uint8  blk_x, blk_y;
		uint8  *temp_pix = (uint8 *)mea_src_buf0;
		int	   top_avail_4x4; 
		int	   left_avail_4x4;
		uint32 left_pix, top_pix;
		int	   top4x4_mode, left4x4_mode, pred4x4_mode;

		for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)//16个块都可以并行一起做
		{
			dc_pred0 = 0;
			sad_dc  = 0;
			sad_hor = 0;
			sad_ver = 0;

			blk_x = (blkIdxInMB % 4);
			blk_y = (blkIdxInMB >> 2);

			top_avail_4x4 = (blk_y==0) ? top_avail : 1;
			left_avail_4x4 = (blk_x==0) ? left_avail : 1;

			for (dir = 0; dir < 2; dir++)
			{

				if (dir == 0)
				{
					if (blk_y == 0)
					{
						line_buf_ptr = mea_out_buf0 + blk_x;
						top_pix = line_buf_ptr[0];		// derek
					}
					else
					{
						line_buf_ptr = mea_src_buf0 + blk_y*16 - 4 + blk_x;
						top_pix = line_buf_ptr[0];
					}
				}
				else if (dir == 1)
				{
					if (blk_x == 0)
					{
						line_buf_ptr = intra_ref_base_ptr + blk_y;
						left_pix = line_buf_ptr[0];		// derek
					}
					else
					{
						pix0 = temp_pix[blk_y*64 + blk_x*4 - 1];
						pix1 = temp_pix[blk_y*64 + blk_x*4 + 15];
						pix2 = temp_pix[blk_y*64 + blk_x*4 + 31];
						pix3 = temp_pix[blk_y*64 + blk_x*4 + 47];

						left_pix = (pix3 << 24) | (pix2 << 16) | (pix1 << 8) | (pix0 << 0);

						line_buf_ptr = &(left_pix);
					}
				}
				
				four_pix = line_buf_ptr[0];			
				pix0 = (four_pix >>  0) & 0xff;
				pix1 = (four_pix >>  8) & 0xff;
				pix2 = (four_pix >> 16) & 0xff;
				pix3 = (four_pix >> 24) & 0xff;			
				sum  = pix0 + pix1 + pix2 + pix3;

				/*dc_pred0 update*/		
				is_add_to_pred0 = ((dir == 0) && top_avail_4x4) || ((dir == 1) && left_avail_4x4);
				
				dc_pred0 = is_add_to_pred0 ? dc_pred0+sum : dc_pred0;
			}

			/*for Y 4x4 block */
			avail_num = top_avail_4x4 + left_avail_4x4;
			rnd = (avail_num == 2) ? 4 :
				  (avail_num == 1) ? 2 : 0;

			shift = (avail_num == 2) ? 3 :
					(avail_num == 1) ? 2 : 0;

			dc_pred0 = (avail_num == 0) ? 128 : (dc_pred0 + rnd) >> shift;

			/*************************************************************************
						compute sad of DC, horizontal and vertical for Y 4x4
			**************************************************************************/
			blk_height = 4; 

			src_blk_ptr  = mea_src_buf0 + blk_y*16 + blk_x;

			for (j = 0; j < 4; j++)
			{
				int index;

				/*get horizontal reference*/
				index    = j & 0x3;
				hor_ref  = (index == 0) ? (left_pix >>  0) & 0xff :
						   (index == 1) ? (left_pix >>  8) & 0xff :
						   (index == 2) ? (left_pix >> 16) & 0xff : (left_pix >> 24) & 0xff;
				

				/*get vertical reference*/
				ver_ref_p0 = (top_pix >>  0) & 0xff;
				ver_ref_p1 = (top_pix >>  8) & 0xff;
				ver_ref_p2 = (top_pix >> 16) & 0xff;
				ver_ref_p3 = (top_pix >> 24) & 0xff;	

				/*get dc prediction*/
				dc_pred = (uint8)dc_pred0;
						
				four_pix = src_blk_ptr[0];
				pix0 = (four_pix >>  0) & 0xff;
				pix1 = (four_pix >>  8) & 0xff;
				pix2 = (four_pix >> 16) & 0xff;
				pix3 = (four_pix >> 24) & 0xff;

				sad_dc  += ABS(pix0 - dc_pred) + ABS(pix1 - dc_pred) + ABS(pix2 - dc_pred) + ABS(pix3 - dc_pred);
				sad_hor += ABS(pix0 - hor_ref) + ABS(pix1 - hor_ref) + ABS(pix2 - hor_ref) + ABS(pix3 - hor_ref);
				sad_ver += ABS(pix0 - ver_ref_p0) + ABS(pix1 - ver_ref_p1) + ABS(pix2 - ver_ref_p2) + ABS(pix3 - ver_ref_p3);

#ifdef SATD_COST
				diff4x4_dc[j*4+0] = (pix0 - dc_pred);
				diff4x4_dc[j*4+1] = (pix1 - dc_pred);
				diff4x4_dc[j*4+2] = (pix2 - dc_pred);
				diff4x4_dc[j*4+3] = (pix3 - dc_pred);

				diff4x4_hor[j*4+0] = (pix0 - hor_ref);
				diff4x4_hor[j*4+1] = (pix1 - hor_ref);
				diff4x4_hor[j*4+2] = (pix2 - hor_ref);
				diff4x4_hor[j*4+3] = (pix3 - hor_ref);

				diff4x4_ver[j*4+0] = (pix0 - ver_ref_p0);
				diff4x4_ver[j*4+1] = (pix1 - ver_ref_p1);
				diff4x4_ver[j*4+2] = (pix2 - ver_ref_p2);
				diff4x4_ver[j*4+3] = (pix3 - ver_ref_p3);
#endif
				src_blk_ptr += 4;
			}

#ifdef SATD_COST
			satd_dc = HadamardSAD4x4(diff4x4_dc);
			satd_hor = HadamardSAD4x4(diff4x4_hor);
			satd_ver = HadamardSAD4x4(diff4x4_ver);
#endif
			/*************************************************************************
						compare sad of three mode, get the best mode
			**************************************************************************/
			lambda = g_mode_dcs_ptr->lambda;
			
			if (top_avail_4x4&&left_avail_4x4)
			{
				top4x4_mode  = (blk_y==0) ? mode_dcs_ptr->top_blk_4x4_mode[blk_x]  : mode_dcs_ptr->intra4x4_pred_mode[blkIdxInMB-4];
				left4x4_mode = (blk_x==0) ? mode_dcs_ptr->left_blk_4x4_mode[blk_y] : mode_dcs_ptr->intra4x4_pred_mode[blkIdxInMB-1];
				pred4x4_mode = (top4x4_mode<left4x4_mode) ? top4x4_mode : left4x4_mode;
			}
			else
			{
				pred4x4_mode = 2;
			}

			if (pred4x4_mode == -1)
			{
				pred4x4_mode = 2;
			}

#ifndef SATD_COST
			{
				mode_cost = lambda * ((pred4x4_mode==INTRA_L_DC)?1:4);
				sad_dc    = sad_dc + mode_cost;

				mode_cost = lambda * ((pred4x4_mode==INTRA_L_HOR)?1:4);
				sad_hor   = sad_hor + mode_cost;

				mode_cost = lambda * ((pred4x4_mode==INTRA_L_VER)?1:4);
				sad_ver   = sad_ver + mode_cost;
			}
#else
			{
				mode_cost = lambda * ((pred4x4_mode==INTRA_L_DC)?1:4);
				sad_dc    = satd_dc + mode_cost;

				mode_cost = lambda * ((pred4x4_mode==INTRA_L_HOR)?1:4);
				sad_hor   = satd_hor + mode_cost;

				mode_cost = lambda * ((pred4x4_mode==INTRA_L_VER)?1:4);
				sad_ver   = satd_ver + mode_cost;
			}
#endif
			
			sad_ver = top_avail_4x4  ? sad_ver : 0xffff;
			sad_hor = left_avail_4x4 ? sad_hor : 0xffff;

			// For IEA Verification
			iea_sad->sad_y_4x4[blkIdxInMB][0] = sad_dc;
			iea_sad->sad_y_4x4[blkIdxInMB][1] = sad_hor;
			iea_sad->sad_y_4x4[blkIdxInMB][2] = sad_ver;

			min_cost = sad_dc;
			min_mode = INTRA_L_DC;

			if (sad_hor < min_cost)
			{
				min_cost = sad_hor;
				min_mode = INTRA_L_HOR;
			}

			if (sad_ver < min_cost)
			{
				min_cost = sad_ver;
				min_mode = INTRA_L_VER;
			}

			mode_dcs_ptr->intra4x4_pred_mode[blkIdxInMB] = min_mode;
			min4x4_cost += min_cost;
			
		}//for (blkIdxInMB = 0; blkIdxInMB < 16; blkIdxInMB++)

		// For IEA Verification
		iea_sad->sad_y_4x4_min = min4x4_cost;
	}//if(is_y)

	dc_pred0 = 0;
	sad_dc  = 0;
	sad_hor = 0;
	sad_ver = 0;

	/*************************************************************************
				compute DC pred for Y, or 4 dc pred value for U
	**************************************************************************/
	word_num = is_y ? 4 : 2;
	for (dir = 0; dir < 2; dir++)
	{
		line_buf_ptr = ((dir == 1) ? intra_ref_base_ptr : mea_out_buf0) + (is_y ? 0 : 4);//dir=1代表什么啊？表明的是左方的像素相加。
		
		for (i = 0; i< word_num; i++)
		{
			four_pix = line_buf_ptr[i];			
			pix0 = (four_pix >>  0) & 0xff;
			pix1 = (four_pix >>  8) & 0xff;
			pix2 = (four_pix >> 16) & 0xff;
			pix3 = (four_pix >> 24) & 0xff;			
			sum  = pix0 + pix1 + pix2 + pix3;

			/*dc_pred0 update*/		
			is_add_to_pred0 = is_y ? ((dir == 0) && top_avail) || ((dir == 1) && left_avail) :
									 (((dir == 0) && top_avail) || ((dir == 1) && left_avail)) && (i == 0);
			
			dc_pred0 = is_add_to_pred0 ? dc_pred0+sum : dc_pred0;

			/*dc_pred1 update*/
			is_add_to_pred1 = ((i == 1) && (dir == 0) && top_avail) || (!top_avail && left_avail && (i == 0) && (dir == 1));

			dc_pred1 = is_add_to_pred1 ? dc_pred1+sum : dc_pred1;

			/*dc_pred2 update*/
			is_add_to_pred2 = ((i == 1) && (dir == 1) && left_avail) || (!left_avail && top_avail && (i == 0) && (dir == 0));

			dc_pred2 = is_add_to_pred2 ? dc_pred2+sum : dc_pred2;

			/*dc_pred2 update*/
			is_add_to_pred3 = ((i == 1) && (dir == 0) && top_avail) || ((i == 1) && (dir == 1) && left_avail);

			dc_pred3 = is_add_to_pred3 ? dc_pred3+sum : dc_pred3;

#ifdef INTRA_CHROME_UV
			if(is_y==0)
			{
				// Calculate V DC_PRED
				four_pix = line_buf_ptr[i+2];			
				pix0 = (four_pix >>  0) & 0xff;
				pix1 = (four_pix >>  8) & 0xff;
				pix2 = (four_pix >> 16) & 0xff;
				pix3 = (four_pix >> 24) & 0xff;			
				sum  = pix0 + pix1 + pix2 + pix3;
					
				dc_pred0_v = is_add_to_pred0 ? dc_pred0_v+sum : dc_pred0_v;
				dc_pred1_v = is_add_to_pred1 ? dc_pred1_v+sum : dc_pred1_v;
				dc_pred2_v = is_add_to_pred2 ? dc_pred2_v+sum : dc_pred2_v;
				dc_pred3_v = is_add_to_pred3 ? dc_pred3_v+sum : dc_pred3_v;
			}
#endif
		}
	}

	/*for Y and dc_pred for chroma block0 and block3*/
	avail_num = top_avail + left_avail;
	rnd = (avail_num == 2) ? (is_y ? 16 : 4) :
		  (avail_num == 1) ? (is_y ? 8 : 2) : 0;

	shift = (avail_num == 2) ? (is_y ? 5 : 3) :
		    (avail_num == 1) ? (is_y ? 4 : 2) : 0;

	dc_pred0 = (avail_num == 0) ? 128 : (dc_pred0 + rnd) >> shift;
	dc_pred3 = (avail_num == 0) ? 128 : (dc_pred3 + rnd) >> shift;
#ifdef INTRA_CHROME_UV
	if(is_y==0)
	{
		dc_pred0_v = (avail_num == 0) ? 128 : (dc_pred0_v + rnd) >> shift;
		dc_pred3_v = (avail_num == 0) ? 128 : (dc_pred3_v + rnd) >> shift;
	}
#endif	

	/*for chroma block1 and block2*/
	avail_num = (top_avail || left_avail) ? 1 : 0;
	dc_pred1  = (avail_num == 0) ? 128 : (dc_pred1 + 2) >> 2;
	dc_pred2  = (avail_num == 0) ? 128 : (dc_pred2 + 2) >> 2;
#ifdef INTRA_CHROME_UV
	if(is_y==0)
	{
		dc_pred1_v = (avail_num == 0) ? 128 : (dc_pred1_v + rnd) >> shift;
		dc_pred2_v = (avail_num == 0) ? 128 : (dc_pred2_v + rnd) >> shift;
	}
#endif

	// For IEA Verification
	iea_sad->dc_pred_16 = dc_pred0;

	/*************************************************************************
				compute sad of DC, horizontal and vertical for Y or U
	**************************************************************************/
	blk_width  = is_y ? 4 : 2;
	blk_height = is_y ? 16 : 8; 

	src_blk_ptr  = is_y ? mea_src_buf0 : mea_src_buf0 + 64;
	top_ref_ptr  = mea_out_buf0 + (is_y ? 0 : 4);
	left_ref_ptr = intra_ref_base_ptr + (is_y ? 0 : 4);

	for (j = 0; j < blk_height; j++)
	{
		int index;

		/*get horizontal reference*/
		four_pix = left_avail ? left_ref_ptr[j >> 2] : 0;
		index    = j & 0x3;
		hor_ref  = (index == 0) ? (four_pix >>  0) & 0xff :
				   (index == 1) ? (four_pix >>  8) & 0xff :
				   (index == 2) ? (four_pix >> 16) & 0xff : (four_pix >> 24) & 0xff;
#ifdef INTRA_CHROME_UV
		if(is_y==0)
		{
			four_pix = left_avail ? left_ref_ptr[(j>>2)+2] : 0;	// V
			hor_ref_v = (index == 0) ? (four_pix >>  0) & 0xff :
						(index == 1) ? (four_pix >>  8) & 0xff :
						(index == 2) ? (four_pix >> 16) & 0xff : (four_pix >> 24) & 0xff;
		}
#endif

		for (i = 0; i < blk_width; i++)
		{
			/*get vertical reference*/
			four_pix   = top_avail ? top_ref_ptr[i] : 0;
			ver_ref_p0 = (four_pix >>  0) & 0xff;
			ver_ref_p1 = (four_pix >>  8) & 0xff;
			ver_ref_p2 = (four_pix >> 16) & 0xff;
			ver_ref_p3 = (four_pix >> 24) & 0xff;	

			/*get dc prediction*/
			dc_pred = is_y ? dc_pred0														: 
							(j < blk_height/2) ? ((i < blk_width/2) ? dc_pred0 : dc_pred1)	: 
							((i < blk_width/2) ? dc_pred2 : dc_pred3); 			

			four_pix = src_blk_ptr[i];
			pix0 = (four_pix >>  0) & 0xff;
			pix1 = (four_pix >>  8) & 0xff;
			pix2 = (four_pix >> 16) & 0xff;
			pix3 = (four_pix >> 24) & 0xff;

			sad_dc  += ABS(pix0 - dc_pred) + ABS(pix1 - dc_pred) + ABS(pix2 - dc_pred) + ABS(pix3 - dc_pred);
			sad_hor += ABS(pix0 - hor_ref) + ABS(pix1 - hor_ref) + ABS(pix2 - hor_ref) + ABS(pix3 - hor_ref);
			sad_ver += ABS(pix0 - ver_ref_p0) + ABS(pix1 - ver_ref_p1) + ABS(pix2 - ver_ref_p2) + ABS(pix3 - ver_ref_p3);

#ifdef INTRA_CHROME_UV
			if(is_y==0)
			{
				/*get vertical reference*/
				four_pix   = top_avail ? top_ref_ptr[i+2] : 0;	// V
				ver_ref_p0 = (four_pix >>  0) & 0xff;
				ver_ref_p1 = (four_pix >>  8) & 0xff;
				ver_ref_p2 = (four_pix >> 16) & 0xff;
				ver_ref_p3 = (four_pix >> 24) & 0xff;

				/*get dc prediction*/
				dc_pred = 	(j < blk_height/2) ? ((i < blk_width/2) ? dc_pred0_v : dc_pred1_v)	: 
							((i < blk_width/2) ? dc_pred2_v : dc_pred3_v);

				four_pix = src_blk_ptr[16+i];	// V
				pix0 = (four_pix >>  0) & 0xff;
				pix1 = (four_pix >>  8) & 0xff;
				pix2 = (four_pix >> 16) & 0xff;
				pix3 = (four_pix >> 24) & 0xff;
				
				sad_dc_v  += ABS(pix0 - dc_pred) + ABS(pix1 - dc_pred) + ABS(pix2 - dc_pred) + ABS(pix3 - dc_pred);
				sad_hor_v += ABS(pix0 - hor_ref_v) + ABS(pix1 - hor_ref_v) + ABS(pix2 - hor_ref_v) + ABS(pix3 - hor_ref_v);
				sad_ver_v += ABS(pix0 - ver_ref_p0) + ABS(pix1 - ver_ref_p1) + ABS(pix2 - ver_ref_p2) + ABS(pix3 - ver_ref_p3);
			}
#endif

#ifdef SATD_COST
			diff16x16_dc[(j/4)*4+i][(j%4)*4+0] = (pix0 - dc_pred);
			diff16x16_dc[(j/4)*4+i][(j%4)*4+1] = (pix1 - dc_pred);
			diff16x16_dc[(j/4)*4+i][(j%4)*4+2] = (pix2 - dc_pred);
			diff16x16_dc[(j/4)*4+i][(j%4)*4+3] = (pix3 - dc_pred);

			diff16x16_hor[(j/4)*4+i][(j%4)*4+0] = (pix0 - hor_ref);
			diff16x16_hor[(j/4)*4+i][(j%4)*4+1] = (pix1 - hor_ref);
			diff16x16_hor[(j/4)*4+i][(j%4)*4+2] = (pix2 - hor_ref);
			diff16x16_hor[(j/4)*4+i][(j%4)*4+3] = (pix3 - hor_ref);

			diff16x16_ver[(j/4)*4+i][(j%4)*4+0] = (pix0 - ver_ref_p0);
			diff16x16_ver[(j/4)*4+i][(j%4)*4+1] = (pix1 - ver_ref_p1);
			diff16x16_ver[(j/4)*4+i][(j%4)*4+2] = (pix2 - ver_ref_p2);
			diff16x16_ver[(j/4)*4+i][(j%4)*4+3] = (pix3 - ver_ref_p3);
#endif
		}

		src_blk_ptr += blk_width;
	}

#ifdef SATD_COST
	if (is_y)
	{
		satd_dc = HadamardSAD16x16(diff16x16_dc);
		satd_hor = HadamardSAD16x16(diff16x16_hor);
		satd_ver = HadamardSAD16x16(diff16x16_ver);
	}
#endif
	

	/*************************************************************************
				compare sad of three mode, get the best mode
	**************************************************************************/
	lambda = g_mode_dcs_ptr->lambda;

#ifdef SATD_COST
	{
		mode_cost = is_y ? lambda * g_bs_imode16[INTRA_L_DC] : lambda * g_bs_imode8[INTRA_C_DC];
		sad_dc    = satd_dc + mode_cost;
		
		mode_cost = is_y ? lambda * g_bs_imode16[INTRA_L_HOR] : lambda * g_bs_imode8[INTRA_C_HOR];
		sad_hor   = satd_hor + mode_cost;
		
		mode_cost = is_y ? lambda * g_bs_imode16[INTRA_L_VER] : lambda * g_bs_imode8[INTRA_C_VER];
		sad_ver   = satd_ver + mode_cost;
	}
#else
	{
		mode_cost = is_y ? lambda * g_bs_imode16[INTRA_L_DC] : lambda * g_bs_imode8[INTRA_C_DC];
		sad_dc    = sad_dc + mode_cost;
		
		mode_cost = is_y ? lambda * g_bs_imode16[INTRA_L_HOR] : lambda * g_bs_imode8[INTRA_C_HOR];
		sad_hor   = sad_hor + mode_cost;
		
		mode_cost = is_y ? lambda * g_bs_imode16[INTRA_L_VER] : lambda * g_bs_imode8[INTRA_C_VER];
		sad_ver   = sad_ver + mode_cost;

#ifdef INTRA_CHROME_UV
		if(is_y==0)
		{
			mode_cost = lambda * g_bs_imode8[INTRA_C_DC];
			sad_dc    += sad_dc_v + mode_cost;
			
			mode_cost = lambda * g_bs_imode8[INTRA_C_HOR];
			sad_hor   += sad_hor_v + mode_cost;
			
			mode_cost = lambda * g_bs_imode8[INTRA_C_VER];
			sad_ver   += sad_ver_v + mode_cost;
		}
#endif
	}
#endif	

	sad_ver = top_avail  ? sad_ver : 0xffff;
	sad_hor = left_avail ? sad_hor : 0xffff;

	// For IEA Verification
	if(is_y)
	{
		iea_sad->sad_y_16x16[0] = sad_dc;
		iea_sad->sad_y_16x16[1] = sad_hor;
		iea_sad->sad_y_16x16[2] = sad_ver;
	}
	else
	{
		iea_sad->sad_c_8x8[0] = sad_dc;
		iea_sad->sad_c_8x8[1] = sad_hor;
		iea_sad->sad_c_8x8[2] = sad_ver;
	}
	
	min_cost = sad_dc;
	min_mode = is_y ? INTRA_L_DC : INTRA_C_DC;

	if (sad_hor < min_cost)
	{
		min_cost = sad_hor;
		min_mode = is_y ? INTRA_L_HOR : INTRA_C_HOR;
	}

	if (sad_ver < min_cost)
	{
		min_cost = sad_ver;
		min_mode = is_y ? INTRA_L_VER : INTRA_C_VER;
	}	

	if (is_y)
	{
		FPRINTF (g_me_trace_fp, "Luma intra_mode: %d, min_cost: %d\n", min_mode, min_cost);
	}
	else
	{
		FPRINTF (g_me_trace_fp, "Chroma intra_mode: %d, min_cost: %d\n", min_mode, min_cost);
	}

//	PrintfImpDcs (sad_dc, sad_hor, sad_ver, left_avail, top_avail);

	if (is_y)
	{
// 		min4x4_cost = 65536;
		iea_sad->iea_ipred_type = 1;
		if (min4x4_cost < min_cost)
		{
			min_mode = INTRA_L_4x4;
			min_cost = min4x4_cost;
			iea_sad->iea_ipred_type = 0;
		}
	}

	*mode_ptr     = min_mode;
	*min_cost_ptr = min_cost;
}

int HadamardSAD4x4 (int* diff)
{
  int k, satd = 0;
  int m[16], d[16];

  /*===== hadamard transform =====*/
  m[ 0] = diff[ 0] + diff[12];
  m[ 1] = diff[ 1] + diff[13];
  m[ 2] = diff[ 2] + diff[14];
  m[ 3] = diff[ 3] + diff[15];
  m[ 4] = diff[ 4] + diff[ 8];
  m[ 5] = diff[ 5] + diff[ 9];
  m[ 6] = diff[ 6] + diff[10];
  m[ 7] = diff[ 7] + diff[11];
  m[ 8] = diff[ 4] - diff[ 8];
  m[ 9] = diff[ 5] - diff[ 9];
  m[10] = diff[ 6] - diff[10];
  m[11] = diff[ 7] - diff[11];
  m[12] = diff[ 0] - diff[12];
  m[13] = diff[ 1] - diff[13];
  m[14] = diff[ 2] - diff[14];
  m[15] = diff[ 3] - diff[15];

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
    satd += abs(d[k]);
  }


  return ((satd+1)>>1);
}

int HadamardSAD16x16 (int (*diff16)[16])
{
  int k,j, satd = 0;
  int temp_satd = 0;
  int m[16], d[16];
  int dcCoeff[16];
  int* diff;

  for(j=0; j<16; j++)
  {
	  diff = diff16[j];
	  temp_satd = 0;
	  
	  /*===== hadamard transform =====*/
	  m[ 0] = diff[ 0] + diff[12];
	  m[ 1] = diff[ 1] + diff[13];
	  m[ 2] = diff[ 2] + diff[14];
	  m[ 3] = diff[ 3] + diff[15];
	  m[ 4] = diff[ 4] + diff[ 8];
	  m[ 5] = diff[ 5] + diff[ 9];
	  m[ 6] = diff[ 6] + diff[10];
	  m[ 7] = diff[ 7] + diff[11];
	  m[ 8] = diff[ 4] - diff[ 8];
	  m[ 9] = diff[ 5] - diff[ 9];
	  m[10] = diff[ 6] - diff[10];
	  m[11] = diff[ 7] - diff[11];
	  m[12] = diff[ 0] - diff[12];
	  m[13] = diff[ 1] - diff[13];
	  m[14] = diff[ 2] - diff[14];
	  m[15] = diff[ 3] - diff[15];

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

	  dcCoeff[j] = (d[0]>>2);
	  //===== sum up =====
	  // Table lookup is faster than abs macro
	  for (k=1; k<16; ++k)
	  {
		temp_satd += abs(d[k]);
	  }

	  satd += ((temp_satd+1)>>1);
  }

  satd += HadamardSAD4x4(dcCoeff);
  
  return satd;
}
