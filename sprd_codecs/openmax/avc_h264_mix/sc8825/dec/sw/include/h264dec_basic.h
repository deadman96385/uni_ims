/******************************************************************************
** File Name:      h264dec_basic.h                                            *
** Author:         Xiaowei Luo                                               *
** DATE:           12/06/2007                                                *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------* 
** DATE          NAME            DESCRIPTION                                 * 
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _H264DEC_BASIC_H_
#define _H264DEC_BASIC_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C" 
{
#endif

#define CTS_PROTECT	//added by xweiiluo @20120818

#define DISPLAY_LIST_SIZE	10

#define DEC_REF_PIC_MARKING_COMMAND_NUM	50

#if 1//hw	
#define MAX_REF_FRAME_NUMBER	16
#define	MAX_REF_FRAME_BUF_NUM	16  //5

#define MAX_PPS	256
#define MAX_SPS	32
#else
#define MAX_REF_FRAME_NUMBER	5
#define	MAX_REF_FRAME_BUF_NUM	5  //5

#define MAX_PPS	10
#define MAX_SPS	5
#endif

#ifdef _VSP_LINUX_
#define SIZE_SLICE_GROUP_ID		960 //512x480
#else
#define SIZE_SLICE_GROUP_ID		3600	//for 720P size// should be sufficient for HUGE pictures, need one int per MB in a picture
#endif                                                                     //MODIFIED By Leon.Li@20120925

#define INVALID_REF_ID	(-135792468)

#define LIST_NOT_USED -1 //FIXME rename?
#define PART_NOT_AVAIL	-2
#define MB_SIZE						16 
#define MB_CHROMA_SIZE				8
#define MB_LUMA_CACHE_WIDTH			16
#define MB_LUMA_CACHE_HEIGHT		16
#define MB_CHROMA_CACHE_WIDTH		8
#define MB_CHROMA_CACHE_HEIGHT		8
#define Y_EXTEND_SIZE				24
#define UV_EXTEND_SIZE				(Y_EXTEND_SIZE / 2) //(16)

//for ref_id, nnz, mv, mvd
#define CTX_CACHE_WIDTH		12
#define CTX_CACHE_WIDTH_X1	12
#define CTX_CACHE_WIDTH_X2	24
#define CTX_CACHE_WIDTH_X3	36
#define CTX_CACHE_WIDTH_X4	48
#define CTX_CACHE_WIDTH_X5	60
#define CTX_CACHE_WIDTH_X6	72
#define CTX_CACHE_WIDTH_X7	84

#define CTX_CACHE_WIDTH_PLUS4		16
#define CTX_CACHE_WIDTH_PLUS4_X2	32

//for mv
#define MV_CACHE_WIDTH			    12 //6
#define MV_CACHE_WIDTH_X1			12 //6
#define MV_CACHE_WIDTH_X2			24 //12
#define MV_CACHE_WIDTH_X3			36 //18
#define MV_CACHE_WIDTH_X4			48 //24
#define MV_CACHE_WIDTH_X5			60 //30
#define MV_CACHE_WIDTH_X6			72 //36
#define MV_CACHE_WIDTH_X7			85 //42
#define MV_CACHE_WIDTH_PLUS1		7
#define MV_CACHE_WIDTH_PLUS1_X2	    14	//(CONTEXT+1)*2

//mb size
#define MB_SIZE_X0			0
#define MB_SIZE_X1			16
#define MB_SIZE_X2			32
#define MB_SIZE_X3			48
#define MB_SIZE_X4			64
#define MB_SIZE_X5			80
#define MB_SIZE_X6			96
#define MB_SIZE_X7			112
#define MB_SIZE_X8			128

#define MB_CHROMA_SIZE_X0	0
#define MB_CHROMA_SIZE_X1	8
#define MB_CHROMA_SIZE_X2	16
#define MB_CHROMA_SIZE_X3	24
#define MB_CHROMA_SIZE_X4	32
#define MB_CHROMA_SIZE_X5	40
#define MB_CHROMA_SIZE_X6	48
#define MB_CHROMA_SIZE_X7	56

#define EOS		1	//!< End Of Sequence
#define SOP		2	//!< Start Of Picture
#define	SOS		3	//!< Start Of Slice

#define	SINT_MAX	0x7fffffff
#define UINT_MAX	0xffffffff

/*define MB type*/
#define	P8x8		8
#define I4MB_264	9
#define	I16MB		10
#define	IBLOCK_264	11
#define	SI4MB		12
#define	I8MB		13
#define	IPCM		14
#define	MAXMODE		15

#define	PMB16x16	1
#define	PMB16x8		2
#define PMB8x16		3

#define PMB8X8_BLOCK8X8	4
#define PMB8X8_BLOCK8X4	5
#define	PMB8X8_BLOCK4X8	6
#define	PMB8X8_BLOCK4X4	7

//nalu type
#define NALU_TYPE_SLICE		1
#define NALU_TYPE_DPA		2
#define	NALU_TYPE_DPB		3
#define	NALU_TYPE_DPC		4
#define	NALU_TYPE_IDR		5
#define	NALU_TYPE_SEI		6
#define	NALU_TYPE_SPS		7
#define	NALU_TYPE_PPS		8
#define	NALU_TYPE_AUD		9
#define	NALU_TYPE_EOSEQ		10
#define	NALU_TYPE_EOSTREAM	11
#define	NALU_TYPE_FILL		12

//nalu priority
#define	NALU_PRIORITY_HIGHEST		3
#define NALU_PRIORITY_HIGH			2
#define	NALU_PRIORITY_LOW			1
#define NALU_PRIORITY_DISPOSABLE	0

//slice type
#define	P_SLICE		0
#define B_SLICE		1
#define I_SLICE		2

#define NO_INTRA_PMODE  9        //!< #intra prediction modes
/* 4x4 intra prediction modes */
#define VERT_PRED             0
#define HOR_PRED              1
#define DC_PRED               2
#define DIAG_DOWN_LEFT_PRED   3
#define DIAG_DOWN_RIGHT_PRED  4
#define VERT_RIGHT_PRED       5
#define HOR_DOWN_PRED         6
#define VERT_LEFT_PRED        7
#define HOR_UP_PRED           8

// 16x16 intra prediction modes
#define VERT_PRED_16    0
#define HOR_PRED_16     1
#define DC_PRED_16      2
#define PLANE_16        3

// 8x8 chroma intra prediction modes
#define DC_PRED_8       0
#define HOR_PRED_8      1
#define VERT_PRED_8     2
#define PLANE_8         3

/*mv predict type*/
#define MVPRED_MEDIAN   0
#define MVPRED_L        1
#define MVPRED_U        2
#define MVPRED_UR       3

#define IS_DIRECT(MB)   ((MB)->mb_type==0     && (img_ptr->type==B_SLICE ))
#define IS_P8x8(MB)     ((MB)->mb_type==P8x8)
#define IS_INTERMV(MB)  ((MB)->mb_type!=I4MB_264  && (MB)->mb_type!=I16MB  && (MB)->mb_type!=0 && (MB)->mb_type!=IPCM)

//--- block category for CABAC ----
#define LUMA_DC			0
#define LUMA_AC_I16		1
#define LUMA_AC			2
#define CHROMA_DC		3
#define CHROMA_AC		4

//sw
typedef void (*MC4xN_LUMA)(uint8 * pFref, uint8 * pPred, int32 width, int32 N);
typedef void (*MC8xN_LUMA)(uint8 * pFref, uint8 * pPred, int32 width, int32 N);
typedef void (*MC16xN_LUMA)(uint8 * pFref, uint8 * pPred, int32 width, int32 N);
typedef void (*MC_LUMA)(uint8 * pFref, uint8 * pPred, int32 xpos, int32 ypos, int32 size);

//error id, added by xiaowei, 20110310
#define ER_REORD_REF_PIC_ID   	2
#define ER_VLD_ID   				4
#define ER_MBC_ID   			8
#define ER_IPRED_ID  			16
#define ER_DBK_ID   				32
#define ER_DCT_ID   			64
#define ER_BSM_ID   			128
#define ER_PICTURE_NULL_ID  		256 	//1<<8
#define ER_AHB_ID   			512 	//1<<9
#define ER_REF_FRM_ID   			1024 	//1<<10
#define ER_GET_SHORT_REF_ID 	2048    //1<<11
#define ER_EXTRA_MEMO_ID      	4096    //1<<12
#define ER_INTER_MEMO_ID       	8192    //1<<13


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BASIC_H_
