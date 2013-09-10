/******************************************************************************
 ** File Name:    vp8dec_interface.c                                             *
 ** Author:       Xiaowei.Luo                                                 *
 ** DATE:         07/04/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 07/04/2013    Xiaowei.Luo     Create.                                     *
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

void VP8DecSetCurRecPic(VPXHandle *vpxHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
    VPXDecObject *vo = (VPXDecObject *)(vpxHandle->videoDecoderData) ;
    VP8_COMMON *cm = &vo->common;
    YV12_BUFFER_CONFIG *rec_frame = &cm->new_frame;

    rec_frame->buffer_alloc =   pFrameY_phy;
    rec_frame->y_buffer = rec_frame->buffer_alloc ;
    rec_frame->y_buffer_virtual = (uint32)pFrameY;
    rec_frame->pBufferHeader = pBufferHeader;
}

MMDecRet VP8DecInit(VPXHandle *vpxHandle, MMCodecBuffer * buffer_ptr)
{
    VPXDecObject*vo;

    SCI_TRACE_LOW("libomx_vpxdec_hw_sprd.so is built on %s %s, Copyright (C) Spreatrum, Inc.", __DATE__, __TIME__);

    vo = (VPXDecObject *) (buffer_ptr->common_buffer_ptr);
    memset(vo, 0, sizeof(VPXDecObject));
    vpxHandle->videoDecoderData = (void *) vo;
    vo->vpxHandle = vpxHandle;

    buffer_ptr->common_buffer_ptr += sizeof(VPXDecObject);
    buffer_ptr->common_buffer_ptr_phy = (void *)((uint32)(buffer_ptr->common_buffer_ptr_phy) + sizeof(VPXDecObject));
    buffer_ptr->size -= sizeof(VPXDecObject);

    Vp8Dec_InitInterMem (vo, buffer_ptr);

    vo->g_fh_reg_ptr = (VSP_FH_REG_T *)Vp8Dec_InterMemAlloc(vo, sizeof(VSP_FH_REG_T), 4);

    Vp8Dec_create_decompressor(vo);

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->ddr_bandwidth_req_cnt= 0;
    vo->vsp_freq_div= 0;
    if(VSP_OPEN_Dev((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }
    return MMDEC_OK;
}

PUBLIC MMDecRet VP8DecDecode(VPXHandle *vpxHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;

    MMDecRet ret;
    uint32 bs_buffer_length, bs_start_addr;
    VP8_COMMON *cm = &vo->common;

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

//    SCI_TRACE_LOW("%s, %d",__FUNCTION__, __LINE__);
    SCI_TRACE_LOW("pBufferHeader %x,pOutFrameY %x,frame_width %d,frame_height %d", dec_output_ptr->pBufferHeader, dec_output_ptr->pOutFrameY, dec_output_ptr->frame_width,dec_output_ptr->frame_height );

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF,V_BIT_6 | STREAM_ID_VP8,"VSP_MODE");

    // Bitstream.
    bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
    bs_buffer_length=dec_input_ptr->dataLen;
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, 0x08000000,0x00000000,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, 0x6,"BSM_OP clr BSM");//clr BSM
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, /*(g_stream_offset)*/0,"BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, 0x80000000|(bs_buffer_length+128)&0xfffffffc,"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

    ret = vp8dx_receive_compressed_data(vo,  bs_buffer_length,(uint8 *)dec_input_ptr->pStream , 0);
    SCI_TRACE_LOW("%s, %d, ret is %d",__FUNCTION__, __LINE__, ret);

    dec_output_ptr->frameEffective = (cm->show_frame && (ret ==MMDEC_OK ) );
    dec_output_ptr->frame_width =  (((cm->Width+ 15)>>4)<<4);
    dec_output_ptr->frame_height = (((cm->Height+ 15)>>4)<<4);

//    SCI_TRACE_LOW("%s, %d, frameEffective is %d,frame_width %d,frame_height %d",__FUNCTION__, __LINE__, dec_output_ptr->frameEffective, dec_output_ptr->frame_width,dec_output_ptr->frame_height);
    if(dec_output_ptr->frameEffective)
    {
        dec_output_ptr->pBufferHeader = (void *)(cm->frame_to_show->pBufferHeader);
        dec_output_ptr->pOutFrameY = (uint8 *)(cm->frame_to_show->y_buffer_virtual);
        dec_output_ptr->pOutFrameU = (uint8 *)(cm->frame_to_show->u_buffer_virtual);
    }

//    SCI_TRACE_LOW("pBufferHeader %x,pOutFrameY %x", dec_output_ptr->pBufferHeader, dec_output_ptr->pOutFrameY );

    VSP_RELEASE_Dev((VSPObject *)vo);

    // Return output.
    return ret;
}

MMDecRet VP8DecRelease(VPXHandle *vpxHandle)
{
    VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;

    VSP_CLOSE_Dev((VSPObject *)vo);
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

