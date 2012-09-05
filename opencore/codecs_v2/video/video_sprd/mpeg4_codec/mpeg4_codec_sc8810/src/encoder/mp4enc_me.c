/******************************************************************************
 ** File Name:    mp4enc_me.c                                                 *
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

PUBLIC void Mp4Enc_JudgeMBMode(ENC_MB_MODE_T * mb_mode_ptr)
{
	uint32 mea_result;
	int32 mea_mv;
	MOTION_VECTOR_T *pMv = mb_mode_ptr->mv;
	
	mea_result = VSP_READ_REG(VSP_MEA_REG_BASE+MEA_CTRL_OFF, "read mea ctrl reg for mea_result");
	mea_result = (mea_result >> MEA_RESULT_BIT)&0x03;

	mb_mode_ptr->dctMd = (mea_result == 0)?INTRA:((mea_result == 1)?INTER:INTER4V);

	mb_mode_ptr->bIntra = FALSE;

	if (mb_mode_ptr->dctMd == INTRA)
	{		
	//	PRINTF("Current MB type: intra MB\n");
		mb_mode_ptr->bIntra = TRUE;
		pMv->y = pMv->x = 0;
		
		pMv[3] = pMv[2] = pMv[1] = pMv[0];

		g_rc_par.sad += (10*MB_NB);
	}
	else if (mb_mode_ptr->dctMd == INTER)  //one mv
	{
	//	PRINTF("Current MB type: inter MB, one mv\n");

		mea_mv = (int32)VSP_READ_REG(VSP_MEA_REG_BASE+MEA_MV_OFF, "read sad and mv of inter MB 16x16 from mea");

		pMv->y = (int16)((mea_mv << 16) >> 24);
		pMv->x = (int16)((mea_mv << 24) >> 24);

		pMv[3] = pMv[2] = pMv[1] = pMv[0];

		g_rc_par.sad += ((mea_mv>>16)&0xFFFF);
	}
	else //four mv
	{
	//	PRINTF("Current MB type: inter MB, four mv\n");

		mea_mv = (int32)VSP_READ_REG(VSP_MEA_REG_BASE+MEA_MV0_OFF, "read 0th block's sad and mv of interMB8x8 from mea");
		pMv[0].y = (int16)((mea_mv << 16) >> 24);
		pMv[0].x = (int16)((mea_mv << 24) >> 24);
		g_rc_par.sad += ((mea_mv>>16)&0xFFFF);

		mea_mv = (int32)VSP_READ_REG(VSP_MEA_REG_BASE+MEA_MV1_OFF, "read 1th block's sad and mv of interMB8x8 from mea");
		pMv[1].y = (int16)((mea_mv << 16) >> 24);
		pMv[1].x = (int16)((mea_mv << 24) >> 24);
		g_rc_par.sad += ((mea_mv>>16)&0xFFFF);

		mea_mv = (int32)VSP_READ_REG(VSP_MEA_REG_BASE+MEA_MV2_OFF, "read 2th block's sad and mv of interMB8x8 from mea");
		pMv[2].y = (int16)((mea_mv << 16) >> 24);
		pMv[2].x = (int16)((mea_mv << 24) >> 24);
		g_rc_par.sad += ((mea_mv>>16)&0xFFFF);

		mea_mv = (int32)VSP_READ_REG(VSP_MEA_REG_BASE+MEA_MV3_OFF, "read 3th block's sad and mv of interMB8x8 from mea");
		pMv[3].y = (int16)((mea_mv << 16) >> 24);
		pMv[3].x = (int16)((mea_mv << 24) >> 24);

		g_rc_par.sad += ((mea_mv>>16)&0xFFFF);
	}
}

PUBLIC void Mp4Enc_Init_MEA_Fetch(ENC_VOP_MODE_T *vop_mode_ptr)
{
	int fetchX;
	int fetchY;
	MEA_FETCH_REF *pFetch = &g_mea_fetch;

	pFetch->start_X = 0;
	pFetch->start_Y = 0;

	pFetch->leftPixX = vop_mode_ptr->FrameWidth;
	pFetch->leftPixY = vop_mode_ptr->FrameHeight;

	fetchX = ((MB_SIZE + MAX_MV_X + 3) >> 2) << 2;    //word align
	pFetch->fetch_XWidth  = fetchX > vop_mode_ptr->FrameWidth ? vop_mode_ptr->FrameWidth : fetchX;
	
	fetchY = MB_SIZE + MAX_MV_Y;
	pFetch->fetch_YHeigth = fetchY > vop_mode_ptr->FrameHeight ? vop_mode_ptr->FrameHeight : fetchY;
}

PUBLIC void Mp4Enc_Update_MEA_Fetch_Y(ENC_VOP_MODE_T * vop_mode_ptr)
{
	int fetchX;
	int y_start_pos = (vop_mode_ptr->mb_y + 1) * MB_SIZE;

	int32 start_y = y_start_pos - MAX_MV_Y;
	int32 end_y = y_start_pos + MB_SIZE + MAX_MV_Y;

	if(start_y < 0)
	{
		start_y = 0;
		
	}
	g_mea_fetch.start_Y = start_y;
	
	if(end_y > vop_mode_ptr->FrameHeight)
	{
		end_y = vop_mode_ptr->FrameHeight;
	}
	g_mea_fetch.fetch_YHeigth = end_y - start_y;

	/*reset X vector*/
	g_mea_fetch.start_X = 0;
	g_mea_fetch.leftPixX = vop_mode_ptr->FrameWidth;

	fetchX = ((MB_SIZE + MAX_MV_X + 3) >> 2) << 2;
	g_mea_fetch.fetch_XWidth  = (fetchX > g_mea_fetch.leftPixX) ? g_mea_fetch.leftPixX : fetchX;
}

PUBLIC void Mp4Enc_Update_MEA_Fetch_X (ENC_VOP_MODE_T * vop_mode_ptr)
{
	MEA_FETCH_REF *mea_fetch_ptr = &g_mea_fetch;

	mea_fetch_ptr->start_X += mea_fetch_ptr->fetch_XWidth;
	if (mea_fetch_ptr->start_X >= vop_mode_ptr->FrameWidth-1) 
	{
		mea_fetch_ptr->start_X = vop_mode_ptr->FrameWidth-1;
	}

	mea_fetch_ptr->leftPixX = mea_fetch_ptr->leftPixX - mea_fetch_ptr->fetch_XWidth;
	if (mea_fetch_ptr->leftPixX < 0)
	{
		mea_fetch_ptr->leftPixX = 0;
	}

	mea_fetch_ptr->fetch_XWidth = (MB_SIZE > mea_fetch_ptr->leftPixX) ? mea_fetch_ptr->leftPixX : MB_SIZE;
}

PUBLIC void Mp4Enc_init_MEA(ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 uCfg;
	int	   four_mv_cost;
	uint32 meEna = FALSE; 
	uint32 prefilterEn = FALSE;
	uint32 predEn = FALSE;
	uint32 fourMVEn = FALSE;
	uint32 intraEn = FALSE;
	uint32 bIsIVop = (vop_mode_ptr->VopPredType == IVOP)?1:0;

#if defined(PREFILTER_EN)
	prefilterEn = 1;
#endif

#if defined(PRED_EN)
	predEn = 1;
#endif

#if defined(_4MV_ENABLE)
	fourMVEn = 1;
#endif

#if defined(INTRA_EN)
	intraEn = 1;
#endif

	if(vop_mode_ptr->short_video_header) //must!
	{
		fourMVEn = 0;
		uCfg = (prefilterEn<<24)|	(8<<16)|(MAX_MV_Y_H263<<8)|(MAX_MV_X_H263<<0);
	}else
	{
		uCfg = (prefilterEn<<24)|	(8<<16)|(MAX_MV_Y<<8)|(MAX_MV_X<<0);
	}

	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG0_OFF, uCfg, "pre-filter, configure Y and X search range");   //Y and X search range

	meEna = (bIsIVop || (vop_mode_ptr->big_size_flag)) ? FALSE : TRUE; 
	uCfg = (MB_SAD_THRESHOLD<<16)|		//sad threshold for stopping search
		(MAX_SEARCH_CYCLE<<8)	|		//max search steps
		(0<<5)					|		//hardware pipeline
		(predEn<<4)				|		//prediction enable, me from predicted MV
		(fourMVEn<<2)			|		///4 mv enable, but encoder not support 4 MV, just verificate hardware
		(intraEn<<1)			|		//intra sad disable, 
		(meEna<<0);						//motion estimation enable
		
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG1_OFF, uCfg, "MEA_CFG1_OFF: disable hardware pipeline");

#if 0
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG4_OFF, ((MB_NB/2+1)<<16)|(MB_NB/2+1), "configure increased and reduced sad");
#else
	four_mv_cost = 22*g_lambda[vop_mode_ptr->StepSize];
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG4_OFF, (four_mv_cost<<16)|(MB_NB/2+1), "configure increased and reduced sad");
#endif

	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG5_OFF, (2*MB_NB), "intra sad increased value");

#if _CMODEL_
	init_mea();
#endif
}

//find mv prediction for motion estimation.
PUBLIC void Mp4Enc_MVprediction(ENC_VOP_MODE_T *vop_mode_ptr, ENC_MB_MODE_T *mb_mode_ptr, uint32 mb_pos_x) 
{
	uint32 num_in_Bound = 0;
	BOOLEAN is_left_bndry;
	BOOLEAN is_right_bndry;
	BOOLEAN is_top_bndry;
	uint32 mb_num;	
//	uint32 mb_pos_x = vop_mode_ptr->mb_x;
	uint32 mb_pos_y = vop_mode_ptr->mb_y;
	BOOLEAN is_in_bound[3] = { FALSE, FALSE, FALSE};	
	MOTION_VECTOR_T candidate_mv[3] = {{0,0}, {0,0}, {0,0}};
	uint32 total_mb_num_x = vop_mode_ptr->MBNumX;
	uint32 i;
	ENC_MB_MODE_T * top_mbmd_ptr = vop_mode_ptr->pMbModeAbv + mb_pos_x;

	mb_num = mb_pos_y * total_mb_num_x + mb_pos_x;

	is_left_bndry	= (mb_pos_x== 0) ? 1 : 0;
	is_top_bndry	= (mb_pos_y == 0) || (mb_mode_ptr->iPacketNumber != top_mbmd_ptr->iPacketNumber);
	is_right_bndry	= is_top_bndry || (mb_pos_x == total_mb_num_x - 1);
	
	if(!is_left_bndry)
	{
		candidate_mv[0].x = (mb_mode_ptr -1)->mv[0].x;
		candidate_mv[0].y = (mb_mode_ptr -1)->mv[0].y;
		
		is_in_bound[0] = TRUE;
		num_in_Bound++;
	}

	if(mb_pos_y != 0)
	{
		if (!is_top_bndry)
		{			
			candidate_mv[1].x = vop_mode_ptr->pMbModeAbv[mb_pos_x].mv[0].x;
			candidate_mv[1].y = vop_mode_ptr->pMbModeAbv[mb_pos_x].mv[0].y;
			is_in_bound[1] = TRUE;
			num_in_Bound++;
		}
		if (!is_right_bndry)
		{			
			candidate_mv[2].x = vop_mode_ptr->pMbModeAbv[mb_pos_x+1].mv[0].x;
			candidate_mv[2].y = vop_mode_ptr->pMbModeAbv[mb_pos_x+1].mv[0].y;
			is_in_bound[2] = TRUE;
			num_in_Bound++;
		}
	}
		
	if(num_in_Bound == 1)
	{
		for (i = 0; i < 3; i++)
		{
			if (is_in_bound [i] == TRUE)
			{
				mb_mode_ptr->mvPred = candidate_mv[i];

				if (vop_mode_ptr->short_video_header)
				{
					int32 ref_pos_x = (((int32)mb_pos_x<<5) + (mb_mode_ptr->mvPred.x));
					int32 ref_pos_y = (((int32)mb_pos_y<<5) + (mb_mode_ptr->mvPred.y));

					/* checking in half-pixel precise */
					//modify mv
					if ((ref_pos_x < 0) || (ref_pos_x > ((vop_mode_ptr->FrameWidth  - MB_SIZE)*2)))
					{
						mb_mode_ptr->mvPred.x = 0;
					}

					if ((ref_pos_y < 0) || (ref_pos_y > ((vop_mode_ptr->FrameHeight - MB_SIZE)*2)))
					{
						mb_mode_ptr->mvPred.y = 0;
					}
				}

				return;
			}
		}
	}

	mb_mode_ptr->mvPred.x = Mp4_GetMedianofThree(candidate_mv[0].x, candidate_mv[1].x, candidate_mv[2].x);
	mb_mode_ptr->mvPred.y = Mp4_GetMedianofThree(candidate_mv[0].y, candidate_mv[1].y, candidate_mv[2].y);

	if (vop_mode_ptr->short_video_header)
	{
		int32 ref_pos_x = (((int32)mb_pos_x<<5) + (mb_mode_ptr->mvPred.x));
		int32 ref_pos_y = (((int32)mb_pos_y<<5) + (mb_mode_ptr->mvPred.y));

		/* checking in half-pixel precise */
		//modify mv
		if ((ref_pos_x < 0) || (ref_pos_x > ((vop_mode_ptr->FrameWidth  - MB_SIZE)*2)))
		{
			mb_mode_ptr->mvPred.x = 0;
		}

		if ((ref_pos_y < 0) || (ref_pos_y > ((vop_mode_ptr->FrameHeight - MB_SIZE)*2)))
		{
			mb_mode_ptr->mvPred.y = 0;
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
