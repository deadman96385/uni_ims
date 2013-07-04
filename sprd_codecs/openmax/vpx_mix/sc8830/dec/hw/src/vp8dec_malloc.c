/******************************************************************************
 ** File Name:    vp8dec_malloc.c                                             *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "vp8dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/*****************************************************************************
 **	Name : 			Vp8Dec_InterMemAlloc
 ** Description:	Alloc the common memory for h264 decoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void *Vp8Dec_InterMemAlloc(VPXDecObject *vo, uint32 need_size, int32 aligned_byte_num)
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
 **	Name : 			Vp8Dec_FreeInterMem
 ** Description:	Free the common memory for h264 decoder.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Vp8Dec_FreeInterMem(VPXDecObject *vo)
{
    vo->s_inter_mem.used_size = 0;
}

PUBLIC void Vp8Dec_InitInterMem(VPXDecObject *vo, MMCodecBuffer *dec_buffer_ptr)
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
PUBLIC uint8 *Vp8Dec_InterMem_V2P(VPXDecObject *vo, uint8 *vAddr)
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
