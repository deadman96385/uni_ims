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
#include "sc8800g_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

PUBLIC void Mp4Dec_InitGlobal (void)
{	
	g_firstBsm_init = TRUE;
	g_nFrame_dec = 0;

#if _CMODEL_
	g_stream_offset = 0;
	VSP_Init_CModel();
#endif //_CMODEL_
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InitDecoderPara
 ** Description:	Initialize the decode parameter. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_InitDecoderPara(DEC_VOP_MODE_T *vop_mode_ptr)
{
	vop_mode_ptr->intra_acdc_pred_disable = TRUE;
	vop_mode_ptr->QuantizerType = Q_H263;
	vop_mode_ptr->bDataPartitioning = FALSE;
	vop_mode_ptr->bReversibleVlc = FALSE;
	vop_mode_ptr->bResyncMarkerDisable = TRUE;
	vop_mode_ptr->bAlternateScan = FALSE;
	vop_mode_ptr->bQuarter_pel = FALSE;
	vop_mode_ptr->bInterlace = FALSE;
	vop_mode_ptr->bInitSuceess = FALSE;

    vop_mode_ptr->pCurDispFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(sizeof(Mp4DecStorablePic));
	vop_mode_ptr->pCurRecFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(sizeof(Mp4DecStorablePic));
	vop_mode_ptr->pBckRefFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(sizeof(Mp4DecStorablePic));
	vop_mode_ptr->pFrdRefFrame = (Mp4DecStorablePic *)Mp4Dec_InterMemAlloc(sizeof(Mp4DecStorablePic));

	vop_mode_ptr->pCurDispFrame->bfrId = 0xFF;
	vop_mode_ptr->pCurRecFrame->bfrId = 0xFF;
	vop_mode_ptr->pBckRefFrame->bfrId = 0xFF;
	vop_mode_ptr->pFrdRefFrame->bfrId = 0xFF;
	vop_mode_ptr->mb_cache_ptr->iDcScalerY = 8;
	vop_mode_ptr->mb_cache_ptr->iDcScalerC = 8;

 	g_dec_pre_vop_format = IVOP;
 
    memset(g_FrmYUVBfr,0,sizeof(g_FrmYUVBfr));
 
}

LOCAL uint8 Mp4Dec_Compute_log2(int32 uNum)
{
	uint8 logLen = 1;

	uNum -= 1;

	SCI_ASSERT(uNum >= 0);/*assert verified*/

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
}

LOCAL MMDecRet Mp4Dec_MallocFrmBfr(DEC_VOP_MODE_T *vop_mode_ptr)
{
//	uint32 size;

    if( MMDEC_OK != Mp4Dec_InitYUVBfr(vop_mode_ptr) )
    {
        return MMDEC_MEMORY_ERROR;
    }

	/*backward reference frame and forward reference frame after extention*/
	//all these address should be 64 word aligned
	vop_mode_ptr->pCurRecFrame->pDecFrame = &g_FrmYUVBfr[0];  

    return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_InitSessionDecode
 ** Description:	Initialize the decode parameter. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_InitSessionDecode(DEC_VOP_MODE_T *vop_mode_ptr)
{
// 	uint32 i;//, j;
	uint32 mb_num_x;
	uint32 mb_num_y;
	uint32 total_mb_num;

	/*MB number in hor and in ver and total MB number*/
	mb_num_x = vop_mode_ptr->MBNumX = (vop_mode_ptr->OrgFrameWidth  + 15) / MB_SIZE;
	mb_num_y = vop_mode_ptr->MBNumY = (vop_mode_ptr->OrgFrameHeight + 15) / MB_SIZE;
	total_mb_num = vop_mode_ptr->MBNum  = (int16)(mb_num_x * mb_num_y);

	vop_mode_ptr->FrameWidth  = (int16)(mb_num_x * MB_SIZE);
	vop_mode_ptr->FrameHeight = (int16)(mb_num_y * MB_SIZE);
	
    /*MB mode for a frame*/
#ifdef _VSP_LINUX_
    vop_mode_ptr->pMbMode = (DEC_MB_MODE_T *)Mp4Dec_ExtraMemCacheAlloc(sizeof(DEC_MB_MODE_T) * total_mb_num);
#else
    vop_mode_ptr->pMbMode = (DEC_MB_MODE_T *)Mp4Dec_ExtraMemAlloc(sizeof(DEC_MB_MODE_T) * total_mb_num);
#endif
    if(NULL == vop_mode_ptr->pMbMode)
    {
        return MMDEC_MEMORY_ERROR;
    }
#ifdef _VSP_LINUX_	
    vop_mode_ptr->pMbMode_prev = (DEC_MB_MODE_T *)Mp4Dec_ExtraMemCacheAlloc(sizeof(DEC_MB_MODE_T) * total_mb_num);
#else
    vop_mode_ptr->pMbMode_prev = (DEC_MB_MODE_T *)Mp4Dec_ExtraMemAlloc(sizeof(DEC_MB_MODE_T) * total_mb_num);
#endif
    if(NULL == vop_mode_ptr->pMbMode_prev)
    {
        return MMDEC_MEMORY_ERROR;
    }
#ifdef _VSP_LINUX_
    vop_mode_ptr->mbdec_stat_ptr = (uint8 *)Mp4Dec_ExtraMemCacheAlloc ( (uint16)(vop_mode_ptr->MBNum) * sizeof(uint8));
#else
    vop_mode_ptr->mbdec_stat_ptr = (uint8 *)Mp4Dec_ExtraMemAlloc ( (uint16)(vop_mode_ptr->MBNum) * sizeof(uint8));
#endif
    if(NULL == vop_mode_ptr->mbdec_stat_ptr)
    {
        return MMDEC_MEMORY_ERROR;
    }

    //should be 64 word aligned
    vop_mode_ptr->pTopCoeff = (int16 *)Mp4Dec_ExtraMemAlloc_64WordAlign( sizeof(int16) * (uint8)(vop_mode_ptr->MBNumX) * 4 * 8);
    if(NULL == vop_mode_ptr->pTopCoeff)
    {
        return MMDEC_MEMORY_ERROR;
    }
	open_vsp_iram();
	VSP_WRITE_REG(VSP_MEMO10_ADDR+ 12, ((uint32)vop_mode_ptr->pTopCoeff)>>8, "AHBM_FRM_ADDR_7: config top coeff address");
	close_vsp_iram();	

#ifdef _VSP_LINUX_	
    vop_mode_ptr->pTopLeftDCLine = (int16 *)Mp4Dec_ExtraMemCacheAlloc( sizeof(int16)* (uint8)(vop_mode_ptr->MBNumX) * 3);
#else
    vop_mode_ptr->pTopLeftDCLine = (int16 *)Mp4Dec_ExtraMemAlloc( sizeof(int16)* (uint8)(vop_mode_ptr->MBNumX) * 3);
#endif

    if(NULL == vop_mode_ptr->pTopLeftDCLine)
    {
        return MMDEC_MEMORY_ERROR;
    }

#ifdef _MP4CODEC_DATA_PARTITION_		
    if (vop_mode_ptr->bDataPartitioning)
    {
		uint32 i;

#ifdef _VSP_LINUX_		
        g_dec_dc_store = (int32 **)Mp4Dec_ExtraMemCacheAlloc(sizeof (int32 *) * total_mb_num);
#else
        g_dec_dc_store = (int32 **)Mp4Dec_ExtraMemAlloc(sizeof (int32 *) * total_mb_num);
#endif
        if( NULL == g_dec_dc_store )
        {
            return MMDEC_MEMORY_ERROR;
        }

        for (i = 0; i < total_mb_num; i++)
        {
#ifdef _VSP_LINUX_	        
            g_dec_dc_store[i] = (int32 *) Mp4Dec_ExtraMemCacheAlloc(sizeof (int32) * 6);
#else
            g_dec_dc_store[i] = (int32 *) Mp4Dec_ExtraMemAlloc(sizeof (int32) * 6);
#endif
            if(NULL == g_dec_dc_store[i])
            {
                return MMDEC_MEMORY_ERROR;
            }
        }
    }
#endif //_MP4CODEC_DATA_PARTITION_

	vop_mode_ptr->MB_in_VOP_length = Mp4Dec_Compute_log2(vop_mode_ptr->MBNum);
	
	if(MPEG4 == vop_mode_ptr->video_std)
	{
		vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Dec_Compute_log2(vop_mode_ptr->time_inc_resolution);
	}
#ifdef _VSP_LINUX_
#else
    //--open or close the deblock filter.
    if (vop_mode_ptr->FrameWidth > 176)
    {
        vop_mode_ptr->post_filter_en = FALSE;
    }else
    {
        vop_mode_ptr->post_filter_en = TRUE;
    }
#endif
	Mp4Dec_InitHuffmanTable(vop_mode_ptr);

    if( MMDEC_OK != Mp4Dec_MallocFrmBfr(vop_mode_ptr) )
    {
        return MMDEC_MEMORY_ERROR;
    }

	/*for B frame support*/
	vop_mode_ptr->time = vop_mode_ptr->time_base = vop_mode_ptr->last_time_base = 0;
	vop_mode_ptr->last_non_b_time = 0;

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
