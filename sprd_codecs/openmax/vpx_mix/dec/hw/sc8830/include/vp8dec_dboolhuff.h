/******************************************************************************
** File Name:      vp8dec_dboolhuff.h                                           *
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
#ifndef _VP8DEC_DBOOLHUFF_H_
#define _VP8DEC_DBOOLHUFF_H_

#include "vp8dec_mode.h"

int32 vp8dx_decode_bool(VPXDecObject *vo, BOOL_DECODER *br, int32 probability) ;
int32 vp8_decode_value(VPXDecObject *vo, BOOL_DECODER *br, int32 bits);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_VP8DEC_DBOOLHUFF_H_
