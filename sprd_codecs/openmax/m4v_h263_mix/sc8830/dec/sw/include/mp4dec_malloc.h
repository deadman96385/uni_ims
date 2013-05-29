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
		
void Mp4Dec_InitInterMem(MP4DecObject *vd, MMCodecBuffer *pBuffer);
void Mp4Dec_FreeExtraMem(MP4DecObject *vd); 
void Mp4Dec_FreeMem(MP4DecObject *vd); 

void *Mp4Dec_InterMemAlloc(MP4DecObject *vd, uint32 need_size, int32 aligned_byte_num);
void *Mp4Dec_ExtraMemAlloc(MP4DecObject *vd, uint32 need_size, int32 aligned_byte_num, int32 type);

uint8 *Mp4Dec_ExtraMem_V2P(MP4DecObject *vd, uint8 *vAddr, int32 type);
MMDecRet Mp4Dec_ExtraMem_GetInfo(MP4DecObject *vd, MMCodecBuffer *pBuffer, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif // _MP4DEC_MALLOC_H_
