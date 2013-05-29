#include "sc6800x_video_header.h"

LOCAL uint32 s_H264EncExtraMemUsed = 0x0;
LOCAL uint32 s_H264EncExtraMemSize = 0x0;

LOCAL uint32 s_H264EncInterMemUsed = 0x0;
LOCAL uint32 s_H264EncInterMemSize = 0x0;

LOCAL uint8 *s_pEnc_Extra_buffer = NULL;
LOCAL uint8 *s_pEnc_Inter_buffer = NULL;

PUBLIC void *h264enc_extra_mem_alloc (uint32 mem_size)
{
	uint8 *mem_ptr;

	mem_size = ((mem_size + 7) &(~7));	//dword align //weihu
	//mem_size = ((mem_size + 3) &(~3));	//word align

	if ( (0 == mem_size) || (mem_size > (s_H264EncExtraMemSize- s_H264EncExtraMemUsed)) )
	{
		SCI_ASSERT(0);
		return 0;
	}

	mem_ptr = s_pEnc_Extra_buffer + s_H264EncExtraMemUsed;
	s_H264EncExtraMemUsed += mem_size;

	return mem_ptr;
}

/*****************************************************************************
 **	Name : 			H264Dec_ExtraMemAlloc_64WordAlign
 ** Description:	Alloc the common memory for h264 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *H264Enc_ExtraMemAlloc_64WordAlign(uint32 mem_size)
{
	uint32 CurrAddr, _64WordAlignAddr;
		
	CurrAddr = (uint32)s_pEnc_Extra_buffer + s_H264EncExtraMemUsed;

	_64WordAlignAddr = ((CurrAddr + 255) >>8)<<8;

	mem_size += (_64WordAlignAddr - CurrAddr);

	if((0 == mem_size)||(mem_size> (s_H264EncExtraMemSize-s_H264EncExtraMemUsed)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	s_H264EncExtraMemUsed += mem_size;
	
	return (void *)_64WordAlignAddr;
}

PUBLIC void *h264enc_inter_mem_alloc (uint32 mem_size)
{
	uint8 *mem_ptr;

	mem_size = ((mem_size + 7) &(~7));	//dword align //weihu
	//mem_size = ((mem_size + 3) &(~3));	//word align

	if ( (0 == mem_size) || (mem_size > (s_H264EncInterMemSize- s_H264EncInterMemUsed)) )
	{
		SCI_ASSERT(0);
		return 0;
	}

	mem_ptr = s_pEnc_Inter_buffer + s_H264EncInterMemUsed;
	s_H264EncInterMemUsed += mem_size;

	return mem_ptr;
}

PUBLIC void h264enc_mem_free (void)
{
	s_H264EncExtraMemUsed = 0;
	s_H264EncInterMemUsed = 0;
}

PUBLIC 	void h264enc_mem_init (MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr)
{
	s_pEnc_Inter_buffer = pInterMemBfr->common_buffer_ptr;
	s_H264EncInterMemSize = pInterMemBfr->size;

	s_pEnc_Extra_buffer = pExtraMemBfr->common_buffer_ptr;
	s_H264EncExtraMemSize = pExtraMemBfr->size;

	s_H264EncExtraMemUsed = 0;
	s_H264EncInterMemUsed = 0;	
}
