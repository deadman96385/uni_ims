/******************************************************************************
 ** File Name:      mp4dec_datapartitioning.h                                 *
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
#ifndef _MP4DEC_DATAPARTITION_H_
#define _MP4DEC_DATAPARTITION_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
PUBLIC MMDecRet Mp4Dec_DecIVOPErrResDataPartitioning(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_DecPVOPErrResDataPartitioning(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Dec_RvlcIntraTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoefQ,int32 iCoefStart);
PUBLIC void Mp4Dec_RvlcInterTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoefQ);
PUBLIC void Mp4Dec_GetSecondPartitionMBHeaderIVOP(DEC_VOP_MODE_T *vop_mode_ptr,DEC_MB_MODE_T *mb_mode_ptr);
PUBLIC void Mp4Dec_GetSecondPartitionMBHeaderPVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 mb_num);
PUBLIC void Mp4Dec_DecSecondPartitionIntraErrRes_vt(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no);
PUBLIC void Mp4Dec_DecSecondPartitionInterErrRes_vt(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no);
PUBLIC int32 Mp4Dec_RvlcIntraTCOEF_vt(DEC_VOP_MODE_T *vop_mode_ptr, int16 * iDCTCoefQ, int32 iCoefStart,char *pNonCoeffPos);
PUBLIC void Mp4Dec_RvlcInterTCOEF_vt(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoefQ, int32 iQP, DEC_BS_T *pBitstrm);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_DATAPARTITION_H_
