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
            SPRD_CODEC_LOGE ("%s, %s is NULL\n", __FUNCTION__, str);   \
            return MMENC_MEMORY_ERROR;  \
        }

void H264Dec_InitInterMem (H264DecContext *vo, MMCodecBuffer *pInterMemBfr);
void *H264Dec_MemAlloc (H264DecContext *vo, uint32 need_size, int32 aligned_byte_num, int32 type);
uint_32or64 H264Dec_MemV2P(H264DecContext *vo, uint8 *vAddr, int32 type);
void H264Dec_FreeExtraMem(H264DecContext *vo);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264DEC_MALLOC_H_
