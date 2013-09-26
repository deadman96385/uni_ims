/******************************************************************************
 ** File Name:      mp4dec_session.h                                       *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of session     *
 **                 of mp4 deccoder.                                          *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_SESSION_H_
#define _MP4DEC_SESSION_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#if defined(SIM_IN_WIN)
/*****************************************************************************
 **	Name : 			read_tsc
 ** Description:	read time of system clock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
__inline __int64 read_tsc(void);
#endif //SIM_IN_WIN

MMDecRet Mp4Dec_InitGlobal (Mp4DecObject *vo);
MMDecRet Mp4Dec_InitSessionDecode(Mp4DecObject *vo);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_MP4DEC_SESSION_H_
