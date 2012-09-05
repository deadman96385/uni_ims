/******************************************************************************
 ** File Name:    mp4dec_mb.c                                              *
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

static const int32 s_Dec_BlkOffset[] = {0, 8, 128, 136};
static const uint8 y_tab[32]=
{
    0, 8, 8, 8, 8,10,12,14,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,34,36,38,40,42,44,46
};
static const uint8 c_tab[32]=
{
    0, 8, 8, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,20,21,22,23,24,25
};


/**----------------------------------------------------------------------------*
**                           Function Prototype                               **
**----------------------------------------------------------------------------*/
 
/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBHeader
 ** Description:	decode intra macroblock header. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 dctMd = INTRA;
	int32 MBtype;
	int32 StepSize;
	int32 iMCBPC, CBPC, CBPY;
	int32 dq_index;
	const int8 *dq_table = Mp4Dec_GetDqTable();

	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = TRUE;
	
	do
	{	
		dctMd = INTRA;

		if(READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
			"polling bsm fifo depth >= 8 words for IVOP MB header"))
		{
			vop_mode_ptr->error_flag = TRUE;
			return;
		}

		iMCBPC = Mp4Dec_VlcDecMCBPC_com_intra(vop_mode_ptr);
		
		if(vop_mode_ptr->error_flag)
		{
			PRINTF("Error decoding MCBPC of macroblock\n");
			return;
		}
		
		MBtype = iMCBPC & 7;
		if(4 == MBtype) 
		{
			dctMd = INTRAQ;
		}else if(7 == MBtype)   
		{
			dctMd = MODE_STUFFING;		
		}
		
		if(MODE_STUFFING != dctMd)
		{
			CBPC = (iMCBPC >> 4) & 3;
			
			mb_mode_ptr->bACPrediction = FALSE;
			
			if(MPEG4 == vop_mode_ptr->video_std)
			{
				mb_mode_ptr->bACPrediction = (BOOLEAN)Mp4Dec_ReadBits(1); // "ACpred_flag"
			}
			
			CBPY = Mp4Dec_VlcDecCBPY(vop_mode_ptr, TRUE);
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF ("Error decoding CBPY of macroblock 2 %d\n");
				return;
			}
			
			mb_mode_ptr->CBP = (int8)(CBPY << 2 | (CBPC));
			
			StepSize = vop_mode_ptr->StepSize;
			
			if(INTRAQ == dctMd)
			{
				dq_index = Mp4Dec_ReadBits(2); // "DQUANT"
				StepSize += dq_table[dq_index];

				if(StepSize > 31 || StepSize < 1)
				{
					PRINTF("QUANTIZER out of range 2!\n");
					StepSize = mmax(1, mmin(31, (StepSize)));				
				}

				if(FLV_H263 == vop_mode_ptr->video_std)
				{
					DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

					//modified by xwluo, 20100506, because soronsen h.263 as itu h.263
					mb_cache_ptr->iDcScalerY = 8; //y_tab[quant];
					mb_cache_ptr->iDcScalerC = 8; //c_tab[quant];
				}
			}	
			
			mb_mode_ptr->StepSize = vop_mode_ptr->StepSize = (int8)StepSize;
		}
	}while(MODE_STUFFING == dctMd);

	mb_mode_ptr->dctMd = (int8)dctMd;

#if _TRACE_
	FPRINTF (g_fp_trace_fw, "mbtype: %d, qp: %d, cbp: %d\n", MBtype, mb_mode_ptr->StepSize, mb_mode_ptr->CBP);
#endif //_TRACE_	

	return;
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBHeader
 ** Description:	decode inter macroblock header. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBHeader(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 dctMd = INTER;
	int32 len;
	int32 bIntra = FALSE;
	uint32 code;
	int32 temp, flush_bits;
	int32 COD;
	int32 MBtype;
	int32 DQUANT, quant;
	int32 iMCBPC, CBPC, CBPY;
	BOOLEAN is_mpeg4 = (vop_mode_ptr->video_std == MPEG4)?1:0;
	static int8 MBTYPE[8] = {INTER, INTERQ, INTER4V, INTRA, INTRAQ, 5, 5, MODE_STUFFING};
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = FALSE;

	quant = vop_mode_ptr->StepSize;

	do 
	{	
		dctMd = INTER;

		if(READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
			 "polling bsm fifo depth >= 8 words for PVOP MB header"))
		{
			vop_mode_ptr->error_flag = TRUE;
			return;
		}

		//COD = Mp4Dec_ReadBits (1); //"COD"
		code = Mp4Dec_ShowBits(32);
	#if _TRACE_		
//		FPRINTF (g_fp_trace_fw, "code: %d\n", code);
	#endif //_TRACE_
		COD = code >> 31;
		code = code << 1;
		flush_bits = 1;

		if(COD)
		{
			mb_mode_ptr->bSkip = TRUE;
			mb_mode_ptr->StepSize = vop_mode_ptr->StepSize;
			mb_mode_ptr->CBP = 0x00;
		}else
		{
			mb_mode_ptr->bSkip = FALSE;
			
			/*mcbpc*/
			temp = code >> 23;
			if(1 == temp)
			{
				code = code << 9;
				flush_bits += 9;
				iMCBPC = MODE_STUFFING;
			}else if(0 == temp)
			{	
				PRINTF ("Invalid MCBPC code\n");
				vop_mode_ptr->error_flag = TRUE;
				return;
			}else if(temp >= 256)
			{
				code = code << 1;
				flush_bits += 1;
				iMCBPC = 0;
			}else 
			{
				len = vop_mode_ptr->pMCBPCtab[temp].len;
				code = code << len;
				flush_bits += len;				
				iMCBPC = vop_mode_ptr->pMCBPCtab[temp].code;
			}

			MBtype = iMCBPC & 7;
			dctMd = MBTYPE[MBtype];
			bIntra = ((dctMd == INTRA)||(dctMd == INTRAQ))? TRUE: FALSE;

			if(dctMd  != MODE_STUFFING) 				
			{
				CBPC = (iMCBPC >> 4) & 3;
			#if 0  //for gmc
				if((vop_mode_ptr->be_gmc_warp) && ((mb_mode_ptr->dctMd == INTER) || (mb_mode_ptr->dctMd == INTERQ)))
				{
					mb_cache_ptr->mcsel = (char)(code >> 31);
					code = code << 1;
					flush_bits += 1;
				}else
				{
					mb_cache_ptr->mcsel = FALSE;
				}
			#endif //0
				
				mb_mode_ptr->bACPrediction = FALSE;
				
				if(is_mpeg4  && bIntra && (!vop_mode_ptr->intra_acdc_pred_disable))
				{
					mb_mode_ptr->bACPrediction = (int8)(code >> 31);
					code = code << 1;
					flush_bits += 1;
				}
		
				/*cbpy*/
				temp = code >> 26;
				if(temp < 2)
				{					
					PRINTF("Invalid CBPY4 code\n");
					vop_mode_ptr->error_flag = TRUE;
					return;
				}else if(temp >= 48)
				{
					code = code << 2;
					flush_bits += 2;
					CBPY = 15;
				}else
				{
					len = vop_mode_ptr->pCBPYtab[temp].len;
					code = code << len;
					flush_bits += len;
					CBPY = vop_mode_ptr->pCBPYtab[temp].code;
				}

				if(!bIntra)
				{
					CBPY = 15-CBPY;			
				}
				
				mb_mode_ptr->CBP = (int8)(CBPY << 2 | (CBPC));
				
				if(INTRAQ == dctMd|| INTERQ == dctMd)
				{
					const int8 *dq_table = Mp4Dec_GetDqTable();

					DQUANT = code >> 30;
					code = code << 2;
					flush_bits += 2;
					quant += dq_table[DQUANT];

					quant = (quant > 31) ? 31 : ((quant < 1) ? 1 : quant);
					
					vop_mode_ptr->StepSize = (int8)quant;

					if(FLV_H263 == vop_mode_ptr->video_std)
					{
						//modified by xwluo, 20100506, because soronsen h.263 as itu h.263
						mb_cache_ptr->iDcScalerY = 8; //y_tab[quant];
						mb_cache_ptr->iDcScalerC = 8; //c_tab[quant];
					}
				}				
				
				mb_mode_ptr->StepSize = vop_mode_ptr->StepSize;

			#if 0
				if (vop_mode_ptr->interlacing)					
				{
					/*field dct*/
					if (mb_cache_ptr->CBP || bIntra)
					{
						mb_cache_ptr->field_dct = code >> 31;
						code <<= 1;
						flush_bits += 1;

						if (mb_cache_ptr->field_dct)
						{
							PRINTF ("field dct!\n");
						}
					}

					/*field pred*/
					mb_cache_ptr->field_pred = 0;					
					mb_mode_ptr->field_pred = 0;
					if (((INTERQ == mb_mode_ptr->dctMd) || (INTER == mb_mode_ptr->dctMd)) && (!mb_cache_ptr->mcsel))
					{
						mb_cache_ptr->field_pred = code >> 31;
						mb_mode_ptr->field_pred = mb_cache_ptr->field_pred;
						code <<= 1;
						flush_bits += 1;

						if (mb_cache_ptr->field_pred)
						{
							PRINTF ("field prediction!\n");
							mb_cache_ptr->field_for_top = code >> 31;
							mb_cache_ptr->field_for_bot = (code >> 30) & 1;
							flush_bits += 2;
						}
					}					
				}
			#endif //0
			}
		}

		Mp4Dec_FlushBits(flush_bits);
		
	}while(MODE_STUFFING == dctMd);

	mb_mode_ptr->dctMd = (int8)dctMd;
	mb_mode_ptr->bIntra = (BOOLEAN)bIntra;

	if ((dctMd == INTER) || (dctMd == INTERQ))
	{
		mb_cache_ptr->mca_type = MCA_BACKWARD;
	}else if (dctMd == INTER4V)
	{
		mb_cache_ptr->mca_type = MCA_BACKWARD_4V;
	}

#if _TRACE_	
	if (!mb_mode_ptr->bSkip)
	{
		FPRINTF (g_fp_trace_fw, "mbtype: %d, qp: %d, cbp: %d, mcsel: %d\n", MBtype, quant, mb_mode_ptr->CBP, 0);
	}
#endif //_TRACE_	
}

/* for decode B-frame mb type */
LOCAL int32 Mp4Dec_GetMbType(void)
{
	int32 mb_type;

	for(mb_type = 0; mb_type <= 3; mb_type++)
	{
		if(Mp4Dec_ReadBits(1))
		{
			return mb_type;
		}
	}

	return -1;
}

/* for decode B-frame dbquant */
LOCAL int32 Mp4Dec_GetDBQuant(void)
{
	if(!Mp4Dec_ReadBits(1))/*  '0' */
	{
		return (0);
	}else if(!Mp4Dec_ReadBits(1))/* '10' */
	{
		return (-2);
	}else	/* '11' */
	{
		return (2);
	}
}

static CONST char s_mbmode_mcatype_map [5] = 
{
	MCA_BI_DRT_4V, MCA_BI_DRT, MCA_BACKWARD, MCA_FORWARD, MCA_BI_DRT_4V
};
/*****************************************************************************
 **	Name : 			Mp4Dec_DecMBHeaderBVOP
 ** Description:	decode inter macroblock header, BVOP. 
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecMBHeaderBVOP(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 mbmode;
	int32 cbp;
	int32 dquant;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 quant = vop_mode_ptr->StepSize;

	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = FALSE;

	if (READ_REG_POLL (VSP_BSM_REG_BASE+BSM_DEBUG_OFF, V_BIT_3, V_BIT_3, TIME_OUT_CLK,
			 "polling bsm fifo depth >= 8 words for BVOP MB header"))
	{
		vop_mode_ptr->error_flag = TRUE;
		return;
	}

	if(!Mp4Dec_ReadBits(1))
	{
		/* modb=='0' */
		const uint8 modb2 = (uint8)Mp4Dec_ReadBits(1);

		mbmode = Mp4Dec_GetMbType();

		if(!modb2)/* modb=='00' */
		{
			cbp = (int32)Mp4Dec_ReadBits(6);
		}else
		{
			cbp = 0;
		}

		if(mbmode && cbp)
		{
			dquant = Mp4Dec_GetDBQuant();
			quant = quant + dquant;
			quant = IClip(1, 31, quant);
		}

		if(vop_mode_ptr->bInterlace) 
		{
			PRINTF ("interlacing is not supported now!\n");
			vop_mode_ptr->error_flag = TRUE;
		}
	}else
	{
		mbmode = MODE_DIRECT_NONE_MV;
		cbp = 0;
	}

	mb_mode_ptr->dctMd = (int8)mbmode;
	mb_mode_ptr->CBP = (int8)cbp;
	vop_mode_ptr->StepSize = mb_mode_ptr->StepSize = (int8)quant;

	mb_cache_ptr->mca_type = s_mbmode_mcatype_map[mbmode];
	
#if _TRACE_
	FPRINTF (g_fp_trace_fw, "mbtype: %d, qp: %d, cbp: %d\n", mbmode, quant, cbp);
#endif //_TRACE_		
}

/*****************************************************************************
 **	Name : 			Mp4Dec_IsIntraDCSwitch
 ** Description:	Judge if need intra DC switch or not.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC BOOLEAN Mp4Dec_IsIntraDCSwitch(DEC_MB_MODE_T *mb_mode_ptr, int32 intra_dc_vlc_thr)
{
	BOOLEAN is_switched = FALSE;
	uint8 QP = mb_mode_ptr->StepSize;
	BOOLEAN bFirstMB_in_VP = mb_mode_ptr->bFirstMB_in_VP;

	if(bFirstMB_in_VP)
	{
		QP = mb_mode_ptr->StepSize; //current mb's qp
	}else
	{
		QP = (mb_mode_ptr-1)->StepSize; //left mb's qp.
	#if _DEBUG_
		if(mb_mode_ptr->StepSize != (mb_mode_ptr-1)->StepSize) //only for debug
		{
			extern void foo ();
			foo();
		}
	#endif //_DEBUG_
	}

	if(0 == intra_dc_vlc_thr) 
	{
		is_switched = FALSE;
	}else if(7 == intra_dc_vlc_thr) 
	{
		is_switched = TRUE;
	}else if(QP >= (intra_dc_vlc_thr * 2 + 11))
	{
		is_switched = TRUE;
	}
	
	return is_switched;
}

LOCAL void Mp4Dec_GetNeighborMBPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 MBPosX = vop_mode_ptr->mb_x;
	int32 MBPosY = vop_mode_ptr->mb_y;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	DEC_MB_MODE_T *pLeftMB, *pTopMB, *pLeftTopMB;
	int32 currMBPacketNumber = mb_mode_ptr->videopacket_num;
	
	mb_cache_ptr->bTopMBAvail = FALSE;
	mb_cache_ptr->bLeftTopAvail = FALSE;
	mb_cache_ptr->bLeftMBAvail = FALSE;
		
	if(MBPosY > 0)
	{
		pTopMB = mb_mode_ptr - vop_mode_ptr->MBNumX;
		if(pTopMB->bIntra && (currMBPacketNumber == pTopMB->videopacket_num))
		{
			mb_cache_ptr->topMBQP = pTopMB->StepSize;
			mb_cache_ptr->bTopMBAvail = TRUE;			
		}
		
		if(MBPosX > 0)
		{
			pLeftTopMB = pTopMB - 1;
			if(pLeftTopMB->bIntra && (currMBPacketNumber == pLeftTopMB->videopacket_num))
			{
				mb_cache_ptr->leftTopMBQP = pLeftTopMB->StepSize;
				mb_cache_ptr->bLeftTopAvail = TRUE;
			}
		}
	}
		
	if(MBPosX > 0)
	{
		pLeftMB = mb_mode_ptr - 1;
		if(pLeftMB->bIntra && (currMBPacketNumber == pLeftMB->videopacket_num))
		{
			mb_cache_ptr->leftMBQP = pLeftMB->StepSize;
			mb_cache_ptr->bLeftMBAvail = TRUE;			
		}
	}
}

LOCAL void Mp4Dec_GetLeftTopDC(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	DEC_MB_MODE_T *pTopLeftMB;
	int16 *pDCCache = mb_cache_ptr->pDCCache;
	int16 *pLeftDCCache = mb_cache_ptr->pLeftDCCache;
	int16 *pTopDCACPred = vop_mode_ptr->pTopLeftDCLine + mb_x * 3; //vop_mode_ptr->pTopCoeff + mb_x * 4 * 8;

	pDCCache[0] = DEFAULT_DC_VALUE;
	pDCCache[1] = DEFAULT_DC_VALUE;
	pDCCache[4] = DEFAULT_DC_VALUE;
	pDCCache[3] = DEFAULT_DC_VALUE;
	pDCCache[7] = DEFAULT_DC_VALUE;

	if(mb_cache_ptr->bLeftTopAvail)
	{
		pTopLeftMB = mb_mode_ptr - vop_mode_ptr->MBNumX - 1;

		if(pTopLeftMB->bIntra)
		{
			if((mb_mode_ptr-1)->bIntra) //left mb has updated pTopLeftDCLine
			{
				pDCCache[0] = (uint16)pLeftDCCache[0];
				pDCCache[3] = (uint16)pLeftDCCache[1];
				pDCCache[7] = (uint16)pLeftDCCache[2];				
			}else
			{
				pDCCache[0] = (uint16)pTopDCACPred[-3];
				pDCCache[3] = (uint16)pTopDCACPred[-2];
				pDCCache[7] = (uint16)pTopDCACPred[-1];
			}			
		}
	}

	pLeftDCCache[0] = pTopDCACPred[0];  //Y copy top DC coeff as left top DC for next MB
	pLeftDCCache[1] = pTopDCACPred[1]; //U
	pLeftDCCache[2] = pTopDCACPred[2]; //V
}

//only for sorenson h.263
LOCAL void Mp4Dec_flv_SwVLD(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr)
{
	int32 iBlkIdx;
	int16 coef_block[6][64];
	
	open_vsp_iram();	
	
	for(iBlkIdx = 0; iBlkIdx < BLOCK_CNT; iBlkIdx++)
	{
		int32 iCoefStart = 0;
		int16 *piDCTCoeff = coef_block[iBlkIdx];
		
		memset(piDCTCoeff, 0, 64 * sizeof(int16));
		
		if(mb_mode_ptr->bIntra)
		{
			int32 DCCoeff;

			/*decode DC coeff*/	
			DCCoeff = Mp4Dec_ReadBits(8);	
			if(128 == DCCoeff)
			{
				vop_mode_ptr->error_flag = TRUE;
				PRINTF ("Illegal DC coeff: 1000000\n");
				return;
			}
			
			if(255 == DCCoeff)
			{
				DCCoeff = 128;	
			}
			
			piDCTCoeff[0] = (int16)DCCoeff;	
			
			iCoefStart++;
		}else
		{
			
		}
		
		/*decode AC coeff*/
		if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
		{
			Mp4Dec_VlcDecInterTCOEF_H263(vop_mode_ptr, piDCTCoeff, iCoefStart);
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF ("decode inter coeff h263 error!\n");
				return;
			}
		}
		
#if _CMODEL_
		{
			uint32 noneZeroFlag[2];
			int16 *pBlk_asic =  (int16*)(vsp_dct_io_1 + 32*iBlkIdx);
			blk_to_asicOrder(piDCTCoeff, pBlk_asic, noneZeroFlag);
			configure_noneZeroFlag_blk (noneZeroFlag, (uint32 *)(vsp_dct_io_1 + 192 + iBlkIdx*4));	
		}
#else
		{
			uint32 i;
			uint32 uTmp;
			uint32 pTableAddr = (uint32)(MP4DEC_IDCTIO_BFR_ADDR + iBlkIdx*32*4);
			
			
			for(i = 0; i < BLOCK_SQUARE_SIZE-1; i += 2)
			{
				uTmp = ((piDCTCoeff[ASIC_DCT_Matrix[i+1]] << 16) | (piDCTCoeff[ASIC_DCT_Matrix[i]]));
				VSP_WRITE_REG(pTableAddr+ i * 2, uTmp, "configure vld coeff");
			}
			
			VSP_WRITE_REG(MP4DEC_IDCTIO_BFR_ADDR+32*4*6+iBlkIdx*4*4+0, 0xffffffff, "configure nnz");
			VSP_WRITE_REG(MP4DEC_IDCTIO_BFR_ADDR+32*4*6+iBlkIdx*4*4+4, 0xffffffff, "configure nnz");
			VSP_WRITE_REG(MP4DEC_IDCTIO_BFR_ADDR+32*4*6+iBlkIdx*4*4+8, 0xffffffff, "configure nnz");
			VSP_WRITE_REG(MP4DEC_IDCTIO_BFR_ADDR+32*4*6+iBlkIdx*4*4+12, 0xffffffff, "configure nnz");
		}
#endif
	}
		
	close_vsp_iram();	
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ConfigVldMB
 ** Description:	config command to do vld of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_ConfigVldMB(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr)
{
	uint32 cmd;

	/*configure mb type, cbp, qp*/
	cmd =  ( (vop_mode_ptr->bReversibleVlc) && (vop_mode_ptr->VopPredType != BVOP) ) ? (1<< 25) : 0;
	cmd |= ( (!mb_mode_ptr->bIntra) ? (1 << 24 ) : 0 ); 
	cmd |= ( (mb_mode_ptr->CBP << 8) );
	VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_CFG0_OFFSET, cmd, "VLD_MPEG4_CFG0: configure mb type and cbp");
	
	/*configure neighbor mb's qp, and availability, and dc_coded_as_ac, ac_pred_ena*/
	if(mb_mode_ptr->bIntra && (MPEG4 == vop_mode_ptr->video_std))
	{
		DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
		int32 QP = mb_mode_ptr->StepSize;
		
		mb_cache_ptr->iDcScalerY = g_dc_scaler_table_y[QP];
		mb_cache_ptr->iDcScalerC = g_dc_scaler_table_c[QP];

		mb_cache_ptr->bCodeDcAsAc = Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr);

		if(vop_mode_ptr->bDataPartitioning && (!mb_cache_ptr->bCodeDcAsAc))
		{
			int32 **dc_store_pptr = Mp4Dec_GetDcStore();
			int32 mb_num = vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX + vop_mode_ptr->mb_x;
			int32  *DCCoef = dc_store_pptr[mb_num];

			VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_DC_Y10_OFFSET, ((DCCoef[1]<<16)|(DCCoef[0]&0xffff)), "VLD_MPEG4_DC_Y10: configure dc of block0 and block1, for data-partitioning");
			VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_DC_Y32_OFFSET, ((DCCoef[3]<<16)|(DCCoef[2]&0xffff)), "VLD_MPEG4_DC_Y32: configure dc of block2 and block3, for data-partitioning");
			VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_DC_UV_OFFSET, ((DCCoef[5]<<16)|(DCCoef[4]&0xffff)), "VLD_MPEG4_DC_UV: configure dc of block_u and block_v, for data-partitioning");			
		}
		
		Mp4Dec_GetNeighborMBPred(vop_mode_ptr, mb_mode_ptr);
		Mp4Dec_GetLeftTopDC(vop_mode_ptr, mb_mode_ptr);	

		cmd = (mb_mode_ptr->StepSize << 24)    | (mb_cache_ptr->bTopMBAvail << 21) | (mb_cache_ptr->topMBQP << 16)          |
			(mb_cache_ptr->bLeftMBAvail << 13) | (mb_cache_ptr->leftMBQP << 8)     | (vop_mode_ptr->bDataPartitioning << 2) |
			(mb_cache_ptr->bCodeDcAsAc << 1)   | (mb_mode_ptr->bACPrediction << 0);
		VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_CFG1_OFFSET, cmd, "VLD_MPEG4_CFG1: config qp, mb availability, bDataPartitioning, bCodeDcAsAc and bACPrediction");
		
		cmd = ((uint16)(mb_cache_ptr->pDCCache[0]));
		VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_Y_OFFSET, cmd, "VLD_MPEG4_TL_DC_Y: configure top_left MB's DC for Y");
		
		cmd = (((uint16)mb_cache_ptr->pDCCache[7]) << 16) | (((uint16)(mb_cache_ptr->pDCCache[3])) << 0);
		VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_UV_OFFSET, cmd, "VLD_MPEG4_TL_DC_UV: configure top_left MB's DC for UV");
	}

	/*start vld one MB*/
	VSP_WRITE_REG(VSP_VLD_REG_BASE+VLD_CTL_OFFSET, 1, "VLD_CTL: configure VLD start");

#if _CMODEL_
	vld_module();
#endif //_CMODEL_
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ConfigIqIdctMB
 ** Description:	config command to do iq idct of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
LOCAL void Mp4Dec_ConfigIqIdctMB(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	if(READ_REG_POLL(VSP_VLD_REG_BASE+VLD_CTL_OFFSET, V_BIT_31, 0, TIME_OUT_CLK,"VLD: polling VLD one MB status")||
		((VSP_READ_REG(VSP_VLD_REG_BASE+VLD_CTL_OFFSET, "VLD: check vld error flag")>>30)&0x01))
	{
		vop_mode_ptr->error_flag = TRUE;
		return;
	}else
	{	
		uint32 uDctCfg;
		uint32 uMpeg4DequanPara = (mb_mode_ptr->StepSize << 16);
//		BOOLEAN quantMode = (vop_mode_ptr->QuantizerType == Q_H263) ? 1 : 0;
		
		if(mb_mode_ptr->bIntra)
		{
			DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

			if(MPEG4 == vop_mode_ptr->video_std)
			{
				int16 *pTopDCACPred = vop_mode_ptr->pTopLeftDCLine + vop_mode_ptr->mb_x * 3; //vop_mode_ptr->pTopCoeff + mb_x * 4 * 8;
				int32 uv_dc = 0;

				pTopDCACPred[0] = (int16)VSP_READ_REG(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_Y_OFFSET, "VLD_MPEG4_TL_DC_Y: read out current MB's Y3 dc");
				uv_dc = (int32)VSP_READ_REG(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_UV_OFFSET, "VLD_MPEG4_TL_DC_UV: read out current MB's U and V dc");
				pTopDCACPred[1] = (uv_dc<<16)>>16;
				pTopDCACPred[2] = (uv_dc<<0)>>16;		
			}

			mb_mode_ptr->CBP = 0x3f;
			uDctCfg = vop_mode_ptr->intra_dct_cfg;
			uMpeg4DequanPara |= ((mb_cache_ptr->iDcScalerC << 8)|(mb_cache_ptr->iDcScalerY));
		}else
		{
			uDctCfg = vop_mode_ptr->inter_dct_cfg;
		}
		
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CONFIG_OFF, uDctCfg, "write DCT_CONFIG reg ");
		VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_MPEG4_DQUANT_PARA_OFF, uMpeg4DequanPara, "configure dequant para");

	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_IN_CBP_OFF, mb_mode_ptr->CBP, "configure cbp");
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_CFG_FINISH_OFF, 1,"configure finish of IDCT");
	VSP_WRITE_REG(VSP_DCT_REG_BASE+DCT_START_OFF, 1,"start IDCT");

	#if _CMODEL_
		dct_module();	
	#endif //_CMODEL_
	}
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBTexture
 ** Description:	Get the texture of intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	if (!vop_mode_ptr->sw_vld_flag)
	{
		Mp4Dec_ConfigVldMB(vop_mode_ptr, mb_mode_ptr);
	}else
	{
		Mp4Dec_flv_SwVLD(vop_mode_ptr, mb_mode_ptr);
	}

	/*iq idct*/
	Mp4Dec_ConfigIqIdctMB(vop_mode_ptr, mb_mode_ptr);
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecInterMBTexture
 ** Description:	Get the texture of inter macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecInterMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	if(mb_mode_ptr->CBP == 0)
	{
		return; //do not need start VLD and idct.
	}

	if (!vop_mode_ptr->sw_vld_flag)
	{
		Mp4Dec_ConfigVldMB(vop_mode_ptr, mb_mode_ptr);
	}else
	{
		Mp4Dec_flv_SwVLD(vop_mode_ptr, mb_mode_ptr);
	}

	/*iq idct*/
	Mp4Dec_ConfigIqIdctMB(vop_mode_ptr, mb_mode_ptr);
}

LOCAL void BS_and_Para (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
//	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
//	int32 topEdgeFilterFlag = mb_y > 0;
//	int32 leftEdgeFilterFlag = mb_x > 0;
	int32 chroma_qp_offset = 0;
	int32 LFAlphaC0Offset = 12;//20;//
	int32 LFBetaOffset = 12;//20;//
	int32 topMB_qp, leftMB_qp;
	int32 qp_tbl[32] = {0, 2, 3, 5, 7, 8, 10, 11, 13, 15, 16, 18, 20, 21, 
		23, 24, 26, 28, 29, 31, 33, 34, 36, 37, 39, 41, 42,44, 46,  47, 49, 51
	};
// 	uint32 bs[32];
	uint32 cmd = 0;

	if (mb_y)
	{
		topMB_qp = (mb_mode_ptr-vop_mode_ptr->MBNumX)->StepSize;
	}else
	{
		topMB_qp = 0;
	}

	if (mb_x)
	{
		leftMB_qp = (mb_mode_ptr-1)->StepSize;
	}else
	{
		leftMB_qp = 0;
	}

	cmd = ((mb_x << 24) | (qp_tbl[topMB_qp] << 16) | (qp_tbl[leftMB_qp] << 8) | (qp_tbl[mb_mode_ptr->StepSize] << 0));
	VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");

	cmd = (((chroma_qp_offset & 0x1f) << 16) | ((LFAlphaC0Offset & 0x1f) << 8) | ((LFBetaOffset & 0x1f) << 0));
	VSP_WRITE_REG (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");

#if 0 //config BS in frame level
#if 1
	mb_cache_ptr->BS[0] = 0x04040404;
	mb_cache_ptr->BS[1] = 0x04040404;
	mb_cache_ptr->BS[2] = 0x04040404;
	mb_cache_ptr->BS[3] = 0x04040404;
	mb_cache_ptr->BS[4] = 0x04040404;
	mb_cache_ptr->BS[5] = 0x04040404;
	mb_cache_ptr->BS[6] = 0x04040404;
	mb_cache_ptr->BS[7] = 0x04040404;		
#else
	mb_cache_ptr->BS[0] = 0x0;//0x00040004;
	mb_cache_ptr->BS[1] = 0x0;//0x00040004;
	mb_cache_ptr->BS[2] = 0x0;//0x00040004;
	mb_cache_ptr->BS[3] = 0x0;//0x00040004;
	mb_cache_ptr->BS[4] = 0x0;//0x00040004;
	mb_cache_ptr->BS[5] = 0x0;//0x00040004;
	mb_cache_ptr->BS[6] = 0x0;//0x00040004;
	mb_cache_ptr->BS[7] = 0x0;//0x00040004;							
#endif	

	/*vertical edge BS*/
	bs[0] = (mb_cache_ptr->BS[0] >> 0) & 0xff;
	bs[1] = (mb_cache_ptr->BS[1] >> 0) & 0xff;
	bs[2] = (mb_cache_ptr->BS[2] >> 0) & 0xff;
	bs[3] = (mb_cache_ptr->BS[3] >> 0) & 0xff;

	bs[4] = (mb_cache_ptr->BS[0] >> 8) & 0xff;
	bs[5] = (mb_cache_ptr->BS[1] >> 8) & 0xff;
	bs[6] = (mb_cache_ptr->BS[2] >> 8) & 0xff;
	bs[7] = (mb_cache_ptr->BS[3] >> 8) & 0xff;

	cmd = (bs[0] << 0)  | (bs[1] << 4)  | (bs[2] << 8)  | (bs[3] << 12) |
		    (bs[4] << 16) | (bs[5] << 20) | (bs[6] << 24) | (bs[7] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, cmd, "configure bs h0");

	bs[8] = (mb_cache_ptr->BS[0] >> 16) & 0xff;
	bs[9] = (mb_cache_ptr->BS[1] >> 16) & 0xff;
	bs[10]= (mb_cache_ptr->BS[2] >> 16) & 0xff;
	bs[11]= (mb_cache_ptr->BS[3] >> 16) & 0xff;

	bs[12]= (mb_cache_ptr->BS[0] >> 24) & 0xff;
	bs[13]= (mb_cache_ptr->BS[1] >> 24) & 0xff;
	bs[14]= (mb_cache_ptr->BS[2] >> 24) & 0xff;
	bs[15]= (mb_cache_ptr->BS[3] >> 24) & 0xff;

	cmd = (bs[8] << 0)   | (bs[9] << 4)   | (bs[10] << 8)  | (bs[11] << 12) |
			(bs[12] << 16) | (bs[13] << 20) | (bs[14] << 24) | (bs[15] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_H1_OFF, cmd, "configure bs h1");

	/*horizontal edge BS*/
	bs[16] = (mb_cache_ptr->BS[4] >> 0) & 0xff;
	bs[17] = (mb_cache_ptr->BS[5] >> 0) & 0xff;
	bs[18] = (mb_cache_ptr->BS[6] >> 0) & 0xff;
	bs[19] = (mb_cache_ptr->BS[7] >> 0) & 0xff;

	bs[20] = (mb_cache_ptr->BS[4] >> 8) & 0xff;
	bs[21] = (mb_cache_ptr->BS[5] >> 8) & 0xff;
	bs[22] = (mb_cache_ptr->BS[6] >> 8) & 0xff;
	bs[23] = (mb_cache_ptr->BS[7] >> 8) & 0xff;

	cmd = (bs[16] << 0)  | (bs[17] << 4)  | (bs[18] << 8)  | (bs[19] << 12) |
		    (bs[20] << 16) | (bs[21] << 20) | (bs[22] << 24) | (bs[23] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, cmd, "configure bs v0");

	bs[24] = (mb_cache_ptr->BS[4] >> 16) & 0xff;
	bs[25] = (mb_cache_ptr->BS[5] >> 16) & 0xff;
	bs[26] = (mb_cache_ptr->BS[6] >> 16) & 0xff;
	bs[27] = (mb_cache_ptr->BS[7] >> 16) & 0xff;

	bs[28] = (mb_cache_ptr->BS[4] >> 24) & 0xff;
	bs[29] = (mb_cache_ptr->BS[5] >> 24) & 0xff;
	bs[30] = (mb_cache_ptr->BS[6] >> 24) & 0xff;
	bs[31] = (mb_cache_ptr->BS[7] >> 24) & 0xff;

	cmd = (bs[24] << 0)  | (bs[25] << 4)  | (bs[26] << 8)  | (bs[27] << 12) |
			(bs[28] << 16) | (bs[29] << 20) | (bs[30] << 24) | (bs[31] << 28);
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_BS_V1_OFF, cmd, "configure bs v1");
#endif	

//	VSP_WRITE_REG(VSP_DBK_REG_BASE+DBK_CTR0_OFF, 1, "start dbk");
	VSP_WRITE_REG(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");

	return;
}

PUBLIC void Mp4Dec_MBC_DBK_Cmd(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	uint32 vsp_cbp;

	if(mb_mode_ptr->bIntra)
	{
		vsp_cbp = 0x3f;
	}else
	{
		vsp_cbp = mb_mode_ptr->CBP;
	}

	VSP_WRITE_REG(VSP_MBC_REG_BASE+MBC_CMD0_OFF,( ( (!mb_mode_ptr->bIntra) ? (1<<30):0 ) | (vsp_cbp << 0) ), "MBC_CMD0: configure mb type and cbp");

	BS_and_Para (vop_mode_ptr, mb_mode_ptr);

	return;	
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
