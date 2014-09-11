/******************************************************************************
 ** File Name:      vp8dec_video_header.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           07/04/2013                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:                                                                       *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                      *
 *****************************************************************************/
#ifndef _VP8DEC_VIDEO_HEADER_H_
#define _VP8DEC_VIDEO_HEADER_H_
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

#include "vp8dec_basic.h"
#include "vp8dec_bfrctrl.h"
#include "vp8dec_dboolhuff.h"
#include "vp8dec_dequant.h"
#include "vp8dec_frame.h"
#include "vp8dec_global.h"
#include "vp8dec_init.h"
#include "vp8dec_malloc.h"
#include "vp8dec_mode.h"
#include "vp8dec_vld.h"
#include "vp8dec.h"
#include "vsp_drv.h"

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_VP8DEC_VIDEO_HEADER_H_