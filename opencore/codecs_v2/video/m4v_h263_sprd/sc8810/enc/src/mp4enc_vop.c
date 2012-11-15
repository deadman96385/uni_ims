/******************************************************************************
 ** File Name:    mp4enc_vop.c		                                          *
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

PUBLIC void Mp4Enc_VspFrameInit(ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;	
	uint32 frm_offset = 0;
	
	//now, for uv_interleaved
	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "Read out Endian Info") & 0xf;
#if _CMODEL_ //for RTL simulation
	cmd |= (((352*288)>>2)<<4); //word unit	
#else
	cmd |= (((vop_mode_ptr->pYUVSrcFrame->imgU - vop_mode_ptr->pYUVSrcFrame->imgY) >> 2)<<4); //word unit //tony.liu modified 2010.5.26 
#endif
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");

	if (!g_is_yuv_frm_malloced)
	{
		Mp4Enc_InitYUVBfr(vop_mode_ptr);
		g_is_yuv_frm_malloced = TRUE;
	}

#if _CMODEL_ //for RTL simulation
	frm_offset = (0x25200*g_nFrame_enc)>>8;
#endif

//	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CFG_OFF, MBC_RUN_AUTO_MODE, "MBC_CFG: mpeg4 use auto_mode");
	/*init AHBM, current frame, forward reference frame, backward reference frame*/	
	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 0, vop_mode_ptr->pYUVRecFrame->imgYAddr,"configure reconstruct frame Y");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 4, vop_mode_ptr->pYUVRefFrame->imgYAddr,"configure reference frame Y");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 16, vop_mode_ptr->pYUVSrcFrame->imgYAddr+frm_offset,"configure source frame Y");
	close_vsp_iram();

	cmd = (JPEG_FW_YUV420 << 24) | (vop_mode_ptr->MBNumY << 12) | vop_mode_ptr->MBNumX;
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");

	cmd = (vop_mode_ptr->FrameHeight << 16) | (vop_mode_ptr->FrameWidth);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, cmd, "DCAM_DCAM_SRC_SZIE: configure frame width and height");

	if (vop_mode_ptr->is_need_init_vsp_huff_tab)
	{
		configure_huff_tab(g_mp4_enc_huff_tbl, 128);

		if (vop_mode_ptr->big_size_flag)
		{
			vop_mode_ptr->is_need_init_vsp_huff_tab = FALSE;
		}
	}

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_RAW_OFF, 0, "DCAM_INT_RAW: clear int_raw flag");

	//dbk.
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, (0<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: disable post-filter and free_run_mode");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1, "DBK_CTR1: configure DBK configure finished");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_0, "DBK_VDB_BUF_ST: init ping buffer to be available");

	//mea register configure
	Mp4Enc_init_MEA(vop_mode_ptr);	

	//vlc
	cmd = ((vop_mode_ptr->MBNumX * vop_mode_ptr->MBNumY)  & 0x1ffff);
	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CFG_OFF, cmd, "VLC_CFG_OFF: total mcu number");
}

MP4_LOCAL void Mp4Enc_ExchangeMbModePerLine(ENC_VOP_MODE_T *vop_mode_ptr)
{
	ENC_MB_MODE_T *pMb_modeTemp;

	pMb_modeTemp = vop_mode_ptr->pMbModeAbv;
	vop_mode_ptr->pMbModeAbv = vop_mode_ptr->pMbModeCur;
	vop_mode_ptr->pMbModeCur = pMb_modeTemp;
}

PUBLIC int32 Mp4Enc_EncNVOP(ENC_VOP_MODE_T *vop_mode_ptr, int32 time_stamp)
{
	int32 Qp;	
	BOOLEAN is_short_header = vop_mode_ptr->short_video_header;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();

		
	vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepI;
	vop_mode_ptr->MBNumOneVP = 0;
	vop_mode_ptr->bCoded = 0;

//	Mp4Enc_VspFrameInit(vop_mode_ptr);
#if 0
	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(vop_mode_ptr);
	}else
#endif	
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(vop_mode_ptr, time_stamp);	
	}

	/*stuffing to byte align*/
	Mp4Enc_ByteAlign(is_short_header);
	Mp4Enc_Picture_Level_Sync();	
	
	return 1;

}

PUBLIC int32 Mp4Enc_EncIVOP(ENC_VOP_MODE_T *vop_mode_ptr, int32 time_stamp)
{
	int32 Qp;	
	int32 cmd;
	uint32 mb_pos_x, mb_pos_x_me;
	uint32 mb_pos_y;
	ENC_MB_MODE_T *cur_mb_mode_ptr, *prs_mb_mode_ptr;	
	uint32 total_mb_num_x =  vop_mode_ptr->MBNumX;
	uint32 total_mb_num_y =  vop_mode_ptr->MBNumY;
	BOOLEAN is_short_header = vop_mode_ptr->short_video_header;
	int	 left_mbline_slice = vop_mode_ptr->mbline_num_slice;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();

	if(vop_mode_ptr->RateCtrlEnable)
	{
		Mp4Enc_UpdateIVOP_StepSize(vop_mode_ptr, &g_stat_rc);
	}
		
	vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepI;
	vop_mode_ptr->MBNumOneVP = 0;
	vop_mode_ptr->bCoded = 1;

	Mp4Enc_VspFrameInit(vop_mode_ptr);

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(vop_mode_ptr);
	}else
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(vop_mode_ptr, time_stamp);	
	}

	for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{	
		vop_mode_ptr->mb_y = mb_pos_y;
		memset(vop_mode_ptr->pMbModeCur, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));
		cur_mb_mode_ptr = vop_mode_ptr->pMbModeCur;
	
		vop_mode_ptr->mb_x = 0;

		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!vol_mode_ptr->bResyncMarkerDisable))
		{
			if(is_short_header)
			{		
				int32 GQUANT = vop_mode_ptr->StepSize;	

				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncGOBHeader(vop_mode_ptr, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_ReSyncHeader(vop_mode_ptr, Qp, time_stamp);
			}
				
			left_mbline_slice = vop_mode_ptr->mbline_num_slice;
		}

		//mb 0
	//	memset(cur_mb_mode_ptr, 0, sizeof(ENC_MB_MODE_T));
		cur_mb_mode_ptr->bIntra = TRUE;
		cur_mb_mode_ptr->dctMd = INTRA;
		cur_mb_mode_ptr->bSkip = FALSE;
		cur_mb_mode_ptr->StepSize = (int8)Qp;
		cur_mb_mode_ptr->iPacketNumber = (uint8)vop_mode_ptr->sliceNumber;
		mb_pos_x_me = 0;

		Mp4Enc_VspMBInit(vop_mode_ptr);

		Mp4Enc_MeaCommand(vop_mode_ptr, &cur_mb_mode_ptr->mvPred, TRUE, mb_pos_x_me);
		prs_mb_mode_ptr = cur_mb_mode_ptr;
		cur_mb_mode_ptr++;
		
		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{			
	//		memset(cur_mb_mode_ptr, 0, sizeof(ENC_MB_MODE_T));
			cur_mb_mode_ptr->bIntra = TRUE;
			cur_mb_mode_ptr->dctMd = INTRA;
			cur_mb_mode_ptr->bSkip = FALSE;
			cur_mb_mode_ptr->StepSize = (int8)Qp;		
			cur_mb_mode_ptr->iPacketNumber = (uint8)vop_mode_ptr->sliceNumber;
			mb_pos_x_me = mb_pos_x + 1;

			Mp4Enc_VspMBInit(vop_mode_ptr);
			if (mb_pos_x < (total_mb_num_x -1))
			{
				Mp4Enc_MeaCommand(vop_mode_ptr, &cur_mb_mode_ptr->mvPred, TRUE, mb_pos_x_me);
			}

			//stage 2
			vop_mode_ptr->mb_x = mb_pos_x;
			cmd = (vop_mode_ptr->mb_y << 8) | (vop_mode_ptr->mb_x << 0);
			VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
			
			Mp4Enc_ConfigDctQuantMB(vop_mode_ptr, prs_mb_mode_ptr);
			Mp4Enc_VlcIntraMB(prs_mb_mode_ptr, vop_mode_ptr);
	//		prs_mb_mode_ptr->CBP = 0;
			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, 0, prs_mb_mode_ptr->CBP);
			Mp4Enc_ConfigIqIdctMB(vop_mode_ptr, prs_mb_mode_ptr);
			Mp4Enc_CheckMBCStatus(vop_mode_ptr);

			cur_mb_mode_ptr++;
			prs_mb_mode_ptr++;
		}
		
		left_mbline_slice--;
		Mp4Enc_ExchangeMbModePerLine(vop_mode_ptr);
	}

	/*stuffing to byte align*/
	Mp4Enc_ByteAlign(is_short_header);
	Mp4Enc_Picture_Level_Sync();	
	
	return 1;
}

PUBLIC void Mp4Enc_UpdateRefFrame(ENC_VOP_MODE_T *vop_mode_ptr)
{
	Mp4EncStorablePic *pTmp = PNULL;

	pTmp = vop_mode_ptr->pYUVRefFrame;
	vop_mode_ptr->pYUVRefFrame = vop_mode_ptr->pYUVRecFrame;
	vop_mode_ptr->pYUVRecFrame = pTmp;
}

PUBLIC int32 Mp4Enc_EncPVOP(ENC_VOP_MODE_T *vop_mode_ptr, int32 time_stamp)
{
	int32  Qp = vop_mode_ptr->StepP;	
	int32 cmd;
	uint32 mb_pos_x, mb_pos_x_me;
	uint32 mb_pos_y;
	ENC_MB_MODE_T *cur_mb_mode_ptr, *prs_mb_mode_ptr;	
	uint32 total_mb_num_x =  vop_mode_ptr->MBNumX;
	uint32 total_mb_num_y =  vop_mode_ptr->MBNumY;
	BOOLEAN  is_short_header = vop_mode_ptr->short_video_header;
	int32 left_mbline_slice = vop_mode_ptr->mbline_num_slice;
	int32 left_intramb_dis = vop_mode_ptr->intra_mb_dis;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	
	if(vop_mode_ptr->RateCtrlEnable)
	{
		Mp4Enc_UpdatePVOP_StepSize(vop_mode_ptr, &g_stat_rc, &g_rc_par);
	}
	vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepP;

	Mp4Enc_Init_MEA_Fetch(vop_mode_ptr);

	memset(vop_mode_ptr->pMbModeCur, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));
	memset(vop_mode_ptr->pMbModeAbv, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));	

	vop_mode_ptr->intra_mb_num = 0;
	vop_mode_ptr->MBNumOneVP = 0;
	vop_mode_ptr->bCoded = 1;

	Mp4Enc_VspFrameInit(vop_mode_ptr);

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(vop_mode_ptr);
	}else
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(vop_mode_ptr, time_stamp);	
	}

	for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{
		vop_mode_ptr->mb_y = mb_pos_y;
		cur_mb_mode_ptr = vop_mode_ptr->pMbModeCur;

		/*code gob header if need*/
		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!vol_mode_ptr->bResyncMarkerDisable))
		{
			if(is_short_header)
			{		
				int32 GQUANT = vop_mode_ptr->StepSize;	

				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncGOBHeader(vop_mode_ptr, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_ReSyncHeader(vop_mode_ptr, Qp, time_stamp);
			}
				
			left_mbline_slice = vop_mode_ptr->mbline_num_slice;
		}

		memset(cur_mb_mode_ptr, 0, sizeof(ENC_MB_MODE_T));
		cur_mb_mode_ptr->StepSize = (int8)Qp;		
		cur_mb_mode_ptr->iPacketNumber = (uint8)(vop_mode_ptr->sliceNumber);
		mb_pos_x_me = 0;
		vop_mode_ptr->mb_x = 0;

		Mp4Enc_MVprediction(vop_mode_ptr, cur_mb_mode_ptr, mb_pos_x_me);   //compute MV pred for set mv prediction for ME
			
		Mp4Enc_VspMBInit(vop_mode_ptr);
		Mp4Enc_MeaCommand(vop_mode_ptr, &cur_mb_mode_ptr->mvPred, FALSE, mb_pos_x_me);
		prs_mb_mode_ptr = cur_mb_mode_ptr;
		cur_mb_mode_ptr++;

		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			vop_mode_ptr->mb_x = mb_pos_x;
			mb_pos_x_me = mb_pos_x + 1;

			memset(cur_mb_mode_ptr, 0, sizeof(ENC_MB_MODE_T));
			cur_mb_mode_ptr->StepSize = (int8)Qp;		
			cur_mb_mode_ptr->iPacketNumber = (uint8)(vop_mode_ptr->sliceNumber);

		#if defined(SIM_IN_WIN)
			FPRINTF (g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, inter MB\n", mb_pos_y, mb_pos_x); 
		#endif //#if defined(SIM_IN_WIN)
		#if _DEBUG_
			if((mb_pos_x == 0x1) && (mb_pos_y == 1) && (g_nFrame_enc == 1))
			{
				PRINTF ("");
			}
		#endif	
			
			Mp4Enc_VspMBInit(vop_mode_ptr);
			Mp4Enc_JudgeMBMode(prs_mb_mode_ptr);

			//start MEA
			if (mb_pos_x < total_mb_num_x - 1)
			{
				Mp4Enc_Update_MEA_Fetch_X (vop_mode_ptr);
				Mp4Enc_MVprediction(vop_mode_ptr, cur_mb_mode_ptr, mb_pos_x_me);   //compute MV pred for set mv prediction for ME
				Mp4Enc_MeaCommand(vop_mode_ptr, &cur_mb_mode_ptr->mvPred, FALSE, mb_pos_x_me);
				VSP_READ_REG_POLL(VSP_MEA_REG_BASE+MEA_CTRL_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling mea done flag");
			}

			//stage 2
			cmd  = (mb_pos_y << 8) | (mb_pos_x << 0);
			VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");

			Mp4Enc_ConfigDctQuantMB(vop_mode_ptr, prs_mb_mode_ptr);

			/*vlc*/
			if(prs_mb_mode_ptr->bIntra)
			{
				vop_mode_ptr->intra_mb_num++;
				Mp4Enc_VlcIntraMB(prs_mb_mode_ptr, vop_mode_ptr);	
				left_intramb_dis = vop_mode_ptr->intra_mb_dis;
			}else{
				Mp4Enc_VlcInterMB(vop_mode_ptr, prs_mb_mode_ptr);
				left_intramb_dis--;
			}	
			
			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, !prs_mb_mode_ptr->bIntra?1:0, prs_mb_mode_ptr->CBP);	
			Mp4Enc_ConfigIqIdctMB(vop_mode_ptr, prs_mb_mode_ptr);
			Mp4Enc_CheckMBCStatus(vop_mode_ptr);
			
			
			prs_mb_mode_ptr++;	
			cur_mb_mode_ptr++;	
		}

		left_mbline_slice--;
		Mp4Enc_Update_MEA_Fetch_Y (vop_mode_ptr);
		Mp4Enc_ExchangeMbModePerLine(vop_mode_ptr);
	}

	Mp4Enc_ByteAlign(is_short_header);
	Mp4Enc_Picture_Level_Sync();	
	
	return 1;
}

PUBLIC int32 Mp4Enc_EncPVOP_BIG_SIZE(ENC_VOP_MODE_T *vop_mode_ptr, int32 time_stamp)
{
	int32  Qp;	
	int32 cmd;
	uint32 mb_pos_x, mb_pos_x_me;
	uint32 mb_pos_y;
	ENC_MB_MODE_T *cur_mb_mode_ptr, *prs_mb_mode_ptr;	
	uint32 total_mb_num_x =  vop_mode_ptr->MBNumX;
	uint32 total_mb_num_y =  vop_mode_ptr->MBNumY;
	BOOLEAN  is_short_header = vop_mode_ptr->short_video_header;
	int32 left_mbline_slice = vop_mode_ptr->mbline_num_slice;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	
	if(vop_mode_ptr->RateCtrlEnable)
	{
		Mp4Enc_UpdatePVOP_StepSize(vop_mode_ptr, &g_stat_rc, &g_rc_par);
	}
	vop_mode_ptr->StepSize = Qp = vop_mode_ptr->StepP;

	vop_mode_ptr->intra_mb_num = 0;
	vop_mode_ptr->MBNumOneVP = 0;
	vop_mode_ptr->bCoded = 1;

	Mp4Enc_VspFrameInit(vop_mode_ptr);

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(vop_mode_ptr);
	}else
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(vop_mode_ptr, time_stamp);	
	}

	for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{
		memset(vop_mode_ptr->pMbModeCur, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));
		vop_mode_ptr->mb_y = mb_pos_y;
		cur_mb_mode_ptr = vop_mode_ptr->pMbModeCur;

		/*code gob header if need*/
		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!vol_mode_ptr->bResyncMarkerDisable))
		{
			if(is_short_header)
			{		
				int32 GQUANT = vop_mode_ptr->StepSize;	

				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncGOBHeader(vop_mode_ptr, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_ReSyncHeader(vop_mode_ptr, Qp, time_stamp);
			}
				
			left_mbline_slice = vop_mode_ptr->mbline_num_slice;
		}

	//	memset(cur_mb_mode_ptr, 0, sizeof(ENC_MB_MODE_T));
		cur_mb_mode_ptr->bIntra = TRUE;
		cur_mb_mode_ptr->dctMd = INTRA;
		cur_mb_mode_ptr->StepSize = (int8)Qp;		
		cur_mb_mode_ptr->iPacketNumber = (uint8)(vop_mode_ptr->sliceNumber);
		mb_pos_x_me = 0;

		Mp4Enc_VspMBInit(vop_mode_ptr);

		Mp4Enc_MeaCommand(vop_mode_ptr,  &cur_mb_mode_ptr->mvPred, TRUE, mb_pos_x_me);
		prs_mb_mode_ptr = cur_mb_mode_ptr;
		cur_mb_mode_ptr++;

		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
	//		memset(cur_mb_mode_ptr, 0, sizeof(ENC_MB_MODE_T));
			cur_mb_mode_ptr->bIntra = TRUE;
			cur_mb_mode_ptr->dctMd = INTRA;
			cur_mb_mode_ptr->StepSize = (int8)Qp;		
			cur_mb_mode_ptr->iPacketNumber = (uint8)(vop_mode_ptr->sliceNumber);
			mb_pos_x_me = mb_pos_x + 1;

		#if defined(SIM_IN_WIN)
			FPRINTF (g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, inter MB\n", mb_pos_y, mb_pos_x); 
		#endif //#if defined(SIM_IN_WIN)
				
			Mp4Enc_VspMBInit(vop_mode_ptr);
			if (mb_pos_x < (total_mb_num_x - 1))
			{
				Mp4Enc_MeaCommand(vop_mode_ptr, &cur_mb_mode_ptr->mvPred, TRUE, mb_pos_x_me);
			}

			//stage 2
			vop_mode_ptr->mb_x = mb_pos_x;
			cmd = (vop_mode_ptr->mb_y << 8) | (vop_mode_ptr->mb_x << 0);
			VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
			
			Mp4Enc_ConfigDctQuantMB(vop_mode_ptr, prs_mb_mode_ptr);
			Mp4Enc_VlcIntraMB(prs_mb_mode_ptr, vop_mode_ptr);
			prs_mb_mode_ptr->CBP = 0;
			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, 0, prs_mb_mode_ptr->CBP);	
			Mp4Enc_ConfigIqIdctMB(vop_mode_ptr, prs_mb_mode_ptr);
		//	Mp4Enc_CheckMBCStatus(vop_mode_ptr);
			
			cur_mb_mode_ptr++;	
			prs_mb_mode_ptr++;
		}

		left_mbline_slice--;
		Mp4Enc_ExchangeMbModePerLine(vop_mode_ptr);
	}

	Mp4Enc_ByteAlign(is_short_header);
	Mp4Enc_Picture_Level_Sync();	
	
	return 1;
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
