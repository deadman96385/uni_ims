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

void H264Dec_mc_4x8 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4x8);
void H264Dec_mc_4x4 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b4);
void H264Dec_mc_16x16 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y);
void H264Dec_mc_16x8 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b16x8);
void H264Dec_mc_8x16 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8x16);
void H264Dec_mc_8x8 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y,  int32 b8);
void H264Dec_mc_8x4 (DEC_IMAGE_PARAMS_T *img_ptr, uint8 *pPredMBY, uint8 **pRefFrame, int32 mv_x, int32 mv_y, int32 b8, int32 b8x4);

#define DECLARE_MC_FUN(size)	\
	void MC_luma##size##_dx0dy0 (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_dxMdy0 (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_dx0dyM (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_xfull (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_yfull (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_xhalf (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_yhalf (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);\
	void MC_luma##size##_xyqpix (uint8 * pRefFrame, uint8 * pPred, int32 width, int32 N);

	DECLARE_MC_FUN(4xN);
	DECLARE_MC_FUN(8xN);
	DECLARE_MC_FUN(16xN);

#undef DECLARE_MC_FUN

void PC_MC_chroma2xN (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N);
void PC_MC_chroma4xN (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N);
void PC_MC_chroma8xN (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N);

typedef void (*MC_chroma8xN) (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N);
typedef void (*MC_chroma4xN) (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N);
typedef void (*MC_chroma2xN) (uint8 ** pRefFrame, uint8 ** pPredUV, int32 width_c, int32 N);

#endif
