/******************************************************************************
 ** File Name:      mp4enc_video_header.h                                    *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           02/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    VSP Driver for video codec.		       			          *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 4/23/2007    Binggo Zhou     Create.                                      *
 *****************************************************************************/
#ifndef _MP4ENC_VIDEO_HEADER_H_
#define _MP4ENC_VIDEO_HEADER_H_

#include "sci_types.h"
#include "mmcodec.h"
#include "video_common.h"
#include "mp4enc_basic.h"

#include "mp4enc_mode.h"
#include "mp4enc_bitstrm.h"
#include "mp4enc_constdef.h"
#include "mp4enc_global.h"
#include "mp4enc_header.h"
#include "mp4enc_malloc.h"
#include "mp4enc_init.h"
#include "mp4enc_ratecontrol.h"
#include "mp4enc_vop.h"
#include "mpeg4enc.h"

#include "vsp_drv_sc8830.h"

#ifdef RUN_IN_PLATFORM
//#include "os_api.h"
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
#endif  //_MP4ENC_VIDEO_HEADER_H_