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

PUBLIC void Mp4Dec_exit_picture(DEC_VOP_MODE_T *vop_mode_ptr)
{	
	/*reorder reference frame list*/
	if(vop_mode_ptr->VopPredType != BVOP)
	{
		Mp4DecStorablePic * pframetmp;

		vop_mode_ptr->pCurRecFrame->pDecFrame->bRef = TRUE;
#ifdef _VSP_LINUX_
		if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader!=NULL))
		{
			(*VSP_bindCb)(g_user_data,vop_mode_ptr->pCurRecFrame->pDecFrame->pBufferHeader,0);
		}
#endif

		/*the buffer for forward reference frame will no longer be a reference frame*/		
		if(vop_mode_ptr->pFrdRefFrame->pDecFrame != NULL)
		{
#ifdef _VSP_LINUX_
			if((!vop_mode_ptr->post_filter_en)&&(vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader!=NULL))
			{
				(*VSP_unbindCb)(g_user_data,vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader,0);
				vop_mode_ptr->pFrdRefFrame->pDecFrame->pBufferHeader = NULL;
			}
#endif		

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
	uint32 i;
	uint32 uTmp;
	uint32 pTableAddr = (uint32)MP4DEC_IQUANT_TBL_ADDR;
	uint8 *pTable = vop_mode_ptr->IntraQuantizerMatrix;
	
	open_vsp_iram();	

	/*the quantization matrix*/
	for(i = 0; i < BLOCK_SQUARE_SIZE-1; i += 2)
	{
		uTmp = ((pTable[ASIC_DCT_Matrix[i+1]] << 16) | (pTable[ASIC_DCT_Matrix[i]]));
		VSP_WRITE_REG(pTableAddr+ i * 2, (uTmp),"MP4DEC_IQUANT_TBL_ADDR: configure intra quant table");
	}
	
	pTable = vop_mode_ptr->InterQuantizerMatrix;
	pTableAddr = MP4DEC_IQUANT_TBL_ADDR + BLOCK_SQUARE_SIZE * 2;
	
	for(i = 0; i < BLOCK_SQUARE_SIZE-1; i += 2)
	{
		uTmp = ((pTable[ASIC_DCT_Matrix[i+1]] << 16) | (pTable[ASIC_DCT_Matrix[i]]));
		VSP_WRITE_REG(pTableAddr+ i * 2, (uTmp),"MP4DEC_IQUANT_TBL_ADDR: configure inter quant table");
	}

	close_vsp_iram();

	return;
}

PUBLIC MMDecRet Mp4Dec_VspFrameInit(DEC_VOP_MODE_T *vop_mode_ptr, uint32 bitstm_addr)
{
	uint32 cmd;
	uint32 pTableAddr = (uint32)VSP_MEMO10_ADDR;
	uint32 phy_addr;

	open_vsp_iram();

#if _CMODEL_|_FW_TEST_VECTOR_
	//configure command infor and data address into register, and enable cmd-exe
	VSP_WRITE_REG(pTableAddr+ 20*4, ((uint32)CMD_CONTROL_INFO_ADDR)>>2, "config CMD control info buffer address");
	VSP_WRITE_REG(pTableAddr+ 21*4, ((uint32)CMD_CONTROL_DATA_ADDR)>>2, "config CMD control data buffer address");
//	VSP_WRITE_REG(pTableAddr+ 2*4, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#else //actual envioronment
        phy_addr = Mp4Dec_ExtraMem_V2Phy(g_cmd_info_base);
	VSP_WRITE_REG(pTableAddr+ 20*4, MAPaddr(phy_addr), "config CMD control info buffer address");
	phy_addr = Mp4Dec_ExtraMem_V2Phy(g_cmd_data_base);
	VSP_WRITE_REG(pTableAddr+ 21*4, MAPaddr(phy_addr), "config CMD control data buffer address");
//	VSP_WRITE_REG(pTableAddr+ 2*4, MAPaddr((uint32)bitstm_addr), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#endif


#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#else //actual envioronment
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, MAPaddr((uint32)(bitstm_addr)), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#endif


	/*init AHBM, current frame, forward reference frame, backward reference frame*/	
	VSP_WRITE_REG(pTableAddr+ 0, MAPaddr(vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr),"configure reconstruct frame Y");
	if (vop_mode_ptr->post_filter_en)
	{
		VSP_WRITE_REG(pTableAddr+ 1*4, MAPaddr(vop_mode_ptr->pCurDispFrame->pDecFrame->imgYAddr), "configure display frame Y");
	}else
	{
		VSP_WRITE_REG(pTableAddr+ 1*4, MAPaddr(vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr), "configure display frame Y");
	}

#if _CMODEL_
	VSP_WRITE_REG(pTableAddr+ 3*4, ((uint32)DC_AC_PREDICTION_ADDR)>>2, "config top coeff address");
#else
	phy_addr = Mp4Dec_ExtraMem_V2Phy(vop_mode_ptr->pTopCoeff);
	VSP_WRITE_REG(pTableAddr+ 3*4, MAPaddr(phy_addr), "config top coeff address");	
	//VSP_WRITE_REG(pTableAddr+ 3*4, MAPaddr((uint32)(vop_mode_ptr->pTopCoeff)), "config top coeff address");	
#endif 
	
	if(IVOP != vop_mode_ptr->VopPredType)
	{
		if (vop_mode_ptr->pBckRefFrame->pDecFrame == NULL)
		{
			if (!vop_mode_ptr->post_filter_en)
			{
				VSP_WRITE_REG(pTableAddr+ 5*4, MAPaddr(vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr), "configure backward reference frame Y");
#ifdef _DEBUG_
				SCI_TRACE_LOW("xx g_nFrame_dec %d ,pBckRefFrame %x",g_nFrame_dec,vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr);
#endif
			}else
			{
				return MMDEC_ERROR;
			}
		}else
		{
			VSP_WRITE_REG(pTableAddr+ 5*4, MAPaddr(vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr), "configure backward reference frame Y");
#ifdef _DEBUG_
			SCI_TRACE_LOW("g_nFrame_dec %d ,pBckRefFrame %x",g_nFrame_dec,vop_mode_ptr->pBckRefFrame->pDecFrame->imgYAddr);
#endif
		}
		
		if(BVOP == vop_mode_ptr->VopPredType)
		{
			if (vop_mode_ptr->pFrdRefFrame->pDecFrame == NULL)
			{
				if (!vop_mode_ptr->post_filter_en)
				{			
					VSP_WRITE_REG(pTableAddr + 4*4, MAPaddr(vop_mode_ptr->pCurRecFrame->pDecFrame->imgYAddr), "configure forward reference frame Y");
				}else
				{
					return MMDEC_ERROR;
				}
			}else
			{
				VSP_WRITE_REG(pTableAddr + 4*4,MAPaddr( vop_mode_ptr->pFrdRefFrame->pDecFrame->imgYAddr), "configure forward reference frame Y");
			}
		}
	}
		
	close_vsp_iram();

#if _CMODEL_ //for RTL simulation
//	cmd |= (((352*288)>>2)<<4); //word unit
//	printf("cmd %x\n",cmd);
	cmd = ((vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight)>>2); //word unit
#else
          cmd = 0;
	cmd |= ((vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight)>>2); //word unit
#endif
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_UV_OFFSET_OFF, cmd, "configure yu offset");


	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AHBM_V_ADDR_OFFSET, cmd/4, "configure uv offset");
#if 0 //vu_interleaved
     	cmd = VSP_READ_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF,"read AXIM_GAP_ENDIAN_OFF");
    	cmd |= ((1<<21)|(1<<18));
	VSP_WRITE_REG(VSP_AXIM_REG_BASE+AXIM_GAP_ENDIAN_OFF,cmd,"write AXIM_GAP_ENDIAN_OFF,vu_interleaved");
#endif 


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

PUBLIC MMDecRet Mp4Dec_InitVop(DEC_VOP_MODE_T *vop_mode_ptr, MMDecInput *dec_input_ptr)
{
	MMDecRet ret = MMDEC_ERROR;

	vop_mode_ptr->sliceNumber	= 0;
	vop_mode_ptr->stop_decoding = FALSE;
	vop_mode_ptr->mbnumDec		= 0;
	vop_mode_ptr->error_flag	= FALSE;
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;
	vop_mode_ptr->frame_len		= dec_input_ptr->dataLen;
	vop_mode_ptr->err_num		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_left		= dec_input_ptr->err_pkt_num;
	vop_mode_ptr->err_pos_ptr	= dec_input_ptr->err_pkt_pos;
	vop_mode_ptr->err_MB_num	= vop_mode_ptr->MBNum;

	open_vsp_iram();
#if _CMODEL_|_FW_TEST_VECTOR_
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, BIT_STREAM_DEC_0,"AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#else //actual envioronment
	//VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, MAPaddr((uint32)(dec_input_ptr->pStream)), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 8, MAPaddr((uint32)(dec_input_ptr->pStream_phy)), "AHBM_FRM_ADDR_6: source bitstream buffer0 address");
#endif
	close_vsp_iram();

	//set all mb in one vop to not-decoded
	memset (vop_mode_ptr->mbdec_stat_ptr, 0, vop_mode_ptr->MBNum);
	Mp4Dec_VspFrameInit(vop_mode_ptr,dec_input_ptr->pStream_phy);
#if 0	//disable it for avoid green block effect in real environment.
{
	int size = vop_mode_ptr->FrameWidth * vop_mode_ptr->FrameHeight;
//	if (dec_input_ptr->err_pkt_num != 0) //for error frame, only for debug
	{
		memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgY, 0xff, size);
		if (vop_mode_ptr->uv_interleaved)
		{
			memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, 0xff, size/2);
		}else
		{
			memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgU, 0xff, size/4);
			memset (vop_mode_ptr->pCurDispFrame->pDecFrame->imgV, 0xff, size/4);
		}
	}
}
#endif

	return ret;
}

PUBLIC void Mp4Dec_Reset (DEC_VOP_MODE_T * vop_mode_ptr)
{
	uint32 cmd;

	
	VSP_Reset();

    /*clear time_out int_raw flag, if timeout occurs*/
    VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_INT_CLR_OFF, V_BIT_15, "DCAM_INT_CLR: clear cmd_done int_raw flag");
        	
    /*init dcam command*/
    VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_CFG_OFF, (0<<4) | (1<<3), "DCAM_CFG: configure DCAM register,switch buffer to hardware");

    cmd = (1 << 16) |((uint32)0xffff);
    VSP_WRITE_REG(VSP_DCAM_REG_BASE+DCAM_VSP_TIME_OUT_OFF, cmd, "DCAM_VSP_TIME_OUT: enable hardware timer out");

#if 0//defined(_VSP_) 
	//enable dcam interrupt
    *(volatile uint32 *)0x40004008 |= (1<<7);	//In 8800G, the interrupt enable bit is BIT23. 

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
			if((pos_y == 1)&&(pos_x == 4)&&(g_nFrame_dec == 0))
			{
				foo();
			}
		#endif //_DEBUG_
		#if _TRACE_			
			SCI_TRACE_LOW( "nframe: %d, Y: %d, X: %d\n", g_nFrame_dec, pos_y, pos_x);
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
			Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr);

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

	//		if (pos_y == 4)		exit(0);
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
	
    //SCI_TRACE_LOW("Mp4Dec_DecPVOP in");
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
				Mp4Dec_DecIntraMBTexture(vop_mode_ptr, mb_mode_ptr);
			}else
			{	
				if(mb_mode_ptr->bSkip)
				{	
					mb_mode_ptr->dctMd = MODE_NOT_CODED;
					Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
				}else
				{
					Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
					Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_ptr);
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
    //SCI_TRACE_LOW("Mp4Dec_DecPVOP out");
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
		#if  _DEBUG_
			if((pos_y == 3)&&(pos_x == 8)&&(g_nFrame_dec == 2))
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
			
			//set mv range
			vop_mode_ptr->mv_x_max = (total_mb_num_x - pos_x) << 5;
			vop_mode_ptr->mv_x_min = (-pos_x - 1) << 5;
			vop_mode_ptr->mv_y_max = (vop_mode_ptr->MBNumY - pos_y) << 5; 
			vop_mode_ptr->mv_y_min = (-pos_y - 1) << 5;
			
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
			Mp4Dec_DecInterMBTexture(vop_mode_ptr, mb_mode_bvop_ptr);

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
