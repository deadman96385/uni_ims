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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

extern int32 Mp4Enc_InitYUVBfr(ENC_VOP_MODE_T *pVop_mode);

PUBLIC void Mp4Enc_VspFrameInit(ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;	
	uint32 frm_offset = 0;
	
	//now, for uv_interleaved
	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "Read out Endian Info") & 0xf;
#if _CMODEL_ //for RTL simulation
	cmd |= (((352*288)>>2)<<4); //word unit	
#else
//	cmd |= (((pVop_mode->pYUVSrcFrame->imgU - pVop_mode->pYUVSrcFrame->imgY) >> 2)<<4); //word unit
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

	configure_huff_tab(g_mp4_enc_huff_tbl, 128);

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

MP4_LOCAL void Mp4Enc_ExchangeMbModePerLine(ENC_VOP_MODE_T *pVop_mode)
{
	ENC_MB_MODE_T *pMb_modeTemp;

	pMb_modeTemp = pVop_mode->pMbModeAbv;
	pVop_mode->pMbModeAbv = pVop_mode->pMbModeCur;
	pVop_mode->pMbModeCur = pMb_modeTemp;
}

PUBLIC int32 Mp4Enc_EncIVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp)
{
	int32 Qp;	
	uint32 mb_pos_x;
	uint32 mb_pos_y;
	ENC_MB_MODE_T *pMb_mode;	
	uint32 total_mb_num_x =  pVop_mode->MBNumX;
	uint32 total_mb_num_y =  pVop_mode->MBNumY;
	BOOLEAN is_short_header = pVop_mode->short_video_header;
	int	 left_mbline_slice = pVop_mode->mbline_num_slice;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();

	if(pVop_mode->RateCtrlEnable)
	{
		Mp4Enc_UpdateIVOP_StepSize(pVop_mode, &g_stat_rc);
	}
		
	pVop_mode->StepSize = Qp = pVop_mode->StepI;

	pVop_mode->MBNumOneVP = 0;

	pVop_mode->mb_x = 0;
	pVop_mode->mb_y = 0;

	Mp4Enc_VspFrameInit(pVop_mode);

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(pVop_mode);
	}else
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}

	for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{	
		pVop_mode->mb_y = mb_pos_y;
		pMb_mode = pVop_mode->pMbModeCur;

		pVop_mode->mb_x = 0;

		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!pVol_mode->bResyncMarkerDisable))
		{
			if(is_short_header)
			{		
				int32 GQUANT = pVop_mode->StepSize;	

				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncGOBHeader(pVop_mode, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_ReSyncHeader(pVop_mode, Qp, time_stamp);
			}
				
			left_mbline_slice = pVop_mode->mbline_num_slice;
			pVop_mode->sliceNumber++;	
		}
			
		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			memset(pMb_mode, 0, sizeof(ENC_MB_MODE_T));
			pMb_mode->bIntra = TRUE;
			pMb_mode->dctMd = INTRA;
			pMb_mode->bSkip = FALSE;
			pMb_mode->StepSize = (int8)Qp;		
			pMb_mode->iPacketNumber = (uint8)pVop_mode->sliceNumber;

		//	if ((mb_pos_x == 0x4) && (mb_pos_y == 7))
		//		PRINTF ("");

		//	FPRINTF(g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, intra MB\n", mb_pos_y, mb_pos_x); 

		//	FPRINTF(g_fp_trace_fw,"encode No. %d MB. start \n",mb_pos_x+mb_pos_y*total_mb_num_x);

			pVop_mode->mb_x = mb_pos_x;

			Mp4Enc_VspMBInit(pVop_mode);

			Mp4Enc_MeaCommand(pVop_mode, &pMb_mode->mvPred, TRUE);
							
			/*dct and quant coonfig*/
			Mp4Enc_ConfigDctQuantMB(pVop_mode, pMb_mode);
				
			/*vlc*/
			Mp4Enc_VlcIntraMB(pMb_mode, pVop_mode);		

			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, 0, pMb_mode->CBP);

			/*iq idct*/
			Mp4Enc_ConfigIqIdctMB(pVop_mode, pMb_mode);

			Mp4Enc_CheckMBCStatus(pVop_mode);

			pMb_mode++;
		}
		left_mbline_slice--;
		Mp4Enc_ExchangeMbModePerLine(pVop_mode);
	}

	/*stuffing to byte align*/
	Mp4Enc_ByteAlign(is_short_header);

	Mp4Enc_Picture_Level_Sync();	
	
	return 1;
}

PUBLIC void Mp4Enc_UpdateRefFrame(ENC_VOP_MODE_T *pVop_mode)
{
	Mp4EncStorablePic *pTmp = PNULL;

	pTmp = pVop_mode->pYUVRefFrame;
	pVop_mode->pYUVRefFrame = pVop_mode->pYUVRecFrame;
	pVop_mode->pYUVRecFrame = pTmp;
}

PUBLIC int32 Mp4Enc_EncPVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp)
{
	int32  Qp = pVop_mode->StepP;	
	uint32 mb_pos_x;
	uint32 mb_pos_y;
	uint32 mb_pos;
	ENC_MB_MODE_T *pMb_mode = NULL;
	uint32 total_mb_num_x =  pVop_mode->MBNumX;
	uint32 total_mb_num_y =  pVop_mode->MBNumY;
	BOOLEAN  is_short_header = pVop_mode->short_video_header;
	int32 left_mbline_slice = pVop_mode->mbline_num_slice;
	int32 left_intramb_dis = pVop_mode->intra_mb_dis;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	
	if(pVop_mode->RateCtrlEnable)
	{
		Mp4Enc_UpdatePVOP_StepSize(pVop_mode, &g_stat_rc, &g_rc_par);
	}
	pVop_mode->StepSize = Qp = pVop_mode->StepP;

	Mp4Enc_Init_MEA_Fetch(pVop_mode);

	memset(pVop_mode->pMbModeCur, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));
	memset(pVop_mode->pMbModeAbv, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));	

	pVop_mode->intra_mb_num = 0;
	pVop_mode->MBNumOneVP = 0;
	Mp4Enc_VspFrameInit(pVop_mode);

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(pVop_mode);
	}else
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}

	for(mb_pos = 0, mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{
		pVop_mode->mb_y = mb_pos_y;
		pMb_mode = pVop_mode->pMbModeCur;

		/*code gob header if need*/
		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!pVol_mode->bResyncMarkerDisable))
		{
			if(is_short_header)
			{		
				int32 GQUANT = pVop_mode->StepSize;	

				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncGOBHeader(pVop_mode, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += (int)Mp4Enc_ReSyncHeader(pVop_mode, Qp, time_stamp);
			}
				
			left_mbline_slice = pVop_mode->mbline_num_slice;
			pVop_mode->sliceNumber++;	
		}

		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			pVop_mode->mb_x = mb_pos_x;

			memset(pMb_mode, 0, sizeof(ENC_MB_MODE_T));
			pMb_mode->StepSize = (int8)Qp;		
			pMb_mode->iPacketNumber = (uint8)(pVop_mode->sliceNumber);

		#if defined(SIM_IN_WIN)
			FPRINTF (g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, inter MB\n", mb_pos_y, mb_pos_x); 
		#endif //#if defined(SIM_IN_WIN)
		
			if((mb_pos_x == 0x1) && (mb_pos_y == 1) && (g_nFrame_enc == 1))
			{
				PRINTF ("");
			}

			Mp4Enc_MVprediction(pVop_mode, pMb_mode);   //compute MV pred for set mv prediction for ME
			
			Mp4Enc_VspMBInit(pVop_mode);
			
			//start mea
			Mp4Enc_MeaCommand(pVop_mode, &pMb_mode->mvPred, FALSE);

			Mp4Enc_JudgeMBMode(pMb_mode);
			
			/*dct and quant config*/
			Mp4Enc_ConfigDctQuantMB(pVop_mode, pMb_mode);

			/*vlc*/
			if(pMb_mode->bIntra)
			{
				pVop_mode->intra_mb_num++;
				Mp4Enc_VlcIntraMB(pMb_mode, pVop_mode);	
				left_intramb_dis = pVop_mode->intra_mb_dis;
			}else{
				Mp4Enc_VlcInterMB(pVop_mode, pMb_mode);
				left_intramb_dis--;
			}	
			
			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, !pMb_mode->bIntra?1:0, pMb_mode->CBP);	
			
		//	printf("No. %d MB, mv (%d, %d)\n", mb_pos_x + mb_pos_y * total_mb_num_x, mot_vec.x, mot_vec.y);

			/*iq idct*/
			Mp4Enc_ConfigIqIdctMB(pVop_mode, pMb_mode);

			Mp4Enc_CheckMBCStatus(pVop_mode);

			pMb_mode++;	
					
			Mp4Enc_Update_MEA_Fetch_X (pVop_mode);

			/***MB synchronization**/
			mb_pos++;
		}

		left_mbline_slice--;
		
		Mp4Enc_Update_MEA_Fetch_Y (pVop_mode);
		
		Mp4Enc_ExchangeMbModePerLine(pVop_mode);
	}

	/*stuffing to byte align*/
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
