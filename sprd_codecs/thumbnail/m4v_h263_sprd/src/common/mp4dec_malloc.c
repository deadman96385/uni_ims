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


#ifdef _VSP_LINUX_
PUBLIC uint8 *Mp4Dec_ExtraMem_V2Phy(VIDEO_DATA_T *vd, uint8 *vAddr)
{
	return (vAddr-vd->s_extra_mem_bfr_ptr)+vd->s_extra_mem_bfr_phy_ptr;
}

PUBLIC void *Mp4Dec_ExtraMemCacheAlloc(VIDEO_DATA_T *vd, uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (vd->s_extra_mem_cache_size-vd->s_used_extra_mem_cache)))
	{
        SCI_TRACE_LOW("Mp4Dec_ExtraMemCacheAlloc failed,required=%d,total=%d,used=%d",
                        mem_size,vd->s_extra_mem_cache_size,vd->s_used_extra_mem_cache);
        return NULL;
	}
	
	pMem = vd->s_extra_mem_cache_bfr_ptr + vd->s_used_extra_mem_cache;
	vd->s_used_extra_mem_cache += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ExtraMemCacheAlloc_64WordAlign
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemCacheAlloc_64WordAlign(VIDEO_DATA_T *vd, uint32 mem_size)
{
	uint32 CurrAddr, _64WordAlignAddr;
		
	CurrAddr = (uint32)vd->s_extra_mem_cache_bfr_ptr + vd->s_used_extra_mem_cache;

	_64WordAlignAddr = ((CurrAddr + 255) >>8)<<8;

	mem_size += (_64WordAlignAddr - CurrAddr);

	if((0 == mem_size)||(mem_size>  (vd->s_extra_mem_cache_size-vd->s_used_extra_mem_cache)))
	{
        SCI_TRACE_LOW("Mp4Dec_ExtraMemCacheAlloc_64WordAlign failed,required=%d,total=%d,used=%d",
                       mem_size,vd->s_extra_mem_size,vd->s_used_extra_mem);
        return NULL;
	}
	
	vd->s_used_extra_mem_cache += mem_size;
	
	return (void *)_64WordAlignAddr;
}

PUBLIC MMDecRet MP4DecMemCacheInit(VideoDecControls *decCtrl, MMCodecBuffer *pBuffer)
{
    	VIDEO_DATA_T *vd = (VIDEO_DATA_T *) decCtrl->videoDecoderData;

//        SCI_TRACE_LOW("MP4DecMemCacheInit: extra_mem_size:%d\n", vd->s_extra_mem_size);


	vd->s_extra_mem_cache_bfr_ptr = pBuffer->common_buffer_ptr;
	vd->s_extra_mem_cache_bfr_phy_ptr = pBuffer->common_buffer_ptr_phy;
	vd->s_extra_mem_cache_size = pBuffer->size;
	SCI_MEMSET(vd->s_extra_mem_cache_bfr_ptr, 0, vd->s_extra_mem_cache_size);
	
	//reset memory used count
	vd->s_used_extra_mem_cache = 0;

	return MMDEC_OK;
}

#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_ExtraMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemAlloc(VIDEO_DATA_T *vd, uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	 SCI_TRACE_LOW("Mp4Dec_ExtraMemAlloc: mem_size:%d\n", mem_size);

	if((0 == mem_size)||(mem_size> (vd->s_extra_mem_size-vd->s_used_extra_mem)))
	{
//        SCI_TRACE_LOW("Mp4Dec_ExtraMemAlloc failed,required=%d,total=%d,used=%d",
//                        mem_size,s_extra_mem_size,s_used_extra_mem);
        return NULL;
	}
	
	pMem = vd->s_extra_mem_bfr_ptr + vd->s_used_extra_mem;
	vd->s_used_extra_mem += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ExtraMemAlloc_64WordAlign
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemAlloc_64WordAlign(VIDEO_DATA_T *vd, uint32 mem_size)
{
	uint32 CurrAddr, _64WordAlignAddr;
		
	CurrAddr = (uint32)vd->s_extra_mem_bfr_ptr + vd->s_used_extra_mem;

	_64WordAlignAddr = ((CurrAddr + 255) >>8)<<8;

	mem_size += (_64WordAlignAddr - CurrAddr);

	if((0 == mem_size)||(mem_size> (vd->s_extra_mem_size-vd->s_used_extra_mem)))
	{
//        SCI_TRACE_LOW("Mp4Dec_ExtraMemAlloc_64WordAlign failed,required=%d,total=%d,used=%d",
//                       mem_size,s_extra_mem_size,s_used_extra_mem);
        return NULL;
	}
	
	vd->s_used_extra_mem += mem_size;
	
	return (void *)_64WordAlignAddr;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InterMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_InterMemAlloc(VIDEO_DATA_T *vd, uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	 SCI_TRACE_LOW("Mp4Dec_InterMemAlloc: mem_size:%d\n", mem_size);

	if((0 == mem_size)||(mem_size> (vd->s_inter_mem_size-vd->s_used_inter_mem)))
	{
//	        SCI_TRACE_LOW("Mp4Dec_InterMemAlloc failed,required=%d,total=%d,used=%d",
//                        mem_size,s_extra_mem_size,s_used_extra_mem);

		SCI_ASSERT(0);
		return 0; //lint !e527
	}
	
	pMem = vd->s_inter_mem_bfr_ptr + vd->s_used_inter_mem;
	vd->s_used_inter_mem += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_MemFree
 ** Description:	Free the common memory for mp4 decoder.  
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_FreeMem(VIDEO_DATA_T *vd) 
{ 
	vd->s_used_inter_mem = 0;
	vd->s_used_extra_mem = 0;
#ifdef _VSP_LINUX_
	vd->s_used_extra_mem_cache = 0;
#endif
}

PUBLIC void Mp4Dec_InitInterMem(VIDEO_DATA_T *vd, MMCodecBuffer *dec_buffer_ptr)
{
	vd->s_inter_mem_bfr_ptr = dec_buffer_ptr->int_buffer_ptr;
	vd->s_inter_mem_size = dec_buffer_ptr->int_size;
	SCI_MEMSET(vd->s_inter_mem_bfr_ptr, 0, vd->s_inter_mem_size);

	 SCI_TRACE_LOW("Mp4Dec_InitInterMem: inter_mem_size:%d\n", vd->s_inter_mem_size);

	//reset memory used count
	vd->s_used_inter_mem = 0;
}

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecMemInit(VideoDecControls *decCtrl, MMCodecBuffer *pBuffer)
{
	VIDEO_DATA_T *vd = decCtrl->videoDecoderData;

	vd->s_extra_mem_bfr_ptr = pBuffer->common_buffer_ptr;
#ifdef _VSP_LINUX_
	vd->s_extra_mem_bfr_phy_ptr = pBuffer->common_buffer_ptr_phy;
#endif	
	vd->s_extra_mem_size = pBuffer->size;
//	SCI_MEMSET(s_extra_mem_bfr_ptr, 0, s_extra_mem_size);

	 SCI_TRACE_LOW("MP4DecMemInit: extra_mem_size:%d\n", vd->s_extra_mem_size);
	
	//reset memory used count
	vd->s_used_extra_mem = 0;

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
