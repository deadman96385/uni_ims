/******************************************************************************
 ** File Name:    mp4dec_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         01/23/2007                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define MP4DEC_MALLOC_PRINT  ALOGD

LOCAL void Init_Mem (MP4DecObject *vd, MMCodecBuffer *pMem, int32 type)
{
    int32 dw_aligned;

    if (type == HW_CACHABLE)
    {
        dw_aligned = (((uint32)(pMem->common_buffer_ptr) + 7) & (~7)) - ((uint32)(pMem->common_buffer_ptr));
    }else
    {
        dw_aligned = (((uint32)(pMem->common_buffer_ptr) + 255) & (~255)) - ((uint32)(pMem->common_buffer_ptr));
    }

    vd->mem[type].used_size = 0;
    vd->mem[type].v_base = pMem->common_buffer_ptr + dw_aligned;
    vd->mem[type].p_base = (uint32)(pMem->common_buffer_ptr_phy) + dw_aligned;
    vd->mem[type].total_size = pMem->size - dw_aligned;
    SCI_MEMSET(vd->mem[type].v_base, 0, vd->mem[type].total_size);

    MP4DEC_MALLOC_PRINT("%s: type:%d, dw_aligned, %d, v_base: %0x, p_base: %0x, mem_size:%d\n",
                        __FUNCTION__, type, dw_aligned, vd->mem[type].v_base, vd->mem[type].p_base, vd->mem[type].total_size);
}

PUBLIC void Mp4Dec_InitInterMem (MP4DecObject *vo, MMCodecBuffer *pInterMemBfr)
{
    Init_Mem(vo, pInterMemBfr, INTER_MEM);
}

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	memory
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
PUBLIC MMDecRet MP4DecMemInit(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer)
{
    MP4DecObject *vd = (MP4DecObject *) mp4Handle->videoDecoderData;
    int32 type;

    for (type = HW_NO_CACHABLE; type < MAX_MEM_TYPE; type++)
    {   
        Init_Mem(vd, &(pBuffer[type]), type);
    }
    
    return MMDEC_OK;
}

/*****************************************************************************
 ** Note:	Alloc the needed memory
 *****************************************************************************/
PUBLIC void *Mp4Dec_MemAlloc (MP4DecObject *vd, uint32 need_size, int32 aligned_byte_num, int32 type)
{
    CODEC_BUF_T *pMem = &(vd->mem[type]);
    uint32 CurrAddr, AlignedAddr;

    CurrAddr = (uint32)(pMem->v_base) + pMem->used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    MP4DEC_MALLOC_PRINT("%s: mem_size:%d, AlignedAddr: %0x, type: %d\n", __FUNCTION__, need_size, AlignedAddr, type);

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
PUBLIC uint32 Mp4Dec_MemV2P(MP4DecObject *vd, uint8 *vAddr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
        ALOGE ("%s, memory type is error!", __FUNCTION__);
        return NULL;
    } else
    {
        CODEC_BUF_T *pMem = &(vd->mem[type]);

        return ((uint32)(vAddr-pMem->v_base)+pMem->p_base);
    }
}

/*****************************************************************************
 **	Note:   for cache flushing operation
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_Mem_GetInfo(MP4DecObject *vd, MMCodecBuffer *pBuffer, int32 type)
{
    pBuffer->common_buffer_ptr = vd->mem[type].v_base;
    pBuffer->common_buffer_ptr_phy = vd->mem[type].p_base;
    pBuffer->size = vd->mem[type].total_size;

    return MMDEC_OK;
}

/*****************************************************************************
 ** Note:	Free the common memory
 *****************************************************************************/
PUBLIC void Mp4Dec_FreeMem(MP4DecObject *vd) 
{
    int32 type;

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
	vd->mem[type].used_size = 0;
    }
        
    MP4DEC_MALLOC_PRINT("%s\n", __FUNCTION__);
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
