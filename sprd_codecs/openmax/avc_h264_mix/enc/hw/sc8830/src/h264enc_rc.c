/******************************************************************************
** File Name:      h264enc_rc.c	                                              *
** Author:         Shangwen li                                                *
** DATE:           11/16/2011                                                 *
** Copyright:      2011 Spreatrum, Incoporated. All Rights Reserved.          *
** Description:    rate control for video codec.	     			          *
*****************************************************************************/
/******************************************************************************
**                   Edit    History                                         *
**---------------------------------------------------------------------------*
** DATE          NAME            DESCRIPTION                                 *
** 11/16/2011    Shangwen Li     Create.                                     *
** 06/18/2013    Xiaowei Luo     Modify.                                     *
*****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "h264enc_video_header.h"
#include <math.h>
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif


#define MOD_GOP_RC
#define DIS_GOPS_RC
#define CALCAULTET_BITS			5
#define P_SLICE_BITS_SHIFT		10
#define ISLICE_QP_OFFSET		2

int32 g_Codec_MinQP[RC_CODEC_NUM] =
{
    1, 0, 0, 0, 0,
};

int32 g_Codec_MaxQP[RC_CODEC_NUM] =
{
    31, 40, 40, 114, 114,
};

int32 g_min_compress_rate = RC_MIN_CR;
int32 *g_RC_Qstep;

//========================================
//The rate control algorithm of GOP_RC
// 1. Frame level
// 2. GOP is the base process unit
// 3. provide CBR and VBR
// 4. provide Frame mode and slice mode
//========================================

//For H264 and HEVC
int32 g_MPEG_RC_Qstep[52] =
{
    640,  704,  832,  896, 1024, 1152, 1280, 1408, 1664, 1792, /* 0-9*/
    2048, 2304, 2560, 2816, 3328, 3584, 4096, 4608, 5120, 5632, /*10-19*/
    6656, 7168, 8192, 9216,10240,11264,13312,14336,16384,18432, /*20-29*/
    20480,22528,26624,28672,32768,36864,40960,45056,53248,57344, /*30-39*/
    65536,73728,81920,90112,106496,90112,90112,90112,90112,90112, /*40-49*/
    90112,90112,                                                     /*50-51*/
};

//For VP8
int32 g_VP8_QP_Cost_table[128] =
{
    4, 5, 6, 7, 8, 9, 10, 10, 11, 12, 13, 14, 15, 16, 17, 17,
    18, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 37, 38, 39, 40, 41, 42, 43,
    44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
    59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
    75, 76, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    91, 93, 95, 96, 98, 100, 101, 102, 104, 106, 108, 110, 112, 114, 116, 118,
    122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 143, 145, 148, 151, 154, 157,
};

//--------------------------------------------
//init and reset local function
//-------------------------------------------
int32 check_RC_parameters(RC_INOUT_PARAS *rc_inout_paras)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;

    if (0 == pRC->nFrame_Rate || 0 == pRC->nIntra_Period ||
            0 == pRC->nTarget_bitrate || 0 == pRC->nWidth ||
            0 == pRC->nHeight || 0 == pRC->nframe_size_in_mbs) {
        return RC_PARA_ZERO;
    }

    if (pRC->nCodec_type >= RC_CODEC_NUM) {
        return RC_NON_SUPPORT_CODEC;
    }

    if (pRC->nRate_control_en >= RC_MODE_NUM) {
        return RC_NON_SUPPORT_RC_MODE;
    }

    return RC_NO_ERROR;
}

void clip_RC_parameters(RC_INOUT_PARAS *rc_inout_paras)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    int32 nCodec = pRC->nCodec_type;
    int32 nMaxBitrate;

    pRC->nIP_Ratio = Clip3(RC_MIN_IPRATIO, RC_MAX_IPRATIO, pRC->nIP_Ratio);
    pRC->nIntra_Period = Clip3(RC_MIN_INTRA_PERIOD, RC_MAX_INTRA_PERIOD, pRC->nIntra_Period);
    pRC->nIQP = Clip3(g_Codec_MinQP[nCodec], g_Codec_MaxQP[nCodec], pRC->nIQP);
    pRC->nPQP = Clip3(g_Codec_MinQP[nCodec], g_Codec_MaxQP[nCodec], pRC->nPQP);
    pRC->nMaxQP = Clip3(g_Codec_MinQP[nCodec], g_Codec_MaxQP[nCodec], pRC->nMaxQP);
    pRC->nMinQP = Clip3(g_Codec_MinQP[nCodec], g_Codec_MaxQP[nCodec], pRC->nMinQP);

    nMaxBitrate = pRC->nWidth * pRC->nHeight * 12 * pRC->nFrame_Rate/g_min_compress_rate;
    pRC->nTarget_bitrate = Clip3(RC_MIN_BITRATE, nMaxBitrate, pRC->nTarget_bitrate);

    if (pRC->nRate_control_en >= RC_GOP_CBR) {
        pRC->nIP_Ratio = 1;
    }
}

static void init_gop_paras(RC_INOUT_PARAS *rc_inout_paras)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    RC_GOP_PARAS *pGOP = &pRC->rc_gop_paras;

    pGOP->curr_buf_full = 0;
    pGOP->nRem_bits = pRC->nBitrate;
    pGOP->nRem_GOPNum = pRC->nNumGOP + 1;
    pGOP->nRemNumFrames = pRC->nNumGOP * (pRC->nIntra_Period + pRC->nIP_Ratio - 1);
    pGOP->avg_GOP_bits = pGOP->nRem_bits/pRC->nNumGOP;
}

static void init_pic_paras(RC_PIC_PARAS *rc_pic_paras)
{
    rc_pic_paras->target_bits = 0;
    rc_pic_paras->encoded_bits = 0;
    rc_pic_paras->encoded_MBnum = 0;
    rc_pic_paras->sum_slice_qp = 0;
    rc_pic_paras->slice_num = 1;
}

void init_internal_RC_para(RC_INOUT_PARAS *rc_inout_paras)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    uint32 Sec_target_bitrate;

    pRC->nFrameCounter = 0;
    pRC->nSV = 0;
    pRC->nMaxOneFrameBits = pRC->nWidth * pRC->nHeight * 12/g_min_compress_rate;

    //init nFrames and nBitrate
    Sec_target_bitrate = (uint32)(pRC->nTarget_bitrate - (pRC->nTarget_bitrate >> 8) - (pRC->nTarget_bitrate >> 10));
#ifdef DIS_GOPS_RC
    {
        uint32 nTBNum;
        nTBNum = 1;
        while(Sec_target_bitrate > (uint32)(1<<nTBNum)) {
            nTBNum++;
        }
        if ((nTBNum + CALCAULTET_BITS) > 32)	{	//avoid bit-rate overflow in 32-bits
            pRC->nSV = (nTBNum + CALCAULTET_BITS) - 32;
        }

        pRC->nFrames = pRC->nIntra_Period;
        pRC->nBitrate = (Sec_target_bitrate >> pRC->nSV)/pRC->nFrame_Rate * pRC->nFrames;
    }
#else
    if(pRC->nFrame_Rate % pRC->nIntra_Period == 0) {
        uint32 nTBNum;
        nTBNum = 1;
        while(Sec_target_bitrate > (uint32)(1<<nTBNum)) {
            nTBNum++;
        }

        if ((nTBNum + CALCAULTET_BITS) > 32) {	//avoid bit-rate overflow in 32-bits
            pRC->nSV = (nTBNum + CALCAULTET_BITS) - 32;
        }
        pRC->nBitrate = (Sec_target_bitrate >> pRC->nSV);
        pRC->nFrames = pRC->nFrame_Rate;
    } else {
        uint32 nTBNum, nFRNum, nRCFNum;
        if (pRC->nFrame_Rate > pRC->nIntra_Period) {
            pRC->nFrames = (pRC->nFrame_Rate + pRC->nIntra_Period - 1)/pRC->nIntra_Period;
            pRC->nFrames = pRC->nFrames * pRC->nIntra_Period;
        } else {
            pRC->nFrames = pRC->nIntra_Period;
        }

        //avoid bit-rate overflow in 32 bits
        nTBNum = 1;
        while(Sec_target_bitrate > (uint32)(1<<nTBNum)) {
            nTBNum++;
        }
        nFRNum = 1;
        while(pRC->nFrame_Rate > (uint32)(1<<nFRNum)) {
            nFRNum++;
        }
        nRCFNum = 1;
        while(pRC->nFrames > (uint32)(1<<nRCFNum)) {
            nRCFNum++;
        }
        if ((nTBNum + nRCFNum - nFRNum + CALCAULTET_BITS) > 32) {
            pRC->nSV = (nTBNum + nRCFNum - nFRNum + CALCAULTET_BITS) - 32;
            Sec_target_bitrate = (Sec_target_bitrate >> pRC->nSV);
        }
        pRC->nBitrate = Sec_target_bitrate/pRC->nFrame_Rate * pRC->nFrames;
    }
#endif
    pRC->nNumGOP = Clip3(1, pRC->nFrames, pRC->nFrames/pRC->nIntra_Period);

    //set Qstep
    switch(pRC->nCodec_type) {
    case RC_VP8:
        g_RC_Qstep = g_MPEG_RC_Qstep;
        break;
    case RC_H264:
    case RC_HEVC:
    default:
        g_RC_Qstep = g_MPEG_RC_Qstep;
        break;
    }
}

//---------------------------------------
//getQP local function
//--------------------------------------
void setGOP_PIC_para(RC_INOUT_PARAS *rc_inout_paras, int32 nSlice_mb_index)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    RC_GOP_PARAS *pGOP = &pRC->rc_gop_paras;
    RC_PIC_PARAS *pPIC = &pRC->rc_pic_paras;

    if (nSlice_mb_index == 0 || !pRC->nSlice_enable) {
        //update rc_gop_paras->rem_bits------------------
        if (0 == (pRC->nFrameCounter % pRC->nIntra_Period)) {
            pGOP->nRem_GOPNum--;
        }
        if ((pRC->nFrameCounter % pRC->nFrames) == (pRC->nFrames >> 1)) {
            pGOP->nRem_bits += pRC->nBitrate;
            pGOP->nRem_GOPNum += pRC->nNumGOP;
            pGOP->nRemNumFrames += pRC->nNumGOP * (pRC->nIntra_Period + pRC->nIP_Ratio - 1);
            pGOP->avg_GOP_bits = pGOP->nRem_bits / pGOP->nRem_GOPNum;
        }

        if (0 == (pRC->nFrameCounter % pRC->nIntra_Period)) {
            if (pRC->nRate_control_en == RC_GOP_CBR) {
                if((pGOP->nRem_bits/pGOP->nRem_GOPNum) > pRC->nBitrate) {
                    pGOP->nRem_bits = pRC->nBitrate*pGOP->nRem_GOPNum;
                }
                pGOP->CBR_P_bits = pGOP->nRem_bits/pGOP->nRemNumFrames;
                pGOP->curr_buf_full = (pGOP->curr_buf_full > 0) ? pGOP->curr_buf_full : 0;
            }
        }
    }
}

static int32 get_CBR_IsliceQP(RC_INOUT_PARAS *rc_inout_paras, int32 nSlice_mb_index, int32 last_slice_bits)
{
    int32 i;
    int32 GOP_init_QP;
    int32 nTargetBits;
    int32 nCalculated_IBits;

    int32 nLast_slice_IQP;
    int32 nLast_slice_Bits;

    RC_INOUT_PARAS *pRC = rc_inout_paras;
    RC_GOP_PARAS *pGOP = &pRC->rc_gop_paras;
    RC_PIC_PARAS *pPIC = &pRC->rc_pic_paras;

    // 1. get target bit_rate
    if (0 == nSlice_mb_index || !pRC->nSlice_enable) {
        if (pGOP->avg_GOP_bits * pGOP->nRem_GOPNum < pGOP->nRem_bits) {
            pGOP->nRem_bits = pGOP->avg_GOP_bits * pGOP->nRem_GOPNum;
        }
        nTargetBits = pGOP->nRem_bits * pRC->nIP_Ratio/pGOP->nRemNumFrames;
        if (nTargetBits > pRC->nMaxOneFrameBits * pRC->nIntra_Period) {
            nTargetBits = pRC->nMaxOneFrameBits * pRC->nIntra_Period;
        }

        if (0 == pRC->nFrameCounter) {
            pPIC->target_bits = nTargetBits;
            pPIC->sum_slice_qp += pRC->nIQP;
            pPIC->nLast_slice_QP = pRC->nIQP;
            return pRC->nIQP;
        }

        nLast_slice_IQP = pGOP->last_IQP;
        nLast_slice_Bits = pGOP->last_IframeBits;
    } else {
        //multiple slice
        if (pRC->nframe_size_in_mbs != pPIC->encoded_MBnum) {
            nTargetBits = (pPIC->target_bits - pPIC->encoded_bits)
                          / (pRC->nframe_size_in_mbs - pPIC->encoded_MBnum)
                          * (nSlice_mb_index - pPIC->encoded_MBnum);
        } else {
            nTargetBits = (pPIC->target_bits - pPIC->encoded_bits);
        }
        pPIC->encoded_bits += last_slice_bits;
        pPIC->encoded_MBnum = nSlice_mb_index;
        pPIC->slice_num++;

        nLast_slice_IQP = pPIC->nLast_slice_QP;
        nLast_slice_Bits = last_slice_bits;
    }

    // 2. get QP
    for (i = -6; i <= 6; i++) {
        GOP_init_QP = nLast_slice_IQP + i;
        GOP_init_QP = Clip3(pRC->nMinQP, pRC->nMaxQP, GOP_init_QP);

        nCalculated_IBits = (nLast_slice_Bits << CALCAULTET_BITS)/g_RC_Qstep[GOP_init_QP] * g_RC_Qstep[nLast_slice_IQP];
        nCalculated_IBits >>= CALCAULTET_BITS;

        if (GOP_init_QP >= pRC->nMaxQP) {
            GOP_init_QP = pRC->nMaxQP;
            break;
        } else if (nCalculated_IBits < nTargetBits) {
            GOP_init_QP = GOP_init_QP - 1;
            break;
        }
    }

    // 3. Update IP_ratio
    pPIC->sum_slice_qp += GOP_init_QP;
    pPIC->nLast_slice_QP = GOP_init_QP;

    return GOP_init_QP;
}

static int32 get_VBR_IsliceQP(RC_INOUT_PARAS *rc_inout_paras, int32 nSlice_mb_index, int last_slice_bits)
{
    int32 i;
    int32 GOP_init_QP;
    int32 nOffset_QP;
    int32 nTargetBits;
    int32 nAvg_prev_QP;
    int32 nCalculated_IBits;
    int32 nCalculated_PBits;

    RC_INOUT_PARAS *pRC = rc_inout_paras;
    RC_GOP_PARAS *pGOP = &pRC->rc_gop_paras;
    RC_PIC_PARAS *pPIC = &pRC->rc_pic_paras;

    int32 nGOPPframeNum = pRC->nIntra_Period - 1;

    if (0 == pRC->nFrameCounter) {
        nTargetBits = pGOP->nRem_bits/pGOP->nRem_GOPNum;
        if (nTargetBits > pRC->nMaxOneFrameBits * pRC->nIntra_Period) {
            nTargetBits = pRC->nMaxOneFrameBits * pRC->nIntra_Period;
        }

        if (0 != pGOP->nRemNumFrames) {
            pPIC->target_bits = nTargetBits * pRC->nIP_Ratio/pGOP->nRemNumFrames;
        }
        if (pPIC->target_bits > pRC->nMaxOneFrameBits) {
            pPIC->target_bits = pRC->nMaxOneFrameBits;
        }
        pPIC->sum_slice_qp += pRC->nIQP;
        pPIC->nLast_slice_QP = pRC->nIQP;
        return pRC->nIQP;
    }

    // 1. get target bit_rate
    nTargetBits = pGOP->nRem_bits/pGOP->nRem_GOPNum;
    if (nTargetBits > pRC->nMaxOneFrameBits * pRC->nIntra_Period) {
        nTargetBits = pRC->nMaxOneFrameBits * pRC->nIntra_Period;
    }

    // 2. get QP
    if (0 != pGOP->last_PframeNum) {
        pGOP->last_PframeBits = pGOP->last_PframeBits/pGOP->last_PframeNum;
        nAvg_prev_QP = pGOP->last_PQP/pGOP->last_PframeNum;

        pGOP->last_IQP = Clip3(pRC->nMinQP, pRC->nMaxQP, pGOP->last_IQP);
        nAvg_prev_QP = Clip3(pRC->nMinQP, pRC->nMaxQP, nAvg_prev_QP);
    } else {
        pGOP->last_PframeBits = pGOP->last_PframeBits;
        nAvg_prev_QP = pGOP->last_PQP;

        pGOP->last_IQP = Clip3(pRC->nMinQP, pRC->nMaxQP, pGOP->last_IQP);
        nAvg_prev_QP = Clip3(pRC->nMinQP, pRC->nMaxQP, nAvg_prev_QP);
    }

    for (i = -6; i <= 6; i++) {
        GOP_init_QP = nAvg_prev_QP + i;
        GOP_init_QP = Clip3(pRC->nMinQP, pRC->nMaxQP, GOP_init_QP);
        nOffset_QP = GOP_init_QP + ISLICE_QP_OFFSET;
        nOffset_QP = Clip3(pRC->nMinQP, pRC->nMaxQP, nOffset_QP);

        nCalculated_IBits = (pGOP->last_IframeBits << CALCAULTET_BITS)/g_RC_Qstep[GOP_init_QP]
                            *g_RC_Qstep[pGOP->last_IQP];
        nCalculated_IBits >>= CALCAULTET_BITS;
        nCalculated_PBits = (pGOP->last_PframeBits << CALCAULTET_BITS)/g_RC_Qstep[nOffset_QP]
                            *g_RC_Qstep[nAvg_prev_QP] * nGOPPframeNum;
        nCalculated_PBits >>= CALCAULTET_BITS;

        if (GOP_init_QP >= pRC->nMaxQP) {
            GOP_init_QP = pRC->nMaxQP;
            break;
        } else if((nCalculated_IBits + nCalculated_PBits) < nTargetBits) {
            break;
        }
    }

    // 3. Update PIC parameters
    if (0 != pGOP->nRemNumFrames) {
        pPIC->target_bits = nTargetBits * pRC->nIP_Ratio / pGOP->nRemNumFrames;
    }
    if (pPIC->target_bits > pRC->nMaxOneFrameBits) {
        pPIC->target_bits = pRC->nMaxOneFrameBits;
    }
    pPIC->sum_slice_qp += GOP_init_QP;
    pPIC->nLast_slice_QP = GOP_init_QP;

    // 4. Update IP_ratio
    if (pRC->nFrameCounter % pRC->nFrames == 0) {
        nCalculated_PBits = pGOP->last_PframeBits / g_RC_Qstep[pGOP->last_IQP] * g_RC_Qstep[nAvg_prev_QP];
        if (0 == nCalculated_PBits) {
            nCalculated_PBits = pGOP->last_PframeBits * g_RC_Qstep[nAvg_prev_QP]/g_RC_Qstep[pGOP->last_IQP];
            if (0 == nCalculated_PBits) {
                nCalculated_PBits = 1;
            }
        }

        pRC->nIP_Ratio = (pGOP->last_IframeBits + (nCalculated_PBits >> 1))/nCalculated_PBits;
        pRC->nIP_Ratio = Clip3(RC_MIN_IPRATIO, RC_MAX_IPRATIO, pRC->nIP_Ratio);
    }
    pGOP->nRemNumFrames = pRC->nNumGOP * (pRC->nIntra_Period + pRC->nIP_Ratio - 1);

    return GOP_init_QP;
}

static int32 getPsliceQP(RC_INOUT_PARAS *rc_inout_paras, int32 nSlice_mb_index, int32 last_slice_bits)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    RC_GOP_PARAS *pGOP = &pRC->rc_gop_paras;
    RC_PIC_PARAS *pPIC = &pRC->rc_pic_paras;

    int32 curr_qp;
    int32 nTargetBits;
    int32 ratio;
    int32 overdue = 0;
    int32 prev_frame_qp;

    if (nSlice_mb_index == 0 || !pRC->nSlice_enable) {
        if (1 == pRC->nFrameCounter) {
            int32 nPIC_Delta_target_buf;
            nPIC_Delta_target_buf = pGOP->curr_buf_full/pGOP->nRemNumFrames;
            nTargetBits = pGOP->nRem_bits/pGOP->nRemNumFrames;
            pPIC->target_bits = nTargetBits - nPIC_Delta_target_buf;
            pPIC->sum_slice_qp += pRC->nPQP;
            pPIC->nLast_slice_QP = pRC->nPQP;
            return pRC->nPQP;
        }

        // 1. get previous frame/slice QP
        if (pGOP->last_frame_type == RC_I_SLICE) {
            prev_frame_qp = pGOP->last_PQP/pGOP->last_PframeNum;
        } else {
            prev_frame_qp = pGOP->last_QP;
        }

        // 2. get target bits
        if (pGOP->nRemNumFrames != 0) {
            int32 nPIC_Delta_target_buf;
            nPIC_Delta_target_buf = pGOP->curr_buf_full/pGOP->nRemNumFrames;
            nTargetBits = pGOP->nRem_bits/pGOP->nRemNumFrames;
            nTargetBits = nTargetBits - nPIC_Delta_target_buf;
            if (pRC->nRate_control_en == RC_GOP_CBR) {
                if (!pGOP->CBR_P_bits && nTargetBits > pGOP->CBR_P_bits) {
                    nTargetBits = pGOP->CBR_P_bits;
                }
            }
        } else {
            nTargetBits = pGOP->nRem_bits;
        }

        if (nTargetBits > pRC->nMaxOneFrameBits) {
            nTargetBits = pRC->nMaxOneFrameBits;
        }

        // 3. get ratio
        if(pRC->nFrameCounter == 1) {
            if (nTargetBits > 0) {
                ratio = ((uint32)(pGOP->last_frameBits << 10) /nTargetBits);
                ratio = ratio/pRC->nIP_Ratio;
            }
        } else {
            if (nTargetBits > 0) {
                if (pGOP->last_frame_type == RC_I_SLICE) {
                    ratio = ((uint32)((pGOP->last_PframeBits / pGOP->last_PframeNum) << P_SLICE_BITS_SHIFT) / nTargetBits);
                } else {
                    ratio = ((uint32)(pGOP->last_frameBits  << P_SLICE_BITS_SHIFT) / nTargetBits);
                }
            }
        }
    } else {
        //multi-slice mode
        // 1. get previous frame/slice QP
        prev_frame_qp = pPIC->nLast_slice_QP;

        // 2. get target bits
        if (pRC->nframe_size_in_mbs != pPIC->encoded_MBnum) {
            nTargetBits = (pPIC->target_bits - pPIC->encoded_bits)
                          / (pRC->nframe_size_in_mbs - pPIC->encoded_MBnum)
                          * (nSlice_mb_index - pPIC->encoded_MBnum);
        } else {
            nTargetBits = (pPIC->target_bits - pPIC->encoded_bits);
        }
        pPIC->encoded_bits += last_slice_bits;
        pPIC->encoded_MBnum = nSlice_mb_index;
        pPIC->slice_num++;

        // 3. get ratio
        if (nTargetBits) {
            ratio = (last_slice_bits << P_SLICE_BITS_SHIFT)/nTargetBits;
        }
    }

    // 4. get QP base on ratio and prev_frame_qp
    if (pRC->nCodec_type == RC_VP8) {
    } else {	//H.264, HEVC
        if (nTargetBits <= 0) { //overdue
            curr_qp = Clip3(pRC->nMinQP, pRC->nMaxQP, prev_frame_qp+2);
        } else if (ratio >= 1126) {
            curr_qp = Clip3(pRC->nMinQP, pRC->nMaxQP, prev_frame_qp+2);
        } else if (ratio < 1126 && ratio >= 1065) {
            curr_qp = Clip3(pRC->nMinQP, pRC->nMaxQP, prev_frame_qp+1);
        } else if (ratio < 1065 && ratio >= 819) {
            curr_qp = Clip3(pRC->nMinQP, pRC->nMaxQP, prev_frame_qp);
        } else if (ratio < 819 && ratio >= 683) {
            curr_qp = Clip3(pRC->nMinQP, pRC->nMaxQP, prev_frame_qp-1);
        } else {
            curr_qp = Clip3(pRC->nMinQP, pRC->nMaxQP, prev_frame_qp-2);
        }
    }

    // 5. Update PIC parameters
    pPIC->target_bits = nTargetBits;
    pPIC->sum_slice_qp += curr_qp;
    pPIC->nLast_slice_QP = curr_qp;

    return curr_qp;
}

//============================================
// global function
//============================================
int32 init_GOPRC(RC_INOUT_PARAS *rc_inout_paras)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    int32 nRC_flag;

    if (!pRC->nRate_control_en) {
        return RC_NO_ERROR;
    }

    //Check RC parameters--------------
    nRC_flag = check_RC_parameters(pRC);
    if (nRC_flag) {
        //Disable RC and return error message
        pRC->nRate_control_en = 0;
        return nRC_flag;
    }

    //Clip RC parameters-----------------
    clip_RC_parameters(pRC);

    //init internal RC parameters-------------
    init_internal_RC_para(pRC);
    init_gop_paras(pRC);
    init_pic_paras(&pRC->rc_pic_paras);

    return 0;
}

int32 reset_GOPRC(RC_INOUT_PARAS *rc_inout_paras)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    int32 nRC_flag;
    int32 curr_buf_full;

    //Check RC parameters----------------
    if (!pRC->nRate_control_en) {
        return RC_NO_ERROR;
    }
    nRC_flag = check_RC_parameters(pRC);
    if (nRC_flag || !pRC->nRate_control_en) {
        //Disable RC and return error message
        pRC->nRate_control_en = 0;
        return nRC_flag;
    }

    //Clip RC parameters------------------
    clip_RC_parameters(pRC);

    //init internal RC parameters-----------------
    init_internal_RC_para(pRC);
    pRC->nFrameCounter = pRC->nFrames;

    //keep last RC parameters----------------
    curr_buf_full = pRC->rc_gop_paras.curr_buf_full;
    init_gop_paras(pRC);
    init_pic_paras(&pRC->rc_pic_paras);
    pRC->rc_gop_paras.curr_buf_full = curr_buf_full;

    return 0;
}

int32 getQP_GOPRC(RC_INOUT_PARAS *rc_inout_paras, int32 nSliceType, int32 nSlice_mb_index, int32 last_slice_bits)
{
    int32 nQP = rc_inout_paras->nMaxQP;

    //check RC parameters-------------------
    if(!rc_inout_paras->nRate_control_en) {
        if (RC_I_SLICE == nSliceType) {
            return rc_inout_paras->nIQP;
        } else {
            return rc_inout_paras->nPQP;
        }
    }

    //set GOP and PIC parameters---------------
    setGOP_PIC_para(rc_inout_paras, nSlice_mb_index);

    //getQP---------------------------
    if (nSliceType == RC_I_SLICE) {
        if (rc_inout_paras->nFrameCounter % rc_inout_paras->nIntra_Period == 0
                && (nSlice_mb_index == 0 || !rc_inout_paras->nSlice_enable)
                && rc_inout_paras->nRate_control_en == RC_GOP_VBR
                && rc_inout_paras->nIntra_Period != 1) {
            nQP = get_VBR_IsliceQP(rc_inout_paras, nSlice_mb_index, last_slice_bits);
        } else {
            nQP = get_CBR_IsliceQP(rc_inout_paras, nSlice_mb_index, last_slice_bits);
        }
    } else {
        nQP = getPsliceQP(rc_inout_paras, nSlice_mb_index, last_slice_bits);
    }

    rc_inout_paras->rc_gop_paras.last_frame_type = nSliceType;

    return nQP;
}

void updatePicPara_GOPRC(RC_INOUT_PARAS *rc_inout_paras, int32 bits)
{
    RC_INOUT_PARAS *pRC = rc_inout_paras;
    RC_GOP_PARAS *pGOP = &pRC->rc_gop_paras;
    RC_PIC_PARAS *pPIC = &pRC->rc_pic_paras;

    if (pRC->nRate_control_en) {
        int delta_bits;
        int BitsS;

        //=====================================
        // 1. update buffer fullness and remain bits
        BitsS = (bits >> pRC->nSV);
        if (0 != pRC->nFrames) {
            delta_bits = BitsS - (int)(pRC->nBitrate / pRC->nFrames);
        } else {
            delta_bits = BitsS - (int)(pRC->nBitrate);
        }
        pGOP->nRem_bits -= BitsS;
        pGOP->curr_buf_full += delta_bits;
        pGOP->last_frameBits = BitsS;

        //======================================
        // 2. get last QP
        if (0 != pPIC->slice_num) {
            pGOP->last_QP = pPIC->sum_slice_qp / pPIC->slice_num;
        } else {
            pGOP->last_QP = pPIC->nLast_slice_QP;
        }

        //=====================================
        // for next GOP and frame calculation
        if (pGOP->last_frame_type == RC_P_SLICE) {
            pGOP->last_PframeBits = BitsS;
            pGOP->last_PQP = pGOP->last_QP;
            pGOP->last_PframeNum = 1;
        } else {
            pGOP->last_IframeBits = BitsS;
            pGOP->last_IQP = pGOP->last_QP;
            pGOP->last_IframeNum = 1;
        }

        if(pRC->nFrameCounter % pRC->nIntra_Period == 0) {
            pGOP->nRemNumFrames -= pRC->nIP_Ratio;
        } else {
            pGOP->nRemNumFrames -= 1;
        }

        //=======================================
        //reset parameters for Slice mode
        pPIC->encoded_bits = 0;
        pPIC->encoded_MBnum = 0;
        pPIC->slice_num = 1;
        pPIC->sum_slice_qp = 0;

        pRC->nFrameCounter++;
    }
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
// End

