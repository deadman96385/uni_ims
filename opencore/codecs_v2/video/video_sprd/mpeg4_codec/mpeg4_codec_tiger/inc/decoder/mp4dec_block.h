/******************************************************************************
 ** File Name:      mp4dec_block.h                                         *
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
#ifndef _MP4DEC_BLOCK_H_
#define _MP4DEC_BLOCK_H_
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

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraBlkTCoef_Mp4
 ** Description:	Get blk_num of current intra MB's TCoef, Mp4.
 ** Author:			Xiaowei Luo
 **	Note:			DeScan and IQ done by SW.
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraBlkTCoef_Mp4(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 iBlkIdx);

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraBlkTCoef_H263
 ** Description:	Get blk_num of current intra MB's TCoef, H263.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraBlkTCoef_H263(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 iBlkIdx);

/*****************************************************************************
 **	Name : 			Mp4Dec_MpegIqBlock
 ** Description:	H263 Inverse quantify current block.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_MpegIqBlock(DEC_VOP_MODE_T *vop_mode_ptr, int16 *piDCTCoeff, int32 iQP, int32 start);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_MP4DEC_BLOCK_H_

