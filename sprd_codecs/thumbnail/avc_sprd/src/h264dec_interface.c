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

PUBLIC void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY, int32 picId)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);

    img_ptr->g_rec_buf.imgY =  pFrameY;
    img_ptr->g_rec_buf.pBufferHeader = NULL;//pBufferHeader;
    img_ptr->g_rec_buf.mPicId = picId;
}

void H264Dec_ReleaseRefBuffers(AVCHandle *avcHandle)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;
    int32 i;

    if(dpb_ptr)
    {
        if (dpb_ptr->delayed_pic_ptr)
        {
            if (dpb_ptr->delayed_pic_ptr->pBufferHeader)
            {
                dpb_ptr->delayed_pic_ptr->pBufferHeader = NULL;
            }
            dpb_ptr->delayed_pic_ptr = NULL;
        }

        for (i = 0; dpb_ptr->delayed_pic[i]; i++)
        {
            int32 j;

            for (j = 0; j < /*dpb_ptr->used_size*/16; j++)
            {
                if (dpb_ptr->delayed_pic[j] == dpb_ptr->fs[j]->frame)
                {
                    if(dpb_ptr->fs[j]->is_reference == DELAYED_PIC_REF)
                    {
                        dpb_ptr->fs[j]->is_reference = 0;
                    }
                }
            }

            dpb_ptr->delayed_pic[i] = NULL;
        }
        H264Dec_flush_dpb(img_ptr, dpb_ptr);
    }

    for (i = 0; i < 16; i++)
    {
        if (dpb_ptr->fs[i] && dpb_ptr->fs[i]->frame && dpb_ptr->fs[i]->frame->pBufferHeader)
        {
            dpb_ptr->fs[i]->frame->pBufferHeader = NULL;
        }
    }

    img_ptr->g_searching_IDR_pic = TRUE;
    if(img_ptr->g_old_slice_ptr)
        img_ptr->g_old_slice_ptr->frame_num = -1;
}

MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, uint8 **pOutput, int32 *picId)
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
    if (dpb_ptr->delayed_pic_num)
    {
        *pOutput = dpb_ptr->delayed_pic[0]->imgY;
        *picId = dpb_ptr->delayed_pic[0]->mPicId;

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

//    SCI_TRACE_LOW("%s, %d, data: %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x", __FUNCTION__, __LINE__,
//		bitstream[0],bitstream[1],bitstream[2],bitstream[3],bitstream[4],bitstream[5],bitstream[6],bitstream[7],bitstream[8]);

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

PUBLIC void H264GetBufferDimensions(AVCHandle *avcHandle, int32 *aligned_width, int32 *aligned_height)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_SPS_T *sps_ptr = &(img_ptr->g_sps_array_ptr[0]);
    int32 crop_left, crop_right, crop_top, crop_bottom;

//	SCI_TRACE_LOW("%s, %d, sps_ptr: %0x", __FUNCTION__, __LINE__, sps_ptr);

    int32 width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
    int32 height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;

    if (sps_ptr->frame_cropping_flag)
    {
        crop_left = 2 * sps_ptr->frame_crop_left_offset;
        crop_right =  width - (2 * sps_ptr->frame_crop_right_offset + 1);

        if (sps_ptr->frame_mbs_only_flag)
        {
            crop_top = 2 * sps_ptr->frame_crop_top_offset;
            crop_bottom =   height -  (2 * sps_ptr->frame_crop_bottom_offset + 1);
        }
        else
        {
            crop_top = 4 * sps_ptr->frame_crop_top_offset;
            crop_bottom =  height - (4 * sps_ptr->frame_crop_bottom_offset + 1);
        }
    } else {
        crop_bottom = height - 1;
        crop_right = width - 1;
        crop_top = crop_left = 0;
    }

    *aligned_width = (crop_right - crop_left + 1 + 15) & ~15;
    *aligned_height = (crop_bottom - crop_top + 1 + 15) & ~15;
}

void H264DecCroppingParams(H264DecContext *img_ptr, uint32 *croppingFlag,
                           uint32 *leftOffset, uint32 *width, uint32 *topOffset, uint32 *height)
{
    DEC_SPS_T *sps_ptr = &(img_ptr->g_sps_array_ptr[0]);
    int32 aligned_width =  (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;
    int32 aligned_height = (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16;


//    if (pStorage->activeSps && pStorage->activeSps->frameCroppingFlag)
    if (sps_ptr->frame_cropping_flag)
    {
        *croppingFlag = 1;
        *leftOffset = 2 * sps_ptr->frame_crop_left_offset;
        *width = aligned_width -
                 2 * (sps_ptr->frame_crop_left_offset +
                      sps_ptr->frame_crop_right_offset);
        *topOffset = 2 * sps_ptr->frame_crop_top_offset;
        *height = aligned_height -
                  2 * (sps_ptr->frame_crop_top_offset +
                       sps_ptr->frame_crop_bottom_offset);
    }
    else
    {
        *croppingFlag = 0;
        *leftOffset = 0;
        *width = 0;
        *topOffset = 0;
        *height = 0;
    }

}

MMDecRet H264DecGetInfo(AVCHandle *avcHandle, H264SwDecInfo *pDecInfo)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    DEC_SPS_T *sps_ptr = &(img_ptr->g_sps_array_ptr[0]);

//    storage_t *pStorage;

//    SCI_TRACE_LOW("H264DecGetInfo#");

    if (/*decInst == NULL ||*/ pDecInfo == NULL)
    {
        SCI_TRACE_LOW("H264SwDecGetInfo# ERROR: decInst or pDecInfo is NULL");
        return(MMDEC_PARAM_ERROR);
    }


//    pStorage = &(((decContainer_t *)decInst)->storage);

//    LOGI("%s, %d, g_active_sps_ptr: %0x, g_active_pps_ptr: %0x", __FUNCTION__, __LINE__, g_active_sps_ptr, g_active_pps_ptr);

//    if (pStorage->activeSps == NULL || pStorage->activePps == NULL)
    if (sps_ptr == NULL /*|| g_active_pps_ptr == NULL*/)
    {
        SCI_TRACE_LOW("H264SwDecGetInfo# ERROR: Headers not decoded yet");
        return(MMDEC_ERROR);
    }

#ifdef H264DEC_TRACE
    sprintf(((decContainer_t*)decInst)->str,
            "H264SwDecGetInfo# decInst %p  pDecInfo %p", decInst, (void*)pDecInfo);
    SCI_TRACE_LOW(((decContainer_t*)decInst)->str);
#endif

    /* h264bsdPicWidth and -Height return dimensions in macroblock units,
     * picWidth and -Height in pixels */
    pDecInfo->picWidth        = /*h264bsdPicWidth(pStorage)*/(sps_ptr->pic_width_in_mbs_minus1+1) << 4;
    pDecInfo->picHeight       = /*h264bsdPicHeight(pStorage)*/ (sps_ptr->pic_height_in_map_units_minus1+1)<< 4;
//    pDecInfo->videoRange      = h264bsdVideoRange(pStorage);
//    pDecInfo->matrixCoefficients = h264bsdMatrixCoefficients(pStorage);

#if 1
    H264DecCroppingParams(img_ptr,
                          &pDecInfo->croppingFlag,
                          &pDecInfo->cropParams.cropLeftOffset,
                          &pDecInfo->cropParams.cropOutWidth,
                          &pDecInfo->cropParams.cropTopOffset,
                          &pDecInfo->cropParams.cropOutHeight);

    /* sample aspect ratio */
//    h264bsdSampleAspectRatio(pStorage,
//                             &pDecInfo->parWidth,
//                             &pDecInfo->parHeight);

    //added for bug#154484 and bug#154498
    if ((pDecInfo->cropParams.cropLeftOffset + pDecInfo->cropParams.cropOutWidth
            > pDecInfo->picWidth) ||
            (pDecInfo->cropParams.cropTopOffset + pDecInfo->cropParams.cropOutHeight
             > pDecInfo->picHeight))
    {
        return(MMDEC_ERROR);
    }

#endif
    /* profile */
    pDecInfo->profile = sps_ptr->profile_idc;//h264bsdProfile(pStorage);

//    SCI_TRACE_LOW("H264DecGetInfo# OK");

    return(MMDEC_OK);

}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);
    MMDecRet ret = MMDEC_ERROR;
    MMCodecBuffer tmp_buffer;

    SCI_ASSERT(NULL != buffer_ptr);
    SCI_ASSERT(NULL != pVideoFormat);

    img_ptr = (H264DecContext *) buffer_ptr->int_buffer_ptr;
    memset(img_ptr, 0, sizeof(H264DecContext));
    avcHandle->videoDecoderData = (void *) img_ptr;
    img_ptr->avcHandle = avcHandle;

    tmp_buffer.int_buffer_ptr = buffer_ptr->int_buffer_ptr + sizeof(H264DecContext);
    tmp_buffer.int_size = buffer_ptr->int_size - sizeof(H264DecContext);

    H264Dec_InitInterMem (img_ptr, &tmp_buffer);

    img_ptr->g_is_avc1_es = FALSE;
    img_ptr->g_ready_to_decode_slice = FALSE;

    H264Dec_init_global_para (img_ptr);
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
        last_slice = get_unit (img_ptr, pInStream, dec_input_ptr->dataLen, &slice_unit_len);
        dec_input_ptr->dataLen -= slice_unit_len;

        ret = H264Dec_Read_SPS_PPS_SliceHeader (img_ptr, img_ptr->g_nalu_ptr->buf, img_ptr->g_nalu_ptr->len, dec_output_ptr);

#if _H264_PROTECT_ & _LEVEL_LOW_
        if (img_ptr->error_flag)
        {
            img_ptr->return_pos |= (1<<20);

            if (img_ptr->not_supported)
            {
                return MMDEC_NOT_SUPPORTED;
            }

            return MMDEC_ERROR;
        }
#endif

        if (img_ptr->g_ready_to_decode_slice)
        {
#if _H264_PROTECT_ & _LEVEL_LOW_
            if (img_ptr->g_dec_picture_ptr == NULL)	//added by xw, 20100526, for mb2frm bug.
            {
                img_ptr->error_flag |= ER_PICTURE_NULL_ID;
                img_ptr->return_pos |= (1<<21);

                return MMDEC_ERROR;
            }
#endif

            ret = H264Dec_decode_one_slice_data (dec_output_ptr, img_ptr);
        }

        pInStream += slice_unit_len;

        if (img_ptr->g_need_back_last_word)
        {
            uint32 byte_rest;
            int32 *word_align_pIn;
            byte_rest = (uint32)pInStream;
            byte_rest = ((byte_rest)>>2)<<2;
            word_align_pIn = (int32 *)(byte_rest);

            *word_align_pIn = img_ptr->g_back_last_word;
        }
    }

    //need IVOP but not found IDR,then return seek ivop
    if(dec_input_ptr->expected_IVOP && img_ptr->g_searching_IDR_pic)
    {
        SCI_TRACE_LOW("H264DecDecode: need IVOP\n");
        return MMDEC_FRAME_SEEK_IVOP;
    }

#if _H264_PROTECT_ & _LEVEL_LOW_
    if (img_ptr->error_flag)
    {
        img_ptr->g_old_slice_ptr->frame_num = -1;
        img_ptr->curr_mb_nr = 0;
        img_ptr->return_pos |= (1<<22);
        return MMDEC_ERROR;
    } else
#endif
    {
        return MMDEC_OK;
    }
}

PUBLIC MMDecRet H264_DecReleaseDispBfr(AVCHandle *avcHandle, uint8 *pBfrAddr)
{
    return MMDEC_OK;
}

MMDecRet H264DecRelease(AVCHandle *avcHandle)
{
    H264DecContext *img_ptr = (H264DecContext *)(avcHandle->videoDecoderData);

    H264Dec_FreeMem(img_ptr);

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
