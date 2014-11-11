/******************************************************************************
 ** File Name:    mp4dec_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/09/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define MP4ENC_MALLOC_PRINT   //ALOGD

PUBLIC MMEncRet Mp4Enc_InitMem (Mp4EncObject *vo, MMCodecBuffer *pMemBfr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
        return MMENC_ERROR;
    } else
    {
        int32 dw_aligned = (((uint_32or64)(pMemBfr->common_buffer_ptr) + 7) & (~7)) - ((uint_32or64)(pMemBfr->common_buffer_ptr));

        vo->mem[type].used_size = 0;
        vo->mem[type].v_base = pMemBfr->common_buffer_ptr;
        vo->mem[type].p_base = pMemBfr->common_buffer_ptr_phy;
        vo->mem[type].total_size = pMemBfr->size;

        CHECK_MALLOC(vo->mem[type].v_base, "vo->mem[type].v_base");
        SCI_MEMSET(vo->mem[type].v_base, 0, vo->mem[type].total_size);

        MP4ENC_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, vo->mem[type].total_size);
    }

    return MMENC_OK;
}

/*****************************************************************************
 ** Note:	Alloc the needed memory
 *****************************************************************************/
PUBLIC void *Mp4Enc_MemAlloc (Mp4EncObject *vo, uint32 need_size, int32 aligned_byte_num, int32 type)
{
    CODEC_BUF_T *pMem = &(vo->mem[type]);
    uint_32or64 CurrAddr, AlignedAddr;

    CurrAddr = (uint_32or64)(pMem->v_base) + pMem->used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    MP4ENC_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

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
PUBLIC uint8 *Mp4Enc_ExtraMem_V2P(Mp4EncObject *vo, uint8 *vAddr, int32 type)
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
