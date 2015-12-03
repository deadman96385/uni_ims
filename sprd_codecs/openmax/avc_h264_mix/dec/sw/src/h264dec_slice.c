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
#if 1//WIN32
PUBLIC int32 get_unit (H264DecContext *img_ptr, uint8 *pInStream, int32 frm_bs_len, int32 *slice_unit_len)
{
    int32 len = 0;
    uint8 *ptr;
    uint8 data = 0;
    /*static*/ int32 declen = 0;
//	static int32 s_bFisrtUnit = TRUE;
    int32 zero_num = 0;
    int32 startCode_len = 0;
    int32 stuffing_num = 0;
// 	uint8 *bfr = g_nalu_ptr->buf = pInStream;
    int32 *stream_ptr;// = (int32 *)bfr;
    int32 code;
    uint_32or64 byte_rest;

    ptr = pInStream;

//	SCI_TRACE_LOW("get_unit 0, frm_bs_len %d, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x, %0x\n",frm_bs_len,
//		ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]);

    //start code
    while ((data = *ptr++) == 0x00)
    {
        len++;
    }
    len++;
    declen += len;
//	g_nalu_ptr->buf += (*start_code_len);

    byte_rest = (uint_32or64)pInStream;
    byte_rest = ((byte_rest)>>2)<<2;	//word aligned

    //destuffing
    stream_ptr = /*(int32 *)bfr =*/ (int32 *)(byte_rest);
    img_ptr->g_nalu_ptr->buf = (uint8 *)(byte_rest);
//	len = 0;

    //read til next start code, and remove the third start code code emulation byte
    byte_rest = 4;
    declen = frm_bs_len - len;
    //while (declen < frm_bs_len)
    do
    {
        data = *ptr++;
//		len++;
//		declen++;

        if (zero_num < 2)
        {
            //*bfr++ = data;
            zero_num++;
            byte_rest--;
            if (byte_rest >= 0)
            {
                code = (code <<8) | data;	//big endian
            }
            if (0 == byte_rest)
            {
                byte_rest = 4;
                *stream_ptr++ = code;
            }

            if(data != 0)
            {
                zero_num = 0;
            }
        } else
        {
            if ((zero_num == 2) && (data == 0x03))
            {
                zero_num = 0;
                stuffing_num++;
                goto next_data;
            } else
            {
                //*bfr++ = data;
                byte_rest--;
                if (byte_rest >= 0)
                {
                    code = (code<<8) | data;	//big endian
                }
                if (0 == byte_rest)
                {
                    byte_rest = 4;
                    *stream_ptr++ = code;
                }

                if (data == 0x1)
                {
                    //	if (zero_num >= 2)
                    {
                        startCode_len = zero_num + 1;
                        break;
                    }
                } else if (data == 0x00)
                {
                    zero_num++;
                } else
                {
                    zero_num = 0;
                }
            }
        }
next_data:
        declen--;
    } while(declen);

    if (((uint_32or64)stream_ptr) == (((((uint_32or64)ptr) - startCode_len/*len*/) >> 2) << 2))
    {
        img_ptr->g_need_back_last_word = 1;
        img_ptr->g_back_last_word = *stream_ptr;
    } else
    {
        img_ptr->g_need_back_last_word = 0;
    }

    *stream_ptr++ = code << (byte_rest*8);

    if (declen == 0)
    {
        declen = 1;
    }
    declen = frm_bs_len - declen;
    declen++;
    len = declen - len;

    *slice_unit_len = (declen - startCode_len /*+ stuffing_num*/);

    img_ptr->g_nalu_ptr->len = len - startCode_len - stuffing_num;

    while (code && !(code&0xff))
    {
        img_ptr->g_nalu_ptr->len--;
        code >>= 8;

//		SCI_TRACE_LOW("code: %0x, nal->len: %d", code, g_nalu_ptr->len);
    }
    declen -= startCode_len;

    if (declen >= frm_bs_len)
    {
//		declen = 0;
//		s_bFisrtUnit = TRUE;
        return 1;
    }

    return 0;
}
#endif

//#define DUMP_H264_ES
#if 0
PUBLIC int32 get_unit_avc1 (uint8 *pInStream, int32 slice_unit_len)
{
    int32 len = 0;
    uint8 *ptr;
    uint8 data;
//	static int32 declen = 0;
//	static int32 s_bFisrtUnit = TRUE;
    int32 zero_num = 0;
    int32 startCode_len = 0;
    int32 stuffing_num = 0;
    uint8 *bfr = g_nalu_ptr->buf = pInStream;

    ptr = pInStream;

//	SCI_TRACE_LOW("get_unit_avc1 0, frm_bs_len %d, %0x, %0x, %0x, %0x\n",slice_unit_len,  bfr[0], bfr[1], bfr[2], bfr[3]);

    //read til next start code, and remove the third start code code emulation byte
    while (len < slice_unit_len)
    {
        data = *ptr++;
        len++;

        if (zero_num < 2)
        {
            *bfr++ = data;
            zero_num++;
            if(data != 0)
            {
                zero_num = 0;
            }
        } else
        {
#ifndef DUMP_H264_ES
            if ((zero_num == 2) && (data == 0x03))
            {
                zero_num = 0;
                stuffing_num++;
                continue;
            } else
#endif
            {
                *bfr++ = data;

                if (data == 0x1)
                {
                    if (zero_num >= 2)
                    {
                        startCode_len = zero_num + 1;
                        break;
                    }
                } else if (data == 0x00)
                {
                    zero_num++;
                } else
                {
                    zero_num = 0;
                }
            }
        }
    }

//	SCI_TRACE_LOW("get_unit_avc1 1, len %d, stuffing_num %d\n", len, stuffing_num);

    g_nalu_ptr->len = len -  stuffing_num;

    return 0;
}
#endif

LOCAL void init_dequant8_coeff_table(H264DecContext *img_ptr) {
    int32 i,j,q,x;
    DEC_PPS_T	*pps_ptr = img_ptr->g_active_pps_ptr;

    for(i=0; i<2; i++ ) {
        img_ptr->dequant8_coeff[i] = img_ptr->dequant8_buffer[i];
        for(j=0; j<i; j++) {
            if(!memcmp(pps_ptr->ScalingList8x8[j], pps_ptr->ScalingList8x8[i], 64*sizeof(uint8))) {
                memcpy(img_ptr->dequant8_coeff[i], img_ptr->dequant8_coeff[j],52*64*sizeof(uint32));
                break;
            }
        }
        if(j<i)
            continue;

        for(q=0; q<52; q++) {
            int32 qpPerRem = g_qpPerRem_tbl[q];
            int32 shift =qpPerRem>>8; //qp_per
            int32 idx = qpPerRem&0xff; //qp_rem
            for(x=0; x<64; x++)
                img_ptr->dequant8_coeff[i][q][x] = ((int32)dequant8_coeff_init[idx][ dequant8_coeff_init_scan[((x>>1)&12) | (x&3)] ] * pps_ptr->ScalingList8x8[i][x]) << shift;
        }
    }
}

LOCAL void init_dequant4_coeff_table(H264DecContext *img_ptr) {
    int32 i,j,q,x;
    DEC_PPS_T	*pps_ptr = img_ptr->g_active_pps_ptr;

    for(i=0; i<6; i++ ) {
        img_ptr->dequant4_coeff[i] = img_ptr->dequant4_buffer[i];
        for(j=0; j<i; j++) {
            if(!memcmp(pps_ptr->ScalingList4x4[j], pps_ptr->ScalingList4x4[i], 16*sizeof(uint8))) {
                memcpy(img_ptr->dequant4_coeff[i], img_ptr->dequant4_buffer[j], 52*16*sizeof(uint32));
                break;
            }
        }
        if(j<i)
            continue;

        for(q=0; q<52; q++) {
            int32 qpPerRem = g_qpPerRem_tbl[q];
            int32 shift = (qpPerRem>>8)+ 2; //qp_per + 2
            int32 idx = qpPerRem&0xff; //qp_rem
            for(x=0; x<16; x++)
                img_ptr->dequant4_coeff[i][q][x] = ((int32)dequant4_coeff_init[idx][(x&1) + ((x>>2)&1)] * pps_ptr->ScalingList4x4[i][x]) << shift;
        }
    }
}

LOCAL void init_dequant_tables(H264DecContext *img_ptr) {
    int32 i,x;
    DEC_PPS_T	*pps_ptr = img_ptr->g_active_pps_ptr;

    if(!pps_ptr->pic_scaling_matrix_present_flag)
    {
        memcpy(pps_ptr->ScalingList4x4, img_ptr->g_active_sps_ptr->ScalingList4x4,6*16*sizeof(uint8));
        memcpy(pps_ptr->ScalingList8x8, img_ptr->g_active_sps_ptr->ScalingList8x8,2*64*sizeof(uint8));
    }
    init_dequant4_coeff_table(img_ptr);
    if(pps_ptr->transform_8x8_mode_flag) {
        init_dequant8_coeff_table(img_ptr);
    }
    if(img_ptr->g_active_sps_ptr->qpprime_y_zero_transform_bypass_flag) {
        for(i=0; i<6; i++) {
            for(x=0; x<16; x++) {
                img_ptr->dequant4_coeff[i][0][x] = 1<<6;
            }
        }
        if(pps_ptr->transform_8x8_mode_flag) {
            for(i=0; i<2; i++) {
                for(x=0; x<64; x++) {
                    img_ptr->dequant8_coeff[i][0][x] = 1<<6;
                }
            }
        }
    }
}

PUBLIC int32 H264Dec_process_slice (H264DecContext *img_ptr, DEC_NALU_T *nalu_ptr)
{
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    int32 curr_header;
    int32 new_picture;

    img_ptr->idr_flag = (nalu_ptr->nal_unit_type == NALU_TYPE_IDR);
    img_ptr->nal_reference_idc = (nalu_ptr->nal_reference_idc);

    H264Dec_FirstPartOfSliceHeader (curr_slice_ptr, img_ptr);

    /*if picture parameter set id changed, FMO will change, and neighbour 4x4 block
    position infomation(available, position) will change*/
#if 0
    if (img_ptr->g_old_pps_id != curr_slice_ptr->pic_parameter_set_id)
#endif
    {
        img_ptr->g_old_pps_id = curr_slice_ptr->pic_parameter_set_id;

        //use PPS and SPS
        H264Dec_use_parameter_set (img_ptr, curr_slice_ptr->pic_parameter_set_id);

#if _H264_PROTECT_ & _LEVEL_LOW_
        if(img_ptr->error_flag)
        {
            img_ptr->return_pos1 |= (1<<25);
            return -1;
        }
#endif
        init_dequant_tables(img_ptr);
    }

    H264Dec_RestSliceHeader (img_ptr, curr_slice_ptr);

    if ((H264Dec_FMO_init(img_ptr) != MMDEC_OK) || img_ptr->error_flag)
    {
#if _H264_PROTECT_ & _LEVEL_LOW_
        img_ptr->error_flag |= ER_BSM_ID;
        img_ptr->return_pos1 |= (1<<26);
#endif
        return -1;
    }

    new_picture = H264Dec_is_new_picture (img_ptr);
    img_ptr->is_new_pic = new_picture;

    if (new_picture)
    {
        if ((img_ptr->curr_mb_nr>0) && (img_ptr->curr_mb_nr != (img_ptr->frame_size_in_mbs-1)))
        {
            img_ptr->curr_mb_nr = 0;
            H264Dec_clear_delayed_buffer(img_ptr);
        }

        if ((H264Dec_init_picture (img_ptr) != MMDEC_OK) || img_ptr->error_flag)
        {
            img_ptr->return_pos1 |= (1<<27);
            return -1;
        }

        curr_header = SOP;
    } else
    {
        curr_header = SOS;
    }

    H264Dec_init_list (img_ptr, img_ptr->type);
    H264Dec_reorder_list (img_ptr, curr_slice_ptr);

    if (img_ptr->is_cabac)
    {
        ff_init_cabac_decoder(img_ptr);	//arideco_start_decoding (img_ptr);
    }

    if (!new_picture)
    {
        DEC_MB_CACHE_T *mb_cache_ptr = img_ptr->g_mb_cache_ptr;
        DEC_STORABLE_PICTURE_T	*dec_picture = img_ptr->g_dec_picture_ptr;
        uint32 offset_y, offset_c;

        img_ptr->curr_mb_nr = curr_slice_ptr->start_mb_nr;
        img_ptr->num_dec_mb = img_ptr->curr_mb_nr;
        img_ptr->mb_x = img_ptr->curr_mb_nr % img_ptr->frame_width_in_mbs;
        img_ptr->mb_y = img_ptr->curr_mb_nr / img_ptr->frame_width_in_mbs;

        offset_y = img_ptr->start_in_frameY +  (img_ptr->mb_y*img_ptr->ext_width + img_ptr->mb_x) * MB_SIZE;
        offset_c = img_ptr->start_in_frameUV + (img_ptr->mb_y*(img_ptr->ext_width>>1) + img_ptr->mb_x) * BLOCK_SIZE;
        mb_cache_ptr->mb_addr[0] = dec_picture->imgYUV[0] + offset_y;
        mb_cache_ptr->mb_addr[1] = dec_picture->imgYUV[1] + offset_c;
        mb_cache_ptr->mb_addr[2] = dec_picture->imgYUV[2] + offset_c;
    }

    return curr_header;
}

PUBLIC void H264Dec_exit_slice (H264DecContext *img_ptr)
{
    DEC_OLD_SLICE_PARAMS_T *old_slice_ptr = img_ptr->g_old_slice_ptr;

    old_slice_ptr->frame_num = img_ptr->frame_num;
    old_slice_ptr->nal_ref_idc = img_ptr->nal_reference_idc;
    old_slice_ptr->pps_id = img_ptr->curr_slice_ptr->pic_parameter_set_id;
    old_slice_ptr->idr_flag = img_ptr->idr_flag;

    if (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)
    {
        old_slice_ptr->frame_num = -1;
    }

    if (img_ptr->idr_flag)
    {
        old_slice_ptr->idr_pic_id = img_ptr->idr_pic_id;
    }

    if (img_ptr->g_active_sps_ptr->pic_order_cnt_type == 0)
    {
        old_slice_ptr->pic_order_cnt_lsb = img_ptr->pic_order_cnt_lsb;
        old_slice_ptr->delta_pic_order_cnt_bottom = img_ptr->delta_pic_order_cnt_bottom;
    }

    if (img_ptr->g_active_sps_ptr->pic_order_cnt_type == 1)
    {
        old_slice_ptr->delta_pic_order_cnt[0] = img_ptr->delta_pic_order_cnt[0];
        old_slice_ptr->delta_pic_order_cnt[1] = img_ptr->delta_pic_order_cnt[1];
    }

    return;
}

#if _DEBUG_
void foo2(void)
{
}
#endif

PUBLIC void set_ref_pic_num(H264DecContext *img_ptr)
{
    int list ,i/*,j*/;
    int slice_id=img_ptr->slice_nr;

    for (list = 0; list < img_ptr->list_count; list++)
    {
        for (i=0; i<img_ptr->g_list_size[list]; i++)
        {
            if (!img_ptr->g_list[list][i])
            {
                img_ptr->error_flag |= ER_BSM_ID;
                return;
            }
            img_ptr->g_dec_picture_ptr->pic_num_ptr		  [slice_id][list][i] = img_ptr->g_list[list][i]->poc * 2 ;
        }
    }
}

LOCAL void H264Dec_fill_wp_params (H264DecContext *img_ptr, DEC_PPS_T *active_pps_ptr)
{
    int32 i, j/*, k*/;
    int32 comp;
    int32 log_weight_denom;
    int32 tb, td;
//  	int32 bframe = (img_ptr->type==B_SLICE);
    int32 max_bwd_ref, max_fwd_ref;
    int32 tx,DistScaleFactor;

    if (active_pps_ptr->weighted_bipred_idc == 2)
    {
        img_ptr->luma_log2_weight_denom = 5;
        img_ptr->chroma_log2_weight_denom = 5;
        img_ptr->wp_round_luma = 16;
        img_ptr->wp_round_chroma = 16;

        for (i = 0; i < MAX_REF_FRAME_NUMBER; i++)
        {
            for (comp = 0; comp < 3; comp++)
            {
                log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
                img_ptr->g_wp_weight[0][i][comp] = 1<<log_weight_denom;
                img_ptr->g_wp_weight[1][i][comp] = 1<<log_weight_denom;
                img_ptr->g_wp_offset[0][i][comp] = 0;
                img_ptr->g_wp_offset[1][i][comp] = 0;
            }
        }
    }

    max_fwd_ref = img_ptr->ref_count[0];
    max_bwd_ref = img_ptr->ref_count[1];

    for (i=0; i<max_fwd_ref; i++)
    {
        for (j=0; j<max_bwd_ref; j++)
        {
            for (comp = 0; comp<3; comp++)
            {
                log_weight_denom = (comp == 0) ? img_ptr->luma_log2_weight_denom : img_ptr->chroma_log2_weight_denom;
                if (active_pps_ptr->weighted_bipred_idc == 1)
                {
                    img_ptr->g_wbp_weight[0][i][j][comp] =  img_ptr->g_wp_weight[0][i][comp];
                    img_ptr->g_wbp_weight[1][i][j][comp] =  img_ptr->g_wp_weight[1][j][comp];
                } else if (active_pps_ptr->weighted_bipred_idc == 2)
                {
                    td = Clip3(-128,127,img_ptr->g_list[1][j]->poc - img_ptr->g_list[0][i]->poc);
                    if (td == 0 || img_ptr->g_list[1][j]->is_long_term || img_ptr->g_list[0][i]->is_long_term)
                    {
                        img_ptr->g_wbp_weight[0][i][j][comp] =   32;
                        img_ptr->g_wbp_weight[1][i][j][comp] =   32;
                    } else
                    {
                        tb = Clip3(-128,127,img_ptr->ThisPOC - img_ptr->g_list[0][i]->poc);
                        tx = (16384 + ABS(td/2))/td;
                        DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);

                        img_ptr->g_wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
                        img_ptr->g_wbp_weight[0][i][j][comp] = 64 - img_ptr->g_wbp_weight[1][i][j][comp];
                        if (img_ptr->g_wbp_weight[1][i][j][comp] < -64 || img_ptr->g_wbp_weight[1][i][j][comp] > 128)
                        {
                            img_ptr->g_wbp_weight[0][i][j][comp] = 32;
                            img_ptr->g_wbp_weight[1][i][j][comp] = 32;
                            img_ptr->g_wp_offset[0][i][comp] = 0;
                            img_ptr->g_wp_offset[1][j][comp] = 0;
                        }
                    }
                }
            }
        }
    }
}

PUBLIC void H264Dec_decode_one_slice_I (H264DecContext *img_ptr)
{
    int32 end_of_slice = FALSE;
    DEC_MB_CACHE_T *mb_cache_ptr = img_ptr->g_mb_cache_ptr;
    DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

    while (!end_of_slice)
    {
        H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        H264Dec_read_one_macroblock_ISlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (img_ptr->error_flag)
        {
            return;
        }
#endif
        end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        curr_mb_info_ptr++;
    }

    H264Dec_exit_slice (img_ptr);

    return;
}

PUBLIC void H264Dec_decode_one_slice_P (H264DecContext *img_ptr)
{
    int32 end_of_slice = FALSE;
    DEC_MB_CACHE_T *mb_cache_ptr = img_ptr->g_mb_cache_ptr;
    DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

    img_ptr->cod_counter = -1;

    set_ref_pic_num(img_ptr);

    while (!end_of_slice)
    {
        H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        H264Dec_read_one_macroblock_PSlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        if (img_ptr->error_flag)
        {
            return;
        }
        end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        curr_mb_info_ptr++;
    }

    H264Dec_exit_slice (img_ptr);

    return;
}

PUBLIC void H264Dec_decode_one_slice_B (H264DecContext *img_ptr)
{
    int32 end_of_slice = FALSE;
    DEC_MB_CACHE_T *mb_cache_ptr = img_ptr->g_mb_cache_ptr;
    DEC_MB_INFO_T *curr_mb_info_ptr = img_ptr->mb_info + img_ptr->curr_mb_nr;

    img_ptr->cod_counter = -1;

    set_ref_pic_num(img_ptr);

    while (!end_of_slice)
    {
        H264Dec_start_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        H264Dec_read_one_macroblock_BSlice (img_ptr, curr_mb_info_ptr, mb_cache_ptr);
        end_of_slice = H264Dec_exit_macroblock (img_ptr, curr_mb_info_ptr, mb_cache_ptr);

        curr_mb_info_ptr++;
    }

    H264Dec_exit_slice (img_ptr);

    return;
}

/*extend 24 pixel*/
#if 1//WIN32
void H264Dec_extent_frame (H264DecContext *img_ptr, DEC_STORABLE_PICTURE_T * dec_picture)
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

    height      = img_ptr->height;
    width       = img_ptr->width;
    extendWidth = img_ptr->ext_width;

    pSrc1 = Frame[0] + img_ptr->start_in_frameY;
    pDst1 = pSrc1 - Y_EXTEND_SIZE;
    pSrc2 = pSrc1 + width - 1;
    pDst2 = pSrc2 + 1;

    /*horizontal repeat Y*/
    for (i = 0; i < height; i++)
    {
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
    pSrc1       = Frame [1] + img_ptr->start_in_frameUV;
    pDst1       = pSrc1 - UV_EXTEND_SIZE;
    pSrc2 = pSrc1 + width / 2 - 1;
    pDst2 = pSrc2 + 1;
    height = height / 2;
    for(i = 0; i < height; i++)
    {
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
    pSrc1 = Frame [2] + img_ptr->start_in_frameUV;
    pDst1 = pSrc1 - UV_EXTEND_SIZE;
    pSrc2 = pSrc1 + width / 2 - 1;
    pDst2 = pSrc2 + 1;
    for(i = 0; i < height; i++)
    {
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
    height = img_ptr->height;
    extendWidth  = img_ptr->ext_width;
    offset = extendWidth * Y_EXTEND_SIZE;
    pSrc1  = Frame[0] + offset;
    pDst1  = Frame[0];
    pSrc2  = pSrc1 + extendWidth * (height - 1);
    pDst2  = pSrc2 + extendWidth;

    for(i = 0; i < Y_EXTEND_SIZE; i++)
    {
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

    for(i = 0; i < UV_EXTEND_SIZE; i++)
    {
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

    for(i = 0; i < UV_EXTEND_SIZE; i++)
    {
        memcpy(pDst1, pSrc1, extendWidth);
        memcpy(pDst2, pSrc2, extendWidth);
        pDst1 += extendWidth;
        pDst2 += extendWidth;
    }
#endif

}
#endif

PUBLIC void H264Dec_write_disp_frame (H264DecContext *img_ptr, DEC_STORABLE_PICTURE_T * dec_picture)
{
    int32 height      = img_ptr->height;
    int32 width       = img_ptr->width;
    int32 extendWidth = img_ptr->ext_width;
    int32 offset;
    uint8 *pSrc_y, *pSrc_u, *pSrc_v;
    uint8 *pDst_y, *pDst_u, *pDst_v;
    int32 row;

    //y
    pSrc_y = dec_picture->imgYUV[0] + img_ptr->start_in_frameY;
    pDst_y = dec_picture->imgY;

    for (row = 0; row < height; row++)
    {
        memcpy(pDst_y, pSrc_y, width);

        pDst_y += width;
        pSrc_y += extendWidth;
    }

    //u and v
    height >>= 1;
    width >>= 1;
    extendWidth >>= 1;
    offset = img_ptr->start_in_frameUV;

    pSrc_u = dec_picture->imgYUV[1] + offset;
    pSrc_v = dec_picture->imgYUV[2] + offset;
    pDst_u = dec_picture->imgU;
    pDst_v = dec_picture->imgV;

    if (img_ptr->yuv_format != YUV420SP_NV12 && img_ptr->yuv_format != YUV420SP_NV21)
    {
        for (row = 0; row < height; row++)
        {
            memcpy(pDst_u, pSrc_u, width);
            memcpy(pDst_v, pSrc_v, width);

            pDst_u += width;
            pDst_v += width;
            pSrc_u += extendWidth;
            pSrc_v += extendWidth;
        }
    } else
    {
        uint8 *pDst = dec_picture->imgU;
        int32 col;

        if (img_ptr->yuv_format == YUV420SP_NV21) //YYYY vuvu should exchange pSrc_u & pSrc_v
        {
            uint8 *pTemp;
            pTemp   =  pSrc_u;
            pSrc_u  =  pSrc_v;
            pSrc_v   =  pTemp;
        }
#ifndef _NEON_OPT_
        for (row = 0; row < height; row++)
        {
            for (col = 0; col < width; col++)
            {
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

        for (row = 0; row < height; row++)
        {
            for (col = 0; col < width; col+=8)
            {
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

PUBLIC void H264Dec_find_smallest_pts(H264DecContext *img_ptr, DEC_STORABLE_PICTURE_T *out)
{
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr = img_ptr->g_dpb_ptr;
    uint32 j;

    for(j = 0; j < dpb_ptr->delayed_pic_num ; j++)
    {
        if(dpb_ptr->delayed_pic[j]->nTimeStamp < out->nTimeStamp)
        {
            uint64 nTimeStamp;

            SPRD_CODEC_LOGD ("%s, [delay time_stamp: %lld], [Cur time_stamp: %lld]", __FUNCTION__, dpb_ptr->delayed_pic[j]->nTimeStamp, out->nTimeStamp);

            nTimeStamp = dpb_ptr->delayed_pic[j]->nTimeStamp;
            dpb_ptr->delayed_pic[j]->nTimeStamp = out->nTimeStamp;
            out->nTimeStamp = nTimeStamp;
        }
    }
}

LOCAL void H264Dec_output_one_frame (H264DecContext *img_ptr, MMDecOutput * dec_out)
{
    DEC_VUI_T *vui_seq_parameters_ptr = img_ptr->g_sps_ptr->vui_seq_parameters;
    DEC_DECODED_PICTURE_BUFFER_T *dpb_ptr= img_ptr->g_dpb_ptr;
    DEC_STORABLE_PICTURE_T *prev = dpb_ptr->delayed_pic_ptr;
    DEC_STORABLE_PICTURE_T *cur = img_ptr->g_dec_picture_ptr;
    DEC_STORABLE_PICTURE_T *out = cur;

    if (img_ptr->yuv_format == YUV420SP_NV12 || img_ptr->yuv_format == YUV420SP_NV21)
    {
        DEC_FRAME_STORE_T *fs = NULL;
        uint32 i, pics, cross_idr, out_of_order, out_idx;

        if(vui_seq_parameters_ptr->bitstream_restriction_flag && (img_ptr->has_b_frames < vui_seq_parameters_ptr->num_reorder_frames))
        {
            img_ptr->has_b_frames = vui_seq_parameters_ptr->num_reorder_frames;
//		s->low_delay = 0;
        }

        SPRD_CODEC_LOGI ("dec poc: %d\t", cur->poc);

        dpb_ptr->delayed_pic[dpb_ptr->delayed_pic_num++] = cur;

        if ((fs = H264Dec_search_frame_from_dpb(img_ptr, cur)) != NULL)
        {
            if(fs->is_reference == 0)
            {
                fs->is_reference = DELAYED_PIC_REF;
                H264DEC_BIND_FRAME(img_ptr, fs->frame);
            }
        }

        cross_idr = 0;
        for(i=0; i < dpb_ptr->delayed_pic_num; i++)
        {
            if(dpb_ptr->delayed_pic[i]->idr_flag || (dpb_ptr->delayed_pic[i]->poc == 0))
            {
                cross_idr = 1;
            }
        }

        //find the smallest POC frame in dpb buffer
        out = dpb_ptr->delayed_pic[0];
        out_idx = 0;
        for(i=1; (i< MAX_DELAYED_PIC_NUM) && dpb_ptr->delayed_pic[i] && !dpb_ptr->delayed_pic[i]->idr_flag; i++)
        {
            if(dpb_ptr->delayed_pic[i]->poc < out->poc)
            {
                out = dpb_ptr->delayed_pic[i];
                out_idx = i;
            }
        }

        pics = dpb_ptr->delayed_pic_num;
        out_of_order = !cross_idr && prev && (out->poc < prev->poc);
        if(vui_seq_parameters_ptr->bitstream_restriction_flag && img_ptr->has_b_frames >= vui_seq_parameters_ptr->num_reorder_frames)
        {
        } else if(prev && pics <= img_ptr->has_b_frames)
        {
            out = prev;
        } else if((out_of_order && (pics-1) == img_ptr->has_b_frames && pics < 15/*why 15?, xwluo@20120316 */)  ||
                  ((img_ptr->g_sps_ptr->profile_idc != 0x42/*!bp*/)&&(img_ptr->low_delay) && ((!cross_idr && prev && out->poc > (prev->poc + 2)) || cur->slice_type == B_SLICE)))
        {
            img_ptr->low_delay = 0;
            img_ptr->has_b_frames++;
            out = prev;
        } else if(out_of_order)
        {
            out = prev;
        }

        if (out != cur)
        {
            dpb_ptr->delayed_pic_ptr = out;
            dec_out->frameEffective = (prev == out) ? 0 : 1;

            //flush one frame from dpb and re-organize the delayed_pic buffer
            if(/*out_of_order ||*/ pics > img_ptr->has_b_frames || dec_out->frameEffective)
            {
                int j;

                out_idx = dpb_ptr->delayed_pic_num;
                for(j = 0; j < dpb_ptr->delayed_pic_num ; j++)
                {
                    if(dpb_ptr->delayed_pic[j] == out)
                    {
                        out_idx = j;
                        SPRD_CODEC_LOGD ("delayed_pic_num : %d, out_idx: %d,\t",  dpb_ptr->delayed_pic_num, out_idx);
                        for(i = out_idx; dpb_ptr->delayed_pic[i]; i++)
                        {
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
        if (dec_out->frameEffective)
        {
            H264Dec_find_smallest_pts(img_ptr, out);

            dec_out->frame_width = img_ptr->width;
            dec_out->frame_height = img_ptr->height;
            dec_out->pOutFrameY = out->imgY;
            dec_out->pOutFrameU = out->imgU;
            dec_out->pOutFrameV = out->imgV;
            dec_out->pBufferHeader = out->pBufferHeader;
            dec_out->mPicId = out->mPicId;
            dec_out->pts = out->nTimeStamp;

            fs = H264Dec_search_frame_from_dpb(img_ptr, out);
            if (fs && (fs->is_reference == DELAYED_PIC_REF))
            {
                fs->is_reference = 0;
                H264DEC_UNBIND_FRAME(img_ptr, fs->frame);
            }

//		SCI_TRACE_LOW("out poc: %d\t", out->poc);
        } else
        {
//		SCI_TRACE_LOW("out poc: %d\n", out->poc);
        }

#if 0   //only for debug
        {
            int32 list_size0 = img_ptr->g_list_size[0];
            int32 list_size1 = img_ptr->g_list_size[1];
            SCI_TRACE_LOW("list_size: (%d, %d), total: %d", list_size0, list_size1, list_size0 + list_size1);

            for (i = 0; i < (MAX_REF_FRAME_NUMBER+1); i++)
            {
                if(dpb_ptr->fs[i]->is_reference)
                {
                    SCI_TRACE_LOW("dpb poc: %d,   %0x,is ref %d,", dpb_ptr->fs[i]->poc, dpb_ptr->fs[i]->frame->pBufferHeader,dpb_ptr->fs[i]->is_reference );
                }
            }

            for (i = 0; i <  dpb_ptr->delayed_pic_num; i++)
            {
                SCI_TRACE_LOW("delay poc: %d, %0x", dpb_ptr->delayed_pic[i]->poc, dpb_ptr->delayed_pic[i]->pBufferHeader);
            }
        }
#endif
    } else	//only for thumbnail,
    {
        dec_out->reqNewBuf = 1;
        dec_out->frameEffective = 1;
        dec_out->frame_width = img_ptr->width;
        dec_out->frame_height = img_ptr->height;
        dec_out->pOutFrameY = cur->imgY;
        dec_out->pOutFrameU = cur->imgU;
        dec_out->pOutFrameV = cur->imgV;
        dec_out->pBufferHeader = cur->pBufferHeader;
        dec_out->mPicId = cur->mPicId;
    }

    return;
}

PUBLIC MMDecRet H264Dec_decode_one_slice_data (MMDecOutput *dec_output_ptr, H264DecContext *img_ptr)
{
    DEC_PPS_T	*active_pps_ptr = img_ptr->g_active_pps_ptr;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    MMDecRet ret = MMDEC_ERROR;

    if (img_ptr->is_cabac)
    {
        init_contexts (img_ptr);
        cabac_new_slice(img_ptr);
    }

//	if ((active_pps_ptr->weighted_bipred_idc > 0 && (img_ptr->type == B_SLICE)) || (active_pps_ptr->weighted_pred_flag && img_ptr->type != I_SLICE))
    if ((img_ptr->type == B_SLICE) && ((active_pps_ptr->weighted_bipred_idc > 0) || (active_pps_ptr->weighted_pred_flag)))
    {
        H264Dec_fill_wp_params (img_ptr, active_pps_ptr);
    }

    img_ptr->apply_weights = ((active_pps_ptr->weighted_pred_flag && (curr_slice_ptr->picture_type == P_SLICE ) )
                              || ((active_pps_ptr->weighted_bipred_idc > 0 ) && (curr_slice_ptr->picture_type == B_SLICE)));

    if (curr_slice_ptr->picture_type == I_SLICE)
    {
        img_ptr->list_count = 0;
        H264Dec_decode_one_slice_I (img_ptr);
    } else if (curr_slice_ptr->picture_type == P_SLICE)
    {
        img_ptr->list_count = 1;
        H264Dec_decode_one_slice_P (img_ptr);
    } else if (curr_slice_ptr->picture_type == B_SLICE)
    {
#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (img_ptr->g_nFrame_dec_h264 < 2 || img_ptr->g_searching_IDR_pic)
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos1 = (1<<30);
            return MMDEC_ERROR;
        }
#endif
        img_ptr->list_count = 2;

        if (!img_ptr->direct_spatial_mv_pred_flag)
        {
            int32 iref_max = mmin(img_ptr->ref_count[0], img_ptr->g_list_size[0]);
            const int32 poc = img_ptr->g_dec_picture_ptr->poc;
            const int32 poc1 = img_ptr->g_list[1][0]->poc;
            int32 i;

            for (i = 0; i < iref_max; i++)
            {
                int32 prescale, iTRb, iTRp;
                int32 poc0 = img_ptr->g_list[0][i]->poc;

                iTRp = Clip3( -128, 127,  (poc1 - poc0));

                if (iTRp!=0)
                {
                    iTRb = Clip3( -128, 127, (poc - poc0 ));
                    prescale = ( 16384 + ABS( iTRp / 2 ) ) / iTRp;
                    img_ptr->dist_scale_factor[i] = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
                } else
                {
                    img_ptr->dist_scale_factor[i] = 9999;
                }
            }
        }

        H264Dec_decode_one_slice_B (img_ptr);
    } else
    {
#if _H264_PROTECT_ & _LEVEL_LOW_
        img_ptr->error_flag |= ER_BSM_ID;
        img_ptr->return_pos2 = (1<<7);
#endif

        SPRD_CODEC_LOGE ("the other picture type is not supported!\n");
    }

#if _H264_PROTECT_ & _LEVEL_HIGH_
    if (img_ptr->error_flag)
    {
        SPRD_CODEC_LOGE ("H264Dec_decode_one_slice_data: mb_x: %d, mb_y: %d, bit_cnt: %d, pos: %x, pos1: %0x, pos2: %0x, err_flag: %x\n",
                         img_ptr->mb_x, img_ptr->mb_y, img_ptr->bitstrm_ptr->bitcnt, img_ptr->return_pos, img_ptr->return_pos1, img_ptr->return_pos2, img_ptr->error_flag);
        img_ptr->return_pos2 = (1<<8);
        img_ptr->g_searching_IDR_pic = 1;
        return MMDEC_ERROR;
    }
#endif

    img_ptr->slice_nr++;

    if (SOP == curr_slice_ptr->next_header) //end of picture
    {
        H264Dec_exit_picture (img_ptr);
        H264Dec_output_one_frame(img_ptr,dec_output_ptr);

        img_ptr->g_dec_picture_ptr = NULL;
        img_ptr->g_nFrame_dec_h264++;
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
