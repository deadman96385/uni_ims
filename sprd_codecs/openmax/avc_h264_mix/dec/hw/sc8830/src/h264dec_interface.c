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

    SPRD_CODEC_LOGI ("%s, %d\n", __FUNCTION__, __LINE__);

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

PUBLIC MMDecRet H264Dec_GetLastDspFrm(AVCHandle *avcHandle, void **pOutput, int32 *picId, uint64 *pts)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr =  vo->g_dpb_layer[0];
    uint32 i;

    //for multi-slice case, we push the current decoding picture into dpb->delayed_pic queue whether it has been finished or not.
    if (vo->g_dec_picture_ptr!= NULL)
    {
        dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = vo->g_dec_picture_ptr;
        vo->g_dec_picture_ptr = NULL;
    }

    if (NULL == dpb_ptr)
    {
        *pOutput = NULL;
        return MMDEC_ERROR;
    } else {
        //pop one picture from delayed picture queue.
        if (dpb_ptr->delayed_pic_num)
        {
            DEC_STORABLE_PICTURE_T *out = dpb_ptr->delayed_pic[0];
            DEC_FRAME_STORE_T *fs = H264Dec_search_frame_from_DBP(vo, out);

            if (!fs)
            {
                SPRD_CODEC_LOGE ("%s, %d, fs is NULL!, delayed_pic_num: %d, delayed_pic_ptr: %p\n", __FUNCTION__, __LINE__, dpb_ptr->delayed_pic_num, dpb_ptr->delayed_pic_ptr);
                *pOutput = NULL;
                dpb_ptr->delayed_pic_ptr = NULL;
                return MMDEC_ERROR;
            }

            for(i = 0; i < dpb_ptr->delayed_pic_num; i++)
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
            H264DEC_UNBIND_FRAME(vo, fs->frame);

            return MMDEC_OK;
        } else
        {
            *pOutput = NULL;
            dpb_ptr->delayed_pic_ptr = NULL;
            if (dpb_ptr->delayed_pic_num != 0)
            {
                SPRD_CODEC_LOGE ("%s, %d, dpb_ptr->delayed_pic_num != 0!\n", __FUNCTION__, __LINE__);
            }
            return MMDEC_ERROR;
        }
    }
}

void H264Dec_SetCurRecPic(AVCHandle *avcHandle, uint8	*pFrameY,uint8 *pFrameY_phy,void *pBufferHeader, int32 picId)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
    DEC_STORABLE_PICTURE_T *rec_buf_ptr = &(vo->g_rec_buf);

    rec_buf_ptr->imgY =  pFrameY;
    rec_buf_ptr->imgYAddr = (uint_32or64)pFrameY_phy;
    rec_buf_ptr->pBufferHeader = pBufferHeader;
    rec_buf_ptr->mPicId = picId;
}

MMDecRet H264DecGetNALType(AVCHandle *avcHandle, uint8 *bitstream, int size, int *nal_type, int *nal_ref_idc)
{
    if (size > 0) {
        uint32 data;
        int32 forbidden_zero_bit;
        uint8 *ptr = bitstream;
        uint32 len = 0;

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
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;
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
//    SCI_TRACE_LOW("%s, %d, aligned_width: %d, aligned_height: %d", __FUNCTION__, __LINE__, aligned_width, aligned_height);

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

    /* profile */
    pDecInfo->profile = sps_ptr->profile_idc;//h264bsdProfile(pStorage);
    pDecInfo->numRefFrames = sps_ptr->num_ref_frames;
    pDecInfo->has_b_frames = sps_ptr->vui_seq_parameters->num_reorder_frames;
    if (vo->g_sps_ptr->profile_idc != 0x42) {
        pDecInfo->has_b_frames = (!pDecInfo->has_b_frames) ? 1 : pDecInfo->has_b_frames;
    }

    return(MMDEC_OK);
}

MMDecRet H264GetCodecCapability(AVCHandle *avcHandle, MMDecCapability *Capability)
{
    H264DecObject *vo = (H264DecObject *)avcHandle->videoDecoderData;

    switch (vo->vsp_version)
    {
    case SHARK:	//limited under 1080p
    case TSHARK:
    case SHARKL:
    case PIKEL:
    case SHARKL64:
    case SHARKLT8:
        Capability->max_width = 1920;
        Capability->max_height = 1088;
        Capability->profile = AVC_HIGH;
        Capability->level = AVC_LEVEL4_1;
        break;
    case DOLPHIN://limited under 720p
    case PIKE:
        Capability->max_width = 1280;
        Capability->max_height = 1023; //720;
        Capability->profile = AVC_HIGH;
        Capability->level = AVC_LEVEL3_1;
        break;
    default:
        Capability->max_width = 352;
        Capability->max_height = 288;
        Capability->profile = AVC_BASELINE;
        Capability->level = AVC_LEVEL2;
        break;
    }

    return MMDEC_OK;
}

MMDecRet H264DecSetParameter(AVCHandle *avcHandle, MMDecVideoFormat * pVideoFormat)
{
    return MMDEC_OK;
}

MMDecRet H264DecInit(AVCHandle *avcHandle, MMCodecBuffer * buffer_ptr,MMDecVideoFormat * pVideoFormat)
{
    H264DecObject *vo;
    MMDecRet ret = MMDEC_OK;
    char value_dump[PROPERTY_VALUE_MAX];

    SPRD_CODEC_LOGI ("libomx_avcdec_hw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.\n", __DATE__, __TIME__);

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

    property_get("h264dec.hw.trace", value_dump, "false");
    vo->trace_enabled = !strcmp(value_dump, "true");

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->vsp_freq_div = SPRD_MAX_VSP_FREQ_LEVEL;
    vo->vsp_version = SHARK;
    vo->yuv_format = pVideoFormat->yuv_format;

    if (VSP_OPEN_Dev((VSPObject *)vo) < 0)
    {
        return ret;
    }

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
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

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }

    return ret;
}

#define MIN_LEN_FOR_HW 8
PUBLIC MMDecRet H264DecDecode(AVCHandle *avcHandle, MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr)
{
    MMDecRet ret = MMDEC_OK;
    int32 i, start_code_len = 0;
    uint32 bs_buffer_length, bs_start_addr, destuffing_num, cmd;
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

    cmd = V_BIT_17|V_BIT_16|V_BIT_11|V_BIT_5|V_BIT_3;
    if (vo->yuv_format == YUV420SP_NV21)  //vu format
    {
        cmd |= V_BIT_6;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, cmd,"axim endian set, vu format"); //VSP and OR endian.
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
    while(vo->g_stream_offset < (int32)bs_buffer_length)
    {
        uint32 cmd;

        // Find the next start code and get length of NALU.
        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
        {
            break;
        }
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
        VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM1_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf1 addr");
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM

        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((V_BIT_31|V_BIT_30)|vo->g_stream_offset),"BSM_cfg1 check startcode");//byte align

        cmd = (((bs_buffer_length - (uint32)(vo->g_stream_offset)) < MIN_LEN_FOR_HW) ? (MIN_LEN_FOR_HW + (uint32)(vo->g_stream_offset)) : bs_buffer_length);
        cmd |= V_BIT_31;
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, cmd, "BSM_cfg0 stream buffer size");//BSM load data
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
            SPRD_CODEC_LOGD ("%s, %d, added_bytes: %d\n", __FUNCTION__, __LINE__, added_bytes);
        }

        SPRD_CODEC_LOGD ("%s, %d, g_stream_offset: %d, g_slice_datalen: %d, g_nalu_ptr->len: %d, destuffing_num: %d\n", __FUNCTION__, __LINE__,
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

        SPRD_CODEC_LOGD ("%s, %d, g_nalu_ptr->len: %d, frame_dec_finish: %d,ret:  %d\n", __FUNCTION__, __LINE__, vo->g_nalu_ptr->len, vo->frame_dec_finish, ret);

        dec_input_ptr->dataLen = vo->g_stream_offset + vo->g_nalu_ptr->len + destuffing_num;

        if( (ret != MMDEC_OK) || vo->frame_dec_finish || (vo->g_nalu_ptr->nal_unit_type == NALU_TYPE_SPS) || (vo->g_nalu_ptr->nal_unit_type == NALU_TYPE_PPS))
        {
            if (ret == MMDEC_ERROR)
            {
                dec_input_ptr->dataLen = bs_buffer_length;

                if (vo->error_flag & ER_MEMORY_ID)
                {
                    vo->memory_error = 1;
                    ret = MMDEC_MEMORY_ERROR;
                } else if ((vo->error_flag & ER_FORMAT_ID) || (vo->error_flag & ER_SPSPPS_ID) )
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
            vo->g_dec_picture_ptr =NULL; //Added for bug352453
            break;	//break loop.
        }

        //for next slice
        vo->g_stream_offset += vo->g_slice_datalen;
    }

DEC_EXIT:

    dec_output_ptr->sawSPS = vo->sawSPS;
    dec_output_ptr->sawPPS = vo->sawPPS;

    SPRD_CODEC_LOGD ("%s, %d, exit decoder, error_flag: %0x\n", __FUNCTION__, __LINE__, vo->error_flag);
    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }

    if (vo->error_flag & ER_HW_ID)
    {
        ret = MMDEC_HW_ERROR;
    }

    if (vo->error_flag)
    {
        H264Dec_clear_delayed_buffer(vo);
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

int32 Is_Interlaced_Sequence(AVCHandle *avcHandle, MMDecInput *dec_input_ptr)
{
    uint32 bs_buffer_length, bs_start_addr, destuffing_num, cmd;
    uint32 nalu_len = 0;
    uint8* nalu_buf = 0;
    uint8* pstream = 0;
    int8 nal_unit_type;
    int32 ret = -1, start_code_len = 0;

    H264DecObject *vo = (H264DecObject *) avcHandle->videoDecoderData;

    SPRD_CODEC_LOGD ("%s, %d, called\n", __FUNCTION__, __LINE__);
    if ((dec_input_ptr->pStream == NULL) ||  (dec_input_ptr->pStream_phy == 0))
    {
        return MMDEC_MEMORY_ERROR;
    }

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    cmd = V_BIT_17|V_BIT_16|V_BIT_11|V_BIT_5|V_BIT_3;
    if (vo->yuv_format == YUV420SP_NV21)  //vu format
    {
        cmd |= V_BIT_6;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, cmd,"axim endian set, vu format"); //VSP and OR endian.
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
    start_code_len = VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+BSM_NAL_LEN,"get NAL_LEN");
    vo->g_stream_offset += start_code_len;

    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        goto DEC_EXIT;
    }

    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");

    // Find the next start code and get length of NALU.
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        goto DEC_EXIT;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM1_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf1 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM

    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, ((V_BIT_31|V_BIT_30)|vo->g_stream_offset),"BSM_cfg1 check startcode");//byte align

    cmd = V_BIT_31 | (((bs_buffer_length - (uint32)(vo->g_stream_offset)) < MIN_LEN_FOR_HW) ? (MIN_LEN_FOR_HW + (uint32)(vo->g_stream_offset)) : bs_buffer_length);
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, cmd, "BSM_cfg0 stream buffer size");//BSM load data
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG1_OFF, V_BIT_2,V_BIT_2,TIME_OUT_CLK_FRAME, "startcode found"))//check bsm is idle
    {
        goto DEC_EXIT;
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
        SPRD_CODEC_LOGD ("%s, %d, added_bytes: %d\n", __FUNCTION__, __LINE__, added_bytes);
    }

    SPRD_CODEC_LOGD ("%s, %d, g_stream_offset: %d, g_slice_datalen: %d, g_nalu_ptr->len: %d, destuffing_num: %d\n", __FUNCTION__, __LINE__,
                     vo->g_stream_offset, vo->g_slice_datalen, vo->g_nalu_ptr->len, destuffing_num);
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        goto DEC_EXIT;
    }
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, 0,"BSM_cfg1 check startcode disable");

    // Configure BSM for decoding.
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+BSM_DBG0_OFF, V_BIT_27,0x0,TIME_OUT_CLK, "BSM_clr enable"))//check bsm is idle
    {
        goto DEC_EXIT;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM0_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf0 addr");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+BSM1_FRM_ADDR_OFF, bs_start_addr/8,"BSM_buf1 addr");
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_OP_OFF, V_BIT_2|V_BIT_1,"BSM_OP clr BSM");//clr BSM
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG1_OFF, (V_BIT_31|vo->g_stream_offset),"BSM_cfg1 stream buffer offset");//point to the start of NALU.
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR+BSM_CFG0_OFF, V_BIT_31|((bs_buffer_length+128)&0xfffffffc),"BSM_cfg0 stream buffer size");// BSM load data. Add 16 DW for BSM fifo loading.

    nal_unit_type = READ_FLC(8) & 0x1f;
    SPRD_CODEC_LOGD ("%s, %d, nal_unit_type = %x\n", __FUNCTION__, __LINE__, nal_unit_type);

    if (nal_unit_type == 7)
    {
        H264Dec_interpret_sps (vo, vo->g_sps_ptr);
        if (vo->error_flag)
        {
            SPRD_CODEC_LOGE ("%s, %d, interpret_sps error\n", __FUNCTION__, __LINE__);
            ret = -1;
            goto DEC_EXIT;
        }
    } else
    {
        ret = -1;
        goto DEC_EXIT;
    }

    SPRD_CODEC_LOGD ("%s, %d, frame_mbs_only_flag = %d\n", __FUNCTION__, __LINE__, vo->g_sps_ptr->frame_mbs_only_flag);
    if(vo->g_sps_ptr->frame_mbs_only_flag)
    {
        ret =  0;
    }
    else
    {
        ret = 1;
    }

DEC_EXIT:

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }
    return  ret;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
