/******************************************************************************
** File Name:      vp8dec_malloc.h                                           *
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
#ifndef _VP8DEC_MALLOC_H_
#define _VP8DEC_MALLOC_H_

#include "mmcodec.h"

void Vp8Dec_InitInterMem(VPXDecObject *vo, MMCodecBuffer *pBuffer);
void *Vp8Dec_InterMemAlloc(VPXDecObject *vo, uint32 need_size, int32 aligned_byte_num);
void Vp8Dec_FreeInterMem(VPXDecObject *vo);
uint8 *Vp8Dec_InterMem_V2P(VPXDecObject *vo, uint8 *vAddr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //VP8DEC_MALLOC_H
