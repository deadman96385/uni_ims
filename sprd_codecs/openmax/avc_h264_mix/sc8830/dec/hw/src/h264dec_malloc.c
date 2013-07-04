/******************************************************************************
 ** File Name:    h264dec_malloc.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         06/09/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei.Luo     Create.                                     *
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

/*****************************************************************************
 **	Name : 			H264Dec_InterMemAlloc
 ** Description:	Alloc the common memory for h264 decoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *H264Dec_InterMemAlloc(H264DecObject *vo, uint32 need_size, int32 aligned_byte_num)
{
    uint32 CurrAddr, AlignedAddr;


    aligned_byte_num = 8;

    CurrAddr = (uint32)(vo->s_inter_mem.v_base) + vo->s_inter_mem.used_size;
    AlignedAddr = (CurrAddr + aligned_byte_num-1) & (~(aligned_byte_num -1));
    need_size += (AlignedAddr - CurrAddr);

//    SCI_TRACE_LOW("%s: left mem size : %d, need mem size : %d", __FUNCTION__,(vo->s_inter_mem.total_size-vo->s_inter_mem.used_size), need_size);

    if((0 == need_size)||(need_size> (vo->s_inter_mem.total_size-vo->s_inter_mem.used_size)))
    {
        SCI_TRACE_LOW("%s  failed, total_size:%d, used_size: %d, need_size:%d\n",
                      __FUNCTION__, vo->s_inter_mem.total_size, vo->s_inter_mem.used_size,need_size);
        return NULL; //lint !e527
    }

    vo->s_inter_mem.used_size+= need_size;

    return (void *)AlignedAddr;
}

/*****************************************************************************
 **	Name : 			H264Dec_FreeInterMem
 ** Description:	Free the common memory for h264 decoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void H264Dec_FreeInterMem(H264DecObject *vo)
{
    vo->s_inter_mem.used_size = 0;
}

PUBLIC void H264Dec_InitInterMem(H264DecObject *vo, MMCodecBuffer *dec_buffer_ptr)
{
    vo->s_inter_mem.used_size = 0;
    vo->s_inter_mem.v_base = dec_buffer_ptr->common_buffer_ptr;
    vo->s_inter_mem.p_base = dec_buffer_ptr->common_buffer_ptr_phy;
    vo->s_inter_mem.total_size = dec_buffer_ptr->size;
    SCI_MEMSET(vo->s_inter_mem.v_base, 0, vo->s_inter_mem.total_size);

    SCI_TRACE_LOW("%s: inter_mem_size:%d\n", __FUNCTION__, vo->s_inter_mem.total_size);

    return;
}

/*****************************************************************************
 ** Note:	 mapping from virtual to physical address
 *****************************************************************************/
PUBLIC uint8 *H264Dec_InterMem_V2P(H264DecObject *vo, uint8 *vAddr)
{
    return ((vAddr-vo->s_inter_mem.v_base)+ vo->s_inter_mem.p_base);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

