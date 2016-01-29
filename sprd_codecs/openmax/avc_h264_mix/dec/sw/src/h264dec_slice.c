/******************************************************************************
 ** File Name:    h264dec_slice.c                                             *
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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC int32 get_unit (H264DecContext *vo, uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len)
{
    int32 len = 0;
    uint8 data;
    int32 declen = 0;
    int32 zero_num = 0;
    int32 stuffing_num = 0;
    int32 cur_start_code_len = 0;
    int32 next_start_code_len = 0;
    uint8 *ptr, *buf;
    int32 i;
    DEC_NALU_T *nalu_ptr = vo->g_nalu_ptr;

    ptr = pInStream;
    nalu_ptr->buf = buf = (uint8 *)(((((uint32)ptr)+3)>>2)<<2);
    while ((data = *ptr++) == 0x00) {
        cur_start_code_len++;
    }
    cur_start_code_len++;
    declen += cur_start_code_len;

    //read til next start code, and remove the third start code code emulation byte
    len = 0;
    while ((declen++) < frm_bs_len) {
        data = *ptr++;
        len++;

        if (zero_num < 2) {
            zero_num++;
            if(data != 0) {
                zero_num = 0;
            }
        } else {
            if ((zero_num == 2) && (data == 0x03)) {
                zero_num = 0;
                stuffing_num++;

                continue;
            }

            if ((zero_num >= 2) && (data == 0x1)) {
                next_start_code_len = zero_num+1;
                break;
            }

            if (data == 0) {
                zero_num++;
            } else {
                zero_num = 0;
            }
        }

        *buf++= data;
    }

    nalu_ptr->len = len - stuffing_num - next_start_code_len;
    *slice_unit_len = len + cur_start_code_len - next_start_code_len;

    //remove tailing zero data
    for (i = (nalu_ptr->len-1); (i > 0) && (nalu_ptr->buf[i] == 0); i--) {
        nalu_ptr->len--;
    }

    if (declen >= frm_bs_len) {
        return 1;
    }

    return 0;
}

LOCAL void init_dequant8_coeff_table(H264DecContext *vo) {
    int32 i,j,q,x;
    DEC_PPS_T *pps_ptr = vo->g_active_pps_ptr;

    for(i=0; i<2; i++ ) {
        vo->dequant8_coeff[i] = vo->dequant8_buffer[i];
        for(j=0; j<i; j++) {
            if(!memcmp(pps_ptr->ScalingList8x8[j], pps_ptr->ScalingList8x8[i], 64*sizeof(uint8))) {
                memcpy(vo->dequant8_coeff[i], vo->dequant8_coeff[j],52*64*sizeof(uint32));
                break;
            }
        }
        if(j<i) {
            continue;
        }

        for(q=0; q<52; q++) {
            int32 qpPerRem = g_qpPerRem_tbl[q];
            int32 shift =qpPerRem>>8; //qp_per
            int32 idx = qpPerRem&0xff; //qp_rem
            for(x=0; x<64; x++) {
                vo->dequant8_coeff[i][q][x] = ((int32)dequant8_coeff_init[idx][ dequant8_coeff_init_scan[((x>>1)&12) | (x&3)] ] * pps_ptr->ScalingList8x8[i][x]) << shift;
            }
        }
    }
}

LOCAL void init_dequant4_coeff_table(H264DecContext *vo) {
    int32 i,j,q,x;
    DEC_PPS_T *pps_ptr = vo->g_active_pps_ptr;

    for(i=0; i<6; i++ ) {
        vo->dequant4_coeff[i] = vo->dequant4_buffer[i];
        for(j=0; j<i; j++) {
            if(!memcmp(pps_ptr->ScalingList4x4[j], pps_ptr->ScalingList4x4[i], 16*sizeof(uint8))) {
                memcpy(vo->dequant4_coeff[i], vo->dequant4_buffer[j], 52*16*sizeof(uint32));
                break;
            }
        }
        if(j<i) {
            continue;
        }

        for(q=0; q<52; q++) {
            int32 qpPerRem = g_qpPerRem_tbl[q];
            int32 shift = (qpPerRem>>8)+ 2; //qp_per + 2
            int32 idx = qpPerRem&0xff; //qp_rem
            for(x=0; x<16; x++) {
                vo->dequant4_coeff[i][q][x] = ((int32)dequant4_coeff_init[idx][(x&1) + ((x>>2)&1)] * pps_ptr->ScalingList4x4[i][x]) << shift;
            }
        }
    }
}

LOCAL void init_dequant_tables(H264DecContext *vo) {
    int32 i,x;
    DEC_PPS_T *pps_ptr = vo->g_active_pps_ptr;

    if(!pps_ptr->pic_scaling_matrix_present_flag) {
        memcpy(pps_ptr->ScalingList4x4, vo->g_active_sps_ptr->ScalingList4x4,6*16*sizeof(uint8));
        memcpy(pps_ptr->ScalingList8x8, vo->g_active_sps_ptr->ScalingList8x8,2*64*sizeof(uint8));
    }
    init_dequant4_coeff_table(vo);
    if(pps_ptr->transform_8x8_mode_flag) {
        init_dequant8_coeff_table(vo);
    }
    if(vo->g_active_sps_ptr->qpprime_y_zero_transform_bypass_flag) {
        for(i=0; i<6; i++) {
            for(x=0; x<16; x++) {
                vo->dequant4_coeff[i][0][x] = 1<<6;
            }
        }
        if(pps_ptr->transform_8x8_mode_flag) {
            for(i=0; i<2; i++) {
                for(x=0; x<64; x++) {
                    vo->dequant8_coeff[i][0][x] = 1<<6;
                }
            }
        }
    }
}

PUBLIC int32 H264Dec_process_slice (H264DecContext *vo, DEC_NALU_T *nalu_ptr)
{
    DEC_SLICE_T *curr_slice_ptr = vo->curr_slice_ptr;
    int32 curr_header;
    int32 new_picture;

    vo->idr_flag = (nalu_ptr->nal_unit_type == NALU_TYPE_IDR);
    vo->nal_reference_idc = (nalu_ptr->nal_reference_idc);

    H264Dec_FirstPartOfSliceHeader (curr_slice_ptr, vo);

    /*if picture parameter set id changed, FMO will change, and neighbour 4x4 block
    position infomation(available, position) will change*/
#if 0
    if (img_ptr->g_old_pps_id != curr_slice_ptr->pic_parameter_set_id)
#endif
    {
        vo->g_old_pps_id = curr_slice_ptr->pic_parameter_set_id;

        //use PPS and SPS
        H264Dec_use_parameter_set (vo, curr_slice_ptr->pic_parameter_set_id);

#if _H264_PROTECT_ & _LEVEL_LOW_
        if(vo->error_flag) {
            vo->return_pos1 |= (1<<25);
            return -1;
        }
#endif
        init_dequant_tables(vo);
    }

    H264Dec_RestSliceHeader (vo, curr_slice_ptr);

    if ((H264Dec_FMO_init(vo) != MMDEC_OK) || vo->error_flag) {
#if _H264_PROTECT_ & _LEVEL_LOW_
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos1 |= (1<<26);
#endif
        return -1;
    }

    new_picture = H264Dec_is_new_picture (vo);
    vo->is_new_pic = new_picture;

    if (new_picture) {
        if ((vo->curr_mb_nr>0) && (vo->curr_mb_nr != (vo->frame_size_in_mbs-1))) {
            vo->curr_mb_nr = 0;
            H264Dec_clear_delayed_buffer(vo);
        }

        if ((H264Dec_init_picture (vo) != MMDEC_OK) || vo->error_flag) {
            vo->return_pos1 |= (1<<27);
            return -1;
        }

        curr_header = SOP;
    } else {
        curr_header = SOS;
    }

    H264Dec_init_list (vo, vo->type);
    H264Dec_reorder_list (vo, curr_slice_ptr);

    if (vo->is_cabac) {
        ff_init_cabac_decoder(vo);	//arideco_start_decoding (img_ptr);
    }

    if (!new_picture) {
        DEC_MB_CACHE_T *mb_cache_ptr = vo->g_mb_cache_ptr;
        DEC_STORABLE_PICTURE_T	*dec_picture = vo->g_dec_picture_ptr;
        uint32 offset_y, offset_c;

        vo->curr_mb_nr = curr_slice_ptr->start_mb_nr;
        vo->num_dec_mb = vo->curr_mb_nr;
        vo->mb_x = vo->curr_mb_nr % vo->frame_width_in_mbs;
        vo->mb_y = vo->curr_mb_nr / vo->frame_width_in_mbs;

        offset_y = vo->start_in_frameY +  (vo->mb_y*vo->ext_width + vo->mb_x) * MB_SIZE;
        offset_c = vo->start_in_frameUV + (vo->mb_y*(vo->ext_width>>1) + vo->mb_x) * BLOCK_SIZE;
        mb_cache_ptr->mb_addr[0] = dec_picture->imgYUV[0] + offset_y;
        mb_cache_ptr->mb_addr[1] = dec_picture->imgYUV[1] + offset_c;
        mb_cache_ptr->mb_addr[2] = dec_picture->imgYUV[2] + offset_c;
    }

    return curr_header;
}

PUBLIC void H264Dec_exit_slice (H264DecContext *vo)
{
    DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = vo->g_old_slice_ptr;

    old_slice_ptr->frame_num = vo->frame_num;
    old_slice_ptr->nal_ref_idc = vo->nal_reference_idc;
    old_slice_ptr->pps_id = vo->curr_slice_ptr->pic_parameter_set_id;
    old_slice_ptr->idr_flag = vo->idr_flag;

    if (vo->num_dec_mb == vo->frame_size_in_mbs) {
        old_slice_ptr->frame_num = -1;
    }

    if (vo->idr_flag) {
        old_slice_ptr->idr_pic_id = vo->idr_pic_id;
    }

    if (vo->g_active_sps_ptr->pic_order_cnt_type == 0) {
        old_slice_ptr->pic_order_cnt_lsb = vo->pic_order_cnt_lsb;
        old_slice_ptr->delta_pic_order_cnt_bottom = vo->delta_pic_order_cnt_bottom;
    }

    if (vo->g_active_sps_ptr->pic_order_cnt_type == 1) {
        old_slice_ptr->delta_pic_order_cnt[0] = vo->delta_pic_order_cnt[0];
        old_slice_ptr->delta_pic_order_cnt[1] = vo->delta_pic_order_cnt[1];
    }

    return;
}

#if _DEBUG_
void foo2(void)
{
}
#endif

PUBLIC void set_ref_pic_num(H264DecContext *vo)
{
    int list ,i/*,j*/;
    int slice_id=vo->slice_nr;

    for (list = 0; list < vo->list_count; list++) {
        for (i=0; i<vo->g_list_size[list]; i++) {
            if (!vo->g_list[list][i]) {
                vo->error_flag |= ER_BSM_ID;
                return;
            }
            vo->g_dec_picture_ptr->pic_num_ptr[slice_id][list][i] = vo->g_list[list][i]->poc * 2 ;
        }
    }
}

LOCAL void H264Dec_fill_wp_params (H264DecContext *vo, DEC_PPS_T *active_pps_ptr)
{
    int32 i, j/*, k*/;
    int32 comp;
    int32 log_weight_denom;
    int32 tb, td;
    int32 max_bwd_ref, max_fwd_ref;
    int32 tx,DistScaleFactor;

    if (active_pps_ptr->weighted_bipred_idc == 2) {
        vo->luma_log2_weight_denom = 5;
        vo->chroma_log2_weight_denom = 5;
        vo->wp_round_luma = 16;
        vo->wp_round_chroma = 16;

        for (i = 0; i < MAX_REF_FRAME_NUMBER; i++) {
            for (comp = 0; comp < 3; comp++) {
                log_weight_denom = (comp == 0) ? vo->luma_log2_weight_denom : vo->chroma_log2_weight_denom;
                vo->g_wp_weight[0][i][comp] = 1<<log_weight_denom;
                vo->g_wp_weight[1][i][comp] = 1<<log_weight_denom;
                vo->g_wp_offset[0][i][comp] = 0;
                vo->g_wp_offset[1][i][comp] = 0;
            }
        }
    }

    max_fwd_ref = vo->ref_count[0];
    max_bwd_ref = vo->ref_count[1];

    for (i=0; i<max_fwd_ref; i++) {
        for (j=0; j<max_bwd_ref; j++) {
            for (comp = 0; comp<3; comp++) {
                log_weight_denom = (comp == 0) ? vo->luma_log2_weight_denom : vo->chroma_log2_weight_denom;
                if (active_pps_ptr->weighted_bipred_idc == 1) {
                    vo->g_wbp_weight[0][i][j][comp] =  vo->g_wp_weight[0][i][comp];
                    vo->g_wbp_weight[1][i][j][comp] =  vo->g_wp_weight[1][j][comp];
                } else if (active_pps_ptr->weighted_bipred_idc == 2) {
                    td = Clip3(-128,127,vo->g_list[1][j]->poc - vo->g_list[0][i]->poc);
                    if (td == 0 || vo->g_list[1][j]->is_long_term || vo->g_list[0][i]->is_long_term) {
                        vo->g_wbp_weight[0][i][j][comp] =   32;
                        vo->g_wbp_weight[1][i][j][comp] =   32;
                    } else {
                        tb = Clip3(-128,127,vo->ThisPOC - vo->g_list[0][i]->poc);
                        tx = (16384 + ABS(td/2))/td;
                        DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);

                        vo->g_wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
                        vo->g_wbp_weight[0][i][j][comp] = 64 - vo->g_wbp_weight[1][i][j][comp];
                        if (vo->g_wbp_weight[1][i][j][comp] < -64 || vo->g_wbp_weight[1][i][j][comp] > 128) {
                            vo->g_wbp_weight[0][i][j][comp] = 32;
                            vo->g_wbp_weight[1][i][j][comp] = 32;
                            vo->g_wp_offset[0][i][comp] = 0;
                            vo->g_wp_offset[1][j][comp] = 0;
                        }
                    }
                }
            }
        }
    }
}

PUBLIC void H264Dec_decode_one_slice_I (H264DecContext *vo)
{
    int32 end_of_slice = FALSE;
    DEC_MB_CACHE_T *mb_cache_ptr = vo->g_mb_cache_ptr;
    DEC_MB_INFO_T *curr_mb_info_ptr = vo->mb_info + vo->curr_mb_nr;

    while (!end_of_slice) {
        H264Dec_start_macroblock (vo, curr_mb_info_ptr, mb_cache_ptr);
        H264Dec_read_one_macroblock_ISlice (vo, curr_mb_info_ptr, mb_cache_ptr);

#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (vo->error_flag) {
            return;
        }
#endif
        end_of_slice = H264Dec_exit_macroblock (vo, curr_mb_info_ptr, mb_cache_ptr);
        curr_mb_info_ptr++;
    }

    H264Dec_exit_slice (vo);

    return;
}

PUBLIC void H264Dec_decode_one_slice_P (H264DecContext *vo)
{
    int32 end_of_slice = FALSE;
    DEC_MB_CACHE_T *mb_cache_ptr = vo->g_mb_cache_ptr;
    DEC_MB_INFO_T *curr_mb_info_ptr = vo->mb_info + vo->curr_mb_nr;

    vo->cod_counter = -1;

    set_ref_pic_num(vo);

    while (!end_of_slice) {
        H264Dec_start_macroblock (vo, curr_mb_info_ptr, mb_cache_ptr);
        H264Dec_read_one_macroblock_PSlice (vo, curr_mb_info_ptr, mb_cache_ptr);
        if (vo->error_flag) {
            return;
        }
        end_of_slice = H264Dec_exit_macroblock (vo, curr_mb_info_ptr, mb_cache_ptr);
        curr_mb_info_ptr++;
    }

    H264Dec_exit_slice (vo);

    return;
}

PUBLIC void H264Dec_decode_one_slice_B (H264DecContext *vo)
{
    int32 end_of_slice = FALSE;
    DEC_MB_CACHE_T *mb_cache_ptr = vo->g_mb_cache_ptr;
    DEC_MB_INFO_T *curr_mb_info_ptr = vo->mb_info + vo->curr_mb_nr;

    vo->cod_counter = -1;

    set_ref_pic_num(vo);

    while (!end_of_slice) {
        H264Dec_start_macroblock (vo, curr_mb_info_ptr, mb_cache_ptr);
        H264Dec_read_one_macroblock_BSlice (vo, curr_mb_info_ptr, mb_cache_ptr);
        end_of_slice = H264Dec_exit_macroblock (vo, curr_mb_info_ptr, mb_cache_ptr);

        curr_mb_info_ptr++;
    }

    H264Dec_exit_slice (vo);

    return;
}

/*extend 24 pixel*/
#ifndef _NEON_OPT_
void H264Dec_extent_frame (H264DecContext *vo, DEC_STORABLE_PICTURE_T * dec_picture)
{
    int32 i;
    int32 height, offset, extendWidth;
    uint8 *pSrc1, *pSrc2, *pDst1, *pDst2;
    uint8 **Frame = dec_picture->imgYUV;
#ifdef _NEON_OPT_
    uint8x8_t vec64;
    uint8x16_t vec128;
#endif

//#ifndef _ASM_HOR_EXTENSION_
    int32 width;

    height      = vo->height;
    width       = vo->width;
    extendWidth = vo->ext_width;

    pSrc1 = Frame[0] + vo->start_in_frameY;
    pDst1 = pSrc1 - Y_EXTEND_SIZE;
    pSrc2 = pSrc1 + width - 1;
    pDst2 = pSrc2 + 1;

    /*horizontal repeat Y*/
    for (i = 0; i < height; i++) {
#ifndef _NEON_OPT_
        int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
        int32 *pIntDst = (int32 *)pDst1;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;
        pIntDst[3] = intValue;
        pIntDst[4] = intValue;
        pIntDst[5] = intValue;

        intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
        pIntDst = (int32 *)pDst2;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;
        pIntDst[3] = intValue;
        pIntDst[4] = intValue;
        pIntDst[5] = intValue;

#else
        //left
        vec64 = vld1_lane_u8(pSrc1, vec64, 0);
        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst1, vec128);

        vec64 = vget_low_u8(vec128);
        vst1_u8(pDst1+16, vec64);

        //right
        vec64 = vld1_lane_u8(pSrc2, vec64, 0);
        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst2, vec128);

        vec64 = vget_low_u8(vec128);
        vst1_u8(pDst2+16, vec64);
#endif

        pSrc1 += extendWidth;
        pDst1 += extendWidth;
        pSrc2 += extendWidth;
        pDst2 += extendWidth;
    }

    /*horizontal repeat U*/
    extendWidth = extendWidth>>1;
    pSrc1       = Frame [1] + vo->start_in_frameUV;
    pDst1       = pSrc1 - UV_EXTEND_SIZE;
    pSrc2 = pSrc1 + width / 2 - 1;
    pDst2 = pSrc2 + 1;
    height = height / 2;
    for(i = 0; i < height; i++) {
#if 1//ndef _NEON_OPT_
        int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
        int32 * pIntDst = (int32 *)pDst1;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;

        intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
        pIntDst = (int32 *)pDst2;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;
#else
        //left
        vec64 = vld1_lane_u8(pSrc1, vec64, 0);
        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst1, vec128);

        //right
        vec64 = vld1_lane_u8(pSrc2, vec64, 0);
        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst2, vec128);
#endif

        pSrc1 += extendWidth;
        pDst1 += extendWidth;
        pSrc2 += extendWidth;
        pDst2 += extendWidth;
    }
    /*horizontal repeat V*/
    pSrc1 = Frame [2] + vo->start_in_frameUV;
    pDst1 = pSrc1 - UV_EXTEND_SIZE;
    pSrc2 = pSrc1 + width / 2 - 1;
    pDst2 = pSrc2 + 1;
    for(i = 0; i < height; i++) {
#if 1//ndef _NEON_OPT_
        int32 intValue = ((int32)(*pSrc1) << 24) | ((int32)(*pSrc1) << 16) | ((int32)(*pSrc1) << 8) | (int32)(*pSrc1);
        int32 * pIntDst = (int32 *)pDst1;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;

        intValue = ((int32)(*pSrc2) << 24) | ((int32)(*pSrc2) << 16) | ((int32)(*pSrc2) << 8) | (int32)(*pSrc2);
        pIntDst = (int32 *)pDst2;
        pIntDst[0] = intValue;
        pIntDst[1] = intValue;
        pIntDst[2] = intValue;
#else
        //left
        vec64 = vld1_lane_u8(pSrc1, vec64, 0);
        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst1, vec128);

        //right
        vec64 = vld1_lane_u8(pSrc2, vec64, 0);
        vec128 = vdupq_lane_u8(vec64, 0);
        vst1q_u8(pDst2, vec128);
#endif
        pSrc1 += extendWidth;
        pDst1 += extendWidth;
        pSrc2 += extendWidth;
        pDst2 += extendWidth;
    }
//#else
//	asm_horExtendYUV_h264(Frame, img_ptr->ext_width, img_ptr->height);
//#endif //_ASM_HOR_EXTENSION_

    /*copy first row and last row*/
    /*vertical repeat Y*/
    height = vo->height;
    extendWidth  = vo->ext_width;
    offset = extendWidth * Y_EXTEND_SIZE;
    pSrc1  = Frame[0] + offset;
    pDst1  = Frame[0];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < Y_EXTEND_SIZE; i++) {
        memcpy(pDst1, pSrc1, extendWidth);
        memcpy(pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }
#if 1
    /*vertical repeat U*/
    height = height / 2;
    extendWidth  = extendWidth / 2;
    offset = extendWidth * UV_EXTEND_SIZE;
    pSrc1  = Frame[1] + offset;
    pDst1  = Frame[1];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < UV_EXTEND_SIZE; i++) {
        memcpy(pDst1, pSrc1, extendWidth);
        memcpy(pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }

    /*vertical repeat V*/
    pSrc1  = Frame[2] + offset;
    pDst1  = Frame[2];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < UV_EXTEND_SIZE; i++)  {
        memcpy(pDst1, pSrc1, extendWidth);
        memcpy(pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }
#endif

}
#endif

PUBLIC void H264Dec_write_disp_frame (H264DecContext *vo, DEC_STORABLE_PICTURE_T * dec_picture)
{
    int32 height      = vo->height;
    int32 width       = vo->width;
    int32 extendWidth = vo->ext_width;
    int32 offset;
    uint8 *pSrc_y, *pSrc_u, *pSrc_v;
    uint8 *pDst_y, *pDst_u, *pDst_v;
    int32 row;

    //y
    pSrc_y = dec_picture->imgYUV[0] + vo->start_in_frameY;
    pDst_y = dec_picture->imgY;

    for (row = 0; row < height; row++) {
        memcpy(pDst_y, pSrc_y, width);

        pDst_y += width;
        pSrc_y += extendWidth;
    }

    //u and v
    height >>= 1;
    width >>= 1;
    extendWidth >>= 1;
    offset = vo->start_in_frameUV;

    pSrc_u = dec_picture->imgYUV[1] + offset;
    pSrc_v = dec_picture->imgYUV[2] + offset;
    pDst_u = dec_picture->imgU;
    pDst_v = dec_picture->imgV;

    if (vo->yuv_format != YUV420SP_NV12 && vo->yuv_format != YUV420SP_NV21) {
        for (row = 0; row < height; row++) {
            memcpy(pDst_u, pSrc_u, width);
            memcpy(pDst_v, pSrc_v, width);

            pDst_u += width;
            pDst_v += width;
            pSrc_u += extendWidth;
            pSrc_v += extendWidth;
        }
    } else {
        uint8 *pDst = dec_picture->imgU;
        int32 col;

        if (vo->yuv_format == YUV420SP_NV21) {//YYYY vuvu should exchange pSrc_u & pSrc_v
            uint8 *pTemp;
            pTemp   =  pSrc_u;
            pSrc_u  =  pSrc_v;
            pSrc_v   =  pTemp;
        }
#ifndef _NEON_OPT_
        for (row = 0; row < height; row++) {
            for (col = 0; col < width; col++) {
                pDst[col*2+0] = pSrc_u[col];
                pDst[col*2+1] = pSrc_v[col];
            }

            pDst += width*2;
            pSrc_u += extendWidth;
            pSrc_v += extendWidth;
        }
#else
        uint8x8_t u_vec64, v_vec64;
        uint16x8_t u_vec128, v_vec128;
        uint16x8_t uv_vec128;

        for (row = 0; row < height; row++) {
            for (col = 0; col < width; col+=8) {
                u_vec64 = vld1_u8(pSrc_u);
                v_vec64 = vld1_u8(pSrc_v);

                u_vec128 = vmovl_u8(u_vec64);
                v_vec128 = vmovl_u8(v_vec64);

                uv_vec128 = vsliq_n_u16(u_vec128, v_vec128, 8);

                vst1q_u16((uint16*)pDst, uv_vec128);

                pSrc_u += 8;
                pSrc_v += 8;
                pDst += 16;
            }
            pSrc_u += (UV_EXTEND_SIZE*2);
            pSrc_v += (UV_EXTEND_SIZE*2);
        }
#endif
    }
}

PUBLIC void H264Dec_find_smallest_pts(H264DecContext *vo, DEC_STORABLE_PICTURE_T *out)
{
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = vo->g_dpb_ptr;
    uint32 j;

    for(j = 0; j < dpb_ptr->delayed_pic_num ; j++) {
        if(dpb_ptr->delayed_pic[j]->nTimeStamp < out->nTimeStamp) {
            uint64 nTimeStamp;

            SPRD_CODEC_LOGD ("%s, [delay time_stamp: %lld], [Cur time_stamp: %lld]\n", __FUNCTION__, dpb_ptr->delayed_pic[j]->nTimeStamp, out->nTimeStamp);

            nTimeStamp = dpb_ptr->delayed_pic[j]->nTimeStamp;
            dpb_ptr->delayed_pic[j]->nTimeStamp = out->nTimeStamp;
            out->nTimeStamp = nTimeStamp;
        }
    }
}

LOCAL void H264Dec_output_one_frame (H264DecContext *vo, MMDecOutput * dec_out)
{
    DEC_VUI_T *vui_seq_parameters_ptr = vo->g_sps_ptr->vui_seq_parameters;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr= vo->g_dpb_ptr;
    DEC_STORABLE_PICTURE_T *prev = dpb_ptr->delayed_pic_ptr;
    DEC_STORABLE_PICTURE_T *cur = vo->g_dec_picture_ptr;
    DEC_STORABLE_PICTURE_T *out = cur;

    if (vo->yuv_format == YUV420SP_NV12 || vo->yuv_format == YUV420SP_NV21) {
        DEC_FRAME_STORE_T *fs = NULL;
        uint32 i, pics, cross_idr, out_of_order, out_idx;

        if(vui_seq_parameters_ptr->bitstream_restriction_flag && (vo->has_b_frames < vui_seq_parameters_ptr->num_reorder_frames)) {
            vo->has_b_frames = vui_seq_parameters_ptr->num_reorder_frames;
//		s->low_delay = 0;
        }

        SCI_TRACE_LOW_DPB ("dec poc: %d\t", cur->poc);

        dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = cur;

        if ((fs = H264Dec_search_frame_from_dpb(vo, cur)) != NULL) {
            if(fs->is_reference == 0) {
                fs->is_reference = DELAYED_PIC_REF;
                H264DEC_BIND_FRAME(vo, fs->frame);
            }
        }

        cross_idr = 0;
        for(i=0; i < dpb_ptr->delayed_pic_num; i++) {
            if(dpb_ptr->delayed_pic[i]->idr_flag || (dpb_ptr->delayed_pic[i]->poc == 0)) {
                cross_idr = 1;
            }
        }

        //find the smallest POC frame in dpb buffer
        out = dpb_ptr->delayed_pic[0];
        out_idx = 0;
        for(i=1; (i< MAX_DELAYED_PIC_NUM) && dpb_ptr->delayed_pic[i] && !dpb_ptr->delayed_pic[i]->idr_flag; i++) {
            if(dpb_ptr->delayed_pic[i]->poc < out->poc) {
                out = dpb_ptr->delayed_pic[i];
                out_idx = i;
            }
        }

        pics = dpb_ptr->delayed_pic_num;
        out_of_order = !cross_idr && prev && (out->poc < prev->poc);
        if(vui_seq_parameters_ptr->bitstream_restriction_flag && vo->has_b_frames >= vui_seq_parameters_ptr->num_reorder_frames) {
        } else if(prev && pics <= vo->has_b_frames) {
            out = prev;
        } else if((out_of_order && (pics-1) == vo->has_b_frames && pics < 15/*why 15?, xwluo@20120316 */)  ||
                  ((vo->g_sps_ptr->profile_idc != 0x42/*!bp*/)&&(vo->low_delay) && ((!cross_idr && prev && out->poc > (prev->poc + 2)) || cur->slice_type == B_SLICE))) {
            SCI_TRACE_LOW_DPB("%s,  %d",  __FUNCTION__, __LINE__);

            vo->low_delay = 0;
            vo->has_b_frames++;
            out = prev;
        } else if(out_of_order) {
            out = prev;
        }

        if (out != cur) {
            dpb_ptr->delayed_pic_ptr = out;
            dec_out->frameEffective = (prev == out) ? 0 : 1;

            //flush one frame from dpb and re-organize the delayed_pic buffer
            if(/*out_of_order ||*/ pics > vo->has_b_frames || dec_out->frameEffective) {
                int j;

                out_idx = dpb_ptr->delayed_pic_num;
                for(j = 0; j < dpb_ptr->delayed_pic_num ; j++) {
                    if(dpb_ptr->delayed_pic[j] == out) {
                        out_idx = j;
                        SCI_TRACE_LOW_DPB ("delayed_pic_num : %d, out_idx: %d,\t",  dpb_ptr->delayed_pic_num, out_idx);
                        for(i = out_idx; dpb_ptr->delayed_pic[i]; i++) {
                            dpb_ptr->delayed_pic[i] = dpb_ptr->delayed_pic[i+1];
                        }
                        dpb_ptr->delayed_pic_num--;
                        break;
                    }
                }
            }
        }

        dec_out->reqNewBuf = 1;
        dec_out->pts = 0;
        if (dec_out->frameEffective) {
            H264Dec_find_smallest_pts(vo, out);

            dec_out->frame_width = vo->width;
            dec_out->frame_height = vo->height;
            dec_out->pOutFrameY = out->imgY;
            dec_out->pOutFrameU = out->imgU;
            dec_out->pOutFrameV = out->imgV;
            dec_out->pBufferHeader = out->pBufferHeader;
            dec_out->mPicId = out->mPicId;
            dec_out->pts = out->nTimeStamp;

            fs = H264Dec_search_frame_from_dpb(vo, out);
            if (fs && (fs->is_reference == DELAYED_PIC_REF)) {
                fs->is_reference = 0;
                H264DEC_UNBIND_FRAME(vo, fs->frame);
            }
        }

        SCI_TRACE_LOW_DPB("out poc: %d, effective: %d\t", out->poc, dec_out->frameEffective);

#if 0   //only for debug
        {
            int32 list_size0 = img_ptr->g_list_size[0];
            int32 list_size1 = img_ptr->g_list_size[1];
            SCI_TRACE_LOW_DPB("list_size: (%d, %d), total: %d", list_size0, list_size1, list_size0 + list_size1);

            for (i = 0; i < (MAX_REF_FRAME_NUMBER+1); i++)
            {
                if(dpb_ptr->fs[i]->is_reference)
                {
                    SCI_TRACE_LOW_DPB("dpb poc: %d, %0x,is ref %d,", dpb_ptr->fs[i]->poc, dpb_ptr->fs[i]->frame->pBufferHeader,dpb_ptr->fs[i]->is_reference );
                }
            }

            for (i = 0; i <  dpb_ptr->delayed_pic_num; i++)
            {
                SCI_TRACE_LOW_DPB("delay poc: %d, %0x", dpb_ptr->delayed_pic[i]->poc, dpb_ptr->delayed_pic[i]->pBufferHeader);
            }
        }
#endif
    } else {	//only for thumbnail,
        dec_out->reqNewBuf = 1;
        dec_out->frameEffective = 1;
        dec_out->frame_width = vo->width;
        dec_out->frame_height = vo->height;
        dec_out->pOutFrameY = cur->imgY;
        dec_out->pOutFrameU = cur->imgU;
        dec_out->pOutFrameV = cur->imgV;
        dec_out->pBufferHeader = cur->pBufferHeader;
        dec_out->mPicId = cur->mPicId;
    }

    return;
}

PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, H264DecContext *vo)
{
    DEC_PPS_T *active_pps_ptr = vo->g_active_pps_ptr;
    DEC_SLICE_T *curr_slice_ptr = vo->curr_slice_ptr;
    MMDecRet ret = MMDEC_ERROR;

    if (vo->is_cabac) {
        init_contexts (vo);
        cabac_new_slice(vo);
    }

//	if ((active_pps_ptr->weighted_bipred_idc > 0 && (img_ptr->type == B_SLICE)) || (active_pps_ptr->weighted_pred_flag && img_ptr->type != I_SLICE))
    if ((vo->type == B_SLICE) && ((active_pps_ptr->weighted_bipred_idc > 0) || (active_pps_ptr->weighted_pred_flag))) {
        H264Dec_fill_wp_params (vo, active_pps_ptr);
    }

    vo->apply_weights = ((active_pps_ptr->weighted_pred_flag && (curr_slice_ptr->picture_type == P_SLICE ) )
                         || ((active_pps_ptr->weighted_bipred_idc > 0 ) && (curr_slice_ptr->picture_type == B_SLICE)));

    if (curr_slice_ptr->picture_type == I_SLICE) {
        vo->list_count = 0;
        H264Dec_decode_one_slice_I (vo);
    } else if (curr_slice_ptr->picture_type == P_SLICE) {
        vo->list_count = 1;
        H264Dec_decode_one_slice_P (vo);
    } else if (curr_slice_ptr->picture_type == B_SLICE) {
#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (vo->g_nFrame_dec_h264 < 2 || vo->g_searching_IDR_pic) {
            vo->error_flag |= ER_REF_FRM_ID;
            vo->return_pos1 = (1<<30);
            return MMDEC_ERROR;
        }
#endif
        vo->list_count = 2;

        if (!vo->direct_spatial_mv_pred_flag) {
            int32 iref_max = mmin(vo->ref_count[0], vo->g_list_size[0]);
            const int32 poc = vo->g_dec_picture_ptr->poc;
            const int32 poc1 = vo->g_list[1][0]->poc;
            int32 i;

            for (i = 0; i < iref_max; i++) {
                int32 prescale, iTRb, iTRp;
                int32 poc0 = vo->g_list[0][i]->poc;

                iTRp = Clip3( -128, 127,  (poc1 - poc0));

                if (iTRp!=0) {
                    iTRb = Clip3( -128, 127, (poc - poc0 ));
                    prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;
                    vo->dist_scale_factor[i] = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
                } else {
                    vo->dist_scale_factor[i] = 9999;
                }
            }
        }

        H264Dec_decode_one_slice_B (vo);
    } else {
#if _H264_PROTECT_ & _LEVEL_LOW_
        vo->error_flag |= ER_BSM_ID;
        vo->return_pos2 = (1<<7);
#endif

        SPRD_CODEC_LOGE ("the other picture type is not supported!\n");
    }

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (vo->error_flag) {
        SPRD_CODEC_LOGE ("%s: mb_x: %d, mb_y: %d, bit_cnt: %d, pos: %x, pos1: %0x, pos2: %0x, err_flag: %x\n",
                         __FUNCTION__, vo->mb_x, vo->mb_y, vo->bitstrm_ptr->bitsCnt - vo->bitstrm_ptr->bitsAligned,
                         vo->return_pos, vo->return_pos1, vo->return_pos2, vo->error_flag);
        vo->return_pos2 = (1<<8);
        vo->g_searching_IDR_pic = 1;
        return MMDEC_ERROR;
    }
#endif

    vo->slice_nr++;

    if (SOP == curr_slice_ptr->next_header) {//end of picture
        H264Dec_exit_picture (vo);
        H264Dec_output_one_frame(vo,dec_output_ptr);

        vo->g_dec_picture_ptr = NULL;
        vo->g_nFrame_dec_h264++;
    }

    return ret;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
