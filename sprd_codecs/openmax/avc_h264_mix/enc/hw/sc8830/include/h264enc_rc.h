/******************************************************************************
** File Name:      h264enc_rc.h	                                              *
** Author:         Shangwen li                                                *
** DATE:           11/16/2011                                                 *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/16/2011    Shangwen Li     Create.                                     *
** 06/18/2013    Xiaowei Luo     Modify.                                     *
*****************************************************************************/
#ifndef _H264ENC_RC_H_
#define _H264ENC_RC_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "h264enc_mode.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
#ifndef SLICE_SIZE
#define SLICE_MB 1
#endif

//Frame type----------------------------
#define RC_B_SLICE		0
#define RC_P_SLICE		1
#define RC_I_SLICE		2

//Rate control mode----------------------
typedef enum {
    RC_DISABLE = 0,
    RC_GOP_VBR,
    RC_GOP_CBR,
    RC_MODE_NUM,
}
RATE_CONTROL_MODE;

//Codec type---------------------------
typedef enum {
    RC_MPEG4 = 0,
    RC_H264,
    RC_HEVC,
    RC_VP8,
    RC_VP9,
    RC_CODEC_NUM,
} RC_ENCODER_TYPES;

//Error message--------------------------------------
typedef enum {
    RC_NO_ERROR = 0,
    RC_PARA_ZERO = -1,
    RC_NON_SUPPORT_CODEC = -2,
    RC_NON_SUPPORT_RC_MODE = -3,
} RC_ERROR_MSG;

//MAX and MIN value define --------------------
#define RC_MIN_INTRA_PERIOD	1
#define RC_MAX_INTRA_PERIOD	300
#define RC_MIN_IPRATIO			1
#define RC_MAX_IPRATIO			30
#define RC_MIN_CR				2
#define RC_MIN_BITRATE			56000

//---------------------------------------
//global function
//--------------------------------------
int32 init_GOPRC(RC_INOUT_PARAS *rc_inout_paras);
int32 reset_GOPRC(RC_INOUT_PARAS *rc_inout_paras);
int32 getQP_GOPRC(RC_INOUT_PARAS *rc_inout_paras, int32 nSliceType, int32 nSlice_mb_index, int32 last_slice_bits);
void updatePicPara_GOPRC(RC_INOUT_PARAS *rc_inout_paras, int32 bits);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264ENC_RC_H_

