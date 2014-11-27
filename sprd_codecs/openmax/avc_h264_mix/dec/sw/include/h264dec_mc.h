/******************************************************************************
 ** File Name:      mc.h                                               *
 ** Author:         jiang.lin                                                *
 ** DATE:           05/02/2006                                                *
 ** Copyright:      2006 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                     *
 ******************************************************************************/

#ifndef _H264DEC_MC_H_
#define _H264DEC_MC_H_

void H264Dec_mc_4x8 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4x8);
void H264Dec_mc_4x4 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4);
void H264Dec_mc_16x16 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y);
void H264Dec_mc_16x8 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b16x8);
void H264Dec_mc_8x16 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8x16);
void H264Dec_mc_8x8 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y,  int32 b8);
void H264Dec_mc_8x4 (H264DecContext *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b8x4);

#define DECLARE_MC_FUN(size)	\
	void MC_luma##size##_dx0dy0 (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_dxMdy0 (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_dx0dyM (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_xfull (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_yfull (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_xhalf (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_yhalf (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);\
	void MC_luma##size##_xyqpix (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N);

DECLARE_MC_FUN(4xN);
DECLARE_MC_FUN(8xN);
DECLARE_MC_FUN(16xN);

#undef DECLARE_MC_FUN

void PC_MC_chroma2xN (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N);
void PC_MC_chroma4xN (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N);
void PC_MC_chroma8xN (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N);

typedef void (*MC_chroma8xN) (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N);
typedef void (*MC_chroma4xN) (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N);
typedef void (*MC_chroma2xN) (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N);

#endif
