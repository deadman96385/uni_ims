/******************************************************************************
 ** File Name:      mp4dec_bfrCtrl.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of buffer	  *
 ** 				controlvop												  *
 **                 of mp4 deccoder.                                          *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/

#ifndef _MP4DEC_BFR_CTRL_H_
#define _MP4DEC_BFR_CTRL_H_

#include "mp4dec_mode.h"

MMDecRet Mp4Dec_InitYUVBfr(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
BOOLEAN Mp4Dec_GetCurRecFrameBfr(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
BOOLEAN Mp4Dec_GetCurDispFrameBfr(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
DEC_FRM_BFR* Mp4Dec_GetDispFrameBfr(VIDEO_DATA_T *vd, Mp4DecStorablePic *rec_pic);
void Mp4Dec_SetRefFlag();
void Mp4Dec_SetDspFlag();
MMDecRet MPEG4_DecReleaseDispBfr(VIDEO_DATA_T *vd, uint8 *pBfrAddr);

DEC_FRM_BFR* Mp4Dec_GetRecFrm(VIDEO_DATA_T *vd, uint8 *pBfrAddr);

#endif //_MP4DEC_BFR_CTRL_H_
