/******************************************************************************
** File Name:      vp8dec_init.h                                           *
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
#ifndef _VP8DEC_INIT_H_
#define _VP8DEC_INIT_H_

#include "vp8dec_mode.h"

uint32 BitstreamReadBits (VPXDecObject *vo, uint32 nbits);
void Vp8Dec_InitVSP(VPXDecObject *vo);
void Write_tbuf_Probs(VPXDecObject *vo);
void Vp8Dec_create_decompressor(VPXDecObject *vo);
void vp8_create_common(VP8_COMMON *oci);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //_VP8DEC_INIT_H_
