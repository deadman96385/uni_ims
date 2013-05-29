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

void Mp4Dec_exit_picture(DEC_VOP_MODE_T *vop_mode_ptr)
{	
	/*reorder reference frame list*/
	if(vop_mode_ptr->VopPredType != BVOP)
	{
		Mp4DecStorablePic * pframetmp;

		vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;

		/*the buffer for forward reference frame will no longer be a reference frame*/		
		if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
		{
			vop_mode_ptr->pFrdRefFrame->pDecFrame->bRef = FALSE;
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
#ifdef SIM_IN_WIN
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
#endif
	return;
}

PUBLIC void Mp4Dec_VspFrameInit(DEC_VOP_MODE_T *vop_mode_ptr)
{
#ifdef SIM_IN_WIN
	uint32 cmd;
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;

//	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CFG_OFF, MBC_RUN_AUTO_MODE, "MBC_CFG: mpeg4 use auto_mode");

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
	VSP_WRITE_REG(pTableAddr+ 12, ((uint32)vop_mode_ptr->pTopCoeff)>>8, "config top coeff address");
#endif //
	
	if(IVOP != vop_mode_ptr->VopPredType)
	{
		if(vop_mode_ptr->pBckRefFrame->pDecFrame != PNULL)
		{
			VSP_WRITE_REG(pTableAddr+ 20, vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr, "configure backward reference frame Y");
		}
		
		if(BVOP == vop_mode_ptr->VopPredType)
		{
			VSP_WRITE_REG(pTableAddr+ 16, vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYAddr, "configure forward reference frame Y");
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

	if(Q_MPEG == vop_mode_ptr->QuantizerType)
	{
		VSP_Mp4DctFillQuantTblBfr(vop_mode_ptr);
	}

	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_RAW_OFF, 0, "DCAM_INT_RAW: clear int_raw flag");

	//dbk.
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CFG_OFF, (vop_mode_ptr->post_filter_en<<2)|(DBK_RUN_FREE_MODE), "DBK_CFG: enable/disable post-filter and free_run_mode");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_VDB_BUF_ST_OFF, V_BIT_1|V_BIT_0, "DBK_VDB_BUF_ST: init ping-pong buffer to be available");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR1_OFF, 1 , "DBK_CTR1: configure DBK configure finished");
#endif
}

PUBLIC void Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
	int size = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;

	vop_mode_ptr->sliceNumber	= 0;
	vop_mode_ptr->stop_decoding = FALSE;
	vop_mode_ptr->mbnumDec		= 0;
	vop_mode_ptr->error_flag	= FALSE;
	vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
	vop_mode_ptr->err_num		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_left		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_pos_ptr	= dec_input_ptr->err_pkt_pos;
	vop_mode_ptr->err_MB_num	= vop_mode_ptr->MBNum;
	
	Mp4Dec_VspFrameInit(vop_mode_ptr);

	//set all mb in one vop to not-decoded
	memset (vop_mode_ptr->mbdec_stat_ptr, 0, vop_mode_ptr->MBNum);

	if (dec_input_ptr->err_pkt_num != 0) //for error frame
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
	}

	return;
}

BOOLEAN SearchResynCode (DEC_VOP_MODE_T * vop_mode_ptr)
{
	int		rsc_len;
	BOOLEAN	rsc_fnd;
	int		dec_len;
	int		nbits_dec_total;
	BOOLEAN	is_short_header;

	is_short_header = (vop_mode_ptr->video_std != MPEG4) ? TRUE : FALSE;

	if (vop_mode_ptr->bResyncMarkerDisable && !is_short_header)
	{
		rsc_fnd = FALSE;
		return rsc_fnd;
	}
	
	rsc_len = 17;	
	
	if ((vop_mode_ptr->VopPredType == PVOP) && !is_short_header)
	{
		rsc_len += (vop_mode_ptr->mvInfoForward.FCode - 1);
	}
	
	Mp4Dec_ByteAlign_Startcode ();

	while (1)
	{
		nbits_dec_total = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg");

		dec_len = nbits_dec_total >> 3;

		/*searching to frame end*/
		if (dec_len >= vop_mode_ptr->frame_len + 3)
		{
			rsc_fnd = FALSE;
			break;
		}

		rsc_fnd = (Mp4Dec_ShowBits (rsc_len) == DEC_RESYNC_MARKER) ? TRUE : FALSE;

		if (rsc_fnd)
		{
			break;
		}
		else
		{
			Mp4Dec_ReadBits (8);
		}
	}	
	
	return rsc_fnd;
}

void ExchangeMBMode (DEC_VOP_MODE_T * vop_mode_ptr)
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
PUBLIC void Mp4Dec_DecIVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T *mb_mode_ptr = vop_mode_ptr->pMbMode;
	BOOLEAN is_itu_h263 = (ITU_H263 == vop_mode_ptr->video_std)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;

	if(FLV_H263 == vop_mode_ptr->video_std)
	{
		DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

		mb_cache_ptr->iDcScalerY = 8;
		mb_cache_ptr->iDcScalerC = 8;
	}
	      
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
			if((pos_y == 0x00)&&(pos_x == 2)&&(g_nFrame_dec == 0))
			{
				foo();
			}
		#endif //_DEBUG_
		#if _TRACE_			
			FPRINTF (g_fp_trace_fw, "\nnframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
		#endif //_TRACE_	
		
			vop_mode_ptr->mb_x = (int8)pos_x;	

			/*if error founded, search next resync header*/
			if (vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->error_flag = FALSE;

				rsc_found = SearchResynCode (vop_mode_ptr);
				if (!rsc_found)
				{
					return;
				}
				else
				{					
					int32 error_mb_start;
					
					Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, 0);

					//set error mb status
					error_mb_start = pos_y * vop_mode_ptr->MBNumX + pos_x;
					for(; error_mb_start < vop_mode_ptr->mbnumDec; error_mb_start++)
					{
						mb_mode_ptr = vop_mode_ptr->pMbMode + error_mb_start;
						mb_mode_ptr->bIntra = FALSE; //let this mb not available in mb pred
					}

					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
				}

				if (is_itu_h263)
				{
					vop_mode_ptr->GobNum++;
					break;
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
						Mp4Dec_GetVideoPacketHeader(vop_mode_ptr,  0);
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
	
			Mp4Dec_VspMBInit(vop_mode_ptr);

			if(vop_mode_ptr->error_flag)
			{
				PRINTF("mbc error!\n");
				continue;
			}

			/*configure MBC and DBK*/
			Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_ptr);

			/*vld*/
			Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr);

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				Mp4Dec_CheckMBCStatus(vop_mode_ptr);

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

//				if(vop_mode_ptr->error_flag)
//				{
//					vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = 0;
//					PRINTF ("\nerror detected by BMA!");
//					continue;
//				}
			}else
			{
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;

				PRINTF ("decode intra mb coeff error!\n");

				continue;
			}
							
			((int*)mb_mode_ptr->mv)[0] = 0;   //set to zero for B frame'mv prediction
			((int*)mb_mode_ptr->mv)[1] = 0;
			((int*)mb_mode_ptr->mv)[2] = 0;
			((int*)mb_mode_ptr->mv)[3] = 0;

			mb_mode_ptr++;
			vop_mode_ptr->mbnumDec++;
		}

		vop_mode_ptr->GobNum++;
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecPVOP
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecPVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T * mb_mode_ptr = NULL;//vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T  * mb_buf_ptr		= vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (vop_mode_ptr->video_std == ITU_H263)?1:0;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;

	ExchangeMBMode (vop_mode_ptr);
	mb_mode_ptr		= vop_mode_ptr->pMbMode;

	if(vop_mode_ptr->video_std == FLV_H263)
	{
		mb_cache_ptr->iDcScalerY = 8;
		mb_cache_ptr->iDcScalerC = 8;
	}
	
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
			if((pos_x == 8)&&(pos_y == 9)&&(g_nFrame_dec == 2))
			{
				foo();
			}
		#endif //_DEBUG_
		#if _TRACE_			
			FPRINTF (g_fp_trace_fw, "\nnframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
		#endif //_TRACE_		

			vop_mode_ptr->mb_x = (int8)pos_x;

			mb_mode_ptr = vop_mode_ptr->pMbMode + pos_y * vop_mode_ptr->MBNumX + pos_x;

			/*if error founded, search next resync header*/
			if (vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->error_flag = FALSE;

				rsc_found = SearchResynCode (vop_mode_ptr);
				if (!rsc_found)
				{
					return;
				}
				else
				{					
					Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);

					pos_x = vop_mode_ptr->mb_x;
					pos_y = vop_mode_ptr->mb_y;
					mb_mode_ptr = vop_mode_ptr->pMbMode + vop_mode_ptr->mbnumDec;
					mb_mode_ptr->bFirstMB_in_VP = TRUE;
					
					if (pos_x == 0)
					{
					    VSP_WRITE_REG (VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, ((pos_y-1)<<16)|(total_mb_num_x-1), "re-config dbk mb_x and mb_y");
					}else
					{
					    VSP_WRITE_REG (VSP_DBK_REG_BASE+DBK_SID_CFG_OFF, (pos_y<<4)|(pos_x-1), "re-config dbk mb_x and mb_y");
					}	
				}

				if (is_itu_h263)
				{
					vop_mode_ptr->GobNum++;
					break;
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
					Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
							
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

			mb_buf_ptr->start_pos = VSP_READ_REG(VSP_BSM_REG_BASE+BSM_TOTAL_BITS_OFF, "read BSMR_TOTAL_BITS reg") / 8;
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

			Mp4Dec_VspMBInit(vop_mode_ptr);
		
			if(vop_mode_ptr->error_flag)
			{
				PRINTF("mbc error!\n");
				continue;
			}

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
					MOTION_VECTOR_T zeroMv = {0, 0};

					mb_mode_ptr->dctMd = MODE_NOT_CODED;

					/*configure zero mv to mca*/
					Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, &zeroMv);	
				}else
				{
					/*vld*/
					Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_ptr);
					
					if(!vop_mode_ptr->error_flag)
					{
					    Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
					}
				}
			}

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				Mp4Dec_CheckMBCStatus(vop_mode_ptr);

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
			
//				if(vop_mode_ptr->error_flag)
//				{
//					vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = 0;
//					PRINTF ("\nerror detected by BMA!");
//					continue;
//				}
			}else
			{
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;

				PRINTF ("\ndecode inter mb of pVop error!");

				continue;
			}	

		// 	mb_mode_ptr++;
			
			vop_mode_ptr->mbnumDec++;	
		}

		vop_mode_ptr->GobNum++;
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecBVOP
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecBVOP(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	DEC_MB_MODE_T *mb_mode_bvop_ptr = vop_mode_ptr->pMbMode_B;
	DEC_MB_MODE_T *pCoMb_mode = vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	MOTION_VECTOR_T FwdPredMv, BckPredMv, zeroMv = {0, 0};
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;

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
					return;
				}

				if(Mp4Dec_CheckResyncMarker(vop_mode_ptr->mvInfoForward.FCode - 1))
				{
					Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);

					//reset prediction mv
					FwdPredMv = zeroMv; 
					BckPredMv = zeroMv;
				}			
			}

			mb_mode_bvop_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);

			Mp4Dec_VspMBInit(vop_mode_ptr);
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF("mbc error!\n");
				return;
			}

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
					return;
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
			}else
			{
				PRINTF ("decode inter mb of B-Vop error!\n");
				return;
			}
				
			pCoMb_mode++;
		}
	}
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
