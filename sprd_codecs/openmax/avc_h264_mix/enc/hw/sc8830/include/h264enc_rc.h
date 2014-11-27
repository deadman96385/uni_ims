/******************************************************************************
** File Name:      h264enc_rc.h	                                              *
** Author:         Shangwen li                                                *
** DATE:           11/16/2011                                                 *
** Copyright:      2007 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/16/2011    Shangwen Li     Create.                                     *
** 06/18/2013    Xiaowei Luo     Modify.                                     *
*****************************************************************************/
#ifndef _H264ENC_RC_H_
#define _H264ENC_RC_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "video_common.h"
#include "h264enc_mode.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define TARGET_BITRATE 1000000

//#define SLICE_SIZE TARGET_BITRATE/INTRA_PERIOD/3
//#define SLICE_SIZE 5000
#ifndef SLICE_SIZE
#define SLICE_MB 1
#endif

#define NO_BU_CHANGE	// slice level update, not BU update
#define I_P_RATIO 3 // the bit allocation for I and P

int32 rc_init_GOP (H264EncObject *vo,  RC_GOP_PARAS *rc_gop_paras);
int32 rc_init_pict (H264EncObject *vo, RC_GOP_PARAS *rc_gop_paras, RC_PIC_PARAS *rc_pic_paras);
int32 rc_init_slice (H264EncObject *vo, ENC_IMAGE_PARAMS_T *img_ptr, RC_PIC_PARAS *rc_pic_paras);
void rc_update_pict (H264EncObject *vo, int32 bits, RC_GOP_PARAS *rc_gop_paras);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264ENC_RC_H_

