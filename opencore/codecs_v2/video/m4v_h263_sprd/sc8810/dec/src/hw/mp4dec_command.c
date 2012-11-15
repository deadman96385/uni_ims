/******************************************************************************
 ** File Name:    mp4dec_vop.c                                             *
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

PUBLIC void Mp4Dec_init_frame_VSP(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
	uint32 cmd;
	
	g_cmd_data_ptr = g_cmd_data_base;
	g_cmd_info_ptr = g_cmd_info_base;

	/*init vsp command*/
	cmd = (0<<16) |(1<<15) | (0<<14) |(1<<12) | ((vop_mode_ptr->video_std)<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
     	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0, disable dbk/mbc out");

	/*frame size, YUV format, max_X, max_Y*/
	cmd = (JPEG_FW_YUV420 << 24) | (vop_mode_ptr->MBNumY << 12) | vop_mode_ptr->MBNumX;
	VSP_WRITE_REG_CQM(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");
	VSP_WRITE_CMD_INFO((VSP_GLB << 29) | (2<<24) | (GLB_CFG1_WOFF<<8) | (GLB_CFG0_WOFF));

	//dbk.
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CFG_OFF, (vop_mode_ptr->post_filter_en<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: enable/disable post-filter and free_run_mode");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, 0x33333333, "configure bs h1");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, 0x33333333, "configure bs v1");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_0, "DBK_VDB_BUF_ST: init ping-pong buffer to be available");
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");
	VSP_WRITE_CMD_INFO( (VSP_DBK << 29)| (5<<24) |(HDBK_BS_V1_WOFF<<16) | (HDBK_BS_H1_WOFF<<8) | (DBK_CFG_WOFF));
	VSP_WRITE_CMD_INFO( (DBK_CTR1_WOFF<<8) | DBK_VDB_BUF_ST_WOFF);

	Mp4Dec_InitBitstream(
#if _CMODEL_
		dec_input_ptr->pStream
#else
		((void *)Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->frame_bistrm_ptr))
#endif		
		,dec_input_ptr->dataLen);

	return;
}

PUBLIC void VSP_Mp4DctFillQuantTblBfr(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 i;
	uint32 uTmp;
	uint32 pTableAddr = (uint32)MP4DEC_IQUANT_TBL_ADDR;
	uint8 *pTable = vop_mode_ptr->IntraQuantizerMatrix;
	
	open_vsp_iram();	

	/*the quantization matrix*/
	for(i = 0; i < BLOCK_SQUARE_SIZE-1; i += 2)
	{
		uTmp = ((pTable[ASIC_DCT_Matrix[i+1]] << 16) | (pTable[ASIC_DCT_Matrix[i]]));
		VSP_WRITE_REG(pTableAddr+ i * 2, uTmp,"MP4DEC_IQUANT_TBL_ADDR: configure intra quant table");
	}
	
	pTable = vop_mode_ptr->InterQuantizerMatrix;
	pTableAddr = MP4DEC_IQUANT_TBL_ADDR + BLOCK_SQUARE_SIZE * 2;
	
	for(i = 0; i < BLOCK_SQUARE_SIZE-1; i += 2)
	{
		uTmp = ((pTable[ASIC_DCT_Matrix[i+1]] << 16) | (pTable[ASIC_DCT_Matrix[i]]));
		VSP_WRITE_REG(pTableAddr+ i * 2, uTmp,"MP4DEC_IQUANT_TBL_ADDR: configure inter quant table");
	}

	close_vsp_iram();

	return;
}

LOCAL MMDecRet Mp4Dec_VSPInit ()
{
	uint32 cmd;

	int vsp_stat = VSP_ACQUIRE_Dev();
	if(vsp_stat)
	{
		VSP_RELEASE_Dev();
		return MMENC_ERROR;
	}
	
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, 0x5, "ENDAIN_SEL: 0x5 for little endian system");	

    	/*clear time_out int_raw flag, if timeout occurs*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");
        	
    	/*init dcam command*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

    	cmd = (1 << 16) |((uint32)0xffff);
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: enable hardware timer out");

#if 0//defined(_VSP_) 
	//enable dcam interrupt
	*(volatile uint32 *)0x80003008 |= (1<<23);	//In 8800G, the interrupt enable bit is BIT23. 

	//INT Enable
	cmd =  (1 << 12) |(1 << 15);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, cmd, "DCAM_INT_MASK: enable related INT bit, timeout and cmd_done");

	//init int
	DCAMMODULE_Init();
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, cmd, "DCAM_INT_MASK: enable related INT bit");

	//register the int fun;
	DCAMMODULE_RegisterIntFunc(VSP_CMD_DONE_ID, CMD_DONE_INT_PROC);
	DCAMMODULE_RegisterIntFunc(VSP_TIMEOUT_ID, TIMEOUT_INT_PROC);
#endif

	return MMENC_OK;
}

PUBLIC MMDecRet Mp4Dec_VspFrameInit(DEC_VOP_MODE_T *vop_mode_ptr, uint32 bitstm_addr)
{
	uint32 cmd;
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;

	if (Mp4Dec_VSPInit() != MMDEC_OK)
	{
		return MMDEC_ERROR;
	}

#if _CMODEL_
	//configure command infor and data address into register, and enable cmd-exe
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_INFO_BUF_ADDR_OFFSET, ((uint32)CMD_CONTROL_INFO_ADDR)>>8, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_DATA_BUF_ADDR_OFFSET, ((uint32)CMD_CONTROL_DATA_ADDR)>>8, "config CMD control data buffer address");
#else
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_INFO_BUF_ADDR_OFFSET, ((uint32)Mp4Dec_ExtraMem_V2Phy(g_cmd_info_base))>>8, "config CMD control info buffer address");
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+CMD_DATA_BUF_ADDR_OFFSET, ((uint32)Mp4Dec_ExtraMem_V2Phy(g_cmd_data_base))>>8, "config CMD control data buffer address");
#endif

	open_vsp_iram();
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#else //actual envioronment
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, bitstm_addr>>8, "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#endif

	/*init AHBM, current frame, forward reference frame, backward reference frame*/	
	VSP_WRITE_REG(pTableAddr+ 0, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr,"configure reconstruct frame Y");
	if (vop_mode_ptr->post_filter_en)
	{
		VSP_WRITE_REG(pTableAddr+ 4, vop_mode_ptr->pCurDispFrame->pDecFrame->imgYAddr, "configure display frame Y");
	}else
	{
		VSP_WRITE_REG(pTableAddr+ 4, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr, "configure display frame Y");
	}

#if _CMODEL_
	VSP_WRITE_REG(pTableAddr+ 12, ((uint32)DC_AC_PREDICTION_ADDR)>>8, "config top coeff address");
#else
	VSP_WRITE_REG(pTableAddr+ 12, ((uint32)Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->pTopCoeff))>>8, "config top coeff address");	
#endif 
	
	if(IVOP != vop_mode_ptr->VopPredType)
	{
		if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
		{
			if (!vop_mode_ptr->post_filter_en)
			{
				VSP_WRITE_REG(pTableAddr+ 20, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr, "configure backward reference frame Y");
			}else
			{
				return MMDEC_ERROR;
			}
		}else
		{
			VSP_WRITE_REG(pTableAddr+ 20, vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr, "configure backward reference frame Y");
		}
		
		if(BVOP == vop_mode_ptr->VopPredType)
		{
			if (vop_mode_ptr->pFrdRefFrame->pDecFrame == NULL)
			{
				if (!vop_mode_ptr->post_filter_en)
				{			
					VSP_WRITE_REG(pTableAddr+ 16, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr, "configure forward reference frame Y");
				}else
				{
					return MMDEC_ERROR;
				}
			}else
			{
				VSP_WRITE_REG(pTableAddr+ 16, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYAddr, "configure forward reference frame Y");
			}
		}
	}
		
	close_vsp_iram();

	//now, for uv_interleaved
	cmd = VSP_READ_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, "Read out Endian Info") & 0xf;

#if _CMODEL_ //for RTL simulation
	cmd |= (((352*288)>>2)<<4); //word unit
#else
	cmd |= (((vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight)>>2)<<4); //word unit
#endif
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");
	
	/*frame width and height in pixel*/
	cmd = (vop_mode_ptr->FrameHeight << 16) | (vop_mode_ptr->FrameWidth);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, cmd, "DCAM_DCAM_SRC_SZIE: configure frame width and height");

	if (!vop_mode_ptr->bReversibleVlc || (vop_mode_ptr->VopPredType == BVOP))
	{
		if (vop_mode_ptr->is_need_init_vsp_huff_tab)
		{
			configure_huff_tab((uint32 *)g_mp4_dec_huff_tbl, 152);

			if (vop_mode_ptr->FrameWidth > 176 && vop_mode_ptr->FrameHeight > 144)
			{
				vop_mode_ptr->is_need_init_vsp_huff_tab = FALSE; //for vt, do NOT go here.
			}
		}
	}else
	{
		configure_huff_tab((uint32 *)g_rvlc_huff_tab, 146);
	}

	// quantMode = (vop_mode_ptr->QuantizerType == Q_H263) ? 1 : 0;
	if(Q_MPEG == vop_mode_ptr->QuantizerType)
	{
		if (vop_mode_ptr->is_need_init_vsp_quant_tab)
		{
			VSP_Mp4DctFillQuantTblBfr(vop_mode_ptr);
			vop_mode_ptr->is_need_init_vsp_quant_tab = FALSE;
		}
	}

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_MASK_OFF, (1<<15), "DCAM_INT_RAW: enable CMD DONE INT bit");

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ConfigVldMB
 ** Description:	config command to do vld of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_ConfigVldMB(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr)
{
	uint32 i, cmd;
	uint32 flush_bits = vop_mode_ptr->bitstrm_ptr->bitcnt - vop_mode_ptr->bitstrm_ptr->bitcnt_before_vld;
	uint32 nWords = flush_bits/32;

	flush_bits -= nWords*32;
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, ((flush_bits<<24) | 1), "BSM_CFG2: flush one byte");	
#if _CMODEL_
	flush_nbits(flush_bits);
#endif
	VSP_WRITE_CMD_INFO((VSP_BSM << 29) | (1<<24) | BSM_CFG2_WOFF);

	if (nWords)
	{
		cmd = (32<<24) | 1;

		for (i = nWords; i > 0; i--)
		{
			VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");
			VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one byte");	
	#if _CMODEL_
			flush_nbits(32);
	#endif
				
			VSP_WRITE_CMD_INFO((VSP_BSM << 29) | (2<<24) | (BSM_CFG2_WOFF<<8) | ((1<<7)|BSM_DEBUG_WOFF));
		}
	}

	/*configure mb type, cbp, qp*/
	cmd =  ( (vop_mode_ptr->bReversibleVlc) && (vop_mode_ptr->VopPredType != BVOP) ) ? (1<< 25) : 0;
	cmd |= ( (!mb_mode_ptr->bIntra) ? (1 << 24 ) : 0 ); 
	cmd |= ( (mb_mode_ptr->CBP << 8) );
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_CFG0_OFFSET, cmd, "VLD_MPEG4_CFG0: configure mb type and cbp");
	VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (1<<24) | VLD_MPEG4_CFG0_WOFF);
	
	/*configure neighbor mb's qp, and availability, and dc_coded_as_ac, ac_pred_ena*/
	if(mb_mode_ptr->bIntra && (VSP_MPEG4 == vop_mode_ptr->video_std))
	{
		DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
		
		cmd = (mb_mode_ptr->StepSize << 24)    | (mb_cache_ptr->bTopMBAvail << 21) | (mb_cache_ptr->topMBQP << 16)          |
			(mb_cache_ptr->bLeftMBAvail << 13) | (mb_cache_ptr->leftMBQP << 8)     | (vop_mode_ptr->bDataPartitioning << 2) |
			(mb_cache_ptr->bCodeDcAsAc << 1)   | (mb_mode_ptr->bACPrediction << 0);
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_CFG1_OFFSET, cmd, "VLD_MPEG4_CFG1: config qp, mb availability, bDataPartitioning, bCodeDcAsAc and bACPrediction");
		
		cmd = ((uint16)(mb_cache_ptr->pDCCache[0]));
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_Y_OFFSET, cmd, "VLD_MPEG4_TL_DC_Y: configure top_left MB's DC for Y");
		
		cmd = (((uint16)mb_cache_ptr->pDCCache[7]) << 16) | (((uint16)(mb_cache_ptr->pDCCache[3])) << 0);
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_UV_OFFSET, cmd, "VLD_MPEG4_TL_DC_UV: configure top_left MB's DC for UV");
		VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (3<<24) | (VLD_MPEG4_TL_DC_UV_WOFF <<16) | (VLD_MPEG4_TL_DC_Y_WOFF << 8) | VLD_MPEG4_CFG1_WOFF); 

		if(vop_mode_ptr->bDataPartitioning && (!mb_cache_ptr->bCodeDcAsAc))
		{
			int32 **dc_store_pptr = Mp4Dec_GetDcStore();
			int32 mb_num = vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX + vop_mode_ptr->mb_x;
			int32  *DCCoef = dc_store_pptr[mb_num];

			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_DC_Y10_OFFSET, ((DCCoef[1]<<16)|(DCCoef[0]&0xffff)), "VLD_MPEG4_DC_Y10: configure dc of block0 and block1, for data-partitioning");
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_DC_Y32_OFFSET, ((DCCoef[3]<<16)|(DCCoef[2]&0xffff)), "VLD_MPEG4_DC_Y32: configure dc of block2 and block3, for data-partitioning");
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_DC_UV_OFFSET, ((DCCoef[5]<<16)|(DCCoef[4]&0xffff)), "VLD_MPEG4_DC_UV: configure dc of block_u and block_v, for data-partitioning");			
			VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (3<<24) | (VLD_MPEG4_DC_UV_WOFF <<16) | (VLD_MPEG4_DC_Y32_WOFF << 8) | VLD_MPEG4_DC_Y10_WOFF); 
		}
	}

	/*start vld one MB*/
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_CTL_OFFSET, 1, "VLD_CTL: configure VLD start");
	VSP_READ_REG_POLL_CQM(VSP_VLD_REG_BASE+VLD_CTL_OFFSET,31, 1, 0,"VLD: polling VLD one MB status");
	VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (2<<24) | (((1<<7)|VLD_CTL_WOFF)<<8) | VLD_CTL_WOFF);

#if _CMODEL_
	vld_module();
#endif //_CMODEL_
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ConfigIqIdctMB
 ** Description:	config command to do iq idct of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_ConfigIqIdctMB(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	uint32 uDctCfg;
	uint32 uMpeg4DequanPara = (mb_mode_ptr->StepSize << 16);

	if(mb_mode_ptr->bIntra)
	{
		DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

		mb_mode_ptr->CBP = 0x3f;
		uDctCfg = vop_mode_ptr->intra_dct_cfg;
		uMpeg4DequanPara |= ((mb_cache_ptr->iDcScalerC << 8)|(mb_cache_ptr->iDcScalerY));
	}else
	{
		uDctCfg = vop_mode_ptr->inter_dct_cfg;
	}

	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_CONFIG_OFF, uDctCfg, "write DCT_CONFIG reg ");
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_MPEG4_DQUANT_PARA_OFF, uMpeg4DequanPara, "configure dequant para");

	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_IN_CBP_OFF, mb_mode_ptr->CBP, "configure cbp");
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_START_OFF, 1,"start IDCT");

	VSP_WRITE_CMD_INFO((VSP_DCT<<29) | (4<<24) | (DCT_IN_CBP_WOFF<<16) | (DCT_MPEG4_DQUANT_PARA_WOFF<<8) | DCT_CONFIG_WOFF);
	VSP_WRITE_CMD_INFO(DCT_START_WOFF);

#if _CMODEL_
	dct_module();	
#endif //_CMODEL_
}

const uint32 s_dbk_qp_tbl[32] = 
{
	0, 2, 3, 5, 7, 8, 10, 11, 13, 15, 16, 18, 20, 21, 
		23, 24, 26, 28, 29, 31, 33, 34, 36, 37, 39, 41, 42,44, 46,  47, 49, 51
};

PUBLIC void BS_and_Para (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 chroma_qp_offset = 0;
	int32 LFAlphaC0Offset = 12;		//jin 20110602 DBK strength
	int32 LFBetaOffset = 12;
	int32 topMB_qp, leftMB_qp;

 	uint32 bs[32];
	uint32 cmd = 0;

	if (mb_y)
	{
		topMB_qp = (mb_mode_ptr-vop_mode_ptr->MBNumX)->StepSize;
	}else
	{
		topMB_qp = 0;
	}

	if (mb_x)
	{
		leftMB_qp = (mb_mode_ptr-1)->StepSize;
	}else
	{
		leftMB_qp = 0;
	}
	
	//polling dbk ready
	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 0, 1, 0,"dbk: polling dbk cfg finish flag = 0");

	cmd = ((mb_x << 24) | (s_dbk_qp_tbl[topMB_qp] << 16) | (s_dbk_qp_tbl[leftMB_qp] << 8) | (s_dbk_qp_tbl[mb_mode_ptr->StepSize] << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");

	cmd = (((chroma_qp_offset & 0x1f) << 16) | ((LFAlphaC0Offset & 0x1f) << 8) | ((LFBetaOffset & 0x1f) << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");

	cmd = mb_x ? 0x33334444 : 0x33330000;
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, cmd, "configure bs h0");

	cmd = mb_y ? 0x33334444 : 0x33330000;
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, cmd, "configure bs v0");

	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");

	VSP_WRITE_CMD_INFO((VSP_DBK << 29) | (6<<24) |(HDBK_PARS_WOFF<<16) | (HDBK_MB_INFO_WOFF<<8) |((1<<7)|HDBK_CFG_FINISH_WOFF));
	VSP_WRITE_CMD_INFO( (HDBK_CFG_FINISH_WOFF<<16) |(HDBK_BS_V0_WOFF<<8) | (HDBK_BS_H0_WOFF));
	
	return;
}

PUBLIC void Mp4Dec_MBC_DBK_Cmd(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	uint32 vsp_cbp;

	if(mb_mode_ptr->bIntra)
	{
		vsp_cbp = 0x3f;
	}else
	{
		vsp_cbp = mb_mode_ptr->CBP;
	}

	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF,( ( (!mb_mode_ptr->bIntra) ? (1<<30):0 ) | (vsp_cbp << 0) ), "MBC_CMD0: configure mb type and cbp");
	VSP_WRITE_CMD_INFO( (VSP_MBC << 29) | (1<<24) | MBC_CMD0_WOFF);

	BS_and_Para (vop_mode_ptr, mb_mode_ptr);

	return;	
}

PUBLIC void Mp4Dec_VspMBInit (int32 mb_x, int32 mb_y)
{
	uint32 cmd;

#if _CMODEL_
	#include "buffer_global.h"
	memset(vsp_fw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	memset(vsp_bw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));		
#endif //_CMODEL_

	cmd  = (mb_y << 8) | (mb_x << 0);
	VSP_WRITE_REG_CQM (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
	VSP_WRITE_CMD_INFO((VSP_GLB << 29) | (1<<24) | GLB_CTRL0_WOFF);
}

PUBLIC void Mp4Dec_CheckMBCStatus(DEC_VOP_MODE_T *vop_mode_ptr)
{
#if _CMODEL_
	#include "mbc_global.h"
	mbc_module();
#endif //_CMODEL_

	//check the mbc done flag, or time out flag; if time out, error occur then reset the vsp	
// 	READ_REG_POLL(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, V_BIT_5, TIME_OUT_CLK, "MBC: polling mbc done");
	VSP_READ_REG_POLL_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF,5, 1, 1, "MBC: polling mbc done");
		
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");
	VSP_WRITE_CMD_INFO((VSP_MBC << 29) | (2<<24) | (MBC_ST0_WOFF<<8) | ((1<<7)|MBC_ST0_WOFF));

#if _CMODEL_
	#include "hdbk_global.h"
	dbk_module();
#endif	
}

PUBLIC void Mp4Dec_VerifyBitstrm(uint8 *pStream, int32 strmLen)
{
	uint8 *pStreamEnd = pStream + (strmLen-4);
    	uint8 *tempPos = pStream;
//     uint32  packetLen; 
    	DEC_VOP_MODE_T *vop_mode_ptr = Mp4Dec_GetVopmode();

 	if (vop_mode_ptr->video_std == VSP_MPEG4) //only verify MPEG4 bitstrm
	{
		while (tempPos < pStreamEnd)
		{
			if (tempPos[0] == 0x00 && tempPos[1] == 0x00)
			{
				if (tempPos[2] == 0x01 && tempPos[3] == 0xB6) /* MPEG4 VOP start code */
				{
					vop_mode_ptr->video_std = VSP_MPEG4;
					return;
				}
				else if ((tempPos[2] & 0xFC) == 0x80 && (tempPos[3] & 0x03)==0x02) /* H.263 PSC*/
				{
					SCI_TRACE_LOW("Mp4Dec_VerifyBitstrm: it is ITU-H.263 format:\n");
					vop_mode_ptr->video_std = VSP_ITU_H263;
					vop_mode_ptr->bDataPartitioning = FALSE; //MUST!, xweiluo@2012.03.01
					vop_mode_ptr->bReversibleVlc = FALSE;
					return;
				}
			}
			tempPos++;
		}

		if (tempPos == pStreamEnd)
		{
			vop_mode_ptr->error_flag = TRUE;
		}
	}	

	return;
}

PUBLIC void Mp4Dec_InitBitstream(void *pOneFrameBitstream, int32 length)
{
	uint32 flushBytes;

#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG1_OFF, g_stream_offset>>2, "BSM_CFG1: configure the bitstream address");
	flushBytes = g_stream_offset & 0x3;
#else //actual envioronment
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG1_OFF, (((uint32)pOneFrameBitstream>>2) & 0x3f), "BSM_CFG1: configure the bitstream address");
	flushBytes = (uint32)pOneFrameBitstream & 0x3;
#endif

	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG0_OFF, (V_BIT_31) | 0x3ffff, "BSM_CFG0: init bsm register: buffer 0 ready and the max buffer size");
	
	/*polling bsm status*/
	VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF,31, 1, 1, "BSM_DEBUG: polling bsm status");
	VSP_WRITE_CMD_INFO((VSP_BSM << 29) | (3<<24) | (((1<<7)|BSM_DEBUG_WOFF)<<16) | (BSM_CFG0_WOFF<<8) | BSM_CFG1_WOFF);
		
	flush_unalign_bytes(flushBytes);

#if _CMODEL_
	g_bs_pingpang_bfr0 = pOneFrameBitstream;
	g_bs_pingpang_bfr_len = length;

	init_bsm();
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
