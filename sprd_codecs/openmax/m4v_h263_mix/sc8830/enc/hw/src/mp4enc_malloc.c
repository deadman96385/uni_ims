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

//for mp4 encoder memory. 4Mbyte,extra
MP4_LOCAL uint32 s_Mp4EncExtraMemUsed = 0x0;
MP4_LOCAL uint32 s_Mp4EncExtraMemSize = 0x0; // = 0x400000;  //16M

//for mp4 decoder memory. 4Mbyte,inter
MP4_LOCAL uint32 s_Mp4EncInterMemUsed = 0x0;
MP4_LOCAL uint32 s_Mp4EncInterMemSize = 0x0; //50kbyte; // = 0x400000;

MP4_LOCAL uint8 *s_pEnc_Extra_buffer = NULL; 
MP4_LOCAL uint8 *s_pEnc_Inter_buffer = NULL;

MP4_LOCAL uint8 *s_pEnc_Inter_buffer_start = NULL;
MP4_LOCAL uint32 s_Mp4EncInterMemPhy = 0x0;

/*****************************************************************************
 **	Name : 			Mp4Enc_ExtraMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void *Mp4Enc_ExtraMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	//mem_size = ((mem_size + 3) &(~3));
	mem_size = ((mem_size + 7) &(~7));//DWORD align

	if((0 == mem_size)||(mem_size> (s_Mp4EncExtraMemSize-s_Mp4EncExtraMemUsed)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	pMem = s_pEnc_Extra_buffer + s_Mp4EncExtraMemUsed;
	s_Mp4EncExtraMemUsed += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_ExtraMemAlloc_64WordAlign
 ** Description:	Alloc the common memory for mp4 encoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Enc_ExtraMemAlloc_64WordAlign(uint32 mem_size)
{
	uint32 CurrAddr, _64WordAlignAddr;
		
	CurrAddr = (uint32)s_pEnc_Extra_buffer + s_Mp4EncExtraMemUsed;

	_64WordAlignAddr = ((CurrAddr + 255) >>8)<<8;

	mem_size += (_64WordAlignAddr - CurrAddr);

	if((0 == mem_size)||(mem_size> (s_Mp4EncExtraMemSize-s_Mp4EncExtraMemUsed)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	s_Mp4EncExtraMemUsed += mem_size;
	
	return (void *)_64WordAlignAddr;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InterMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void *Mp4Enc_InterMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	//mem_size = ((mem_size + 3) &(~3));
	mem_size = ((mem_size + 7) &(~7));//DWORD align

	if((0 == mem_size)||(mem_size> (s_Mp4EncInterMemSize-s_Mp4EncInterMemUsed)))
	{
		SCI_ASSERT(0);
		return 0;
	}
	
	pMem = s_pEnc_Inter_buffer + s_Mp4EncInterMemUsed;
	s_Mp4EncInterMemUsed += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_MemFree
 ** Description:	Free the common memory for mp4 encoder.  
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Enc_MemFree(void) 
{ 
	s_Mp4EncExtraMemUsed = 0;
	s_Mp4EncInterMemUsed = 0;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitMem
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
void Mp4Enc_InitMem (MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtraMemBfr)
{
	s_pEnc_Inter_buffer_start = pInterMemBfr->common_buffer_ptr;
	s_pEnc_Inter_buffer = s_pEnc_Inter_buffer_start;
	s_Mp4EncInterMemSize = pInterMemBfr->size;
	s_Mp4EncInterMemPhy = (uint32)pInterMemBfr->common_buffer_ptr_phy;

	s_pEnc_Extra_buffer = (uint8	*)pExtraMemBfr->common_buffer_ptr_phy;
	s_Mp4EncExtraMemSize = pExtraMemBfr->size;

	//reset memory used count
	s_Mp4EncInterMemUsed = 0;
	s_Mp4EncExtraMemUsed = 0;
}


PUBLIC uint32 Mp4ENC_GetPhyAddr(void * vitual_ptr)
{
	return (((uint32)vitual_ptr - (uint32)s_pEnc_Inter_buffer_start) + s_Mp4EncInterMemPhy);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
