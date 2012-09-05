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
#include "sc8800g_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

MP4_LOCAL void Mp4Enc_SetMVInfo(void);
MP4_LOCAL void Mp4Enc_InitHuffmanTable(ENC_VOP_MODE_T *pVop_mode);
//MP4_LOCAL void Mp4Enc_InitBitCount(void);

PUBLIC void Mp4Enc_InitVolVopPara(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode, uint32 frame_rate)
{   	
	/*set to default value*/
	pVol_mode->bNot8Bit					= FALSE;
	pVol_mode->QuantPrecision			= 5;
	pVol_mode->nBits					= 8;
	pVol_mode->bCodeSequenceHead		= FALSE;
	pVol_mode->ProfileAndLevel			= 0;	
	pVol_mode->bRoundingControlDisable	= TRUE;
	pVol_mode->InitialRoundingType		= 0;
	pVol_mode->PbetweenI				= 300;//28;
	pVol_mode->GOVperiod				= 0;
	pVol_mode->bAllowSkippedPMBs		= 1;
	pVol_mode->bReversibleVlc			= FALSE;
	pVol_mode->bDataPartitioning		= 0;
	pVol_mode->bVPBitTh					= FALSE;
	pVol_mode->QuantizerType			= Q_H263;
	pVol_mode->TemporalRate				= 1;
	pVol_mode->bOriginalForME			= FALSE;
	pVol_mode->bAdvPredDisable			= TRUE;
	pVol_mode->ClockRate				= frame_rate;
	pVol_mode->fAUsage					= RECTANGLE;
	pVol_mode->FrameHz					= 15; //30; //modidied by lxw,@0807
	pVol_mode->MVRadiusPerFrameAwayFromRef = 8;

	pVop_mode->OrgFrameWidth			= pVol_mode->VolWidth ;
	pVop_mode->OrgFrameHeight			= pVol_mode->VolHeight;  										  
	pVop_mode->bInterlace				= FALSE;
	pVop_mode->IntraDcSwitchThr			= 0; 
	
	if(pVol_mode->short_video_header)
	{
		pVop_mode->SearchRangeForward		= MAX_MV_Y_H263;	
	}else
	{
		pVop_mode->SearchRangeForward		= MAX_MV_Y;	
	}
	

	pVol_mode->bResyncMarkerDisable		= 0;//modified by lxw,@20090803
	pVop_mode->mbline_num_slice			= 1;
	pVop_mode->intra_mb_dis				= 300;
	pVop_mode->StepI					= 10;
	pVop_mode->StepP					= 10;

	pVop_mode->bInitRCSuceess			= FALSE;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitH263
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
static void Mp4Enc_InitH263(VOL_MODE_T  *pVol_mode, ENC_VOP_MODE_T  *pVop_mode)
{
	pVol_mode->QuantizerType = Q_H263;
	
	pVop_mode->GOBResync = TRUE;
	pVop_mode->bAlternateScan = 0;

	g_enc_frame_skip_number = 0;
	
	switch(pVop_mode->FrameWidth)
	{
	case 128:
		pVop_mode->H263SrcFormat = 1;
		pVop_mode->MBLineOneGOB  = 1;
		break;
	case 176:
		pVop_mode->H263SrcFormat = 2;
		pVop_mode->MBLineOneGOB  = 1;
		break;
	case 352:
		pVop_mode->H263SrcFormat = 3;
		pVop_mode->MBLineOneGOB  = 1;
		break;
#if 0 //dont support above CIF size.
	case 704:
		pVop_mode->H263SrcFormat = 4;
		pVop_mode->MBLineOneGOB  = 2;
		break;
	case 1408:
		pVop_mode->H263SrcFormat = 5; 
		pVop_mode->MBLineOneGOB  = 4;
		break;
#endif  //#if 0
	default:
		PRINTF ("Illegal format!\n");
		//exit(1);		
		pVop_mode->error_flag = TRUE;
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
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();
	MV_INFO_T *mvInfoForward = &(pVop_mode->mvInfoForward);

	if(pVop_mode->SearchRangeForward <= 512)
	{
		mvInfoForward->FCode = ((pVop_mode->SearchRangeForward-1)>>4) + 1;
	}else
	{
		mvInfoForward->FCode = 7;
	}
	
	mvInfoForward->Range = 1 << (mvInfoForward->FCode + 4);	
	mvInfoForward->ScaleFactor = 1 << (mvInfoForward->FCode - 1);//uiScaleFactor is f
	
	if((pVop_mode->SearchRangeForward) == (mvInfoForward->Range >> 1))
	{
		pVop_mode->SearchRangeForward--; // avoid out of range half pel	
	}
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitVOEncoder
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
static void Mp4Enc_InitVOEncoder(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode)
{
	uint32 clock_rate;
	uint32 num_bits_time_incr;

	/*set clip table*/
// 	pVop_mode->ClipTable = Mp4_SetClipTable();
	
	
	Mp4Enc_SetMVInfo();
	
	clock_rate = pVol_mode->ClockRate-1;
	
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

	pVop_mode->time_inc_resolution_in_vol_length = num_bits_time_incr;
	
	pVop_mode->RoundingControlEncSwitch = pVol_mode->InitialRoundingType;
    if(!pVol_mode->bRoundingControlDisable)
    {
		pVop_mode->RoundingControlEncSwitch ^= 0x00000001;
    }
    pVop_mode->RoundingControl = pVop_mode->RoundingControlEncSwitch;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_InitHuffmanTable
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
MP4_LOCAL void Mp4Enc_InitHuffmanTable(ENC_VOP_MODE_T *pVop_mode)
{
	pVop_mode->pQuant_pa = (uint16 *)g_quant_pa;
	pVop_mode->pDC_scaler = (uint16 *)g_DC_scaler;
 	pVop_mode->pMcbpc_intra_tab = g_mcbpc_intra_tab;
	pVop_mode->pCbpy_tab = g_cbpy_tab;
	pVop_mode->pMcbpc_inter_tab = g_mcbpc_inter_tab;
	pVop_mode->pMvtab = g_mv_tab;
}

MP4_LOCAL uint8 Mp4Enc_Compute_log2(int32 uNum)
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

PUBLIC void Mp4Enc_InitSession(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode)
{
	uint32 total_mb_num_x;
	uint32 total_mb_num_y;

	/*MB number in hor and in ver and total MB number*/
	pVop_mode->OrgFrameWidth = pVol_mode->VolWidth;
	pVop_mode->OrgFrameHeight = pVol_mode->VolHeight;
	total_mb_num_x = pVop_mode->MBNumX = (pVol_mode->VolWidth  + 15) / MB_SIZE;
	total_mb_num_y = pVop_mode->MBNumY = (pVol_mode->VolHeight + 15) / MB_SIZE;
	pVop_mode->MBNum  = total_mb_num_x * total_mb_num_y;
	pVop_mode->FrameWidth  = total_mb_num_x * MB_SIZE;
	pVop_mode->FrameHeight = total_mb_num_y * MB_SIZE;	

	if(pVop_mode->short_video_header)
	{
		Mp4Enc_InitH263(pVol_mode, pVop_mode);
	}

	pVop_mode->pMbModeCur = (ENC_MB_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_MB_MODE_T) * total_mb_num_x);
	pVop_mode->pMbModeAbv = (ENC_MB_MODE_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_MB_MODE_T) * total_mb_num_x);	
	pVop_mode->pMBCache = (ENC_MB_BFR_T *)Mp4Enc_InterMemAlloc(sizeof(ENC_MB_BFR_T));

	pVop_mode->pYUVSrcFrame = (Mp4EncStorablePic *)Mp4Enc_InterMemAlloc(sizeof(Mp4EncStorablePic));
	pVop_mode->pYUVRecFrame = (Mp4EncStorablePic *)Mp4Enc_InterMemAlloc(sizeof(Mp4EncStorablePic));
	pVop_mode->pYUVRefFrame = (Mp4EncStorablePic *)Mp4Enc_InterMemAlloc(sizeof(Mp4EncStorablePic));

#if defined(_SIMULATION_) 
{		
	/*backward reference frame and forward reference frame after extention*/
	int32 size = (pVop_mode->FrameWidth) * (pVop_mode->FrameHeight);

	pVop_mode->pYUVSrcFrame->imgY = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size);
	if (!pVop_mode->uv_interleaved)
	{
		pVop_mode->pYUVSrcFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
		pVop_mode->pYUVSrcFrame->imgV = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>2);
	}else
	{
		pVop_mode->pYUVSrcFrame->imgU = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(size>>1);
		pVop_mode->pYUVSrcFrame->imgV = NULL;
	}

	pVop_mode->pYUVSrcFrame->imgYAddr = (uint32)pVop_mode->pYUVSrcFrame->imgY >> 8;
}
#endif
	
#if _CMODEL_
	pVop_mode->pYUVSrcFrame->imgYAddr = SRC_FRAME_Y_ADDR >> 8;
#endif
	
	pVop_mode->iMaxVal = 1<<(pVol_mode->nBits+3);	
	pVop_mode->QuantizerType = pVol_mode->QuantizerType;
	pVop_mode->intra_acdc_pred_disable = pVol_mode->intra_acdc_pred_disable;
	pVop_mode->short_video_header = pVol_mode->short_video_header;
	pVop_mode->QuantPrecision = pVol_mode->QuantPrecision;
	
	pVop_mode->MB_in_VOP_length = Mp4Enc_Compute_log2(pVop_mode->MBNum);
	
	if(!pVop_mode->short_video_header) //MPEG4
	{
		pVop_mode->time_inc_resolution_in_vol_length = Mp4Enc_Compute_log2(pVol_mode->ClockRate);
	}

	Mp4Enc_InitHuffmanTable(pVop_mode);

	Mp4Enc_InitVOEncoder(pVol_mode, pVop_mode);

	//initialize the one frame bitstream buffer.
	pVop_mode->pOneFrameBitstream = (uint8 *)Mp4Enc_ExtraMemAlloc_64WordAlign(ONEFRAME_BITSTREAM_BFR_SIZE);  //allocate BITSTREAM_BFR_SIZE for one frame encoded bitstream.
	pVop_mode->OneframeStreamLen = ONEFRAME_BITSTREAM_BFR_SIZE;
}


/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
