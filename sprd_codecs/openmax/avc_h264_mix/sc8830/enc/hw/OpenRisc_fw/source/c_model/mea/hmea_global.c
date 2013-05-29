#include "video_common.h"
#include "mmcodec.h"
#include "hmea_mode.h"

/********************************************************************************
search window, only for Y, dual port buffer, 

intra prediction is based on original frame other than on reconstructed frame
the last 16 word is for intra prediction 
(first 8 for even mb_x number, and last 8 for odd mb_x number): 
4            2           2
left_y       left_u       null
********************************************************************************/
uint32	mea_window_buf[32*96 + 16];			//[32*64 + 16];	

uint32	mea_src_buf0[96];			//ping-pong source buffer
uint32	mea_src_buf1[96];			//ping-pong source buffer

uint32	mea_out_buf0[96];			//ping-pong mea out buffer
uint32	mea_out_buf1[96];			//ping-pong mea out buffer

int		mea_bur_index = 0;

int		g_fetch_width;

uint32 * g_ref_frame[2];
uint32 * g_src_frame[2];

uint32 * g_top_ref_pix;
uint32 * g_left_ref_pix;

int		g_mb_num_x;
int		g_mb_num_y;

uint32	g_mb_mean_y;
uint32	g_mb_dev;
int		g_mean_cal_done;
int		g_dev_cal_done;


int		g_sad_blk0_pe0;
int		g_sad_blk1_pe0;
int		g_sad_blk2_pe0;
int		g_sad_blk3_pe0;

int		g_sad_blk0_pe1;
int		g_sad_blk1_pe1;
int		g_sad_blk2_pe1;
int		g_sad_blk3_pe1;

int		g_sad_blk0_pe2;
int		g_sad_blk1_pe2;
int		g_sad_blk2_pe2;
int		g_sad_blk3_pe2;

int		g_sad_blk0_pe3;
int		g_sad_blk1_pe3;
int		g_sad_blk2_pe3;
int		g_sad_blk3_pe3;

// int			g_sad_mv0_blk0;
// int			g_sad_mv0_blk1;
// int			g_sad_mv0_blk2;
// int			g_sad_mv0_blk3;

int			g_min_cost_blk0;		//min cost in block0 in 16x16 search
int			g_min_cost_blk1;
int			g_min_cost_blk2;
int			g_min_cost_blk3;

int			g_sad_pe0;				//pe0 cost
int			g_sad_pe1;
int			g_sad_pe2;
int			g_sad_pe3;

int		g_mb_num_x;
int		g_mb_num_y;
int		g_standard;

//*************	HD	Project	**************//
uint32	g_mea_frame_width;	//word
uint32	g_mea_frame_height;

uint32	g_mea_frame_width;	//word
uint32	g_mea_frame_height;

int16	g_16x16_sad_mv[2];//sad and mv
int		g_16x16_sad;
int		g_16x16_mvd_cost;
int16	g_8x8_sad_mv[4][2];
int		g_8x8_sad[4];
int		g_8x8_mvd_cost[4];
int16	g_16x8_sad_mv[2][2];//sad and mv
int		g_16x8_sad[2];
int16	g_8x16_sad_mv[2][2];//sad and mv
int		g_8x16_sad[2];

//int16	g_mv[4][2];

uint8	g_halfpel_intpl[17*2*17*2/*4*/]; //

//		*     r
//		b	m

int diff[4][16][16];


ME_MODE_T		* g_me_mode_ptr;
MEA_FETCH_T		* g_mea_fetch_ptr;
MODE_DECISION_T	* g_mode_dcs_ptr;
VSP_GLOBAL		* g_vsp_glb_ptr;

int g_step_count;
int g_step_count16;
int g_step_count8;
int g_step_count16_8;
int g_step_count8_16;
int g_steps_total16;
int g_steps_total8;
int g_bestpoint;
int g_bestlast;

//****************************************//