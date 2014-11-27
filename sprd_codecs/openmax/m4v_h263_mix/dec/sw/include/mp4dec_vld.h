/******************************************************************************
 ** File Name:      mp4dec_vld.h				                              *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           12/14/2006                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of vld         *
 **                 of mp4 deccoder.				                          *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4DEC_VLD_H_
#define _MP4DEC_VLD_H_
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

int32 Mp4Dec_VlcDecMCBPC_com_intra(DEC_VOP_MODE_T *vop_mode_ptr);
int16 Mp4Dec_VlcDecMCBPC_com_inter(DEC_VOP_MODE_T *vop_mode_ptr);
int32 Mp4Dec_VlcDecCBPY(DEC_VOP_MODE_T *vop_mode_ptr, BOOLEAN is_intra_mb);
int32 Mp4Dec_VlcDecPredIntraDC(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num);
int16 Mp4Dec_VlcDecMV(DEC_VOP_MODE_T *vop_mode_ptr, DEC_BS_T *bitstrm_ptr);

int32 Mp4Dec_VlcDecIntraTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iCoefQ, int32 iCoefStart, int8 *pNonCoeffPos);
void Mp4Dec_VlcDecInterTCOEF_H263(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP, DEC_BS_T *pBitstrm);
void Mp4Dec_VlcDecInterTCOEF_Mpeg(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoef, int32 iQP, DEC_BS_T *pBitstrm);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_MP4DEC_VLD_H_
