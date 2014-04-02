/******************************************************************************
 ** File Name:    h264dec_mb.c			                                      *
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
//for mv and mvd
LOCAL void fill_cache_templet_int16 (int32 *src_ptr, int32 *dst_cache, DEC_MB_CACHE_T *mb_cache_ptr, int32 b4_pitch)
{
    //clear by 0.
#ifndef _NEON_OPT_
    dst_cache[16] = dst_cache[17] = dst_cache[18] = dst_cache[19] =
                                        dst_cache[28] = dst_cache[29] = dst_cache[30] = dst_cache[31] =
                                                dst_cache[40] = dst_cache[41] = dst_cache[42] = dst_cache[43] =
                                                        dst_cache[52] = dst_cache[53] = dst_cache[54] = dst_cache[55] = 0;
#else
    int32x4_t v32x4_zero = vmovq_n_s32(0);

    vst1q_s32(dst_cache+16, v32x4_zero);
    vst1q_s32(dst_cache+28, v32x4_zero);
    vst1q_s32(dst_cache+40, v32x4_zero);
    vst1q_s32(dst_cache+52, v32x4_zero);
#endif

    //fill top
    if (mb_cache_ptr->mb_avail_b)
    {
        int32 *top_src_ptr = src_ptr - b4_pitch;
#ifndef _NEON_OPT_
        dst_cache[4] = top_src_ptr[0];
        dst_cache[5] = top_src_ptr[1];
        dst_cache[6] = top_src_ptr[2];
        dst_cache[7] = top_src_ptr[3];
#else
        int32x4_t v32x4 = vld1q_s32(top_src_ptr);
        vst1q_s32(dst_cache+4, v32x4);
#endif
    } else
    {
#ifndef _NEON_OPT_
        dst_cache[4] = dst_cache[5] = dst_cache[6] = dst_cache[7] = 0;
#else
        vst1q_s32(dst_cache+4, v32x4_zero);
#endif
    }

    //fill left
    if (mb_cache_ptr->mb_avail_a)
    {
        dst_cache[15] = src_ptr[b4_pitch*0 - 1];
        dst_cache[27] = src_ptr[b4_pitch*1 - 1];
        dst_cache[39] = src_ptr[b4_pitch*2 - 1];
        dst_cache[51] = src_ptr[b4_pitch*3 - 1];
    } else
    {
        dst_cache[15] = dst_cache[27] =
                            dst_cache[39] = dst_cache[51] = 0;
    }
}

//for direct, ref_idx
LOCAL void fill_cache_templet_int8 (int8 *src_ptr, int8 *dst_cache, DEC_MB_CACHE_T *mb_cache_ptr, int32 b4_pitch)
{
    if (!mb_cache_ptr->is_direct)
    {
        //clear by 0.
        ((int32 *)dst_cache)[4]  =
            ((int32 *)dst_cache)[7]  =
                ((int32 *)dst_cache)[10] =
                    ((int32 *)dst_cache)[13] = 0;
    }

    //fill top
    if (mb_cache_ptr->mb_avail_b)
    {
        ((int32 *)dst_cache)[1] = ((int32 *)(src_ptr - b4_pitch))[0];
    } else
    {
        ((int32 *)dst_cache)[1] = 0x0;
    }

    //fill left
    if (mb_cache_ptr->mb_avail_a)
    {
        dst_cache[CTX_CACHE_WIDTH_X1 + 3] = src_ptr[b4_pitch*0 - 1];
        dst_cache[CTX_CACHE_WIDTH_X2 + 3] = src_ptr[b4_pitch*1 - 1];
        dst_cache[CTX_CACHE_WIDTH_X3 + 3] = src_ptr[b4_pitch*2 - 1];
        dst_cache[CTX_CACHE_WIDTH_X4 + 3] = src_ptr[b4_pitch*3 - 1];
    } else
    {
        dst_cache[CTX_CACHE_WIDTH_X1 + 3] =
            dst_cache[CTX_CACHE_WIDTH_X2 + 3] =
                dst_cache[CTX_CACHE_WIDTH_X3 + 3] =
                    dst_cache[CTX_CACHE_WIDTH_X4 + 3] = 0;
    }
}

void fill_cache (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 list;
    int32 b4_pitch = img_ptr->b4_pitch;
    int8 *nnz_ptr, *nnz_cache;
    int32 *mv_ptr, *mv_cache;
    int32 *mvd_ptr, *mvd_cache;
    int8 *ref_idx_ptr, *ref_idx_cache;

    //nnz_cache
    nnz_ptr = img_ptr->nnz_ptr + img_ptr->b4_offset_nnz;
    nnz_cache = mb_cache_ptr->nnz_cache;
    ((int32 *)nnz_cache)[4]  = ((int32 *)nnz_cache)[7]  =
                                   ((int32 *)nnz_cache)[10] = ((int32 *)nnz_cache)[13] = 0;//y
    ((int16 *)nnz_cache)[38] = ((int16 *)nnz_cache)[44] = 0;//u
    ((int16 *)nnz_cache)[40] = ((int16 *)nnz_cache)[46] = 0;//v

    //top nnz reference
    if (mb_cache_ptr->mb_avail_b)
    {
        ((int32 *)nnz_cache)[1]  = ((int32 *)(nnz_ptr - b4_pitch*3))[0];//y
        ((int16 *)nnz_cache)[32] = ((int16 *)(nnz_ptr - b4_pitch))[0]; //cb
        ((int16 *)nnz_cache)[34] = ((int16 *)(nnz_ptr - b4_pitch))[1]; //cr
    } else/*set nnz_ref not available*/
    {
        ((int32 *)nnz_cache)[1]	= 0x40404040;//y
        ((int16 *)nnz_cache)[32] = 0x4040;	//cb
        ((int16 *)nnz_cache)[34] = 0x4040;	//cr
    }

    //left nnz ref
    if (mb_cache_ptr->mb_avail_a)
    {
        nnz_cache[CTX_CACHE_WIDTH_X1 + 3] = nnz_ptr[b4_pitch*0 - 1];//y
        nnz_cache[CTX_CACHE_WIDTH_X2 + 3] = nnz_ptr[b4_pitch*1 - 1];
        nnz_cache[CTX_CACHE_WIDTH_X3 + 3] = nnz_ptr[b4_pitch*2 - 1];
        nnz_cache[CTX_CACHE_WIDTH_X4 + 3] = nnz_ptr[b4_pitch*3 - 1];

        nnz_cache[CTX_CACHE_WIDTH_X6 + 3] = nnz_ptr[b4_pitch*4 - 3];//cb
        nnz_cache[CTX_CACHE_WIDTH_X7 + 3] = nnz_ptr[b4_pitch*5 - 3];

        nnz_cache[CTX_CACHE_WIDTH_X6 + 7] = nnz_ptr[b4_pitch*4 - 1];//cr
        nnz_cache[CTX_CACHE_WIDTH_X7 + 7] = nnz_ptr[b4_pitch*5 - 1];
    } else/*set nnz_ref not available*/
    {
        nnz_cache[CTX_CACHE_WIDTH_X1 + 3] = 	//Y
            nnz_cache[CTX_CACHE_WIDTH_X2 + 3] =
                nnz_cache[CTX_CACHE_WIDTH_X3 + 3] =
                    nnz_cache[CTX_CACHE_WIDTH_X4 + 3] = 0x40;

        nnz_cache[CTX_CACHE_WIDTH_X6 + 3] = 0x40;//cb
        nnz_cache[CTX_CACHE_WIDTH_X7 + 3] = 0x40;

        nnz_cache[CTX_CACHE_WIDTH_X6 + 7] = 0x40;//cr
        nnz_cache[CTX_CACHE_WIDTH_X7 + 7] = 0x40;
    }

    for (list = 0; list < img_ptr->list_count; list++)
    {
        mv_ptr = (int32 *)(img_ptr->g_dec_picture_ptr->mv_ptr[list]) + img_ptr->b4_offset;
        mv_cache = (int32 *)(mb_cache_ptr->mv_cache[list]);
        fill_cache_templet_int16(mv_ptr, mv_cache, mb_cache_ptr, b4_pitch);
        //fill top-right
        if (mb_cache_ptr->mb_avail_c)
        {
            mv_cache[8] = mv_ptr[4 - b4_pitch];
        } else
        {
            mv_cache[8] = 0;
        }
        //fill top-left
        if (mb_cache_ptr->mb_avail_d)
        {
            mv_cache[3] = mv_ptr[-1 - b4_pitch];
        } else
        {
            mv_cache[3] = 0;
        }

        mvd_ptr = (int32 *)(img_ptr->mvd_ptr[list]) + img_ptr->b4_offset;
        mvd_cache = (int32 *)(mb_cache_ptr->mvd_cache[list]);
        fill_cache_templet_int16(mvd_ptr, mvd_cache, mb_cache_ptr, b4_pitch);

        ref_idx_ptr = img_ptr->g_dec_picture_ptr->ref_idx_ptr[list] + img_ptr->b4_offset;
        ref_idx_cache = mb_cache_ptr->ref_idx_cache[list];
        //	fill_cache_templet_int8(ref_idx_ptr, ref_idx_cache, mb_cache_ptr, b4_pitch);
        if (mb_info_ptr->is_intra)
        {   //clear by -1.
            ((int32 *)ref_idx_cache)[4]  = ((int32 *)ref_idx_cache)[7]  =
                                               ((int32 *)ref_idx_cache)[10] = ((int32 *)ref_idx_cache)[13] = -1;

        } else
        {   //clear by 0.
            ((int32 *)ref_idx_cache)[4]  = ((int32 *)ref_idx_cache)[7]  =
                                               ((int32 *)ref_idx_cache)[10] = ((int32 *)ref_idx_cache)[13] = 0;
        }

        //fill top
        if (mb_cache_ptr->mb_avail_b)
        {
            ((int32 *)ref_idx_cache)[1] = ((int32 *)(ref_idx_ptr - b4_pitch))[0];
        } else
        {
            ((int32 *)ref_idx_cache)[1] = 0xfefefefe;
        }

        //fill left
        if (mb_cache_ptr->mb_avail_a)
        {
            ref_idx_cache[CTX_CACHE_WIDTH_X1 + 3] = ref_idx_ptr[b4_pitch*0 - 1];
            ref_idx_cache[CTX_CACHE_WIDTH_X2 + 3] = ref_idx_ptr[b4_pitch*1 - 1];
            ref_idx_cache[CTX_CACHE_WIDTH_X3 + 3] = ref_idx_ptr[b4_pitch*2 - 1];
            ref_idx_cache[CTX_CACHE_WIDTH_X4 + 3] = ref_idx_ptr[b4_pitch*3 - 1];
        } else
        {
            ref_idx_cache[CTX_CACHE_WIDTH_X1 + 3] =
                ref_idx_cache[CTX_CACHE_WIDTH_X2 + 3] =
                    ref_idx_cache[CTX_CACHE_WIDTH_X3 + 3] =
                        ref_idx_cache[CTX_CACHE_WIDTH_X4 + 3] = -2;
        }

        //fill top-right
        if (mb_cache_ptr->mb_avail_c)
        {
            ref_idx_cache[8] = ref_idx_ptr[4 - b4_pitch];
        } else
        {
            ref_idx_cache[8] = -2;
        }

        //fill top-left
        if (mb_cache_ptr->mb_avail_d)
        {
            ref_idx_cache[3] = ref_idx_ptr[-1 - b4_pitch];
        } else
        {
            ref_idx_cache[3] = -2;
        }

        //fill right
        ref_idx_cache[20] =
            ref_idx_cache[32] =
                ref_idx_cache[44] =
                    ref_idx_cache[56] = -2;
    }

    if ( img_ptr->is_cabac)
    {
        //direct cache
        if (img_ptr->type == B_SLICE)
        {
            int8 *direct_ptr = img_ptr->direct_ptr + img_ptr->b4_offset;
            int8 *direct_cache = mb_cache_ptr->direct_cache;
            fill_cache_templet_int8(direct_ptr, direct_cache, mb_cache_ptr, b4_pitch);
        }

        //mvd
        for (list = 0; list < img_ptr->list_count; list++)
        {
            int32 *mvd_ptr = (int32*)(img_ptr->mvd_ptr[list]) + img_ptr->b4_offset;
            int32 *mvd_cache = (int32*)(mb_cache_ptr->mvd_cache[list]);
            fill_cache_templet_int16(mvd_ptr, mvd_cache, mb_cache_ptr, b4_pitch);
        }
    }
}

LOCAL void H264Dec_store_mc_info (H264DecContext *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    DEC_STORABLE_PICTURE_T *pic = img_ptr->g_dec_picture_ptr;
    int32 b4_offset= img_ptr->b4_offset;
    int32 b4_pitch = img_ptr->b4_pitch;
    int32 list, row;
#ifdef _NEON_OPT_
    int32x4_t v32x4;
#endif

    for (list = 0; list < img_ptr->list_count; list++)
    {
        int8  *ref_idx_cache = mb_cache_ptr->ref_idx_cache[list] + CTX_CACHE_WIDTH_PLUS4;
        int32 *mv_cache = (int32*)(mb_cache_ptr->mv_cache[list]) + CTX_CACHE_WIDTH_PLUS4;
        int32 *src_ref_pic_num_ptr = pic->pic_num_ptr[img_ptr->slice_nr][list];

        int8 *ref_idx_ptr = pic->ref_idx_ptr[list] + b4_offset;
        int32 *ref_pic_id_ptr = pic->ref_pic_id_ptr[list] + b4_offset;
        int32 *mv_ptr = (int32*)(pic->mv_ptr[list]) + b4_offset;

        for (row = 4; row > 0; row--)
        {
            ((int32 *)ref_idx_ptr)[0] = ((int32 *)ref_idx_cache)[0];

            ref_pic_id_ptr[0] = src_ref_pic_num_ptr[ref_idx_cache[0]];
            ref_pic_id_ptr[1] = src_ref_pic_num_ptr[ref_idx_cache[1]];
            ref_pic_id_ptr[2] = src_ref_pic_num_ptr[ref_idx_cache[2]];
            ref_pic_id_ptr[3] = src_ref_pic_num_ptr[ref_idx_cache[3]];
#ifndef _NEON_OPT_
            mv_ptr[0] = mv_cache[0];
            mv_ptr[1] = mv_cache[1];
            mv_ptr[2] = mv_cache[2];
            mv_ptr[3] = mv_cache[3];
#else
            v32x4 = vld1q_s32(mv_cache);
            vst1q_s32(mv_ptr, v32x4);
#endif

            ref_idx_ptr += b4_pitch;
            ref_pic_id_ptr += b4_pitch;
            mv_ptr += b4_pitch;
            ref_idx_cache += CTX_CACHE_WIDTH;
            mv_cache += CTX_CACHE_WIDTH;
        }
    }
}

void write_back_cache (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 i, list;
    int32 b4_pitch = img_ptr->b4_pitch;
    int32 pitch = b4_pitch >> 2;
    int8 *i4_pred_mode_ptr, *i4_pred_mode_cache;
    int8 *nnz_ptr, *nnz_cache;

    //nnz cache
    nnz_ptr = img_ptr->nnz_ptr + img_ptr->b4_offset_nnz;
    nnz_cache = mb_cache_ptr->nnz_cache;
    ((int32 *)nnz_ptr)[0*pitch] = ((int32 *)nnz_cache)[4];
    ((int32 *)nnz_ptr)[1*pitch] = ((int32 *)nnz_cache)[7];
    ((int32 *)nnz_ptr)[2*pitch] = ((int32 *)nnz_cache)[10];
    ((int32 *)nnz_ptr)[3*pitch] = ((int32 *)nnz_cache)[13];
    nnz_ptr += b4_pitch*4;//y

    ((int16 *)nnz_ptr)[0] = ((int16 *)nnz_cache)[38];
    ((int16 *)nnz_ptr)[1] = ((int16 *)nnz_cache)[40];
    nnz_ptr += b4_pitch;

    ((int16 *)nnz_ptr)[0] = ((int16 *)nnz_cache)[44];
    ((int16 *)nnz_ptr)[1] = ((int16 *)nnz_cache)[46];

    //i4x4_pred_mode_cache
    if (mb_info_ptr->is_intra)
    {
        i4_pred_mode_ptr = img_ptr->i4x4pred_mode_ptr + img_ptr->b4_offset;
        i4_pred_mode_cache = mb_cache_ptr->i4x4pred_mode_cache;

        ((int32 *)i4_pred_mode_ptr)[0*pitch] = ((int32 *)i4_pred_mode_cache)[4];
        ((int32 *)i4_pred_mode_ptr)[1*pitch] = ((int32 *)i4_pred_mode_cache)[7];
        ((int32 *)i4_pred_mode_ptr)[2*pitch] = ((int32 *)i4_pred_mode_cache)[10];
        ((int32 *)i4_pred_mode_ptr)[3*pitch] = ((int32 *)i4_pred_mode_cache)[13];
    }

    //lsw for direct mode
    H264Dec_store_mc_info(img_ptr, mb_cache_ptr);

    if (img_ptr->is_cabac)
    {
        //direct cache
        if (img_ptr->type == B_SLICE)
        {
            int8 *direct_ptr = img_ptr->direct_ptr + img_ptr->b4_offset;
            int8 *direct_cache = mb_cache_ptr->direct_cache;

            ((int32 *)direct_ptr)[0*pitch] = ((int32 *)direct_cache)[4];
            ((int32 *)direct_ptr)[1*pitch] = ((int32 *)direct_cache)[7];
            ((int32 *)direct_ptr)[2*pitch] = ((int32 *)direct_cache)[10];
            ((int32 *)direct_ptr)[3*pitch] = ((int32 *)direct_cache)[13];
        }

        //mvd cache
        for (list = 0; list < img_ptr->list_count; list++)
        {
            int32 *mvd_ptr = (int32 *)(img_ptr->mvd_ptr[list]) + img_ptr->b4_offset;
            int32 *mvd_cache = (int32 *)(mb_cache_ptr->mvd_cache[list]) + CTX_CACHE_WIDTH_PLUS4;

            for (i = 4; i > 0; i--)
            {
#ifndef _NEON_OPT_
                mvd_ptr[0] = mvd_cache[0];
                mvd_ptr[1] = mvd_cache[1];
                mvd_ptr[2] = mvd_cache[2];
                mvd_ptr[3] = mvd_cache[3];
#else
                int32x4_t v32x4 = vld1q_s32(mvd_cache);
                vst1q_s32(mvd_ptr, v32x4);
#endif
                mvd_ptr += b4_pitch;
                mvd_cache += CTX_CACHE_WIDTH;
            }
        }
    }

    return;
}

PUBLIC void H264Dec_start_macroblock (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 mb_x = img_ptr->mb_x;
    int32 mb_y = img_ptr->mb_y;
    int32 frame_width_in_mbs = img_ptr->frame_width_in_mbs;
    int32 curr_mb_slice_nr =img_ptr->slice_nr;

    img_ptr->slice_nr_ptr[img_ptr->curr_mb_nr] = curr_mb_slice_nr;
    mb_info_ptr->is_intra = FALSE;
    mb_info_ptr->qp = img_ptr->qp;
    mb_info_ptr->skip_flag = 0;
    mb_info_ptr->c_ipred_mode = 0;
    mb_info_ptr->cbp = 0;
    mb_info_ptr->transform_size_8x8_flag = 0;

    mb_cache_ptr->is_skipped = FALSE;
    ((int32 *)(mb_cache_ptr->b8mode))[0] = 0;
    ((int32 *)(mb_cache_ptr->b8pdir))[0] = 0;
    ((int32 *)(mb_cache_ptr->mbc_b8pdir))[0] = 0;

    mb_cache_ptr->cbp_uv = 0;
    mb_cache_ptr->is_direct = 0;
    mb_cache_ptr->noSubMbPartSizeLess8x8 = TRUE;

    img_ptr->abv_mb_info = mb_info_ptr - frame_width_in_mbs;
    mb_cache_ptr->mb_avail_a = ((mb_x > 0) && (curr_mb_slice_nr == img_ptr->slice_nr_ptr[img_ptr->curr_mb_nr - 1])); //left
    mb_cache_ptr->mb_avail_b = ((mb_y > 0) && (curr_mb_slice_nr == img_ptr->slice_nr_ptr[img_ptr->curr_mb_nr - frame_width_in_mbs])); //top
    mb_cache_ptr->mb_avail_c = ((mb_y > 0) &&
                                (mb_x < (frame_width_in_mbs -1)) && (curr_mb_slice_nr == img_ptr->slice_nr_ptr[img_ptr->curr_mb_nr - frame_width_in_mbs + 1])); //top right
    mb_cache_ptr->mb_avail_d = ((mb_x > 0) && (mb_y > 0) && (curr_mb_slice_nr == img_ptr->slice_nr_ptr[img_ptr->curr_mb_nr - frame_width_in_mbs - 1])); //top left

    img_ptr->b4_offset = (mb_y<<2)* img_ptr->b4_pitch+ (mb_x<<2);
    img_ptr->b4_offset_nnz = (mb_y*6)* img_ptr->b4_pitch + (mb_x<<2);

    if (img_ptr->type != I_SLICE)
    {
        //set mv range
        img_ptr->mv_x_min = -20*4 - mb_x*MB_SIZE_X4;
        img_ptr->mv_x_max = (frame_width_in_mbs - 1 - mb_x)*MB_SIZE_X4 + 80;
        img_ptr->mv_y_min = -20*4 - mb_y*MB_SIZE*4;
        img_ptr->mv_y_max = (img_ptr->frame_height_in_mbs - 1 - mb_y)*MB_SIZE_X4 + 80;

        //for mc
        img_ptr->xpos = (mb_x<<6) + 96;//(mb_x * 16 + Y_EXTEND_SIZE)*4;
        img_ptr->ypos = (mb_y<<6) + 96;//(mb_y * 16 + Y_EXTEND_SIZE)*4;

        if (img_ptr->type == B_SLICE)
        {
            int32 i, j, ii, jj;
            int32 blk_map_8x8[4] = {0, 0, 3, 3};
            int32 blk_map_normal[4] = {0, 1, 2, 3};
            int32 *blk_map;
            int32 blk_x = (mb_x << 2);
            int32 blk_y = (mb_y << 2);
            int32 b4_pitch = img_ptr->b4_pitch;

            if (img_ptr->g_active_sps_ptr->direct_8x8_inference_flag == 1)
            {
                blk_map = blk_map_8x8;
            } else
            {
                blk_map = blk_map_normal;
            }

            for (j = 0; j < 4; j++)
            {
                jj = blk_y + blk_map[j];
                for (i = 0; i < 4; i++)
                {
                    ii = blk_x + blk_map[i];
                    img_ptr->corner_map[j*4+i] = jj*b4_pitch + ii;
                }
            }
        }
    }

    if (img_ptr->is_cabac)
    {
        mb_cache_ptr->vld_dc_coded_flag = (img_ptr->abv_mb_info->dc_coded_flag << 4) | ((mb_info_ptr - 1)->dc_coded_flag);
    }

    return;
}

void H264Dec_interpret_mb_mode_I (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 mb_mode = mb_info_ptr->mb_type;
    int32 idx = (mb_mode-1)>>2;

    if (idx < 0 || idx > TBL_SIZE_ICBP-1)
    {
        idx = Clip3(0, TBL_SIZE_ICBP-1, idx);
        img_ptr->error_flag |= ER_BSM_ID;
    }

    if (mb_mode == 0)
    {
        mb_info_ptr->mb_type = I4MB_264;
    } else if (mb_mode == 25)
    {
        mb_info_ptr->mb_type = IPCM;
    } else
    {
        mb_info_ptr->mb_type = I16MB;
        mb_info_ptr->cbp = g_ICBP_TBL[idx];
        mb_cache_ptr->i16mode = ((mb_mode-1)&0x3);
    }

    return;
}

void H264Dec_interpret_mb_mode_P (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 mb_mode;

    if (img_ptr->is_cabac && !mb_cache_ptr->is_skipped)	mb_info_ptr->mb_type++;
    mb_mode = mb_info_ptr->mb_type;

    switch (mb_mode)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        mb_cache_ptr->all_zero_ref = 0;
        break;
    case 4:
    case 5:
        mb_info_ptr->mb_type = P8x8;
        mb_cache_ptr->all_zero_ref = mb_mode - 4;
        {
            uint32 sub_mb_mode;
            int32 blk8x8Nr;

            for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
            {
                sub_mb_mode = img_ptr->read_b8mode ((void *)img_ptr, mb_cache_ptr, blk8x8Nr);
                if (sub_mb_mode > 0)
                {
                    mb_cache_ptr->noSubMbPartSizeLess8x8 = FALSE;
                }
#if _H264_PROTECT_ & _LEVEL_LOW_
                if((sub_mb_mode < 0) || (sub_mb_mode >= 4))
                {
                    img_ptr->error_flag |= ER_BSM_ID;
                    img_ptr->return_pos |= (1<<23);
                    return;
                }
#endif
                mb_cache_ptr->b8mode[blk8x8Nr] = sub_mb_mode + 4;
                mb_cache_ptr->b8pdir[blk8x8Nr] = 0;
            }
        }
        break;
    case 6:
        mb_info_ptr->is_intra = TRUE;
        mb_info_ptr->mb_type = I4MB_264;
        break;
    case 31:
        mb_info_ptr->is_intra = TRUE;
        mb_info_ptr->mb_type = IPCM;
        break;
    default:
        mb_mode -= 7;
        mb_info_ptr->is_intra = TRUE;
        mb_info_ptr->mb_type = I16MB;
        mb_info_ptr->cbp = g_ICBP_TBL[mb_mode>>2];
        mb_cache_ptr->i16mode = (mb_mode&0x3);
    }

    return;
}

static const int b_v2b8 [14] = {0, 4, 4, 4, 5, 6, 5, 6, 5, 6, 7, 7, 7, 11};
static const int b_v2pd [14] = {2, 0, 1, 2, 0, 0, 1, 1, 2, 2, 0, 1, 2, -1};

static const int32 offset2pdir16x16[12]   = {0, 0, 1, 2, 0,0,0,0,0,0,0,0};
static const int32 offset2pdir16x8[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},{1,0},
    {0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2},{0,0}
};
static const int32 offset2pdir8x16[22][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{1,1},{0,0},{0,1},{0,0},
    {1,0},{0,0},{0,2},{0,0},{1,2},{0,0},{2,0},{0,0},{2,1},{0,0},{2,2}
};

void H264Dec_interpret_mb_mode_B (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 /*i,*/ mbmode;
    int32 mbtype = mb_info_ptr->mb_type;
    int8 *b8mode = mb_cache_ptr->b8mode;
    int8 *b8pdir = mb_cache_ptr->b8pdir;

    //set mbtype, b8type, and b8pdir
    if (mbtype== 0) //B_Skip,B_Direct_16*16
    {
        int8 *direct_cache = mb_cache_ptr->direct_cache;
        mbmode = 0;
        ((int32 *)b8mode)[0] = 0;
        ((int32 *)b8pdir)[0] = 0x02020202;

        ((int32*)direct_cache)[4] =
            ((int32*)direct_cache)[7] =
                ((int32*)direct_cache)[10] =
                    ((int32*)direct_cache)[13] = 0x01010101;

        mb_cache_ptr->is_direct = 1;
    } else if (mbtype == 23) //intra4x4
    {
        mbmode = I4MB_264;
        ((int32 *)b8mode)[0] = 0x0b0b0b0b;
        ((int32 *)b8pdir)[0] = 0xffffffff;
        mb_info_ptr->is_intra = TRUE;
    } else if ((mbtype>23) && (mbtype < 48)) //intra16x16
    {
        mbmode = I16MB;
        ((int32 *)b8mode)[0] = 0x00000000;
        ((int32 *)b8pdir)[0] = 0xffffffff;

        mb_info_ptr->is_intra = TRUE;
        mb_info_ptr->cbp = g_ICBP_TBL[(mbtype-24)>>2];
        mb_cache_ptr->i16mode = (mbtype-24)&0x3;
    } else if (mbtype == 22) //8x8(+split)
    {
        DEC_BS_T *stream = img_ptr->bitstrm_ptr;
        uint32 sub_mb_mode;
        int32 blk8x8Nr;

        //b8mode and pdir is transmitted in additional codewords
        for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
        {
            int32 offset = (blk8x8Nr>>1)*CTX_CACHE_WIDTH_X2 + (blk8x8Nr&0x1)*2;
            int8 *direct_dst_ptr = mb_cache_ptr->direct_cache + CTX_CACHE_WIDTH_PLUS4+ offset;

            sub_mb_mode = img_ptr->read_b8mode ((void *)img_ptr, mb_cache_ptr, blk8x8Nr);
            if (!sub_mb_mode)
            {
                ((int16*)direct_dst_ptr)[0] = 0x0101;
                direct_dst_ptr += CTX_CACHE_WIDTH;
                ((int16*)direct_dst_ptr)[0] = 0x0101;

                mb_cache_ptr->is_direct = 1;

                if (!img_ptr->g_active_sps_ptr->direct_8x8_inference_flag)
                {
                    mb_cache_ptr->noSubMbPartSizeLess8x8 = FALSE;
                }
            } else
            {
                ((int16*)direct_dst_ptr)[0] = 0x0;
                direct_dst_ptr += CTX_CACHE_WIDTH;
                ((int16*)direct_dst_ptr)[0] = 0x0;

                if (sub_mb_mode > 3)
                {
                    mb_cache_ptr->noSubMbPartSizeLess8x8 = FALSE;
                }
            }

            b8mode[blk8x8Nr] = b_v2b8[sub_mb_mode];
            b8pdir[blk8x8Nr] = b_v2pd[sub_mb_mode];
        }
        mbmode = P8x8;
    } else if (mbtype < 4) //16x16
    {
        mbmode = 1;
        ((int32 *)b8mode)[0] = 0x01010101;
        ((int32 *)b8pdir)[0] = 0x01010101 * offset2pdir16x16[mbtype];
    } else if (mbtype == 48)
    {
        mbmode = IPCM;
        ((int32 *)b8mode)[0] = 0x00000000;
        ((int32 *)b8pdir)[0] = 0xffffffff;

        mb_info_ptr->is_intra = TRUE;
        mb_info_ptr->cbp = -1;
        mb_cache_ptr->i16mode = 0;
    } else if (mbtype%2==0) //16x8
    {
        mbmode = 2;
        ((int32 *)b8mode)[0] = 0x02020202;
        b8pdir[0] = b8pdir[1] = offset2pdir16x8[mbtype][0];
        b8pdir[2] = b8pdir[3] = offset2pdir16x8[mbtype][1];
    } else
    {
        mbmode = 3;
        ((int32 *)b8mode)[0] = 0x03030303;
        b8pdir[0] = b8pdir[2] = offset2pdir8x16[mbtype][0];
        b8pdir[1] = b8pdir[3] = offset2pdir8x16[mbtype][1];
    }
    mb_info_ptr->mb_type = mbmode;

    return;
}

void H264Dec_fill_intraMB_context (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 top_mb_avail, left_mb_avail;
    DEC_MB_INFO_T *left_mb_info_ptr;
    int32 is_constained_intra = img_ptr->constrained_intra_pred_flag;
    int32 b4_pitch = img_ptr->b4_pitch;

    int8 *i4x4_pred_mode_cache = mb_cache_ptr->i4x4pred_mode_cache;
    int8 *i4x4_pred_mode_ptr = img_ptr->i4x4pred_mode_ptr + img_ptr->b4_offset;

    //top ipred mde reference
    top_mb_avail = mb_cache_ptr->mb_avail_b;
    if (top_mb_avail && img_ptr->abv_mb_info->is_intra)
    {
        ((int32 *)i4x4_pred_mode_cache)[1] = ((int32 *)(i4x4_pred_mode_ptr - b4_pitch))[0];
    } else
    {
        if (top_mb_avail && !is_constained_intra)
        {
            ((int32 *)i4x4_pred_mode_cache)[1] = 0x02020202;
        } else
        {
            ((int32 *)i4x4_pred_mode_cache)[1] = (int32)0xfefefefe;
        }
    }

    //left ipred mode reference
    left_mb_info_ptr = mb_info_ptr-1;
    left_mb_avail = mb_cache_ptr->mb_avail_a;
    if (left_mb_avail && left_mb_info_ptr->is_intra)
    {
        i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X1 + 3] = i4x4_pred_mode_ptr[b4_pitch*0 - 1];
        i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X2 + 3] = i4x4_pred_mode_ptr[b4_pitch*1 - 1];
        i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X3 + 3] = i4x4_pred_mode_ptr[b4_pitch*2 - 1];
        i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X4 + 3] = i4x4_pred_mode_ptr[b4_pitch*3 - 1];
    } else
    {
        int8 i4Pred;

        i4Pred = (left_mb_avail && !is_constained_intra) ? 2 : -2;

        i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X1 + 3] =
            i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X2 + 3] =
                i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X3 + 3] =
                    i4x4_pred_mode_cache[CTX_CACHE_WIDTH_X4 + 3] = i4Pred;
    }

    fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

    return;
}

void H264Dec_read_CBPandDeltaQp(H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    DEC_BS_T *stream = img_ptr->bitstrm_ptr;

    if (mb_info_ptr->mb_type != I16MB)
    {
        if (!img_ptr->is_cabac)
        {
            int32 val = READ_UE_V(stream);
            if (val >= 48)
            {
                img_ptr->error_flag |= ER_BSM_ID;
                img_ptr->return_pos |= (1<<26);
                return;
            }
            if(mb_info_ptr->is_intra)
            {
                mb_info_ptr->cbp = g_cbp_intra_tbl[val];
            } else
            {
                mb_info_ptr->cbp = g_cbp_inter_tbl[val];
            }
        } else
        {
            mb_info_ptr->cbp = decode_cabac_mb_cbp (img_ptr,  mb_info_ptr, mb_cache_ptr);
        }

        //decode transform_size_8x8_flag if needed
        if (((mb_info_ptr->cbp & 0xF) > 0) && (img_ptr->g_active_pps_ptr->transform_8x8_mode_flag) && (mb_info_ptr->mb_type != I4MB_264)
                && (mb_cache_ptr->noSubMbPartSizeLess8x8) &&
                ((mb_info_ptr->mb_type != 0) || (img_ptr->g_active_sps_ptr->direct_8x8_inference_flag)))
            /*mb_type != B_Direct_16x16*/
        {
            if (img_ptr->is_cabac)
            {
                mb_info_ptr->transform_size_8x8_flag = decode_cabac_mb_transform_size (img_ptr, mb_info_ptr, mb_cache_ptr);
            } else	//cavlc
            {
                mb_info_ptr->transform_size_8x8_flag = READ_BITS1(stream);
            }
        }
    }

    if ((mb_info_ptr->cbp > 0) || (mb_info_ptr->mb_type == I16MB))
    {
        int32 delta_q;
        if (!img_ptr->is_cabac)
        {
            delta_q = READ_SE_V(stream);
        } else
        {
            delta_q = decode_cabac_mb_dqp (img_ptr,  mb_info_ptr);
        }
        img_ptr->qp = mb_info_ptr->qp = (img_ptr->qp+delta_q+52)%52;
    }

    return;
}

/************************************************************************/
/* set IPCM NNZ                                                         */
/************************************************************************/
void H264Dec_set_IPCM_nnz(DEC_MB_CACHE_T *mb_cache_ptr)
{
    int8 *nnz_cache = mb_cache_ptr->nnz_cache;

    ((int32 *)nnz_cache)[4]  = ((int32 *)nnz_cache)[7]  =
                                   ((int32 *)nnz_cache)[10] = ((int32 *)nnz_cache)[13] = 0x10101010;//y
    ((int16 *)nnz_cache)[38] = ((int16 *)nnz_cache)[44] = 0x1010;//u
    ((int16 *)nnz_cache)[40] = ((int16 *)nnz_cache)[46] = 0x1010;//v
}

PUBLIC uint32 readB8_typeInfo_cavlc (void *img_ptr, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk8x8Nr)
{
    DEC_BS_T *stream = ((H264DecContext *)img_ptr)->bitstrm_ptr;
    uint32 b8mode;

    b8mode = READ_UE_V(stream);

    return b8mode;
}


LOCAL int32 decode_cabac_mb_skip(H264DecContext *img_ptr, DEC_MB_INFO_T *curr_mb_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 act_ctx;

    act_ctx = 0;
    if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->skip_flag)
        act_ctx++;
    if (mb_cache_ptr->mb_avail_a && (curr_mb_ptr - 1)->skip_flag)
        act_ctx++;

    if( img_ptr->type == B_SLICE )
        act_ctx += 13;
    return get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[11+act_ctx] );
}

PUBLIC int32 decode_cabac_mb_type (void  *img, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264DecContext *img_ptr = (H264DecContext *)img;

    if (img_ptr->type == I_SLICE)
    {
        return decode_cabac_intra_mb_type(img_ptr, mb_info_ptr, mb_cache_ptr, 3, 1);
    } else
    {
        int32 skip = decode_cabac_mb_skip (img_ptr, mb_info_ptr, mb_cache_ptr);

        mb_info_ptr->skip_flag = !skip;
        if (skip)
        {
            mb_cache_ptr->is_skipped = TRUE;
            mb_info_ptr->dc_coded_flag = 0;

            img_ptr->last_dquant=0;
            return 0;
        }

        if (img_ptr->type == P_SLICE)
        {
            if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[14]) == 0)
            {
                /*P-type*/
                if (get_cabac(&img_ptr->cabac, &img_ptr->cabac_state[15]) == 0)
                {
                    /* P_L0_D16x16, P_8x8 */
                    return 3 * get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[16] );
                } else
                {
                    /* P_L0_D8x16, P_L0_D16x8 */
                    return 2 - get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[17] );
                }
            } else
            {
                return decode_cabac_intra_mb_type(img_ptr, mb_info_ptr, mb_cache_ptr, 17, 0) + 5;
            }
        } else if( img_ptr->type == B_SLICE )
        {
            int32 ctx = 0;
            int32 bits;

            if (mb_cache_ptr->mb_avail_b && img_ptr->abv_mb_info->mb_type != 0)
            {
                ctx++;
            }

            if (mb_cache_ptr->mb_avail_a && (mb_info_ptr - 1)->mb_type != 0)
            {
                ctx++;
            }

            if( !get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+ctx] ) )
            {
                return 0; /* B_Direct_16x16 */
            }

            if( !get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+3] ) )
            {
                return 1 + get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+5] ); /* B_L[01]_16x16 */
            }

            bits = get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+4] ) << 3;
            bits|= get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+5] ) << 2;
            bits|= get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+5] ) << 1;
            bits|= get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+5] );
            if( bits < 8 )
            {
                return bits + 3; /* B_Bi_16x16 through B_L1_L0_16x8 */
            } else if( bits == 13 )
            {
                return decode_cabac_intra_mb_type(img_ptr, mb_info_ptr, mb_cache_ptr, 32, 0) + 23;
            } else if( bits == 14 )
            {
                return 11; /* B_L1_L0_8x16 */
            } else if( bits == 15 )
            {
                return 22; /* B_8x8 */
            }

            bits= ( bits<<1 ) | get_cabac( &img_ptr->cabac, &img_ptr->cabac_state[27+5] );
            return bits - 4; /* B_L0_Bi_* through B_Bi_Bi_* */
        }
    }

    return -1;
}

PUBLIC int32 decode_cavlc_mb_type (void *img, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264DecContext *img_ptr = (H264DecContext *)img;
    DEC_BS_T *stream = img_ptr->bitstrm_ptr;
    int32 mb_type;

    if (I_SLICE == img_ptr->type)
    {
        mb_type = READ_UE_V(stream);
    } else
    {
        //cod counter
        if (img_ptr->cod_counter == -1)
        {
            img_ptr->cod_counter = (int16)READ_UE_V(stream);
        }

        if (img_ptr->cod_counter == 0)
        {
            mb_type = READ_UE_V(stream) + (img_ptr->type == P_SLICE);
            mb_info_ptr->skip_flag = 0;
        } else
        {
            mb_type = 0;
            mb_info_ptr->skip_flag = 1;
            mb_cache_ptr->is_skipped = TRUE;
        }
        img_ptr->cod_counter--;
    }

    return mb_type;
}

//replace H264Dec_more_rbsp_data() function. same as !H264Dec_more_rbsp_data
PUBLIC uint32 uvlc_startcode_follows (void *img)
{
    H264DecContext *img_ptr = (H264DecContext *)img;
    int32 tmp;
    int32 has_no_zero = 0;
    int32 byte_offset;
    int32 bit_offset;
    DEC_BS_T * stream = img_ptr->bitstrm_ptr;
    uint32 nDecTotalBits = stream->bitcnt;

    byte_offset = nDecTotalBits>>3;
    bit_offset = nDecTotalBits&0x7;

    if (byte_offset < img_ptr->g_nalu_ptr->len -1)
    {
        return FALSE;
    }

    tmp = BITSTREAMSHOWBITS(stream, (uint32)(8-bit_offset));
    if (tmp == (1<<(7-bit_offset)))
    {
        return TRUE;
    } else
    {
        return FALSE;
    }
}

PUBLIC int32 H264Dec_exit_macroblock (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    //updated for next mb
    write_back_cache (img_ptr, mb_info_ptr, mb_cache_ptr);
    img_ptr->num_dec_mb++;

    mb_cache_ptr->mb_addr[0] += MB_SIZE;
    mb_cache_ptr->mb_addr[1] += BLOCK_SIZE;
    mb_cache_ptr->mb_addr[2] += BLOCK_SIZE;

    img_ptr->mb_x++;
    if (img_ptr->mb_x == img_ptr->frame_width_in_mbs)
    {
        img_ptr->mb_x = 0;
        img_ptr->mb_y ++;

        mb_cache_ptr->mb_addr[0] += ((img_ptr->ext_width<<4)- img_ptr->width);
        mb_cache_ptr->mb_addr[1] += ((img_ptr->ext_width<<2)- (img_ptr->width>>1));
        mb_cache_ptr->mb_addr[2] += ((img_ptr->ext_width<<2)- (img_ptr->width>>1));
    }

    if (img_ptr->num_dec_mb == img_ptr->frame_size_in_mbs)
    {
        img_ptr->curr_slice_ptr->next_header = SOP;
        return TRUE;
    } else
    {
        img_ptr->curr_mb_nr = H264Dec_Fmo_get_next_mb_num(img_ptr->curr_mb_nr, img_ptr);

        if (img_ptr->curr_mb_nr == -1)
        {
            img_ptr->nal_startcode_follows((void *)img_ptr);
            return TRUE;
        }

        if (!img_ptr->nal_startcode_follows((void *)img_ptr))
        {
            return FALSE;
        }

        if (img_ptr->type != I_SLICE)
        {
            if (img_ptr->cod_counter <= 0)
            {
                return TRUE;
            }

            return FALSE;
        } else
        {
            return TRUE;
        }
    }
}

#ifdef WIN32
void put_mb2Frame (uint8 *mb_pred, uint8 *mb_addr[3], int32 pitch)
{
    int32 uv;
    int32 i;
    uint8 * pFrame;
    int32 * pIntMB;

    /*put Y*/
    pFrame = mb_addr[0];
    pIntMB = (int32 *)mb_pred;

    for (i = 0; i < MB_SIZE; i++)
    {
        int32 * pIntFrame = (int32 *)pFrame;

        *pIntFrame++ = *pIntMB++;
        *pIntFrame++ = *pIntMB++;
        *pIntFrame++ = *pIntMB++;
        *pIntFrame++ = *pIntMB++;

        pFrame += pitch;
    }

    pitch >>= 1;
    for (uv = 0; uv < 2; uv++)
    {
        pFrame = mb_addr[uv+1];
        for (i = 0; i < 8; i++)
        {
            int32 * pIntFrame = (int32 *)pFrame;
            *pIntFrame++ = *pIntMB++;
            *pIntFrame++ = *pIntMB++;
            pFrame += pitch;
        }
    }
}
#endif

LOCAL void H264Dec_read_and_derive_ipred_modes(H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    //get 16 block4x4 intra prediction mode, luma
    if (mb_info_ptr->mb_type == I4MB_264)
    {
        int32 blk4x4StrIdx, blk4x4Idx, blk8x8Idx;
        int32 pred_mode;
        int32 up_ipred_mode;
        int32 left_ipred_mode;
        int32 most_probable_ipred_mode;
        int8 *i4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;
        const uint8 *blk_order_map_tbl_ptr = g_blk_order_map_tbl;
        DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;
        int32 val;

        int32 i, j;

        //First decode transform_size_8x8_flag.
        if (img_ptr->g_active_pps_ptr->transform_8x8_mode_flag)
        {
            if (img_ptr->is_cabac)
            {
                mb_info_ptr->transform_size_8x8_flag = decode_cabac_mb_transform_size(img_ptr, mb_info_ptr, mb_cache_ptr);
            } else
            {
                mb_info_ptr->transform_size_8x8_flag = READ_BITS1(bitstrm);
            }
        }

        if (!mb_info_ptr->transform_size_8x8_flag)
        {
            //Intra 4x4
            for (blk4x4Idx = 0, i = 0; i < 8; i++)
            {
                for (j = 1; j >= 0; j--)
                {
                    if (!img_ptr->is_cabac)
                    {
                        val = BITSTREAMSHOWBITS(bitstrm, 4);
                        if(val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!, noted by xwluo@20100618
                        {
                            pred_mode = -1;
                            READ_BITS1(bitstrm);
                        } else
                        {
                            pred_mode = val & 0x7; //remaining_mode_selector
                            READ_FLC(bitstrm, 4);
                        }
                    } else
                    {
                        pred_mode = decode_cabac_mb_intra4x4_pred_mode (img_ptr);
                    }

                    blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
                    left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
                    up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-CTX_CACHE_WIDTH];
                    most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

                    if (most_probable_ipred_mode < 0)
                    {
                        most_probable_ipred_mode = DC_PRED;
                    }

                    pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + (pred_mode >= most_probable_ipred_mode)));

                    i4_pred_mode_ref_ptr[blk4x4StrIdx] = pred_mode;
                    blk4x4Idx++;
                }
            }
        } else
        {
            //Intra 8x8
            for (blk8x8Idx = 0; blk8x8Idx < 4; blk8x8Idx++)
            {
                if (!img_ptr->is_cabac)
                {
                    val = BITSTREAMSHOWBITS (bitstrm, 4);
                    if (val >= 8)	//b'1xxx, use_most_probable_mode_flag = 1!
                    {
                        pred_mode = -1;
                        READ_BITS1(bitstrm);
                    } else
                    {
                        pred_mode = val & 0x7;	//remaining_mode_selector
                        READ_FLC(bitstrm, 4);
                    }
                } else
                {
                    pred_mode = decode_cabac_mb_intra4x4_pred_mode (img_ptr);
                }

                blk4x4Idx = (blk8x8Idx << 2);
                blk4x4StrIdx = blk_order_map_tbl_ptr[blk4x4Idx];
                left_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx-1];
                up_ipred_mode = i4_pred_mode_ref_ptr[blk4x4StrIdx - CTX_CACHE_WIDTH];
                most_probable_ipred_mode = mmin(left_ipred_mode, up_ipred_mode);

                if (most_probable_ipred_mode < 0)
                {
                    most_probable_ipred_mode = DC_PRED;
                }
                pred_mode = ((pred_mode == -1) ? most_probable_ipred_mode : (pred_mode + (pred_mode >= most_probable_ipred_mode)));

                i4_pred_mode_ref_ptr[blk4x4StrIdx] =
                    i4_pred_mode_ref_ptr[blk4x4StrIdx + 1] =
                        i4_pred_mode_ref_ptr[blk4x4StrIdx + CTX_CACHE_WIDTH] =
                            i4_pred_mode_ref_ptr[blk4x4StrIdx + CTX_CACHE_WIDTH + 1] = pred_mode;
            }
        }
    } else
    {
        int8 *i4x4_pred_mode_ref_ptr = mb_cache_ptr->i4x4pred_mode_cache;

        ((int32*)i4x4_pred_mode_ref_ptr)[4] =
            ((int32*)i4x4_pred_mode_ref_ptr)[7] =
                ((int32*)i4x4_pred_mode_ref_ptr)[10] =
                    ((int32*)i4x4_pred_mode_ref_ptr)[13] = 0x02020202;
    }

    if (!img_ptr->is_cabac)
    {
        DEC_BS_T *bitstrm = img_ptr->bitstrm_ptr;

        //get chroma intra prediction mode
        mb_info_ptr->c_ipred_mode = READ_UE_V(bitstrm);
    } else
    {
        mb_info_ptr->c_ipred_mode = decode_cabac_mb_chroma_pre_mode(img_ptr, mb_info_ptr, mb_cache_ptr);
    }

    return;
}

/************************************************************************/
/* decode I_PCM data                                                    */
/************************************************************************/
LOCAL void H264Dec_decode_IPCM_MB_sw(H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr, DEC_BS_T * bitstrm)
{
    int32 value;
    int32 i,j;
    int32 bit_offset;
    int32 comp;
    int32 *i4x4pred_mode_cache = (int32 *)(mb_cache_ptr->i4x4pred_mode_cache);

    i4x4pred_mode_cache[4] = i4x4pred_mode_cache[7] =
                                 i4x4pred_mode_cache[10] = i4x4pred_mode_cache[13] = 0x02020202;

    bit_offset  = (bitstrm->bitsLeft & 0x7);
    if(bit_offset)
    {
        READ_FLC(bitstrm, bit_offset);
    }

    if (img_ptr->cabac.low & 0x1)
    {
        REVERSE_BITS(bitstrm, 8);
    }

    if (img_ptr->cabac.low & 0x1ff)
    {
        REVERSE_BITS(bitstrm, 8);
    }

    //Y, U, V
    for (comp = 0; comp < 3; comp++)
    {
        int32 blk_size = (comp == 0) ? MB_SIZE : MB_CHROMA_SIZE;
        int32 pitch = (comp == 0) ? img_ptr->ext_width : (img_ptr->ext_width>>1);
        uint8 *I_PCM_value = mb_cache_ptr->mb_addr[comp];

        for (j = 0; j < blk_size; j++)
        {
            for (i = 0; i < blk_size; i+=4)
            {
                value = READ_FLC(bitstrm, 32);

                I_PCM_value[i] 	 = (value>>24) & 0xff;
                I_PCM_value[i+1] = (value>>16) & 0xff;
                I_PCM_value[i+2] = (value>>8) & 0xff;
                I_PCM_value[i+3] = (value>>0) & 0xff;
            }
            I_PCM_value += pitch;
        }
    }

    H264Dec_set_IPCM_nnz(mb_cache_ptr);

    mb_info_ptr->qp = 0;
    mb_info_ptr->cbp = 0x3f;
    mb_info_ptr->skip_flag = 1;

    //for CABAC decoding of Dquant
    if (img_ptr->g_active_pps_ptr->entropy_coding_mode_flag)
    {
        img_ptr->last_dquant = 0;
        mb_info_ptr->dc_coded_flag = 7;
        ff_init_cabac_decoder(img_ptr);	//arideco_start_decoding(img_ptr);
    }
}

LOCAL void FilterPixel_I8x8 (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr,DEC_MB_CACHE_T *mb_cache_ptr, int blkidx_8x8, uint8 *pRec, int32 pitch)
{
    int32 i;
    uint8 * ptop;
    uint8 * pLeft;
    uint8 * pDst;
    uint8  topleftpix;
    uint8  toppix[16];
    uint8  leftpix[8];
    int8    availA ;
    int8    availB ;
    int8    availC ;
    int8    availD ;


    if(blkidx_8x8 == 0)
    {
        availA = mb_cache_ptr->mb_avail_a;
        availB = mb_cache_ptr->mb_avail_b;
        availD = mb_cache_ptr->mb_avail_d;
        if(img_ptr->constrained_intra_pred_flag)
        {
            availA &= (mb_info_ptr -1)->is_intra;
            availB &= (img_ptr->abv_mb_info)->is_intra;
            availD &= (img_ptr->abv_mb_info -1)->is_intra;
        }
        availC = availB;
    }
    else if(blkidx_8x8 == 1)
    {
        availB = mb_cache_ptr->mb_avail_b;
        availC = mb_cache_ptr->mb_avail_c;
        if(img_ptr->constrained_intra_pred_flag)
        {
            availB &= (img_ptr->abv_mb_info)->is_intra;
            availC &= (img_ptr->abv_mb_info +1)->is_intra;
        }
        availA = 1;
        availD = availB;
    }
    else if(blkidx_8x8 == 2)
    {
        availA = mb_cache_ptr->mb_avail_a;
        if(img_ptr->constrained_intra_pred_flag)
        {
            availA &= (mb_info_ptr -1)->is_intra;
        }
        availD = availA;
        availB = 1;
        availC = 1;
    }
    else
    {
        availA = 1;
        availB = 1;
        availC = 0;
        availD = 1;
    }

    ptop = pRec - pitch;
    topleftpix = ptop[-1];
    pLeft = pRec -1;

    //store availx to mb_cache which can be used directly by intra prediction process.
    mb_cache_ptr->mb_avail_a_8x8_ipred = availA;
    mb_cache_ptr->mb_avail_b_8x8_ipred = availB;
    mb_cache_ptr->mb_avail_c_8x8_ipred = availC;
    mb_cache_ptr->mb_avail_d_8x8_ipred = availD;

    //load pixel vaule to array toppix and leftpix.

    ((uint32 *)toppix)[0] = ((uint32 *)ptop)[0];
    ((uint32 *)toppix)[1] = ((uint32 *)ptop)[1];
    if(availC)
    {
        ((uint32 *)toppix)[2] = ((uint32 *)ptop)[2];
        ((uint32 *)toppix)[3] = ((uint32 *)ptop)[3];
    }
    else
    {
        uint32 tmp_pix = ptop[7] * 0x01010101;
        ((uint32 *)toppix)[2]  = ((uint32 *)toppix)[3] = tmp_pix;
    }

    for(i= 0; i<8; i++)
    {
        leftpix[i] = pLeft[0];
        pLeft += pitch;
    }

    /*filter top border pixel*/

    pDst = mb_cache_ptr->topFilteredPix;

    //p[-1,-1]
    if(!availA && availB)
        pDst[0] = (3*topleftpix + toppix[0] +2) >>2;
    else if(availA && !availB)
        pDst[0] = (3*topleftpix+ leftpix[0] +2) >>2;
    else if (!availA &&  !availB)
        pDst[0] =topleftpix;
    else
        pDst[0] = (leftpix[0] + 2*topleftpix + toppix[0] +2) >>2;

    //p[0,-1]
    if(availD)
        pDst[1] = (topleftpix + 2*toppix[0] + toppix[1] +2) >>2;
    else
        pDst[1] = (3*toppix[0] +  toppix[1]+2) >>2;

    //p[x,-1] x=1...14
    for(i = 1; i <15; i++)
        pDst[i+1] =  (toppix[i-1] + 2*toppix[i] + toppix[i+1] +2)>>2;

    //p[15, -1]
    pDst[16] =  (toppix[14] + 3* toppix[15]+2) >>2;


    /*filter left border pixel*/
    // leftPredPix_Y[8..15] to store left filtered pixel. leftPredPix_Y[7] to duplicate topleft pixel
    pDst = mb_cache_ptr->leftPredPix_Y + 8;

    //p[-1,-1]
    pDst[-1] = mb_cache_ptr->topFilteredPix[0];

    //p[-1,0]
    if(availD)
        pDst[0] = (topleftpix + 2*leftpix[0] + leftpix[1] +2) >>2;
    else
        pDst[0] =  (3*leftpix[0] +  leftpix[1]+2) >>2;

    //p[-1,y] y=1...6
    for(i=1; i<7; i++)
        pDst[i] =  (leftpix[i-1] + 2*leftpix[i] + leftpix[i+1] +2)>>2;

    //p[-1,7]
    pDst[7] = ( leftpix[6] + 3*leftpix[7] +2) >>2;

}

/*blk4x4Nr order
___ ___ ___ ___
| 0 | 1 | 4 | 5 |
|___|___|___|___|
| 2 | 3 | 6 | 7 |
|___|___|___|___|
| 8 | 9 | 12| 13|
|___|___|___|___|
| 10| 11| 14| 15|
|___|___|___|___|

*/

LOCAL void H264Dec_decode_intra_mb(H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 uv;
    int32 blkStrOdr;
    int32 blk4x4Nr, blk8x8Nr;
    int32 blkScanIdx;
    int32 mb_type = mb_info_ptr->mb_type;
    int16 *coff;
    uint8 *pred, *rec;
    uint8 *mb_pred, *mb_rec;
    const uint8 * pMbCacheAddr = g_mbcache_addr_map_tbl;
    int32 MbPos_x = img_ptr->mb_x;
    int32 MbPos_y = img_ptr->mb_y;
    const uint8 * pBlkOrderMap = g_blk_order_map_tbl;
    const uint8 * pDecToScnOdrMap = g_dec_order_to_scan_order_map_tbl;
    int32 ext_width = img_ptr->ext_width;
    int32 uvCBPMsk, uvCBP;

    coff = mb_cache_ptr->coff_Y;
    mb_pred = mb_cache_ptr->pred_cache[0].pred_Y;
    mb_rec = mb_cache_ptr->mb_addr[0];

    if (mb_type == I16MB)
    {
        int32 i16mode = mb_cache_ptr->i16mode;
#if defined(CTS_PROTECT)
        i16mode = IClip(0, 3, i16mode);
#endif
        g_intraPred16x16[i16mode]((void *)img_ptr, mb_info_ptr,mb_cache_ptr, mb_pred, mb_rec, ext_width);

        for (blk4x4Nr = 0; blk4x4Nr < 16; blk4x4Nr++)
        {
//			if (blk4x4Nr == 4)
//				PRINTF (" ");
            pred = mb_pred + pMbCacheAddr [blk4x4Nr];
            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
            itrans_4x4 (coff, pred, MB_SIZE, rec, ext_width);
            coff += 16;
        }
    }
    else if (!mb_info_ptr->transform_size_8x8_flag)	//intra 4x4
    {
        int8 * pI4PredMode = mb_cache_ptr->i4x4pred_mode_cache;
        int8 *pNnzRef = mb_cache_ptr->nnz_cache;
        Intra4x4Pred * intraPred4x4 = g_intraPred4x4;
        int32 intraPredMode;

        for (blk4x4Nr = 0; blk4x4Nr < 16; blk4x4Nr++)
        {
            blkStrOdr = pBlkOrderMap[blk4x4Nr];
            intraPredMode = pI4PredMode [blkStrOdr];

            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
            pred = mb_pred + pMbCacheAddr [blk4x4Nr];
            blkScanIdx = pDecToScnOdrMap [blk4x4Nr];

#if defined(CTS_PROTECT)
            intraPredMode = IClip(0, 8, intraPredMode);
#endif
            intraPred4x4[intraPredMode] (img_ptr, mb_info_ptr, mb_cache_ptr, pred, blkScanIdx, rec, ext_width);

            if (pNnzRef[blkStrOdr])
            {
                itrans_4x4 (coff, pred, MB_SIZE, rec, ext_width);
            } else
            {
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_SIZE;
            }

            coff += 16;

        }
    } else	//Intra 8x8
    {
        int8 * pI4PredMode = mb_cache_ptr->i4x4pred_mode_cache;
        int8 *pNnzRef = mb_cache_ptr->nnz_cache;
        int32 intraPredMode;

        for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
        {
            blk4x4Nr = blk8x8Nr << 2;
            blkStrOdr = pBlkOrderMap[blk4x4Nr];
            intraPredMode = pI4PredMode[blkStrOdr];

            pred = mb_pred + pMbCacheAddr[blk4x4Nr];
            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
            FilterPixel_I8x8 (img_ptr, mb_info_ptr, mb_cache_ptr, blk8x8Nr, rec, ext_width);

            //intraPred8x8 function
            g_intraPred8x8[intraPredMode] (mb_cache_ptr, pred, blk8x8Nr);

            if (pNnzRef[blkStrOdr]||pNnzRef[blkStrOdr+1]||pNnzRef[blkStrOdr+12]||pNnzRef[blkStrOdr+13])
            {
                itrans_8x8 (coff, pred, MB_SIZE, rec, ext_width);
            } else
            {
                //copy 8x8
#ifdef WIN32
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];	//rec += ext_width; pred += MB_SIZE;
#else
                copyPredblk_8x8 (pred, rec, ext_width);
#endif
            }

            coff += 64;
        }
    }

    uvCBPMsk = 1;
    uvCBP = mb_cache_ptr->cbp_uv;
    ext_width >>= 1;

    for (uv = 0; uv < 2; uv++)
    {
        int predMode = mb_info_ptr->c_ipred_mode;

#if defined(CTS_PROTECT)
        predMode = IClip(0, 3, predMode);
#endif
        coff = mb_cache_ptr->coff_UV [uv];
        mb_pred = mb_cache_ptr->pred_cache[0].pred_UV [uv];
        mb_rec = mb_cache_ptr->mb_addr[1+uv];

//		ptopLeftPix = mb_cache_ptr->mb_addr[uv+1] - ext_width - 1;
        g_intraChromaPred[predMode] (img_ptr, mb_info_ptr, mb_cache_ptr,mb_pred, mb_rec, ext_width);

        for (blk4x4Nr = 16; blk4x4Nr < 20; blk4x4Nr++)
        {
            pred = mb_pred + pMbCacheAddr [blk4x4Nr];
            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
            if (uvCBP & uvCBPMsk)
            {
                itrans_4x4 (coff, pred, MB_CHROMA_SIZE, rec, ext_width);
            } else
            {
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_CHROMA_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_CHROMA_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_CHROMA_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_CHROMA_SIZE;
            }

            uvCBPMsk = uvCBPMsk << 1;
            coff += 16;
        }
    }
}

LOCAL void H264Dec_decode_inter_mb (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 uv;
    int32 blk4x4Nr, blk8x8Nr;
    int32 blkStrIdx;
    int16 * coff;
    uint8 * mb_pred, * mb_rec;
    uint8 * pred, *rec;
    const uint8 * pBlkOdrMap = g_blk_order_map_tbl;
    const uint8 * pMbCacheAddr = g_mbcache_addr_map_tbl;
    int32 ext_width = img_ptr->ext_width;
    int uvCBPMsk, uvCBP;

    /*luminance*/
    coff = mb_cache_ptr->coff_Y;
    mb_pred = mb_cache_ptr->pred_cache[0].pred_Y;
    mb_rec = mb_cache_ptr->mb_addr[0];

    if (!mb_info_ptr->transform_size_8x8_flag)
    {
        // 4x4 idct
        for (blk4x4Nr = 0; blk4x4Nr < 16; blk4x4Nr++)
        {
            blkStrIdx = pBlkOdrMap [blk4x4Nr];

            pred = mb_pred + pMbCacheAddr [blk4x4Nr];
            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
            if (mb_cache_ptr->nnz_cache [blkStrIdx])
            {
                itrans_4x4 (coff, pred, MB_SIZE, rec, ext_width);
            } else
            {
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_SIZE;
            }

            coff += 16;
        }
    } else
    {
        int8 *pNnzRef = mb_cache_ptr->nnz_cache;
        //8x8 idct
        for (blk8x8Nr = 0; blk8x8Nr < 4; blk8x8Nr++)
        {
            blk4x4Nr = blk8x8Nr << 2;
            blkStrIdx = pBlkOdrMap[blk4x4Nr];

            pred = mb_pred+ pMbCacheAddr[blk4x4Nr];
            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];

            if (pNnzRef[blkStrIdx]||pNnzRef[blkStrIdx+1]||pNnzRef[blkStrIdx+12]||pNnzRef[blkStrIdx+13])
            {
                itrans_8x8 (coff, pred, MB_SIZE, rec, ext_width);
            } else
            {
                //copy 8x8
#ifdef WIN32
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];
                rec += ext_width;
                pred += MB_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                ((uint32 *)rec)[1] = ((uint32 *)pred)[1];	//rec += ext_width; pred += MB_SIZE;
#else
                copyPredblk_8x8 (pred, rec, ext_width);
#endif
            }

            coff += 64;
        }
    }

    /*chroma*/
    ext_width >>= 1;
    uvCBP = mb_cache_ptr->cbp_uv;
    uvCBPMsk = 1;
    for (uv = 0; uv < 2; uv++)
    {
        coff = mb_cache_ptr->coff_UV[uv];
        mb_pred = mb_cache_ptr->pred_cache[0].pred_UV [uv];
        mb_rec = mb_cache_ptr->mb_addr[1+uv];

        for (blk4x4Nr = 16; blk4x4Nr < 20; blk4x4Nr++)
        {
            pred = mb_pred + pMbCacheAddr [blk4x4Nr];
            rec = mb_rec + mb_cache_ptr->blk4x4_offset[blk4x4Nr];
            if (uvCBP & uvCBPMsk)
            {
                itrans_4x4 (coff, pred, MB_CHROMA_SIZE, rec, ext_width);
            } else
            {
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_CHROMA_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_CHROMA_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];
                rec += ext_width;
                pred += MB_CHROMA_SIZE;
                ((uint32 *)rec)[0] = ((uint32 *)pred)[0];	//rec += ext_width; pred += MB_CHROMA_SIZE;
            }

            uvCBPMsk = uvCBPMsk << 1;
            coff += 16;
        }
    }
}

LOCAL void H264Dec_read_intraMB_context (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264Dec_fill_intraMB_context (img_ptr, mb_info_ptr, mb_cache_ptr);

    if (mb_info_ptr->mb_type != IPCM)
    {
        H264Dec_read_and_derive_ipred_modes (img_ptr, mb_info_ptr, mb_cache_ptr);
        H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (img_ptr->error_flag)
        {
            return;
        }
#endif

        img_ptr->sw_vld_mb((void *)img_ptr, mb_info_ptr, mb_cache_ptr);
#if _H264_PROTECT_ & _LEVEL_HIGH_
        if (img_ptr->error_flag)
        {
            return;
        }
#endif

        H264Dec_decode_intra_mb (img_ptr, mb_info_ptr, mb_cache_ptr);
    } else
    {
        H264Dec_decode_IPCM_MB_sw(img_ptr, mb_info_ptr, mb_cache_ptr, img_ptr->bitstrm_ptr);
    }
}

PUBLIC void H264Dec_read_one_macroblock_ISlice (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    mb_info_ptr->is_intra = TRUE;

    mb_info_ptr->mb_type = img_ptr->read_mb_type ((void *)img_ptr, mb_info_ptr, mb_cache_ptr);
    H264Dec_interpret_mb_mode_I (img_ptr, mb_info_ptr, mb_cache_ptr);
    H264Dec_read_intraMB_context(img_ptr, mb_info_ptr, mb_cache_ptr);

    return;
}

PUBLIC void H264Dec_read_one_macroblock_PSlice (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    mb_info_ptr->mb_type = img_ptr->read_mb_type ((void *)img_ptr, mb_info_ptr, mb_cache_ptr);
    H264Dec_interpret_mb_mode_P(img_ptr, mb_info_ptr, mb_cache_ptr);

    if (mb_info_ptr->is_intra)
    {
        H264Dec_read_intraMB_context(img_ptr, mb_info_ptr, mb_cache_ptr);
    } else
    {
        mb_cache_ptr->read_ref_id_flag[0] =( (img_ptr->ref_count[0] > 1) && (!mb_cache_ptr->all_zero_ref));
        mb_cache_ptr->read_ref_id_flag[1] = ((img_ptr->ref_count[1] > 1) && (!mb_cache_ptr->all_zero_ref));

        fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

        if (!mb_cache_ptr->is_skipped)
        {
            H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
            H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
            img_ptr->sw_vld_mb ((void *)img_ptr, mb_info_ptr, mb_cache_ptr);
        }

        H264Dec_mv_prediction (img_ptr, mb_info_ptr, mb_cache_ptr);

        if (!mb_cache_ptr->is_skipped)
        {
            H264Dec_decode_inter_mb (img_ptr, mb_info_ptr, mb_cache_ptr);
        } else
        {
            put_mb2Frame (mb_cache_ptr->pred_cache[0].pred_Y, mb_cache_ptr->mb_addr, img_ptr->ext_width);
        }
    }

    return;
}

PUBLIC void H264Dec_read_one_macroblock_BSlice (H264DecContext *img_ptr, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    mb_info_ptr->mb_type = img_ptr->read_mb_type ((void *)img_ptr, mb_info_ptr, mb_cache_ptr);
    H264Dec_interpret_mb_mode_B(img_ptr, mb_info_ptr, mb_cache_ptr);

    if (mb_info_ptr->is_intra)
    {
        H264Dec_read_intraMB_context(img_ptr, mb_info_ptr, mb_cache_ptr);
    } else
    {
#if _H264_PROTECT_ & _LEVEL_LOW_
        if (img_ptr->g_list[0][0] == NULL)
        {
            img_ptr->error_flag |= ER_REF_FRM_ID;
            img_ptr->return_pos1 |= (1<<1);
            return;
        }
#endif

        mb_cache_ptr->read_ref_id_flag[0] = (img_ptr->ref_count[0] > 1);
        mb_cache_ptr->read_ref_id_flag[1] = (img_ptr->ref_count[1] > 1);

        fill_cache (img_ptr, mb_info_ptr, mb_cache_ptr);

        if (!mb_cache_ptr->is_skipped)
        {
#if 0
            if (IS_INTERMV(mb_info_ptr))
#else
            if (mb_info_ptr->mb_type != 0)
#endif
            {
                H264Dec_read_motionAndRefId (img_ptr, mb_info_ptr, mb_cache_ptr);
            }
            H264Dec_read_CBPandDeltaQp (img_ptr, mb_info_ptr, mb_cache_ptr);
            img_ptr->sw_vld_mb((void *)img_ptr, mb_info_ptr, mb_cache_ptr);
        }

        if ( mb_cache_ptr->is_direct)
        {
            img_ptr->direct_mv (img_ptr, mb_info_ptr, mb_cache_ptr);
        }

        H264Dec_mv_prediction (img_ptr, mb_info_ptr, mb_cache_ptr);

        if (!mb_cache_ptr->is_skipped)
        {
            H264Dec_decode_inter_mb (img_ptr, mb_info_ptr, mb_cache_ptr);
        } else
        {
            put_mb2Frame (mb_cache_ptr->pred_cache[0].pred_Y, mb_cache_ptr->mb_addr, img_ptr->ext_width);
        }
    }

    return;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
