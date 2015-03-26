/******************************************************************************
 ** File Name:      mp4dec_basic.h                                               *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic interfaces of mp4 codec       *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_BASIC_H_
#define _MP4DEC_BASIC_H_

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
/*----------------------------------------------------------------------------*
**                            Mcaro Definitions                               *
**---------------------------------------------------------------------------*/
#define MP4_LOCAL static

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif
#ifdef OPAQUE
#undef OPAQUE
#endif
#define TRANSPARENT		0
#define OPAQUE			255

#ifndef NOT_IN_TABLE
#define NOT_IN_TABLE	-1
#endif
#ifndef	TCOEF_ESCAPE
#define TCOEF_ESCAPE	102
#endif

#define RC_MPEG4		1
#define RC_TM5			3

//for dc
#define DEFAULT_DC_VALUE		1024

#define ESCAPE				7167
#define TCOEF_RVLC_ESCAPE	169

#define DEC_YUV_BUFFER_NUM   3
#define DISP_YUV_BUFFER_NUM   2

#define YEXTENTION_SIZE  16
#define UVEXTENTION_SIZE 8

//for quantizer mode
typedef enum {
    Q_H263,
    Q_MPEG
}
QUANTIZER_E;

typedef enum {
    DIRECT,
    INTERPOLATE,
    BACKWARD,
    FORWARD
} MB_TYPE_E;

typedef enum {
    UNKNOWN_DIR,
    HORIZONTAL,
    VERTICAL,
    DIAGONAL
} DIRECTION_E;

typedef enum {
    NOT_DECODED,
    DECODED_IN_ERR_PKT,
    DECODED_NOT_IN_ERR_PKT,
    DECODED_OK,
    ERR_CONCEALED
} MB_DEC_STATUS_E;

//for vop prediction type
typedef enum {
    IVOP,
    PVOP,
    BVOP,
    SVOP,
    NVOP
} VOP_PRED_TYPE_E;

//binaryshape
typedef enum {
    RECTANGLE,
    ONE_BIT,
    EIGHT_BIT
} ALPHA_USAGE_E;

typedef enum {
    ALL,
    PARTIAL,
    NONE
} TRANSPARENT_STATUS_E;
typedef enum {
    ALPHA_CODED,
    ALPHA_SKIPPED,
    ALPHA_ALL255
} COD_ALPHA_E;

typedef enum {
    ALL_TRANSP,
    ALL_OPAQUE,
    INTRA_CAE,
    INTER_CAE_MVDZ,
    INTER_CAE_MVDNZ,
    MVDZ_NOUPDT,
    MVDNZ_NOUPDT,
    UNKNOWN
} SHAPE_MODE_E;

typedef enum {
    B_FORWARD,
    B_BACKWARD
} SHAPE_BPRED_DIR_E;

typedef enum {
    INTRA,
    INTRAQ,
    INTER,
    INTERQ,
    INTER4V,
    MODE_STUFFING=7
} MB_MODE_E;

#define VLC_ERROR		(0) //(-1)

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _MP4DEC_BASIC_H_
