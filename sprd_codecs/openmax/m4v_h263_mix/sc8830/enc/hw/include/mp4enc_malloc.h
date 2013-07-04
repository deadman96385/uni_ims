/******************************************************************************
 ** File Name:      mp4dec_malloc.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           01/23/2007                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the malloc function interfaces of       *
 **					mp4 decoder												  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _MP4ENC_MALLOC_H_
#define _MP4ENC_MALLOC_H_
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

void Mp4Enc_InitMem (Mp4EncObject *vo, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr);
void *Mp4Enc_MemAlloc (Mp4EncObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type);
uint8 *Mp4Enc_ExtraMem_V2P(Mp4EncObject *vo, uint8 *vAddr, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _MP4ENC_MALLOC_H_
