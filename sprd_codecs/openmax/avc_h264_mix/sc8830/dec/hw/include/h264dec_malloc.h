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

#define CHECK_MALLOC(addr, str)    \
        if (NULL == addr)   \
        {   \
            SCI_TRACE_LOW ("%s, %s is NULL", __FUNCTION__, str);   \
            return MMENC_MEMORY_ERROR;  \
        }

MMDecRet H264Dec_InitInterMem (H264DecObject *vo, MMCodecBuffer *pInterMemBfr);
void *H264Dec_MemAlloc (H264DecObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type);
uint32 H264Dec_MemV2P(H264DecObject *vo, uint8 *vAddr, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264DEC_MALLOC_H_
