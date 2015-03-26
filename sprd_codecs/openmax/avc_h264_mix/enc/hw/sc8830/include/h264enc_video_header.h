/******************************************************************************
 ** File Name:      h264enc_video_header.h                                    *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/17/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    common header file for H264 encoder.                      *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/17/2013    Xiaowei.Luo     Create.                                      *
 *****************************************************************************/
#ifndef _H264ENC_VIDEO_HEADER_H_
#define _H264ENC_VIDEO_HEADER_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#include "sci_types.h"
#include "video_common.h"
#include "mmcodec.h"

#include "vsp_drv.h"
#include "osal_log.h"

#include "h264enc_mode.h"
#include "h264enc_init.h"
#include "h264enc_malloc.h"
#include "h264enc_bitstrm.h"
#include "h264enc_slice.h"
#include "h264enc_set.h"
#include "h264enc_frame.h"
#include "h264enc_rc.h"
#include "h264enc_global.h"
#include "h264enc.h"

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_H264ENC_VIDEO_HEADER_H_
