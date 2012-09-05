/******************************************************************************
 ** File Name:      sc800g_video_header.h                                           *
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
#ifndef _SC8800G_VIDEO_HEADER_H_
#define _SC8800G_VIDEO_HEADER_H_

#include "sci_types.h"
#include "video_common.h"
#include "mmcodec.h"

#include "vsp_global_define.h"
#include "vsp_dcam.h"
#include "vsp_dct.h"
#include "vsp_mbc.h"
#include "vsp_dbk.h"
#include "vsp_mca.h"
#include "vsp_bsm.h"
#include "vsp_drv_sc8800g.h"
#include "vsp_vld.h"
#include "vsp_vlc.h"
#include "vsp_mea.h"
#include "h264dec_basic.h"
#include "h264dec_mode.h"
#include "h264dec_global.h"
#include "h264dec_buffer.h"
#include "h264dec_malloc.h"
#include "h264dec_init.h"
#include "h264dec_bitstream.h"
#include "h264dec_header.h"
#include "h264dec_slice.h"
#include "h264dec_parset.h"
#include "h264dec_command.h"
#include "h264dec_image.h"
#include "h264dec_mb.h"
#include "h264dec_deblock.h"
#include "h264dec_mv.h"
#include "h264dec_fmo.h"
#include "h264dec.h"

#ifndef _VSP_LINUX_
#include "os_api.h"
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_SC8801H_VIDEO_HEADER_H_
