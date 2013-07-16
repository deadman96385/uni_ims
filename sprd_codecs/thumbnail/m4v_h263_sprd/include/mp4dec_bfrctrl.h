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

MMDecRet Mp4Dec_InitYUVBfr(Mp4DecObject *vd);
BOOLEAN Mp4Dec_GetCurRecFrameBfr(Mp4DecObject *vd);
MMDecRet MPEG4_DecReleaseDispBfr(Mp4DecObject *vd, uint8 *pBfrAddr);

#endif //_MP4DEC_BFR_CTRL_H_
