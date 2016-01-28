/******************************************************************************
 ** File Name:      h264dec_video_header.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           02/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    common header file for H.264 decoder.               *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 4/23/2007    Binggo Zhou     Create.                                      *
 *****************************************************************************/
#ifndef _H264DEC_VIDEO_HEADER_H_
#define _H264DEC_VIDEO_HEADER_H_
#ifdef   __cplusplus
extern   "C"
{
#endif

#include "sci_types.h"
#include "video_common.h"
#include "mmcodec.h"

#include "vsp_drv.h"
#include "osal_log.h"

#include "h264dec_basic.h"
#include "h264dec_mode.h"
#include "h264dec_global.h"
#include "h264dec_buffer.h"
#include "h264dec_malloc.h"
#include "h264dec_init.h"
#include "h264dec_bitstream.h"
#include "h264dec_biaridecod.h"
#include "h264dec_context_init.h"
#include "h264dec_header.h"
#include "h264dec_slice.h"
#include "h264dec_parset.h"
#include "h264dec_image.h"
#include "h264dec.h"

#include <cutils/properties.h>

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
#endif  //_H264DEC_VIDEO_HEADER_H_
