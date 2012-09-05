/******************************************************************************
 ** File Name:      sc8800g_video_header.h                                    *
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
#ifndef _SC8800G_VIDEO_HEADER_H_
#define _SC8800G_VIDEO_HEADER_H_

#include "sci_types.h"
#include "mmcodec.h"
#include "video_common.h"
#include "mp4_basic.h"
#include "mp4_common_func.h"
#include "mp4_global.h"

#include "mp4dec_bfrctrl.h"
#include "mp4dec_bitstream.h"
#include "mp4dec_datapartitioning.h"
#include "mp4dec_mode.h"
#include "mp4dec_global.h"
#include "mp4dec_header.h"
#include "mp4dec_malloc.h"
#include "mp4dec_mb.h"
#include "mp4dec_mc.h"
#include "mp4dec_mv.h"
#include "mp4dec_session.h"
#include "mp4dec_vop.h"
#include "mp4dec_vld.h"
#include "mp4dec_error_handle.h"
#include "mpeg4dec.h"

#include "mp4enc_mode.h"
#include "mp4enc_bitstrm.h"
#include "mp4enc_command.h"
#include "mp4enc_bfrctrl.h"
#include "mp4enc_constdef.h"
#include "mp4enc_global.h"
#include "mp4enc_header.h"
#include "mp4enc_malloc.h"
#include "mp4enc_init.h"
#include "mp4enc_mb.h"
#include "mp4enc_me.h"
#include "mp4enc_mv.h"
#include "mp4enc_ratecontrol.h"
#include "mp4enc_trace.h"
#include "mp4enc_vlc.h"
#include "mp4enc_vop.h"
#include "mpeg4enc.h"

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
#endif  //_SC8800G_VIDEO_HEADER_H_