/******************************************************************************
 ** File Name:      mp4dec_rvld.h                                             *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of rvlc        *
 **                 of mp4 deccoder.			                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_RVLD_H_
#define _MP4DEC_RVLD_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
#include "mp4dec_bitstream.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraRvlcTcoef
 ** Description:	Get the intra rvlc tcoef.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraRvlcTcoef(DEC_VOP_MODE_T *vop_mode_ptr, DEC_BS_T *bitstrm_ptr, TCOEF_T *run_level_ptr);
/*****************************************************************************
 **	Name : 			Mp4Dec_GetInterRvlcTcoef
 ** Description:	Get the inter rvlc tcoef.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetInterRvlcTcoef(DEC_VOP_MODE_T *vop_mode_ptr, DEC_BS_T *bitstrm_ptr, TCOEF_T *run_level_ptr);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_MP4DEC_RVLD_H_
