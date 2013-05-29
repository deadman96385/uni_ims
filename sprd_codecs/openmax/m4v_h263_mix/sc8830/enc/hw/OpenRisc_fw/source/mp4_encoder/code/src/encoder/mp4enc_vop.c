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
#ifdef SIM_IN_WIN
	uint32 cmd;	
	uint32 frm_offset = 0;

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
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_0, "DBK_VDB_BUF_ST: init ping buffer to be available");

	//mea register configure
	Mp4Enc_init_MEA(vop_mode_ptr);	

	//vlc
	cmd = ((vop_mode_ptr->MBNumX * vop_mode_ptr->MBNumY)  & 0x1ffff);
	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CFG_OFF, cmd, "VLC_CFG_OFF: total mcu number");
#endif
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
	int mea_start = 0;

	if(pVop_mode->RateCtrlEnable)
	{
		Mp4Enc_UpdateIVOP_StepSize(pVop_mode, &g_stat_rc);
	}

	pVop_mode->StepSize = Qp = pVop_mode->StepI;

	pVop_mode->MBNumOneVP = 0;

	pVop_mode->mb_x = 0;
	pVop_mode->mb_y = 0;

	ORSC_Init(IVOP);
	BSM_Init();

	Mp4Enc_VspFrameInit(pVop_mode);

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(pVop_mode);
	}else
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}
	or1200_print = 0;
	mea_start = 1;
	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x18, V_BIT_27, 0x00000000, "ORSC: BSM_DBG0: BSM_clr enable"); //check bsm is idle
	//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x04, 0/*V_BIT_2*/, "ORSC: VSP_INT_MASK: MBW_FMR_DONE"); // enable HW INT
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0xd, "ORSC: VSP_START: ENCODE_START=1");
	//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
	//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x08, V_BIT_2, "ORSC: VSP_INT_CLR: MBW_FMR_DONE"); // clear HW INT
	//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
	//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");


	for(mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{	
		pVop_mode->mb_y = mb_pos_y;
		pVop_mode->mb_x = 0;
#ifdef SIM_IN_WIN	
		pMb_mode = pVop_mode->pMbModeCur;
#endif
				
		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!pVol_mode->bResyncMarkerDisable))
		{
			or1200_print = 1;
#ifdef OUTPUT_TEST_VECTOR
			if(g_vector_enable_flag&VECTOR_ENABLE_FW) 
			{
				fprintf(g_fp_global_tv, "//Frame_no %d, slice_no %d\n", g_nFrame_enc, pVop_mode->sliceNumber);
			}
#endif
			ORSC_Init(IVOP);

			if(is_short_header)
			{		
				int32 GQUANT = pVop_mode->StepSize;	

				/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncGOBHeader(pVop_mode, GQUANT);
			}else
			{
				/*g_rc_par.nbits_hdr_mv += */Mp4Enc_ReSyncHeader(pVop_mode, Qp, time_stamp);
			}
			left_mbline_slice = pVop_mode->mbline_num_slice;
			pVop_mode->sliceNumber++;	
			//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
			//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x18, V_BIT_27, 0x00000000, "ORSC: BSM_DBG0: BSM_clr enable"); //check bsm is idle
			//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x04, 0/*V_BIT_2*/, "ORSC: VSP_INT_MASK: MBW_FMR_DONE"); // enable HW INT
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0x5, "ORSC: VSP_START: ENCODE_START=1");
			//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x08, V_BIT_2, "ORSC: VSP_INT_CLR: MBW_FMR_DONE"); // clear HW INT
			//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");
			or1200_print = 0;
			mea_start = 1;
		}
#ifdef SIM_IN_WIN
		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			memset(pMb_mode, 0, sizeof(ENC_MB_MODE_T));
			pMb_mode->bIntra = TRUE;
			pMb_mode->dctMd = INTRA;
			pMb_mode->bSkip = FALSE;
			pMb_mode->StepSize = (int8)Qp;		
			pMb_mode->iPacketNumber = (uint8)pVop_mode->sliceNumber;
#ifdef OUTPUT_TEST_VECTOR
			VLC_FPRINTF (g_fp_vlc_tv, "//Frame_NO %d, MB_Y: %d, MB_X: %d, intra MB\n", g_nFrame_enc, mb_pos_y, mb_pos_x);
#endif
		//	if ((mb_pos_x == 0x4) && (mb_pos_y == 7))
		//		PRINTF ("");

		//	FPRINTF(g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, intra MB\n", mb_pos_y, mb_pos_x); 

		//	FPRINTF(g_fp_trace_fw,"encode No. %d MB. start \n",mb_pos_x+mb_pos_y*total_mb_num_x);

			pVop_mode->mb_x = mb_pos_x;

			Mp4Enc_VspMBInit(pVop_mode);

			Mp4Enc_MeaCommand(pVop_mode, &pMb_mode->mvPred, TRUE);
							
			/*dct and quant coonfig*/
			Mp4Enc_ConfigDctQuantMB(pVop_mode, pMb_mode);
			if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
			{
				fprintf(g_fp_cbp_in_tv, "//frm_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, pVop_mode->mb_x, pVop_mode->mb_y);
				fprintf(g_fp_cbp_in_tv, "cbp_blk = %x\n", pMb_mode->CBP);
			}
			/*vlc*/
			Mp4Enc_VlcIntraMB(pMb_mode, pVop_mode);		
			
			Mp4Enc_MbcCommand (mb_pos_x, mb_pos_y, 0, pMb_mode->CBP);

			/*iq idct*/
			Mp4Enc_ConfigIqIdctMB(pVop_mode, pMb_mode);

			Mp4Enc_CheckMBCStatus(pVop_mode);
#ifdef OUTPUT_TEST_VECTOR
			PrintfTV(pVop_mode, pMb_mode, IVOP);
#endif
			pMb_mode++;
		}

		left_mbline_slice--;
		Mp4Enc_ExchangeMbModePerLine(pVop_mode);
#if 0
		if (left_mbline_slice == 0)
		{
			/*stuffing to byte align*/
			Mp4Enc_ByteAlign(is_short_header);
		}
#endif
		if(mea_start)
		{
/*
			if (g_fp_bsm_total_bits_tv != NULL)
			{
				fprintf(g_fp_bsm_total_bits_tv, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
			}
			//clear bsm-fifo, polling inactive status reg
			VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
		#if _CMODEL_
			clear_bsm_fifo();
		#endif
			READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");
			
*/
			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_1, V_BIT_1, "ORSC: Polling VSP_INT_RAW: VLC_FRM_DONE "); //check vlc frame done
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x08, V_BIT_2|V_BIT_1, "ORSC: VSP_INT_CLR: MBW_FMR_DONE"); // clear HW INT
/*
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
*/
			mea_start = 0;
		}
#else
		left_mbline_slice--;
		if(mea_start)
		{
			//ÖÐ¶Ï»½ÐÑ
			uint32 tmp = OR1200_READ_REG(GLB_REG_BASE_ADDR+0x0C, "ORSC: Check VSP_INT_RAW");
			while ((tmp&0x34)==0)	// not (MBW_FMR_DONE|VLC_ERR|TIME_OUT)
				tmp = OR1200_READ_REG(GLB_REG_BASE_ADDR+0x0C, "ORSC: Check VSP_INT_RAW");
			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_1, V_BIT_1, "ORSC: Polling VSP_INT_RAW: VLC_FRM_DONE "); //check vlc frame done
			if(tmp&0x30)	// (VLC_ERR|TIME_OUT)
			{
				pVop_mode->error_flag=TRUE;
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+0x08, 0x2c,"ORSC: VSP_INT_CLR: clear BSM_frame error int"); // (MBW_FMR_DONE|PPA_FRM_DONE|TIME_OUT)
			}
			else if((tmp&0x00000004)==0x00000004)	// MBW_FMR_DONE
			{
				pVop_mode->error_flag=FALSE;
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+0x08, 0x6,"ORSC: VSP_INT_CLR: clear MBW_FMR_DONE");
			}
/*
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
*/
			mea_start = 0;
		}
#endif//SIM_IN_WIN

	}
	/*stuffing to byte align*/
	or1200_print = 0;
	Mp4Enc_ByteAlign(is_short_header);
	or1200_print = 1;
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
	int mea_start = 0;
	
	if(pVop_mode->RateCtrlEnable)
	{
		Mp4Enc_UpdatePVOP_StepSize(pVop_mode, &g_stat_rc, &g_rc_par);
	}
	pVop_mode->StepSize = Qp = pVop_mode->StepP;
	
#ifdef SIM_IN_WIN
	Mp4Enc_Init_MEA_Fetch(pVop_mode);

	memset(pVop_mode->pMbModeCur, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));
	memset(pVop_mode->pMbModeAbv, 0, total_mb_num_x*sizeof(ENC_MB_MODE_T));	
#endif

	pVop_mode->MBNumOneVP = 0;
	Mp4Enc_VspFrameInit(pVop_mode);

	pVop_mode->mb_x = 0;
	pVop_mode->mb_y = 0;

	ORSC_Init(PVOP);
	BSM_Init();

	if(is_short_header)
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncH263PicHeader(pVop_mode);
	}else
	{
		g_rc_par.nbits_hdr_mv += Mp4Enc_EncVOPHeader(pVop_mode, time_stamp);	
	}

	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
	//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x18, V_BIT_27, 0x00000000, "ORSC: BSM_DBG0: BSM_clr enable"); //check bsm is idle
	//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x04, 0/*V_BIT_2*/, "ORSC: VSP_INT_MASK: MBW_FMR_DONE"); // enable HW INT
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0xd, "ORSC: VSP_START: ENCODE_START=1");
	//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
	//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x08, V_BIT_2, "ORSC: VSP_INT_CLR: MBW_FMR_DONE"); // clear HW INT
	//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
//	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
//	OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");
	or1200_print = 0;
	mea_start = 1;

	for(mb_pos = 0, mb_pos_y = 0; mb_pos_y < total_mb_num_y; mb_pos_y++)
	{
		pVop_mode->mb_y = mb_pos_y;
		pVop_mode->mb_x = 0;
#ifdef SIM_IN_WIN		
		pMb_mode = pVop_mode->pMbModeCur;
#endif
		/*code gob header if need*/
		if( (mb_pos_y > 0) && (left_mbline_slice == 0) && (!pVol_mode->bResyncMarkerDisable))
		{
			or1200_print = 1;
#ifdef OUTPUT_TEST_VECTOR
			if(g_vector_enable_flag&VECTOR_ENABLE_FW) 
			{
				fprintf(g_fp_global_tv, "//Frame_no %d, slice_no %d\n", g_nFrame_enc, pVop_mode->sliceNumber);
			}
#endif
			ORSC_Init(PVOP);
			if(is_short_header)
			{		
				int32 GQUANT = pVop_mode->StepSize;	

				/*g_rc_par.nbits_hdr_mv += */Mp4Enc_EncGOBHeader(pVop_mode, GQUANT);
			}else
			{
				/*g_rc_par.nbits_hdr_mv += */Mp4Enc_ReSyncHeader(pVop_mode, Qp, time_stamp);
			}
				
			left_mbline_slice = pVop_mode->mbline_num_slice;
			pVop_mode->sliceNumber++;
			or1200_print = 0;
			mea_start = 1;
			//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x20, V_BIT_0, 0x00000001, "ORSC: Polling BSM_RDY");
			//OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR + 0x18, V_BIT_27, 0x00000000, "ORSC: BSM_DBG0: BSM_clr enable"); //check bsm is idle
			//OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x06, "ORSC: BSM_OPERATE: BSM_CLR & COUNT_CLR"); // Move data remain in fifo to external memeory
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x04, 0/*V_BIT_2*/, "ORSC: VSP_INT_MASK: MBW_FMR_DONE"); // enable HW INT
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x28, 0x1, "ORSC: RAM_ACC_SEL: SETTING_RAM_ACC_SEL=1(HW)");
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x30, 0x5, "ORSC: VSP_START: ENCODE_START=1");
			//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x08, V_BIT_2, "ORSC: VSP_INT_CLR: MBW_FMR_DONE"); // clear HW INT
			//OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			//	OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x84, 0x1, "ORSC: CLR_START: Write 1 to clear IMCU_START");
			//OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x88, 0x1, "ORSC: MCU_SLEEP: Set MCU_SLEEP=1");
		}
#ifdef SIM_IN_WIN
		for(mb_pos_x = 0; mb_pos_x < total_mb_num_x; mb_pos_x++)
		{	
			pVop_mode->mb_x = mb_pos_x;

			memset(pMb_mode, 0, sizeof(ENC_MB_MODE_T));
			pMb_mode->StepSize = (int8)Qp;		
			pMb_mode->iPacketNumber = (uint8)(pVop_mode->sliceNumber);

		#ifdef OUTPUT_TEST_VECTOR
			FPRINTF (g_fp_trace_vlc, "MB_Y: %d, MB_X: %d, inter MB\n", mb_pos_y, mb_pos_x); 
			VLC_FPRINTF (g_fp_vlc_tv, "//Frame_NO %d, MB_Y: %d, MB_X: %d, inter MB\n", g_nFrame_enc, mb_pos_y, mb_pos_x);
		#endif //#if defined(SIM_IN_WIN)
		
			if((mb_pos_x == 7) && (mb_pos_y == 0) && (g_nFrame_enc == 7))
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
			if(g_vector_enable_flag&VECTOR_ENABLE_PPA)
			{
				fprintf(g_fp_cbp_in_tv, "//frm_cnt=%x, mb_x=%x, mb_y=%x\n", g_nFrame_enc, pVop_mode->mb_x, pVop_mode->mb_y);
				fprintf(g_fp_cbp_in_tv, "cbp_blk = %x\n", pMb_mode->CBP);
			}

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
#ifdef OUTPUT_TEST_VECTOR
			PrintfTV(pVop_mode, pMb_mode, PVOP);
#endif
			pMb_mode++;	
					
			Mp4Enc_Update_MEA_Fetch_X (pVop_mode);

			/***MB synchronization**/
			mb_pos++;
		}
		
		left_mbline_slice--;
		
		Mp4Enc_Update_MEA_Fetch_Y (pVop_mode);
		
		Mp4Enc_ExchangeMbModePerLine(pVop_mode);
#if 0
		if (left_mbline_slice == 0)
		{
			/*stuffing to byte align*/
			Mp4Enc_ByteAlign(is_short_header);
		}
#endif
		if(mea_start)
		{
/*
			if (g_fp_bsm_total_bits_tv != NULL)
			{
				fprintf(g_fp_bsm_total_bits_tv, "%08x\n", g_bsm_reg_ptr->TOTAL_BITS);
			}
			//clear bsm-fifo, polling inactive status reg
			VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 
		#if _CMODEL_
			clear_bsm_fifo();
		#endif
			READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");
*/
			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_2, V_BIT_2, "ORSC: Polling VSP_INT_RAW: MBW_FMR_DONE"); // check HW INT
			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_1, V_BIT_1, "ORSC: Polling VSP_INT_RAW: VLC_FRM_DONE "); //check vlc frame done
			OR1200_WRITE_REG(GLB_REG_BASE_ADDR + 0x08, V_BIT_2|V_BIT_1, "ORSC: VSP_INT_CLR: MBW_FMR_DONE"); // clear HW INT
/*
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
*/
			mea_start = 0;
		}
#else
		left_mbline_slice--;
		if(mea_start)
		{
			//ÖÐ¶Ï»½ÐÑ
			uint32 tmp = OR1200_READ_REG(GLB_REG_BASE_ADDR+0x0C, "ORSC: Check VSP_INT_RAW");
			while ((tmp&0x34)==0)	// not (MBW_FMR_DONE|VLC_ERR|TIME_OUT)
				tmp = OR1200_READ_REG(GLB_REG_BASE_ADDR + 0x0C, "ORSC: Check VSP_INT_RAW");
			OR1200_READ_REG_POLL(GLB_REG_BASE_ADDR + 0x0C, V_BIT_1, V_BIT_1, "ORSC: Polling VSP_INT_RAW: Polling VLC_FRM_DONE "); //check vlc frame done
			if(tmp&0x30)	// (VLC_ERR|TIME_OUT)
			{
				pVop_mode->error_flag=TRUE;
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+0x08, 0x2c,"ORSC: VSP_INT_CLR: clear BSM_frame error int"); // (MBW_FMR_DONE|PPA_FRM_DONE|TIME_OUT)
			}
			else if((tmp&0x00000004)==0x00000004)	// MBW_FMR_DONE
			{
				pVop_mode->error_flag=FALSE;
				OR1200_WRITE_REG(GLB_REG_BASE_ADDR+0x08, 0x6,"ORSC: VSP_INT_CLR: clear MBW_FMR_DONE");
			}
/*
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_27, 0x00000000, "ORSC: Polling BSM_DBG0: !DATA_TRAN, BSM_clr enable"); //check bsm is idle
			OR1200_WRITE_REG(BSM_CTRL_REG_BASE_ADDR + 0x08, 0x2, "ORSC: BSM_OPERATE: BSM_CLR");
			OR1200_READ_REG_POLL(BSM_CTRL_REG_BASE_ADDR+0x18, V_BIT_31, V_BIT_31, "ORSC: Polling BSM_DBG0: BSM inactive"); //check bsm is idle
*/
			mea_start = 0;
		}
#endif//SIM_IN_WIN
	}
	/*stuffing to byte align*/
	or1200_print = 0;
	Mp4Enc_ByteAlign(is_short_header);
	or1200_print = 1;
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
