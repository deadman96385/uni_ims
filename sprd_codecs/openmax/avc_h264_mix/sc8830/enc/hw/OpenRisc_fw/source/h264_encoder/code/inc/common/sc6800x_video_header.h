/******************************************************************************
 ** File Name:      sc6800x_video_header.h                                    *
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
#ifndef _SC6800X_VIDEO_HEADER_H_
#define _SC6800X_VIDEO_HEADER_H_

#include "sci_types.h"
#include "video_common.h"
#include "mmcodec.h"

#include "vsp_global_define.h"
#include "vsp_drv_sc8800g.h"
#ifdef SIM_IN_WIN
#include "vsp_dcam.h"
#include "vsp_dct.h"
#include "vsp_mbc.h"
#include "vsp_dbk.h"
#include "vsp_mca.h"
#include "vsp_bsm.h"
#include "vsp_vld.h"
#include "vsp_vlc.h"
#include "vsp_mea.h"
#include "vsp_ip_syntax.h"
#else
#ifdef ORSC_FW
	#include "spr-defs.h"
#endif
#endif

#include "h264enc_reg.h"
#include "h264enc_mode.h"
#include "h264enc_global.h"
#include "h264enc_malloc.h"
#include "h264enc_bitstrm.h"
#include "h264enc_slice.h"
#include "h264enc_set.h"
#include "h264enc_frame.h"
#include "h264enc_rc.h"
#include "h264enc.h"
#ifdef SIM_IN_WIN
#include "h264enc_bfrctrl.h"
#include "h264enc_mb.h"
#include "h264enc_mv.h"
#include "h264enc_vlc.h"
#endif

#ifdef SIM_IN_WIN
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#if _CMODEL_
#include "ahbm_global.h"
#include "bsm_global.h"
#include "buffer_global.h"
#include "common_global.h"
#include "dcam_global.h"
#include "dct_global.h"
#include "global_global.h"
#include "dct_global.h"
#include "iict_global.h"
#include "hdct_global.h"
#include "hdct_mode.h"
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
#include "hvlc_global.h"
#include "hvlc_mode.h"
#include "hvlc_tv.h"
#include "hmea_global.h"
//#endif //_CMODEL_
#endif //SIM_IN_WIN

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
#endif  //_SC6800X_VIDEO_HEADER_H_