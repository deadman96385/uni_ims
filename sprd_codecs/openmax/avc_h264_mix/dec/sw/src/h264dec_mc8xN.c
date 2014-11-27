/******************************************************************************
 ** File Name:    h264dec_mc8x8.c		                                      *
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

#ifdef _ARM_MC_ASSEMBLY_
#include "arm_interpolation.h"
#endif
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
#if 1 //WIN32
//N: 8 or 16
void MC_luma8xN_dx0dy0 (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 i;
    int32 width = img_ptr->ext_width;
#ifdef _NEON_OPT_
    uint8x8_t vec64;
#endif

    for (i = N; i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        int32 j;

        for (j = 0; j < 8; j++)
        {
            pPred [j] = pRefFrame [j];
        }
        pRefFrame += width;
#else
        vec64 = vld1_u8(pRefFrame);
        pRefFrame += width;
        vst1_u8(pPred, vec64);
#endif

        pPred += MB_SIZE;
    }
}
#endif

#if 1 //WIN32
//N: 8 or 16, M: 1 or 3
void MC_luma8xN_dxMdy0 (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 i;
    int32 width = img_ptr->ext_width;
    int32 col = ((img_ptr->g_refPosx & 0x3) == 0x1) ? 2 : 3;
#ifdef _NEON_OPT_
    uint8x8_t v64[6], v64_next;
    uint16x8_t v05, v14, v23;
    uint16x8_t halfpix;
#endif

    for (i = N; i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        int32 j;
        const uint8 * pClipTab = g_rgiClipTab;
        int32 p0, p1, p2, p3, p4, p5;
        uint8 halfpix;

        for (j = 0; j < 8; j++)	//x dir
        {
            p0 = pRefFrame [j - 2];
            p1 = pRefFrame [j - 1];
            p2 = pRefFrame [j + 0];
            p3 = pRefFrame [j + 1];
            p4 = pRefFrame [j + 2];
            p5 = pRefFrame [j + 3];

            halfpix = (uint8)pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 16) / 32];
            if (col == 2)
            {
                pPred[j] = (halfpix + p2 + 1)>>1;
            } else
            {
                pPred[j] = (halfpix + p3 + 1)>>1;
            }
        }
#else
        v64_next = vld1_u8(pRefFrame + 6);

        v64[0] = vld1_u8(pRefFrame - 2);
        v64[5] = vext_u8(v64[0], v64_next, 5);
        v05 = vaddl_u8(v64[0], v64[5]);

        v64[2] = vext_u8(v64[0], v64_next, 2);
        v64[3] = vext_u8(v64[0], v64_next, 3);
        v23 = vaddl_u8(v64[2], v64[3]);

        halfpix = vmlaq_n_u16(v05, v23, 20);

        v64[1] = vext_u8(v64[0], v64_next, 1);
        v64[4] = vext_u8(v64[0], v64_next, 4);
        v14 = vaddl_u8(v64[1], v64[4]);

        halfpix = vmlsq_n_u16(halfpix, v14, 5);
        v64[0] = vqrshrun_n_s16(vreinterpretq_s16_u16(halfpix), 5);

        //(a+b+1)/2
        v64[0] = vrhadd_u8(v64[0], v64[col]);
        vst1_u8(pPred, v64[0]);
#endif

        //update for next line
        pPred += MB_SIZE;
        pRefFrame += width;
    }
}
#endif

#if 1 //WIN32
//N: 8 or 16
void MC_luma8xN_yfull (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 i;
    int32 width = img_ptr->ext_width;
#ifdef _NEON_OPT_
    uint8x8_t v64[6], v64_next;
    uint16x8_t v05, v14, v23;
    uint16x8_t halfpix;
#endif

    for (i = N; i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        int32 j;
        const uint8 * pClipTab = g_rgiClipTab;
        int32 p0, p1, p2, p3, p4, p5;

        for (j = 0; j < 8; j++)	//x dir
        {
            p0 = pRefFrame [j-2];
            p1 = pRefFrame [j-1];
            p2 = pRefFrame [j+0];
            p3 = pRefFrame [j+1];
            p4 = pRefFrame [j+2];
            p5 = pRefFrame [j+3];

            pPred[j] = pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 16) / 32];
        }
#else
        v64_next = vld1_u8(pRefFrame  + 6);

        v64[0] = vld1_u8(pRefFrame  - 2);
        v64[5] = vext_u8(v64[0], v64_next, 5);
        v05 = vaddl_u8(v64[0], v64[5]);

        v64[2] = vext_u8(v64[0], v64_next, 2);
        v64[3] = vext_u8(v64[0], v64_next, 3);
        v23 = vaddl_u8(v64[2], v64[3]);

        halfpix = vmlaq_n_u16(v05, v23, 20);

        v64[1] = vext_u8(v64[0], v64_next, 1);
        v64[4] = vext_u8(v64[0], v64_next, 4);
        v14 = vaddl_u8(v64[1], v64[4]);

        halfpix = vmlsq_n_u16(halfpix, v14, 5);
        v64[0] = vqrshrun_n_s16(vreinterpretq_s16_u16(halfpix), 5);

        vst1_u8(pPred, v64[0]);
#endif
        pPred += MB_SIZE;
        pRefFrame += width;
    }
}
#endif

#if 1 //WIN32
//N: 8 or 16, M: 1 or 3
void MC_luma8xN_dx0dyM (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 i;
    uint8 * pRefTemp;//, * pPredTemp;
    int32 width = img_ptr->ext_width;
    int32 row = ((img_ptr->g_refPosy & 0x3) == 0x1) ? 0x2 : 0x3;
#ifdef _NEON_OPT_
    uint8x8_t v64[6];
    uint16x8_t v05, v14, v23;
    uint16x8_t halfpix;
#endif

    pRefFrame = pRefFrame - 2 * width;

#ifndef _NEON_OPT_
    for (i = 0; i < 8; i++) //x dir
    {
        int32 j;
        const uint8 * pClipTab = g_rgiClipTab;
        int32 p0, p1, p2, p3, p4, p5;
        uint8 halfpix;

        pRefTemp = pRefFrame + i;
        //pPredTemp = pPred + i;

        for (j = 0; j < N; j++) //y dir
        {
            p0 = pRefTemp[0 * width];
            p1 = pRefTemp[1 * width];
            p2 = pRefTemp[2 * width];
            p3 = pRefTemp[3 * width];
            p4 = pRefTemp[4 * width];
            p5 = pRefTemp[5 * width];

            halfpix = (uint8)pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 16) / 32];
            if (row == 2)
            {
                pPred[j * MB_SIZE]  = (halfpix + p2 + 1)>>1;
            } else
            {
                pPred[j * MB_SIZE]	= (halfpix + p3 + 1)>>1;
            }
            //update for next line
            pRefTemp += width;
        }

        //update for next pix
        pPred++;
    }
#else
    v64[0] = vld1_u8(pRefFrame + 0*width);
    v64[1] = vld1_u8(pRefFrame + 1*width);
    v64[2] = vld1_u8(pRefFrame + 2*width);
    v64[3] = vld1_u8(pRefFrame + 3*width);
    v64[4] = vld1_u8(pRefFrame + 4*width);
    v64[5] = vld1_u8(pRefFrame + 5*width);
    pRefFrame += width;

    for (i = N; i > 0; i--)	//y dir
    {
        v05 = vaddl_u8(v64[0], v64[5]);
        v23 = vaddl_u8(v64[2], v64[3]);
        halfpix = vmlaq_n_u16(v05, v23, 20);

        v14 = vaddl_u8(v64[1], v64[4]);
        halfpix = vmlsq_n_u16(halfpix, v14, 5);

        v64[0] = vqrshrun_n_s16(vreinterpretq_s16_u16(halfpix), 5);
        v64[0] = vrhadd_u8(v64[0], v64[row]);

        vst1_u8(pPred, v64[0]);

        //update for next line
        v64[0] = v64[1];
        v64[1] = v64[2];
        v64[2] = v64[3];
        v64[3] = v64[4];
        v64[4] = v64[5];
        v64[5] = vld1_u8(pRefFrame + 5*width);
        pRefFrame += width;
        pPred += MB_SIZE;
    }
#endif
}
#endif

#if 1 //WIN32
void MC_luma8xN_xyqpix (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 i;
    int32 dx, dy;
    uint8 * pRefHorFilter;
    uint8 * pRefVerFilter;
    int32 width = img_ptr->ext_width;
    uint8 * pPredTemp = pPred;
#ifndef _NEON_OPT_
    int32 j, halfPix;
    int32 p0, p1, p2, p3, p4, p5;
    uint8 * pRefTemp;
    const uint8 * pClipTab = g_rgiClipTab;
#else
    uint8x8_t v64[6], v64_tmp;
    uint16x8_t v05, v14, v23;
    uint16x8_t halfpix;
#endif

    dx = img_ptr->g_refPosx & 0x3;
    pRefVerFilter = pRefFrame + (dx >> 1) - width * 2;

    //first, vertical interpolation
#ifndef _NEON_OPT_
//	pPredTemp += 7;
    for (i = 0; i < 8; i++)	//x dir
    {
        pRefTemp = pRefVerFilter + i;
        for (j = 0; j < N; j++)	//y dir
        {
            p0 = pRefTemp[0 * width];
            p1 = pRefTemp[1 * width];
            p2 = pRefTemp[2 * width];
            p3 = pRefTemp[3 * width];
            p4 = pRefTemp[4 * width];
            p5 = pRefTemp[5 * width];

            pPredTemp[MB_SIZE * j] = (uint8)pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 16) / 32];

            //update for next line
            pRefTemp += width;
        }
        //update for next pix
        pPredTemp++;
    }
#else
    v64[0] = vld1_u8(pRefVerFilter + 0*width);
    v64[1] = vld1_u8(pRefVerFilter + 1*width);
    v64[2] = vld1_u8(pRefVerFilter + 2*width);
    v64[3] = vld1_u8(pRefVerFilter + 3*width);
    v64[4] = vld1_u8(pRefVerFilter + 4*width);
    v64[5] = vld1_u8(pRefVerFilter + 5*width);
    pRefVerFilter += width;

    for (i = N; i > 0; i--)	//y dir
    {
        v05 = vaddl_u8(v64[0], v64[5]);
        v23 = vaddl_u8(v64[2], v64[3]);
        halfpix = vmlaq_n_u16(v05, v23, 20);

        v14 = vaddl_u8(v64[1], v64[4]);
        halfpix = vmlsq_n_u16(halfpix, v14, 5);

        v64[0] = vqrshrun_n_s16(vreinterpretq_s16_u16(halfpix), 5);
        vst1_u8(pPredTemp, v64[0]);

        //update for next line
        pPredTemp += MB_SIZE;
        v64[0] = v64[1];
        v64[1] = v64[2];
        v64[2] = v64[3];
        v64[3] = v64[4];
        v64[4] = v64[5];
        v64[5] = vld1_u8(pRefVerFilter + 5*width);
        pRefVerFilter += width;
    }
#endif

    if (!dx)	return; //xfull

    /*luma 8x8 horizontal interpolation and bilinear interpolation*/
    dy = img_ptr->g_refPosy & 0x3;
    pRefHorFilter = pRefFrame + width * (dy >> 1) - 2;

    /*get horizontal interpolate half pixel, and compute quarter pixel*/
    for (i = N; i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        for (j = 0; j < 8; j++)	//x dir
        {
            p0 = pRefHorFilter [j + 0];
            p1 = pRefHorFilter [j + 1];
            p2 = pRefHorFilter [j + 2];
            p3 = pRefHorFilter [j + 3];
            p4 = pRefHorFilter [j + 4];
            p5 = pRefHorFilter [j + 5];

            halfPix = (uint8)pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 16) / 32];
            pPred [j] = (halfPix + pPred [j] + 1) / 2;
        }
#else
        v64_tmp = vld1_u8(pRefHorFilter + 8);

        v64[0] = vld1_u8(pRefHorFilter);
        v64[5] = vext_u8(v64[0], v64_tmp, 5);
        v05 = vaddl_u8(v64[0], v64[5]);

        v64[2] = vext_u8(v64[0], v64_tmp, 2);
        v64[3] = vext_u8(v64[0], v64_tmp, 3);
        v23 = vaddl_u8(v64[2], v64[3]);

        halfpix = vmlaq_n_u16(v05, v23, 20);

        v64[1] = vext_u8(v64[0], v64_tmp, 1);
        v64[4] = vext_u8(v64[0], v64_tmp, 4);
        v14 = vaddl_u8(v64[1], v64[4]);

        halfpix = vmlsq_n_u16(halfpix, v14, 5);
        v64[0]  = vqrshrun_n_s16(vreinterpretq_s16_u16(halfpix), 5);

        v64_tmp = vld1_u8(pPred);
        v64[0] = vrhadd_u8(v64[0], v64_tmp);

        vst1_u8(pPred, v64[0]);
#endif

        pPred += MB_SIZE;
        pRefHorFilter += width;
    }
}
#endif

#if 1 //WIN32
/******************************************************************
get 8x(5+N) half pixel, put into temp 8x(5+N) buffer
for dx2dy1, dx2y2, dx2dy3
*******************************************************************/
void luma13x5pN_interpolation_hor (uint8 * pRefFrame, int16 * pHalfPix, int32 width, int32 N)
{
    int32 i;
#ifdef _NEON_OPT_
    uint8x8_t v64[6], v64_next;
    uint16x8_t v05, v14, v23;
    uint16x8_t halfpix;
#endif

    pRefFrame = pRefFrame  - 2 * width;

    for (i = (5 + N); i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        int32 j;
        int32 p0, p1, p2, p3, p4, p5;

        for (j = 0; j < 8; j++)	//x dir
        {
            p0 = pRefFrame [j-2];
            p1 = pRefFrame [j-1];
            p2 = pRefFrame [j+0];
            p3 = pRefFrame [j+1];
            p4 = pRefFrame [j+2];
            p5 = pRefFrame [j+3];
            *pHalfPix++ = (p0+p5) - (p1+p4)*5 + (p2+p3)*20;
        }
#else
        v64_next = vld1_u8(pRefFrame + 6);

        v64[0] = vld1_u8(pRefFrame - 2);
        v64[5] = vext_u8(v64[0], v64_next, 5);
        v05 = vaddl_u8(v64[0], v64[5]);

        v64[2] = vext_u8(v64[0], v64_next, 2);
        v64[3] = vext_u8(v64[0], v64_next, 3);
        v23 = vaddl_u8(v64[2], v64[3]);

        halfpix = vmlaq_n_u16(v05, v23, 20);

        v64[1] = vext_u8(v64[0], v64_next, 1);
        v64[4] = vext_u8(v64[0], v64_next, 4);
        v14 = vaddl_u8(v64[1], v64[4]);
        halfpix = vmlsq_n_u16(halfpix, v14, 5);

        vst1q_s16(pHalfPix, vreinterpretq_s16_u16(halfpix));
        pHalfPix += 8;
#endif

        //update for next line
        pRefFrame += width;
    }
}

/*
description: 8x(5+N) half pixel vertical interpolation
pSrc: 8x(5+N) half pixel
*/
void luma8x5pN_interpolation_ver (int16 * pHalfPix, uint8 * pPred, int32 N)
{
    int32 i;
#ifndef _NEON_OPT_
    int32 j;
    const uint8 * pClipTab = g_rgiClipTab;
    int32 p0, p1, p2, p3, p4, p5;

    for (i = 8; i > 0; i--)	//x dir
    {
        for (j = 0; j < N; j++)	//y dir
        {
            p0 = pHalfPix [(j+0) * 8];
            p1 = pHalfPix [(j+1) * 8];
            p2 = pHalfPix [(j+2) * 8];
            p3 = pHalfPix [(j+3) * 8];
            p4 = pHalfPix [(j+4) * 8];
            p5 = pHalfPix [(j+5) * 8];

            pPred [j * MB_SIZE] = (uint8)pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 512) / 1024];
        }

        //update for next pix
        pPred++;
        pHalfPix++;
    }
#else
    int16x8_t v16x8[6];
    int16x8_t v05, v14, v23;
    int16x4_t v16x4;
    int32x4_t v32x4;
    uint16x4_t p16x4_low, p16x4_high;
    uint16x8_t p16x8;

    v16x8[0] = vld1q_s16(pHalfPix + 0*8);
    v16x8[1] = vld1q_s16(pHalfPix + 1*8);
    v16x8[2] = vld1q_s16(pHalfPix + 2*8);
    v16x8[3] = vld1q_s16(pHalfPix + 3*8);
    v16x8[4] = vld1q_s16(pHalfPix + 4*8);
    v16x8[5] = vld1q_s16(pHalfPix + 5*8);
    pHalfPix += 8;

    for (i = N; i > 0; i--)	//y dir
    {
        v05 = vaddq_s16(v16x8[0], v16x8[5]);
        v14 = vaddq_s16(v16x8[1], v16x8[4]);
        v23 = vaddq_s16(v16x8[2], v16x8[3]);

        //the low 4 data
        v16x4 = vget_low_s16(v23);
        v32x4 = vmull_n_s16(v16x4, 20);

        v16x4 = vget_low_s16(v05);
        v32x4 = vaddw_s16(v32x4, v16x4);

        v16x4 = vget_low_s16(v14);
        v32x4 = vmlsl_n_s16(v32x4, v16x4, 5);
        p16x4_low = vqrshrun_n_s32(v32x4, 10);

        //the high 4 data
        v16x4 = vget_high_s16(v23);
        v32x4 = vmull_n_s16(v16x4, 20);

        v16x4 = vget_high_s16(v05);
        v32x4 = vaddw_s16(v32x4, v16x4);

        v16x4 = vget_high_s16(v14);
        v32x4 = vmlsl_n_s16(v32x4, v16x4, 5);
        p16x4_high = vqrshrun_n_s32(v32x4, 10);

        p16x8 = vcombine_u16(p16x4_low, p16x4_high);
        vst1_u8(pPred, vqmovn_u16(p16x8));

        //update for next line
        pPred += MB_SIZE;
        v16x8[0] = v16x8[1];
        v16x8[1] = v16x8[2];
        v16x8[2] = v16x8[3];
        v16x8[3] = v16x8[4];
        v16x8[4] = v16x8[5];
        v16x8[5] = vld1q_s16(pHalfPix + 5*8);
        pHalfPix += 8;
    }
#endif
}
#endif

#if 1 //WIN32
void bilinear_filter8xN_short (int16 * phalfPix, uint8 * pPred, int32 width, int32 N)
{
    int32 i;
#ifdef _NEON_OPT_
    uint8x8_t v8x8_pred, v8x8_hpred;
    int16x8_t v16x8_hpred;
#endif

    for (i = N; i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        int32 j;
        const uint8 * pClipTab = g_rgiClipTab;

        for (j = 0; j < 8; j++)	//x dir
        {
            pPred [j] = (pPred [j] + pClipTab[(phalfPix[j]+16)/32] + 1) / 2;
        }
#else
        v16x8_hpred = vld1q_s16(phalfPix);
        v8x8_pred = vld1_u8(pPred);

        v8x8_hpred = vqrshrun_n_s16(v16x8_hpred, 5);
        v8x8_pred = vrhadd_u8(v8x8_pred, v8x8_hpred);
        vst1_u8(pPred, v8x8_pred);
#endif
        pPred += MB_SIZE;
        phalfPix += width;
    }
}
#endif

#if 1 //WIN32
void MC_luma8xN_xhalf (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 dy;
//	uint8 * pRefBlk;
    int32 width = img_ptr->ext_width;
    int16 *pHalfPix = img_ptr->g_halfPixTemp;

//	pRefBlk = pRefFrame  - 2 * width;
    luma13x5pN_interpolation_hor (pRefFrame, pHalfPix, width, N);

    luma8x5pN_interpolation_ver (pHalfPix, pPred, N);

    dy = img_ptr->g_refPosy & 0x3;
    if (dy != 2)
    {
        bilinear_filter8xN_short (pHalfPix + (2 + (dy>>1))*8, pPred, 8, N);
    }
}
#endif

#if 1 //WIN32
/*******************************************************************/
/*get 13xN half pixel*/
/*******************************************************************/
void luma13xN_interpolation_ver (uint8 * pRefFrame, int16 * pHalfPix, int32 width, int32 N)
{
    int32 i;
#ifndef _NEON_OPT_
    int32 j;
    uint8 * pRefTmp;
    int32 p0, p1, p2, p3, p4, p5;
#else
    uint8x16_t v128[6];
    uint8x8_t v64[6];
    uint16x8_t v05, v14, v23;
    uint16x8_t halfpix;
#endif

    pRefFrame = pRefFrame -2 * width - 2;

#ifndef _NEON_OPT_	// 13 -> 16, for neon opt
//	pHalfPix += 12;
    for (i = 0; i < 13; i++)	//x dir
    {
        pRefTmp = pRefFrame + i;

        for (j = 0; j < N; j++)	//y dir
        {
            p0 = pRefTmp[(j+0)*width];
            p1 = pRefTmp[(j+1)*width];
            p2 = pRefTmp[(j+2)*width];
            p3 = pRefTmp[(j+3)*width];
            p4 = pRefTmp[(j+4)*width];
            p5 = pRefTmp[(j+5)*width];

            pHalfPix [j * 16] = (p0+p5) - (p1+p4)*5 + (p2+p3)*20;
        }

        //update for next pix
        pHalfPix++;
    }
#else

    v128[0] = vld1q_u8(pRefFrame + 0*width);
    v128[1] = vld1q_u8(pRefFrame + 1*width);
    v128[2] = vld1q_u8(pRefFrame + 2*width);
    v128[3] = vld1q_u8(pRefFrame + 3*width);
    v128[4] = vld1q_u8(pRefFrame + 4*width);
    v128[5] = vld1q_u8(pRefFrame + 5*width);
    pRefFrame += width;

    for (i = N; i > 0; i--)	//y dir
    {
        //the first 8 pix
        v64[0] = vget_low_u8(v128[0]);
        v64[5] = vget_low_u8(v128[5]);
        v05 = vaddl_u8(v64[0], v64[5]);

        v64[2] = vget_low_u8(v128[2]);
        v64[3] = vget_low_u8(v128[3]);
        v23 = vaddl_u8(v64[2], v64[3]);

        halfpix = vmlaq_n_u16(v05, v23, 20);

        v64[1] = vget_low_u8(v128[1]);
        v64[4] = vget_low_u8(v128[4]);
        v14 = vaddl_u8(v64[1], v64[4]);

        halfpix = vmlsq_n_u16(halfpix, v14, 5);

        vst1q_s16(pHalfPix, vreinterpretq_s16_u16(halfpix));
        pHalfPix += 8;

        //the second 8 pix
        v64[0] = vget_high_u8(v128[0]);
        v64[5] = vget_high_u8(v128[5]);
        v05 = vaddl_u8(v64[0], v64[5]);

        v64[2] = vget_high_u8(v128[2]);
        v64[3] = vget_high_u8(v128[3]);
        v23 = vaddl_u8(v64[2], v64[3]);

        halfpix = vmlaq_n_u16(v05, v23, 20);

        v64[1] = vget_high_u8(v128[1]);
        v64[4] = vget_high_u8(v128[4]);
        v14 = vaddl_u8(v64[1], v64[4]);

        halfpix = vmlsq_n_u16(halfpix, v14, 5);

        vst1q_s16(pHalfPix, vreinterpretq_s16_u16(halfpix));
        pHalfPix += 8;

        //update for next line
        v128[0] = v128[1];
        v128[1] = v128[2];
        v128[2] = v128[3];
        v128[3] = v128[4];
        v128[4] = v128[5];
        v128[5] = vld1q_u8(pRefFrame + 5*width);
        pRefFrame += width;
    }
#endif
}

void luma13xN_interpolation_hor (int16 * pHalfPix, uint8 * pPred, int32 N)
{
    int32 i;
#ifndef _NEON_OPT_
    const uint8 * pClipTab = g_rgiClipTab;
    int32 p0, p1, p2, p3, p4, p5;
#else
    int16x8_t v16x8[6], v16x8_next;
    int16x8_t v05, v14, v23;
    int16x4_t v16x4;
    int32x4_t v32x4;
    uint16x4_t p16x4_low, p16x4_high;
    uint16x8_t p16x8;
#endif

    for (i = N; i > 0; i--)	//y dir
    {
#ifndef _NEON_OPT_
        int j;

        for (j = 0; j < 8; j++) //x dir
        {
            p0 = pHalfPix[j+0];
            p1 = pHalfPix[j+1];
            p2 = pHalfPix[j+2];
            p3 = pHalfPix[j+3];
            p4 = pHalfPix[j+4];
            p5 = pHalfPix[j+5];

            pPred[j] = (uint8)pClipTab [((p0+p5) - (p1+p4)*5 + (p2+p3)*20 + 512) / 1024];
        }
#else
        v16x8[0] = vld1q_s16(pHalfPix);
        v16x8_next = vld1q_s16(pHalfPix + 8);

        v16x8[1] = vextq_s16(v16x8[0], v16x8_next, 1);
        v16x8[2] = vextq_s16(v16x8[0], v16x8_next, 2);
        v16x8[3] = vextq_s16(v16x8[0], v16x8_next, 3);
        v16x8[4] = vextq_s16(v16x8[0], v16x8_next, 4);
        v16x8[5] = vextq_s16(v16x8[0], v16x8_next, 5);

        v05 = vaddq_s16(v16x8[0], v16x8[5]);
        v14 = vaddq_s16(v16x8[1], v16x8[4]);
        v23 = vaddq_s16(v16x8[2], v16x8[3]);

        //the low 4 data
        v16x4 = vget_low_s16(v23);
        v32x4 = vmull_n_s16(v16x4, 20);

        v16x4 = vget_low_s16(v05);
        v32x4 = vaddw_s16(v32x4, v16x4);

        v16x4 = vget_low_s16(v14);
        v32x4 = vmlsl_n_s16(v32x4, v16x4, 5);
        p16x4_low = vqrshrun_n_s32(v32x4, 10);

        //the high 4 data
        v16x4 = vget_high_s16(v23);
        v32x4 = vmull_n_s16(v16x4, 20);

        v16x4 = vget_high_s16(v05);
        v32x4 = vaddw_s16(v32x4, v16x4);

        v16x4 = vget_high_s16(v14);
        v32x4 = vmlsl_n_s16(v32x4, v16x4, 5);
        p16x4_high = vqrshrun_n_s32(v32x4, 10);

        p16x8 = vcombine_u16(p16x4_low, p16x4_high);
        vst1_u8(pPred, vqmovn_u16(p16x8));
#endif

        pHalfPix += 16;
        pPred += MB_SIZE;
    }
}

//only for dx = 1 or 3, because dx = 2 has been implemented with MC_luma8xN_xhalf() func.
void MC_luma8xN_yhalf (H264DecContext *img_ptr, uint8 * pRefFrame, uint8 * pPred, int32 N)
{
    int32 dx;
    int32 width = img_ptr->ext_width;
    int16 *pHalfPix = img_ptr->g_halfPixTemp;

    luma13xN_interpolation_ver (pRefFrame, pHalfPix, width, N);

    luma13xN_interpolation_hor (pHalfPix, pPred, N);

    /*bilinear filter*/
    dx = img_ptr->g_refPosx & 0x3;
    bilinear_filter8xN_short (pHalfPix + 2 + (dx>>1), pPred, 16, N);
}
#endif

#if 1 //WIN32
//N: 4 or 2
void PC_MC_chroma4xN (H264DecContext *img_ptr, uint8 ** pRefFrame, uint8 ** pPredUV, int32 N)
{
    int32 i;
    int32 uv;
    int32 dx1, dy1;
    int32 dx2, dy2;
    int32 offset;
    uint8 * pPred;
    uint8 downRight, upRight, downLeft, upLeft;
    uint8 * pRefUp, * pRefDown;
    int32 width_c = img_ptr->ext_width>>1;
#ifdef _NEON_OPT_
    uint8x16_t v8x16_t, v8x16_b;
    uint8x8_t v8x8_t[2], v8x8_b[2];
    uint8x8_t coef8x8_dr, coef8x8_ur, coef8x8_dl, coef8x8_ul;
    uint16x8_t v16x8;
    uint8x8_t p8x8;
    uint32 p;
#endif

    dx1 = img_ptr->g_refPosx & 0x7;
    dy1 = img_ptr->g_refPosy & 0x7;
    dx2 = 8 - dx1;
    dy2 = 8 - dy1;

    downRight = dx1 * dy1;
    upRight = dx1 * dy2;
    downLeft = dx2 * dy1;
    upLeft = dx2 * dy2;

#ifdef _NEON_OPT_
    coef8x8_dr = vdup_n_u8(downRight);
    coef8x8_ur = vdup_n_u8(upRight);
    coef8x8_dl = vdup_n_u8(downLeft);
    coef8x8_ul = vdup_n_u8(upLeft);
#endif

    offset = (img_ptr->g_refPosy >> 3) * width_c + (img_ptr->g_refPosx >> 3);

    for (uv = 0; uv < 2; uv++)
    {
        pPred = pPredUV [uv];
        pRefUp = pRefFrame [1+uv] + offset;
        pRefDown = pRefUp + width_c;

#ifdef _NEON_OPT_
        v8x16_t = vld1q_u8(pRefUp);
        v8x16_b = vld1q_u8(pRefDown);
        pRefDown += width_c;
#endif

        for (i = N; i > 0; i--)	//y dir
        {
#ifndef _NEON_OPT_
            int32 j;
            int32 t0, t1, b0, b1;

            for (j = 0; j < 4; j++)	//x dir
            {
                t0 = pRefUp[j];
                t1 = pRefUp[j+1];
                b0 = pRefDown[j];
                b1 = pRefDown[j+1];
                pPred [j] = (upLeft * t0 + upRight * t1 + downLeft * b0 + downRight * b1 + 32) / 64;
            }

            //update for next line
            pRefUp += width_c;
#else
            v8x8_t[0] = vget_low_u8(v8x16_t);
            v8x8_t[1] = vext_u8(v8x8_t[0], vget_high_u8(v8x16_t), 1);

            v16x8 = vmull_u8(v8x8_t[0], coef8x8_ul);
            v16x8 = vmlal_u8(v16x8, v8x8_t[1], coef8x8_ur);

            v8x8_b[0] = vget_low_u8(v8x16_b);
            v8x8_b[1] = vext_u8(v8x8_b[0], vget_high_u8(v8x16_b), 1);

            v16x8 = vmlal_u8(v16x8, v8x8_b[0], coef8x8_dl);
            v16x8 = vmlal_u8(v16x8, v8x8_b[1], coef8x8_dr);

            p8x8 = vqrshrn_n_u16(v16x8, 6);
            p = (vget_lane_u8(p8x8, 3) << 8) | vget_lane_u8(p8x8, 2);
            p = (p<<8) |vget_lane_u8(p8x8, 1);
            p = (p<<8) |vget_lane_u8(p8x8, 0);
            ((uint32 *)pPred)[0] = p;

            //update for next line
            v8x16_t = v8x16_b;
            v8x16_b = vld1q_u8(pRefDown);
#endif
            pRefDown += width_c;
            pPred += MB_CHROMA_SIZE;
        }
    }
}
#endif

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End


