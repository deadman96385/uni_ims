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
#ifndef _MP4DEC_MALLOC_H_
#define _MP4DEC_MALLOC_H_
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4_basic.h"
#include "mp4dec_mode.h"
#include "mmcodec.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
		
PUBLIC void *Mp4Dec_ExtraMemAlloc(uint32 mem_size);
PUBLIC void *Mp4Dec_ExtraMemAlloc_64WordAlign(uint32 mem_size);
PUBLIC void *Mp4Dec_InterMemAlloc(uint32 mem_size);
PUBLIC void Mp4Dec_FreeMem(void); 
PUBLIC void Mp4Dec_InitInterMem(MMCodecBuffer *pBuffer);
#ifdef _VSP_LINUX_
PUBLIC uint8 *Mp4Dec_ExtraMem_V2Phy(uint8 *vAddr);
PUBLIC void *Mp4Dec_ExtraMemCacheAlloc(uint32 mem_size);
#endif
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif // _MP4DEC_MALLOC_H_
