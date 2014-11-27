/******************************************************************************
 ** File Name:    mp4enc_interface.c										  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         09/14/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 09/14/2013    Xiaowei Luo     Create.                                     *
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "mp4enc_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/*****************************************************************************/
//  Description:   Generate mpeg4 header
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGenHeader(MP4Handle *mp4Handle, MMEncOut *pOutput)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    uint8 video_type = (vol_mode_ptr->short_video_header ? STREAM_ID_H263 : STREAM_ID_MPEG4);
    uint32 NumBits = 0;

    if(ARM_VSP_RST((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (V_BIT_4 |video_type), "VSP_MODE: Set standard, work mode and manual mode");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0, "RAM_ACC_SEL: software access.");

    Mp4Enc_InitBSM(vo);

    if(!vop_mode_ptr->short_video_header)   //MPEG-4 case
    {
        NumBits = Mp4Enc_EncSequenceHeader(vo);
        NumBits += Mp4Enc_EncVOHeader(vo);
        NumBits += Mp4Enc_EncVOLHeader(vo);

        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_27, 0x00000000, TIME_OUT_CLK, "Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable")) //check bsm is idle
        {
            goto HEADER_EXIT;
        }
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, V_BIT_1, "BSM_OPERATE: BSM_CLR");
        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "Polling BSM_DBG0: BSM inactive")) //check bsm is idle
        {
            goto HEADER_EXIT;
        }

        pOutput->strmSize = (VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "read total bits") + 7) >> 3;
    } else
    {
        pOutput->strmSize = 0;
    }

    pOutput->pOutBuf = vop_mode_ptr->pOneFrameBitstream;

HEADER_EXIT:

    SCI_TRACE_LOW("%s, %d, exit generating header, error_flag: %0x", __FUNCTION__, __LINE__, vo->error_flag);

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Set mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncSetConf(MP4Handle *mp4Handle, MMEncConfig *pConf)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;

    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    SCI_ASSERT(NULL != pConf);
    SCI_ASSERT(NULL != vol_mode_ptr);
    SCI_ASSERT(NULL != vop_mode_ptr);

    vol_mode_ptr->short_video_header	= pConf->h263En;
    vol_mode_ptr->ProfileAndLevel		= pConf->profileAndLevel;

    vop_mode_ptr->FrameRate			= pConf->FrameRate;
    vop_mode_ptr->targetBitRate		= pConf->targetBitRate;
    vop_mode_ptr->RateCtrlEnable		= pConf->RateCtrlEnable;

    SCI_TRACE_LOW("%s, %d, vop_mode_ptr->FrameRate: %d, vop_mode_ptr->targetBitRate: %d, vop_mode_ptr->RateCtrlEnable: %d",
                  __FUNCTION__, __LINE__, vop_mode_ptr->FrameRate, vop_mode_ptr->targetBitRate, vop_mode_ptr->RateCtrlEnable);

    vop_mode_ptr->StepI				= pConf->QP_IVOP;
    vop_mode_ptr->StepP				= pConf->QP_PVOP;

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Get mpeg4 encode config
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncGetConf(MP4Handle *mp4Handle, MMEncConfig *pConf)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;

    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    SCI_ASSERT(NULL != pConf);
    SCI_ASSERT(NULL != vol_mode_ptr);
    SCI_ASSERT(NULL != vop_mode_ptr);

    pConf->QP_IVOP = vop_mode_ptr->StepI;
    pConf->QP_PVOP = vop_mode_ptr->StepP;

    pConf->h263En = vol_mode_ptr->short_video_header;
    pConf->profileAndLevel = vol_mode_ptr->ProfileAndLevel;

    pConf->targetBitRate = vop_mode_ptr->targetBitRate;
    pConf->FrameRate = vop_mode_ptr->FrameRate;
    pConf->RateCtrlEnable = vop_mode_ptr->RateCtrlEnable;

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Close mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncRelease(MP4Handle *mp4Handle)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;

    if (VSP_CLOSE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    return MMENC_OK;
}

MMEncRet MP4EncGetCodecCapability(MP4Handle *mp4Handle, MMEncCapability *Capability)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;

    int32 codec_capability = vo->vsp_capability;
    if (codec_capability == 0)   //limited under 720p
    {
        Capability->max_width = 1280;
        Capability->max_height = 1023; //720;
    } else if (codec_capability == 1)   //limited under 1080p
    {
        Capability->max_width = 1920;
        Capability->max_height = 1088;
    } else if (codec_capability == 2)   //limited under 1080p
    {
        Capability->max_width = 1920;
        Capability->max_height = 1088;
    } else
    {
        Capability->max_width = 352;
        Capability->max_height = 288;
    }
    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Pre-Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncPreInit(MP4Handle *mp4Handle, MMCodecBuffer *pInterMemBfr)
{
    Mp4EncObject*vo;
    MMEncRet ret;

    SCI_TRACE_LOW("libomx_m4vh263enc_hw_sprd.so is built on %s %s, Copyright (C) Spreadtrum, Inc.", __DATE__, __TIME__);

    CHECK_MALLOC(pInterMemBfr, "pInterMemBfr");
    CHECK_MALLOC(pInterMemBfr->common_buffer_ptr, "internal memory");

    vo = (Mp4EncObject *) (pInterMemBfr->common_buffer_ptr);
    memset(vo, 0, sizeof(Mp4EncObject));
    mp4Handle->videoEncoderData = (void *) vo;
    vo->mp4Handle = mp4Handle;

    pInterMemBfr->common_buffer_ptr += sizeof(Mp4EncObject);
    pInterMemBfr->common_buffer_ptr_phy = pInterMemBfr->common_buffer_ptr_phy + sizeof(Mp4EncObject);
    pInterMemBfr->size -= sizeof(Mp4EncObject);

    ret = Mp4Enc_InitMem(vo, pInterMemBfr, INTER_MEM);
    if (ret != MMENC_OK)
    {
        return ret;
    }

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->vsp_capability = -1;
    if(VSP_OPEN_Dev((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncInit(MP4Handle *mp4Handle, MMCodecBuffer *pExtraMemBfr,
                    MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;
    uint16 frame_width = pVideoFormat->frame_width;
    uint16 frame_height = pVideoFormat->frame_height;
    VOL_MODE_T  *vol_mode_ptr = NULL;
    ENC_VOP_MODE_T  *vop_mode_ptr = NULL;
    MMEncRet ret;

    CHECK_MALLOC(pExtraMemBfr, "pExtraMemBfr");
    CHECK_MALLOC(pExtraMemBfr->common_buffer_ptr, "extranal memory");

    CHECK_MALLOC(pBitstreamBfr, "pBitstreamBfr");
    CHECK_MALLOC(pBitstreamBfr->common_buffer_ptr, "bit stream memory");

    ret = Mp4Enc_InitMem(vo, pExtraMemBfr, EXTRA_MEM);
    if (ret != MMENC_OK)
    {
        return ret;
    }

    vo->g_enc_vol_mode_ptr = vol_mode_ptr = (VOL_MODE_T *)Mp4Enc_MemAlloc(vo, sizeof(VOL_MODE_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_enc_vol_mode_ptr, "vo->g_enc_vol_mode_ptr");

    vo->g_enc_vop_mode_ptr = vop_mode_ptr = (ENC_VOP_MODE_T *)Mp4Enc_MemAlloc(vo, sizeof(ENC_VOP_MODE_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_enc_vop_mode_ptr, "vo->g_enc_vop_mode_ptr");

    vo->g_vlc_hw_ptr = (uint32 *) Mp4Enc_MemAlloc(vo, 320*2*sizeof(uint32), 8, EXTRA_MEM);
    CHECK_MALLOC(vo->g_vlc_hw_ptr, "vo->g_vlc_hw_ptr");

    vo->g_rc_ptr = (rc_single_t *)Mp4Enc_MemAlloc(vo, sizeof(rc_single_t), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_rc_ptr, "vo->g_rc_ptr");

    vo->g_rc_data_ptr = (xvid_plg_data_t *)Mp4Enc_MemAlloc(vo, sizeof(xvid_plg_data_t), 4, INTER_MEM);
    CHECK_MALLOC(vo->g_rc_data_ptr, "vo->g_rc_data_ptr");

    vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header  = pVideoFormat->is_h263;
    //vop_mode_ptr->uv_interleaved = pVideoFormat->uv_interleaved;

    vol_mode_ptr->VolWidth = frame_width;
    vol_mode_ptr->VolHeight = frame_height;

    vo->g_enc_last_modula_time_base = 0;
    vo->g_enc_tr = 0;
    vo->g_enc_is_prev_frame_encoded_success = FALSE;
    vo->g_enc_p_frame_count = 0;
    vo->g_nFrame_enc = 0;

    vo->g_anti_shake.enable_anti_shake = pVideoFormat->b_anti_shake;
    vo->g_anti_shake.shift_x = 0;
    vo->g_anti_shake.shift_y = 0;
    vo->g_anti_shake.input_width = 0;
    vo->g_anti_shake.input_height= 0;
    vo->yuv_format = pVideoFormat->yuv_format;

    Mp4Enc_InitVolVopPara(vol_mode_ptr, vop_mode_ptr, pVideoFormat->time_scale);
    ret = Mp4Enc_InitSession(vo);
    if (ret != MMENC_OK)
    {
        return ret;
    }

    vop_mode_ptr->pOneFrameBitstream = pBitstreamBfr->common_buffer_ptr;
    vop_mode_ptr->OneFrameBitstream_addr_phy = pBitstreamBfr->common_buffer_ptr_phy;
    vop_mode_ptr->OneframeStreamLen = pBitstreamBfr->size;

    if (vo->vsp_capability == 2)
    {
        memcpy(vo->g_vlc_hw_ptr,& g_vlc_hw_tbl[320*2], (320*2*sizeof(uint32)));
    }
    else
    {
        memcpy(vo->g_vlc_hw_ptr, g_vlc_hw_tbl, (320*2*sizeof(uint32)));
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (V_BIT_4 | STREAM_ID_MPEG4), "VSP_MODE: Set standard and work mode");

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Encode one vop
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncStrmEncode(MP4Handle *mp4Handle, MMEncIn *pInput, MMEncOut *pOutput)
{
    Mp4EncObject *vo = (Mp4EncObject *) mp4Handle->videoEncoderData;
    int32 ret;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    ENC_ANTI_SHAKE_T *anti_shark_ptr = &(vo->g_anti_shake);
    BOOLEAN frame_skip = FALSE; //now, not support skip frame, noted by xiaowei@2013.08.16
    uint8 video_type = (vol_mode_ptr->short_video_header ? STREAM_ID_H263 : STREAM_ID_MPEG4);

    vo->error_flag = 0;

    anti_shark_ptr->input_width = pInput->org_img_width;
    anti_shark_ptr->input_height = pInput->org_img_height;
    anti_shark_ptr->shift_x = pInput->crop_x;
    anti_shark_ptr->shift_y = pInput->crop_y;

    if (STREAM_ID_H263 == video_type)
    {
        Mp4Enc_ReviseLumaData(pInput->p_src_y, pInput->org_img_width, pInput->org_img_height);
    }

    if(ARM_VSP_RST((VSPObject *)vo) < 0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, 0x30868, "axim endian set, vu format"); // VSP and OR endian.
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, (V_BIT_4 |video_type), "VSP_MODE: Set standard, work mode and manual mode");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, 0, "RAM_ACC_SEL: software access.");

    vop_mode_ptr->mbline_num_slice	= (vop_mode_ptr->FrameHeight + MB_SIZE-1)>>4;//5;//pVop_mode->FrameHeight/MB_SIZE;

    if(!vop_mode_ptr->bInitRCSuceess)
    {
        rc_single_create( vop_mode_ptr, vo->g_rc_ptr, vo->g_rc_data_ptr);

        vop_mode_ptr->bInitRCSuceess = TRUE;

        vo->g_nFrame_enc = 0;
        vo->g_stat_rc_nvop_cnt = -1;
        vo->g_enc_last_modula_time_base = pInput->time_stamp / (int32)vol_mode_ptr->ClockRate; //the first frame
    }

    rc_single_before(vo->g_rc_ptr, vo->g_rc_data_ptr);

    if (vo->g_nFrame_enc > 7)
    {
        if(vop_mode_ptr->QP_last[0] +vop_mode_ptr->QP_last[1] +vop_mode_ptr->QP_last[2] +vop_mode_ptr->QP_last[3] +
                vop_mode_ptr->QP_last[4] +vop_mode_ptr->QP_last[5] +vop_mode_ptr->QP_last[6] +vop_mode_ptr->QP_last[7]  < 16)
        {
            vop_mode_ptr->Need_MinQp_flag =1;
        } else
        {
            vop_mode_ptr->Need_MinQp_flag =0;
        }
    } else
    {
        vop_mode_ptr->Need_MinQp_flag =0;
    }

    if(!frame_skip)
    {
        if (!vo->g_enc_is_prev_frame_encoded_success)
        {
            vop_mode_ptr->VopPredType = IVOP;
        }
        else
        {
            vop_mode_ptr->VopPredType = (pInput->vopType == IVOP) ? IVOP : PVOP;
        }

        pInput->vopType = vop_mode_ptr->VopPredType;

        SCI_TRACE_LOW("g_nFrame_enc %d frame_type %d ", vo->g_nFrame_enc, vop_mode_ptr->VopPredType );

        vop_mode_ptr->pYUVSrcFrame->imgY = pInput->p_src_y_phy;
        vop_mode_ptr->pYUVSrcFrame->imgYAddr = (((uint_32or64)vop_mode_ptr->pYUVSrcFrame->imgY) >> 3);

        vop_mode_ptr->pYUVSrcFrame->imgU = pInput->p_src_u_phy;
        vop_mode_ptr->pYUVSrcFrame->imgUAddr = (((uint_32or64)vop_mode_ptr->pYUVSrcFrame->imgU) >> 3);

        if ((vo->yuv_format == MMENC_YUV420P_YU12)||(vo->yuv_format == MMENC_YUV420P_YV12)) //three plane
        {
            vop_mode_ptr->pYUVSrcFrame->imgV = pInput->p_src_v_phy;
            vop_mode_ptr->pYUVSrcFrame->imgVAddr = (((uint_32or64)vop_mode_ptr->pYUVSrcFrame->imgV) >> 3);
        }

        if(IVOP == vop_mode_ptr->VopPredType)
        {
            vop_mode_ptr->StepSize = vop_mode_ptr->StepI = ((vo->g_rc_data_ptr->quant > 0) ? vo->g_rc_data_ptr->quant : 1);
            ret = Mp4Enc_EncVOP(vo, pInput->time_stamp);
            vo->g_enc_p_frame_count = vol_mode_ptr->PbetweenI;
            Update_lastQp(vop_mode_ptr->QP_last,8);
            vop_mode_ptr->QP_last[7] = vop_mode_ptr->StepI;
        } else if (PVOP == vop_mode_ptr->VopPredType)
        {
            vop_mode_ptr->StepP = ((vo->g_rc_data_ptr->quant > 0) ? vo->g_rc_data_ptr->quant : 1);

            Update_lastQp(vop_mode_ptr->QP_last, 8);
            vop_mode_ptr->QP_last[7] = vop_mode_ptr->StepP;

            if(vop_mode_ptr->Need_MinQp_flag == 1)
            {
                vop_mode_ptr->StepP = 1;
            }
            vop_mode_ptr->StepSize = vop_mode_ptr->StepP;
            ret = Mp4Enc_EncVOP(vo, pInput->time_stamp);

            vo->g_enc_p_frame_count--;
        } else
        {
#if 0
            vo->g_enc_p_frame_count--;
            SCI_TRACE_LOW ("No.%d Frame:\t NVOP\n", vo->g_nFrame_enc);
            ret = Mp4Enc_EncNVOP(vo, pInput->time_stamp);
#else
            SCI_TRACE_LOW ("ERROR!, NVOP is not supported");
            ret = 0;
#endif
        }

        if (vo->error_flag)
        {
            Mp4Enc_EncNVOP(vo, pInput->time_stamp);
            ret = MMENC_OK;
            vo->error_flag = 0;
            vo->g_nFrame_enc++;
            vo->g_enc_is_prev_frame_encoded_success =0;

            goto ENC_EXIT;
        } else
        {
            vo->g_enc_is_prev_frame_encoded_success =1;
        }

        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_27, 0x00000000, TIME_OUT_CLK, "Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable")) //check bsm is idle
        {
            goto ENC_EXIT;
        }
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, V_BIT_1, "BSM_OPERATE: BSM_CLR");
        if (VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + BSM_DBG0_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "Polling BSM_DBG0: BSM inactive")) //check bsm is idle
        {
            goto ENC_EXIT;
        }

        pOutput->strmSize = (VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR + TOTAL_BITS_OFF, "read total bits") + 7)>>3;
        pOutput->pOutBuf = vop_mode_ptr->pOneFrameBitstream;

        vo->g_rc_data_ptr->quant = vop_mode_ptr->StepSize;
        vo->g_rc_data_ptr->length = pOutput->strmSize;
        vo->g_rc_data_ptr->type = vop_mode_ptr->VopPredType + 1;
        rc_single_after(vo->g_rc_ptr, vo->g_rc_data_ptr);

        //update ref frame
        if(NVOP != vop_mode_ptr->VopPredType)
        {
            Mp4Enc_UpdateRefFrame(vop_mode_ptr);
        }

        vo->g_nFrame_enc++;
    } else
    {
        pOutput->strmSize = 0;
    }

ENC_EXIT:

    SCI_TRACE_LOW("%s, %d, exit encoder, error_flag: %0x", __FUNCTION__, __LINE__, vo->error_flag);

    if (VSP_RELEASE_Dev((VSPObject *)vo) < 0)
    {
        return MMENC_HW_ERROR;
    }

    if (vo->error_flag & ER_HW_ID)
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
