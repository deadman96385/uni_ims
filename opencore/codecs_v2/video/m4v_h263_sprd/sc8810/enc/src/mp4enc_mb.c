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
#include "sc8810_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

static const int32 s_Enc_BlkOffset[] = {0, 8, 128, 136};
static const uint8 s_MbType[] = {3, 4, 0, 1, 2};

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
MP4_LOCAL uint32 Mp4Enc_beZeroMV(ENC_MB_MODE_T *mb_mode_ptr)
{
	uint32 bZeroMV = FALSE;
	
	if(mb_mode_ptr->dctMd == INTER)
	{
		if((mb_mode_ptr->mv[0].x == 0) && (mb_mode_ptr->mv[0].y == 0))
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

MP4_LOCAL int32 Mp4Enc_EncMBHeadOfIVOP(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr)
{
	int32  cbpy = (mb_mode_ptr->CBP >> 2) & 0xf;
	uint32 cbpc = mb_mode_ptr->CBP & 3;
	int32 NumBits = 0;
	int32 mb_type = s_MbType[mb_mode_ptr->dctMd];
	
	NumBits = Mp4Enc_OutputMCBPC_Intra(vop_mode_ptr, cbpc, mb_type);
	
	if(!vop_mode_ptr->short_video_header)
	{
		NumBits += (int32)Mp4Enc_OutputBits( 0, 1);	//AC prediction, Now, we disable it!
	}
	
	NumBits += (int32)Mp4Enc_OutputCBPY(vop_mode_ptr,cbpy);

	return NumBits;
}

MP4_LOCAL int32 Mp4Enc_EncMBHeadOfPVOP(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr)
{
	uint32 cbpy = (mb_mode_ptr->CBP >> 2) & 0xf;
	uint32 cbpc = mb_mode_ptr->CBP & 3;
	uint32 NumBits = 0;
	int32 mb_type = s_MbType[mb_mode_ptr->dctMd];

	mb_mode_ptr->bSkip = Mp4Enc_beZeroMV(mb_mode_ptr) && (mb_mode_ptr->CBP == 0); 

//	FPRINTF(g_fp_trace_fw,"MB's Qp = %d\n", Qp);	

	//per defintion of H.263's CBPY 
	NumBits += Mp4Enc_OutputBits(  mb_mode_ptr->bSkip, 1);
	
	if(!mb_mode_ptr->bSkip)
	{
		NumBits += Mp4Enc_OutputMCBPC_Inter(vop_mode_ptr, cbpc, mb_type);

   		if(mb_mode_ptr->bIntra)
		{
			mb_mode_ptr->bACPrediction = FALSE;

			if(!vop_mode_ptr->short_video_header)
			{
				NumBits += Mp4Enc_OutputBits(mb_mode_ptr->bACPrediction, 1);	//AC prediction, Now, we disable it!
			}
			cbpy = (mb_mode_ptr->CBP >> 2) & 0xf;
		}else
		{
			cbpy = 15 - cbpy;
		}
		NumBits += Mp4Enc_OutputCBPY(vop_mode_ptr, cbpy);
	}

	return NumBits;
}

void Mp4Enc_VlcIntraMB(ENC_MB_MODE_T *mb_mode_ptr, ENC_VOP_MODE_T  *vop_mode_ptr)
{
//	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;

#ifdef SIM_IN_WIN
	FPRINTF(g_fp_trace_fw,"MB cbp: %0x\n", mb_mode_ptr->CBP);
#endif //SIM_IN_WIN

	if(vop_mode_ptr->VopPredType == PVOP)
	{
		/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncMBHeadOfPVOP(vop_mode_ptr, mb_mode_ptr);
	}else
	{
		/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncMBHeadOfIVOP(vop_mode_ptr, mb_mode_ptr);
	}
	
	if(!vop_mode_ptr->short_video_header)
	{
		Mp4Enc_GetNeighborMBPred(vop_mode_ptr, mb_mode_ptr);
	}

	Mp4Enc_ConfigVLC(vop_mode_ptr, mb_mode_ptr);
}

void Mp4Enc_VlcInterMB(ENC_VOP_MODE_T  *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr)
{
//	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;
//	MOTION_VECTOR_T *pMv = pMb_mode->mv;
	
	/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncMBHeadOfPVOP(vop_mode_ptr, mb_mode_ptr);

	if(!mb_mode_ptr->bSkip)
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncMVVP(vop_mode_ptr, mb_mode_ptr);

		Mp4Enc_ConfigVLC(vop_mode_ptr, mb_mode_ptr);
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
