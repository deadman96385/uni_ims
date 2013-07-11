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

#define H264_MALLOC_PRINT   //ALOGE

/*****************************************************************************
 ** Note:	Alloc the internal memory for h264 decoder.
 *****************************************************************************/
PUBLIC void *H264Dec_InterMemAlloc(H264DecContext *img_ptr, uint32 need_size, int32 aligned_byte_num)
{
    uint32 CurrAddr, AlignedAddr;

    CurrAddr = (uint32)(img_ptr->s_inter_mem.v_base) + img_ptr->s_inter_mem.used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    H264_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

    if((0 == need_size)||(need_size> (img_ptr->s_inter_mem.total_size-img_ptr->s_inter_mem.used_size)))
    {
        ALOGE("%s  failed, total_size:%d, used_size: %d, need_size:%d\n", __FUNCTION__, img_ptr->s_inter_mem.total_size, img_ptr->s_inter_mem.used_size,need_size);
        return NULL; //lint !e527
    }

    img_ptr->s_inter_mem.used_size+= need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	Alloc the external memory for h264 decoder.
 *****************************************************************************/
PUBLIC void *H264Dec_ExtraMemAlloc(H264DecContext *img_ptr, uint32 need_size, int32 aligned_byte_num, int32 type)
{
    uint32 CurrAddr, AlignedAddr;

    CurrAddr = (uint32)img_ptr->s_extra_mem[type].v_base + img_ptr->s_extra_mem[type].used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

    H264_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

    if((0 == need_size)||(need_size >  (img_ptr->s_extra_mem[type].total_size-img_ptr->s_extra_mem[type].used_size)))
    {
        ALOGE("%s  failed, total_size:%d, used_size: %d, need_size:%d, type: %d\n",
              __FUNCTION__, img_ptr->s_extra_mem[type].total_size, img_ptr->s_extra_mem[type].used_size,need_size, type);
        return NULL;
    }

    img_ptr->s_extra_mem[type].used_size += need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	 mapping from virtual to physical address
 *****************************************************************************/
PUBLIC uint8 *H264Dec_ExtraMem_V2P(H264DecContext *img_ptr, uint8 *vAddr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
//        LOGE("%s, memory type is error!", __FUNCTION__);
        return NULL;
    } else
    {
        return ((vAddr-img_ptr->s_extra_mem[type].v_base)+img_ptr->s_extra_mem[type].p_base);
    }
}

/*****************************************************************************
 **	Note:               for cache flushing operation
 *****************************************************************************/
PUBLIC MMDecRet H264Dec_ExtraMem_GetInfo(H264DecContext *img_ptr, MMCodecBuffer *pBuffer, int32 type)
{
    pBuffer->common_buffer_ptr = img_ptr->s_extra_mem[type].v_base;
    pBuffer->size = img_ptr->s_extra_mem[type].total_size;

    return MMDEC_OK;
}

/*****************************************************************************
 ** Note:	Free the common memory for h264 decoder.
 *****************************************************************************/
PUBLIC void H264Dec_FreeMem(H264DecContext *img_ptr)
{
    img_ptr->s_inter_mem.used_size = 0;
    H264Dec_FreeExtraMem(img_ptr);
}

/*****************************************************************************
 ** Note:	Free the common memory for h264 decoder.
 *****************************************************************************/
PUBLIC void H264Dec_FreeExtraMem(H264DecContext *img_ptr)
{
    int32 type;

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
        img_ptr->s_extra_mem[type].used_size = 0;
    }

//    H264_MALLOC_PRINT("%s\n", __FUNCTION__);
}

/*****************************************************************************
 ** Note:	initialize internal memory
 *****************************************************************************/
PUBLIC void H264Dec_InitInterMem(H264DecContext *img_ptr, MMCodecBuffer *dec_buffer_ptr)
{
    img_ptr->s_inter_mem.used_size = 0;
    img_ptr->s_inter_mem.v_base = dec_buffer_ptr->int_buffer_ptr;
    img_ptr->s_inter_mem.total_size = dec_buffer_ptr->int_size;
    SCI_MEMSET(img_ptr->s_inter_mem.v_base, 0, img_ptr->s_inter_mem.total_size);

    H264_MALLOC_PRINT("%s: inter_mem_size:%d\n", __FUNCTION__, img_ptr->s_inter_mem.total_size);

    return;
}

/*****************************************************************************
 ** Note:	initialize extra memory, physical continuous and no-cachable
*****************************************************************************/
PUBLIC MMDecRet H264DecMemInit(AVCHandle *avcHandle, MMCodecBuffer pBuffer[])
{
    int32 type;
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
        img_ptr->s_extra_mem[type].used_size = 0;
        img_ptr->s_extra_mem[type].v_base = pBuffer[type].common_buffer_ptr;
        img_ptr->s_extra_mem[type].total_size = pBuffer[type].size;

        H264_MALLOC_PRINT("%s: extra_mem_size:%d\n", __FUNCTION__, img_ptr->s_extra_mem[type].total_size);
    }

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

