/******************************************************************************
 ** File Name:      mp4dec_mc.h                                            *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of bitstream   *
 **                 operation of mp4 deccoder.                                *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_MC_H_
#define _MP4DEC_MC_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_MCOneMbBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_StartMcaOneDir(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, MOTION_VECTOR_T *pMv);
PUBLIC void Mp4Dec_MCA_BVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *pFwdPredMv, 
					   MOTION_VECTOR_T *pBckPredMv, MOTION_VECTOR_T *pCoMv);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_MC_H_
