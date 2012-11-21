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
 ** 06/26/2012   Leon Li             Modify.                                                                                      *
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

#define DISPLAY_LIST_SIZE	10

#define DEC_REF_PIC_MARKING_COMMAND_NUM	50
	
#define MAX_REF_FRAME_NUMBER	16
// #define	MAX_REF_FRAME_BUF_NUM	16  //5

#define MAX_PPS	256
#define MAX_SPS	32

#ifdef _VSP_LINUX_
#define SIZE_SLICE_GROUP_ID		960 //512x480
#else
#define SIZE_SLICE_GROUP_ID		3600//for CIF size// should be sufficient for HUGE pictures, need one int per MB in a picture
#endif

#define INVALID_REF_ID	(-135792468)

#define LIST_NOT_USED -1 //FIXME rename?
#define PART_NOT_AVAIL	-2

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

#define NO_INTRA_PMODE	9	//!< #intra prediction modes

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

//define MB type
//#define P8x8    8
//#define I4MB_264    9
//#define I16MB   10
//#define IBLOCK_264  11
//#define SI4MB   12
//#define I8MB    13
//#define IPCM    14
//#define MAXMODE 15

#define IS_DIRECT(MB)   ((MB)->mb_type==0     && (img_ptr->type==B_SLICE ))
#define IS_P8x8(MB)     ((MB)->mb_type==P8x8)
#define IS_INTERMV(MB)  ((MB)->mb_type!=I4MB_264  && (MB)->mb_type!=I16MB  && (MB)->mb_type!=0 && (MB)->mb_type!=IPCM)

//--- block types for CABAC ----
#define MB_TYPE_LUMA_16DC       0
#define MB_TYPE_LUMA_16AC       1
//#define MB_TYPE_LUMA_8x8        2
//#define MB_TYPE_LUMA_8x4        3
//#define MB_TYPE_LUMA_4x8        4
#define MB_TYPE_LUMA_4x4        5
#define MB_TYPE_CHROMA_DC       6
#define MB_TYPE_CHROMA_AC       7
#define NUM_BLOCK_TYPES 8

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_H264DEC_BASIC_H_
