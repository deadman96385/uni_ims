/******************************************************************************
 ** File Name:    h264dec_init.c                                             *
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

LOCAL void H264Dec_init_old_slice (DEC_OLD_SLICE_PARAMS_T *old_slice_ptr)
{
    old_slice_ptr->field_pic_flag = 0;
    old_slice_ptr->pps_id = (uint8)UINT_MAX;
    old_slice_ptr->frame_num = (int32)UINT_MAX;
    old_slice_ptr->nal_ref_idc = (int8)0xff;
    old_slice_ptr->idr_flag = 0;
    old_slice_ptr->pic_order_cnt_lsb = SINT_MAX;
    old_slice_ptr->delta_pic_order_cnt_bottom = SINT_MAX;
    old_slice_ptr->delta_pic_order_cnt[0] = SINT_MAX;
    old_slice_ptr->delta_pic_order_cnt[1] = SINT_MAX;

    return;
}

PUBLIC MMDecRet H264Dec_init_global_para (H264DecContext *img_ptr)
{
    int32 i;

    img_ptr->g_sps_array_ptr = (DEC_SPS_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_SPS_T) * MAX_SPS, 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_sps_array_ptr, "img_ptr->g_sps_array_ptr");

    img_ptr->g_pps_array_ptr = (DEC_PPS_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_PPS_T) * MAX_PPS, 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_pps_array_ptr, "img_ptr->g_pps_array_ptr");

    img_ptr->g_sps_ptr = (DEC_SPS_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_SPS_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_sps_ptr, "img_ptr->g_sps_ptr");

    img_ptr->g_sps_ptr->vui_seq_parameters =  (DEC_VUI_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_VUI_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_sps_ptr->vui_seq_parameters, "img_ptr->g_sps_ptr->vui_seq_parameters");

    img_ptr->g_pps_ptr = (DEC_PPS_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_PPS_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_pps_ptr, "img_ptr->g_pps_ptr");

    img_ptr->g_dpb_ptr = (DEC_DECODED_PICTURE_BUFFER_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_DECODED_PICTURE_BUFFER_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_dpb_ptr, "img_ptr->g_dpb_ptr");

    img_ptr->g_dpb_ptr->used_size = 0;
    img_ptr->g_active_sps_ptr = NULL;
    img_ptr->g_active_pps_ptr = NULL;

    img_ptr->g_old_slice_ptr = (DEC_OLD_SLICE_PARAMS_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_OLD_SLICE_PARAMS_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_old_slice_ptr, "img_ptr->g_old_slice_ptr");

    img_ptr->g_nalu_ptr = (DEC_NALU_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_NALU_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_nalu_ptr, "img_ptr->g_nalu_ptr");

    img_ptr->bitstrm_ptr = (DEC_BS_T *)H264Dec_MemAlloc(img_ptr, sizeof(DEC_BS_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->bitstrm_ptr, "img_ptr->bitstrm_ptr");

    img_ptr->g_curr_slice_ptr = (DEC_SLICE_T *)H264Dec_MemAlloc(img_ptr, sizeof(DEC_SLICE_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_curr_slice_ptr, "img_ptr->g_curr_slice_ptr");

    img_ptr->g_no_reference_picture_ptr = (DEC_STORABLE_PICTURE_T *)H264Dec_MemAlloc(img_ptr, sizeof(DEC_STORABLE_PICTURE_T), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_no_reference_picture_ptr, "img_ptr->g_no_reference_picture_ptr");

    img_ptr->g_mb_cache_ptr = (DEC_MB_CACHE_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_MB_CACHE_T), 16, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_mb_cache_ptr, "img_ptr->g_mb_cache_ptr");

    img_ptr->g_dpb_ptr->fs = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (img_ptr, sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_dpb_ptr->fs, "img_ptr->g_dpb_ptr->fs");

    img_ptr->g_dpb_ptr->fs_ref = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (img_ptr, sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_dpb_ptr->fs_ref, "img_ptr->g_dpb_ptr->fs_ref");

    img_ptr->g_dpb_ptr->fs_ltref = (DEC_FRAME_STORE_T **)H264Dec_MemAlloc (img_ptr, sizeof(DEC_FRAME_STORE_T*)*(MAX_REF_FRAME_NUMBER+1), 4, INTER_MEM);
    CHECK_MALLOC(img_ptr->g_dpb_ptr->fs_ltref, "img_ptr->g_dpb_ptr->fs_ltref");

    for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
    {
        img_ptr->g_dpb_ptr->fs[i] = (DEC_FRAME_STORE_T *)H264Dec_MemAlloc (img_ptr, sizeof(DEC_FRAME_STORE_T), 4, INTER_MEM);
        CHECK_MALLOC(img_ptr->g_dpb_ptr->fs[i], "dpb_ptr->fs[i]");

        img_ptr->g_dpb_ptr->fs[i]->frame = (DEC_STORABLE_PICTURE_T *)H264Dec_MemAlloc(img_ptr, sizeof(DEC_STORABLE_PICTURE_T), 4, INTER_MEM);
        CHECK_MALLOC(img_ptr->g_dpb_ptr->fs[i]->frame, "dpb_ptr->fs[i]->frame");
    }

    //init global vars
    img_ptr->g_dec_ref_pic_marking_buffer_size = 0;
    img_ptr->g_ready_to_decode_slice = FALSE;
    img_ptr->g_searching_IDR_pic = 1;
    img_ptr->g_nFrame_dec_h264 = 0;
    img_ptr->g_old_pps_id = -1; //initialize to a impossible value
    img_ptr->last_dquant = 0;

    for (i = 0; i < MAX_REF_FRAME_NUMBER+1; i++)
    {
        img_ptr->g_list[0][i] = img_ptr->g_no_reference_picture_ptr;
    }

    img_ptr->dec_ref_pic_marking_buffer = img_ptr->g_dec_ref_pic_marking_buffer;

    for (i = 0; i < MAX_SPS; i++)
    {
        img_ptr->g_sps_array_ptr[i].valid = FALSE;
    }

    for (i = 0; i < MAX_PPS; i++)
    {
        img_ptr->g_pps_array_ptr[i].valid = FALSE;
    }

    img_ptr->curr_slice_ptr = img_ptr->g_curr_slice_ptr;
    img_ptr->curr_mb_nr = 0;
    img_ptr->error_flag = FALSE;
    img_ptr->return_pos = 0;
    img_ptr->return_pos1 = 0;
    img_ptr->return_pos2 = 0;
    img_ptr->not_supported = FALSE;
    img_ptr->sawSPS = FALSE;
    img_ptr->sawPPS = FALSE;

    img_ptr->g_no_reference_picture_ptr->used_for_reference = 0;
    img_ptr->g_no_reference_picture_ptr->is_long_term = 0;
    img_ptr->g_no_reference_picture_ptr->frame_num = 0;
    img_ptr->g_no_reference_picture_ptr->pic_num = 0;
    img_ptr->g_no_reference_picture_ptr->long_term_pic_num = 0;
    img_ptr->g_no_reference_picture_ptr->long_term_frame_idx = 0;

    img_ptr->g_active_pps_ptr = NULL;
    img_ptr->g_active_sps_ptr = NULL;

    img_ptr->g_MbToSliceGroupMap = NULL;

    //init old slice
    H264Dec_init_old_slice (img_ptr->g_old_slice_ptr);

    return MMDEC_OK;
}

//sw
PUBLIC void H264Dec_setClipTab (void)
{
#if 0
    int TabSize, maxVal, iOffset, i;

    TabSize = CLIP_TAB_SIZE;
    maxVal = 255;
    iOffset = (TabSize - maxVal)/2;

    //	g_rgiClipTab = (unsigned char *)malloc(sizeof(unsigned char)*TabSize);

    g_rgiClipTab = g_ClipTab_264 + iOffset;
    for (i = -iOffset; i < TabSize - iOffset; i++)
        g_rgiClipTab [i] = (i < 0) ? 0 : (i > maxVal) ? maxVal : i;
#else
    g_rgiClipTab = g_clip_tbl + 32*16;
#endif
}

//dx<<2 | dy
#define MAKE_LUMA_MC_FUNC(s)	\
		g_MC##s##_luma [0]  = MC_luma##s##_dx0dy0;\
		g_MC##s##_luma [1] = MC_luma##s##_dx0dyM;\
		g_MC##s##_luma [2] = MC_luma##s##_xyqpix;\
		g_MC##s##_luma [3] = MC_luma##s##_dx0dyM;\
		g_MC##s##_luma [4] = MC_luma##s##_dxMdy0;\
		g_MC##s##_luma [5]  = MC_luma##s##_xyqpix;\
		g_MC##s##_luma [6] = MC_luma##s##_yhalf;\
		g_MC##s##_luma [7]  = MC_luma##s##_xyqpix;\
		g_MC##s##_luma [8]  = MC_luma##s##_yfull;\
		g_MC##s##_luma [9]  = MC_luma##s##_xhalf;\
		g_MC##s##_luma [10] = MC_luma##s##_xhalf;\
		g_MC##s##_luma [11] = MC_luma##s##_xhalf;\
		g_MC##s##_luma [12] = MC_luma##s##_dxMdy0;\
		g_MC##s##_luma [13] = MC_luma##s##_xyqpix;\
		g_MC##s##_luma [14] = MC_luma##s##_yhalf;\
		g_MC##s##_luma [15] = MC_luma##s##_xyqpix;

PUBLIC void H264Dec_init_mc_function (void)
{
    MAKE_LUMA_MC_FUNC(4xN);
    MAKE_LUMA_MC_FUNC(8xN);
    MAKE_LUMA_MC_FUNC(16xN);

    g_MC_chroma8xN = PC_MC_chroma8xN;
    g_MC_chroma4xN = PC_MC_chroma4xN;
    g_MC_chroma2xN = PC_MC_chroma2xN;

}
#undef MAKE_LUMA_MC_FUNC

PUBLIC void H264Dec_init_intra4x4Pred_function (void)
{
    Intra4x4Pred * intraPred4x4 = g_intraPred4x4;

    intraPred4x4 [0] = intra_pred_luma4x4_VERT_PRED;
    intraPred4x4 [1] = intra_pred_luma4x4_HOR_PRED;
    intraPred4x4 [2] = intra_pred_luma4x4_DC_PRED;
    intraPred4x4 [3] = intra_pred_luma4x4_DIAG_DOWN_LEFT_PRED;
    intraPred4x4 [4] = intra_pred_luma4x4_DIAG_DOWN_RIGHT_PRED;
    intraPred4x4 [5] = intra_pred_luma4x4_VERT_RIGHT_PRED;
    intraPred4x4 [6] = intra_pred_luma4x4_HOR_DOWN_PRED;
    intraPred4x4 [7] = intra_pred_luma4x4_VERT_LEFT_PRED;
    intraPred4x4 [8] = intra_pred_luma4x4_HOR_UP_PRED;
}

PUBLIC void H264Dec_init_intra8x8Pred_function (void)
{
    Intra8x8Pred * intraPred8x8 = g_intraPred8x8;

    intraPred8x8 [0] = intra_pred_luma8x8_VERT_PRED;
    intraPred8x8 [1] = intra_pred_luma8x8_HOR_PRED;
    intraPred8x8 [2] = intra_pred_luma8x8_DC_PRED;
    intraPred8x8 [3] = intra_pred_luma8x8_DIAG_DOWN_LEFT_PRED;
    intraPred8x8 [4] = intra_pred_luma8x8_DIAG_DOWN_RIGHT_PRED;
    intraPred8x8 [5] = intra_pred_luma8x8_VERT_RIGHT_PRED;
    intraPred8x8 [6] = intra_pred_luma8x8_HOR_DOWN_PRED;
    intraPred8x8 [7] = intra_pred_luma8x8_VERT_LEFT_PRED;
    intraPred8x8 [8] = intra_pred_luma8x8_HOR_UP_PRED;
}

PUBLIC void H264Dec_init_intra16x16Pred_function (void)
{
    Intra16x16Pred * intraPred16x16 = g_intraPred16x16;

    intraPred16x16 [0] = intraPred_VERT_PRED_16;
    intraPred16x16 [1] = intraPred_HOR_PRED_16;
    intraPred16x16 [2] = intraPred_DC_PRED_16;
    intraPred16x16 [3] = intraPred_PLANE_16;
}

PUBLIC void H264Dec_init_intraChromaPred_function (void)
{
    IntraChromPred * intraChromaPred = g_intraChromaPred;

    intraChromaPred[0] = intraPred_chroma_DC;
    intraChromaPred[1] = intraPred_chroma_Hor;
    intraChromaPred[2] = intraPred_chroma_Ver;
    intraChromaPred[3] = intraPred_chroma_Plane;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
