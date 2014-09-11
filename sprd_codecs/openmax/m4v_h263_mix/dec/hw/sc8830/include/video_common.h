/******************************************************************************
** File Name:      video_commmon.h                                           *
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
#ifndef _VIDEO_COMMON_H_
#define _VIDEO_COMMON_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "sci_types.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define SCI_MEMSET  memset
#define SCI_MEMCPY	memcpy
#define PRINTF
#define FPRINTF
#define FFLUSH
#define SCI_ASSERT
#define EXIT

//for motion vector
typedef struct motion_vector_tag
{
    int16 x;
    int16 y;
} MOTION_VECTOR_T;

typedef struct mv_info_tag		/* for motion vector coding*/
{
    uint16 Range;			/* search range (32f (-32f, 32f-1))*/
    uint8  FCode;			/* f-code  (vop_fcode)*/
    uint8  ScaleFactor;	/* scale factor (f)*/
} MV_INFO_T;

typedef struct mcbpc_table_code_len_tag
{
    uint8 code;	/* right justified *///the value of the variable length code
    uint8 len;	// the length of the variable length code
} MCBPC_TABLE_CODE_LEN_T;

typedef struct mv_table_code_len_tag
{
    int8 code;	/* right justified *///the value of the variable length code
    uint8 len;	// the length of the variable length code
} MV_TABLE_CODE_LEN_T;

//for vlc table structure
typedef struct vlc_table_code_len_tag
{
    int16 code;	/* right justified *///the value of the variable length code
    uint16 len;	// the length of the variable length code
} VLC_TABLE_CODE_LEN_T;

typedef enum {INTRA,INTRAQ,INTER,INTERQ,INTER4V,MODE_STUFFING} MB_MODE_E;

//for macroblock
#define MB_SIZE					16
#define MB_CHROMA_SIZE			8
#define MB_SQUARE_SIZE			256

//for block
#define BLOCK_SIZE				8
#define BLOCK_SQUARE_SIZE		64
#define BLOCK_CNT				6

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define Clip3(min,max,val) (((val)<(min))?(min):(((val)>(max))?(max):(val)))
#define YUV420					0
#define YUV411					1
#define YUV444					2
#define YUV422					3
#define YUV400					4

#define mmax(aa,bb)		(((aa) > (bb)) ? (aa) : (bb))
#define mmin(aa,bb)		(((aa) < (bb)) ? (aa) : (bb))
#define ssign(x)		((x) > 0 ? 1 : (-1))
#define ssignof(x)		(((x) > 0) ? 1 : 0)
#define rounded(x)		(((x) > 0) ? (x + 0.5) :(x - 0.5))
#define IClip(Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))
#define MEDIAN(aa, bb, cc) ((aa-bb)*(aa-cc)<=0)?aa :(((bb-cc)*(bb-aa)<=0)?bb:cc)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_VIDEO_COMMON_H_