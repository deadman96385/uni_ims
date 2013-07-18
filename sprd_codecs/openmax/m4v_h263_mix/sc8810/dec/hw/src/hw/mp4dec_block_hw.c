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
#include "sc8810_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/*****************************************************************************
 **	Name : 			Mp4_DivRoundnearest_hw
 ** Description:	DivRoundnearest function. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
int32 Mp4_DivRoundnearest_hw(int32 i, int32 iDenom)
{
	if(i >= 0)
	{
		return (i+(iDenom>>1))/iDenom;
	}
	else
	{
		return (i-(iDenom>>1))/iDenom;
	}
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
LOCAL int16 *Mp4Dec_GetLeftBlkPred_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, 
					int32 blk_num, int32 *QpLeft, int32 *predAvail)
{
	int16 *pLeftPred = NULL;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 leftAvail = mb_cache_ptr->bLeftMBAvail;
	
	*predAvail = FALSE;
	
	if(blk_num < 4)
	{
		pLeftPred = vop_mode_ptr->pLeftCoeff + (blk_num>>1);// + (blk_num >> 1) * 8;
		if((blk_num & 1) == 0)
		{
			if(leftAvail)
			{
				*predAvail = TRUE;			
				*QpLeft = mb_cache_ptr->leftMBQP;
			}
		}else
		{
			*predAvail = TRUE;	
			*QpLeft = mb_mode_ptr->StepSize;
		}
	}else
	{	
		if(leftAvail)
		{
			*predAvail = TRUE;	
			*QpLeft = mb_cache_ptr->leftMBQP;		
		}
		pLeftPred = vop_mode_ptr->pLeftCoeff + (blk_num - 2); // + (blk_num - 2) * 8;
	}
	
	return pLeftPred;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetTopBlkPred
 ** Description:	get top block prediction. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL int16 *Mp4Dec_GetTopBlkPred_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, 
					int32 blk_num, int32 *QpTop, int32 *predAvail)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	int16 *pTopPred = NULL;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 topAvail = mb_cache_ptr->bTopMBAvail;
	
	*predAvail = FALSE;
	
	if(blk_num < 4)
	{	
	//	pTopPred = vop_mode_ptr->pTopCoeff_sw + mb_x * 4 * 8 + (blk_num & 1) * 8;
		pTopPred = vop_mode_ptr->pTopLeftDCLine + mb_x * 4 + (blk_num & 1);
		if(blk_num < 2)
		{
			if(topAvail)
			{
				*predAvail = TRUE;			
				*QpTop = mb_cache_ptr->topMBQP;
			}			
		}else
		{
			*predAvail = TRUE;	
			*QpTop = mb_mode_ptr->StepSize;
		}
	}else
	{	
		if(topAvail)
		{
			*predAvail = TRUE;
			*QpTop = mb_cache_ptr->topMBQP;
		}	
	//	pTopPred = vop_mode_ptr->pTopCoeff_sw + mb_x * 4 * 8 + (blk_num- 2) * 8;
		pTopPred = vop_mode_ptr->pTopLeftDCLine + mb_x * 4 + (blk_num - 2);
	}
	
	return pTopPred;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetLeftTopBlkPred
 ** Description:	get left top block prediction. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL int32 Mp4Dec_GetLeftTopBlkPred_hw(DEC_VOP_MODE_T *vop_mode_ptr, int32 blk_num)
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
LOCAL void Mp4Dec_GetDCACPredDir_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num)
{
	int16 *predLeft, *predTop;
	int32 leftPredAvail, topPredAvail;
	int32 QpLeft, QpTop;
	int32 iPredLeftTop;
	int32 iHorizontalGrad;
	int32 iVerticalGrad;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	
	predLeft = Mp4Dec_GetLeftBlkPred_hw(vop_mode_ptr, mb_mode_ptr, blk_num, &QpLeft, &leftPredAvail);
	predTop  = Mp4Dec_GetTopBlkPred_hw(vop_mode_ptr, mb_mode_ptr, blk_num, &QpTop, &topPredAvail);
	iPredLeftTop = Mp4Dec_GetLeftTopBlkPred_hw(vop_mode_ptr, blk_num);

	iHorizontalGrad = ((topPredAvail) ? predTop[0] : DEFAULT_DC_VALUE) - iPredLeftTop;
	iVerticalGrad   = ((leftPredAvail) ? predLeft[0] : DEFAULT_DC_VALUE) - iPredLeftTop;

	mb_cache_ptr->BlockPred = NULL;
	mb_cache_ptr->leftPred = predLeft;
	mb_cache_ptr->topPred = predTop;
	if(ABS(iHorizontalGrad) > ABS(iVerticalGrad))
	{
		mb_cache_ptr->preddir   = VERTICAL;
		mb_cache_ptr->BlockPred = predTop;
		mb_cache_ptr->QpPred    = (int8)QpTop;
		mb_cache_ptr->predAvail = (int8)topPredAvail;
		vop_mode_ptr->pZigzag = vop_mode_ptr->pHorizontalZigzag;
	}else
	{
		mb_cache_ptr->preddir   = HORIZONTAL;
		mb_cache_ptr->BlockPred = predLeft;
		mb_cache_ptr->QpPred    = (int8)QpLeft;
		mb_cache_ptr->predAvail = (int8)leftPredAvail;
		vop_mode_ptr->pZigzag = vop_mode_ptr->pVerticalZigzag;
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
LOCAL void Mp4Dec_GetInverseDCACPred_hw(DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, 
					int32 iBlkIdx, int16 *rgiCoefQ, 	int32 iDcScaler)
{	
	int32 iDefVal = DEFAULT_DC_VALUE; // NBIT
	int16 *pLeftPred = mb_cache_ptr->leftPred;
	int16 *pTopPred = mb_cache_ptr->topPred;
	int16 *blkmPred = mb_cache_ptr->BlockPred;	
	int32 predAvail = mb_cache_ptr->predAvail;
	int16 tmp;

	if(predAvail)
	{
		rgiCoefQ[0] += (int16)Mp4_DivRoundnearest_hw(blkmPred[0], iDcScaler);	
	}else
	{
		rgiCoefQ[0] += (int16)Mp4_DivRoundnearest_hw(iDefVal, iDcScaler); 	
	}
	rgiCoefQ[0] = IClip(-2048, 2047, rgiCoefQ[0]);
#if 0
	if(mb_mode_ptr->bACPrediction && predAvail)
	{
		int32 i;
		int32 iQP = mb_mode_ptr->StepSize;
		int32 iQpPred = mb_cache_ptr->QpPred;		
		
		if(HORIZONTAL == mb_cache_ptr->preddir)
		{
			if(iQP == iQpPred)
			{
				rgiCoefQ[1*8] += blkmPred[1];
				rgiCoefQ[2*8] += blkmPred[2];
				rgiCoefQ[3*8] += blkmPred[3];
				rgiCoefQ[4*8] += blkmPred[4];
				rgiCoefQ[5*8] += blkmPred[5];
				rgiCoefQ[6*8] += blkmPred[6];
				rgiCoefQ[7*8] += blkmPred[7];
			}else
			{
				for(i = 1; i < 8; i++)
				{
					rgiCoefQ[i*8] += (int16)Mp4_DivRoundnearest_hw(blkmPred[i] * iQpPred, iQP);
				}
			}
		}else
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
			}else
			{
				for(i = 1; i < 8; i++)
				{
					rgiCoefQ[i] += (int16)Mp4_DivRoundnearest_hw(blkmPred[i] * iQpPred, iQP);
				}
			}
		}
	}
#endif
	tmp = rgiCoefQ[0];

	/*store first row and first column for later prediction*/
	rgiCoefQ[0] = (int16)(rgiCoefQ[0] * iDcScaler);   //inverse quantization of DC coefficient
	rgiCoefQ[0] = IClip(-2048, 2047, rgiCoefQ[0]);

	if(iBlkIdx < 2)
	{
		mb_cache_ptr->pDCCache[5 + iBlkIdx] = rgiCoefQ[0];
	}
#if 0
	((int32 *)pTopPred)[0] = ((int32 *)rgiCoefQ)[0];
	((int32 *)pTopPred)[1] = ((int32 *)rgiCoefQ)[1];
	((int32 *)pTopPred)[2] = ((int32 *)rgiCoefQ)[2];
	((int32 *)pTopPred)[3] = ((int32 *)rgiCoefQ)[3];

	pLeftPred[0] = rgiCoefQ[0*8];
	pLeftPred[1] = rgiCoefQ[1*8];  
	pLeftPred[2] = rgiCoefQ[2*8];
	pLeftPred[3] = rgiCoefQ[3*8];
	pLeftPred[4] = rgiCoefQ[4*8];
	pLeftPred[5] = rgiCoefQ[5*8];
	pLeftPred[6] = rgiCoefQ[6*8];
	pLeftPred[7] = rgiCoefQ[7*8];
#else
	pTopPred[0] = pLeftPred[0] = rgiCoefQ[0];	
#endif
	rgiCoefQ[0] = tmp;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraBlkTCoef_Mp4
 ** Description:	Get blk_num of current intra MB's TCoef, Mp4.
 ** Author:			Xiaowei Luo
 **	Note:			DeScan and IQ done by SW.
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraBlkTCoef_Mp4_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 iBlkIdx)
{
	int32 iCoefStart = 0;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int16 *pBlk = vop_mode_ptr->coef_block[iBlkIdx];
	int32 iDCScalar = (iBlkIdx < 4) ? mb_cache_ptr->iDcScalerY : mb_cache_ptr->iDcScalerC;
	
//	memset(pBlk, 0, 64 * sizeof(int16));	
	Mp4Dec_GetDCACPredDir_hw(vop_mode_ptr, mb_mode_ptr, iBlkIdx);

	if(!mb_mode_ptr->bACPrediction)
	{
		vop_mode_ptr->pZigzag = vop_mode_ptr->pStandardZigzag;
	}
	
	/*decode DC coeff*/	
	if(!mb_cache_ptr->bCodeDcAsAc)
	{
		if(!vop_mode_ptr->bDataPartitioning)
		{
			pBlk[0] = (int16)Mp4Dec_VlcDecPredIntraDC(vop_mode_ptr, iBlkIdx);			
		}
	#ifdef _MP4CODEC_DATA_PARTITION_	
		else
		{
			int32 mb_x = vop_mode_ptr->mb_x;
			int32 mb_y = vop_mode_ptr->mb_y;
			int32 total_mb_num_x = vop_mode_ptr->MBNumX;
			int32 mb_num = mb_x+ total_mb_num_x*mb_y;

			pBlk[0] = (int16)vop_mode_ptr->g_dec_dc_store[mb_num][iBlkIdx];
		}
	#endif //DATA_PARTITION	
		iCoefStart++;	
	}else
	{
		pBlk[0] = 0;
	}
	
	/*decode AC coeff*/
	if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
	{
	#ifdef _MP4CODEC_DATA_PARTITION_
		if(vop_mode_ptr->bReversibleVlc)
		{			
			Mp4Dec_RvlcIntraTCOEF(vop_mode_ptr, pBlk, iCoefStart);		
		}else
	#endif //DATA_PARTITION	
		{
			Mp4Dec_VlcDecIntraTCOEF_hw(vop_mode_ptr, pBlk, iCoefStart);
		}
		
		if(vop_mode_ptr->error_flag)
		{
			PRINTF ("decode intra mb coefficient error!\n");
			vop_mode_ptr->return_pos1 |= (1<<0);
			return;
		}
	}

	Mp4Dec_GetInverseDCACPred_hw(mb_mode_ptr, mb_cache_ptr, iBlkIdx, pBlk, iDCScalar);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetIntraBlkTCoef_H263
 ** Description:	Get blk_num of current intra MB's TCoef, H263.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetIntraBlkTCoef_H263_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 iBlkIdx)
{
	int32 DCCoeff;
	int16 *piDCTCoeff = vop_mode_ptr->coef_block[iBlkIdx];
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
		
	/*decode DC coeff*/	
	DCCoeff = Mp4Dec_ReadBits(bitstrm_ptr, 8);	
	if(128 == DCCoeff)
	{
		vop_mode_ptr->error_flag = TRUE;
		PRINTF ("Illegal DC coeff: 1000000\n");
		vop_mode_ptr->return_pos1 |= (1<<1);
		return;
	}
	
	if(255 == DCCoeff)
	{
		DCCoeff = 128;	
	}
	
	piDCTCoeff[0] = (int16)DCCoeff;	
	
	/*decode AC coeff*/
	if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
	{
		Mp4Dec_VlcDecInterTCOEF_H263_hw(vop_mode_ptr/*, piDCTCoeff*/, 1, bitstrm_ptr);

		if(vop_mode_ptr->error_flag)
		{
			PRINTF ("decode inter coeff h263 error!\n");
			vop_mode_ptr->return_pos2 |= (1<<4);
			return;
		}
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
