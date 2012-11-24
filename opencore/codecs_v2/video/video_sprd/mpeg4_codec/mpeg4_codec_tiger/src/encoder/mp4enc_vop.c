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
 ** 06/26/2012   Leon Li             modify
 *****************************************************************************/
/*----------------------------------------------------------------------------*
**                        Dependencies                                        *
**---------------------------------------------------------------------------*/
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

void Mp4Enc_init_MEA(ENC_VOP_MODE_T *pVop_mode)
{
	uint32 uCfg;
	uint32 meEna = FALSE; 
	uint32 prefilterEn = FALSE;
	uint32 predEn = FALSE;
	uint32 fourMVEn = FALSE;
	uint32 intraEn = FALSE;
	uint32 bIsIVop = (pVop_mode->VopPredType == IVOP)?1:0;

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

	if(pVop_mode->short_video_header) //must!
	{
		fourMVEn = 0;
		uCfg = (prefilterEn<<24)|	(8<<16)|(MAX_MV_Y_H263<<8)|(MAX_MV_X_H263<<0);
	}else
	{
		uCfg = (prefilterEn<<24)|	(8<<16)|(MAX_MV_Y<<8)|(MAX_MV_X<<0);
	}

	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG0_OFF, uCfg, "pre-filter, configure Y and X search range");   //Y and X search range

	meEna = bIsIVop ? FALSE : TRUE; 
	uCfg = (MB_SAD_THRESHOLD<<16)|		//sad threshold for stopping search
		(MAX_SEARCH_CYCLE<<8)	|		//max search steps
		(0<<5)					|		//hardware pipeline
		(predEn<<4)				|		//prediction enable, me from predicted MV
		(fourMVEn<<2)			|		///4 mv enable, but encoder not support 4 MV, just verificate hardware
		(intraEn<<1)			|		//intra sad disable, 
		(meEna<<0);						//motion estimation enable
		
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG1_OFF, uCfg, "MEA_CFG1_OFF: disable hardware pipeline");

	
	//mea register configure
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG4_OFF, ((MB_NB/2+1)<<16)|(MB_NB/2+1), "configure increased and reduced sad");
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG5_OFF, (2*MB_NB), "intra sad increased value");

	
#if _CMODEL_
//	init_glb_ctrl();
#endif 

}

PUBLIC void Mp4Enc_VspFrameInit(ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;	
	uint32 frm_offset = 0;

#if _CMODEL_ //for RTL simulation
	//frm_offset = (vop_mode_ptr->FrameWidth*vop_mode_ptr->FrameHeight*3/2*g_nFrame_enc)>>2;//modified by xwluo@20111205, for D1 supported, from 0x25200
	frm_offset = 0; //czzheng modified @20120420
#endif

//	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CFG_OFF, MBC_RUN_AUTO_MODE, "MBC_CFG: mpeg4 use auto_mode");
	/*init AHBM, current frame, forward reference frame, backward reference frame*/	
	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 0, MAPaddr( vop_mode_ptr->pYUVRecFrame->imgYAddr),"configure reconstruct frame Y");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 4, MAPaddr( vop_mode_ptr->pYUVRefFrame->imgYAddr),"configure reference frame Y");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 16, MAPaddr( vop_mode_ptr->pYUVSrcFrame->imgYAddr+frm_offset),"configure source frame Y");
	close_vsp_iram();

	cmd = (JPEG_FW_YUV420 << 24) | (vop_mode_ptr->MBNumY << 12) | vop_mode_ptr->MBNumX;
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");

	cmd = (vop_mode_ptr->FrameHeight << 16) | (vop_mode_ptr->FrameWidth);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, cmd, "DCAM_DCAM_SRC_SZIE: configure frame width and height");

	configure_huff_tab(g_mp4_enc_huff_tbl, 128);

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_RAW_OFF, 0, "DCAM_INT_RAW: clear int_raw flag");

	//vlc
	cmd = ((vop_mode_ptr->MBNumX * vop_mode_ptr->MBNumY)  & 0x1ffff);
	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CFG_OFF, cmd, "VLC_CFG_OFF: total mcu number");

	cmd = (vop_mode_ptr->MB_in_VOP_length<<15) | (vop_mode_ptr->mvInfoForward.FCode<<12) |
		(vop_mode_ptr->VopPredType<<9) | (vop_mode_ptr->mbline_num_slice<<6) | 
		(vop_mode_ptr->short_video_header<<5) | (vop_mode_ptr->StepSize);
	VSP_WRITE_REG(VSP_GLB_CTRL_REG_BASE+MP4ENC_GLB_CTRL_OFFSET, cmd, "MP4 enc global ctrl ");

	//cfg MEA cfg
	Mp4Enc_init_MEA(vop_mode_ptr);

	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (1<<10) |(0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_MP4ENC_CTRL_OFF, 1, "start full hardware encoding");
	g_HW_CMD_START = 1;

	//dbk.
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, (0<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: disable post-filter and free_run_mode");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_0, "DBK_VDB_BUF_ST: init ping buffer to be available");

#if _CMODEL_
	init_mea();
#endif

	////vlc
	//cmd = ((vop_mode_ptr->MBNumX * vop_mode_ptr->MBNumY)  & 0x1ffff);
	//VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CFG_OFF, cmd, "VLC_CFG_OFF: total mcu number");
}

MP4_LOCAL void Mp4Enc_ExchangeMbModePerLine(ENC_VOP_MODE_T *pVop_mode)
{
	ENC_MB_MODE_T *pMb_modeTemp;

	pMb_modeTemp = pVop_mode->pMbModeAbv;
	pVop_mode->pMbModeAbv = pVop_mode->pMbModeCur;
	pVop_mode->pMbModeCur = pMb_modeTemp;
}
PUBLIC int32 Mp4Enc_EncNVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp)
{
	int32 Qp;	
	BOOLEAN is_short_header = pVop_mode->short_video_header;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();

		
	pVop_mode->StepSize = Qp = pVop_mode->StepI;
	pVop_mode->MBNumOneVP = 0;
	pVop_mode->bCoded = 0;

//	Mp4Enc_VspFrameInit(vop_mode_ptr);
#if 0
	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(vop_mode_ptr);
	}else
#endif	
	{
		g_rc_par.nbits_hdr_mv += (int)Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}

	/*stuffing to byte align*/
	Mp4Enc_ByteAlign(is_short_header);
	Mp4Enc_Picture_Level_Sync();	
	
	return 1;

}

PUBLIC int32 Mp4Enc_EncIVOP(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp)
{
	int32 Qp;	
	uint32 mb_pos_x, mb_me_x;
	uint32 mb_pos_y;
	ENC_MB_MODE_T *pMb_mode;	
	uint32 total_mb_num_x =  pVop_mode->MBNumX;
	uint32 total_mb_num_y =  pVop_mode->MBNumY;
	BOOLEAN is_short_header = pVop_mode->short_video_header;
	int	 left_mbline_slice = pVop_mode->mbline_num_slice;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	int32 nbits_hdr_mv = 0;
	
	if(pVop_mode->RateCtrlEnable)
	{
		Mp4Enc_UpdateIVOP_StepSize(pVop_mode, &g_stat_rc);
	}
		
	pVop_mode->StepSize = Qp = pVop_mode->StepI;

	pVop_mode->MBNumOneVP = 0;

	pVop_mode->mb_x = 0;
	pVop_mode->mb_y = 0;

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(pVop_mode);
	}else
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}

	pVop_mode->bCoded = 1;
	Mp4Enc_VspFrameInit(pVop_mode);
#if _CMODEL_ //for RTL simulation

	nbits_hdr_mv = g_rc_par.nbits_hdr_mv;
	{
		FPRINTF(g_pInput_para,"qp: %x, Frame_width:%x,Frame_height:%x, Is_short_header: %x, ", pVop_mode->StepI, pVop_mode->FrameWidth,pVop_mode->FrameHeight, is_short_header);
		FPRINTF(g_pInput_para,"Mbline_num_gob:%x, vopPredType: %x, fcode: %x, Mb_in_vop_length: %x\n", pVop_mode->mbline_num_slice, 0,pVop_mode->mvInfoForward.FCode,pVop_mode->MB_in_VOP_length);
		FFLUSH(g_pInput_para);
	}
	for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{	
		pVop_mode->mb_y = mb_pos_y;
		pMb_mode = pVop_mode->pMbModeCur;

		pVop_mode->mb_x = 0;
		mb_me_x = 0;

		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!pVol_mode->bResyncMarkerDisable))
		{
			int32 bits_cnt = 0;
			if(is_short_header)
			{		
				int32 GQUANT = pVop_mode->StepSize;	

				g_rc_par.nbits_hdr_mv += Mp4Enc_EncGOBHeader(pVop_mode, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += Mp4Enc_ReSyncHeader(pVop_mode, Qp, time_stamp);
			}
				
			left_mbline_slice = pVop_mode->mbline_num_slice;
			bits_cnt = g_rc_par.nbits_hdr_mv - nbits_hdr_mv;
			bits_cnt = bits_cnt;
		}

		//first mb of mb-line
		Mp4Enc_VspMBInit(pVop_mode, mb_me_x);
		Mp4Enc_MeaCommand(pVop_mode, &pMb_mode->mvPred, TRUE, mb_me_x);
		mb_me_x++;
			
		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			if ((mb_pos_x == 41) && (mb_pos_y == 1))
				PRINTF ("");

		//	FPRINTF(g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, intra MB\n", mb_pos_y, mb_pos_x); 

		//	FPRINTF(g_fp_trace_fw,"encode No. %d MB. start \n",mb_pos_x+mb_pos_y*total_mb_num_x);

			pVop_mode->mb_x = mb_pos_x;

			VSP_READ_REG_POLL(VSP_MEA_REG_BASE+MEA_CTRL_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling mea done flag");

			//Mp4Enc_VspMBInit(pVop_mode, mb_me_x);

			if (mb_me_x < total_mb_num_x)
			{
				Mp4Enc_VspMBInit(pVop_mode, mb_me_x);
				Mp4Enc_MeaCommand(pVop_mode, &(pMb_mode+1)->mvPred, TRUE, mb_me_x);
				mb_me_x++;
			}
										
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
    nbits_hdr_mv = g_rc_par.nbits_hdr_mv - nbits_hdr_mv;
	{
		FPRINTF(g_pPureSadFile, "%08x, %08d\n", 0 ,nbits_hdr_mv);
	}	
#endif
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
	uint32 mb_pos_x, mb_me_x;
	uint32 mb_pos_y;
	uint32 mb_pos;
	ENC_MB_MODE_T *pMb_mode = NULL;
	uint32 total_mb_num_x =  pVop_mode->MBNumX;
	uint32 total_mb_num_y =  pVop_mode->MBNumY;
	BOOLEAN  is_short_header = pVop_mode->short_video_header;
	int32 left_mbline_slice = pVop_mode->mbline_num_slice;
	int32 left_intramb_dis = pVop_mode->intra_mb_dis;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	int32 nbits_hdr_mv = 0;
	
	if(pVop_mode->RateCtrlEnable)
	{
		Mp4Enc_UpdatePVOP_StepSize(pVop_mode, &g_stat_rc, &g_rc_par);
	}
	pVop_mode->StepSize = Qp = pVop_mode->StepP;

	memset(pVop_mode->pMbModeCur, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));
	memset(pVop_mode->pMbModeAbv, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));	

	pVop_mode->MBNumOneVP = 0;	

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(pVop_mode);
	}else
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}
	pVop_mode->bCoded = 1;
	Mp4Enc_VspFrameInit(pVop_mode);
#if _CMODEL_ //for RTL simulation
	nbits_hdr_mv = g_rc_par.nbits_hdr_mv;
	Mp4Enc_Init_MEA_Fetch(pVop_mode);

	{
		FPRINTF(g_pInput_para,"qp: %x, Frame_width:%x,Frame_height:%x, Is_short_header: %x, ", pVop_mode->StepP, pVop_mode->FrameWidth,pVop_mode->FrameHeight, is_short_header);
		FPRINTF(g_pInput_para,"Mbline_num_gob:%x, vopPredType: %x, fcode: %x, Mb_in_vop_length: %x\n", pVop_mode->mbline_num_slice, 1,pVop_mode->mvInfoForward.FCode,pVop_mode->MB_in_VOP_length);
	}

	//init
	pVop_mode->mb_x_fetch = 0;
	pVop_mode->mb_y_fetch = 0;
	pVop_mode->mb_x_me = 0;
	pVop_mode->mb_y_me = 0;

	for(mb_pos = 0, mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{
		pVop_mode->mb_y = mb_pos_y;
		pMb_mode = pVop_mode->pMbModeCur;

		pVop_mode->mb_x = 0;
		mb_me_x = 0;

		/*code gob header if need*/
		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!pVol_mode->bResyncMarkerDisable))
		{
			if(is_short_header)
			{		
				int32 GQUANT = pVop_mode->StepSize;	

				g_rc_par.nbits_hdr_mv += Mp4Enc_EncGOBHeader(pVop_mode, GQUANT);
			}else
			{
				g_rc_par.nbits_hdr_mv += Mp4Enc_ReSyncHeader(pVop_mode, Qp, time_stamp);
			}
				
			left_mbline_slice = pVop_mode->mbline_num_slice;
		}
		
		Mp4Enc_VspMBInit(pVop_mode, mb_me_x); 
		Mp4Enc_MVprediction(pVop_mode, mb_me_x);  		
			
		//start mea
		Mp4Enc_MeaCommand(pVop_mode, &pMb_mode->mvPred, FALSE, mb_me_x);

		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			pVop_mode->mb_x = mb_pos_x;

		#if defined(SIM_IN_WIN)
			FPRINTF (g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, inter MB\n", mb_pos_y, mb_pos_x); 
		#endif //#if defined(SIM_IN_WIN)
		
			if((mb_pos_x == 9) && (mb_pos_y == 8) && (g_nFrame_enc == 1))
			{
				PRINTF ("");
			}

			Mp4Enc_JudgeMBMode(pVop_mode->pMbModeCur+mb_me_x);
			mb_me_x++;

			//Mp4Enc_VspMBInit(pVop_mode, mb_me_x);

			if (mb_me_x < total_mb_num_x)
			{
				Mp4Enc_VspMBInit(pVop_mode, mb_me_x);
				Mp4Enc_MVprediction(pVop_mode, mb_me_x);   //compute MV pred for set mv prediction for ME			
				
				//start mea
				Mp4Enc_MeaCommand(pVop_mode, &(pMb_mode+1)->mvPred, FALSE, mb_me_x);
			//	Mp4Enc_JudgeMBMode(pVop_mode->pMbModeCur+mb_me_x);
			//	mb_me_x++;
			}
						
			/*dct and quant config*/
			Mp4Enc_ConfigDctQuantMB(pVop_mode, pMb_mode);

			/*vlc*/
			if(pMb_mode->bIntra)
			{
				Mp4Enc_VlcIntraMB(pMb_mode, pVop_mode);	
				left_intramb_dis = pVop_mode->intra_mb_dis;
			}else{
				Mp4Enc_VlcInterMB(pVop_mode, pMb_mode);
				left_intramb_dis--;
			}	
			
			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, !pMb_mode->bIntra, pMb_mode->CBP);	
			
		//	printf("No. %d MB, mv (%d, %d)\n", mb_pos_x + mb_pos_y * total_mb_num_x, mot_vec.x, mot_vec.y);

			/*iq idct*/
			Mp4Enc_ConfigIqIdctMB(pVop_mode, pMb_mode);

			Mp4Enc_CheckMBCStatus(pVop_mode);

			pMb_mode++;	
					

			/***MB synchronization**/
			mb_pos++;
		}

		left_mbline_slice--;
		
		
		Mp4Enc_ExchangeMbModePerLine(pVop_mode);
	}

	/*stuffing to byte align*/
	Mp4Enc_ByteAlign(is_short_header);

	nbits_hdr_mv = g_rc_par.nbits_hdr_mv - nbits_hdr_mv;
	{
		FPRINTF(g_pPureSadFile, "%08x, %08d\n", g_rc_par.sad ,nbits_hdr_mv);
	}	

#endif
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
