#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "tv_mea.h"
#include "hdct_global.h"

int16 g_blk_err[256];
int32 cost_array[9];

void SmallDiamondTrace ()
{	
	FPRINTF_ME (g_me_trace_fp, "small diamond search:\n");
	FPRINTF_ME (g_me_trace_fp, "\t      %04x\n",			cost_array[1]);
	FPRINTF_ME (g_me_trace_fp, "\t%04x  %04x  %04x\n",		cost_array[2], cost_array[0], cost_array[3]);
	FPRINTF_ME (g_me_trace_fp, "\t      %04x\n",			cost_array[4]);
}

void GetMinCost (
				 int	rd_cost_0,
				 int	rd_cost_1, 
				 int	rd_cost_2, 
				 int	rd_cost_3, 
				 int	rd_cost_4, 
				 int *	min_pos_ptr, 
				 int *	min_cost_ptr
				 )
{
	int min_pos;
	int min_cost;
	
	min_pos  = 0;
	min_cost = rd_cost_0;
	
	if (rd_cost_1 < min_cost)
	{
		min_cost = rd_cost_1;
		min_pos  = 1;
	}
	
	if (rd_cost_2 < min_cost)
	{
		min_cost = rd_cost_2;
		min_pos  = 2;
	}
	
	if (rd_cost_3 < min_cost)
	{
		min_cost = rd_cost_3;
		min_pos  = 3;
	}
	
	if (rd_cost_4 < min_cost)
	{
		min_cost = rd_cost_4;
		min_pos  = 4;
	}

//	SmallDiamondTrace (rd_cost_0, rd_cost_1, rd_cost_2, rd_cost_3, rd_cost_4);
	
	*min_pos_ptr  = min_pos;
	*min_cost_ptr = min_cost;
}

int OutRangeJudge (
				   int		mb_x,
				   int		mb_y,
				   int		start_x, 
				   int		start_y, 
				   int		blk_width, 
				   int		blk_height,
				   int		mv_x,
				   int		mv_y,
				   int		is_sub_x,
				   int		is_sub_y
				   )
{
	int		is_l_out;
	int		is_r_out;
	int		is_h_out;
	int		is_t_out;
	int		is_b_out;
	int		is_v_out;
	int		is_out_wind;

	int		is_l_out_frm;
	int		is_r_out_frm;
	int		is_h_out_frm;
	int		is_t_out_frm;
	int		is_b_out_frm;
	int		is_v_out_frm;
	int		is_out_frm;
//	int		is_out_range;

	int		srch_range_x;
	int		srch_range_y;

	int		x_max = g_mb_num_x*MB_SIZE;
	int		y_max = g_mb_num_y*MB_SIZE;

	srch_range_x = is_sub_x ? (SEARCH_RANGE_X - 3) : SEARCH_RANGE_X;
	srch_range_y = is_sub_y ? (SEARCH_RANGE_Y - 3) : SEARCH_RANGE_Y;

	is_l_out = (start_x + mv_x) < -srch_range_x;
	is_r_out = (start_x + blk_width + mv_x) > (16 + srch_range_x);
	is_h_out = is_l_out || is_r_out;

	is_t_out = (start_y + mv_y) < -srch_range_y;
	is_b_out = (start_y + blk_height + mv_y) > (16 + srch_range_y);
	is_v_out = is_t_out || is_b_out;

	is_out_wind = is_h_out || is_v_out;

	/*mv is out of frame judgement*/
	is_l_out_frm = ((mb_x*MB_SIZE + start_x + mv_x) < (is_sub_x ? 3 : 0)) ? 1 : 0;
	is_r_out_frm = ((mb_x*MB_SIZE + start_x + mv_x + blk_width) > (is_sub_x ? x_max-3 : x_max)) ? 1 : 0;
	is_h_out_frm = is_l_out_frm || is_r_out_frm;
	
	is_t_out_frm = ((mb_y*MB_SIZE + start_y + mv_y) < (is_sub_y ? 3 : 0)) ? 1 : 0;
	is_b_out_frm = ((mb_y*MB_SIZE + start_y + mv_y + blk_height) > (is_sub_y ? y_max-3 : y_max)) ? 1 : 0;
	is_v_out_frm = is_t_out_frm || is_b_out_frm;
	
	is_out_frm = g_mode_dcs_ptr->unrestricted_mv ? 0 : (is_h_out_frm || is_v_out_frm);

	return (is_out_wind || is_out_frm);
}

void GetMVPredictor( int blk_width, int blk_height, int blk_idx, MOTION_VECTOR_T * mvp, uint8 ignoreD)
{

	int		i_refa;
	int     i_refb;
	int     i_refc;
	int     i_refd;
	int		i_count;
	int		i_ref = 0;
	int		is_h264;
	int		is_blk16x8_mvp = 0;
	int		is_blk8x16_mvp = 0;
	MOTION_VECTOR_T 	mv_a;
	MOTION_VECTOR_T 	mv_b;
	MOTION_VECTOR_T 	mv_c;
	MOTION_VECTOR_T 	mv_d;
	MOTION_VECTOR_T		mv_tmp;
	MOTION_VECTOR_T		mvp_blk16x8;
	MOTION_VECTOR_T		mvp_blk8x16;
	MOTION_VECTOR_T		zero_mv = {0, 0};	

	is_h264 = (g_standard == VSP_H264) ? 1 : 0;

	/*get left, top and top_right reference index*/
	if (blk_idx == 0)
	{
		i_refa = g_mode_dcs_ptr->ref_idx_a;
		i_refb = g_mode_dcs_ptr->ref_idx_b;
		i_refc = ((blk_width == 16) || !is_h264) ? g_mode_dcs_ptr->ref_idx_c : g_mode_dcs_ptr->ref_idx_b;
		i_refd = g_mode_dcs_ptr->ref_idx_d;

		mv_a   = g_mode_dcs_ptr->left_blk_mv[0];
		mv_b   = g_mode_dcs_ptr->top_blk_mv[0];
		mv_c   = ((blk_width == 16) || !is_h264) ? g_mode_dcs_ptr->tr_blk_mv : g_mode_dcs_ptr->top_blk_mv[2];
		mv_d   = g_mode_dcs_ptr->tl_blk_mv;
	}
	else if (blk_idx == 1)//8x16,8x8
	{
	//	if (blk_width == 8)//8x16
		{
			i_refa = 0;
			i_refb = g_mode_dcs_ptr->ref_idx_b;
			i_refc = g_mode_dcs_ptr->ref_idx_c;
			i_refd = g_mode_dcs_ptr->ref_idx_b;

			mv_a.x   = (blk_height == 16) ? g_8x16_sad_mv[0][MVX] : g_8x8_sad_mv[0][MVX];
			mv_a.y  =  (blk_height == 16) ? g_8x16_sad_mv[0][MVY] : g_8x8_sad_mv[0][MVY];
			mv_b   = g_mode_dcs_ptr->top_blk_mv[2];
			mv_c   = g_mode_dcs_ptr->tr_blk_mv;
			mv_d   = g_mode_dcs_ptr->top_blk_mv[1];
		}
//  		else
//  		{
//  		i_refa = g_mode_dcs_ptr->ref_idx_a;
//  		i_refb = 0;
//  		i_refc = -2;
//  		i_refd = g_mode_dcs_ptr->ref_idx_d;
//  
//  		mv_a   = g_mode_dcs_ptr->left_blk_mv[1];
//  		mv_b.x   = g_16x8_sad_mv[0][MVX];
//  		mv_b.y   = g_16x8_sad_mv[0][MVY];
//  		mv_c   = zero_mv;
//  		mv_d   = g_mode_dcs_ptr->left_blk_mv[0];
//  		}
	}
	else if (blk_idx == 2)
	{
		if (blk_width == 8)
		{
			i_refa = g_mode_dcs_ptr->ref_idx_a;
			i_refb = 0;
			i_refc = 0;
			i_refd = g_mode_dcs_ptr->ref_idx_a;

			mv_a = g_mode_dcs_ptr->left_blk_mv[2];
			mv_b.x =  g_8x8_sad_mv[0][MVX]/*g_me_mode_ptr->mv_blk8[0]*/;
			mv_b.y = g_8x8_sad_mv[0][MVY];
			mv_c.x =  g_8x8_sad_mv[1][MVX]/*g_me_mode_ptr->mv_blk8[1]*/;
			mv_c.y =  g_8x8_sad_mv[1][MVY];
			mv_d = g_mode_dcs_ptr->left_blk_mv[1];
		}
		else//16x8
		{
			i_refa = g_mode_dcs_ptr->ref_idx_a;
			i_refb = 0;
			i_refd = g_mode_dcs_ptr->ref_idx_a;
			i_refc = -2; //i_refd;

			mv_a   = g_mode_dcs_ptr->left_blk_mv[2];
			mv_b.x   = g_16x8_sad_mv[0][MVX]/*g_me_mode_ptr->mv_blk16x8[0]*/;
			mv_b.y   = g_16x8_sad_mv[0][MVY];
			mv_d   = g_mode_dcs_ptr->left_blk_mv[1];
			mv_c   = zero_mv; //mv_d;
		}
	}
	else if (blk_idx == 3)
	{
		i_refa = 0;
		i_refb = 0;
		i_refc = -2;
		i_refd = 0;

		mv_a.x = g_8x8_sad_mv[2][MVX]/*g_me_mode_ptr->mv_blk8[2]*/;
		mv_a.y = g_8x8_sad_mv[2][MVY];
		mv_b.x = g_8x8_sad_mv[1][MVX]/*g_me_mode_ptr->mv_blk8[1]*/;
		mv_b.y = g_8x8_sad_mv[1][MVY];
		mv_c = zero_mv;
		mv_d.x = g_8x8_sad_mv[0][MVX]/*g_me_mode_ptr->mv_blk8[0]*/;
		mv_d.y = g_8x8_sad_mv[0][MVY];
	}

	if ((i_refc == -2) && is_h264)
	{
		if(ignoreD)
		{
			//i_refc = 0;
			//mv_c   = zero_mv;
		}
		else
		{
			i_refc = i_refd;
			mv_c   = mv_d;
		}
	}

	if ((blk_width == 16) && (blk_height == 8))
	{
		if( blk_idx == 0 && i_refb == 0 )
		{
			mvp_blk16x8.x = mv_b.x;
			mvp_blk16x8.y = mv_b.y;
			is_blk16x8_mvp = 1;
		}
		else if( blk_idx != 0 && i_refa ==  i_ref)
		{
			mvp_blk16x8.x = mv_a.x;
			mvp_blk16x8.y = mv_a.y;
			is_blk16x8_mvp = 1;
		}
	}

	if ((blk_width == 8) && (blk_height == 16))
	{		
		if( blk_idx == 0 && i_refa == i_ref )
		{
			mvp_blk8x16.x = mv_a.x;
			mvp_blk8x16.y = mv_a.y;
			is_blk8x16_mvp = 1;
		}
		else if( blk_idx != 0 && i_refc == i_ref )
		{
			mvp_blk8x16.x = mv_c.x;
			mvp_blk8x16.y = mv_c.y;
			is_blk8x16_mvp = 1;
		}
	}

	if(ignoreD)
	{
		mv_b.x = (int16)(mv_b.x&0xfffc);
		mv_b.y = (int16)(mv_b.y&0xfffc);
		mv_c.x = (int16)(mv_c.x&0xfffc);
		mv_c.y = (int16)(mv_c.y&0xfffc);
	}

	i_count = 0;
	if( i_refa == i_ref ) i_count++;
	if( i_refb == i_ref ) i_count++;
	if( i_refc == i_ref ) i_count++;

	if( i_count > 1 )
	{
		mv_tmp.x = MEDIAN( mv_a.x, mv_b.x, mv_c.x );
		mv_tmp.y = MEDIAN( mv_a.y, mv_b.y, mv_c.y );
	}
	else if( i_count == 1 )
	{
		if( i_refa == i_ref )
		{
			mv_tmp.x = mv_a.x;
			mv_tmp.y = mv_a.y;
		}
		else if( i_refb == i_ref )
		{
			mv_tmp.x = mv_b.x;
			mv_tmp.y = mv_b.y;
		}
		else
		{
			mv_tmp.x = mv_c.x;
			mv_tmp.y = mv_c.y;
		}
	}
	else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
	{
		mv_tmp.x = mv_a.x;
		mv_tmp.y = mv_a.y;
	}
	else
	{
		mv_tmp.x = MEDIAN( mv_a.x, mv_b.x, mv_c.x );
		mv_tmp.y = MEDIAN( mv_a.y, mv_b.y, mv_c.y );
	}

	*mvp = is_blk16x8_mvp ? mvp_blk16x8 :
		is_blk8x16_mvp ? mvp_blk8x16 : mv_tmp;	

}

uint32 WindBndryLimit (
					 int	mb_x,
					 int	mb_y,
					 int	mv_x,
					 int	mv_y,
					 int	offset_x_mb,
					 int	offset_y_mb,
					 int	blk_width,
					 int	blk_height,
					 uint32	rd_cost
					 )
{
	int		is_l_out_wind;
	int		is_r_out_wind;
	int		is_h_out_wind;
	int		is_t_out_wind;
	int		is_b_out_wind;
	int		is_v_out_wind;
	int		is_out_wind;

	int		is_l_out_frm;
	int		is_r_out_frm;
	int		is_h_out_frm;
	int		is_t_out_frm;
	int		is_b_out_frm;
	int		is_v_out_frm;
	int		is_out_frm;

	int		unrestricted_mv;

	int		cost_ret;
	int		srch_range_x;
	int		srch_range_y;

	int		x_max = g_mb_num_x*MB_SIZE;
	int		y_max = g_mb_num_y*MB_SIZE;

	unrestricted_mv = g_mode_dcs_ptr->unrestricted_mv;

	/*out of search window judgement*/
	srch_range_x = SEARCH_RANGE_X;
	srch_range_y = SEARCH_RANGE_Y;
//	srch_range_x = (g_standard == VSP_H264) ? SEARCH_RANGE_X : SEARCH_RANGE_X;
//	srch_range_y = (g_standard == VSP_H264) ? SEARCH_RANGE_Y : SEARCH_RANGE_Y;

	is_l_out_wind = (offset_x_mb*4 + mv_x) < -srch_range_x*4;
	is_r_out_wind = (offset_x_mb*4 + blk_width*4 + mv_x) > (16 + srch_range_x)*4;
	is_h_out_wind = is_l_out_wind || is_r_out_wind;
	
	is_t_out_wind = (offset_y_mb*4 + mv_y) < -srch_range_y*4;
	is_b_out_wind = (offset_y_mb*4 + blk_height*4 + mv_y) > (16 + srch_range_y)*4;
	is_v_out_wind = is_t_out_wind || is_b_out_wind;
	
	is_out_wind = is_h_out_wind || is_v_out_wind;

	/*mv is out of frame judgement*/
	is_l_out_frm = ((mb_x*MB_SIZE*4 + offset_x_mb*4 + mv_x) < 0) ? 1 : 0;
	is_r_out_frm = ((mb_x*MB_SIZE*4 + offset_x_mb*4 + mv_x + blk_width*4) > x_max*4) ? 1 : 0;
	is_h_out_frm = is_l_out_frm || is_r_out_frm;
	
	is_t_out_frm = ((mb_y*MB_SIZE*4 + offset_y_mb*4 + mv_y) < 0) ? 1 : 0;
	is_b_out_frm = ((mb_y*MB_SIZE*4 + offset_y_mb*4 + mv_y + blk_height*4) > y_max*4) ? 1 : 0;
	is_v_out_frm = is_t_out_frm || is_b_out_frm;

	is_out_frm = unrestricted_mv ? 0 : (is_h_out_frm || is_v_out_frm);
	
	cost_ret = (is_out_wind || is_out_frm) ? 0xffff : rd_cost;

	/*if((is_out_wind || is_out_frm) && (mv != NULL))
	{
		if(is_l_out_wind)
			mv->x = -srch_range_x*4 - offset_x_mb*4;
		if(is_r_out_wind)
			mv->x = (16 + srch_range_x)*4 - (offset_x_mb*4 + blk_width*4);
		if(is_t_out_wind)
			mv->y = -srch_range_y*4 - offset_y_mb*4;
		if(is_b_out_wind)
			mv->y = (16 + srch_range_y)*4 - (offset_y_mb*4 + blk_height*4);
		if(is_l_out_frm)
			mv->x = -(mb_x*MB_SIZE*4 + offset_x_mb*4);
		if(is_r_out_frm)
			mv->x = x_max*4 - (mb_x*MB_SIZE*4 + offset_x_mb*4 + blk_width*4);
		if(is_t_out_frm)
			mv->y = -(mb_y*MB_SIZE*4 + offset_y_mb*4);
		if(is_b_out_frm)
			mv->y = y_max*4 - (mb_y*MB_SIZE*4 + offset_y_mb*4 + blk_height*4);
	}*/

	return cost_ret;
}

int ComputSATDBlk (int blk_width, int blk_height)
{
	int		satd = 0;
	//	int		coeff_num;
	int		blk_y;
	int		blk_x;
	int		i;
	int16 *	blk_ptr;
	int16	blk_tmp[16];
	int		s01;
	int		s23;
	int		d01;
	int		d23;

	for (blk_y = 0; blk_y < blk_height/4; blk_y++)
	{
		for (blk_x = 0; blk_x < blk_width/4; blk_x++)
		{
			blk_ptr = g_blk_err + blk_y*4*blk_width + blk_x*4;

			/*horizontal transform*/
			for (i = 0; i < 4; i++)
			{
				s01 = blk_ptr[0] + blk_ptr[1];
				d01 = blk_ptr[0] - blk_ptr[1];
				s23 = blk_ptr[2] + blk_ptr[3];
				d23 = blk_ptr[2] - blk_ptr[3];

				blk_tmp[i*4+0] = s01 + s23;
				blk_tmp[i*4+1] = s01 - s23;
				blk_tmp[i*4+2] = d01 - d23;
				blk_tmp[i*4+3] = d01 + d23;

				//	satd += ABS(blk_tmp[i*4+0]) + ABS(blk_tmp[i*4+1]) + ABS(blk_tmp[i*4+2]) + ABS(blk_tmp[i*4+3]);

				blk_ptr += blk_width;
			}

			blk_ptr = blk_tmp;

			/*vertical transform*/
			for (i = 0; i < 4; i++)
			{
				s01 = blk_ptr[0*4] + blk_ptr[1*4];
				d01 = blk_ptr[0*4] - blk_ptr[1*4];
				s23 = blk_ptr[2*4] + blk_ptr[3*4];
				d23 = blk_ptr[2*4] - blk_ptr[3*4];				

				satd += ABS(s01 + s23) + ABS(s01 - s23) + ABS(d01 - d23) + ABS(d01 + d23);

				blk_ptr++;
			}
		}
	}

	return satd/2;
}

int SubPixSAD_SixTaps (
					   int		mb_x,
					   int		mb_y,
					   int		offset_x_mb,
					   int		offset_y_mb,
					   int		mv_x,			//in quarter pixel resolution
					   int		mv_y, 
					   int		blk_width, 
					   int		blk_height
					   )
{
	int		i;
	int		j;
	int		start_pos_x;
	int		start_pos_y;
	int		blk_size;
	int		mb_offset;
	int		residual;
	int		sad = 0;
	uint8 * rec_blk_y_ptr;
	uint8 * src_blk_y_ptr;
	int16 *	blk_err_ptr;
//	FILE * tv_me_interpolation;

	blk_err_ptr = g_blk_err;

	memset (blk_err_ptr, 0, 256*sizeof(int16));

	//interpolation
	mb_offset	  = offset_y_mb*16 + offset_x_mb;
	rec_blk_y_ptr = (uint8 *)vsp_mea_out_bfr + mb_offset;
	start_pos_x   = mb_x*16*4 + offset_x_mb*4 + mv_x;//intergel * 16 *4 + sub(8x8) * 4
	start_pos_y   = mb_y*16*4 + offset_y_mb*4 + mv_y;
	if (blk_width == blk_height)  //16x16 8x8 4x4
	{
		blk_size = blk_width;
		me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);
	}
	else if (blk_width > blk_height) //16x8 8x4
	{
		blk_size = blk_height;
		me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);
		// 		if (mb_y == mb_x == 0)
		// 		{
		// 			tv_me_interpolation = fopen ("D:\\jin.zhou\\HD_project\\firmware_sd_fmd_cmodel\\h264_encoder\\simulation\\trace\\soft_interpolation", "w");
		// 			for(j = 0; j < 16; j++)
		// 		}	

		rec_blk_y_ptr += blk_size;
		start_pos_x   += blk_size*4;
		me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);
	}
	else  //8x16 4x8
	{
		blk_size = blk_width;
		me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);

		rec_blk_y_ptr += blk_size*16;
		start_pos_y   += blk_size*4;
		me_mc_luma (start_pos_x, start_pos_y, blk_size, rec_blk_y_ptr);		
	}

	//compute SAD
	rec_blk_y_ptr = (uint8 *)vsp_mea_out_bfr + mb_offset;
	src_blk_y_ptr = (uint8 *)mea_src_buf0 + mb_offset;
	for (j = 0; j < blk_height; j++)
	{
		for (i = 0; i < blk_width; i++)
		{
			residual = src_blk_y_ptr[i] - rec_blk_y_ptr[i];
			sad += ABS(residual);

			*blk_err_ptr++ = residual;
		}

		src_blk_y_ptr += 16;
		rec_blk_y_ptr += 16;
	}

	return sad;
}

int	g_spix_pos[9][2] = {
	{0, 0}, {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}, 
};

int g_sd_cor_ime[5][2] = {
	{0, 0}, {-1, 0}, {0, -1}, {1, 0}, {0, 1}
};
int g_sd_cor_fme[5][2] = {
	{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}
};

int IntPixSad (
			    int		mb_x,
				int		mb_y,
				int		offset_x_mb,
				int		offset_y_mb,
				int		mv_x,			//in quarter pixel resolution
				int		mv_y, 
				int		blk_width, 
				int		blk_height
				)
{

	int		i;
	int		j;
	int		sad = 0;
	int		x_cor;
	int		y_cor;
	int		x_cor_clip;
	int		y_cor_clip;
	int		frm_width;
	int		frm_height;
	int		residual;
	uint8 *	src_mb_ptr;
	uint8 *	ref_frm_ptr;
	int16 * blk_err_ptr;
	
	blk_err_ptr = g_blk_err;
	
	memset (blk_err_ptr, 0, 256*sizeof(int16));

	frm_width  = g_mb_num_x * MB_SIZE;
	frm_height = g_mb_num_y * MB_SIZE;

	src_mb_ptr  = (uint8 *)mea_src_buf0;
	ref_frm_ptr = (uint8 *)g_ref_frame[0];	

	for (j = 0; j < blk_height; j++)
	{
		y_cor		= mb_y*16 + offset_y_mb + mv_y + j;
		y_cor_clip	= IClip(0, frm_height-1, y_cor);

		for (i = 0; i < blk_width; i++)
		{
			x_cor		= mb_x*16 + offset_x_mb +mv_x + i;
			x_cor_clip	= IClip(0, frm_width-1, x_cor);
			residual = src_mb_ptr[(j+offset_y_mb)*16 + i + offset_x_mb] - ref_frm_ptr[y_cor_clip*frm_width + x_cor_clip];
			
			sad += ABS(residual);
			*blk_err_ptr++ = residual;
		}
	}	

	return sad;
}

int SubPixSad (
			    int		mb_x,
				int		mb_y,
				int		offset_x_mb,
				int		offset_y_mb,
				int		mv_x,			//in quarter pixel resolution
				int		mv_y, 
				int		blk_width, 
				int		blk_height
				)
{
	int			i;
	int			j;
	int			frm_width;
	int			frm_height;
	uint8	 *	ref_frm_ptr;
	int			x_cor;
	int			y_cor;
	int			x_ncor;			//next column
	int			y_ncor;			//next row
	int			x_cor_clip;
	int			y_cor_clip;
	int			x_ncor_clip;
	int			y_ncor_clip;
	uint8		pix_0;
	uint8		pix_1;
	uint8		pix_2;
	uint8		pix_3;
	int			dx;
	int			dy;
	int			int_mv_x;
	int			int_mv_y;
	int			offset_mb;
	int			pix_intp;
	int			residual;
	int			sad = 0;
	uint8	*	src_mb_ptr;	
	int16	*	blk_err_ptr;
	
	blk_err_ptr = g_blk_err;
	
	memset (blk_err_ptr, 0, 256*sizeof(int16));
	
	frm_width  = g_mb_num_x * MB_SIZE;
	frm_height = g_mb_num_y * MB_SIZE;

	src_mb_ptr  = (uint8 *)mea_src_buf0;
	ref_frm_ptr = (uint8 *)g_ref_frame[0];	
#if 0
	{
	if(!mb_x&&!mb_y)
	{	
		FILE * file = fopen("D:\\jin.zhou\\ref_2.dat","w");
		FILE * file2 = fopen("D:\\jin.zhou\\src_2.dat","w");
		uint8 data0,data1,data2,data3;
		uint32 data;
		int i,j;
		for(j= 0; j<16; j++)
		{
			for(i = 0; i < 4; i++)
			{
				data0 = ref_frm_ptr[j*frm_width  + i*4];
				data1= ref_frm_ptr[j*frm_width  + i*4 +1];
				data2= ref_frm_ptr[j*frm_width  + i*4 +2];
				data3= ref_frm_ptr[j*frm_width  + i*4 +3];
				FPRINTF(file, "%x%x%x%x\n",data0,data1,data2,data3);
			}
		}
		fclose(file);

		for(j=0; j<24; j++)
		{
			for(i=0; i<4; i++)
			{
				data = mea_src_buf[j*4 + i];
				FPRINTF(file2, "%x\n",data);
			}
		}
		fclose(file2);
	}
	}
#endif
	
	dx		= mv_x & 0x3;
	dy		= mv_y & 0x3;
	int_mv_x	= mv_x >> 2;
	int_mv_y	= mv_y >> 2;

	for (j = 0; j < blk_height; j++)
	{
		y_cor		= mb_y*16 + offset_y_mb + int_mv_y + j;
		y_ncor		= y_cor + 1;
		y_cor_clip	= IClip(0, frm_height-1, y_cor);
		y_ncor_clip 	= IClip(0, frm_height-1, y_ncor);

		for (i = 0; i < blk_width; i++)
		{
			x_cor		= mb_x*16 + offset_x_mb + int_mv_x + i;
			x_ncor		= x_cor + 1;
			x_cor_clip	= IClip(0, frm_width-1, x_cor);
			x_ncor_clip 	= IClip(0, frm_width-1, x_ncor);

			pix_0 = ref_frm_ptr[y_cor_clip*frm_width  + x_cor_clip];
			pix_1 = ref_frm_ptr[y_cor_clip*frm_width  + x_ncor_clip];
			pix_2 = ref_frm_ptr[y_ncor_clip*frm_width + x_cor_clip];
			pix_3 = ref_frm_ptr[y_ncor_clip*frm_width + x_ncor_clip];

			pix_intp = (pix_0 * (4 - dx) * (4 - dy) + 
						pix_1 * dx * (4 - dy)		+ 
						pix_2 * (4 - dx) * dy		+ 
						pix_3 * dx * dy +  8) / 16;

			offset_mb = (j+offset_y_mb)*16 + offset_x_mb + i;	
			if(j == 15)
				printf("");
			residual  = src_mb_ptr[offset_mb] - pix_intp;
			sad      += ABS(residual);
			*blk_err_ptr++ = residual;
		}
	}

	return sad;
}

int SubPixME (
			  MOTION_VECTOR_T *	mv_blk_ptr,
			  int					mb_y,
			  int					mb_x,
			  int					blk_width, 
			  int					blk_height, 
			  int					offset_x_mb, 
			  int					offset_y_mb,
			  int					blk_id,				//block index in 4x4 unit
			  MOTION_VECTOR_T	*	mvp,
			  int					is_out_range[4][3]	// 0:original point, 1:horizontal, 2:vertical
			  )
{
	int			is_blk16x16, is_blk8x8;
	int			i_pos;
	int			frm_width;
	int			frm_height;
	int			min_pos;
	int			min_cost;
//	int			min_half_pos;
	int			imv_x;
	int			imv_y;
	int			mv_x;
	int			mv_y;
	int			step;
	int			coeff_cost;
	int			cost;
	int			cent_x;
	int			cent_y;
	int			dir;				// 0:original point, 1:horizontal, 2:vertical
	uint8	*	src_mb_ptr;
	uint8	*	ref_frm_ptr;
	int			is_sub_x;
	int			is_sub_y;
//	MOTION_VECTOR_T		mvp;

	is_blk16x16 = ((blk_width == 16) && (blk_height == 16)) ? 1 : 0;
	is_blk8x8   = ((blk_width ==  8) && (blk_height ==  8)) ? 1 : 0;

	frm_width  = g_mb_num_x * MB_SIZE;
	frm_height = g_mb_num_y * MB_SIZE;

	src_mb_ptr  = (uint8 *)mea_src_buf0;
	ref_frm_ptr = (uint8 *)g_ref_frame[0];

	min_pos = 0;
	min_cost = 65535;

//	GetMVPredictor (blk_width, blk_height, blk_id, &mvp, 1);
	is_out_range[blk_id][0] = is_out_range[blk_id][1] = is_out_range[blk_id][2] = 0;
	
	FPRINTF_ME (g_me_trace_fp, "start sub_pix me, mv_x: %d, int_mv_y: %d\n", mv_blk_ptr->x, mv_blk_ptr->y);
	FPRINTF_ME (g_me_trace_fp, "half pixel search\n");

	cost_array[0] = 65535; 
	memset(cost_array, 65535, 9*sizeof(int));
	for (step = 0; step < 1; step++)
	{
		imv_x = mv_blk_ptr->x >> 2;
		imv_y = mv_blk_ptr->y >> 2;
	// half pixel estimation
	// can start from i_pos=1, if IME's cost pass to this func
	for (i_pos = 0; i_pos < 5; i_pos++)
	{
		dir = (i_pos+1)>>1;
		mv_x = mv_blk_ptr->x + g_sd_cor_fme[i_pos][0]*2;
		mv_y = mv_blk_ptr->y + g_sd_cor_fme[i_pos][1]*2;

		is_sub_x = ((mv_x&3) != 0) ? 1 : 0;
		is_sub_y = ((mv_y&3) != 0) ? 1 : 0;

		// dir=0 will never out of range, can be skipped if needed
		is_out_range[blk_id][dir] |= OutRangeJudge (mb_x, mb_y, offset_x_mb, offset_y_mb, blk_width, blk_height, imv_x, imv_y, is_sub_x, is_sub_y);

		cost = 65535;

		if (!is_out_range[blk_id][dir])
		{
			if((mb_x == 5)&&(mb_y == 0))
				printf("");

#ifdef SATD_COST
			coeff_cost = ComputSATDBlk (blk_width, blk_height);
#else
			coeff_cost = SubPixSAD_SixTaps (mb_x, mb_y, offset_x_mb, offset_y_mb, mv_x, mv_y, blk_width, blk_height);
#endif
			//cost = coeff_cost + CostMVD(mv_x, mv_y, mvp->x, mvp->y);
			if(is_blk16x16)
				cost = coeff_cost + g_16x16_mvd_cost;
			else if(is_blk8x8)
				cost = coeff_cost + g_8x8_mvd_cost[blk_id];

#if MEA_PATTERN
			{
				unsigned char s[100];
				sprintf(s, "Frame_Cnt %d MB %d : FME%d Block8_id=%d 1/2 MV(%d,%d) MVP(%d,%d)", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, blk_id, mv_x, mv_y, mvp->x, mvp->y);
				Print_sad_mv_cost(coeff_cost, cost-coeff_cost, s, g_fme_mb_sad_fp);
			}
#endif

			if (cost < min_cost)
			{
				min_cost = cost;
				min_pos = i_pos;
			}
		}
		/*else
		{
			min_cost = min_cost;	// for debug only
		}*/

		cost_array[i_pos] = cost;
	}
	SmallDiamondTrace();
	}

	FPRINTF_ME (g_me_trace_fp, "quarter pixel search\n");

	mv_blk_ptr->x = mv_blk_ptr->x + g_sd_cor_fme[min_pos][0]*2;
	mv_blk_ptr->y = mv_blk_ptr->y + g_sd_cor_fme[min_pos][1]*2;

#if MEA_PATTERN
	{
		unsigned char s[100];
		uint32 mvd_cost;
		mvd_cost = is_blk16x16 ? g_16x16_mvd_cost : g_8x8_mvd_cost[blk_id];
		sprintf(s, "Frame_Cnt %d MB %d : FME%d Block8_id=%d 1/2 Best_MV(%d,%d) mvd_cost=0x%x", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, blk_id, mv_blk_ptr->x, mv_blk_ptr->y, mvd_cost);
		Print_best_sad_mv_cost(min_cost-mvd_cost, s, g_fme_best_sad_fp);
	}
#endif

	//quarter pixel estimation
	for (step = 0; step < 1; step++)	// HW cycles can only do one time
	{
		cent_x = mv_blk_ptr->x;
		cent_y = mv_blk_ptr->y;

		min_pos  = 0; 
		min_cost = 65535; 
		cost_array[0] = 65535;

		// can start from i_pos=1, if half's cost pass to this func
		for (i_pos = 0; i_pos < 5; i_pos++)
		{
			if((i_pos == 2)&&(mb_x == 0x5)&&(mb_y == 0x0))
				printf("");

			if(mb_x == 5)
				printf("");

			dir = (i_pos+1)>>1;
			mv_x = cent_x + g_sd_cor_fme[i_pos][0];
			mv_y = cent_y + g_sd_cor_fme[i_pos][1];

			/*imv_x = mv_x >> 2;
			imv_y = mv_y >> 2;

			is_sub_x = ((mv_x&3) != 0) ? 1 : 0;
			is_sub_y = ((mv_y&3) != 0) ? 1 : 0;

			// dir=0 will never out of range, can be skipped if needed
			is_out_range[dir] |= OutRangeJudge (offset_x_mb, offset_y_mb, blk_width, blk_height, imv_x, imv_y, is_sub_x, is_sub_y);*/

			cost = 65535;

			if (!is_out_range[blk_id][dir])
			{
#ifdef SATD_COST
				coeff_cost = ComputSATDBlk (blk_width, blk_height);
#else
				coeff_cost = SubPixSAD_SixTaps (mb_x, mb_y, offset_x_mb, offset_y_mb, mv_x, mv_y, blk_width, blk_height);
#endif
				//cost = coeff_cost + CostMVD(mv_x, mv_y, mvp->x, mvp->y)/**1.5*/;
				if(is_blk16x16)
					cost = coeff_cost + g_16x16_mvd_cost;
				else if(is_blk8x8)
					cost = coeff_cost + g_8x8_mvd_cost[blk_id];

#if MEA_PATTERN
				{
					unsigned char s[100];
					sprintf(s, "Frame_Cnt %d MB %d : FME%d Block8_id=%d 1/4 MV(%d,%d) MVP(%d,%d)", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, blk_id, mv_x, mv_y, mvp->x, mvp->y);
					Print_sad_mv_cost(coeff_cost, cost-coeff_cost, s, g_fme_mb_sad_fp);
				}
#endif

				if (cost < min_cost)
				{
					min_cost = cost;
					min_pos  = i_pos;
				}
			}

			cost_array[i_pos] = cost;
		}
		SmallDiamondTrace();
		mv_blk_ptr->x = cent_x + g_sd_cor_fme[min_pos][0];
		mv_blk_ptr->y = cent_y + g_sd_cor_fme[min_pos][1];
#if MEA_PATTERN
		{
			unsigned char s[100];
			uint32 mvd_cost;
			mvd_cost = is_blk16x16 ? g_16x16_mvd_cost : g_8x8_mvd_cost[blk_id];
			sprintf(s, "Frame_Cnt %d MB %d : FME%d Block8_id=%d 1/4 Step=%d Best_MV(%d,%d) mvd_cost=0x%x", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, blk_id, step, mv_blk_ptr->x, mv_blk_ptr->y, mvd_cost);
			Print_best_sad_mv_cost(min_cost-mvd_cost, s, g_fme_best_sad_fp);
		}
#endif
		if (min_pos == 0)
			break;
	}

	FPRINTF_ME (g_me_trace_fp, "min_cost_subpix: %04x, mv_x: %d, mv_y: %d\n", min_cost, mv_blk_ptr->x, mv_blk_ptr->y);

	return min_cost;
}

int MotionEstBlk_SD (
					   MOTION_VECTOR_T *	pmv,			//start point for small diamond 
					   int					i_blk,			//blk8 unit
					   int					mb_x, 
					   int					mb_y,
					   int					blk_width, 
					   int					blk_height,
					   MOTION_VECTOR_T *	mv_blk_ptr,		//return searched mv for the block
					   MOTION_VECTOR_T *	mvp
					   )
{
	int		is_blk16x16, is_blk8x8;
	int		cent_x;
	int		cent_y;
	int		i_point;
//	int		blk4x4_id;
	int		coeff_cost;
	int		cost;
	int		mv_x;
	int		mv_y;
	int		start_x;
	int		start_y;
	int		min_cost;
	int		min_pos;
	int		is_out_range;
	int		i_step;
	int		i_step_limit;
//	MOTION_VECTOR_T	mvp;

	is_blk16x16 = ((blk_width == 16) && (blk_height == 16)) ? 1 : 0;
	is_blk8x8   = ((blk_width ==  8) && (blk_height ==  8)) ? 1 : 0;
	
	start_x = (i_blk&1)*8;
	start_y = (i_blk>>1)*8;

	*mv_blk_ptr = *pmv;
	memset(cost_array, 65535, 9*sizeof(int));

//	GetMVPredictor (blk_width, blk_height, i_blk, &mvp, 1);
//	fprintf(g_me_trace_fp, "Block %d (%d,%d)\n", i_blk, mvp.x, mvp.y);
	
	FPRINTF_ME (g_me_trace_fp, "\ncenter mv_x: %d, mv_y: %d\n", pmv->x >> 2, pmv->y >> 2);

	i_step_limit = is_blk16x16 ? MEA_SEARCH_ROUND_16 : MEA_SEARCH_ROUND_8;
	//i_step_limit = is_blk16x16 ? (MEA_SEARCH_ROUND_16 - (g_nFrame_enc%3)) : (MEA_SEARCH_ROUND_8 + (g_nFrame_enc%3));	

	for (i_step = 0; i_step < i_step_limit; i_step++)
	{		
		cent_x = mv_blk_ptr->x >> 2;
		cent_y = mv_blk_ptr->y >> 2;
		
		min_cost = 65535;
		min_pos = 0;
		
		for (i_point = 0; i_point < 5; i_point++)
		{
			mv_x = cent_x + g_sd_cor_ime[i_point][0];
			mv_y = cent_y + g_sd_cor_ime[i_point][1];			
			
			//is_out_range = OutRangeJudge (start_x, start_y, blk_width, blk_height, mv_x, mv_y, 0, 0);
			is_out_range = WindBndryLimit (mb_x, mb_y, mv_x*4, mv_y*4, start_x, start_y, blk_width, blk_height, 0);
			
			cost = 65535;
			if (is_out_range==0)
			{
#ifdef SATD_COST
				coeff_cost = ComputSATDBlk (blk_width, blk_height);
#else
				coeff_cost = IntPixSad (mb_x, mb_y, start_x, start_y, mv_x, mv_y, blk_width, blk_height);
#endif				
				cost = coeff_cost + CostMVD(mv_x*4, mv_y*4, mvp->x, mvp->y);

#if MEA_PATTERN
				{
					unsigned char s[100];
					sprintf(s, "Frame_Cnt %d MB %d : IME%d Block8_id=%d Stage 2 MV(%d,%d) MVP(%d,%d)", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, i_blk, mv_x*4, mv_y*4, mvp->x, mvp->y);
					Print_sad_mv_cost(coeff_cost, cost-coeff_cost, s, g_ime_mb_sad_fp);
				}
#endif
				
				if (cost < min_cost)
				{
					min_cost = cost;
					min_pos  = i_point;
					if(is_blk16x16)
						g_16x16_mvd_cost = min_cost - coeff_cost;
					if(is_blk8x8)
						g_8x8_mvd_cost[i_blk] = min_cost - coeff_cost;
				}
			}
			
			cost_array[i_point] = cost;
		}

		SmallDiamondTrace();
		
		mv_blk_ptr->x = (cent_x + g_sd_cor_ime[min_pos][0]) << 2;
		mv_blk_ptr->y = (cent_y + g_sd_cor_ime[min_pos][1]) << 2;
#if MEA_PATTERN
		{
			unsigned char s[100];
			uint32 mvd_cost;
			mvd_cost = is_blk16x16 ? g_16x16_mvd_cost : g_8x8_mvd_cost[i_blk];
			sprintf(s, "Frame_Cnt %d MB %d : IME%d round 2 Block8_id=%d Step=%d Best_MV(%d,%d) mvd_cost=0x%x", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, i_blk, i_step, mv_blk_ptr->x, mv_blk_ptr->y, mvd_cost);
			Print_best_sad_mv_cost(min_cost-mvd_cost, s, g_ime_best_sad_fp);
		}
#endif
		if (min_pos == 0)
		{
			break;
		}		
	}

	return min_cost;
}

// Only Used in IME
int BlkSearch ( 
					int				mb_x, 
					int				mb_y, 
					int				blk_width, 
					int				blk_height, 
					int				blk8_id,		
					MOTION_VECTOR_T *mv_cand,		
					MOTION_VECTOR_T *mv_blk_ptr,
					int				round,
					int				step,
					MOTION_VECTOR_T *mvp,
					int				*mvd_cost
					)		
{
	int		is_blk16x16, is_blk8x8, is_blk16x8, is_blk8x16;
	int		offset_x_mb, offset_y_mb;
//	int		offset_x;
//	int		offset_y;
	int		is_sp;
//	int		mv_cost_pe0; //, mv_cost_pe1, mv_cost_pe2, mv_cost_pe3;
	int		rd_cost_pe0; //, rd_cost_pe1, rd_cost_pe2, rd_cost_pe3;
//	int		min_pos;
	int		min_cost;
	MOTION_VECTOR_T pmv;
	
	is_blk16x16 = ((blk_width == 16) && (blk_height == 16)) ? 1 : 0;
	is_blk8x8   = ((blk_width ==  8) && (blk_height ==  8)) ? 1 : 0;
	is_blk16x8  = ((blk_width == 16) && (blk_height ==  8)) ? 1 : 0;
	is_blk8x16  = ((blk_width ==  8) && (blk_height == 16)) ? 1 : 0;

	offset_x_mb = (blk8_id & 1) * 8;
	offset_y_mb = (blk8_id >> 1) * 8;

	is_sp = (step >> 2 & 1) ? 1 : 0;
	pmv.x = mv_cand->x;
	pmv.y = mv_cand->y;
	if(is_blk16x8) blk8_id= blk8_id*2;
	if(round == 1)
	{
		step = 0;

		// Limit MV
		//WindBndryLimit (mb_x, mb_y, pmv.x, pmv.y, offset_x_mb, offset_y_mb, blk_width, blk_height, 0, &pmv);

		//PEArray (mb_x, mb_y, blk_width, blk_height, blk8_id, &pmv, step, 1);
		//rd_cost_pe0 = SubPixSad(mb_x, mb_y, offset_x_mb, offset_y_mb, pmv.x, pmv.y, blk_width, blk_height);
		rd_cost_pe0 = IntPixSad(mb_x, mb_y, offset_x_mb, offset_y_mb, pmv.x >> 2, pmv.y >> 2, blk_width, blk_height);
		*mvd_cost = CostMVD(pmv.x, pmv.y, mvp->x, mvp->y);

#if MEA_PATTERN
		{
			unsigned char s[100];
			sprintf(s, "Frame_Cnt %d MB %d : IME%d Block8_id=%d Stage 1 MV(%d,%d) MVP(%d,%d)", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, is_blk16x16?16:8, blk8_id, pmv.x, pmv.y, mvp->x, mvp->y);
			Print_sad_mv_cost(rd_cost_pe0, *mvd_cost, s, g_ime_mb_sad_fp);
		}
#endif
		rd_cost_pe0 += *mvd_cost;
		rd_cost_pe0 = WindBndryLimit (mb_x, mb_y, pmv.x, pmv.y, offset_x_mb, offset_y_mb, blk_width, blk_height, rd_cost_pe0);
		*mv_blk_ptr = pmv;
		FPRINTF_ME (g_me_trace_fp, "\tmvx ; %d,  mvy: %d  cost :%x\n",pmv.x, pmv.y, rd_cost_pe0);
	//	rd_cost_pe0 -= mv_cost_pe0;

		return rd_cost_pe0;
	}
	else if(round == 4)
	{
		//PEArray (mb_x, mb_y, blk_width, blk_height, blk8_id, &pmv, step, 0);
		min_cost = MotionEstBlk_SD ( &pmv, blk8_id, mb_x, mb_y, blk_width, blk_height, mv_blk_ptr, mvp);
/*
		mv_cost_pe0 = CostMVD (pmv.x, pmv.y - step, mvp->x, mvp->y);
		mv_cost_pe1 = CostMVD (pmv.x - step, pmv.y, mvp->x, mvp->y);
		mv_cost_pe2 = CostMVD (pmv.x + step, pmv.y, mvp->x, mvp->y);
		mv_cost_pe3 = CostMVD (pmv.x, pmv.y + step, mvp->x, mvp->y);

		rd_cost_pe0 = g_sad_pe0 + mv_cost_pe0;
		rd_cost_pe1 = g_sad_pe1 + mv_cost_pe1;
		rd_cost_pe2 = g_sad_pe2 + mv_cost_pe2;
		rd_cost_pe3 = g_sad_pe3 + mv_cost_pe3;
		FPRINTF_ME (g_me_trace_fp, "\nrd_cost_po0: %d, rd_cost_pe1: %d, rd_cost_pe2: %d, rd_cost_pe3: %d\n", rd_cost_pe0, rd_cost_pe1, rd_cost_pe2, rd_cost_pe3);

		rd_cost_pe0 = WindBndryLimit (mb_x, mb_y, pmv.x, pmv.y-step, offset_x_mb, offset_y_mb, blk_width, blk_height, rd_cost_pe0);
		rd_cost_pe1 = WindBndryLimit (mb_x, mb_y, pmv.x-step, pmv.y, offset_x_mb, offset_y_mb, blk_width, blk_height, rd_cost_pe1);
		rd_cost_pe2 = WindBndryLimit (mb_x, mb_y, pmv.x+step, pmv.y, offset_x_mb, offset_y_mb, blk_width, blk_height, rd_cost_pe2);
		rd_cost_pe3 = WindBndryLimit (mb_x, mb_y, pmv.x, pmv.y+step, offset_x_mb, offset_y_mb, blk_width, blk_height, rd_cost_pe3);
		FPRINTF_ME (g_me_trace_fp, "\t\t%x\n%x\t\t\t%x\n\t\t%x\n", rd_cost_pe0, rd_cost_pe1, rd_cost_pe2, rd_cost_pe3);

		GetMinCost (0xfffffff, rd_cost_pe0, rd_cost_pe1, rd_cost_pe2, rd_cost_pe3, &min_pos, &min_cost);

		offset_x = (min_pos == 2) ? -step:
				   (min_pos == 3) ? step : 0;
		
		offset_y = (min_pos == 1) ? -step :
				   (min_pos == 4) ? step : 0;

		pmv.x = pmv.x + offset_x;
		pmv.y = pmv.y + offset_y;

		*mv_blk_ptr = pmv;

		g_bestpoint = (min_pos == 1) ? 1 : (min_pos == 2) ? 2 : (min_pos == 3) ? 0 : 3;
		FPRINTF_ME (g_me_trace_fp, "min_cost : %x, mv_x: %d, mv_y: %d\n", min_cost, pmv.x, pmv.y);


//		min_cost -= ((min_pos == 1) ? mv_cost_pe0 : (min_pos == 2) ? mv_cost_pe1 : (min_pos == 3) ? mv_cost_pe2 : mv_cost_pe3);
*/
		return min_cost;
	}
	return 0xfffff;
}
