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

#define _MVC_		1

#define DISPLAY_LIST_SIZE	10

#define DEC_REF_PIC_MARKING_COMMAND_NUM	50

#define MAX_REF_FRAME_NUMBER	16
// #define	MAX_REF_FRAME_BUF_NUM	16  //5

#define MAX_PPS	256
#define MAX_SPS	32

#define INVALID_REF_ID	(-135792468)

#define LIST_NOT_USED -1 //FIXME rename?
#define PART_NOT_AVAIL	-2

#define EOS		1	//!< End Of Sequence
#define SOP		2	//!< Start Of Picture
#define	SOS		3	//!< Start Of Slice

#define	SINT_MAX	0x7fffffff
#define UINT_MAX	0xffffffff

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
#if _MVC_
#define NALU_TYPE_PREFIX	14
#define NALU_TYPE_SUB_SPS	15
#define NALU_TYPE_SLC_EXT	20
#define NALU_TYPE_VDRD		24  // View and Dependency Representation Delimiter NAL Unit
#endif

//nalu priority
#define	NALU_PRIORITY_HIGHEST		3
#define NALU_PRIORITY_HIGH			2
#define	NALU_PRIORITY_LOW			1
#define NALU_PRIORITY_DISPOSABLE	0

//slice type
#define P_SLICE		1
#define B_SLICE		2
#define I_SLICE		0

typedef enum {
    FREXT_CAVLC444 = 44,       //!< YUV 4:4:4/14 "CAVLC 4:4:4"
    BASELINE       = 66,       //!< YUV 4:2:0/8  "Baseline"
    MAIN           = 77,       //!< YUV 4:2:0/8  "Main"
    EXTENDED       = 88,       //!< YUV 4:2:0/8  "Extended"
    FREXT_HP       = 100,      //!< YUV 4:2:0/8  "High"
    FREXT_Hi10P    = 110,      //!< YUV 4:2:0/10 "High 10"
    FREXT_Hi422    = 122,      //!< YUV 4:2:2/10 "High 4:2:2"
    FREXT_Hi444    = 244,      //!< YUV 4:4:4/14 "High 4:4:4"
    MVC_HIGH       = 118,      //!< YUV 4:2:0/8  "Multiview High"
    STEREO_HIGH    = 128       //!< YUV 4:2:0/8  "Stereo High"
}
ProfileIDC;

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264DEC_BASIC_H_