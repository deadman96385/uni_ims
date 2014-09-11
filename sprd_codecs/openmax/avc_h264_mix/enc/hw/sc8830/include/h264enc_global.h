/******************************************************************************
 ** File Name:      h264enc_global.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/18/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:                                                                                                   *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264ENC_GLOBAL_H_
#define _H264ENC_GLOBAL_H_

#include "sci_types.h"

#ifdef   __cplusplus
extern   "C"
{
#endif
extern uint32 g_vlc_hw_tbl [406*2*2];
extern uint32 g_skipBlock_QP_table[52];

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

#endif //_H264ENC_GLOBAL_H_