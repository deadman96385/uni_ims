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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//extra memory
LOCAL uint32 s_used_extra_mem;// = 0x0;
LOCAL uint32 s_extra_mem_size;// = 0x1000000;	//16Mbyte

//inter memory
LOCAL uint32 s_used_inter_mem;// = 0x0;
LOCAL uint32 s_inter_mem_size;// = 0x400000;	//4Mbyte

LOCAL uint8 *s_extra_mem_bfr_ptr = NULL;
LOCAL uint8 *s_inter_mem_bfr_ptr = NULL;

/*****************************************************************************
 **	Name : 			H264Dec_ExtraMemAlloc
 ** Description:	Alloc the common memory for h264 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *H264Dec_ExtraMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 7) &(~7));//3 //dword align //weihu

	if((0 == mem_size)||(mem_size> (s_extra_mem_size-s_used_extra_mem)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	pMem = s_extra_mem_bfr_ptr + s_used_extra_mem;
	s_used_extra_mem += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			H264Dec_ExtraMemAlloc_64WordAlign
 ** Description:	Alloc the common memory for h264 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *H264Dec_ExtraMemAlloc_64WordAlign(uint32 mem_size)
{
	uint32 CurrAddr, _64WordAlignAddr;
		
	CurrAddr = (uint32)s_extra_mem_bfr_ptr + s_used_extra_mem;

	_64WordAlignAddr = ((CurrAddr + 255) >>8)<<8;

	mem_size += (_64WordAlignAddr - CurrAddr);

	if((0 == mem_size)||(mem_size> (s_extra_mem_size-s_used_extra_mem)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	s_used_extra_mem += mem_size;
	
	return (void *)_64WordAlignAddr;
}
/*****************************************************************************
 **	Name : 			H264Dec_InterMemAlloc
 ** Description:	Alloc the common memory for h264 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *H264Dec_InterMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 7) &(~7));//3 //dword align //weihu

	if((0 == mem_size)||(mem_size> (s_inter_mem_size-s_used_inter_mem)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	pMem = s_inter_mem_bfr_ptr + s_used_inter_mem;
	s_used_inter_mem += mem_size;
	
	return pMem;
}

PUBLIC void H264Dec_InterMemFree(uint32 mem_size)
{
	//uint8 *pMem;
	mem_size = ((mem_size + 7) &(~7));//3 //dword align //weihu
	
	if((0 == mem_size)||(mem_size>s_used_inter_mem))
	{
		SCI_ASSERT(0);
		//return 0;
	}
	
	s_used_inter_mem -= mem_size;
	//pMem = s_inter_mem_bfr_ptr + s_used_inter_mem;	
	
	//return pMem;
}
/*****************************************************************************
 **	Name : 			H264Dec_MemFree
 ** Description:	Free the common memory for h264 decoder.  
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void H264Dec_FreeMem(void) 
{ 
	s_used_inter_mem = 0;
	s_used_extra_mem = 0;
}

/*****************************************************************************
 **	Name : 			H264Dec_FreeExtraMem
 ** Description:	Free the common memory for h264 decoder.  
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void H264Dec_FreeExtraMem(void) 
{ 
	s_used_extra_mem = 0;
}

PUBLIC void H264Dec_InitInterMem(MMCodecBuffer *dec_buffer_ptr)
{
	s_inter_mem_bfr_ptr = dec_buffer_ptr->int_buffer_ptr;
	s_inter_mem_size = dec_buffer_ptr->int_size;
	SCI_MEMSET(s_inter_mem_bfr_ptr, 0, s_inter_mem_size);
	
	//reset memory used count
	s_used_inter_mem = 0;
}

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet H264DecMemInit(MMCodecBuffer *pBuffer)
{
	s_extra_mem_bfr_ptr = pBuffer->common_buffer_ptr;
	s_extra_mem_size = pBuffer->size;
	SCI_MEMSET(s_extra_mem_bfr_ptr, 0, s_extra_mem_size);
	
	//reset memory used count
	s_used_extra_mem = 0;

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
		