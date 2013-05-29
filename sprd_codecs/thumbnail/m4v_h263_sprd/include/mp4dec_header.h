/******************************************************************************
 ** File Name:      mp4dec_header.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of header      *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_HEADER_H_
#define _MP4DEC_HEADER_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
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
#define MARKER_BIT						1
#define START_CODE_PREFIX				1
#define NUMBITS_START_CODE_PREFIX		24
#define NUMBITS_START_CODE_SUFFIX		8

// session overhead information
#define SESSION_START_CODE				0xB0
#define SESSION_END_CODE				0xB1


/**/
#define SHORT_VIDEO_START_MARKER		0x20 
#define SHORT_VIDEO_END_MARKER			0x3F  
#define SHORT_VIDEO_START_MARKER_LENGTH	22   
  

#define DEC_VO_START_CODE				0x8  
#define DEC_VOL_START_CODE				0x12	/* 25-MAR-97 JDL : according to WD2 */
#define VOL_START_CODE_LENGTH			28

#define DEC_VOP_START_CODE				0x1B6	/* 25-MAR-97 JDL : according to WD2 */
#define VOP_START_CODE_LENGTH			32	

#define GROUP_START_CODE				0x01B3	/* 05-05-1997 Minhua Zhou */
#define GROUP_START_CODE_LENGTH			32		/* 10.12.97 Luis Ducla-Soares */

#define USER_DATA_START_CODE			0x01B2	/* Due to N2171 Cl. 2.1.9, MW 23-MAR-1998 */
#define USER_DATA_START_CODE_LENGTH		32		/* Due to N2171 Cl. 2.1.9, MW 23-MAR-1998 */

#define DEC_RESYNC_MARKER				1        /* 26.04.97 LDS: according to VM6.0 */
#define RESYNC_MARKER_LENGTH			17

#define EOB_CODE						1
#define EOB_CODE_LENGTH					32

#define GOB_RESYNC_MARKER				0x01 
#define GOB_RESYNC_MARKER_LENGTH		17  

#define DEC_DC_MARKER					438273	/* 09.10.97 LDS: according to WD4.0 */
#define DC_MARKER_LENGTH				19

#define MOTION_MARKER_COMB				126977	/* 26.04.97 LDS: according to VM7.0 */
#define MOTION_MARKER_COMB_LENGTH       17

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */

#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1   /*????*/
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
#define VISOBJ_START_CODE		0x000001b5

PUBLIC MMDecRet Mp4Dec_DecH263Header(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_DecMp4Header(DEC_VOP_MODE_T *vop_mode_ptr, uint32 uOneFrameLen);
PUBLIC MMDecRet Mp4Dec_FlvH263PicHeader(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_DecGobHeader(DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_GetVideoPacketHeader(DEC_VOP_MODE_T *vop_mode_ptr, uint32 uAddBits);
PUBLIC BOOLEAN Mp4Dec_CheckResyncMarker(DEC_VOP_MODE_T *vop_mode_ptr, uint32 uAddbit);
PUBLIC BOOLEAN Mp4Dec_SearchResynCode(DEC_VOP_MODE_T * vop_mode_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_HEADER_H_
