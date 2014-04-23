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

    SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

    if (dpb_ptr != NULL)
    {
        H264Dec_clear_delayed_buffer(vo);
        H264Dec_flush_dpb(vo, dpb_ptr);

        for (i = 0; i <  (MAX_REF_FRAME_NUMBER+1); i++)
        {
            if (dpb_ptr->fs &&dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame && dpb_ptr->fs[i]->frame->pBufferHeader)
            {
                (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->fs[i]->frame->pBufferHeader);
                dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
            }
        }
    }
}

PUBLIC MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, void **pOutput, int32 *picId)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr =  vo->g_dpb_layer[0];
    int32 i;


    //for multi-slice case, we push the current decoding picture into dpb->delayed_pic queue whether it has been finished or not.
    if (vo->g_dec_picture_ptr!= NULL)
    {
        dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = vo->g_dec_picture_ptr;
        vo->g_dec_picture_ptr = NULL;
    }

    //pop one picture from delayed picture queue.
    if (dpb_ptr && dpb_ptr->delayed_pic_num)
    {
        *pOutput = dpb_ptr->delayed_pic[0]->pBufferHeader;

        if(dpb_ptr->delayed_pic[0]->pBufferHeader !=NULL)
        {
            (*(vo->avcHandle->VSP_unbindCb))(vo->avcHandle->userdata,dpb_ptr->delayed_pic[0]->pBufferHeader);
            dpb_ptr->delayed_pic[0]->pBufferHeader = NULL;
        }

        for(i =0; i < dpb_ptr->delayed_pic_num; i++)
        {
            dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
        }
        dpb_ptr->delayed_pic_num--;

        return MMDEC_OK;
    } else
    {
        *pOutput = NULL;
        return MMDEC_ERROR;
    }
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
    int32 aligned_width, aligned_height;

    if((sps_ptr == NULL) || (sps_ptr->vui_seq_parameters == NULL))
    {
        return (MMDEC_ERROR);
    }

//    SCI_TRACE_LOW("%s, %d, aligned_width: %d, aligned_height: %d", __FUNCTION__, __LINE__, aligned_width, aligned_height);

    if (pDecInfo == NULL)
    {
        return(MMDEC_PARAM_ERROR);
    }

    aligned_width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
    aligned_height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;

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

MMDecRet H264GetCodecCapability(AVCHandle *avcHandle, int32 *max_width, int32 *max_height)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;

    int32 codec_capability = vo->vsp_capability;
    if (codec_capability == 0)   //limited under 720p
    {
        *max_width = 1280;
        *max_height = 1023; //720;
    } else if (codec_capability == 1)   //limited under 1080p
    {
        *max_width = 1920;
        *max_height = 1088;
    }
    else if (codec_capability == 2)   //limited under 1080p
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

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    H264DecObject*vo;
    MMDecRet ret = MMDEC_OK;

    SCI_TRACE_LOW("libomx_avcdec_hw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    CHECK_MALLOC(pVideoFormat, "pVideoFormat");
    CHECK_MALLOC(buffer_ptr, "buffer_ptr");
    CHECK_MALLOC(buffer_ptr->common_buffer_ptr, "internal memory");

    vo = (H264DecObject *) (buffer_ptr->common_buffer_ptr);
    memset(vo, 0, sizeof(H264DecObject));
    avcHandle->videoDecoderData = (void *) vo;
    vo->avcHandle = avcHandle;

    buffer_ptr->common_buffer_ptr += sizeof(H264DecObject);
    buffer_ptr->common_buffer_ptr_phy+= sizeof(H264DecObject);
    buffer_ptr->size -= sizeof(H264DecObject);

    ret = H264Dec_InitInterMem (vo, buffer_ptr);
    if (ret != MMDEC_OK)
    {
        return ret;
    }

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->vsp_freq_div = 0;
    vo->vsp_capability = -1;
    if (VSP_OPEN_Dev((VSPObject *)vo) < 0)
    {
        return ret;
    }

    ret = H264Dec_init_global_para (vo);
    if (ret != MMDEC_OK)
    {
        return ret;
    }

    if (vo->error_flag)
    {
        ret = MMDEC_ERROR;
    }
    return ret;
}

PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret = MMDEC_OK;
    int32 i, start_code_len = 0;
    uint32 bs_buffer_length, bs_start_addr, destuffing_num;
    H264DecObject *vo = (H264DecObject *) avcHandle->videoDecoderData;

    if ((dec_input_ptr->pStream == NULL) && (!vo->memory_error))
    {
        vo->memory_error = 1;
        return MMDEC_MEMORY_ERROR;
    }

    if (vo->memory_error)
    {
        return MMDEC_ERROR;
    }

    vo->frame_dec_finish=0;
    vo->error_flag = 0;

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+VSP_MODE_OFF, STREAM_ID_H264,"VSP_MODE");
    vo->is_need_init_vsp_hufftab = TRUE;

    // Bitstream.
    bs_start_addr = ((uint32)dec_input_ptr->pStream_phy) ;	// bs_start_addr should be phycial address and 64-biit aligned.
    bs_buffer_length = dec_input_ptr->dataLen;
    vo->pStream = dec_input_ptr->pStream;
    vo->g_stream_offset = 0;

    //for bug281448, add start code at the tail of stream.
    vo->pStream[bs_buffer_length] = 0xf4;
    vo->pStream[bs_buffer_length+1] = 0xf3;
    vo->pStream[bs_buffer_length+2] = 0xf2;
    vo->pStream[bs_buffer_length+3] = 0xf1;

    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        goto DEC_EXIT;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM1_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf1 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM

    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((V_BIT_31|V_BIT_30)|vo->g_stream_offset),"BSM_cfg1 check startcode");//byte align
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (V_BIT_31|((bs_buffer_length < MIN_LEN_FOR_HW) ? MIN_LEN_FOR_HW : bs_buffer_length)),"BSM_cfg0 stream buffer size");//BSM load data
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, V_BIT_2,V_BIT_2,TIME_OUT_CLK, "startcode found"))//check bsm is idle
    {
        goto DEC_EXIT;
    }

    //Get start code length of first NALU.
    start_code_len=VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");
    vo->g_stream_offset+=start_code_len;

    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        goto DEC_EXIT;
    }

    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");
    while(vo->g_stream_offset<bs_buffer_length)
    {
        // Find the next start code and get length of NALU.
        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
        {
            break;
        }
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM1_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf1 addr");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM

        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((V_BIT_31|V_BIT_30)|vo->g_stream_offset),"BSM_cfg1 check startcode");//byte align
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, (V_BIT_31|(((bs_buffer_length - vo->g_stream_offset) < MIN_LEN_FOR_HW) ? (MIN_LEN_FOR_HW + vo->g_stream_offset) : bs_buffer_length)),"BSM_cfg0 stream buffer size");//BSM load data
        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, V_BIT_2,V_BIT_2,TIME_OUT_CLK_FRAME, "startcode found"))//check bsm is idle
        {
            break;
        }

        // Get length of NALU and net bitstream length.
        vo->g_slice_datalen = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + BSM_NAL_LEN, "get NAL_LEN");
        vo->g_nalu_ptr->len = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + BSM_NAL_DATA_LEN, "get NAL_DATA_LEN");
        destuffing_num = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + DSTUF_NUM_OFF, "get DSTUF_NUM");

        //Added for bug293635
        if ((bs_buffer_length - vo->g_stream_offset) < MIN_LEN_FOR_HW)
        {
            int32 added_bytes = (MIN_LEN_FOR_HW - (bs_buffer_length - vo->g_stream_offset));
            vo->g_slice_datalen -= added_bytes;
            vo->g_nalu_ptr->len -= added_bytes;
            SCI_TRACE_LOW("%s, %d, added_bytes: %d", __FUNCTION__, __LINE__, added_bytes);
        }

        SCI_TRACE_LOW("%s, %d, g_stream_offset: %d, g_slice_datalen: %d, g_nalu_ptr->len: %d, destuffing_num: %d", __FUNCTION__, __LINE__,
                      vo->g_stream_offset, vo->g_slice_datalen, vo->g_nalu_ptr->len, destuffing_num);

        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
        {
            break;
        }
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");

        // Configure BSM for decoding.
        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
        {
            break;
        }
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM1_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf1 addr");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (V_BIT_31|vo->g_stream_offset),"BSM_cfg1 stream buffer offset");//point to the start of NALU.
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, V_BIT_31|((bs_buffer_length+128)&0xfffffffc),"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

        ret = H264DecDecode_NALU(vo, dec_input_ptr, dec_output_ptr);

        SCI_TRACE_LOW("%s, %d, g_nalu_ptr->len: %d, frame_dec_finish: %d,ret:  %d ", __FUNCTION__, __LINE__, vo->g_nalu_ptr->len, vo->frame_dec_finish, ret);

        dec_input_ptr->dataLen = vo->g_stream_offset + vo->g_nalu_ptr->len + destuffing_num;

        if( (MMDEC_ERROR == ret) || vo->frame_dec_finish)//dec_output.frameEffective
        {
            if (MMDEC_ERROR == ret)
            {
                dec_input_ptr->dataLen = bs_buffer_length;

                if (vo->error_flag & ER_MEMORY_ID)
                {
                    vo->memory_error = 1;
                    ret = MMDEC_MEMORY_ERROR;
                } else if (vo->error_flag & ER_FORMAT_ID)
                {
                    ret = MMDEC_NOT_SUPPORTED;
                } else if (vo->error_flag & ER_SREAM_ID)
                {
                    ret = MMDEC_STREAM_ERROR;
                } else if (vo->error_flag & (ER_REF_FRM_ID |ER_HW_ID))
                {
                    ret = MMDEC_HW_ERROR;
                } else
                {
                    ret = MMDEC_ERROR;
                }
            }

            break;	//break loop.
        }

        //for next slice
        vo->g_stream_offset += vo->g_slice_datalen;
    }

DEC_EXIT:

    SCI_TRACE_LOW("%s, %d, exit decoder, error_flag: %0x", __FUNCTION__, __LINE__, vo->error_flag);
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

MMDecRet H264DecRelease(AVCHandle *avcHandle)
{
    H264DecObject *vo = (H264DecObject *) avcHandle->videoDecoderData;

    H264Dec_ReleaseRefBuffers(avcHandle);

    if (VSP_CLOSE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

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
