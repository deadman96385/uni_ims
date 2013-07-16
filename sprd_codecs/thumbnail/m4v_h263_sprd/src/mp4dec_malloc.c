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
#include "mp4dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define MP4_MALLOC_PRINT   //ALOGE

/*****************************************************************************
 ** Note:	Alloc the internal memory for h264 decoder.
 *****************************************************************************/
PUBLIC void *Mp4Dec_InterMemAlloc(Mp4DecObject *vd, uint32 need_size, int32 aligned_byte_num)
{
    uint32 CurrAddr, AlignedAddr;

    CurrAddr = (uint32)(vd->s_inter_mem.v_base) + vd->s_inter_mem.used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    MP4_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

    if((0 == need_size)||(need_size> (vd->s_inter_mem.total_size-vd->s_inter_mem.used_size)))
    {
        ALOGE("%s  failed, total_size:%d, used_size: %d, need_size:%d\n", __FUNCTION__, vd->s_inter_mem.total_size, vd->s_inter_mem.used_size,need_size);
        return NULL; //lint !e527
    }

    vd->s_inter_mem.used_size+= need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	Alloc the external memory for h264 decoder.
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemAlloc(Mp4DecObject *vd, uint32 need_size, int32 aligned_byte_num)
{
    uint32 CurrAddr, AlignedAddr;

    CurrAddr = (uint32)vd->s_extra_mem.v_base + vd->s_extra_mem.used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    MP4_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

    if((0 == need_size)||(need_size >  (vd->s_extra_mem.total_size-vd->s_extra_mem.used_size)))
    {
        ALOGE("%s  failed, total_size:%d, used_size: %d, need_size:%d", __FUNCTION__, vd->s_extra_mem.total_size, vd->s_extra_mem.used_size,need_size);
        return NULL;
    }

    vd->s_extra_mem.used_size += need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	Free the common memory for h264 decoder.
 *****************************************************************************/
PUBLIC void Mp4Dec_FreeMem(Mp4DecObject *vd)
{
    vd->s_inter_mem.used_size = 0;
    vd->s_extra_mem.used_size = 0;
}

/*****************************************************************************
 ** Note:	initialize internal memory
 *****************************************************************************/
PUBLIC void Mp4Dec_InitInterMem(Mp4DecObject *vd, MMCodecBuffer *dec_buffer_ptr)
{
    vd->s_inter_mem.used_size = 0;
    vd->s_inter_mem.v_base = dec_buffer_ptr->int_buffer_ptr;
    vd->s_inter_mem.total_size = dec_buffer_ptr->int_size;
    SCI_MEMSET(vd->s_inter_mem.v_base, 0, vd->s_inter_mem.total_size);

    MP4_MALLOC_PRINT("%s: inter_mem_size:%d\n", __FUNCTION__, vd->s_inter_mem.total_size);

    return;
}

/*****************************************************************************
 ** Note:	initialize extra memory, physical continuous and no-cachable
*****************************************************************************/
PUBLIC MMDecRet MP4DecMemInit(MP4Handle *mp4Handle, MMCodecBuffer *pBuffer)
{
    Mp4DecObject *vd = (Mp4DecObject *) mp4Handle->videoDecoderData;

    vd->s_extra_mem.used_size = 0;
    vd->s_extra_mem.v_base = pBuffer->common_buffer_ptr;
    vd->s_extra_mem.total_size = pBuffer->size;

    MP4_MALLOC_PRINT("%s: extra_mem_size:%d\n", __FUNCTION__, vd->s_extra_mem.total_size);

    return MMDEC_OK;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
