/******************************************************************************
 ** File Name:      vsp_mea.h	                                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/11/2009                                                *
 ** Copyright:      2009 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP MEA Module Driver									  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 06/11/2009    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _VSP_MEA_H_
#define _VSP_MEA_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif 

/* MEA register map */
#define VSP_MEA_REG_BASE		(VSP_CTRL_BASE + 0x2000)
#define VSP_MEA_REG_SIZE		0xAC	//0x4C
	
#define MEA_CFG0_OFF			(0x00)
#define MEA_CFG1_OFF			(0x04)
#define MEA_CFG2_OFF			(0x08)
#define MEA_CFG3_OFF			(0x0C)
#define MEA_CFG4_OFF			(0x10)
#define MEA_CFG5_OFF			(0x14)
#define MEA_REF_CFG_OFF			(0x18)
#define MEA_REF_OFF				(0x1C)
#define MEA_MV_OFF				(0x20)
#define MEA_MV0_OFF				(0x24)
#define MEA_MV1_OFF				(0x28)
#define MEA_MV2_OFF				(0x2C)
#define MEA_MV3_OFF				(0x30)
#define MEA_UV_MV_OFF			(0x34)
#define MEA_CTRL_OFF			(0x38)
#define MEA_DEBUG_OFF			(0x3C)
#define MEA_INTRA_SAD_OFF		(0x40)
#define MEA_VDB_BUF_ST_OFF		(0x44)
#define MEA_MCU_NUM_OFF			(0x48)

#define MEA_RESULT_BIT	1

/*mea control register for MB*/
typedef struct mea_control_tag
{
	volatile uint32 MEA_CFG0;			//[31:25]: reserved
										//[24]: PRE_FLT_ENA, MEA source MB data pre-filtering enable, active high
										//[23:16]: PRE_FLT_TH, MEA pre-filter threshold value.
										//[12:8]: search_rangeY -- size of search window. maxium 63 pixels in vertical component.
										//[7:5]: reserved
										//[4:0]:search_rangeX -- size of search window. maxium 63 pixels in horizontal component.   
	
	volatile uint32 MEA_CFG1;			//[31:16]:mea_sad_thres -- the rhreshold of minimum sad value.
										//[15:8]: max_search_step -- max. search steps for ds algorithm
										//[7:6]: reserved
										//[5]: auto_en -- 0: mea controlled by sw; 1: used hardware pipeline
										//[4]: prediction_en -- nss search algorithm enable
										//[3]: mea_test -- 0: motion estimation will be started at(0,0), 1: start at the prediction point
										//[2]: 4mv_en -- 0: disalbe search in 8x8 mode; 1: enable 8x8 mode
										//[1]: intra_en -- 0: disable intra sad calculation; 1: enable intra sad calculation
										//[0]: me_enable -- 0: disable motion estimation; 1: enable motion estimaion

	volatile uint32 MEA_CFG2;			//[31:26]: reserved
										//[25:16]: mea_y_id -- the id of the current source mb in y direction
										//[9:0]: mea_x_id -- the id of the current source mb in x direction
      
	volatile uint32 MEA_CFG3;			//[31:16]: reserved
										//[15:9] mea_y_predmv -- the prediction motion vectorin y direction
										//[8]: reserved
										//[7:1]: mea_x_predmv -- the prediction motion vector in x direction
										//[0]: reserved

	volatile uint32 MEA_CFG4;			//[31:16]: increase_sad0 -- sad8 increased value
										//[15:0]: reduce_sad -- sad16(0,0) reduced value
      
	volatile uint32 MEA_CFG5;			//[31:16]: reserved
										//[15:0]: increase_sad -- intra sad increased value

	volatile uint32 REF_CFG;				//[31]: transfer_en -- transfer reference Y data enable
										//[30:20]: reserved
										//[19:16]: vdb_burst -- transfer width, the unit is word
										//[15:7]: reserved
										//[6:0]: vdb_line -- transfer height

	volatile uint32	REF_OFFSET;			//[31:29]: reserved
										//[28:16]: ref_addrY -- transfer top-left anddress x offset from picture top-left
										//[15:13]: reserved
										//[12:0]: ref_addrX -- transfer top-left anddress x offset from picture top-left, the unit is word
	
	volatile uint32 mea_result_mv_16x16;//[31:16]: mea_sad -- the initial SAD input and the output SAD
										//[15:8]: mea_y_mv -- the initial search motion vector in Y direction
										//[7:0]: mea_x_mv -- the initial search motion vector in X direction;
	volatile uint32 mea_result_mv_8x8[4];

	volatile uint32 MEA_UV_MV;				//[31:16]: reserved     
										//[15:8]:  mea_uv_y_mv -- u/v motion vector in y direction
										//[7:0]: mea_uv_x_mv -- u/v motion vector in x direction

	volatile uint32 MEA_CTL;			//[31]: mea_done -- 0: running; 1: idle
										//[30:4]: reserved
										//[3:1]: mea_result -- 00: intra mode, 01: 16x16 mode, 10: 8x8 mode
										//[0]: sw_ready -- 1: sw is ready for mea start, auto cleared by hw

	volatile uint32 MEA_DEBUG;			//[31:25]: reserved
										//[24]: mea_wflag
										//[23:18]: reserved//[17:16]: wea_wptr
										//[15:3]: reserved
										//[2:0]: mea_status	
	volatile uint32 INTRA_SAD;			//[31:16]: intra_threshold
										//[15:0]: intra sad
	volatile uint32 MEA_VDB_BUF_ST;		//[31:3]: reserved
										//[2]: MEA_JPEG_END
										//[1]: MEA_VDB_BUF1_RDY
										//[0]: MEA_VDB_BUF0_RDY
	volatile uint32 MEA_MCU_NUM;		//[15:0]:	MEA_MCU_NUM, MEA total MCU number

//	volatile uint32	SAD_TH;				//[16:31]: threshold for 8x8
									//[7:0] SAD threshold for 16x16
}VSP_MEA_REG_T;


/*mb type macro definition*/
#define MB_INTRA16X16		0
#define MB_INTRA4X4			1
#define MB_INTER_SKIP		2
#define MB_INTER16			3
#define MB_INTER16X8		4
#define MB_INTER8X16		5
#define MB_INTER8			6



/*inter blk type macro definition*/
#define BLK_INTER8			0
#define BLK_INTER8X4		1
#define BLK_INTER4X8		2
#define BLK_INTER4			3 

#define MEA_I_SLICE				0
#define MEA_P_SLICE				1


#define HMEA_PREFILT_CFG_OFF	(0x00)
#define HMEA_SEARCH_RANGE_OFF	(0x04)
#define HMEA_MB_POS_OFF			(0x08)
#define HMEA_PRED_MV_OFF		(0x0C)
#define HMEA_FETCH_CMD_OFF		(0x10)
#define HMEA_FETCH_ADDR_OFF		(0x14)
#define HMEA_MB_AVAIL_OFF		(0x18)
#define HMEA_MB_TYPE_OFF		(0x1C)
#define HMEA_ME_PARA_OFF		(0x20)
#define HMEA_ME_THRES_OFF		(0x24)
#define HMEA_INTRA_THRES_OFF	(0x28)
#define HMEA_LAMBDA_OFF			(0x2C)
#define HMEA_TOP_MV0_OFF		(0x30)
#define HMEA_TOP_MV1_OFF		(0x34)
#define HMEA_TOP_MV2_OFF		(0x38)
#define HMEA_TOP_MV3_OFF		(0x3C)
#define HMEA_LEFT_MV0_OFF		(0x40)
#define HMEA_LEFT_MV1_OFF		(0x44)
#define HMEA_LEFT_MV2_OFF		(0x48)
#define HMEA_LEFT_MV3_OFF		(0x4C)
#define HMEA_TR_MV_OFF			(0x50)
#define HMEA_TL_MV_OFF			(0x54)
#define HMEA_SUBBLK_MV0_OFF		(0x58)
#define HMEA_SUBBLK_MV1_OFF		(0x5C)
#define HMEA_SUBBLK_MV2_OFF		(0x60)
#define HMEA_SUBBLK_MV3_OFF		(0x64)
#define HMEA_SUBBLK_MV4_OFF		(0x68)
#define HMEA_SUBBLK_MV5_OFF		(0x6C)
#define HMEA_SUBBLK_MV6_OFF		(0x70)
#define HMEA_SUBBLK_MV7_OFF		(0x74)
#define HMEA_SUBBLK_MV8_OFF		(0x78)
#define HMEA_SUBBLK_MV9_OFF		(0x7C)
#define HMEA_SUBBLK_MV10_OFF	(0x80)
#define HMEA_SUBBLK_MV11_OFF	(0x84)
#define HMEA_SUBBLK_MV12_OFF	(0x88)
#define HMEA_SUBBLK_MV13_OFF	(0x8C)
#define HMEA_SUBBLK_MV14_OFF	(0x90)
#define HMEA_SUBBLK_MV15_OFF	(0x94)
#define HMEA_16X16_IPMODE_OFF	(0x98)
#define HMEA_NEI_IPMODE_4X4_OFF	(0x9C)
#define HMEA_4X4_IPMODE_0_OFF	(0xA0)
#define HMEA_4X4_IPMODE_1_OFF	(0xA4)
#define HMEA_CTRL_OFF			(0xA8)

/*block mv in register organization*/
//inter16 mv: mv_blk[0]
//inter16x8: mv_blk[0] and mv_blk[1];
//inter8x16: mv_blk[0] and mv_blk[1];
//mv start index for inter8x8:  mv_blk[blk8_id*4]
	//blk_inter8: mv_blk[blk8_id*4]
	//blk_inter8x4: mv_blk[blk8_id*4] and mv_blk[blk8_id*4+1]
	//blk_inter4x8: mv_blk[blk8_id*4] and mv_blk[blk8_id*4+1]
	//blk_interf4: mv_blk[blk8_id*4], mv_blk[blk8_id*4+1], mv_blk[blk8_id*4+2], mv_blk[blk8_id*4+3]

typedef struct h264_mea_control_tag
{
	uint32	prefilt_cfg;			//[7:0] :	prefilt_thesh
									//[8]   :	prefilt enable

	uint32	search_range;			//[4:0] :	horizontal search range
									//[12:8]:	vertical search range

	uint32	mb_pos;					//[9:0] :	current source mb x id
									//[25:16] :	current source mb y id

	uint32	pred_mv;				//[7:0] :	horizontal predicted mv
									//[15:8]:	vertical predicted mv

	uint32	fetch_cmd;				//[6:0] :	line number to be fetched
									//[3:0]:	transfer width, the unit is word

	uint32	fetch_addr;				//[12:0]:	horizontal position in one frame
									//[28:16]:	vertical position in one frame

	uint32	mb_avail;				//[0]:		left mb availability
									//[1]:		top mb availability	
									//[2]:		top-right availability
									//[3]:		top-left availability

	uint32	mb_type;				//decided mb_type and sub_mb_type
									//[2:0]:	mb type
									//[5:4]:	block type for 0 blk8x8
									//[9:8]:	block type for 1 blk8x8
									//[13:12]:	block type for 2 blk8x8
									//[17:16]:	block type for 3 blk8x8

	uint32	me_para;				//[0:7]:	max search step
									//[8:15]:	horizontal search range 
									//[16:23]:	vertical search range

	int32	cost_thresh;			//[15:0]	search terminated of SAD threshold

	int32	intra_thresh;			//[15:0]:	if cost of inter is less than the thresh, do intra prediction;

	uint32	lambda;					//[7:0]		
	
	uint32	top_blk_mv[4];			//[15:0]:	horizontal mv in quarter pixel precision
									//[31:16]:	vertical mv in quarter pixel precision
	uint32	left_blk_mv[4];

	uint32	tr_blk_mv;

	uint32	tl_blk_mv;
	
	uint32	mv_blk[16];				//mv for 16 blocks
									//[15:0]:	horizontal mv in quarter pixel precision
									//[31:16]:	vertical mv in quarter pixel precision
	
	uint32	ip_mode_mb;				//[2:0]:	intra prediction mode for intra16
									//[6:4]:	intra prediction mode for U
									//[10:8]:	intra prediction mode for V

	uint32	nei_ip_mode;			//neighbor block's intra prediction mode
									//[3:0]:	intra prediction mode for left block 0
									//[7:4]:	intra prediction mode for left block 1
									//[11:8]:	intra prediction mode for left block 2
									//[15:12]:	intra prediction mode for left block 3
									//[19:16]:	intra prediction mode for top block 0
									//[23:20]:	intra prediction mode for top block 1
									//[27:24]:	intra prediction mode for top block 2
									//[31:28]:	intra prediction mode for top block 3

	uint32	ip_blk4x4_mode0;		//[3:0]:	intra prediction mode for block 0
									//[7:4]:	intra prediction mode for block 1
									//[31:28]:	intra prediction mode for block 7

	uint32	ip_blk4x4_mode1;		//[3:0]:	intra prediction mode for block 8
									//[31:28]:	intra prediction mode for block 15


	uint32	mea_ctr;				//[0]:		mea_start_p
									//[1]:		mea_done
									//[2]:		slice_type

} VSP_HMEA_REG_T;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif 
/**---------------------------------------------------------------------------*/
// End 
#endif //_VSP_MEA_H_
