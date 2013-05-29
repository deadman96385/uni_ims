#ifndef _VSP_IP_SYNTAX_H_
#define _VSP_IP_SYNTAX_H_

#include "vp8_basic.h"

typedef struct
{	
	volatile uint32 ISYN_CFG0;					// [1:0]:	segment_id
												// 1'b0
												// [9:3]:	cur_mb_y
												// [16:10]:	cur_mb_x
												// [17]:	is_skip
												// [21:18]:	mb_mode, 4b
												// 2'b0
												// [24]:	dc_diff
												// [26:25]:	ref_frame
												// [28:27]:	mv_split_mode
												// [30:29]:	intra_chroma_mode
												// [31]:	is_last_mb
	volatile uint32 ISYN_CFG1;					// [24:0]:	CBP_BLK
	volatile uint32 ISYN_CFG2;					//			INTRA								INTER
												// [3:0]:	intra_block_mode_0			[1:0]:		sub_mv_mode_0
												// [7:4]:	intra_block_mode_1			[3:2]:		sub_mv_mode_1
												// [11:8]:	intra_block_mode_2			[5:4]:		sub_mv_mode_2
												// [15:12]:	intra_block_mode_3			[7:6]:		sub_mv_mode_3
												// [19:16]:	intra_block_mode_4			[9:8]:		sub_mv_mode_4
												// [23:20]:	intra_block_mode_5			[11:10]:	sub_mv_mode_5
												// [27:24]:	intra_block_mode_6			[13:12]:	sub_mv_mode_6
												// [31:28]:	intra_block_mode_7			[15:14]:	sub_mv_mode_7
												//										[17:16]:	sub_mv_mode_8
												//										[19:18]:	sub_mv_mode_9
												//										[21:20]:	sub_mv_mode_10
												//										[23:22]:	sub_mv_mode_11
												//										[25:24]:	sub_mv_mode_12
												//										[27:26]:	sub_mv_mode_13
												//										[29:28]:	sub_mv_mode_14
												//										[31:30]:	sub_mv_mode_15
	volatile uint32 ISYN_CFG3;					//			INTRA								INTER
												// [3:0]:	intra_block_mode_8			32'd0
												// [7:4]:	intra_block_mode_9
												// [11:8]:	intra_block_mode_10
												// [15:12]:	intra_block_mode_11
												// [19:16]:	intra_block_mode_12
												// [23:20]:	intra_block_mode_13
												// [27:24]:	intra_block_mode_14
												// [31:28]:	intra_block_mode_15
	volatile uint32 ISYN_CFG4;					//			INTRA								INTER
												//										[11:0]:	sub_mv_row_0
												//										[23:12]:sub_mv_col_0
												//
												//
												//
												//
												//
												//
	volatile uint32 ISYN_CFG5;					// [11:0]:	sub_mv_row_1
												// [23:12]:	sub_mv_col_1
	volatile uint32 ISYN_CFG6;					// [11:0]:	sub_mv_row_2
												// [23:12]:	sub_mv_col_2
	volatile uint32 ISYN_CFG7;					// [11:0]:	sub_mv_row_3
												// [23:12]:	sub_mv_col_3
	volatile uint32 ISYN_CFG8;					// [11:0]:	sub_mv_row_4
												// [23:12]:	sub_mv_col_4
	volatile uint32 ISYN_CFG9;					// [11:0]:	sub_mv_row_5
												// [23:12]:	sub_mv_col_5
	volatile uint32 ISYN_CFG10;					// [11:0]:	sub_mv_row_6
												// [23:12]:	sub_mv_col_6
	volatile uint32 ISYN_CFG11;					// [11:0]:	sub_mv_row_7
												// [23:12]:	sub_mv_col_7
	volatile uint32 ISYN_CFG12;					// [11:0]:	sub_mv_row_8
												// [23:12]:	sub_mv_col_8
	volatile uint32 ISYN_CFG13;					// [11:0]:	sub_mv_row_9
												// [23:12]:	sub_mv_col_9
	volatile uint32 ISYN_CFG14;					// [11:0]:	sub_mv_row_10
												// [23:12]:	sub_mv_col_10
	volatile uint32 ISYN_CFG15;					// [11:0]:	sub_mv_row_11
												// [23:12]:	sub_mv_col_14
	volatile uint32 ISYN_CFG16;					// [11:0]:	sub_mv_row_12
												// [23:12]:	sub_mv_col_12
	volatile uint32 ISYN_CFG17;					// [11:0]:	sub_mv_row_13
												// [23:12]:	sub_mv_col_13
	volatile uint32 ISYN_CFG18;					// [11:0]:	sub_mv_row_14
												// [23:12]:	sub_mv_col_14
	volatile uint32 ISYN_CFG19;					// [11:0]:	sub_mv_row_15
												// [23:12]:	sub_mv_col_15
}VSP_ISYN_BUF_T;


typedef struct
{	
	volatile uint32 PPAL_CFG0;					// [1:0]:	ref_frame
												// [2]:		is_skip
												// [6:3]:	CBP_BLK of bottom 4 blocks
	volatile uint32 PPAL_CFG1;					// [3:0]:	intra_block_mode_12
												// [7:4]:	intra_block_mode_13
												// [11:8]:	intra_block_mode_14
												// [15:12]:	intra_block_mode_15
	volatile uint32 PPAL_CFG2;					// [11:0]:	sub_mv_row_12
												// [23:12]:	sub_mv_col_12
	volatile uint32 PPAL_CFG3;					// [11:0]:	sub_mv_row_13
												// [23:12]:	sub_mv_col_13
	volatile uint32 PPAL_CFG4;					// [11:0]:	sub_mv_row_14
												// [23:12]:	sub_mv_col_14
	volatile uint32 PPAL_CFG5;					// [11:0]:	sub_mv_row_15
												// [23:12]:	sub_mv_col_15
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