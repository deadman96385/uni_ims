/******************************************************************************
 ** File Name:    mp4enc_mv.c			                                      *
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

MP4_LOCAL void Mp4Enc_ScaleMV(int32 *vlc, int32 *residual, int32 diff_mv_component, const MV_INFO_T *pMv_dir)
{
	uint32 abs_diff_mv_component;
	uint32 vlc_magnitude;
	
	if(diff_mv_component < (-1 * (int32) pMv_dir->Range))
	{
		diff_mv_component += 2 * pMv_dir->Range;
	}else if(diff_mv_component > (int32) (pMv_dir->Range - 1))
	{
		diff_mv_component -= 2 * pMv_dir->Range;
	}
	
	if(diff_mv_component == 0)	
	{									//nothing to do
		*vlc = 0;
		*residual = 0;
	}else
	{
		if(pMv_dir->ScaleFactor == 1)
		{									//simple no-scale case
			*vlc = diff_mv_component;
			*residual = 0;
		}else
		{														//stupid scaling
			abs_diff_mv_component = abs(diff_mv_component);
			*residual = (abs_diff_mv_component - 1) % pMv_dir->ScaleFactor;																	
			vlc_magnitude = (abs_diff_mv_component - (uint32)(*residual) + (pMv_dir->ScaleFactor - 1))/(pMv_dir->ScaleFactor);	// absorb ++ into here (g_EncVolmd.mvInfo.uiScaleFactor - 1)
			*vlc = (int32)vlc_magnitude * ssign(diff_mv_component);
		}
	}
}

MP4_LOCAL uint32 Mp4Enc_SendDiffMV(ENC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *diff_mv_halfpel)
{
	int32 vlc;
	int32 residual;
	uint32 NumBits = 0;
	int32 entry;	
	const MV_INFO_T *pMv_dir = &(vop_mode_ptr->mvInfoForward);
	
	Mp4Enc_ScaleMV(&vlc, &residual, (*diff_mv_halfpel).x, pMv_dir);

	if(vlc < 0)
	{
		entry = vlc + 65;
	}else
	{
		entry = vlc;
	}

	NumBits += Mp4Enc_OutputMV(vop_mode_ptr, entry);

	if((pMv_dir->FCode != 1) && (vlc != 0))
	{
		NumBits += Mp4Enc_OutputBits(residual, pMv_dir->FCode - 1);
	}
	
	Mp4Enc_ScaleMV(&vlc, &residual, (*diff_mv_halfpel).y, pMv_dir);
	
	if(vlc < 0)
	{
		entry = vlc + 65;
	}else
	{
		entry = vlc;
	}
	
	NumBits += Mp4Enc_OutputMV(vop_mode_ptr, entry);
	
	if((pMv_dir->FCode != 1) && (vlc != 0))
	{
		NumBits += Mp4Enc_OutputBits(residual, pMv_dir->FCode - 1);
	}

	return NumBits;
}

#define JudegeLeftBndry(pos_x, mb_mode_ptr) \
	((pos_x == 0) || ((mb_mode_ptr-1)->iPacketNumber != mb_mode_ptr->iPacketNumber)) ? FALSE : TRUE ;
#define JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr)  \
	((pos_y == 0) || ((vop_mode_ptr->pMbModeAbv+pos_x)->iPacketNumber != mb_mode_ptr->iPacketNumber)) ? FALSE : TRUE;
#define JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr)  \
	((pos_x == vop_mode_ptr->MBNumX - 1) || (pos_y == 0) || ((vop_mode_ptr->pMbModeAbv+pos_x + 1)->iPacketNumber != mb_mode_ptr->iPacketNumber)) ? FALSE : TRUE;

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPredAtBndry
 ** Description:	Get motion vector prediction in 4mv condition when at boundary. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Enc_Get8x8MVPredAtBndry(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr, ENC_MB_BFR_T *pMBCache, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 MBPosX)
{
	int32 nInBound = 0;
	MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
	ENC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;

	VecCand[0] = zeroMv;
	VecCand[1] = zeroMv;
	VecCand[2] = zeroMv;

	pMb_mode_Left = mb_mode_ptr - 1;
	pMb_mode_Top = vop_mode_ptr->pMbModeAbv + MBPosX;
	pMb_mode_Right = pMb_mode_Top + 1;

	switch(blk_num)
	{
	case 0:
		if(pMBCache->bLeftMBAvail)
		{
			((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[1];
			nInBound++;			
		}

		if(pMBCache->bTopMBAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Top->mv))[2];
			nInBound++;
		}

		if(pMBCache->rightAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Right->mv))[2];
			nInBound++;
		}  
		break;

	case 1:
		((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[0];
		nInBound++;

		if(pMBCache->bTopMBAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Top->mv))[3];
			nInBound++;
		}

		if(pMBCache->rightAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Right->mv))[2];
			nInBound++;
		}		
		
		break;

	case 2:
		if(pMBCache->bLeftMBAvail)
		{
			((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[3];
			nInBound++;
		}

		((int32 *)VecCand)[nInBound] = ((int32 *)(mb_mode_ptr->mv))[0];
		nInBound++;		
		
		((int32 *)VecCand)[nInBound] = ((int32 *)(mb_mode_ptr->mv))[1];
		nInBound++;		

		break;

	case 3:
		((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[2];
		((int32 *)VecCand)[1] = ((int32 *)(mb_mode_ptr->mv))[0];
		((int32 *)VecCand)[2] = ((int32 *)(mb_mode_ptr->mv))[1];

		nInBound = 3;
		
		break;

	default:

		break;
	}

	if(nInBound == 1)
	{
		((int32 *)mvPred)[0] = ((int32 *)VecCand)[0];
		return;
	}	
	
	mvPred->x = (int16)Mp4_GetMedianofThree(VecCand[0].x, VecCand[1].x, VecCand[2].x);
	mvPred->y = (int16)Mp4_GetMedianofThree(VecCand[0].y, VecCand[1].y, VecCand[2].y);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPredNotAtBndry
 ** Description:	Get motion vector prediction in 4mv condition when not at boundary. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Enc_Get8x8MVPredNotAtBndry(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 MBPosX)
{
	MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
	ENC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;
	
	VecCand[0] = zeroMv;
	VecCand[1] = zeroMv;
	VecCand[2] = zeroMv;

	pMb_mode_Left = mb_mode_ptr - 1;
	pMb_mode_Top = vop_mode_ptr->pMbModeAbv + MBPosX;
	pMb_mode_Right = pMb_mode_Top + 1;

	switch(blk_num)
	{
	case 0:
		((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[1];		//left
		((int32 *)VecCand)[1] = ((int32 *)(pMb_mode_Top->mv))[2];		//top
		((int32 *)VecCand)[2] = ((int32 *)(pMb_mode_Right->mv))[2];//top right
		break;

	case 1:
		((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[0];
		((int32 *)VecCand)[1] = ((int32 *)(pMb_mode_Top->mv))[3];
		((int32 *)VecCand)[2] = ((int32 *)(pMb_mode_Right->mv))[2];
		break;

	case 2:
		((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[3];
		((int32 *)VecCand)[1] = ((int32 *)(mb_mode_ptr->mv))[0];
		((int32 *)VecCand)[2] = ((int32 *)(mb_mode_ptr->mv))[1];
		break;

	case 3:
		((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[2];
		((int32 *)VecCand)[1] = ((int32 *)(mb_mode_ptr->mv))[0];
		((int32 *)VecCand)[2] = ((int32 *)(mb_mode_ptr->mv))[1];
		break;

	default:
		break;
	}

	mvPred->x = (int16)Mp4_GetMedianofThree(VecCand[0].x, VecCand[1].x, VecCand[2].x);
	mvPred->y = (int16)Mp4_GetMedianofThree(VecCand[0].y, VecCand[1].y, VecCand[2].y);	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPred
 ** Description:	Get motion vector prediction in 4mv condition. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Enc_Get8x8MVPred(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred)
{
	ENC_MB_BFR_T *pMBCache = vop_mode_ptr->pMBCache;

	if(pMBCache->bLeftMBAvail && pMBCache->bTopMBAvail && pMBCache->rightAvail)
	{
		Mp4Enc_Get8x8MVPredNotAtBndry(vop_mode_ptr, mb_mode_ptr, blk_num, mvPred, vop_mode_ptr->mb_x);
	}else
	{
		Mp4Enc_Get8x8MVPredAtBndry(vop_mode_ptr, mb_mode_ptr, pMBCache, blk_num, mvPred, vop_mode_ptr->mb_x);
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_Get16x16MVPred
 ** Description:	Get motion vector prediction in one mv condition. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Enc_Get16x16MVPred(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *mvPred, int32 MBPosX)
{
	int32 nInBound = 0;
	MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
	ENC_MB_BFR_T *pMBCache = vop_mode_ptr->pMBCache;
	ENC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;
	
	VecCand[0] = zeroMv;
	VecCand[1] = zeroMv;
	VecCand[2] = zeroMv;
		
	if(pMBCache->bLeftMBAvail)
	{
		pMb_mode_Left = mb_mode_ptr - 1;
		VecCand[0] = pMb_mode_Left->mv[1];
		
		nInBound++;
	}
	
	if(pMBCache->bTopMBAvail)
	{
		pMb_mode_Top = vop_mode_ptr->pMbModeAbv + MBPosX;
		VecCand[nInBound] = pMb_mode_Top->mv[2];
		nInBound++;
	}
	
	if(pMBCache->rightAvail)
	{
		pMb_mode_Right = vop_mode_ptr->pMbModeAbv + MBPosX + 1;
		VecCand[nInBound] = pMb_mode_Right->mv[2];
		
		nInBound++;
	}
	
	if(nInBound == 1)
	{
		((int32 *)mvPred)[0] = ((int32 *)(VecCand))[0];
		return;
	}	
	
	mvPred->x = (int16)Mp4_GetMedianofThree(VecCand[0].x, VecCand[1].x, VecCand[2].x);
	mvPred->y = (int16)Mp4_GetMedianofThree(VecCand[0].y, VecCand[1].y, VecCand[2].y);	
}

uint32 Mp4Enc_EncMVVP(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr)
{
	int32 mb_pos_x = vop_mode_ptr->mb_x;
	int32 mb_pos_y = vop_mode_ptr->mb_y;
	MOTION_VECTOR_T mvPred;
	uint32 nBits = 0;
	MOTION_VECTOR_T vector_diff;
	ENC_MB_BFR_T *pMBCache = vop_mode_ptr->pMBCache;

	pMBCache->bLeftMBAvail = JudegeLeftBndry(mb_pos_x, mb_mode_ptr);
	pMBCache->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, mb_pos_x, mb_pos_y, mb_mode_ptr);
	pMBCache->rightAvail = JudgeRightBndry(vop_mode_ptr, mb_pos_x, mb_pos_y, mb_mode_ptr);

	if(INTER4V == mb_mode_ptr->dctMd)
	{
		int32 blk_num;
		
		/*has 4 MV*/
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			/*get mv predictor*/
			Mp4Enc_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);

			vector_diff.x = mb_mode_ptr->mv[blk_num].x - mvPred.x;
			vector_diff.y = mb_mode_ptr->mv[blk_num].y - mvPred.y;		
			nBits += Mp4Enc_SendDiffMV(vop_mode_ptr, &vector_diff);
			
		#if _TRACE_
//			FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", (pMv + blk_num)->x, (pMv + blk_num)->y);
		#endif //_TRACE_
		}
	}else   /*has one MV*/
	{	
		/*get mv predictor*/			
		Mp4Enc_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, vop_mode_ptr->mb_x);	

		/*get mv difference*/
		vector_diff.x = mb_mode_ptr->mv[0].x - mvPred.x;
		vector_diff.y = mb_mode_ptr->mv[0].y - mvPred.y;		
		nBits = Mp4Enc_SendDiffMV(vop_mode_ptr, &vector_diff);
		
	#if _TRACE_	
//		FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", pMv->x, pMv->y);
	#endif //_TRACE_	
	}	
	
	return nBits;
}

#undef JudegeLeftBndry
#undef JudgeTopBndry
#undef JudgeRightBndry

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
