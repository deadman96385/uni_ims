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
#include "sc8810_video_header.h"

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
MP4_LOCAL uint32 Mp4Enc_InitVOPHeader(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr, int32 time_stamp)
{
	uint32 NumBits = 0;
//	static int last_modula_time_base;
	int32 modula_time_base;
	int32 vop_time_increment;
	int32 nbits_modula_time_base;

	NumBits += Mp4Enc_OutputBits(START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( ENC_VOP_START_CODE, NUMBITS_VOP_START_CODE);
	NumBits += Mp4Enc_OutputBits( vop_mode_ptr->VopPredType, NUMBITS_VOP_PRED_TYPE);

	modula_time_base = time_stamp / (int32)vol_mode_ptr->ClockRate;
	vop_time_increment = time_stamp - modula_time_base * (int32)vol_mode_ptr->ClockRate;
	
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
	NumBits += Mp4Enc_OutputBits( vop_time_increment, vop_mode_ptr->time_inc_resolution_in_vol_length);	

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
PUBLIC uint32 Mp4Enc_EncH263PicHeader(ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 nBits = 0;
	
	nBits += Mp4Enc_OutputBits( 1, 17);
	nBits += Mp4Enc_OutputBits( 0, 5);
	nBits += Mp4Enc_OutputBits( g_enc_tr, 8);

	/*PTYPE 13 bits*/
	nBits += Mp4Enc_OutputBits( 1, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( vop_mode_ptr->H263SrcFormat, 3);
	nBits += Mp4Enc_OutputBits( vop_mode_ptr->VopPredType, 1);
	nBits += Mp4Enc_OutputBits( 0, 1); //unrestricted mv is OFF, modified from 'on' to 'off', 20100111
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);

	nBits += Mp4Enc_OutputBits( vop_mode_ptr->StepSize, 5);
	
	nBits += Mp4Enc_OutputBits( 0, 1);
	nBits += Mp4Enc_OutputBits( 0, 1);

	g_enc_tr += (int32)g_enc_frame_skip_number + 1; 
	g_enc_tr = g_enc_tr % 256;

	return (int32)nBits; //= 50
}

/*****************************************************************************
 **	Name : 			Mp4Enc_EncSequenceHeader
 ** Description:	
 ** Author:			Xiaowei Luo
 **	Note:	
 *****************************************************************************/
PUBLIC uint32 Mp4Enc_EncSequenceHeader(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 NumBits = 0;
	BOOLEAN is_short_header = vop_mode_ptr->short_video_header;
	
	NumBits += Mp4Enc_OutputBits( START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( VSS_START_CODE, NUMBITS_START_CODE_SUFFIX);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->ProfileAndLevel, NUMBITS_VSS_PROFILE);
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
PUBLIC uint32 Mp4Enc_EncVOHeader(ENC_VOP_MODE_T *vop_mode_ptr)
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
PUBLIC uint32 Mp4Enc_EncVOLHeader(VOL_MODE_T *vol_mode_ptr, ENC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 NumBits = 0;
	ALPHA_USAGE_E alpha_usage;
	BOOLEAN is_short_header = vop_mode_ptr->short_video_header;

	NumBits += Mp4Enc_OutputBits( START_CODE_PREFIX, NUMBITS_START_CODE_PREFIX);
	NumBits += Mp4Enc_OutputBits( ENC_VOL_START_CODE, NUMBITS_VOL_START_CODE);
	NumBits += Mp4Enc_OutputBits( 0, NUMBITS_VOL_ID);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( 1, 8);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( 1, 4);
	NumBits += Mp4Enc_OutputBits( 0, 1);//VOL_Control is disable.
	
	alpha_usage = vol_mode_ptr->fAUsage;
	if(2 == alpha_usage) // CD: 0 = rect, 1 = binary, 2 = shape only, 3 = grey alpha
	{
		alpha_usage = 3;
	}
	NumBits += Mp4Enc_OutputBits( alpha_usage, NUMBITS_VOL_SHAPE);//0 is RECTANGLE.
	NumBits += Mp4Enc_OutputBits( 1, 1);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->ClockRate, NUMBITS_TIME_RESOLUTION); 
	NumBits += Mp4Enc_OutputBits( 1, 1);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	
	if(RECTANGLE == vol_mode_ptr->fAUsage) 
	{
		NumBits += Mp4Enc_OutputBits( MARKER_BIT, 1);
		NumBits += Mp4Enc_OutputBits( vop_mode_ptr->OrgFrameWidth, NUMBITS_VOP_WIDTH); 
		NumBits += Mp4Enc_OutputBits( MARKER_BIT, 1);
		NumBits += Mp4Enc_OutputBits( vop_mode_ptr->OrgFrameHeight, NUMBITS_VOP_HEIGHT); 
		NumBits += Mp4Enc_OutputBits( MARKER_BIT, 1);
		
	// 	NumBits += NUMBITS_VOP_WIDTH + NUMBITS_VOP_HEIGHT + 3;
	}

	NumBits += Mp4Enc_OutputBits( vop_mode_ptr->bInterlace, 1);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->bAdvPredDisable, 1);
	NumBits += Mp4Enc_OutputBits( 0, 1);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->bNot8Bit, 1);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->QuantizerType, 1); 

	NumBits += Mp4Enc_OutputBits( 1, 1);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->bResyncMarkerDisable, 1);
	NumBits += Mp4Enc_OutputBits( vol_mode_ptr->bDataPartitioning, 1);

	if( vol_mode_ptr->bDataPartitioning )
	{
		NumBits += Mp4Enc_OutputBits( vol_mode_ptr->bReversibleVlc, 1);
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
PUBLIC uint32 Mp4Enc_EncVOPHeader(ENC_VOP_MODE_T *vop_mode_ptr, int32 time_stamp)
{
	uint32 NumBits = 0;
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();

	NumBits += Mp4Enc_InitVOPHeader(vol_mode_ptr, vop_mode_ptr, time_stamp);
	NumBits += Mp4Enc_OutputBits( vop_mode_ptr->bCoded, 1);/* vop_coded */

	if (vop_mode_ptr->bCoded) 
	{
		if(PVOP == vop_mode_ptr->VopPredType) 
		{
			NumBits += Mp4Enc_OutputBits(vop_mode_ptr->RoundingControl, 1);
		}	

		/*intra_dc_vlc_thr is */
		NumBits += Mp4Enc_OutputBits(vop_mode_ptr->IntraDcSwitchThr, 3);/* intra_dc_vlc_threshold */
		
	   	vop_mode_ptr->bAlternateScan = FALSE;
		
		/*output quantization of this frame*/
		NumBits += Mp4Enc_OutputBits(vop_mode_ptr->StepSize, vop_mode_ptr->QuantPrecision);
		
		if(PVOP == vop_mode_ptr->VopPredType) 
		{		
			NumBits += Mp4Enc_OutputBits( vop_mode_ptr->mvInfoForward.FCode, NUMBITS_VOP_FCODE);/* forward_fixed_code */
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
PUBLIC uint32 Mp4Enc_EncGOBHeader(ENC_VOP_MODE_T *vop_mode_ptr, int32 gob_quant)
{
	uint32 NumBits = 0;
	uint32 gob_num;
	int32 gfid = 0;

	gob_num = (vop_mode_ptr->mb_y) / vop_mode_ptr->MBLineOneGOB;

	if ((vop_mode_ptr->mb_y > 0) && (!(vop_mode_ptr->mb_y % vop_mode_ptr->MBLineOneGOB)))
	{
		NumBits += Mp4Enc_OutputBits( 1, 17);
		NumBits += Mp4Enc_OutputBits( gob_num, 5);
		NumBits += Mp4Enc_OutputBits( gfid, 2);
		NumBits += Mp4Enc_OutputBits( gob_quant, 5);
		vop_mode_ptr->sliceNumber++;	
	}

	return NumBits;
}

PUBLIC uint32 Mp4Enc_ReSyncHeader(ENC_VOP_MODE_T * vop_mode_ptr, int Qp, int32 time_stamp)
{
	uint32 NumBits = 0;
	uint32 nBitsResyncmarker = 17;
	uint32 hec = 0; //now, not insert header information, 20091120
	VOL_MODE_T *vol_mode_ptr = Mp4Enc_GetVolmode();
	int32 mbnumEnc = vop_mode_ptr->mb_y*vop_mode_ptr->MBNumX/*+vop_mode_ptr->mb_x*/;

	NumBits += Mp4Enc_ByteAlign(vop_mode_ptr->short_video_header);
	
	if (PVOP == vop_mode_ptr->VopPredType)
	{
		nBitsResyncmarker += vop_mode_ptr->mvInfoForward.FCode - 1;
	}
	NumBits += Mp4Enc_OutputBits( 1, nBitsResyncmarker); //resync marker

	//only add resync header at the beginning of one MB Line
	//video packet
	NumBits += Mp4Enc_OutputBits(mbnumEnc, vop_mode_ptr->MB_in_VOP_length);//,"macro_block_num"
	NumBits += Mp4Enc_OutputBits(vop_mode_ptr->StepSize, vop_mode_ptr->QuantPrecision);//,"quant_scale"					 
	NumBits += Mp4Enc_OutputBits(hec, 1);//,"header_extension_code"

	if(hec) //lint !e774
	{				
		int32 modula_time_base;
		int32 vop_time_increment;
		int32 nbits_modula_time_base;

		modula_time_base = time_stamp / (int32)vol_mode_ptr->ClockRate;
		vop_time_increment = time_stamp - modula_time_base * (int32)vol_mode_ptr->ClockRate;
		
		nbits_modula_time_base = modula_time_base - g_enc_last_modula_time_base;
		NumBits += Mp4Enc_OutputBits( g_msk[nbits_modula_time_base] << 1, nbits_modula_time_base + 1);
		NumBits += Mp4Enc_OutputBits( (int32) 1, 1);	
		NumBits += Mp4Enc_OutputBits( vop_time_increment, vop_mode_ptr->time_inc_resolution_in_vol_length);	

		g_enc_last_modula_time_base = modula_time_base;

		NumBits += Mp4Enc_OutputBits( (int32) 1, 1);	
		NumBits += Mp4Enc_OutputBits( vop_mode_ptr->VopPredType, NUMBITS_VOP_PRED_TYPE);
		NumBits += Mp4Enc_OutputBits( vop_mode_ptr->IntraDcSwitchThr, 3);

		if(PVOP == vop_mode_ptr->VopPredType)
		{
			NumBits += Mp4Enc_OutputBits( vop_mode_ptr->mvInfoForward.FCode, NUMBITS_VOP_FCODE);/* forward_fixed_code */

		}
	}

	vop_mode_ptr->sliceNumber++;	
		
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
