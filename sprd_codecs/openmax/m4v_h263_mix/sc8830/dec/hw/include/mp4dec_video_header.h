/******************************************************************************
 ** File Name:      mp4dec_video_header.h                                     *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           02/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    common header file for sc8801h video(jpeg).               *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 4/23/2007    Binggo Zhou     Create.                                      *
 *****************************************************************************/
#ifndef _MP4DEC_VIDEO_HEADER_H_
#define _MP4DEC_VIDEO_HEADER_H_
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#include "sci_types.h"
#include "mmcodec.h"
#include "video_common.h"
#include "mp4dec_basic.h"

#include "mp4dec_bfrctrl.h"
#include "mp4dec_bitstream.h"


#include "mp4dec_global.h"
#include "mp4dec_header.h"
#include "mp4dec_malloc.h"
#include "mp4dec_session.h"


#include "mpeg4dec.h"
#include "vsp_drv_sc8830.h"



#ifdef RUN_IN_PLATFORM
#include "os_api.h"
#endif //RUN_IN_PLATFORM

#ifdef SIM_IN_ADS
#include <stdlib.h>
#endif //SIM_IN_ADS
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_MP4DEC_VIDEO_HEADER_H_

