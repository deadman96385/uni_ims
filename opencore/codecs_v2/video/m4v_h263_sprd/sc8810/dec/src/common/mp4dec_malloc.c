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

#define MP4_MALLOC_PRINT   //LOGD

typedef struct codec_buf_tag
{
    uint32 used_size;
    uint32 total_size;
    uint8* v_base;  //virtual address
    uint8* p_base;  //physical address    
}CODEC_BUF_T;

CODEC_BUF_T s_inter_mem, s_extra_mem[MAX_MEM_TYPE];

/*****************************************************************************
 ** Note:	Alloc the internal memory
 *****************************************************************************/
PUBLIC void *Mp4Dec_InterMemAlloc(uint32 need_size, int32 aligned_byte_num)
{
	uint32 CurrAddr, AlignedAddr;
		
	CurrAddr = (uint32)(s_inter_mem.v_base) + s_inter_mem.used_size;
	AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
	need_size += (AlignedAddr - CurrAddr);

	if((0 == need_size)||(need_size> (s_inter_mem.total_size-s_inter_mem.used_size)))
	{
            SCI_TRACE_LOW("%s  failed, total_size:%d, used_size: %d, need_size:%d\n", __FUNCTION__, s_inter_mem.total_size, s_inter_mem.used_size,need_size);	
            return NULL; //lint !e527
	}
	
	s_inter_mem.used_size+= need_size;

	return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	Alloc the external memory
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemAlloc(uint32 need_size, int32 aligned_byte_num, int32 type)
{
	uint32 CurrAddr, AlignedAddr;
		
	CurrAddr = (uint32)s_extra_mem[type].v_base + s_extra_mem[type].used_size;
	AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
	need_size += (AlignedAddr - CurrAddr);

	MP4_MALLOC_PRINT("%s: mem_size:%d\n", __FUNCTION__, need_size);

	if((0 == need_size)||(need_size >  (s_extra_mem[type].total_size-s_extra_mem[type].used_size)))
	{
            SCI_TRACE_LOW("%s  failed, total_size:%d, used_size: %d, need_size:%d, type: %d\n", __FUNCTION__, s_extra_mem[type].total_size, s_extra_mem[type].used_size,need_size, type);	
            return NULL;
	}
	
	s_extra_mem[type].used_size += need_size;
	
	return (void *)AlignedAddr;
}

/*****************************************************************************
 ** Note:	 mapping from virtual to physical address
 *****************************************************************************/
PUBLIC uint8 *Mp4Dec_ExtraMem_V2P(uint8 *vAddr, int32 type)
{
    if (type >= MAX_MEM_TYPE)
    {
        SCI_TRACE_LOW("%s, memory type is error!", __FUNCTION__);
        return NULL;
    }else
    {
	return ((vAddr-s_extra_mem[type].v_base)+s_extra_mem[type].p_base);
    }
}

/*****************************************************************************
 **	Note:   for cache flushing operation
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_ExtraMem_GetInfo(MMCodecBuffer *pBuffer, int32 type)
{
    pBuffer->common_buffer_ptr = s_extra_mem[type].v_base;
    pBuffer->common_buffer_ptr_phy = s_extra_mem[type].p_base;
    pBuffer->size = s_extra_mem[type].total_size;

    return MMDEC_OK;
}

/*****************************************************************************
 ** Note:	Free the common memory
 *****************************************************************************/
PUBLIC void Mp4Dec_FreeMem(void) 
{ 
	s_inter_mem.used_size = 0;
	Mp4Dec_FreeExtraMem();
}

/*****************************************************************************
 ** Note:	Free the common memory
 *****************************************************************************/
PUBLIC void Mp4Dec_FreeExtraMem(void) 
{
    int32 type;

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
	s_extra_mem[type].used_size = 0;
    }
        
    MP4_MALLOC_PRINT("%s\n", __FUNCTION__);
}

/*****************************************************************************
 ** Note:	initialize internal memory
 *****************************************************************************/
PUBLIC void Mp4Dec_InitInterMem(MMCodecBuffer *dec_buffer_ptr)
{
    s_inter_mem.used_size = 0;
    s_inter_mem.v_base = dec_buffer_ptr->int_buffer_ptr;
    s_inter_mem.total_size = dec_buffer_ptr->int_size;
    SCI_MEMSET(s_inter_mem.v_base, 0, s_inter_mem.total_size);

    MP4_MALLOC_PRINT("%s: inter_mem_size:%d\n", __FUNCTION__, s_inter_mem.total_size);
	
    return;	
}

/*****************************************************************************
 ** Note:	initialize extra memory, physical continuous and no-cachable 
*****************************************************************************/
PUBLIC MMDecRet MP4DecMemInit(MMCodecBuffer pBuffer[])
{
    int32 type;

    for (type = 0; type < MAX_MEM_TYPE; type++)
    {
	s_extra_mem[type].used_size = 0;
	s_extra_mem[type].v_base = pBuffer[type].common_buffer_ptr;
	s_extra_mem[type].p_base = pBuffer[type].common_buffer_ptr_phy;

	s_extra_mem[type].total_size = pBuffer[type].size;

#if 0   //NOT NEED
	SCI_MEMSET(s_extra_mem[type].v_base, 0, s_extra_mem[type].total_size);
#endif

	MP4_MALLOC_PRINT("%s: extra_mem_size:%d\n", __FUNCTION__, s_extra_mem[type].total_size);
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
