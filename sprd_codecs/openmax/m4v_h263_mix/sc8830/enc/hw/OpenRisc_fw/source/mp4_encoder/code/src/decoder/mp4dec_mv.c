/******************************************************************************
 ** File Name:    mp4dec_mv.c                                              *
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
/*----------------------------------------------------------------------------*
**                            Mcaro Definitions                               *
**---------------------------------------------------------------------------*/
#define JudegeLeftBndry(pos_x, mb_mode_ptr) \
	((pos_x == 0) || ((mb_mode_ptr-1)->videopacket_num != mb_mode_ptr->videopacket_num)) ? FALSE : TRUE ;
#define JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr)  \
	((pos_y == 0) || ((mb_mode_ptr - vop_mode_ptr->MBNumX)->videopacket_num != mb_mode_ptr->videopacket_num)) ? FALSE : TRUE;
#define JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr)  \
	((pos_x == vop_mode_ptr->MBNumX - 1) || (pos_y == 0) || ((mb_mode_ptr - vop_mode_ptr->MBNumX + 1)->videopacket_num != mb_mode_ptr->videopacket_num)) ? FALSE : TRUE;

LOCAL void Mp4Dec_Get16x16MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX);
LOCAL void Mp4Dec_Get8x8MVPredAtBndry(DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX);
LOCAL void Mp4Dec_Get8x8MVPredNotAtBndry(DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX);
LOCAL void Mp4Dec_Get8x8MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred);
LOCAL void Mp4Dec_DeScaleMVD(int32 f_code, int32 residual, int32 vlc_code_mag, int32 pred_vector, int16 *pVector);
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_Get16x16MVPred
 ** Description:	Get motion vector prediction in one mv condition. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_Get16x16MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX)
{
	int32 nInBound = 0;
	MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	DEC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;
	
	VecCand[0] = zeroMv;
	VecCand[1] = zeroMv;
	VecCand[2] = zeroMv;
		
	if(mb_cache_ptr->bLeftMBAvail)
	{
		pMb_mode_Left = mb_mode_ptr - 1;
		VecCand[0] = pMb_mode_Left->mv[1];
		
		nInBound++;
	}
	
	if(mb_cache_ptr->bTopMBAvail)
	{
		pMb_mode_Top = mb_mode_ptr - TotalMbNumX;
		VecCand[nInBound] = pMb_mode_Top->mv[2];
		nInBound++;
	}
	
	if(mb_cache_ptr->rightAvail)
	{
		pMb_mode_Right = mb_mode_ptr - TotalMbNumX + 1;
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

/*****************************************************************************
 **	Name : 			Mp4Dec_Get8x8MVPredAtBndry
 ** Description:	Get motion vector prediction in 4mv condition when at boundary. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_Get8x8MVPredAtBndry(DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX)
{
	int32 nInBound = 0;
	MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
	DEC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

	VecCand[0] = zeroMv;
	VecCand[1] = zeroMv;
	VecCand[2] = zeroMv;

	pMb_mode_Left = mb_mode_ptr - 1;
	pMb_mode_Top = mb_mode_ptr - TotalMbNumX;
	pMb_mode_Right = pMb_mode_Top + 1;

	switch(blk_num)
	{
	case 0:
		if(mb_cache_ptr->bLeftMBAvail)
		{
			((int32 *)VecCand)[0] = ((int32 *)(pMb_mode_Left->mv))[1];
			nInBound++;			
		}

		if(mb_cache_ptr->bTopMBAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Top->mv))[2];
			nInBound++;
		}

		if(mb_cache_ptr->rightAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Right->mv))[2];
			nInBound++;
		}  
		break;

	case 1:
		((int32 *)VecCand)[0] = ((int32 *)(mb_mode_ptr->mv))[0];
		nInBound++;

		if(mb_cache_ptr->bTopMBAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Top->mv))[3];
			nInBound++;
		}

		if(mb_cache_ptr->rightAvail)
		{
			((int32 *)VecCand)[nInBound] = ((int32 *)(pMb_mode_Right->mv))[2];
			nInBound++;
		}		
		
		break;

	case 2:
		if(mb_cache_ptr->bLeftMBAvail)
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
LOCAL void Mp4Dec_Get8x8MVPredNotAtBndry(DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred, int32 TotalMbNumX)
{
	MOTION_VECTOR_T VecCand[3], zeroMv = {0, 0};
	DEC_MB_MODE_T *pMb_mode_Left, *pMb_mode_Top, *pMb_mode_Right;
	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();
	
	VecCand[0] = zeroMv;
	VecCand[1] = zeroMv;
	VecCand[2] = zeroMv;

	pMb_mode_Left = mb_mode_ptr - 1;
	pMb_mode_Top = mb_mode_ptr - TotalMbNumX;
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
LOCAL void Mp4Dec_Get8x8MVPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 blk_num, MOTION_VECTOR_T *mvPred)
{
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

	if(mb_cache_ptr->bLeftMBAvail && mb_cache_ptr->bTopMBAvail && mb_cache_ptr->rightAvail)
	{
		Mp4Dec_Get8x8MVPredNotAtBndry(mb_mode_ptr, blk_num, mvPred, vop_mode_ptr->MBNumX);
	}else
	{
		Mp4Dec_Get8x8MVPredAtBndry(mb_mode_ptr, mb_cache_ptr, blk_num, mvPred, vop_mode_ptr->MBNumX);
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DeScaleMVD
 ** Description:	DeScale motion vector. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_DeScaleMVD(int32 f_code, int32 residual, int32 vlc_code_mag, 	
					int32 pred_vector, int16 *pVector)
{
	int32 r_size = f_code-1;
	int32 scale_factor = 1<<r_size;
	int32 range = 32*scale_factor;
	int32 low   = -range;
	int32 high  =  range-1;
	int32 diff_vector;
	
	if(scale_factor==1 || vlc_code_mag==0)
	{
		diff_vector = vlc_code_mag;
	}else
    {
		diff_vector = ((ABS(vlc_code_mag)-1)<<r_size) + residual + 1;
		if (vlc_code_mag < 0)
		{
			diff_vector = -diff_vector;
		}
    }
	
	*pVector = (int16)(pred_vector + diff_vector);
	
	if(*pVector < low)
	{
		*pVector += (int16)(2*range);
	}else if (*pVector > high)
	{
		*pVector -= (int16)(2*range);
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecodeOneMV
 ** Description:	Get one motion vector from bitstream. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecodeOneMV(DEC_VOP_MODE_T *vop_mode_ptr, MOTION_VECTOR_T *pMv, MOTION_VECTOR_T *pPredMv, int32 fcode)
{
	int32 mvData;
	int32 mvResidual;

	/*mv x*/
	mvData = Mp4Dec_VlcDecMV(vop_mode_ptr);

	if(vop_mode_ptr->error_flag)
	{
		return;
	}

	if(fcode > 1 && mvData != 0)
	{
		mvResidual = (int32)Mp4Dec_ReadBits((uint32)(fcode-1));//, "mv residual"
	}else
	{
		mvResidual = 0;
	}

	Mp4Dec_DeScaleMVD(fcode, mvResidual, mvData, pPredMv->x, &(pMv->x));

    /*mv y*/
	mvData = Mp4Dec_VlcDecMV(vop_mode_ptr);
	
	if(vop_mode_ptr->error_flag)
	{
		return;
	}
	
	if(fcode > 1 && mvData != 0)
	{
		mvResidual = (int32)Mp4Dec_ReadBits((uint32)(fcode-1));//, "mv residual"
	}else
	{
		mvResidual = 0;
	}
	
	Mp4Dec_DeScaleMVD(fcode, mvResidual, mvData, pPredMv->y, &(pMv->y));

//	PRINTF("No.%d MB: motion vector:(%d,%d)\n",vop_mode_ptr->mb_x+vop_mode_ptr->mb_y*vop_mode_ptr->MBNumX,pMv->x, pMv->y);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMV
 ** Description:	Get the motion vector of current macroblock from bitstream, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMV(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 blk_num;
	MOTION_VECTOR_T mvPred;
	int32 pos_x = vop_mode_ptr->mb_x;
 	int32 pos_y = vop_mode_ptr->mb_y;	
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	MOTION_VECTOR_T zeroMv = {0, 0};
	MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
	int32 forward_fcode = (int32)vop_mode_ptr->mvInfoForward.FCode;

	if(mb_mode_ptr->bIntra || mb_mode_ptr->bSkip)
	{
		/*set the MB's vector to 0*/
		pMv[0] = zeroMv;
		pMv[1] = zeroMv;
		pMv[2] = zeroMv;
		pMv[3] = zeroMv;

		return;
	}

	mb_cache_ptr->bLeftMBAvail = JudegeLeftBndry(pos_x, mb_mode_ptr);
	mb_cache_ptr->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
	mb_cache_ptr->rightAvail = JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
	
#if _TRACE_	
	FPRINTF (g_fp_trace_fw, "mb_x: %d, mb_y: %d\n", pos_x, pos_y);
#endif //_TRACE_

	if(INTER4V == mb_mode_ptr->dctMd)
	{
		/*has 4 MV*/
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			/*get mv predictor*/
			Mp4Dec_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);

			Mp4Dec_DecodeOneMV (vop_mode_ptr, pMv + blk_num, &mvPred, forward_fcode);	
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF ("decode four mv error!\n");
				return;
			}	
		#if _TRACE_
			FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", (pMv + blk_num)->x, (pMv + blk_num)->y);
		#endif //_TRACE_
		}
	}else   /*has one MV*/
	{	
		/*get mv predictor*/			
		Mp4Dec_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, total_mb_num_x);			

		/*get mv difference*/
		Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv, &mvPred, forward_fcode);

		if (vop_mode_ptr->error_flag)
		{
			PRINTF("decode one mv error!\n");
			return;
		}

		pMv[1] = pMv[0];
		pMv[2] = pMv[0];
		pMv[3] = pMv[0];
		
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", pMv->x, pMv->y);
	#endif //_TRACE_	
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
