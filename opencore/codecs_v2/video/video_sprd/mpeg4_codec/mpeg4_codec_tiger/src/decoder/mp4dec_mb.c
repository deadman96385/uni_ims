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
#include "tiger_video_header.h"
/**---------------------------------------------------------------------------*
**                        Compiler Flag                                       *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    extern   "C" 
    {
#endif

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
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = TRUE;
	
	do
	{	
		dctMd = INTRA;

		iMCBPC = Mp4Dec_VlcDecMCBPC_com_intra(vop_mode_ptr);
		
		if(vop_mode_ptr->error_flag)
		{
			PRINTF("Error decoding MCBPC of macroblock\n");
			vop_mode_ptr->return_pos2 |= (1<<6);
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
				mb_mode_ptr->bACPrediction = (BOOLEAN)Mp4Dec_ReadBits(bitstrm_ptr, 1); // "ACpred_flag"
			}
			
			CBPY = Mp4Dec_VlcDecCBPY(vop_mode_ptr, TRUE);
			
			if(vop_mode_ptr->error_flag)
			{
				PRINTF ("Error decoding CBPY of macroblock 2 %d\n");
				vop_mode_ptr->return_pos2 |= (1<<7);
				return;
			}
			
			mb_mode_ptr->CBP = (int8)(CBPY << 2 | (CBPC));
			
			StepSize = vop_mode_ptr->StepSize;
			
			if(INTRAQ == dctMd)
			{
				dq_index = Mp4Dec_ReadBits(bitstrm_ptr, 2); // "DQUANT"
				StepSize += dq_table[dq_index];

				if(StepSize > 31 || StepSize < 1)
				{
					PRINTF("QUANTIZER out of range 2!\n");
					StepSize = mmax(1, mmin(31, (StepSize)));				
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
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = FALSE;

	quant = vop_mode_ptr->StepSize;

	do 
	{	
		dctMd = INTER;

		//COD = Mp4Dec_ReadBits (1); //"COD"
		code = Mp4Dec_ShowBits(bitstrm_ptr, 32);
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
				vop_mode_ptr->return_pos2 |= (1<<8);
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
					vop_mode_ptr->return_pos2 |= (1<<9);
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

		Mp4Dec_FlushBits(bitstrm_ptr, flush_bits);
		
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
LOCAL int32 Mp4Dec_GetMbType(DEC_BS_T *bitstrm_ptr)
{
	int32 mb_type;

	for(mb_type = 0; mb_type <= 3; mb_type++)
	{
		if(Mp4Dec_ReadBits(bitstrm_ptr, 1))
		{
			return mb_type;
		}
	}

	return -1;
}

/* for decode B-frame dbquant */
LOCAL int32 Mp4Dec_GetDBQuant(DEC_BS_T *bitstrm_ptr)
{
	if(!Mp4Dec_ReadBits(bitstrm_ptr, 1))/*  '0' */
	{
		return (0);
	}else if(!Mp4Dec_ReadBits(bitstrm_ptr, 1))/* '10' */
	{
		return (-2);
	}else	/* '11' */
	{
		return (2);
	}
}

CONST char s_mbmode_mcatype_map [5] = 
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
	DEC_BS_T *bitstrm_ptr = vop_mode_ptr->bitstrm_ptr;

	mb_mode_ptr->bSkip = FALSE;
	mb_mode_ptr->bIntra = FALSE;

	if(!Mp4Dec_ReadBits(bitstrm_ptr, 1))
	{
		/* modb=='0' */
		const uint8 modb2 = (uint8)Mp4Dec_ReadBits(bitstrm_ptr, 1);

		mbmode = Mp4Dec_GetMbType(bitstrm_ptr);

		if(!modb2)/* modb=='00' */
		{
			cbp = (int32)Mp4Dec_ReadBits(bitstrm_ptr, 6);
		}else
		{
			cbp = 0;
		}

		if(mbmode && cbp)
		{
			dquant = Mp4Dec_GetDBQuant(bitstrm_ptr);
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
	int32 is_switched = FALSE;

	if(0 == intra_dc_vlc_thr) 
	{
		is_switched = FALSE;
	}else if(7 == intra_dc_vlc_thr) 
	{
		is_switched = TRUE;
	}else
	{
		uint8 QP;// = mb_mode_ptr->StepSize;

		if(mb_mode_ptr->bFirstMB_in_VP)
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

		if(QP >= (intra_dc_vlc_thr * 2 + 11))
		{
			is_switched = TRUE;
		}
	}
	
	return (BOOLEAN)is_switched;
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
	int16 *pTopDCACPred = vop_mode_ptr->pTopLeftDCLine + mb_x * 4; //vop_mode_ptr->pTopCoeff + mb_x * 4 * 8;

	pDCCache[0] = DEFAULT_DC_VALUE;
	pDCCache[1] = DEFAULT_DC_VALUE;
	pDCCache[4] = DEFAULT_DC_VALUE;
	pDCCache[3] = DEFAULT_DC_VALUE;
	pDCCache[7] = DEFAULT_DC_VALUE;

	if(mb_cache_ptr->bTopMBAvail)
	{
		DEC_MB_MODE_T *pTopMB = mb_mode_ptr - vop_mode_ptr->MBNumX;
		if(pTopMB->bIntra)
		{
			pDCCache[1] = pTopDCACPred[0];
		}
	}

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

	if(mb_cache_ptr->bLeftMBAvail)
	{
		DEC_MB_MODE_T *pLeftMB = mb_mode_ptr - 1;
		if(pLeftMB->bIntra)
		{
			pDCCache[4] = pDCCache[6];
		}
	}

	pLeftDCCache[0] = pTopDCACPred[1];  //Y copy top DC coeff as left top DC for next MB
	pLeftDCCache[1] = pTopDCACPred[2]; //U
	pLeftDCCache[2] = pTopDCACPred[3]; //V
}

/*****************************************************************************
 **	Name : 			Mp4Dec_ConfigVldMB
 ** Description:	config command to do vld of one mb.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
void Mp4Dec_ConfigVldMB(DEC_VOP_MODE_T * vop_mode_ptr, DEC_MB_MODE_T * mb_mode_ptr)
{
	uint32 i, cmd;
	uint32 flush_bits = vop_mode_ptr->bitstrm_ptr->bitcnt - vop_mode_ptr->bitstrm_ptr->bitcnt_before_vld;
	uint32 nWords = flush_bits/32;

	flush_bits -= nWords*32;
	VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, ((flush_bits<<24) | 1), "BSM_CFG2: flush one byte");	
#if _CMODEL_
	flush_nbits(flush_bits);
#endif
	VSP_WRITE_CMD_INFO((VSP_BSM << 29) | (1<<24) | BSM_CFG2_WOFF);

	if (nWords)
	{
		cmd = (32<<24) | 1;

		for (i = nWords; i > 0; i--)
		{
			VSP_READ_REG_POLL_CQM(VSP_BSM_REG_BASE+BSM_DEBUG_OFF, 3, 1, 1, "polling bsm fifo fifo depth >= 8 words for gob header");
			VSP_WRITE_REG_CQM(VSP_BSM_REG_BASE+BSM_CFG2_OFF, cmd, "BSM_CFG2: flush one byte");	
	#if _CMODEL_
			flush_nbits(32);
	#endif
				
			VSP_WRITE_CMD_INFO((VSP_BSM << 29) | (2<<24) | (BSM_CFG2_WOFF<<8) | ((1<<7)|BSM_DEBUG_WOFF));
		}
	}

	/*configure mb type, cbp, qp*/
	cmd =  ( (vop_mode_ptr->bReversibleVlc) && (vop_mode_ptr->VopPredType != BVOP) ) ? (1<< 25) : 0;
	cmd |= ( (!mb_mode_ptr->bIntra) ? (1 << 24 ) : 0 ); 
	cmd |= ( (mb_mode_ptr->CBP << 8) );
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_CFG0_OFFSET, cmd, "VLD_MPEG4_CFG0: configure mb type and cbp");
	VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (1<<24) | VLD_MPEG4_CFG0_WOFF);
	
	/*configure neighbor mb's qp, and availability, and dc_coded_as_ac, ac_pred_ena*/
	if(mb_mode_ptr->bIntra && (MPEG4 == vop_mode_ptr->video_std))
	{
		DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
		
		cmd = (mb_mode_ptr->StepSize << 24)    | (mb_cache_ptr->bTopMBAvail << 21) | (mb_cache_ptr->topMBQP << 16)          |
			(mb_cache_ptr->bLeftMBAvail << 13) | (mb_cache_ptr->leftMBQP << 8)     | (vop_mode_ptr->bDataPartitioning << 2) |
			(mb_cache_ptr->bCodeDcAsAc << 1)   | (mb_mode_ptr->bACPrediction << 0);
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_CFG1_OFFSET, cmd, "VLD_MPEG4_CFG1: config qp, mb availability, bDataPartitioning, bCodeDcAsAc and bACPrediction");
		
		cmd = ((uint16)(mb_cache_ptr->pDCCache[0]));
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_Y_OFFSET, cmd, "VLD_MPEG4_TL_DC_Y: configure top_left MB's DC for Y");
		
		cmd = (((uint16)mb_cache_ptr->pDCCache[7]) << 16) | (((uint16)(mb_cache_ptr->pDCCache[3])) << 0);
		VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_TL_DC_UV_OFFSET, cmd, "VLD_MPEG4_TL_DC_UV: configure top_left MB's DC for UV");
		VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (3<<24) | (VLD_MPEG4_TL_DC_UV_WOFF <<16) | (VLD_MPEG4_TL_DC_Y_WOFF << 8) | VLD_MPEG4_CFG1_WOFF); 

		if(vop_mode_ptr->bDataPartitioning && (!mb_cache_ptr->bCodeDcAsAc))
		{
			int32 **dc_store_pptr = Mp4Dec_GetDcStore();
			int32 mb_num = vop_mode_ptr->mb_y * vop_mode_ptr->MBNumX + vop_mode_ptr->mb_x;
			int32  *DCCoef = dc_store_pptr[mb_num];

			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_DC_Y10_OFFSET, ((DCCoef[1]<<16)|(DCCoef[0]&0xffff)), "VLD_MPEG4_DC_Y10: configure dc of block0 and block1, for data-partitioning");
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_DC_Y32_OFFSET, ((DCCoef[3]<<16)|(DCCoef[2]&0xffff)), "VLD_MPEG4_DC_Y32: configure dc of block2 and block3, for data-partitioning");
			VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_MPEG4_DC_UV_OFFSET, ((DCCoef[5]<<16)|(DCCoef[4]&0xffff)), "VLD_MPEG4_DC_UV: configure dc of block_u and block_v, for data-partitioning");			
			VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (3<<24) | (VLD_MPEG4_DC_UV_WOFF <<16) | (VLD_MPEG4_DC_Y32_WOFF << 8) | VLD_MPEG4_DC_Y10_WOFF); 
		}
	}

	/*start vld one MB*/
	VSP_WRITE_REG_CQM(VSP_VLD_REG_BASE+VLD_CTL_OFFSET, 1, "VLD_CTL: configure VLD start");
	VSP_READ_REG_POLL_CQM(VSP_VLD_REG_BASE+VLD_CTL_OFFSET,31, 1, 0,"VLD: polling VLD one MB status");
	VSP_WRITE_CMD_INFO((VSP_VLD << 29) | (2<<24) | (((1<<7)|VLD_CTL_WOFF)<<8) | VLD_CTL_WOFF);

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
	uint32 uDctCfg;
	uint32 uMpeg4DequanPara = (mb_mode_ptr->StepSize << 16);

	if(mb_mode_ptr->bIntra)
	{
		DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;

		mb_mode_ptr->CBP = 0x3f;
		uDctCfg = vop_mode_ptr->intra_dct_cfg;
		uMpeg4DequanPara |= ((mb_cache_ptr->iDcScalerC << 8)|(mb_cache_ptr->iDcScalerY));
	}else
	{
		uDctCfg = vop_mode_ptr->inter_dct_cfg;
	}

	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_CONFIG_OFF, uDctCfg, "write DCT_CONFIG reg ");
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_MPEG4_DQUANT_PARA_OFF, uMpeg4DequanPara, "configure dequant para");

	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_IN_CBP_OFF, mb_mode_ptr->CBP, "configure cbp");
	VSP_WRITE_REG_CQM(VSP_DCT_REG_BASE+DCT_START_OFF, 1,"start IDCT");

	VSP_WRITE_CMD_INFO((VSP_DCT<<29) | (4<<24) | (DCT_IN_CBP_WOFF<<16) | (DCT_MPEG4_DQUANT_PARA_WOFF<<8) | DCT_CONFIG_WOFF);
	VSP_WRITE_CMD_INFO(DCT_START_WOFF);

#if _CMODEL_
	dct_module();	
#endif //_CMODEL_
}

/*****************************************************************************
 **	Name : 			Mp4Dec_DecIntraMBTexture
 ** Description:	Get the texture of intra macroblock.
 ** Author:			Xiaowei Luo
 **	Note:
 *****************************************************************************/
PUBLIC void Mp4Dec_DecIntraMBTexture(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 QP = mb_mode_ptr->StepSize;
	DEC_MB_BFR_T *mb_cache_ptr = vop_mode_ptr->mb_cache_ptr;
	int32 uv_dc = 0;
	int32 blk_num;

	mb_cache_ptr->bCodeDcAsAc = FALSE;

	if(MPEG4 == vop_mode_ptr->video_std)
	{
		mb_cache_ptr->iDcScalerY = g_dc_scaler_table_y[QP];
		mb_cache_ptr->iDcScalerC = g_dc_scaler_table_c[QP];
		mb_cache_ptr->bCodeDcAsAc = Mp4Dec_IsIntraDCSwitch(mb_mode_ptr, vop_mode_ptr->IntraDcSwitchThr);

		Mp4Dec_GetNeighborMBPred(vop_mode_ptr, mb_mode_ptr);
		Mp4Dec_GetLeftTopDC(vop_mode_ptr, mb_mode_ptr);
	}

	Mp4Dec_ConfigVldMB(vop_mode_ptr, mb_mode_ptr);

	for(blk_num = 0; blk_num < BLOCK_CNT; blk_num++)
	{
		g_Mp4Dec_GetIntraBlkTCoef(vop_mode_ptr, mb_mode_ptr, blk_num);

		if(vop_mode_ptr->error_flag)
		{
			PRINTF("Mp4Dec_GetIntraBlockTexture error!\n");
			vop_mode_ptr->return_pos2 |= (1<<10);
			return;
		}
	}
	vop_mode_ptr->bitstrm_ptr->bitcnt_before_vld = vop_mode_ptr->bitstrm_ptr->bitcnt;

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
	int32 iBlkIdx;
	
	if(mb_mode_ptr->CBP == 0)
	{
		return; //do not need start VLD and idct.
	}

	Mp4Dec_ConfigVldMB(vop_mode_ptr, mb_mode_ptr);

	/*sw vld*/
	for (iBlkIdx = 0; iBlkIdx < BLOCK_CNT; iBlkIdx++)
	{
		if(mb_mode_ptr->CBP & (32 >> iBlkIdx))
		{
			if(MPEG4 != vop_mode_ptr->video_std)
			{
				Mp4Dec_VlcDecInterTCOEF_H263(vop_mode_ptr/*, pBlk*/, 0, vop_mode_ptr->bitstrm_ptr);
			}
			else 
			{
				if((!vop_mode_ptr->bReversibleVlc) || (BVOP == vop_mode_ptr->VopPredType))
				{
					Mp4Dec_VlcDecInterTCOEF_Mpeg(vop_mode_ptr/*, pBlk*/, vop_mode_ptr->bitstrm_ptr);
				}
			#ifdef _MP4CODEC_DATA_PARTITION_	
				else
				{
					int16 *pBlk = vop_mode_ptr->coef_block[iBlkIdx];
					Mp4Dec_RvlcInterTCOEF(vop_mode_ptr, pBlk);
				}
			#endif //DATA_PARTITION	

			}		
		
			if (vop_mode_ptr->error_flag)
			{
				PRINTF ("decodeTextureInterBlock error!\n");	
				vop_mode_ptr->return_pos2 |= (1<<11);
				return;
			}
		}
	}

	vop_mode_ptr->bitstrm_ptr->bitcnt_before_vld = vop_mode_ptr->bitstrm_ptr->bitcnt;

	/*iq idct*/
	Mp4Dec_ConfigIqIdctMB(vop_mode_ptr, mb_mode_ptr);
}

const uint32 s_dbk_qp_tbl[32] = 
{
	0, 2, 3, 5, 7, 8, 10, 11, 13, 15, 16, 18, 20, 21, 
		23, 24, 26, 28, 29, 31, 33, 34, 36, 37, 39, 41, 42,44, 46,  47, 49, 51
};

LOCAL void BS_and_Para (DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
{
	int32 mb_x = vop_mode_ptr->mb_x;
	int32 mb_y = vop_mode_ptr->mb_y;
	int32 chroma_qp_offset = 0;
	int32 LFAlphaC0Offset = 12;		//jin 20110602 DBK strength
	int32 LFBetaOffset = 12;
	int32 topMB_qp, leftMB_qp;

 	uint32 bs[32];
	uint32 cmd = 0;

if(BVOP == vop_mode_ptr->VopPredType)
{
	topMB_qp = leftMB_qp = 0;

	//polling dbk ready
	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 0, 1, 0,"dbk: polling dbk cfg finish flag = 0");

	cmd = ((mb_x << 24) | (s_dbk_qp_tbl[topMB_qp] << 16) | (s_dbk_qp_tbl[leftMB_qp] << 8) | (s_dbk_qp_tbl[mb_mode_ptr->StepSize] << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");

	cmd = (((chroma_qp_offset & 0x1f) << 16) | ((LFAlphaC0Offset & 0x1f) << 8) | ((LFBetaOffset & 0x1f) << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");

	cmd = 0;//.mb_x ? 0x33334444 : 0x33330000;
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, cmd, "configure bs h0");

	cmd = 0;//mb_y ? 0x33334444 : 0x33330000;
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, cmd, "configure bs v0");

	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");

	VSP_WRITE_CMD_INFO((VSP_DBK << 29) | (6<<24) |(HDBK_PARS_WOFF<<16) | (HDBK_MB_INFO_WOFF<<8) |((1<<7)|HDBK_CFG_FINISH_WOFF));
	VSP_WRITE_CMD_INFO( (HDBK_CFG_FINISH_WOFF<<16) |(HDBK_BS_V0_WOFF<<8) | (HDBK_BS_H0_WOFF));	
}
else
{
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
	
	//polling dbk ready
	VSP_READ_REG_POLL_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 0, 1, 0,"dbk: polling dbk cfg finish flag = 0");

	cmd = ((mb_x << 24) | (s_dbk_qp_tbl[topMB_qp] << 16) | (s_dbk_qp_tbl[leftMB_qp] << 8) | (s_dbk_qp_tbl[mb_mode_ptr->StepSize] << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_MB_INFO_OFF, cmd, "configure mb information");

	cmd = (((chroma_qp_offset & 0x1f) << 16) | ((LFAlphaC0Offset & 0x1f) << 8) | ((LFBetaOffset & 0x1f) << 0));
	VSP_WRITE_REG_CQM (VSP_DBK_REG_BASE+HDBK_PARS_OFF, cmd, "configure dbk parameter");

	cmd = mb_x ? 0x33334444 : 0x33330000;
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_H0_OFF, cmd, "configure bs h0");

	cmd = mb_y ? 0x33334444 : 0x33330000;
	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_BS_V0_OFF, cmd, "configure bs v0");

	VSP_WRITE_REG_CQM(VSP_DBK_REG_BASE+HDBK_CFG_FINISH_OFF, 1, "config finished");

	VSP_WRITE_CMD_INFO((VSP_DBK << 29) | (6<<24) |(HDBK_PARS_WOFF<<16) | (HDBK_MB_INFO_WOFF<<8) |((1<<7)|HDBK_CFG_FINISH_WOFF));
	VSP_WRITE_CMD_INFO( (HDBK_CFG_FINISH_WOFF<<16) |(HDBK_BS_V0_WOFF<<8) | (HDBK_BS_H0_WOFF));
}	
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

	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_CMD0_OFF,( ( (!mb_mode_ptr->bIntra) ? (1<<30):0 ) | (vsp_cbp << 0) ), "MBC_CMD0: configure mb type and cbp");
	VSP_WRITE_CMD_INFO( (VSP_MBC << 29) | (1<<24) | MBC_CMD0_WOFF);

	BS_and_Para (vop_mode_ptr, mb_mode_ptr);

	return;	
}

PUBLIC void Mp4Dec_VspMBInit (int32 mb_x, int32 mb_y)
{
	uint32 cmd;

#if _CMODEL_
	#include "buffer_global.h"
	memset(vsp_fw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));	
	memset(vsp_bw_mca_out_bfr, 0, MCA_BFR_SIZE*sizeof(uint32));		
#endif //_CMODEL_

	cmd  = (mb_y << 8) | (mb_x << 0);
	VSP_WRITE_REG_CQM (VSP_GLB_REG_BASE+GLB_CTRL0_OFF, cmd, "vsp global reg: configure current MB position");
	VSP_WRITE_CMD_INFO((VSP_GLB << 29) | (1<<24) | GLB_CTRL0_WOFF);
}

PUBLIC void Mp4Dec_CheckMBCStatus(DEC_VOP_MODE_T *vop_mode_ptr)
{
#if _CMODEL_
	#include "mbc_global.h"
	mbc_module();
#endif //_CMODEL_

	//check the mbc done flag, or time out flag; if time out, error occur then reset the vsp	
// 	VSP_READ_REG_POLL(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, V_BIT_5, TIME_OUT_CLK, "MBC: polling mbc done");
	VSP_READ_REG_POLL_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF,5, 1, 1, "MBC: polling mbc done");
		
	VSP_WRITE_REG_CQM(VSP_MBC_REG_BASE+MBC_ST0_OFF, V_BIT_5, "clear MBC done flag");
	VSP_WRITE_CMD_INFO((VSP_MBC << 29) | (2<<24) | (MBC_ST0_WOFF<<8) | ((1<<7)|MBC_ST0_WOFF));

#if _CMODEL_
	#include "hdbk_global.h"
	dbk_module();
#endif	
}
/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
