/*analyse_mode.h*/
#ifndef ANALYSE_MODE_H
#define ANALYSE_MODE_H

#include "vsp_drv_sc8800g.h"

///////// Search method define///////////////
#define SINGLE_FME
#define IME8_EN
//#define SATD_COST
//////////////////////////////////////////

#define	PRE_FILTER_EN		0
#define FILTER_THERSH		0

#define SEARCH_RANGE_X		48//128		//max search range for X: 48, should be integer times of 4
#define SEARCH_RANGE_Y		24//96		//max search range for Y: 24

#define SEARCH_MAX_STEP		100

#define UNRESTRICTED_MV		1
#define FAST_MODE_DCS_EN	1

/*
#define JPEG_STD			0
#define MPEG4_STD			1
#define H263_STD			2
#define h264_STD			3
*/

#define I_VOP				0
#define P_VOP				1

#define INTRA_MB_16X16	0
#define INTRA_MB_4X4 	1
#define INTER_MB_SKIP	2
#define INTER_MB_16X16	3
#define INTER_MB_16X8	4
#define INTER_MB_8X16	5
#define INTER_MB_8X8	6

#define BLK8X8			3

/*for chroma*/
#define INTRA_C_DC		0
#define INTRA_C_HOR		1
#define INTRA_C_VER		2

/*for luma*/
#define INTRA_L_VER		0
#define INTRA_L_HOR		1
#define INTRA_L_DC		2
#define INTRA_L_4x4     3

#define LUMA_BLK8_COEFF_COST       4 //!< threshold for luma coeffs
#define CHROMA_COEFF_COST			7 //!< threshold for chroma coeffs, used to be 7
#define LUMA_MB_COEFF_COST			6 //!< threshold for luma coeffs of inter Macroblocks
#define MEA_SKIP_THRESHOLD			(1800>>3) // derek_2012-08-24, total 11-bit, but only config [10:3]

#define MEA_SEARCH_ROUND_16			8	// 0=unlimited, or lesst than 10
#define MEA_SEARCH_ROUND_8			3	// 0=unlimited, or lesst than 4

#define ABS(x) ((x) > 0 ? (x) : -(x))

//************************HD**************************//
#define	ME_CYCLE	193			

#define	BLK0_16x16	0
#define	BLK0_8x8	0
#define	BLK1_8x8	1
#define	BLK2_8x8	2
#define	BLK3_8x8	3
#define	BLK0_16x8	0
#define	BLK1_16x8	2
#define	BLK0_8x16	0
#define	BLK1_8x16	1

#define	MVX		0
#define	MVY		1
#define	SAD			2

#define	RIGHT		0
#define	TOP			1
#define	LEFT		2
#define	BELOW		3

#define	FULL_SEH	4
#define	HALF_SEH	2
#define	FULL_HALF	3
#define	SINGLE		1

#define	FIRST_THREE_STEP	3

#define	FINISH_SD			1		//finish search  with SD search finished
#define	FINISH_NO_SD		2		//finish search without SD search(no count left)

#define	WIND_W		32


//****************************************************//

typedef struct
{	
	MOTION_VECTOR_T		mv_blk16;				//for compute cost for block 16x16
	int32				cost_blk16;
	
	MOTION_VECTOR_T		mv_blk8[4];				//for compute cost for block 8x8
	uint32				cost_blk8[4];
	int32				cost_8x8;
	
	uint32				cost_blk16x8[2];		//16x8
	MOTION_VECTOR_T		mv_blk16x8[2];
	int32				cost_16x8;
	
	
	uint32				cost_blk8x16[2];		// 8x16
	MOTION_VECTOR_T		mv_blk8x16[2];
	int32				cost_8x16;
	
	int					MB_blk_type;		
	int32				min_cost;

	MOTION_VECTOR_T		mv_cache[6*5];
	int					ref_idx[6*5];	
} ME_MODE_T;

typedef struct 
{
	int			start_pos;
	int			burst_len;
	int			line_num;
	int			fetch_mb_x;
	int			fetch_mb_y;
	int			pre_filter_en;
	int			filt_thresh;
	int			is_i_frame;

	uint32		src_frame[2];
	uint32		ref_frame[2];

} MEA_FETCH_T;

typedef struct 
{
	int				is_i_frame;	
	
	int				fmd_en;						//fast mode decision enable for h.264, sad for half pixel is used for mode decision
	int				unrestricted_mv;			//0: mv can point to out of frame, 1: mv is limited in frame
	int				max_sch_step;				//max search step	
	int				rnd_ctr;			
	int				fast_skip_det;
	
	int				sch_range_x;				//horizontal search range
	int				sch_range_y;				//vertical search range
	
	int				mb_x;
	int				mb_y;
	
	//int			me_flag;					//
	int32			cost_thresh;				//search terminated of SAD threshold
	int32			intra_thresh;				//if cost of inter is larger than the thresh, do intra prediction;


	
	int				inter16_mode_cost_th;		//if three or four of neighbor(A,B,C) and co-locatedMB are inter16x16, 
												//and if current MB's inter16 cost less than the thresh, 
												//then other mode detection are all skipped.
												//the threshold is the average of the three or four MB's cost.  All modes are checked for those MB on frame boundary 
												
	uint32			lambda;						//parameter for compute cost function for mv and intra_pred type
	uint32			lambda2;
	
	int				pred_mv_num;
	MOTION_VECTOR_T	pred_mv[12];				//mv_pred candidate
	
	int				qp_per;						//for fast skip detection
	int				qp_rem;
	int				qp_per_c;
	int				qp_rem_c;
	int				left_avail;
	int				top_avail;
	int				tr_avail;
	int				tl_avail;
	
	int				ref_idx_a;					//-2: not available  -1: intra  0: reference frame 0
	int				ref_idx_b;
	int				ref_idx_c;
	int				ref_idx_d;
	
//	int				left_mb_type;				//0: skip, 1: inter16  2:intra  3: other 
//	int				top_mb_type;
//	int				tr_mb_type;
//	int				tl_mb_type;

	//这里需要加一个变量，来表示相邻宏块的4x4的帧内预测模式以及当前块的4x4帧内预测模式
	
	int				top_blk_4x4_mode[4];
	int				left_blk_4x4_mode[4];

	int				intra4x4_pred_mode[16];//in raster scan order

	MOTION_VECTOR_T	top_blk_mv[4];
	MOTION_VECTOR_T	left_blk_mv[4];
	MOTION_VECTOR_T	tr_blk_mv;
	MOTION_VECTOR_T	tl_blk_mv;
	
	int				mb_type;					//mb type
	
	int				intra16_mode_y;
	int				intra_mode_c;
	
	int				min_cost;
	
	MOTION_VECTOR_T blk_mv[4];

	int				skip_ctx;

	int				left_ref_idx;
//	int				top_right_ref_idx;
	MOTION_VECTOR_T	left_blk_mv_ime[4];	// this is for IME pipeline stage, available mv of left block is limited
	MOTION_VECTOR_T	left_blk_mv_fme[4]; // store left MB finalized MV
}MODE_DECISION_T;

typedef struct 
{
	int		standard;
	int		mcu_format;
	int		mb_num_x;
	int		mb_num_y;
} VSP_GLOBAL;

#define		MEA_REG_BASE		0x0

#define		MEA_FRM_TYPE_OFF	0x0
#define		MEA_CFG_OFF			0x4
#define		MEA_SRCH_RANGE_OFF	0x8
#define		MEA_MB_POS_OFF		0xC
#define		MEA_ME_THR_OFF		0x10
#define		MEA_PRE_NUM_OFF		0x14
#define		MEA_MVP0_OFF		0x18
#define		MEA_MVP1_OFF		0x1C
#define		MEA_MVP2_OFF		0x20		
#define		MEA_REF_IDX_OFF		0x24
#define		MEA_L0_MV_OFF		0x28
#define		MEA_L1_MV_OFF		0x2C
#define		MEA_T0_MV_OFF		0x30
#define		MEA_T1_MV_OFF		0x34
#define		MEA_TR_MV_OFF		0x38
#define		MEA_TL_MV_OFF		0x3C
#define		MEA_NEI_AVAIL_OFF	0x40
#define		MEA_ME_MODE_OFF		0x44
#define		MEA_MIN_COST_OFF	0x48
#define		MEA_MV_BLK0_OFF		0x4C
#define		MEA_MV_BLK1_OFF		0x50
#define		MEA_MV_BLK2_OFF		0x54
#define		MEA_MV_BLK3_OFF		0x58
#define		MEA_STATUS_OFF		0x5C
#define		MEA_CTR_OFF			0x60

typedef struct mea_reg
{
	/*these register for mode decision*/
	uint32			frm_type;		//[0]: I_vop: 0    P_vop: 1
	
	uint32			me_cfg;			//[24]: fast mode decision enable
									//[22:16]: lambda
									//[8]: unrestricted_mv
									//[7:4]: max search steps
									//[1]: rnt_ctr
									//[0]: fast_skip_det_en

	uint32			srch_range;		//[13:8]: vertical search range
									//[5:0]: horizontal search range

	uint32			mb_pos;			//[13:8]: mb vertical position, mb_y
									//[5:0]: mb horizontal position, mb_x

	uint32			me_thr;			//[31:16]: me termimate threshold, if (inter_sad <= thr), me terminated
									//[15:0]: inter16 thershold, if (inter16_sad < the thr), other mode is skipped

	uint32			pred_mv_num;	//[2:0]: predicted mv number

	uint32			pred_mv[3];		//[24:16]: mvp_y
									//[8:0]: mvp_x

	uint32			ref_idx;		//[25:24]: ref_idx_d
									//[17:16]: ref_idx_c
									//[9:8]: ref_idx_b
									//[1:0]: ref_idx_a

	uint32			left_blk_mv[2];	//[24:16]: mv_y
									//[8:0]: mv_x

	uint32			top_blk_mv[2];		//[24:16]: mv_y
									//[8:0]: mv_x

	uint32			tr_blk_mv;		//[24:16]: mv_y
									//[8:0]: mv_x

	uint32			tl_blk_mv;		//[24:16]: mv_y
									//[8:0]: mv_x

	uint32			nei_avail;		//[1]: top_avail
									//[0]: left_avail

	uint32			me_mode;		//[9:8]: intra c mode
									//[5:4]: intra y mode
									//[2:0]: mb_type

	uint32			min_cost;		//[16:0]: minimum cost for current mode

	uint32			mv_blk[4];		//[24:16]: mv_y
									//[8:0]: mv_x

	uint32			me_status;		//[3:0]: mea_ctr status

	uint32			me_ctr;			//[1]: mea done 1: done, 0; busy
									//[0]: mea start pulse

	/*these register for mea master*/


}MEA_REG_T;

#endif