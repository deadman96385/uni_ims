/******************************************************************************
 ** File Name:    h264dec_interface.c                                         *
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

PUBLIC void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr =  vo->g_dpb_layer[0];
    int32 i;

    {

        SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);
        if (dpb_ptr->delayed_pic_ptr)
        {
            if (dpb_ptr->delayed_pic_ptr->pBufferHeader)
            {
                (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->delayed_pic_ptr->pBufferHeader);
                dpb_ptr->delayed_pic_ptr->pBufferHeader = NULL;
            }
            dpb_ptr->delayed_pic_ptr = NULL;
        }

        for (i = 0; dpb_ptr->delayed_pic[i]; i++)
        {
            int32 j;

            for (j = 0; j < /*dpb_ptr->used_size*/ MAX_REF_FRAME_NUMBER+1; j++)
            {
                if (dpb_ptr->delayed_pic[j] == dpb_ptr->fs[j]->frame)
                {
                    if(dpb_ptr->fs[j]->is_reference == DELAYED_PIC_REF)
                    {
                        dpb_ptr->fs[j]->is_reference = 0;

#ifdef _VSP_LINUX_
                        if(dpb_ptr->fs[j]->frame->pBufferHeader!=NULL)
                        {
//							SCI_TRACE_LOW("unbind in H264Dec_ReleaseRefBuffers\t");
                            (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->fs[j]->frame->pBufferHeader);
                            dpb_ptr->fs[j]->frame->pBufferHeader = NULL;
                        }
#endif
                    }
                }
            }

            dpb_ptr->delayed_pic[i] = NULL;
            dpb_ptr->delayed_pic_num --;
        }
        H264Dec_flush_dpb(vo, dpb_ptr);
    }

    if( 0 != dpb_ptr->delayed_pic_num )
    {
        SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers delayed_pic_num is %d\n", dpb_ptr->delayed_pic_num);
    }

    for (i = 0; i <  MAX_REF_FRAME_NUMBER+1; i++)
    {
        if (dpb_ptr->fs &&dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame && dpb_ptr->fs[i]->frame->pBufferHeader)
        {
            (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->fs[i]->frame->pBufferHeader);
            dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
            //	dpb_ptr->fs[i]->frame->need_unbind = 0;

//			SCI_TRACE_LOW("H264Dec_ReleaseRefBuffers, unbind\n");
        }
    }



}

PUBLIC MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, uint8 **pOutput, int32 *picId)
{
    *pOutput = NULL;
    return MMDEC_ERROR;
}

void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
    DEC_STORABLE_PICTURE_T *rec_buf_ptr = &(vo->g_rec_buf);

    rec_buf_ptr->imgY =  pFrameY;
    rec_buf_ptr->imgYAddr = (uint32)pFrameY_phy;
    rec_buf_ptr->pBufferHeader = pBufferHeader;
    rec_buf_ptr->mPicId = picId;
}

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc)
{
    int forbidden_zero_bit;

    if (size > 0)
    {
        forbidden_zero_bit = bitstream[0] >> 7;

        if (forbidden_zero_bit != 0)
            return MMDEC_ERROR;

        *nal_ref_idc = (bitstream[0] & 0x60) >> 5;
        *nal_type = bitstream[0] & 0x1F;
        return MMDEC_OK;
    }

    return MMDEC_ERROR;
}

MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
    DEC_SPS_T *sps_ptr = &(vo->g_sps_array_ptr[0]);
    int32 aligned_width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
    int32 aligned_height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;

//    SCI_TRACE_LOW("%s, %d, aligned_width: %d, aligned_height: %d", __FUNCTION__, __LINE__, aligned_width, aligned_height);

    if (pDecInfo == NULL)
    {
        return(MMDEC_PARAM_ERROR);
    }

    pDecInfo->picWidth        = aligned_width;
    pDecInfo->picHeight       = aligned_height;

    if (sps_ptr->frame_cropping_flag)
    {
        pDecInfo->croppingFlag = 1;
        pDecInfo->cropParams.cropLeftOffset = 2 * sps_ptr->frame_crop_left_offset;
        pDecInfo->cropParams.cropOutWidth = aligned_width -
                                            2 * (sps_ptr->frame_crop_left_offset + sps_ptr->frame_crop_right_offset);
        pDecInfo->cropParams.cropTopOffset = 2 * sps_ptr->frame_crop_top_offset;
        pDecInfo->cropParams.cropOutHeight= aligned_height -
                                            2 * (sps_ptr->frame_crop_top_offset + sps_ptr->frame_crop_bottom_offset);
    } else
    {
        pDecInfo->croppingFlag = 0;
        pDecInfo->cropParams.cropLeftOffset = 0;
        pDecInfo->cropParams.cropOutWidth  = 0;
        pDecInfo->cropParams.cropTopOffset = 0;
        pDecInfo->cropParams.cropOutHeight= 0;
    }
    
    /* profile */
    pDecInfo->profile = sps_ptr->profile_idc;//h264bsdProfile(pStorage);
    pDecInfo->numRefFrames = sps_ptr->num_ref_frames;
    pDecInfo->has_b_frames = sps_ptr->vui_seq_parameters->num_reorder_frames;

    return(MMDEC_OK);

}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    H264DecObject*vo = NULL;
    MMDecRet ret = MMDEC_ERROR;

    SCI_TRACE_LOW("libomx_avcdec_hw_sprd.so is built on %s %s, Copyright (C) Spreatrum, Inc.", __DATE__, __TIME__);

    SCI_ASSERT(NULL != buffer_ptr);
    SCI_ASSERT(NULL != pVideoFormat);

    vo = (H264DecObject *) (buffer_ptr->common_buffer_ptr);
    memset(vo, 0, sizeof(H264DecObject));
    avcHandle->videoDecoderData = (void *) vo;
    vo->avcHandle = avcHandle;

    buffer_ptr->common_buffer_ptr += sizeof(H264DecObject);
    buffer_ptr->common_buffer_ptr_phy+= sizeof(H264DecObject);
    buffer_ptr->size -= sizeof(H264DecObject);

    H264Dec_InitInterMem (vo, buffer_ptr);

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    if (VSP_OPEN_Dev((VSPObject *)vo) < 0)
    {
        return ret;
    }

    // Physical memory as internal memory.
    H264Dec_init_global_para (vo);

    if (vo->error_flag)
    {
        return MMDEC_ERROR;
    }
    return MMDEC_OK;
}

//int32 b_video_buffer_malloced = 0;

PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret;
    int32 i;
    uint32 bs_buffer_length, bs_start_addr;
    H264DecObject *vo = (H264DecObject *) avcHandle->videoDecoderData;

    vo->frame_dec_finish=0;

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_H264,"VSP_MODE");
    vo->is_need_init_vsp_hufftab = TRUE;

    // Bitstream.
    bs_start_addr=((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
    bs_buffer_length=dec_input_ptr->dataLen;
    vo->g_stream_offset=0;

    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM

    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((V_BIT_31|V_BIT_30)|vo->g_stream_offset),"BSM_cfg1 check startcode");//byte align
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (V_BIT_31|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, V_BIT_2,V_BIT_2,TIME_OUT_CLK, "startcode found");//check bsm is idle

    //Get start code length of first NALU.
    vo->g_slice_datalen=VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");
    vo->g_stream_offset+=vo->g_slice_datalen;
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");
    while(vo->g_stream_offset<bs_buffer_length)
    {
        // Find the next start code and get length of NALU.
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM

        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((V_BIT_31|V_BIT_30)|vo->g_stream_offset),"BSM_cfg1 check startcode");//byte align
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (V_BIT_31|bs_buffer_length),"BSM_cfg0 stream buffer size");//BSM load data
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, V_BIT_2,V_BIT_2,TIME_OUT_CLK, "startcode found");//check bsm is idle

        // Get length of NALU and net bitstream length.
        vo->g_slice_datalen=VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");
        vo->g_nalu_ptr->len=VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_DATA_LEN,"get NAL_DATA_LEN");

        SCI_TRACE_LOW("%s, %d, g_slice_datalen: %d, g_nalu_ptr->len: %d", __FUNCTION__, __LINE__, vo->g_slice_datalen, vo->g_nalu_ptr->len);

        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");

        // Configure BSM for decoding.
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable");//check bsm is idle
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (V_BIT_31|vo->g_stream_offset),"BSM_cfg1 stream buffer offset");//point to the start of NALU.
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, V_BIT_31|((bs_buffer_length+128)&0xfffffffc),"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

        ret = H264DecDecode_NALU(vo, dec_input_ptr, dec_output_ptr);

        SCI_TRACE_LOW("%s, %d, g_slice_datalen: %d, g_stream_offset: %d, bs_buffer_length: %d, frame_dec_finish: %d,ret:  %d ", __FUNCTION__, __LINE__, vo->g_slice_datalen, vo->g_stream_offset, bs_buffer_length, vo->frame_dec_finish, ret);

        vo->g_stream_offset += vo->g_slice_datalen;//dec_input_ptr->dataLen;

        if( (MMDEC_ERROR ==ret) ||vo->frame_dec_finish)//dec_output.frameEffective
        {
            break;	//break loop.
        }
    }

    VSP_RELEASE_Dev((VSPObject *)vo);

    return ret;
}

PUBLIC MMDecRet H264_DecReleaseDispBfr(AVCHandle *avcHandle, uint8 *pBfrAddr)
{
    return MMDEC_OK;
}

MMDecRet H264DecRelease(AVCHandle *avcHandle)
{
    H264DecObject *vo = (H264DecObject *) avcHandle->videoDecoderData;

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
