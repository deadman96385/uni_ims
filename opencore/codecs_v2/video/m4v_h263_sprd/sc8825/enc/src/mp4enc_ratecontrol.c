/******************************************************************************
** File Name:    mp4enc_ratecontrol.c										  *
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

#define RC_PRINTF	//PRINTF

PUBLIC void Mp4Enc_InitRateCtrl(RateCtrlPara *rc_par_ptr, RCMode *rc_mode_ptr)
{
	VOL_MODE_T *pVol_mode = Mp4Enc_GetVolmode();
	ENC_VOP_MODE_T *pVop_mode = Mp4Enc_GetVopmode();
	
	rc_par_ptr->nbits_hdr_mv		= 0;
	rc_par_ptr->nbits_texture		= 0;
	rc_par_ptr->nbits_total			= 0;

	rc_par_ptr->nbit_hdr_mv_seq		= 0;
	rc_par_ptr->nbits_texture_seq	= 0;
	rc_par_ptr->nbits_total_seq		= 0;

	rc_par_ptr->skip_cnt			= 0;
	rc_par_ptr->total_bits			= 0;

	rc_par_ptr->rc_ena				= pVop_mode->RateCtrlEnable;
	rc_par_ptr->bit_rate			= pVop_mode->targetBitRate;
	rc_par_ptr->frame_rate			= pVop_mode->FrameRate;
	rc_par_ptr->qp_i				= pVop_mode->StepI;
	rc_par_ptr->qp_p				= pVop_mode->StepP;
	rc_par_ptr->p_between_i			= pVol_mode->PbetweenI;
	rc_par_ptr->p_count				= 0;

	rc_mode_ptr->FirstGOP			= TRUE; 

//	rc_mode_ptr->Ns					= enc_ctr_ptr->samp_rate;
	rc_mode_ptr->Rs					= rc_par_ptr->bit_rate; 
	rc_mode_ptr->Bs					= pVop_mode->vbv_buf_size;

	rc_mode_ptr->Nc 			= 0;
	
	/* average bits to be removed from the buffer */
	rc_mode_ptr->Rp					= rc_mode_ptr->Rs / (rc_par_ptr->frame_rate);
	rc_mode_ptr->BytesPerMb = rc_mode_ptr->Rp/(8*pVop_mode->FrameWidth*pVop_mode->FrameHeight/256);

	if(rc_mode_ptr->BytesPerMb <= 3)
	{
		rc_mode_ptr->delta_Qp_4Clip = 4;
		rc_mode_ptr->delta_Qp_4IncQc0 = 2;
		rc_mode_ptr->delta_Qp_4IncQc1 = 3;
	}
	else if(rc_mode_ptr->BytesPerMb <= 5)
	{
		rc_mode_ptr->delta_Qp_4Clip = 3;
		rc_mode_ptr->delta_Qp_4IncQc0 = 2;
		rc_mode_ptr->delta_Qp_4IncQc1 = 2;
	}
	else if(rc_mode_ptr->BytesPerMb <= 7)
	{
		rc_mode_ptr->delta_Qp_4Clip = 3;
		rc_mode_ptr->delta_Qp_4IncQc0 = 1;
		rc_mode_ptr->delta_Qp_4IncQc1 = 2;
	}
	else if(rc_mode_ptr->BytesPerMb <= 9)
	{
		rc_mode_ptr->delta_Qp_4Clip = 2;
		rc_mode_ptr->delta_Qp_4IncQc0 = 1;
		rc_mode_ptr->delta_Qp_4IncQc1 = 2;
	}
	else
	{
		rc_mode_ptr->delta_Qp_4Clip = 2;
		rc_mode_ptr->delta_Qp_4IncQc0 = 1;
		rc_mode_ptr->delta_Qp_4IncQc1 = 1;
	}
		

#if defined(SIM_IN_WIN)
	g_rgstat_fp		= fopen ("..\\seq\\rc_stat.txt", "w");
	g_buf_full_pf	= fopen ("..\\seq\\buf_fullness.txt", "w");
	g_psnr_pf		= fopen ("..\\seq\\psnr.txt", "w");
#endif
}


/************************************************************
condintion for coding I frame:
1. number of P frame between I frame are all coded;
2. scene change;
3. serious error occured in decoder;
************************************************************/
PUBLIC int32 Mp4Enc_JudgeFrameType(RateCtrlPara *rc_par_ptr, RCMode *rc_mode_ptr)
{
	int32 vop_type;
	int32 skip_margin;

// 	return IVOP;

	if ((0 == rc_par_ptr->p_count) || (TRUE == rc_mode_ptr->be_scene_cut))
	{
		rc_par_ptr->p_count = rc_par_ptr->p_between_i;
		vop_type = IVOP;
	}else
	{
		rc_par_ptr->p_count--;
		vop_type = PVOP;	
	}
#if 0
	if (g_enc_vop_mode_ptr->RateCtrlEnable)
	{
	/*skip test according to vop type*/
	skip_margin = (vop_type == IVOP) ? IVOP_SKIP_MARGIN : PVOP_SKIP_MARGIN;

	if (rc_mode_ptr->B * 100 > skip_margin * rc_mode_ptr->Bs)
	{
		if (vop_type == IVOP)
			rc_par_ptr->p_count = 0;

		rc_mode_ptr->skipNextFrame = TRUE;	

//		rc_mode_ptr->Nr--;

		/*decrease current buffer level*/
//		rc_mode_ptr->B -= rc_mode_ptr->Rp;

		vop_type = NVOP;
	}else
	{
		rc_mode_ptr->skipNextFrame = FALSE;
		}
	}
#endif

	return vop_type;
}

PUBLIC void Mp4Enc_InitRCFrame(RateCtrlPara	*rc_par_ptr)
{
	rc_par_ptr->nbits_hdr_mv	= 0;
	rc_par_ptr->nbits_texture	= 0;
	rc_par_ptr->nbits_total		= 0;

	rc_par_ptr->sad				= 0;
}

PUBLIC void Mp4Enc_InitRCGOP(RateCtrlPara *rc_par_ptr)
{
	rc_par_ptr->nbit_hdr_mv_seq		= 0;
	rc_par_ptr->nbits_texture_seq	= 0;
	rc_par_ptr->nbits_total_seq		= 0;
}


PUBLIC void Mp4Enc_ResetRCModel(ENC_VOP_MODE_T *pVop_mode, RCMode *rc_mode_ptr, RateCtrlPara *rc_par_ptr) 
{
//	uint32 i;
//	uint32 SumQP = 0;    
//	int32 frame_rate;
//	int32 p_between_i = rc_par_ptr->p_between_i;
	uint32 nbit_fst_frame = rc_par_ptr->nbits_total;

//	for (i = 0; i < RC_MAX_SLIDING_WINDOW; i++)
//	{
//		rc_mode_ptr->EcP_Q8[i] = 0;
//	}

//	rc_mode_ptr->Ts = (p_between_i + 1) / frame_rate;	

//	rc_mode_ptr->X1 = rc_mode_ptr->Rs * rc_mode_ptr->Ns / 2.0;
//	rc_mode_ptr->X2 = 0.0;	
//	rc_mode_ptr->Nr = p_between_i;

//	rc_mode_ptr->Nc_prev_gop = rc_mode_ptr->Nc;
//	rc_mode_ptr->Nc = 0;
//	rc_mode_ptr->Hp = 500; // guess of header bits
//	rc_mode_ptr->Hc = 500; // guess of header bits

	rc_mode_ptr->be_scene_cut = FALSE;

	rc_mode_ptr->Qp = rc_mode_ptr->Qc;
	
	rc_mode_ptr->Rf = nbit_fst_frame;
	rc_mode_ptr->Rc = nbit_fst_frame;
	rc_mode_ptr->S	= nbit_fst_frame;

	g_rc_par.nbits_total_seq += rc_mode_ptr->Rc;
	g_rc_par.total_bits		 += rc_mode_ptr->Rc;
	
	/* total number of bits available for this segment */
//	rc_mode_ptr->Rr = (uint32) (rc_mode_ptr->Ts * rc_mode_ptr->Rs) - rc_mode_ptr->Rf;	

//	rc_mode_ptr->B += nbit_fst_frame;	
	rc_mode_ptr->T = rc_mode_ptr->Rp;

	PRINTF ("\t QP: %d\n", pVop_mode->StepI);
//	PRINTF ("\t BitsTotalCurr=%d\n\t buffer fullness: %f\n", nbit_fst_frame, 1.0*rc_mode_ptr->B / rc_mode_ptr->Bs);

#if defined(SIM_IN_WIN)
	FPRINTF (g_rgstat_fp, "\t QP: %d\n", pVop_mode->StepI);
	FPRINTF (g_rgstat_fp, "\t buffer fullness: %f\n\t\ nBitsTotalCurr=%d\n", 
		1.0*rc_mode_ptr->B/rc_mode_ptr->Bs, nbit_fst_frame);
	FPRINTF (g_buf_full_pf, "%f\n", 1.0*rc_mode_ptr->B / rc_mode_ptr->Bs);
#endif
}

void RCModelEstimator (RCMode* rc_mode_ptr, uint32 n_windowSize)
{
	uint32 i;
	BOOLEAN estimateX2;
	int32 oneSampleQ = 0;
	int32 a00_Q0  = 0; //4
	int32 a01_Q26 = 0; //16 bit
	int32 a11_Q26 = 0;
	int32 inv_a00_Q10 = 0; //4
	int32 inv_a01_Q10 = 0;	//16 bit
	int32 inv_a11_Q10 = 0;
	int32 b0 = 0;
	int32 b1 = 0;
	int32 X1;
	int32 X2;
	int32 det_Q16;
	uint32 n_realSize = n_windowSize;

	static int32 n = 0;

	for (i = 0; i < n_windowSize; i++) 
	{
		// find the number of samples which are not rejected
		if (rc_mode_ptr->rgRejected[i])
		{
			n_realSize--;
		}
	}

	// default RD model estimation results
	estimateX2 = FALSE;
	
	rc_mode_ptr->X1 = rc_mode_ptr->X2 = 0;
	
	for (i = 0; i < n_windowSize; i++)
	{
		if (!rc_mode_ptr->rgRejected[i])
		{
			oneSampleQ = rc_mode_ptr->rgQp[i];
		}
	}

	for (i = 0; i < n_windowSize; i++)	
	{
		// if all non-rejected Q are the same, take 1st order model
		if ((rc_mode_ptr->rgQp[i] != oneSampleQ) && !rc_mode_ptr->rgRejected[i])
		{
			estimateX2 = TRUE;
		}

		if (!rc_mode_ptr->rgRejected[i])
		{
			rc_mode_ptr->X1 += (rc_mode_ptr->rgQp[i] * rc_mode_ptr->rgRp[i]) / (int32)n_realSize;
		}
	}

	// take 2nd order model to estimate X1 and X2
	if ((n_realSize >= 1) && estimateX2) 
	{
		for (i = 0; i < n_windowSize; i++)
		{
			if (!rc_mode_ptr->rgRejected[i]) 
			{
				a00_Q0  += 1;
				a01_Q26 += ((1<<26) / rc_mode_ptr->rgQp[i]);
				a11_Q26 += ((1<<26) / (rc_mode_ptr->rgQp[i] * rc_mode_ptr->rgQp[i]));
				b0		+= rc_mode_ptr->rgQp[i] * rc_mode_ptr->rgRp[i];
				b1		+= rc_mode_ptr->rgRp[i];
			}
		}

//		if (n == 524)
//			printf("hello\n");

		n++;

		// solve the equation of AX = B

		/*CMatrix2x2D AInv = A.inverse ();*/
		det_Q16 = ((a00_Q0 * a11_Q26) >> 10) - (((a01_Q26 >> 14) * (a01_Q26 >> 14)) >> 8);

		if (det_Q16 != 0)
		{
			inv_a00_Q10 = a11_Q26/det_Q16;
			inv_a11_Q10 = (a00_Q0 << 26)/det_Q16;
			inv_a01_Q10 = (-a01_Q26 / det_Q16);
		}
		/*~CMatrix2x2D AInv = A.inverse ();*/

		//solution = AInv.apply (B);
		X1 = (inv_a00_Q10 * b0 + inv_a01_Q10 * b1) >> 10;
		X2 = (inv_a01_Q10 * b0 + inv_a11_Q10 * b1) >> 10;

		if (X1 != 0) 
		{
			rc_mode_ptr->X1 = X1;
			rc_mode_ptr->X2 = X2;

			PRINTF("\tX1: %d, X2: %d\n", rc_mode_ptr->X1, rc_mode_ptr->X2);
		}	
	}
}

PUBLIC void Mp4Enc_UpdateRCModel(ENC_VOP_MODE_T *pVop_mode,	RCMode *rc_mode_ptr, RateCtrlPara *rc_par_ptr, int32 Ec_Q8)
{
	int32  i;
	uint32 n_windowSize;
//	double error[RC_MAX_SLIDING_WINDOW];
//	double std = 0.0;
//	double threshold;
	uint32 tot_bits_cur = rc_par_ptr->nbits_total; 
	uint32 bits_hdr_cur = rc_par_ptr->nbits_hdr_mv;

	rc_mode_ptr->Rc				= tot_bits_cur;								// total bits used for the current frame 
	rc_par_ptr->total_bits		+= rc_mode_ptr->Rc;
	rc_par_ptr->nbits_total_seq	+= rc_mode_ptr->Rf;
//	rc_mode_ptr->B				+= rc_mode_ptr->Rc - rc_mode_ptr->Rp;		// update buffer fullness
//	rc_mode_ptr->Rr				-= rc_mode_ptr->Rc;							// update the remaining bits

//	if (rc_mode_ptr->B < 0)
//	{
//		rc_mode_ptr->B = 0;
//	}

#if defined(SIM_IN_WIN)
	FPRINTF("\t BitsTotalCurr=%d\n\t complex: %f\n\t buffer fullness: %f\n", 
		tot_bits_cur, Ec_Q8*1.0/256, 1.0*rc_mode_ptr->B / rc_mode_ptr->Bs);	
	FPRINTF (g_rgstat_fp, "\t buffer fullness: %f\n\t\ complexity: %f\n\t nBitsTotalCurr=%d\n", 
		1.0*rc_mode_ptr->B/rc_mode_ptr->Bs, Ec_Q8*1.0/256, tot_bits_cur);
	FPRINTF (g_buf_full_pf, "%f\n", 1.0*rc_mode_ptr->B / rc_mode_ptr->Bs);
#endif

	rc_mode_ptr->S			= rc_mode_ptr->Rc;				// update the previous bits
	rc_mode_ptr->Hc			= bits_hdr_cur;					// update the current header and motion bits
	rc_mode_ptr->Hp			= rc_mode_ptr->Hc;				// update the previous header and motion bits
	rc_mode_ptr->Qp			= rc_mode_ptr->Qc;				// update the previous qunatization level
	rc_mode_ptr->Ep_Q8		= rc_mode_ptr->Ec_Q8;
	rc_mode_ptr->Ec_Q8		= Ec_Q8;

//	rc_mode_ptr->Nr--;										// update the frame counter
	rc_mode_ptr->Nc++;
	
   rc_mode_ptr->EcP_Q8[0]	= rc_mode_ptr->Ec_Q8;	
   for (i = RC_MAX_SLIDING_WINDOW - 1; i > 0; i--) 
	{
		rc_mode_ptr->rgQp[i] = rc_mode_ptr->rgQp[i - 1];
		rc_mode_ptr->rgRp[i] = rc_mode_ptr->rgRp[i - 1];
		rc_mode_ptr->EcP_Q8[i] = rc_mode_ptr->EcP_Q8[i - 1];
	}
	rc_mode_ptr->rgQp[0]	= rc_mode_ptr->Qc;
	rc_mode_ptr->rgRp[0]	= ((rc_mode_ptr->Rc - rc_mode_ptr->Hc) * 256) / rc_mode_ptr->Ec_Q8;
 	
	/*adjust window size*/
	n_windowSize = (rc_mode_ptr->Ep_Q8 > rc_mode_ptr->Ec_Q8) ? 
				   ((rc_mode_ptr->Ec_Q8 * RC_MAX_SLIDING_WINDOW) / rc_mode_ptr->Ep_Q8)	: 
				   ((rc_mode_ptr->Ep_Q8 * RC_MAX_SLIDING_WINDOW) / rc_mode_ptr->Ec_Q8);

	n_windowSize = IClip(1, (uint32)rc_mode_ptr->Nc, n_windowSize);
	
	for (i = 0; i < RC_MAX_SLIDING_WINDOW; i++) 
	{
		rc_mode_ptr->rgRejected[i] = FALSE;
	}

	// initial RD model estimator
	RCModelEstimator (rc_mode_ptr, n_windowSize);
}

int32 Qp_FirstPinGop(ENC_VOP_MODE_T *pVop_mode, RCMode *rc_mode_ptr)
{
	int32 Qp;
//	int32 Qp_sum = 0;

	if (rc_mode_ptr->be_re_enc)
	{
		switch(rc_mode_ptr->Qp_type)
		{
		case 0:
			Qp = rc_mode_ptr->Qc - 2;
			break;
		case 1:
			Qp = rc_mode_ptr->Qc - 1;
			break;
		case 2:
			Qp = rc_mode_ptr->Qc + 1;
			break;
		default:
			Qp = rc_mode_ptr->Qc + 2;
			break;
		}
	}else
	{
		Qp = pVop_mode->StepI;
	}

	Qp = mmax(Qp, (pVop_mode->StepI * (100 - RC_MAX_Q_INCREASE) + 99)/100);
	Qp = mmin(Qp, (pVop_mode->StepI * (100 + RC_MAX_Q_INCREASE) + 99)/100);

	return Qp;
}

//comput target bits number according to its complex
void GetTargetBits(RCMode *rc_mode_ptr, int32 Ec_Q8)
{
	int32 i;
	int32 Nc;
	int32 total_Ec_Q8 = 0;

	Nc = rc_mode_ptr->Nc;
	Nc = (Nc > (RC_MAX_SLIDING_WINDOW -1)) ? (RC_MAX_SLIDING_WINDOW -1) : Nc;

	for (i = 1; i <= Nc; i++)
	{
		total_Ec_Q8 += rc_mode_ptr->EcP_Q8[i];
	}
	rc_mode_ptr->T = rc_mode_ptr->Rp * Ec_Q8 * Nc / total_Ec_Q8;

	rc_mode_ptr->T = (rc_mode_ptr->Rp * (100 - COMPLEX_RATIO) + rc_mode_ptr->T*COMPLEX_RATIO)/100;

//	adjust according to buffer fullness, 1/3 buffer fullness
	rc_mode_ptr->T = (rc_mode_ptr->T * (rc_mode_ptr->Bs*7 - rc_mode_ptr->B*5))/(rc_mode_ptr->Bs*5 + rc_mode_ptr->B*5);

	if (((rc_mode_ptr->B + rc_mode_ptr->T)*100) > (90 * rc_mode_ptr->Bs))
	{
		// to avoid possible overflow
		rc_mode_ptr->T = mmax(rc_mode_ptr->Rs/30, (((rc_mode_ptr->Bs*90)/100) - rc_mode_ptr->B));
	}else if (((rc_mode_ptr->B - rc_mode_ptr->Rp + rc_mode_ptr->T)*100) < (RC_SAFETY_MARGIN * rc_mode_ptr->Bs))
	{
		// to avoid possible underflow
		rc_mode_ptr->T = rc_mode_ptr->Rp - rc_mode_ptr->B + (( (RC_SAFETY_MARGIN + 5)* rc_mode_ptr->Bs)/100);
	}

	// min bits number limitation for quality
	rc_mode_ptr->T = mmax(rc_mode_ptr->Rp/3 + rc_mode_ptr->Hp, rc_mode_ptr->T);

	PRINTF("\t target bit number: %d", rc_mode_ptr->T);
#if defined(SIM_IN_WIN)
	FPRINTF(g_rgstat_fp, "\t target bits number: %d\n", rc_mode_ptr->T);
#endif
}

uint32 sqrt_simple(uint32 d)
{
	uint32 N = 16;
	uint32 t, q= 0, r =d;

	do{
		N--;
		t = 2*q+(1<<N);
		if((r>>N) >= t)
		{
			r -= (t<<N);
			q += (1<<N);
		}
	}while(N);

	return q;
}

PUBLIC void Mp4Enc_UpdatePVOP_StepSize(ENC_VOP_MODE_T *pVop_mode, RCMode *rc_mode_ptr, RateCtrlPara *rc_par_ptr)
{
	int32 dtmp_QN4; //avoid dtmp overflow
//	int32 min_qp;
	int32 Ec_Q8;
	int32 Qc;

	//rc_mode_ptr->MADr = g_iMAD/(MB_SIZE*MB_SIZE);	

	if (rc_mode_ptr->Nc == 0)
	{
		pVop_mode->StepP = rc_mode_ptr->Qc = Qp_FirstPinGop(pVop_mode, rc_mode_ptr);
		
		PRINTF ("\t QP: %d\n", rc_mode_ptr->Qc);
	#if defined(SIM_IN_WIN)
		FPRINTF (g_rgstat_fp, "\t QP: %d\n", rc_mode_ptr->Qc);
	#endif

		return;
	}

	Ec_Q8 = rc_mode_ptr->be_re_enc ? rc_mode_ptr->Ec_actual_Q8 : rc_mode_ptr->EcP_Q8[1];

//  Target bit calculation 

//	according to complex to allocate target bits
	GetTargetBits (rc_mode_ptr, Ec_Q8);
	
	// Quantization level calculation
	dtmp_QN4 = ((Ec_Q8 * rc_mode_ptr->X1) >> 10) * ((Ec_Q8 * rc_mode_ptr->X1) >> 10) +
		((rc_mode_ptr->X2 >> 4) * (Ec_Q8 >> 4) * ((rc_mode_ptr->T - rc_mode_ptr->Hp) >>2));

	// to use last p frame's EC to predict current p frame's EC
	if ((rc_mode_ptr->X2 == 0) || (dtmp_QN4 <= 0)) //fall back 1st order mode
	{
		Qc = (((rc_mode_ptr->X1 * Ec_Q8)>>8)/(rc_mode_ptr->T - rc_mode_ptr->Hp));
	}else //2nd order mode
	{
		int32 tmp;
		uint32 utmp = ABS(dtmp_QN4);

		utmp = sqrt_simple(utmp);

		tmp = ssign(dtmp_QN4) * (int32)utmp;
		
	//	tmp = sqrt(dtmp_QN4) - (rc_mode_ptr->X1 * Ec_Q8 >> 10);
		tmp -= ((rc_mode_ptr->X1 * Ec_Q8) >> 10);

		if (tmp != 0)   //to avoid devided by 0
		{
		Qc = ((2 * rc_mode_ptr->X2 * Ec_Q8) >> 10)/tmp;
		}
		else
		{
			Qc = rc_mode_ptr->Qp;
		}
	
	}
	if (rc_mode_ptr->is_pfrm_skipped) //sync from 8810
	{
		Qc = mmax(Qc, rc_mode_ptr->Qp);  //if previous frame is skipped, qp for current frame should >= previous QP
	}

	SCI_TRACE_LOW("%d, rc_mode_ptr->Qc: %d, qp_type: %d, rc_mode_ptr->be_re_enc: %d", __LINE__, rc_mode_ptr->Qc, rc_mode_ptr->Qp_type, rc_mode_ptr->be_re_enc);

	if (rc_mode_ptr->be_re_enc)
	{
		switch(rc_mode_ptr->Qp_type)
		{
		case 0:
			Qc = mmin(Qc, rc_mode_ptr->Qc -2);
			break;
		case 1:
			Qc = mmin(Qc, rc_mode_ptr->Qc -1);
			break;
		case 2:
			Qc = mmax(Qc, rc_mode_ptr->Qc + 1);
			break;
		default:
			Qc = mmax(Qc, rc_mode_ptr->Qc + 2);
			break;
		}
	}
{
	Qc = mmin((rc_mode_ptr->Qp * (100 + RC_MAX_Q_INCREASE) + 50)/100, Qc); //control variation
	Qc = mmin(Qc, RC_MAX_QUANT); //clipping

	Qc = mmax((rc_mode_ptr->Qp * (100 - RC_MAX_Q_INCREASE) + 50)/100, Qc); //control variation
	Qc = mmax(Qc, RC_MIN_QUANT); //clipping

//	Qc = Clip3(rc_mode_ptr->Qp -3, rc_mode_ptr->Qp+rc_mode_ptr->delta_Qp_4Clip, Qc);
		
}

	if(rc_mode_ptr->need_to_skip < 0)
	{

	}
	else
	{
		//rc_mode_ptr->Qp = Qc;
		Qc = mmin(Qc + rc_mode_ptr->delta_Qp_4IncQc0,rc_mode_ptr->Qp+rc_mode_ptr->delta_Qp_4IncQc1);
		Qc =  Clip3(RC_MIN_QUANT, RC_MAX_QUANT, Qc );
	}

    Qc = Clip3(rc_mode_ptr->Qp -3, rc_mode_ptr->Qp+rc_mode_ptr->delta_Qp_4Clip, Qc);
    rc_mode_ptr->Qp = Qc;
	rc_mode_ptr->need_to_skip--;
	pVop_mode->StepP = rc_mode_ptr->Qc = Qc;

	PRINTF("\t QP: %d\n", rc_mode_ptr->Qc);
#if defined(SIM_IN_WIN)
	FPRINTF(g_rgstat_fp, "\t QP: %d\n", rc_mode_ptr->Qc);
#endif
}

PUBLIC void Mp4Enc_UpdateIVOP_StepSize(ENC_VOP_MODE_T *pVop_mode, RCMode * rc_mode_ptr)
{
	int32 Qp = 0;
	int32 total_bits_cur = rc_mode_ptr->Rf;
	int32 Stepsize_IVOP = pVop_mode->StepI;

	if (rc_mode_ptr->FirstGOP)
	{
		Qp = pVop_mode->StepI;
		rc_mode_ptr->FirstGOP = FALSE;			
		rc_mode_ptr->Qp = Qp;
	}else
	{
		if (rc_mode_ptr->be_scene_cut)
		{
			Qp = RC_MAX_QP_I;
		}else if (rc_mode_ptr->be_re_enc)
		{
			switch(rc_mode_ptr->Qp_type) {
			case 0:
				Qp = RC_MAX_QP_I;
				break;
			case 1:
				Qp = Stepsize_IVOP + 4;
				break;
			case 2:
				Qp = Stepsize_IVOP + 3;
				break;
			case 3:
				Qp = Stepsize_IVOP +2;
			case 4:
				Qp = Stepsize_IVOP -2;
				break;	
			default:
				break;
			}
		}else
		{
			if(total_bits_cur < (rc_mode_ptr->Bs>>2)) //0.25
			{
				Qp = Stepsize_IVOP - 2;
			}else if(total_bits_cur < ((rc_mode_ptr->Bs * 89) >> 8)) //0.35
			{
				Qp = Stepsize_IVOP - 1;
			}
			else if(total_bits_cur < (rc_mode_ptr->Bs >>1)) //0.35
			{
				Qp = Stepsize_IVOP ;
			}else if(total_bits_cur < ((rc_mode_ptr->Bs * 153) >> 8)) //0.6
			{
				Qp = Stepsize_IVOP + 1;
			}else
			{
				Qp = Stepsize_IVOP + 2;
			}

		}
		Qp = mmax(Qp, (rc_mode_ptr->Qp * (100 - RC_MAX_Q_INCREASE) + 50)/100);
		Qp = mmin(Qp, (rc_mode_ptr->Qp * (100 + RC_MAX_Q_INCREASE) + 50)/100);
	}

	Qp = mmax(Qp, RC_MIN_QP_I);
	Qp = mmin(Qp, RC_MAX_QP_I);
	rc_mode_ptr->need_to_skip--;
	pVop_mode->StepI = rc_mode_ptr->Qc = Qp;
}

PUBLIC void Mp4Enc_AnalyzeEncResult(RCMode *rc_mode_ptr, int32 total_bits_cur, int32 vop_type, int32 Ec_Q8)
{
	int32 target_bits;
	int32 be_scene_cut = FALSE;

	if (rc_mode_ptr->be_re_enc)
	{
		rc_mode_ptr->be_re_enc = 0;
		return;
	}

	if (Ec_Q8 > (SCENE_CUT_SAD * 256))
	{
		be_scene_cut = TRUE;
	}

	if (IVOP == vop_type)
	{
		if (total_bits_cur > ((230 * rc_mode_ptr->Bs)>>8)) //0.9
		{
			rc_mode_ptr->be_re_enc	= TRUE;
			rc_mode_ptr->Qp_type	= 0; //max qp for IVOP
		}else if (total_bits_cur > ((217 * rc_mode_ptr->Bs)>>8)) //0.85
		{
			rc_mode_ptr->be_re_enc	= TRUE;
			rc_mode_ptr->Qp_type	= 1; //QP increased by 4
		}else if (total_bits_cur > ((205 * rc_mode_ptr->Bs)>>8)) //0.8
		{
			rc_mode_ptr->be_re_enc	= 2;
			rc_mode_ptr->Qp_type	= 2; //Qp increased by 2
		}else if (total_bits_cur > ((192 * rc_mode_ptr->Bs)>>8)) //0.75
		{
			rc_mode_ptr->be_re_enc	= 1;
			rc_mode_ptr->Qp_type	= 3; //Qp increased by 2
		}else if (total_bits_cur < rc_mode_ptr->Bs/3) //0.75
		{
			rc_mode_ptr->be_re_enc	= 1;
			rc_mode_ptr->Qp_type	= 4; //Qp increased by 2
		}
		

		if (rc_mode_ptr->be_re_enc)
		{
			//rc_mode_ptr->be_re_enc = (rc_mode_ptr->Qc == RC_MAX_QP_I) ? FALSE : TRUE;
			if ((((rc_mode_ptr->Qc + 1)*100) <= (rc_mode_ptr->Qp * (RC_MAX_Q_I_INCREASE + 100))) &&
				(rc_mode_ptr->Qc < RC_MAX_QP_I))
			{
				rc_mode_ptr->be_re_enc = 1;
			}
			else
			{
				rc_mode_ptr->be_re_enc = 0;
			}
		}
	}else if (be_scene_cut)
	{
		//it can be looked as scene change, I frame should be inserted
//		PRINTF("\t complexity: %f\n", Ec_Q8*1.0/256);
	#if defined(SIM_IN_WIN)
		FPRINTF(g_rgstat_fp, "\t complexity: %f\n", Ec_Q8*1.0/256);
	#endif
		rc_mode_ptr->be_scene_cut = TRUE;

		if ((rc_mode_ptr->B * 100) > (IVOP_SKIP_MARGIN * rc_mode_ptr->Bs))
		{
//			rc_mode_ptr->be_skip_frame = TRUE;
			rc_mode_ptr->need_to_skip = (38-rc_mode_ptr->Qp)>>2;
		}else
		{
			rc_mode_ptr->be_re_enc = TRUE;
		}
	}else // normal p frame
	{
		target_bits = rc_mode_ptr->T;
		rc_mode_ptr->Ec_actual_Q8 = Ec_Q8;

		if (total_bits_cur < (target_bits*5/8))
		{
			//use the new Ec_Q8, and re-encode the frame, or use the actual Ec to compute the Qp
			if (rc_mode_ptr->B < (rc_mode_ptr->Bs/3))
			{
				if ((((rc_mode_ptr->Qc -1) * 100) >= (rc_mode_ptr->Qp * (100 - RC_MAX_Q_INCREASE))) && (rc_mode_ptr->Qc > RC_MIN_QUANT))
				{
					rc_mode_ptr->be_re_enc = TRUE;
					rc_mode_ptr->Qp_type = (total_bits_cur < (target_bits/3)) ? 0 : 1;
				}
			}
		}else if (total_bits_cur < (target_bits*11/8))
		{
			//nothing to do
		}else
		{
			if (((rc_mode_ptr->B + total_bits_cur) > rc_mode_ptr->Bs) && (rc_mode_ptr->B > rc_mode_ptr->Bs/2))
			{
				//buffer fullness is high, skip the frame
//				rc_mode_ptr->be_skip_frame = TRUE;
				rc_mode_ptr->need_to_skip = (38-rc_mode_ptr->Qp)>>2;
			}else
			{
				if (rc_mode_ptr->B > rc_mode_ptr->Bs/4)
				{
					if ((((rc_mode_ptr->Qc + 1)*100) <= (rc_mode_ptr->Qp * (RC_MAX_Q_INCREASE + 100))) && 
						(rc_mode_ptr->Qc < RC_MAX_QUANT))
					{
						//increase the Qp and re-encode the frame, or use the actual Ec to compute the Qp
						rc_mode_ptr->be_re_enc = TRUE;

						rc_mode_ptr->Qp_type = (total_bits_cur > (target_bits * 5/2)) ? 3 : 2;
					}
				}
			}
		}
	}

//SCI_TRACE_LOW("%s, %d, rc_mode_ptr->be_re_enc: %d", __FUNCTION__, __LINE__, rc_mode_ptr->be_re_enc);

	if (rc_mode_ptr->be_re_enc || rc_mode_ptr->be_skip_frame)
	{
		PRINTF("\t bitTotalCur: %d\n", total_bits_cur);
	#if defined(SIM_IN_WIN)
		FPRINTF(g_rgstat_fp, "\t bitTotalCur: %d\n", total_bits_cur);
	#endif

	#if defined(SIM_IN_WIN)
		if (rc_mode_ptr->be_re_enc)
		{
			FPRINTF(g_rgstat_fp, "\t frame re-encoded\n");
		}else
		{
			FPRINTF(g_rgstat_fp, "\t frame is dropped\n");
		}
	#endif
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
