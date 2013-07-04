/******************************************************************************
 ** File Name:      h264dec_malloc.h                                          *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/09/2013                                                *
 ** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the malloc function interfaces of       *
 **					h264 decoder												  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264DEC_MALLOC_H_
#define _H264DEC_MALLOC_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264dec_basic.h"
#include "h264dec_mode.h"
#include "mmcodec.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void H264Dec_InitInterMem(H264DecObject *vo, MMCodecBuffer *pBuffer);
void *H264Dec_InterMemAlloc(H264DecObject *vo, uint32 need_size, int32 aligned_byte_num);
void H264Dec_FreeInterMem(H264DecObject *vo);
uint8 *H264Dec_InterMem_V2P(H264DecObject *vo, uint8 *vAddr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264DEC_MALLOC_H_
