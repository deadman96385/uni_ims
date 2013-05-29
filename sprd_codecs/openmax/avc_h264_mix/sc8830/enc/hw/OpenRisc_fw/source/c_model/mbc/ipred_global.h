/******************************************************************************
 ** File Name:      ipred_global.h                                            *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           11/20/2007                                                *
 ** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP bsm Driver for video codec.	  						  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 11/20/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _IPRED_GLOBAL_H_
#define _IPRED_GLOBAL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif


//4x4 luma intra prediction
void intra_pred_luma4x4_VERT_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 0
void intra_pred_luma4x4_HOR_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 1
void intra_pred_luma4x4_DC_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 2
void intra_pred_luma4x4_DIAG_DOWN_LEFT_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 3
void intra_pred_luma4x4_DIAG_DOWN_RIGHT_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix,uint8 *pLeftPix);//mode 4
void intra_pred_luma4x4_VERT_RIGHT_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 5
void intra_pred_luma4x4_HOR_DOWN_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 6
void intra_pred_luma4x4_VERT_LEFT_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 7
void intra_pred_luma4x4_HOR_UP_PRED (int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 8

//16x16 luma intra prediction
void intra_pred_luma16x16_VERT_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 0
void intra_pred_luma16x16_HOR_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 1
void intra_pred_luma16x16_DC_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 2
void intra_pred_luma16x16_PLANE_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 3

//chroma intra prediction
void intra_pred_chroma8x8_DC_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//chorma mode 0
void intra_pred_chroma8x8_HOR_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 1
void intra_pred_chroma8x8_VERT_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 2
void intra_pred_chroma8x8_PLANE_PRED(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);//mode 3

typedef void(*IntraLuma4x4Pred)(int32 blkIdxInMB, uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);
typedef void(*IntraLuma16x16Pred)(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);
typedef void(*IntraChroma8x8Pred)(uint8 *pPred, uint8 *pTopLeftPix, uint8 *pLeftPix);


void Trace_after_intra_prediction_luma16x6(uint8 *pred_y);
void Trace_after_intra_prediction_chroma8x8 (uint8 *pred_uv);
void Trace_after_intra_prediction_Blk4x4(uint8 *pred, uint32 width);
void trace_ipred(uint8 *tmp_pred_Y, uint8 *tmp_pred_U, uint8 *tmp_pred_V);

void ipred_module ();
void init_ipred ();

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_IPRED_GLOBAL_H_