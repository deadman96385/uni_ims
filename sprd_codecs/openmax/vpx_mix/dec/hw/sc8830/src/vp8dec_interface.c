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

PUBLIC void VP8GetVideoDimensions(VPXHandle *vpxHandle, int32 *display_width, int32 *display_height)
{
    VPXDecObject *vo = (VPXDecObject *)(vpxHandle->videoDecoderData) ;
    VP8_COMMON *cm = &vo->common;

    *display_width = cm->Width;
    *display_height =  cm->Height;

    SPRD_CODEC_LOGD ("%s, %d, display_width: %d, display_height: %d\n", __FUNCTION__, __LINE__, *display_width, *display_height);
}

PUBLIC void VP8GetBufferDimensions(VPXHandle *vpxHandle, int32 *width, int32 *height)
{
    VPXDecObject *vo = (VPXDecObject *)(vpxHandle->videoDecoderData) ;
    VP8_COMMON *cm = &vo->common;

    *width =(((cm->Width + 15)>>4)<<4);
    *height = (((cm->Height + 15)>>4)<<4);

    SPRD_CODEC_LOGD ("%s, %d, buffer_width: %d, buffer_height: %d\n", __FUNCTION__, __LINE__, *width, *height);
}

PUBLIC MMDecRet VP8GetCodecCapability(VPXHandle *vpxHandle, int32 *max_width, int32 *max_height)
{
    VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;

    switch (vo->vsp_version)
    {
    case SHARK:	//limited under 1080p
    case TSHARK:
    case SHARKL:
    case PIKEL:
    case SHARKL64:
    case SHARKLT8:
        *max_width = 1920;
        *max_height = 1088;
        break;
    case DOLPHIN://limited under 720p
    case PIKE:
        *max_width = 1280;
        *max_height = 1023; //720;
        break;
    default:
        *max_width = 352;
        *max_height = 288;
        break;
    }

    return MMDEC_OK;
}

PUBLIC void VP8DecSetCurRecPic(VPXHandle *vpxHandle, uint8 *pFrameY,uint8 *pFrameY_phy,void *pBufferHeader)
{
    VPXDecObject *vo = (VPXDecObject *)(vpxHandle->videoDecoderData) ;
    VP8_COMMON *cm = &vo->common;
    YV12_BUFFER_CONFIG *rec_frame = &cm->new_frame;

    rec_frame->buffer_alloc =   pFrameY_phy;
    rec_frame->y_buffer = rec_frame->buffer_alloc ;
    rec_frame->y_buffer_virtual = (uint_32or64)pFrameY;
    rec_frame->pBufferHeader = pBufferHeader;
}

PUBLIC int32 VP8DecGetLastDspFrm(VPXHandle *vpxHandle,void **pOutput)
{
    VPXDecObject *vo = (VPXDecObject *) vpxHandle->videoDecoderData;
    VP8_COMMON *cm = &vo->common;
    int32 buffer_index;

    *pOutput = NULL;

    return ((NULL != *pOutput) ? 1 : 0);
}

PUBLIC MMDecRet VP8DecInit(VPXHandle *vpxHandle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr, MMDecVideoFormat *pVideoFormat)
{
    VPXDecObject*vo;
    MMDecRet ret;
    char value_dump[PROPERTY_VALUE_MAX];

    SPRD_CODEC_LOGI("libomx_vpxdec_hw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.\n", __DATE__, __TIME__);

    CHECK_MALLOC(pInterMemBfr, "pInterMemBfr");
    CHECK_MALLOC(pInterMemBfr->common_buffer_ptr, "internal memory");
    CHECK_MALLOC(pExtaMemBfr, "pExtaMemBfr");
    CHECK_MALLOC(pExtaMemBfr->common_buffer_ptr, "extranal memory");

    vo = (VPXDecObject *) (pInterMemBfr->common_buffer_ptr);
    memset(vo, 0, sizeof(VPXDecObject));
    vpxHandle->videoDecoderData = (void *) vo;
    vo->vpxHandle = vpxHandle;

    pInterMemBfr->common_buffer_ptr += sizeof(VPXDecObject);
    pInterMemBfr->common_buffer_ptr_phy = pInterMemBfr->common_buffer_ptr_phy + sizeof(VPXDecObject);
    pInterMemBfr->size -= sizeof(VPXDecObject);

    ret = Vp8Dec_InitMem (vo, pInterMemBfr, pExtaMemBfr);
    if (ret != MMDEC_OK)
    {
        return ret;
    }

    vo->g_fh_reg_ptr = (VSP_FH_REG_T *)Vp8Dec_MemAlloc(vo, sizeof(VSP_FH_REG_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_fh_reg_ptr, "vo->g_fh_reg_ptr");

    Vp8Dec_create_decompressor(vo);

    property_get("vp8dec.hw.trace", value_dump, "false");
    vo->trace_enabled = !strcmp(value_dump, "true");

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->vsp_freq_div= SPRD_MAX_VSP_FREQ_LEVEL;
    vo->vsp_version = SHARK;
    vo->yuv_format = pVideoFormat->yuv_format;

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
    uint32 bs_buffer_length, cmd;
    uint_32or64 	bs_start_addr;
    VP8_COMMON *cm = &vo->common;

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        ret = MMDEC_HW_ERROR;
        goto DEC_EXIT;
    }

    cmd = V_BIT_17|V_BIT_16|V_BIT_11|V_BIT_5|V_BIT_3;
    if (vo->yuv_format == YUV420SP_NV21)  //vu format
    {
        cmd |= V_BIT_6;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, cmd,"axim endian set, vu format"); //VSP and OR endian.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (V_BIT_6 | STREAM_ID_VP8),"VSP_MODE");

    dec_output_ptr->frameEffective = FALSE;

    // Bitstream.
    bs_start_addr=((uint_32or64)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
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

    ret = vp8dx_receive_compressed_data(vo,  bs_buffer_length, (uint8 *)dec_input_ptr->pStream);
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

    SPRD_CODEC_LOGD ("%s,  exit decoder, error flag: 0x%x\n", __FUNCTION__, vo->error_flag);
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

    SPRD_CODEC_LOGD ("%s, %d\n", __FUNCTION__, __LINE__);
    VP8DecReleaseRefBuffers(vpxHandle);

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
    int32 buffer_index;

    SPRD_CODEC_LOGD ("%s, %d\n", __FUNCTION__, __LINE__);
    for(buffer_index = 0; buffer_index < YUV_BUFFER_NUM; buffer_index ++)
    {
        if(cm->buffer_pool[buffer_index] != NULL)
        {
            (*(vo->vpxHandle->VSP_unbindCb))(vo->vpxHandle->userdata,(void *)(cm->buffer_pool[buffer_index]), 0);
            cm->buffer_pool[buffer_index] = NULL;
            cm->ref_count[buffer_index] = 0;
        }
    }

    cm->last_frame.pBufferHeader = NULL;
    cm->golden_frame.pBufferHeader = NULL;
    cm->alt_ref_frame.pBufferHeader = NULL;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

