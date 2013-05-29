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
		
void H264Dec_InitInterMem(H264DecContext *img_ptr, MMCodecBuffer *pBuffer);
void H264Dec_FreeExtraMem(H264DecContext *img_ptr); 
void H264Dec_FreeMem(H264DecContext *img_ptr); 

void *H264Dec_InterMemAlloc(H264DecContext *img_ptr, uint32 need_size, int32 aligned_byte_num);
void *H264Dec_ExtraMemAlloc(H264DecContext *img_ptr, uint32 need_size, int32 aligned_byte_num, int32 type);

uint8 *H264Dec_ExtraMem_V2P(H264DecContext *img_ptr, uint8 *vAddr, int32 type);
MMDecRet H264Dec_ExtraMem_GetInfo(H264DecContext *img_ptr, MMCodecBuffer *pBuffer, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
#endif // _H264DEC_MALLOC_H_
