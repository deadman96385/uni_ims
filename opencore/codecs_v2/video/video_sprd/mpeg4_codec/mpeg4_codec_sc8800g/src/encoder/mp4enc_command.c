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
		
void Mp4Enc_MeaCommand(ENC_VOP_MODE_T *pVop_mode, MOTION_VECTOR_T *pmvPred, uint32 bIsIVop)
{
	uint32 cfg, ref_offset;	

	uint32 mb_pos_x = pVop_mode->mb_x;
	uint32 mb_pos_y = pVop_mode->mb_y;
	
//following code should be same as C-MODEL.

	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG2_OFF, (mb_pos_y<<16)|(mb_pos_x<<0), "configure current MB position");	
		 
	/*compute reference start point, and copy width and height*/
	cfg = 0;
	ref_offset = 0;
	if (!bIsIVop)
	{
		int transEn;
		
		transEn = (g_mea_fetch.leftPixX > 0) && (g_mea_fetch.leftPixY > 0);         //if hor and ver has left pixel, transfer enable 
		cfg = (transEn << 31) | ((g_mea_fetch.fetch_XWidth/4) << 16) | (g_mea_fetch.fetch_YHeigth); 

		ref_offset = (g_mea_fetch.start_Y<<16 ) | (g_mea_fetch.start_X/4);

		/*configure mv prediction*/
		{
			short mvp_y, mvp_x;
			int mvPredXY;

			mvp_x = ((short)(pmvPred->x & 0xfffe))/2;
			mvp_y = ((short)(pmvPred->y & 0xfffe))/2;
			mvPredXY = ((mvp_y<<9) & (0x7f<<9)) | ((mvp_x<<1) & (0x7f<<1));

			VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CFG3_OFF, mvPredXY, "configure MV prediction");
		}
	}
		
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_REF_CFG_OFF, cfg, "configure transfer enable and fetch width and height");
	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_REF_OFF, ref_offset, "configure mea ref offset");
	
	//modify the standard from h.263 to flash to avoid mv point to out of frame, @2010.5.21	
	if (pVop_mode->short_video_header)
	{			
		cfg = VSP_READ_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, "read gobal register");
		
		cfg = (cfg & 0xfffff8ff) | (FLV_H263 << 8);
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cfg, "GLB_CFG0: init the global register");
	}

	VSP_WRITE_REG(VSP_MEA_REG_BASE+MEA_CTRL_OFF, 1, "start mea");		/*start mea*/
	
#if _CMODEL_
	mea_module();
#endif

	READ_REG_POLL(VSP_MEA_REG_BASE+MEA_CTRL_OFF, V_BIT_31, V_BIT_31, TIME_OUT_CLK, "polling mea done flag");

	//modify the standard back from flash to h.263, @2010.5.21	
	if (pVop_mode->short_video_header)
	{			
		cfg = VSP_READ_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, "read gobal register");
		
		cfg = (cfg & 0xfffff8ff) | (ITU_H263 << 8);
		VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cfg, "GLB_CFG0: init the global register");
	}
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
	if(READ_REG_POLL(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, V_BIT_5, TIME_OUT_CLK, "MBC: polling mbc done"))
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
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CFG_FINISH_OFF, 1, "configure finish of DCT");   
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_START_OFF, 1, "start DCT");	

#if _CMODEL_
	dct_module();
#endif //_CMODEL_

	READ_REG_POLL(VSP_DCT_REG_BASE+DCT_STATUS_OFF, V_BIT_4, V_BIT_4, TIME_OUT_CLK, "polling dct done flag");      //wait DCT done
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
//	int32 cbp = 0x3F;
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
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CFG_FINISH_OFF, 1, "configure finish of IDCT");  
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_START_OFF, 1, "start DCT");	

	READ_REG_POLL(VSP_DCT_REG_BASE+DCT_STATUS_OFF, V_BIT_4, V_BIT_4,TIME_OUT_CLK, "polling dct done flag");      //wait DCT done
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_STATUS_OFF, 0x10, "clear IDCT done flag");	  //clear Idct done flag

#if _CMODEL_
	dct_module();
#endif //_CMODEL_
}

void Mp4Enc_ConfigVLC(ENC_VOP_MODE_T *pVop_mode, ENC_MB_MODE_T *pMb_mode)
{
	uint32		cmd;
	ENC_MB_BFR_T *pMBCache = pVop_mode->pMBCache;

	cmd =	( pVop_mode->mb_x << 24 )           | 
			( pMb_mode->StepSize << 16 )        | 
			( pMBCache->bLeftMBAvail << 11 )    | 
			( pMBCache->bTopMBAvail << 10 )	    | 
			( pMBCache->bLeftTopAvail << 9 );
    cmd |=  ( (!pMb_mode->bIntra) ? (1<<8) : 0 );
	cmd |=  ( pMb_mode->CBP << 0);
	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_CTRL_OFF, cmd, "VLC_CTRL_OFF: config vlc");

	VSP_WRITE_REG(VSP_VLC_REG_BASE+VLC_ST_OFF, 1<<1, "VLC_ST_OFF: start MB vlc");

#if _CMODEL_
	mvlc_ctr ();
#endif

	READ_REG_POLL(VSP_VLC_REG_BASE+VLC_ST_OFF, V_BIT_31, 0, TIME_OUT_CLK, "VLC_ST_OFF: polling vlc status");
}

void Mp4Enc_VspMBInit(ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;

#if _CMODEL_
	memset(vsp_fw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	memset(vsp_bw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	memset(vsp_mea_out_bfr, 0, MEA_OUT_BFR_SIZE*sizeof(uint32));	
	
	memset(vsp_dct_io_0, 0, 216*sizeof(int32));
	memset(vsp_dct_io_1, 0, 216*sizeof(int32));
#endif //_CMODEL_
	
	//MB synchronization: previous MEA done, and source, DCT I/O, MBC out buffer are all available 
	READ_REG_POLL(VSP_MEA_REG_BASE+MEA_CTRL_OFF, V_BIT_31|V_BIT_30|V_BIT_29|V_BIT_28, V_BIT_31, TIME_OUT_CLK, "polling mea done and source, DCT i/o and mbc out buffer available");
	
	cmd  = (vop_mode_ptr->mb_y << 8) | (vop_mode_ptr->mb_x << 0);
	VSP_WRITE_REG (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
}

//////////////////////////////////////////////////////////////////////////
///sync
//////////////////////////////////////////////////////////////////////////
void Mp4Enc_Picture_Level_Sync()
{
	READ_REG_POLL(VSP_DBK_REG_BASE+DBK_CTR1_OFF, V_BIT_0, 0 , TIME_OUT_CLK, "DBK_CTR1: polling mbc done, the last mb of current frame");
	READ_REG_POLL(VSP_AHBM_REG_BASE+AHBM_STS_OFFSET, V_BIT_0, 0, TIME_OUT_CLK, "AHBM_STS: polling AHB idle status");	
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
