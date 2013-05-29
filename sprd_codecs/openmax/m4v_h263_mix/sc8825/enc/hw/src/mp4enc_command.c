/******************************************************************************
 ** File Name:    mp4dec_command.c                                            *
 ** Author:       Xiaowei Luo                                                 *
 ** DATE:         11/16/2007                                                  *
 ** Copyright:    2007 Spreatrum, Incoporated. All Rights Reserved.           *
 ** Description:  command  generation and send                                *
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
#include "sc8825_video_header.h"
#include "sys/time.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif
PUBLIC MMEncRet Mp4Enc_VSPInit (ENC_VOP_MODE_T * vop_mode_ptr)
{
	uint32 cmd;

	int vsp_stat = VSP_ACQUIRE_Dev();
	if(vsp_stat)
	{
		VSP_RELEASE_Dev();
		SCI_TRACE_LOW("VSP_ACQUIRE_Dev()  ERR");
		return MMENC_ERROR;
	}
		
//	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x5, "ENDAIN_SEL: 0x5 for little endian system");	

	/*clear time_out int_raw flag, if timeout occurs*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");

	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register");
		
	/*init vsp command*/	
			
	cmd = (1<<17) |(0<<15) |(1<<14)|(1<<13)|(1<<12) | ((!vop_mode_ptr->short_video_header)<<8) | (1<<7) | (0<<6) | 
			(1<<5) | (1<<4) |(0<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: init the global register, little endian");

	cmd = (1 << 31) |(1 << 30) |(TIME_OUT_CLK);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

	cmd =vop_mode_ptr->uv_interleaved ?0:1;//00: uv_interleaved, two plane, 1: three plane
    	cmd = (cmd<<27);
	cmd |= 0x0101;
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
		//if(2 ==vop_mode_ptr->uv_interleaved )//vu_interleaved
	{
		cmd |= ((1<<21)|(1<<18));
		VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, cmd , "configure AXIM register: GAP: 0, frame_mode: UVUV, Little endian");
	}
	return MMENC_OK;
}
void Mp4Enc_MeaCommand(ENC_VOP_MODE_T *pVop_mode, MOTION_VECTOR_T *pmvPred, uint32 bIsIVop, uint32 mb_pos_x)
{
	uint32 cfg, ref_offset;	

// 	uint32 mb_pos_x = pVop_mode->mb_x;
	uint32 mb_pos_y = pVop_mode->mb_y;
	
//following code should be same as C-MODEL.

	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG2_OFF, (mb_pos_y<<16)|(mb_pos_x<<0), "configure current MB position");	
		 
	/*compute reference start point, and copy width and height*/
	cfg = 0;
	ref_offset = 0;
	if (!bIsIVop)
	{
		if (!(pVop_mode->mb_y == (pVop_mode->MBNumY-1) && pVop_mode->mb_x == (pVop_mode->MBNumX-2))) { //czzheng added @20120418
		Mp4Enc_MEA_Fetch_cfg (pVop_mode);

		

		//update 
		Mp4Enc_Update_MEA_Fetch_X (pVop_mode);
		if (pVop_mode->mb_x_fetch == (pVop_mode->MBNumX-1))
		{
			pVop_mode->mb_x_fetch = 0;
			pVop_mode->mb_y_fetch++;
			Mp4Enc_Update_MEA_Fetch_Y (pVop_mode);
		}
		else 
			pVop_mode->mb_x_fetch++;	

		//only for the first mb
		if (pVop_mode->mb_x_fetch == 1 && pVop_mode->mb_y_fetch == 0)
		{
			Mp4Enc_MEA_Fetch_cfg (pVop_mode);

			//update 
			Mp4Enc_Update_MEA_Fetch_X (pVop_mode);
			if (pVop_mode->mb_x_fetch == (pVop_mode->MBNumX-1))
			{
				pVop_mode->mb_x_fetch = 0;
				pVop_mode->mb_y_fetch++;
				Mp4Enc_Update_MEA_Fetch_Y (pVop_mode);
			}
			else
				pVop_mode->mb_x_fetch++;	
		}
		} //czzheng added @20120418
		
		/*configure mv prediction*/
		{
			short mvp_y, mvp_x;
			int mvPredXY;

			mvp_x = ((short)(pmvPred->x & 0xfffe))/2;
			mvp_y = ((short)(pmvPred->y & 0xfffe))/2;
			mvPredXY = ((mvp_y<<9) & (0x7f<<9)) | ((mvp_x<<1) & (0x7f<<1));

			VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG3_OFF, mvPredXY, "configure MV prediction");
		}
	}else
	{
		VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_REF_CFG_OFF, cfg, "configure transfer enable and fetch width and height");
		VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_REF_OFF, ref_offset, "configure mea ref offset");	
	}
		
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CTRL_OFF, 1, "start mea");		/*start mea*/
	
#if _CMODEL_
	mea_module();
#endif

}

void Mp4Enc_MbcCommand (int mb_x, int mb_y, int bInter, int cbp)
{
	if(!bInter)
	{
		cbp = 0x3f;
	}
	
	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD0_OFF, ((bInter) << 30) | (cbp << 0), "MBC_CMD0: configure mb type and cbp");	
}

PUBLIC void Mp4Enc_CheckMBCStatus(ENC_VOP_MODE_T *vop_mode_ptr)
{
#if _CMODEL_
	mbc_module();
#endif //_CMODEL_

	//check the mbc done flag, or time out flag; if time out, error occur then reset the vsp	
	if(VSP_READ_REG_POLL(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, V_BIT_5, TIME_OUT_CLK, "MBC: polling mbc done"))
	{
		vop_mode_ptr->error_flag = TRUE;
		return;
	}
	
	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");
}

/*****************************************************************************
 **	Name : 			Mp4Enc_ConfigDctQuantMB
 ** Description:	config command to do dct and quant of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Enc_ConfigDctQuantMB(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	int32 Qp;	
	uint32 shift_bits;
	uint32 shift_val;
	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;
	const uint8 *dc_scaler_table_y = g_dc_scaler_table_y;
	const uint8 *dc_scaler_table_c = g_dc_scaler_table_c;
	uint32 uDctCfg;
	uint32 uDctQuanPara;
	uint32 cmd;

	cmd  = (pVop_mode->mb_y << 8) | (pVop_mode->mb_x << 0);
	VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
	
	/*configure QUANT PA*/
	Qp = pMb_mode->StepSize;
	shift_bits = pVop_mode->pQuant_pa[Qp<<1]; 
	shift_val = pVop_mode->pQuant_pa[(Qp<<1) + 1]; 
	
	if(pMb_mode->bIntra)
	{
		uint32 Y_inv_shift;
		uint32 Y_shift;
		uint32 UV_inv_shift;
		uint32 UV_shift;

		pMBCache->dc_scaler_y = 8;
		pMBCache->dc_scaler_c = 8;

		/*computer DC scaler for Y or UV*/
		if(!pVop_mode->short_video_header)
		{
			pMBCache->dc_scaler_y = dc_scaler_table_y[Qp];
			pMBCache->dc_scaler_c = dc_scaler_table_c[Qp];
		}
		
		/*write dct config command, use Q_H263-1 and manual-0 mode*/
		uDctCfg = (DCT_QUANT_EN << 8 ) | (DCT_INTRA_MB << 5) |	(1 << 4)| (0 << 1) | (DCT_MODE << 0);
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CONFIG_OFF, uDctCfg, "configure dct config");
		
		/*config quant parameter*/
		uDctQuanPara =	(shift_val<<9) |(Qp<< 4)|(shift_bits<< 0);	
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_QUANT_PARA_OFF, uDctQuanPara, "configure dct quant para");

		Y_inv_shift = pVop_mode->pDC_scaler[pMBCache->dc_scaler_y * 2 + 1];
		Y_shift = pVop_mode->pDC_scaler[pMBCache->dc_scaler_y * 2];
		UV_inv_shift = pVop_mode->pDC_scaler[pMBCache->dc_scaler_c * 2 +1];
		UV_shift = pVop_mode->pDC_scaler[pMBCache->dc_scaler_c * 2];
		/*config quant parameter for DC*/
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_Y_QUAN_PARA_OFF, (Y_inv_shift<<16) | Y_shift, "configure Y quant para");
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_UV_QUAN_PARA_OFF, (UV_inv_shift<<16) | UV_shift, "configure UV quant para");
	}else
	{
		uDctCfg = (DCT_QUANT_EN << 8 )|(DCT_INTER_MB << 5)|(1<<4)|(1<<1)|(DCT_MODE << 0);	
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CONFIG_OFF, uDctCfg, "configure dct config");

		uDctQuanPara = (shift_val<<9)|(Qp<<4)|(shift_bits<<0);
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_QUANT_PARA_OFF, uDctQuanPara, "configure DCT quant para");
	}	

	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_IN_CBP_OFF, 0x3f, "configure DCT input cbp");
	//VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CFG_FINISH_OFF, 1, "configure finish of DCT");   
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_START_OFF, 1, "start DCT");	

#if _CMODEL_
	dct_module();
#endif //_CMODEL_

	//VSP_READ_REG_POLL(VSP_DCT_REG_BASE+DCT_STATUS_OFF, V_BIT_4, V_BIT_4, TIME_OUT_CLK, "polling dct done flag");      //wait DCT done
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_STATUS_OFF, 0x10, "clear DCT done flag");	
	
	pMb_mode->CBP = VSP_READ_REG(VSP_DCT_REG_BASE+DCT_OUT_CBP_OFF, "read CBP from DCT out_CBP");   //read CBP from DCT core 

}

/*****************************************************************************
 **	Name : 			Mp4Enc_ConfigIqIdctMB
 ** Description:	config command to do iq idct of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Enc_ConfigIqIdctMB(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;
	int32 QP= pMb_mode->StepSize;	
	int32 cbp = 0x3F;
	const uint8 *dc_scaler_table_y = g_dc_scaler_table_y;
	const uint8 *dc_scaler_table_c = g_dc_scaler_table_c;
	uint32 uDctCfg, uMpeg4DequanPara;

	if(pMb_mode->bIntra)
	{
		if(pVop_mode->short_video_header)
		{
			pMBCache->dc_scaler_y = 8;
			pMBCache->dc_scaler_c = 8;
		}else
		{
			pMBCache->dc_scaler_y = dc_scaler_table_y[QP];
			pMBCache->dc_scaler_c = dc_scaler_table_c[QP];
		}
	}else
	{
		pMBCache->dc_scaler_y = 1;
		pMBCache->dc_scaler_c = 1;
	}

	if(pMb_mode->bIntra)
	{
		uDctCfg = ((DCT_QUANT_EN << 8 )	| (DCT_QUANT_WIDHOUT_TBL << 6) | (DCT_INTRA_MB << 5)	|  (DCT_AUTO_MODE << 1) |(IDCT_MODE << 0));
		uMpeg4DequanPara = ((QP <<  16) |((pMBCache->dc_scaler_c) << 8)|((pMBCache->dc_scaler_y) << 0));
	}else
	{
		uDctCfg = ((DCT_QUANT_EN << 8 )	| (DCT_QUANT_WIDHOUT_TBL << 6) | (DCT_INTER_MB << 5)	|  (DCT_AUTO_MODE << 1) |(IDCT_MODE << 0));
		uMpeg4DequanPara = (QP <<  16);
	}	

	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CONFIG_OFF, uDctCfg, "IDCT config:quant_en, manual mode, idct");
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_MPEG4_DQUANT_PARA_OFF, uMpeg4DequanPara, "DEQUANT PARA");
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_IN_CBP_OFF, pMb_mode->CBP, "IDCT CBP");
	//VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CFG_FINISH_OFF, 1, "configure finish of IDCT");  
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_START_OFF, 1, "start DCT");	

	//VSP_READ_REG_POLL(VSP_DCT_REG_BASE+DCT_STATUS_OFF, V_BIT_4, V_BIT_4,TIME_OUT_CLK, "polling dct done flag");      //wait DCT done
	//VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_STATUS_OFF, 0x10, "clear IDCT done flag");	  //clear Idct done flag

#if _CMODEL_
	dct_module();
#endif //_CMODEL_
}

void Mp4Enc_ConfigVLC(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	uint32		cmd;
	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;

	cmd =	(pVop_mode->mb_x << 24)			| 
			(pMb_mode->StepSize << 16)		| 
			(pMBCache->bLeftMBAvail << 11) | 
			(pMBCache->bTopMBAvail << 10)	| 
			(pMBCache->bLeftTopAvail << 9)	| 
			((!pMb_mode->bIntra) << 8)		| 
			(pMb_mode->CBP << 0);
	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CTRL_OFF, cmd, "VLC_CTRL_OFF: config vlc");

	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_ST_OFF, 1<<1, "VLC_ST_OFF: start MB vlc");

#if _CMODEL_
	mvlc_ctr ();
#endif

	VSP_READ_REG_POLL(VSP_VLC_REG_BASE+VLC_ST_OFF, V_BIT_31, 0, TIME_OUT_CLK, "VLC_ST_OFF: polling vlc status");
}

void Mp4Enc_VspMBInit(ENC_VOP_MODE_T *pVop_mode, uint32 mb_pos_x)
{
	uint32 cmd;
	ENC_MB_MODE_T *pMb_mode = pVop_mode->pMbModeCur+mb_pos_x;

#if _CMODEL_
	memset(vsp_fw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	memset(vsp_bw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	
	memset(vsp_dct_io_0, 0, 216*sizeof(int32));
	memset(vsp_dct_io_1, 0, 216*sizeof(int32));
#endif //_CMODEL_
	
	//MB synchronization: previous MEA done,// and source, DCT I/O, MBC out buffer are all available 
//	VSP_READ_REG_POLL(VSP_MEA_REG_BASE+MEA_CTRL_OFF, V_BIT_31/*|V_BIT_30|V_BIT_29|V_BIT_28*/, V_BIT_31, TIME_OUT_CLK, "polling mea done and source, DCT i/o and mbc out buffer available");
	
	//cmd  = (pVop_mode->mb_y << 8) | (pVop_mode->mb_x << 0);
	//VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");

	memset(pMb_mode, 0, sizeof(ENC_MB_MODE_T));
	pMb_mode->StepSize = (int8)pVop_mode->StepSize;		
	pMb_mode->iPacketNumber = (uint8)pVop_mode->sliceNumber;
	if (IVOP == pVop_mode->VopPredType)
	{
		pMb_mode->bIntra = TRUE;
		pMb_mode->dctMd = INTRA;
		pMb_mode->bSkip = FALSE;	
	}
}

//////////////////////////////////////////////////////////////////////////
///sync
//////////////////////////////////////////////////////////////////////////
void Mp4Enc_Picture_Level_Sync()
{
	int ret=2;
	int delay_time = 0;
	uint32 nbits_header_mv = 0;
	struct timespec slptm;
	
#ifdef _DEBUG_TIME_
       long long  cur_time;
#endif
	g_HW_CMD_START = 0;

#ifdef _DEBUG_TIME_
	gettimeofday(&tpstart,NULL);
#endif
	VSP_START_CQM();	
#ifdef _DEBUG_TIME_
	gettimeofday(&tpend1,NULL);

	cur_time = tpend1.tv_usec-tpstart.tv_usec;
	if(cur_time < 0)
		cur_time += 1000000;
	SCI_TRACE_LOW("cur frame  Hw  time frameNO %lld",cur_time);
#endif	

	g_rc_par.sad = (VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_MP4ENC_CTRL_OFF, "read total sad")>>1);
	nbits_header_mv = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_MP4ENC_HEADER_STAT_OFF, "read nbits_hdr_mv");
	g_rc_par.nbits_hdr_mv += nbits_header_mv;
	//VSP_READ_REG_POLL(VSP_AXIM_REG_BASE+AXIM_STS_OFF, V_BIT_0, 0, TIME_OUT_CLK, "AXIM_STS: polling AXI idle status");		
       VSP_WRITE_REG(VSP_DCAM_REG_BASE,0,"reset VSP_DCAM_BASE");
//SCI_TRACE_LOW("sad %x nbits_header_mv %x",g_rc_par.sad ,nbits_header_mv);
	VSP_WRITE_REG(VSP_BSM_REG_BASE+BSM_CFG2_OFF, V_BIT_1, "clear bsm-fifo"); 

	VSP_READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling BSMW inactive status");

//       VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_16, "DCAM_INT_CLR: clear MP4 encoding done interrupt flag");
#if 0
	VSP_WRITE_REG (VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF, ((1<<26) |(1<<25)), "'STOP AXIM");
	VSP_READ_REG_POLL(VSP_AXIM_REG_BASE+AXIM_STS_OFF, V_BIT_0, 0, TIME_OUT_CLK, "AXIM_STS: polling AXI idle status");	
	
	VSP_READ_REG(VSP_RESET_ADDR,"read VSP_RESET_ADDR");
    VSP_WRITE_REG(VSP_RESET_ADDR,V_BIT_15,"reset VSP_RESET_ADDR");
	VSP_WRITE_REG(VSP_RESET_ADDR,0,"reset VSP_RESET_ADDR");
#endif
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
