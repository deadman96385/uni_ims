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
#include "mp4_basic.h"
#include "mp4enc_mode.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void *Mp4Enc_ExtraMemAlloc(MP4EncHandle* mp4Handle,uint32 mem_size);
PUBLIC void *Mp4Enc_ExtraMemAlloc_64WordAlign(MP4EncHandle* mp4Handle,uint32 mem_size);
PUBLIC void *Mp4Enc_InterMemAlloc(MP4EncHandle* mp4Handle,uint32 mem_size);
PUBLIC void Mp4Enc_MemFree(MP4EncHandle* mp4Handle);
PUBLIC void Mp4Enc_InitMem(MP4EncHandle* mp4Handle,MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr);
#ifdef _VSP_LINUX_
PUBLIC uint8 *Mp4Enc_ExtraMem_V2Phy(MP4EncHandle* mp4Handle,uint8 *vAddr);
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif // _MP4ENC_MALLOC_H_
