/*rvld_mode.h*/
#ifndef _RVLD_MODE_H_
#define _RVLD_MODE_H_

//#include "rvdec_basic.h"
#include "video_common.h"
#include "vsp_bsm.h"


#define INTRA4X4			0
#define INTRA16X16			1
#define INTER16X16			2


#define MAX_DEPTH   16
/*macro for vld*/
#define MAX_INTRA_QP_REGIONS 5

#define MAX_INTER_QP_REGIONS 7

/* 1296 */
#define MAX_CBP     (16*3*3*3*3)

#define MAX_8x8_DSC 16

#define MAX_0_LEVEL     4
#define MAX_123_LEVEL   3
#define MAX_456_LEVEL   2

/* 864 */
#define MAX_4x4_DSC (MAX_0_LEVEL * MAX_123_LEVEL * MAX_123_LEVEL * MAX_123_LEVEL * MAX_456_LEVEL * MAX_456_LEVEL * MAX_456_LEVEL)
  
/* 108 */
#define MAX_2x2_DSC (MAX_0_LEVEL * MAX_123_LEVEL * MAX_123_LEVEL * MAX_123_LEVEL)

#define MAX_EXTRA_LEVEL 23
#define MAX_EXTRA_BITS  8
#define MAX_LEVEL_DSC   (MAX_EXTRA_LEVEL+MAX_EXTRA_BITS+1)

typedef struct 
{
	uint32		min_reg[17];	
	uint32		max_reg[16];	//max value for code in this length
	uint32		base_addr[17];	//base address in huffman table for this length
	uint32		cnt_len[17];
//	uint32		valid_reg;		//valid register, whether has code in this length 
	uint32	*	code_tab_ptr;	//code table
}HUFF_DEC_S;

#define INTRA_CODE_SIZE			215
#define INTRA_MAX_REG_BASE		215


#define INTRA_CBP0_MAX_REG				INTRA_MAX_REG_BASE				//5 word
#define INTRA_CBP0_BASE_ADDR			(INTRA_MAX_REG_BASE	+ 5)		//8 word

#define INTRA_CBP1_MAX_REG				(INTRA_MAX_REG_BASE	+ 13)		//5 word	
#define INTRA_CBP1_BASE_ADDR			(INTRA_MAX_REG_BASE	+ 18)		//8 word

#define INTRA_L4X4DSC0_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 26)		//5 word
#define INTRA_L4X4DSC0_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 31)		//8 word
#define INTRA_L4X4DSC1_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 39)		//5 word
#define INTRA_L4X4DSC1_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 44)		//8 word
#define INTRA_L4X4DSC2_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 52)		//5 word
#define INTRA_L4X4DSC2_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 57)		//8 word

#define INTRA_C4X4DSC_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 65)		//5 word
#define INTRA_C4X4DSC_BASE_ADDR			(INTRA_MAX_REG_BASE	+ 70)		//8 word

//use same offset for one length for different case FOR 8X8_DSC
#define INTRA_8X8DSC00_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 78)		//2 word
#define INTRA_8X8DSC01_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 80)		//2 word
#define INTRA_8X8DSC02_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 82)		//2 word
#define INTRA_8X8DSC03_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 84)		//2 word		

//use same offset for one length for different case FOR 8X8_DSC
#define INTRA_8X8DSC10_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 86)		//2 word
#define INTRA_8X8DSC11_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 88)		//2 word
#define INTRA_8X8DSC12_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 90)		//2 word
#define INTRA_8X8DSC13_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 92)		//2 word	


#define INTRA_L2X2DSC0_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 94)		//5 word
#define INTRA_L2X2DSC0_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 99)		//8 word	
#define INTRA_L2X2DSC1_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 107)		//5 word
#define INTRA_L2X2DSC1_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 112)		//8 word


#define INTRA_C2X2DSC0_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 120)		//5 word
#define INTRA_C2X2DSC0_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 125)		//8 word	
#define INTRA_C2X2DSC1_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 133)		//5 word
#define INTRA_C2X2DSC1_BASE_ADDR		(INTRA_MAX_REG_BASE	+ 138)		//8 word	

//use same offset for one certain length for different case
#define INTRA_LEVDSC_MAX_REG_BASE		(INTRA_MAX_REG_BASE	+ 146)		//5 word


/******************************************************************
				inter table
*****************************************************************/
#define INTER_CODE_BASE					(INTRA_MAX_REG_BASE  + 151)

#define INTER_CODE_SIZE					171 //1035
#define INTER_MAX_REG_BASE				(INTER_CODE_BASE + INTER_CODE_SIZE)

/*inter case*/
#define INTER_CBP_MAX_REG				INTER_MAX_REG_BASE				//5 word
#define INTER_CBP_BASE_ADDR				(INTER_MAX_REG_BASE	+ 5)		//8 word

#define INTER_L4X4DSC_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 13)		//5 word
#define INTER_L4X4DSC_BASE_ADDR			(INTER_MAX_REG_BASE	+ 18)		//8 word

#define INTER_C4X4DSC_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 26)		//5 word
#define INTER_C4X4DSC_BASE_ADDR			(INTER_MAX_REG_BASE	+ 31)		//8 word

//use same offset for one length for different case FOR 8X8_DSC
#define INTER_8X8DSC0_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 39)		//2 word
#define INTER_8X8DSC1_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 41)		//2 word
#define INTER_8X8DSC2_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 43)		//2 word
#define INTER_8X8DSC3_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 45)		//2 word	

#define INTER_L2X2DSC0_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 47)		//5 word
#define INTER_L2X2DSC0_BASE_ADDR		(INTER_MAX_REG_BASE	+ 52)		//8 word	
#define INTER_L2X2DSC1_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 60)		//5 word
#define INTER_L2X2DSC1_BASE_ADDR		(INTER_MAX_REG_BASE	+ 65)		//8 word

#define INTER_C2X2DSC0_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 73)		//5 word
#define INTER_C2X2DSC0_BASE_ADDR		(INTER_MAX_REG_BASE	+ 78)		//8 word	
#define INTER_C2X2DSC1_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 86)		//5 word
#define INTER_C2X2DSC1_BASE_ADDR		(INTER_MAX_REG_BASE	+ 91)		//8 word	

#define INTER_LEVDSC_MAX_REG_BASE		(INTER_MAX_REG_BASE	+ 99)		//5 word


//8X8_DSC use same offset for one certain length in difference case
#define DSC8x8_CODE_BASE				0	
#define DSC8X8_LEN1_BASE				(DSC8x8_CODE_BASE + 0)		//max number: 1
#define DSC8X8_LEN2_BASE				(DSC8x8_CODE_BASE + 1)		//max number: 2
#define DSC8X8_LEN3_BASE				(DSC8x8_CODE_BASE + 3)		//max number: 6	
#define DSC8X8_LEN4_BASE				(DSC8x8_CODE_BASE + 9)		//max number: 11	
#define DSC8X8_LEN5_BASE				(DSC8x8_CODE_BASE + 20)		//max number: 9	
#define DSC8X8_LEN6_BASE				(DSC8x8_CODE_BASE + 29)		//max number: 6	
#define DSC8X8_LEN7_BASE				(DSC8x8_CODE_BASE + 35)		//max number: 6	
#define DSC8X8_LEN8_BASE				(DSC8x8_CODE_BASE + 41)		//max number: 2

//LEV_DSC use same offset for one certain length in difference case
#define DSCLEV_CODE_BASE				0							//byte offset
#define DSCLEV_LEN1_BASE				(DSCLEV_CODE_BASE + 0)		//max number: 1
#define DSCLEV_LEN2_BASE				(DSCLEV_CODE_BASE + 1)		//max number: 1
#define DSCLEV_LEN3_BASE				(DSCLEV_CODE_BASE + 2)		//max number: 2
#define DSCLEV_LEN4_BASE				(DSCLEV_CODE_BASE + 4)		//max number: 2
#define DSCLEV_LEN5_BASE				(DSCLEV_CODE_BASE + 6)		//max number: 2
#define DSCLEV_LEN6_BASE				(DSCLEV_CODE_BASE + 8)		//max number: 3
#define DSCLEV_LEN7_BASE				(DSCLEV_CODE_BASE + 11)		//max number: 3
#define DSCLEV_LEN8_BASE				(DSCLEV_CODE_BASE + 14)		//max number: 3
#define DSCLEV_LEN9_BASE				(DSCLEV_CODE_BASE + 17)		//max number: 4
#define DSCLEV_LEN10_BASE				(DSCLEV_CODE_BASE + 21)		//max number: 6
#define DSCLEV_LEN11_BASE				(DSCLEV_CODE_BASE + 27)		//max number: 7
#define DSCLEV_LEN12_BASE				(DSCLEV_CODE_BASE + 34)		//max number: 6
#define DSCLEV_LEN13_BASE				(DSCLEV_CODE_BASE + 40)		//max number: 7
#define DSCLEV_LEN14_BASE				(DSCLEV_CODE_BASE + 47)		//max number: 6
#define DSCLEV_LEN15_BASE				(DSCLEV_CODE_BASE + 53)		//max number: 2
#define DSCLEV_LEN16_BASE				(DSCLEV_CODE_BASE + 55)		//max number: 18

//#define INTRA_DSC8X8_OFFSET				3024
//#define INTER_DSC8X8_OFFSET				1512


#define INTRA_LEV_CODE_BASE				196
#define INTER_LEV_CODE_BASE				152

//#define HUFF_TAB_SIZE					(INTRA_CODE_SIZE + INTRA_MAX_BASE_SIZE + INTER_CODE_SIZE + INTER_MAX_BASE_SIZE)  //641

//#define CACHE_TAG_SIZE					14

#define MAX_DSC8X8_NUM					44		//(43+3)/4 * 4
#define MAX_DSCLEV_NUM					76		//(73+3)/4*4


#define	LUMA_DC_BLK						0
#define LUMA_AC_BLK						1
#define CHROMA_AC_BLK					2 

#define DSC_CBP							0
#define DSC_8X8							1
#define DSC_4X4							2
#define DSC_2X2							3
#define DSC_LEV							4

#define TAG_BASE_ADDR					641


/************************************************************************/
/* 
DCT/IO buffer organization for H264 dct coefficient
coefficient:
blk0~blk23 4x4 AC coefficient: [0, 191], one block uses 8 word
luma 4x4 DC coefficient: [192, 199]

  non_zero coefficient flag: 
  blk0~blk23: [204, 227], one block uses one word
  luma 4x4 DC: 228
*/
/************************************************************************/
#define COEFF_LUMA_AC_BASE		0
#define COEFF_CHROMA_AC_BASE	128
#define COEFF_LUMA_DC_BASE		192
#define COEFF_CHROMA_DC_BASE	200

#define NZFLAG_LUMA_AC_BASE		204
#define NZFLAG_CHROMA_AC_BASE	220
#define NZFLAG_LUMA_DC_BASE		228
#define NZFLAG_CHROMA_DC_BASE	229
#define NZFLAG_CR_DC_BASE		230

typedef struct 
{
	uint32	max_reg_len1;		//[1]: valid	[0]: max code for length 1
	uint32	max_reg_len2;		//[2]: valid	[1:0]: max code for length 2
	uint32	max_reg_len3;		//[3]: valid	[2:0]: max code for length 3
	uint32	max_reg_len4;		//[4]: valid	[3:0]: max code for length 4
	uint32	max_reg_len5;		//[5]: valid	[4:0]: max code for length 5
	uint32	max_reg_len6;		//[6]: valid	[5:0]: max code for length 6
	uint32	max_reg_len7;		//[7]: valid	[6:0]: max code for length 7
	uint32	max_reg_len8;		//[8]: valid	[7:0]: max code for length 8
	uint32	max_reg_len9;		//[9]: valid	[8:0]: max code for length 9
	uint32	max_reg_len10;		//[10]: valid	[9:0]: max code for length 10
	uint32	max_reg_len11;		//[11]: valid	[10:0]: max code for length 11
	uint32	max_reg_len12;		//[12]: valid	[11:0]: max code for length 12
	uint32	max_reg_len13;		//[13]: valid	[12:0]: max code for length 13
	uint32	max_reg_len14;		//[14]: valid	[13:0]: max code for length 14
	uint32	max_reg_len15;		//[15]: valid	[14:0]: max code for length 15
}MAX_REG_ARR_T;

typedef struct 
{
	MAX_REG_ARR_T c84_reg_arr;			//used for cbp and 8x8_dsc and 4x4_dsc
	MAX_REG_ARR_T dsc2x2_reg_arr0;		//used for 2x2_dsc for L4 and L5
	MAX_REG_ARR_T dsc2x2_reg_arr1;		//used for 2x2_dsc for L6
	MAX_REG_ARR_T intra_lev_reg_arr;	//used for level decoding
	MAX_REG_ARR_T inter_lev_reg_arr;	//used for level decoding
}RVLD_MODE_T;


#endif