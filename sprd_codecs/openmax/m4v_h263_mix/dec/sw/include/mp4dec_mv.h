/******************************************************************************
 ** File Name:      mp4dec_mv.h                                            *
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
#ifndef _MP4DEC_MV_H_
#define _MP4DEC_MV_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4dec_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define CLIP_MV(vop_mode_ptr, mvx, mvy)\
{\
	if (mvx > vop_mode_ptr->mv_x_max)\
	{\
		mvx = vop_mode_ptr->mv_x_max;\
	}\
	else if (mvx < vop_mode_ptr->mv_x_min)\
	{\
		mvx = vop_mode_ptr->mv_x_min;\
	}\
	\
	if (mvy > vop_mode_ptr->mv_y_max)\
	{\
		mvy = vop_mode_ptr->mv_y_max;\
	}\
	else if (mvy < vop_mode_ptr->mv_y_min)\
	{\
		mvy= vop_mode_ptr->mv_y_min;\
	}\
}

#define JudegeLeftBndry(pos_x, mb_mode_ptr) \
	((pos_x == 0) || ((mb_mode_ptr-1)->videopacket_num != mb_mode_ptr->videopacket_num)) ? FALSE : TRUE ;
#define JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr)  \
	((pos_y == 0) || ((mb_mode_ptr - vop_mode_ptr->MBNumX)->videopacket_num != mb_mode_ptr->videopacket_num)) ? FALSE : TRUE;
#define JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr)  \
	((pos_x == vop_mode_ptr->MBNumX - 1) || (pos_y == 0) || ((mb_mode_ptr - vop_mode_ptr->MBNumX + 1)->videopacket_num != mb_mode_ptr->videopacket_num)) ? FALSE : TRUE;



void Mp4Dec_Get16x16MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX);
void Mp4Dec_Get8x8MVPredAtBndry(DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX);
void Mp4Dec_Get8x8MVPredNotAtBndry(DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX);
void Mp4Dec_Get8x8MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred);
void Mp4Dec_DeScaleMVD(int32 long_vectors, int32 f_code, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pPredMv);

void Mp4Dec_DecMV(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr);
void Mp4Dec_DecodeOneMV(DEC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pPredMv, int32 fcode);

void Mp4Dec_DecMV_DP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_MP4DEC_DEC_MV_H_
