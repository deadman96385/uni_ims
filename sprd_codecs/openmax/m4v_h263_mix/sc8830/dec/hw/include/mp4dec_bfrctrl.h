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

MMDecRet Mp4Dec_InitYUVBfr(Mp4DecObject *vo);
BOOLEAN Mp4Dec_GetCurRecFrameBfr(Mp4DecObject *vo);
BOOLEAN Mp4Dec_GetCurDispFrameBfr(Mp4DecObject *vo);
DEC_FRM_BFR* Mp4Dec_GetDispFrameBfr(Mp4DecObject *vo, Mp4DecStorablePic *rec_pic);

#endif //_MP4DEC_BFR_CTRL_H_
