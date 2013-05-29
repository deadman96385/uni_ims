#ifndef _VSP_IP_SYNTAX_H_
#define _VSP_IP_SYNTAX_H_

//#include "vp8_basic.h"

typedef struct
{	
	volatile uint32 ISYN_CFG0;					// [6:0]:	cur_mb_x
												// [13:7]:	cur_mb_y
												// [14]:	is_skip
												// [15]:	is_intra
												// [16]:	mb_partition, 0:16x16, 1:intra_4x4 or inter_8x8
	volatile uint32 ISYN_CFG1;					//			INTRA								INTER
												// [31:0]:	Intra_pred_mode0			[9:0]:		mv_l0_0_x
												//										[18:10]:	mv_l0_0_y
	volatile uint32 ISYN_CFG2;					//			INTRA								INTER
												// [31:0]:	Intra_pred_mode1			[9:0]:		mv_l0_1_x
												//										[18:10]:	mv_l0_1_y
	volatile uint32 ISYN_CFG3;					//			INTRA								INTER
												// [1:0]:	chrom_pred_mode				[9:0]:		mv_l0_2_x
												//										[18:10]:	mv_l0_2_y
	volatile uint32 ISYN_CFG4;					//			INTRA								INTER
												//										[9:0]:		mv_l0_3_x
												//										[18:10]:	mv_l0_3_y
	volatile uint32 ISYN_CFG5;					// [5:0]:	QP
}VSP_ISYN_BUF_T;


typedef struct
{	
	volatile uint32 PPAL_CFG0;					// [2:0]:	mb_mode
												// [8:3]:	QP
												// [12:9]:	CBP_BLK of bottom 4 blocks
												// [13]:	is_skipped
												// [14]:	is_intra
												// [23:15]:	slice_nr
	volatile uint32 PPAL_CFG1;					// [3:0]:	intra_block_mode_12 or intra_16x16_pred_mode | ref_idx_2_l0
												// [7:4]:	intra_block_mode_13							 | ref_idx_3_l0
												// [11:8]:	intra_block_mode_14
												// [15:12]:	intra_block_mode_15
	volatile uint32 PPAL_CFG2;					// [11:0]:	mv_l0_12_y
												// [25:12]:	mv_l0_12_x
	volatile uint32 PPAL_CFG3;					// [11:0]:	mv_l0_13_y
												// [25:12]:	mv_l0_13_x
	volatile uint32 PPAL_CFG4;					// [11:0]:	mv_l0_14_y
												// [25:12]:	mv_l0_14_x
	volatile uint32 PPAL_CFG5;					// [11:0]:	mv_l0_15_y
												// [25:12]:	mv_l0_15_x
}VSP_PPA_LINE_BUF_T;


/*typedef struct
{	
	volatile uint32 MCA_CFG0;					// [1:0]:	ref_frame
												// [8:2]:	cur_mb_y
												// [15:9]:	cur_mb_x
												// [18:16]:	mv_mode
												// [20:19]:	mv_mode
	volatile uint32 MCA_CFG1;					// [11:0]:	sub_mv_col_0
												// [23:12]:	sub_mv_row_0
	volatile uint32 MCA_CFG2;					// [11:0]:	sub_mv_col_1
												// [23:12]:	sub_mv_row_1
	volatile uint32 MCA_CFG3;					// [11:0]:	sub_mv_col_2
												// [23:12]:	sub_mv_row_2
	volatile uint32 MCA_CFG4;					// [11:0]:	sub_mv_col_3
												// [23:12]:	sub_mv_row_3
	volatile uint32 MCA_CFG5;					// [11:0]:	sub_mv_col_4
												// [23:12]:	sub_mv_row_4
	volatile uint32 MCA_CFG6;					// [11:0]:	sub_mv_col_5
												// [23:12]:	sub_mv_row_5
	volatile uint32 MCA_CFG7;					// [11:0]:	sub_mv_col_6
												// [23:12]:	sub_mv_row_6
	volatile uint32 MCA_CFG8;					// [11:0]:	sub_mv_col_7
												// [23:12]:	sub_mv_row_7
	volatile uint32 MCA_CFG9;					// [11:0]:	sub_mv_col_8
												// [23:12]:	sub_mv_row_8
	volatile uint32 MCA_CFG10;					// [11:0]:	sub_mv_col_9
												// [23:12]:	sub_mv_row_9
	volatile uint32 MCA_CFG11;					// [11:0]:	sub_mv_col_10
												// [23:12]:	sub_mv_row_10
	volatile uint32 MCA_CFG12;					// [11:0]:	sub_mv_col_11
												// [23:12]:	sub_mv_row_11
	volatile uint32 MCA_CFG13;					// [11:0]:	sub_mv_col_12
												// [23:12]:	sub_mv_row_12
	volatile uint32 MCA_CFG14;					// [11:0]:	sub_mv_col_13
												// [23:12]:	sub_mv_row_13
	volatile uint32 MCA_CFG15;					// [11:0]:	sub_mv_col_14
												// [23:12]:	sub_mv_row_14
	volatile uint32 MCA_CFG16;					// [11:0]:	sub_mv_col_15
												// [23:12]:	sub_mv_row_15
}VSP_MCA_PAR_BUF_T;*/

#endif