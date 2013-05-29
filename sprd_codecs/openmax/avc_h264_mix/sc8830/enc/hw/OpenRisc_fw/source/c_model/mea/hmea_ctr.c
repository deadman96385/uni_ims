#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"
#include "hmea_global.h"
#include "common_global.h"
#include "buffer_global.h"
#include "tv_mea.h"
#include "hdct_global.h"


uint8 g_SrcMB[6][64];
uint8 g_RefMB[6][64];
int16 g_ErrMB[6][64];
uint8 g_FinalRefMB[6][64];

void GetMinMode (
				 int	rd_cost_1, 
				 int	rd_cost_2, 
				 int	rd_cost_3, 
				 int	rd_cost_4, 
				 int *	me_mode, 
				 int *	cost
				 )
{
	int min_pos;
	int min_cost;

	min_pos  = 0;
	min_cost = 0xfffffff;

	if (rd_cost_1 < min_cost)
	{
		min_cost = rd_cost_1;
		min_pos  = INTER_MB_16X16;
	}

	if (rd_cost_2 < min_cost)
	{
		min_cost = rd_cost_2;
		min_pos  = INTER_MB_8X8;
	}

	if (rd_cost_3 < min_cost)
	{
		min_cost = rd_cost_3;
		min_pos  = INTER_MB_16X8;
	}

	if (rd_cost_4 < min_cost)
	{
		min_cost = rd_cost_4;
		min_pos  = INTER_MB_8X16;
	}
	*cost = min_cost;
	*me_mode = min_pos;
}
/*
void Update8x8sad(int cost)
{ 
	switch (g_bestpoint)
	{
	case 0:
		if(g_8x8_sad[0] > g_sad_blk0_pe2 + cost)
		{
			g_8x8_sad[0] = g_sad_blk0_pe2+ cost;
			g_8x8_sad_mv[0][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[0][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[1] > g_sad_blk1_pe2 + cost)
		{
			g_8x8_sad[1] = g_sad_blk1_pe2+ cost;
			g_8x8_sad_mv[1][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[1][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[2] > g_sad_blk2_pe2 + cost)
		{
			g_8x8_sad[2] = g_sad_blk2_pe2+ cost;
			g_8x8_sad_mv[2][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[2][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[3] > g_sad_blk3_pe2 + cost)
		{
			g_8x8_sad[3] = g_sad_blk3_pe2+ cost;
			g_8x8_sad_mv[3][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[3][MVY] = g_16x16_sad_mv[MVY]; 
		}
		break;
	case 1:
		if(g_8x8_sad[0] > g_sad_blk0_pe0 + cost)
		{
			g_8x8_sad[0] = g_sad_blk0_pe0+ cost;
			g_8x8_sad_mv[0][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[0][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[1] > g_sad_blk1_pe0 + cost)
		{
			g_8x8_sad[1] = g_sad_blk1_pe0+ cost;
			g_8x8_sad_mv[1][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[1][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[2] > g_sad_blk2_pe0 + cost)
		{
			g_8x8_sad[2] = g_sad_blk2_pe0+ cost;
			g_8x8_sad_mv[2][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[2][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[3] > g_sad_blk3_pe0 + cost)
		{
			g_8x8_sad[3] = g_sad_blk3_pe0+ cost;
			g_8x8_sad_mv[3][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[3][MVY] = g_16x16_sad_mv[MVY]; 
		}
		break;
	case 2:
		if(g_8x8_sad[0] > g_sad_blk0_pe1 + cost)
		{
			g_8x8_sad[0] = g_sad_blk0_pe1+ cost;
			g_8x8_sad_mv[0][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[0][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[1] > g_sad_blk1_pe1 + cost)
		{
			g_8x8_sad[1] = g_sad_blk1_pe1+ cost;
			g_8x8_sad_mv[1][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[1][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[2] > g_sad_blk2_pe1 + cost)
		{
			g_8x8_sad[2] = g_sad_blk2_pe1+ cost;
			g_8x8_sad_mv[2][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[2][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[3] > g_sad_blk3_pe1 + cost)
		{
			g_8x8_sad[3] = g_sad_blk3_pe1+ cost;
			g_8x8_sad_mv[3][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[3][MVY] = g_16x16_sad_mv[MVY]; 
		}
		break;
	case 3:
		if(g_8x8_sad[0] > g_sad_blk0_pe3 + cost)
		{
			g_8x8_sad[0] = g_sad_blk0_pe3+ cost;
			g_8x8_sad_mv[0][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[0][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[1] > g_sad_blk1_pe3 + cost)
		{
			g_8x8_sad[1] = g_sad_blk1_pe3+ cost;
			g_8x8_sad_mv[1][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[1][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[2] > g_sad_blk2_pe3 + cost)
		{
			g_8x8_sad[2] = g_sad_blk2_pe3+ cost;
			g_8x8_sad_mv[2][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[2][MVY] = g_16x16_sad_mv[MVY]; 
		}
		if(g_8x8_sad[3] > g_sad_blk3_pe3 + cost)
		{
			g_8x8_sad[3] = g_sad_blk3_pe3+ cost;
			g_8x8_sad_mv[3][MVX] = g_16x16_sad_mv[MVX];
			g_8x8_sad_mv[3][MVY] = g_16x16_sad_mv[MVY]; 
		}
		break;
	}
}*/

int MBSAD2PartEst (int cost_8x8, int cost16x16, MOTION_VECTOR_T *mvp_pool)
{
	int32		thresh16x8;
	uint32		mvd_cost1;
	uint32		mvd_cost2;
	uint32		be_est_2p;
	//MOTION_VECTOR_T		mvp;
	ME_MODE_T * me_mode_ptr = g_me_mode_ptr;

	//GetMVPredictor (8, 8, 1, &mvp, 1);
	mvd_cost1 = CostMVD (g_8x8_sad_mv[1][MVX], g_8x8_sad_mv[1][MVY], mvp_pool[2].x, mvp_pool[2].y);

	//GetMVPredictor (8, 8, 2, &mvp, 1);
	mvd_cost2 = CostMVD (g_8x8_sad_mv[2][MVX], g_8x8_sad_mv[2][MVY], mvp_pool[3].x, mvp_pool[3].y);

	thresh16x8 = mvd_cost1 + mvd_cost2;

	be_est_2p = (cost_8x8 < cost16x16 + thresh16x8) ? 1 : 0;

	be_est_2p = 1;

	return be_est_2p;
}

void hmea_Init()
{
//	TraceInit ();

	g_mea_fetch_ptr			= (MEA_FETCH_T *)malloc(sizeof(MEA_FETCH_T));

	g_me_mode_ptr			= (ME_MODE_T *)malloc(sizeof(ME_MODE_T));
	g_mode_dcs_ptr			= (MODE_DECISION_T *)malloc(sizeof(MODE_DECISION_T));
}


void MeaMBReset ()
{
//	g_step_count =0;
	g_step_count16 = 0;
	g_step_count8 = 0;
	g_step_count16_8 = g_step_count8_16 = 0;

	g_mb_mean_y = 0;
	g_mb_dev	= 0;
	g_mean_cal_done = 0;
	g_dev_cal_done	= 0;

	g_sad_pe0 =	0;
	g_sad_pe1 = 0;
	g_sad_pe2 = 0;
	g_sad_pe3 = 0;

	g_min_cost_blk0 = 0xffff;	//0;
	g_min_cost_blk1 = 0xffff;	//0;
	g_min_cost_blk2 = 0xffff;	//0;
	g_min_cost_blk3 = 0xffff;	//0;

	g_8x8_sad[0] = g_8x8_sad[1] = g_8x8_sad[2] = g_8x8_sad[3] = 16384;
//	g_16x8_sad_mv[BLK0_16x8][SAD] = g_16x8_sad_mv[BLK1_16x8][SAD]  = 32768;
///	g_8x16_sad_mv[BLK0_8x16][SAD] = g_8x16_sad_mv[BLK1_8x16][SAD] = 32768;
	g_16x16_sad = 65536;
	g_16x8_sad[0]= g_16x8_sad[1]= 32768;
	g_8x16_sad[0]= g_8x16_sad[1]= 32768;

	g_mode_dcs_ptr->intra16_mode_y = 0;
	g_mode_dcs_ptr->intra_mode_c   = 0;

	memset(vsp_mea_out_bfr, 0, MEA_OUT_BFR_SIZE * sizeof(uint32));
}

uint8 FME(int mb_x, int mb_y, int mode, MOTION_VECTOR_T *mvp_pool, int is_out_range[4][3])
{
//	int	step, round;
	int j;
	int	blk_cost;
	int offset_x_mb;
	int offset_y_mb;
	int blk8_id;
//	int ret;
	MOTION_VECTOR_T	mv_cand;
//	MOTION_VECTOR_T	mvp;
//	MOTION_VECTOR_T	mv_blk;
	
	if(mode == INTER_MB_16X16)
	{
		//step = 2;
		FPRINTF_ME (g_me_trace_fp, "\n**************************\nFME mode: 16x16 \n");
		
		mv_cand.x = g_16x16_sad_mv[MVX];
		mv_cand.y = g_16x16_sad_mv[MVY];
		FPRINTF_ME(g_me_trace_fp, "mv_x : %d,  mv_y : %d \n", mv_cand.x, mv_cand.y);

//		blk_cost = BlkSearch (mb_x, mb_y, 16, 16, 0, &mv_cand, &mv_blk, round, step, &mvp);
		blk_cost = SubPixME (&mv_cand, mb_y, mb_x, 16, 16, 0, 0, 0, &mvp_pool[0], is_out_range);	
	//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y);			
// 		if( blk_cost < g_16x16_sad )
// 		{
			g_16x16_sad = blk_cost;
			g_16x16_sad_mv[MVX] = mv_cand.x;
			g_16x16_sad_mv[MVY] = mv_cand.y;
//		}
		g_mode_dcs_ptr->min_cost = g_16x16_sad;
#if 0	
		step = 1;
		round = 4;
		GetMVPredictor (16,16, 0, &mvp, 1);
		FPRINTF_ME (g_me_trace_fp, "\n**************************\n1/4 mode: 16x16 \n");
		{
			mv_cand.x = g_16x16_sad_mv[MVX];
			mv_cand.y = g_16x16_sad_mv[MVY];
			
			FPRINTF_ME(g_me_trace_fp, "mv_x : %d,  mvy : %d \n", mv_cand.x, mv_cand.y);
			blk_cost = BlkSearch (mb_x, mb_y, 16, 16, 0, &mv_cand, &mv_blk, round, step, &mvp);
	//		blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y, 1);	
			if( blk_cost < g_16x16_sad )
			{
				mv_cand.x = mv_blk.x;
				mv_cand.y = mv_blk.y;
				g_16x16_sad = blk_cost;
				g_16x16_sad_mv[MVX] = mv_blk.x;
				g_16x16_sad_mv[MVY] = mv_blk.y;
			}
		}
#endif
	}
	else if(mode == INTER_MB_8X8)
	{
		FPRINTF_ME (g_me_trace_fp, "\n**************************\nFME mode: 8x8 \n");
		for (blk8_id=0 ; blk8_id< 4; blk8_id++)
		{
			offset_x_mb = (blk8_id & 1) * 8;
			offset_y_mb = (blk8_id >> 1) * 8;
			//step = 2;
	 		mv_cand.x = g_8x8_sad_mv[blk8_id][MVX];
	 		mv_cand.y = g_8x8_sad_mv[blk8_id][MVY];
			FPRINTF_ME(g_me_trace_fp, "mv_x : %d,  mvy : %d \n", mv_cand.x, mv_cand.y);
 
	 		blk_cost = SubPixME (&mv_cand, mb_y, mb_x, 8, 8, offset_x_mb, offset_y_mb, blk8_id, &mvp_pool[blk8_id+1], is_out_range);
	 	//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y);	 
//	 		if( blk_cost < g_8x8_sad[blk8_id] )
//	 		{
	 			g_8x8_sad[blk8_id] = blk_cost;
				g_8x8_sad_mv[blk8_id][MVX] = mv_cand.x;
	 			g_8x8_sad_mv[blk8_id][MVY] = mv_cand.y;
//	 		}
#if 0
			round =4;
	 		//FME	1/4
			GetMVPredictor (8, 8, blk8_id, &mvp, 1);
	 		FPRINTF_ME(g_me_trace_fp, "1/4 pel search:  mvx: %d, mvy : %d\n", g_8x8_sad_mv[blk8_id][MVX], g_8x8_sad_mv[blk8_id][MVY]);
	 		step = 1;
	 		mv_cand.x = g_8x8_sad_mv[blk8_id][MVX];
			mv_cand.y = g_8x8_sad_mv[blk8_id][MVY];
	 
	 		blk_cost = BlkSearch (mb_x, mb_y, BLOCK_SIZE, BLOCK_SIZE, blk8_id, &mv_cand, &mv_blk, round, step, &mvp);
	 	//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y, 1);	 
			if( blk_cost < g_8x8_sad[blk8_id] )
	 		{
				g_8x8_sad[blk8_id] = blk_cost;
	 			g_8x8_sad_mv[blk8_id][MVX] = mv_blk.x;
	 			g_8x8_sad_mv[blk8_id][MVY] = mv_blk.y;
	 		}
#endif
		}
		g_mode_dcs_ptr->min_cost = g_8x8_sad[0]+g_8x8_sad[1]+g_8x8_sad[2]+g_8x8_sad[3]/* + 4*g_mode_dcs_ptr->lambda*/;
	}
	else if(mode == INTER_MB_16X8)
	{
		FPRINTF_ME (g_me_trace_fp, "\n**************************\nFME mode: 16x8 \n");
		for (j=0 ; j< 2; j++)
		{
			offset_x_mb = ((j*2) & 1) * 8;
			offset_y_mb = ((j*2) >> 1) * 8;
			
			//step = 2;
	 		mv_cand.x = g_16x8_sad_mv[j][MVX];
	 		mv_cand.y = g_16x8_sad_mv[j][MVY];
			FPRINTF_ME(g_me_trace_fp, "mv_x : %d,  mvy : %d \n", mv_cand.x, mv_cand.y);
 
	 		blk_cost = SubPixME (&mv_cand, mb_y, mb_x, 16, 8, offset_x_mb, offset_y_mb, j*2, &mvp_pool[j+5], is_out_range);
	 	//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y);	 
	// 		if( blk_cost < g_16x8_sad[j] )
	// 		{
	 			g_16x8_sad[j] = blk_cost;
				g_16x8_sad_mv[j][MVX] = mv_cand.x;
	 			g_16x8_sad_mv[j][MVY] = mv_cand	.y;
	// 		}
#if 0
	 		//FME	1/4
	 		FPRINTF_ME(g_me_trace_fp, "1/4 pel search:  mvx: %d, mvy : %d\n", g_8x8_sad_mv[j][MVX], g_8x8_sad_mv[j][MVY]);
			round =4;
	 		step = 1;
			GetMVPredictor (16, 8, j*2, &mvp, 1);
	 		mv_cand.x = g_16x8_sad_mv[j][MVX];
			mv_cand.y = g_16x8_sad_mv[j][MVY];
	 
	 		blk_cost = BlkSearch (mb_x, mb_y, 16, 8, j, &mv_cand, &mv_blk, round, step, &mvp);
	 	//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y, 1);	 
			if( blk_cost < g_16x8_sad[j] )
	 		{
				g_16x8_sad[j] = blk_cost;
	 			g_16x8_sad_mv[j][MVX] = mv_blk.x;
	 			g_16x8_sad_mv[j][MVY] = mv_blk.y;
	 		}
#endif
		}
		g_mode_dcs_ptr->min_cost = g_16x8_sad[0]+g_16x8_sad[1];
	}
	else
	{
		FPRINTF_ME (g_me_trace_fp, "\n**************************\nFME mode: 8x16 \n");
		for (j=0 ; j< 2; j++)
		{
			offset_x_mb = (j & 1) * 8;
			offset_y_mb = (j >> 1) * 8;
			
			//step = 2;
	 		mv_cand.x = g_8x16_sad_mv[j][MVX];
	 		mv_cand.y = g_8x16_sad_mv[j][MVY];
			FPRINTF_ME(g_me_trace_fp, "mv_x : %d,  mvy : %d \n", mv_cand.x, mv_cand.y);
 
	 		blk_cost = SubPixME (&mv_cand, mb_y, mb_x, 8, 16, offset_x_mb, offset_y_mb, j, &mvp_pool[j+7], is_out_range);
	 		//blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y);	 
	// 		if( blk_cost < g_8x16_sad[j] )
	 //		{
	 			g_8x16_sad[j] = blk_cost;
				g_8x16_sad_mv[j][MVX] = mv_cand.x;
	 			g_8x16_sad_mv[j][MVY] = mv_cand.y;
	 //		}
#if 0
	 		//FME	1/4
	 		FPRINTF_ME(g_me_trace_fp, "1/4 pel search:  mvx: %d, mvy : %d\n", g_8x8_sad_mv[j][MVX], g_8x8_sad_mv[j][MVY]);
			round =4;
	 		step = 1;
			GetMVPredictor (8, 16, j, &mvp, 1);
	 		mv_cand.x = g_8x16_sad_mv[j][MVX];
			mv_cand.y = g_8x16_sad_mv[j][MVY];
	 
	 		blk_cost = BlkSearch (mb_x, mb_y, 8, 16, j, &mv_cand, &mv_blk, round, step, &mvp);
	 	//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y, 1);	 
			if( blk_cost < g_8x16_sad[j] )
	 		{
				g_8x16_sad[j] = blk_cost;
	 			g_8x16_sad_mv[j][MVX] = mv_blk.x;
	 			g_8x16_sad_mv[j][MVY] = mv_blk.y;
	 		}
#endif
		}
		g_mode_dcs_ptr->min_cost = g_8x16_sad[0]+g_8x16_sad[1];
	}
	return 0;
}

int IME8(int mb_x, int mb_y, MOTION_VECTOR_T *mvp_pool)
{
	int	step;
	int	i, blk8_id, round, x, cost_8x8;
	uint16	blk_cost;
	int		mvd_cost;
	MOTION_VECTOR_T	mv_cand1[5];
	MOTION_VECTOR_T mv_cand;
	MOTION_VECTOR_T	mvp;
	MOTION_VECTOR_T	mv_blk;	
	FPRINTF_ME (g_me_trace_fp, "\nIME 8x8\n");
	FPRINTF_ME(g_me_trace_fp, "8x8 initial state:\n g_8x8_sad0 = %x, 8x8_blk0_mvx= %d, 8x8_blk0_mvy = %d\n", g_8x8_sad[0], g_8x8_sad_mv[0][MVX], g_8x8_sad_mv[0][MVY]);
	FPRINTF_ME(g_me_trace_fp, "g_8x8_sad1 = %x, 8x8_blk1_mvx= %d, 8x8_blk1_mvy = %d\n", g_8x8_sad[1], g_8x8_sad_mv[1][MVX], g_8x8_sad_mv[1][MVY]);
	FPRINTF_ME(g_me_trace_fp, "g_8x8_sad2 = %x, 8x8_blk2_mvx= %d, 8x8_blk2_mvy = %d\n", g_8x8_sad[2], g_8x8_sad_mv[2][MVX], g_8x8_sad_mv[2][MVY]);
	FPRINTF_ME(g_me_trace_fp, "g_8x8_sad3 = %x, 8x8_blk3_mvx= %d, 8x8_blk3_mvy = %d\n", g_8x8_sad[3], g_8x8_sad_mv[3][MVX], g_8x8_sad_mv[3][MVY]);

	mv_cand1[1].x = 0;
	mv_cand1[1].y = 0;
	mv_cand1[2].x = g_16x16_sad_mv[MVX];
	mv_cand1[2].y = g_16x16_sad_mv[MVY];

	if(mb_x == 0 && mb_y == 0)
		printf("");

	//******* start point selection *********//

	for(blk8_id = 0; blk8_id<4; blk8_id++)
	{
		step = 0;
		round = 1;
		GetMVPredictor (8, 8, blk8_id, &mvp, 1);
		mvp_pool[blk8_id+1].x = mvp.x;
		mvp_pool[blk8_id+1].y = mvp.y;

		FPRINTF_ME(g_me_trace_fp, "\n******************\nBLOCK %d", blk8_id);
		/*if(blk8_id!=0)
		{
			x = 5;
			mv_cand1[4].x = g_8x8_sad_mv[blk8_id-1][MVX];
			mv_cand1[4].y = g_8x8_sad_mv[blk8_id-1][MVY];
		}
		else
			x = 4;
		mv_cand1[3].x = mvp.x;
		mv_cand1[3].y = mvp.y;*/

		// Force IME16's mv as the only one candidate
		x= 1;

		for(i = 0; i < x; i++)
		{
			if(i == 2)
				printf("");
			if(!i)
			{
				mv_cand.x = g_16x16_sad_mv[MVX];
				mv_cand.y = g_16x16_sad_mv[MVY];
				//mv_cand.x = g_8x8_sad_mv[blk8_id][MVX];
				//mv_cand.y = g_8x8_sad_mv[blk8_id][MVY];
				FPRINTF_ME (g_me_trace_fp, "\nmv_cand_x: %d, mv_cand_y: %d", mv_cand.x, mv_cand.y);
			}
			else
			{
				mv_cand.x = mv_cand1[i].x;// + j&1 ? 8*4 : 0;
				mv_cand.y = mv_cand1[i].y;// + (j>>1)&1 ? 8*4 : 0;
				FPRINTF_ME (g_me_trace_fp, "\nmv_cand_x: %d, mv_cand_y: %d", mv_cand.x, mv_cand.y);
			}

			mv_cand.x = ((mv_cand.x + 2)>>2)<<2;
			mv_cand.y = ((mv_cand.y + 2)>>2)<<2;

			blk_cost = BlkSearch (mb_x, mb_y, BLOCK_SIZE, BLOCK_SIZE, blk8_id, &mv_cand, &mv_blk, round, step, &mvp, &mvd_cost);
		//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y,step);

			if(!i)
			{
				g_8x8_sad[blk8_id] = blk_cost;
				g_8x8_mvd_cost[blk8_id] = mvd_cost;
				g_8x8_sad_mv[blk8_id][MVX] = mv_blk.x;
				g_8x8_sad_mv[blk8_id][MVY] = mv_blk.y;
			}
			else if( blk_cost < g_8x8_sad[blk8_id])
			{
				g_8x8_sad[blk8_id] = blk_cost;
				g_8x8_mvd_cost[blk8_id] = mvd_cost;
				g_8x8_sad_mv[blk8_id][MVX] = mv_blk.x;
				g_8x8_sad_mv[blk8_id][MVY] = mv_blk.y;
			}
				g_step_count8 += 1;
		}
#if MEA_PATTERN
		{
			unsigned char s[100];
			sprintf(s, "Frame_Cnt %d MB %d : IME8 round 1 Block8_id=%d Best_MV(%d,%d) mvd_cost=0x%x", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, blk8_id, g_8x8_sad_mv[blk8_id][MVX], g_8x8_sad_mv[blk8_id][MVY], g_8x8_mvd_cost[blk8_id]);
			Print_best_sad_mv_cost(g_8x8_sad[blk8_id]-g_8x8_mvd_cost[blk8_id], s, g_ime_best_sad_fp);
		}
#endif
		round = FULL_SEH;
		FPRINTF_ME(g_me_trace_fp, "mvx: %d, mvy : %d\n", g_8x8_sad_mv[blk8_id][MVX], g_8x8_sad_mv[blk8_id][MVY]);
		//for(i = 0; i < 20; i++)
		{
			mv_cand.x = g_8x8_sad_mv[blk8_id][MVX];
			mv_cand.y = g_8x8_sad_mv[blk8_id][MVY];

			blk_cost = BlkSearch (mb_x, mb_y, BLOCK_SIZE, BLOCK_SIZE, blk8_id, &mv_cand, &mv_blk, round, step, &mvp, NULL);
		//	blk_cost += CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y,step);
//			FPRINTF_ME (g_me_trace_fp, "blkcost = %x, mvx= %d, mvy = %d\n", blk_cost, mv_blk.x, mv_blk.y);

			if( blk_cost < g_8x8_sad[blk8_id])
			{
				g_8x8_sad[blk8_id] = blk_cost;
				g_8x8_sad_mv[blk8_id][MVX] = mv_blk.x;
				g_8x8_sad_mv[blk8_id][MVY] = mv_blk.y;
			}
			//else
			//{
				FPRINTF_ME (g_me_trace_fp, "blkcost = %x, mvx= %d, mvy = %d\n", g_8x8_sad[blk8_id], g_8x8_sad_mv[blk8_id][MVX], g_8x8_sad_mv[blk8_id][MVY]);
				//break;
			//}
			g_step_count8 += 3;
		}
		g_steps_total8+= g_step_count8;

		//FPRINTF_ME(g_me_trace_fp, "1/2 pel search:  mvx: %d, mvy : %d\n", g_8x8_sad_mv[blk8_id][MVX], g_8x8_sad_mv[blk8_id][MVY]);
	}
#ifndef SINGLE_FME
	FME(mb_x,mb_y,INTER_MB_8X8);
#endif
	g_step_count += g_step_count8/4;
	cost_8x8 = g_8x8_sad[0] + g_8x8_sad[1] + g_8x8_sad[2] + g_8x8_sad[3] /*+ 4*g_mode_dcs_ptr->lambda*/;
	return cost_8x8;
}

int IME16x8(int mb_x, int mb_y, MOTION_VECTOR_T *mvp_pool)
{
	int	step, i;
	int blk8_id, round,cost;
	uint16	blk_cost;
	MOTION_VECTOR_T mv_cand;
	MOTION_VECTOR_T mv_cand1[5];
	MOTION_VECTOR_T	mvp;
	MOTION_VECTOR_T	mv_blk;

	step = 4;
	mv_cand1[0].x = g_16x16_sad_mv[0];
	mv_cand1[0].y = g_16x16_sad_mv[1];
	mv_cand1[1].x = 0;
	mv_cand1[1].y = 0;
	
	for (blk8_id = 0; blk8_id < 2; blk8_id++)
	{
		FPRINTF_ME (g_me_trace_fp, "\n**********************\n16x8 ME block: %d\n", blk8_id);

		mv_cand1[2].x = g_8x8_sad_mv[blk8_id*2][0];
		mv_cand1[2].y = g_8x8_sad_mv[blk8_id*2][1];
		mv_cand1[3].x = g_8x8_sad_mv[blk8_id*2+1][0];
		mv_cand1[3].y = g_8x8_sad_mv[blk8_id*2+1][1];
		GetMVPredictor (16, 8, blk8_id*2, &mvp, 1);
		mv_cand1[4].x = mvp.x;
		mv_cand1[4].y = mvp.y;
		mvp_pool[blk8_id+5].x = mvp.x;
		mvp_pool[blk8_id+5].y = mvp.y;
		round = 1;
		for(i = 0; i<5; i++)
		{
			mv_cand.x = mv_cand1[i].x;
			mv_cand.y = mv_cand1[i].y;

			blk_cost = BlkSearch (mb_x, mb_y, 16, 8, blk8_id, &mv_cand, &mv_blk, round, step, &mvp, NULL);

			if(blk_cost < g_16x8_sad[blk8_id])
			{
				g_16x8_sad[blk8_id]= blk_cost;
				g_16x8_sad_mv[blk8_id][MVX]= mv_blk.x;
				g_16x8_sad_mv[blk8_id][MVY]= mv_blk.y;
			}
		}
		g_step_count16_8 += 1;
		round = 4;
		for(i=0; i<20; i++)
		{
			mv_cand.x = g_16x8_sad_mv[blk8_id][MVX];
			mv_cand.y = g_16x8_sad_mv[blk8_id][MVY];
			blk_cost = BlkSearch (mb_x, mb_y, 16, 8, blk8_id, &mv_cand, &mv_blk, round, step, &mvp, NULL);

			if(blk_cost < g_16x8_sad[blk8_id])
			{
				g_16x8_sad[blk8_id]= blk_cost;
				g_16x8_sad_mv[blk8_id][MVX]= mv_blk.x;
				g_16x8_sad_mv[blk8_id][MVY]= mv_blk.y;
			}
			else 
			{
				break;
			}
			g_step_count16_8 += 3;
		}
	}
#ifndef SINGLE_FME
	FME(mb_x,mb_y,INTER_MB_16X8);
#endif
	g_step_count += g_step_count16_8/2;
	cost = g_16x8_sad[0] + g_16x8_sad[1]+ g_mode_dcs_ptr->lambda*2;
	return cost;
}

int IME8x16(int mb_x, int mb_y, MOTION_VECTOR_T *mvp_pool)
{
	int	step,i;
	int blk8_id, round, cost;
	uint16	blk_cost;
	MOTION_VECTOR_T mv_cand;
	MOTION_VECTOR_T mv_cand1[5];
	MOTION_VECTOR_T	mvp;
	MOTION_VECTOR_T	mv_blk;

	step = 4;
	mv_cand1[0].x = g_16x16_sad_mv[0];
	mv_cand1[0].y = g_16x16_sad_mv[1];
	mv_cand1[1].x = 0;
	mv_cand1[1].y = 0;
	for (blk8_id = 0; blk8_id < 2; blk8_id++)
	{
		FPRINTF_ME (g_me_trace_fp, "\n********************\n8x16 ME block: %d\n", blk8_id);
		mv_cand1[2].x = g_8x8_sad_mv[blk8_id][0];
		mv_cand1[2].y = g_8x8_sad_mv[blk8_id][1];
		mv_cand1[3].x = g_8x8_sad_mv[blk8_id+1][0];
		mv_cand1[3].y = g_8x8_sad_mv[blk8_id+1][1];
		GetMVPredictor (8, 16, blk8_id, &mvp, 1);
		mv_cand1[4].x = mvp.x;
		mv_cand1[4].y = mvp.y;
		mvp_pool[blk8_id+7].x = mvp.x;
		mvp_pool[blk8_id+7].y = mvp.y;

		round = 1;
		for(i = 0; i<5; i++)
		{
			mv_cand.x = mv_cand1[i].x;
			mv_cand.y = mv_cand1[i].y;

			blk_cost = BlkSearch (mb_x, mb_y, 8, 16, blk8_id, &mv_cand, &mv_blk, round, step, &mvp, NULL);

			if(blk_cost < g_8x16_sad[blk8_id])
			{
				g_8x16_sad[blk8_id]= blk_cost;
				g_8x16_sad_mv[blk8_id][MVX]= mv_blk.x;
				g_8x16_sad_mv[blk8_id][MVY]= mv_blk.y;
			}
		}
		g_step_count8_16+=1;
		round = 4;
		for(i=0; i<20; i++)
		{
			mv_cand.x = g_8x16_sad_mv[blk8_id][MVX];
			mv_cand.y = g_8x16_sad_mv[blk8_id][MVY];
			blk_cost = BlkSearch (mb_x, mb_y, 8, 16, blk8_id, &mv_cand, &mv_blk, round, step, &mvp, NULL);

			if(blk_cost < g_8x16_sad[blk8_id])
			{
				g_8x16_sad[blk8_id]= blk_cost;
				g_8x16_sad_mv[blk8_id][MVX]= mv_blk.x;
				g_8x16_sad_mv[blk8_id][MVY]= mv_blk.y;
			}
			else
			{
				break;
			}
			g_step_count8_16 += 3;
		}
	}
#ifndef SINGLE_FME
	FME(mb_x,mb_y,INTER_MB_8X16);
#endif
	g_step_count += g_step_count8_16/2;
	cost = g_8x16_sad[0] +g_8x16_sad[1]+ g_mode_dcs_ptr->lambda*2;
	return cost;
}

int IME16	(int mb_x,int mb_y, MOTION_VECTOR_T *mvp_pool)
{
	int	step;
	int	i, round;
	int	blk_cost;
	int	mvd_cost;
	int	i_mvc;
	MOTION_VECTOR_T	mvp;
	MOTION_VECTOR_T	mv_cand;
	MOTION_VECTOR_T	mv_blk;

	mvp.x = g_mode_dcs_ptr->pred_mv[1].x;
	mvp.y = g_mode_dcs_ptr->pred_mv[1].y;
	mvp_pool[0].x = g_mode_dcs_ptr->pred_mv[1].x;
	mvp_pool[0].y = g_mode_dcs_ptr->pred_mv[1].y;

	if(mb_x == 4)
		printf("");
	i_mvc = g_mode_dcs_ptr->pred_mv_num;


	//******* start point selection *********//

	//g_bestpoint = 1;
	FPRINTF_ME (g_me_trace_fp, "\n16x16ME\n");
	for( i = 0; i < i_mvc; i++ )
	{
		//mv_cand.x = ((g_mode_dcs_ptr->pred_mv[i].x+ 2)>>2)<<2;
		//mv_cand.y = (( g_mode_dcs_ptr->pred_mv[i].y + 2)>>2)<<2;
		// already rounded in H264Enc_mb_predict_mv(ignoreD=1), Derek
		mv_cand.x = g_mode_dcs_ptr->pred_mv[i].x;
		mv_cand.y = g_mode_dcs_ptr->pred_mv[i].y;

		round = SINGLE;
		step = 0;
		blk_cost = BlkSearch (mb_x, mb_y, MB_SIZE, MB_SIZE, BLK0_16x16, &mv_cand, &mv_blk, round, step, &mvp, &mvd_cost);
	//	cost = CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y,step);
	//	blk_cost += cost;
		FPRINTF_ME(g_me_trace_fp, "blkcost = %x, mvx= %d, mvy = %d\n", blk_cost, mv_blk.x, mv_blk.y);
		
		if( blk_cost < g_16x16_sad )
		{
			mv_cand.x = mv_blk.x;
			mv_cand.y = mv_blk.y;
			g_16x16_sad = blk_cost;
			g_16x16_mvd_cost = mvd_cost;
			g_16x16_sad_mv[MVX] = mv_blk.x;
			g_16x16_sad_mv[MVY] = mv_blk.y;
		}
//		Update8x8sad(blk_cost);

		g_step_count16 += 1;
	}
#if MEA_PATTERN
	{
		unsigned char s[100];
		sprintf(s, "Frame_Cnt %d MB %d : IME16 round 1 Best_MV(%d,%d) mvd_cost=0x%x", g_nFrame_enc, g_mb_num_x*mb_y+mb_x, g_16x16_sad_mv[MVX], g_16x16_sad_mv[MVY], g_16x16_mvd_cost);
		Print_best_sad_mv_cost(g_16x16_sad-g_16x16_mvd_cost, s, g_ime_best_sad_fp);
	}
#endif
//	Update8x8sad(blk_cost);

	//************** SD search with step 1***************//
	FPRINTF_ME (g_me_trace_fp, "\n\nSD search step=4, round = 4\n");
	mv_cand.x = g_16x16_sad_mv[MVX];
	mv_cand.y = g_16x16_sad_mv[MVY];
	step = 4;
	round = 4;
	blk_cost =BlkSearch (mb_x, mb_y, MB_SIZE, MB_SIZE, BLK0_16x16, &mv_cand, &mv_blk, round, step, &mvp, NULL);//update g_best point
	//cost = CostMVD (mv_blk.x, mv_blk.y, mvp.x, mvp.y,step);
	//blk_cost += cost;
	FPRINTF_ME(g_me_trace_fp, "blkcost = %x, mvx= %d, mvy = %d\n", blk_cost, mv_blk.x, mv_blk.y);

	if( blk_cost < g_16x16_sad )
	{
		mv_cand.x = mv_blk.x;
		mv_cand.y = mv_blk.y;
		g_16x16_sad = blk_cost;
		g_16x16_sad_mv[MVX] = mv_blk.x;
		g_16x16_sad_mv[MVY] = mv_blk.y;
	}
	g_step_count16 += 4;
//	Update8x8sad(blk_cost);
	FPRINTF_ME (g_me_trace_fp, "\nIME16 end: g_16sad: %x, [MVX]: %d, [MVY]: %d\n",g_16x16_sad, g_16x16_sad_mv[MVX], g_16x16_sad_mv[MVY]);
	FPRINTF_ME (g_me_trace_fp, "\n\n16x16 search step = %d", g_step_count16);
	g_steps_total16 += g_step_count16;
	g_step_count += g_step_count16;
#ifndef SINGLE_FME
	FME(mb_x,mb_y,INTER_MB_16X16);
#endif
	FPRINTF_ME (g_me_trace_fp, "\n**********************************************\n");
	return g_16x16_sad;	
}


void ModeDecisionModule()
{
//	int be_est_2p, cost16x8, cost8x16;
	int	intra_cost;
	int	top_avail = g_mode_dcs_ptr->top_avail;// (g_mea_reg_ptr->MEA_CFG2 >> 10) & 0x1;
	int	left_avail = g_mode_dcs_ptr->left_avail;//(g_mea_reg_ptr->MEA_CFG2 >> 11) & 0x1;
	int	standard = (g_glb_reg_ptr->VSP_CFG0 >> 8) & 0x7;
	int	mb_x = g_mode_dcs_ptr->mb_x ;//(g_mea_reg_ptr->MEA_CFG2) & 0xff;
	int	mb_y = g_mode_dcs_ptr->mb_y ;//(g_mea_reg_ptr->MEA_CFG2 >> 16) & 0xff;
	int	me_mode = INTRA_MB_16X16;
	int	is_i_frame = g_mode_dcs_ptr->is_i_frame;//(g_mea_reg_ptr->MEA_CFG1 >> 0) & 0x01;
//	int	th16 = 0; // (g_mea_reg_ptr->SAD_TH >> 0) & 0x1ffff;
//	int	th8 = 0; //(g_mea_reg_ptr->SAD_TH >> 16) & 0x1ffff;
	int	min_cost;
	int cost_8x8, cost16x16;
	MOTION_VECTOR_T mvp[8];		// For 16x16(1) and 8x8(4)

	IEA_SAD iea_sad;
	MEA_INFO mea_info;

	memset(&mea_info, 0, sizeof(MEA_INFO));
	MeaMBReset( );

	//PrintfSrcMB (mea_src_buf0);	
	//PrintfSrchWind (mea_window_buf);

	g_mb_num_x = ( g_glb_reg_ptr->VSP_CFG1 ) & 0x3ff;
	g_mb_num_y = ( g_glb_reg_ptr->VSP_CFG1 >> 12 ) & 0x3ff;
	//g_mb_dev = g_mea_reg_ptr->INTRA_SAD >> 16 & 0xffff;
	//mvp.x = ( g_mea_reg_ptr->MEA_CFG3 >> 1 ) & 0xff;
	//mvp.y = ( g_mea_reg_ptr->MEA_CFG3 >> 9 ) & 0xff;
	//mvp.x = g_mode_dcs_ptr->pred_mv[0].x;
	//mvp.y = g_mode_dcs_ptr->pred_mv[0].y;

	if( standard != JPEG )
	{
		g_mea_frame_width	= g_mb_num_x * BLOCK_SIZE >> 2;		//unit is word
		g_mea_frame_height	= g_mb_num_y * MB_SIZE;
	}
	else
	{
		int32 mcu_format = (( g_glb_reg_ptr->VSP_CFG1 >> 24 ) & 0x7 );

		switch( mcu_format )
		{
		case JPEG_FW_YUV420:
			g_mea_frame_width  *= 4; //word
			g_mea_frame_height *= MB_SIZE;
			break;
		case JPEG_FW_YUV422:
			g_mea_frame_width  *= 4; //word
			g_mea_frame_height *= BLOCK_SIZE;
			break;
		default:
			PRINTF("error format!");
		}
	}

	if( is_i_frame )
	{
 		if (g_standard == VSP_H264)				//need modify jin
 		{
 			IntraModeDecision (
 				g_mode_dcs_ptr, 
 				top_avail, 
 				left_avail, 
 				1,								//is luma
 				&intra_cost,
 				&g_mode_dcs_ptr->intra16_mode_y,
 				mb_x,
 				mb_y,
				&iea_sad
				);
 
			IntraModeDecision (
 				g_mode_dcs_ptr, 
 				top_avail, 
				left_avail, 
				0,								//not luma
 				&intra_cost,
				&g_mode_dcs_ptr->intra_mode_c,
 				mb_x,
				mb_y,
				&iea_sad
 				);
		}

		if (g_mode_dcs_ptr->intra16_mode_y == INTRA_L_4x4)
		{
			g_mode_dcs_ptr->mb_type = INTRA_MB_4X4;
		}
		else
		{
			g_mode_dcs_ptr->mb_type = INTRA_MB_16X16;
		}
#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_enc>FRAME_X)
#endif
		Print_IEA_SAD(&iea_sad);
		Print_IME_block(1);	// Intra no use, just print xx
#endif
	}
	else
	{
		int is_out_range[4][3];	// 0:original point, 1:horizontal, 2:vertical

		//***IME***//
		FPRINTF_ME (g_me_trace_fp, "********************ME Begin**********************");
		cost16x16= IME16 (mb_x, mb_y, mvp);

#ifdef IME8_EN
		cost_8x8 = IME8 (mb_x, mb_y, mvp);
#else
		me_mode = INTER_MB_16X16;
		min_cost = cost16x16;
#endif
		if(mb_x == 59)
			PRINTF("");

		/*be_est_2p = MBSAD2PartEst (cost_8x8, cost16x16, mvp);
		be_est_2p = 0;
 		if(be_est_2p)
 		{
 			cost16x8 = IME16x8(mb_x, mb_y, mvp);
			 			
			cost8x16 = IME8x16(mb_x, mb_y, mvp);
 		}
		GetMinMode(cost16x16, cost_8x8, cost16x8, cost8x16, &me_mode, &min_cost);*/
		GetMinMode(cost16x16, cost_8x8, 0xfffffff, 0xfffffff, &me_mode, &min_cost);
		g_mode_dcs_ptr->mb_type = me_mode;

#ifdef TV_OUT
#ifdef ONE_FRAME
		if(g_nFrame_enc>FRAME_X)
#endif
		Print_IME_block(0);	// Use IME MV, must be here
#endif

		// Store IME MV result for next MB, and restore cached mv
		if(me_mode == INTER_MB_16X16)
		{
			g_mode_dcs_ptr->left_blk_mv_ime[0].x =
			g_mode_dcs_ptr->left_blk_mv_ime[1].x =
			g_mode_dcs_ptr->left_blk_mv_ime[2].x =
			g_mode_dcs_ptr->left_blk_mv_ime[3].x = g_16x16_sad_mv[MVX];
			g_mode_dcs_ptr->left_blk_mv_ime[0].y = 
			g_mode_dcs_ptr->left_blk_mv_ime[1].y = 
			g_mode_dcs_ptr->left_blk_mv_ime[2].y = 
			g_mode_dcs_ptr->left_blk_mv_ime[3].y = g_16x16_sad_mv[MVY];
			mea_info.imv_x_blk0 =
			mea_info.imv_x_blk1 = 
			mea_info.imv_x_blk2 = 
			mea_info.imv_x_blk3 = (g_16x16_sad_mv[MVX] >>2);
			mea_info.imv_y_blk0 =
			mea_info.imv_y_blk1 = 
			mea_info.imv_y_blk2 = 
			mea_info.imv_y_blk3 = (g_16x16_sad_mv[MVY] >>2);
		}
		else
		{
			g_mode_dcs_ptr->left_blk_mv_ime[0].x =
			g_mode_dcs_ptr->left_blk_mv_ime[1].x = g_8x8_sad_mv[1][MVX];
			g_mode_dcs_ptr->left_blk_mv_ime[2].x =
			g_mode_dcs_ptr->left_blk_mv_ime[3].x = g_8x8_sad_mv[3][MVX];
			g_mode_dcs_ptr->left_blk_mv_ime[0].y =
			g_mode_dcs_ptr->left_blk_mv_ime[1].y = g_8x8_sad_mv[1][MVY];
			g_mode_dcs_ptr->left_blk_mv_ime[2].y = 
			g_mode_dcs_ptr->left_blk_mv_ime[3].y = g_8x8_sad_mv[3][MVY];
			mea_info.imv_x_blk0 = (g_8x8_sad_mv[0][MVX] >>2);
			mea_info.imv_x_blk1 = (g_8x8_sad_mv[1][MVX] >>2);
			mea_info.imv_x_blk2 = (g_8x8_sad_mv[2][MVX] >>2);
			mea_info.imv_x_blk3 = (g_8x8_sad_mv[3][MVX] >>2);
			mea_info.imv_y_blk0 = (g_8x8_sad_mv[0][MVY] >>2);
			mea_info.imv_y_blk1 = (g_8x8_sad_mv[1][MVY] >>2);
			mea_info.imv_y_blk2 = (g_8x8_sad_mv[2][MVY] >>2);
			mea_info.imv_y_blk3 = (g_8x8_sad_mv[3][MVY] >>2);
		}
		/*g_mode_dcs_ptr->ref_idx_a = g_mode_dcs_ptr->left_ref_idx;
		g_mode_dcs_ptr->left_blk_mv[0].x = g_mode_dcs_ptr->left_blk_mv_fme[0].x;
		g_mode_dcs_ptr->left_blk_mv[0].y = g_mode_dcs_ptr->left_blk_mv_fme[0].y;
		g_mode_dcs_ptr->left_blk_mv[1].x = g_mode_dcs_ptr->left_blk_mv_fme[1].x;
		g_mode_dcs_ptr->left_blk_mv[1].y = g_mode_dcs_ptr->left_blk_mv_fme[1].y;
		g_mode_dcs_ptr->left_blk_mv[2].x = g_mode_dcs_ptr->left_blk_mv_fme[2].x;
		g_mode_dcs_ptr->left_blk_mv[2].y = g_mode_dcs_ptr->left_blk_mv_fme[2].y;
		g_mode_dcs_ptr->left_blk_mv[3].x = g_mode_dcs_ptr->left_blk_mv_fme[3].x;
		g_mode_dcs_ptr->left_blk_mv[3].y = g_mode_dcs_ptr->left_blk_mv_fme[3].y;*/

#ifdef SINGLE_FME
		FME(mb_x,mb_y,me_mode, mvp, is_out_range);
#endif

#ifdef TV_OUT
		// Store Info of bottom two lines of curr MB buf
		if((me_mode==INTER_MB_16X16))
		{
			mea_info.total_cost_min = g_16x16_mvd_cost;
			mea_info.mv_valid = (1-is_out_range[0][2]);
			mea_info.mv_valid |= ((1-is_out_range[0][1]) << 1);
			mea_info.mv_valid |= ((1-is_out_range[0][2]) << 2);
			mea_info.mv_valid |= ((1-is_out_range[0][1]) << 3);
			mea_info.mv_valid |= ((1-is_out_range[0][2]) << 4);
			mea_info.mv_valid |= ((1-is_out_range[0][1]) << 5);
			mea_info.mv_valid |= ((1-is_out_range[0][2]) << 6);
			mea_info.mv_valid |= ((1-is_out_range[0][1]) << 7);
			mea_info.partition_mode = 0;
		}
		else
		{
			mea_info.total_cost_min = (g_8x8_mvd_cost[0]+g_8x8_mvd_cost[1]+g_8x8_mvd_cost[2]+g_8x8_mvd_cost[3]/*+4*g_mode_dcs_ptr->lambda*/);
			mea_info.mv_valid = (1-is_out_range[3][2]);
			mea_info.mv_valid |= ((1-is_out_range[3][1]) << 1);
			mea_info.mv_valid |= ((1-is_out_range[2][2]) << 2);
			mea_info.mv_valid |= ((1-is_out_range[2][1]) << 3);
			mea_info.mv_valid |= ((1-is_out_range[1][2]) << 4);
			mea_info.mv_valid |= ((1-is_out_range[1][1]) << 5);
			mea_info.mv_valid |= ((1-is_out_range[0][2]) << 6);
			mea_info.mv_valid |= ((1-is_out_range[0][1]) << 7);
			mea_info.partition_mode = 1;
		}
#ifdef ONE_FRAME
//		if(g_nFrame_enc>FRAME_X)
#endif
//		Print_IME_Src_MB(&mea_info);
#endif
		
		//****INTRA******//
#if 1
		if (g_standard == VSP_H264)
		{
			IntraModeDecision (
				g_mode_dcs_ptr, 
				top_avail, 
				left_avail, 
				1,								//is luma
				&intra_cost,
				&g_mode_dcs_ptr->intra16_mode_y,
				mb_x,
				mb_y,
				&iea_sad
				);
			
			/*compare the intra cost and inter cost*/
			if ( intra_cost < g_mode_dcs_ptr->min_cost )
			{				
				FPRINTF_ME (g_me_trace_fp, "MB is intra, intra_cost: %x, inter_cost: %x\n", intra_cost, g_mode_dcs_ptr->min_cost);
				
				if (g_mode_dcs_ptr->intra16_mode_y == INTRA_L_4x4)
				{
					g_mode_dcs_ptr->mb_type = INTRA_MB_4X4;
					g_mea_reg_ptr->MEA_CTL = (1 >> 31 | (INTRA_MB_4X4& 0x3) >>1);
				}
				else
				{
					g_mode_dcs_ptr->mb_type = INTRA_MB_16X16;
					g_mea_reg_ptr->MEA_CTL = (1 >> 31 | (INTRA_MB_16X16& 0x3) >>1);
				}

				g_mode_dcs_ptr->min_cost = intra_cost;
				
				IntraModeDecision (
					g_mode_dcs_ptr, 
					top_avail, 
					left_avail, 
					0,								//not luma
					&intra_cost,
					&g_mode_dcs_ptr->intra_mode_c,
					mb_x,
					mb_y,
					&iea_sad
					);
			}
#ifdef TV_OUT
#ifdef ONE_FRAME
			if(g_nFrame_enc>FRAME_X)
#endif
			Print_IEA_SAD(&iea_sad);
#endif
		}
#endif
		
		if(g_mode_dcs_ptr->mb_type == INTER_MB_16X16)
		{
			g_mea_reg_ptr->MEA_CTL = (1 >> 31 | (INTER_MB_16X16 & 0x3) >>1);
		}
		else if(g_mode_dcs_ptr->mb_type == INTER_MB_8X8)
		{
			g_mea_reg_ptr->MEA_CTL = (1 >> 31 | (INTER_MB_8X8 & 0x3) >>1);
		}

		/*if (g_mode_dcs_ptr ->mb_type == INTER_MB_16X16)
		{
			FPRINTF_ME (g_me_trace_fp, "inter mb_type: INTER_MB_16X16\n");
		}
		else if (g_mode_dcs_ptr ->mb_type == INTER_MB_16X8)
		{
			FPRINTF_ME (g_me_trace_fp, "inter mb_type: INTER_MB_16X8\n");
		}
		else if (g_mode_dcs_ptr ->mb_type == INTER_MB_8X16)
		{
			FPRINTF_ME (g_me_trace_fp, "inter mb_type: INTER_MB_8X16\n");
		}
		else
		{
			FPRINTF_ME (g_me_trace_fp, "inter mb_type: INTER_MB_8X8\n");
		}*/
	}

#ifdef TV_OUT
#ifdef ONE_FRAME
	if(g_nFrame_enc>FRAME_X)
#endif
	Print_IME_Src_MB(&mea_info);
#endif

	if (g_mode_dcs_ptr->mb_type <= INTRA_MB_4X4)
	{
#ifdef H264_ENC
		IntraPred_get_ref_mb (g_mode_dcs_ptr);	
#endif
	}
	else
	{
		if (g_mode_dcs_ptr->mb_type == INTER_MB_16X16)
		{
			g_mode_dcs_ptr->blk_mv[0].x = g_16x16_sad_mv[MVX];
			g_mode_dcs_ptr->blk_mv[0].y = g_16x16_sad_mv[MVY];
			g_mode_dcs_ptr->blk_mv[1].x = g_16x16_sad_mv[MVX];
			g_mode_dcs_ptr->blk_mv[1].y = g_16x16_sad_mv[MVY];
			g_mode_dcs_ptr->blk_mv[2].x = g_16x16_sad_mv[MVX];
			g_mode_dcs_ptr->blk_mv[2].y = g_16x16_sad_mv[MVY];
			g_mode_dcs_ptr->blk_mv[3].x = g_16x16_sad_mv[MVX];
			g_mode_dcs_ptr->blk_mv[3].y = g_16x16_sad_mv[MVY];

			// Skip Detect, if true, force residue to zero (conformance with decoder)
			if( (g_mode_dcs_ptr->min_cost-g_16x16_mvd_cost) < (MEA_SKIP_THRESHOLD<<3))
			{
				MOTION_VECTOR_T mvp_skip;

				// restore left MB's mv & ref
				g_mode_dcs_ptr->left_blk_mv[0].x = g_mode_dcs_ptr->left_blk_mv_fme[0].x;
				g_mode_dcs_ptr->left_blk_mv[0].y = g_mode_dcs_ptr->left_blk_mv_fme[0].y;
				g_mode_dcs_ptr->ref_idx_a = g_mode_dcs_ptr->left_ref_idx;
				//g_mode_dcs_ptr->ref_idx_c = g_mode_dcs_ptr->top_right_ref_idx;
				
				if( g_mode_dcs_ptr->ref_idx_a == -2 || g_mode_dcs_ptr->ref_idx_b == -2 ||
					( g_mode_dcs_ptr->ref_idx_a == 0 && g_mode_dcs_ptr->left_blk_mv[0].x == 0 && g_mode_dcs_ptr->left_blk_mv[0].y == 0 ) ||
					( g_mode_dcs_ptr->ref_idx_b == 0 && g_mode_dcs_ptr->top_blk_mv[0].x == 0 && g_mode_dcs_ptr->top_blk_mv[0].y == 0 ) )
				{
					mvp_skip.x = mvp_skip.y = 0;
				}
				else
				{
					//GetSkipMVPredictor(g_mb_cache_ptr, mvp_skip);
					GetMVPredictor( 16, 16, 0, &mvp_skip, 0);
				}

				if( (mvp_skip.x == g_16x16_sad_mv[MVX]) && (mvp_skip.y == g_16x16_sad_mv[MVY]) )
				{
					g_mode_dcs_ptr->mb_type = INTER_MB_SKIP;
				}
			}
			//g_mode_dcs_ptr->min_cost = g_16x16_sad;
		}
		else if (g_mode_dcs_ptr->mb_type == INTER_MB_16X8)
		{
			g_mode_dcs_ptr->blk_mv[0].x = g_16x8_sad_mv[0][MVX];
			g_mode_dcs_ptr->blk_mv[0].y = g_16x8_sad_mv[0][MVY];
			g_mode_dcs_ptr->blk_mv[1].x = g_16x8_sad_mv[0][MVX];
			g_mode_dcs_ptr->blk_mv[1].y = g_16x8_sad_mv[0][MVY];
			g_mode_dcs_ptr->blk_mv[2].x = g_16x8_sad_mv[1][MVX];
			g_mode_dcs_ptr->blk_mv[2].y = g_16x8_sad_mv[1][MVY];
			g_mode_dcs_ptr->blk_mv[3].x = g_16x8_sad_mv[1][MVX];
			g_mode_dcs_ptr->blk_mv[3].y = g_16x8_sad_mv[1][MVY];
			//g_mode_dcs_ptr->min_cost = g_16x8_sad[0] +g_16x8_sad[1];
		}
		else if (g_mode_dcs_ptr->mb_type == INTER_MB_8X16)
		{	
			g_mode_dcs_ptr->blk_mv[0].x = g_8x16_sad_mv[0][MVX];
			g_mode_dcs_ptr->blk_mv[0].y = g_8x16_sad_mv[0][MVY];
			g_mode_dcs_ptr->blk_mv[2].x = g_8x16_sad_mv[0][MVX];
			g_mode_dcs_ptr->blk_mv[2].y = g_8x16_sad_mv[0][MVY];
			g_mode_dcs_ptr->blk_mv[1].x = g_8x16_sad_mv[1][MVX];
			g_mode_dcs_ptr->blk_mv[1].y = g_8x16_sad_mv[1][MVY];
			g_mode_dcs_ptr->blk_mv[3].x = g_8x16_sad_mv[1][MVX];
			g_mode_dcs_ptr->blk_mv[3].y = g_8x16_sad_mv[1][MVY];
			//g_mode_dcs_ptr->min_cost = g_8x16_sad[0] + g_8x16_sad[1];
		}
		else
		{
			g_mode_dcs_ptr->blk_mv[0].x = g_8x8_sad_mv[0][MVX];
			g_mode_dcs_ptr->blk_mv[0].y = g_8x8_sad_mv[0][MVY];
			g_mode_dcs_ptr->blk_mv[1].x = g_8x8_sad_mv[1][MVX];
			g_mode_dcs_ptr->blk_mv[1].y = g_8x8_sad_mv[1][MVY];
			g_mode_dcs_ptr->blk_mv[2].x = g_8x8_sad_mv[2][MVX];
			g_mode_dcs_ptr->blk_mv[2].y = g_8x8_sad_mv[2][MVY];
			g_mode_dcs_ptr->blk_mv[3].x = g_8x8_sad_mv[3][MVX];
			g_mode_dcs_ptr->blk_mv[3].y = g_8x8_sad_mv[3][MVY];
			//g_mode_dcs_ptr->min_cost = g_8x8_sad[0] +g_8x8_sad[1] +g_8x8_sad[2] +g_8x8_sad[3];
		}
		
		FPRINTF_ME (g_me_trace_fp, "\ncost: %x\n0x= %d, 0y= %d,\nmv[1].x= %d, mv[1].y= %d,\nmv[2].x= %d, mv[2].y= %d,\nmv[3].x= %d, mv[3].y= %d, ", g_mode_dcs_ptr->min_cost, g_mode_dcs_ptr->blk_mv[0].x, g_mode_dcs_ptr->blk_mv[0].y, g_mode_dcs_ptr->blk_mv[1].x, g_mode_dcs_ptr->blk_mv[1].y, g_mode_dcs_ptr->blk_mv[2].x, g_mode_dcs_ptr->blk_mv[2].y, g_mode_dcs_ptr->blk_mv[3].x, g_mode_dcs_ptr->blk_mv[3].y);
		FPRINTF_ME (g_me_trace_fp, "\n cost %x", g_mode_dcs_ptr->min_cost);
		FPRINTF_ME (g_me_trace_fp, "\n///////////////////////////////////////////////////////////////////////\n");

#ifdef H264_ENC
		MC_get_ref_mb(g_mode_dcs_ptr);	
#else
		Mp4_Interpolation (g_mode_dcs_ptr);
#endif
	}	

//	PrintfMBResult (g_mode_dcs_ptr);

	if(g_mode_dcs_ptr->mb_type == INTER_MB_SKIP)
		memset(vsp_dct_io_0, 0, sizeof(uint32)*DCT_IO_BFR_SIZE);
	else
		hmea_fetch_err_mb_to_dctiobfr();
}

