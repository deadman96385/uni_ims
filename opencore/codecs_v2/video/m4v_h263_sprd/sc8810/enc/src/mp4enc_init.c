/******************************************************************************
 ** File Name:    mp4enc_init.c  		                                      *
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

PUBLIC void Mp4Enc_InitVolVopPara(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr, uint32 frame_rate)
{   	
	/*set to default value*/
	vol_mode_ptr->bNot8Bit					= FALSE;
	vol_mode_ptr->QuantPrecision			= 5;
	vol_mode_ptr->nBits					= 8;
	vol_mode_ptr->bCodeSequenceHead		= FALSE;
	vol_mode_ptr->ProfileAndLevel			= 0;	
	vol_mode_ptr->bRoundingControlDisable	= TRUE;
	vol_mode_ptr->InitialRoundingType		= 0;
	vol_mode_ptr->PbetweenI				= 28;//28;
	if (vop_mode_ptr->big_size_flag)
	{
		vol_mode_ptr->PbetweenI		= 10;
	}
	vol_mode_ptr->GOVperiod				= 0;
	vol_mode_ptr->bAllowSkippedPMBs		= 1;
	vol_mode_ptr->bReversibleVlc			= FALSE;
	vol_mode_ptr->bDataPartitioning		= 0;
	vol_mode_ptr->bVPBitTh					= FALSE;
	vol_mode_ptr->QuantizerType			= Q_H263;
	vol_mode_ptr->TemporalRate				= 1;
	vol_mode_ptr->bOriginalForME			= FALSE;
	vol_mode_ptr->bAdvPredDisable			= TRUE;
	vol_mode_ptr->ClockRate				= frame_rate;
	vol_mode_ptr->fAUsage					= RECTANGLE;
	vol_mode_ptr->FrameHz					= 15; //30; //modidied by lxw,@0807
	vol_mode_ptr->MVRadiusPerFrameAwayFromRef = 8;

	vop_mode_ptr->OrgFrameWidth			= vol_mode_ptr->VolWidth ;
	vop_mode_ptr->OrgFrameHeight			= vol_mode_ptr->VolHeight;  										  
	vop_mode_ptr->bInterlace				= FALSE;
	vop_mode_ptr->IntraDcSwitchThr			= 0; 
	
	if(vol_mode_ptr->short_video_header)
	{
		vop_mode_ptr->SearchRangeForward		= MAX_MV_Y_H263;	
	}else
	{
		vop_mode_ptr->SearchRangeForward		= MAX_MV_Y;	
	}
	
	vol_mode_ptr->bResyncMarkerDisable		= 0;//modified by lxw,@20090803
	vop_mode_ptr->mbline_num_slice			= 1;
	vop_mode_ptr->intra_mb_dis				= 300;
	vop_mode_ptr->StepI					= 10;
	vop_mode_ptr->StepP					= 10;

	vop_mode_ptr->bInitRCSuceess			= FALSE;
	vop_mode_ptr->is_need_init_vsp_huff_tab = TRUE;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitH263
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
MP4_LOCAL void Mp4Enc_InitH263(VOL_MODE_T  *vol_mode_ptr, ENC_VOP_MODE_T  *vop_mode_ptr)
{
	vol_mode_ptr->QuantizerType = Q_H263;
	
	vop_mode_ptr->GOBResync = TRUE;
	vop_mode_ptr->bAlternateScan = 0;

	g_enc_frame_skip_number = 0;
	
	switch(vop_mode_ptr->FrameWidth)
	{
	case 128:
		vop_mode_ptr->H263SrcFormat = 1;
		vop_mode_ptr->MBLineOneGOB  = 1;
		break;
	case 176:
		vop_mode_ptr->H263SrcFormat = 2;
		vop_mode_ptr->MBLineOneGOB  = 1;
		break;
	case 352:
		vop_mode_ptr->H263SrcFormat = 3;
		vop_mode_ptr->MBLineOneGOB  = 1;
		break;
	case 704:
		vop_mode_ptr->H263SrcFormat = 4;
		vop_mode_ptr->MBLineOneGOB  = 2;
		break;
#if 0 //dont support above CIF size.
	case 1408:
		vop_mode_ptr->H263SrcFormat = 5; 
		vop_mode_ptr->MBLineOneGOB  = 4;
		break;
#endif  //#if 0
	default:
		PRINTF ("Illegal format!\n");
		//exit(1);		
		vop_mode_ptr->error_flag = TRUE;
	}
}

/*****************************************************************************
 **	Name : 			Mp4Enc_SetMVInfo
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
MP4_LOCAL void Mp4Enc_SetMVInfo(void)
{
	ENC_VOP_MODE_T *vop_mode_ptr = Mp4Enc_GetVopmode();
	MV_INFO_T *mvInfoForward = &(vop_mode_ptr->mvInfoForward);

	if(vop_mode_ptr->SearchRangeForward <= 512)
	{
		mvInfoForward->FCode = ((vop_mode_ptr->SearchRangeForward-1)>>4) + 1;
	}else
	{
		mvInfoForward->FCode = 7;
	}
	
	mvInfoForward->Range = 1 << (mvInfoForward->FCode + 4);	
	mvInfoForward->ScaleFactor = 1 << (mvInfoForward->FCode - 1);//uiScaleFactor is f
	
	if((vop_mode_ptr->SearchRangeForward) == (mvInfoForward->Range >> 1))
	{
		vop_mode_ptr->SearchRangeForward--; // avoid out of range half pel	
	}
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitVOEncoder
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
void Mp4Enc_InitVOEncoder(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 clock_rate;
	uint32 num_bits_time_incr;

	Mp4Enc_SetMVInfo();
	
	clock_rate = vol_mode_ptr->ClockRate-1;
	
	if(clock_rate>0)
	{
		//compute how many bits to denote iClockRate
		for(num_bits_time_incr = 1; num_bits_time_incr < NUMBITS_TIME_RESOLUTION; num_bits_time_incr++)
		{	
			if(clock_rate == 1)			
			{
				break;
			}
			clock_rate = (clock_rate >> 1);
		}
	}else
	{
		num_bits_time_incr = 1;
	}

	vop_mode_ptr->time_inc_resolution_in_vol_length = num_bits_time_incr;

	vop_mode_ptr->RoundingControlEncSwitch = vol_mode_ptr->InitialRoundingType;
	
    if(!vol_mode_ptr->bRoundingControlDisable)
    {
		vop_mode_ptr->RoundingControlEncSwitch ^= 0x00000001;
    }
    vop_mode_ptr->RoundingControl = vop_mode_ptr->RoundingControlEncSwitch;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitHuffmanTable
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
MP4_LOCAL void Mp4Enc_InitHuffmanTable(ENC_VOP_MODE_T *vop_mode_ptr)
{
	vop_mode_ptr->pQuant_pa = g_quant_pa;
	vop_mode_ptr->pDC_scaler = g_DC_scaler;
 	vop_mode_ptr->pMcbpc_intra_tab = g_mcbpc_intra_tab;
	vop_mode_ptr->pCbpy_tab = g_cbpy_tab;
	vop_mode_ptr->pMcbpc_inter_tab = g_mcbpc_inter_tab;
	vop_mode_ptr->pMvtab = g_mv_tab;
}

MP4_LOCAL uint8 Mp4Enc_Compute_log2(int32 uNum)
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

PUBLIC void Mp4Enc_InitSession(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 total_mb_num_x;
	uint32 total_mb_num_y;

	/*MB number in hor and in ver and total MB number*/
	vop_mode_ptr->OrgFrameWidth = vol_mode_ptr->VolWidth;
	vop_mode_ptr->OrgFrameHeight = vol_mode_ptr->VolHeight;
	total_mb_num_x = vop_mode_ptr->MBNumX = (vol_mode_ptr->VolWidth  + 15) / MB_SIZE;
	total_mb_num_y = vop_mode_ptr->MBNumY = (vol_mode_ptr->VolHeight + 15) / MB_SIZE;
	vop_mode_ptr->MBNum  = total_mb_num_x * total_mb_num_y;
	vop_mode_ptr->FrameWidth  = total_mb_num_x * MB_SIZE;
	vop_mode_ptr->FrameHeight = total_mb_num_y * MB_SIZE;	

	if(vop_mode_ptr->short_video_header)
	{
		Mp4Enc_InitH263(vol_mode_ptr, vop_mode_ptr);
	}

	vop_mode_ptr->pMbModeCur = (ENC_MB_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_MB_MODE_T) * total_mb_num_x);
	vop_mode_ptr->pMbModeAbv = (ENC_MB_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_MB_MODE_T) * total_mb_num_x);	
	vop_mode_ptr->pMBCache = (ENC_MB_BFR_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_MB_BFR_T));

	vop_mode_ptr->pYUVSrcFrame = (Mp4EncStorablePic *)Mp4Enc_InterMemAlloc(sizeof(Mp4EncStorablePic));
	vop_mode_ptr->pYUVRecFrame = (Mp4EncStorablePic *)Mp4Enc_InterMemAlloc(sizeof(Mp4EncStorablePic));
	vop_mode_ptr->pYUVRefFrame = (Mp4EncStorablePic *)Mp4Enc_InterMemAlloc(sizeof(Mp4EncStorablePic));

#if defined(_SIMULATION_) 
{		
	/*backward reference frame and forward reference frame after extention*/
	int32 size = (vop_mode_ptr->FrameWidth) * (vop_mode_ptr->FrameHeight);

	vop_mode_ptr->pYUVSrcFrame->imgY = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size);
	if (!vop_mode_ptr->uv_interleaved)
	{
		vop_mode_ptr->pYUVSrcFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
		vop_mode_ptr->pYUVSrcFrame->imgV = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
	}else
	{
		vop_mode_ptr->pYUVSrcFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>1);
		vop_mode_ptr->pYUVSrcFrame->imgV = NULL;
	}

	vop_mode_ptr->pYUVSrcFrame->imgYAddr = (uint32)vop_mode_ptr->pYUVSrcFrame->imgY >> 8;
}
#endif
	
#if _CMODEL_
	vop_mode_ptr->pYUVSrcFrame->imgYAddr = SRC_FRAME_Y_ADDR >> 8;
#endif
	
	vop_mode_ptr->iMaxVal = 1<<(vol_mode_ptr->nBits+3);	
	vop_mode_ptr->QuantizerType = vol_mode_ptr->QuantizerType;
	vop_mode_ptr->intra_acdc_pred_disable = vol_mode_ptr->intra_acdc_pred_disable;
	vop_mode_ptr->short_video_header = vol_mode_ptr->short_video_header;
	vop_mode_ptr->QuantPrecision = vol_mode_ptr->QuantPrecision;
	
	vop_mode_ptr->MB_in_VOP_length = Mp4Enc_Compute_log2(vop_mode_ptr->MBNum);
	
	if(!vop_mode_ptr->short_video_header) //MPEG4
	{
		vop_mode_ptr->time_inc_resolution_in_vol_length = Mp4Enc_Compute_log2(vol_mode_ptr->ClockRate);
	}

	Mp4Enc_InitHuffmanTable(vop_mode_ptr);

	Mp4Enc_InitVOEncoder(vol_mode_ptr, vop_mode_ptr);

	//initialize the one frame bitstream buffer.
	vop_mode_ptr->pOneFrameBitstream = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(ONEFRAME_BITSTREAM_BFR_SIZE);  //allocate BITSTREAM_BFR_SIZE for one frame encoded bitstream.
	vop_mode_ptr->OneframeStreamLen = ONEFRAME_BITSTREAM_BFR_SIZE;

	if (vop_mode_ptr->big_size_flag)
	{
		g_EncPVOP = Mp4Enc_EncPVOP_BIG_SIZE;
	}else
	{
		g_EncPVOP = Mp4Enc_EncPVOP;
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
