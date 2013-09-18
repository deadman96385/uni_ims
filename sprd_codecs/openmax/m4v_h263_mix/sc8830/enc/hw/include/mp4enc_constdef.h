/******************************************************************************
 ** File Name:      mp4enc_constdef.h                                         *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/09/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the basic operation interfaces of       *
 **                 mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_CONSTDEF_H_
#define _MP4ENC_CONSTDEF_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
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
//for encoder inter used buffer.
#define PUBLIC_BUFFER_SIZE				(400*1024)   //(2 * 1024 * 1024)

//for motion estimation
#define FAVOR_INTER						512
#define SAD_THRESHOLD					600

#define MARKER_BIT						1

#define START_CODE_PREFIX				1
#define NUMBITS_START_CODE_PREFIX		24
//	Added for data partitioning mode
#define NUMBITS_START_CODE_SUFFIX		8

// session overhead information
#define VSS_START_CODE					0xB0	// 8-bit
#define NUMBITS_VSS_PROFILE				8
#define VSO_START_CODE					0xB5	// 8-bit
#define VSO_TYPE						1
#define NUMBITS_VSO_TYPE				4

// VO overhead information
#define NUMBITS_VO_START_CODE			3
#define ENC_VO_START_CODE				0
#define NUMBITS_VO_ID					5

// VOL overhead information
#define NUMBITS_VOL_START_CODE			4
#define ENC_VOL_START_CODE				2
#define NUMBITS_VOL_ID					4
#define NUMBITS_VOL_SHAPE				2
#define NUMBITS_TIME_RESOLUTION			16

// GOV overhead information
#define GOV_START_CODE					0xB3
#define NUMBITS_GOV_START_CODE			8
#define NUMBITS_GOV_TIMECODE_HOUR		5
#define NUMBITS_GOV_TIMECODE_MIN		6
#define NUMBITS_GOV_TIMECODE_SEC		6

// VOP overhead information
#define ENC_VOP_START_CODE				0xB6
#define NUMBITS_VOP_START_CODE			8
#define NUMBITS_VOP_WIDTH				13
#define NUMBITS_VOP_HEIGHT				13
#define NUMBITS_VOP_PRED_TYPE			2
#define NUMBITS_VOP_QUANTIZER			5
#define NUMBITS_VOP_FCODE				3

// Video Packet	overhead,
#define	NUMBITS_VP_RESYNC_MARKER		17
#define	ENC_RESYNC_MARKER				0x1
#define	NUMBITS_VP_QUANTIZER			NUMBITS_VOP_QUANTIZER
#define	NUMBITS_VP_HEC					1
#define	NUMBITS_VP_PRED_TYPE			NUMBITS_VOP_PRED_TYPE
#define	NUMBITS_VP_INTRA_DC_SWITCH_THR	3
#define NUMBITS_QMATRIX					8
#define NUMBITS_VOP_VERTICAL_SPA_REF	13
#define NUMBITS_VOP_HORIZONTAL_SPA_REF	13

// for Data Partitioning
#define	NUMBITS_DP_MOTION_MARKER		17
#define	MOTION_MARKER					0x1F001
#define	NUMBITS_DP_DC_MARKER			19
#define	ENC_DC_MARKER					0x6B001

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

#endif	//_MP4ENC_CONSTDEF_H_
