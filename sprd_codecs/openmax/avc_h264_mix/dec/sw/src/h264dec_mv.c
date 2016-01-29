/******************************************************************************
 ** File Name:    h264dec_mv.c			                                      *
 ** Author:       Xiaowei Luo                                                 *
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

#define PITCH	CONTEXT_CACHE_WIDTH

//#define CHIP_ENDIAN_LITTLE

#if defined(_VSP_)
#ifdef CHIP_ENDIAN_LITTLE
#else
#define H264_BIG_ENDIAN
#endif
#endif

#define IS_DIR(pdir)	(((pdir) == 2) || (s_list == (pdir)))

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
PUBLIC int32 H264Dec_get_te (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 blk_id, int32 list)
{
    int32 ref_frm_id;
    H264DecContext *vo = (H264DecContext *)img;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;
    int32 num_ref_idx_active = vo->ref_count[list];

    if (num_ref_idx_active == 2) {
        ref_frm_id = READ_FLAG();
        ref_frm_id = 1 - ref_frm_id;
    } else {
        ref_frm_id = UE_V();
    }

    return ref_frm_id;
}

PUBLIC int32 decode_cavlc_mb_mvd(void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 sub_blk_id, int32 list)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 mvd_x, mvd_y, mvd_xy;
    DEC_BS_T *bs_ptr = vo->bitstrm_ptr;

    mvd_x = SE_V();
    mvd_y = SE_V();

#if defined(H264_BIG_ENDIAN)
    mvd_xy  = (mvd_x << 16) | (mvd_y & 0xffff);
#else
    mvd_xy  = (mvd_y << 16) | (mvd_x & 0xffff);
#endif

    return mvd_xy;
}

#define IS_DIR(pdir)	(((pdir) == 2) || (s_list == (pdir)))

LOCAL void H264Dec_read_motionAndRefId_PMB16x16 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    DEC_STORABLE_PICTURE_T	**listX;
    int32 ref_frm_id_s32;
    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;
    int8 *s_ref_idx_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;

#ifdef _NEON_OPT_
    int32x4_t v32x4;
#endif

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        if(IS_DIR(pdir)) {
            listX = vo->g_list[s_list];

            if (mb_cache_ptr->read_ref_id_flag[s_list]) {
                int32 ref_frm_id = vo->readRefFrame ((void *)vo, mb_cache_ptr, 0, s_list);

#if _H264_PROTECT_ & _LEVEL_HIGH_
                if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1)) {
                    vo->error_flag |= ER_REF_FRM_ID;
                    vo->return_pos |= (1<<27);
                    return;
                }
#endif
                ref_frm_id_s32 = ref_frm_id * 0x01010101;
            } else {
                ref_frm_id_s32 = 0;
            }
        } else {
            ref_frm_id_s32 = LIST_NOT_USED;
        }

        if (ref_frm_id_s32) {// != 0
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list];
            ((int32 *)s_ref_idx_ptr)[4] = ((int32 *)s_ref_idx_ptr)[7] =
                                              ((int32 *)s_ref_idx_ptr)[10] = ((int32 *)s_ref_idx_ptr)[13] = ref_frm_id_s32;
        }
    }

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        if(IS_DIR(pdir)) {
            s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4, s_list);

            if (s_mvd_xy) {
                s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]);

#ifndef _NEON_OPT_
                s_mvd_ptr[16] = s_mvd_ptr[17] = s_mvd_ptr[18] = s_mvd_ptr[19] =
                                                    s_mvd_ptr[28] = s_mvd_ptr[29] = s_mvd_ptr[30] = s_mvd_ptr[31] =
                                                            s_mvd_ptr[40] = s_mvd_ptr[41] = s_mvd_ptr[42] = s_mvd_ptr[43] =
                                                                    s_mvd_ptr[52] = s_mvd_ptr[53] = s_mvd_ptr[54] = s_mvd_ptr[55] = s_mvd_xy;
#else
                v32x4 = vmovq_n_s32 (s_mvd_xy);
                vst1q_s32 (s_mvd_ptr+16, v32x4);
                vst1q_s32 (s_mvd_ptr+28, v32x4);
                vst1q_s32 (s_mvd_ptr+40, v32x4);
                vst1q_s32 (s_mvd_ptr+52, v32x4);
#endif
            }
        }
    }

    return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB16x8 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    DEC_STORABLE_PICTURE_T	**listX;
    int32 ref_frm_id_s32;
    uint32 b16x8;
    int32 pdir;
#ifdef _NEON_OPT_
    int32x4_t v32x4;
#endif
    int32 s_list;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        for (b16x8= 0; b16x8 < 2; b16x8++) {
            pdir = mb_cache_ptr->b8pdir[2*b16x8];

            if(IS_DIR(pdir)) {
                listX = vo->g_list[s_list];

                if (mb_cache_ptr->read_ref_id_flag[s_list]) {
                    int32 ref_frm_id = vo->readRefFrame ((void *)vo, mb_cache_ptr, 2*b16x8, s_list);

#if _H264_PROTECT_ & _LEVEL_HIGH_
                    if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1)) {
                        vo->error_flag |= ER_REF_FRM_ID;
                        vo->return_pos |= (1<<28);

                        return;
                    }
#endif
                    ref_frm_id_s32 = ref_frm_id * 0x01010101;
                } else {
                    ref_frm_id_s32 = 0;
                }
            } else {
                ref_frm_id_s32 = LIST_NOT_USED;
            }

            if (ref_frm_id_s32) {// != 0
                s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
                ((int32 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
                s_ref_idx_ptr += CTX_CACHE_WIDTH;
                ((int32 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
            }
        }
    }

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        for (b16x8 = 0; b16x8 < 2; b16x8++) {
            pdir = mb_cache_ptr->b8pdir[2*b16x8];

            if(IS_DIR(pdir)) {
                s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4+b16x8*CTX_CACHE_WIDTH_X2, s_list);

                if (s_mvd_xy) {
                    s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2 *b16x8;
#ifndef _NEON_OPT_
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_ptr[2] = s_mvd_ptr[3] = s_mvd_xy;
                    s_mvd_ptr += CTX_CACHE_WIDTH;
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_ptr[2] = s_mvd_ptr[3] = s_mvd_xy;
#else
                    v32x4 = vmovq_n_s32 (s_mvd_xy);
                    vst1q_s32 (s_mvd_ptr+0, v32x4);
                    vst1q_s32 (s_mvd_ptr+CTX_CACHE_WIDTH, v32x4);
#endif
                }
            }
        }
    }

    return;
}

LOCAL void H264Dec_read_motionAndRefId_PMB8x16 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    DEC_STORABLE_PICTURE_T	**listX;
    int32 ref_frm_id_s32;
    uint32 b8x16;
    int32 pdir;
#ifdef _NEON_OPT_
    int32x2_t v32x2;
#endif
    int32 s_list;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        for (b8x16 = 0; b8x16 < 2; b8x16++) {
            pdir = mb_cache_ptr->b8pdir[b8x16];

            if(IS_DIR(pdir)) {
                listX = vo->g_list[s_list];

                if (mb_cache_ptr->read_ref_id_flag[s_list]) {
                    int32 ref_frm_id = vo->readRefFrame ((void *)vo, mb_cache_ptr, b8x16, s_list);

#if _H264_PROTECT_ & _LEVEL_HIGH_
                    if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1)) {
                        vo->error_flag |= ER_REF_FRM_ID;
                        vo->return_pos |= (1<<(29+b8x16));

                        return;
                    }
#endif
                    ref_frm_id_s32 = ref_frm_id * 0x0101;
                } else {
                    ref_frm_id_s32 = 0;
                }
            } else {
                ref_frm_id_s32 = LIST_NOT_USED;
            }

            if (ref_frm_id_s32) {// != 0
                s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ 2*b8x16;
                ((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
                s_ref_idx_ptr += CTX_CACHE_WIDTH;
                ((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
                s_ref_idx_ptr += CTX_CACHE_WIDTH;
                ((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
                s_ref_idx_ptr += CTX_CACHE_WIDTH;
                ((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
            }
        }
    }

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        for (b8x16 = 0; b8x16 < 2; b8x16++) {
            pdir = mb_cache_ptr->b8pdir[b8x16];

            if(IS_DIR(pdir)) {
                s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, CTX_CACHE_WIDTH_PLUS4+b8x16*2, s_list);

                if (s_mvd_xy) {
                    s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4+ 2 *b8x16;
#ifndef _NEON_OPT_
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
                    s_mvd_ptr += CTX_CACHE_WIDTH;
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
                    s_mvd_ptr += CTX_CACHE_WIDTH;
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
                    s_mvd_ptr += CTX_CACHE_WIDTH;
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
#else
                    v32x2 = vmov_n_s32 (s_mvd_xy);
                    vst1_s32 (s_mvd_ptr+0, v32x2 );
                    vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH_X1, v32x2);
                    vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH_X2, v32x2);
                    vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH_X3, v32x2);
#endif
                }
            }
        }
    }

    return;
}

LOCAL void H264Dec_read_motionAndRefId_P8x8 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    DEC_STORABLE_PICTURE_T	**listX;
    int32 ref_frm_id_s32;
    uint32 b8, cache_offset;
    int32 pdir;
#ifdef _NEON_OPT_
    int32x2_t v32x2;
#endif
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr_array[2];
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;
    uint32 s_row;

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        for (b8 = 0; b8 < 4; b8++) {
            if (mb_cache_ptr->b8mode[b8] == 0)	continue;	//direct 8x8

            pdir = mb_cache_ptr->b8pdir[b8];
            if(IS_DIR(pdir)) {
                listX = vo->g_list[s_list];

                if (mb_cache_ptr->read_ref_id_flag[s_list]) {
                    int32 ref_frm_id =  vo->readRefFrame ((void *)vo, mb_cache_ptr, b8, s_list);

#if _H264_PROTECT_ & _LEVEL_HIGH_
                    if (ref_frm_id >= (MAX_REF_FRAME_NUMBER+1)) {
                        vo->error_flag |= ER_REF_FRM_ID;
                        vo->return_pos1 |= (1<<(2+b8));

                        return;
                    }
#endif
                    ref_frm_id_s32 = ref_frm_id * 0x0101;
                } else {
                    ref_frm_id_s32 = 0;
                }
            } else {
                ref_frm_id_s32 = LIST_NOT_USED;
            }

            if (ref_frm_id_s32) {// != 0
                s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + CTX_CACHE_WIDTH_PLUS4+ CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
                ((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
                s_ref_idx_ptr += CTX_CACHE_WIDTH;
                ((int16 *)s_ref_idx_ptr)[0] = ref_frm_id_s32;
            }
        }
    }

    for (s_list = 0; s_list < vo->list_count; s_list++) {
        for (b8 = 0; b8 < 4; b8++) {
            if (mb_cache_ptr->b8mode[b8] == 0) {
                continue;
            }

            pdir = mb_cache_ptr->b8pdir[b8];
            if(IS_DIR(pdir)) {
                int32 blk_offset = CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);
                int32 sub_blk_id = CTX_CACHE_WIDTH_PLUS4+blk_offset;///{7, 9, 19, 21};

                cache_offset = CTX_CACHE_WIDTH_PLUS4 + blk_offset;
                s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

                switch(mb_cache_ptr->b8mode[b8]) {
                case PMB8X8_BLOCK8X8:
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id, s_list);
#ifndef _NEON_OPT_
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
                    s_mvd_ptr += CTX_CACHE_WIDTH;
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
#else
                    v32x2 = vmov_n_s32 (s_mvd_xy);
                    vst1_s32 (s_mvd_ptr+0, v32x2 );
                    vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH, v32x2 );
#endif

                    break;
                case PMB8X8_BLOCK8X4:
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id+0*CTX_CACHE_WIDTH, s_list);
#ifndef _NEON_OPT_
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
                    s_mvd_ptr += CTX_CACHE_WIDTH;
#else
                    v32x2 = vmov_n_s32 (s_mvd_xy);
                    vst1_s32 (s_mvd_ptr+0, v32x2 );
#endif

                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id+1*CTX_CACHE_WIDTH, s_list);
#ifndef _NEON_OPT_
                    s_mvd_ptr[0] = s_mvd_ptr[1] = s_mvd_xy;
#else
                    v32x2 = vmov_n_s32 (s_mvd_xy);
                    vst1_s32 (s_mvd_ptr+CTX_CACHE_WIDTH, v32x2 );
#endif

                    break;
                case PMB8X8_BLOCK4X8:
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id+0, s_list);
#if 1//ndef _NEON_OPT_		
                    s_mvd_ptr[0] = s_mvd_ptr[CTX_CACHE_WIDTH] = s_mvd_xy;
                    s_mvd_ptr ++;
#else
                    v32x2 = vset_lane_s32(s_mvd_xy, v32x2, 0);
#endif

                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id+1, s_list);
#if 1//ndef _NEON_OPT_				
                    s_mvd_ptr[0] = s_mvd_ptr[CTX_CACHE_WIDTH] = s_mvd_xy;
#else
                    v32x2 = vset_lane_s32(s_mvd_xy, v32x2, 1);
                    vst1_s32 (s_mvd_ptr, v32x2 );
                    vst1_s32 (s_mvd_ptr + CTX_CACHE_WIDTH, v32x2 );
#endif

                    break;
                case PMB8X8_BLOCK4X4:

                    ///0
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id, s_list);
                    s_mvd_ptr[0] = s_mvd_xy;

                    ///1
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id+1, s_list);
                    s_mvd_ptr[1] = s_mvd_xy;

                    sub_blk_id += CTX_CACHE_WIDTH;
                    s_mvd_ptr += CTX_CACHE_WIDTH;

                    ///2
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id, s_list);
                    s_mvd_ptr[0] = s_mvd_xy;

                    ///3
                    s_mvd_xy = vo->decode_mvd_xy((void *)vo, mb_cache_ptr, sub_blk_id+1, s_list);
                    s_mvd_ptr[1] = s_mvd_xy;

                    break;
                default:
                    break;
                }
            }
        }
    }

    return;
}

PUBLIC void H264Dec_read_motionAndRefId (H264DecContext *vo, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    switch (mb_info_ptr->mb_type) {
    case PMB16x16:
        H264Dec_read_motionAndRefId_PMB16x16 (vo, mb_cache_ptr);
        break;
    case PMB16x8:
        H264Dec_read_motionAndRefId_PMB16x8 (vo, mb_cache_ptr);
        break;
    case PMB8x16:
        H264Dec_read_motionAndRefId_PMB8x16 (vo, mb_cache_ptr);
        break;
    case P8x8:
        H264Dec_read_motionAndRefId_P8x8 (vo, mb_cache_ptr);
        break;
    default:
        break;
    }

    return;
}

int32 H264Dec_pred_motion (int8 *ref_idx_ptr, int16 *ref_mv_ptr, int32 blk_width)
{
    int32 ref_idx_a, ref_idx_b, ref_idx_c;
    int32 mv_a_x, mv_b_x, mv_c_x;
    int32 mv_a_y, mv_b_y, mv_c_y;
    int32 pred_mv;
    int32 ref_idx = ref_idx_ptr[0];
    int32 match_cnt;

    //fetch diagonal mv and reference frame index
    int32 blk_pos = (-CTX_CACHE_WIDTH + blk_width);
    ref_idx_c = ref_idx_ptr[blk_pos];

    if (ref_idx_c == PART_NOT_AVAIL) {
        ref_idx_c = ref_idx_ptr[-(CTX_CACHE_WIDTH+1)];
        mv_c_x = (ref_mv_ptr-(CTX_CACHE_WIDTH+1)*2)[0];
        mv_c_y = (ref_mv_ptr-(CTX_CACHE_WIDTH+1)*2)[1];
    } else {
        mv_c_x = (ref_mv_ptr+blk_pos*2)[0];
        mv_c_y = (ref_mv_ptr+blk_pos*2)[1];
    }

    //left block ref and mv
    ref_idx_a = ref_idx_ptr[-1];
    mv_a_x = (ref_mv_ptr-1*2)[0];
    mv_a_y = (ref_mv_ptr-1*2)[1];

    //top block ref and mv
    ref_idx_b = ref_idx_ptr[-CTX_CACHE_WIDTH];
    mv_b_x = (ref_mv_ptr-CTX_CACHE_WIDTH*2)[0];
    mv_b_y = (ref_mv_ptr-CTX_CACHE_WIDTH*2)[1];

    match_cnt = ((ref_idx == ref_idx_a) ? 1 : 0) + ((ref_idx == ref_idx_b) ? 1 : 0) + ((ref_idx == ref_idx_c) ? 1 : 0);

    if (match_cnt == 0) {
        if (!((ref_idx_b == PART_NOT_AVAIL) && (ref_idx_c == PART_NOT_AVAIL) && (ref_idx_a != PART_NOT_AVAIL))) {
            int32 tmp;

            tmp = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
            pred_mv = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
            pred_mv = ((pred_mv<<16)|(tmp&0xffff));
        } else {
            pred_mv = ((mv_a_y << 16) | (mv_a_x &0xffff));
        }
    } else if (match_cnt > 1) {
        int32 tmp;

        tmp = MEDIAN(mv_a_x, mv_b_x, mv_c_x);
        pred_mv = MEDIAN(mv_a_y, mv_b_y, mv_c_y);
        pred_mv = ((pred_mv<<16)|(tmp&0xffff));
    } else {
        if (ref_idx == ref_idx_a) {
            pred_mv = ((mv_a_y << 16) | (mv_a_x &0xffff));
        } else if (ref_idx == ref_idx_b) {
            pred_mv = ((mv_b_y << 16) | (mv_b_x &0xffff));
        } else {// (ref_idx == ref_idx_c)
            pred_mv = ((mv_c_y << 16) | (mv_c_x &0xffff));
        }
    }

    return pred_mv;
}

int32 H264Dec_pred16x8_motion (DEC_MB_CACHE_T *mb_cache_ptr, int32 blkIdx, int32 list)
{
    int32 blk_pos;
    int32 ref_id;
    int32 pred_mv;
    int8 *ref_idx_ptr;
    int16 *ref_mv_ptr;

    ref_mv_ptr = mb_cache_ptr->mv_cache[list];
    ref_idx_ptr = mb_cache_ptr->ref_idx_cache[list];
    ref_id = ref_idx_ptr[blkIdx];

    if (blkIdx == CTX_CACHE_WIDTH_PLUS4) {//the top 16x8 partition
        blk_pos = blkIdx - CTX_CACHE_WIDTH;

        if (ref_id == ref_idx_ptr[blk_pos]) {
            pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
            return pred_mv;
        }
    } else {
        blk_pos = blkIdx - 1;

        if (ref_id == ref_idx_ptr[blk_pos]) {
            pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
            return pred_mv;
        }
    }

    pred_mv = H264Dec_pred_motion (ref_idx_ptr + blkIdx,  ref_mv_ptr + 2*blkIdx, 4);

    return pred_mv;
}

int32 H264Dec_pred8x16_motion (DEC_MB_CACHE_T *mb_cache_ptr, int32 blkIdx, int32 list)
{
    int32 blk_pos;
    int32 ref_id;
    int32 pred_mv;
    int8 *ref_idx_ptr;
    int16 *ref_mv_ptr;

    ref_mv_ptr = mb_cache_ptr->mv_cache[list];
    ref_idx_ptr = mb_cache_ptr->ref_idx_cache[list];
    ref_id = ref_idx_ptr[blkIdx];

    if (blkIdx == CTX_CACHE_WIDTH_PLUS4) {//the left 8x16 partition
        blk_pos = blkIdx - 1;

        if (ref_id == ref_idx_ptr[blk_pos]) {
            pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
            return pred_mv;
        }
    } else {
        //diagonal block
        int32 ref_idx_c;

        blk_pos = blkIdx - CTX_CACHE_WIDTH + 2;
        ref_idx_c = ref_idx_ptr[blk_pos];

        if (ref_idx_c == PART_NOT_AVAIL) {
            blk_pos = blkIdx - (CTX_CACHE_WIDTH+1);
            ref_idx_c = ref_idx_ptr[blk_pos];
        }

        if (ref_id == ref_idx_c) {
            pred_mv = ((int32 *)ref_mv_ptr)[blk_pos];
            return pred_mv;
        }
    }

    pred_mv = H264Dec_pred_motion (ref_idx_ptr + blkIdx, ref_mv_ptr + 2*blkIdx, 2);

    return pred_mv;
}

//not reference
void H264Dec_FillNoRefList_8x8(H264DecContext *vo, uint32 b8, int32 b8pdir)
{
    if (vo->type != B_SLICE) return;

    if (b8pdir != 2) {
        uint32 cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2*(b8>>1)+2*(b8&1);

        int32 s_list = 1 - b8pdir;
        int8 *s_ref_idx_ptr;

        s_ref_idx_ptr = vo->g_mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;

        ((int16 *)s_ref_idx_ptr)[0] = LIST_NOT_USED;
        s_ref_idx_ptr += CTX_CACHE_WIDTH;
        ((int16 *)s_ref_idx_ptr)[0] = LIST_NOT_USED;
    }
    return;
}

PUBLIC void H264Dec_mv_prediction_P8x8 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    uint32 i;
    int8  *ref_idx_ptr[2];
    int32 b8mode;
    int32 ref_idx_b8_1[2], ref_idx_b8_3[2];
    int32 cache_offset;
    int8  b8pdir;

    for (i = 0; i < 2; i++) {
        ref_idx_ptr[i] = mb_cache_ptr->ref_idx_cache[i] + CTX_CACHE_WIDTH_PLUS4;

        ref_idx_b8_1[i] = ref_idx_ptr[i][2];
        ref_idx_ptr[i][2] = PART_NOT_AVAIL;
        ref_idx_b8_3[i] = ref_idx_ptr[i][CTX_CACHE_WIDTH_X2+2];
        ref_idx_ptr[i][CTX_CACHE_WIDTH_X2+2] = PART_NOT_AVAIL;
    }

    //8x8 0
    cache_offset = CTX_CACHE_WIDTH_PLUS4;
    b8mode = mb_cache_ptr->b8mode[0];
    b8pdir = (*vo->b8_mv_pred_func[b8mode])((void *)vo, mb_cache_ptr, cache_offset, 0);

    if ((vo->type == B_SLICE) && (b8pdir != 2)) {
        H264Dec_FillNoRefList_8x8(vo, 0, b8pdir);
    }

    //8x8 1
    cache_offset = CTX_CACHE_WIDTH_PLUS4+2;
    ref_idx_ptr[0][2] = ref_idx_b8_1[0];
    ref_idx_ptr[1][2] = ref_idx_b8_1[1];
    b8mode = mb_cache_ptr->b8mode[1];
    b8pdir = (*vo->b8_mv_pred_func[b8mode])((void *)vo, mb_cache_ptr, cache_offset, 1);

    if ((vo->type == B_SLICE) && (b8pdir != 2)) {
        H264Dec_FillNoRefList_8x8(vo, 1, b8pdir);
    }

    //8x8 2
    cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2;
    b8mode = mb_cache_ptr->b8mode[2];
    b8pdir = (*vo->b8_mv_pred_func[b8mode])((void *)vo, mb_cache_ptr, cache_offset, 2);

    if ((vo->type == B_SLICE) && (b8pdir != 2)) {
        H264Dec_FillNoRefList_8x8(vo, 2, b8pdir);
    }

    //8x8 3
    cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2 + 2;
    ref_idx_ptr[0] = mb_cache_ptr->ref_idx_cache[0] + cache_offset;
    ref_idx_ptr[1] = mb_cache_ptr->ref_idx_cache[1] + cache_offset;
    ref_idx_ptr[0][0] = ref_idx_b8_3[0];
    ref_idx_ptr[1][0] = ref_idx_b8_3[1];
    b8mode = mb_cache_ptr->b8mode[3];
    b8pdir = (*vo->b8_mv_pred_func[b8mode])((void *)vo, mb_cache_ptr, cache_offset, 3);

    if ((vo->type == B_SLICE) && (b8pdir != 2)) {
        H264Dec_FillNoRefList_8x8(vo, 3, b8pdir);
    }

    return;
}

//int mvscale[MAX_REF_FRAME_NUMBER];

#define JUDGE_MOVING_BLOCK	\
{\
	int32 condition0, condition1;\
\
	*ref_idx_cache_direct0 = fs->ref_idx_ptr[0][offset];\
	*ref_idx_cache_direct1 = fs->ref_idx_ptr[1][offset];\
	*ref_pic_id_cache_direct0++ = fs->ref_pic_id_ptr[0][offset];\
	*ref_pic_id_cache_direct1++ = fs->ref_pic_id_ptr[1][offset];\
	((int32 *)ref_mv_cache_direct0)[0]  = ((int32 *)(fs->mv_ptr[0]))[offset];\
	((int32 *)ref_mv_cache_direct1)[0]  = ((int32 *)(fs->mv_ptr[1]))[offset];\
\
	condition0 = (fs->is_long_term				|| \
				((ref_idx_cache_direct0[0] != 0)			|| \
				((ABS(ref_mv_cache_direct0[0])>>1) != 0) || \
				((ABS(ref_mv_cache_direct0[1])>>1) != 0))); \
\
	if (condition0){\
		condition1 = ((ref_idx_cache_direct0[0] != -1)		|| \
					(ref_idx_cache_direct1[0] != 0)			|| \
					((ABS(ref_mv_cache_direct1[0])>>1) != 0) || \
					((ABS(ref_mv_cache_direct1[1])>>1) != 0));\
		mb_cache_ptr->moving_block[blk4x4Idx] = condition1;\
	}else{\
		mb_cache_ptr->moving_block[blk4x4Idx] = 0;\
	}\
\
	ref_idx_cache_direct0++;\
	ref_idx_cache_direct1++;\
	ref_mv_cache_direct0 += 2;\
	ref_mv_cache_direct1 += 2;\
}

PUBLIC void H264Dec_direct_mv_spatial(void *img, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264DecContext *vo = (H264DecContext *)img;
    int fw_rFrameL, fw_rFrameU, fw_rFrameUL, fw_rFrameUR;
    int bw_rFrameL, bw_rFrameU, bw_rFrameUL, bw_rFrameUR;
    int32 fw_rFrame,bw_rFrame;
    int32 i, offset ,blk4x4Idx;
    int32 ref_frm_id_s32;
    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr_array[2];
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr_array[2];
    int32 *s_ref_mv_ptr;

    DEC_STORABLE_PICTURE_T	*fs = vo->g_list[1][0];
    int8  *ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
    int8  *ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
    int32 *ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
    int32 *ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
    int16 *ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
    int16 *ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

    if (!fs || !fs->mv_ptr[0] || !fs->mv_ptr[1]) {
        vo->error_flag |= ER_REF_FRM_ID;
        return;
    }

    for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++) {
        offset = vo->corner_map[blk4x4Idx];
        JUDGE_MOVING_BLOCK
    }

    for (i = 0; i < 2; i++) {
        s_ref_mv_ptr_array[i] = (int32 *)(mb_cache_ptr->mv_cache[i]) + CTX_CACHE_WIDTH_PLUS4;
        s_ref_idx_ptr_array[i] = mb_cache_ptr->ref_idx_cache[i] + CTX_CACHE_WIDTH_PLUS4;
    }

    fw_rFrameL = s_ref_idx_ptr_array[0][-1];
    fw_rFrameU = s_ref_idx_ptr_array[0][-CTX_CACHE_WIDTH];
    fw_rFrameUL = s_ref_idx_ptr_array[0][-CTX_CACHE_WIDTH-1];
    fw_rFrameUR = mb_cache_ptr->mb_avail_c ? (s_ref_idx_ptr_array[0][-CTX_CACHE_WIDTH+4]) : fw_rFrameUL;

    bw_rFrameL = s_ref_idx_ptr_array[1][-1];
    bw_rFrameU = s_ref_idx_ptr_array[1][-CTX_CACHE_WIDTH];
    bw_rFrameUL = s_ref_idx_ptr_array[1][-CTX_CACHE_WIDTH-1];
    bw_rFrameUR = mb_cache_ptr->mb_avail_c ? (s_ref_idx_ptr_array[1][-CTX_CACHE_WIDTH+4]) : bw_rFrameUL;

    fw_rFrame = (fw_rFrameL >= 0 && fw_rFrameU >= 0) ? mmin(fw_rFrameL,fw_rFrameU): mmax(fw_rFrameL,fw_rFrameU);
    fw_rFrame = (fw_rFrame >= 0 && fw_rFrameUR >= 0) ? mmin(fw_rFrame,fw_rFrameUR): mmax(fw_rFrame,fw_rFrameUR);
    mb_cache_ptr->fw_rFrame = fw_rFrame;

    bw_rFrame = (bw_rFrameL >= 0 && bw_rFrameU >= 0) ? mmin(bw_rFrameL,bw_rFrameU): mmax(bw_rFrameL,bw_rFrameU);
    bw_rFrame = (bw_rFrame >= 0 && bw_rFrameUR >= 0) ? mmin(bw_rFrame,bw_rFrameUR): mmax(bw_rFrame,bw_rFrameUR);
    mb_cache_ptr->bw_rFrame = bw_rFrame;

    mb_cache_ptr->direct_pdir = 0;
    if (fw_rFrame >= 0) {
        int32 tmp;
        s_ref_idx_ptr = s_ref_idx_ptr_array[0];
        s_ref_mv_ptr = s_ref_mv_ptr_array[0];
        tmp = s_ref_idx_ptr[0];
        s_ref_idx_ptr[0] = fw_rFrame;
        mb_cache_ptr->fw_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
        s_ref_idx_ptr[0] = tmp;
        mb_cache_ptr->direct_pdir++;
    }

    if (bw_rFrame >= 0) {
        int32 tmp;
        s_ref_idx_ptr = s_ref_idx_ptr_array[1];
        s_ref_mv_ptr = s_ref_mv_ptr_array[1];
        tmp = s_ref_idx_ptr[0];
        s_ref_idx_ptr[0] = bw_rFrame;
        mb_cache_ptr->bw_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
        s_ref_idx_ptr[0] = tmp;
        mb_cache_ptr->direct_pdir++;
    }
}

PUBLIC void H264Dec_direct_mv_temporal(void *img, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264DecContext *vo = (H264DecContext *)img;
    DEC_STORABLE_PICTURE_T	*fs = vo->g_list[1][0];
    int32 iref_max = mmin(vo->ref_count[0], vo->g_list_size[0]);
    int32 /*i,*/ offset, blk4x4Idx;
    int32 blk4x4_id;
    int8  *ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
    int8  *ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
    int32 *ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
    int32 *ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
    int16 *ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
    int16 *ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

    for (blk4x4Idx = 0; blk4x4Idx < 16; blk4x4Idx++) {
        offset = vo->corner_map[blk4x4Idx];

        *ref_idx_cache_direct0++ = fs->ref_idx_ptr[0][offset];
        *ref_idx_cache_direct1++ = fs->ref_idx_ptr[1][offset];
        *ref_pic_id_cache_direct0++ = fs->ref_pic_id_ptr[0][offset];
        *ref_pic_id_cache_direct1++ = fs->ref_pic_id_ptr[1][offset];
        ((int32 *)ref_mv_cache_direct0)[0]  = ((int32 *)(fs->mv_ptr[0]))[offset];
        ((int32 *)ref_mv_cache_direct1)[0]  = ((int32 *)(fs->mv_ptr[1]))[offset];

        ref_mv_cache_direct0 += 2;
        ref_mv_cache_direct1 += 2;
    }

    //////////////lsw for direct mode (temporal direct mode)
    ref_idx_cache_direct0 = mb_cache_ptr->ref_idx_cache_direct[0];
    ref_idx_cache_direct1 = mb_cache_ptr->ref_idx_cache_direct[1];
    ref_pic_id_cache_direct0 = mb_cache_ptr->ref_pic_id_cache_direct[0];
    ref_pic_id_cache_direct1 = mb_cache_ptr->ref_pic_id_cache_direct[1];
    ref_mv_cache_direct0 = mb_cache_ptr->mv_cache_direct[0];
    ref_mv_cache_direct1 = mb_cache_ptr->mv_cache_direct[1];

    for(blk4x4_id = 0; blk4x4_id < 16; blk4x4_id++) {
        int32 b8 = g_b8map[blk4x4_id] ;//((j>>1)<<1) + (i>>1);
        int32 refList = (ref_idx_cache_direct0[0]== -1 ? 1 : 0);

        if (mb_cache_ptr->b8mode[b8] == 0) {
            int32 col_ref_idx =  (refList == 1) ? ref_idx_cache_direct1[0] : ref_idx_cache_direct0[0];

            if (col_ref_idx == -1) {
                mb_cache_ptr->fw_rFrame = 0;
                mb_cache_ptr->fw_mv_xy = 0;
                mb_cache_ptr->bw_mv_xy = 0;
                ((int32 *)ref_mv_cache_direct0)[0] = 0;
                ((int32 *)ref_mv_cache_direct1)[0] = 0;
            } else {
                int mapped_idx=0;
                int32 ref_pic_id = ((refList == 0) ? ref_pic_id_cache_direct0[0] : ref_pic_id_cache_direct1[0]);
                int iref_max = mmin(vo->ref_count[0], vo->g_list_size[0]);
                int iref;

                for (iref = 0; iref < iref_max; iref++) {
                    // If the current MB is a frame MB and the colocated is from a field picture,
                    // then the co_located_ref_id may have been generated from the wrong value of
                    // frame_poc if it references it's complementary field, so test both POC values
                    if((vo->g_list[0][iref]->poc * 2) == ref_pic_id) {
                        mapped_idx=iref;
                        break;
                    } else {//! invalid index. Default to zero even though this case should not happen
                        mapped_idx = INVALID_REF_ID;
                    }
                }

                if (INVALID_REF_ID == mapped_idx) {
                    PRINTF("temporal direct error\ncolocated block has ref that is unavailable\n");
                    //	exit(-1);
                }

                if (vo->dist_scale_factor[mapped_idx] == 9999 || vo->g_list[0][mapped_idx]->is_long_term) {
                    if (refList == 0) {
                        mb_cache_ptr->fw_mv_xy = ((int32 *)ref_mv_cache_direct0)[0];
                    } else {
                        mb_cache_ptr->fw_mv_xy = ((int32 *)ref_mv_cache_direct1)[0];
                        ((int32 *)ref_mv_cache_direct0)[0] = ((int32 *)ref_mv_cache_direct1)[0];
                    }
                    mb_cache_ptr->bw_mv_xy = 0;
                    ((int32 *)ref_mv_cache_direct1)[0] = 0;
                } else {
                    int32 tmp1, tmp2;

                    int16 *ref_mv_ptr = ((refList == 0) ? ref_mv_cache_direct0 : ref_mv_cache_direct1);

                    tmp1 = (vo->dist_scale_factor[mapped_idx] * ref_mv_ptr[0] + 128) >> 8;
                    tmp2 = (vo->dist_scale_factor[mapped_idx] * ref_mv_ptr[1] + 128) >> 8;
                    mb_cache_ptr->fw_mv_xy = ((tmp2<<16) | (tmp1 & 0xffff));
                    tmp1 -= ref_mv_ptr[0];
                    tmp2 -= ref_mv_ptr[1];
                    mb_cache_ptr->bw_mv_xy = ((tmp2<<16) |(tmp1 & 0xffff));
                    ((int32 *)ref_mv_cache_direct0)[0] = mb_cache_ptr->fw_mv_xy;
                    ((int32 *)ref_mv_cache_direct1)[0] = mb_cache_ptr->bw_mv_xy;
                }

                mb_cache_ptr->fw_rFrame = mapped_idx;
            }

            mb_cache_ptr->bw_rFrame = 0;
            ref_idx_cache_direct0[0] = mb_cache_ptr->fw_rFrame;
            ref_idx_cache_direct1[0] = mb_cache_ptr->bw_rFrame;
        }

        ref_idx_cache_direct0 ++;
        ref_idx_cache_direct1 ++;
        ref_pic_id_cache_direct0 ++;
        ref_pic_id_cache_direct1 ++;
        ref_mv_cache_direct0 += 2;
        ref_mv_cache_direct1 += 2;
    }

    mb_cache_ptr->direct_pdir = 2;
}

//weighted prediction
typedef struct WP_info_tag
{
    uint8 *b8map_ptr;
    uint8 *clip_data_ptr;
    int8 *b8pdir_ptr;
    int16 *g_wp_offset_ptr;
    int16 *g_wbp_weight_ptr;
    int16 *g_wp_weight_ptr;
} DEC_WP_MEM_INFO_T;

LOCAL void H264Dec_Config_WP_info(H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
#if 1//WIN32
    uint32 row, col;
    int32 blk4x4_id;
    int32 b8;
    int32 comp;
    int32 pdir;
    int32 offset[2];
    int32 weight[2];
    int32 mc_ref_idx[2];
    int8 *ref_idx_ptr0;
    int8 *ref_idx_ptr1;
    int32 pitch;
    int32 shift;
    int32 blk_size;
    int32 pix_offset;
    uint8 * src_ptr[2];
    int i, j;
    int32 tmp, fw, bw;

    blk4x4_id = 0;

    for (comp = 0; comp < 3; comp++) {
        blk4x4_id = 0;
        ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
        ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache[1] + CTX_CACHE_WIDTH_PLUS4;
        pitch = (comp == 0) ? MB_SIZE : MB_CHROMA_SIZE;
        shift = (comp == 0) ? vo->luma_log2_weight_denom : vo->chroma_log2_weight_denom;
        blk_size = (comp == 0) ? 4 : 2;

        for (row = 0; row < 4; row++) {
            for (col = 0; col < 4; col++) {
                b8 = g_b8map[blk4x4_id];
                pdir = mb_cache_ptr->b8pdir[b8];
                mc_ref_idx[0] = ref_idx_ptr0[col];
                mc_ref_idx[1] = ref_idx_ptr1[col];

                if (0 == comp) {//y
                    pix_offset = 64 * row + 4 * col;
                    src_ptr[0] = mb_cache_ptr->pred_cache[0].pred_Y + pix_offset;
                    src_ptr[1] = mb_cache_ptr->pred_cache[1].pred_Y + pix_offset;
                } else {//uv
                    pix_offset = 16 * row + 2 * col;
                    src_ptr[0] = mb_cache_ptr->pred_cache[0].pred_UV[comp -1] + pix_offset;
                    src_ptr[1] = mb_cache_ptr->pred_cache[1].pred_UV[comp -1] + pix_offset;
                }

                if (2 == pdir) {//bi-dir
                    offset[0] = vo->g_wp_offset[0][mc_ref_idx[0]][comp];
                    offset[1] = vo->g_wp_offset[1][mc_ref_idx[1]][comp];
                    weight[0] = vo->g_wbp_weight[0][mc_ref_idx[0]][mc_ref_idx[1]][comp];
                    weight[1] = vo->g_wbp_weight[1][mc_ref_idx[0]][mc_ref_idx[1]][comp];

                    for (i = 0; i < blk_size; i++) {
                        for (j = 0; j < blk_size; j++) {
                            fw = src_ptr[0][j];
                            bw = src_ptr[1][j];

                            tmp = ((fw*weight[0] + bw*weight[1] + (1<<shift)) >>(shift+1)) + ((offset[0] + offset[1] +1 )>>1);
                            src_ptr[0][j] = IClip(0, 255, tmp);
                        }
                        src_ptr[0] += pitch;
                        src_ptr[1] += pitch;
                    }
                } else {//list 0 or list 1
                    offset[pdir] = vo->g_wp_offset[pdir][mc_ref_idx[pdir]][comp];
                    weight[pdir] = vo->g_wp_weight[pdir][mc_ref_idx[pdir]][comp];

                    for (i = 0; i < blk_size; i++) {
                        for (j = 0; j < blk_size; j++) {
                            tmp = src_ptr[pdir][j];
                            if (shift >= 1) {
                                tmp = ((tmp * weight[pdir] + (1<<(shift-1))) >> shift) + offset[pdir];
                            } else {
                                tmp = tmp * weight[pdir] + offset[pdir];
                            }
                            src_ptr[0][j] = IClip(0, 255, tmp);
                        }
                        src_ptr[0] += pitch;
                        src_ptr[1] += pitch;
                    }
                }

                blk4x4_id++;
            }

            ref_idx_ptr0 += CTX_CACHE_WIDTH;
            ref_idx_ptr1 += CTX_CACHE_WIDTH;
        }
    }
#else
    int32 shift;
    int8 *ref_idx_ptr0;
    uint8 *src_ptr0;
    DEC_WP_MEM_INFO_T mem_t;

    shift = img_ptr->luma_log2_weight_denom;
    ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
    mem_t.b8map_ptr = (uint8 *)g_b8map;
//	mem_t.clip_data_ptr = g_clip_ptr
    mem_t.b8pdir_ptr = mb_cache_ptr->b8pdir;
    mem_t.g_wp_offset_ptr = (int16 *)img_ptr->g_wp_offset;
    mem_t.g_wbp_weight_ptr = (int16 *)img_ptr->g_wbp_weight;
    mem_t.g_wp_weight_ptr = (int16 *)img_ptr->g_wp_weight;

    //y component
    src_ptr0 = mb_cache_ptr->pred_cache[0].pred_Y;
    H264Dec_wp_y(ref_idx_ptr0, src_ptr0, shift, &mem_t);

    //u component
    shift = img_ptr->chroma_log2_weight_denom;
    src_ptr0 = mb_cache_ptr->pred_cache[0].pred_UV[0];
    mem_t.g_wp_offset_ptr += 1;
    mem_t.g_wbp_weight_ptr += 1;
    mem_t.g_wp_weight_ptr += 1;
    H264Dec_wp_c(ref_idx_ptr0, src_ptr0, shift, &mem_t);

    //v component
    src_ptr0 = mb_cache_ptr->pred_cache[0].pred_UV[1];
    mem_t.g_wp_offset_ptr += 1;
    mem_t.g_wbp_weight_ptr += 1;
    mem_t.g_wp_weight_ptr += 1;
    H264Dec_wp_c(ref_idx_ptr0, src_ptr0, shift, &mem_t);
#endif
}

//for one 8x8 block in one MB
void H264Dec_Config_8x8MC_info(H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
    uint32 b8_cache_offset;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;

    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;

    b8_cache_offset = CTX_CACHE_WIDTH_PLUS4 + g_b8_offset[b8];

    s_dir = 0;
    do {
        s_list = (b8pdir == 2) ? s_dir : b8pdir;
        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+b8_cache_offset;
        s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + b8_cache_offset;

        mv_xy = s_ref_mv_ptr[0];
        mv_x = ((mv_xy << 16) >> 16);
        mv_y = (mv_xy >> 16);

        list = vo->g_list[s_list];
        pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
        if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
            vo->error_flag |= ER_REF_FRM_ID;
            vo->return_pos2 |= (1<<15);
            return;
        }
#endif
        H264Dec_mc_8x8 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8);

        s_dir++;
    } while(s_dir < b8pdir);

    return;
}

//for four 4x4 subblock in one 8x8 block
void H264Dec_Config_4x4MC_info(H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr, int32 b8, int32 b8pdir)
{
    uint32 b4;
    uint32 b8_cache_offset;
    uint32 b4_cache_offset;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint8 ** pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;

    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;

    b8_cache_offset = CTX_CACHE_WIDTH_PLUS4 + g_b8_offset[b8];
    for(b4 = 0; b4 < 4; b4++) {
        b4_cache_offset = b8_cache_offset + g_b4_offset[b4];

        s_dir = 0;
        do {
            s_list = (b8pdir == 2) ? s_dir : b8pdir;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+b4_cache_offset;
            s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + b4_cache_offset;

            mv_xy = s_ref_mv_ptr[0];
            mv_x = ((mv_xy << 16) >> 16);
            mv_y = (mv_xy >> 16);

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<16);
                return;
            }
#endif

            H264Dec_mc_4x4 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b4);

            s_dir++;
        } while(s_dir < b8pdir);
    }

    return;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_spatial (void *img, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 mv_xy;
    int32 use_8x8mc;
    int32 use_4x4mc[4] = {0,0,0,0};
    int32 b8;
    uint32 row;
    int32 b8pdir = 0;
    int32 fw_rFrame,bw_rFrame;

    uint32 cache_offset;
    int32 mv_x, mv_y;
    uint8 ** pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;

    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;

    s_dir = 0;

    fw_rFrame = mb_cache_ptr->fw_rFrame;
    bw_rFrame = mb_cache_ptr->bw_rFrame;

    use_8x8mc = 0;
    if (fw_rFrame >= 0) {
        mv_xy  = mb_cache_ptr->fw_mv_xy;

        //set mv cache
        if (mv_xy)	{//curr mv context has been set to value 0 in fill_mb func.
            uint32 col;
            int32 condition = (fw_rFrame || (vo->g_list[1][0]->is_long_term)) ? 0 : 1;
            int8 *moving_block = mb_cache_ptr->moving_block;

            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + CTX_CACHE_WIDTH_PLUS4;
            for (row = 0; row < 4; row++) {
                for (col = 0; col < 4; col++) {
                    b8 = ((row>>1)<<1)+(col>>1);
                    if  (condition  && (!moving_block[col])) {
                        use_8x8mc = 1;
                        use_4x4mc[b8] = 1;
                    } else {
                        s_ref_mv_ptr[col] = mv_xy;
                    }
                }

                moving_block += 4;
                s_ref_mv_ptr += CTX_CACHE_WIDTH;
            }
        }

        b8pdir = 0;
        s_dir++;
    }

    if (bw_rFrame >= 0) {
        mv_xy  = mb_cache_ptr->bw_mv_xy;

        //set mv cache
        if (mv_xy) {//curr mv context has been set to value 0 in fill_mb func.
            uint32 col;
            int32 condition = (bw_rFrame || (vo->g_list[1][0]->is_long_term)) ? 0 : 1;
            int8 *moving_block = mb_cache_ptr->moving_block;

            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[1])+ CTX_CACHE_WIDTH_PLUS4;
            for (row = 0; row < 4; row++) {
                for (col = 0; col < 4; col++) {
                    b8 = ((row>>1)<<1)+(col>>1);
                    if  (condition  && (!moving_block[col])) {
                        use_8x8mc = 1;
                        use_4x4mc[b8] = 1;
                    } else {
                        s_ref_mv_ptr[col] = mv_xy;
                    }
                }

                moving_block += 4;
                s_ref_mv_ptr += CTX_CACHE_WIDTH;
            }
        }

        b8pdir = 1;
        s_dir++;
    } else {
        // fw < 0 and bw < 0
        if (fw_rFrame < 0) {
            s_dir = 2;
            fw_rFrame = bw_rFrame = 0;
        }
    }

    //save ref_idx to mb_cache.
    if (fw_rFrame >= 0) {
        uint32 ref_u32 = fw_rFrame * 0x01010101;

        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0];
        ((uint32 *)s_ref_idx_ptr)[4] =
            ((uint32 *)s_ref_idx_ptr)[7] =
                ((uint32 *)s_ref_idx_ptr)[10] =
                    ((uint32 *)s_ref_idx_ptr)[13] = ref_u32;
    }

    if (bw_rFrame >= 0) {
        uint32 ref_u32 = bw_rFrame * 0x01010101;

        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1];
        ((uint32 *)s_ref_idx_ptr)[4] =
            ((uint32 *)s_ref_idx_ptr)[7] =
                ((uint32 *)s_ref_idx_ptr)[10] =
                    ((uint32 *)s_ref_idx_ptr)[13] = ref_u32;
    }

    b8pdir = (s_dir == 2) ? 2 : b8pdir;
    ((int32 *)mb_cache_ptr->b8pdir)[0] = b8pdir * 0x01010101;

    //configure mv command
    if (!use_8x8mc) {
        s_dir = 0;
        do {
            s_list = (b8pdir == 2) ? s_dir : b8pdir;
            cache_offset = CTX_CACHE_WIDTH_PLUS4;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
            s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
            mv_xy = s_ref_mv_ptr[0];

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<17);
                return 0;
            }
#endif
            mv_x = ((mv_xy << 16) >> 16);
            mv_y = (mv_xy >> 16);
            H264Dec_mc_16x16 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y);

            s_dir++;
        } while(s_dir < b8pdir);
    } else {
        for (b8 = 0; b8 < 4; b8++) {
            if (use_4x4mc[b8]) {
                H264Dec_Config_4x4MC_info(vo, mb_cache_ptr, b8, b8pdir);
            } else {
                H264Dec_Config_8x8MC_info(vo, mb_cache_ptr, b8, b8pdir);
            }
        }
    }

    return b8pdir;
}

PUBLIC int8 H264Dec_mv_prediction_skipped_bslice_temporal (void *img, DEC_MB_CACHE_T *mb_cache_ptr)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 use_8x8mc;
    int32 use_4x4mc[4] = {0, 0, 0, 0};
    int32 b4, offset_b4;
    int32 b8;
    int32 b8pdir;
    int32 offset;
    int8  *ref_idx_ptr0, *ref_idx_ptr1;
    int32 *ref_mv_ptr0, *ref_mv_ptr1;

    int32 mv_xy;
    uint32 cache_offset;
    int32 mv_x, mv_y;
    uint8 ** pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;

    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;

    //save mv and ref_idx to mb_cache
    for (s_list = 0; s_list < 2; s_list++) {
        int32  *ref_idx_src_ptr = (int32 *)(mb_cache_ptr->ref_idx_cache_direct[s_list]);
        int32  *ref_idx_dst_ptr = (int32 *)(mb_cache_ptr->ref_idx_cache[s_list]);
        int32  *ref_mv_src_ptr = (int32 *)(mb_cache_ptr->mv_cache_direct[s_list]);
        int32  *ref_mv_dst_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + CTX_CACHE_WIDTH_PLUS4;
        int32 row;

        ref_idx_dst_ptr[4] = ref_idx_src_ptr[0];
        ref_idx_dst_ptr[7] = ref_idx_src_ptr[1];
        ref_idx_dst_ptr[10] = ref_idx_src_ptr[2];
        ref_idx_dst_ptr[13] = ref_idx_src_ptr[3];

        for (row = 4 ; row > 0; row--) {
            ref_mv_dst_ptr[0] =  ref_mv_src_ptr[0];
            ref_mv_dst_ptr[1] =  ref_mv_src_ptr[1];
            ref_mv_dst_ptr[2] =  ref_mv_src_ptr[2];
            ref_mv_dst_ptr[3] =  ref_mv_src_ptr[3];

            ref_mv_src_ptr += 4;
            ref_mv_dst_ptr += CTX_CACHE_WIDTH;
        }
    }

    ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache_direct[0];
    ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache_direct[1];
    ref_mv_ptr0 = (int32*)(mb_cache_ptr->mv_cache_direct[0]);
    ref_mv_ptr1 = (int32*)(mb_cache_ptr->mv_cache_direct[1]);

    for (b8 = 0 ; b8 < 4; b8++) {
        offset = ((b8>>1)<<3)+((b8&1)<<1);

        if (vo->g_active_sps_ptr->direct_8x8_inference_flag == 0) {
            for (b4=1 ; b4 < 4; b4++) {
                offset_b4 = ((b4>>1)<<2)+(b4&1);

                if(ref_idx_ptr0[offset+offset_b4]!=ref_idx_ptr0[offset]
                        || ref_idx_ptr1[offset+offset_b4]!=ref_idx_ptr1[offset]
                        || ref_mv_ptr0[offset+offset_b4]!= ref_mv_ptr0[offset]
                        || ref_mv_ptr1[offset+offset_b4]!= ref_mv_ptr1[offset]) {
                    use_4x4mc[b8] = 1;
                    use_8x8mc = 1;
                    break;
                }
            }
        }

        if (use_4x4mc[b8] == 0 && b8 != 0) {
            if(ref_idx_ptr0[0]!=ref_idx_ptr0[offset]
                    || ref_idx_ptr1[0]!=ref_idx_ptr1[offset]
                    || ref_mv_ptr0[0]!= ref_mv_ptr0[offset]
                    || ref_mv_ptr1[0]!= ref_mv_ptr1[offset]) {
                use_8x8mc = 1;
            }
        }
    }

    s_dir = 2;
    b8pdir = 2;
    ((int32 *)mb_cache_ptr->b8pdir)[0] = b8pdir * 0x01010101;

    //configure mv command
    if (!use_8x8mc) {
        s_dir = 0;
        do {
            s_list = (b8pdir == 2) ? s_dir : b8pdir;
            cache_offset = CTX_CACHE_WIDTH_PLUS4;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
            s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
            mv_xy = s_ref_mv_ptr[0];

            mv_x = ((mv_xy << 16) >> 16);
            mv_y = (mv_xy >> 16);

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<18);
                return 0;
            }
#endif
            H264Dec_mc_16x16 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y);

            s_dir++;
        } while(s_dir < b8pdir);
    } else {
        for (b8 = 0; b8 < 4; b8++) {
            if (use_4x4mc[b8]) {
                H264Dec_Config_4x4MC_info(vo, mb_cache_ptr, b8, b8pdir);
            } else {
                H264Dec_Config_8x8MC_info(vo, mb_cache_ptr, b8, b8pdir);
            }
        }
    }

    return b8pdir;
}

//pred_mv = mv_y<<16 | mv_x
LOCAL void H264Dec_mv_prediction_skipped_pslice (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 pred_mv;
    int32 mv_xy;
    int32 mv_x, mv_y;
    int32 top_ref_idx, left_ref_idx;
    uint8 ** pRefFrame;
    DEC_STORABLE_PICTURE_T ** list = vo->g_list[0];
    int32 pdir = mb_cache_ptr->b8pdir[0];
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
#ifdef _NEON_OPT_
    int32x4_t v32x4;
#endif

    s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0] + CTX_CACHE_WIDTH_PLUS4;
    s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + CTX_CACHE_WIDTH_PLUS4;
    top_ref_idx = s_ref_idx_ptr[-CTX_CACHE_WIDTH];
    left_ref_idx = s_ref_idx_ptr[-1];

    if ((top_ref_idx == PART_NOT_AVAIL) || (left_ref_idx == PART_NOT_AVAIL) ||
            ((top_ref_idx == 0) && (s_ref_mv_ptr[-CTX_CACHE_WIDTH] == 0)) ||
            ((left_ref_idx == 0) && (s_ref_mv_ptr[-1] == 0))) {
        mv_xy = 0;
    } else {
        pred_mv = H264Dec_pred_motion(s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);

#if defined(H264_BIG_ENDIAN)
        mv_x = ((pred_mv << 16) >> 16);
        mv_y = (pred_mv >> 16);
        mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
        mv_xy  = pred_mv;
#endif
    }

    //set mv cache
    if (mv_xy)	{//curr mv context has been set to value 0 in fill_mb func.
#ifndef _NEON_OPT_
        s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;
        s_ref_mv_ptr += CTX_CACHE_WIDTH;
        s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;
        s_ref_mv_ptr += CTX_CACHE_WIDTH;
        s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;
        s_ref_mv_ptr += CTX_CACHE_WIDTH;
        s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;
#else
        v32x4 = vmovq_n_s32 (mv_xy);
        vst1q_s32 (s_ref_mv_ptr+0, v32x4);
        vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x4);
        vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X2, v32x4);
        vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X3, v32x4);
#endif
    }

    mv_x = ((mv_xy << 16) >> 16);
    mv_y = (mv_xy >> 16);

    pRefFrame = list[0]->imgYUV;
#if defined(CTS_PROTECT)
    if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
        vo->error_flag |= ER_REF_FRM_ID;
        vo->return_pos2 |= (1<<19);
        return;
    }
#endif

    H264Dec_mc_16x16 (vo, mb_cache_ptr->pred_cache[0].pred_Y, pRefFrame, mv_x, mv_y);

    return;
}

LOCAL void H264Dec_mv_prediction_PMB16x16 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint32 row;
    uint32 cache_offset;
    int32 pdir = mb_cache_ptr->b8pdir[0];
    uint8 ** pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;
#ifdef _NEON_OPT_
    int32x4_t v32x4;
#endif

    s_dir = 0;
    do {
        s_list = (pdir == 2) ? s_dir : pdir;
        cache_offset = CTX_CACHE_WIDTH_PLUS4;
        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
        s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
        s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

        pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 4);
        pred_mv_x = ((pred_mv_xy << 16) >> 16);
        pred_mv_y = (pred_mv_xy >> 16);

        s_mvd_xy = s_mvd_ptr[0];
        mvd_x = ((s_mvd_xy << 16) >> 16);
        mvd_y = (s_mvd_xy >> 16);

        mv_x = mvd_x + pred_mv_x;
        mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
        mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
        mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
#endif

        //set mv cache
#ifndef _NEON_OPT_
        for (row = 4; row > 0; row--) {
            s_ref_mv_ptr[0] = s_ref_mv_ptr[1] =
                                  s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;

            s_ref_mv_ptr += CTX_CACHE_WIDTH;
        }
#else
        v32x4 = vmovq_n_s32 (mv_xy);
        vst1q_s32 (s_ref_mv_ptr+0, v32x4);
        vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x4);
        vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X2, v32x4);
        vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X3, v32x4);
#endif

        /*motion compensation*/
        list = vo->g_list[s_list];
        pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
        if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
            vo->error_flag |= ER_REF_FRM_ID;
            vo->return_pos2 |= (1<<20);
            return;
        }
#endif

        H264Dec_mc_16x16 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y);

        s_dir++;
    } while (s_dir < pdir);

    return;
}

LOCAL void H264Dec_mv_prediction_PMB16x8 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint32 row, b16x8;
    uint32 cache_offset;
    int32 pdir;
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;
#ifdef _NEON_OPT_
    int32x4_t v32x4;
#endif

    for (b16x8 = 0; b16x8 < 2; b16x8++) {
        pdir = mb_cache_ptr->b8pdir[2*b16x8];

        s_dir = 0;
        do {
            s_list = (pdir == 2) ? s_dir : pdir;
            cache_offset = CTX_CACHE_WIDTH_PLUS4 + CTX_CACHE_WIDTH_X2 *b16x8;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
            s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

            pred_mv_xy = H264Dec_pred16x8_motion (mb_cache_ptr, cache_offset, s_list);
            pred_mv_x = ((pred_mv_xy << 16) >> 16);
            pred_mv_y = (pred_mv_xy >> 16);

            s_mvd_xy = s_mvd_ptr[0];
            mvd_x = ((s_mvd_xy << 16) >> 16);
            mvd_y = (s_mvd_xy >> 16);

            mv_x = mvd_x + pred_mv_x;
            mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
            mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
            mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
#endif

            //set mv cache
#ifndef _NEON_OPT_
            for (row = 2; row > 0; row--) {
                s_ref_mv_ptr[0] = s_ref_mv_ptr[1] =
                                      s_ref_mv_ptr[2] = s_ref_mv_ptr[3] = mv_xy;

                s_ref_mv_ptr += CTX_CACHE_WIDTH;
            }
#else
            v32x4 = vmovq_n_s32 (mv_xy);
            vst1q_s32 (s_ref_mv_ptr+0, v32x4);
            vst1q_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x4);
#endif

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<21);
                return;
            }
#endif

            H264Dec_mc_16x8 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b16x8);

            s_dir++;
        } while(s_dir < pdir);
    }

    return;
}

LOCAL void H264Dec_mv_prediction_PMB8x16 (H264DecContext *vo, DEC_MB_CACHE_T *mb_cache_ptr)
{
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint32 row, b8x16;
    uint32 cache_offset;
    int32 pdir = mb_cache_ptr->b8pdir[0];
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;
#ifdef _NEON_OPT_
    int32x2_t v32x2;
#endif

    for (b8x16 = 0; b8x16 < 2; b8x16++) {
        pdir = mb_cache_ptr->b8pdir[b8x16];

        s_dir = 0;
        do {
            s_list = (pdir == 2) ? s_dir : pdir;
            cache_offset = CTX_CACHE_WIDTH_PLUS4 + 2*b8x16;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list] + cache_offset;
            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
            s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

            pred_mv_xy = H264Dec_pred8x16_motion (mb_cache_ptr, cache_offset, s_list);
            pred_mv_x = ((pred_mv_xy << 16) >> 16);
            pred_mv_y = (pred_mv_xy >> 16);

            s_mvd_xy = s_mvd_ptr[0];
            mvd_x = ((s_mvd_xy << 16) >> 16);
            mvd_y = (s_mvd_xy >> 16);

            mv_x = mvd_x + pred_mv_x;
            mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
            mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
            mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
#endif

            //set mv cache
            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
#ifndef _NEON_OPT_
            for (row = 0; row < 4; row++) {
                s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
                s_ref_mv_ptr += CTX_CACHE_WIDTH;
            }
#else
            v32x2 = vmov_n_s32 (mv_xy);
            vst1_s32 (s_ref_mv_ptr+0, v32x2);
            vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x2);
            vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X2, v32x2);
            vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X3, v32x2);
#endif

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<22);
                return;
            }
#endif

            H264Dec_mc_8x16 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8x16);

            s_dir++;
        } while(s_dir < pdir);
    }

    return;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_8x8 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    int32 pdir = mb_cache_ptr->b8pdir[b8];
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;
#ifdef _NEON_OPT_
    int32x2_t v32x2;
#endif

    s_dir = 0;
    do {
        s_list = (pdir == 2) ? s_dir : pdir;
        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
        s_ref_mv_ptr = (int32*)(mb_cache_ptr->mv_cache[s_list]) + cache_offset;
        s_mvd_ptr =(int32*)(mb_cache_ptr->mvd_cache[s_list]) + cache_offset;

        pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 2);
        pred_mv_x = ((pred_mv_xy << 16) >> 16);
        pred_mv_y = (pred_mv_xy >> 16);

        s_mvd_xy = s_mvd_ptr[0];
        mvd_x = ((s_mvd_xy << 16) >> 16);
        mvd_y = (s_mvd_xy >> 16);

        mv_x = mvd_x + pred_mv_x;
        mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
        mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
        mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
#endif

        //set mv cache
#ifndef _NEON_OPT_
        s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
        s_ref_mv_ptr += CTX_CACHE_WIDTH;
        s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
#else
        v32x2 = vmov_n_s32 (mv_xy);
        vst1_s32 (s_ref_mv_ptr+0, v32x2);
        vst1_s32 (s_ref_mv_ptr+CTX_CACHE_WIDTH_X1, v32x2);
#endif

        list = vo->g_list[s_list];
        pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
        if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
            vo->error_flag |= ER_REF_FRM_ID;
            vo->return_pos2 |= (1<<23);
            return 0;
        }
#endif

        H264Dec_mc_8x8 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8);

        s_dir++;
    } while(s_dir < pdir);

    return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_8x4 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint32 b8x4;
    uint32 offset;
    int32 pdir = mb_cache_ptr->b8pdir[b8];
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;
#ifdef _NEON_OPT_
    int32x2_t v32x2;
#endif

    for (b8x4 = 0; b8x4 < 2; b8x4++) {
        offset = cache_offset + b8x4 * CTX_CACHE_WIDTH;

        s_dir = 0;
        do {
            s_list = (pdir == 2) ? s_dir : pdir;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+offset;
            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + offset;
            s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + offset;

            pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 2);
            pred_mv_x = ((pred_mv_xy << 16) >> 16);
            pred_mv_y = (pred_mv_xy >> 16);

            s_mvd_xy = s_mvd_ptr[0];
            mvd_x = ((s_mvd_xy << 16) >> 16);
            mvd_y = (s_mvd_xy >> 16);

            mv_x = mvd_x + pred_mv_x;
            mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
            mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
            mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
#endif

#ifndef _NEON_OPT_
            s_ref_mv_ptr[0] = s_ref_mv_ptr[1] = mv_xy;
#else
            v32x2 = vmov_n_s32 (mv_xy);
            vst1_s32 (s_ref_mv_ptr+0, v32x2);
#endif

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<24);
                return 0;
            }
#endif

            H264Dec_mc_8x4 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b8x4);

            s_dir++;
        } while(s_dir < pdir);
    }

    return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x8 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint32 b4x8;
    uint32 offset;
    int32 pdir = mb_cache_ptr->b8pdir[b8];
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;

    for (b4x8 = 0; b4x8 < 2; b4x8++) {
        offset = cache_offset + b4x8;

        s_dir = 0;
        do {
            s_list = (pdir == 2) ? s_dir : pdir;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+offset;
            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + offset;
            s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + offset;

            pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 1);
            pred_mv_x = ((pred_mv_xy << 16) >> 16);
            pred_mv_y = (pred_mv_xy >> 16);

            s_mvd_xy = s_mvd_ptr[0];
            mvd_x = ((s_mvd_xy << 16) >> 16);
            mvd_y = (s_mvd_xy >> 16);

            mv_x = mvd_x + pred_mv_x;
            mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
            mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
            mv_xy  = (mv_y << 16) | (mv_x & 0xffff);
#endif

            s_ref_mv_ptr[0] = s_ref_mv_ptr[CTX_CACHE_WIDTH] = mv_xy;

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<25);
                return 0;
            }
#endif
            H264Dec_mc_4x8 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b4x8);

            s_dir++;
        } while(s_dir < pdir);
    }

    return pdir;
}

PUBLIC int32 H264Dec_mv_prediction_P8x8_4x4 (void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 pred_mv_x, pred_mv_y, pred_mv_xy;
    int32 mvd_x, mvd_y;
    int32 mv_x, mv_y;
    int32 mv_xy;
    uint32 b4;
    uint32 offset;
    int32 pdir = mb_cache_ptr->b8pdir[b8];
    uint8 **pRefFrame;
    DEC_STORABLE_PICTURE_T ** list;
    int32 s_list;
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;
    int32 *s_mvd_ptr;
    int32 s_mvd_xy;

    for (b4 = 0; b4 < 4; b4++) {
        offset = cache_offset + (b4>>1) * CTX_CACHE_WIDTH + (b4&1);

        s_dir = 0;
        do {
            s_list = (pdir == 2) ? s_dir : pdir;
            s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[s_list]+offset;
            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list]) + offset;
            s_mvd_ptr = (int32 *)(mb_cache_ptr->mvd_cache[s_list]) + offset;

            pred_mv_xy = H264Dec_pred_motion (s_ref_idx_ptr, (int16 *)s_ref_mv_ptr, 1);
            pred_mv_x = ((pred_mv_xy << 16) >> 16);
            pred_mv_y = (pred_mv_xy >> 16);

            s_mvd_xy = s_mvd_ptr[0];
            mvd_x = ((s_mvd_xy << 16) >> 16);
            mvd_y = (s_mvd_xy >> 16);

            mv_x = mvd_x + pred_mv_x;
            mv_y = mvd_y + pred_mv_y;

#if defined(H264_BIG_ENDIAN)
            mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
            mv_xy  =  (mv_y << 16) | (mv_x & 0xffff);
#endif
            s_ref_mv_ptr[0] = mv_xy;

            list = vo->g_list[s_list];
            pRefFrame = list[s_ref_idx_ptr[0]]->imgYUV;
#if defined(CTS_PROTECT)
            if ((pRefFrame == NULL) || (pRefFrame[0] == NULL) ||(pRefFrame[1] == NULL) ) {
                vo->error_flag |= ER_REF_FRM_ID;
                vo->return_pos2 |= (1<<26);
                return 0;
            }
#endif
            H264Dec_mc_4x4 (vo, mb_cache_ptr->pred_cache[s_list].pred_Y, pRefFrame, mv_x, mv_y, b8, b4);

            s_dir++;
        } while(s_dir < pdir);
    }

    return pdir;
}

PUBLIC int32 H264Dec_MC8x8_direct_spatial(void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
    H264DecContext *vo = (H264DecContext *)img;
    int32 fw_rFrame,bw_rFrame;
    int32 mv_xy;
    uint32 row;
    uint32 use_4x4mc = 0;
    int32 b8pdir = 0;
    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_dir;
    int8 *s_ref_idx_ptr;
    int32 *s_ref_mv_ptr;

    fw_rFrame = mb_cache_ptr->fw_rFrame;
    bw_rFrame = mb_cache_ptr->bw_rFrame;

    s_dir = 0;

    if (fw_rFrame >= 0) {
#if defined(H264_BIG_ENDIAN)
        mv_x = (mb_cache_ptr->fw_mv_xy >> 16);
        mv_y = ((mb_cache_ptr->fw_mv_xy << 16) >> 16);
        mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
        mv_xy  = mb_cache_ptr->fw_mv_xy;
#endif

        //set mv cache
        if (mv_xy) {	//curr mv context has been set to value 0 in fill_mb func.

            uint32 col;
            int32 condition = (fw_rFrame || (vo->g_list[1][0]->is_long_term)) ? 0 : 1;
            int8 *moving_block = mb_cache_ptr->moving_block + (( (b8>>1)*2)*4 + (b8&0x1)*2);

            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[0]) + cache_offset;
            for (row = 0; row < 2; row++) {
                for (col = 0; col < 2; col++) {
                    if (condition && (!moving_block[col]) ) {
                        use_4x4mc = 1;
                    } else {
                        s_ref_mv_ptr[col] = mv_xy;
                    }
                }

                moving_block += 4;
                s_ref_mv_ptr += CTX_CACHE_WIDTH;
            }
        }

        b8pdir = 0;
        s_dir++;
    }

    if (bw_rFrame >= 0) {
#if defined(H264_BIG_ENDIAN)
        mv_x = (mb_cache_ptr->bw_mv_xy >> 16);
        mv_y = ((mb_cache_ptr->bw_mv_xy << 16) >> 16);
        mv_xy  = (mv_x << 16) | (mv_y & 0xffff);
#else
        mv_xy  = mb_cache_ptr->bw_mv_xy;
#endif

        //set mv cache
        if (mv_xy) {	//curr mv context has been set to value 0 in fill_mb func.
            uint32 col;
            int32 condition = (bw_rFrame || (vo->g_list[1][0]->is_long_term)) ? 0 : 1;
            int8 *moving_block = mb_cache_ptr->moving_block + (( (b8>>1)*2)*4 + (b8&0x1)*2);

            s_ref_mv_ptr = (int32 *)(mb_cache_ptr->mv_cache[1]) + cache_offset;
            for (row = 0; row < 2; row++) {
                for (col = 0; col < 2; col++) {
                    if  (condition && (!moving_block[col])) {
                        use_4x4mc = 1;
                    } else {
                        s_ref_mv_ptr[col] = mv_xy;
                    }
                }

                moving_block += 4;
                s_ref_mv_ptr += CTX_CACHE_WIDTH;
            }
        }

        b8pdir = 1;
        s_dir++;
    } else {
        // fw < 0 and bw < 0
        if (fw_rFrame < 0) {
            fw_rFrame =  bw_rFrame = 0;
            s_dir = 2;
        }
    }

    //save ref_idx to mb_cache.
    if (fw_rFrame >= 0) {
        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[0]+cache_offset;
        s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = fw_rFrame;
        s_ref_idx_ptr += CTX_CACHE_WIDTH;
        s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = fw_rFrame;
    }

    if (bw_rFrame >= 0) {
        s_ref_idx_ptr = mb_cache_ptr->ref_idx_cache[1]+cache_offset;
        s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = bw_rFrame;
        s_ref_idx_ptr += CTX_CACHE_WIDTH;
        s_ref_idx_ptr[0] = s_ref_idx_ptr[1] = bw_rFrame;
    }

    b8pdir = (s_dir == 2) ? 2 : b8pdir;
    mb_cache_ptr->b8pdir[b8] = b8pdir;

    if (!use_4x4mc) {
        H264Dec_Config_8x8MC_info (vo, mb_cache_ptr, b8, b8pdir);
    } else {
        H264Dec_Config_4x4MC_info(vo, mb_cache_ptr, b8, b8pdir);
    }

    return b8pdir;
}

PUBLIC int32 H264Dec_MC8x8_direct_temporal(void *img, DEC_MB_CACHE_T *mb_cache_ptr, int32 cache_offset, int32 b8)
{
    H264DecContext *vo = (H264DecContext *)img;
    uint32 use_4x4mc = 0;
    uint32 b8pdir = 0;
    int32 j,offset_b4;
    int32 src_offset = (b8>>1)*8 + (b8&1)*2;
    int8  *ref_idx_ptr0, *ref_idx_ptr1;
    int32 *ref_mv_ptr0, *ref_mv_ptr1;
    int32 pdir = mb_cache_ptr->b8pdir[0];
    int32 s_list;

    //save mv and ref_idx to mb_cache
    for (s_list = 0; s_list < 2; s_list++) {
        int8  *ref_idx_src_ptr = mb_cache_ptr->ref_idx_cache_direct[s_list]+src_offset;
        int8  *ref_idx_dst_ptr = mb_cache_ptr->ref_idx_cache[s_list]+cache_offset;
        int32  *ref_mv_src_ptr = (int32 *)(mb_cache_ptr->mv_cache_direct[s_list])+src_offset;
        int32  *ref_mv_dst_ptr = (int32 *)(mb_cache_ptr->mv_cache[s_list])+cache_offset;
        int32 row;

        for (row = 0 ; row < 2; row++) {
            ((int16 *)ref_idx_dst_ptr)[0] = ((int16 *)ref_idx_src_ptr)[0];

            ref_mv_dst_ptr[0] = ref_mv_src_ptr[0];
            ref_mv_dst_ptr[1] = ref_mv_src_ptr[1];

            ref_idx_src_ptr += 4;
            ref_idx_dst_ptr += CTX_CACHE_WIDTH;
            ref_mv_src_ptr += 4;
            ref_mv_dst_ptr += CTX_CACHE_WIDTH;
        }
    }

    ref_idx_ptr0 = mb_cache_ptr->ref_idx_cache_direct[0] + src_offset;
    ref_idx_ptr1 = mb_cache_ptr->ref_idx_cache_direct[1] + src_offset;
    ref_mv_ptr0 = (int32*)(mb_cache_ptr->mv_cache_direct[0]) + src_offset;
    ref_mv_ptr1 = (int32*)(mb_cache_ptr->mv_cache_direct[1]) + src_offset;

    for (j=1 ; j < 4; j++) {
        offset_b4 = ((j>>1)<<2)+(j&1);

        if(ref_idx_ptr0[offset_b4]!=ref_idx_ptr0[0]
                || ref_idx_ptr1[offset_b4]!=ref_idx_ptr1[0]
                || ref_mv_ptr0[offset_b4]!= ref_mv_ptr0[0]
                || ref_mv_ptr1[offset_b4]!= ref_mv_ptr1[0]) {
            use_4x4mc = 1;
            break;
        }
    }

    b8pdir = 2;
    mb_cache_ptr->b8pdir[b8] = b8pdir;

    if (!use_4x4mc) {
        H264Dec_Config_8x8MC_info(vo, mb_cache_ptr, b8, b8pdir);
    } else {
        H264Dec_Config_4x4MC_info(vo, mb_cache_ptr, b8, b8pdir);
    }

    return b8pdir;
}

LOCAL void H264_MC_GetAverage (DEC_MB_CACHE_T *mb_cache_ptr, int32 b8)
{
    int i;
    uint8 *pPred0, *pPred1;
    uint8 *fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_Y;
    uint8 *bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_Y;

    //for 8x8 block mc
    int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
    int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};
#ifndef _NEON_OPT_
    int32 j;
#else
    uint8x8_t v8x8_0, v8x8_1;
#endif

    //for Y
    pPred0 = fw_mca_bfr + b8_luma_offset[b8];
    pPred1 = bw_mca_bfr + b8_luma_offset[b8];
    for (i = 8; i > 0; i--)	{//y dir
#ifndef _NEON_OPT_
        for (j = 0; j < 8; j++) {
            pPred0[j] = (pPred0[j] + pPred1[j] + 1)/2;
        }
#else
        v8x8_0 = vld1_u8(pPred0);
        v8x8_1 = vld1_u8(pPred1);

        v8x8_0 = vrhadd_u8(v8x8_0, v8x8_1);
        vst1_u8(pPred0, v8x8_0);
#endif
        pPred0 += MB_SIZE;
        pPred1 += MB_SIZE;
    }

    //for U
    fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[0];
    bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[0];

    pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
    pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
    for (i = 0; i < 4; i++) {
#ifndef _NEON_OPT_
        for (j = 0; j < 4; j++) {
            pPred0[j] = (pPred0[j] + pPred1[j] + 1)/2;
        }
#else
        v8x8_0 = vld1_u8(pPred0);
        v8x8_1 = vld1_u8(pPred1);

        v8x8_0 = vrhadd_u8(v8x8_0, v8x8_1);
        pPred0[0] = vget_lane_u8(v8x8_0, 0);
        pPred0[1] = vget_lane_u8(v8x8_0, 1);
        pPred0[2] = vget_lane_u8(v8x8_0, 2);
        pPred0[3] = vget_lane_u8(v8x8_0, 3);
#endif
        pPred0 += MB_CHROMA_SIZE;
        pPred1 += MB_CHROMA_SIZE;
    }

    //for V
    fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[1];
    bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[1];

    pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
    pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
    for (i = 4; i > 0; i--) {
#ifndef _NEON_OPT_
        for (j = 0; j < 4; j++) {
            pPred0[j] = (pPred0[j] + pPred1[j] + 1)/2;
        }
#else
        v8x8_0 = vld1_u8(pPred0);
        v8x8_1 = vld1_u8(pPred1);

        v8x8_0 = vrhadd_u8(v8x8_0, v8x8_1);
        pPred0[0] = vget_lane_u8(v8x8_0, 0);
        pPred0[1] = vget_lane_u8(v8x8_0, 1);
        pPred0[2] = vget_lane_u8(v8x8_0, 2);
        pPred0[3] = vget_lane_u8(v8x8_0, 3);
#endif
        pPred0 += MB_CHROMA_SIZE;
        pPred1 += MB_CHROMA_SIZE;
    }
}

LOCAL void H264_MC_Copy (DEC_MB_CACHE_T *mb_cache_ptr, int32 b8)
{
    int i;
    uint8 *pPred0, *pPred1;
    uint8 *fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_Y;
    uint8 *bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_Y;

    //for 8x8 block mc
    int32 b8_luma_offset[4] = {0, 8, 8*16, 8*16+8};
    int32 b8_chroma_offset[4] = {0, 4, 4*8, 4*8+4};

    //for Y
    pPred0 = fw_mca_bfr + b8_luma_offset[b8];
    pPred1 = bw_mca_bfr + b8_luma_offset[b8];
    for (i = 0; i < 8; i++) {
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;

        pPred0 += 8;
        pPred1 += 8;
    }

    //for U
    fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[0];
    bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[0];

    pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
    pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
    for (i = 0; i < 4; i++) {
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;

        pPred0 += 4;
        pPred1 += 4;
    }

    //for V
    fw_mca_bfr = mb_cache_ptr->pred_cache[0].pred_UV[1];
    bw_mca_bfr = mb_cache_ptr->pred_cache[1].pred_UV[1];

    pPred0 = fw_mca_bfr + b8_chroma_offset[b8];
    pPred1 = bw_mca_bfr + b8_chroma_offset[b8];
    for (i = 0; i < 4; i++) {
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;
        *pPred0++ = *pPred1++;

        pPred0 += 4;
        pPred1 += 4;
    }
}

PUBLIC void H264Dec_mv_prediction (H264DecContext *vo, DEC_MB_INFO_T *mb_info_ptr, DEC_MB_CACHE_T *mb_cache_ptr)
{
    if (mb_cache_ptr->is_skipped || mb_info_ptr->mb_type == 0) {
        int8 b8pdir;
        if ( vo->type == B_SLICE) {
            b8pdir = vo->pred_skip_bslice ((void *)vo, mb_cache_ptr);

            if (b8pdir != 2) {
                H264Dec_FillNoRefList_8x8(vo, 0, b8pdir);
                H264Dec_FillNoRefList_8x8(vo, 1, b8pdir);
                H264Dec_FillNoRefList_8x8(vo, 2, b8pdir);
                H264Dec_FillNoRefList_8x8(vo, 3, b8pdir);
            }
        } else {//pslice
            H264Dec_mv_prediction_skipped_pslice (vo, mb_cache_ptr);
            b8pdir = 0;
        }
    } else {
        switch(mb_info_ptr->mb_type) {
        case PMB16x16:
            H264Dec_mv_prediction_PMB16x16 (vo, mb_cache_ptr);
            break;
        case PMB16x8:
            H264Dec_mv_prediction_PMB16x8 (vo, mb_cache_ptr);
            break;
        case PMB8x16:
            H264Dec_mv_prediction_PMB8x16 (vo, mb_cache_ptr);
            break;
        case P8x8:
            H264Dec_mv_prediction_P8x8 (vo, mb_cache_ptr);
            break;
        default:
            break;
        }
    }
#if defined(CTS_PROTECT)
    if(vo->error_flag) {
        return;
    }
#endif

    if (vo->apply_weights) {
        H264Dec_Config_WP_info(vo, mb_cache_ptr);
    } else if (vo->type == B_SLICE) {//B_SLICE
        int b8;
        int pdir;

        for (b8 = 0; b8 < 4; b8++) {
            pdir = mb_cache_ptr->b8pdir[b8];
            if (2 == pdir) {//calculate average for bi-prd block
                H264_MC_GetAverage(mb_cache_ptr, b8);
            } else if (1 == pdir) {//copy pred pixel from pred_cache[1] to pred_cache[0]
                H264_MC_Copy(mb_cache_ptr, b8);
            }
        }
    }

    return;
}

#if defined(_VSP_)
#if defined(H264_BIG_ENDIAN)
#undef H264_BIG_ENDIAN
#endif
#endif

#if defined(_VSP_)
#if defined(H264_BIG_ENDIAN)
#undef H264_BIG_ENDIAN
#endif
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End
