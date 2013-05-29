/******************************************************************************
 ** File Name:      h264dec_ipred.h                                          *
 ** Author:         jiang.lin                                                *
 ** DATE:           27/01/2006                                                *
 ** Copyright:      2006 Spreadtrum, Incoporated. All Rights Reserved.        *
 ** Description:    interface of transplant                                   *
 ** Note:           None                                                     *
 ******************************************************************************/

#ifndef _H264DEC_INTRAPRED_H_
#define _H264DEC_INTRAPRED_H_

void intraPred_VERT_PRED_16 (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, uint8 * pRec, int32 pitch);
void intraPred_HOR_PRED_16 (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, uint8 * pRec, int32 pitch);
void intraPred_DC_PRED_16 (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, uint8 * pRec, int32 pitch);
void intraPred_PLANE_16 (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, uint8 * pRec, int32 pitch);

void intraPred_chroma_DC (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPredMB, uint8 * pRec, int32 pitch);
void intraPred_chroma_Hor (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPredMB, uint8 * pRec, int32 pitch);
void intraPred_chroma_Ver (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPredMB, uint8 * pRec, int32 pitch);
void intraPred_chroma_Plane (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPredMB, uint8 * pRec, int32 pitch);

void intra_pred_luma4x4_DC_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_VERT_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_HOR_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_DIAG_DOWN_RIGHT_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_DIAG_DOWN_LEFT_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_VERT_RIGHT_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_VERT_LEFT_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_HOR_UP_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);
void intra_pred_luma4x4_HOR_DOWN_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB, uint8 * pRec, int32 pitch);

void intra_pred_luma8x8_VERT_PRED (DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_HOR_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_DC_PRED (DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_DIAG_DOWN_LEFT_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_DIAG_DOWN_RIGHT_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_VERT_RIGHT_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_HOR_DOWN_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_VERT_LEFT_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);
void intra_pred_luma8x8_HOR_UP_PRED(DEC_MB_CACHE_T * mb_cache_ptr, uint8 * pPred, int blkIdxInMB);

#endif