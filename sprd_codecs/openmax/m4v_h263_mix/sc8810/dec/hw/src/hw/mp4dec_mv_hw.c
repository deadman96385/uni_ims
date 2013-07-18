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
/*lint -save -e744 -e767*/
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
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_Get16x16MVPred
 ** Description:	Get motion vector prediction in one mv condition. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMV_hw
 ** Description:	Get the motion vector of current macroblock from bitstream, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMV_hw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 blk_num;
	MOTION_VECTOR_T mvPred;
	int32 pos_x = vop_mode_ptr->mb_x;
 	int32 pos_y = vop_mode_ptr->mb_y;	
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
	int32 forward_fcode = (int32)vop_mode_ptr->mvInfoForward.FCode;

	if(mb_mode_ptr->bIntra || mb_mode_ptr->bSkip)
	{
		/*set the MB's vector to 0*/
		((int32 *)pMv)[0] = 0;
		((int32 *)pMv)[1] = 0;
		((int32 *)pMv)[2] = 0;
		((int32 *)pMv)[3] = 0;

		return;
	}

	if(INTER4V != mb_mode_ptr->dctMd)/*has one MV*/
	{
		/*get mv difference*/
		Mp4Dec_DecodeOneMVD(vop_mode_ptr, pMv, forward_fcode);

		if (vop_mode_ptr->error_flag)
		{
			PRINTF("decode one mv error!\n");
			vop_mode_ptr->return_pos2 |= (1<<12);
			return;
		}

		((int32 *)pMv)[1] = ((int32 *)pMv)[0];
		((int32 *)pMv)[2] = ((int32 *)pMv)[0];
		((int32 *)pMv)[3] = ((int32 *)pMv)[0];
		
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", pMv->x, pMv->y);
	#endif //_TRACE_		
	}else   
	{	
		/*has 4 MV*/
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			Mp4Dec_DecodeOneMVD (vop_mode_ptr, pMv + blk_num, forward_fcode);	
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF ("decode four mv error!\n");
				vop_mode_ptr->return_pos2 |= (1<<13);
				return;
			}	
		}
	}	

	mb_cache_ptr->bLeftMBAvail = JudegeLeftBndry(pos_x, mb_mode_ptr);
	mb_cache_ptr->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
	mb_cache_ptr->rightAvail = JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
	
#if _TRACE_	
	FPRINTF (g_fp_trace_fw, "mb_x: %d, mb_y: %d\n", pos_x, pos_y);
#endif //_TRACE_

	if(INTER4V != mb_mode_ptr->dctMd)
	{
		mb_cache_ptr->mca_type = MCA_BACKWARD;
		
		/*get mv predictor*/			
		Mp4Dec_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, total_mb_num_x);			

		Mp4Dec_DeScaleMVD(vop_mode_ptr->long_vectors, forward_fcode, pMv, &mvPred);
		
		((int32 *)pMv)[1] = ((int32 *)pMv)[0];
		((int32 *)pMv)[2] = ((int32 *)pMv)[0];
		((int32 *)pMv)[3] = ((int32 *)pMv)[0];
		
	#if _TRACE_	
		FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", pMv->x, pMv->y);
	#endif //_TRACE_
	}else   /*has one MV*/
	{
		mb_cache_ptr->mca_type = MCA_BACKWARD_4V;
		
		/*has 4 MV*/
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			/*get mv predictor*/
			Mp4Dec_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);

			Mp4Dec_DeScaleMVD(vop_mode_ptr->long_vectors, forward_fcode, pMv + blk_num, &mvPred);
		
		#if _TRACE_
			FPRINTF (g_fp_trace_fw, "\tmv_x: %d, mv_y: %d\n", (pMv + blk_num)->x, (pMv + blk_num)->y);
		#endif //_TRACE_
		}
	}	
	
	//set mv range
	vop_mode_ptr->mv_x_max = ((total_mb_num_x - pos_x) << 5)-1;
	vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
	vop_mode_ptr->mv_y_max = ((vop_mode_ptr->MBNumY - pos_y) << 5)-1; 
	vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
