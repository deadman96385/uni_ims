#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"
#include "common_global.h"
#include "buffer_global.h"

int wind_pos_x;
int wind_start_pos_x;
uint32 src_line_buf[5];

void FetchRefWindow ()
{
	int			i;
	int			j;
	int			mb_x;
	int			mb_y;
	int			burst_len;
	int			line_num;
	int			wind_pos_y;
	uint32	*	ref_ptr;
	int			wind_buf_addr;

	mb_x		= g_mea_fetch_ptr->fetch_mb_x;
	mb_y		= g_mea_fetch_ptr->fetch_mb_y;
	burst_len	= g_mea_fetch_ptr->burst_len;
	line_num	= g_mea_fetch_ptr->line_num;

	if (mb_x == 0)
	{
		wind_start_pos_x = SEARCH_RANGE_X/4;
	}

	wind_pos_y = (mb_y*MB_SIZE < SEARCH_RANGE_Y) ? (SEARCH_RANGE_Y - mb_y*MB_SIZE) : 0;

	ref_ptr = g_ref_frame[0] + g_mea_fetch_ptr->start_pos;

	for (j = 0; j < line_num; j++)
	{
		for (i = 0; i < burst_len; i++)
		{
			if ((j == 16) && (i == 0))
				printf ("");

			wind_pos_x = (wind_start_pos_x + i) & 0x1f;
			wind_buf_addr = wind_pos_y*32 + wind_pos_x;
			
			mea_window_buf[wind_buf_addr] = ref_ptr[i];
		}

		wind_pos_y++;

	//	wind_pos_y = wind_pos_y & 0x5f;

		ref_ptr += g_mb_num_x*MB_SIZE/4;
	}	

	wind_start_pos_x = (wind_start_pos_x + burst_len) & 0x1f;
}

void FetchSrcMB ()
{
	int		i;
	int		j;
	uint32	src_buf_addr;
	uint32	src_frame_addr;
	int		is_last_mb_col;
	int		is_last_mb_row;
	uint32	cur_word;
	uint32	cur_word_filt;
	uint32	pix_in_fifo;
	uint8	x0;
	uint8	x1;
	uint8	x2;
	uint8	x3;
	uint8	x0_f;
	uint8	x1_f;
	uint8	x2_f;
	uint8	x3_f;
	int		prefilt_en;
	int		thresh;

	uint16	two_pix_u;
	uint16	two_pix_v;
	uint16  pix2_reg_u;
	uint16  pix2_reg_v;

	uint32	four_pix_u;
	uint32	four_pix_v;

	int		src_addr_u;
	int		src_addr_v;

	uint32	word_reg = 0;					//for storing left column pixel for next MB's inra prediction
	uint32	word_reg_v = 0;

	int		frame_width;				//word unit

	int		ref_wind_addr;

	uint32 * intra_ref_ptr;
	
	int		mb_x = g_mea_fetch_ptr->fetch_mb_x;
	int		mb_y = g_mea_fetch_ptr->fetch_mb_y;

	prefilt_en	= g_mea_fetch_ptr->pre_filter_en;
	thresh		= g_mea_fetch_ptr->filt_thresh;
	frame_width = g_mb_num_x * MB_SIZE / 4; 

	is_last_mb_col = (mb_x == g_mb_num_x - 1) ? 1 : 0;
	is_last_mb_row = (mb_y == g_mb_num_y - 1) ? 1 : 0;

	src_frame_addr = mb_y*MB_SIZE * frame_width + mb_x*MB_SIZE/4;

	intra_ref_ptr = mea_window_buf + INTRA_REF_BASE_ADDR;

	/*fetch, pre-filter source MB Y and save the last column for next mb's intra prediction*/
	for (j = 0; j < 17; j++)
	{
		for (i = 0; i < 4; i++)
		{
			cur_word	= (is_last_mb_row && (j == 16)) ? 0 : *(g_src_frame[0]+src_frame_addr+i);
			pix_in_fifo = ((is_last_mb_row && (j == 16)) || (is_last_mb_col && (i == 3))) ? 0 : *(g_src_frame[0]+src_frame_addr+i+1);			
			
			if (j > 0)
			{
				if ((j == MB_SIZE) && is_last_mb_row)
				{
					//cur_word_filt = src_line_buf[i];
					x0_f = (src_line_buf[i] >>  0) & 0xff;
					x1_f = (src_line_buf[i] >>  8) & 0xff;
					x2_f = (src_line_buf[i] >> 16) & 0xff;
					x3_f = (src_line_buf[i] >> 24) & 0xff;
				}
				else
				{
					x0 = src_line_buf[i] & 0xff;
					x1 = (src_line_buf[i] >> 8) & 0xff;
					x2 = cur_word & 0xff;
					x3 = (cur_word >> 8) & 0xff;
					x0_f = PreFilter (x0, x1, x2, x3, thresh);
					
					x0 = (src_line_buf[i] >> 8) & 0xff;
					x1 = (src_line_buf[i] >> 16) & 0xff;
					x2 = (cur_word >> 8) & 0xff;
					x3 = (cur_word >> 16) & 0xff;
					x1_f = PreFilter (x0, x1, x2, x3, thresh);
					
					x0 = (src_line_buf[i] >> 16) & 0xff;
					x1 = (src_line_buf[i] >> 24) & 0xff;
					x2 = (cur_word >> 16) & 0xff;
					x3 = (cur_word >> 24) & 0xff;
					x2_f = PreFilter (x0, x1, x2, x3, thresh);
					
					x0 = (src_line_buf[i] >> 24) & 0xff;
					x1 = (src_line_buf[i+1] >> 0) & 0xff;
					x2 = (cur_word >> 24) & 0xff;
					x3 = (pix_in_fifo >> 0) & 0xff;
					
					if (is_last_mb_col && (i == 3))
					{
						x3_f = x0;						
					}
					else
					{
						x3_f = PreFilter (x0, x1, x2, x3, thresh);
					}
				}				

				cur_word_filt = (x3_f << 24) | (x2_f << 16) | (x1_f << 8) | (x0_f << 0);

				if (i == 3)
				{
					if ((j & 3) != 0)
					{
						word_reg |= x3_f << ((j&3)-1) * 8;
					}
					else
					{
						word_reg |= x3_f << 24;

						//store the four pixel
						ref_wind_addr = INTRA_REF_BASE_ADDR + ((mb_x & 1) ? 8 : 0) + ((j - 1) >> 2);//这里应该存的是左边的参考值才对啊

						mea_window_buf[ref_wind_addr] = word_reg;

						word_reg = 0;
					}
				}

				src_buf_addr = (j-1)*4 + i;
				if (mea_bur_index == 0)
				{
					mea_src_buf0[src_buf_addr] = cur_word_filt;
				}
				else
				{
					mea_src_buf1[src_buf_addr] = cur_word_filt;
				}				
			}


			src_line_buf[i] = cur_word;
			if (i == 3)
			{
				src_line_buf[4] = pix_in_fifo;
			}
		}

		src_frame_addr += frame_width; 
	}

	/*fetch and filter source MB UV*/
	src_frame_addr = mb_y*8 * frame_width + mb_x*MB_SIZE/4;
	
	/*fetch and pre-filter source MB UV*/
	for (j = 0; j < 9; j++)
	{
		if (j == 4)
			printf ("");

		for (i = 0; i < 4; i++)
		{
			cur_word	= (is_last_mb_row && (j == 8)) ? 0 : *(g_src_frame[1]+src_frame_addr+i);
			pix_in_fifo = ((is_last_mb_row && (j == 8)) || (is_last_mb_col && (i == 3))) ? 0 : *(g_src_frame[1]+src_frame_addr+i+1);	
			
			if (j > 0)
			{
				if ((j == 8) && is_last_mb_row)
				{
					x0_f = (src_line_buf[i] >>  0) & 0xff;
					x1_f = (src_line_buf[i] >>  8) & 0xff;
					x2_f = (src_line_buf[i] >> 16) & 0xff;
					x3_f = (src_line_buf[i] >> 24) & 0xff;
				}
				else
				{
					//u
					x0 = src_line_buf[i] & 0xff;
					x1 = (src_line_buf[i] >> 16) & 0xff;
					x2 = cur_word & 0xff;
					x3 = (cur_word >> 16) & 0xff;
					x0_f = PreFilter (x0, x1, x2, x3, thresh);
					
					//v
					x0 = (src_line_buf[i] >> 8) & 0xff;
					x1 = (src_line_buf[i] >> 24) & 0xff;
					x2 = (cur_word >> 8) & 0xff;
					x3 = (cur_word >> 24) & 0xff;
					x1_f = PreFilter (x0, x1, x2, x3, thresh);
					
					//u
					x0 = (src_line_buf[i] >> 16) & 0xff;
					x1 = (src_line_buf[i+1] >> 0) & 0xff;
					x2 = (cur_word >> 16) & 0xff;
					x3 = (pix_in_fifo >> 0) & 0xff;
					if (is_last_mb_col && (i == 3))
					{
						x2_f = x0;					
					}
					else
					{
						x2_f = PreFilter (x0, x1, x2, x3, thresh);
					}
					
					//v
					x0 = (src_line_buf[i] >> 24) & 0xff;
					x1 = (src_line_buf[i+1] >> 8) & 0xff;
					x2 = (cur_word >> 24) & 0xff;
					x3 = (pix_in_fifo >> 8) & 0xff;					
					if (is_last_mb_col && (i == 3))
					{
						x3_f = x0;						
					}
					else
					{
						x3_f = PreFilter (x0, x1, x2, x3, thresh);
					}					
				}

				if (i == 3)
				{
					if ((j & 3) != 0)
					{
						word_reg |= x2_f << ((j&3)-1) * 8;
						word_reg_v |= x3_f << ((j&3)-1) * 8;
					}
					else
					{
						word_reg |= x2_f << 24;
						word_reg_v |= x3_f << 24;
						
						//store the four pixel
						ref_wind_addr = INTRA_REF_BASE_ADDR + ((mb_x & 1) ? 8 : 0) + 4 + ((j - 1) >> 2);
						
						mea_window_buf[ref_wind_addr] = word_reg;

						mea_window_buf[ref_wind_addr+2] = word_reg_v;
						
						word_reg = 0;
						word_reg_v = 0;
					}
				}
				
				two_pix_u = (x2_f << 8) | x0_f;
				two_pix_v = (x3_f << 8) | x1_f;

				if ((i & 1) == 0)
				{
					pix2_reg_u = two_pix_u;
					pix2_reg_v = two_pix_v;					
				}
				else
				{		
					four_pix_u = (two_pix_u << 16) | pix2_reg_u;
					four_pix_v = (two_pix_v << 16) | pix2_reg_v;
					
					src_addr_u = 64 + (j-1)*2 + i/2;
					src_addr_v = 64 + 16 + (j-1)*2 + i/2;

					if (mea_bur_index == 0)
					{
						mea_src_buf0[src_addr_u] = four_pix_u;
						mea_src_buf0[src_addr_v] = four_pix_v;
					}
					else
					{
						mea_src_buf1[src_addr_u] = four_pix_u;
						mea_src_buf1[src_addr_v] = four_pix_v;
					}
				}			
			}	

			src_line_buf[i] = cur_word;
			if (i == 3)
			{
				src_line_buf[4] = pix_in_fifo;
			}			
		}
		
		src_frame_addr += frame_width; 
	}
	
//	OutSrcMBToFrame (mea_src_buf0, mb_x, mb_y, frame_width);
}

void MeaMaster ()
{
	
	g_src_frame[0] = (uint32 *)g_mea_fetch_ptr->src_frame[0];
	g_src_frame[1] = (uint32 *)g_mea_fetch_ptr->src_frame[1];
	g_ref_frame[0] = (uint32 *)g_mea_fetch_ptr->ref_frame[0];
	g_ref_frame[1] = (uint32 *)g_mea_fetch_ptr->ref_frame[1];	

	/*search window*/
	if (!g_mea_fetch_ptr->is_i_frame)
	{
		FetchRefWindow ();
	}

	/*read source MB, and prefilter if enable*/
	FetchSrcMB ();
}
int hmea_fetch_err_mb_to_dctiobfr(void)
{
	uint32 i;
	uint32 blk_num;
	int	   cost = 0;
	
	uint8 * pSrc = (uint8 *)mea_src_buf0;
	uint8 * pRef = (uint8 *)vsp_mea_out_bfr;
	int16 * pDst;
	
	int16 * dct_io_bfr = (int16*)vsp_dct_io_0;
	
	for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
	{
		pDst = dct_io_bfr + 64 * blk_num;
		
		for(i = 0; i < 64; i++)
		{
			pDst[i] = (int16)pSrc[i] - (int16)pRef[i]; 	
			
			cost += ABS(pDst[i]);
		}
		
		pSrc += 64;
		pRef += 64;
	}
	
	return cost;
}

void Mp4Enc_FetchErrMB2DctBuf ()
{
	uint32	i, j;
	uint32	blk_num;	
	int		offset;
	int		stride;
	uint8 * pSrc;
	uint8 * pRef;	
	int16 * pDst = (int16*)vsp_dct_io_0;
	
	for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
	{
		stride = (blk_num < 4) ? 16 : 8;
		
		offset = (blk_num <  4) ? ((blk_num >> 1)*8*16 + (blk_num & 1)*8) : (256 + (blk_num-4)*64);
		pSrc   = ((uint8 *)mea_src_buf0) + offset;
		pRef   = ((uint8 *)vsp_mea_out_bfr) + offset;
		
		for(i = 0; i < 8; i++)
		{
			for (j = 0; j < 8; j++)
			{
				*pDst++ = (int16)pSrc[j] - (int16)pRef[j];
			}
			
			pSrc += stride;
			pRef += stride;
		}		
	}
}


void ConfigureMeaFetch (int mb_y, int mb_x, int is_i_frame)
{
	int line_num;
	int burst_len;
	int start_pos;
	int line_top	= SEARCH_RANGE_Y;
	int line_bellow = SEARCH_RANGE_Y;
	
	g_mb_num_x = (g_glb_reg_ptr->VSP_CFG1 >> 0 ) & 0xff;
	g_mb_num_y = (g_glb_reg_ptr->VSP_CFG1 >> 12) & 0xff;
	g_standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;
	
	g_mea_fetch_ptr->fetch_mb_y = mb_y;
	g_mea_fetch_ptr->fetch_mb_x = mb_x;
	
	line_top = (mb_y*MB_SIZE < line_top) ? mb_y*MB_SIZE : line_top;
	
	if (((mb_y+1)*MB_SIZE + line_bellow) >= g_mb_num_y*MB_SIZE)
	{
		line_bellow = g_mb_num_y*MB_SIZE - (mb_y+1)*MB_SIZE;
		line_bellow = (line_bellow < 0) ? 0 : line_bellow;
	}
	
	line_num = line_top + MB_SIZE + line_bellow;
	
	if (mb_x == 0)
	{
		burst_len = (MB_SIZE + SEARCH_RANGE_X)/4;
	}
	else
	{
		burst_len = MB_SIZE/4;
	}
	
	if ((g_fetch_width + burst_len) > g_mb_num_x*MB_SIZE/4)
	{
		burst_len = g_mb_num_x*4 - g_fetch_width;
		burst_len = burst_len < 0 ? 0 : burst_len;
	}
	
	start_pos = (mb_y*MB_SIZE - line_top) * (g_mb_num_x*MB_SIZE / 4) + g_fetch_width;	
	
	g_fetch_width += burst_len;
	
	if (mb_x == g_mb_num_x - 1)
	{
		g_fetch_width = 0;
	}	
	
	g_mea_fetch_ptr->line_num	= line_num;
	g_mea_fetch_ptr->burst_len	= burst_len;
	g_mea_fetch_ptr->start_pos	= start_pos;
	
	g_mea_fetch_ptr->pre_filter_en = PRE_FILTER_EN;
	g_mea_fetch_ptr->filt_thresh   = FILTER_THERSH;
	
	g_mea_fetch_ptr->is_i_frame	= is_i_frame;
	
	/*start mea to fetch reference pixel and current MB*/
	MeaMaster ();
}

