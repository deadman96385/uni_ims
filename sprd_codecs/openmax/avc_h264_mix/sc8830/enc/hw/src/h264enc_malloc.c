/******************************************************************************
 ** File Name:    h264enc_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/17/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 01/23/2007    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define H264ENC_MALLOC_PRINT   //ALOGD

PUBLIC void H264Enc_InitMem (H264EncObject *vo, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr)
{
    MMCodecBuffer *pMem = pInterMemBfr;
    int32 type;

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
        int32 dw_aligned = (((uint32)(pMem->common_buffer_ptr) + 7) & (~7)) - ((uint32)(pMem->common_buffer_ptr));

        vo->mem[type].used_size = 0;
        vo->mem[type].v_base = pMem->common_buffer_ptr;
        vo->mem[type].p_base = pMem->common_buffer_ptr_phy;
        vo->mem[type].total_size = pMem->size;
        SCI_MEMSET(vo->mem[type].v_base, 0, vo->mem[type].total_size);

        H264ENC_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, vo->mem[type].total_size);

        pMem = pExtraMemBfr;
    }
}

/*****************************************************************************
 ** Note:	Alloc the needed memory
 *****************************************************************************/
PUBLIC void *H264Enc_MemAlloc (H264EncObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type)
{
    CODEC_BUF_T *pMem = &(vo->mem[type]);
    uint32 CurrAddr, AlignedAddr;

    CurrAddr = (uint32)(pMem->v_base) + pMem->used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    H264ENC_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

    if((0 == need_size)||(need_size >  (pMem->total_size -pMem->used_size)))
    {
        ALOGE("%s  failed, total_size:%d, used_size: %d, need_size:%d, type: %d\n", __FUNCTION__, pMem->total_size, pMem->used_size,need_size, type);
        return NULL;
    }

    pMem->used_size += need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	 mapping from virtual to physical address
 *****************************************************************************/
PUBLIC uint8 *H264Enc_ExtraMem_V2P(H264EncObject *vo, uint8 *vAddr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
        ALOGE ("%s, memory type is error!", __FUNCTION__);
        return NULL;
    } else
    {
        CODEC_BUF_T *pMem = &(vo->mem[type]);

        return ((vAddr-pMem->v_base)+pMem->p_base);
    }
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

