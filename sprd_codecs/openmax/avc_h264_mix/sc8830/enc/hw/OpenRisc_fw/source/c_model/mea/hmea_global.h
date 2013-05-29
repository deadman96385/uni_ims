#ifndef HMEA_GLOBAL_H
#define HMEA_GLOBAL_H

#include <stdio.h>
#include "hmea_mode.h"
#include "tv_mea.h"

extern uint32	mea_window_buf[32*96 + 16];
#define INTRA_REF_BASE_ADDR		32*96

extern uint32	mea_src_buf0[96];
extern uint32	mea_src_buf1[96];

extern uint32	mea_out_buf0[96];
extern uint32	mea_out_buf1[96];

extern int		g_fetch_width;
extern uint32	g_nFrame_enc;

extern uint32	g_mb_mean_y;

extern int		g_sad_blk0_pe0;
extern int		g_sad_blk1_pe0;
extern int		g_sad_blk2_pe0;
extern int		g_sad_blk3_pe0;

extern int		g_sad_blk0_pe1;
extern int		g_sad_blk1_pe1;
extern int		g_sad_blk2_pe1;
extern int		g_sad_blk3_pe1;

extern int		g_sad_blk0_pe2;
extern int		g_sad_blk1_pe2;
extern int		g_sad_blk2_pe2;
extern int		g_sad_blk3_pe2;

extern int		g_sad_blk0_pe3;
extern int		g_sad_blk1_pe3;
extern int		g_sad_blk2_pe3;
extern int		g_sad_blk3_pe3;

extern int			g_sad_mv0_blk0;
extern int			g_sad_mv0_blk1;
extern int			g_sad_mv0_blk2;
extern int			g_sad_mv0_blk3;

extern int			g_min_cost_blk0;
extern int			g_min_cost_blk1;
extern int			g_min_cost_blk2;
extern int			g_min_cost_blk3;

extern int			g_sad_pe0;
extern int			g_sad_pe1;
extern int			g_sad_pe2;
extern int			g_sad_pe3;

extern uint32		*g_ref_frame[2];
extern uint32		*g_src_frame[2];

extern int		g_mb_num_x;
extern int		g_mb_num_y;

extern int		g_standard;
extern FILE 	*	g_me_trace_fp;
//extern FILE		*	g_me_intpl_fp;
extern	int		mea_bur_index;

extern uint32	g_mb_mean_y;
extern uint32	g_mb_dev;
extern int		g_mean_cal_done;
extern int		g_dev_cal_done;

//*************	HD	Project	**************//
extern uint32	g_mea_frame_width;	//word
extern uint32	g_mea_frame_height;

extern int16	g_16x16_sad_mv[2];//sad and mv
extern int		g_16x16_sad;
extern int		g_16x16_mvd_cost;
extern int16	g_8x8_sad_mv[4][2];
extern int		g_8x8_sad[4];
extern int		g_8x8_mvd_cost[4];
extern int16	g_16x8_sad_mv[2][2];//sad and mv
extern int		g_16x8_sad[2];
extern int16	g_8x16_sad_mv[2][2];//sad and mv
extern int		g_8x16_sad[2];

extern uint8	g_halfpel_intpl[17*17*3];
extern int diff[4][16][16];

extern ME_MODE_T		* g_me_mode_ptr;
extern MEA_FETCH_T		* g_mea_fetch_ptr;
extern MODE_DECISION_T	* g_mode_dcs_ptr;
extern VSP_GLOBAL		* g_vsp_glb_ptr;

extern int g_step_count;
extern int g_step_count16;
extern int g_step_count16_8;
extern int g_step_count8_16;
extern int g_step_count8;
extern int g_steps_total16;
extern int g_steps_total8;
extern int g_bestpoint;
extern int g_bestlast;
extern FILE *  g_me_pred_mv_fp;


//****************************************//
void ModeDecisionModule();

void ConfigureMeaFetch (int mb_y, int mb_x, int is_i_frame);

void MeaMaster ();

uint8 PreFilter (uint8 x0, uint8 x1, uint8 x2, uint8 x3, int thresh);

void GetMVPredictor( int blk_width, int blk_height, int blk_idx, MOTION_VECTOR_T * mvp, uint8 ignoreD);

int CostMVD (int mv_x, int mv_y, int mvp_x, int mvp_y);

void TraceInit();

void IntraPred_get_ref_mb (MODE_DECISION_T * mode_dcs_ptr);
void IntraPred_get_ref_blk (uint8 blkIdx);

void OutSrcMBToFrame (uint32 * mea_src_buf, int mb_x, int mb_y, int frame_width);

int hmea_fetch_err_mb_to_dctiobfr(void);

void MC_get_ref_mb(MODE_DECISION_T * mode_dcs_ptr);

void Mp4Enc_FetchErrMB2DctBuf ();

void hmea_Init();
void me_mc_luma (int32 start_pos_x, int32 start_pos_y, int32 blk_size, uint8 *rec_blk_ptr);
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
			 int					is_out_range[4][3]
			 );

void GetMinCost (
				 int	rd_cost_0,
				 int	rd_cost_1, 
				 int	rd_cost_2, 
				 int	rd_cost_3, 
				 int	rd_cost_4, 
				 int *	min_pos_ptr, 
				 int *	min_cost_ptr
				 );

void PEArray(
				int mb_x, 
				int mb_y, 
				int blk_width, 
				int blk_height, 
				int blk8_id, 
				MOTION_VECTOR_T * cent_mv_ptr, 
				int step_len, 
				int is_sp);

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
						   );

void ProcessElement(
						uint32 ref_top0,
						uint32 ref_top1, 
						uint32 ref_bot0, 
						uint32 ref_bot1, 
						uint32 src_word, 
						int byte_offset, 
						int dx, 
						int dy, 
						int col_cnt, 
						int row_cnt, 
						int pe_num, 
						int pe_active
						);

void ProcessElement_SATD(
								uint32 ref_top0,
								uint32 ref_top1, 
								uint32 ref_bot0, 
								uint32 ref_bot1, 
								uint32 src_word, 
								int byte_offset, 
								int dx, 
								int dy, 
								int col_cnt, 
								int row_cnt, 
								int pe_num, 
								int pe_active
								);

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
					);

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
						);
#endif