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
#include "mp4dec_basic.h"
#include "mp4dec_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void Mp4Dec_motionCompY_oneMV(DEC_VOP_MODE_T *vop_mode_ptr,  MOTION_VECTOR_T *pmv,uint8* pDstFrameY, int32 dstWidth , int32 bck_direction);
void Mp4Dec_motionCompY_fourMV(DEC_VOP_MODE_T *vop_mode_ptr,  MOTION_VECTOR_T *pmv,uint8* pDstFrameY, int32 dstWidth, int32 bck_direction);
void Mp4Dec_motionCompensationUV(DEC_VOP_MODE_T *vop_mode_ptr, int32 iMVUVx, int32 iMVUVy,uint8* pDstFrameU,uint8* pDstFrameV, int32 dstWidth , int32 bck_direction);


void mc_xyfull_16x16 (uint8 *pframe_ref, uint8 *pRecMB,int32 width, int32 dstWidth);
void mc_xyfull_8x8 (uint8 *pframe_ref, uint8 * pRecMB, int32 width, int32 dstWidth);
void mc_xfullyhalf_16x16_rnd0 (uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth);
void mc_xhalfyfull_16x16_rnd0 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xyhalf_16x16_rnd0 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xfullyhalf_8x8_rnd0 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xhalfyfull_8x8_rnd0 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xyhalf_8x8_rnd0 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xfullyhalf_16x16_rnd1 (uint8 *pframe_ref, uint8 *pRecMB,  int32 width, int32 dstWidth);
void mc_xhalfyfull_16x16_rnd1 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xyhalf_16x16_rnd1 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xfullyhalf_8x8_rnd1 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xhalfyfull_8x8_rnd1 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);
void mc_xyhalf_8x8_rnd1 (uint8 *pframe_ref, uint8 *pRecMB, int32 width, int32 dstWidth);

void Mp4Dec_MCOneMbBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr);
void Mp4Dec_StartMcaOneDir(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, MOTION_VECTOR_T *pMv);
void Mp4Dec_MCA_BVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *pFwdPredMv,
                     MOTION_VECTOR_T *pBckPredMv, MOTION_VECTOR_T *pCoMv);
//void Mp4Dec_MCA_BVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *pFwdPredMv,
//					   MOTION_VECTOR_T *pBckPredMv, MOTION_VECTOR_T *pCoMv,\
//					   MOTION_VECTOR_T*pFwdMvCliped,MOTION_VECTOR_T*pBckMvCliped,\
//					   MOTION_VECTOR_T*pFwdMvUV,MOTION_VECTOR_T*pBckMvUV);
void Mp4Dec_RecOneMbBvop(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr,\
                         MOTION_VECTOR_T*pFwdMvCliped,MOTION_VECTOR_T*pBckMvCliped,\
                         MOTION_VECTOR_T*pFwdMvUV,MOTION_VECTOR_T*pBckMvUV);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_MP4DEC_MC_H_
