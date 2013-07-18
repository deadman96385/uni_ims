/******************************************************************************
 ** File Name:      sc8810_video_header.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           02/07/2010                                                *
 ** Copyright:      2010 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    common header file for sc8810 MPEG-4/H.263 CODEC.               *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------* 
 ** DATE          NAME            DESCRIPTION                                 * 
 ** 4/23/2007    Binggo Zhou     Create.                                      *
 *****************************************************************************/
#ifndef _SC8810_VIDEO_HEADER_H_
#define _SC8810_VIDEO_HEADER_H_

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
#include "mmcodec.h"
#include "video_common.h"
#include "mp4_basic.h"
#include "mp4_common_func.h"
#include "mp4_global.h"

#include "mp4dec_global.h"
#include "mp4dec_bfrctrl.h"
#include "mp4dec_bitstream.h"
#include "mp4dec_datapartitioning.h"
#include "mp4dec_mode.h"
#include "mp4dec_command.h"

#include "mp4dec_header.h"
#include "mp4dec_malloc.h"
#include "mp4dec_mb.h"
#include "mp4dec_mc.h"
#include "mp4dec_mv.h"
#include "mp4dec_session.h"
#include "mp4dec_vop.h"
#include "mp4dec_vld.h"
#include "mp4dec_rvld.h"
#include "mp4dec_block.h"
#include "mp4dec_error_handle.h"
#include "mpeg4dec.h"

#include "vsp_global_define.h"
#include "vsp_dcam.h"
#include "vsp_dct.h"
#include "vsp_mbc.h"
#include "vsp_dbk.h"
#include "vsp_mca.h"
#include "vsp_bsm.h"
#include "vsp_drv_sc8810.h"
#include "vsp_vld.h"
#include "vsp_vlc.h"
#include "vsp_mea.h"

#ifndef _VSP_LINUX_
//#include "os_api.h"
#endif
//#define _DEBUG_TIME_
#ifdef _DEBUG_TIME_
#include <sys/time.h>
int gettimeofday(struct timeval *tv,struct timezone *tz);

/*typedef struct timeval {
long tv_sec; 
long tv_usec; 
}timeval; 
*/
extern long recMb_time;
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif  //_SC8810_VIDEO_HEADER_H_
