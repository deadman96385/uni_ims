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

#ifdef __ICC
    #define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
    #define DECLARE_ASM_CONST(n,t,v)    const t __attribute__ ((aligned (n))) v
#elif defined(__GNUC__)
    #define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
    #define DECLARE_ASM_CONST(n,t,v)    const t v  __attribute__ ((aligned (n)))
#elif defined(_MSC_VER)
    #define DECLARE_ALIGNED(n,t,v)      __declspec(align(n)) t v
    #define DECLARE_ASM_CONST(n,t,v)    __declspec(align(n)) static const t v
#elif defined(HAVE_INLINE_ASM)
    #error The asm code needs alignment, but we do not know how to do it for this compiler.
#else
    #define DECLARE_ALIGNED(n,t,v)      t v
    #define DECLARE_ASM_CONST(n,t,v)    static const t v
#endif

#ifndef WIN32
#define _NEON_OPT_
#define _ARM_CLZ_OPT_
#include <arm_neon.h>
#endif

//define the protect level for error bitstream
#define _LEVEL_LOW_			(1<<0)		//for common case
#define _LEVEL_MEDIUM_			(1<<1)		//for reserved
#define _LEVEL_HIGH_			(1<<2)		//for CMMB or streaming case
#define _H264_PROTECT_	  	( _LEVEL_LOW_ | _LEVEL_MEDIUM_ | _LEVEL_HIGH_)

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
#include "vsp_drv_sc8810.h"
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

//sw
#include "h264dec_ipred.h"
#include "h264dec_isqt.h"
#include "h264dec_mc.h"

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
#endif  //_SC8810_VIDEO_HEADER_H_