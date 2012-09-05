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

#ifndef INTRA_IDCT_ASSEMBLY
#define	DCT8X8_INTRA	IDCTcFix16
#else
#define	DCT8X8_INTRA	arm_IDCTcFix
#endif

#ifndef INTER_IDCT_ASSEMBLY 
#define	DCT8X8_INTER	IDCTiFix
#else
#define	DCT8X8_INTER 	arm_IDCTiFix
#endif
 
LOCAL void Mp4Dec_GetLeftTopDC_sw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	DEC_MB_MODE_T *pTopLeftMB;
	int16 *pDCCache = mb_cache_ptr->pDCCache;
	int16 *pLeftDCCache = mb_cache_ptr->pLeftDCCache;
       int16 *pTopDCACPred = vop_mode_ptr->pTopCoeff + (mb_x <<5); 

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
				pDCCache[0] = (uint16)pTopDCACPred[-24];
				pDCCache[3] = (uint16)pTopDCACPred[-16];
				pDCCache[7] = (uint16)pTopDCACPred[-8];
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

	pLeftDCCache[0] = pTopDCACPred[8];  //Y copy top DC coeff as left top DC for next MB
	pLeftDCCache[1] = pTopDCACPred[16]; //U
	pLeftDCCache[2] = pTopDCACPred[24]; //V
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBTexture_sw
 ** Description:	Get the texture of intra macroblock.
 ** Author:			Xiaowei Luo, Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBTexture_sw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
	int32 blk_num;
	uint8 *rgchBlkDst;
	int32 iWidthDst;
	int16 *coef_block;

	if(MPEG4 == vop_mode_ptr->video_std)
	{
		int32 QP = mb_mode_ptr->StepSize;
		
		mb_cache_ptr->iDcScalerY = g_dc_scaler_table_y[QP];
		mb_cache_ptr->iDcScalerC = g_dc_scaler_table_c[QP];
		mb_cache_ptr->bCodeDcAsAc = Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr);

		Mp4Dec_GetNeighborMBPred(vop_mode_ptr, mb_mode_ptr);
		Mp4Dec_GetLeftTopDC_sw(vop_mode_ptr, mb_mode_ptr, mb_cache_ptr);
	}

	for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
	{
	 	coef_block = vop_mode_ptr->coef_block[blk_num];
		memset(coef_block, 0, 64 * sizeof(int16));	
		g_Mp4Dec_GetIntraBlkTCoef_sw(vop_mode_ptr, mb_mode_ptr, coef_block, blk_num);
	}

	//y
	iWidthDst = vop_mode_ptr->FrameExtendWidth;
	for(blk_num = 0; blk_num < (BLOCK_CNT-2); blk_num++)
	{
		coef_block = vop_mode_ptr->coef_block[blk_num];
		rgchBlkDst = mb_cache_ptr->mb_addr[0] + mb_cache_ptr->blk8x8_offset[blk_num];
		DCT8X8_INTRA(coef_block, rgchBlkDst, iWidthDst);
	}

	//u
	iWidthDst = vop_mode_ptr->FrameExtendWidth>>1;
	DCT8X8_INTRA(vop_mode_ptr->coef_block[4], mb_cache_ptr->mb_addr[1], iWidthDst);
	//v
	DCT8X8_INTRA(vop_mode_ptr->coef_block[5], mb_cache_ptr->mb_addr[2], iWidthDst);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBTexture_sw
 ** Description:	Get the texture of inter macroblock.
 ** Author:			Xiaowei Luo,Leon Li
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBTexture_sw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
	int32 CBP = mb_mode_ptr->CBP;
	int32 iBlkIdx;
	int32 iQP = mb_mode_ptr->StepSize;
	int16* rgiBlkCurrQ;
	int32 iWidthCurrQ = MB_SIZE;
	uint8*refBlk;
	uint8 *dstImg;
	int32 dstImg_width;
	
	vop_mode_ptr->iCoefStart = 0;
	
	for (iBlkIdx = 0; iBlkIdx < BLOCK_CNT; iBlkIdx++)
	{
		rgiBlkCurrQ = vop_mode_ptr->coef_block[iBlkIdx];
			
		if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
		{		
			memset(rgiBlkCurrQ, 0, 64 * sizeof(int16));	
			g_Mp4Dec_GetInterBlkTCoef_sw(vop_mode_ptr, rgiBlkCurrQ, iQP, vop_mode_ptr->bitstrm_ptr);
		}
	}

	//y
	for (iBlkIdx = 0; iBlkIdx < (BLOCK_CNT-2); iBlkIdx++)
	{
		rgiBlkCurrQ = vop_mode_ptr->coef_block[iBlkIdx];
		refBlk = mb_cache_ptr->pMBBfrY+ ((iBlkIdx>> 1) <<7)+( (iBlkIdx & 1) <<3);
		dstImg_width = vop_mode_ptr->FrameExtendWidth;
		dstImg = mb_cache_ptr->mb_addr[0] + mb_cache_ptr->blk8x8_offset[iBlkIdx];

		if(CBP & (32 >> iBlkIdx))
		{
			DCT8X8_INTER (rgiBlkCurrQ, dstImg, dstImg_width, refBlk, iWidthCurrQ);
		}else
		{
			mc_xyfull_8x8(refBlk, dstImg, iWidthCurrQ, dstImg_width);
		}
	}

	//u
	dstImg_width = vop_mode_ptr->FrameExtendWidth>>1;
	rgiBlkCurrQ = vop_mode_ptr->coef_block[4];
	refBlk = mb_cache_ptr->pMBBfrU;
	dstImg = mb_cache_ptr->mb_addr[1];
	if(CBP & 0x2)
	{
		DCT8X8_INTER (rgiBlkCurrQ, dstImg, dstImg_width, refBlk, BLOCK_SIZE);
	}else
	{
		mc_xyfull_8x8(refBlk, dstImg, BLOCK_SIZE, dstImg_width);
	}

	//v
	rgiBlkCurrQ = vop_mode_ptr->coef_block[5];
	refBlk = mb_cache_ptr->pMBBfrV;
	dstImg = mb_cache_ptr->mb_addr[2];
	if(CBP & 0x1)
	{
		DCT8X8_INTER (rgiBlkCurrQ, dstImg, dstImg_width, refBlk, BLOCK_SIZE);
	}else
	{
		mc_xyfull_8x8(refBlk, dstImg, BLOCK_SIZE, dstImg_width);
	}
}

#ifndef _NEON_OPT_
//only for interpolation Bframe

void Mp4Dec_CopyRef2RecMB_BFrame(DEC_VOP_MODE_T *pVopmd, unsigned char* pRefFrame[],\
uint8* pBlkY256,uint8* pBlkU64,uint8* pBlkV64)//frameFlag 0: frdRef  1,backRef
{
	int32 i;
	int32 offset;
	int32 width = pVopmd->FrameExtendWidth;
	uint8 * pRefFrm, * pRecMB;
	int32 * pIntSrc, * pIntDst;
	int32 iMBX = pVopmd->mb_x;
	int32 iMBY = pVopmd->mb_y;
	
	offset  = (iMBY * MB_SIZE + YEXTENTION_SIZE) * width + iMBX * MB_SIZE + YEXTENTION_SIZE;
//SCI_TRACE_LOW("leonYUVRefFrame0[0] %x %s,%d",pVopmd->YUVRefFrame0[0],__FUNCTION__,__LINE__);

    pRefFrm =  pRefFrame[0] + offset;  //Y refFrame
	pRecMB = pBlkY256;//	pMBCache->pMBBfrY
	pIntDst = (int32 *)pRecMB;

	/*copy Y ref MB*/
	for(i = 0; i < MB_SIZE; i++)
	{
		pIntSrc = (int32 *)pRefFrm;

		*pIntDst++ = pIntSrc[0];
		*pIntDst++ = pIntSrc[1];
		*pIntDst++ = pIntSrc[2];
		*pIntDst++ = pIntSrc[3];

		pRefFrm += width;
	}
	
	width = width / 2;
	offset = (iMBY * BLOCK_SIZE + UVEXTENTION_SIZE) * width + iMBX * BLOCK_SIZE + UVEXTENTION_SIZE;

	/*U*/

         pRefFrm =  pRefFrame[1] + offset;  //U refFrame
	pRecMB = pBlkU64;//pMBCache->pMBBfrU; 
	pIntDst = (int32 *)pRecMB;
	
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		pIntSrc = (int32 *)pRefFrm;
		*pIntDst++ = pIntSrc[0];
		*pIntDst++ = pIntSrc[1];

		pRefFrm += width;
	}
	
	/*V*/


	pRefFrm =  pRefFrame[2] + offset;  //V refFrame
	pRecMB = pBlkV64;//pMBCache->pMBBfrV; 
	pIntDst = (int32 *)pRecMB;
	
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pIntSrc = (int32 *)pRefFrm;
		*pIntDst++ = pIntSrc[0];
		*pIntDst++ = pIntSrc[1];
		
		pRefFrm += width;
	}
}
#else
void Mp4Dec_CopyRef2RecMB_BFrame(DEC_VOP_MODE_T *pVopmd, unsigned char* pRefFrame[],\
uint8* pBlkY256,uint8* pBlkU64,uint8* pBlkV64)//frameFlag 0: frdRef  1,backRef
{
	int32_t i;
	int32_t offset;
	int32_t width = pVopmd->FrameExtendWidth;
	uint8 *pRefFrm, *pRecMB;
	uint8 *pIntSrc, *pIntDst;
	int32_t iMBX = pVopmd->mb_x;
	int32_t iMBY = pVopmd->mb_y;
	uint8x16_t i16bytes;
	uint8x8_t i8bytes;
	
	//offset  = (iMBY * MB_SIZE + YEXTENTION_SIZE) * width + iMBX * MB_SIZE + YEXTENTION_SIZE;
	offset  = ((iMBY <<4) + YEXTENTION_SIZE) * width + (iMBX <<4) + YEXTENTION_SIZE;
        pRefFrm =  pRefFrame[0] + offset;  //Y refFrame
	pRecMB = pBlkY256;//	pMBCache->pMBBfrY
	pIntDst = pRecMB;

	/*copy Y ref MB*/
	for(i = 0; i < MB_SIZE; i++)
	{
		pIntSrc = pRefFrm;
		
         	i16bytes = vld1q_u8(pIntSrc);
		pRefFrm += width;
          	vst1q_u8(pIntDst,i16bytes);
			

                pIntDst += MB_SIZE;
		
	}
	
	width = width >>1;
	offset = ((iMBY <<3) + UVEXTENTION_SIZE) * width + (iMBX <<3) + UVEXTENTION_SIZE;

	/*U*/

         pRefFrm =  pRefFrame[1] + offset;  //U refFrame
	pRecMB = pBlkU64;//pMBCache->pMBBfrU; 
	pIntDst = pRecMB;
	
	for(i = 0; i < BLOCK_SIZE; i++)
	{
		pIntSrc = pRefFrm;
		
         	i8bytes = vld1_u8(pIntSrc);
		pRefFrm += width;	
          	vst1_u8(pIntDst,i8bytes);

		
		pIntDst += BLOCK_SIZE;
	}
	
	/*V*/


	pRefFrm =  pRefFrame[2] + offset;  //V refFrame
	pRecMB = pBlkV64;//pMBCache->pMBBfrV; 
	pIntDst = pRecMB;
	
	for (i = 0; i < BLOCK_SIZE; i++)
	{
		pIntSrc = pRefFrm;
		
         	i8bytes = vld1_u8(pIntSrc);
		pRefFrm += width;	
          	vst1_u8(pIntDst,i8bytes);

		
		pIntDst += BLOCK_SIZE;
	}
}

#endif //_NEON_OPT_

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
