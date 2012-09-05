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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_exit_picture(DEC_VOP_MODE_T *vop_mode_ptr)
{	
	/*reorder reference frame list*/
	if(vop_mode_ptr->VopPredType != BVOP)
	{
		Mp4DecStorablePic * pframetmp;

		vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;
#ifdef _VSP_LINUX_
		if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader!=NULL))
		(*VSP_bindCb)(g_user_data,vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader,0);
#endif
		/*the buffer for forward reference frame will no longer be a reference frame*/		
		if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
		{
			vop_mode_ptr->pFrdRefFrame->pDecFrame->bRef = FALSE;
#ifdef _VSP_LINUX_
		if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader!=NULL)){
			(*VSP_unbindCb)(g_user_data,vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader,0);
			vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader = NULL;
		}
#endif			
			vop_mode_ptr->pFrdRefFrame->pDecFrame = NULL;
		}
		
		//exchange buffer address. bck->frd, current->bck, frd->current
		pframetmp = vop_mode_ptr->pFrdRefFrame;
		vop_mode_ptr->pFrdRefFrame = vop_mode_ptr->pBckRefFrame;
		vop_mode_ptr->pBckRefFrame = vop_mode_ptr->pCurRecFrame;	
		vop_mode_ptr->pCurRecFrame = pframetmp;	
	}
}

LOCAL void VSP_Mp4DctFillQuantTblBfr(DEC_VOP_MODE_T *vop_mode_ptr)
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

PUBLIC MMDecRet Mp4Dec_VspFrameInit(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;

	open_vsp_iram();	

	/*init AHBM, current frame, forward reference frame, backward reference frame*/	
	VSP_WRITE_REG(pTableAddr+ 0, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr,"configure reconstruct frame Y");
	if (vop_mode_ptr->post_filter_en)
	{
		VSP_WRITE_REG(pTableAddr+ 4, vop_mode_ptr->pCurDispFrame->pDecFrame->imgYAddr, "configure display frame Y");
	}

#if _CMODEL_
	VSP_WRITE_REG(pTableAddr+ 12, ((uint32)DC_AC_PREDICTION_ADDR)>>8, "config top coeff address");
#else	
#ifdef _VSP_LINUX_
	VSP_WRITE_REG(pTableAddr+ 12, ((uint32)Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->pTopCoeff))>>8, "AHBM_FRM_ADDR_7: config top coeff address");	
#else
	VSP_WRITE_REG(pTableAddr+ 12, ((uint32)vop_mode_ptr->pTopCoeff)>>8, "config top coeff address");
#endif //
#endif	
	if(IVOP != vop_mode_ptr->VopPredType)
	{
		if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
		{
			if (!vop_mode_ptr->post_filter_en){
				VSP_WRITE_REG(pTableAddr+ 20, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr, "configure backward reference frame Y");
			}else{
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
				if (!vop_mode_ptr->post_filter_en){			
					VSP_WRITE_REG(pTableAddr+ 16, vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr, "configure forward reference frame Y");
				}else{
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
	cmd |= ((((vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight))>>2)<<4); //word unit
#endif
	VSP_WRITE_REG(VSP_AHBM_REG_BASE+AHBM_ENDAIN_SEL_OFFSET, cmd, "configure uv offset");

	/*frame size, YUV format, max_X, max_Y*/
	cmd = (JPEG_FW_YUV420 << 24) | (vop_mode_ptr->MBNumY << 12) | vop_mode_ptr->MBNumX;
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG1_OFF, cmd, "GLB_CFG1: configure max_y and max_X");
	
	/*frame width and height in pixel*/
	cmd = (vop_mode_ptr->FrameHeight << 16) | (vop_mode_ptr->FrameWidth);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_SRC_SIZE_OFF, cmd, "DCAM_DCAM_SRC_SZIE: configure frame width and height");
	
	if (!vop_mode_ptr->bReversibleVlc || (vop_mode_ptr->VopPredType == BVOP))
	{
		configure_huff_tab(g_mp4_dec_huff_tbl, 152);
	}
	else
	{
		configure_huff_tab(g_rvlc_huff_tab, 146);
	}

	// quantMode = (vop_mode_ptr->QuantizerType == Q_H263) ? 1 : 0;
	if(Q_MPEG == vop_mode_ptr->QuantizerType)
	{
		VSP_Mp4DctFillQuantTblBfr(vop_mode_ptr);
		vop_mode_ptr->intra_dct_cfg =  0x180;//((1 << 8 ) | ((int32)(!0) << 7) | (0 << 6) | (0 << 5)	| (0 << 4) | (0 << 1) |(0 << 0));
		vop_mode_ptr->inter_dct_cfg =  0x1a0;//((1 << 8 ) | ((int)(!0) << 7) | (0 << 6) | (1 << 5) | (0 << 4) | (0 << 1) |(0 << 0));
	}else
	{
		vop_mode_ptr->intra_dct_cfg =  0x150;//((1 << 8 ) | ((int32)(!1) << 7) | (1 << 6) | (0 << 5)	| (1 << 4) | (0 << 1) |(0 << 0));;
		vop_mode_ptr->inter_dct_cfg =  0x170;//((1 << 8 ) | ((int)(!1) << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (0 << 1) |(0 << 0));
	}

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_RAW_OFF, 0, "DCAM_INT_RAW: clear int_raw flag");

	//dbk.
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, (vop_mode_ptr->post_filter_en<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: enable/disable post-filter and free_run_mode");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_1|V_BIT_0, "DBK_VDB_BUF_ST: init ping-pong buffer to be available");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, 0x44444444, "configure bs h0");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, 0x44444444, "configure bs h1");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, 0x44444444, "configure bs v0");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, 0x44444444, "configure bs v1");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");

	return MMDEC_OK;
}

PUBLIC MMDecRet Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
	MMDecRet ret = MMDEC_ERROR;
//	int size = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

	vop_mode_ptr->sliceNumber	= 0;
	vop_mode_ptr->stop_decoding = FALSE;
	vop_mode_ptr->mbnumDec		= 0;
	vop_mode_ptr->error_flag	= FALSE;
	vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
	vop_mode_ptr->err_num		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_left		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_pos_ptr	= dec_input_ptr->err_pkt_pos;
	vop_mode_ptr->err_MB_num	= vop_mode_ptr->MBNum;
	
	ret = Mp4Dec_VspFrameInit(vop_mode_ptr);

	//set all mb in one vop to not-decoded
	memset (vop_mode_ptr->mbdec_stat_ptr, 0, vop_mode_ptr->MBNum);
	
	if(MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->mb_cache_ptr->iDcScalerY = g_dc_scaler_table_y[vop_mode_ptr->StepSize];
		vop_mode_ptr->mb_cache_ptr->iDcScalerC = g_dc_scaler_table_c[vop_mode_ptr->StepSize];
	}

#if 0	//disable it for avoid green block effect in real environment.
	if (dec_input_ptr->err_pkt_num != 0) //for error frame, only for debug
	{
		memset (vop_mode_ptr->pCurRecFrame->pDecFrame->imgY, 0, size);
		if (vop_mode_ptr->uv_interleaved)
		{
			memset (vop_mode_ptr->pCurRecFrame->pDecFrame->imgU, 0, size/2);
		}else
		{
			memset (vop_mode_ptr->pCurRecFrame->pDecFrame->imgU, 0, size/4);
			memset (vop_mode_ptr->pCurRecFrame->pDecFrame->imgV, 0, size/4);
		}
#endif

	return ret;
}

PUBLIC void Mp4Dec_Reset (DEC_VOP_MODE_T * vop_mode_ptr)
{
	uint32 cmd;

	VSP_Reset();

    /*clear time_out int_raw flag, if timeout occurs*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_12, "DCAM_INT_CLR: clear time_out int_raw flag");
        	
	/*init dcam command*/
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

	/*init vsp command*/
	cmd = (0<<16) |(1<<15) | (0<<14) |(1<<12) | ((vop_mode_ptr->video_std)<<8) | (1<<7) | (1<<6) | (1<<3) | (1<<2) | (1<<1) | (1<<0);
	VSP_WRITE_REG(VSP_GLB_REG_BASE+GLB_CFG0_OFF, cmd, "GLB_CFG0: global config0, disable dbk/mbc out");

	cmd = (0 << 16) |((uint32)0xffff);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: disable hardware timer out");

#if _CMODEL_
	g_vld_reg_ptr->VLD_CTL = 0;
#endif
}

LOCAL void ExchangeMBMode (DEC_VOP_MODE_T * vop_mode_ptr)
{
	DEC_MB_MODE_T *mb_mode_tmp_ptr;

	mb_mode_tmp_ptr				= vop_mode_ptr->pMbMode;
	vop_mode_ptr->pMbMode		= vop_mode_ptr->pMbMode_prev;
	vop_mode_ptr->pMbMode_prev	= mb_mode_tmp_ptr;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIVOP
 ** Description:	Decode the IVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecIVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
	BOOLEAN is_itu_h263 = (ITU_H263 == vop_mode_ptr->video_std)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;

	vop_mode_ptr->mb_cache_ptr->mca_type = MCA_BACKWARD;
	      
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
		vop_mode_ptr->mb_y = (int8)pos_y;

		if(is_itu_h263)
		{
			/*decode GOB header*/	
			ret = Mp4Dec_DecGobHeader(vop_mode_ptr);
		}

		/*decode one MB line*/		
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{
		#if _DEBUG_
			if((pos_y == 6)&&(pos_x == 9)&&(g_nFrame_dec == 53))
			{
				foo();
			}
		#endif //_DEBUG_
		#if _TRACE_			
			FPRINTF (g_fp_trace_fw, "\nnframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
		#endif //_TRACE_	
		
			vop_mode_ptr->mb_x = (int8)pos_x;	
			mb_mode_ptr = vop_mode_ptr->pMbMode + pos_y * total_mb_num_x + pos_x;

			/*if error founded, search next resync header*/
			if (vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->error_flag = FALSE;

				rsc_found = Mp4Dec_SearchResynCode (vop_mode_ptr);
				if (!rsc_found)
				{
					return MMDEC_STREAM_ERROR;
				}
				else
				{					
					if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
					{
						return MMDEC_ERROR;
					}

					if(is_itu_h263)
					{
						/*decode GOB header*/	
						ret = Mp4Dec_DecGobHeader(vop_mode_ptr);
					}else
					{
						ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, 0);
					}

                    if (vop_mode_ptr->error_flag)
					{
						PRINTF ("decode resync header error!\n");
						continue;
					}
					
					vop_mode_ptr->VopPredType = PVOP; //here, for use MCA hardware module
					ret = Mp4Dec_EC_FillMBGap (vop_mode_ptr);
					vop_mode_ptr->VopPredType = IVOP;  //here, set it's original value
					if (ret != MMDEC_OK)
					{
						return ret;
					}
				
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}
			}else
			{
				//normal case.
				if(!vop_mode_ptr->bResyncMarkerDisable)
				{
					 mb_mode_ptr->bFirstMB_in_VP = FALSE;

					if(READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
						"polling bsm fifo depth >= 8 words for check resync marke and packet header"))
					{
						vop_mode_ptr->error_flag = TRUE;
						continue;
					}

					if(Mp4Dec_CheckResyncMarker(0))
					{
						ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,  0);
						pos_x = vop_mode_ptr->mb_x;
						pos_y = vop_mode_ptr->mb_y;
						mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
						mb_mode_ptr->bFirstMB_in_VP = TRUE;
					}

					if (vop_mode_ptr->error_flag)
					{
						PRINTF ("decode resync header error!\n");
						continue;
					}
				}
			}

			if((0 == pos_x) && (0 == pos_y))
			{
				mb_mode_ptr->bFirstMB_in_VP = TRUE;
			}

			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);	
			vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;

			Mp4Dec_DecIntraMBHeader(vop_mode_ptr, mb_mode_ptr);
			if(vop_mode_ptr->error_flag)
			{
				PRINTF("decode intra mb header error!\n");
				continue;
			}
	
			Mp4Dec_VspMBInit(pos_x, pos_y);

			/*configure MBC and DBK*/
			Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_ptr);

			/*vld*/
			Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr);

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				Mp4Dec_CheckMBCStatus(vop_mode_ptr);
				if(vop_mode_ptr->error_flag)
				{
					PRINTF("mbc time out!\n");
					return MMDEC_HW_ERROR;
				}

				vop_mode_ptr->err_MB_num--;
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_NOT_IN_ERR_PKT;

				if (vop_mode_ptr->err_left != 0)
				{
					/*determine whether the mb is located in error domain*/
					mb_end_pos = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg") + 7) / 8;		
					err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;
					if (mb_end_pos >= err_start_pos)
					{
						vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_IN_ERR_PKT;
					}
				}
			}else
			{
				if (vop_mode_ptr->video_std != FLV_H263)
				{
					vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
					PRINTF ("decode intra mb coeff error!\n");
					continue;
				}else
				{
					return MMDEC_HW_ERROR;
				}		
			}
							
			((int*)mb_mode_ptr->mv)[0] = 0;   //set to zero for B frame'mv prediction
			((int*)mb_mode_ptr->mv)[1] = 0;
			((int*)mb_mode_ptr->mv)[2] = 0;
			((int*)mb_mode_ptr->mv)[3] = 0;

			vop_mode_ptr->mbnumDec++;
		}

		vop_mode_ptr->GobNum++;
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecPVOP
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecPVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T * mb_mode_ptr = NULL;//vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (vop_mode_ptr->video_std == ITU_H263)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;

	ExchangeMBMode (vop_mode_ptr);
	mb_mode_ptr		= vop_mode_ptr->pMbMode;
	
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
		vop_mode_ptr->mb_y = (int8)pos_y;

		if(is_itu_h263)
		{
			/*decode GOB header*/
			Mp4Dec_DecGobHeader(vop_mode_ptr);
		}
			
		/*decode one MB line*/
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{	
		#if _DEBUG_
			if((pos_x == 0)&&(pos_y == 0)&&(g_nFrame_dec == 25))
			{
				foo();
			}
		#endif //_DEBUG_
		#if _TRACE_			
			FPRINTF (g_fp_trace_fw, "\nnframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
		#endif //_TRACE_		

			vop_mode_ptr->mb_x = (int8)pos_x;
			mb_mode_ptr = vop_mode_ptr->pMbMode + pos_y * total_mb_num_x + pos_x;

			/*if error founded, search next resync header*/
			if (vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->error_flag = FALSE;

				rsc_found = Mp4Dec_SearchResynCode (vop_mode_ptr);
				if (!rsc_found)
				{
					return MMDEC_STREAM_ERROR;
				}
				else
				{		
					if(is_itu_h263)
					{
						/*decode GOB header*/	
						ret = Mp4Dec_DecGobHeader(vop_mode_ptr);	
					}else
					{
						ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
					}

                    if (vop_mode_ptr->error_flag)
					{
						PRINTF ("decode resync header error!\n");
						continue;
					}
					
					ret = Mp4Dec_EC_FillMBGap (vop_mode_ptr);
					if (ret != MMDEC_OK)
					{
						return ret;
					}			

					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}
			}
			
			if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				mb_mode_ptr->bFirstMB_in_VP = FALSE;

				if(READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
					"polling bsm fifo depth >= 8 words for check resync marke and packet header"))
				{
					vop_mode_ptr->error_flag = TRUE;
					continue;
				}
				
				if(Mp4Dec_CheckResyncMarker(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
							
					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}	
				
				if (vop_mode_ptr->error_flag)
				{
					PRINTF ("decode resync header error!\n");
					continue;
				}
			}

			if((0 == pos_x) && (0 == pos_y))
			{
				mb_mode_ptr->bFirstMB_in_VP = TRUE;
			}

			mb_cache_ptr->start_pos = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg") / 8;
			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
			vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;

			Mp4Dec_DecInterMBHeader(vop_mode_ptr, mb_mode_ptr);
			if (vop_mode_ptr->error_flag)
			{
				PRINTF ("decode inter mb header error!\n");
				continue;
			}

			Mp4Dec_DecMV(vop_mode_ptr, mb_mode_ptr);
			if(vop_mode_ptr->error_flag)
			{
				PRINTF("decode mv error!\n");
				continue;
			}

			Mp4Dec_VspMBInit(pos_x, pos_y);

			/*configure MBC and DBK*/
			Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_ptr);

			if(mb_mode_ptr->bIntra)
			{			
				/*vld*/
				Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr);
			}else
			{	
				if(mb_mode_ptr->bSkip)
				{	
					mb_mode_ptr->dctMd = MODE_NOT_CODED;
				}else
				{
					/*vld*/
					Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_ptr);
				}
				
				if(!vop_mode_ptr->error_flag)
				{
					Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
				}
			}

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				Mp4Dec_CheckMBCStatus(vop_mode_ptr);
				if(vop_mode_ptr->error_flag)
				{
					PRINTF("mbc time out!\n");
					return MMDEC_HW_ERROR;
				}

				vop_mode_ptr->err_MB_num--;
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_NOT_IN_ERR_PKT;

				if (vop_mode_ptr->err_left != 0)
				{
					/*determine whether the mb is located in error domain*/
					mb_end_pos = (VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg") + 7) / 8;		
					err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;
					if (mb_end_pos >= err_start_pos)
					{
						vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_IN_ERR_PKT;
					}
				}
			}else
			{
				if (vop_mode_ptr->video_std != FLV_H263)
				{
					vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
					PRINTF ("\ndecode inter mb of pVop error!");
					continue;
				}else
				{
					return MMDEC_HW_ERROR;
				}						
			}	
			
			vop_mode_ptr->mbnumDec++;	
		}

		vop_mode_ptr->GobNum++;
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecBVOP
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecBVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	DEC_MB_MODE_T *mb_mode_bvop_ptr = vop_mode_ptr->pMbMode_B;
	DEC_MB_MODE_T *pCoMb_mode = vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdPredMv, BckPredMv, zeroMv = {0, 0};
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;

	FwdPredMv = zeroMv;
	BckPredMv = zeroMv;
	
	for(pos_y = 0; pos_y < total_mb_num_y; pos_y++)
	{	
		vop_mode_ptr->mb_y = (int8)pos_y;

		//reset prediction mv
		FwdPredMv = zeroMv;
		BckPredMv = zeroMv;

		/*decode one MB line*/
		for(pos_x = 0; pos_x < total_mb_num_x; pos_x++)
		{	
		#if _DEBUG_
			if((pos_y == 10)&&(pos_x == 4)&&(g_nFrame_dec == 2))
			{
				foo();
			}
		#endif //_DEBUG_	
			
		#if _TRACE_	
			FPRINTF (g_fp_trace_fw, "\nnframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
		#endif //_TRACE_
		
			vop_mode_ptr->mb_x = (int8)pos_x;

			if(!vop_mode_ptr->bResyncMarkerDisable)
			{
				if(READ_REG_POLL(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
					"polling bsm fifo depth >= 8 words for check resync marke and packet header"))
				{
					vop_mode_ptr->error_flag = TRUE;
					return MMDEC_STREAM_ERROR;
				}

				if(Mp4Dec_CheckResyncMarker(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);

					//reset prediction mv
					FwdPredMv = zeroMv; 
					BckPredMv = zeroMv;
				}			
			}

			mb_mode_bvop_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);

			Mp4Dec_VspMBInit(pos_x, pos_y);
			
			if(MODE_NOT_CODED == pCoMb_mode->dctMd)
			{
			#if _TRACE_
				FPRINTF (g_fp_trace_fw, "co-located MB skipped!\n");
			#endif //_TRACE_
				mb_mode_bvop_ptr->bIntra = FALSE; //for dbk.
				mb_mode_bvop_ptr->StepSize = vop_mode_ptr->StepSize; //for dbk.
				mb_mode_bvop_ptr->dctMd = FORWARD;
				mb_mode_bvop_ptr->CBP = 0x00;
				mb_cache_ptr->mca_type = MCA_FORWARD;
				memset(mb_cache_ptr->fmv, 0, sizeof(mb_cache_ptr->fmv));
				memset(mb_cache_ptr->bmv, 0, sizeof(mb_cache_ptr->bmv));
	
				/*configure MBC and DBK*/
				Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_bvop_ptr);

				/*configure zero mv to mca*/
				Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, &zeroMv);
			}else
			{
				Mp4Dec_DecMBHeaderBVOP(vop_mode_ptr,  mb_mode_bvop_ptr);

				/*configure MBC and DBK*/
				Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_bvop_ptr);

				Mp4Dec_MCA_BVOP(vop_mode_ptr, mb_mode_bvop_ptr, &FwdPredMv, &BckPredMv, pCoMb_mode->mv);

				if(vop_mode_ptr->error_flag)
				{
					PRINTF("decode mv of B-VOP error!\n");
					return MMDEC_STREAM_ERROR;
				}
			}
	
			if(mb_mode_bvop_ptr->bSkip)
			{	
				mb_mode_bvop_ptr->CBP = 0x00;
			}

			/*vld*/
			Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_bvop_ptr);

			if(!vop_mode_ptr->error_flag)
			{
				Mp4Dec_CheckMBCStatus(vop_mode_ptr);
				if(vop_mode_ptr->error_flag)
				{
					PRINTF("mbc time out!\n");
					return MMDEC_HW_ERROR;
				}
			}else
			{
				PRINTF ("decode inter mb of B-Vop error!\n");
				return MMDEC_STREAM_ERROR;
			}
				
			pCoMb_mode++;
		}
	}

	return ret;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
