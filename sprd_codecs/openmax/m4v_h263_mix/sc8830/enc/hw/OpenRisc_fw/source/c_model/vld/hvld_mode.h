/*hvld_mode.h*/
#ifndef _HVLD_MODE_H_
#define _HVLD_MODE_H_

#define VLD_FPRINTF	fprintf

#define H264_IPCM		0
#define H264_IMB16X16	1
#define H264_IMB4X4		2
#define H264_INTERMB	3

#define	LUMA_DC			0
#define LUMA_AC_I16		1
#define LUMA_AC			2
#define CHROMA_DC		3
#define CHROMA_AC		4
#ifdef LUMA_8x8_CABAC
#define LUMA_AC_8x8		5
#endif

#define LEV_TBUF_BASE_ADDR	176	

/*cabac*/
#define CABAC_CODED_FLAG	0
#define CABAC_SIG_LAST		1
#define CABAC_LEV_BIN0		2
#define CABAC_LEV_BIN_OTH	3
#define CABAC_GLM_PRX		5
#define CABAC_GLM_SFX		6
#define CABAC_LEV_SIGN		7


/************************************************************************/
/* 
	DCT/IO buffer organization for H264 dct coefficient
	coefficient:
	blk0~blk23 4x4 AC coefficient: [0, 191], one block uses 8 word
	luma 4x4 DC coefficient: [192, 199]
	U 2x2 DC coefficient: [200, 201]
	V 2x2 DC coefficient: [202: 203]
	
	non_zero coefficient flag: 
	blk0~blk23: [204, 227], one block uses one word
	luma 4x4 DC: 228
	U 2x2 DC: 229
	V 2x2 DC: 230
                                                                     */
/************************************************************************/
#define COEFF_LUMA_AC_BASE		0
#define COEFF_CHROMA_AC_BASE	128
#define COEFF_LUMA_DC_BASE		192
#define COEFF_CHROMA_DC_BASE	200
//#define COEFF_CR_DC_BASE		202

#define NZFLAG_LUMA_AC_BASE		204
#define NZFLAG_CHROMA_AC_BASE	220
#define NZFLAG_LUMA_DC_BASE		228
#define NZFLAG_CHROMA_DC_BASE	229
//#define NZFLAG_CR_DC_BASE		230



/************************************************************************/
/************************************************************************/

//typedef struct 
//{
//	uint32		hvld_ctr;			//[0]			start vld
//									//[1]			vld_done
//									//[2]			vld_error
//
//	uint32		mb_info;			//[1:0]			mb_type
//									//[13:8]		cbp
//									//[16]			left MB availability
//									//[17]			top MB availability

//	
//	/*neighbor block's nnz configured by software*/
//	uint32		top_nnz_y;			//[28:24]:top0, [20:16]: top1, [12:8]: top2, [4:0]: top3	
//	uint32		left_nnz_y;			//[28:24]:left0, [20:16]: left1, [12:8]: left2, [4:0]: left3
//	uint32		tl_nnz_cb;			//[28:24]:top0, [20:16]: top1, [12:8]: left0, [4:0]: left1
//	uint32		tl_nnz_cr;			//[28:24]:top0, [20:16]: top1, [12:8]: left0, [4:0]: left1
//
//	/*current mb's nnz for each 4x4 block*/
//	uint32		nnz_blk_0;			//[28:24]:block0, [20:16]: block1, [12:8]: block2, [4:0]: block3
//	uint32		nnz_blk_1;
//	uint32		nnz_blk_2;
//	uint32		nnz_blk_3;
//	uint32		nnz_blk_4;
//	uint32		nnz_blk_5;	
//	uint32		cbp_uv;				//[7:0]: bit0--blk0_u, bit1--blk1_u, ... bit7--blk3_v
//}HVLD_REG_T;

typedef struct 
{
	uint8	ctx_model[5];
} CONTEX_MOD_BIN_T;




#endif