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
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_STORABLE_PICTURE_T *rec_buf_ptr = &(vo->g_rec_buf);

    rec_buf_ptr->imgY =  pFrameY;
    rec_buf_ptr->imgYAddr = (uint_32or64)pFrameY_phy;
    rec_buf_ptr->pBufferHeader = pBufferHeader;
    rec_buf_ptr->mPicId = picId;
}

void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle)
{
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = vo->g_dpb_ptr;
    int32 i;

    if(dpb_ptr)
    {
        H264Dec_clear_delayed_buffer(vo);
        H264Dec_flush_dpb(vo, dpb_ptr);

        for (i = 0; i <  (MAX_REF_FRAME_NUMBER+1); i++)
        {
            if (dpb_ptr->fs &&dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame)
            {
                H264DEC_UNBIND_FRAME(vo, dpb_ptr->fs[i]->frame);
            }
        }
    }

    if(vo->g_old_slice_ptr)
    {
        vo->g_old_slice_ptr->frame_num = -1;
    }

    vo->g_searching_IDR_pic = TRUE;
}

MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, void **pOutput, int32 *picId, uint64 *pts)
{
    int32 i;
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = vo->g_dpb_ptr;

    //for multi-slice case, we push the current decoding picture into dpb->delayed_pic queue whether it has been finished or not.
    if (vo->g_dec_picture_ptr != NULL)
    {
        dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = vo->g_dec_picture_ptr;
        vo->g_dec_picture_ptr = NULL;
    }

    //pop one picture from delayed picture queue.
    if (NULL == dpb_ptr)
    {
        *pOutput = NULL;
        return MMDEC_ERROR;
    } else {
        if (dpb_ptr->delayed_pic_num)
        {
            DEC_STORABLE_PICTURE_T *out = dpb_ptr->delayed_pic[0];
            DEC_FRAME_STORE_T *fs  = H264Dec_search_frame_from_dpb(vo, out);

            if (!fs)
            {
                SPRD_CODEC_LOGE ("%s, %d, fs is NULL!, delayed_pic_num: %d, delayed_pic_ptr: %p\n'", __FUNCTION__, __LINE__, dpb_ptr->delayed_pic_num, dpb_ptr->delayed_pic_ptr);
                *pOutput = NULL;
                dpb_ptr->delayed_pic_ptr = NULL;
                return MMDEC_ERROR;
            }

            for(i =0; i < dpb_ptr->delayed_pic_num; i++)
            {
                dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
            }
            dpb_ptr->delayed_pic_num--;

            H264Dec_find_smallest_pts(vo, out);

            *pOutput = out->pBufferHeader;
            *picId = out->mPicId;
            *pts = out->nTimeStamp;

            SPRD_CODEC_LOGD ("%s, %d, fs->is_reference: %0x\n", __FUNCTION__, __LINE__, fs->is_reference);
            fs->is_reference = 0;
            H264DEC_UNBIND_FRAME(vo, out);

            return MMDEC_OK;
        } else {
            *pOutput = NULL;
            dpb_ptr->delayed_pic_ptr = NULL;
            if (dpb_ptr->delayed_pic_num != 0)
            {
                SPRD_CODEC_LOGE ("%s, %d, dpb_ptr->delayed_pic_num != 0!\n'", __FUNCTION__, __LINE__);
            }
            return MMDEC_ERROR;
        }
    }
}

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc)
{
    if (size > 0) {
        uint32 data;
        int32 forbidden_zero_bit;
        uint8 *ptr = bitstream;
        uint32 len = 0;

        //SPRD_CODEC_LOGI("%s, %d, check Nal type: [%0x, %0x, %0x, %0x, %0x, %0x, %0x]",
        //                 __FUNCTION__, __LINE__, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

        while ((len < size) && (data = ptr[0]) == 0x00) {
            len++;
            ptr++;
        }

        if ((len > 2) && (ptr[0] == 0x1)) {
            ptr++; //start code found, skip the byte '0x1'
        }

        data = ptr[0];
        forbidden_zero_bit = data >> 7;
        if (forbidden_zero_bit != 0) {
            return MMDEC_ERROR;
        }

        *nal_ref_idc = (data & 0x60) >> 5;
        *nal_type = data & 0x1F;
        return MMDEC_OK;
    }

    return MMDEC_ERROR;
}

MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo)
{
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_SPS_T *sps_ptr = NULL;
    int32 aligned_width, aligned_height;

    if (pDecInfo == NULL)
    {
        return (MMDEC_PARAM_ERROR);
    }

    if (vo->g_sps_array_ptr && vo->g_sps_ptr && (vo->g_sps_ptr->seq_parameter_set_id < MAX_SPS))
    {
        sps_ptr = &(vo->g_sps_array_ptr[vo->g_sps_ptr->seq_parameter_set_id]);
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
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);

    //can support 1080p
    Capability->max_width = 1920;
    Capability->max_height = 1088;
    Capability->profile = AVC_HIGH;
    Capability->level = AVC_LEVEL4_1;

    return MMDEC_OK;
}

MMDecRet H264DecSetParameter(AVCHandle *avcHandle, MMDecVideoFormat * pVideoFormat)
{
    H264DecContext *vo = NULL;
    MMDecRet ret = MMDEC_OK;

    SCI_ASSERT(NULL != pVideoFormat);
    vo = (H264DecContext *) avcHandle->videoDecoderData;
    vo->yuv_format = pVideoFormat->yuv_format;

    return ret;
}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    H264DecContext *vo = NULL;
    MMDecRet ret = MMDEC_ERROR;
    char value_dump[PROPERTY_VALUE_MAX];

    SPRD_CODEC_LOGI ("libomx_avcdec_sw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    SCI_ASSERT(NULL != buffer_ptr);
    SCI_ASSERT(NULL != pVideoFormat);

    vo = (H264DecContext *) (buffer_ptr->common_buffer_ptr);

    memset(vo, 0, sizeof(H264DecContext));
    avcHandle->videoDecoderData = (void *) vo;
    vo->avcHandle = avcHandle;

    buffer_ptr->common_buffer_ptr += sizeof(H264DecContext);
    buffer_ptr->size -= sizeof(H264DecContext);

    H264Dec_InitInterMem (vo, buffer_ptr);

    property_get("h264dec.sw.trace", value_dump, "false");
    vo->trace_enabled = !strcmp(value_dump, "true");

    vo->g_is_avc1_es = FALSE;
    vo->g_ready_to_decode_slice = FALSE;
    vo->yuv_format = pVideoFormat->yuv_format;

    if (H264Dec_init_global_para (vo) != MMDEC_OK)
    {
        vo->return_pos2 |= (1<<29);
        return MMDEC_ERROR;
    }
    H264Dec_init_vld_table ();

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (vo->error_flag)
    {
        vo->return_pos |= (1<<19);
        return MMDEC_ERROR;
    }
#endif

    return MMDEC_OK;
}

PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret = MMDEC_ERROR;
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_SLICE_T *curr_slice_ptr = vo->curr_slice_ptr;
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
    vo->error_flag = FALSE;
    vo->return_pos = 0;
    vo->return_pos1 = 0;
    vo->return_pos2 = 0;

    if((dec_input_ptr->expected_IVOP) && (vo->curr_mb_nr == 0))
    {
        vo->g_searching_IDR_pic = TRUE;
    }

    dec_output_ptr->frameEffective = 0;
    dec_output_ptr->reqNewBuf = 0;

    while (!last_slice)
    {
        last_slice = get_unit (vo, pInStream, stream_lenght, &slice_unit_len);
        stream_lenght -= slice_unit_len;

        ret = H264Dec_Read_SPS_PPS_SliceHeader (vo, vo->g_nalu_ptr->buf, vo->g_nalu_ptr->len, dec_output_ptr);
        if( (vo->g_nalu_ptr->nal_unit_type == NALU_TYPE_SPS) || (vo->g_nalu_ptr->nal_unit_type == NALU_TYPE_PPS))
        {
            SPRD_CODEC_LOGW ("%s, %d, slice_unit_len: %d\n", __FUNCTION__, __LINE__, slice_unit_len);
            vo->g_dec_picture_ptr = NULL; //Added for bug352453
            goto DEC_EXIT;
        }

#if _H264_PROTECT_ & _LEVEL_LOW_
        if (vo->error_flag)
        {
            vo->return_pos |= (1<<20);

            SPRD_CODEC_LOGE ("%s, %d, vo->error_flag: %0x, pos: %0x, pos1: %0x, pos2: %0x\n",
                             __FUNCTION__, __LINE__, vo->error_flag, vo->return_pos, vo->return_pos1, vo->return_pos2);

            goto DEC_EXIT;
        }
#endif

        if (vo->g_ready_to_decode_slice)
        {
#if _H264_PROTECT_ & _LEVEL_LOW_
            if (vo->g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
            {
                vo->error_flag |= ER_PICTURE_NULL_ID;
                vo->return_pos |= (1<<21);
                goto DEC_EXIT;
            }
#endif
            vo->g_dec_picture_ptr->nTimeStamp = dec_input_ptr->nTimeStamp;

            if ((dec_input_ptr->expected_IVOP) && (vo->type != I_SLICE))
            {
                SPRD_CODEC_LOGW ("%s, %d, need I slice, return\n", __FUNCTION__, __LINE__);
                return MMDEC_FRAME_SEEK_IVOP;
            }

            ret = H264Dec_decode_one_slice_data (dec_output_ptr, vo);
        }

        pInStream += slice_unit_len;

        //Added for Bug#162875
        if (SOP == curr_slice_ptr->next_header)
        {
            last_slice = 1;
            vo->type = -1;
        }
    }

DEC_EXIT:

    dec_output_ptr->sawSPS = vo->sawSPS;
    dec_output_ptr->sawPPS = vo->sawPPS;

    //need IVOP but not found IDR,then return seek ivop
    if(dec_input_ptr->expected_IVOP && vo->g_searching_IDR_pic)
    {
        SPRD_CODEC_LOGW ("H264DecDecode: need IVOP\n");
        dec_input_ptr->dataLen -= stream_lenght;
        vo->g_dec_picture_ptr = NULL;
        return MMDEC_FRAME_SEEK_IVOP;
    }

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (vo->error_flag)
    {
        SPRD_CODEC_LOGE ("%s, %d, vo->error_flag: %0x, pos: %0x, pos1: %0x, pos2: %0x\n",
                         __FUNCTION__, __LINE__, vo->error_flag, vo->return_pos, vo->return_pos1, vo->return_pos2);

        vo->g_old_slice_ptr->frame_num = -1;
        vo->curr_mb_nr = 0;
        vo->return_pos |= (1<<22);
        vo->g_dec_picture_ptr =NULL; //Added for bug352453
        H264Dec_clear_delayed_buffer(vo);

        if (vo->not_supported)
        {
            return MMDEC_NOT_SUPPORTED;
        }

        if (vo->error_flag & ER_EXTRA_MEMO_ID)
        {
            SPRD_CODEC_LOGE ("%s, %d\n", __FUNCTION__, __LINE__);
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
    H264DecContext *vo = (H264DecContext *)(avcHandle->videoDecoderData);

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
