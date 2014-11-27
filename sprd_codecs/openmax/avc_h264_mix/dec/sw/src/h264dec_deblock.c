/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264dec_video_header.h"

#include <stdio.h>
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

#define IS_INTRA(type)  ((type) > 8)

/* Deblocking filter (p153) */
const int alpha_table[52] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
    7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
    255, 255
};

const int beta_table[52] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
    3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
    8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18
};

const int tc0_table[52][3] = {
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};

#ifndef _NEON_OPT_
static void filter_mb_edgev(DEC_SLICE_T *curr_slice_ptr, uint8 *pix, int stride, int bS[4], int qp ) {
    int i, d;
    //DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    const int index_a = Clip3(0, 51, qp + curr_slice_ptr->LFAlphaC0Offset);
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[Clip3( 0, 51, qp + curr_slice_ptr->LFBetaOffset)];
    for( i = 0; i < 4; i++ ) {
        if( bS[i] == 0 ) {
            pix += 4 * stride;
            continue;
        }
        if( bS[i] < 4 ) {
            const int tc0 = tc0_table[index_a][bS[i] - 1];
            /* 4px edge length */
            for( d = 0; d < 4; d++ ) {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int p2 = pix[-3];
                const int q0 = pix[0];
                const int q1 = pix[1];
                const int q2 = pix[2];
                if( ABS( p0 - q0 ) < alpha && ABS( p1 - p0 ) < beta && ABS( q1 - q0 ) < beta )
                {
                    int tc = tc0;
                    int i_delta;
                    if( ABS( p2 - p0 ) < beta ) {
                        int tmp = ( p0 + q0 + 1 ) >> 1 ;
                        tmp += p2;
                        tmp = tmp - ( p1 << 1 );
                        tmp = tmp >> 1;
                        pix[-2] = p1 + Clip3(-tc0, tc0, tmp);
                        //pix[-2] = p1 + Clip3(-tc0, tc0, ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1);
                        tc++;
                    }
                    if( ABS( q2 - q0 ) < beta ) {
                        pix[1] = q1 + Clip3(-tc0, tc0, ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1);
                        tc++;
                    }
                    i_delta = Clip3(-tc, tc, (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3);
                    pix[-1] = Clip3(0, 255, p0 + i_delta );    /* p0' */
                    pix[0]  = Clip3(0, 255, q0 - i_delta );    /* q0' */
                }
                pix += stride;
            }
        } else {
            /* 4px edge length */
            for( d = 0; d < 4; d++ ) {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int p2 = pix[-3];
                const int q0 = pix[0];
                const int q1 = pix[1];
                const int q2 = pix[2];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    if(ABS( p0 - q0 ) < (( alpha >> 2 ) + 2 )) {
                        if( ABS( p2 - p0 ) < beta)
                        {
                            const int p3 = pix[-4];
                            /* p0', p1', p2' */
                            pix[-1] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                            pix[-2] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                            pix[-3] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                        } else {
                            /* p0' */
                            pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        }
                        if( ABS( q2 - q0 ) < beta)
                        {
                            const int q3 = pix[3];
                            /* q0', q1', q2' */
                            pix[0] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                            pix[1] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                            pix[2] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                        } else {
                            /* q0' */
                            pix[0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                        }
                    } else {
                        /* p0', q0' */
                        pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        pix[ 0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }
                pix += stride;
            }
        }
    }
}

static void filter_mb_edgecv( H264DecContext *img_ptr, uint8 *pix, int stride, int bS[4], int qp ) {
    int i, d;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    const int index_a = Clip3(0, 51, qp + curr_slice_ptr->LFAlphaC0Offset);
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[Clip3(0, 51, qp + curr_slice_ptr->LFBetaOffset)];
    for( i = 0; i < 4; i++ ) {
        if( bS[i] == 0 ) {
            pix += 2 * stride;
            continue;
        }
        if( bS[i] < 4 ) {
            const int tc = tc0_table[index_a][bS[i] - 1] + 1;
            /* 2px edge length (because we use same bS than the one for luma) */
            for( d = 0; d < 2; d++ ) {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int q0 = pix[0];
                const int q1 = pix[1];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    const int i_delta = Clip3(-tc, tc, (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3);
                    pix[-1] = Clip3(0 ,255,  p0 + i_delta );    /* p0' */
                    pix[0]  = Clip3(0, 255, q0 - i_delta );    /* q0' */
                }
                pix += stride;
            }
        } else {
            /* 2px edge length (because we use same bS than the one for luma) */
            for( d = 0; d < 2; d++ ) {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int q0 = pix[0];
                const int q1 = pix[1];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                    pix[0]  = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                }
                pix += stride;
            }
        }
    }
}

static void filter_mb_edgeh( H264DecContext *img_ptr, uint8 *pix, int stride, int bS[4], int qp ) {
    int i, d;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    const int index_a = Clip3(0, 51, qp + curr_slice_ptr->LFAlphaC0Offset);
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[Clip3(0, 51, qp + curr_slice_ptr->LFBetaOffset)];
    const int pix_next  = stride;
    for( i = 0; i < 4; i++ ) {
        if( bS[i] == 0 ) {
            pix += 4;
            continue;
        }
        if( bS[i] < 4 ) {
            const int tc0 = tc0_table[index_a][bS[i] - 1];
            /* 4px edge length */
            for( d = 0; d < 4; d++ ) {
                const int p0 = pix[-1*pix_next];
                const int p1 = pix[-2*pix_next];
                const int p2 = pix[-3*pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*pix_next];
                const int q2 = pix[2*pix_next];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    int tc = tc0;
                    int i_delta;
                    if( ABS( p2 - p0 ) < beta ) {
                        pix[-2*pix_next] = p1 + Clip3(-tc0, tc0, ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1);
                        tc++;
                    }
                    if( ABS( q2 - q0 ) < beta ) {
                        pix[pix_next] = q1 + Clip3(-tc0, tc0, ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1);
                        tc++;
                    }
                    i_delta = Clip3(-tc, tc, (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3);
                    pix[-pix_next] = Clip3(0, 255, p0 + i_delta );    /* p0' */
                    pix[0]         = Clip3(0, 255, q0 - i_delta );    /* q0' */
                }
                pix++;
            }
        } else {
            /* 4px edge length */
            for( d = 0; d < 4; d++ ) {
                const int p0 = pix[-1*pix_next];
                const int p1 = pix[-2*pix_next];
                const int p2 = pix[-3*pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*pix_next];
                const int q2 = pix[2*pix_next];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    const int p3 = pix[-4*pix_next];
                    const int q3 = pix[ 3*pix_next];
                    if(ABS( p0 - q0 ) < (( alpha >> 2 ) + 2 )) {
                        if( ABS( p2 - p0 ) < beta) {
                            /* p0', p1', p2' */
                            pix[-1*pix_next] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                            pix[-2*pix_next] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                            pix[-3*pix_next] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                        } else {
                            /* p0' */
                            pix[-1*pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        }
                        if( ABS( q2 - q0 ) < beta) {
                            /* q0', q1', q2' */
                            pix[0*pix_next] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                            pix[1*pix_next] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                            pix[2*pix_next] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                        } else {
                            /* q0' */
                            pix[0*pix_next] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                        }
                    } else {
                        /* p0', q0' */
                        pix[-1*pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        pix[ 0*pix_next] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }
                pix++;
            }
        }
    }
}

static void filter_mb_edgech( H264DecContext *img_ptr, uint8 *pix, int stride, int bS[4], int qp ) {
    int i, d;
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    const int index_a = Clip3(0, 51, qp + curr_slice_ptr->LFAlphaC0Offset);
    const int alpha = alpha_table[index_a];
    const int beta  = beta_table[Clip3(0, 51, qp + curr_slice_ptr->LFBetaOffset)];
    const int pix_next  = stride;
    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 ) {
            pix += 2;
            continue;
        }
        if( bS[i] < 4 ) {
            int tc = tc0_table[index_a][bS[i] - 1] + 1;
            /* 2px edge length (see deblocking_filter_edgecv) */
            for( d = 0; d < 2; d++ ) {
                const int p0 = pix[-1*pix_next];
                const int p1 = pix[-2*pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*pix_next];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    int i_delta = Clip3(-tc, tc, (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3);
                    pix[-pix_next] = Clip3(0, 255, p0 + i_delta );    /* p0' */
                    pix[0]         = Clip3(0, 255, q0 - i_delta );    /* q0' */
                }
                pix++;
            }
        } else {
            /* 2px edge length (see deblocking_filter_edgecv) */
            for( d = 0; d < 2; d++ ) {
                const int p0 = pix[-1*pix_next];
                const int p1 = pix[-2*pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*pix_next];
                if( ABS( p0 - q0 ) < alpha &&
                        ABS( p1 - p0 ) < beta &&
                        ABS( q1 - q0 ) < beta ) {
                    pix[-pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                    pix[0]         = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                }
                pix++;
            }
        }
    }
}
#endif

static void H264Dec_filter_mb( H264DecContext *img_ptr, int mb_x, int mb_y, uint8 *img_y, uint8 *img_cb, uint8 *img_cr)
{
    DEC_SLICE_T *curr_slice_ptr = img_ptr->curr_slice_ptr;
    DEC_MB_INFO_T *mb_info_ptr = img_ptr->mb_info;
    const int32 mb_xy= mb_x + mb_y*img_ptr->frame_width_in_mbs;

    int32 b4_offset = (mb_y<<2)* img_ptr->b4_pitch+ (mb_x<<2);
    int32 b4_offset_nnz = (mb_y*6)* img_ptr->b4_pitch + (mb_x<<2);
    int8 * nnz_ptr = img_ptr->nnz_ptr + b4_offset_nnz;

    DEC_STORABLE_PICTURE_T *pic = img_ptr->g_dec_picture_ptr;
    int8 *ref_idx_ptr = pic->ref_idx_ptr[0] + b4_offset;
    int32 *mv_ptr = (int32*)(pic->mv_ptr[0]) + b4_offset;

    int32 linesize = img_ptr->ext_width;
    int32 uvlinesize = linesize >> 1;
    int32 dir, mbn_xy, i;
    int32 b_idx, bn_idx;
    int32 bS[4];
    uint8 qp_cur, qp_n, qp_u, qp_v, qp_u_n, qp_v_n;
    int16 mv_x, mv_y, mvn_x, mvn_y;

    DEBLK_PARAS_T dbk_para;
    dbk_para.linesize = img_ptr->ext_width;

    /* FIXME Implement deblocking filter for field MB */
    if( img_ptr->g_sps_ptr->mb_adaptive_frame_field_flag) {
        return;
    }

    /* dir : 0 -> vertical edge, 1 -> horizontal edge */
    for( dir = 0; dir < 2; dir++ )
    {
        int start = 0;
        int edge;

        dbk_para.dir = dir;

        /* test picture boundary */
        if( ( dir == 0 && mb_x == 0 ) || ( dir == 1 && mb_y == 0 ) ) {
            start = 1;
        }

        /* FIXME test slice boundary
        if( img_ptr->deblocking_filter == 2 ) {
        }

        /* Calculate bS */
        for( edge = start; edge < 4; edge++ ) {
            /* mbn_xy: neighbour macroblock (how that works for field ?) */
            mbn_xy = edge > 0 ? mb_xy : ( dir == 0 ? mb_xy -1 : mb_xy - img_ptr->frame_width_in_mbs);

            if( IS_INTRA( (mb_info_ptr + mb_xy)->mb_type ) ||
                    IS_INTRA( (mb_info_ptr + mbn_xy)->mb_type ) )
            {
                bS[0] = bS[1] = bS[2] = bS[3] = ( edge == 0 ? 4 : 3 );
            }
            else
            {
                for( i = 0; i < 4; i++ )
                {
                    int32 x = dir == 0 ? edge : i;
                    int32 y = dir == 0 ? i : edge;

                    b_idx = y*img_ptr->b4_pitch + x;
                    bn_idx = dir == 0 ? (b_idx - 1) : ((0 == edge) ?
                                                       (b_idx - 3*img_ptr->b4_pitch) : (b_idx - img_ptr->b4_pitch));   //for nnz_ptr

                    if( nnz_ptr[b_idx] != 0 ||nnz_ptr[bn_idx] != 0 )
                    {
                        bS[i] = 2;
                    }
                    else if( img_ptr->type == P_SLICE)
                    {
                        if (1 == dir && 0 == edge)
                        {
                            bn_idx = (b_idx - img_ptr->b4_pitch); //for ref_idx_ptr &  mv
                        }

#if defined(H264_BIG_ENDIAN)
                        mv_x = mv_ptr[b_idx] >> 16;
                        mv_y = mv_ptr[b_idx] & 0xffff;

                        mvn_x = mv_ptr[bn_idx] >> 16;
                        mvn_y = mv_ptr[bn_idx] & 0xffff;
#else
                        mv_x = mv_ptr[b_idx] & 0xffff;
                        mv_y = mv_ptr[b_idx] >> 16;

                        mvn_x = mv_ptr[bn_idx] & 0xffff;
                        mvn_y = mv_ptr[bn_idx] >> 16;
#endif

                        if( ref_idx_ptr[b_idx] != ref_idx_ptr[bn_idx] ||ABS(mv_x - mvn_x) >= 4 ||ABS(mv_y - mvn_y) >= 4)
                            bS[i] = 1;
                        else
                            bS[i] = 0;
                    }
                    else
                    {
                        /* FIXME Add support for B frame */
                        return;
                    }
                }

                if(bS[0]+bS[1]+bS[2]+bS[3] == 0) continue;
            }

            /* Filter edge */
            qp_cur = (mb_info_ptr + mb_xy)->qp;
            qp_n = (mb_info_ptr + mbn_xy)->qp;
            dbk_para.qp_y = (qp_cur + qp_n + 1 ) >> 1;

            qp_u = g_QP_SCALER_CR_TBL[qp_cur + img_ptr->chroma_qp_offset];
            qp_v = g_QP_SCALER_CR_TBL[qp_cur + img_ptr->second_chroma_qp_index_offset];
            qp_u_n = g_QP_SCALER_CR_TBL[qp_n + img_ptr->chroma_qp_offset];
            qp_v_n = g_QP_SCALER_CR_TBL[qp_n + img_ptr->second_chroma_qp_index_offset];
            dbk_para.qp_u = ( qp_u + qp_u_n + 1 ) >> 1;
            dbk_para.qp_v = ( qp_v + qp_v_n + 1 ) >> 1;

#ifndef _NEON_OPT_
            if( dir == 0 )
            {
                filter_mb_edgev( curr_slice_ptr, &img_y[4*edge], linesize, bS, dbk_para.qp_y );
                if( (edge&1) == 0 ) {
                    filter_mb_edgecv( img_ptr, &img_cb[2*edge], uvlinesize, bS, dbk_para.qp_u );
                    filter_mb_edgecv( img_ptr, &img_cr[2*edge], uvlinesize, bS, dbk_para.qp_v );
                }
            }
            else
            {
                filter_mb_edgeh( img_ptr, &img_y[4*edge*linesize], linesize, bS, dbk_para.qp_y );
                if( (edge&1) == 0 ) {
                    filter_mb_edgech( img_ptr, &img_cb[2*edge*uvlinesize], uvlinesize, bS, dbk_para.qp_u );
                    filter_mb_edgech( img_ptr, &img_cr[2*edge*uvlinesize], uvlinesize, bS, dbk_para.qp_v );
                }
            }
#else
            if( dir == 0 )
            {
                filter_mb_edge_neon( curr_slice_ptr, &img_y[(edge << 2) - 4                         ], &dbk_para, bS);
                filter_mb_edge_neon( curr_slice_ptr, &img_y[(edge << 2) + (linesize << 3) - 4], &dbk_para, bS+2);

                if( (edge&1) == 0 ) {
                    dbk_para.is_uv = 0;
                    filter_mb_edgec_neon( curr_slice_ptr, &img_cb[2*edge-4], &dbk_para, bS);

                    dbk_para.is_uv = 1;
                    filter_mb_edgec_neon( curr_slice_ptr, &img_cr[2*edge-4], &dbk_para, bS );
                }
            }
            else
            {
                filter_mb_edge_neon( curr_slice_ptr, &img_y[4*(edge-1)*linesize    ], &dbk_para, bS);
                filter_mb_edge_neon( curr_slice_ptr, &img_y[4*(edge-1)*linesize + 8], &dbk_para, bS+2);
                if( (edge&1) == 0 ) {
                    dbk_para.is_uv = 0;
                    filter_mb_edgec_neon( curr_slice_ptr, &img_cb[2*(edge-2)*uvlinesize], &dbk_para, bS);

                    dbk_para.is_uv = 1;
                    filter_mb_edgec_neon( curr_slice_ptr, &img_cr[2*(edge-2)*uvlinesize], &dbk_para, bS );
                }
            }
#endif
        }
    }
}

PUBLIC void H264Dec_deblock_picture(H264DecContext *img_ptr, DEC_STORABLE_PICTURE_T *dec_picture_ptr)
{
    int32 mb_x, mb_y;
    int32 i, j;

    uint8 * dest_y = dec_picture_ptr->imgYUV[0] + img_ptr->start_in_frameY;
    uint8 * dest_u = dec_picture_ptr->imgYUV[1] + img_ptr->start_in_frameUV;
    uint8 * dest_v = dec_picture_ptr->imgYUV[2] + img_ptr->start_in_frameUV;


    ALOGE("%s, %d", __FUNCTION__, __LINE__);

    for (mb_y = 0; mb_y < img_ptr->frame_height_in_mbs; mb_y++)
    {
        for (mb_x = 0; mb_x < img_ptr->frame_width_in_mbs; mb_x++)
        {

            H264Dec_filter_mb(img_ptr, mb_x, mb_y, dest_y, dest_u, dest_v);

#if 0
            if (mb_x == 29 && mb_y == 21 && img_ptr->g_nFrame_dec_h264 == 12)
            {
                FILE *fp = fopen("mb1_vsp.txt", "wb");
                for (j = 0; j < 16; j++)
                {
                    for (i = 0; i < 16; i++)
                    {
                        fprintf(fp, "%4x", dest_y[j*img_ptr->ext_width + i]);
                    }

                    fprintf(fp, "\n");
                }

                for (j = 0; j < 8; j++)
                {
                    for (i = 0; i < 8; i++)
                    {
                        fprintf(fp, "%4x", dest_u[j*img_ptr->ext_width/2 + i]);
                    }

                    fprintf(fp, "\n");
                }

                for (j = 0; j < 8; j++)
                {
                    for (i = 0; i < 8; i++)
                    {
                        fprintf(fp, "%4x", dest_v[j*img_ptr->ext_width/2 + i]);
                    }

                    fprintf(fp, "\n");
                }

                fclose(fp);
            }
#endif

            dest_y += MB_SIZE;
            dest_u += BLOCK_SIZE;
            dest_v += BLOCK_SIZE;
        }

        dest_y += ((img_ptr->ext_width<<4)- img_ptr->width);
        dest_u += ((img_ptr->ext_width<<2)- (img_ptr->width>>1));
        dest_v += ((img_ptr->ext_width<<2)- (img_ptr->width>>1));
    }

#if 0
    if (img_ptr->g_nFrame_dec_h264 == 61)
    {
        FILE *fp = fopen("frame_vsp.txt", "wb");

        for (i = 0; i < 16; i++)
            fwrite(dec_picture_ptr->imgYUV[0] + img_ptr->start_in_frameY + i *img_ptr->ext_width,
                   1, /*img_ptr->width-*/ 16*1, fp);
        fclose(fp);
    }
#endif

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


