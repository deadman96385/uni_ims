/******************************************************************************
 ** File Name:      sc8810_video_header.h                                           *
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
#ifndef _SC8810_VIDEO_HEADER_H_
#define _SC8810_VIDEO_HEADER_H_

#include "sci_types.h"
#include "video_common.h"
#include "mmcodec.h"

#include "vsp_global_define.h"
#include "vsp_drv_sc8800g.h"
#if SIM_IN_WIN
#include "vsp_dcam.h"
#include "vsp_dct.h"
#include "vsp_mbc.h"
#include "vsp_dbk.h"
#include "vsp_mca.h"
#include "vsp_bsm.h"
#include "vsp_vld.h"
#include "vsp_vlc.h"
#include "vsp_mea.h"
#include "vsp_ppa.h"//weihu
#include "vsp_parser.h"
#else
#include "spr-defs.h"
#endif

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
#if SIM_IN_WIN
//#include "h264dec_mv.h"
#include "h264dec_fmo.h"
//#include "h264dec_vld.h"
//#include "h264dec_mb.h"
#include "h264dec_command.h"
//#include "h264dec_cabac.h"
//#include "h264dec_deblock.h"
#endif

#if SIM_IN_WIN
#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#endif //SIM_IN_WIN

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
#include "rvld_mode.h"
#include "rvld_global.h"
#include "ppa_global.h"//weihu
#include "parser_global.h"
#include "coeff_vld_global.h"

//#endif //_CMODEL_

//#if SIM_IN_WIN
extern int32 sw_wr_bsm;
#endif


#ifdef RUN_IN_PLATFORM
#include "os_api.h"
#endif //RUN_IN_PLATFORM

#ifdef SIM_IN_ADS
#include <stdlib.h>
#endif //SIM_IN_ADS

//#define H264_MCA //weihu
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_SC8810_VIDEO_HEADER_H_