/******************************************************************************
 ** File Name:      mp4enc_ratecontrol.h                                      *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/09/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the operation interfaces of rate control*
 **                 of mp4 encoder.				                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/

#ifndef _RATECONTROL_H_
#define _RATECONTROL_H_

/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_basic.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC int rc_single_create(ENC_VOP_MODE_T * vop_mode_ptr, rc_single_t * rc, xvid_plg_data_t * data);
PUBLIC int rc_single_before(rc_single_t * rc, xvid_plg_data_t * data);
PUBLIC int rc_single_after(rc_single_t * rc, xvid_plg_data_t * data);
PUBLIC void Update_lastQp(int8 *QP_last,int n);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_RATECONTROL_H_

