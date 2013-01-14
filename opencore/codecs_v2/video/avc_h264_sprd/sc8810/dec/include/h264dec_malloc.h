/******************************************************************************
 ** File Name:      h264dec_malloc.h                                          *
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
		
void H264Dec_InitInterMem(MMCodecBuffer *pBuffer);
void H264Dec_FreeExtraMem(void); 
void H264Dec_FreeMem(void); 

void *H264Dec_InterMemAlloc(uint32 need_size, int32 aligned_byte_num);
void *H264Dec_ExtraMemAlloc(uint32 need_size, int32 aligned_byte_num, int32 type);

uint8 *H264Dec_ExtraMem_V2P(uint8 *vAddr, int32 type);
MMDecRet H264Dec_ExtraMem_GetInfo(MMCodecBuffer *pBuffer, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif // _H264DEC_MALLOC_H_
