/******************************************************************************
 ** File Name:    h264dec_interface.c                                         *
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
#include "h264dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_STORABLE_PICTURE_T *rec_buf_ptr = &(img_ptr->g_rec_buf);

    rec_buf_ptr->imgY =  pFrameY;
    rec_buf_ptr->imgYAddr = (uint_32or64)pFrameY_phy;
    rec_buf_ptr->pBufferHeader = pBufferHeader;
    rec_buf_ptr->mPicId = picId;
}

void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;
    int32 i;

    if(dpb_ptr)
    {
        H264Dec_clear_delayed_buffer(img_ptr);
        H264Dec_flush_dpb(img_ptr, dpb_ptr);
    }

    for (i = 0; i <  MAX_REF_FRAME_NUMBER+1; i++)
    {
        if (dpb_ptr->fs &&dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame)
        {
            H264DEC_UNBIND_FRAME(img_ptr, dpb_ptr->fs[i]->frame);
        }
    }

    if(img_ptr->g_old_slice_ptr)
    {
        img_ptr->g_old_slice_ptr->frame_num = -1;
    }

    img_ptr->g_searching_IDR_pic = TRUE;
}

MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, void **pOutput, int32 *picId)
{
    int32 i;
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;

    //for multi-slice case, we push the current decoding picture into dpb->delayed_pic queue whether it has been finished or not.
    if (img_ptr->g_dec_picture_ptr != NULL)
    {
        dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = img_ptr->g_dec_picture_ptr;
        img_ptr->g_dec_picture_ptr = NULL;
    }

    //pop one picture from delayed picture queue.
    if (dpb_ptr && dpb_ptr->delayed_pic_num)
    {
        *pOutput = dpb_ptr->delayed_pic[0]->pBufferHeader;
        *picId = dpb_ptr->delayed_pic[0]->mPicId;

        H264DEC_UNBIND_FRAME(img_ptr, dpb_ptr->delayed_pic[0]);

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
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_SPS_T *sps_ptr = NULL;
    int32 aligned_width, aligned_height;

    if (pDecInfo == NULL)
    {
        return (MMDEC_PARAM_ERROR);
    }

    if (img_ptr->g_sps_array_ptr && img_ptr->g_sps_ptr && (img_ptr->g_sps_ptr->seq_parameter_set_id < MAX_SPS))
    {
        sps_ptr = &(img_ptr->g_sps_array_ptr[img_ptr->g_sps_ptr->seq_parameter_set_id]);
    }

    if((sps_ptr == NULL) || (sps_ptr->vui_seq_parameters == NULL))
    {
        return (MMDEC_ERROR);
    }

    aligned_width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
    aligned_height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;
//    SPRD_CODEC_LOGD ("%s, %d, aligned_width: %d, aligned_height: %d", __FUNCTION__, __LINE__, aligned_width, aligned_height);

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
        pDecInfo->cropParams.cropOutWidth  = pDecInfo->picWidth;
        pDecInfo->cropParams.cropTopOffset = 0;
        pDecInfo->cropParams.cropOutHeight= pDecInfo->picHeight;
    }

    //added for bug#154484 and bug#154498
    if ((pDecInfo->cropParams.cropLeftOffset + pDecInfo->cropParams.cropOutWidth
            > pDecInfo->picWidth) ||
            (pDecInfo->cropParams.cropTopOffset + pDecInfo->cropParams.cropOutHeight
             > pDecInfo->picHeight))
    {
        return(MMDEC_ERROR);
    }

    /* profile */
    pDecInfo->profile = sps_ptr->profile_idc;//h264bsdProfile(pStorage);
    pDecInfo->numRefFrames = sps_ptr->num_ref_frames;
    pDecInfo->has_b_frames = sps_ptr->vui_seq_parameters->num_reorder_frames;

    return(MMDEC_OK);

}

MMDecRet H264GetCodecCapability(AVCHandle *avcHandle, MMDecCapability *Capability)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);

    //can support 1080p
    Capability->max_width = 1920;
    Capability->max_height = 1088;
    Capability->profile = AVC_HIGH;
    Capability->level = AVC_LEVEL4_1;

    return MMDEC_OK;
}

MMDecRet H264DecSetParameter(AVCHandle *avcHandle, MMDecVideoFormat * pVideoFormat)
{
    H264DecContext *img_ptr = NULL;
    MMDecRet ret = MMDEC_OK;

    SCI_ASSERT(NULL != pVideoFormat);
    img_ptr = (H264DecContext *) avcHandle->videoDecoderData;
    img_ptr->yuv_format = pVideoFormat->yuv_format;

    return ret;
}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    H264DecContext *img_ptr = NULL;
    MMDecRet ret = MMDEC_ERROR;

    SPRD_CODEC_LOGI ("libomx_avcdec_sw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    SCI_ASSERT(NULL != buffer_ptr);
    SCI_ASSERT(NULL != pVideoFormat);

    img_ptr = (H264DecContext *) (buffer_ptr->common_buffer_ptr);

    memset(img_ptr, 0, sizeof(H264DecContext));
    avcHandle->videoDecoderData = (void *) img_ptr;
    img_ptr->avcHandle = avcHandle;

    buffer_ptr->common_buffer_ptr += sizeof(H264DecContext);
    buffer_ptr->size -= sizeof(H264DecContext);

    H264Dec_InitInterMem (img_ptr, buffer_ptr);

    img_ptr->g_is_avc1_es = FALSE;
    img_ptr->g_ready_to_decode_slice = FALSE;
    img_ptr->yuv_format = pVideoFormat->yuv_format;

    if (H264Dec_init_global_para (img_ptr) != MMDEC_OK)
    {
        img_ptr->return_pos2 |= (1<<29);
        return MMDEC_ERROR;
    }
    H264Dec_init_vld_table ();

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (img_ptr->error_flag)
    {
        img_ptr->return_pos |= (1<<19);
        return MMDEC_ERROR;
    }
#endif

    return MMDEC_OK;
}

PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret = MMDEC_ERROR;
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    int32 slice_unit_len = 0;
    uint8 *pInStream = dec_input_ptr->pStream;
    int32 last_slice = 0;
    int32 slice_len_sum = 0, rbsp_len = 0;
    int32 stream_lenght = dec_input_ptr->dataLen;

    SCI_ASSERT(NULL != dec_input_ptr);
    SCI_ASSERT(NULL != dec_output_ptr);

    if (dec_input_ptr->dataLen <= 0)
    {
        return MMDEC_ERROR;
    }

    curr_slice_ptr->next_header = -1;
    img_ptr->error_flag = FALSE;
    img_ptr->return_pos = 0;
    img_ptr->return_pos1 = 0;
    img_ptr->return_pos2 = 0;

    if((dec_input_ptr->expected_IVOP) && (img_ptr->curr_mb_nr == 0))
    {
        img_ptr->g_searching_IDR_pic = TRUE;
    }

    dec_output_ptr->frameEffective = 0;
    dec_output_ptr->reqNewBuf = 0;

    while (!last_slice)
    {
        last_slice = get_unit (img_ptr, pInStream, stream_lenght, &slice_unit_len);
        stream_lenght -= slice_unit_len;

        ret = H264Dec_Read_SPS_PPS_SliceHeader (img_ptr, img_ptr->g_nalu_ptr->buf, img_ptr->g_nalu_ptr->len, dec_output_ptr);

#if _H264_PROTECT_ & _LEVEL_LOW_
        if (img_ptr->error_flag)
        {
            img_ptr->return_pos |= (1<<20);

            SPRD_CODEC_LOGE ("%s, %d, img_ptr->error_flag: %0x, pos: %0x, pos1: %0x, pos2: %0x",
                             __FUNCTION__, __LINE__, img_ptr->error_flag, img_ptr->return_pos, img_ptr->return_pos1, img_ptr->return_pos2);

            goto DEC_EXIT;
        }
#endif

        if (img_ptr->g_ready_to_decode_slice)
        {
#if _H264_PROTECT_ & _LEVEL_LOW_
            if (img_ptr->g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
            {
                img_ptr->error_flag |= ER_PICTURE_NULL_ID;
                img_ptr->return_pos |= (1<<21);
                goto DEC_EXIT;
            }
#endif

            if ((dec_input_ptr->expected_IVOP) && (img_ptr->type != I_SLICE))
            {
                SPRD_CODEC_LOGW ("%s, %d, need I slice, return", __FUNCTION__, __LINE__);
                return MMDEC_FRAME_SEEK_IVOP;
            }

            ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);
        }

        pInStream += slice_unit_len;

        if (img_ptr->g_need_back_last_word)
        {
            uint_32or64 byte_rest;
            int32 *word_align_pIn;
            byte_rest = (uint_32or64)pInStream;
            byte_rest = ((byte_rest)>>2)<<2;
            word_align_pIn = (int32 *)(byte_rest);

            *word_align_pIn = img_ptr->g_back_last_word;
        }

        //Added for Bug#162875
        if (SOP == curr_slice_ptr->next_header)
        {
            last_slice = 1;
            img_ptr->type = -1;
        }
    }

DEC_EXIT:

    //need IVOP but not found IDR,then return seek ivop
    if(dec_input_ptr->expected_IVOP && img_ptr->g_searching_IDR_pic)
    {
        SPRD_CODEC_LOGW ("H264DecDecode: need IVOP\n");
        img_ptr->g_dec_picture_ptr = NULL;
        return MMDEC_FRAME_SEEK_IVOP;
    }

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (img_ptr->error_flag)
    {
        SPRD_CODEC_LOGE ("%s, %d, img_ptr->error_flag: %0x, pos: %0x, pos1: %0x, pos2: %0x",
                         __FUNCTION__, __LINE__, img_ptr->error_flag, img_ptr->return_pos, img_ptr->return_pos1, img_ptr->return_pos2);

        img_ptr->g_old_slice_ptr->frame_num = -1;
        img_ptr->curr_mb_nr = 0;
        img_ptr->return_pos |= (1<<22);
        img_ptr->g_dec_picture_ptr =NULL; //Added for bug352453
        H264Dec_clear_delayed_buffer(img_ptr);

        if (img_ptr->not_supported)
        {
            return MMDEC_NOT_SUPPORTED;
        }

        if (img_ptr->error_flag & ER_EXTRA_MEMO_ID)
        {
            SPRD_CODEC_LOGE ("%s, %d", __FUNCTION__, __LINE__);
            return MMDEC_MEMORY_ERROR;
        }

        return MMDEC_ERROR;
    } else
#endif
    {
        dec_input_ptr->dataLen -= stream_lenght;
        return MMDEC_OK;
    }
}

MMDecRet H264DecRelease(AVCHandle *avcHandle)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);

    H264Dec_ReleaseRefBuffers(avcHandle);

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
