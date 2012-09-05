/******************************************************************************
 ** File Name:    mp4dec_mb.c                                              *
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

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
 
LOCAL void Mp4Dec_GetLeftTopDC_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	DEC_MB_MODE_T *pTopLeftMB;
	int16 *pDCCache = mb_cache_ptr->pDCCache;
	int16 *pLeftDCCache = mb_cache_ptr->pLeftDCCache;
	int16 *pTopDCACPred = vop_mode_ptr->pTopLeftDCLine + mb_x * 4; //vop_mode_ptr->pTopCoeff + mb_x * 4 * 8;

	pDCCache[0] = DEFAULT_DC_VALUE;
	pDCCache[1] = DEFAULT_DC_VALUE;
	pDCCache[4] = DEFAULT_DC_VALUE;
	pDCCache[3] = DEFAULT_DC_VALUE;
	pDCCache[7] = DEFAULT_DC_VALUE;

	if(mb_cache_ptr->bTopMBAvail)
	{
		DEC_MB_MODE_T *pTopMB = mb_mode_ptr - vop_mode_ptr->MBNumX;
		if(pTopMB->bIntra)
		{
			pDCCache[1] = pTopDCACPred[0];
		}
	}

	if(mb_cache_ptr->bLeftTopAvail)
	{
		pTopLeftMB = mb_mode_ptr - vop_mode_ptr->MBNumX - 1;

		if(pTopLeftMB->bIntra)
		{
			if((mb_mode_ptr-1)->bIntra) //left mb has updated pTopLeftDCLine
			{
				pDCCache[0] = (uint16)pLeftDCCache[0];
				pDCCache[3] = (uint16)pLeftDCCache[1];
				pDCCache[7] = (uint16)pLeftDCCache[2];				
			}else
			{
				pDCCache[0] = (uint16)pTopDCACPred[-3];
				pDCCache[3] = (uint16)pTopDCACPred[-2];
				pDCCache[7] = (uint16)pTopDCACPred[-1];
			}			
		}
	}

	if(mb_cache_ptr->bLeftMBAvail)
	{
		DEC_MB_MODE_T *pLeftMB = mb_mode_ptr - 1;
		if(pLeftMB->bIntra)
		{
			pDCCache[4] = pDCCache[6];
		}
	}

	pLeftDCCache[0] = pTopDCACPred[1];  //Y copy top DC coeff as left top DC for next MB
	pLeftDCCache[1] = pTopDCACPred[2]; //U
	pLeftDCCache[2] = pTopDCACPred[3]; //V
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBTexture_hw
 ** Description:	Get the texture of intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBTexture_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 QP = mb_mode_ptr->StepSize;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 uv_dc = 0;
	int32 blk_num;

	mb_cache_ptr->bCodeDcAsAc = FALSE;

	if(MPEG4 == vop_mode_ptr->video_std)
	{
		mb_cache_ptr->iDcScalerY = g_dc_scaler_table_y[QP];
		mb_cache_ptr->iDcScalerC = g_dc_scaler_table_c[QP];
		mb_cache_ptr->bCodeDcAsAc = Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr);

		Mp4Dec_GetNeighborMBPred(vop_mode_ptr, mb_mode_ptr);
		Mp4Dec_GetLeftTopDC_hw(vop_mode_ptr, mb_mode_ptr);
	}

	Mp4Dec_ConfigVldMB(vop_mode_ptr, mb_mode_ptr);

	for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
	{
		g_Mp4Dec_GetIntraBlkTCoef_hw(vop_mode_ptr, mb_mode_ptr, blk_num);

		if(vop_mode_ptr->error_flag)
		{
			PRINTF("Mp4Dec_GetIntraBlockTexture error!\n");
			vop_mode_ptr->return_pos2 |= (1<<10);
			return;
		}
	}
	vop_mode_ptr->bitstrm_ptr->bitcnt_before_vld = vop_mode_ptr->bitstrm_ptr->bitcnt;

	/*iq idct*/
	Mp4Dec_ConfigIqIdctMB(vop_mode_ptr, mb_mode_ptr);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBTexture
 ** Description:	Get the texture of inter macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBTexture_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 iBlkIdx;
	
	if(mb_mode_ptr->CBP == 0)
	{
		return; //do not need start VLD and idct.
	}

	Mp4Dec_ConfigVldMB(vop_mode_ptr, mb_mode_ptr);

	/*sw vld*/
	for (iBlkIdx = 0; iBlkIdx < BLOCK_CNT; iBlkIdx++)
	{
		if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
		{
			if(MPEG4 != vop_mode_ptr->video_std)
			{
				Mp4Dec_VlcDecInterTCOEF_H263_hw(vop_mode_ptr/*, pBlk*/, 0, vop_mode_ptr->bitstrm_ptr);
			}
			else 
			{
				if((!vop_mode_ptr->bReversibleVlc) || (BVOP == vop_mode_ptr->VopPredType))
				{
					Mp4Dec_VlcDecInterTCOEF_Mpeg_hw(vop_mode_ptr/*, pBlk*/, vop_mode_ptr->bitstrm_ptr);
				}
			#ifdef _MP4CODEC_DATA_PARTITION_	
				else
				{
					int16 *pBlk = vop_mode_ptr->coef_block[iBlkIdx];
					Mp4Dec_RvlcInterTCOEF(vop_mode_ptr, pBlk);
				}
			#endif //DATA_PARTITION	

			}		
		
			if (vop_mode_ptr->error_flag)
			{
				PRINTF ("decodeTextureInterBlock error!\n");	
				vop_mode_ptr->return_pos2 |= (1<<11);
				return;
			}
		}
	}

	vop_mode_ptr->bitstrm_ptr->bitcnt_before_vld = vop_mode_ptr->bitstrm_ptr->bitcnt;

	/*iq idct*/
	Mp4Dec_ConfigIqIdctMB(vop_mode_ptr, mb_mode_ptr);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
