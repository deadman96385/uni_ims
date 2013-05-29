/******************************************************************************
 ** File Name:      mp4dec_error_handle.h                                     *
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
#ifndef _MP4DEC_ERROR_HANDLE_H_
#define _MP4DEC_ERROR_HANDLE_H_
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

PUBLIC void Mp4Dec_UpdateErrInfo (DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Dec_EC_IVOP (DEC_VOP_MODE_T * vop_mode_ptr);
PUBLIC void Mp4Dec_EC_PVOP (DEC_VOP_MODE_T * vop_mode_ptr);
PUBLIC MMDecRet Mp4Dec_EC_FillMBGap (DEC_VOP_MODE_T *vop_mode_ptr);
PUBLIC void Mp4Dec_EC_IVOP_vt (DEC_VOP_MODE_T * vop_mode_ptr);
PUBLIC void Mp4Dec_EC_PVOP_vt (DEC_VOP_MODE_T * vop_mode_ptr);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif //_MP4DEC_ERROR_HANDLE_H_