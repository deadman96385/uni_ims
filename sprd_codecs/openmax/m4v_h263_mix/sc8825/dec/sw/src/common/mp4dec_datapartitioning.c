/******************************************************************************
 ** File Name:    mp4dec_datapartitioning.c                                   *
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

#ifdef _MP4CODEC_DATA_PARTITION_
/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
/*****************************************************************************
 **	Name : 			Mp4Dec_GetIVOPMBHeaderDataPartitioning
 ** Description:	get a IVOP MB's header of first data partition. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_GetIVOPMBHeaderDataPartitioning(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 mbnumDec)
{
	int32 MCBPC = 0;
	int32 MBtype = 0;
	int32 blk_num = 0;
	int32 dctMd = INTRA;
	int32 StepSize = vop_mode_ptr->StepSize;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	
	mb_mode_ptr->CBP = 0;
	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = TRUE;

	do 
	{
		dctMd = INTRA;
		
		MCBPC = Mp4Dec_VlcDecMCBPC_com_intra(vop_mode_ptr);

		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos |= (1<<1);
			PRINTF ("Error decoding MCBPC of macroblock %d\n");
			return;
		}
		
		MBtype = MCBPC & 7;
		
		if(4 == MBtype) 
		{
			dctMd = INTRAQ;		
		}else if(7 == MBtype)
		{
			dctMd = MODE_STUFFING;
		}
		
		if(dctMd != MODE_STUFFING)
		{
			mb_mode_ptr->CBP = (char)((MCBPC >> 4) & 3);
			
			if(INTRAQ == dctMd)
			{	
				const int8 *dq_table = Mp4Dec_GetDqTable();
				int32 DQUANT = (int32)Mp4Dec_ReadBits(bitstrm_ptr, 2);// "DQUANT"
			
				StepSize += dq_table[DQUANT];
				
				if(StepSize > 31 || StepSize < 1)
				{
					PRINTF("QUANTIZER out of range! 4\n");
					StepSize = mmax(1, mmin (31, StepSize));
				}					
			}

			mb_mode_ptr->StepSize = (int8)StepSize; //Notes: Dont delete this code line,for next Mp4Dec_IsIntraDCSwitch function. xiaowei.luo,20081226

		#if _TRACE_	
			FPRINTF (g_fp_trace_fw, "\tdctMd:%d, cbp:%d,qp:%d\n", dctMd, mb_mode_ptr->CBP, mb_mode_ptr->StepSize);
		#endif //_TRACE_	

			if(!Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr))
			{
				int32 **dc_store_pptr = vop_mode_ptr->g_dec_dc_store;
				int32 *DCCoef = dc_store_pptr[mbnumDec];

			#if 0 //this condition is impossible, because intra_acdc_pred_disable is setted to "FALSE" when dec vol header.Noted by xiaowei,20081208
				if(vop_mode_ptr->intra_acdc_pred_disable)
				{
					for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
					{
						;
					}
				}else
			#endif //0	
				{
					for(blk_num = 0 ;blk_num < BLOCK_CNT; blk_num++)		
					{
						DCCoef[blk_num] = (int16)Mp4Dec_VlcDecPredIntraDC(vop_mode_ptr, blk_num);		
					}
				#if _TRACE_			
					FPRINTF (g_fp_trace_fw, "\tDCCoef:%d, %d, %d, %d, %d, %d\n", DCCoef[0], DCCoef[1], DCCoef[2], 
						DCCoef[3], DCCoef[4], DCCoef[5]);
				#endif //_TRACE_	
				}
			}
		}
	}while ((MODE_STUFFING == dctMd) && !(DEC_DC_MARKER == Mp4Dec_ShowBits(bitstrm_ptr, DC_MARKER_LENGTH))); 

	mb_mode_ptr->StepSize = vop_mode_ptr->StepSize = (int8)StepSize;
	mb_mode_ptr->dctMd = (int8)dctMd;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetPVOPMBHeaderDataPartitioning
 ** Description:	get a PVOP MB's header of first data partition. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_GetPVOPMBHeaderDataPartitioning(DEC_VOP_MODE_T *vop_mode_ptr,	DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 dctMd = INTER;
	int32 COD;
	int32 MCBPC;
	static int8 MBTYPE[8] = {INTER, INTERQ, INTER4V, INTRA, INTRAQ, 5, 5, MODE_STUFFING};
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	mb_mode_ptr->CBP = 0;
	
	do 
	{
		dctMd = INTER;

		COD = Mp4Dec_ReadBits(bitstrm_ptr, 1); //"NO DCT FLAG"

		if (COD)
		{	/*SKIP flag*/
			mb_mode_ptr->bSkip = TRUE;
			mb_mode_ptr->CBP = 0x00;
		}else
		{
			mb_mode_ptr->bSkip = FALSE;
			MCBPC = Mp4Dec_VlcDecMCBPC_com_inter(vop_mode_ptr);

			if(vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->return_pos |= (1<<2);
				PRINTF ("Error decoding MCBPC of macroblock\n");
				return;
			}

			dctMd = MBTYPE[MCBPC & 7];
			
			mb_mode_ptr->CBP = (char)((MCBPC >> 4) & 3);
		}

	#if _TRACE_			
		FPRINTF (g_fp_trace_fw, "\tdctMd:%d, cbp:%d\n", dctMd, mb_mode_ptr->CBP);
	#endif //_TRACE_	
	}while (!COD && (dctMd == MODE_STUFFING)
		&& (Mp4Dec_ShowBits(bitstrm_ptr, MOTION_MARKER_COMB_LENGTH) != MOTION_MARKER_COMB));	

	mb_mode_ptr->dctMd = (int8)dctMd;
	mb_mode_ptr->bIntra = ((dctMd == INTRA)||(dctMd == INTRAQ))? TRUE: FALSE;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetSecondPartitionMBHeaderIVOP
 ** Description:	get second part MB header of data partition, IVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetSecondPartitionMBHeaderIVOP(DEC_VOP_MODE_T *vop_mode_ptr,DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 CBPY;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	if(!vop_mode_ptr->intra_acdc_pred_disable)
	{
		mb_mode_ptr->bACPrediction = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1); // "ACpred_flag"
	}
    
    CBPY = Mp4Dec_VlcDecCBPY(vop_mode_ptr, TRUE);

    if(vop_mode_ptr->error_flag)
	{
		vop_mode_ptr->return_pos |= (1<<3);
		PRINTF ("Error decoding CBPY of macroblock 4\n");
		return;
	}   
    
    mb_mode_ptr->CBP = (uint8)(CBPY << 2 | mb_mode_ptr->CBP); //the whole cbp value is here. Noted by xiaowei.luo,20081208
}

/*****************************************************************************
 **	Name : 			Mp4Dec_GetSecondPartitionMBHeaderPVOP
 ** Description:	get second part MB header of data partition, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_GetSecondPartitionMBHeaderPVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr, int32 mb_num)
{
	int32 blk_num;
	int32 CBPY;
	int8 mb_dctMd= mb_mode_ptr->dctMd;
	BOOLEAN bIsIntra = mb_mode_ptr->bIntra;
	int32 StepSize = vop_mode_ptr->StepSize;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	if(bIsIntra && (!vop_mode_ptr->intra_acdc_pred_disable))
	{
		mb_mode_ptr->bACPrediction = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1); // "ACpred_flag"
	}

	CBPY = Mp4Dec_VlcDecCBPY(vop_mode_ptr, bIsIntra);

	if(vop_mode_ptr->error_flag)
	{
		vop_mode_ptr->return_pos |= (1<<4);
		PRINTF ("Error decoding CBPY of macroblock 5\n");
		return;
	}
	
	mb_mode_ptr->CBP = (int8)(CBPY << 2 | mb_mode_ptr->CBP);
	
	if((INTRAQ == mb_dctMd) || (INTERQ == mb_dctMd))
	{
		const int8 *dq_table = Mp4Dec_GetDqTable();
		int32 DQUANT = Mp4Dec_ReadBits(bitstrm_ptr, 2);  // "DQUANT"

		StepSize += dq_table[DQUANT];	
		if(StepSize > 31 || StepSize < 1)
		{
			PRINTF ("QUANTIZER out of range! 5\n");
			StepSize = mmax(1, mmin (31, (StepSize)));
		}		
	}				
	
	mb_mode_ptr->StepSize = vop_mode_ptr->StepSize = StepSize;	//Notes: Dont delete this code line,for next Mp4Dec_IsIntraDCSwitch function. xiaowei.luo,20081226

	if(mb_mode_ptr->bIntra)
	{
		if(!Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr))
		{
			int32 **dc_store_pptr = vop_mode_ptr->g_dec_dc_store;
			int32 *DCCoef = dc_store_pptr[mb_num];

			for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
			{
			#if 0 //this condition is impossible, because intra_acdc_pred_disable is setted to "FALSE" when dec vol header.Noted by xiaowei,20081208
				if(vop_mode_ptr->intra_acdc_pred_disable) 
				{
					/**/
					DCStore[blk_num] = Mp4Dec_ReadBits(bitstrm_ptr, 8); //, "DC coeff"
					
					if(128 == DCStore[blk_num])
					{
						vop_mode_ptr->error_flag = TRUE;
						PRINTF ("Illegal DC value\n");
						return;
					}
					if(255 == DCStore[blk_num])
					{
						DCCoef[blk_num] = 128;
					}
				}else
			#endif //0	
				{
					DCCoef[blk_num] = (int16)Mp4Dec_VlcDecPredIntraDC(vop_mode_ptr, blk_num);	
					//printf("dc_coeff = %d\t", DCStore[blk_num]);
				}
			}
			//printf("\n");
		}
	}
}

#if 0
/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraRVLCTCOEF
 ** Description:	get rvlc tcoef of intra macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:			vld, de-scan
 *****************************************************************************/
PUBLIC void Mp4Dec_RvlcIntraTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 * iDCTCoefQ, int32 iCoefStart)
{
	int32 i = iCoefStart;
	int32 last = 0;
	TCOEF_T run_level = {0, 0, 0, 0};
	const uint8 *rgiZigzag = vop_mode_ptr->pZigzag; 
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	
	do 
	{
		Mp4Dec_GetIntraRvlcTcoef(vop_mode_ptr, bitstrm_ptr, &run_level);
		
		i += run_level.run;
		
		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos |= (1<<5);
			return;
		}
		
		if(i >= 64)
		{
			vop_mode_ptr->return_pos |= (1<<6);
			vop_mode_ptr->error_flag = TRUE;
			PRINTF ("TOO MUCH COEFF !!");
			return;
		}
		
		if(run_level.sign == 1)
		{
			iDCTCoefQ [rgiZigzag[i]] = - run_level.level;
		}else
		{
			iDCTCoefQ [rgiZigzag[i]] = run_level.level;
		}
		
		last = run_level.last;
		i++;	
	}while(!last);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterRVLCTCOEF
 ** Description:	get rvlc tcoef of inter macroblock. 
 ** Author:			Xiaowei Luo
 **	Note:			rvld, de-scan
 *****************************************************************************/
PUBLIC void Mp4Dec_RvlcInterTCOEF(DEC_VOP_MODE_T *vop_mode_ptr, int16 *iDCTCoefQ)
{
	int32 i = 0;
	int32 last = 0;
	TCOEF_T run_level = {0, 0, 0, 0};
	const uint8 *rgiZigzag = vop_mode_ptr->pZigzag; 
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	do 
	{
		Mp4Dec_GetInterRvlcTcoef(vop_mode_ptr, bitstrm_ptr, &run_level);
		
		i += run_level.run;
		
		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos |= (1<<7);
			return;
		}
		
		if(i >= 64)
		{
			vop_mode_ptr->return_pos |= (1<<8);
			vop_mode_ptr->error_flag = TRUE;
			PRINTF ("TOO MUCH COEFF !\n");
			return;
		}

		if(run_level.sign == 1)
		{
			iDCTCoefQ [rgiZigzag[i]] = - run_level.level;
		}else
		{
			iDCTCoefQ [rgiZigzag[i]] = run_level.level;
		}
		
		last = run_level.last;
		i++;		
	}while(!last);
}
/*****************************************************************************
 **	Name : 			Mp4Dec_DecSecondPartitionIntraErrRes
 ** Description:	get second part of data partition, IVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_DecSecondPartitionIntraErrRes(DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no)
{
	int32 mb_num = start_mb_no;
	DEC_MB_MODE_T *mb_mode_ptr = PNULL;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	
	/*decode MB header in second partition*/
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

	#if _DEBUG_
		if(mb_num == 34)
		{
			mb_num = mb_num;
		}
	#endif //_DEBUG_
		
		Mp4Dec_GetSecondPartitionMBHeaderIVOP(vop_mode_ptr, mb_mode_ptr);
		
		mb_num++;		
	}
		
	/*decode MB texture in second partition*/
	mb_num = start_mb_no;
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		int32 mb_x, mb_y;
		mb_x = mb_num % total_mb_num_x;
		mb_y = mb_num / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

	#if _DEBUG_
		if((mb_y == 2)&&(mb_x == 0)&&(vop_mode_ptr->g_nFrame_dec == 0))
		{
			foo();
		}
	#endif //_DEBUG_

		Mp4Dec_VspMBInit(mb_x, mb_y);
		
		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos1 |= (1<<5);
			PRINTF("mbc error!\n");
			return;
		}

		/*configure MBC and DBK*/
		Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_ptr);
		
		/*decode a MB*/
		Mp4Dec_DecIntraMBTexture_hw(vop_mode_ptr, mb_mode_ptr);

		if(!vop_mode_ptr->error_flag)
		{
		//	Mp4Dec_CheckMBCStatus(vop_mode_ptr);
			vop_mode_ptr->err_MB_num--;
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_NOT_IN_ERR_PKT;

#if 1
			if (vop_mode_ptr->err_left != 0)
			{
				int32 mb_end_pos;
				int32 err_start_pos;
				
				/*determine whether the mb is located in error domain*/
				mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;		

				err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;

				if (mb_end_pos >= err_start_pos)
				{
					vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_IN_ERR_PKT;
				}
			}
#endif			
		}else
		{
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = NOT_DECODED;
			PRINTF ("decode intra mb coeff error!\n");
		}

		//mbc is started by done signal of dct and mca automatically
		mb_num++;
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecSecondPartitionInterErrRes
 ** Description:	get second part of data partition, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_DecSecondPartitionInterErrRes(DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no)
{
	int32 mb_num = start_mb_no;
	DEC_MB_MODE_T *mb_mode_ptr = PNULL;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	
#if _DEBUG_
	if((vop_mode_ptr->g_nFrame_dec == 242)&&(vop_mode_ptr->mbnumDec == 86))
	{
		foo();
	}
#endif //_DEBUG_

	/*decode MB header in second partition*/
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		int32 mb_x, mb_y;
	
		mb_x = mb_num % total_mb_num_x;
		mb_y = mb_num / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

		if(!mb_mode_ptr->bSkip)
		{
			Mp4Dec_GetSecondPartitionMBHeaderPVOP(vop_mode_ptr, mb_mode_ptr, mb_num);

			if(vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->return_pos |= (1<<9);
				return;
			}
		}
		mb_num++;
	}
		
	/*decode MB texture in second partition*/
	mb_num = start_mb_no;
	while(mb_num < (vop_mode_ptr->mbnumDec))
	{
		int32 mb_x, mb_y;
	
		mb_x = mb_num % total_mb_num_x;
		mb_y = mb_num / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

		mb_mode_ptr = vop_mode_ptr->pMbMode + mb_num;

	#if _DEBUG_
		if((mb_y == 0)&&(mb_x == 0x2)&&(vop_mode_ptr->g_nFrame_dec == 1))
		{
			foo();
		}
	#endif //_DEBUG_
	#if _DEBUG_
		if((mb_num >= 81)&&(vop_mode_ptr->g_nFrame_dec == 9))
		{
			PRINTF("%mb bskip :%d, cbp = %d, bIntra = %d\n", mb_mode_ptr->bSkip, mb_mode_ptr->CBP, mb_mode_ptr->bIntra);
		}
	#endif //_DEBUG_

		Mp4Dec_VspMBInit(mb_x, mb_y);
		
		if(vop_mode_ptr->error_flag)
		{
			vop_mode_ptr->return_pos1 |= (1<<6);
			PRINTF("mbc error!\n");
			return;
		}
			
		/*configure MBC and DBK*/
		Mp4Dec_MBC_DBK_Cmd(vop_mode_ptr, mb_mode_ptr);

		if(mb_mode_ptr->bIntra)
		{				
			/*vld*/
			Mp4Dec_DecIntraMBTexture_hw(vop_mode_ptr, mb_mode_ptr);
			if(vop_mode_ptr->error_flag)
			{
				vop_mode_ptr->return_pos |= (1<<10);
				PRINTF("decode intra mb of pVop error!\n");
				return;
			}	
		}else
		{	
			DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

			if((INTER == mb_mode_ptr->dctMd) || (INTERQ == mb_mode_ptr->dctMd))
			{
				mb_cache_ptr->mca_type = MCA_BACKWARD;
			}else if(INTER4V == mb_mode_ptr->dctMd)
			{
				mb_cache_ptr->mca_type = MCA_BACKWARD_4V;
			}

			//set mv range
			vop_mode_ptr->mv_x_max = (total_mb_num_x - mb_x) << 5;
			vop_mode_ptr->mv_x_min = (-mb_x - 1) << 5;
			vop_mode_ptr->mv_y_max = (vop_mode_ptr->MBNumY - mb_y) << 5; 
			vop_mode_ptr->mv_y_min = (-mb_y - 1) << 5;


			/*decode inter data partition MB */
			if(mb_mode_ptr->bSkip)
			{	
				MOTION_VECTOR_T zeroMv = {0, 0};

				mb_mode_ptr->dctMd = MODE_NOT_CODED;
				
				/*configure zero mv to mca*/
				Mp4Dec_StartMcaOneDir(vop_mode_ptr, mb_cache_ptr, &zeroMv);
			}else
			{
				Mp4Dec_StartMcaOneDir (vop_mode_ptr, mb_cache_ptr, mb_mode_ptr->mv);
				
				/*vld*/
				Mp4Dec_DecInterMBTexture_hw(vop_mode_ptr, mb_mode_ptr);
			}		
		}

		if(!vop_mode_ptr->error_flag)
		{
			int32 mb_end_pos;
			int32 err_start_pos;

		//	Mp4Dec_CheckMBCStatus(vop_mode_ptr);

			vop_mode_ptr->err_MB_num--;
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_NOT_IN_ERR_PKT;
#if 1
			if (vop_mode_ptr->err_left != 0)
			{
				/*determine whether the mb is located in error domain*/
				mb_end_pos = (vop_mode_ptr->bitstrm_ptr->bitcnt + 7) / 8;	

				err_start_pos = vop_mode_ptr->err_pos_ptr[0].start_pos;

				if (mb_end_pos >= err_start_pos)
				{
					vop_mode_ptr->mbdec_stat_ptr[mb_num] = DECODED_IN_ERR_PKT;
				}
			}	
#endif			
		}else
		{
			vop_mode_ptr->mbdec_stat_ptr[mb_num] = NOT_DECODED;
			PRINTF ("\ndecode inter mb of pVop error!");
		}	

		mb_num++;	
	}	
}
#endif

/*****************************************************************************
 **	Name : 			Mp4Dec_DecFirstPartitionIntraErrRes
 ** Description:	get first part of data partition, IVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_DecFirstPartitionIntraErrRes(DEC_VOP_MODE_T *vop_mode_ptr)
{
	DEC_MB_MODE_T *mb_mode_ptr = PNULL;
	uint8 bFirstMB_in_VP = TRUE;
	int32 total_mb_num = vop_mode_ptr->MBNum;
	int32 mbnumDec = vop_mode_ptr->mbnumDec;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	
#if _TRACE_			
	FPRINTF(g_fp_trace_fw, "\n----------------------------\n", mbnumDec);
#endif //_TRACE_	
	
	do 
	{
	#if _DEBUG_
		if((vop_mode_ptr->mbnumDec == 395) && (vop_mode_ptr->g_nFrame_dec == 10))
		{
			vop_mode_ptr = vop_mode_ptr;
		}
	#endif //_DEBUG_

		mb_mode_ptr = vop_mode_ptr->pMbMode + mbnumDec;
		mb_mode_ptr->bFirstMB_in_VP = bFirstMB_in_VP;
		mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);	
		mb_mode_ptr->bIntra = TRUE;		
		mb_mode_ptr->dctMd = INTRA;
		mb_mode_ptr->bSkip = FALSE;

	#if _TRACE_			
		FPRINTF (g_fp_trace_fw, "\nNo.%d MB header\n", mbnumDec);
	#endif //_TRACE_	
		
		Mp4Dec_GetIVOPMBHeaderDataPartitioning(vop_mode_ptr, mb_mode_ptr, mbnumDec);
		
		if(MODE_STUFFING == mb_mode_ptr->dctMd)
		{
			break;
		}
		
		((int*)mb_mode_ptr->mv)[0] = 0;   //set to zero for B frame'mv prediction
		((int*)mb_mode_ptr->mv)[1] = 0;
		((int*)mb_mode_ptr->mv)[2] = 0;
		((int*)mb_mode_ptr->mv)[3] = 0;

		mbnumDec++;
		bFirstMB_in_VP = FALSE;
	}while(((Mp4Dec_ShowBits(bitstrm_ptr, DC_MARKER_LENGTH)) != DEC_DC_MARKER) && (mbnumDec < total_mb_num));
	vop_mode_ptr->mbnumDec = mbnumDec; //must set back!
	
	if(mbnumDec == total_mb_num) 
	{
		if((Mp4Dec_ShowBits(bitstrm_ptr, DC_MARKER_LENGTH)) != DEC_DC_MARKER) 
		{
			while(1 == Mp4Dec_ShowBits(bitstrm_ptr, 9)) 
			{
				Mp4Dec_FlushBits(bitstrm_ptr, 9);
			}
			
			if((Mp4Dec_ShowBits(bitstrm_ptr, DC_MARKER_LENGTH)) != DEC_DC_MARKER) 
			{
				PRINTF("! DEC_DC_MARKER\n");
				vop_mode_ptr->error_flag = TRUE;
				vop_mode_ptr->return_pos |= (1<<11);
				return;
			}
		}
	}
	
	/** end of MCBPC Stuffing (Oki) 24-MAY-2000 **/	
	if(mbnumDec > total_mb_num)
	{
		PRINTF("mb_num > vop_mode_ptr->MBNum!\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<12);
		return;
	}	
	
	Mp4Dec_ReadBits(bitstrm_ptr, DC_MARKER_LENGTH);  //, "dc marker"
	
	return;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecFirstPartitionInterErrRes
 ** Description:	get first part of data partition, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_DecFirstPartitionInterErrRes(DEC_VOP_MODE_T *vop_mode_ptr)
{
	DEC_MB_MODE_T *mb_mode_ptr = PNULL;
	int8 bFirstMB_in_VP = TRUE;
	int32 mb_x, mb_y;
	int32 total_mb_num_x = vop_mode_ptr->MBNumX;
	int32 total_mb_num = vop_mode_ptr->MBNum;
	int32 mbnumDec = vop_mode_ptr->mbnumDec;
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;
	
#if _TRACE_			
	FPRINTF (g_fp_trace_fw, "\n----------------------------\n", mbnumDec);
#endif //_TRACE_	

	do 
	{
	#if _DEBUG_
		if((mbnumDec == 84) && (vop_mode_ptr->g_nFrame_dec == 242))
		{
			foo();
		}
	#endif //_DEBUG_

		mb_mode_ptr = vop_mode_ptr->pMbMode + mbnumDec;
		mb_mode_ptr->bFirstMB_in_VP = bFirstMB_in_VP;
		mb_mode_ptr->videopacket_num = (uint8)(vop_mode_ptr->sliceNumber);	
		mb_mode_ptr->bIntra = FALSE;		
		mb_mode_ptr->dctMd = INTER;
		mb_mode_ptr->bSkip = FALSE;

	#if _TRACE_		
		FPRINTF (g_fp_trace_fw, "\nNo.%d MB header\n", mbnumDec);
	#endif //_TRACE_	
	
		Mp4Dec_GetPVOPMBHeaderDataPartitioning(vop_mode_ptr, mb_mode_ptr);	
		
		if (MODE_STUFFING == mb_mode_ptr->dctMd)
		{
			break;
		}
	#if _DEBUG_	
		if(vop_mode_ptr->g_nFrame_dec == 9)
		{
			PRINTF("--------%d MB, cbp = %d, dctmd = %d------\n",mbnumDec, mb_mode_ptr->CBP, mb_mode_ptr->dctMd);
		}
	#endif //_DEBUG_		

		mb_x = mbnumDec % total_mb_num_x;
		mb_y = mbnumDec / total_mb_num_x;

		vop_mode_ptr->mb_x = (int8)mb_x;
		vop_mode_ptr->mb_y = (int8)mb_y;

	#if _DEBUG_
		if((mb_x == 4)&&(mb_y == 7) && (vop_mode_ptr->g_nFrame_dec == 9))
		{
			foo();
		}
	#endif //_DEBUG_	

		if(vop_mode_ptr->VSP_used)
		{
	#if 0	
			Mp4Dec_DecMV_hw(vop_mode_ptr, mb_mode_ptr);
	#endif
		}else
		{
			Mp4Dec_DecMV_DP_vt(vop_mode_ptr, mb_mode_ptr,vop_mode_ptr->mb_cache_ptr);
		}
	
		mbnumDec++;
		bFirstMB_in_VP = FALSE;
	}while(((Mp4Dec_ShowBits(bitstrm_ptr, MOTION_MARKER_COMB_LENGTH)) != MOTION_MARKER_COMB) && (mbnumDec < total_mb_num));
	vop_mode_ptr->mbnumDec = mbnumDec; //must set back!
	
	if(mbnumDec == total_mb_num) 
	{
		if((Mp4Dec_ShowBits(bitstrm_ptr, MOTION_MARKER_COMB_LENGTH)) != MOTION_MARKER_COMB) 
		{
			while(1 == Mp4Dec_ShowBits(bitstrm_ptr, 10)) 
			{
				Mp4Dec_FlushBits(bitstrm_ptr, 10);
			}
			if((Mp4Dec_ShowBits(bitstrm_ptr, MOTION_MARKER_COMB_LENGTH)) != MOTION_MARKER_COMB)
			{
				PRINTF("!=MOTION_MARKER_COMB\n");
				vop_mode_ptr->error_flag = TRUE;
				vop_mode_ptr->return_pos |= (1<<13);
				return;
			}
		}
	}
	
	/** end of MCBPC Stuffing (Oki) 24-MAY-2000 **/	
	if(mbnumDec > total_mb_num)
	{
		PRINTF ("mb_num > vop_mode_ptr->MBNum!\n");
		vop_mode_ptr->error_flag = TRUE;
		vop_mode_ptr->return_pos |= (1<<14);
		return;
	}	
	
	Mp4Dec_ReadBits(bitstrm_ptr, MOTION_MARKER_COMB_LENGTH);  //,"motion marker"

	return;
}

static MMDecRet Mp4Dec_ResyncDataPartitioning (DEC_VOP_MODE_T *vop_mode_ptr, int32 start_mb_no, uint32 uAddBits)
{
	/*error concealment*/
	int32 rsc_found;
	MMDecRet ret = MMDEC_OK;
	
	vop_mode_ptr->error_flag = FALSE;
	
	rsc_found = Mp4Dec_SearchResynCode (vop_mode_ptr);
	if (!rsc_found)
	{
		vop_mode_ptr->stop_decoding = TRUE;
	}else
	{					
		ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, uAddBits);
		if (vop_mode_ptr->error_flag)
		{
			PRINTF ("decode resync header error!\n");
			vop_mode_ptr->return_pos |= (1<<15);
			return ret;
		}

	//	vop_mode_ptr->VopPredType = PVOP; //here, for use MCA hardware module
		if(vop_mode_ptr->VSP_used)
		{
	#if 0	
			ret = Mp4Dec_EC_FillMBGap (vop_mode_ptr);
	#endif
		}
	//	vop_mode_ptr->VopPredType = IVOP;  //here, set it's original value
		if (ret != MMDEC_OK)
		{
			return ret;
		}
	}

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIVOPErrResDataPartitioning
 ** Description:	get data of data partition, IVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecIVOPErrResDataPartitioning(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
	MMDecRet ret = MMDEC_OK;
	
	while(!vop_mode_ptr->stop_decoding)
	{
		int32 start_mb_no = vop_mode_ptr->mbnumDec;
	#if _DEBUG_
		if((vop_mode_ptr->mb_x == 0xa) && (vop_mode_ptr->mb_y == 0x8) && (vop_mode_ptr->g_nFrame_dec == 0x13))
		{
			foo();
		}
	#endif //_DEBUG_

		Mp4Dec_DecFirstPartitionIntraErrRes(vop_mode_ptr);
			
		if(vop_mode_ptr->error_flag)
		{
			/*error concealment*/
			ret = Mp4Dec_ResyncDataPartitioning (vop_mode_ptr, start_mb_no, 0);
#if 0	//remvode by xweiluo@20120201, for CQM mode			
			if (ret != MMDEC_HW_ERROR)
			{
				continue;
			}else
			{
				return ret;
			}
#else
			continue;
#endif			
		}

		if(vop_mode_ptr->VSP_used)
		{
		#if 0
			Mp4Dec_DecSecondPartitionIntraErrRes(vop_mode_ptr, start_mb_no);
		#endif
		}else
		{
			Mp4Dec_DecSecondPartitionIntraErrRes_vt(vd, vop_mode_ptr, start_mb_no);
		}
		
		if(vop_mode_ptr->error_flag)
		{
			/*error concealment*/
			ret = Mp4Dec_ResyncDataPartitioning (vop_mode_ptr, start_mb_no, 0);
#if 0	//remvode by xweiluo@20120201, for CQM mode			
			if (ret != MMDEC_HW_ERROR)
			{
				continue;
			}else
			{
				return ret;
			}
#else
			continue;
#endif			
		}
		
		/**/
		if(Mp4Dec_CheckResyncMarker(vop_mode_ptr, 0) && (vop_mode_ptr->mbnumDec < vop_mode_ptr->MBNum))
		{
			ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, 0);
#if 0	//remvode by xweiluo@20120201, for CQM mode			
			if (ret != MMDEC_HW_ERROR)
			{
				continue;
			}else
			{
				return ret;
			}
#else
			continue;
#endif			
		}else
		{
			vop_mode_ptr->stop_decoding = TRUE;
		}		
	}	

	return ret;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecPVOPErrResDataPartitioning
 ** Description:	get data of data partition, PVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC MMDecRet Mp4Dec_DecPVOPErrResDataPartitioning(MP4DecObject *vd, DEC_VOP_MODE_T *vop_mode_ptr)
{
	uint32 uAddBits =vop_mode_ptr->mvInfoForward.FCode - 1;
	MMDecRet ret = MMDEC_OK;

	while(!vop_mode_ptr->stop_decoding)
	{
		int32 start_mb_no = vop_mode_ptr->mbnumDec;
		
		Mp4Dec_DecFirstPartitionInterErrRes(vop_mode_ptr);
		
		if(vop_mode_ptr->error_flag)
		{
			/*error concealment*/
			ret = Mp4Dec_ResyncDataPartitioning (vop_mode_ptr, start_mb_no, uAddBits);
#if 0	//remvode by xweiluo@20120201, for CQM mode			
			if (ret != MMDEC_HW_ERROR)
			{
				continue;
			}else
			{
				return ret;
			}
#else
			continue;
#endif			
		}
		if(vop_mode_ptr->VSP_used)
		{
		#if 0
			Mp4Dec_DecSecondPartitionInterErrRes(vop_mode_ptr, start_mb_no);
		#endif
		}else
		{
			Mp4Dec_DecSecondPartitionInterErrRes_vt(vd, vop_mode_ptr, start_mb_no);
		}
		
		if(vop_mode_ptr->error_flag)
		{
			/*error concealment*/
			ret = Mp4Dec_ResyncDataPartitioning (vop_mode_ptr, start_mb_no, uAddBits);
#if 0	//remvode by xweiluo@20120201, for CQM mode			
			if (ret != MMDEC_HW_ERROR)
			{
				continue;
			}else
			{
				return ret;
			}
#else
			continue;
#endif			
		}

		/**/
		if(Mp4Dec_CheckResyncMarker(vop_mode_ptr, uAddBits) && (vop_mode_ptr->mbnumDec < vop_mode_ptr->MBNum))
		{
			ret = Mp4Dec_GetVideoPacketHeader(vop_mode_ptr, vop_mode_ptr->mvInfoForward.FCode - 1);
#if 0	//remvode by xweiluo@20120201, for CQM mode			
			if (ret != MMDEC_HW_ERROR)
			{
				continue;
			}else
			{
				return ret;
			}
#else
			continue;
#endif			
		}else
		{
			vop_mode_ptr->stop_decoding = TRUE;
		}			
	}	

	return ret;
}

#endif //_MP4CODEC_DATA_PARTITION_
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
