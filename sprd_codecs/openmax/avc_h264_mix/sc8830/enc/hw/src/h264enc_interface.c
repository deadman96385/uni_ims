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
    MMEncRet is_init_success = MMENC_OK;

    SCI_TRACE_LOW("libomx_avcenc_hw_sprd.so is built on %s %s, Copyright (C) Spreatrum, Inc.", __DATE__, __TIME__);

    SCI_ASSERT(NULL != pInterMemBfr);
    SCI_ASSERT(NULL != pExtaMemBfr);
    SCI_ASSERT(NULL != pVideoFormat);

    vo = (H264EncObject *) (pInterMemBfr->common_buffer_ptr);
    memset(vo, 0, sizeof(H264EncObject));
    avcHandle->videoEncoderData = (void *) vo;
    vo->avcHandle = avcHandle;

    pInterMemBfr->common_buffer_ptr += sizeof(H264EncObject);
    pInterMemBfr->common_buffer_ptr_phy = (void *)((uint32)(pInterMemBfr->common_buffer_ptr_phy) + sizeof(H264EncObject));
    pInterMemBfr->size -= sizeof(H264EncObject);

    H264Enc_InitMem(vo, pInterMemBfr, pExtaMemBfr);

    vo->g_nFrame_enc = 0;

    img_ptr = vo->g_enc_image_ptr = (ENC_IMAGE_PARAMS_T *)H264Enc_MemAlloc (vo, sizeof(ENC_IMAGE_PARAMS_T), 8, INTER_MEM);

    vo->g_h264_enc_config = (MMEncConfig *)H264Enc_MemAlloc (vo, sizeof(MMEncConfig), 8, INTER_MEM);
    vo->g_vlc_hw_ptr = (uint8 *)H264Enc_MemAlloc(vo, 406*8, 8, INTER_MEM);
    memcpy(vo->g_vlc_hw_ptr, g_vlc_hw_tbl, (406*8));

    img_ptr->orig_width = pVideoFormat->frame_width;
    img_ptr->orig_height = pVideoFormat->frame_height;

    img_ptr->width = (img_ptr->orig_width + 15)&(~15);
    img_ptr->height = (img_ptr->orig_height + 15)&(~15);

// 	img_ptr->i_frame = 0;
    img_ptr->frame_num = 0;

    img_ptr->i_idr_pic_id = 0;

    img_ptr->i_sps_id = 0;
    img_ptr->i_level_idc = 51;	//as close to "unresticted as we can get
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
    img_ptr->pYUVRecFrame = (H264EncStorablePic *)H264Enc_MemAlloc(vo, sizeof(H264EncStorablePic), 8, INTER_MEM);
    img_ptr->pYUVRefFrame = (H264EncStorablePic *)H264Enc_MemAlloc(vo, sizeof(H264EncStorablePic), 8, INTER_MEM);
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
    img_ptr->pYUVRecFrame->imgUV = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size/2, 8, EXTRA_MEM);
    img_ptr->pYUVRecFrame->imgYAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRecFrame->imgY, EXTRA_MEM) >> 3;	// DWORD
    img_ptr->pYUVRecFrame->imgUVAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRecFrame->imgUV, EXTRA_MEM) >> 3;	// DWORD

    img_ptr->pYUVRefFrame->imgY = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size, 8, EXTRA_MEM);
    img_ptr->pYUVRefFrame->imgUV = (uint8 *)H264Enc_MemAlloc(vo, frame_buf_size/2, 8, EXTRA_MEM);
    img_ptr->pYUVRefFrame->imgYAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRefFrame->imgY, EXTRA_MEM) >> 3;	// DWORD
    img_ptr->pYUVRefFrame->imgUVAddr = (uint32)H264Enc_ExtraMem_V2P(vo, img_ptr->pYUVRefFrame->imgUV, EXTRA_MEM) >> 3;	// DWORD

    vo->g_anti_shake.enable_anti_shake = pVideoFormat->b_anti_shake;
    vo->g_anti_shake.shift_x = 0;
    vo->g_anti_shake.shift_y = 0;
    vo->g_anti_shake.input_width = 0;
    vo->g_anti_shake.input_height= 0;

    h264enc_sps_init (img_ptr);
    h264enc_pps_init (vo, img_ptr);

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->ddr_bandwidth_req_cnt = 0;
    vo->vsp_freq_div = 0;
    if (VSP_OPEN_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_ERROR;
    }

    return is_init_success;
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
        // for cr#211038, avoid div 0
        if (0 == (pConf->FrameRate%INTRA_PERIOD))
        {
            vo->rc_gop_paras.intra_period = INTRA_PERIOD;
        }
        else
        {
            vo->rc_gop_paras.intra_period = pConf->FrameRate;
        }
    }
    else
    {
        vo->rc_gop_paras.intra_period = pConf->FrameRate;
    }

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

    VSP_CLOSE_Dev((VSPObject *)vo);

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
        }
        else
        {
            img_ptr->pYUVSrcFrame->i_type = i_slice_type = SLICE_TYPE_P;
        }
    }
    else
    {
        img_ptr->pYUVSrcFrame->i_type = i_slice_type =( pInput->vopType == 0)? SLICE_TYPE_I : SLICE_TYPE_P/* = (int32)h264enc_slicetype_decide(img_ptr)*/;
    }

    img_ptr->pYUVSrcFrame->imgY =  pInput->p_src_y_phy;
    img_ptr->pYUVSrcFrame->imgUV =  pInput->p_src_u_phy;
    img_ptr->pYUVSrcFrame->imgYAddr = (uint32)img_ptr->pYUVSrcFrame->imgY >> 3;
    img_ptr->pYUVSrcFrame->imgUVAddr = (uint32)img_ptr->pYUVSrcFrame->imgUV >> 3;

    if( i_slice_type == SLICE_TYPE_I )
    {
        //img_ptr->i_last_idr = img_ptr->pYUVSrcFrame->i_frame;

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

    H264Enc_InitVSP(vo);

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
    *((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset])) = 0x01000000;
    img_ptr->slice_sz[img_ptr->slice_nr] += (VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x2c,"ORSC: DSTUF_NUM") << 3);

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
        vo->g_nFrame_enc++;
        if(img_ptr->pYUVRecFrame->b_kept_as_ref)
        {
            img_ptr->frame_num++;
        }

        if(img_ptr->i_nal_type == NAL_SLICE_IDR)
        {
            img_ptr->i_idr_pic_id = (img_ptr->i_idr_pic_id + 1) % 65536;
        }
    }

    VSP_RELEASE_Dev((VSPObject *)vo);

    return MMENC_OK;
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

    SCI_TRACE_LOW("%s, %d.", __FUNCTION__, __LINE__);
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x20, STREAM_ID_H264|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

    img_ptr->stm_offset = 0;

    H264Enc_InitBSM(vo);

    if (is_sps)
    {
        //generate sequence parameters
        h264enc_sps_write(vo, img_ptr->sps);
    } else
    {
        h264enc_pps_write(vo, img_ptr->pps);
    }
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, TIME_OUT_CLK, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
    VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
    VSP_READ_REG_POLL(GLB_REG_BASE_ADDR+0x1C, V_BIT_1, 0x0, TIME_OUT_CLK, "ORSC: Polling AXIM_STS: not Axim_wch_busy"); //check all data has written to DDR

    *((volatile uint32*)(&img_ptr->pOneFrameBitstream[img_ptr->stm_offset])) = 0x01000000;
    img_ptr->stm_offset = ( VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC: TOTAL_BITS") >> 3);
    img_ptr->stm_offset += VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x2c,"ORSC: DSTUF_NUM");
    img_ptr->stm_offset = (img_ptr->stm_offset+7)&0xfffffff8; // DWORD aligned

    pOutput->strmSize = img_ptr->stm_offset;
    pOutput->pOutBuf = img_ptr->pOneFrameBitstream;

    VSP_RELEASE_Dev((VSPObject *)vo);

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

