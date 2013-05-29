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
#include "sc8825_video_header.h"
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

/*only for one directional interpolation MB*/
PUBLIC void Mp4Dec_StartMcaOneDir_vt (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, MOTION_VECTOR_T *pMv)
{
	int16 mv_x, mv_y;		
	int16 mvc_x, mvc_y;
	MOTION_VECTOR_T mv;
	int32 pos_x = vop_mode_ptr->mb_x;
	int32 pos_y = vop_mode_ptr->mb_y;	
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 mca_type = mb_cache_ptr->mca_type;
	uint8 *pDstFrameY, *pDstFrameU, *pDstFrameV;
	int32 dst_width;

	//set mv range
	vop_mode_ptr->mv_x_max = ((total_mb_num_x - pos_x) << 5)-1;
	vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
	vop_mode_ptr->mv_y_max = ((vop_mode_ptr->MBNumY - pos_y) << 5)-1; 
	vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;

	//Directly output to memory. 
	pDstFrameY = mb_cache_ptr->mb_addr[0];
	pDstFrameU = mb_cache_ptr->mb_addr[1];
	pDstFrameV = mb_cache_ptr->mb_addr[2];	
	dst_width = vop_mode_ptr->FrameExtendWidth;
	
	/*configure Y block infor*/
	if (mca_type != MCA_BACKWARD_4V)
	{
		mv = pMv[0];
		CLIP_MV(vop_mode_ptr, mv.x, mv.y);
		
		/*compute mv of uv*/		
		mv_x = mv.x;
		mv_y = mv.y;
		mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
		mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];

		Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  pMv, pDstFrameY, dst_width,1);
	}else
	{
		int iBlk;	
		int dmvcx = 0, dmvcy = 0;

		for (iBlk = 4; iBlk > 0; iBlk--)
		{
			mv = pMv[0];
			CLIP_MV(vop_mode_ptr, mv.x, mv.y);		
		
			dmvcx += mv.x;
			dmvcy += mv.y;
			pMv++;
		}

		mvc_x = (dmvcx >> 3) + g_MvRound16[dmvcx & 0xf];
		mvc_y = (dmvcy >> 3) + g_MvRound16[dmvcy & 0xf];
		
		Mp4Dec_motionCompY_fourMV(vop_mode_ptr,  pMv,pDstFrameY,dst_width, 1);
	}

	Mp4Dec_motionCompensationUV(vop_mode_ptr,  mvc_x, mvc_y, pDstFrameU, pDstFrameV, dst_width/2,1);	
}


/*only for one directional interpolation MB*/
PUBLIC void Mp4Dec_StartMcaOneDir_sw (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, MOTION_VECTOR_T *pMv,int16* mvc_x, int16* mvc_y)
{
	int16 mv_x, mv_y;	
	MOTION_VECTOR_T mv;

	int32 mca_type = mb_cache_ptr->mca_type;

	/*configure Y block infor*/
	if (mca_type != MCA_BACKWARD_4V)
	{
		mv = pMv[0];
		CLIP_MV(vop_mode_ptr, mv.x, mv.y);
		
		/*compute mv of uv*/		
		mv_x = mv.x;
		mv_y = mv.y;
		*mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
		*mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];
	}else
	{
		int iBlk;	
		int dmvcx = 0, dmvcy = 0;

		for (iBlk = 4; iBlk > 0; iBlk--)
		{
			mv = pMv[0];
			CLIP_MV(vop_mode_ptr, mv.x, mv.y);		
		
			dmvcx += mv.x;
			dmvcy += mv.y;
			pMv++;
		}

		*mvc_x = (dmvcx >> 3) + g_MvRound16[dmvcx & 0xf];
		*mvc_y = (dmvcy >> 3) + g_MvRound16[dmvcy & 0xf];	
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMV_sw
 ** Description:	Get the motion vector of current macroblock from bitstream, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMV_sw(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
	MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
	MOTION_VECTOR_T Mv_cliped[4];	

	if(mb_mode_ptr->bIntra || mb_mode_ptr->bSkip)
	{
		/*set the MB's vector to 0*/
		((int32 *)pMv)[0] = 0;
		((int32 *)pMv)[1] = 0;
		((int32 *)pMv)[2] = 0;
		((int32 *)pMv)[3] = 0;

		if (mb_mode_ptr->bSkip)
		{
			int32 offset;
			uint8* pRefFrm;
			int32 width = vop_mode_ptr->FrameExtendWidth;

			//y
			offset = mb_cache_ptr->mb_addr[0] - vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[0];
			pRefFrm = vop_mode_ptr->YUVRefFrame0[0] + offset;
			mc_xyfull_16x16(pRefFrm, mb_cache_ptr->mb_addr[0], width, width);

			//u and v
			offset = mb_cache_ptr->mb_addr[1] - vop_mode_ptr->pCurRecFrame->pDecFrame->imgYUV[1];
			width >>= 1;
			
			pRefFrm = vop_mode_ptr->YUVRefFrame0[1] + offset;
			mc_xyfull_8x8(pRefFrm, mb_cache_ptr->mb_addr[1], width, width);

			pRefFrm = vop_mode_ptr->YUVRefFrame0[2] + offset;
			mc_xyfull_8x8(pRefFrm, mb_cache_ptr->mb_addr[2], width, width);
			
			mb_mode_ptr->dctMd = MODE_NOT_CODED;
		}
	}else
	{
		int32 blk_num;
		MOTION_VECTOR_T mvPred;
		int32 pos_x = vop_mode_ptr->mb_x;
	 	int32 pos_y = vop_mode_ptr->mb_y;	
		int32 total_mb_num_x = vop_mode_ptr->MBNumX;
		int32 forward_fcode = (int32)vop_mode_ptr->mvInfoForward.FCode;
		uint8 *pDstFrameY, *pDstFrameU, *pDstFrameV;
		int32 mv_x, mv_y;
		int32 mvc_x, mvc_y;
		int32 dst_width;

		//set mv range
		vop_mode_ptr->mv_x_max = ((total_mb_num_x - pos_x) << 5)-1;
		vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
		vop_mode_ptr->mv_y_max = ((vop_mode_ptr->MBNumY - pos_y) << 5)-1; 
		vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;

		if (mb_mode_ptr->CBP)	//has resi coeff
		{
			pDstFrameY = mb_cache_ptr->pMBBfrY;
			pDstFrameU = mb_cache_ptr->pMBBfrU;
			pDstFrameV = mb_cache_ptr->pMBBfrV;
			dst_width = MB_SIZE;
		}else
		{
			pDstFrameY = mb_cache_ptr->mb_addr[0];
			pDstFrameU = mb_cache_ptr->mb_addr[1];
			pDstFrameV = mb_cache_ptr->mb_addr[2];		
			dst_width = vop_mode_ptr->FrameExtendWidth;
		}


		mb_cache_ptr->bLeftMBAvail = JudegeLeftBndry(pos_x, mb_mode_ptr);
		mb_cache_ptr->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
		mb_cache_ptr->rightAvail = JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);


		if(INTER4V != mb_mode_ptr->dctMd)/*has one MV*/
		{
			mb_cache_ptr->mca_type = MCA_BACKWARD;
			
			Mp4Dec_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, total_mb_num_x);	/*get mv predictor*/
			Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv, &mvPred, forward_fcode);
			
			((int32 *)pMv)[1] = ((int32 *)pMv)[0];
			((int32 *)pMv)[2] = ((int32 *)pMv)[0];
			((int32 *)pMv)[3] = ((int32 *)pMv)[0];
			
			mv_x = pMv[0].x;
			mv_y = pMv[0].y;
			CLIP_MV(vop_mode_ptr, mv_x, mv_y);
			//Simon.Wang @20120820. Store cliped mvs.
			Mv_cliped[0].x = mv_x;
			Mv_cliped[0].y = mv_y;
			
			/*compute mv of uv*/		
			mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
			mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];

			Mp4Dec_motionCompY_oneMV(vop_mode_ptr,  Mv_cliped, pDstFrameY, dst_width, 1);
		}else   /*has 4 MV*/
		{	
			int32 dmvcx = 0, dmvcy = 0;
			
			mb_cache_ptr->mca_type = MCA_BACKWARD_4V;
			
			for(blk_num = 0; blk_num < 4; blk_num++)
			{
				Mp4Dec_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);/*get mv predictor*/
				Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv + blk_num, &mvPred, forward_fcode);

				mv_x = pMv[blk_num].x;
				mv_y = pMv[blk_num].y;
				CLIP_MV(vop_mode_ptr, mv_x, mv_y);
				dmvcx += mv_x;
				dmvcy += mv_y;
				//Simon.Wang @20120820. Store cliped mvs.
				Mv_cliped[blk_num].x = mv_x;
				Mv_cliped[blk_num].y = mv_y;
			}

			mvc_x = (dmvcx >> 3) + g_MvRound16[dmvcx & 0xf];
			mvc_y = (dmvcy >> 3) + g_MvRound16[dmvcy & 0xf];	
			
			Mp4Dec_motionCompY_fourMV(vop_mode_ptr,  Mv_cliped,pDstFrameY, dst_width, 1);
		}		
		
		Mp4Dec_motionCompensationUV(vop_mode_ptr,  mvc_x, mvc_y, pDstFrameU, pDstFrameV, dst_width/2, 1);
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMV_DP_vt
 ** Description:	Get the motion vector of current macroblock from bitstream, DataPartition. 
 ** Author:			Simon.Wang
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMV_DP_vt(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr)
{
	MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;

	if(mb_mode_ptr->bIntra || mb_mode_ptr->bSkip)
	{
		/*set the MB's vector to 0*/
		((int32 *)pMv)[0] = 0;
		((int32 *)pMv)[1] = 0;
		((int32 *)pMv)[2] = 0;
		((int32 *)pMv)[3] = 0;
		
	}else
	{
		int32 blk_num;
		MOTION_VECTOR_T mvPred;
		int32 pos_x = vop_mode_ptr->mb_x;
	 	int32 pos_y = vop_mode_ptr->mb_y;	
		int32 total_mb_num_x = vop_mode_ptr->MBNumX;
		int32 forward_fcode = (int32)vop_mode_ptr->mvInfoForward.FCode;

		mb_cache_ptr->bLeftMBAvail = JudegeLeftBndry(pos_x, mb_mode_ptr);
		mb_cache_ptr->bTopMBAvail = JudgeTopBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
		mb_cache_ptr->rightAvail = JudgeRightBndry(vop_mode_ptr, pos_x, pos_y, mb_mode_ptr);
	
		if(INTER4V != mb_mode_ptr->dctMd)/*has one MV*/
		{
			mb_cache_ptr->mca_type = MCA_BACKWARD;
			
			Mp4Dec_Get16x16MVPred(vop_mode_ptr, mb_mode_ptr, &mvPred, total_mb_num_x);	/*get mv predictor*/
			Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv, &mvPred, forward_fcode);
			
			((int32 *)pMv)[1] = ((int32 *)pMv)[0];
			((int32 *)pMv)[2] = ((int32 *)pMv)[0];
			((int32 *)pMv)[3] = ((int32 *)pMv)[0];			
			
		}else   /*has 4 MV*/
		{	
			int32 dmvcx = 0, dmvcy = 0;
			
			mb_cache_ptr->mca_type = MCA_BACKWARD_4V;
			
			for(blk_num = 0; blk_num < 4; blk_num++)
			{
				Mp4Dec_Get8x8MVPred(vop_mode_ptr, mb_mode_ptr, blk_num, &mvPred);/*get mv predictor*/
				Mp4Dec_DecodeOneMV(vop_mode_ptr, pMv + blk_num, &mvPred, forward_fcode);
			}
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
