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
//#include "sc8800g_video_header.h"
#include "sci_types.h"
#include "mmcodec.h"
#include "video_common.h"

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

LOCAL uint8 *s_extra_mem_bfr_ptr;
LOCAL uint8 *s_inter_mem_bfr_ptr;
LOCAL uint8 *s_inter_mem_bfr_start = NULL;
LOCAL uint32 s_inter_mem_phy_addr = 0;

/*****************************************************************************
 **	Name : 			vp8Dec_ExtraMemAlloc
 ** Description:	Alloc the common memory for vp8 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *vp8dec_ExtraMemAlloc(uint32 mem_size)
{
	uint8 *pMem;

	mem_size = ((mem_size + 7) &(~7));	//dword align //weihu
	//mem_size = ((mem_size + 3) &(~3));	//word align

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
 **	Name : 			vp8Dec_ExtraMemAlloc_64WordAlign
 ** Description:	Alloc the common memory for vp8 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *vp8dec_ExtraMemAlloc_64WordAlign(uint32 mem_size)
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
 **	Name : 			vp8Dec_InterMemAlloc
 ** Description:	Alloc the common memory for vp8 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *vp8dec_InterMemAlloc(uint32 mem_size)
{
	uint8 *pMem;

	mem_size = ((mem_size + 7) &(~7));	//dword align //weihu
	//mem_size = ((mem_size + 3) &(~3));	//word align

	if((0 == mem_size)||(mem_size> (s_inter_mem_size-s_used_inter_mem)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	pMem = s_inter_mem_bfr_ptr + s_used_inter_mem;
	s_used_inter_mem += mem_size;
	
	return pMem;
}

PUBLIC void vp8dec_ExtraMemFree(uint32 mem_size)
{
	mem_size = ((mem_size + 7) &(~7));	//dword align //weihu
	//mem_size = ((mem_size + 3) &(~3));	//word align
	
	if((0 == mem_size)||(mem_size>s_used_extra_mem))
	{
		SCI_ASSERT(0);
		//return 0;
	}
	s_used_extra_mem -= mem_size;
}

PUBLIC void vp8dec_InterMemFree(uint32 mem_size)
{
	//uint8 *pMem;

	mem_size = ((mem_size + 7) &(~7));	//dword align //weihu
	//mem_size = ((mem_size + 3) &(~3));	//word align
	
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
 **	Name : 			vp8Dec_MemFree
 ** Description:	Free the common memory for vp8 decoder.  
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void vp8dec_FreeMem(void) 
{ 
	s_used_inter_mem = 0;
	s_used_extra_mem = 0;
}

PUBLIC void vp8dec_InitInterMem(MMCodecBuffer *dec_buffer_ptr)
{
	s_inter_mem_bfr_start = dec_buffer_ptr->common_buffer_ptr;
	s_inter_mem_bfr_ptr = s_inter_mem_bfr_start;
	s_inter_mem_size = dec_buffer_ptr->size;
	s_inter_mem_phy_addr = (uint32)dec_buffer_ptr->common_buffer_ptr_phy;
	SCI_MEMSET(s_inter_mem_bfr_ptr, 0, s_inter_mem_size);
	
	//reset memory used count
	s_used_inter_mem = 0;
}

PUBLIC uint32 VP8Dec_GetPhyAddr(void * vitual_ptr)
{
	return (((uint32)vitual_ptr - (uint32)s_inter_mem_bfr_start) + s_inter_mem_phy_addr);
}

/*****************************************************************************/
//  Description:   Init mpeg4 decoder	memory
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet VP8DecMemInit(MMCodecBuffer *pBuffer)
{
	s_extra_mem_bfr_ptr = pBuffer->common_buffer_ptr;
	s_extra_mem_size = pBuffer->size;
	// memset to 0 when malloc
//	SCI_MEMSET(s_extra_mem_bfr_ptr, 0, s_extra_mem_size);
	
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
