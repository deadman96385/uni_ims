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

#if 0
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
#endif
#include "mp4enc_mode.h"
#include "mp4enc_bitstrm.h"
#include "mp4enc_constdef.h"
#include "mp4enc_global.h"
#include "mp4enc_header.h"
#include "mp4enc_malloc.h"
#include "mp4enc_init.h"
#include "mp4enc_ratecontrol.h"
#include "mp4enc_trace.h"
#include "mp4enc_vop.h"
#include "mpeg4enc.h"

#include "vsp_drv_sc8830.h"

#if 0

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

#endif

#if defined(SIM_IN_WIN)
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif //SIM_IN_WIN

#if _CMODEL_
#include "ahbm_global.h"
#include "bsm_global.h"
#include "buffer_global.h"
#include "common_global.h"
#include "dcam_global.h"
#include "dct_global.h"
#include "global_global.h"
#include "dct_global.h"
#include "iict_global.h"
#include "mbc_global.h"
#include "ipred_global.h"
#include "mca_global.h"
#include "vld_global.h"
#include "hvld_global.h"
#include "hvld_mode.h"
#include "hvld_test_vector.h"
#include "hvld_trace.h"
#include "h264dbk_trace.h"
#include "hdbk_global.h"
#include "hdbk_mode.h"
#include "hdbk_test_vector.h"
#include "vlc_global.h"
#include "mea_global.h"
#endif //_CMODEL_

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
#endif  //_SC8800G_VIDEO_HEADER_H_