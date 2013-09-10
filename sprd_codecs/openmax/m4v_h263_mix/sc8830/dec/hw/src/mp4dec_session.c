/******************************************************************************
 ** File Name:    mp4dec_session.c                                         *
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
#include "mp4dec_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

PUBLIC void Mp4Dec_InitGlobal (Mp4DecObject *vo)
{
    vo->g_nFrame_dec = 0;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InitDecoderPara
 ** Description:	Initialize the decode parameter.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_InitDecoderPara(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;

    vop_mode_ptr->uv_interleaved = /*video_format_ptr->uv_interleaved = */1;
    vop_mode_ptr->intra_acdc_pred_disable = TRUE;
    vop_mode_ptr->QuantizerType = Q_H263;
    vop_mode_ptr->bDataPartitioning = FALSE;
    vop_mode_ptr->bReversibleVlc = FALSE;
    vop_mode_ptr->bResyncMarkerDisable = TRUE;
    vop_mode_ptr->bAlternateScan = FALSE;
    vop_mode_ptr->bQuarter_pel = FALSE;
    vop_mode_ptr->bInterlace = FALSE;
    vop_mode_ptr->bInitSuceess = FALSE;
    memset(vop_mode_ptr->InterQuantizerMatrix,0,64);
    memset(vop_mode_ptr->IntraQuantizerMatrix,0,64);
    vop_mode_ptr->NumGobInVop=0;
    vop_mode_ptr->NumMBInGob=0;
    vop_mode_ptr->num_mbline_gob=0;
    vop_mode_ptr->last_non_b_time=0;
    vop_mode_ptr->last_time_base=0;
    vop_mode_ptr->time_base=0;
    vop_mode_ptr->time=0;
    vop_mode_ptr->time_pp=0;
    vop_mode_ptr->time_bp=0;
    vop_mode_ptr->mvInfoForward.FCode=0;
    vop_mode_ptr->mvInfoBckward.FCode=0;
    vop_mode_ptr->QuantPrecision = 5;
    vop_mode_ptr->bCoded = TRUE;
    vop_mode_ptr->RoundingControl = 0;
    vop_mode_ptr->IntraDcSwitchThr = 0;

    vop_mode_ptr->pCurDispFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    vop_mode_ptr->pCurRecFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    vop_mode_ptr->pBckRefFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    vop_mode_ptr->pFrdRefFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);

    vop_mode_ptr->pCurDispFrame->bfrId = 0xFF;
    vop_mode_ptr->pCurRecFrame->bfrId = 0xFF;
    vop_mode_ptr->pBckRefFrame->bfrId = 0xFF;
    vop_mode_ptr->pFrdRefFrame->bfrId = 0xFF;

    vo->g_dec_pre_vop_format = IVOP;

    memset(vo->g_FrmYUVBfr,0,sizeof(vo->g_FrmYUVBfr));
}

LOCAL uint8 Mp4Dec_Compute_log2(int32 uNum)
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

LOCAL MMDecRet Mp4Dec_MallocFrmBfr(Mp4DecObject *vo, DEC_VOP_MODE_T *vop_mode_ptr)
{
//	uint32 size;

    if( MMDEC_OK != Mp4Dec_InitYUVBfr(vo) )
    {
        return MMDEC_MEMORY_ERROR;
    }

    /*backward reference frame and forward reference frame after extention*/
    //all these address should be 64 word aligned
    vop_mode_ptr->pCurRecFrame->pDecFrame = &(vo->g_FrmYUVBfr[0]);

    return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InitSessionDecode
 ** Description:	Initialize the decode parameter.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_InitSessionDecode(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->g_dec_vop_mode_ptr;
    uint32 mb_num_x;
    uint32 mb_num_y;
    uint32 total_mb_num;

    /*MB number in hor and in ver and total MB number*/
    mb_num_x = vop_mode_ptr->MBNumX = (vop_mode_ptr->OrgFrameWidth  + 15) / MB_SIZE;
    mb_num_y = vop_mode_ptr->MBNumY = (vop_mode_ptr->OrgFrameHeight + 15) / MB_SIZE;
    total_mb_num = vop_mode_ptr->MBNum  = (int16)(mb_num_x * mb_num_y);

    vop_mode_ptr->FrameWidth  = (int16)(mb_num_x * MB_SIZE);
    vop_mode_ptr->FrameHeight = (int16)(mb_num_y * MB_SIZE);

    VSP_config_freq(vo, 	vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight);

    vo->mp4Handle->VSP_extMemCb(vo->mp4Handle->userdata,  vop_mode_ptr->FrameWidth, vop_mode_ptr->FrameHeight);

    vop_mode_ptr->MB_in_VOP_length = Mp4Dec_Compute_log2(vop_mode_ptr->MBNum);

    if(STREAM_ID_MPEG4 == vop_mode_ptr->video_std)
    {
        vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Dec_Compute_log2(vop_mode_ptr->time_inc_resolution);
    }

    if( MMDEC_OK != Mp4Dec_MallocFrmBfr(vo, vop_mode_ptr) )
    {
        return MMDEC_MEMORY_ERROR;
    }

    // Malloc data_partition_buffer. 32 bytes for each MB.
    vop_mode_ptr->data_partition_buffer_ptr = Mp4Dec_MemAlloc(vo, (uint32)((mb_num_x * mb_num_y)*32), 256, EXTRA_MEM);
    vop_mode_ptr->data_partition_buffer_Addr = (uint32)Mp4Dec_MemV2P(vo, vop_mode_ptr->data_partition_buffer_ptr, EXTRA_MEM);

    /*for B frame support*/
    vop_mode_ptr->time = vop_mode_ptr->time_base = vop_mode_ptr->last_time_base = 0;
    vop_mode_ptr->last_non_b_time = 0;

    vo->g_tmp_buf.imgY = Mp4Dec_MemAlloc(vo, (uint32)(total_mb_num*256), 256, EXTRA_MEM);
    vo->g_tmp_buf.imgYAddr = (uint32)Mp4Dec_MemV2P(vo, vo->g_tmp_buf.imgY, EXTRA_MEM);
    vo->g_tmp_buf.imgU = Mp4Dec_MemAlloc(vo, (uint32)((total_mb_num*128)), 256, EXTRA_MEM);
    vo->g_tmp_buf.imgUAddr = (uint32)Mp4Dec_MemV2P(vo, vo->g_tmp_buf.imgU, EXTRA_MEM);

    memset(vo->g_tmp_buf.imgY, 16, sizeof(uint8)*(total_mb_num*256));
    memset(vo->g_tmp_buf.imgU, 128, sizeof(uint8)*(total_mb_num*128));

    vo->g_tmp_buf.rec_info = (uint8 *)Mp4Dec_MemAlloc(vo, (uint32)(total_mb_num*80), 256, EXTRA_MEM);
    vo->g_tmp_buf.rec_infoAddr = (uint32)Mp4Dec_MemV2P(vo, vo->g_tmp_buf.rec_info, EXTRA_MEM);



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
