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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

//extra memory
LOCAL uint32 s_used_extra_mem = 0x0;
LOCAL uint32 s_extra_mem_size = 0x1000000;	//16Mbyte

//inter memory
LOCAL uint32 s_used_inter_mem = 0x0;
LOCAL uint32 s_inter_mem_size = 0x400000;	//4Mbyte

LOCAL uint8 *s_extra_mem_bfr_ptr = NULL;
LOCAL uint8 *s_inter_mem_bfr_ptr = NULL;

#ifdef _VSP_LINUX_
LOCAL uint8 *s_extra_mem_bfr_phy_ptr = NULL;
PUBLIC uint8 *H264Dec_ExtraMem_V2Phy(uint8 *vAddr)
{
	return (vAddr-s_extra_mem_bfr_ptr)+s_extra_mem_bfr_phy_ptr;
}
LOCAL uint8 *s_extra_mem_cache_bfr_ptr = NULL;
LOCAL uint8 *s_extra_mem_cache_bfr_phy_ptr = NULL;
LOCAL uint32 s_used_extra_mem_cache = 0x0;
LOCAL uint32 s_extra_mem_cache_size = 0x1000000;	//16Mbyte
PUBLIC void *H264Dec_ExtraMemCacheAlloc(uint32 mem_size)
{
#if 1
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_extra_mem_cache_size-s_used_extra_mem_cache)))
	{
        g_image_ptr->error_flag |= ER_EXTRA_MEMO_ID;
	    SCI_TRACE_LOW("H264Dec_ExtraMemCacheAlloc  failed %d,%d,%d\n",s_used_extra_mem_cache,s_extra_mem_cache_size,mem_size);
		SCI_ASSERT(0);
		return 0; //lint !e527
	}
	
	pMem = s_extra_mem_cache_bfr_ptr + s_used_extra_mem_cache;
	s_used_extra_mem_cache += mem_size;
	
	return pMem;
#else
       return H264Dec_ExtraMemAlloc(mem_size);
#endif
}
MMDecRet H264DecMemCacheInit(MMCodecBuffer *pBuffer)
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
 **	Name : 			H264Dec_ExtraMemAlloc
 ** Description:	Alloc the common memory for h264 decoder. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *H264Dec_ExtraMemAlloc(uint32 mem_size)
{
	uint8 *pMem;
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_extra_mem_size-s_used_extra_mem)))
	{
        g_image_ptr->error_flag |= ER_EXTRA_MEMO_ID;
	    SCI_TRACE_LOW("H264Dec_ExtraMemAlloc s_used_inter_mem  failed %d,%d,%d\n",s_used_extra_mem,s_extra_mem_size,mem_size);
		SCI_ASSERT(0);
		return 0; //lint !e527
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
        g_image_ptr->error_flag |= ER_EXTRA_MEMO_ID;
		SCI_TRACE_LOW("H264Dec_ExtraMemAlloc s_used_inter_mem  failed %d,%d,%d\n",s_used_extra_mem,s_extra_mem_size,mem_size);	
		SCI_ASSERT(0);
		return 0; //lint !e527
	}
	
	s_used_extra_mem += mem_size;

 	//SCI_TRACE_LOW("H264Dec_ExtraMemAlloc_64WordAlign s_used_inter_mem %d\n",s_used_extra_mem);	
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
	mem_size = ((mem_size + 3) &(~3));

	if((0 == mem_size)||(mem_size> (s_inter_mem_size-s_used_inter_mem)))
	{
		g_image_ptr->error_flag |= ER_INTER_MEMO_ID;
		SCI_TRACE_LOW("H264Dec_InterMemAlloc s_used_inter_mem  failed %d,%d,%d\n",s_used_inter_mem,s_inter_mem_size,mem_size);	
		SCI_ASSERT(0);
		return 0; //lint !e527
	}
	
	pMem = s_inter_mem_bfr_ptr + s_used_inter_mem;
	s_used_inter_mem += mem_size;
	//SCI_TRACE_LOW("H264Dec_InterMemAlloc s_used_inter_mem %d\n",s_used_inter_mem);
	return pMem;
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
#ifdef _VSP_LINUX_
	s_used_extra_mem_cache = 0;
#endif	
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
#ifdef _VSP_LINUX_
	s_used_extra_mem_cache = 0;
#endif
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
//  Description:   Calculate the size of external memory for H.264 decoder
//	Global resource dependence: 
//  Author:        
//	Note:          
/*****************************************************************************/
MMDecRet H264DecMemSizeCal(uint8 *pInStream, uint32 str_length, MMDecMemSize *mem_size)
{
	MMDecVideoFormat video_format;
	MMDecVideoFormat *video_format_ptr = &video_format;	
	uint16 frm_width, frm_height, frm_width_aligned, frm_height_aligned;
	uint8 mb_x, mb_y;
	uint32 size;
	uint8 *ptr;
	uint16 offset = 0;
	int32 len = 0;
	uint8 data;

	video_format_ptr->p_extra = pInStream;
	video_format_ptr->i_extra = str_length;

	while (g_image_ptr->size_decode_flag == FALSE)
	{
	      uint32 declen = 0;
	      int zero_num = 0;
	      int stuffing_num = 0;
            int startCode_len = 0;
	      
		ptr = (uint8 *)video_format_ptr->p_extra;
		len = 0;
		offset = 0;

		while (declen < str_length)
	      {
        		data = *ptr++;
        		declen++;

        		if (zero_num < 2)
        		{
        			zero_num++;
        			if(data != 0)
        			{
        				zero_num = 0;
        			}
        		}else
        		{
        			if ((zero_num == 2) && (data == 0x03))
        			{
        				zero_num = 0;
        				stuffing_num++;
        				continue;
        			}
        							
        			if ((zero_num == 2) && (data == 0x1))
        			{
        				startCode_len = 3;
        				break;
        			}

        			if ((zero_num == 3) && (data == 0x1))
        			{
        				startCode_len = 4;
        				break;
        			}
        			if ((zero_num == 4) && (data == 0x1))
        			{
        				startCode_len = 5;
        				break;
        			}
        			if ((zero_num == 5) && (data == 0x1))
        			{
        				startCode_len = 6;
        				break;
        			}
        			if ((zero_num == 6) && (data == 0x1))
        			{
        				startCode_len = 7;
        				break;
        			}
        			if ((zero_num == 7) && (data == 0x1))
        			{
        				startCode_len = 8;
        				break;
        			}

        			if (data == 0)
        			{
        				zero_num++;
        			}else
        			{
        				zero_num = 0;
        			}
		    }
	    }

	    if(startCode_len < 2 || startCode_len > 7)
	        return MMDEC_ERROR;

 		offset = declen;

		video_format_ptr->p_extra = (void *)((uint8*)(video_format_ptr->p_extra) + offset);

		video_format_ptr->i_extra -= offset;

		H264Dec_VSPInit ();

		H264Dec_Read_SPS_PPS_SliceHeader((uint8 *)video_format_ptr->p_extra, video_format_ptr->i_extra);
		
	}
	

	frm_width = (g_sps_ptr->pic_width_in_mbs_minus1+1)*MB_SIZE;
	frm_height = (g_sps_ptr->pic_height_in_map_units_minus1+1)*MB_SIZE;
	mem_size->frame_width = frm_width;
	mem_size->frame_height = frm_height;

	mb_x = (frm_width+15)/16;
	mb_y = (frm_height+15)/16;

	frm_width_aligned = mb_x * 16;
	frm_height_aligned = mb_y * 16;

	size = (frm_width_aligned+48) * (frm_height_aligned+48) * 3/2 * (MAX_REF_FRAME_NUMBER+1);	// frame buffer
	size += mb_x*mb_y* 62*4;											// mb_info
	size += mb_x * mb_y * 1;											// MbToSliceGroupMap
	size += mb_x * 32;													// ipred_top_line_buffer
	size += (MAX_REF_FRAME_NUMBER+1)*(3+(7+17)) *4;
	size += 300;

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
		