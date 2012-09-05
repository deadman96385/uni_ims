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
PUBLIC MMDecRet Mp4Dec_frm_level_sync_hw_sw_pipeline (MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
#if 0
	uint32 cmd;
	MMDecRet ret = MMDEC_OK;

#if _DEBUG_			
//	SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_pipeline: frm_num%d\n", g_nFrame_dec); 
#endif

	if (g_nFrame_dec == 0)
	{
		g_cmd_done_init = 1;
	}else
	{
		ret = VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), (TIME_OUT_CLK), "DCAM_INT_STS_OFF: polling CMD DONE initerupt");

		if (ret)
		{
			VSP_Reset();
			SCI_TRACE_LOW("Mp4Dec_frm_level_sync_hw_sw_pipeline, cmd NOT done");
		}else
		{
			uint32 disp_y_addr;
			uint32 uv_offset;
			
			g_cmd_done_init = 1;//!ret;

			Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);
		}
	}

	if(g_cmd_done_init)
	{
		g_cmd_done_init = 0;

		memcpy (vop_mode_ptr->frame_bistrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);
		
		/*clear time_out int_raw flag, if timeout occurs*/
	    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");

		Mp4Dec_Reset(vop_mode_ptr);
		VSP_Write_CMD_by_BSM(((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_info_base)), ((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_data_base)));
		ret = Mp4Dec_VspFrameInit(vop_mode_ptr, ((uint32)(Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->frame_bistrm_ptr))));

		cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
		cmd |= (1<<8)|(1<<9);
		VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	
	}

	return ret;
#else
	uint32 cmd;
	MMDecRet ret = MMDEC_OK;
	
	SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_pipeline: Current frame type: %s\n", vop_mode_ptr->VopPredType == IVOP? "IVOP"  : vop_mode_ptr->VopPredType == PVOP ? "PVOP":"BVOP"); 
	
	if(!vop_mode_ptr->is_first_frame && !vop_mode_ptr->is_previous_B_VOP)
	{
		SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_pipeline: Wait for previous vsp done. Frame %d\n", g_nFrame_dec ==0 ? 0 : g_nFrame_dec -1); 
		VSP_START_CQM();			
	}
	
#if 0
	if(50 >g_nFrame_dec && 0< g_nFrame_dec)
//	if(g_nFrame_dec ==  1)
	{
		dump_file1(vop_mode_ptr->pBckRefFrame->pDecFrame->imgY,(vop_mode_ptr->FrameWidth*vop_mode_ptr->FrameHeight*3/2));
	}
#endif

	vop_mode_ptr->is_first_frame = FALSE;

	VSP_ACQUAIRE_Dev();

	memcpy(vop_mode_ptr->frame_bistrm_ptr,dec_input_ptr->pStream,dec_input_ptr->dataLen);
		
	Mp4Dec_Reset(vop_mode_ptr);
//	VSP_Write_CMD_by_BSM(((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_info_base)), ((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_data_base)));
	ret = Mp4Dec_VspFrameInit(vop_mode_ptr,((uint32)(Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->frame_bistrm_ptr))));

	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	cmd |= (1<<8)|(1<<9);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	

	if (vop_mode_ptr->VopPredType == BVOP)
	{
		SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_pipeline: Wait for current frame vsp done. Frame %d\n", g_nFrame_dec); 
		VSP_START_CQM();
		vop_mode_ptr->is_previous_B_VOP = TRUE;
	}
       else
	{
		vop_mode_ptr->is_previous_B_VOP = FALSE;
	}
	

	Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);	
	

	return MMDEC_OK;
#endif
}

PUBLIC MMDecRet Mp4Dec_frm_level_sync_hw_sw_normal (MMDecInput *dec_input_ptr, MMDecOutput *dec_output_ptr, DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 cmd;
	MMDecRet ret = MMDEC_OK;


	SCI_TRACE_LOW ("Mp4Dec_frm_level_sync_hw_sw_normal: frm_num%d\n", g_nFrame_dec); 


	VSP_ACQUAIRE_Dev();
	
	memcpy (vop_mode_ptr->frame_bistrm_ptr, dec_input_ptr->pStream, dec_input_ptr->dataLen);

	Mp4Dec_Reset(vop_mode_ptr);
//	VSP_Write_CMD_by_BSM(((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_info_base)), ((uint32)Mp4Dec_ExtraMem_V2Phy(g_vsp_cmd_data_base)));
	ret = Mp4Dec_VspFrameInit(vop_mode_ptr, ((uint32)(Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->frame_bistrm_ptr))));

	cmd = VSP_READ_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, "DCAM_CFG: readout DCAM CFG");
	cmd |= (1<<8)|(1<<9);
	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, cmd, "DCAM_CFG: enable cmd-exe");	

#if 0
#if	1//_CMODEL_
	VSP_READ_REG_POLL(VSP_DCAM_REG_BASE+DCAM_INT_STS_OFF, (1<<15), (1<<15), (TIME_OUT_CLK), "DCAM_INT_STS_OFF: polling CMD DONE initerupt");
#else
		do{
		}while(!g_cmd_done_init);

		g_cmd_done_init = 0;		
#endif	

   	 /*clear time_out int_raw flag, if timeout occurs*/
    	VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");
#else
	VSP_START_CQM();
#endif

	Mp4Dec_output_one_frame (dec_output_ptr, vop_mode_ptr);	

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIVOP
 ** Description:	Decode the IVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecIVOP_hw(DEC_VOP_MODE_T *vop_mode_ptr)
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
		//	SCI_TRACE_LOW( "nframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
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
					
				//	vop_mode_ptr->VopPredType = PVOP; //here, for use MCA hardware module
					ret = Mp4Dec_EC_FillMBGap (vop_mode_ptr);
				//	vop_mode_ptr->VopPredType = IVOP;  //here, set it's original value
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

			if((0 == pos_y) && (0 == pos_x))
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
			Mp4Dec_DecIntraMBTexture_hw(vop_mode_ptr, mb_mode_ptr);

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				Mp4Dec_CheckMBCStatus(vop_mode_ptr);
				
				vop_mode_ptr->err_MB_num--;
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_NOT_IN_ERR_PKT;
#if 1
				if (vop_mode_ptr->err_left != 0)
				{
					/*determine whether the mb is located in error domain*/
					mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;	
					err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;
					if (mb_end_pos >= err_start_pos)
					{
						vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_IN_ERR_PKT;
					}
				}
#endif				
			}else
			{
			//	if (vop_mode_ptr->video_std != FLV_H263)
				{
					vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
					PRINTF ("decode intra mb coeff error!\n");
					continue;
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
PUBLIC MMDecRet Mp4Dec_DecPVOP_hw(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 pos_y, pos_x;
	int32 rsc_found;
	DEC_MB_MODE_T * mb_mode_ptr = NULL;//vop_mode_ptr->pMbMode;
	DEC_MB_BFR_T  * mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	BOOLEAN is_itu_h263 = (vop_mode_ptr->video_std == ITU_H263)?1:0;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num_y = vop_mode_ptr->MBNumY;
	MMDecRet ret = MMDEC_OK;

	VSP_WRITE_REG_CQM(VSP_MCA_REG_BASE+MCA_CFG_OFF, vop_mode_ptr->RoundingControl << 1, "configure MCA register: MCA CFG");
	VSP_WRITE_CMD_INFO((VSP_MCA<< 29) | (1<<24) | MCA_CFG_WOFF);

	Mp4Dec_ExchangeMBMode (vop_mode_ptr);
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
			if((pos_x == 0)&&(pos_y == 0)&&(g_nFrame_dec == 1))
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
				}else
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

			mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);
			vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;

			Mp4Dec_DecInterMBHeader(vop_mode_ptr, mb_mode_ptr);
			if (vop_mode_ptr->error_flag)
			{
				PRINTF ("decode inter mb header error!\n");
				continue;
			}

			Mp4Dec_DecMV_hw(vop_mode_ptr, mb_mode_ptr);
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
				Mp4Dec_DecIntraMBTexture_hw(vop_mode_ptr, mb_mode_ptr);
			}else
			{	
				if(mb_mode_ptr->bSkip)
				{	
					mb_mode_ptr->dctMd = MODE_NOT_CODED;
					Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
				}else
				{
					Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
					Mp4Dec_DecInterMBTexture_hw(vop_mode_ptr, mb_mode_ptr);
				}
			}

			if(!vop_mode_ptr->error_flag)
			{
				int32 mb_end_pos;
				int32 err_start_pos;

				Mp4Dec_CheckMBCStatus(vop_mode_ptr);
				
				vop_mode_ptr->err_MB_num--;
				vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_NOT_IN_ERR_PKT;
#if 1
				if (vop_mode_ptr->err_left != 0)
				{
					/*determine whether the mb is located in error domain*/
					mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;		
					err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;
					if (mb_end_pos >= err_start_pos)
					{
						vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = DECODED_IN_ERR_PKT;
					}
				}
#endif				
			}else
			{
			//	if (vop_mode_ptr->video_std != FLV_H263)
				{
					vop_mode_ptr->mbdec_stat_ptr[vop_mode_ptr->mbnumDec] = NOT_DECODED;
					PRINTF ("\ndecode inter mb of pVop error!");
					continue;
				}					
			}	
			
			vop_mode_ptr->mbnumDec++;	
		}

		vop_mode_ptr->GobNum++;
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecBVOP_hw
 ** Description:	Decode the PVOP and error resilience is disable. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecBVOP_hw(DEC_VOP_MODE_T *vop_mode_ptr)
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
			if((pos_y == 0)&&(pos_x == 7)&&(g_nFrame_dec == 2))
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
					vop_mode_ptr->return_pos2 |= (1<<0);
					return MMDEC_STREAM_ERROR;
				}
			}
	
			if(mb_mode_bvop_ptr->bSkip)
			{	
				mb_mode_bvop_ptr->CBP = 0x00;
			}

			/*vld*/
			Mp4Dec_DecInterMBTexture_hw(vop_mode_ptr, mb_mode_bvop_ptr);

			if(!vop_mode_ptr->error_flag)
			{
				Mp4Dec_CheckMBCStatus(vop_mode_ptr);
			}else
			{
				PRINTF ("decode inter mb of B-Vop error!\n");
				vop_mode_ptr->return_pos |= (1<<0);
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
