/******************************************************************************
 ** File Name:    mp4dec_block.c                                           *
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

#define CHECHRANGE(x, cMin, cMax) ((x) < (cMin)) ? (cMin) : (((x) > (cMax)) ? (cMax) : (x))

/*****************************************************************************
 **	Name : 			Mp4_DivRoundnearest_sw
 ** Description:	DivRoundnearest function.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
uint16 inv_scaler[49] =
{
    0x0000,0x0000,0x0000,0xaaab,0x8001,0x6667,0x5556,0x4925,
    0x4001,0x38e4,0x3334,0x2e8c,0x2aab,0x2763,0x2493,0x2223,
    0x2001,0x1e1f,0x1c72,0x1af3,0x199a,0x1862,0x1746,0x1643,
    0x1556,0x147b,0x13b2,0x12f7,0x124a,0x11a8,0x1112,0x1085,
    0x1001,0x0f84,0x0f10,0x0ea1,0x0e39,0x0dd7,0x0d7a,0x0d21,
    0x0ccd,0x0c7d,0x0c31,0x0be9,0x0ba3,0x0b61,0x0b22,0x0ae5,
    0x0aab,
};

#define SHIFT 17
/*inline*/ int32 Mp4_DivRoundnearest_sw(int32 i, int32 iDenom)
{
#if 0
    if(i >= 0)
    {
        //return (i+(iDenom>>1))/iDenom;
        return ((i+(iDenom>>1))*inv_scaler[iDenom])>>SHIFT;
    }
    else
    {
        //return (i-(iDenom>>1))/iDenom;
        return ((i-(iDenom>>1))*inv_scaler[iDenom])>>SHIFT;
    }
#else
    int32 abs_i = i > 0 ? i : -i;
    int32 val = ((abs_i+(iDenom>>1))*inv_scaler[iDenom])>>SHIFT;

    return ((i > 0) ? val : (-val));
#endif
}

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_GetLeftBlkPred
 ** Description:	get left block prediction.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL int16 *Mp4Dec_GetLeftBlkPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, int8 *QpLeft, int32 *predAvail)
{
    int16 *pLeftPred = NULL;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    int32 leftAvail = mb_cache_ptr->bLeftMBAvail;

    *predAvail = FALSE;

    if(blk_num < 4)
    {
        pLeftPred = vop_mode_ptr->pLeftCoeff  + ((blk_num >> 1) <<3);
        if((blk_num & 1) == 0)
        {
            if(leftAvail)
            {
                *predAvail = TRUE;
                *QpLeft = mb_cache_ptr->leftMBQP;
            }
        } else
        {
            *predAvail = TRUE;
            *QpLeft = mb_mode_ptr->StepSize;
        }
    } else
    {
        if(leftAvail)
        {
            *predAvail = TRUE;
            *QpLeft = mb_cache_ptr->leftMBQP;
        }
        pLeftPred = vop_mode_ptr->pLeftCoeff + ( (blk_num - 2) <<3);
    }

    return pLeftPred;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetTopBlkPred
 ** Description:	get top block prediction.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL int16 *Mp4Dec_GetTopBlkPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, int8 *QpTop, int32 *predAvail)
{
    int32 mb_x = vop_mode_ptr->mb_x;
    int16 *pTopPred = NULL;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    int32 topAvail = mb_cache_ptr->bTopMBAvail;

    *predAvail = FALSE;

    if(blk_num < 4)
    {
        pTopPred = vop_mode_ptr->pTopCoeff + (mb_x <<5) + ((blk_num & 1) <<3);
        if(blk_num < 2)
        {
            if(topAvail)
            {
                *predAvail = TRUE;
                *QpTop = mb_cache_ptr->topMBQP;
            }
        } else
        {
            *predAvail = TRUE;
            *QpTop = mb_mode_ptr->StepSize;
        }
    } else
    {
        if(topAvail)
        {
            *predAvail = TRUE;
            *QpTop = mb_cache_ptr->topMBQP;
        }
        pTopPred = vop_mode_ptr->pTopCoeff + (mb_x << 5) +( (blk_num- 2) <<3);
    }

    return pTopPred;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetLeftTopBlkPred
 ** Description:	get left top block prediction.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL /*inline*/ int32 Mp4Dec_GetLeftTopBlkPred(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num)
{
    int32 pred;
    static int8 s_blkIdx[6] = {0, 1, 4, 5, 3, 7};

    pred = vop_mode_ptr->mb_cache_ptr->pDCCache[s_blkIdx[blk_num]];

    return pred;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetDCACPredDir
 ** Description:	Get blk_num of current MB's DC AC prediction, prediction direction,
					QP of prediction Block.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_GetDCACPredDir(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num)
{
    int16 *predLeft, *predTop;
    int32 leftPredAvail, topPredAvail;
    int8 QpLeft = 0, QpTop = 0;
    int32 iPredLeftTop;
    int32 iHorizontalGrad;
    int32 iVerticalGrad;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

    predLeft = Mp4Dec_GetLeftBlkPred(vop_mode_ptr, mb_mode_ptr, blk_num, &QpLeft, &leftPredAvail);
    predTop  = Mp4Dec_GetTopBlkPred(vop_mode_ptr, mb_mode_ptr, blk_num, &QpTop, &topPredAvail);
    iPredLeftTop = Mp4Dec_GetLeftTopBlkPred(vop_mode_ptr, blk_num);

    iHorizontalGrad = ((topPredAvail) ? predTop[0] : DEFAULT_DC_VALUE) - iPredLeftTop;
    iVerticalGrad   = ((leftPredAvail) ? predLeft[0] : DEFAULT_DC_VALUE) - iPredLeftTop;

    mb_cache_ptr->BlockPred = NULL;
    mb_cache_ptr->leftPred = predLeft;
    mb_cache_ptr->topPred = predTop;
    if(ABS(iHorizontalGrad) > ABS(iVerticalGrad))
    {
        mb_cache_ptr->preddir   = HORIZONTAL;	//for trans, normal is VERTICAL
        mb_cache_ptr->BlockPred = predTop;
        mb_cache_ptr->QpPred    = (int8)QpTop;
        mb_cache_ptr->predAvail = (int8)topPredAvail;
    } else
    {
        mb_cache_ptr->preddir   = VERTICAL;		//for trans, normal is HORIZONTAL
        mb_cache_ptr->BlockPred = predLeft;
        mb_cache_ptr->QpPred    = QpLeft;
        mb_cache_ptr->predAvail = leftPredAvail;
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetInverseDCACPred
 ** Description:	Get inverse dc ac predition.
 ** Author:			Xiaowei Luo
 **	Note:
					1. inverse DC AC prediction
					2. store first col and first row for later prediction
					3. inverse DC IQUANT;
 *****************************************************************************/
LOCAL void Mp4Dec_GetInverseDCACPred(DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, int32 iBlkIdx, int16 *rgiCoefQ,	int32 iDcScaler)
{
    int16 *pLeftPred, *pTopPred;

    if(mb_cache_ptr->predAvail)
    {
        int16 *blkmPred = mb_cache_ptr->BlockPred;
        int32 i;

        if(mb_mode_ptr->bACPrediction)
        {
            int32 iQP = mb_mode_ptr->StepSize;
            int32 iQpPred = mb_cache_ptr->QpPred;

            if(HORIZONTAL == mb_cache_ptr->preddir)
            {
                if(iQP == iQpPred)
                {
                    rgiCoefQ[8] += blkmPred[1];
                    rgiCoefQ[16] += blkmPred[2];
                    rgiCoefQ[24] += blkmPred[3];
                    rgiCoefQ[32] += blkmPred[4];
                    rgiCoefQ[40] += blkmPred[5];
                    rgiCoefQ[48] += blkmPred[6];
                    rgiCoefQ[56] += blkmPred[7];
                } else
                {
                    for (i = 1; i < 8; i++)
                    {
                        int32 val = blkmPred[i] * iQpPred;

                        if (val > 0)
                        {
                            rgiCoefQ[i<<3] += ((((iQP>>1) + val) * inv_scaler[iQP]) >> SHIFT);
                        } else
                        {
                            rgiCoefQ[i<<3] -= ((((iQP>>1) - val) * inv_scaler[iQP]) >> SHIFT);
                        }
                    }
                }
            } else
            {
                if(iQP == iQpPred)
                {
                    rgiCoefQ[1] += blkmPred[1];
                    rgiCoefQ[2] += blkmPred[2];
                    rgiCoefQ[3] += blkmPred[3];
                    rgiCoefQ[4] += blkmPred[4];
                    rgiCoefQ[5] += blkmPred[5];
                    rgiCoefQ[6] += blkmPred[6];
                    rgiCoefQ[7] += blkmPred[7];
                } else
                {
                    for (i = 1; i < 8; i++)
                    {
                        int32 val = blkmPred[i] * iQpPred;

                        if (val > 0)
                        {
                            rgiCoefQ[i] += ((((iQP>>1) + val) * inv_scaler[iQP]) >> SHIFT);
                        } else
                        {
                            rgiCoefQ[i] -= ((((iQP>>1) - val) * inv_scaler[iQP]) >> SHIFT);
                        }
                    }
                }
            }
        }

        rgiCoefQ[0] += (int16)Mp4_DivRoundnearest_sw(blkmPred[0], iDcScaler);
    } else
    {
        rgiCoefQ[0] += ((((iDcScaler>>1) + DEFAULT_DC_VALUE) * inv_scaler[iDcScaler]) >> SHIFT);
    }

    rgiCoefQ[0] = (int16)(rgiCoefQ[0] * iDcScaler);	//inverse quantization of DC coefficient
    if(iBlkIdx < 2)
    {
        mb_cache_ptr->pDCCache[5 + iBlkIdx] = rgiCoefQ[0];
    }

    /*store first row and first column for later prediction*/
    pTopPred = mb_cache_ptr->leftPred;	//for trans, normal is topPred;
    ((int32 *)pTopPred)[0] = ((int32 *)rgiCoefQ)[0];
    ((int32 *)pTopPred)[1] = ((int32 *)rgiCoefQ)[1];
    ((int32 *)pTopPred)[2] = ((int32 *)rgiCoefQ)[2];
    ((int32 *)pTopPred)[3] = ((int32 *)rgiCoefQ)[3];

    pLeftPred = mb_cache_ptr->topPred;	//for trans, normal is leftPred;
    pLeftPred[0] = rgiCoefQ[0];
    pLeftPred[1] = rgiCoefQ[8];
    pLeftPred[2] = rgiCoefQ[16];
    pLeftPred[3] = rgiCoefQ[24];
    pLeftPred[4] = rgiCoefQ[32];
    pLeftPred[5] = rgiCoefQ[40];
    pLeftPred[6] = rgiCoefQ[48];
    pLeftPred[7] = rgiCoefQ[56];
}

#define DQUANT_H263MODE(idx)	\
	if (coeff)	\
{\
	if (coeff > 0)	piDCTCoeff[idx] = coeff * qmul + qadd;	\
	else	piDCTCoeff[idx] = coeff * qmul - qadd;	\
}

void Mp4Dec_H263IqIntraBlock(DEC_MB_MODE_T *mb_mode_ptr , DEC_MB_BFR_T *pMBCache,int16 * piDCTCoeff, int8 *nonCoeffPos, int32 nonCoeffNum)
{
    int32 i;
    int32 coeff;
    int32 index;
    int32 iQP = mb_mode_ptr->StepSize;
    int32 qadd = (iQP -1) | 0x1;
    int32 qmul = iQP << 1;

    if(!(mb_mode_ptr->bACPrediction && pMBCache->predAvail))
    {
        int32 start_pos = 0;

        if (0 == nonCoeffPos[0])
        {
            start_pos = 1;
        }

        for(i = start_pos; i < nonCoeffNum; i++)
        {
            index = nonCoeffPos[i];
            coeff = piDCTCoeff[index];
            DQUANT_H263MODE (index);
        }
    } else
    {
        if(pMBCache->preddir == HORIZONTAL)
        {
            /*IQ for first column*/
            for(i = 1; i < 8; i++)
            {
                index = i<<3;
                coeff = piDCTCoeff[index];
                DQUANT_H263MODE (index);
            }

            /*IQ for rest coefficient*/
            for(i = 0; i < nonCoeffNum; i++)
            {
                index = nonCoeffPos [i];
                if((index & 0x7) != 0)
                {
                    coeff = piDCTCoeff [index];
                    DQUANT_H263MODE (index);
                }
            }
        } else //IQ for first row
        {
            /*IQ for first row*/
            for(i = 1; i < 8; i++)
            {
                index = i;
                coeff = piDCTCoeff[index];
                DQUANT_H263MODE (index);
            }

            /*IQ for rest coefficient*/
            for(i = 0; i < nonCoeffNum; i++)
            {
                index = nonCoeffPos[i];
                if(index > 7)
                {
                    coeff = piDCTCoeff[index];
                    DQUANT_H263MODE (index);
                }
            }
        }
    }
}

void Mp4Dec_MpegIqIntraBlock(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr ,DEC_MB_BFR_T *pMBCache, int16 *piDCTCoeff, int8 *nonCoeffPos, int32 nonCoeffNum)
{
    int32 i;
    int32 coeff;
    int32 index;
    int32 iSum;
    BOOLEAN bCoefQAllZero;
    int32 iQP = mb_mode_ptr->StepSize;
    int32 iMaxVal = 2048;//vop_mode_ptr->iMaxVal; // NBIT ??leon
    uint8 *piQuantizerMatrix = vop_mode_ptr->IntraQuantizerMatrix;

    bCoefQAllZero = (piDCTCoeff[0] == 0) ? TRUE : FALSE;
    iSum = piDCTCoeff[0];

    if(!(mb_mode_ptr->bACPrediction && pMBCache->predAvail))
    {
        int32 start_pos = 0;

        if (0 == nonCoeffPos[0])
        {
            start_pos = 1;
        }

        for(i = start_pos; i < nonCoeffNum; i++)
        {
            index = nonCoeffPos[i];
            coeff = piDCTCoeff[index];

            if (coeff < 0)
            {
                coeff = -coeff;
                coeff = (iQP * coeff * piQuantizerMatrix[index] )>> 3;
                coeff = -coeff;
            } else
            {
                coeff = (iQP * coeff * piQuantizerMatrix[index] )>> 3;
            }

            iSum ^= coeff;
            piDCTCoeff[index] = (int16)coeff;
            bCoefQAllZero = FALSE;
        }
    } else
    {
        /*inverse quantization for all the coefficient*/
        for(i = 1; i < 64; i++)
        {
            for(i = 1; i < BLOCK_SQUARE_SIZE; i++)
            {
                coeff = piDCTCoeff[i];
                if(coeff != 0)
                {
                    if (coeff < 0)
                    {
                        coeff = -coeff;
                        coeff = (iQP * coeff * piQuantizerMatrix[i] )>> 3;
                        coeff = -coeff;
                    } else
                    {
                        coeff = (iQP * coeff * piQuantizerMatrix[i] )>> 3;
                    }

                    iSum ^= coeff;
                    piDCTCoeff [i] = (int16)coeff;
                    bCoefQAllZero = FALSE;
                }
            }
        }

    }

    if(!bCoefQAllZero)
    {
        if((iSum & 0x00000001) == 0)
        {
            piDCTCoeff [63] ^= 0x00000001;
        }
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraBlkTCoef_Mpeg
 ** Description:	Get blk_num of current intra MB's TCoef, Mp4.
 ** Author:			Xiaowei Luo
 **	Note:			DeScan and IQ done by SW.
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraBlkTCoef_Mpeg (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int16 *coef_block, int32 iBlkIdx)
{
    int32 iCoefStart = 0;
    DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
    int32 iDCScalar = (iBlkIdx < 4) ? mb_cache_ptr->iDcScalerY : mb_cache_ptr->iDcScalerC;
    int8 nonCoeffPos[64];
    int32 nonCoeffNum = 0;

    Mp4Dec_GetDCACPredDir(vop_mode_ptr, mb_mode_ptr, iBlkIdx);
    vop_mode_ptr->pZigzag = (!mb_mode_ptr->bACPrediction) ? vop_mode_ptr->pStandardZigzag :
                            ((mb_cache_ptr->preddir == VERTICAL) ? vop_mode_ptr->pHorizontalZigzag : vop_mode_ptr->pVerticalZigzag);

    /*decode DC coeff*/
    if(!mb_cache_ptr->bCodeDcAsAc)
    {
        if(!vop_mode_ptr->bDataPartitioning)
        {
            coef_block[0] = (int16)Mp4Dec_VlcDecPredIntraDC(vop_mode_ptr, iBlkIdx);
        }
#ifdef _MP4CODEC_DATA_PARTITION_
        else
        {
            int32 mb_x = vop_mode_ptr->mb_x;
            int32 mb_y = vop_mode_ptr->mb_y;
            int32 total_mb_num_x = vop_mode_ptr->MBNumX;
            int32 mb_num = mb_x+ total_mb_num_x*mb_y;

            coef_block[0] = (int16)vop_mode_ptr->g_dec_dc_store[mb_num][iBlkIdx];
        }
#endif //DATA_PARTITION	
        iCoefStart++;
    }

    /*decode AC coeff*/
    if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
    {
#ifdef _MP4CODEC_DATA_PARTITION_
        if(vop_mode_ptr->bReversibleVlc)
        {
            nonCoeffNum = Mp4Dec_RvlcIntraTCOEF(vop_mode_ptr, coef_block, iCoefStart, nonCoeffPos);
        } else
#endif //DATA_PARTITION	
        {
            nonCoeffNum = Mp4Dec_VlcDecIntraTCOEF(vop_mode_ptr, coef_block, iCoefStart, nonCoeffPos);
        }
    }

    Mp4Dec_GetInverseDCACPred(mb_mode_ptr, mb_cache_ptr, iBlkIdx, coef_block, iDCScalar);

    if(vop_mode_ptr->QuantizerType== Q_MPEG)
    {
        Mp4Dec_MpegIqIntraBlock(vop_mode_ptr,mb_mode_ptr, mb_cache_ptr, coef_block, nonCoeffPos, nonCoeffNum);
    } else
    {
        Mp4Dec_H263IqIntraBlock(mb_mode_ptr, mb_cache_ptr, coef_block, nonCoeffPos, nonCoeffNum);
    }
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraBlkTCoef_H263
 ** Description:	Get blk_num of current intra MB's TCoef, H263.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraBlkTCoef_H263(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int16 *coef_block, int32 iBlkIdx)
{
    int32 DCCoeff;
    DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

    /*decode DC coeff*/
    DCCoeff = Mp4Dec_ReadBits(bitstrm_ptr, 8);
    if(128 == DCCoeff)
    {
        vop_mode_ptr->error_flag = TRUE;
        PRINTF ("Illegal DC coeff: 1000000\n");
        return;
    }

    if(255 == DCCoeff)
    {
        DCCoeff = 128;
    }

    vop_mode_ptr->iCoefStart = 1;
    coef_block[0] = (int16)(DCCoeff *8);
// coef_block[0] = IClip(-2048,2047, coef_block[0] );

    /*decode AC coeff*/
    if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
    {
        Mp4Dec_VlcDecInterTCOEF_H263(vop_mode_ptr, coef_block, mb_mode_ptr->StepSize, bitstrm_ptr);
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
