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
#include "sc8825_video_header.h"
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
		code = Mp4Dec_Show32Bits(bitstrm_ptr);
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
		}

		if(QP >= (intra_dc_vlc_thr * 2 + 11))
		{
			is_switched = TRUE;
		}
	}
	
	return (BOOLEAN)is_switched;
}

PUBLIC void Mp4Dec_GetNeighborMBPred(DEC_VOP_MODE_T *vop_mode_ptr, DEC_MB_MODE_T *mb_mode_ptr)
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



/**---------------------------------------------------------------------------*
**                         Compiler Flag                                      *
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
    }
#endif
/**---------------------------------------------------------------------------*/
// End 
