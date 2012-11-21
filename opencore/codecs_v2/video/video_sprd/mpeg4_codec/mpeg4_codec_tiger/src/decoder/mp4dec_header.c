/******************************************************************************
 ** File Name:    mp4dec_header.c                                             *
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
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

#define PRINTF_HEAD_INFO	//printf

/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_DecGobHeader
 ** Description:	get the header of Gob. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecGobHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 tmp_var, tmp_var2;
	int32 mb_num;
	int32 gob_number = vop_mode_ptr->GobNum;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	if(gob_number > 0)
	{
		tmp_var = Mp4Dec_ShowBits(bitstrm_ptr, GOB_RESYNC_MARKER_LENGTH); 
		tmp_var2 = Mp4Dec_ShowBitsByteAlign_H263(bitstrm_ptr, GOB_RESYNC_MARKER_LENGTH);

		if((GOB_RESYNC_MARKER == tmp_var) || (GOB_RESYNC_MARKER == tmp_var2))
		{
			if(GOB_RESYNC_MARKER == tmp_var2)
			{
				Mp4Dec_ByteAlign_Startcode(bitstrm_ptr);
			}
			
			Mp4Dec_FlushBits(bitstrm_ptr, GOB_RESYNC_MARKER_LENGTH);
			
			//tmp_var bit[6:2] is gob_number and bit[1:0] is "gob_frame_id"
			tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 7);	
			vop_mode_ptr->GobNum = (int8)(tmp_var>>2); 
			vop_mode_ptr->mb_y		= vop_mode_ptr->GobNum * vop_mode_ptr->num_mbline_gob;
			if (vop_mode_ptr->mb_y >= vop_mode_ptr->MBNumY)
			{
				vop_mode_ptr->error_flag = TRUE;
				vop_mode_ptr->return_pos |= (1<<16);
				return MMDEC_STREAM_ERROR;
			}
				
			vop_mode_ptr->StepSize = (int8)Mp4Dec_ReadBits(bitstrm_ptr, (uint32)(vop_mode_ptr->QuantPrecision));//, "quant_scale"
			vop_mode_ptr->StepSize = IClip(1, 31, vop_mode_ptr->StepSize);
				
			mb_num = vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX;

			//set skipped MB "NOT DECODED" status, for EC later
			if (mb_num < vop_mode_ptr->mbnumDec)
			{
				int32 mb_pos;
				for (mb_pos = mb_num; mb_pos < vop_mode_ptr->mbnumDec; mb_pos++)
				{
					vop_mode_ptr->mbdec_stat_ptr[mb_pos] = NOT_DECODED;
				}
				vop_mode_ptr->error_flag = TRUE;
				vop_mode_ptr->err_MB_num += (vop_mode_ptr->mbnumDec - mb_num);
				vop_mode_ptr->return_pos |= (1<<17);
				return MMDEC_STREAM_ERROR;
			}else
			{
				int32 mb_pos;
				for (mb_pos = vop_mode_ptr->mbnumDec; mb_pos < mb_num; mb_pos++)
				{
					vop_mode_ptr->mbdec_stat_ptr[mb_pos] = NOT_DECODED;
				}
			}

			vop_mode_ptr->mbnumDec = mb_num;
			vop_mode_ptr->mb_y	   = mb_num / vop_mode_ptr->MBNumX;
			vop_mode_ptr->mb_x	   = mb_num - vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX;
			
			vop_mode_ptr->sliceNumber++;
		}
	}

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecVolHeader
 ** Description:	get the header of VoVol.Mp4 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL MMDecRet Mp4Dec_DecVolHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 tmpVar;
	BOOLEAN is_object_layer_indentifier;
	uint8 vol_verid;
	uint32 vol_priority;  
	BOOLEAN vol_ctrl_para;
	uint32 aspect_ration_info;
	uint8 fAUsage;
	uint32 marker;
	BOOLEAN is_fixed_frame_rate;
	BOOLEAN is_complexity_estimation_disable;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	
	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, VOL_START_CODE_LENGTH); //"vol_start_code"
	if(tmpVar != DEC_VOL_START_CODE)
	{
		PRINTF_HEAD_INFO("bitstream does not start with DEC_VOL_START_CODE\n");
		return MMDEC_STREAM_ERROR;
	}

	vop_mode_ptr->intra_acdc_pred_disable = FALSE;
	
	//tmp_var bit[13:1] is "VOL identifier","Vol random access" and "Vol type indication"
	//and bit[0] is "is object layer identifier"
	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 14); 
	is_object_layer_indentifier = (tmpVar & 0x01);
	if(is_object_layer_indentifier)	
	{
		//tmp_var 
		//bit[6:3] is "video object layer verid"
		//bit[2:0] is "video object layer priority"
		tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 7);
		vol_verid = (uint8)(tmpVar>>3);
		vol_priority = (uint32)(tmpVar&0x03);
	}else
	{
		vol_verid = 1;
		vol_priority = 1;
	}

	aspect_ration_info = Mp4Dec_ReadBits(bitstrm_ptr, 4); //"aspect_ration_info"

	if(15 == aspect_ration_info)
	{
		//tmp_var
		//bit[15:8] is "par width" 
		//bit[7:0] is "par height"
		tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 16);
	}

	vol_ctrl_para = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1); //"vol control parameter"
	if(vol_ctrl_para)
	{
	//	uint32 chroma_format;
	//	uint32 low_delay;
		uint32 vbv_parameter;
		
		//tmp_var 
		//bit[3:2] is "chroma_format"
		//bit[1] is "low_delay"
		//bit[0] is "vbv_parameter"
		tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 4); 
		vbv_parameter = (tmpVar&0x01);
		
		if(1 == vbv_parameter)
		{	
			//for speed up.xiaowei.luo@2007.07.27	
			//total bits is 79	
			Mp4Dec_ReadBits(bitstrm_ptr, 31);
			Mp4Dec_ReadBits(bitstrm_ptr, 31);
			Mp4Dec_ReadBits(bitstrm_ptr, 17);  			
		}
	}

    fAUsage = (ALPHA_USAGE_E)Mp4Dec_ReadBits(bitstrm_ptr, 2); // "video object layer shape"

	marker = Mp4Dec_ReadBits(bitstrm_ptr, 1); //"marker"
	if(marker != 1)
	{
		return MMDEC_STREAM_ERROR;
	}

	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 16); //"vop time increment resolution"
	vop_mode_ptr->time_inc_resolution = tmpVar;

//	assert((vop_mode_ptr->time_inc_resolution >= 1) && (vop_mode_ptr->time_inc_resolution < 65536));

	marker = Mp4Dec_ReadBits(bitstrm_ptr, 1); //, "marker"
	if(marker != 1)
	{
		return MMDEC_STREAM_ERROR;
	}
	
	/*when vop time increment resolution is power of 2, the bits for time increment is not correct*/
	if(1 == tmpVar)
	{
		vop_mode_ptr->NumBitsTimeIncr = 1;
	}else
	{
		uint8 NumBitsTimeIncr = vop_mode_ptr->NumBitsTimeIncr;
		tmpVar -= 1;
		for(NumBitsTimeIncr = 1; NumBitsTimeIncr < 16; NumBitsTimeIncr++)
		{
			if(1 == tmpVar)
			{
				break;
			}
			tmpVar = (tmpVar >> 1);
		}
		vop_mode_ptr->NumBitsTimeIncr = NumBitsTimeIncr;
	}
	
	is_fixed_frame_rate = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//, "fixed vop rate"
	if(is_fixed_frame_rate)
	{
		uint32 uiFixedVOPTimeIncrement;
		uiFixedVOPTimeIncrement = Mp4Dec_ReadBits(bitstrm_ptr, vop_mode_ptr->NumBitsTimeIncr);//, "fixed vop time increment"
	}

	if(RECTANGLE == fAUsage)
	{
		//tmp_var 
		//bit[28] is "marker"
		//bit[27:15] is "video object layer width"
		//bit[14] is "marker"
		//bit[13:1] is "video object layer height"
		//bit[0] is "marker"
		tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 29); 
		vop_mode_ptr->OrgFrameWidth = ((tmpVar>>15)&0x1FFF);
		vop_mode_ptr->OrgFrameHeight = ((tmpVar>>1)&0x1FFF);

		if (!vop_mode_ptr->OrgFrameHeight || !vop_mode_ptr->OrgFrameWidth)
		{
			return MMDEC_STREAM_ERROR;
		}

		PRINTF_HEAD_INFO("frame width = %d, frame height = %d\n", vop_mode_ptr->OrgFrameWidth, vop_mode_ptr->OrgFrameHeight);
	}else
	{
		PRINTF_HEAD_INFO("fAUsage is not RECTANGLE!\n");
		return MMDEC_NOT_SUPPORTED;
	}

	vop_mode_ptr->bInterlace = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1); 
	if(vop_mode_ptr->bInterlace)
	{
		PRINTF_HEAD_INFO("interlacing!\n");	
		return MMDEC_NOT_SUPPORTED;
	}

	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 1); //"obmc_disable"
	
	if(1 == vol_verid)
	{
		vop_mode_ptr->sprite_enable = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//, "sprite usage"
	}else
	{
		vop_mode_ptr->sprite_enable = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 2);//, "sprite usage"
	}

    if((1 == vop_mode_ptr->sprite_enable) || (2 == vop_mode_ptr->sprite_enable)) //sprite_static or sprite_gmc
	{
#if 1
		PRINTF_HEAD_INFO("sprite is not supported!\n");
		return MMDEC_NOT_SUPPORTED;
#else
		int low_latency_sprite_enable;
		
		PRINTF ("GMC!\n");
		
		if (vop_mode_ptr->sprite_enable != 2)
		{
			int sprite_width;
			int sprite_height;
			int sprite_left_coord;
			int sprite_top_coord;
			sprite_width = Mp4Dec_ReadBits (bitstrm_ptr, 13);		/* sprite_width */
			Mp4Dec_ReadBits(bitstrm_ptr, 1);
			sprite_height = Mp4Dec_ReadBits (bitstrm_ptr, 13);	/* sprite_height */
			Mp4Dec_ReadBits(1);  //marker
			sprite_left_coord = Mp4Dec_ReadBits(bitstrm_ptr, 13);	/* sprite_left_coordinate */
			Mp4Dec_ReadBits(1);  //marker
			sprite_top_coord = Mp4Dec_ReadBits (bitstrm_ptr, 13);	/* sprite_top_coordinate */
			Mp4Dec_ReadBits(1);  //marker
		}

		vop_mode_ptr->sprite_warping_points = Mp4Dec_ReadBits (bitstrm_ptr, 6);		/* no_of_sprite_warping_points */
		vop_mode_ptr->sprite_warping_accuracy = Mp4Dec_ReadBits (bitstrm_ptr, 2);		/* sprite_warping_accuracy */
		vop_mode_ptr->sprite_brightness_change = Mp4Dec_ReadBits (bitstrm_ptr, 1);		/* brightness_change */

		if (vop_mode_ptr->sprite_enable != 2)  //!=gmc
		{
			low_latency_sprite_enable = Mp4Dec_ReadBits(bitstrm_ptr, 1);		/* low_latency_sprite_enable */
		}
		
		PRINTF ("warping point: %d\n", vop_mode_ptr->sprite_warping_points);
#endif
	}

	tmpVar = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//, "not 8 bit"

	if(tmpVar)
	{
		PRINTF_HEAD_INFO("be not 8 bit not supported!\n");
		return MMDEC_NOT_SUPPORTED;
	}else
	{
		vop_mode_ptr->QuantPrecision = 5;
	}

#if 0 //removed in 6600L platform. XiaoweiLuo@20090514
	if(EIGHT_BIT == fAUsage)
	{
		PRINTF_HEAD_INFO("grayscale is not supported!\n");
		return MMDEC_NOT_SUPPORTED;
	}
#endif	

	vop_mode_ptr->QuantizerType = (QUANTIZER_E)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"quant type"
	if(Q_MPEG == vop_mode_ptr->QuantizerType)
	{	
		PRINTF_HEAD_INFO("MPEG quantization mode!\n");
		vop_mode_ptr->is_need_init_vsp_quant_tab = TRUE;
		vop_mode_ptr->bLoadIntraQMatrix = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"load intra quant mat"
		if(vop_mode_ptr->bLoadIntraQMatrix)
		{
			uint8 QMatrixElement;
			uint8 j, i = 0;
             
			do 
			{
				QMatrixElement = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 8);//"intra quant mat"
				vop_mode_ptr->IntraQuantizerMatrix[g_standard_zigzag[i]] = QMatrixElement;
				i++;
			}while((0 != QMatrixElement) && (i < BLOCK_SQUARE_SIZE));

			if(0 == QMatrixElement)
			{
				i--;//NOTE!

				for (j = i; (j < BLOCK_SQUARE_SIZE)&&((i-1) < BLOCK_SQUARE_SIZE); j++)
				{
					vop_mode_ptr->IntraQuantizerMatrix[g_standard_zigzag[j]] = 
					vop_mode_ptr->IntraQuantizerMatrix[g_standard_zigzag[i-1]];
				}	
			}
		}else
		{
			memcpy(vop_mode_ptr->IntraQuantizerMatrix, g_default_intra_qmatrix, BLOCK_SQUARE_SIZE * sizeof (uint8));
		}

		vop_mode_ptr->bLoadInterQMatrix = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"load inter quant mat"
		if (vop_mode_ptr->bLoadInterQMatrix)
		{
			uint8 QMatrixElement;
			uint8 j, i = 0;
			
			do 
			{
				QMatrixElement = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 8);//"inter quant mat"
				vop_mode_ptr->InterQuantizerMatrix [g_standard_zigzag[i]] = QMatrixElement;
				i++;			
			}while((QMatrixElement != 0) && (i < BLOCK_SQUARE_SIZE));

			if(0 == QMatrixElement)
			{
				i--;//NOTE!
				
				for (j = i; (j < BLOCK_SQUARE_SIZE)&&((i-1) < BLOCK_SQUARE_SIZE); j++)
				{
					vop_mode_ptr->InterQuantizerMatrix [g_standard_zigzag[j]] = 
						vop_mode_ptr->InterQuantizerMatrix[g_standard_zigzag[i-1]];
				}	
			}					
		}else
		{
			memcpy (vop_mode_ptr->InterQuantizerMatrix, g_default_inter_qmatrix, BLOCK_SQUARE_SIZE * sizeof (uint8));
		}
	}

	if(1 != vol_verid)
	{
		vop_mode_ptr->bQuarter_pel = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"quarter sample"	
	}else
	{
		vop_mode_ptr->bQuarter_pel = FALSE;
	}

	if(vop_mode_ptr->bQuarter_pel)
	{
		PRINTF_HEAD_INFO("Using quarter pel.\n");

		PRINTF_HEAD_INFO("quarter pel is not supported!\n");
		return MMDEC_NOT_SUPPORTED;
	}

	is_complexity_estimation_disable = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"complexity estimation disable"

	if(is_complexity_estimation_disable)
	{
		PRINTF_HEAD_INFO("complexity estimation disable.\n");
	}else
	{
		PRINTF_HEAD_INFO("complexity estimation is not supported!\n");
		return MMDEC_NOT_SUPPORTED;
	}

	vop_mode_ptr->bResyncMarkerDisable = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"resync marker disable"
	if(vop_mode_ptr->bResyncMarkerDisable)
	{
		PRINTF_HEAD_INFO("not using resync marker.\n");
	}else
	{
		PRINTF_HEAD_INFO("Using resync marker.\n");
	}

	vop_mode_ptr->bDataPartitioning = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"data partitioned"

	if(vop_mode_ptr->bDataPartitioning)
	{
		PRINTF_HEAD_INFO("Using data partition.\n");
		vop_mode_ptr->bReversibleVlc = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"reversible vlc"
		if(vop_mode_ptr->bReversibleVlc)
		{
			PRINTF_HEAD_INFO("Using reversible vlc.\n");
		}
	}else
	{
		PRINTF_HEAD_INFO("not using data partition.\n");
		vop_mode_ptr->bReversibleVlc = FALSE;
	}

	if(1 != vol_verid)
	{
		BOOLEAN is_new_pred_enable;
		BOOLEAN is_reduced_resolution_vop_enable;
		is_new_pred_enable = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"newpred enable"
		if(is_new_pred_enable)
		{
			PRINTF_HEAD_INFO("newpred is supported!\n");
			return MMDEC_NOT_SUPPORTED;
		}else
		{
			PRINTF_HEAD_INFO("newpred: OFF.\n\n");
		}

		is_reduced_resolution_vop_enable = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"reduced resolution vop enable"	
	}

	tmpVar = /*(VOL_TYPE_E)*/Mp4Dec_ReadBits(bitstrm_ptr, 1);//"scalability"
	if(tmpVar)
	{
		PRINTF_HEAD_INFO("scalability is not supported!\n");
		return MMDEC_NOT_SUPPORTED;
	}	

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecVopHeader
 ** Description:	Get the header of Vop from the bitstream,Mp4
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL MMDecRet Mp4Dec_DecVopHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 tmp_var;
	int32 time_base = 0;
	int32 time_increment;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, VOP_START_CODE_LENGTH);//"vop_start_code"
	if(DEC_VOP_START_CODE != tmp_var)
	{
		PRINTF_HEAD_INFO("Bitstream does not start with DEC_VOP_START_CODE\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<18);
		return MMDEC_STREAM_ERROR;
	}
	
	//tmp_var 
	//bit[2:1] is "vop_prediction_type"
	//bit[0] is "modulo_time_base"
	tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 3);
	vop_mode_ptr->VopPredType = (tmp_var>>1); 	
	tmp_var = (tmp_var & 0x01);

	while(1 == tmp_var)
	{
		tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 1);//"modulo_time_base"
		time_base++;
	}

	tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 1);//"marker_bit"

	time_increment = (int32)Mp4Dec_ReadBits(bitstrm_ptr, vop_mode_ptr->NumBitsTimeIncr);//"vop_time_increment"

	/*compute time_bp and time_pp for direct MV computation*/
	if(vop_mode_ptr->VopPredType != BVOP)
	{
		vop_mode_ptr->last_time_base = vop_mode_ptr->time_base;
		vop_mode_ptr->time_base += time_base;
		vop_mode_ptr->time = vop_mode_ptr->time_base * vop_mode_ptr->time_inc_resolution + time_increment;
		vop_mode_ptr->time_pp = (int32)(vop_mode_ptr->time - vop_mode_ptr->last_non_b_time);
		vop_mode_ptr->last_non_b_time = vop_mode_ptr->time;
	}else
	{
		if (vop_mode_ptr->time_pp == 0)
	    	{
	        	return MMDEC_ERROR;            
		}

		vop_mode_ptr->time = (vop_mode_ptr->last_time_base + time_base) * vop_mode_ptr->time_inc_resolution + time_increment;
		vop_mode_ptr->time_bp = vop_mode_ptr->time_pp - (int32)(vop_mode_ptr->last_non_b_time - vop_mode_ptr->time);
	}
	
	//tmp_var 
	//bit[1] is "marker_bit"
	//bit[0] is "vop_coded"
	tmp_var = Mp4Dec_ReadBits(bitstrm_ptr, 2);

	vop_mode_ptr->bCoded = (BOOLEAN)(tmp_var & 0x01);
	if(!vop_mode_ptr->bCoded)
	{
		vop_mode_ptr->VopPredType = NVOP;
		return MMDEC_OK;
	}

	if((PVOP == vop_mode_ptr->VopPredType) || ((SVOP == vop_mode_ptr->VopPredType) &&(vop_mode_ptr->sprite_enable == 2)))
	{
		vop_mode_ptr->RoundingControl = (int32)Mp4Dec_ReadBits(bitstrm_ptr, 1);//"vop_rounding_type"
	}
	vop_mode_ptr->IntraDcSwitchThr = (int32)Mp4Dec_ReadBits(bitstrm_ptr, 3);//"intra_dc_vlc_thr"
	
	if(vop_mode_ptr->bInterlace)
	{
		PRINTF_HEAD_INFO("interlace does not be supported\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<19);
		return MMDEC_STREAM_ERROR;
	}

	if ( vop_mode_ptr->sprite_enable == 2 && vop_mode_ptr->VopPredType == SVOP) 
	{
		PRINTF_HEAD_INFO("GMC NOT SUPPORTED!\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<20);
		return MMDEC_STREAM_ERROR;
	}

	vop_mode_ptr->StepSize = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, vop_mode_ptr->QuantPrecision);//, "vop_quant"

    if(IVOP != vop_mode_ptr->VopPredType)
	{
		MV_INFO_T *pMvInfo = &(vop_mode_ptr->mvInfoForward);
		pMvInfo->FCode = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 3);//"vop_fcode_forward"
		pMvInfo->ScaleFactor = 1 << (pMvInfo->FCode - 1);
		pMvInfo->Range = 16 << (pMvInfo->FCode);
	}

	if(BVOP == vop_mode_ptr->VopPredType)
	{
		MV_INFO_T *pMvInfo = &(vop_mode_ptr->mvInfoBckward);
		pMvInfo->FCode = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 3);//"vop_fcode_backward"
		pMvInfo->ScaleFactor = 1 << (pMvInfo->FCode - 1);
		pMvInfo->Range = 16 << (pMvInfo->FCode);
	}

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecVoHeader
 ** Description:	Get the header of Visual object.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL MMDecRet Mp4Dec_DecVoHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	MMDecRet ret = MMDEC_OK;
	int32 is_visual_object_identifier;
	int32 visual_object_verid;
	int32 visual_object_priority;
	int32 visual_object_type;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	Mp4Dec_FlushBits(bitstrm_ptr, 32);
	is_visual_object_identifier = Mp4Dec_ReadBits(bitstrm_ptr, 1);

	if(is_visual_object_identifier)
	{
		visual_object_verid = Mp4Dec_ReadBits(bitstrm_ptr, 4);
		visual_object_priority = Mp4Dec_ReadBits(bitstrm_ptr, 3);
	}else
	{
		visual_object_verid = 1;
	}

	visual_object_type = Mp4Dec_ReadBits(bitstrm_ptr, 4);

	if((visual_object_type == 1) || (visual_object_type == 2))
	{
		/*video signal type*/
		int32 video_signal_type;
		int32 video_format;
		int32 video_range;
		int32 colour_description;
		int32 colour_primaries;
		int32 transfer_charachteristics;
		int32 matrix_coefficient;

		video_signal_type = Mp4Dec_ReadBits(bitstrm_ptr, 1);

		if(video_signal_type)
		{
			video_format = Mp4Dec_ReadBits(bitstrm_ptr, 3);
			video_range = Mp4Dec_ReadBits(bitstrm_ptr, 1);
			colour_description = Mp4Dec_ReadBits(bitstrm_ptr, 1);

			if(colour_description)
			{
				colour_primaries = Mp4Dec_ReadBits(bitstrm_ptr, 8);
				transfer_charachteristics = Mp4Dec_ReadBits(bitstrm_ptr, 8);
				matrix_coefficient = Mp4Dec_ReadBits(bitstrm_ptr, 8);
			}
		}
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecGOV
 ** Description:	Decode the GOV information. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL MMDecRet Mp4Dec_DecGOV(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 tmp_var;
	uint32 times;
	uint32 closed_gov;
	uint32 broken_link;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	tmp_var = (uint32)Mp4Dec_ReadBits(bitstrm_ptr, 32); // "group_start_code"

	if(tmp_var != GROUP_START_CODE)
	{
		PRINTF_HEAD_INFO("\nBitstream does not start with GROUP_START_CODE\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<21);
		return MMDEC_STREAM_ERROR;
	}
	
	//tmp_var
	//bit[20:16], "Hour"
	//bit[15:9], "Minute"
	//bit[8], "Marker_bit"
	//bit[7:2], "Second"
	//bit[1], "closed_gov"
	//bit[0], "broken_link"
	tmp_var = (uint32)Mp4Dec_ReadBits(bitstrm_ptr, 21);
	
	times = (tmp_var>>16) * 3600; 
	times += ((tmp_var>>9)&0x3F) * 60;
	times += ((tmp_var>>2)&0x3F);

	closed_gov = ((tmp_var>>1)&0x01);
	broken_link = (tmp_var&0x01);

	if((closed_gov == 0)&&(broken_link == 1))
	{
		PRINTF_HEAD_INFO("closed_gov = 0\nbroken_link = 1\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<22);
		return MMDEC_STREAM_ERROR;
	}

	/**/
	Mp4Dec_ByteAlign_Mp4(bitstrm_ptr);
	while((tmp_var = Mp4Dec_ShowBits(bitstrm_ptr, USER_DATA_START_CODE_LENGTH)) == USER_DATA_START_CODE)
	{
		Mp4Dec_FlushBits(bitstrm_ptr, USER_DATA_START_CODE_LENGTH);
		
		while((tmp_var = Mp4Dec_ShowBits(bitstrm_ptr, NUMBITS_START_CODE_PREFIX)) != START_CODE_PREFIX)
		{
			Mp4Dec_FlushBits(bitstrm_ptr, 8);
		}
	} 

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecVoSeqHeader
 ** Description:	Decode the vo sequence header. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL MMDecRet Mp4Dec_DecVoSeqHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	MMDecRet ret = MMDEC_OK;
	int32 profile_level_indication;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	Mp4Dec_FlushBits(bitstrm_ptr, 32);
	profile_level_indication = Mp4Dec_ReadBits(bitstrm_ptr, 8);

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecMp4Header
 ** Description:	Get the header of Vop from the bitstream,mp4
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecMp4Header(DEC_VOP_MODE_T *vop_mode_ptr, uint32 uOneFrameLen)
{
	MMDecRet ret = MMDEC_OK;
	uint32 uStartCode;
	uint32 uTmpVar;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	uint32 nDecTotalBits = bitstrm_ptr->bitcnt;

	while(((nDecTotalBits>>3)/* + 4*/) <= uOneFrameLen)
	{
		Mp4Dec_ByteAlign_Startcode(bitstrm_ptr);

		uStartCode = Mp4Dec_ShowBits(bitstrm_ptr, 32);

		if(DEC_VOP_START_CODE == uStartCode)
		{
			ret = Mp4Dec_DecVopHeader(vop_mode_ptr);
			vop_mode_ptr->find_vop_header = 1;
			break;
		}else if(VISOBJSEQ_STOP_CODE == uStartCode)
		{
			Mp4Dec_ReadBits(bitstrm_ptr, 32);
		}else if(VISOBJ_START_CODE == uStartCode)
		{
			ret = Mp4Dec_DecVoHeader(vop_mode_ptr);
		}else if(VIDOBJ_START_CODE == ((uStartCode>>5)<<5))
		{
			Mp4Dec_ReadBits(bitstrm_ptr, 32);
		}else if(VIDOBJLAY_START_CODE == ((uStartCode>>4)<<4))
		{
			ret = Mp4Dec_DecVolHeader(vop_mode_ptr);
		}else if(GRPOFVOP_START_CODE == uStartCode)
		{
			ret = Mp4Dec_DecGOV(vop_mode_ptr);
		}else if(VISOBJSEQ_STOP_CODE == uStartCode)
		{
			ret = Mp4Dec_DecVoSeqHeader(vop_mode_ptr);
		}else if(USERDATA_START_CODE == uStartCode)
		{
			Mp4Dec_FlushBits(bitstrm_ptr, 32);
            nDecTotalBits += 32;
            while( ( (uTmpVar = Mp4Dec_ShowBits(bitstrm_ptr, 24)) != START_CODE_PREFIX )  && 
                    ( (nDecTotalBits>>3) < uOneFrameLen ) 
            )
            {
                Mp4Dec_FlushBits(bitstrm_ptr, 8);
                nDecTotalBits += 8;
			}
		}else
		{
			//skip 8 bits
			Mp4Dec_FlushBits(bitstrm_ptr, 8);
		}

		nDecTotalBits = bitstrm_ptr->bitcnt;

		if(ret != MMDEC_OK)
		{
			break;
		}
	}

	return ret;
}

MMDecRet Mp4Dec_DecH263PicInfo(DEC_VOP_MODE_T *vop_mode_ptr, int32 source_format)
{
	switch(source_format)
	{
	case 1: //SQCIF
		vop_mode_ptr->NumMBInGob = 8;
		vop_mode_ptr->NumGobInVop = 6;		
		vop_mode_ptr->num_mbline_gob = 1;
		vop_mode_ptr->OrgFrameWidth = 128;
		vop_mode_ptr->OrgFrameHeight = 96;
		break;
	case 2://QCIF
		vop_mode_ptr->NumMBInGob = 11;
		vop_mode_ptr->NumGobInVop = 9;			
		vop_mode_ptr->num_mbline_gob = 1;		
		vop_mode_ptr->OrgFrameWidth = 176;
		vop_mode_ptr->OrgFrameHeight = 144;
		break;
	case 3://CIF
		vop_mode_ptr->NumMBInGob = 22;
		vop_mode_ptr->NumGobInVop = 18;			
		vop_mode_ptr->num_mbline_gob = 1;		
		vop_mode_ptr->OrgFrameWidth = 352;
		vop_mode_ptr->OrgFrameHeight = 288;
		break;
	case 4:///4CIF
		vop_mode_ptr->NumMBInGob = 88;
		vop_mode_ptr->NumGobInVop = 18;			
		vop_mode_ptr->num_mbline_gob = 2;		
		vop_mode_ptr->OrgFrameWidth = 704;
		vop_mode_ptr->OrgFrameHeight = 576;
		break;
	case 5:///16CIF
		vop_mode_ptr->NumMBInGob = 352;
		vop_mode_ptr->NumGobInVop = 18;			
		vop_mode_ptr->num_mbline_gob = 4;		
		vop_mode_ptr->OrgFrameWidth = 1408;
		vop_mode_ptr->OrgFrameHeight = 1152;
		break;
	case 7: //Special,for asic romcode verification
		PRINTF_HEAD_INFO("extent profile, not supported!\n");
		return MMDEC_NOT_SUPPORTED;
//		break;
	default:
		{				
			PRINTF_HEAD_INFO("H.263 source format not legal\n");
			return MMDEC_STREAM_ERROR;		
		}
	}

	return MMDEC_OK;
}

/* H.263 source formats */
#define SF_SQCIF                        1 /* 001 */
#define SF_QCIF                         2 /* 010 */
#define SF_CIF                          3 /* 011 */
#define SF_4CIF                         4 /* 100 */
#define SF_16CIF                        5 /* 101 */
#define SF_CUSTOM                       6 /* 110 */
#define EXTENDED_PTYPE                  7 /* 111 */

#define EXTENDED_PAR                    15  /* 1111 */

/* Pixel aspect ration for custom source format */
#define PAR_0                           0  /* 0000 */
#define PAR_SQUARE                      1  /* 0001 */
#define PAR_CIF                         2  /* 0010 */
#define PAR_525                         3  /* 0011 */
#define PAR_CIF_STRETCHED               4  /* 0100 */
#define PAR_525_STRETCHED               5  /* 0101 */
#define PAR_EXTENDED                    15 /* 1111 */

/* picture types */
#define PCT_INTRA                       0
#define PCT_INTER                       1
#define PCT_IPB                         2
#define PCT_B                           3
#define PCT_EI                          4
#define PCT_EP                          5
#define PCT_PB                          6

MMDecRet Mp4Dec_DecH263PlusHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	H263_PLUS_HEAD_INFO_T *h263_plus = Mp4Dec_GetH263PlusHeadInfo();
	int32 tmp;
	
	PRINTF_HEAD_INFO("\n----------EXTENDED_PTYPE----------");
    h263_plus->plus_type = 1;
    h263_plus->UFEP = Mp4Dec_ReadBits(bitstrm_ptr, 3);
	PRINTF_HEAD_INFO("\nUFEP: %d", h263_plus->UFEP);
    if(h263_plus->UFEP == 1)
    {   
		/* OPPTYPE */
		PRINTF_HEAD_INFO("\n----------OPTIONAL PLUS PTYPE----------");
		h263_plus->source_format = Mp4Dec_ReadBits(bitstrm_ptr, 3);
        PRINTF_HEAD_INFO("\nsource_format: %d", h263_plus->source_format);
		
		/* optional custom picture clock frequency */
		h263_plus->optional_custom_PCF = Mp4Dec_ReadBits(bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\noptional_custom_PCF: %d", h263_plus->optional_custom_PCF);
		if(h263_plus->optional_custom_PCF)
		{
		#if 0 //Removed for support Intel H.263.
			PRINTF_HEAD_INFO("error: Optional custom picture clock frequency is not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
		#endif
		}
		
		h263_plus->mv_outside_frame = Mp4Dec_ReadBits(bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nmv_outside_frame: %d", h263_plus->mv_outside_frame);
		
		vop_mode_ptr->long_vectors = h263_plus->long_vectors = (h263_plus->mv_outside_frame ? 1 : 0);
		h263_plus->syntax_arith_coding = Mp4Dec_ReadBits(bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nsyntax_arith_coding: %d", h263_plus->syntax_arith_coding);
		
		h263_plus->adv_pred_mode = Mp4Dec_ReadBits(bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nadv_pred_mode: %d", h263_plus->adv_pred_mode);
		
		h263_plus->mv_outside_frame = (h263_plus->adv_pred_mode ? 1 : h263_plus->mv_outside_frame);
		
		h263_plus->overlapping_MC = (h263_plus->adv_pred_mode ? 1 : 0);
		h263_plus->use_4mv = (h263_plus->adv_pred_mode ? 1 : 0);
		h263_plus->pb_frame = 0;
		h263_plus->advanced_intra_coding = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nadvanced_intra_coding: %d", h263_plus->advanced_intra_coding);
		
		h263_plus->deblocking_filter_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\ndeblocking_filter_mode: %d", h263_plus->deblocking_filter_mode);
		
		h263_plus->mv_outside_frame = (h263_plus->deblocking_filter_mode ? 1 : h263_plus->mv_outside_frame);
		h263_plus->use_4mv = (h263_plus->deblocking_filter_mode ? 1 : h263_plus->use_4mv);
		
		h263_plus->slice_structured_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nslice_structured_mode: %d", h263_plus->slice_structured_mode);
		
		if(h263_plus->slice_structured_mode)
		{
			PRINTF_HEAD_INFO("error: Slice structured mode is not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<23);
			return MMDEC_NOT_SUPPORTED;
		}
		
		h263_plus->reference_picture_selection_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nreference_picture_selection_mode: %d", h263_plus->reference_picture_selection_mode);
		
		h263_plus->independently_segmented_decoding_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nindependently_segmented_decoding_mode: %d", h263_plus->independently_segmented_decoding_mode);
		
		if(h263_plus->independently_segmented_decoding_mode)
		{
			PRINTF_HEAD_INFO("error: Independently segmented decoding mode is not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<24);
			return MMDEC_NOT_SUPPORTED;
		}
		
		h263_plus->alternative_inter_VLC_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nalternative_inter_VLC_mode: %d", h263_plus->alternative_inter_VLC_mode);
		
		h263_plus->modified_quantization_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nmodified_quantization_mode: %d", h263_plus->modified_quantization_mode);
		
		tmp = Mp4Dec_ReadBits (bitstrm_ptr, 4);
        PRINTF_HEAD_INFO("\nspare, reserve, reserve, reserve: %d", tmp);
		
		if (tmp != 8)
		{                         /* OPPTYPE : bit15=1, bit16,bit17,bit18=0 */
			PRINTF_HEAD_INFO("error: The last 4 bits of OPPTYPE is expected to be 1000\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<25);
			return MMDEC_NOT_SUPPORTED;
		}
    }
	
    if ((h263_plus->UFEP == 1) || (h263_plus->UFEP == 0))
    {
		if (h263_plus->UFEP == 0)
		{
			//			PRINTF_HEAD_INFO("error: do not supported in this version\n");
			//			vop_mode_ptr->error_flag = TRUE;
			//return MMDEC_NOT_SUPPORTED;
		}
		
		/* MMPTYPE */
		PRINTF_HEAD_INFO("\n----------MANDATORY PLUS PTYPE----------");
		vop_mode_ptr->VopPredType = (VOP_PRED_TYPE_E)Mp4Dec_ReadBits (bitstrm_ptr, 3);
        PRINTF_HEAD_INFO("\npict_type: %d", vop_mode_ptr->VopPredType);
		
		if(vop_mode_ptr->VopPredType > PVOP)
		{
			PRINTF_HEAD_INFO("error: do not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<26);
			return MMDEC_NOT_SUPPORTED;
		}
		
        h263_plus->pb_frame = 0;
		
		h263_plus->reference_picture_resampling_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nreference_picture_resampling_mode: %d", h263_plus->reference_picture_resampling_mode);
		if(h263_plus->reference_picture_resampling_mode)
		{
			PRINTF_HEAD_INFO("error: Reference picture resampling mode is not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<27);
			return MMDEC_NOT_SUPPORTED;
		}
		
		h263_plus->reduced_resolution_update_mode = Mp4Dec_ReadBits (bitstrm_ptr, 1);
        PRINTF_HEAD_INFO("\nreduced_resolution_update_mode: %d", h263_plus->reduced_resolution_update_mode);
		if (h263_plus->reduced_resolution_update_mode)
		{
			PRINTF_HEAD_INFO("error: Reduced resolution update mode is not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<28);
			return MMDEC_NOT_SUPPORTED;
		}
		
		h263_plus->rtype = Mp4Dec_ReadBits (bitstrm_ptr, 1);      /* rounding type */
		PRINTF_HEAD_INFO("\nrounding_type: %d", h263_plus->rtype);
		
		tmp = Mp4Dec_ReadBits (bitstrm_ptr, 3);
        PRINTF_HEAD_INFO("\nreserve, reserve, spare: %d", tmp);
		if (tmp != 1)
		{                         /* MPPTYPE : bit7,bit8=0  bit9=1 */
			PRINTF_HEAD_INFO("error: do not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
			vop_mode_ptr->return_pos |= (1<<29);
			return MMDEC_NOT_SUPPORTED;
		}
    } else
    {
		/* UFEP is neither 001 nor 000 */
		PRINTF_HEAD_INFO("error: UFEP should be either 001 or 000.\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<30);
		return MMDEC_NOT_SUPPORTED;
    }
	
    tmp = Mp4Dec_ReadBits (bitstrm_ptr, 1);
	PRINTF_HEAD_INFO("\nCPM: %d", tmp);
    if (tmp)
    {
		PRINTF_HEAD_INFO("error: CPM not supported in this version\n");
		vop_mode_ptr->error_flag = TRUE;
    }
	
    if (h263_plus->UFEP && (h263_plus->source_format == SF_CUSTOM))
    {
		uint32 CP_PAR_code;
		/* Read custom picture format */

		CP_PAR_code = Mp4Dec_ReadBits(bitstrm_ptr, 4);

		if (CP_PAR_code != PAR_CIF)
		{ 
		#if 0 //Removed for supporting Intel H.263 format
			PRINTF_HEAD_INFO("error: do not supported in this version\n");
			vop_mode_ptr->error_flag = TRUE;
		#endif
		}

		tmp=Mp4Dec_ReadBits(bitstrm_ptr, 9);

		vop_mode_ptr->OrgFrameWidth = (tmp + 1 ) * 4;
		PRINTF_HEAD_INFO("\nCP_picture_width_indication: %d", vop_mode_ptr->OrgFrameWidth);
		tmp = Mp4Dec_ReadBits(bitstrm_ptr, 1);
		if(!tmp)
		{
			PRINTF_HEAD_INFO("error: The 14th bit of Custom Picture Format(CPFMT) should be 1\n");
			vop_mode_ptr->error_flag = TRUE;
		}

		tmp = Mp4Dec_ReadBits (bitstrm_ptr, 9);
		vop_mode_ptr->OrgFrameHeight = tmp * 4;
		PRINTF_HEAD_INFO("\nCP_picture_height_indication: %d", vop_mode_ptr->OrgFrameHeight);

		if ((vop_mode_ptr->OrgFrameWidth%16) || (vop_mode_ptr->OrgFrameHeight%16))
		{
			PRINTF_HEAD_INFO ("error: only factor of 16 custom source format supported\n");
			vop_mode_ptr->error_flag = TRUE;
		}

		if (CP_PAR_code == EXTENDED_PAR)
		{
			/*uint32 PAR_width = */Mp4Dec_ReadBits (bitstrm_ptr, 8);
			/*uint32 PAR_height = */Mp4Dec_ReadBits (bitstrm_ptr, 8);
		}
	}else
	{
		Mp4Dec_DecH263PicInfo(vop_mode_ptr, h263_plus->source_format);
	}
    
    if (h263_plus->optional_custom_PCF)
    {  
		int32 clock_conversion_code = 0;
		int32 clock_divisor = 0;
		int32 extended_temporal_reference = 0;

		PRINTF_HEAD_INFO ("\noptional_custom_PCF \n");
		if(h263_plus->UFEP)
		{
			clock_conversion_code = Mp4Dec_ReadBits(bitstrm_ptr, 1);
			PRINTF_HEAD_INFO ("\nclock_conversion_code: ");
			clock_divisor = Mp4Dec_ReadBits(bitstrm_ptr, 7);
			PRINTF_HEAD_INFO ("\nclock_divisor: ");
        
			h263_plus->CP_clock_frequency = (int32) (1800 / (/*(float)*/ clock_divisor * (8 + clock_conversion_code)) * 1000);
		}
		/* regardless of the value of UFEP */
		extended_temporal_reference = Mp4Dec_ReadBits(bitstrm_ptr, 2);
		PRINTF_HEAD_INFO ("\nextended_temporal_reference: %d", extended_temporal_reference);
      
		h263_plus->temp_ref = (extended_temporal_reference<<8) + h263_plus->temp_ref;

		if (PCT_B == vop_mode_ptr->VopPredType)
		{
			h263_plus->true_b_trb = h263_plus->temp_ref - h263_plus->prev_non_disposable_temp_ref;
		} else
		{
			h263_plus->trd = h263_plus->temp_ref - h263_plus->prev_non_disposable_temp_ref;
		}

		if (h263_plus->trd < 0)
		{
			h263_plus->trd += 1024;
		}
    }
	
    if (h263_plus->UFEP && h263_plus->long_vectors)
    {
		if (Mp4Dec_ReadBits(bitstrm_ptr, 1)) 
		{
			h263_plus->unlimited_unrestricted_motion_vectors = 0;
			PRINTF_HEAD_INFO("\nunlimited_unrestricted_motion_vectors indicator: 0");
		}
		else 
		{
			Mp4Dec_FlushBits(bitstrm_ptr, 1);
			h263_plus->unlimited_unrestricted_motion_vectors = 1;
			PRINTF_HEAD_INFO("\nunlimited_unrestricted_motion_vectors indicator: 1");
		}
    }
    if (h263_plus->UFEP && h263_plus->slice_structured_mode)
    {
		PRINTF_HEAD_INFO("error: do not supported in this version\n");
		vop_mode_ptr->error_flag = TRUE;   
		vop_mode_ptr->return_pos |= (1<<31);
		return MMDEC_NOT_SUPPORTED;
    }
	
    if (h263_plus->reference_picture_resampling_mode)
    {
        /* reading RPRP info is not implemented */
        PRINTF_HEAD_INFO("error: RPRP reading is not implemented in this version\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos2 |= (1<<1);
		return MMDEC_NOT_SUPPORTED;
	}
    
	vop_mode_ptr->StepSize = (int8)Mp4Dec_ReadBits(bitstrm_ptr, 5);

	return MMDEC_OK;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecH263Header
 ** Description:	Get the header of Vop from the bitstream,H263(short video header)
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecH263Header(DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 tmpVar;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	BOOLEAN *pIs_stop_decode_vol = &g_dec_is_stop_decode_vol;
	MMDecRet ret;

	tmpVar = Mp4Dec_ShowBits(bitstrm_ptr, SHORT_VIDEO_START_MARKER_LENGTH);

	if(SHORT_VIDEO_START_MARKER != tmpVar)
	{
		if(SHORT_VIDEO_END_MARKER == tmpVar)
		{		
			(*pIs_stop_decode_vol) = TRUE;
			PRINTF("end of sequence!\n");	
			return MMDEC_ERROR;		
		}else
		{
			return MMDEC_STREAM_ERROR;
		}
	}

	Mp4Dec_FlushBits(bitstrm_ptr, SHORT_VIDEO_START_MARKER_LENGTH);
	
	//tmpVar 
	//bit[8:1] is "h263_temporal_reference"
	//bit[0] is "h263_marker_bit"
	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 9);
	if(!(tmpVar & 0x01))
    {
		PRINTF_HEAD_INFO("This is not a legal H.263 bitstream\n");
		return MMDEC_STREAM_ERROR;		
    }

	//tmpVar 
	//bit[6] is "h263_bitstream_type"
	//bit[5] is "h263_split_screen_indicator"
	//bit[4] is "h263_document_freeze_camera"
	//bit[3] is "h263_freeze_picture_release"
	//bit[2:0] is "source_format"
	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 7);
	if(tmpVar>>3)
	{
		PRINTF_HEAD_INFO("This is not a legal H.263 bitstream\n");
		return MMDEC_NOT_SUPPORTED;
    }

	tmpVar = (tmpVar & 0x07);
	if((tmpVar != (uint32)g_dec_pre_vop_format) && (!g_dec_is_first_frame))
	{
		g_dec_is_changed_format = TRUE;
	}

	g_dec_pre_vop_format = (VOP_PRED_TYPE_E)tmpVar;

	if(tmpVar == EXTENDED_PTYPE)
	{
		ret = Mp4Dec_DecH263PlusHeader(vop_mode_ptr);

		if(ret != MMDEC_OK)
		{
			return ret;
		}
		
		if(IVOP != vop_mode_ptr->VopPredType)
		{
			vop_mode_ptr->mvInfoForward.FCode = 1;
		}
	}else
	{
		H263_PLUS_HEAD_INFO_T *h263_plus = Mp4Dec_GetH263PlusHeadInfo();

		ret = Mp4Dec_DecH263PicInfo(vop_mode_ptr, tmpVar);		
		
		if(ret != MMDEC_OK)
		{
			return ret;
		}

		//tmpVar 
		//bit[10] is "picture_coding_type"
		//bit[9:6] is "four_reserved_zero_bits"
		//bit[5:1] is "vop_quant"
		//bit[0] is "zero_bit"
		tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 11);
		
		vop_mode_ptr->VopPredType = (tmpVar>>10);

		if(IVOP != vop_mode_ptr->VopPredType)
		{
			vop_mode_ptr->mvInfoForward.FCode = 1;
		}

		if((tmpVar >> 9)&0x01) //unrestricted mv is ON
		{
			vop_mode_ptr->long_vectors = h263_plus->long_vectors = TRUE;
			PRINTF_HEAD_INFO("unrestricted mv is ON\n");
		}else
		{
			vop_mode_ptr->long_vectors = h263_plus->long_vectors = FALSE;
		}

		//tmpVar = Mp4Dec_ReadBits(3);//"three_reserved_zero_bits"
		if((tmpVar>>6)&0x07)
		{
			PRINTF_HEAD_INFO("three_reserved_zero_bits are not zero\n");
	//		return MMDEC_STREAM_ERROR;		
		}

		vop_mode_ptr->StepSize = (uint8)((tmpVar>>1)&0x1F);

		if((tmpVar)&0x01)
		{
			PRINTF_HEAD_INFO("zero_bit is not zero\n");
			return MMDEC_STREAM_ERROR;		
		}
	}

	if(g_dec_is_first_frame || g_dec_is_changed_format)
	{
		vop_mode_ptr->QuantPrecision = 5;
		vop_mode_ptr->bCoded = TRUE;
		vop_mode_ptr->RoundingControl = 0;
		vop_mode_ptr->IntraDcSwitchThr = 0;
		vop_mode_ptr->bInterlace = FALSE;	
		
		PRINTF_HEAD_INFO("frame width = %d, frame height = %d\n", vop_mode_ptr->OrgFrameWidth, vop_mode_ptr->OrgFrameHeight);
	}


	tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 1);
	if(tmpVar)
	{
		uint32 pei;
		
		pei = Mp4Dec_ReadBits(bitstrm_ptr, 8);//"psupp"

		while(1 == (tmpVar = Mp4Dec_ReadBits(bitstrm_ptr, 1)))//"pei"
		{		
			pei = Mp4Dec_ReadBits(bitstrm_ptr, 8);	//"psupp"	
		}
	}

	vop_mode_ptr->GobNum = 0;

	return MMDEC_OK;
}

PUBLIC MMDecRet Mp4Dec_FlvH263PicHeader(DEC_VOP_MODE_T *vop_mode_ptr)
{
	int32 format, width, height;
	DEC_VOP_MODE_T *s = vop_mode_ptr;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	    
	/* picture header */
    if (Mp4Dec_ReadBits(bitstrm_ptr, 17) != 1) {
        PRINTF("Bad picture start code\n");
        return MMDEC_STREAM_ERROR;//-1;
    }
    format = Mp4Dec_ReadBits(bitstrm_ptr, 5);
    if (format != 0 && format != 1) {
        PRINTF("Bad picture format\n");
        return MMDEC_STREAM_ERROR;// -1;
    }
    s->h263_flv = format+1;
    s->picture_number = Mp4Dec_ReadBits(bitstrm_ptr, 8); /* picture timestamp */
    format = Mp4Dec_ReadBits(bitstrm_ptr, 3);
		
	if((format != (int32)g_dec_pre_vop_format) && (!g_dec_is_first_frame))
	{
		g_dec_is_changed_format = TRUE;
	}

	g_dec_pre_vop_format = (VOP_PRED_TYPE_E) format;

    switch (format) {
    case 0:
        width = Mp4Dec_ReadBits(bitstrm_ptr, 8);
        height = Mp4Dec_ReadBits(bitstrm_ptr, 8);
        break;
    case 1:
        width = Mp4Dec_ReadBits(bitstrm_ptr, 16);
        height = Mp4Dec_ReadBits(bitstrm_ptr, 16);
        break;
    case 2:
        width = 352;
        height = 288;
        break;
    case 3:
        width = 176;
        height = 144;
        break;
    case 4:
        width = 128;
        height = 96;
        break;
    case 5:
        width = 320;
        height = 240;
        break;
    case 6:
        width = 160;
        height = 120;
        break;
    default:
        width = height = 0;
        break;
    }

    s->OrgFrameWidth = width;
    s->OrgFrameHeight = height;

	PRINTF("\tWidth:%d, Height:%d\n", width, height);

    s->VopPredType = IVOP + Mp4Dec_ReadBits(bitstrm_ptr, 2);

    Mp4Dec_FlushBits(bitstrm_ptr, 1); /* deblocking flag */
    s->StepSize = (int8)Mp4Dec_ReadBits(bitstrm_ptr, 5);

    s->h263_plus = 0;

    s->unrestricted_mv = 1;
    s->h263_long_vectors = 0;
    
    /* PEI */
    while (Mp4Dec_ReadBits(bitstrm_ptr, 1) != 0)
	{
        Mp4Dec_FlushBits(bitstrm_ptr, 8);
    }
    s->mvInfoForward.FCode = 1;

	if(g_dec_is_first_frame || g_dec_is_changed_format)
	{
		vop_mode_ptr->bCoded = TRUE;
		vop_mode_ptr->RoundingControl = 0;
		
	//	PRINTF_HEAD_INFO("frame width = %d, frame height = %d\n", vop_mode_ptr->OrgFrameWidth, vop_mode_ptr->OrgFrameHeight);
	}

    return MMDEC_OK;
}

PUBLIC BOOLEAN Mp4Dec_CheckResyncMarker(uint32 uAddbit)
{
	uint32 uCode;
	uint32 nStuffBits;
	uint32	bitsLeft;
	BOOLEAN	is_rsc;
	uint32 nBitsResyncmarker = 17 + uAddbit;
	DEC_BS_T *bitstrm_ptr = g_dec_vop_mode_ptr->bitstrm_ptr;
	uint32 nDecTotalBits = bitstrm_ptr->bitcnt;

	bitsLeft = 32 - (nDecTotalBits % 32);

	nStuffBits = bitsLeft & 0x7;
	if(nStuffBits == 0)
	{
		nStuffBits = 8;
	}

	uCode = Mp4Dec_ShowBits(bitstrm_ptr, nStuffBits);

	if(uCode == (((uint32)1 << (nStuffBits - 1)) - 1))
	{
		is_rsc = (Mp4Dec_ShowBitsByteAlign (bitstrm_ptr, nBitsResyncmarker) == DEC_RESYNC_MARKER) ? TRUE : FALSE;

		if (is_rsc)
		{
			//flush stuffing bits for resync start code
			Mp4Dec_ByteAlign_Mp4(bitstrm_ptr);
		}

		return is_rsc;
	}

	return FALSE;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetVideoPacketHeader
 ** Description:	Get the header of video packet.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_GetVideoPacketHeader(DEC_VOP_MODE_T *vop_mode_ptr, uint32 uAddBits)
{
	int32	tmp_var;
	int32	hec;
	int		mb_num;
	BOOLEAN	is_fake_rsc_mbnum;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
// 	MMDecRet ret = MMDEC_OK;
	
	Mp4Dec_ReadBits(bitstrm_ptr, 17 + uAddBits);//,"resync_marker"
	
	/*parsing packet header*/
	mb_num	= Mp4Dec_ReadBits(bitstrm_ptr, vop_mode_ptr->MB_in_VOP_length);//,"macro_block_num"	
	is_fake_rsc_mbnum	= ((mb_num == 0) || (mb_num >= vop_mode_ptr->MBNum))  ? 1 : 0;

	if (is_fake_rsc_mbnum)
	{ 
		vop_mode_ptr->error_flag = 1;
		vop_mode_ptr->return_pos2 |= (1<<2);
		return MMDEC_STREAM_ERROR;
	}
	vop_mode_ptr->StepSize		= (int8)Mp4Dec_ReadBits(bitstrm_ptr, vop_mode_ptr->QuantPrecision);//,"quant_scale"
	
	hec = Mp4Dec_ReadBits(bitstrm_ptr, 1);//,"header_extension_code"
	
	if(hec)
	{
		int32 bits;
		
		tmp_var = (int32)Mp4Dec_ReadBits(bitstrm_ptr, 1);//,"modulo_time_base"
		while(tmp_var == 1)
		{
			tmp_var = (int32) Mp4Dec_ReadBits(bitstrm_ptr, 1);	//,"modulo_time_base"
		}
		
		bits = (int32)vop_mode_ptr->time_inc_resolution_in_vol_length; 
		if(bits < 1)
		{
			bits = 1;
		}
		
		Mp4Dec_ReadBits(bitstrm_ptr, 1);//,"marker_bit"
		
		tmp_var = (int32)Mp4Dec_ReadBits(bitstrm_ptr, bits);//,"vop_time_increment"
		
		//tmp_var
		//bit[5], "marker_bit"
		//bit[4:3],"vop_prediction_type"
		//bit[2:0],"intra_dc_vlc_thr"
		Mp4Dec_ReadBits(bitstrm_ptr, 6);
		
		if(IVOP != vop_mode_ptr->VopPredType)
		{
			MV_INFO_T *pMvInfo = &(vop_mode_ptr->mvInfoForward);
			pMvInfo->FCode = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 3);//"vop_fcode_forward"
			pMvInfo->ScaleFactor = 1 << (pMvInfo->FCode - 1);
			pMvInfo->Range = 16 << (pMvInfo->FCode);
		}
		
		if(BVOP == vop_mode_ptr->VopPredType)
		{
			MV_INFO_T *pMvInfo = &(vop_mode_ptr->mvInfoBckward);
			pMvInfo->FCode = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 3);//"vop_fcode_backward"
			pMvInfo->ScaleFactor = 1 << (pMvInfo->FCode - 1);
			pMvInfo->Range = 16 << (pMvInfo->FCode);
		}
	}

	if (vop_mode_ptr->err_left > 0)
	{
		Mp4Dec_UpdateErrInfo (vop_mode_ptr);
	}

	//set skipped MB "NOT DECODED" status, for EC later
	if (mb_num < vop_mode_ptr->mbnumDec)
	{
		int32 mb_pos;
		for (mb_pos = mb_num; mb_pos < vop_mode_ptr->mbnumDec; mb_pos++)
		{
			vop_mode_ptr->mbdec_stat_ptr[mb_pos] = NOT_DECODED;
		}
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->err_MB_num += (vop_mode_ptr->mbnumDec - mb_num);
		vop_mode_ptr->return_pos2 |= (1<<3);
		return MMDEC_STREAM_ERROR;
	}else
	{
		int32 mb_pos;
		for (mb_pos = vop_mode_ptr->mbnumDec; mb_pos < mb_num; mb_pos++)
		{
			vop_mode_ptr->mbdec_stat_ptr[mb_pos] = NOT_DECODED;
		}
	}
	
	vop_mode_ptr->mbnumDec = mb_num;
	if (vop_mode_ptr->VopPredType != BVOP)
	{
		vop_mode_ptr->mb_y = mb_num / vop_mode_ptr->MBNumX;
		vop_mode_ptr->mb_x = mb_num - vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX;
	}
	
	vop_mode_ptr->sliceNumber++;
	
	return MMDEC_OK;
}

PUBLIC BOOLEAN Mp4Dec_SearchResynCode(DEC_VOP_MODE_T * vop_mode_ptr)
{
	int32	rsc_len;
	BOOLEAN	rsc_fnd;
	int32	dec_len;
	int32	nbits_dec_total;
	BOOLEAN	is_short_header;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

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
	
	if (!is_short_header)
	{
		Mp4Dec_ByteAlign_Startcode (bitstrm_ptr);
	}

	while (1)
	{
		nbits_dec_total = bitstrm_ptr->bitcnt;

		dec_len = nbits_dec_total >> 3;

		/*searching to frame end*/
		if (dec_len >= vop_mode_ptr->frame_len + 3)
		{
			rsc_fnd = FALSE;
			break;
		}

		rsc_fnd = (Mp4Dec_ShowBits (bitstrm_ptr, rsc_len) == DEC_RESYNC_MARKER) ? TRUE : FALSE;

		if (rsc_fnd)
		{
			break;
		}
		else
		{
			if (!is_short_header)
			{
				Mp4Dec_ReadBits (bitstrm_ptr, 8);
			}else
			{
				Mp4Dec_ReadBits (bitstrm_ptr, 1);
			}
		}
	}	
	
	return rsc_fnd;
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
