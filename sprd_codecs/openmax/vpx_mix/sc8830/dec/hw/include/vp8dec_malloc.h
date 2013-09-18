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

#include "mmcodec.h"

#define CHECK_MALLOC(addr, str)    \
        if (NULL == addr)   \
        {   \
            SCI_TRACE_LOW ("%s, %s is NULL", __FUNCTION__, str);   \
            return MMENC_MEMORY_ERROR;  \
        }

MMDecRet Vp8Dec_InitInterMem (VPXDecObject *vo, MMCodecBuffer *pInterMemBfr);
void *Vp8Dec_MemAlloc (VPXDecObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type);
uint32 Vp8Dec_MemV2P(VPXDecObject *vo, uint8 *vAddr, int32 type);

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
#endif //VP8DEC_MALLOC_H
