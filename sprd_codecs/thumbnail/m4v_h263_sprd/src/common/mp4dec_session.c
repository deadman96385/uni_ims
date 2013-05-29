/******************************************************************************
 ** File Name:    mp4dec_session.c                                         *
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

PUBLIC void Mp4Dec_InitGlobal (void)
{	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InitDecoderPara
 ** Description:	Initialize the decode parameter. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_InitDecoderPara(VIDEO_DATA_T *vd)
{
	DEC_VOP_MODE_T *vop_mode_ptr = vd->vop_mode_ptr;

	vd->g_dec_is_first_frame = TRUE;
	vd->g_dec_is_stop_decode_vol = FALSE;
	vd->g_dec_is_changed_format = FALSE;
 	vd->g_dec_pre_vop_format = IVOP;
 
    	memset(vd->g_FrmYUVBfr,0,sizeof(vd->g_FrmYUVBfr));
	
	vop_mode_ptr->error_flag = FALSE;	
	vop_mode_ptr->return_pos = 0;
	vop_mode_ptr->return_pos1 = 0;
	vop_mode_ptr->return_pos2 = 0;
	vop_mode_ptr->VSP_used = 0;
	vop_mode_ptr->VT_used = 0;

	vop_mode_ptr->intra_acdc_pred_disable = TRUE;
	vop_mode_ptr->QuantizerType = Q_H263;
	vop_mode_ptr->bDataPartitioning = FALSE;
	vop_mode_ptr->bReversibleVlc = FALSE;
	vop_mode_ptr->bResyncMarkerDisable = TRUE;
	vop_mode_ptr->bAlternateScan = FALSE;
	vop_mode_ptr->bQuarter_pel = FALSE;
	vop_mode_ptr->bInterlace = FALSE;
	vop_mode_ptr->bInitSuceess = FALSE;
	vop_mode_ptr->is_need_init_vsp_quant_tab = FALSE;
	vop_mode_ptr->is_need_init_vsp_huff_tab = TRUE;
	vop_mode_ptr->is_first_frame = TRUE;
	vop_mode_ptr->is_previous_B_VOP = FALSE;
	vop_mode_ptr->is_work_mode_set = FALSE;

    vop_mode_ptr->pCurDispFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(vd, sizeof(Mp4DecStorablePic));
	vop_mode_ptr->pCurRecFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(vd, sizeof(Mp4DecStorablePic));
	vop_mode_ptr->pBckRefFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(vd, sizeof(Mp4DecStorablePic));
	vop_mode_ptr->pFrdRefFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(vd, sizeof(Mp4DecStorablePic));

	vop_mode_ptr->pCurDispFrame->bfrId = 0xFF;
	vop_mode_ptr->pCurRecFrame->bfrId = 0xFF;
	vop_mode_ptr->pBckRefFrame->bfrId = 0xFF;
	vop_mode_ptr->pFrdRefFrame->bfrId = 0xFF; 

	vd->g_firstBsm_init = TRUE;
	vop_mode_ptr->g_nFrame_dec = 0;

#if _CMODEL_
	g_stream_offset = 0;
	VSP_Init_CModel();
#endif //_CMODEL_
}

LOCAL uint8 Mp4Dec_Compute_log2(int32 uNum)
{
	uint8 logLen = 1;

	uNum -= 1;

	SCI_ASSERT(uNum >= 0);

	while( (uNum >>= 1) > 0 )
	{
		logLen++;
	}

	return logLen;		
}

LOCAL void Mp4Dec_InitHuffmanTable(DEC_VOP_MODE_T *vop_mode_ptr)
{
	vop_mode_ptr->pMCBPCtabintra = g_dec_mcbpc_tab_intra;
	vop_mode_ptr->pMCBPCtab = g_dec_mcbpc_tab;
	vop_mode_ptr->pCBPYtab  = g_dec_cbpy_tab;
	vop_mode_ptr->pTMNMVtab0 = g_dec_tmn_mv_tab0;
	vop_mode_ptr->pTMNMVtab1 = g_dec_tmn_mv_tab1;
	vop_mode_ptr->pTMNMVtab2 = g_dec_tmn_mv_tab2;

	vop_mode_ptr->pDCT3Dtab0 = g_dec_dct3d_tab0;
	vop_mode_ptr->pDCT3Dtab1 = g_dec_dct3d_tab1;
	vop_mode_ptr->pDCT3Dtab2 = g_dec_dct3d_tab2;
	vop_mode_ptr->pDCT3Dtab3 = g_dec_dct3d_tab3;
	vop_mode_ptr->pDCT3Dtab4 = g_dec_dct3d_tab4;
	vop_mode_ptr->pDCT3Dtab5 = g_dec_dct3d_tab5;

	vop_mode_ptr->pInter_max_level = g_inter_max_level;
	vop_mode_ptr->pInter_max_run = g_inter_max_run;

	vop_mode_ptr->pIntra_max_level = g_intra_max_level;
	vop_mode_ptr->pIntra_max_run = g_intra_max_run;
}

LOCAL MMDecRet Mp4Dec_MallocFrmBfr(VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
//	uint32 size;

    if( MMDEC_OK != Mp4Dec_InitYUVBfr(vd, vop_mode_ptr) )
    {
        return MMDEC_MEMORY_ERROR;
    }

	/*backward reference frame and forward reference frame after extention*/
	//all these address should be 64 word aligned
	vop_mode_ptr->pCurRecFrame->pDecFrame = &(vd->g_FrmYUVBfr[0]);  

    return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InitSessionDecode
 ** Description:	Initialize the decode parameter. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_InitSessionDecode(VideoDecControls *decCtrl, VIDEO_DATA_T *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	uint32 mb_num_x, mb_num_y, total_mb_num;
	uint32 i;

	/*MB number in hor and in ver and total MB number*/
	mb_num_x = vop_mode_ptr->MBNumX = (vop_mode_ptr->OrgFrameWidth  + 15)>>4;
	mb_num_y = vop_mode_ptr->MBNumY = (vop_mode_ptr->OrgFrameHeight + 15) >>4;
	total_mb_num = vop_mode_ptr->MBNum  = (int16)(mb_num_x * mb_num_y);

	vop_mode_ptr->FrameWidth  = (int16)(mb_num_x <<4);
	vop_mode_ptr->FrameHeight = (int16)(mb_num_y <<4);

    vop_mode_ptr->FrameExtendWidth  = vop_mode_ptr->FrameWidth  + 2 * YEXTENTION_SIZE;
	vop_mode_ptr->FrameExtendHeigth = vop_mode_ptr->FrameHeight + 2 * YEXTENTION_SIZE;

	vop_mode_ptr->iStartInFrameY = vop_mode_ptr->FrameExtendWidth * YEXTENTION_SIZE + YEXTENTION_SIZE;
	vop_mode_ptr->iStartInFrameUV = (vop_mode_ptr->FrameExtendWidth >>1) * UVEXTENTION_SIZE + UVEXTENTION_SIZE;

    vd->VSP_extMemCb(decCtrl, /*vd*/ vd->g_user_data, vop_mode_ptr->FrameWidth, vop_mode_ptr->FrameHeight);

    /*MB mode for a frame*/
    vop_mode_ptr->pMbMode = (DEC_MB_MODE_T *)Mp4Dec_ExtraMemCacheAlloc(vd, sizeof(DEC_MB_MODE_T) * total_mb_num);
    if(NULL == vop_mode_ptr->pMbMode)
    {
        return MMDEC_MEMORY_ERROR;
    }
	
    vop_mode_ptr->pMbMode_prev = (DEC_MB_MODE_T *)Mp4Dec_ExtraMemCacheAlloc(vd, sizeof(DEC_MB_MODE_T) * total_mb_num);
    if(NULL == vop_mode_ptr->pMbMode)
    {
        return MMDEC_MEMORY_ERROR;
    }

	if (vop_mode_ptr->VSP_used || vop_mode_ptr->VT_used)	
	{
#if 0	
	vop_mode_ptr->mbdec_stat_ptr = (uint8 *)Mp4Dec_ExtraMemCacheAlloc (vd, total_mb_num * sizeof(uint8));
		if(NULL == vop_mode_ptr->mbdec_stat_ptr)
		{
		    return MMDEC_MEMORY_ERROR;
		}
#endif		
	}
	
#ifdef _MP4CODEC_DATA_PARTITION_   
	if (vop_mode_ptr->bDataPartitioning)
	{
		uint32 i;

		vop_mode_ptr->g_dec_dc_store = (int32 **)Mp4Dec_ExtraMemCacheAlloc(vd, sizeof (int32 *) * total_mb_num);
		if( NULL == vop_mode_ptr->g_dec_dc_store )
		{
			return MMDEC_MEMORY_ERROR;
		}

		for (i = 0; i < total_mb_num; i++)
		{
			vop_mode_ptr->g_dec_dc_store[i] = (int32 *) Mp4Dec_ExtraMemCacheAlloc(vd, sizeof (int32) * 6);
			if(NULL == vop_mode_ptr->g_dec_dc_store[i])
			{
		                return MMDEC_MEMORY_ERROR;
			}
		}
	}
#endif //_MP4CODEC_DATA_PARTITION_	

	if (vop_mode_ptr->VSP_used)
	{
#if 0
	    //should be 64 word aligned
	    vop_mode_ptr->pTopCoeff = (int16 *)Mp4Dec_ExtraMemAlloc_64WordAlign( sizeof(int16) * (uint8)(vop_mode_ptr->MBNumX) * 4 * 8);
	    if(NULL == vop_mode_ptr->pTopCoeff)
	    {
	        return MMDEC_MEMORY_ERROR;
	    }

		vop_mode_ptr->frame_bistrm_ptr = (uint8 *)Mp4Dec_ExtraMemAlloc_64WordAlign( sizeof(uint8) * MP4DEC_FRM_STRM_BUF_SIZE);
		
	//for sw vld
	    vop_mode_ptr->pTopLeftDCLine = (int16 *)Mp4Dec_ExtraMemCacheAlloc(vd, sizeof(int16)* mb_num_x * 4);
	    if(NULL == vop_mode_ptr->pTopLeftDCLine)
	    {
	        return MMDEC_MEMORY_ERROR;
	    }
#endif		
	}else
	{
		vop_mode_ptr->pTopCoeff = (int16 *)Mp4Dec_ExtraMemCacheAlloc( vd, sizeof(int16) * (uint8)(vop_mode_ptr->MBNumX) * 4 * 8);
	    	SCI_ASSERT(NULL != vop_mode_ptr->pTopCoeff);

		vop_mode_ptr->mb_cache_ptr->pMBBfrY = (int8 *)Mp4Dec_ExtraMemCacheAlloc_64WordAlign(vd, sizeof(int8) * BLOCK_SQUARE_SIZE*4 );
   		SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr->pMBBfrY);
	
   		vop_mode_ptr->mb_cache_ptr->pMBBfrU = (int8 *)Mp4Dec_ExtraMemCacheAlloc_64WordAlign(vd, sizeof(int8) * BLOCK_SQUARE_SIZE );
   		SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr->pMBBfrU);
	
   		vop_mode_ptr->mb_cache_ptr->pMBBfrV = (int8 *)Mp4Dec_ExtraMemCacheAlloc_64WordAlign(vd, sizeof(int8) * BLOCK_SQUARE_SIZE );
   		SCI_ASSERT(NULL != vop_mode_ptr->mb_cache_ptr->pMBBfrV);
	}

	vop_mode_ptr->pLeftCoeff = (int16 *)Mp4Dec_ExtraMemCacheAlloc(vd, sizeof(int16) * 4 * 8);
	SCI_ASSERT(NULL != vop_mode_ptr->pLeftCoeff );
	
	for (i = 0; i < BLOCK_CNT; i++)
	{
		vop_mode_ptr->coef_block[i] = (int16 *)Mp4Dec_ExtraMemCacheAlloc(vd, sizeof(int16) * BLOCK_SQUARE_SIZE);
		SCI_ASSERT(NULL != vop_mode_ptr->coef_block[i]);

		if (i < 4)
		{
			mb_cache_ptr->blk8x8_offset[i] = (((i >> 1) *vop_mode_ptr->FrameExtendWidth+ (i & 1) )<<3);
		}
	}
	   
	vop_mode_ptr->MB_in_VOP_length = Mp4Dec_Compute_log2(vop_mode_ptr->MBNum);

    //--open or close the deblock filter.
#ifdef _VSP_LINUX_
	if(vop_mode_ptr->VT_used)
	{
		vop_mode_ptr->post_filter_en = FALSE;
	}
#else
    
    if (vop_mode_ptr->FrameWidth > 512)
    {
        vop_mode_ptr->post_filter_en = FALSE;
    }else
    {
        vop_mode_ptr->post_filter_en = TRUE;
    }
#endif	

	Mp4Dec_InitHuffmanTable(vop_mode_ptr);

    if( MMDEC_OK != Mp4Dec_MallocFrmBfr(vd, vop_mode_ptr) )
    {
        return MMDEC_MEMORY_ERROR;
    }

	if (vop_mode_ptr->VSP_used)
	{
#if 0	
		vop_mode_ptr->pStandardZigzag = g_standard_zigzag_normal;
		g_Mp4Dec_BVOP = Mp4Dec_DecBVOP_hw;
	#ifdef _MP4CODEC_DATA_PARTITION_
		if (vop_mode_ptr->bDataPartitioning)
		{
			vd->g_Mp4Dec_IVOP = Mp4Dec_DecIVOPErrResDataPartitioning;
			vd->g_Mp4Dec_PVOP = Mp4Dec_DecPVOPErrResDataPartitioning;
		}else
	#endif	
		{
			vd->g_Mp4Dec_IVOP = Mp4Dec_DecIVOP_hw;
			vd->g_Mp4Dec_PVOP = Mp4Dec_DecPVOP_hw;
		}	

	//	if(vop_mode_ptr->VSP_pipeline)
	//	{
	//		vd->g_Mp4Dec_Frm_Level_Sync =Mp4Dec_frm_level_sync_hw_sw_pipeline ;
	//	}else
	//	{
	//		vd->g_Mp4Dec_Frm_Level_Sync =Mp4Dec_frm_level_sync_hw_sw_normal ;
	//	}		
#endif	
	}else if(vop_mode_ptr->VT_used)
	{
		vop_mode_ptr->post_filter_en = FALSE;
		vop_mode_ptr->pStandardZigzag = g_standard_zigzag_trans;
		vd->g_Mp4Dec_BVOP = Mp4Dec_DecBVOP_vt; 
		if(vop_mode_ptr->bDataPartitioning)
		{			
			vd->g_Mp4Dec_IVOP = Mp4Dec_DecIVOPErrResDataPartitioning;
			vd->g_Mp4Dec_PVOP = Mp4Dec_DecPVOPErrResDataPartitioning;	
			
		}else
		{
			vd->g_Mp4Dec_IVOP = Mp4Dec_DecIVOP_vt;
			vd->g_Mp4Dec_PVOP = Mp4Dec_DecPVOP_vt;			
		}
	}else
	{
		vop_mode_ptr->post_filter_en = FALSE;
		vop_mode_ptr->pStandardZigzag = g_standard_zigzag_trans;
		vd->g_Mp4Dec_IVOP = Mp4Dec_DecIVOP_sw;
		vd->g_Mp4Dec_PVOP = Mp4Dec_DecPVOP_sw;
		vd->g_Mp4Dec_BVOP = Mp4Dec_DecBVOP_sw;
	}
	
	vop_mode_ptr->pVerticalZigzag = g_vertical_scan;
	vop_mode_ptr->pHorizontalZigzag = g_horizontal_scan;
	vop_mode_ptr->pZigzag = vop_mode_ptr->pStandardZigzag;	

	if(MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Dec_Compute_log2(vop_mode_ptr->time_inc_resolution);
		if (vop_mode_ptr->VSP_used)
		{
#if 0		
			vd->g_Mp4Dec_GetIntraBlkTCoef_hw = Mp4Dec_GetIntraBlkTCoef_Mp4_hw;
//			vd->g_Mp4Dec_GetInterBlkTCoef_hw = Mp4Dec_VlcDecInterTCOEF_Mpeg_hw;
#endif
		}else
		{
			vd->g_Mp4Dec_GetIntraBlkTCoef_sw = Mp4Dec_GetIntraBlkTCoef_Mp4_sw;
			vd->g_Mp4Dec_GetInterBlkTCoef_sw = Mp4Dec_VlcDecInterTCOEF_Mpeg_sw;
		}
	}else
	{
		vop_mode_ptr->mb_cache_ptr->iDcScalerY = 8;
		vop_mode_ptr->mb_cache_ptr->iDcScalerC = 8;

		if (vop_mode_ptr->VSP_used)
		{
#if 0
			vd->g_Mp4Dec_GetIntraBlkTCoef_hw = Mp4Dec_GetIntraBlkTCoef_H263_hw;
//			vd->g_Mp4Dec_GetInterBlkTCoef_hw = Mp4Dec_VlcDecInterTCOEF_H263_hw;
#endif
		}else
		{
			vd->g_Mp4Dec_GetIntraBlkTCoef_sw = Mp4Dec_GetIntraBlkTCoef_H263_sw;
			vd->g_Mp4Dec_GetInterBlkTCoef_sw = Mp4Dec_VlcDecInterTCOEF_H263_sw;
		}
	}

	/*for B frame support*/
	vop_mode_ptr->time = vop_mode_ptr->time_base = vop_mode_ptr->last_time_base = 0;
	vop_mode_ptr->last_non_b_time = 0;

	if (vop_mode_ptr->VSP_used)
	{
#if 0	
		if(Q_MPEG == vop_mode_ptr->QuantizerType)
		{
			vop_mode_ptr->intra_dct_cfg =  0x180;//((1 << 8 ) | ((int32)(!0) << 7) | (0 << 6) | (0 << 5)	| (0 << 4) | (0 << 1) |(0 << 0));
			vop_mode_ptr->inter_dct_cfg =  0x1a0;//((1 << 8 ) | ((int)(!0) << 7) | (0 << 6) | (1 << 5) | (0 << 4) | (0 << 1) |(0 << 0));
		}else
		{
			vop_mode_ptr->intra_dct_cfg =  0x150;//((1 << 8 ) | ((int32)(!1) << 7) | (1 << 6) | (0 << 5)	| (1 << 4) | (0 << 1) |(0 << 0));;
			vop_mode_ptr->inter_dct_cfg =  0x170;//((1 << 8 ) | ((int)(!1) << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (0 << 1) |(0 << 0));
		}

#if 0
	        g_cmd_data_base = (uint32 *) Mp4Dec_ExtraMemCacheAlloc_64WordAlign(vd, 3*(vop_mode_ptr->MBNum)*256);
		//g_cmd_info_base = (uint32 *) Mp4Dec_ExtraMemCacheAlloc_64WordAlign(vd, (vop_mode_ptr->MBNum)*256);
		//g_vsp_cmd_data_base = (uint32 *) Mp4Dec_ExtraMemAlloc_64WordAlign(vd, 3*(vop_mode_ptr->MBNum)*256);
		//g_vsp_cmd_info_base = (uint32 *) Mp4Dec_ExtraMemAlloc_64WordAlign(vd, (vop_mode_ptr->MBNum)*256);
		if(NULL == g_cmd_data_base)
		{
		    return MMDEC_MEMORY_ERROR;
		}
		g_cmd_info_base = (uint32 *) Mp4Dec_ExtraMemCacheAlloc_64WordAlign(vd, (vop_mode_ptr->MBNum)*256);
			//SCI_TRACE_LOW("g_cmd_info_base : %x\n", g_cmd_info_base);
		if(NULL == g_cmd_info_base)
		{
		    return MMDEC_MEMORY_ERROR;
		}
#else
		for(i = 0; i< 2; i++)
		{
			vop_mode_ptr->cmd_data_buf[i]= (uint32 *) Mp4Dec_ExtraMemAlloc_64WordAlign(3*(vop_mode_ptr->MBNum)*256);

			if(NULL == vop_mode_ptr->cmd_data_buf[i])
			{
			    return MMDEC_MEMORY_ERROR;
			}
			vop_mode_ptr->cmd_info_buf[i] = (uint32 *) Mp4Dec_ExtraMemAlloc_64WordAlign((vop_mode_ptr->MBNum)*256);
			if(NULL == vop_mode_ptr->cmd_info_buf[i])
			{
			    return MMDEC_MEMORY_ERROR;
			}
		}
	
#endif

#if 0
		g_vsp_cmd_data_base = (uint32 *) Mp4Dec_ExtraMemAlloc_64WordAlign(3*(vop_mode_ptr->MBNum)*256);
			//SCI_TRACE_LOW("g_vsp_cmd_data_base : %x\n", g_vsp_cmd_data_base);
		if(NULL == g_vsp_cmd_data_base)
		{
		    return MMDEC_MEMORY_ERROR;
		}
		g_vsp_cmd_info_base = (uint32 *) Mp4Dec_ExtraMemAlloc_64WordAlign((vop_mode_ptr->MBNum)*256);
			//SCI_TRACE_LOW("g_vsp_cmd_info_base : %x\n", g_vsp_cmd_info_base);
		if(NULL == g_vsp_cmd_info_base)
		{
		    return MMDEC_MEMORY_ERROR;
		}
#endif

#endif
	}

	return MMDEC_OK;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
