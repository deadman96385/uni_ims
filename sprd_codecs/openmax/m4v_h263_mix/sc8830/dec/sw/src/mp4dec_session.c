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

/*****************************************************************************
 **	Name : 			Mp4Dec_InitDecoderPara
 ** Description:	Initialize the decode parameter.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
MMDecRet Mp4Dec_InitDecoderPara(Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;

    vo->g_dec_is_first_frame = TRUE;
    vo->g_dec_is_stop_decode_vol = FALSE;
    vo->g_dec_is_changed_format = FALSE;
    vo->g_dec_pre_vop_format = IVOP;

    memset(vo->g_FrmYUVBfr,0,sizeof(vo->g_FrmYUVBfr));

    vop_mode_ptr->error_flag = FALSE;
    vop_mode_ptr->return_pos = 0;
    vop_mode_ptr->return_pos1 = 0;
    vop_mode_ptr->return_pos2 = 0;
    vop_mode_ptr->g_nFrame_dec = 0;
    vop_mode_ptr->is_work_mode_set = 0;

    vop_mode_ptr->intra_acdc_pred_disable = TRUE;
    vop_mode_ptr->QuantizerType = Q_H263;
    vop_mode_ptr->bDataPartitioning = FALSE;
    vop_mode_ptr->bReversibleVlc = FALSE;
    vop_mode_ptr->bResyncMarkerDisable = TRUE;
    vop_mode_ptr->bAlternateScan = FALSE;
    vop_mode_ptr->bQuarter_pel = FALSE;
    vop_mode_ptr->bInterlace = FALSE;
    vop_mode_ptr->bInitSuceess = FALSE;

    vop_mode_ptr->pCurDispFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pCurDispFrame, "vop_mode_ptr->pCurDispFrame");

    vop_mode_ptr->pCurRecFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pCurRecFrame, "vop_mode_ptr->pCurRecFrame");

    vop_mode_ptr->pBckRefFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pBckRefFrame, "vop_mode_ptr->pBckRefFrame");

    vop_mode_ptr->pFrdRefFrame = (Mp4DecStorablePic *)Mp4Dec_MemAlloc(vo, sizeof(Mp4DecStorablePic), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pFrdRefFrame, "vop_mode_ptr->pFrdRefFrame");

    vop_mode_ptr->pCurDispFrame->bfrId = 0xFF;
    vop_mode_ptr->pCurRecFrame->bfrId = 0xFF;
    vop_mode_ptr->pBckRefFrame->bfrId = 0xFF;
    vop_mode_ptr->pFrdRefFrame->bfrId = 0xFF;

    return MMDEC_OK;
}

PUBLIC MMDecRet Mp4Dec_InitGlobal (Mp4DecObject *vo)
{
    DEC_VOP_MODE_T *vop_mode_ptr = NULL;

    vo->vop_mode_ptr = (DEC_VOP_MODE_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_VOP_MODE_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->vop_mode_ptr, "vo->vop_mode_ptr");
    vop_mode_ptr = vo->vop_mode_ptr;

    //for H263 plus header
    vo->h263_plus_head_info_ptr = (H263_PLUS_HEAD_INFO_T *)Mp4Dec_MemAlloc(vo, sizeof(H263_PLUS_HEAD_INFO_T), 4, INTER_MEM);
    CHECK_MALLOC(vo->h263_plus_head_info_ptr, "vo->h263_plus_head_info_ptr");

    vop_mode_ptr->bitstrm_ptr = (DEC_BS_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_BS_T), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->bitstrm_ptr, "vop_mode_ptr->bitstrm_ptr");

    vop_mode_ptr->pMbMode_B = (DEC_MB_MODE_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_MB_MODE_T), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->pMbMode_B, "vop_mode_ptr->pMbMode_B");

    vop_mode_ptr->mb_cache_ptr = (DEC_MB_BFR_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_MB_BFR_T), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->mb_cache_ptr, "vop_mode_ptr->mb_cache_ptr");

    vop_mode_ptr->IntraQuantizerMatrix = (uint8 *)Mp4Dec_MemAlloc(vo, sizeof(uint8)*BLOCK_SQUARE_SIZE, 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->IntraQuantizerMatrix, "vop_mode_ptr->IntraQuantizerMatrix");

    vop_mode_ptr->InterQuantizerMatrix = (uint8 *)Mp4Dec_MemAlloc(vo, sizeof(uint8)*BLOCK_SQUARE_SIZE, 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->InterQuantizerMatrix, "vop_mode_ptr->InterQuantizerMatrix");

    vop_mode_ptr->dbk_para = (DBK_PARA_T *)Mp4Dec_MemAlloc(vo, sizeof(DBK_PARA_T), 4, INTER_MEM);
    CHECK_MALLOC(vop_mode_ptr->dbk_para, "vop_mode_ptr->dbk_para");

    return Mp4Dec_InitDecoderPara(vo);
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

LOCAL void Mp4Dec_InitHuffmanTable(DEC_VOP_MODE_T *vop_mode_ptr)
{
    vop_mode_ptr->pMCBPCtabintra = g_dec_mcbpc_tab_intra;
    vop_mode_ptr->pMCBPCtab = g_dec_mcbpc_tab;
    vop_mode_ptr->pCBPYtab  = g_dec_cbpy_tab;
    vop_mode_ptr->pTMNMVtab0 = g_dec_tmn_mv_tab0;
    vop_mode_ptr->pTMNMVtab1 = g_dec_tmn_mv_tab1;
    vop_mode_ptr->pTMNMVtab2 = g_dec_tmn_mv_tab2;

    vop_mode_ptr->pDCT3Dtab0 = g_dec_dct3d_tab0;
    vop_mode_ptr->pDCT3Dtab1 = g_dec_dct3d_tab1;
    vop_mode_ptr->pDCT3Dtab2 = g_dec_dct3d_tab2;
    vop_mode_ptr->pDCT3Dtab3 = g_dec_dct3d_tab3;
    vop_mode_ptr->pDCT3Dtab4 = g_dec_dct3d_tab4;
    vop_mode_ptr->pDCT3Dtab5 = g_dec_dct3d_tab5;

    vop_mode_ptr->pInter_max_level = g_inter_max_level;
    vop_mode_ptr->pInter_max_run = g_inter_max_run;

    vop_mode_ptr->pIntra_max_level = g_intra_max_level;
    vop_mode_ptr->pIntra_max_run = g_intra_max_run;
}

LOCAL MMDecRet Mp4Dec_MallocFrmBfr(Mp4DecObject *vo)
{
//	uint32 size;

    if( MMDEC_OK != Mp4Dec_InitYUVBfr(vo) )
    {
        return MMDEC_MEMORY_ERROR;
    }

    /*backward reference frame and forward reference frame after extention*/
    //all these address should be 64 word aligned
    vo->vop_mode_ptr->pCurRecFrame->pDecFrame = &(vo->g_FrmYUVBfr[0]);

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
    DEC_VOP_MODE_T *vop_mode_ptr = vo->vop_mode_ptr;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    uint32 mb_num_x, mb_num_y, total_mb_num;
    uint32 i;

    /*MB number in hor and in ver and total MB number*/
    mb_num_x = vop_mode_ptr->MBNumX = (vop_mode_ptr->OrgFrameWidth + 15) >>4;
    mb_num_y = vop_mode_ptr->MBNumY = (vop_mode_ptr->OrgFrameHeight + 15) >>4;
    total_mb_num = vop_mode_ptr->MBNum  = (int16)(mb_num_x * mb_num_y);

    vop_mode_ptr->FrameWidth  = (int16)(mb_num_x <<4);
    vop_mode_ptr->FrameHeight = (int16)(mb_num_y <<4);

    vop_mode_ptr->FrameExtendWidth  = vop_mode_ptr->FrameWidth  + 2 * YEXTENTION_SIZE;
    vop_mode_ptr->FrameExtendHeight = vop_mode_ptr->FrameHeight + 2 * YEXTENTION_SIZE;

    vop_mode_ptr->iStartInFrameY = vop_mode_ptr->FrameExtendWidth * YEXTENTION_SIZE + YEXTENTION_SIZE;
    vop_mode_ptr->iStartInFrameUV = (vop_mode_ptr->FrameExtendWidth >>1) * UVEXTENTION_SIZE + UVEXTENTION_SIZE;

    if (vo->mp4Handle->VSP_extMemCb(vo->mp4Handle->userdata, vop_mode_ptr->FrameWidth, vop_mode_ptr->FrameHeight, vop_mode_ptr->bDataPartitioning) < 0)
    {
        SCI_TRACE_LOW("%s, %d, extra memory is not enough", __FUNCTION__, __LINE__);
        return MMDEC_MEMORY_ERROR;
    }

    /*MB mode for a frame*/
    vop_mode_ptr->pMbMode = (DEC_MB_MODE_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_MB_MODE_T) * total_mb_num, 4, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->pMbMode, "vop_mode_ptr->pMbMode");

    vop_mode_ptr->pMbMode_prev = (DEC_MB_MODE_T *)Mp4Dec_MemAlloc(vo, sizeof(DEC_MB_MODE_T) * total_mb_num, 4, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->pMbMode_prev, "vop_mode_ptr->pMbMode_prev");

    vop_mode_ptr->mbdec_stat_ptr = (uint8 *)Mp4Dec_MemAlloc (vo, total_mb_num * sizeof(uint8), 4, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->mbdec_stat_ptr, "vop_mode_ptr->mbdec_stat_ptr");

#ifdef _MP4CODEC_DATA_PARTITION_
    if (vop_mode_ptr->bDataPartitioning)
    {
        uint32 i;

        vop_mode_ptr->g_dec_dc_store = (int32 **)Mp4Dec_MemAlloc(vo, sizeof (int32 *) * total_mb_num, 4, SW_CACHABLE);
        CHECK_MALLOC(vop_mode_ptr->g_dec_dc_store, "vop_mode_ptr->g_dec_dc_store");

        for (i = 0; i < total_mb_num; i++)
        {
            vop_mode_ptr->g_dec_dc_store[i] = (int32 *) Mp4Dec_MemAlloc(vo, sizeof (int32) * 6, 4, SW_CACHABLE);
            CHECK_MALLOC(vop_mode_ptr->g_dec_dc_store[i], "vop_mode_ptr->g_dec_dc_store[i]");
        }
    }
#endif //_MP4CODEC_DATA_PARTITION_	

    vop_mode_ptr->pTopCoeff = (int16 *)Mp4Dec_MemAlloc( vo, sizeof(int16) * (uint8)(vop_mode_ptr->MBNumX) * 4 * 8, 4, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->pTopCoeff, "vop_mode_ptr->pTopCoeff");

    vop_mode_ptr->mb_cache_ptr->pMBBfrY = (int8 *)Mp4Dec_MemAlloc(vo, sizeof(int8) * BLOCK_SQUARE_SIZE*4, 256, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->mb_cache_ptr->pMBBfrY, "vop_mode_ptr->mb_cache_ptr->pMBBfrY");

    vop_mode_ptr->mb_cache_ptr->pMBBfrU = (int8 *)Mp4Dec_MemAlloc(vo, sizeof(int8) * BLOCK_SQUARE_SIZE, 256, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->mb_cache_ptr->pMBBfrU, "vop_mode_ptr->mb_cache_ptr->pMBBfrU");

    vop_mode_ptr->mb_cache_ptr->pMBBfrV = (int8 *)Mp4Dec_MemAlloc(vo, sizeof(int8) * BLOCK_SQUARE_SIZE, 256, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->mb_cache_ptr->pMBBfrV, "vop_mode_ptr->mb_cache_ptr->pMBBfrV");

    vop_mode_ptr->pLeftCoeff = (int16 *)Mp4Dec_MemAlloc(vo, sizeof(int16) * 4 * 8, 4, SW_CACHABLE);
    CHECK_MALLOC(vop_mode_ptr->pLeftCoeff, "vop_mode_ptr->pLeftCoeff");

    for (i = 0; i < BLOCK_CNT; i++)
    {
        vop_mode_ptr->coef_block[i] = (int16 *)Mp4Dec_MemAlloc(vo, sizeof(int16) * BLOCK_SQUARE_SIZE, 4, SW_CACHABLE);
        CHECK_MALLOC(vop_mode_ptr->coef_block[i], "vop_mode_ptr->coef_block[i]");

        if (i < 4)
        {
            mb_cache_ptr->blk8x8_offset[i] = (((i >> 1) *vop_mode_ptr->FrameExtendWidth+ (i & 1) )<<3);
        }
    }

    vop_mode_ptr->MB_in_VOP_length = Mp4Dec_Compute_log2(vop_mode_ptr->MBNum);

    Mp4Dec_InitHuffmanTable(vop_mode_ptr);

    vop_mode_ptr->post_filter_en = FALSE;
    if(vop_mode_ptr->VT_used)
    {
        vop_mode_ptr->post_filter_en = TRUE;
    }

    if( MMDEC_OK != Mp4Dec_MallocFrmBfr(vo) )
    {
        return MMDEC_MEMORY_ERROR;
    }

    vop_mode_ptr->pStandardZigzag = g_standard_zigzag_trans;
    if(vop_mode_ptr->bDataPartitioning)
    {
        vo->g_Mp4Dec_IVOP = Mp4Dec_DecIVOPErrResDataPartitioning;
        vo->g_Mp4Dec_PVOP = Mp4Dec_DecPVOPErrResDataPartitioning;
    } else
    {
        vo->g_Mp4Dec_IVOP = Mp4Dec_DecIVOP;
        vo->g_Mp4Dec_PVOP = Mp4Dec_DecPVOP;
        vo->g_Mp4Dec_BVOP = Mp4Dec_DecBVOP;
    }

    vop_mode_ptr->pVerticalZigzag = g_vertical_scan;
    vop_mode_ptr->pHorizontalZigzag = g_horizontal_scan;
    vop_mode_ptr->pZigzag = vop_mode_ptr->pStandardZigzag;

    if(MPEG4 == vop_mode_ptr->video_std)
    {
        vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Dec_Compute_log2(vop_mode_ptr->time_inc_resolution);
    } else
    {
        vop_mode_ptr->mb_cache_ptr->iDcScalerY = 8;
        vop_mode_ptr->mb_cache_ptr->iDcScalerC = 8;
    }

    /*for B frame support*/
    vop_mode_ptr->time = vop_mode_ptr->time_base = vop_mode_ptr->last_time_base = 0;
    vop_mode_ptr->last_non_b_time = 0;

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
