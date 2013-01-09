/******************************************************************************
 ** File Name:    mp4dec_mc.c                                              *
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

/*****************************************************************************
 **	Name : 			Mp4Dec_LookupUVMV
 ** Description:	Get motion vector of u/v component. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
//static const int32 s_MvRound16[16] = { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1 };
//static const int32 s_MvRound4[4] = { 0, 1, 0, 0 };

/*only for one directional interpolation MB*/
PUBLIC void Mp4Dec_StartMcaOneDir (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_BFR_T *mb_cache_ptr, MOTION_VECTOR_T *pMv)
{
	uint32 cmd;
	int32 mvc_x, mvc_y;
	int32 mv_x, mv_y;	
	MOTION_VECTOR_T mv;
	int32 ref_blk_id, ref_blk_size;
	int32 ref_blk_end, ref_bir_blk, ref_cmd_type;
	int32 ref_fw_frame_id;
	int32 mca_type = mb_cache_ptr->mca_type;

	ref_fw_frame_id = (mca_type==MCA_FORWARD) ? 0 : 1;
	ref_blk_end = FALSE;
	ref_cmd_type = 1;//FOR Y
	ref_bir_blk = FALSE;

	/*configure Y block infor*/
	if (mca_type != MCA_BACKWARD_4V)
	{
		ref_blk_id = 0;
		ref_blk_size = MC_BLKSIZE_16x16;
			
		cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) |/* (ref_bw_frame_id << 20)|*/
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");

		mv = pMv[0];
		CLIP_MV(vop_mode_ptr, mv.x, mv.y);
		
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, ((uint32*)(&mv))[0], "MCA_MV: MCA MV CBUF");
		VSP_WRITE_CMD_INFO((VSP_MCA << 29)| (2<<24) |(MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));

		/*compute mv of uv*/		
		mv_x = mv.x;
		mv_y = mv.y;
		mvc_x = (mv_x >> 1) + g_MvRound4[mv_x & 0x3];
		mvc_y = (mv_y >> 1) + g_MvRound4[mv_y & 0x3];
	}	else
	{
		int iBlk;	
		int dmvcx = 0, dmvcy = 0;

		ref_blk_id = 0;
		ref_blk_size = MC_BLKSIZE_8x8;

		for (iBlk = 4; iBlk > 0; iBlk--)
		{
			cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) |/* (ref_bw_frame_id << 20)|*/
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);	
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");
			
			mv = pMv[0];
			CLIP_MV(vop_mode_ptr, mv.x, mv.y);		
			
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, ((uint32*)(&mv))[0], "MCA_MV: MCA MV CBUF");
			VSP_WRITE_CMD_INFO((VSP_MCA << 29)| (2<<24) |(MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));
		
			dmvcx += mv.x;
			dmvcy += mv.y;
			pMv++;
			ref_blk_id++;
		}

		mvc_x = (dmvcx >> 3) + g_MvRound16[dmvcx & 0xf];
		mvc_y = (dmvcy >> 3) + g_MvRound16[dmvcy & 0xf];	
	}
	
	/*configure UV block infor*/	
	ref_blk_end = TRUE;
	ref_cmd_type = 2;
	ref_blk_id = 0;
	ref_blk_size = MC_BLKSIZE_8x8;

	cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | /*(ref_bw_frame_id << 20)|*/
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);				
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");
	
	cmd = ((mvc_y&0xffff) << 16) | (mvc_x & 0xffff);
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF");
	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, (vop_mode_ptr->RoundingControl<<1)|1, "MCA_CFG: start MCA");
	VSP_WRITE_CMD_INFO((VSP_MCA << 29)| (3<<24) | (MCA_CFG_WOFF<<16) |(MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));

#if _CMODEL_&&defined(MPEG4_DEC)
{
	DEC_MB_MODE_T *pCoMb_mode = vop_mode_ptr->pMbMode + vop_mode_ptr->MBNumX*vop_mode_ptr->mb_y+vop_mode_ptr->mb_x;

	//for the inter-mb of pvop or the mb of bvop whose co-mb's dctmd is MODE_NOT_CODED (skip)
	if((PVOP == vop_mode_ptr->VopPredType)||
		((BVOP == vop_mode_ptr->VopPredType)&&(MODE_NOT_CODED == pCoMb_mode->dctMd)))
	{
		mca_module();
	}
}
#endif //_CMODEL_
}

/*****************************************************************************
 **	Name : 			Mp4Dec_MCA_BVOP
 ** Description:	Get the motion vector of current macroblock from bitstream. BVOP
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_MCA_BVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, MOTION_VECTOR_T *pFwdPredMv, 
					   MOTION_VECTOR_T *pBckPredMv, MOTION_VECTOR_T *pCoMv)
{
	uint32 cmd;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdMv, BckMv;
	int32 mbmode = mb_mode_ptr->dctMd;
	MOTION_VECTOR_T mv, zeroMv = {0, 0};
	int32 ref_blk_id, ref_blk_size;
	int32 ref_blk_end, ref_bir_blk, ref_cmd_type;
	int32 ref_bw_frame_id, ref_fw_frame_id;
//	int32 mca_type = mb_cache_ptr->mca_type;
	int32 blk_num;
	int32 dmvcx_fwd,dmvcy_fwd, dmvcx_bck, dmvcy_bck;
	int mvc_x_fwd, mvc_y_fwd, mvc_x_bck, mvc_y_bck;

	ref_bw_frame_id = 1;
	ref_fw_frame_id = 0;
	ref_blk_end = FALSE;
	ref_cmd_type = 1;//FOR Y
	ref_bir_blk = TRUE;

	mv = zeroMv;
	
#if _TRACE_	
	FPRINTF(g_fp_trace_fw, "pfpmv_x: %d, pfpmv_y: %d, pbpmv_x:%d, pbpmv_y: %d\n", pFwdPredMv->x, pFwdPredMv->y, pBckPredMv->x, pBckPredMv->y);
#endif //_TRACE_

	switch(mbmode)
	{
	case MODE_DIRECT:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, &mv, &zeroMv, 1);
		//Noted: do not add "break" here.xiaoweiluo@20081204
	case MODE_DIRECT_NONE_MV: //lint !e825
		dmvcx_fwd = dmvcy_fwd = dmvcx_bck = dmvcy_bck = 0;
		ref_blk_size = MC_BLKSIZE_8x8;
		for(blk_num = 0; blk_num < 4; blk_num++)
		{
			int32 tmp;
			int32 comv_x, comv_y;

			ref_blk_id = blk_num; 

			cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20) |
				(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");

			comv_x = pCoMv[blk_num].x;
			comv_y = pCoMv[blk_num].y;

			FwdMv.x = comv_x * vop_mode_ptr->time_bp/vop_mode_ptr->time_pp + mv.x;
			FwdMv.y = comv_y * vop_mode_ptr->time_bp/vop_mode_ptr->time_pp + mv.y;
			
			tmp = (vop_mode_ptr->time_bp - vop_mode_ptr->time_pp);
			BckMv.x = (mv.x) ? (FwdMv.x - (int16)comv_x) : (int16)(comv_x*tmp/vop_mode_ptr->time_pp);
			BckMv.y = (mv.y) ? (FwdMv.y - (int16)comv_y) : (int16)(comv_y*tmp/vop_mode_ptr->time_pp);

		#if 0
			mb_cache_ptr->fmv[blk_num].x = FwdMv.x;
			mb_cache_ptr->fmv[blk_num].y = FwdMv.y;
			mb_cache_ptr->bmv[blk_num].x = BckMv.x;
			mb_cache_ptr->bmv[blk_num].y = BckMv.y;
		#else
			((int32 *)(&(mb_cache_ptr->fmv[blk_num])))[0] = ((int32 *)(&FwdMv))[0];
			((int32 *)(&(mb_cache_ptr->bmv[blk_num])))[0] = ((int32 *)(&BckMv))[0];
		#endif

			//fwd
			CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);
			cmd = ((uint32*)(&FwdMv))[0];	//cmd = ((FwdMv.y&0xffff) << 16) | (FwdMv.x & 0xffff);
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF, fwd_mv");

			//bck
			CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);
			cmd = ((uint32*)(&BckMv))[0];	//cmd = ((BckMv.y&0xffff) << 16) | (BckMv.x & 0xffff);
			VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF, bck_mv");
			VSP_WRITE_CMD_INFO((VSP_MCA << 29)| (3<<24) |(MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));

			dmvcx_fwd += FwdMv.x;
			dmvcy_fwd += FwdMv.y;
			dmvcx_bck += BckMv.x;
			dmvcy_bck += BckMv.y;
			
		#if _TRACE_
			FPRINTF(g_fp_trace_fw, "fmv_x: %d, fmv_y: %d, bmv_x:%d, bmv_y: %d\n", FwdMv.x, FwdMv.y, BckMv.x, BckMv.y);
		#endif //_TRACE_	
		}

		/*configure UV block infor*/	
		ref_blk_end = TRUE;
		ref_cmd_type = 2;
		ref_blk_id = 0;
		ref_blk_size = MC_BLKSIZE_8x8;
		cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");

		//uv mv_fwd
		mvc_x_fwd = (dmvcx_fwd >> 3) + g_MvRound16[dmvcx_fwd & 0xf];
		mvc_y_fwd = (dmvcy_fwd >> 3) + g_MvRound16[dmvcy_fwd & 0xf];		
		cmd = ((mvc_y_fwd&0xffff) << 16) | (mvc_x_fwd & 0xffff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF, fwd_mv");

		//uv mv_bck
		mvc_x_bck = (dmvcx_bck >> 3) + g_MvRound16[dmvcx_bck & 0xf];
		mvc_y_bck = (dmvcy_bck >> 3) + g_MvRound16[dmvcy_bck & 0xf];		
		cmd = ((mvc_y_bck&0xffff) << 16) | (mvc_x_bck & 0xffff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF, bck_mv");

		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, (0<<1)|1, "MCA_CFG: start MCA");

		VSP_WRITE_CMD_INFO((VSP_MCA << 29)| (4<<24) |(MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));
		VSP_WRITE_CMD_INFO(MCA_CFG_WOFF);

		break;
	case MODE_INTERPOLATE:

		ref_blk_id = 0;
		ref_blk_size = MC_BLKSIZE_16x16;

		cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");
		
	//fwd
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->fmv, pFwdPredMv, vop_mode_ptr->mvInfoForward.FCode);

		((int32 *)pFwdPredMv)[0] = ((int32 *)mb_cache_ptr->fmv)[0];

		FwdMv = mb_cache_ptr->fmv[0];
		CLIP_MV(vop_mode_ptr, FwdMv.x, FwdMv.y);
			
		cmd = ((uint32*)(&FwdMv))[0];	//((FwdMv.y&0xffff) << 16) | (FwdMv.x & 0xffff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF");

	//bck
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->bmv, pBckPredMv, vop_mode_ptr->mvInfoBckward.FCode);
		((int32 *)pBckPredMv)[0] = ((int32 *)mb_cache_ptr->bmv)[0];

		BckMv = mb_cache_ptr->bmv[0];
		CLIP_MV(vop_mode_ptr, BckMv.x, BckMv.y);
			
		cmd = ((uint32*)(&BckMv))[0];	//((BckMv.y&0xffff) << 16) | (BckMv.x & 0xffff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF");

		/*configure UV block infor*/	
		ref_blk_end = TRUE;
		ref_cmd_type = 2;
		ref_blk_id = 0;
		ref_blk_size = MC_BLKSIZE_8x8;
		cmd = (ref_blk_end << 27) | (ref_bir_blk << 26) | (ref_cmd_type << 24) | (ref_bw_frame_id << 20)|
			(ref_fw_frame_id << 16) | (ref_blk_id << 8) | (ref_blk_size & 0xff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_BLK_CBUF_OFF, cmd, "MCA_BLK_IN: MCA BLK CBUF");

	/*compute mv of uv*/		
		mvc_x_fwd = (FwdMv.x >> 1) + g_MvRound4[FwdMv.x & 0x3];
		mvc_y_fwd = (FwdMv.y >> 1) + g_MvRound4[FwdMv.y & 0x3];
		cmd = ((mvc_y_fwd&0xffff) << 16) | (mvc_x_fwd & 0xffff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF, fwd_mv");

		mvc_x_bck = (BckMv.x >> 1) + g_MvRound4[BckMv.x & 0x3];
		mvc_y_bck = (BckMv.y >> 1) + g_MvRound4[BckMv.y & 0x3];
		cmd = ((mvc_y_bck&0xffff) << 16) | (mvc_x_bck & 0xffff);
		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_MV_CBUF_OFF, cmd, "MCA_MV: MCA MV CBUF, fwd_mv");

		VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, (0<<1)|1, "MCA_CFG: start MCA");
		
		VSP_WRITE_CMD_INFO((VSP_MCA << 29)| (7<<24) |(MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));
		VSP_WRITE_CMD_INFO((MCA_CFG_WOFF <<24) | (MCA_MV_CBUF_WOFF<<16) | (MCA_MV_CBUF_WOFF<<8) | (MCA_BLK_CBUF_WOFF));

	#if _TRACE_	
		FPRINTF(g_fp_trace_fw, "fmv_x: %d, fmv_y: %d, bmv_x:%d, bmv_y: %d\n", mb_cache_ptr->fmv->x, mb_cache_ptr->fmv->y, mb_cache_ptr->bmv->x, mb_cache_ptr->bmv->y);
	#endif //_TRACE_
		break;
	case MODE_BACKWARD:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->bmv, pBckPredMv, vop_mode_ptr->mvInfoBckward.FCode);
		((int32 *)pBckPredMv)[0] = ((int32 *)mb_cache_ptr->bmv)[0];

		Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_cache_ptr->bmv);
	#if _TRACE_
		FPRINTF(g_fp_trace_fw, "bmv_x: %d, bmv_y: %d\n", mb_cache_ptr->bmv->x, mb_cache_ptr->bmv->y);
	#endif //_TRACE_
		break;
	case MODE_FORWARD:
		Mp4Dec_DecodeOneMV(vop_mode_ptr, mb_cache_ptr->fmv, pFwdPredMv, vop_mode_ptr->mvInfoForward.FCode);
		((int32 *)pFwdPredMv)[0] = ((int32 *)mb_cache_ptr->fmv)[0];
		
		Mp4Dec_StartMcaOneDir (vop_mode_ptr, mb_cache_ptr, mb_cache_ptr->fmv);
	#if _TRACE_	
		FPRINTF(g_fp_trace_fw, "fmv_x: %d, fmv_y: %d\n", mb_cache_ptr->fmv->x, mb_cache_ptr->fmv->y);
	#endif //_TRACE_	
		break;
	default:
		break;
	}

#if _CMODEL_&&defined(MPEG4_DEC)
	mca_module();
#endif //_CMODEL_
}

/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
