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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//for mp4 decoder memory. 4Mbyte,extra
LOCAL uint32 s_used_extra_mem = 0x0;
LOCAL uint32 s_extra_mem_size = 0x1000000;  //16M

//for mp4 decoder memory. 4Mbyte,inter
LOCAL uint32 s_used_inter_mem = 0x0;
LOCAL uint32 s_inter_mem_size = 0x400000;

LOCAL uint8 *s_extra_mem_bfr_ptr = NULL;
LOCAL uint8 *s_inter_mem_bfr_ptr = NULL;

#ifdef _VSP_LINUX_
LOCAL uint8 *s_extra_mem_bfr_phy_ptr = NULL;
PUBLIC uint8 *Mp4Dec_ExtraMem_V2Phy(uint8 *vAddr)
{
	return (vAddr-s_extra_mem_bfr_ptr)+s_extra_mem_bfr_phy_ptr;
}
LOCAL uint8 *s_extra_mem_cache_bfr_ptr = NULL;
LOCAL uint8 *s_extra_mem_cache_bfr_phy_ptr = NULL;
LOCAL uint32 s_used_extra_mem_cache = 0x0;
LOCAL uint32 s_extra_mem_cache_size = 0x1000000;	//16Mbyte

PUBLIC void *Mp4Dec_ExtraMemCacheAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_extra_mem_cache_size-s_used_extra_mem_cache)))
	{
        SCI_TRACE_LOW("Mp4Dec_ExtraMemCacheAlloc failed,required=%d,total=%d,used=%d",
                        mem_size,s_extra_mem_cache_size,s_used_extra_mem_cache);
        return NULL;
	}
	
	pMem = s_extra_mem_cache_bfr_ptr + s_used_extra_mem_cache;
	s_used_extra_mem_cache += mem_size;
	
	return pMem;
}

MMDecRet MP4DecMemCacheInit(MMCodecBuffer *pBuffer)
{
	s_extra_mem_cache_bfr_ptr = pBuffer->common_buffer_ptr;
	s_extra_mem_cache_bfr_phy_ptr = pBuffer->common_buffer_ptr_phy;
	s_extra_mem_cache_size = pBuffer->size;
	SCI_MEMSET(s_extra_mem_cache_bfr_ptr, 0, s_extra_mem_cache_size);
	
	//reset memory used count
	s_used_extra_mem_cache = 0;

	return MMDEC_OK;
}

#endif
/*****************************************************************************
 **	Name : 			Mp4Dec_ExtraMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_extra_mem_size-s_used_extra_mem)))
	{
        SCI_TRACE_LOW("Mp4Dec_ExtraMemAlloc failed,required=%d,total=%d,used=%d",
                        mem_size,s_extra_mem_size,s_used_extra_mem);
        return NULL;
	}
	
	pMem = s_extra_mem_bfr_ptr + s_used_extra_mem;
	s_used_extra_mem += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ExtraMemAlloc_64WordAlign
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_ExtraMemAlloc_64WordAlign(uint32 mem_size)
{
	uint32 CurrAddr, _64WordAlignAddr;
		
	CurrAddr = (uint32)s_extra_mem_bfr_ptr + s_used_extra_mem;

	_64WordAlignAddr = ((CurrAddr + 255) >>8)<<8;

	mem_size += (_64WordAlignAddr - CurrAddr);

	if((0 == mem_size)||(mem_size> (s_extra_mem_size-s_used_extra_mem)))
	{
        SCI_TRACE_LOW("Mp4Dec_ExtraMemAlloc_64WordAlign failed,required=%d,total=%d,used=%d",
                       mem_size,s_extra_mem_size,s_used_extra_mem);
        return NULL;
	}
	
	s_used_extra_mem += mem_size;
	
	return (void *)_64WordAlignAddr;
}
/*****************************************************************************
 **	Name : 			Mp4Dec_InterMemAlloc
 ** Description:	Alloc the common memory for mp4 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Mp4Dec_InterMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_inter_mem_size-s_used_inter_mem)))
	{
	        SCI_TRACE_LOW("Mp4Dec_ExtraMemAlloc failed,required=%d,total=%d,used=%d",
                        mem_size,s_inter_mem_size,s_used_inter_mem);
		SCI_ASSERT(0);
		return 0; //lint !e527
	}
	
	pMem = s_inter_mem_bfr_ptr + s_used_inter_mem;
	s_used_inter_mem += mem_size;
	
	return pMem;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_MemFree
 ** Description:	Free the common memory for mp4 decoder.  
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_FreeMem(void) 
{ 
	s_used_inter_mem = 0;
	s_used_extra_mem = 0;
#ifdef _VSP_LINUX_
	s_used_extra_mem_cache = 0;
#endif
}

PUBLIC void Mp4Dec_InitInterMem(MMCodecBuffer *dec_buffer_ptr)
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
MMDecRet MP4DecMemInit(MMCodecBuffer *pBuffer)
{
	s_extra_mem_bfr_ptr = pBuffer->common_buffer_ptr;
#ifdef _VSP_LINUX_
	s_extra_mem_bfr_phy_ptr = pBuffer->common_buffer_ptr_phy;
#endif	
	s_extra_mem_size = pBuffer->size;
	SCI_MEMSET(s_extra_mem_bfr_ptr, 0, s_extra_mem_size);
	
	//reset memory used count
	s_used_extra_mem = 0;

	return MMDEC_OK;
}

/*****************************************************************************/
//  Description:   Calculate the size of external memory for mpeg4 decoder
//	Global resource dependence: 
//  Author:        
//	Note:           
/*****************************************************************************/
MMDecRet MP4DecMemSizeCal(uint8 *pInStream, uint32 str_length, MMDecMemSize *mem_size)
{
	MMDecVideoFormat video_format;
	MMDecVideoFormat *video_format_ptr = &video_format;	
	uint16 frm_width, frm_height, frm_width_aligned, frm_height_aligned;
	uint8 mb_x, mb_y;
	uint32 size;
	int8 video_std;
	uint8 *ptr = pInStream;
	int32 counter = 0;
	
	if((ptr[0] == 0x00) && (ptr[1] == 0x00))
	{
		if((ptr[2]&0xFC) == 0x80)
		{
			video_std = ITU_H263;
		}else if((ptr[2]&0xF8) == 0x80)
		{
			video_std = FLV_H263;
		}
		else
			video_std = MPEG4;
	}
	else
		video_std = MPEG4;

	mem_size->video_std = video_std;
	
	while (g_dec_vop_mode_ptr->size_decode_flag == FALSE)
	{
		if (video_std != MPEG4)
		{
			
			video_format_ptr->video_std = video_std;

			video_format_ptr->p_extra = pInStream;

			video_format_ptr->i_extra = str_length;

			Mp4Dec_Reset(g_dec_vop_mode_ptr);
			Mp4Dec_InitBitstream(video_format_ptr->p_extra, video_format_ptr->i_extra);

			if(ITU_H263 == video_format_ptr->video_std)
			{
				Mp4Dec_DecH263PicSize(g_dec_vop_mode_ptr);
			}
			else
			{
				Mp4Dec_FlvH263PicSize(g_dec_vop_mode_ptr);
			}
		}
		else//MPEG4
		{
			video_format_ptr->video_std = video_std;

			video_format_ptr->p_extra = pInStream;

			video_format_ptr->i_extra = str_length;

			Mp4Dec_Reset(g_dec_vop_mode_ptr);			
			Mp4Dec_InitBitstream(video_format_ptr->p_extra, video_format_ptr->i_extra);
			Mp4Dec_DecMp4Header(g_dec_vop_mode_ptr, video_format_ptr->i_extra);
		}

		counter++;

		if (counter > 100)
		{
		    return MMDEC_ERROR;
		}
			
	}

	frm_width = g_dec_vop_mode_ptr->OrgFrameWidth;
	frm_height = g_dec_vop_mode_ptr->OrgFrameHeight;
	mem_size->frame_width = frm_width;
	mem_size->frame_height = frm_height;

	mb_x = (frm_width+15)/16;
	mb_y = (frm_height+15)/16;

	frm_width_aligned = mb_x * 16;
	frm_height_aligned = mb_y * 16;

	size = (frm_width_aligned) * (frm_height_aligned) * 3/2 * 3;	// frame buffer
	if (frm_width <= 176)
	{
		size += (frm_width_aligned) * (frm_height_aligned) * 3/2 * 2;	// frame buffer for deblock
	}
	size += mb_x*mb_y* 6*4;											// mb_info
	size += mb_x*4*8*2;												// vop_mode_ptr->pTopCoeff
	size += mb_x*3*2;												// vop_mode_ptr->pTopLeftDCLine
	size += mb_x * mb_y * 4 * 6;									// data-partition
	size += 1024;

	mem_size->memSize = size;

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
