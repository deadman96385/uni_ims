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
#include "sc8825_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

void Mp4Enc_JudgeMBMode(ENC_MB_MODE_T * pMb_mode)
{
	uint32 mea_result;
	int32 mea_mv;
	MOTION_VECTOR_T *pMv = pMb_mode->mv;
	
	VSP_READ_REG_POLL(VSP_MEA_REG_BASE+MEA_CTRL_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling mea done flag");

	mea_result = VSP_READ_REG(VSP_MEA_REG_BASE+MEA_CTRL_OFF, "read mea ctrl reg for mea_result");
	mea_result = (mea_result >> MEA_RESULT_BIT)&0x03;

	pMb_mode->dctMd = (mea_result == 0)?INTRA:((mea_result == 1)?INTER:INTER4V);

	pMb_mode->bIntra = FALSE;

	if (pMb_mode->dctMd == INTRA)
	{		
	//	PRINTF("Current MB type: intra MB\n");
		pMb_mode->bIntra = TRUE;
		pMv->y = pMv->x = 0;
		
		pMv[3] = pMv[2] = pMv[1] = pMv[0];

		g_rc_par.sad += (2*MB_NB);
	}
	else if (pMb_mode->dctMd == INTER)  //one mv
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

void Mp4Enc_Init_MEA_Fetch(ENC_VOP_MODE_T *pVop_mode)
{
	int fetchX;
	int fetchY;
	MEA_FETCH_REF *pFetch = &g_mea_fetch;

	pFetch->start_X = 0;
	pFetch->start_Y = 0;

	pFetch->leftPixX = pVop_mode->FrameWidth;
	pFetch->leftPixY = pVop_mode->FrameHeight;

	fetchX = ((MB_SIZE + MAX_MV_X + 3) >> 2) << 2;    //word align
	pFetch->fetch_XWidth  = fetchX > pVop_mode->FrameWidth ? pVop_mode->FrameWidth : fetchX;
	
	fetchY = MB_SIZE + MAX_MV_Y;
	pFetch->fetch_YHeigth = fetchY > pVop_mode->FrameHeight ? pVop_mode->FrameHeight : fetchY;
}

void Mp4Enc_MEA_Fetch_cfg (ENC_VOP_MODE_T *pVop_mode)
{
	uint32 transEn, cfg, ref_offset;
	
	transEn = (g_mea_fetch.leftPixX > 0) && (g_mea_fetch.leftPixY > 0);         //if hor and ver has left pixel, transfer enable 
	cfg = (transEn << 31) | (g_mea_fetch.fetch_XWidth/4 << 16) | (g_mea_fetch.fetch_YHeigth); 
	if (transEn == 0)
	{
		transEn = transEn;
	}
	ref_offset = (g_mea_fetch.start_Y<<16 ) | (g_mea_fetch.start_X/4);
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_REF_CFG_OFF, cfg, "configure transfer enable and fetch width and height");
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_REF_OFF, ref_offset, "configure mea ref offset");
#if _CMODEL_

	if (g_mea_reg_ptr->REF_CFG>>31) //fetch enable
	{
		trace_mea_fetch_one_macroblock(&g_mea_fetch);
	}
#endif
}

void Mp4Enc_Update_MEA_Fetch_Y(ENC_VOP_MODE_T * pVop_mode)
{
	int fetchX;
	int y_start_pos = pVop_mode->mb_y_fetch * MB_SIZE;

	int32 start_y = y_start_pos - MAX_MV_Y;
	int32 end_y = y_start_pos + MB_SIZE + MAX_MV_Y;

	if(start_y < 0)
	{
		start_y = 0;
		
	}
	g_mea_fetch.start_Y = start_y;
	
	if(end_y > pVop_mode->FrameHeight)
	{
		end_y = pVop_mode->FrameHeight;
	}
	g_mea_fetch.fetch_YHeigth = end_y - start_y;

	/*reset X vector*/
	g_mea_fetch.start_X = 0;
	g_mea_fetch.leftPixX = pVop_mode->FrameWidth;

	fetchX = ((MB_SIZE + MAX_MV_X + 3) >> 2) << 2;
	g_mea_fetch.fetch_XWidth  = (fetchX > g_mea_fetch.leftPixX) ? g_mea_fetch.leftPixX : fetchX;
}

void Mp4Enc_Update_MEA_Fetch_X (ENC_VOP_MODE_T * pVop_mode)
{
	g_mea_fetch.start_X += g_mea_fetch.fetch_XWidth;
	if (g_mea_fetch.start_X >= pVop_mode->FrameWidth-1) 
		g_mea_fetch.start_X = pVop_mode->FrameWidth-1;

	g_mea_fetch.leftPixX = g_mea_fetch.leftPixX - g_mea_fetch.fetch_XWidth;
	if (g_mea_fetch.leftPixX < 0)
		g_mea_fetch.leftPixX = 0;

	g_mea_fetch.fetch_XWidth = (MB_SIZE > g_mea_fetch.leftPixX) ? g_mea_fetch.leftPixX : MB_SIZE;
}

//find mv prediction for motion estimation.
PUBLIC void Mp4Enc_MVprediction(ENC_VOP_MODE_T *pVop_mode, uint32 mb_pos_x) 
{
	uint32 num_in_Bound = 0;
	BOOLEAN is_left_bndry;
	BOOLEAN is_right_bndry;
	BOOLEAN is_top_bndry;
	uint32 mb_num;	
	uint32 mb_pos_y = pVop_mode->mb_y;
	BOOLEAN is_in_bound[3] = { FALSE, FALSE, FALSE};	
	MOTION_VECTOR_T candidate_mv[3] = {{0,0}, {0,0}, {0,0}};
	uint32 total_mb_num_x = pVop_mode->MBNumX;	
	ENC_MB_MODE_T * top_mbmd_ptr = pVop_mode->pMbModeAbv + mb_pos_x;
	ENC_MB_MODE_T *pMb_mode = pVop_mode->pMbModeCur + mb_pos_x;
	uint32 i;

	mb_num = mb_pos_y * total_mb_num_x + mb_pos_x;

	is_left_bndry	= (mb_pos_x== 0) ? 1 : 0;
	is_top_bndry	= (mb_pos_y == 0) || (pMb_mode->iPacketNumber != top_mbmd_ptr->iPacketNumber);
	is_right_bndry	= is_top_bndry || (mb_pos_x == total_mb_num_x - 1);
	
	if(!is_left_bndry)
	{
		candidate_mv[0].x = (pMb_mode -1)->mv[1].x;
		candidate_mv[0].y = (pMb_mode -1)->mv[1].y;
		
		is_in_bound[0] = TRUE;
		num_in_Bound++;
	}

	if(mb_pos_y != 0)
	{
		if (!is_top_bndry)
		{			
			candidate_mv[1].x = pVop_mode->pMbModeAbv[mb_pos_x].mv[2].x;
			candidate_mv[1].y = pVop_mode->pMbModeAbv[mb_pos_x].mv[2].y;
			is_in_bound[1] = TRUE;
			num_in_Bound++;
		}
		if (!is_right_bndry)
		{			
			candidate_mv[2].x = pVop_mode->pMbModeAbv[mb_pos_x+1].mv[2].x;
			candidate_mv[2].y = pVop_mode->pMbModeAbv[mb_pos_x+1].mv[2].y;
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
				pMb_mode->mvPred.x = candidate_mv[i].x;
				pMb_mode->mvPred.y = candidate_mv[i].y;
				return;
			}
		}
	}

	pMb_mode->mvPred.x = Mp4_GetMedianofThree(candidate_mv[0].x, candidate_mv[1].x, candidate_mv[2].x);
	pMb_mode->mvPred.y = Mp4_GetMedianofThree(candidate_mv[0].y, candidate_mv[1].y, candidate_mv[2].y);
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
