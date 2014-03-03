/******************************************************************************
 ** File Name:    h264enc_interface.c										  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/17/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/17/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/*****************************************************************************/
//  Description:   Init h264 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncInit(AVCHandle *avcHandle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,
                     MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat)
{
    H264EncObject*vo;
    ENC_IMAGE_PARAMS_T *img_ptr;
    uint32 frame_buf_size;
    MMEncRet ret;

    SCI_TRACE_LOW("libomx_avcenc_hw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    CHECK_MALLOC(pInterMemBfr, "pInterMemBfr");
    CHECK_MALLOC(pExtaMemBfr, "pExtaMemBfr");
    CHECK_MALLOC(pBitstreamBfr, "pBitstreamBfr");

    CHECK_MALLOC(pInterMemBfr->common_buffer_ptr, "internal memory");
    CHECK_MALLOC(pExtaMemBfr->common_buffer_ptr, "external memory");
    CHECK_MALLOC(pBitstreamBfr->common_buffer_ptr, "bitstream memory");

    vo = (H264EncObject *) (pInterMemBfr->common_buffer_ptr);
    memset(vo, 0, sizeof(H264EncObject));
    avcHandle->videoEncoderData = (void *) vo;
    vo->avcHandle = avcHandle;

    pInterMemBfr->common_buffer_ptr += sizeof(H264EncObject);
    pInterMemBfr->common_buffer_ptr_phy = (void *)((uint32)(pInterMemBfr->common_buffer_ptr_phy) + sizeof(H264EncObject));
    pInterMemBfr->size -= sizeof(H264EncObject);

    ret = H264Enc_InitMem(vo, pInterMemBfr, pExtaMemBfr);
    if (ret != MMENC_OK)
    {
        return ret;
    }

    vo->g_nFrame_enc = 0;
    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->vsp_freq_div = 0;
    vo->error_flag = 0;
    vo->b_previous_frame_failed = 0;
    vo->vsp_capability = -1;
    if (VSP_OPEN_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_ERROR;
    }

    img_ptr = vo->g_enc_image_ptr = (ENC_IMAGE_PARAMS_T *)H264Enc_MemAlloc (vo, sizeof(ENC_IMAGE_PARAMS_T), 8, INTER_MEM);
    CHECK_MALLOC(vo->g_enc_image_ptr, "vo->g_enc_image_ptr");

    vo->g_h264_enc_config = (MMEncConfig *)H264Enc_MemAlloc (vo, sizeof(MMEncConfig), 8, INTER_MEM);
    CHECK_MALLOC(vo->g_h264_enc_config, "vo->g_h264_enc_config");

    vo->g_vlc_hw_ptr = (uint8 *)H264Enc_MemAlloc(vo, (406*2*sizeof(uint32)), 8, EXTRA_MEM);
    CHECK_MALLOC(vo->g_vlc_hw_ptr, "vo->g_vlc_hw_ptr");
    memcpy(vo->g_vlc_hw_ptr, g_vlc_hw_tbl, (406*2*sizeof(uint32)));

    img_ptr->orig_width = pVideoFormat->frame_width;
    img_ptr->orig_height = pVideoFormat->frame_height;

    img_ptr->width = (img_ptr->orig_width + 15)&(~15);
    img_ptr->height = (img_ptr->orig_height + 15)&(~15);

// 	img_ptr->i_frame = 0;
    img_ptr->frame_num = 0;
    img_ptr->i_idr_pic_id = 0;
    img_ptr->i_sps_id = 0;

    if (vo->vsp_capability == 0)   //limited under 720p
    {
        img_ptr->i_level_idc = 31;
    } else if (vo->vsp_capability == 1)   //limited under 1080p
    {
        img_ptr->i_level_idc = 41;
    } else
    {
        img_ptr->i_level_idc = 51;	//as close to "unresticted as we can get
    }

//	img_ptr->i_max_ref0 = img_ptr->i_frame_reference_num;
//	img_ptr->i_max_dpb = 2;	//h->sps->vui.i_max_dec_frame_buffering + 1;
// 	h->fdec->i_poc = 0; //tmp add, 20100512
    img_ptr->i_keyint_max	= 15;
//	img_ptr->i_last_idr	= -img_ptr->i_keyint_max;
    img_ptr->i_chroma_qp_offset = 0;

    img_ptr->i_ref0 = 0;
    img_ptr->sh.i_first_mb = 0;
    img_ptr->pic_sz = 0;
    img_ptr->stm_offset = 0;
    img_ptr->slice_nr = 0;
    img_ptr->prev_slice_bits = 0;
    img_ptr->crop_x = 0;
    img_ptr->crop_y = 0;
    img_ptr->frame_width_in_mbs = (img_ptr->width + 15) >> 4;
    img_ptr->frame_height_in_mbs = (img_ptr->height + 15) >>4;
    img_ptr->frame_size_in_mbs = img_ptr->frame_width_in_mbs * img_ptr->frame_height_in_mbs;
    img_ptr->slice_mb = (img_ptr->frame_height_in_mbs / SLICE_MB)*img_ptr->frame_width_in_mbs;

    //init frames
    img_ptr->pYUVSrcFrame = (H264EncStorablePic *)H264Enc_MemAlloc(vo, sizeof(H264EncStorablePic), 8, INTER_MEM);
    CHECK_MALLOC(img_ptr->pYUVSrcFrame, "img_ptr->pYUVSrcFrame");

    img_ptr->pYUVRecFrame = (H264EncStorablePic *)H264Enc_MemAlloc(vo, sizeof(H264EncStorablePic), 8, INTER_MEM);
    CHECK_MALLOC(img_ptr->pYUVRecFrame, "img_ptr->pYUVRecFrame");

    img_ptr->pYUVRefFrame = (H264EncStorablePic *)H264Enc_MemAlloc(vo, sizeof(H264EncStorablePic), 8, INTER_MEM);
    CHECK_MALLOC(img_ptr->pYUVRefFrame, "img_ptr->pYUVRefFrame");

    img_ptr->pYUVSrcFrame->i_poc = 0;
    img_ptr->pYUVRecFrame->i_poc = 0;
    img_ptr->pYUVRefFrame->i_poc = 0;
    img_ptr->pYUVSrcFrame->addr_idx = 0;
    img_ptr->pYUVRecFrame->addr_idx = 1;
    img_ptr->pYUVRefFrame->addr_idx = 2;

    // init RC parameters
    vo->rc_gop_paras.rem_bits = 0;
    vo->rc_gop_paras.curr_buf_full = 0;
    vo->rc_gop_paras.intra_period = INTRA_PERIOD;
    vo->rc_gop_paras.I_P_ratio = I_P_RATIO;

    /* rate control */
    img_ptr->pOneFrameBitstream = pBitstreamBfr->common_buffer_ptr;
    img_ptr->OneFrameBitstream_addr_phy = (uint32 )pBitstreamBfr->common_buffer_ptr_phy;
    img_ptr->OneframeStreamLen = pBitstreamBfr->size;

    frame_buf_size = img_ptr->width * img_ptr->height;
    img_ptr->pYUVRecFrame->imgY = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size, 8, EXTRA_MEM);
    CHECK_MALLOC(img_ptr->pYUVRecFrame->imgY, "img_ptr->pYUVRecFrame->imgY");

    img_ptr->pYUVRecFrame->imgUV = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size/2, 8, EXTRA_MEM);
    CHECK_MALLOC(img_ptr->pYUVRecFrame->imgUV, "img_ptr->pYUVRecFrame->imgUV");

    img_ptr->pYUVRecFrame->imgYAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRecFrame->imgY, EXTRA_MEM) >> 3;	// DWORD
    img_ptr->pYUVRecFrame->imgUVAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRecFrame->imgUV, EXTRA_MEM) >> 3;	// DWORD

    img_ptr->pYUVRefFrame->imgY = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size, 8, EXTRA_MEM);
    CHECK_MALLOC(img_ptr->pYUVRefFrame->imgY, "img_ptr->pYUVRefFrame->imgY");

    img_ptr->pYUVRefFrame->imgUV = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size/2, 8, EXTRA_MEM);
    CHECK_MALLOC(img_ptr->pYUVRefFrame->imgUV, "img_ptr->pYUVRefFrame->imgUV");

    img_ptr->pYUVRefFrame->imgYAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRefFrame->imgY, EXTRA_MEM) >> 3;	// DWORD
    img_ptr->pYUVRefFrame->imgUVAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRefFrame->imgUV, EXTRA_MEM) >> 3;	// DWORD

    vo->g_anti_shake.enable_anti_shake = pVideoFormat->b_anti_shake;
    vo->g_anti_shake.shift_x = 0;
    vo->g_anti_shake.shift_y = 0;
    vo->g_anti_shake.input_width = 0;
    vo->g_anti_shake.input_height= 0;

    h264enc_sps_init (img_ptr);
    h264enc_pps_init (vo, img_ptr);

    return MMENC_OK;
}

MMEncRet H264EncSetConf(AVCHandle *avcHandle, MMEncConfig *pConf)
{
    H264EncObject *vo = (H264EncObject *) avcHandle->videoEncoderData;
    MMEncConfig * enc_config = vo->g_h264_enc_config;

    SCI_ASSERT(NULL != pConf);

    enc_config->FrameRate		= pConf->FrameRate;
    enc_config->targetBitRate		= pConf->targetBitRate;
    enc_config->RateCtrlEnable	= pConf->RateCtrlEnable;

    enc_config->QP_IVOP		= pConf->QP_IVOP;
    enc_config->QP_PVOP		= pConf->QP_PVOP;

    if ((1920 == vo->g_enc_image_ptr->width) && (1088 == vo->g_enc_image_ptr->height))
    {
        vo->rc_gop_paras.intra_period = INTRA_PERIOD;
    } else
    {
        vo->rc_gop_paras.intra_period = pConf->FrameRate;
    }

    SCI_TRACE_LOW("%s, %d, intra_period: %d", __FUNCTION__, __LINE__, vo->rc_gop_paras.intra_period);

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Get H264 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncGetConf(AVCHandle *avcHandle, MMEncConfig *pConf)
{
    H264EncObject *vo = (H264EncObject *) avcHandle->videoEncoderData;
    MMEncConfig * enc_config = vo->g_h264_enc_config;

    SCI_ASSERT(NULL != pConf);

    pConf->QP_IVOP					 =enc_config->QP_IVOP;
    pConf->QP_PVOP					 = enc_config->QP_PVOP ;

    pConf->targetBitRate 				= enc_config->targetBitRate;
    pConf->FrameRate 				= enc_config->FrameRate;
    pConf->RateCtrlEnable 			= enc_config->RateCtrlEnable;

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Close mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncRelease(AVCHandle *avcHandle)
{
    H264EncObject *vo = (H264EncObject *) avcHandle->videoEncoderData;

    if ( VSP_CLOSE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Produce fake nalu.
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264Enc_FakeNALU(ENC_IMAGE_PARAMS_T *img_ptr, MMEncOut *pOutput)
{
    uint32 nal_header;

    SCI_TRACE_LOW("%s, %d", __FUNCTION__, __LINE__);

    /* nal header, ( 0x00 << 7 ) | ( nal->i_ref_idc << 5 ) | nal->i_type; */
    nal_header = ( 0x00 << 7 ) | ( NAL_PRIORITY_HIGHEST << 5 ) | NAL_UNKNOWN;

    img_ptr->pOneFrameBitstream[0] = 0x0;
    img_ptr->pOneFrameBitstream[1] = 0x0;
    img_ptr->pOneFrameBitstream[2] = 0x0;
    img_ptr->pOneFrameBitstream[3] = 0x1;
    img_ptr->pOneFrameBitstream[4] = nal_header;
    img_ptr->pOneFrameBitstream[5] = 0x80;
    img_ptr->pOneFrameBitstream[6] = 0x0;
    img_ptr->pOneFrameBitstream[7] = 0x0;

    pOutput->strmSize = 8;
    pOutput->pOutBuf = img_ptr->pOneFrameBitstream;

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Encode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncStrmEncode(AVCHandle *avcHandle, MMEncIn *pInput, MMEncOut *pOutput)
{
    int32 i_slice_type;
    int32 i_nal_type;
    int32 i_nal_ref_idc;
    int32 i_global_qp;
    H264EncObject *vo = (H264EncObject *) avcHandle->videoEncoderData;
    ENC_IMAGE_PARAMS_T *img_ptr = vo->g_enc_image_ptr;
    MMEncConfig * enc_config = vo->g_h264_enc_config;
    ENC_ANTI_SHAKE_T *anti_shark_ptr = &(vo->g_anti_shake);
    uint32 rate_control_en = enc_config->RateCtrlEnable;
    MMEncRet ret = MMENC_OK;

    vo->error_flag = 0;

    anti_shark_ptr->input_width = pInput->org_img_width;
    anti_shark_ptr->input_height = pInput->org_img_height;
    anti_shark_ptr->shift_x = pInput->crop_x;
    anti_shark_ptr->shift_y = pInput->crop_y;

    img_ptr->stm_offset = 0;
    img_ptr->pYUVSrcFrame->i_frame = vo->g_nFrame_enc;

    if ((1920 == vo->g_enc_image_ptr->width) && (1088 == vo->g_enc_image_ptr->height))
    {
        if (0 == (vo->g_nFrame_enc % vo->rc_gop_paras.intra_period))
        {
            img_ptr->pYUVSrcFrame->i_type = i_slice_type = SLICE_TYPE_I;
        } else
        {
            img_ptr->pYUVSrcFrame->i_type = i_slice_type = SLICE_TYPE_P;
        }
    } else
    {
        img_ptr->pYUVSrcFrame->i_type = i_slice_type = ((pInput->vopType == 0) ? SLICE_TYPE_I : SLICE_TYPE_P);
    }

    if(vo->b_previous_frame_failed)
    {
        img_ptr->pYUVSrcFrame->i_type = i_slice_type = SLICE_TYPE_I;
    }

    img_ptr->pYUVSrcFrame->imgY =  pInput->p_src_y_phy;
    img_ptr->pYUVSrcFrame->imgUV =  pInput->p_src_u_phy;
    img_ptr->pYUVSrcFrame->imgYAddr = (uint32)img_ptr->pYUVSrcFrame->imgY >> 3;
    img_ptr->pYUVSrcFrame->imgUVAddr = (uint32)img_ptr->pYUVSrcFrame->imgUV >> 3;

    if( i_slice_type == SLICE_TYPE_I )
    {
        //rate_control
        if(img_ptr->sh.i_first_mb==0)
        {
            i_global_qp = vo->rc_pic_paras.curr_pic_qp = rc_init_GOP(vo, &(vo->rc_gop_paras)); // 20
            rc_init_pict(vo,&(vo->rc_gop_paras), &(vo->rc_pic_paras));
        } else
        {
#ifdef NO_BU_CHANGE
            i_global_qp = vo->rc_pic_paras.curr_pic_qp = rc_init_slice(vo, img_ptr, &(vo->rc_pic_paras));
#else
            i_global_qp = vo->rc_pic_paras.curr_pic_qp;
#endif
        }

        if( img_ptr->frame_num == 0 )
        {
            i_nal_type = NAL_SLICE_IDR;
        } else
        {
            i_nal_type = NAL_SLICE;
        }
        i_nal_ref_idc = NAL_PRIORITY_HIGHEST;
    } else if (i_slice_type == SLICE_TYPE_P)
    {
        //rate_control
        if(img_ptr->sh.i_first_mb==0)
        {
            enc_config->QP_PVOP = rc_init_pict(vo,&(vo->rc_gop_paras), &(vo->rc_pic_paras));
            vo->rc_pic_paras.curr_pic_qp = enc_config->QP_PVOP;
            i_global_qp = enc_config->QP_PVOP;
            h264enc_reference_update(img_ptr);
        } else
        {
#ifdef NO_BU_CHANGE
            enc_config->QP_PVOP = vo->rc_pic_paras.curr_pic_qp = rc_init_slice(vo, img_ptr, &(vo->rc_pic_paras));
            i_global_qp = enc_config->QP_PVOP;
#else
            enc_config->QP_PVOP = vo->rc_pic_paras.curr_pic_qp;
            i_global_qp = enc_config->QP_PVOP;
#endif
        }

        i_nal_type = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH;
    }
    vo->prev_qp = i_global_qp;	// MUST HAVE prev_qp updated!!

    img_ptr->pYUVRecFrame->i_poc =
        img_ptr->pYUVSrcFrame->i_poc = 2 * img_ptr->frame_num;
    img_ptr->pYUVRecFrame->i_type = img_ptr->pYUVSrcFrame->i_type;
    img_ptr->pYUVRecFrame->i_frame = img_ptr->pYUVSrcFrame->i_frame;
    img_ptr->pYUVSrcFrame->b_kept_as_ref =
        img_ptr->pYUVRecFrame->b_kept_as_ref = (i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE);

    //init
    h264enc_reference_build_list (img_ptr, img_ptr->pYUVRecFrame->i_poc, i_slice_type);

    /* ------------------------ Create slice header  ----------------------- */
    h264enc_slice_init(vo,  img_ptr, i_nal_type, i_slice_type, i_global_qp );

    ret = H264Enc_InitVSP(vo);
    if (ret != MMENC_OK)
    {
        H264Enc_FakeNALU(img_ptr, pOutput);
        ret = MMENC_OK;
        vo->error_flag = 0;
        vo->b_previous_frame_failed =1;

        goto ENC_EXIT;
    }

    /* ---------------------- Write the bitstream -------------------------- */
    /* Init bitstream context */
    img_ptr->i_nal_type = i_nal_type;
    img_ptr->i_nal_ref_idc = i_nal_ref_idc;

    //if(img_ptr->sh.i_first_mb==0)	// Flush each frame, not each slice
    H264Enc_InitBSM(vo);

    //write SPS and PPS
    if ((i_nal_type == NAL_SLICE_IDR) && (img_ptr->sh.i_first_mb==0))
    {
        img_ptr->pYUVRecFrame->i_poc = 0;
    }

    //write frame
    img_ptr->slice_sz[img_ptr->slice_nr] = h264enc_slice_write(vo, img_ptr);
    if (vo->error_flag)
    {
        H264Enc_FakeNALU(img_ptr, pOutput);
        ret = MMENC_OK;
        vo->error_flag = 0;
        vo->b_previous_frame_failed =1;

        goto ENC_EXIT;
    } else
    {
        vo->b_previous_frame_failed =0;
    }
    *((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset])) = 0x01000000;
    img_ptr->slice_sz[img_ptr->slice_nr] += (VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + DSTUF_NUM_OFF, "DSTUF_NUM") << 3);

    // calculate slice size and stream offset
    {
        uint32 tmp = (64 - (img_ptr->slice_sz[img_ptr->slice_nr] & 0x3f)) & 0x3f;
        img_ptr->slice_sz[img_ptr->slice_nr] = (img_ptr->slice_sz[img_ptr->slice_nr]+63)&0xffffffc0;
        img_ptr->pic_sz += img_ptr->slice_sz[img_ptr->slice_nr];
        img_ptr->stm_offset += (img_ptr->slice_sz[img_ptr->slice_nr] >> 3);
        //img_ptr->stm_offset = (img_ptr->stm_offset+7)&0xfffffff8; // DWORD aligned
        img_ptr->slice_sz[img_ptr->slice_nr] -= tmp;
        img_ptr->slice_sz[img_ptr->slice_nr] += img_ptr->prev_slice_bits;
        img_ptr->prev_slice_bits = tmp;
        img_ptr->slice_nr++;
    }

    //rate_control
    if(img_ptr->sh.i_first_mb==0)
    {
        rc_update_pict(vo, img_ptr->pic_sz, &(vo->rc_gop_paras));
        img_ptr->pic_sz = 0;
        img_ptr->slice_nr = 0;
        pOutput->strmSize = img_ptr->stm_offset;
        pOutput->pOutBuf = img_ptr->pOneFrameBitstream;
    }

    //increase frame count
    if(img_ptr->sh.i_first_mb == 0)
    {
//        vo->g_nFrame_enc++;
        if(img_ptr->pYUVRecFrame->b_kept_as_ref)
        {
            img_ptr->frame_num++;
        }

        if(img_ptr->i_nal_type == NAL_SLICE_IDR)
        {
            img_ptr->i_idr_pic_id = (img_ptr->i_idr_pic_id + 1) % 65536;
        }
    }

ENC_EXIT:

    SCI_TRACE_LOW("%s, %d, exit encoder, error_flag: %0x", __FUNCTION__, __LINE__, vo->error_flag);

    if(img_ptr->sh.i_first_mb == 0)
    {
        vo->g_nFrame_enc++;
    }

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    if (vo->error_flag & ER_HW_ID)
    {
        ret = MMENC_HW_ERROR;
    }

    return ret;
}

/*****************************************************************************/
//  Description:   generate sps or pps header
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet H264EncGenHeader(AVCHandle *avcHandle, MMEncOut *pOutput, int is_sps)
{
    H264EncObject *vo = (H264EncObject *) avcHandle->videoEncoderData;
    ENC_IMAGE_PARAMS_T *img_ptr = vo->g_enc_image_ptr;

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (STREAM_ID_H264 | V_BIT_4), "VSP_MODE: Set standard and work mode");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

    img_ptr->stm_offset = 0;

    H264Enc_InitBSM(vo);

    if (is_sps)
    {
        h264enc_sps_write(vo, img_ptr->sps);
    } else
    {
        h264enc_pps_write(vo, img_ptr->pps);
    }
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_27, 0x00000000, TIME_OUT_CLK, "Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable")) //check bsm is idle
    {
        goto HEADER_EXIT;
    }
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, V_BIT_1, "BSM_OPERATE: BSM_CLR");
    if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "Polling BSM_DBG0: BSM inactive")) //check bsm is idle
    {
        goto HEADER_EXIT;
    }
    if (VSP_READ_REG_POLL(GLB_REG_BASE_ADDR + AXIM_STS_OFF, V_BIT_1, 0x0, TIME_OUT_CLK, "Polling AXIM_STS: not Axim_wch_busy")) //check all data has written to DDR
    {
        goto HEADER_EXIT;
    }

//    *((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset])) = 0x01000000;
    img_ptr->stm_offset = ( VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF,"TOTAL_BITS") >> 3);
    img_ptr->stm_offset += VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + DSTUF_NUM_OFF,"DSTUF_NUM");
    img_ptr->stm_offset = (img_ptr->stm_offset+7)&(~0x7); // DWORD aligned

    img_ptr->pOneFrameBitstream[0] = 0x0;
    img_ptr->pOneFrameBitstream[1] = 0x0;
    img_ptr->pOneFrameBitstream[2] = 0x0;
    img_ptr->pOneFrameBitstream[3] = 0x1;

    pOutput->strmSize = img_ptr->stm_offset;
    pOutput->pOutBuf = img_ptr->pOneFrameBitstream;

HEADER_EXIT:

    SCI_TRACE_LOW("%s, %d, exit generating header, error_flag: %0x", __FUNCTION__, __LINE__, vo->error_flag);

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    return MMENC_OK;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

