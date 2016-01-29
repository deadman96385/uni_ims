/******************************************************************************
 ** File Name:    h264dec_malloc.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         03/29/2010                                                  *
 ** Copyright:    2010 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 03/29/2010    Xiaowei.Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define H264DEC_MALLOC_PRINT   //SPRD_CODEC_LOGD

LOCAL void Init_Mem (H264DecContext *vo, MMCodecBuffer *pMem, int32 type)
{
    int32 dw_aligned = (((uint_32or64)(pMem->common_buffer_ptr) + 7) & (~7)) - ((uint_32or64)(pMem->common_buffer_ptr));

    vo->mem[type].used_size = 0;
    vo->mem[type].v_base = (uint_32or64)(pMem->common_buffer_ptr) + dw_aligned;
    vo->mem[type].p_base = (uint_32or64)(pMem->common_buffer_ptr_phy) + dw_aligned;
    vo->mem[type].total_size = pMem->size - dw_aligned;

    SCI_MEMSET((void *)(vo->mem[type].v_base), 0, vo->mem[type].total_size);

    H264DEC_MALLOC_PRINT("%s: type:%d, dw_aligned, %d, v_base: %lx, p_base: %lx, mem_size:%d\n",
                         __FUNCTION__, type, dw_aligned, vo->mem[type].v_base, vo->mem[type].p_base, vo->mem[type].total_size);
}

PUBLIC void H264Dec_InitInterMem (H264DecContext *vo, MMCodecBuffer *pInterMemBfr)
{
    Init_Mem(vo, pInterMemBfr, INTER_MEM);
}

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
PUBLIC MMDecRet H264DecMemInit(AVCHandle *avcHandle, MMCodecBuffer *pBuffer)
{
    H264DecContext *vo = (H264DecContext *) avcHandle->videoDecoderData;
    int32 type = SW_CACHABLE;

    Init_Mem(vo, &(pBuffer[type]), type);

    return MMDEC_OK;
}

/*****************************************************************************
 ** Note:	Alloc the needed memory
 *****************************************************************************/
PUBLIC void *H264Dec_MemAlloc (H264DecContext *vo, uint32 need_size, int32 aligned_byte_num, int32 type)
{
    CODEC_BUF_T *pMem = &(vo->mem[type]);
    uint_32or64 CurrAddr, AlignedAddr;

    CurrAddr = (uint_32or64)(pMem->v_base) + pMem->used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    H264DEC_MALLOC_PRINT("%s: mem_size:%d, AlignedAddr: 0x%0x, type: %d, more: %d, pMem->used_size: %d\n",
                         __FUNCTION__, need_size, AlignedAddr, type, (AlignedAddr - CurrAddr), pMem->used_size);

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
PUBLIC uint_32or64 H264Dec_MemV2P(H264DecContext *vo, uint8 *vAddr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
        SPRD_CODEC_LOGE ("%s, memory type is error!\n", __FUNCTION__);
        return (uint_32or64)NULL;
    } else
    {
        CODEC_BUF_T *pMem = &(vo->mem[type]);

        return ((uint_32or64)(vAddr)-pMem->v_base+pMem->p_base);
    }
}

/*****************************************************************************
 ** Note:	Free the common memory for h264 decoder.
 *****************************************************************************/
PUBLIC void H264Dec_FreeExtraMem(H264DecContext *vo)
{
    int32 type;

    for (type = HW_NO_CACHABLE; type < MAX_MEM_TYPE; type++)
    {
        vo->mem[type].used_size = 0;
    }

//    H264_MALLOC_PRINT("%s\n", __FUNCTION__);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

