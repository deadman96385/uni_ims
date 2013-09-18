/******************************************************************************
** File Name:      vp8dec_bfrctrl.h                                           *
** Author:         Xiaowei Luo                                               *
** DATE:           07/04/2013                                                *
** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _VP8DEC_BFRCTRL_H_
#define _VP8DEC_BFRCTRL_H_

#include "vp8dec.h"
#include "vp8dec_mode.h"

void vp8_copy_yv12_buffer(VPXDecObject *vo,VP8_COMMON *cm, YV12_BUFFER_CONFIG *src_frame, YV12_BUFFER_CONFIG *dst_frame);
void vp8_create_common(VP8_COMMON *oci);
void vp8_setup_version(VP8_COMMON *oci);
MMDecRet vp8_init_frame_buffers(VPXDecObject *vo, VP8_COMMON *oci);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif  //_VP8DEC_BFRCTRL_H_