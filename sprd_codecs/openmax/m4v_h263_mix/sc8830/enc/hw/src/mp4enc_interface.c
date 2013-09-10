/******************************************************************************
 ** File Name:    mp4enc_interface.c										  *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         12/14/2006                                                  *
 ** Copyright:    2006 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 12/14/2006    Xiaowei Luo     Create.                                     *
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
    uint8 video_type = vol_mode_ptr->short_video_header ?STREAM_ID_H263: STREAM_ID_MPEG4 ;
    uint32 NumBits = 0;

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF,( 1 << 4) |video_type, "ORSC: VSP_MODE: Set standard, work mode and manual mode");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");

    Mp4Enc_InitBSM(vo);

    if(!vop_mode_ptr->short_video_header)   //MPEG-4 case
    {
        NumBits = Mp4Enc_EncSequenceHeader(vo);
        NumBits += Mp4Enc_EncVOHeader(vo);
        NumBits += Mp4Enc_EncVOLHeader(vo);

        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, TIME_OUT_CLK, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle

        pOutput->strmSize = (VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC_SHARE: read total bits") + 7) >> 3;
    } else
    {
        pOutput->strmSize = 0;
    }

    pOutput->pOutBuf = vop_mode_ptr->pOneFrameBitstream;

    VSP_RELEASE_Dev((VSPObject *)vo);

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

    VSP_CLOSE_Dev((VSPObject *)vo);

    return MMENC_OK;
}

/*****************************************************************************/
//  Description:   Init mpeg4 encoder
//	Global resource dependence:
//  Author:
//	Note:
/*****************************************************************************/
MMEncRet MP4EncInit(MP4Handle *mp4Handle, MMCodecBuffer *pInterMemBfr, MMCodecBuffer *pExtaMemBfr,MMCodecBuffer *pBitstreamBfr, MMEncVideoInfo *pVideoFormat)
{
    Mp4EncObject*vo;
    uint16 frame_width = pVideoFormat->frame_width;
    uint16 frame_height = pVideoFormat->frame_height;
    VOL_MODE_T  *vol_mode_ptr = NULL;
    ENC_VOP_MODE_T  *vop_mode_ptr = NULL;
    MMEncRet init_return;

    SCI_TRACE_LOW("libomx_m4vh263enc_hw_sprd.so is built on %s %s, Copyright (C) Spreatrum, Inc.", __DATE__, __TIME__);

    vo = (Mp4EncObject *) (pInterMemBfr->common_buffer_ptr);
    memset(vo, 0, sizeof(Mp4EncObject));
    mp4Handle->videoEncoderData = (void *) vo;
    vo->mp4Handle = mp4Handle;

    pInterMemBfr->common_buffer_ptr += sizeof(Mp4EncObject);
    pInterMemBfr->common_buffer_ptr_phy = (void *)((uint32)(pInterMemBfr->common_buffer_ptr_phy) + sizeof(Mp4EncObject));
    pInterMemBfr->size -= sizeof(Mp4EncObject);


    Mp4Enc_InitMem(vo, pInterMemBfr, pExtaMemBfr);
    vo->g_enc_vol_mode_ptr= vol_mode_ptr = (VOL_MODE_T *)Mp4Enc_MemAlloc(vo, sizeof(VOL_MODE_T), 4, INTER_MEM);
    vo->g_enc_vop_mode_ptr = vop_mode_ptr = (ENC_VOP_MODE_T *)Mp4Enc_MemAlloc(vo, sizeof(ENC_VOP_MODE_T), 4, INTER_MEM);

    vo->g_vlc_hw_ptr = (uint32 *) Mp4Enc_MemAlloc(vo, 320*8, 8, INTER_MEM);
    memcpy(vo->g_vlc_hw_ptr, g_vlc_hw_tbl, (320*8));

    vo->g_rc_ptr = (rc_single_t *)Mp4Enc_MemAlloc(vo, sizeof(rc_single_t), 4, INTER_MEM);
    vo->g_rc_data_ptr = (xvid_plg_data_t *)Mp4Enc_MemAlloc(vo, sizeof(xvid_plg_data_t), 4, INTER_MEM);

    vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header  = pVideoFormat->is_h263;
    vop_mode_ptr->uv_interleaved = pVideoFormat->uv_interleaved = 1;

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

    Mp4Enc_InitVolVopPara(vol_mode_ptr, vop_mode_ptr, pVideoFormat->time_scale);
    Mp4Enc_InitSession(vo);

    vop_mode_ptr->pOneFrameBitstream = pBitstreamBfr->common_buffer_ptr;
    vop_mode_ptr->OneFrameBitstream_addr_phy = (uint32)pBitstreamBfr->common_buffer_ptr_phy;
    vop_mode_ptr->OneframeStreamLen = pBitstreamBfr->size;

    vo->s_vsp_fd = -1;
    vo->s_vsp_Vaddr_base = 0;
    vo->ddr_bandwidth_req_cnt = 0;
    vo->vsp_freq_div = 0;
    if(VSP_OPEN_Dev((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, STREAM_ID_MPEG4|(1<<4), "ORSC: VSP_MODE: Set standard and work mode");

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
    VOP_PRED_TYPE_E frame_type;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    BOOLEAN *pIs_prev_frame_success = &(vo->g_enc_is_prev_frame_encoded_success);
    ENC_ANTI_SHAKE_T *anti_shark_ptr = &(vo->g_anti_shake);
    uint32 frame_width = vop_mode_ptr->FrameWidth;
    uint32 frame_height = vop_mode_ptr->FrameHeight;
    uint32 frame_size = frame_width * frame_height;
    BOOLEAN frame_skip = FALSE;
    uint8 video_type =vol_mode_ptr->short_video_header ?STREAM_ID_H263: STREAM_ID_MPEG4 ;

    anti_shark_ptr->input_width = pInput->org_img_width;
    anti_shark_ptr->input_height = pInput->org_img_height;
    anti_shark_ptr->shift_x = pInput->crop_x;
    anti_shark_ptr->shift_y = pInput->crop_y;

    if (video_type == STREAM_ID_H263)
    {
        Mp4Enc_ReviseLumaData(pInput->p_src_y, pInput->org_img_width, pInput->org_img_height);
    }

    if(ARM_VSP_RST((VSPObject *)vo)<0)
    {
        return MMDEC_HW_ERROR;
    }

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+AXIM_ENDIAN_OFF, 0x30868,"axim endian set, vu format"); // VSP and OR endian.

    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF,( 1 << 4) |video_type, "ORSC: VSP_MODE: Set standard, work mode and manual mode");

    VSP_WRITE_REG(GLB_REG_BASE_ADDR+RAM_ACC_SEL_OFF, 0,"RAM_ACC_SEL: software access.");

    vop_mode_ptr->mbline_num_slice	= (vop_mode_ptr->FrameHeight+MB_SIZE-1)>>4;//5;//pVop_mode->FrameHeight/MB_SIZE;
    vop_mode_ptr->intra_mb_dis		= 30;

    if(!vop_mode_ptr->bInitRCSuceess)
    {
        rc_single_create( vop_mode_ptr, vo->g_rc_ptr, vo->g_rc_data_ptr);

        vop_mode_ptr->bInitRCSuceess = TRUE;

        vo->g_nFrame_enc = 0;
        vo->g_stat_rc_nvop_cnt = -1;
        vo->g_enc_last_modula_time_base = pInput->time_stamp / (int32)vol_mode_ptr->ClockRate; //the first frame
    }

    rc_single_before(vo->g_rc_ptr, vo->g_rc_data_ptr);

    if(vop_mode_ptr->QP_last[0] +vop_mode_ptr->QP_last[1] +vop_mode_ptr->QP_last[2] +vop_mode_ptr->QP_last[3] +
            vop_mode_ptr->QP_last[4] +vop_mode_ptr->QP_last[5] +vop_mode_ptr->QP_last[6] +vop_mode_ptr->QP_last[7]  < 16)
    {
        vop_mode_ptr->Need_MinQp_flag =1;
    } else
    {
        vop_mode_ptr->Need_MinQp_flag =0;
    }

    if(!frame_skip)
    {
        frame_type  = PVOP;
        vop_mode_ptr->VopPredType	= (pInput->vopType == IVOP)?IVOP:frame_type;
        pInput->vopType = vop_mode_ptr->VopPredType;

        SCI_TRACE_LOW("g_nFrame_enc %d frame_type %d ", vo->g_nFrame_enc, vop_mode_ptr->VopPredType );

        vop_mode_ptr->pYUVSrcFrame->imgY = pInput->p_src_y_phy;
        vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgY / 8;
        vop_mode_ptr->pYUVSrcFrame->imgU = pInput->p_src_u_phy;
        vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgU / 8;

        if(IVOP == vop_mode_ptr->VopPredType)
        {
            vop_mode_ptr->StepI = vo->g_rc_data_ptr->quant >0? vo->g_rc_data_ptr->quant :1;
            ret = Mp4Enc_EncIVOP(vo, pInput->time_stamp);
            vo->g_enc_p_frame_count = vol_mode_ptr->PbetweenI;
            Update_lastQp(vop_mode_ptr->QP_last,8);
            vop_mode_ptr->QP_last[7] = vop_mode_ptr->StepI;
        } else if (PVOP == vop_mode_ptr->VopPredType)
        {
            int32 intra_mb_num = 0;

            vop_mode_ptr->StepP = vo->g_rc_data_ptr->quant >0? vo->g_rc_data_ptr->quant :1;

            Update_lastQp(vop_mode_ptr->QP_last,8);
            vop_mode_ptr->QP_last[7] = vop_mode_ptr->StepP;

            if(vop_mode_ptr->Need_MinQp_flag == 1)
            {
                vop_mode_ptr->StepP = 1;
            }

            ret = Mp4Enc_EncPVOP(vo, pInput->time_stamp);

            if(intra_mb_num > ((vop_mode_ptr->MBNumX  -2)*(vop_mode_ptr->MBNumY -2))/2)
            {
                ret = 0;
                frame_type = IVOP;
            } else
            {
                vo->g_enc_p_frame_count--;
            }
        } else
        {
            vo->g_enc_p_frame_count--;
            PRINTF ("No.%d Frame:\t NVOP\n", vo->g_nFrame_enc);
            ret = Mp4Enc_EncNVOP(vo, pInput->time_stamp);
        }

        (*pIs_prev_frame_success) = ret;

        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, TIME_OUT_CLK, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
        VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
        VSP_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle

        pOutput->strmSize = (VSP_READ_REG(BSM_CTRL_REG_BASE_ADDR+0x14,"ORSC_SHARE: read total bits") + 7)>>3;
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
    } else
    {
        pOutput->strmSize = 0;
    }

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
