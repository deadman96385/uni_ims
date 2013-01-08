/******************************************************************************
 ** File Name:    mp4enc_header.c                                             *
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
#include "sc8825_video_header.h"

/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

/*****************************************************************************
 **	Name : 			Mp4Enc_InitVOPHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
MP4_LOCAL uint32 Mp4Enc_InitVOPHeader(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode, int32 time_stamp)
{
	uint32 NumBits = 0;
//	static int last_modula_time_base;
	int32 modula_time_base;
	int32 vop_time_increment;
	int32 nbits_modula_time_base;

	NumBits += Mp4Enc_OutputBits(START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( ENC_VOP_START_CODE, NUMBITS_VOP_START_CODE);
	NumBits += Mp4Enc_OutputBits( pVop_mode->VopPredType, NUMBITS_VOP_PRED_TYPE);

	modula_time_base = time_stamp / (int32)pVol_mode->ClockRate;
	vop_time_increment = time_stamp - modula_time_base * (int32)pVol_mode->ClockRate;
	
	nbits_modula_time_base = modula_time_base - g_enc_last_modula_time_base;

	if (nbits_modula_time_base < 0)	
	{
		nbits_modula_time_base = 0;
	}
	else if (nbits_modula_time_base > 32)	
	{
		nbits_modula_time_base = 32;
	}
	
	NumBits += Mp4Enc_OutputBits( g_msk[nbits_modula_time_base] << 1, nbits_modula_time_base + 1);
	NumBits += Mp4Enc_OutputBits( (int32) 1, 1);	
	NumBits += Mp4Enc_OutputBits( vop_time_increment, pVop_mode->time_inc_resolution_in_vol_length);	

	g_enc_last_modula_time_base = modula_time_base;

	NumBits += Mp4Enc_OutputBits( (int32) 1, 1);	

	return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncH263PicHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
int32 Mp4Enc_EncH263PicHeader(ENC_VOP_MODE_T *pVop_mode)
{
	int32 nBits = 0;
	
	nBits += Mp4Enc_OutputBits( 1, 17);
	nBits += Mp4Enc_OutputBits( 0, 5);
	nBits += Mp4Enc_OutputBits( g_enc_tr, 8);

	/*PTYPE 13 bits*/
	nBits += Mp4Enc_OutputBits( 1, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( pVop_mode->H263SrcFormat, 3);
	nBits += Mp4Enc_OutputBits( pVop_mode->VopPredType, 1);
	nBits += Mp4Enc_OutputBits( 0, 1); //unrestricted mv is OFF, modified from 'on' to 'off', 20100111
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);

	nBits += Mp4Enc_OutputBits( pVop_mode->StepSize, 5);
	
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);

	g_enc_tr += g_enc_frame_skip_number + 1; 
	g_enc_tr = g_enc_tr % 256;

	return nBits; //= 50
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncSequenceHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
uint32 Mp4Enc_EncSequenceHeader(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode)
{
	uint32 NumBits = 0;
	BOOLEAN is_short_header = pVop_mode->short_video_header;
	
	NumBits += Mp4Enc_OutputBits( START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( VSS_START_CODE, NUMBITS_START_CODE_SUFFIX);
	NumBits += Mp4Enc_OutputBits( pVol_mode->ProfileAndLevel, NUMBITS_VSS_PROFILE);
	NumBits += Mp4Enc_OutputBits( START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( VSO_START_CODE, NUMBITS_START_CODE_SUFFIX);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( VSO_TYPE, NUMBITS_VSO_TYPE);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	
	NumBits += Mp4Enc_ByteAlign( is_short_header);	
	
	return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncVOHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
uint32 Mp4Enc_EncVOHeader(ENC_VOP_MODE_T *pVop_mode)
{
	uint32 NumBits = 0;
	
	NumBits += Mp4Enc_OutputBits(START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits(ENC_VO_START_CODE, NUMBITS_VO_START_CODE); //plus 3 bits
	NumBits += Mp4Enc_OutputBits(0, NUMBITS_VO_ID);
	
	PRINTF("VO Overhead\t\t%d bits\n\n", NumBits);

	return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncVOLHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
uint32 Mp4Enc_EncVOLHeader(VOL_MODE_T *pVol_mode, ENC_VOP_MODE_T *pVop_mode)
{
	uint32 NumBits = 0;
	ALPHA_USAGE_E alpha_usage;
	BOOLEAN is_short_header = pVop_mode->short_video_header;

	NumBits += Mp4Enc_OutputBits( START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( ENC_VOL_START_CODE, NUMBITS_VOL_START_CODE);
	NumBits += Mp4Enc_OutputBits( 0, NUMBITS_VOL_ID);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( 1, 8);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( 1, 4);
	NumBits += Mp4Enc_OutputBits( 0, 1);//VOL_Control is disable.
	
	alpha_usage = pVol_mode->fAUsage;
	if(2 == alpha_usage) // CD: 0 = rect, 1 = binary, 2 = shape only, 3 = grey alpha
	{
		alpha_usage = 3;
	}
	NumBits += Mp4Enc_OutputBits( alpha_usage, NUMBITS_VOL_SHAPE);//0 is RECTANGLE.
	NumBits += Mp4Enc_OutputBits( 1, 1);
	NumBits += Mp4Enc_OutputBits( pVol_mode->ClockRate, NUMBITS_TIME_RESOLUTION); 
	NumBits += Mp4Enc_OutputBits( 1, 1);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	
	if(RECTANGLE == pVol_mode->fAUsage) 
	{
		NumBits += Mp4Enc_OutputBits( MARKER_BIT, 1);
		NumBits += Mp4Enc_OutputBits( pVop_mode->OrgFrameWidth, NUMBITS_VOP_WIDTH); 
		NumBits += Mp4Enc_OutputBits( MARKER_BIT, 1);
		NumBits += Mp4Enc_OutputBits( pVop_mode->OrgFrameHeight, NUMBITS_VOP_HEIGHT); 
		NumBits += Mp4Enc_OutputBits( MARKER_BIT, 1);
		
	// 	NumBits += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT + 3;
	}

	NumBits += Mp4Enc_OutputBits( pVop_mode->bInterlace, 1);
	NumBits += Mp4Enc_OutputBits( pVol_mode->bAdvPredDisable, 1);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( pVol_mode->bNot8Bit, 1);
	NumBits += Mp4Enc_OutputBits( pVol_mode->QuantizerType, 1); 

	NumBits += Mp4Enc_OutputBits( 1, 1);
	NumBits += Mp4Enc_OutputBits( pVol_mode->bResyncMarkerDisable, 1);
	NumBits += Mp4Enc_OutputBits( pVol_mode->bDataPartitioning, 1);

	if( pVol_mode->bDataPartitioning )
	{
		NumBits += Mp4Enc_OutputBits( pVol_mode->bReversibleVlc, 1);
	}

	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_ByteAlign( is_short_header);	

	return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncVOPHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
uint32 Mp4Enc_EncVOPHeader(ENC_VOP_MODE_T *pVop_mode, int32 time_stamp)
{
	uint32 NumBits = 0;
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();

	NumBits += Mp4Enc_InitVOPHeader(pVol_mode, pVop_mode, time_stamp);
	
	NumBits += Mp4Enc_OutputBits(pVop_mode->bCoded , 1);/* vop_coded */
if (pVop_mode->bCoded) 
{	
	if(PVOP == pVop_mode->VopPredType) 
	{
		NumBits += Mp4Enc_OutputBits(pVop_mode->RoundingControl, 1);
	}	

	/*intra_dc_vlc_thr is */
	NumBits += Mp4Enc_OutputBits(pVop_mode->IntraDcSwitchThr, 3);/* intra_dc_vlc_threshold */
	
   	pVop_mode->bAlternateScan = FALSE;
	
	/*output quantization of this frame*/
	NumBits += Mp4Enc_OutputBits(pVop_mode->StepSize, pVop_mode->QuantPrecision);
	
	if(PVOP == pVop_mode->VopPredType) 
	{		
		NumBits += Mp4Enc_OutputBits( pVop_mode->mvInfoForward.FCode, NUMBITS_VOP_FCODE);/* forward_fixed_code */
	}
}
	return NumBits;
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncGOBHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
uint32 Mp4Enc_EncGOBHeader(ENC_VOP_MODE_T *pVop_mode, int32 gob_quant)
{
	uint32 NumBits = 0;
	uint32 gob_num;
	int32 gfid = 0;
	uint32 cmd;

	gob_num = (pVop_mode->mb_y) / pVop_mode->MBLineOneGOB;

	if ((pVop_mode->mb_y > 0) && (!(pVop_mode->mb_y % pVop_mode->MBLineOneGOB)))
	{
		cmd = (1<<12)|(gob_num<<7)|(gfid<<5)|(gob_quant);

		NumBits += Mp4Enc_OutputBits(cmd,29);

		//NumBits += Mp4Enc_OutputBits( 1, 17);
		//NumBits += Mp4Enc_OutputBits( gob_num, 5);
		//NumBits += Mp4Enc_OutputBits( gfid, 2);
		//NumBits += Mp4Enc_OutputBits( gob_quant, 5);
		
		pVop_mode->sliceNumber++;	
	}

	return NumBits;
}

uint32 Mp4Enc_ReSyncHeader(ENC_VOP_MODE_T * pVop_mode, int Qp, int32 time_stamp)
{
	uint32 cmd;
	uint32 NumBits = 0;
	uint32 nBitsResyncmarker = 17;
	uint32 hec = 0; //now, not insert header information, 20091120
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	int32 mbnumEnc = pVop_mode->mb_y*pVop_mode->MBNumX/*+pVop_mode->mb_x*/;

	NumBits += Mp4Enc_ByteAlign(pVop_mode->short_video_header);
	
	if (PVOP == pVop_mode->VopPredType)
	{
		nBitsResyncmarker += pVop_mode->mvInfoForward.FCode - 1;
	}
	NumBits += Mp4Enc_OutputBits( 1, nBitsResyncmarker); //resync marker

	//only add resync header at the beginning of one MB Line
	//video packet
	NumBits += Mp4Enc_OutputBits(mbnumEnc, pVop_mode->MB_in_VOP_length);//,"macro_block_num"
	//NumBits += Mp4Enc_OutputBits(pVop_mode->StepSize, pVop_mode->QuantPrecision);//,"quant_scale"					 
	//NumBits += Mp4Enc_OutputBits(hec, 1);//,"header_extension_code"

	cmd = ((pVop_mode->StepSize) << 1 )| hec;
	NumBits += Mp4Enc_OutputBits(cmd, 1+(pVop_mode->QuantPrecision));

	if(hec)
	{				
		int32 modula_time_base;
		int32 vop_time_increment;
		int32 nbits_modula_time_base;

		modula_time_base = time_stamp / (int32)pVol_mode->ClockRate;
		vop_time_increment = time_stamp - modula_time_base * (int32)pVol_mode->ClockRate;
		
		nbits_modula_time_base = modula_time_base - g_enc_last_modula_time_base;
		NumBits += Mp4Enc_OutputBits( g_msk[nbits_modula_time_base] << 1, nbits_modula_time_base + 1);
		NumBits += Mp4Enc_OutputBits( (int32) 1, 1);	
		NumBits += Mp4Enc_OutputBits( vop_time_increment, pVop_mode->time_inc_resolution_in_vol_length);	

		g_enc_last_modula_time_base = modula_time_base;

		NumBits += Mp4Enc_OutputBits( (int32) 1, 1);	
		NumBits += Mp4Enc_OutputBits( pVop_mode->VopPredType, NUMBITS_VOP_PRED_TYPE);
		NumBits += Mp4Enc_OutputBits( pVop_mode->IntraDcSwitchThr, 3);

		if(PVOP == pVop_mode->VopPredType)
		{
			NumBits += Mp4Enc_OutputBits( pVop_mode->mvInfoForward.FCode, NUMBITS_VOP_FCODE);/* forward_fixed_code */

		}
	}

	pVop_mode->sliceNumber++;	
		
	return NumBits;
}	
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
