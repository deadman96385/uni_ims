/******************************************************************************
 ** File Name:      sc8825_video_header.h                                    *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           02/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    common header file for sc8810p video(jpeg).               *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 09/28/2012    Leon Li     Create.                                      *
 *****************************************************************************/
#ifndef _SC8825_VIDEO_HEADER_H_
#define _SC8825_VIDEO_HEADER_H_

#ifdef __ICC
    #define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
    #define DECLARE_ASM_CONST(n,t,v)    const t __attribute__ ((aligned (n))) v
#elif defined(__GNUC__)
    #define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
    #define DECLARE_ASM_CONST(n,t,v)    const t v  __attribute__ ((aligned (n)))
#elif defined(_MSC_VER)
    #define DECLARE_ALIGNED(n,t,v)      /*__declspec(align(n))*/ t v
    #define DECLARE_ASM_CONST(n,t,v)    /*__declspec(align(n))*/ const t v
#elif defined(HAVE_INLINE_ASM)
    #error The asm code needs alignment, but we do not know how to do it for this compiler.
#else
    #define DECLARE_ALIGNED(n,t,v)      t v
    #define DECLARE_ASM_CONST(n,t,v)    const t v
#endif

#ifndef WIN32
#define _NEON_OPT_
#define _ARM_CLZ_OPT_
#include <arm_neon.h>

#define INTRA_IDCT_ASSEMBLY
#define INTER_IDCT_ASSEMBLY
//#define MC_ASSEMBLY
     
#endif
#include "sci_types.h"
#include "video_common.h"
#include "mmcodec.h"
#include "sc6600l_dcam_common.h"
#include "sc6600l_isp_reg.h"

#define MAPaddr( a ) (( (a)&0x7fffffff) >>2)

#include "vsp_global_define.h"
#include "vsp_axim.h"
#include "vsp_dct.h"
#include "vsp_mbc.h"
#include "vsp_dbk.h"
#include "vsp_mca.h"
#include "vsp_bsm.h"
#include "vsp_drv_sc8825.h"
#include "vsp_vld.h"
#include "vsp_vlc.h"
#include "vsp_mea.h"
#include "vsp_glb_ctrl.h"

#if defined(H264_DEC)
#include "h264dec_basic.h"
#include "h264dec_mode.h"
#include "h264dec_global.h"
#include "h264dec_buffer.h"
#include "h264dec_malloc.h"
#include "h264dec_init.h"
#include "h264dec_bitstream.h"
#include "h264dec_biaridecod.h"
#include "h264dec_cabac.h"
#include "h264dec_context_init.h"
#include "h264dec_header.h"
#include "h264dec_slice.h"
#include "h264dec_parset.h"
#include "h264dec_command.h"
#include "h264dec_image.h"
#include "h264dec_mb.h"
#include "h264dec_deblock.h"
#include "h264dec_mv.h"
#include "h264dec_fmo.h"
#include "h264dec_vld.h"
#include "h264dec.h"
#elif defined(MPEG4_DEC)
	#include "mmcodec.h"
	#include "mp4_basic.h"
	#include "mp4_common_func.h"
	#include "mp4_global.h"
	#include "mp4dec_bfrctrl.h"
	#include "mp4dec_bitstream.h"
	#include "mp4dec_block.h"
	#include "mp4dec_datapartitioning.h"
	#include "mp4dec_error_handle.h"
	#include "mp4dec_global.h"
	#include "mp4dec_command.h"
	#include "mp4dec_header.h"
	#include "mp4dec_malloc.h"
	#include "mp4dec_mb.h"
	#include "mp4dec_mc.h"
	#include "mp4dec_mode.h"	
	#include "mp4dec_mv.h"
	#include "mp4dec_rvld.h"
	#include "mp4dec_session.h"
	#include "mp4dec_vld.h"
	#include "mp4dec_vop.h"	
	#include "mpeg4dec.h"
#elif defined(MPEG4_ENC)
	#include "mmcodec.h"
	#include "mp4_basic.h"
	#include "mp4_common_func.h"
	#include "mp4_global.h"
	#include "mp4enc_mode.h"
	#include "mp4enc_bitstrm.h"
	#include "mp4enc_command.h"
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
#elif defined(REAL_DEC)
#include "rvdec.h"
#include "rvdec_basic.h"
#include "rvdec_bfrctrl.h"
#include "rvdec_bitstrm.h"
#include "rvdec_frame.h"
#include "rvdec_global.h"
#include "rvdec_header.h"
#include "rvdec_init.h"
#include "rvdec_malloc.h."
#include "rvdec_mb.h"
#include "rvdec_mode.h"
#include "rvdec_mv.h"
#include "rvdec_vld.h"
#else
	#include "jpegcodec_global.h"
	#include "jpegcodec_table.h"
	#include "jpegcodec_def.h"
	#include "jpeg_fw_def.h"
	#include "jpeg_common.h"
	#include "jpeg_jfif.h"
	#if defined(JPEG_ENC)
		#include "jpegenc_init.h"
		#include "jpegenc_header.h"
		#include "jpegenc_frame.h"
		#include "jpegenc_bitstream.h"
		#include "jpegenc_malloc.h"
	#elif defined(JPEG_DEC)
		#include "jpegdec_out.h"
		#include "jpegdec_init.h"
		#include "jpegdec_vld.h"
		#include "jpegdec_dequant.h"
		#include "jpegdec_malloc.h"
		#include "jpegdec_bitstream.h"
		#include "jpegdec_parse.h"
		#include "JpegDec_frame.h"
		#include "jpegdec_pvld.h"
	#endif
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
#include "axim_global.h"
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
#include "glb_ctrl_global.h"
#include "rdbk_global.h"     
#include "rdbk_mode.h"       
#endif //_CMODEL_

#ifndef _VSP_LINUX_
#include "os_api.h"
#endif //RUN_IN_PLATFORM

//#define _DEBUG_TIME_
#ifdef _DEBUG_TIME_
#include "sys/time.h"
extern struct timeval tpstart;
extern  struct timeval tpend1;
extern struct timeval tpend2;
#endif
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
#endif  //_SC8825_VIDEO_HEADER_H_