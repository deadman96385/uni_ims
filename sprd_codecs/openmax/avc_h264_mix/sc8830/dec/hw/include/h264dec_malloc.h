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
		
PUBLIC void *H264Dec_ExtraMemAlloc(uint32 mem_size);
PUBLIC void *H264Dec_ExtraMemAlloc_64WordAlign(uint32 mem_size);
PUBLIC void *H264Dec_InterMemAlloc(uint32 mem_size);
PUBLIC void H264Dec_InterMemFree(uint32 mem_size);//weihu
PUBLIC void H264Dec_FreeExtraMem(void); 
PUBLIC void H264Dec_FreeMem(void); 
PUBLIC void H264Dec_InitInterMem(MMCodecBuffer *pBuffer);
PUBLIC uint32 H264Dec_GetPhyAddr(void * vitual_ptr);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif // _H264DEC_MALLOC_H_
