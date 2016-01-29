/******************************************************************************
** File Name:      vp8dec_malloc.h                                           *
** Author:         Xiaowei Luo                                               *
** DATE:           07/04/2013                                                *
** Copyright:      2013 Spreatrum, Incoporated. All Rights Reserved.         *
** Description:    common define for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/20/2007    Xiaowei Luo     Create.                                     *
*****************************************************************************/
#ifndef _VP8DEC_MALLOC_H_
#define _VP8DEC_MALLOC_H_
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

MMDecRet Vp8Dec_InitMem (VPXDecObject *vo, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr);
void *Vp8Dec_MemAlloc (VPXDecObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type);
uint32 Vp8Dec_MemV2P(VPXDecObject *vo, uint8 *vAddr, int32 type);
uint_32or64 Vp8Dec_ExtraMem_V2P(VPXDecObject *vo, uint8 *vAddr, int32 type);
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //VP8DEC_MALLOC_H
