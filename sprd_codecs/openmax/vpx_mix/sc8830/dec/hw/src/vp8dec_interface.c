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

void VP8GetBufferDimensions(VPXHandle *vpxHandle, int32 *width, int32 *height)
{
    VPXDecObject *vo = (VPXDecObject *)(vpxHandle->videoDecoderData) ;
    VP8_COMMON *cm = &vo->common;

    *width =(((cm->Width + 15)>>4)<<4);
    *height = (((cm->Height + 15)>>4)<<4);

    SCI_TRACE_LOW("%s, %d, width: %d, height: %d", __FUNCTION__, __LINE__, *width, *height);
}

MMDecRet VP8GetCodecCapability(VPXHandle *vpxHandle, int32 *max_width, int32 *max_height)
{
	VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;

	int32 codec_capability = vo->vsp_capability;
	if (codec_capability == 0)   //limited under 720p
	{
	    *max_width = 1280;
	    *max_height = 720;
	} else if (codec_capability == 1)   //limited under 1080p
	{
	    *max_width = 1920;
	    *max_height = 1088;
	} else
	{
	    *max_width = 352;
	    *max_height = 288;
	}
	return MMDEC_OK;
}

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

MMDecRet VP8DecInit(VPXHandle *vpxHandle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr)
{
    VPXDecObject*vo;
    MMDecRet ret;

    SCI_TRACE_LOW("libomx_vpxdec_hw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    CHECK_MALLOC(pInterMemBfr, "pInterMemBfr");
    CHECK_MALLOC(pInterMemBfr->common_buffer_ptr, "internal memory");
    CHECK_MALLOC(pExtaMemBfr, "pExtaMemBfr");
    CHECK_MALLOC(pExtaMemBfr->common_buffer_ptr, "extranal memory");

    vo = (VPXDecObject *) (pInterMemBfr->common_buffer_ptr);
    memset(vo, 0, sizeof(VPXDecObject));
    vpxHandle->videoDecoderData = (void *) vo;
    vo->vpxHandle = vpxHandle;

    pInterMemBfr->common_buffer_ptr += sizeof(VPXDecObject);
    pInterMemBfr->common_buffer_ptr_phy = ((uint32)(pInterMemBfr->common_buffer_ptr_phy) + sizeof(VPXDecObject));
    pInterMemBfr->size -= sizeof(VPXDecObject);

    ret = Vp8Dec_InitMem (vo, pInterMemBfr, pExtaMemBfr);
    if (ret != MMENC_OK)
    {
        return ret;
    }

    vo->g_fh_reg_ptr = (VSP_FH_REG_T *)Vp8Dec_MemAlloc(vo, sizeof(VSP_FH_REG_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_fh_reg_ptr, "vo->g_fh_reg_ptr");

    Vp8Dec_create_decompressor(vo);

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->ddr_bandwidth_req_cnt= 0;
    vo->vsp_freq_div= 0;
    vo->vsp_capability = -1;
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
        ret = MMDEC_HW_ERROR;
        goto DEC_EXIT;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (V_BIT_6 | STREAM_ID_VP8),"VSP_MODE");

    dec_output_ptr->frameEffective = FALSE;

    // Bitstream.
    bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
    bs_buffer_length=dec_input_ptr->dataLen;
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_27, 0x00000000, TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        ret = MMDEC_HW_ERROR;
        goto DEC_EXIT;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, (V_BIT_2 | V_BIT_1), "BSM_OP clr BSM");//clr BSM
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_CFG1_OFF, 0, "BSM_cfg1 stream buffer offset & destuff disable");//point to the start of NALU.
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_CFG0_OFF, (V_BIT_31|((bs_buffer_length+128)&0xfffffffc)), "BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

    ret = vp8dx_receive_compressed_data(vo,  bs_buffer_length, (uint8 *)dec_input_ptr->pStream , 0);
    if (ret != MMDEC_OK)
    {
        goto DEC_EXIT;
    }
    dec_output_ptr->frameEffective = (cm->show_frame && (ret ==MMDEC_OK ) );
    dec_output_ptr->frame_width =  (((cm->Width+ 15)>>4)<<4);
    dec_output_ptr->frame_height = (((cm->Height+ 15)>>4)<<4);

    if(dec_output_ptr->frameEffective)
    {
        dec_output_ptr->pBufferHeader = (void *)(cm->frame_to_show->pBufferHeader);
        dec_output_ptr->pOutFrameY = (uint8 *)(cm->frame_to_show->y_buffer_virtual);
        dec_output_ptr->pOutFrameU = (uint8 *)(cm->frame_to_show->u_buffer_virtual);
    }

DEC_EXIT:

    SCI_TRACE_LOW("%s,  exit decoder, error flag: 0x%x", __FUNCTION__, vo->error_flag);

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }

    if (vo->error_flag & ER_HW_ID)
    {
        ret = MMDEC_HW_ERROR;
    }

    return ret;
}

MMDecRet VP8DecRelease(VPXHandle *vpxHandle)
{
    VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;

    if (VSP_CLOSE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    return MMDEC_OK;
}

void VP8DecReleaseRefBuffers(VPXHandle *vpxHandle)
{
    VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;
    VP8_COMMON *cm = &vo->common;
    int buffer_index;

    for(buffer_index = 0; buffer_index <4; buffer_index ++)
    {
        if(cm->buffer_pool[buffer_index]  != NULL)
        {
            (*(vo->vpxHandle->VSP_unbindCb))(vo->vpxHandle->userdata,(void *)(cm->buffer_pool[buffer_index]), 0);
            cm->buffer_pool[buffer_index] = NULL;
            cm->ref_count[buffer_index] = 0;
        }
    }
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

