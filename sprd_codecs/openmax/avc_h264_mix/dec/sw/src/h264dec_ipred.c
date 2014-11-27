/******************************************************************************
 ** File Name:    h264dec_ipred.c                                             *
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

//void intra_pred_luma4x4_DC_PRED (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, uint8 * pPred, int blkIdxInMB)
void intra_pred_luma4x4_DC_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    int32 a;
    int32 avail;
    int32 pred = 0;
    uint8 * pPix;
    uint32 * pIntPix;
    int32 blk_avail_num = 0;
    int32 constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;
#ifdef _NEON_OPT_
    uint8x8_t v8x8_top, v8x8_left;
    uint16x4_t v16x4;
    uint32x2_t v32x2;
#endif

    /*left pixel*/
    if ((blkIdxInMB & 0x3) != 0)
    {
        avail = 1;
    } else
    {
        avail = mb_cache_ptr->mb_avail_a;

        if (avail)
        {
            if (constrainedIntra)
            {
                avail = (currMB - 1)->is_intra;
            }
        }
    }

    if (avail)
    {
        blk_avail_num++;
        pPix = pRec - 1;
        pred += pPix [0];
        pPix += pitch;
        pred += pPix [0];
        pPix += pitch;
        pred += pPix [0];
        pPix += pitch;
        pred += pPix [0];	//pPix += pitch;
    }

    /*top pixel*/
    if (blkIdxInMB >= 4)
    {
        avail = 1;
    } else
    {
        avail = mb_cache_ptr->mb_avail_b;

        if (avail)
        {
            if (constrainedIntra)
            {
                avail = ((H264DecContext *)img)->abv_mb_info->is_intra;
            }
        }
    }

    if (avail)
    {
        blk_avail_num++;
        pPix = pRec - pitch;

#ifndef _NEON_OPT_
        pred += *pPix++;
        pred += *pPix++;
        pred += *pPix++;
        pred += *pPix++;
#else
        v8x8_top = vld1_u8(pPix);
        v16x4 = vpaddl_u8(v8x8_top);
        v32x2 = vpaddl_u16(v16x4);

        pred += vget_lane_u32(v32x2, 0);
#endif
    }

    if (blk_avail_num == 2)
    {
        pred = (pred + 4) / 8;
    }
    else if (blk_avail_num == 0)
    {
        pred = 128;
    }
    else
    {
        pred = (pred + 2) / 4;;
    }

    /*assign pred to reference block*/
    a = (pred << 8) | pred;
    a = (a << 16) | a;
    pIntPix = (uint32 *)pPred;

    pIntPix [0] = a;      //position in a MB
    pIntPix [4] = a;
    pIntPix [8] = a;
    pIntPix [12] = a;
}

void intra_pred_luma4x4_VERT_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB,  uint8 *pRec, int32 pitch)
{
    uint32 a;
    uint8 * pPix = NULL;
    uint32 * pIntPix = NULL;

    pPix = pRec - pitch;
    a = *((uint32*)pPix);

    pIntPix = (uint32 *)pPred;
    pIntPix [0]		= a;
    pIntPix [4]		= a;
    pIntPix [8]		= a;
    pIntPix [12]	= a;
}

void intra_pred_luma4x4_HOR_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    int32 i;
    uint8 *pPix = NULL, *pPixBlk = NULL;

    pPix = pRec - 1;
    pPixBlk = pPred;
    for (i = 0; i < 4; i++)
    {
        uint32 a = pPix [0] * 0x01010101;
        pPix += pitch;
        ((uint32 *)pPixBlk)[0] = a;

        pPixBlk += MB_SIZE;
    }
}

void intra_pred_luma4x4_DIAG_DOWN_RIGHT_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    uint8	*pPix = NULL, * pPixBlk = NULL;
    uint8	tn1, t0, t1, t2, t3;
    uint8	ln1, l0, l1, l2, l3;

    pPix = pRec - pitch - 1;
    tn1 = pPix [0];
    t0 = pPix [1];
    t1 = pPix [2];
    t2 = pPix [3];
    t3 = pPix [4];
    ln1 = tn1;
    pPix += pitch;
    l0 = pPix [0];
    pPix += pitch;
    l1 = pPix [0];
    pPix += pitch;
    l2 = pPix [0];
    pPix += pitch;
    l3 = pPix [0];

    pPixBlk = pPred;

    /*x coordinate greater than y*/
    pPixBlk [1] = pPixBlk [18/*2 + MB_SIZE*/] = pPixBlk [35/*3 + 2 * MB_SIZE*/] = (tn1 + t0 * 2 + t1 + 2) / 4;
    pPixBlk [2] = pPixBlk [19/*3 + MB_SIZE*/] = (t0 + t1 * 2 + t2 + 2) / 4;
    pPixBlk [3] = (t1 + t2 * 2 + t3 + 2) / 4;

    /*x coordinate equal y*/
    pPixBlk [0] = pPixBlk [17/*1 + MB_SIZE*/] = pPixBlk [34/*2 + 2*MB_SIZE*/] = pPixBlk [51/*3 + 3*MB_SIZE*/] = (t0 + 2 * tn1 + l0 + 2) / 4;

    /*x < y*/
    pPixBlk [MB_SIZE] = pPixBlk [33/*1 + 2*MB_SIZE*/] = pPixBlk [50/*2 + 3*MB_SIZE*/] = (ln1 + l0 * 2 + l1 + 2) / 4;
    pPixBlk [32/*2*MB_SIZE*/] = pPixBlk [49/*1 + 3 * MB_SIZE*/] = (l0 + l1 * 2 + l2 + 2) / 4;
    pPixBlk [48/*3*MB_SIZE*/] = (l1 + l2 * 2 + l3 + 2) / 4;
}

void intra_pred_luma4x4_DIAG_DOWN_LEFT_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    int32 blkCAvai = 1;
    uint8	*pPix, *pPixBlk;
    uint8	t0, t1, t2, t3, t4, t5, t6, t7;
    int32 topAvail = 1;

    if (blkIdxInMB >= 4)
    {
        blkCAvai = g_blkC_avaiable_tbl [blkIdxInMB];
    } else
    {
        int constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;

        topAvail = mb_cache_ptr->mb_avail_b;
        if (constrainedIntra)
        {
            topAvail = topAvail &&  ((H264DecContext *)img)->abv_mb_info->is_intra;

            if (blkIdxInMB <3)
                blkCAvai = topAvail;
            else
                blkCAvai = mb_cache_ptr->mb_avail_c && (((H264DecContext *)img)->abv_mb_info + 1)->is_intra;
        } else
        {
            if (blkIdxInMB <3)
                blkCAvai = mb_cache_ptr->mb_avail_b;
            else
                blkCAvai = mb_cache_ptr->mb_avail_c;
        }
    }

    pPix = pRec - pitch;
    t0 = *pPix++;
    t1 = *pPix++;
    t2 = *pPix++;
    t3 = *pPix++;

    pPixBlk = pPred;
    pPixBlk [0] = (t0 + t1 * 2 + t2 + 2) / 4;
    pPixBlk [1] = pPixBlk [MB_SIZE] = (t1 + t2 * 2 + t3 + 2) / 4;

    if (blkCAvai)
    {
        t4 = *pPix++;
        t5 = *pPix++;
        t6 = *pPix++;
        t7 = *pPix++;
    } else
    {
        t4 = t3;
        t5 = t3;
        t6 = t3;
        t7 = t3;
    }

    pPixBlk [2] = pPixBlk [17/*1 + MB_SIZE*/] = pPixBlk [32/*2*MB_SIZE*/] = (t2 + t3 * 2 + t4 + 2) / 4;
    pPixBlk [3] = pPixBlk [18/*2 + MB_SIZE*/] = pPixBlk [33/*1 + 2*MB_SIZE*/] = pPixBlk [48/*3*MB_SIZE*/] = (t3 + t4 * 2 + t5 + 2) / 4;
    pPixBlk [19/*3 + MB_SIZE*/] = pPixBlk [34/*2 + 2*MB_SIZE*/] = pPixBlk [49/*1 + 3*MB_SIZE*/] = (t4 + t5 * 2 + t6 + 2) / 4;
    pPixBlk [35/*3 + 2*MB_SIZE*/] = pPixBlk [50/*2 + 3*MB_SIZE*/] = (t5 + t6 * 2 + t7 + 2) / 4;
    pPixBlk [51/*3 + 3*MB_SIZE*/] = (t6 + t7 * 3 + 2) / 4;
}

void intra_pred_luma4x4_VERT_RIGHT_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    uint8	* pPix = NULL, * pPixBlk = NULL;
    uint8	tn1, t0, t1, t2, t3;
    uint8	l0, l1, l2;

    //PRINTF ("VERT_RIGHT_PRED\n");

    pPix = pRec - pitch - 1;
    tn1 = pPix [0];
    t0 = pPix [1];
    t1 = pPix [2];
    t2 = pPix [3];
    t3 = pPix [4];
    pPix += pitch;
    l0 = pPix [0];
    pPix += pitch;
    l1 = pPix [0];
    pPix += pitch;
    l2 = pPix [0];
    pPix += pitch;

    pPixBlk = pPred;

    /* (2*x - y) equal 0, 2, 4, 6*/
    pPixBlk [0] = pPixBlk [33/*1 + 2*MB_SIZE*/] = (tn1 + t0 + 1) / 2;
    pPixBlk [1] = pPixBlk [34/*2 + 2*MB_SIZE*/] = (t0 + t1 + 1) / 2;
    pPixBlk [2] = pPixBlk [35/*3 + 2*MB_SIZE*/] = (t1 + t2 + 1) / 2;
    pPixBlk [3] = (t2 + t3 + 1) / 2;

    /* (2*x - y) equal 1, 3, 5*/
    pPixBlk [17/*1 + MB_SIZE*/] = pPixBlk [50/*2 + 3*MB_SIZE*/] = (tn1 + t0 * 2 + t1 + 2) / 4;
    pPixBlk [18/*2 + MB_SIZE*/] = pPixBlk [51/*3 + 3*MB_SIZE*/] = (t0 + t1 * 2 + t2 + 2) / 4;
    pPixBlk [19/*3 + MB_SIZE*/] = (t1 + t2 * 2 + t3 + 2) / 4;

    /* (2*x - y) equal -1*/
    pPixBlk [MB_SIZE] = pPixBlk [49/*1 + 3*MB_SIZE*/] = (l0 + tn1 * 2 + t0 + 2) / 4;

    /* (2*x - y) equal 1, 3, 5*/
    pPixBlk [MB_SIZE_X2] = (l1 + l0 * 2 + tn1 + 2) / 4;
    pPixBlk [MB_SIZE_X3] = (l2 + l1 * 2 + l0 + 2) / 4;
}

void intra_pred_luma4x4_VERT_LEFT_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    int32 blkCAvai = 1;
    uint8	*pPix, *pPixBlk;
    uint8	t0, t1, t2, t3, t4, t5, t6;
    int32 topAvail = 1;
    int32 constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;

    if (blkIdxInMB >= 4)
    {
        blkCAvai = g_blkC_avaiable_tbl [blkIdxInMB];
    }
    else
    {
        topAvail = mb_cache_ptr->mb_avail_b;
        if (constrainedIntra)
        {
            topAvail = topAvail &&  ((H264DecContext *)img)->abv_mb_info->is_intra;

            if (blkIdxInMB < 3)
            {
                blkCAvai = topAvail;
            }
            else
            {
                blkCAvai = mb_cache_ptr->mb_avail_c && (((H264DecContext *)img)->abv_mb_info + 1)->is_intra;
            }
        }
        else
        {
            if (blkIdxInMB < 3)
                blkCAvai = mb_cache_ptr->mb_avail_b;
            else
                blkCAvai = mb_cache_ptr->mb_avail_c;
        }
    }

    pPix = pRec - pitch;
    pPixBlk = pPred;

    t0 = *pPix++;
    t1 = *pPix++;
    t2 = *pPix++;
    t3 = *pPix++;

    /*y = 0 or 2*/
    pPixBlk [0] = (t0 + t1 + 1) / 2;
    pPixBlk [1] = pPixBlk [MB_SIZE_X2] = (t1 + t2 + 1) / 2;
    pPixBlk [2] = pPixBlk [33/*1 + 2*MB_SIZE*/] = (t2 + t3 + 1) / 2;

    if (!blkCAvai)
    {
        t4 = t3;
        t5 = t3;
        t6 = t3;
    }
    else
    {
        t4 = *pPix++;
        t5 = *pPix++;
        t6 = *pPix++;
    }

    pPixBlk [3] = pPixBlk [2 + 2*MB_SIZE] = (t3 + t4 + 1) / 2;
    pPixBlk [3 + 2*MB_SIZE] = (t4 + t5 + 1) / 2;

    /*y = 1 or 3*/
    pPixBlk [MB_SIZE] = (t0 + t1 * 2 + t2 + 2) / 4;
    pPixBlk [17/*1 + MB_SIZE*/] = pPixBlk [MB_SIZE_X3] = (t1 + t2 * 2 + t3 + 2) / 4;
    pPixBlk [18/*2 + MB_SIZE*/] = pPixBlk [49/*1 + 3*MB_SIZE*/] = (t2 + t3 * 2 + t4 + 2) / 4;
    pPixBlk [19/*3 + MB_SIZE*/] = pPixBlk [50/*2 + 3*MB_SIZE*/] = (t3 + t4 * 2 + t5 + 2) / 4;
    pPixBlk [51/*3 + 3*MB_SIZE*/] = (t4 + t5 * 2 + t6 + 2) / 4;

}

void intra_pred_luma4x4_HOR_UP_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    uint8	*pPix = NULL, *pPixBlk = NULL;
    uint8	l0, l1, l2, l3;

    //PRINTF ("HOR_UP_PRED\n");

    pPix = pRec - 1;
    l0 = pPix[0];
    pPix += pitch;
    l1 = pPix[0];
    pPix += pitch;
    l2 = pPix[0];
    pPix += pitch;
    l3 = pPix[0];

    pPixBlk = pPred;

    /* (x + 2y) = 0, 2, 4*/
    pPixBlk [0] = (l0 + l1 + 1) / 2;
    pPixBlk [2] = pPixBlk [MB_SIZE] = (l1 + l2 + 1) / 2;
    pPixBlk [18/*2 + MB_SIZE*/] = pPixBlk [MB_SIZE_X2] = (l2 + l3 + 1) / 2;

    /* (x + 2y) = 1, 3*/
    pPixBlk [1] = (l0 + l1 * 2 + l2 + 2) / 4;
    pPixBlk [3] = pPixBlk [17/*1 + MB_SIZE*/] = (l1 + l2 * 2 + l3 + 2) / 4;

    /* (x + 2y) = 5*/
    pPixBlk [19/*3 + MB_SIZE*/] = pPixBlk [33/*1 + 2*MB_SIZE*/] = (l2 + l3 * 3 + 2) / 4;

    /* (x + 2y) > 5*/
    pPixBlk [34/*2 + 2*MB_SIZE*/] = l3;
    pPixBlk [35/*3 + 2*MB_SIZE*/] = l3;
    pPixBlk [MB_SIZE_X3] = l3;
    pPixBlk [49/*1 + 3*MB_SIZE*/] = l3;
    pPixBlk [50/*2 + 3*MB_SIZE*/] = l3;
    pPixBlk [51/*3 + 3*MB_SIZE*/] = l3;

}

void intra_pred_luma4x4_HOR_DOWN_PRED (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB, uint8 *pRec, int32 pitch)
{
    uint8	*pPix = NULL, *pPixBlk = NULL;
    uint8	tn1, t0, t1, t2;
    uint8	ln1, l0, l1, l2, l3;

    pPix = pRec - pitch - 1;
    tn1 = pPix [0];
    t0 = pPix [1];
    t1 = pPix [2];
    t2 = pPix [3];
    ln1 = tn1;
    pPix += pitch;
    l0 = pPix [0];
    pPix += pitch;
    l1 = pPix [0];
    pPix += pitch;
    l2 = pPix [0];
    pPix += pitch;
    l3 = pPix [0];

    pPixBlk = pPred;

    /* (2 * y - x) equal 0, 2, 4, 6 */
    pPixBlk [0] = pPixBlk [18/*2 + MB_SIZE*/] = (ln1 + l0 + 1) / 2;
    pPixBlk [MB_SIZE] = pPixBlk [34/*2 + 2*MB_SIZE*/] = (l0 + l1 + 1) / 2;
    pPixBlk [MB_SIZE_X2] = pPixBlk [50/*2 + 3*MB_SIZE*/] = (l1 + l2 + 1) / 2;
    pPixBlk [MB_SIZE_X3] = (l2 + l3 + 1) / 2;

    /* (2 * y - x) equal 1, 3, 5 */
    pPixBlk [17/*1+MB_SIZE*/] = pPixBlk [35/*3 + 2*MB_SIZE*/] = (ln1 + l0 * 2 + l1 + 2) / 4;
    pPixBlk [33/*1 + 2*MB_SIZE*/] = pPixBlk [51/*3 + 3*MB_SIZE*/] = (l0 + l1 * 2 + l2 + 2) / 4;
    pPixBlk [49/*1 + 3*MB_SIZE*/] = (l1 + l2 * 2 + l3 + 2) / 4;

    /* (2 * y - x) equal -1 */
    pPixBlk [1] = pPixBlk [19/*3 + MB_SIZE*/] = (l0 + ln1 * 2 + t0 + 2) / 4;

    /* (2 * y - x) equal -2, -3 */
    pPixBlk [2] = (t1 + t0 * 2 + ln1 + 2) / 4;
    pPixBlk [3] = (t2 + t1 * 2 + t0 + 2) / 4;
}

/*************************************************************************
                        intra prediction for I8x8 MB
**************************************************************************/
//  Z   A   B   C  D   E   F   G  H   I  J  K  L  M   N  O  P
//  Q  a1 b1 c1 d1 e1 f1 g1 h1
//  R  a2 b2 c2 d2 e2 f2 g2 h2
//  S  a3 b3 c3 d3 e3 f3 g3 h3
//  T  a4 b4 c4 d4 e4 f4 g4 h4
//  U  a5 b5 c5 d5 e5 f5 g5 h5
//  V  a6 b6 c6 d6 e6 f6 g6 h6
//  W  a7 b7 c7 d7 e7 f7 g7 h7
//  X  a8 b8 c8 d8 e8 f8 g8 h8


// Predictor array index definitions
#define P_Z (ptopPix[-1])
#define P_A (ptopPix[0])
#define P_B (ptopPix[1])
#define P_C (ptopPix[2])
#define P_D (ptopPix[3])
#define P_E (ptopPix[4])
#define P_F (ptopPix[5])
#define P_G (ptopPix[6])
#define P_H (ptopPix[7])
#define P_I (ptopPix[8])
#define P_J (ptopPix[9])
#define P_K (ptopPix[10])
#define P_L (ptopPix[11])
#define P_M (ptopPix[12])
#define P_N (ptopPix[13])
#define P_O (ptopPix[14])
#define P_P (ptopPix[15])
#define P_Q (pleftPix[0])
#define P_R (pleftPix[1])
#define P_S (pleftPix[2])
#define P_T (pleftPix[3])
#define P_U (pleftPix[4])
#define P_V (pleftPix[5])
#define P_W (pleftPix[6])
#define P_X (pleftPix[7])

#define EXPAND_8x8_IPRED

void intra_pred_luma8x8_VERT_PRED (DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint32 a, b;
    int32 i;
    uint8 *ptopLeftPix = mb_cache_ptr->topFilteredPix;
    uint32 *pIntSrc = (uint32 *)(ptopLeftPix +1);
    uint32 *pIntPix;

    a = pIntSrc[0];
    b = pIntSrc[1];

    pIntPix = (uint32 *)pPred;

    for(i=0; i<8; i++)
    {
        pIntPix [0] = a;
        pIntPix [1] =b;
        pIntPix += 4;
    }
}

void intra_pred_luma8x8_HOR_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    int32 i;
    uint32 a;
    uint8 *pleftPix = mb_cache_ptr->leftPredPix_Y + 8;
    uint32 *pIntPix;

    pIntPix = (uint32 *)pPred;

    for(i = 0 ; i < 8; i++)
    {
        a = pleftPix[i] * 0x01010101;
        pIntPix[0] =a;
        pIntPix[1] = a;
        pIntPix +=4;
    }

}

void intra_pred_luma8x8_DC_PRED (DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 *pleftPix = mb_cache_ptr->leftPredPix_Y + 8;
    uint8 *ptopPix = mb_cache_ptr->topFilteredPix + 1;
    int8 i;
    int32 a;
    int32 pred = 0;
    uint32 * pIntPix;
    int32 blk_avail_num = 0;

    /*left pixel*/
    if(mb_cache_ptr->mb_avail_a_8x8_ipred)
    {
        blk_avail_num++;
        for(i=0; i<8; i++)
            pred += pleftPix[i];
    }

    /*top pixel*/
    if(mb_cache_ptr->mb_avail_b_8x8_ipred)
    {
        blk_avail_num++;
        for(i=0; i<8; i++)
            pred += ptopPix[i];
    }

    if (blk_avail_num == 2)
    {
        pred = (pred + 8) >>4;
    }
    else if (blk_avail_num == 0)
    {
        pred = 128;
    }
    else
    {
        pred = (pred + 4) >>3;;
    }

    /*assign pred to reference block*/
    a =pred * 0x01010101;
    pIntPix = (uint32 *)pPred;

    for(i=0; i<8; i++)
    {
        pIntPix [0] = a;
        pIntPix [1] = a;
        pIntPix += 4;
    }
}

void intra_pred_luma8x8_DIAG_DOWN_LEFT_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 *ptopPix = mb_cache_ptr->topFilteredPix+ 1;
#ifndef EXPAND_8x8_IPRED
    int x, y;
    for(y =0; y< 8; y++)
    {
        for(x =0; x<8; x++)
        {
            if(x == 7 && y ==7)
                pPred[x] = (ptopPix[14] + 3*ptopPix[15] + 2) >>2;
            else
                pPred[x] = (ptopPix[x+y] +2* ptopPix[x+y+1] + ptopPix[x+y+2] +2) >>2;
        }
        pPred +=MB_SIZE;
    }
#else
    // Mode DIAG_DOWN_LEFT_PRED
    pPred[(0<<4)+0] = ((P_A + P_C + 2*(P_B) + 2) >> 2);
    pPred[(1<<4)+0] =
        pPred[(0<<4)+1] = ((P_B + P_D + 2*(P_C) + 2) >> 2);
    pPred[(2<<4)+0] =
        pPred[(1<<4)+1] =
            pPred[(0<<4)+2] = ((P_C + P_E + 2*(P_D) + 2) >> 2);
    pPred[(3<<4)+0] =
        pPred[(2<<4)+1] =
            pPred[(1<<4)+2] =
                pPred[(0<<4)+3] = ((P_D + P_F + 2*(P_E) + 2) >> 2);
    pPred[(4<<4)+0] =
        pPred[(3<<4)+1] =
            pPred[(2<<4)+2] =
                pPred[(1<<4)+3] =
                    pPred[(0<<4)+4] = ((P_E + P_G + 2*(P_F) + 2) >> 2);
    pPred[(5<<4)+0] =
        pPred[(4<<4)+1] =
            pPred[(3<<4)+2] =
                pPred[(2<<4)+3] =
                    pPred[(1<<4)+4] =
                        pPred[(0<<4)+5] =  ((P_F + P_H + 2*(P_G) + 2) >> 2);
    pPred[(6<<4)+0] =
        pPred[(5<<4)+1] =
            pPred[(4<<4)+2] =
                pPred[(3<<4)+3] =
                    pPred[(2<<4)+4] =
                        pPred[(1<<4)+5] =
                            pPred[(0<<4)+6] = ((P_G + P_I + 2*(P_H) + 2) >> 2);
    pPred[(7<<4)+0] =
        pPred[(6<<4)+1] =
            pPred[(5<<4)+2] =
                pPred[(4<<4)+3] =
                    pPred[(3<<4)+4] =
                        pPred[(2<<4)+5] =
                            pPred[(1<<4)+6] =
                                pPred[(0<<4)+7] =  ((P_H + P_J + 2*(P_I) + 2) >> 2);
    pPred[(7<<4)+1] =
        pPred[(6<<4)+2] =
            pPred[(5<<4)+3] =
                pPred[(4<<4)+4] =
                    pPred[(3<<4)+5] =
                        pPred[(2<<4)+6] =
                            pPred[(1<<4)+7] =  ((P_I + P_K + 2*(P_J) + 2) >> 2);
    pPred[(7<<4)+2] =
        pPred[(6<<4)+3] =
            pPred[(5<<4)+4] =
                pPred[(4<<4)+5] =
                    pPred[(3<<4)+6] =
                        pPred[(2<<4)+7] =  ((P_J + P_L + 2*(P_K) + 2) >> 2);
    pPred[(7<<4)+3] =
        pPred[(6<<4)+4] =
            pPred[(5<<4)+5] =
                pPred[(4<<4)+6] =
                    pPred[(3<<4)+7] =  ((P_K + P_M + 2*(P_L) + 2) >> 2);
    pPred[(7<<4)+4] =
        pPred[(6<<4)+5] =
            pPred[(5<<4)+6] =
                pPred[(4<<4)+7] = ((P_L + P_N + 2*(P_M) + 2) >> 2);
    pPred[(7<<4)+5] =
        pPred[(6<<4)+6] =
            pPred[(5<<4)+7] = ((P_M + P_O + 2*(P_N) + 2) >> 2);
    pPred[(7<<4)+6] =
        pPred[(6<<4)+7] = ((P_N + P_P + 2*(P_O) + 2) >> 2);
    pPred[(7<<4)+7] = ((P_O + 3*(P_P) + 2) >> 2);
#endif
}

void intra_pred_luma8x8_DIAG_DOWN_RIGHT_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 *pleftPix = mb_cache_ptr->leftPredPix_Y + 8;
    uint8 *ptopPix = mb_cache_ptr->topFilteredPix + 1;
#ifndef EXPAND_8x8_IPRED
    int32 x, y;

    for(y =0; y< 8; y++)
    {
        for(x =0; x<8; x++)
        {
            if(x > y)
                pPred[x] = (ptopPix[x-y-2] + 2* ptopPix[x-y-1] + ptopPix[x-y] + 2) >>2;
            else if(x < y)
                pPred[x]  = (pleftPix[y-x-2] + 2*pleftPix[y-x-1] + pleftPix[y-x] + 2) >>2;
            else
                pPred[x] = (ptopPix[0] + 2*ptopPix[-1] + pleftPix[0] + 2) >>2;
        }
        pPred +=MB_SIZE;
    }
#else
    // Mode DIAG_DOWN_RIGHT_PRED
    pPred[(7<<4)+0] =  ((P_X + P_V + 2*(P_W) + 2) >> 2);
    pPred[(6<<4)+0] =
        pPred[(7<<4)+1] =  ((P_W + P_U + 2*(P_V) + 2) >> 2);
    pPred[(5<<4)+0] =
        pPred[(6<<4)+1] =
            pPred[(7<<4)+2] =  ((P_V + P_T + 2*(P_U) + 2) >> 2);
    pPred[(4<<4)+0] =
        pPred[(5<<4)+1] =
            pPred[(6<<4)+2] =
                pPred[(7<<4)+3] =  ((P_U + P_S + 2*(P_T) + 2) >> 2);
    pPred[(3<<4)+0] =
        pPred[(4<<4)+1] =
            pPred[(5<<4)+2] =
                pPred[(6<<4)+3] =
                    pPred[(7<<4)+4] =  ((P_T + P_R + 2*(P_S) + 2) >> 2);
    pPred[(2<<4)+0] =
        pPred[(3<<4)+1] =
            pPred[(4<<4)+2] =
                pPred[(5<<4)+3] =
                    pPred[(6<<4)+4] =
                        pPred[(7<<4)+5] =  ((P_S + P_Q + 2*(P_R) + 2) >> 2);
    pPred[(1<<4)+0] =
        pPred[(2<<4)+1] =
            pPred[(3<<4)+2] =
                pPred[(4<<4)+3] =
                    pPred[(5<<4)+4] =
                        pPred[(6<<4)+5] =
                            pPred[(7<<4)+6] =  ((P_R + P_Z + 2*(P_Q) + 2) >> 2);
    pPred[(0<<4)+0] =
        pPred[(1<<4)+1] =
            pPred[(2<<4)+2] =
                pPred[(3<<4)+3] =
                    pPred[(4<<4)+4] =
                        pPred[(5<<4)+5] =
                            pPred[(6<<4)+6] =
                                pPred[(7<<4)+7] =  ((P_Q + P_A + 2*(P_Z) + 2) >> 2);
    pPred[(0<<4)+1] =
        pPred[(1<<4)+2] =
            pPred[(2<<4)+3] =
                pPred[(3<<4)+4] =
                    pPred[(4<<4)+5] =
                        pPred[(5<<4)+6] =
                            pPred[(6<<4)+7] =  ((P_Z + P_B + 2*(P_A) + 2) >> 2);
    pPred[(0<<4)+2] =
        pPred[(1<<4)+3] =
            pPred[(2<<4)+4] =
                pPred[(3<<4)+5] =
                    pPred[(4<<4)+6] =
                        pPred[(5<<4)+7] =  ((P_A + P_C + 2*(P_B) + 2) >> 2);
    pPred[(0<<4)+3] =
        pPred[(1<<4)+4] =
            pPred[(2<<4)+5] =
                pPred[(3<<4)+6] =
                    pPred[(4<<4)+7] =  ((P_B + P_D + 2*(P_C) + 2) >> 2);
    pPred[(0<<4)+4] =
        pPred[(1<<4)+5] =
            pPred[(2<<4)+6] =
                pPred[(3<<4)+7] =  ((P_C + P_E + 2*(P_D) + 2) >> 2);
    pPred[(0<<4)+5] =
        pPred[(1<<4)+6] =
            pPred[(2<<4)+7] =  ((P_D + P_F + 2*(P_E) + 2) >> 2);
    pPred[(0<<4)+6] =
        pPred[(1<<4)+7] =  ((P_E + P_G + 2*(P_F) + 2) >> 2);
    pPred[(0<<4)+7] =  ((P_F + P_H + 2*(P_G) + 2) >> 2);
#endif
}

void intra_pred_luma8x8_VERT_RIGHT_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 * pleftPix = mb_cache_ptr->leftPredPix_Y + 8;
    uint8 * ptopPix = mb_cache_ptr->topFilteredPix + 1;
#ifndef EXPAND_8x8_IPRED
    int32 x, y;
    int32	  zVR;

    for(y =0; y< 8; y++)
    {
        for(x =0; x<8; x++)
        {
            zVR = 2*x - y;
            if(zVR >=0)
            {
                if(!(zVR & 0x1))
                    pPred[x] = (ptopPix[x-(y>>1)-1] + ptopPix[x-(y>>1)] + 1) >>1;
                else
                    pPred[x] = (ptopPix[x-(y>>1)-2] + 2*ptopPix[x-(y>>1)-1] + ptopPix[x-(y>>1)] + 2) >>2;
            }
            else if (zVR == -1)
                pPred[x] = (pleftPix[0] + 2*ptopPix[-1] + ptopPix[0] + 2) >>2;
            else
                pPred[x] = (pleftPix[y-2*x-1] + 2*pleftPix[y-2*x-2] + pleftPix[y-2*x-3] + 2) >>2;
        }
        pPred +=MB_SIZE;
    }
#else
    pPred[(0<<4)+0] =
        pPred[(2<<4)+1] =
            pPred[(4<<4)+2] =
                pPred[(6<<4)+3] =  ((P_Z + P_A + 1) >> 1);
    pPred[(0<<4)+1] =
        pPred[(2<<4)+2] =
            pPred[(4<<4)+3] =
                pPred[(6<<4)+4] =  ((P_A + P_B + 1) >> 1);
    pPred[(0<<4)+2] =
        pPred[(2<<4)+3] =
            pPred[(4<<4)+4] =
                pPred[(6<<4)+5] =  ((P_B + P_C + 1) >> 1);
    pPred[(0<<4)+3] =
        pPred[(2<<4)+4] =
            pPred[(4<<4)+5] =
                pPred[(6<<4)+6] =  ((P_C + P_D + 1) >> 1);
    pPred[(0<<4)+4] =
        pPred[(2<<4)+5] =
            pPred[(4<<4)+6] =
                pPred[(6<<4)+7] =  ((P_D + P_E + 1) >> 1);
    pPred[(0<<4)+5] =
        pPred[(2<<4)+6] =
            pPred[(4<<4)+7] =  ((P_E + P_F + 1) >> 1);
    pPred[(0<<4)+6] =
        pPred[(2<<4)+7] =  ((P_F + P_G + 1) >> 1);
    pPred[(0<<4)+7] =  ((P_G + P_H + 1) >> 1);
    pPred[(1<<4)+0] =
        pPred[(3<<4)+1] =
            pPred[(5<<4)+2] =
                pPred[(7<<4)+3] =  ((P_Q + P_A + 2*P_Z + 2) >> 2);
    pPred[(1<<4)+1] =
        pPred[(3<<4)+2] =
            pPred[(5<<4)+3] =
                pPred[(7<<4)+4] =  ((P_Z + P_B + 2*P_A + 2) >> 2);
    pPred[(1<<4)+2] =
        pPred[(3<<4)+3] =
            pPred[(5<<4)+4] =
                pPred[(7<<4)+5] =  ((P_A + P_C + 2*P_B + 2) >> 2);
    pPred[(1<<4)+3] =
        pPred[(3<<4)+4] =
            pPred[(5<<4)+5] =
                pPred[(7<<4)+6] =  ((P_B + P_D + 2*P_C + 2) >> 2);
    pPred[(1<<4)+4] =
        pPred[(3<<4)+5] =
            pPred[(5<<4)+6] =
                pPred[(7<<4)+7] =  ((P_C + P_E + 2*P_D + 2) >> 2);
    pPred[(1<<4)+5] =
        pPred[(3<<4)+6] =
            pPred[(5<<4)+7] =  ((P_D + P_F + 2*P_E + 2) >> 2);
    pPred[(1<<4)+6] =
        pPred[(3<<4)+7] =  ((P_E + P_G + 2*P_F + 2) >> 2);
    pPred[(1<<4)+7] =  ((P_F + P_H + 2*P_G + 2) >> 2);
    pPred[(2<<4)+0] =
        pPred[(4<<4)+1] =
            pPred[(6<<4)+2] =  ((P_R + P_Z + 2*P_Q + 2) >> 2);
    pPred[(3<<4)+0] =
        pPred[(5<<4)+1] =
            pPred[(7<<4)+2] =  ((P_S + P_Q + 2*P_R + 2) >> 2);
    pPred[(4<<4)+0] =
        pPred[(6<<4)+1] =  ((P_T + P_R + 2*P_S + 2) >> 2);
    pPred[(5<<4)+0] =
        pPred[(7<<4)+1] =  ((P_U + P_S + 2*P_T + 2) >> 2);
    pPred[(6<<4)+0] =  ((P_V + P_T + 2*P_U + 2) >> 2);
    pPred[(7<<4)+0] =  ((P_W + P_U + 2*P_V + 2) >> 2);

#endif
}

void intra_pred_luma8x8_HOR_DOWN_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 * pleftPix = mb_cache_ptr->leftPredPix_Y + 8;
    uint8 * ptopPix = mb_cache_ptr->topFilteredPix + 1;
#ifndef EXPAND_8x8_IPRED
    int32 x, y;
    int32	  zHD;

    for(y =0; y< 8; y++)
    {
        for(x =0; x<8; x++)
        {
            zHD = 2*y - x;
            if(zHD >=0)
            {
                if(!(zHD & 0x1))
                    pPred[x] = (pleftPix[y-(x>>1)-1] + pleftPix[y-(x>>1)] + 1) >>1;
                else
                    pPred[x] = (pleftPix[y-(x>>1)-2] + 2*pleftPix[y-(x>>1)-1] + pleftPix[y-(x>>1)] + 2) >>2;
            }
            else if (zHD == -1)
                pPred[x] = (pleftPix[0] + 2*ptopPix[-1] +ptopPix[0] + 2) >>2;
            else
                pPred[x] =  (ptopPix[x-2*y-1] + 2*ptopPix[x-2*y-2] + ptopPix[x-2*y-3] + 2) >>2;
        }
        pPred +=MB_SIZE;
    }
#else
    pPred[(0<<4)+0] =
        pPred[(1<<4)+2] =
            pPred[(2<<4)+4] =
                pPred[(3<<4)+6] =  ((P_Q + P_Z + 1) >> 1);
    pPred[(1<<4)+0] =
        pPred[(2<<4)+2] =
            pPred[(3<<4)+4] =
                pPred[(4<<4)+6] =  ((P_R + P_Q + 1) >> 1);
    pPred[(2<<4)+0] =
        pPred[(3<<4)+2] =
            pPred[(4<<4)+4] =
                pPred[(5<<4)+6] =  ((P_S + P_R + 1) >> 1);
    pPred[(3<<4)+0] =
        pPred[(4<<4)+2] =
            pPred[(5<<4)+4] =
                pPred[(6<<4)+6] =  ((P_T + P_S + 1) >> 1);
    pPred[(4<<4)+0] =
        pPred[(5<<4)+2] =
            pPred[(6<<4)+4] =
                pPred[(7<<4)+6] =  ((P_U + P_T + 1) >> 1);
    pPred[(5<<4)+0] =
        pPred[(6<<4)+2] =
            pPred[(7<<4)+4] =  ((P_V + P_U + 1) >> 1);
    pPred[(6<<4)+0] =
        pPred[(7<<4)+2] =  ((P_W + P_V + 1) >> 1);
    pPred[(7<<4)+0] =  ((P_X + P_W + 1) >> 1);
    pPred[(0<<4)+1] =
        pPred[(1<<4)+3] =
            pPred[(2<<4)+5] =
                pPred[(3<<4)+7] =  ((P_Q + P_A + 2*P_Z + 2) >> 2);
    pPred[(1<<4)+1] =
        pPred[(2<<4)+3] =
            pPred[(3<<4)+5] =
                pPred[(4<<4)+7] =  ((P_Z + P_R + 2*P_Q + 2) >> 2);
    pPred[(2<<4)+1] =
        pPred[(3<<4)+3] =
            pPred[(4<<4)+5] =
                pPred[(5<<4)+7] =  ((P_Q + P_S + 2*P_R + 2) >> 2);
    pPred[(3<<4)+1] =
        pPred[(4<<4)+3] =
            pPred[(5<<4)+5] =
                pPred[(6<<4)+7] =  ((P_R + P_T + 2*P_S + 2) >> 2);
    pPred[(4<<4)+1] =
        pPred[(5<<4)+3] =
            pPred[(6<<4)+5] =
                pPred[(7<<4)+7] =  ((P_S + P_U + 2*P_T + 2) >> 2);
    pPred[(5<<4)+1] =
        pPred[(6<<4)+3] =
            pPred[(7<<4)+5] =  ((P_T + P_V + 2*P_U + 2) >> 2);
    pPred[(6<<4)+1] =
        pPred[(7<<4)+3] =  ((P_U + P_W + 2*P_V + 2) >> 2);
    pPred[(7<<4)+1] =  ((P_V + P_X + 2*P_W + 2) >> 2);
    pPred[(0<<4)+2] =
        pPred[(1<<4)+4] =
            pPred[(2<<4)+6] =  ((P_Z + P_B + 2*P_A + 2) >> 2);
    pPred[(0<<4)+3] =
        pPred[(1<<4)+5] =
            pPred[(2<<4)+7] =  ((P_A + P_C + 2*P_B + 2) >> 2);
    pPred[(0<<4)+4] =
        pPred[(1<<4)+6] =  ((P_B + P_D + 2*P_C + 2) >> 2);
    pPred[(0<<4)+5] =
        pPred[(1<<4)+7] =  ((P_C + P_E + 2*P_D + 2) >> 2);
    pPred[(0<<4)+6] =  ((P_D + P_F + 2*P_E + 2) >> 2);
    pPred[(0<<4)+7] =  ((P_E + P_G + 2*P_F + 2) >> 2);
#endif
}

void intra_pred_luma8x8_VERT_LEFT_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 * ptopPix = mb_cache_ptr->topFilteredPix + 1;
#ifndef EXPAND_8x8_IPRED
    int32 x, y;

    for(y =0; y< 8; y++)
    {
        for(x =0; x<8; x++)
        {
            if(!(y & 0x1))
                pPred[x] = (ptopPix[x+(y>>1)] + ptopPix[x+(y>>1)+1]  + 1) >>1;
            else
                pPred[x] = (ptopPix[x+(y>>1)] + 2*ptopPix[x+(y>>1)+1] + ptopPix[x+(y>>1)+2] + 2) >>2;
        }
        pPred +=MB_SIZE;
    }
#else
    pPred[(0<<4)+0] =  ((P_A + P_B + 1) >> 1);
    pPred[(0<<4)+1] =
        pPred[(2<<4)+0] =  ((P_B + P_C + 1) >> 1);
    pPred[(0<<4)+2] =
        pPred[(2<<4)+1] =
            pPred[(4<<4)+0] =  ((P_C + P_D + 1) >> 1);
    pPred[(0<<4)+3] =
        pPred[(2<<4)+2] =
            pPred[(4<<4)+1] =
                pPred[(6<<4)+0] =  ((P_D + P_E + 1) >> 1);
    pPred[(0<<4)+4] =
        pPred[(2<<4)+3] =
            pPred[(4<<4)+2] =
                pPred[(6<<4)+1] =  ((P_E + P_F + 1) >> 1);
    pPred[(0<<4)+5] =
        pPred[(2<<4)+4] =
            pPred[(4<<4)+3] =
                pPred[(6<<4)+2] =  ((P_F + P_G + 1) >> 1);
    pPred[(0<<4)+6] =
        pPred[(2<<4)+5] =
            pPred[(4<<4)+4] =
                pPred[(6<<4)+3] =  ((P_G + P_H + 1) >> 1);
    pPred[(0<<4)+7] =
        pPred[(2<<4)+6] =
            pPred[(4<<4)+5] =
                pPred[(6<<4)+4] =  ((P_H + P_I + 1) >> 1);
    pPred[(2<<4)+7] =
        pPred[(4<<4)+6] =
            pPred[(6<<4)+5] =  ((P_I + P_J + 1) >> 1);
    pPred[(4<<4)+7] =
        pPred[(6<<4)+6] =  ((P_J + P_K + 1) >> 1);
    pPred[(6<<4)+7] =  ((P_K + P_L + 1) >> 1);
    pPred[(1<<4)+0] =  ((P_A + P_C + 2*P_B + 2) >> 2);
    pPred[(1<<4)+1] =
        pPred[(3<<4)+0] =  ((P_B + P_D + 2*P_C + 2) >> 2);
    pPred[(1<<4)+2] =
        pPred[(3<<4)+1] =
            pPred[(5<<4)+0] =  ((P_C + P_E + 2*P_D + 2) >> 2);
    pPred[(1<<4)+3] =
        pPred[(3<<4)+2] =
            pPred[(5<<4)+1] =
                pPred[(7<<4)+0] =  ((P_D + P_F + 2*P_E + 2) >> 2);
    pPred[(1<<4)+4] =
        pPred[(3<<4)+3] =
            pPred[(5<<4)+2] =
                pPred[(7<<4)+1] =  ((P_E + P_G + 2*P_F + 2) >> 2);
    pPred[(1<<4)+5] =
        pPred[(3<<4)+4] =
            pPred[(5<<4)+3] =
                pPred[(7<<4)+2] =  ((P_F + P_H + 2*P_G + 2) >> 2);
    pPred[(1<<4)+6] =
        pPred[(3<<4)+5] =
            pPred[(5<<4)+4] =
                pPred[(7<<4)+3] =  ((P_G + P_I + 2*P_H + 2) >> 2);
    pPred[(1<<4)+7] =
        pPred[(3<<4)+6] =
            pPred[(5<<4)+5] =
                pPred[(7<<4)+4] =  ((P_H + P_J + 2*P_I + 2) >> 2);
    pPred[(3<<4)+7] =
        pPred[(5<<4)+6] =
            pPred[(7<<4)+5] =  ((P_I + P_K + 2*P_J + 2) >> 2);
    pPred[(5<<4)+7] =
        pPred[(7<<4)+6] =  ((P_J + P_L + 2*P_K + 2) >> 2);
    pPred[(7<<4)+7] =  ((P_K + P_M + 2*P_L + 2) >> 2);

#endif
}

void intra_pred_luma8x8_HOR_UP_PRED(DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, int32 blkIdxInMB)
{
    uint8 *pleftPix = mb_cache_ptr->leftPredPix_Y + 8;
#ifndef EXPAND_8x8_IPRED
    int32 x, y;
    int32	  zHU;

    for(y = 0; y < 8; y++)
    {
        for(x =0; x<8; x++)
        {
            zHU = x + 2*y;
            if(zHU < 13)
            {
                if(!(zHU & 0x1))
                    pPred[x] = (pleftPix[y+(x>>1)] + pleftPix[y+(x>>1)+1] + 1) >>1;
                else
                    pPred[x] = (pleftPix[y+(x>>1)] + 2*pleftPix[y+(x>>1)+1] + pleftPix[y+(x>>1)+2] + 2) >>2;
            }
            else if(zHU == 13)
                pPred[x] = (pleftPix[6] + 3*pleftPix[7] + 2) >>2;
            else
                pPred[x] = pleftPix[7];
        }
        pPred +=MB_SIZE;
    }
#else
    pPred[(0<<4)+0] =  ((P_Q + P_R + 1) >> 1);
    pPred[(1<<4)+0] =
        pPred[(0<<4)+2] =  ((P_R + P_S + 1) >> 1);
    pPred[(2<<4)+0] =
        pPred[(1<<4)+2] =
            pPred[(0<<4)+4] =  ((P_S + P_T + 1) >> 1);
    pPred[(3<<4)+0] =
        pPred[(2<<4)+2] =
            pPred[(1<<4)+4] =
                pPred[(0<<4)+6] =  ((P_T + P_U + 1) >> 1);
    pPred[(4<<4)+0] =
        pPred[(3<<4)+2] =
            pPred[(2<<4)+4] =
                pPred[(1<<4)+6] =  ((P_U + P_V + 1) >> 1);
    pPred[(5<<4)+0] =
        pPred[(4<<4)+2] =
            pPred[(3<<4)+4] =
                pPred[(2<<4)+6] =  ((P_V + P_W + 1) >> 1);
    pPred[(6<<4)+0] =
        pPred[(5<<4)+2] =
            pPred[(4<<4)+4] =
                pPred[(3<<4)+6] =  ((P_W + P_X + 1) >> 1);
    pPred[(4<<4)+6] =
        pPred[(4<<4)+7] =
            pPred[(5<<4)+4] =
                pPred[(5<<4)+5] =
                    pPred[(5<<4)+6] =
                        pPred[(5<<4)+7] =
                            pPred[(6<<4)+2] =
                                pPred[(6<<4)+3] =
                                    pPred[(6<<4)+4] =
                                        pPred[(6<<4)+5] =
                                            pPred[(6<<4)+6] =
                                                    pPred[(6<<4)+7] =
                                                            pPred[(7<<4)+0] =
                                                                    pPred[(7<<4)+1] =
                                                                            pPred[(7<<4)+2] =
                                                                                    pPred[(7<<4)+3] =
                                                                                            pPred[(7<<4)+4] =
                                                                                                    pPred[(7<<4)+5] =
                                                                                                            pPred[(7<<4)+6] =
                                                                                                                    pPred[(7<<4)+7] =  P_X;
    pPred[(6<<4)+1] =
        pPred[(5<<4)+3] =
            pPred[(4<<4)+5] =
                pPred[(3<<4)+7] =  ((P_W + 3*P_X + 2) >> 2);
    pPred[(5<<4)+1] =
        pPred[(4<<4)+3] =
            pPred[(3<<4)+5] =
                pPred[(2<<4)+7] =  ((P_X + P_V + 2*P_W + 2) >> 2);
    pPred[(4<<4)+1] =
        pPred[(3<<4)+3] =
            pPred[(2<<4)+5] =
                pPred[(1<<4)+7] =  ((P_W + P_U + 2*P_V + 2) >> 2);
    pPred[(3<<4)+1] =
        pPred[(2<<4)+3] =
            pPred[(1<<4)+5] =
                pPred[(0<<4)+7] =  ((P_V + P_T + 2*P_U + 2) >> 2);
    pPred[(2<<4)+1] =
        pPred[(1<<4)+3] =
            pPred[(0<<4)+5] =  ((P_U + P_S + 2*P_T + 2) >> 2);
    pPred[(1<<4)+1] =
        pPred[(0<<4)+3] =  ((P_T + P_R + 2*P_S + 2) >> 2);
    pPred[(0<<4)+1] =  ((P_S + P_Q + 2*P_R + 2) >> 2);

#endif
}

/*************************************************************************
                        intra prediction for I16x16 MB
**************************************************************************/
//void intraPred_VERT_PRED_16 (uint8 * ptopRefPix, uint8 * pPred)
void intraPred_VERT_PRED_16 (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, uint8 *pRec, int32 pitch)
{
    int32 i;
    int32 a, b, c, d;
    int32 *pIntPixel;
    uint8 * ptopRefPix = pRec -pitch;
    int32 topAvail = mb_cache_ptr->mb_avail_b;
    int32 constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;
#ifdef _NEON_OPT_
    uint8x16_t vec128;
#endif

#ifndef _NEON_OPT_
    a = ((int32 *)ptopRefPix) [0];
    b = ((int32 *)ptopRefPix) [1];
    c = ((int32 *)ptopRefPix) [2];
    d = ((int32 *)ptopRefPix) [3];

    pIntPixel = (int32 *)pPred;

    /*store the up 16 pixel to reference MB*/
    for (i = 16; i > 0; i--)
    {
        *pIntPixel++ = a;
        *pIntPixel++ = b;
        *pIntPixel++ = c;
        *pIntPixel++ = d;
    }
#else
    vec128 = vld1q_u8(ptopRefPix);
    for (i = MB_SIZE; i > 0; i--)
    {
        vst1q_u8(pPred, vec128);
        pPred += MB_SIZE;
    }
#endif
}

void intraPred_HOR_PRED_16 (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, uint8 *pRec, int32 pitch)
{
    int32 i;
    uint8 * pPix = pRec - 1;
    uint8 pChar;
    uint32 * pIntPix;

    pChar = pPix[0];
    pPix += pitch;
#ifndef _NEON_OPT_
    pIntPix = (uint32 *)pPred;
#endif

    for (i = MB_SIZE; i > 0; i--)
    {
#ifndef _NEON_OPT_
        uint32 b;
        b = pChar * 0x01010101;

        *pIntPix++ = b;
        *pIntPix++ = b;
        *pIntPix++ = b;
        *pIntPix++ = b;
#else
        uint8x16_t v128 = vdupq_n_u8(pChar);
        vst1q_u8(pPred, v128);
        pPred += MB_SIZE;
#endif

        pChar = pPix[0];
        pPix += pitch;
    }
}

void intraPred_DC_PRED_16 (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, uint8 *pRec, int32 pitch)
{
    int32 i;
    uint32 a;
    int32 pred = 0;
    uint8 *pPix;
    uint32 * pIntPred;
    int32 topMBAvail, leftMBAvail;
    int32 availCnt = 0;
    int32 constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;
#ifdef _NEON_OPT_
    uint8x16_t vec128;
    uint16x8_t v16x8;
    uint32x4_t v32x4;
#endif
    topMBAvail = mb_cache_ptr->mb_avail_b;
    leftMBAvail = mb_cache_ptr->mb_avail_a;

    if (constrainedIntra)
    {
        if (leftMBAvail)
            leftMBAvail = (currMB - 1)->is_intra;

        if (topMBAvail)
            topMBAvail = ((H264DecContext *)img)->abv_mb_info->is_intra;
    }

    if (leftMBAvail)
    {
        pPix = pRec - 1;

        for (i = 4; i > 0; i--)
        {
            pred += pPix[0];
            pPix += pitch;
            pred += pPix[0];
            pPix += pitch;
            pred += pPix[0];
            pPix += pitch;
            pred += pPix[0];
            pPix += pitch;
        }

        availCnt++;
    }

    if (topMBAvail)
    {
#ifndef _NEON_OPT_
        pPix = pRec - pitch;

        for (i = 4; i > 0; i--)
        {
            pred += *pPix++;
            pred += *pPix++;
            pred += *pPix++;
            pred += *pPix++;
        }
#else
        uint8x16_t v8x16_top;

        pPix = pRec - pitch;
        v8x16_top = vld1q_u8(pPix);

        v16x8 = vpaddlq_u8(v8x16_top);
        v32x4 = vpaddlq_u16(v16x8);

        pred += vgetq_lane_u32(v32x4, 0);
        pred += vgetq_lane_u32(v32x4, 1);
        pred += vgetq_lane_u32(v32x4, 2);
        pred += vgetq_lane_u32(v32x4, 3);
#endif
        availCnt++;
    }

    if (availCnt == 2)
    {
        pred = (pred + 16) / 32;
    }
    else if (availCnt == 1)
    {
        pred = (pred + 8) / 16;
    }
    else
    {
        pred = 128;
    }

    /*copy pred to currMB->pred_Y*/
#ifndef _NEON_OPT_
    pIntPred = (uint32 *)pPred;
    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    for (i = 16; i > 0; i--)
    {
        *pIntPred++ = a;
        *pIntPred++ = a;
        *pIntPred++ = a;
        *pIntPred++ = a;
    }
#else
    vec128 = vdupq_n_u8((uint8)pred);
    for (i = MB_SIZE; i > 0; i--)
    {
        vst1q_u8(pPred, vec128);
        pPred += MB_SIZE;
    }
#endif
}

void intraPred_PLANE_16 (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPred, uint8 *pRec, int32 pitch)
{
    int32 i;
    int32 x, y;
    int32 H, V;
    int32 a, b, c;
    uint8 * pred;
    int32 factor_b, factor_a_c_16;
    const uint8 * pClip = g_rgiClipTab;
    uint8 *pLeftPix, *pTopPix;
    uint8 * lpt, * lpb;

    pTopPix = pRec - pitch;
    pLeftPix = pRec - 1;
    lpb = pLeftPix + 8*pitch;
    lpt = lpb - 2*pitch;

    /*compute H parameter*/
    H = V = 0;
    for (i = 1; i < 9; i++)
    {
        H += i * (pTopPix[7+i] - pTopPix[7-i]);
        V += i * (lpb[0] - lpt[0]);

        lpb += pitch;
        lpt -= pitch;
    }

    /*compute a, b, c parameter*/
    a = (pLeftPix [15*pitch] + pTopPix [15]) * 16;
    b = (5 * H + 32) >> 6;
    c = (5 * V + 32) >> 6;

    /*compute each position's pixel prediction value*/
    pred = pPred;
    factor_a_c_16 = a + c * (-7) + 16;
    for (y = 16; y > 0; y--)
    {
        factor_b = b * (-7);
        for (x = 4; x > 0; x--)
        {
            /*  *pred++ = (a + b * (x-7) + c * (y - 7) + 16 ) / 32		*/
            *pred++ = (uint8)pClip [(factor_b + factor_a_c_16) >> 5];
            factor_b += b;
            *pred++ = (uint8)pClip [(factor_b + factor_a_c_16) >> 5];
            factor_b += b;
            *pred++ = (uint8)pClip [(factor_b + factor_a_c_16) >> 5];
            factor_b += b;
            *pred++ = (uint8)pClip [(factor_b + factor_a_c_16) >> 5];
            factor_b += b;
        }

        factor_a_c_16 += c;
    }
}


/****************************chroma intra prediction*******************************/
//void intraPred_chroma_DC (DEC_IMAGE_PARAMS_T * img, DEC_MB_INFO_T * currMB, uint8 * pPredMB, int uv)
void intraPred_chroma_DC (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPredMB, uint8 *pRec, int32 pitch)
{
    uint32 pred = 0;
    int32 topBlkAvail, leftBlkAvail;
    uint8 *pTopPredPix = NULL, *pLeftPredPix = NULL;
    uint8 p0, p1, p2, p3;
    uint32 a;
    uint32 * pIntPred;
    uint32 top4_left = 0, top4_right = 0;
    uint32 left4_top = 0, left4_down = 0;
    int32 constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;
#ifdef _NEON_OPT_
    uint8x8_t v8x8_top, v8x8_left;
    uint16x4_t v16x4;
    uint32x2_t v32x2;
#endif

    topBlkAvail = mb_cache_ptr->mb_avail_b;
    leftBlkAvail = mb_cache_ptr->mb_avail_a;
    if (constrainedIntra)
    {
        topBlkAvail = topBlkAvail ?  ((H264DecContext *)img)->abv_mb_info->is_intra : 0;
        leftBlkAvail = leftBlkAvail ? (currMB - 1)->is_intra : 0;
    }

#if 1//ndef _NEON_OPT_
    /*for 1th 4x4 block*/

    /*top samples*/
    if (topBlkAvail)
    {
        pTopPredPix = pRec - pitch;

        p0 = pTopPredPix[0];
        p1 = pTopPredPix[1];
        p2 = pTopPredPix[2];
        p3 = pTopPredPix[3];
        top4_left = p0 + p1 + p2 + p3;

        pred += (uint32)top4_left;
    }

    /*left samples*/
    if (leftBlkAvail)
    {
        pLeftPredPix = pRec - 1;

        p0 = pLeftPredPix[0];
        pLeftPredPix += pitch;
        p1 = pLeftPredPix[0];
        pLeftPredPix += pitch;
        p2 = pLeftPredPix[0];
        pLeftPredPix += pitch;
        p3 = pLeftPredPix[0];
        pLeftPredPix += pitch;

        left4_top = p0 + p1 + p2 + p3;

        pred += (uint32)left4_top;
    }

    if (topBlkAvail && leftBlkAvail)
    {
        pred = (pred + 4) / 8;
    }
    else if (!topBlkAvail && !leftBlkAvail)
    {
        pred = 128;
    }
    else
    {
        pred = (pred + 2) / 4;
    }

    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    pIntPred = (uint32 *)pPredMB;
    pIntPred[0] = a;
    pIntPred[2] = a;
    pIntPred[4] = a;
    pIntPred[6] = a;

    /*for 2th 4x4 block*/
    if (topBlkAvail)
    {
        //lint -save -e644
        p0 = pTopPredPix [4];
        p1 = pTopPredPix [5];
        p2 = pTopPredPix [6];
        p3 = pTopPredPix [7];

        top4_right = p0 + p1 + p2 + p3;
        pred = (top4_right + 2) / 4;
    }
    else if (leftBlkAvail)
    {
        pred = (left4_top + 2) / 4;
    }
    else
    {
        pred = 128;
    }

    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    pIntPred = (uint32 *)(pPredMB + 4);
    pIntPred[0] = a;
    pIntPred[2] = a;
    pIntPred[4] = a;
    pIntPred[6] = a;

    /*for 3th 4x4 block*/
    if (leftBlkAvail)
    {
        p0 = pLeftPredPix[0];
        pLeftPredPix += pitch;
        p1 = pLeftPredPix[0];
        pLeftPredPix += pitch;
        p2 = pLeftPredPix[0];
        pLeftPredPix += pitch;
        p3 = pLeftPredPix[0];
        pLeftPredPix += pitch;

        left4_down = p0 + p1 + p2 + p3;
        pred = (left4_down + 2) / 4;
    }
    else if (topBlkAvail)
    {
        pred = (top4_left + 2) / 4;
    }
    else
        pred = 128;

    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    pIntPred = (uint32 *)(pPredMB + MB_CHROMA_SIZE_X4);
    pIntPred[0] = a;
    pIntPred[2] = a;
    pIntPred[4] = a;
    pIntPred[6] = a;


    /*for 4th 4x4 block*/
    pred = 0;
    if (topBlkAvail)
        pred += (uint32)top4_right;

    if (leftBlkAvail)
        pred += (uint32)left4_down;

    if (topBlkAvail && leftBlkAvail)
        pred = (pred + 4) / 8;
    else if (!topBlkAvail && !leftBlkAvail)
        pred = 128;
    else
        pred = (pred + 2) / 4;

    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    pIntPred = (uint32 *)(pPredMB + MB_CHROMA_SIZE_X4 + 4);
    pIntPred[0] = a;
    pIntPred[2] = a;
    pIntPred[4] = a;
    pIntPred[6] = a;
#else
    v8x8_top = vld1_u8(pRec - pitch);
    v8x8_left = vld1_u8(pRec - 1);

    //top
    v16x4 = vpaddl_u8(v8x8_top);
    v32x2 = vpaddl_u16(v16x4);

    top4_left = vget_lane_u32(v32x2, 0);
    top4_right = vget_lane_u32(v32x2, 1);

    //left
    v16x4 = vpaddl_u8(v8x8_left);
    v32x2 = vpaddl_u16(v16x4);

    left4_top = vget_lane_u32(v32x2, 0);
    left4_down = vget_lane_u32(v32x2, 1);

    /*for 1th 4x4 block*/
    if (topBlkAvail)		/*top samples*/
    {
        pred += (uint32)top4_left;
    }

    if (leftBlkAvail)		/*left samples*/
    {
        pred += (uint32)left4_top;
    }

    if (topBlkAvail && leftBlkAvail)
    {
        pred = (pred + 4) / 8;
    } else if (!topBlkAvail && !leftBlkAvail)
    {
        pred = 128;
    } else
    {
        pred = (pred + 2) / 4;
    }
    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    v32x2 = vset_lane_u32(a, v32x2, 0);

    /*for 2th 4x4 block*/
    if (topBlkAvail)
    {
        pred = (top4_right + 2) / 4;
    } else if (leftBlkAvail)
    {
        pred = (left4_top + 2) / 4;
    } else
    {
        pred = 128;
    }
    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    v32x2 = vset_lane_u32(a, v32x2, 1);

    //store the top 4 line
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;

    /*for 3th 4x4 block*/
    if (leftBlkAvail)
    {
        pred = (left4_down + 2) / 4;
    } else if (topBlkAvail)
    {
        pred = (top4_left + 2) / 4;
    } else
    {
        pred = 128;
    }
    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    v32x2 = vset_lane_u32(a, v32x2, 0);

    /*for 4th 4x4 block*/
    pred = 0;
    if (topBlkAvail)
    {
        pred += (uint32)top4_right;
    }

    if (leftBlkAvail)
    {
        pred += (uint32)left4_down;
    }

    if (topBlkAvail && leftBlkAvail)
    {
        pred = (pred + 4) / 8;
    } else if (!topBlkAvail && !leftBlkAvail)
    {
        pred = 128;
    } else
    {
        pred = (pred + 2) / 4;
    }
    a = (pred << 24) | (pred << 16) | (pred << 8) | pred;
    v32x2 = vset_lane_u32(a, v32x2, 1);

    //store the bottom 4 line
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;
    vst1_u32((uint32*)pPredMB, v32x2);
    pPredMB += MB_CHROMA_SIZE;
    vst1_u32((uint32*)pPredMB, v32x2);//	pPredMB += MB_CHROMA_SIZE;
#endif
}

void intraPred_chroma_Hor (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPredMB, uint8 *pRec, int32 pitch)
{
    int32 i;
    uint8 *pPredPix;
    uint8 pred;
    uint32 a, *pIntPredMB;

    pPredPix = pRec - 1;
#ifndef _NEON_OPT_
    pIntPredMB = (uint32 *)pPredMB;
#endif
    pred = pPredPix[0];
    pPredPix += pitch;

    for (i = MB_CHROMA_SIZE; i > 0; i--)
    {
#ifndef _NEON_OPT_
        a = pred | (pred << 8);
        a = a | (a << 16);
        *pIntPredMB++ = a;
        *pIntPredMB++ = a;
#else
        uint8x8_t v8x8 = vdup_n_u8(pred);
        vst1_u8(pPredMB, v8x8);
        pPredMB += MB_CHROMA_SIZE;
#endif
        pred = pPredPix[0];
        pPredPix += pitch;
    }
}

void intraPred_chroma_Ver (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPredMB, uint8 *pRec, int32 pitch)
{
    int32 i;
    uint8 * pPredPix;
    int32 topAvail;
    int32 constrainedIntra = ((H264DecContext *)img)->constrained_intra_pred_flag;
#ifdef _IPRED_NEON_OPT_
    uint8x8_t v8x8;
#else
    int32 a, b;
    uint32 * pIntPredMB;
#endif

    topAvail = mb_cache_ptr->mb_avail_b;

    if (constrainedIntra)
    {
        topAvail = topAvail ?  ((H264DecContext *)img)->abv_mb_info->is_intra : 0;
    }

#ifndef _IPRED_NEON_OPT_
    pPredPix = pRec - pitch;

    a = ((int32 *)pPredPix)[0];
    b = ((int32 *)pPredPix)[1];

    pIntPredMB = (uint32 *)pPredMB;
    for (i = 8; i > 0; i--)
    {
        *pIntPredMB++ = a;
        *pIntPredMB++ = b;
    }
#else
    v8x8 = vld1_u8(pRec - pitch);
    for (i = MB_CHROMA_SIZE; i > 0; i--)
    {
        vst1_u8(pPredPix, v8x8);
        pPredPix += MB_CHROMA_SIZE;
    }
#endif

    return;
}

void intraPred_chroma_Plane (void *img, DEC_MB_INFO_T *currMB, DEC_MB_CACHE_T *mb_cache_ptr, uint8 *pPredMB, uint8 *pRec, int32 pitch)
{
    int32 i, j;
    int32 a, b, c;
    int32 H, V;
    uint8 * pLeftPix, *pTopPix;
    uint8 * lpt, *lpb;
    const uint8 * pClipTable = g_rgiClipTab;
    int32 factor_a_c, factor_b;

    pTopPix = pRec - pitch;
    pLeftPix = pRec - 1;
    lpb = pLeftPix + 4*pitch;
    lpt = lpb - 2*pitch;

    /*compute H/V parameter*/
    H = V =0;
    for (i = 1; i < 5; i++)
    {
        H += i * (pTopPix[3+i] - pTopPix[3-i]);
        V += i * (lpb[0] - lpt[0]);

        lpb += pitch;
        lpt -= pitch;
    }


    a = (pLeftPix [7*pitch] + pTopPix [7]) * 16;
    b = (H * 17 + 16) >> 5;
    c = (V * 17 + 16) >> 5;

    factor_a_c = a + c * (- 3) + 16;

    for (j = 8; j > 0; j--)
    {
        factor_b = b * (-3);
        for (i = 8; i > 0; i--)
        {
            *pPredMB++ = pClipTable [(factor_a_c + factor_b) >> 5];

            factor_b += b;
        }
        factor_a_c += c;
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
