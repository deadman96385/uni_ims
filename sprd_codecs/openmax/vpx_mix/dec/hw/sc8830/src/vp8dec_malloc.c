/******************************************************************************
 ** File Name:    vp8dec_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define VP8DEC_MALLOC_PRINT   //SPRD_CODEC_LOGD

PUBLIC MMDecRet Vp8Dec_InitMem (VPXDecObject *vo, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr)
{
    MMCodecBuffer *pMem = pInterMemBfr;
    int32 type;

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
        int32 dw_aligned = (((uint_32or64)(pMem->common_buffer_ptr) + 7) & (~7)) - ((uint_32or64)(pMem->common_buffer_ptr));

        vo->mem[type].used_size = 0;
        vo->mem[type].v_base = (uint_32or64)(pMem->common_buffer_ptr) + dw_aligned;
        vo->mem[type].p_base = (uint_32or64)(pMem->common_buffer_ptr_phy) + dw_aligned;
        vo->mem[type].total_size = pMem->size - dw_aligned;

        CHECK_MALLOC((void *)(vo->mem[type].v_base), "vo->mem[type].v_base");
        SCI_MEMSET((void *)(vo->mem[type].v_base), 0, vo->mem[type].total_size);

        VP8DEC_MALLOC_PRINT("%s: type:%d, dw_aligned, %d, v_base: 0x%lx, p_base: 0x%lx, mem_size:%d\n",
                            __FUNCTION__, type, dw_aligned, vo->mem[type].v_base, vo->mem[type].p_base, vo->mem[type].total_size);

        pMem = pExtraMemBfr;
    }

    return MMDEC_OK;
}

/*****************************************************************************
 ** Note:	Alloc the needed memory
 *****************************************************************************/
PUBLIC void *Vp8Dec_MemAlloc (VPXDecObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type)
{
    CODEC_BUF_T *pMem = &(vo->mem[type]);
    uint_32or64 CurrAddr, AlignedAddr;

    CurrAddr = pMem->v_base + pMem->used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    VP8DEC_MALLOC_PRINT("%s: mem_size:%d, AlignedAddr: %lx, type: %d\n", __FUNCTION__, need_size, AlignedAddr, type);

    if((0 == need_size)||(need_size >  (pMem->total_size -pMem->used_size)))
    {
        SPRD_CODEC_LOGE ("%s  failed, total_size:%d, used_size: %d, need_size:%d, type: %d\n", __FUNCTION__, pMem->total_size, pMem->used_size,need_size, type);
        return NULL;
    }

    pMem->used_size += need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	 mapping from virtual to physical address
 *****************************************************************************/
PUBLIC uint_32or64 Vp8Dec_ExtraMem_V2P(VPXDecObject *vo, uint8 *vAddr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
        SPRD_CODEC_LOGE ("%s, memory type is error!", __FUNCTION__);
        return (uint_32or64)NULL;
    } else
    {
        CODEC_BUF_T *pMem = &(vo->mem[type]);

        return ((uint_32or64)(vAddr)-pMem->v_base+pMem->p_base);
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
