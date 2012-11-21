/******************************************************************************
 ** File Name:    mp4enc_mb.c	                                              *
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
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

const int32 s_Enc_BlkOffset[] = {0, 8, 128, 136};
const uint8 s_MbType[] = {3, 4, 0, 1, 2};

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
MP4_LOCAL uint32 Mp4Enc_beZeroMV(ENC_MB_MODE_T *pMb_mode)
{
	uint32 bZeroMV = FALSE;
	
	if(pMb_mode->dctMd == INTER)
	{
		if((pMb_mode->mv[0].x == 0) && (pMb_mode->mv[0].y == 0))
		{
			bZeroMV = TRUE;
		}
	}

	return bZeroMV;
}

LOCAL void Mp4Enc_GetNeighborMBPred(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr)
{
	int32 MBPosX = vop_mode_ptr->mb_x;
	int32 MBPosY = vop_mode_ptr->mb_y;
	ENC_MB_BFR_T *pMBCache = vop_mode_ptr->pMBCache;
	ENC_MB_MODE_T *pLeftMB, *pTopMB, *pLeftTopMB;
	int32 currMBPacketNumber = mb_mode_ptr->iPacketNumber;
	
	pMBCache->bTopMBAvail = FALSE;
	pMBCache->bLeftTopAvail = FALSE;
	pMBCache->bLeftMBAvail = FALSE;
		
	if(MBPosY > 0)
	{
		pTopMB = vop_mode_ptr->pMbModeAbv + MBPosX;
		if(pTopMB->bIntra && (currMBPacketNumber == pTopMB->iPacketNumber))
		{
		//	pMBCache->topMBQP = pTopMB->StepSize;
			pMBCache->bTopMBAvail = TRUE;			
		}
		
		if(MBPosX > 0)
		{
			pLeftTopMB = pTopMB - 1;
			if(pLeftTopMB->bIntra && (currMBPacketNumber == pLeftTopMB->iPacketNumber))
			{
		//		pMBCache->leftTopMBQP = pLeftTopMB->StepSize;
				pMBCache->bLeftTopAvail = TRUE;
			}
		}
	}
		
	if(MBPosX > 0)
	{
		pLeftMB = mb_mode_ptr - 1;
		if(pLeftMB->bIntra && (currMBPacketNumber == pLeftMB->iPacketNumber))
		{
		//	pMBCache->leftMBQP = pLeftMB->StepSize;
			pMBCache->bLeftMBAvail = TRUE;			
		}
	}
}

MP4_LOCAL int32 Mp4Enc_EncMBHeadOfIVOP(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	int32  cbpy = (pMb_mode->CBP >> 2) & 0xf;
	uint32 cbpc = pMb_mode->CBP & 3;
	int32 NumBits = 0;
	int32 mb_type = s_MbType[pMb_mode->dctMd];
	
	NumBits = Mp4Enc_OutputMCBPC_Intra(pVop_mode, cbpc, mb_type);
	
	if(!pVop_mode->short_video_header)
	{
		NumBits += Mp4Enc_OutputBits( 0, 1);	//AC prediction, Now, we disable it!
	}
	
	NumBits += Mp4Enc_OutputCBPY(pVop_mode,cbpy);

	return NumBits;
}

MP4_LOCAL int32 Mp4Enc_EncMBHeadOfPVOP(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	uint32 cbpy = (pMb_mode->CBP >> 2) & 0xf;
	uint32 cbpc = pMb_mode->CBP & 3;
	uint32 NumBits = 0;
	int32 mb_type = s_MbType[pMb_mode->dctMd];

	pMb_mode->bSkip = Mp4Enc_beZeroMV(pMb_mode) && (pMb_mode->CBP == 0); 

//	FPRINTF(g_fp_trace_fw,"MB's Qp = %d\n", Qp);	

	//per defintion of H.263's CBPY 
	NumBits += Mp4Enc_OutputBits(  pMb_mode->bSkip, 1);
	
	if(!pMb_mode->bSkip)
	{
		NumBits += Mp4Enc_OutputMCBPC_Inter(pVop_mode, cbpc, mb_type);

   		if(pMb_mode->bIntra)
		{
			pMb_mode->bACPrediction = FALSE;

			if(!pVop_mode->short_video_header)
			{
				NumBits += Mp4Enc_OutputBits(pMb_mode->bACPrediction, 1);	//AC prediction, Now, we disable it!
			}
			cbpy = (pMb_mode->CBP >> 2) & 0xf;
		}else
		{
			cbpy = 15 - cbpy;
		}
		NumBits += Mp4Enc_OutputCBPY(pVop_mode, cbpy);
	}

	return NumBits;
}

void Mp4Enc_VlcIntraMB(ENC_MB_MODE_T *pMb_mode, ENC_VOP_MODE_T  *pVop_mode)
{
	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;

#ifdef SIM_IN_WIN
	FPRINTF(g_fp_trace_fw,"MB cbp: %0x\n", pMb_mode->CBP);
#endif //SIM_IN_WIN

	if(pVop_mode->VopPredType == PVOP)
	{
		/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncMBHeadOfPVOP(pVop_mode, pMb_mode);
	}else
	{
		/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncMBHeadOfIVOP(pVop_mode, pMb_mode);
	}
	
	if(!pVop_mode->short_video_header)
	{
		Mp4Enc_GetNeighborMBPred(pVop_mode, pMb_mode);
	} else { //zcz: short head H263 don't have DC prediction,so don't need these parameters
		pVop_mode->pMBCache->bTopMBAvail = FALSE;
		pVop_mode->pMBCache->bLeftTopAvail = FALSE;
		pVop_mode->pMBCache->bLeftMBAvail = FALSE;
	}

	Mp4Enc_ConfigVLC(pVop_mode, pMb_mode);
}

void Mp4Enc_VlcInterMB(ENC_VOP_MODE_T  *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;
	MOTION_VECTOR_T *pMv = pMb_mode->mv;
	
	/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncMBHeadOfPVOP(pVop_mode, pMb_mode);

	if(!pMb_mode->bSkip)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncMVVP(pVop_mode, pMb_mode);
		
		//zcz: inter MB don't do DC prediction, so don't need these parameters
		pVop_mode->pMBCache->bTopMBAvail = FALSE;
		pVop_mode->pMBCache->bLeftTopAvail = FALSE;
		pVop_mode->pMBCache->bLeftMBAvail = FALSE;
		Mp4Enc_ConfigVLC(pVop_mode, pMb_mode);
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
