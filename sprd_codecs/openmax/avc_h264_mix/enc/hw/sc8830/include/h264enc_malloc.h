/******************************************************************************
 ** File Name:      h264enc_malloc.h                                           *
 ** Author:         Xiaowei Luo                                               *
 ** DATE:           06/18/2013                                                *
 ** Copyright:      2006 Spreatrum, Incoporated. All Rights Reserved.         *
 ** Description:    This file defines the malloc function interfaces of       *
 **					h264 encoder												  *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/18/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
#ifndef _H264ENC_MALLOC_H_
#define _H264ENC_MALLOC_H_
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

MMEncRet H264Enc_InitMem (H264EncObject *vo, MMCodecBuffer *pMemBfr, int32 type);
void *H264Enc_MemAlloc (H264EncObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type);
uint_32or64 H264Enc_ExtraMem_V2P(H264EncObject *vo, uint8 *vAddr, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif // _H264ENC_MALLOC_H_

