/******************************************************************************
 ** File Name:    mp4enc_init.c  		                                      *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         06/09/2013                                                  *
 ** Copyright:    2013 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:                                                              *
 *****************************************************************************/
/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          NAME            DESCRIPTION                                 *
 ** 06/09/2013    Xiaowei Luo     Create.                                     *
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

PUBLIC void Mp4Enc_InitVolVopPara(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr, uint32 frame_rate)
{
    /*set to default value*/
    vol_mode_ptr->bNot8Bit					= FALSE;
    vol_mode_ptr->QuantPrecision			= 5;
    vol_mode_ptr->nBits					= 8;
    vol_mode_ptr->bCodeSequenceHead		= FALSE;
    vol_mode_ptr->ProfileAndLevel			= 0;
    vol_mode_ptr->bRoundingControlDisable	= TRUE;
    vol_mode_ptr->InitialRoundingType		= 0;
    vol_mode_ptr->PbetweenI				= 15;//300;//28;//weihu
    vol_mode_ptr->GOVperiod				= 0;
    vol_mode_ptr->bAllowSkippedPMBs		= 1;
    vol_mode_ptr->bReversibleVlc			= FALSE;
    vol_mode_ptr->bDataPartitioning		        = 0;
    vol_mode_ptr->bVPBitTh				= FALSE;
    vol_mode_ptr->QuantizerType			= Q_H263;
    vol_mode_ptr->TemporalRate			= 1;
    vol_mode_ptr->bOriginalForME			= FALSE;
    vol_mode_ptr->bAdvPredDisable			= TRUE;
    vol_mode_ptr->ClockRate				= (frame_rate ? frame_rate : 15);   //avoid to be divided by zero.
    vol_mode_ptr->fAUsage				= RECTANGLE;
    vol_mode_ptr->FrameHz				= 15; //30; //modidied by lxw,@0807
    vol_mode_ptr->MVRadiusPerFrameAwayFromRef = 8;
    vol_mode_ptr->intra_acdc_pred_disable  = FALSE;

    vop_mode_ptr->OrgFrameWidth			= vol_mode_ptr->VolWidth ;
    vop_mode_ptr->OrgFrameHeight			= vol_mode_ptr->VolHeight;
    vop_mode_ptr->bInterlace				= FALSE;
    vop_mode_ptr->IntraDcSwitchThr			= 0;

    if(vol_mode_ptr->short_video_header)
    {
        vop_mode_ptr->SearchRangeForward	= MAX_MV_Y_H263;
    } else
    {
        vop_mode_ptr->SearchRangeForward	= MAX_MV_X;//MAX_MV_Y;
    }

    vol_mode_ptr->bResyncMarkerDisable		= 0;//modified by lxw,@20090803
    vop_mode_ptr->mbline_num_slice			= 10;
    vop_mode_ptr->intra_mb_dis				= 30;
    vop_mode_ptr->StepI					= 12;
    vop_mode_ptr->StepP					= 12;

    vop_mode_ptr->bInitRCSuceess			= FALSE;
    vop_mode_ptr->sliceNumber				= 0;
    vop_mode_ptr->mb_x						= 0;
    vop_mode_ptr->mb_y						= 0;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitH263
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Enc_InitH263(Mp4EncObject *vo, VOL_MODE_T  *vol_mode_ptr, ENC_VOP_MODE_T  *vop_mode_ptr)
{
    vol_mode_ptr->QuantizerType = Q_H263;

    vop_mode_ptr->GOBResync = TRUE;
    vop_mode_ptr->bAlternateScan = 0;

    vo->g_enc_frame_skip_number = 0;

    switch(vop_mode_ptr->FrameWidth)
    {
    case 128:
        vop_mode_ptr->H263SrcFormat = 1;
        vop_mode_ptr->MBLineOneGOB  = 1;
        break;
    case 176:
        vop_mode_ptr->H263SrcFormat = 2;
        vop_mode_ptr->MBLineOneGOB  = 1;
        break;
    case 352:
        vop_mode_ptr->H263SrcFormat = 3;
        vop_mode_ptr->MBLineOneGOB  = 1;
        break;
#if 1 //dont support above CIF size.
    case 704:
        vop_mode_ptr->H263SrcFormat = 4;
        vop_mode_ptr->MBLineOneGOB  = 2;
        break;
    case 1408:
        vop_mode_ptr->H263SrcFormat = 5;
        vop_mode_ptr->MBLineOneGOB  = 4;
        break;
#endif  //#if 0
    default:
        PRINTF ("Illegal format!\n");
        vo->error_flag |= ER_SREAM_ID;
    }
}

/*****************************************************************************
 **	Name : 			Mp4Enc_SetMVInfo
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
MP4_LOCAL void Mp4Enc_SetMVInfo(ENC_VOP_MODE_T *vop_mode_ptr)
{
    MV_INFO_T *mvInfoForward = &(vop_mode_ptr->mvInfoForward);

    if(vop_mode_ptr->SearchRangeForward <= 512)
    {
        mvInfoForward->FCode = ((vop_mode_ptr->SearchRangeForward-1)>>4) + 1;
    } else
    {
        mvInfoForward->FCode = 7;
    }

    mvInfoForward->Range = 1 << (mvInfoForward->FCode + 4);
    mvInfoForward->ScaleFactor = 1 << (mvInfoForward->FCode - 1);//uiScaleFactor is f

    if((vop_mode_ptr->SearchRangeForward) == (mvInfoForward->Range >> 1))
    {
        vop_mode_ptr->SearchRangeForward--; // avoid out of range half pel
    }
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitVOEncoder
 ** Description:
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Enc_InitVOEncoder(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr)
{
    uint32 clock_rate;
    uint32 num_bits_time_incr;

    Mp4Enc_SetMVInfo(vop_mode_ptr);

    clock_rate = vol_mode_ptr->ClockRate-1;
    if(clock_rate>0)
    {
        //compute how many bits to denote iClockRate
        for(num_bits_time_incr = 1; num_bits_time_incr < NUMBITS_TIME_RESOLUTION; num_bits_time_incr++)
        {
            if(clock_rate == 1)
            {
                break;
            }
            clock_rate = (clock_rate >> 1);
        }
    } else
    {
        num_bits_time_incr = 1;
    }

    vop_mode_ptr->time_inc_resolution_in_vol_length = num_bits_time_incr;
    vop_mode_ptr->RoundingControlEncSwitch = vol_mode_ptr->InitialRoundingType;
    if(!vol_mode_ptr->bRoundingControlDisable)
    {
        vop_mode_ptr->RoundingControlEncSwitch ^= 0x00000001;
    }
    vop_mode_ptr->RoundingControl = vop_mode_ptr->RoundingControlEncSwitch;
}

MP4_LOCAL uint8 Mp4Enc_Compute_log2(int32 uNum)
{
    uint8 logLen = 1;

    uNum -= 1;

    SCI_ASSERT(uNum >= 0);

    while( (uNum >>= 1) > 0 )
    {
        logLen++;
    }

    return logLen;
}

PUBLIC MMEncRet Mp4Enc_InitSession(Mp4EncObject *vo)
{
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    uint32 total_mb_num_x;
    uint32 total_mb_num_y;
    int32 size;

    /*MB number in hor and in ver and total MB number*/
    vop_mode_ptr->OrgFrameWidth = vol_mode_ptr->VolWidth;
    vop_mode_ptr->OrgFrameHeight = vol_mode_ptr->VolHeight;
    total_mb_num_x = vop_mode_ptr->MBNumX = (vol_mode_ptr->VolWidth  + 15) / MB_SIZE;
    total_mb_num_y = vop_mode_ptr->MBNumY = (vol_mode_ptr->VolHeight + 15) / MB_SIZE;
    vop_mode_ptr->MBNum  = total_mb_num_x * total_mb_num_y;
    vop_mode_ptr->FrameWidth  = total_mb_num_x * MB_SIZE;
    vop_mode_ptr->FrameHeight = total_mb_num_y * MB_SIZE;
    vop_mode_ptr->iMaxVal = 1<<(vol_mode_ptr->nBits+3);
    vop_mode_ptr->QuantizerType = vol_mode_ptr->QuantizerType;
    vop_mode_ptr->intra_acdc_pred_disable = vol_mode_ptr->intra_acdc_pred_disable;
    vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header;
    vop_mode_ptr->QuantPrecision = vol_mode_ptr->QuantPrecision;
    vop_mode_ptr->MB_in_VOP_length = Mp4Enc_Compute_log2(vop_mode_ptr->MBNum);

    if(vop_mode_ptr->short_video_header)
    {
        Mp4Enc_InitH263(vo, vol_mode_ptr, vop_mode_ptr);
    } else
    {
        vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Enc_Compute_log2(vol_mode_ptr->ClockRate);
    }

    Mp4Enc_InitVOEncoder(vol_mode_ptr, vop_mode_ptr);

    /*backward reference frame and forward reference frame*/
    vop_mode_ptr->pYUVSrcFrame = (Mp4EncStorablePic *)Mp4Enc_MemAlloc(vo, sizeof(Mp4EncStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pYUVSrcFrame, "vop_mode_ptr->pYUVSrcFrame");

    vop_mode_ptr->pYUVRecFrame = (Mp4EncStorablePic *)Mp4Enc_MemAlloc(vo, sizeof(Mp4EncStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pYUVRecFrame, "vop_mode_ptr->pYUVRecFrame");

    vop_mode_ptr->pYUVRefFrame = (Mp4EncStorablePic *)Mp4Enc_MemAlloc(vo, sizeof(Mp4EncStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pYUVRefFrame, "vop_mode_ptr->pYUVRefFrame");

    size = (vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight);
    vop_mode_ptr->pYUVRecFrame->imgY = (uint8 *)Mp4Enc_MemAlloc(vo, size, 8, EXTRA_MEM);
    CHECK_MALLOC(vop_mode_ptr->pYUVRecFrame->imgY, "vop_mode_ptr->pYUVRecFrame->imgY");

    if ((vo->yuv_format == MMENC_YUV420P_YU12)||(vo->yuv_format == MMENC_YUV420P_YV12)) //three plane
    {
        vop_mode_ptr->pYUVRecFrame->imgU = (uint8 *)Mp4Enc_MemAlloc(vo, (size>>2), 8, EXTRA_MEM);
        CHECK_MALLOC(vop_mode_ptr->pYUVRecFrame->imgU, "vop_mode_ptr->pYUVRecFrame->imgU");

        vop_mode_ptr->pYUVRecFrame->imgV = (uint8 *)Mp4Enc_MemAlloc(vo, (size>>2), 8, EXTRA_MEM);
        CHECK_MALLOC(vop_mode_ptr->pYUVRecFrame->imgV, "vop_mode_ptr->pYUVRecFrame->imgV");
    } else
    {
        vop_mode_ptr->pYUVRecFrame->imgU = (uint8 *)Mp4Enc_MemAlloc(vo, (size>>1), 8, EXTRA_MEM);
        CHECK_MALLOC(vop_mode_ptr->pYUVRecFrame->imgU, "vop_mode_ptr->pYUVRecFrame->imgU");

        vop_mode_ptr->pYUVRecFrame->imgV = NULL;
    }

    vop_mode_ptr->pYUVRefFrame->imgY = (uint8 *)Mp4Enc_MemAlloc(vo, size, 8, EXTRA_MEM);
    CHECK_MALLOC(vop_mode_ptr->pYUVRefFrame->imgY, "vop_mode_ptr->pYUVRefFrame->imgY");

    if ((vo->yuv_format == MMENC_YUV420P_YU12)||(vo->yuv_format == MMENC_YUV420P_YV12)) //three plane
    {
        vop_mode_ptr->pYUVRefFrame->imgU = (uint8 *)Mp4Enc_MemAlloc(vo, (size>>2), 8, EXTRA_MEM);
        CHECK_MALLOC(vop_mode_ptr->pYUVRefFrame->imgU, "vop_mode_ptr->pYUVRefFrame->imgU");

        vop_mode_ptr->pYUVRefFrame->imgV = (uint8 *)Mp4Enc_MemAlloc(vo, (size>>2), 8, EXTRA_MEM);
        CHECK_MALLOC(vop_mode_ptr->pYUVRefFrame->imgV, "vop_mode_ptr->pYUVRefFrame->imgV");
    } else
    {
        vop_mode_ptr->pYUVRefFrame->imgU = (uint8 *)Mp4Enc_MemAlloc(vo, (size>>1), 8, EXTRA_MEM);
        CHECK_MALLOC(vop_mode_ptr->pYUVRefFrame->imgU, "vop_mode_ptr->pYUVRefFrame->imgU");

        vop_mode_ptr->pYUVRefFrame->imgV = NULL;
    }

    vop_mode_ptr->pYUVRecFrame->imgYAddr = (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, vop_mode_ptr->pYUVRecFrame->imgY, EXTRA_MEM) >> 3;
    vop_mode_ptr->pYUVRecFrame->imgUAddr = (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, vop_mode_ptr->pYUVRecFrame->imgU, EXTRA_MEM) >> 3;
    vop_mode_ptr->pYUVRecFrame->imgVAddr = (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, vop_mode_ptr->pYUVRecFrame->imgV, EXTRA_MEM) >> 3;

    vop_mode_ptr->pYUVRefFrame->imgYAddr = (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, vop_mode_ptr->pYUVRefFrame->imgY, EXTRA_MEM) >> 3;
    vop_mode_ptr->pYUVRefFrame->imgUAddr = (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, vop_mode_ptr->pYUVRefFrame->imgU, EXTRA_MEM) >> 3;
    vop_mode_ptr->pYUVRefFrame->imgVAddr = (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, vop_mode_ptr->pYUVRefFrame->imgV, EXTRA_MEM) >> 3;

    return MMENC_OK;
}

PUBLIC void Mp4Enc_InitVSP(Mp4EncObject *vo)
{
    uint32 cmd;
    uint32 slice_num, slice_num_of_frame;
    uint16 slice_first_mb_x, slice_first_mb_y,  slice_last_mb_y;
    int is_last_slice;
    ENC_VOP_MODE_T *vop_mode_ptr = vo->g_enc_vop_mode_ptr;
    VOL_MODE_T *vol_mode_ptr = vo->g_enc_vol_mode_ptr;
    ENC_ANTI_SHAKE_T *anti_shark_ptr = &(vo->g_anti_shake);
    VOP_PRED_TYPE_E frame_type = vop_mode_ptr->VopPredType;

    slice_num = vop_mode_ptr->mb_y / vop_mode_ptr->mbline_num_slice;
    slice_num_of_frame = vop_mode_ptr->MBNumY/vop_mode_ptr->mbline_num_slice + ((vop_mode_ptr->MBNumY%vop_mode_ptr->mbline_num_slice)?1:0);

    slice_first_mb_x = 0;
    slice_first_mb_y = vop_mode_ptr->mb_y - (vop_mode_ptr->mb_y%vop_mode_ptr->mbline_num_slice);

    slice_last_mb_y = slice_first_mb_y + vop_mode_ptr->mbline_num_slice - 1;
    slice_last_mb_y = (slice_last_mb_y>(vop_mode_ptr->MBNumY - 1)) ? (vop_mode_ptr->MBNumY - 1) : slice_last_mb_y;

    is_last_slice = ((vop_mode_ptr->MBNumY-vop_mode_ptr->mb_y) <= vop_mode_ptr->mbline_num_slice) ? 1 : 0;

    cmd = V_BIT_17|V_BIT_16|V_BIT_11|V_BIT_5|V_BIT_3;
    if (vo->yuv_format == MMENC_YUV420SP_NV21)  //vu format
    {
        cmd |= V_BIT_6;
    }
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + AXIM_ENDIAN_OFF, cmd,"axim endian set, vu format"); //VSP and OR endian.

    cmd = (((vol_mode_ptr->short_video_header) ? STREAM_ID_H263 : STREAM_ID_MPEG4) << 0);	// VSP_standard[3:0], 0x1:STREAM_ID_MPEG4
    cmd |= (1 << 4);		// Work_mode[4], 1:encode, 0:decode
    cmd |= (0 << 5);		// Manual_mode[5], 1:enable manual mode
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_MODE_OFF, cmd, "VSP_MODE: Set standard, work mode and manual mode");

    cmd = (0 << 0);	// SETTING_RAM_ACC_SEL[0], 1:access by HW ACC, 0:access by SW
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + RAM_ACC_SEL_OFF, cmd, "RAM_ACC_SEL: SETTING_RAM_ACC_SEL=0(SW)");

    cmd = (vop_mode_ptr->MBNumX << 0);	// MB_X_MAX[7:0]
    cmd |= (vop_mode_ptr->MBNumY << 8);	// MB_Y_MAX[15:8]
    cmd |= ((anti_shark_ptr->enable_anti_shake ? (anti_shark_ptr->input_width>>3) : (2*vop_mode_ptr->MBNumX)) <<16); // CUR_IMG_WIDTH[24:16]Unit 8 BYTE
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + IMG_SIZE_OFF, cmd, "IMG_SIZE: Set MB_X_MAX & MB_Y_MAX");

    cmd = ((frame_type == IVOP) ? 0 : 1);// FRM_TYPE[2:0], 0:I, 1:P
    cmd |= ((vop_mode_ptr->MBNum & 0x1fff) << 3);	// Max_mb_num[15:3]
    cmd |= ((slice_num & 0x1ff) << 16);// Slice_num[24:16]
    cmd |= (vop_mode_ptr->StepSize << 25);// SliceQP[30:25]
    cmd |= (0 << 31);	// Deblocking_eb[31]//???????????
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG0_OFF, cmd, "VSP_CFG0");

    cmd = ((0 & 0xff) << 0);	// Skip_threshold[7:0]
    cmd |= (9 << 8);	// Ime_16X16_max[11:8], less than 9
    cmd |= (1 << 12);	// Ime_8X8_max[15:12], less than 1
    cmd |= (511 << 16);	// Ipred_mode_cfg[24:16], IEA prediction mode setting
    cmd |= (vop_mode_ptr->StepSize << 25); // MB_Qp[30:25]
    cmd |= ((vol_mode_ptr->short_video_header ? (is_last_slice ? 1 : 0) : 1)<<31);//1:hardware auto bytealign at the end of each slice, Only for Encode mode
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG1_OFF, cmd, "VSP_CFG1");

    cmd = (slice_last_mb_y << 0);	// last_mb_y[6:0]
    cmd |= ((slice_num*vop_mode_ptr->mbline_num_slice) << 8);	// First_mb_y[14:8]
    cmd |= ((anti_shark_ptr->shift_x/2) << 16); //CUR_IMG_ST_X[25:16]Horizontal start position of cur image; unit:2 pixel
    cmd |= (0 << 29);	// Dct_h264_scale_en[29]
    cmd |= (1 << 30);	// MCA_rounding_type[30], For MCA only
    cmd |= (0 << 31);	// Ppa_info_vdb_eb[31], 1: PPA need write MB info to DDR
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG2_OFF, cmd, "VSP_CFG2");

    cmd = ((vop_mode_ptr->short_video_header & 0x1) << 0);	// is_short_header[0]
    cmd |= ((vol_mode_ptr->bDataPartitioning & 0x1) << 1);		// bDataPartitioning[1]
    cmd |= ((vol_mode_ptr->bReversibleVlc & 0x1) << 2);// bReversibleVlc[2]
    cmd |= ((vop_mode_ptr->IntraDcSwitchThr & 0x7) << 3);	// IntraDcSwitchThr[5:3]
    cmd |= ((vop_mode_ptr->mvInfoForward.FCode & 0x7) << 6);	// Vop_fcode_forward[8:6]
    cmd |= (0 << 9);	// Vop_fcode_backward[11:9]
    cmd |= (0 << 12);	// For MCA[12]
    cmd |= (0 << 18); // Reserved [31:18]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_CFG3_OFF, cmd, "VSP_CFG3");

    if (anti_shark_ptr->enable_anti_shake)
    {
        vop_mode_ptr->pYUVSrcFrame->imgY += (anti_shark_ptr->shift_y*anti_shark_ptr->input_width + anti_shark_ptr->shift_x);
        vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint_32or64)vop_mode_ptr->pYUVSrcFrame->imgY >> 3;
        vop_mode_ptr->pYUVSrcFrame->imgU += (anti_shark_ptr->shift_y*anti_shark_ptr->input_width/2 + anti_shark_ptr->shift_x);
        vop_mode_ptr->pYUVSrcFrame->imgUAddr = (uint_32or64)vop_mode_ptr->pYUVSrcFrame->imgU >> 3;
    }
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x20, vop_mode_ptr->pYUVSrcFrame->imgYAddr, "Frm_addr8: Start address of current frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x24, vop_mode_ptr->pYUVSrcFrame->imgUAddr, "Frm_addr9: Start address of current frame UV");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x0, vop_mode_ptr->pYUVRecFrame->imgYAddr, "Frm_addr0: Start address of reconstruct frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x4, vop_mode_ptr->pYUVRecFrame->imgUAddr, "Frm_addr1: Start address of reconstruct frame UV");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x80, vop_mode_ptr->pYUVRefFrame->imgYAddr, "Frm_addr32: Start address of Reference list0 frame Y");
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0x100, vop_mode_ptr->pYUVRefFrame->imgUAddr, "Frm_addr64: Start address of Reference list0 frame UV");

    cmd	= (uint_32or64)Mp4Enc_ExtraMem_V2P(vo, (uint8 *)vo->g_vlc_hw_ptr, EXTRA_MEM)>>3;		// Frm_addr3[31:0], VLC Table set by Fixed Table Address
    VSP_WRITE_REG(FRAME_ADDR_TABLE_BASE_ADDR + 0xC, cmd, "Frm_addr3: Start address of VLC table");
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + VSP_SIZE_SET_OFF, 128, "VSP_SIZE_SET: VLC_table_size");
}

PUBLIC void Mp4Enc_InitBSM(Mp4EncObject *vo)
{
    uint32 cmd;
    ENC_VOP_MODE_T  *vop_mode_ptr = vo->g_enc_vop_mode_ptr;

    cmd	= ((uint_32or64)vop_mode_ptr->OneFrameBitstream_addr_phy) >> 3;	// Bsm_buf0_frm_addr[31:0]
    VSP_WRITE_REG(GLB_REG_BASE_ADDR + BSM0_FRM_ADDR_OFF, cmd, "BSM0_FRM_ADDR");

    cmd = 0x04;	// Move data remain in fifo to external memeory
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_OP_OFF, cmd, "BSM_OPERATE: COUNT_CLR");

    cmd = (1 << 31);	// BUFF0_STATUS[31], 1: active
    cmd |= (0 << 30);	// BUFF1_STATUS[30]
    cmd |= ((vop_mode_ptr->OneframeStreamLen&(~0x3)) & (V_BIT_30 -1)); // BUFFER_SIZE[29:0], unit byte//cmd = g_bsm_reg_ptr->BSM_CFG0 << 0;
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_CFG0_OFF, cmd, "BSM_CFG0");

    cmd = (0 << 31);	// DESTUFFING_EN
    cmd |= ((0 & 0x3FFFFFFF) << 0);	// OFFSET_ADDR[29:0], unit word //cmd = g_bsm_reg_ptr->BSM_CFG1 << 0;
    VSP_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + BSM_CFG1_OFF, cmd, "BSM_CFG1");
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
