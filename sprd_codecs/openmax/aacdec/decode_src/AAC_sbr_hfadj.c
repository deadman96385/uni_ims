/*************************************************************************
** File Name:      sbr_hfadj.c                                           *
** Author:         Reed zhang                                            *
** Date:           19/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    High Frequency adjustment                             *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 19/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "AAC_sbr_common.h"
#include "AAC_sbr_hfadj.h"
typedef struct
{
    int32 G_lim_boost[MAX_L_E][MAX_M];
    int32 Q_M_lim_boost[MAX_L_E][MAX_M];
    int32 S_M_boost[MAX_L_E][MAX_M];
    int32 E_enrg[MAX_L_E][MAX_M];
} SBR_HFADJ_INFO_T;
typedef struct
{
    int16 h_SL;
	int16 GQ_ringbuf_index;
	int32 *aac_sbr_h_smooth_ptr;
	int32 *G_temp_prev_ptr;
	int32 *Q_temp_prev_ptr;
	int32 *S_M_boost_ptr;
	int16  fIndexNoise;
	int16  fIndexSine;
	int16  no_noise;
	int16  rev;
	int16 *V_tab_ptr;
	int16  *phi_re_im_ptr;
} SBR_HFADJ_DATA_T;
#ifndef WIN32
extern void AAC_SBRAdj_CalcInterpolFreq1Enrg(int32 *XBuf_ptr, int32 *tE_curr_ptr, int32 M_t, int16 i_div);
extern void AAC_SBRAdj_CalcInterpolFreq0Enrg(int32 *XBuf_ptr, int32 *tE_curr_ptr, int32 M, int16 i_div);

#endif

extern const int16 pan_log2_tab[];
extern const int32 log_Qplus1_pan[6][13];
extern const int32 log_Qplus1[31];
extern const int32 log2_int_tab[];

extern const uint16 INV_tab[];
extern const int16 SBR_phi_re_im[4][2];
//extern const int8 phi_im[];
extern const int32 limGain[]; 
extern  const int16 pow2_tab[];
extern const int16 log2_tab[];
static void AAC_EstimateCurrentEnvelope(AAC_SBR_INFO_T       *sbr_ptr, 
									aac_complex          Xsbr[MAX_NTSRHFG][64],
									uint8 ch);
static void AAC_CalculateGain(AAC_SBR_INFO_T       *sbr_ptr, 									
							  uint8        ch,
							  int32      *G_lim_boost_ptr);
static void AAC_HfAssemblyFilter( AAC_SBR_INFO_T      *sbr_ptr, 
                                                       aac_complex          Xsbr[MAX_NTSRHFG][64], // fixed-point: S22.3  
                                                       uint8        ch,
                                                       int32       *G_lim_boost_ptr);
                                                       
static uint8 AAC_GetSMapped(AAC_SBR_INFO_T *sbr_ptr, uint8 ch, uint8 l, uint8 current_band)
{
    if (sbr_ptr->f[ch][l] == AAC_HI_RES)
    {
        if ((l >= sbr_ptr->l_A[ch]) ||
            (sbr_ptr->bs_add_harmonic_prev[ch][current_band] && sbr_ptr->bs_add_harmonic_flag_prev[ch]))
        {
            return sbr_ptr->bs_add_harmonic[ch][current_band];
        }
    } else {
        uint8 b, lb, ub;		
        /* in case of f_table_low we check if any of the HI_RES bands
		* within this LO_RES band has bs_add_harmonic[l][k] turned on
		* (note that borders in the LO_RES table are also present in
		* the HI_RES table)	*/		
        /* find first HI_RES band in current LO_RES band */
        lb = (uint8)( 2*current_band - ((sbr_ptr->N_high & 1) ? 1 : 0));
        /* find first HI_RES band in next LO_RES band */
        ub = (uint8)(2*(current_band+1) - ((sbr_ptr->N_high & 1) ? 1 : 0));		
        /* check all HI_RES bands in current LO_RES band for sinusoid */
        for (b = lb; b < ub; b++)
        {
            if ((l >= sbr_ptr->l_A[ch]) ||
                (sbr_ptr->bs_add_harmonic_prev[ch][b] && sbr_ptr->bs_add_harmonic_flag_prev[ch]))
            {
                if (sbr_ptr->bs_add_harmonic[ch][b] == 1)
                    return 1;
            }
        }
    }	
    return 0;
}

static int32 AAC_FindLog2E(AAC_SBR_INFO_T *sbr_ptr, uint8 k, uint8 l, uint8 ch)
{
    /* check for coupled energy/noise data */
    if (sbr_ptr->bs_coupling == 1)
    {
        uint8 amp0 = (uint8) ((sbr_ptr->amp_res[0]) ? 0 : 1);
        uint8 amp1 = (uint8) ((sbr_ptr->amp_res[1]) ? 0 : 1);
        int32 tmp = (114688) + ((int32)sbr_ptr->E[0][l][k] << (REAL_BITS-amp0));//(7 << REAL_BITS) + (sbr_ptr->E[0][l][k] << (REAL_BITS-amp0));
        int32 pan;
		
        /* E[1] should always be even so shifting is OK */
        uint8 E = (uint8)(sbr_ptr->E[1][l][k] >> amp1);
		
        if (ch == 0)
        {
            if (E > 12)
            {
                /* negative */
                pan = pan_log2_tab[-12 + E];
            } else {
                /* positive */
                pan = (int32)pan_log2_tab[12 - E] + ((int32)(12 - E)<<REAL_BITS);
            }
        } else {
            if (E < 12)
            {
                /* negative */
                pan = pan_log2_tab[-E + 12];
            } else {
                /* positive */
                pan = pan_log2_tab[E - 12] + ((int32)(E - 12)<<REAL_BITS);
            }
        }
		
        /* tmp / pan in log2 */
        return tmp - pan;
    } else {
        uint8 amp = (uint8) ((sbr_ptr->amp_res[ch]) ? 0 : 1);
        return 98304+ ((int32)sbr_ptr->E[ch][l][k] << (REAL_BITS-amp));
    }
}
extern  const int16 log_Qplus1_pan1[17][13];





static int32 AAC_FindLog2Qplus1(AAC_SBR_INFO_T *sbr_ptr, uint8 k, uint8 l, uint8 ch)
{
    /* check for coupled energy/noise data */
    if (sbr_ptr->bs_coupling == 1)
    {
        if ((sbr_ptr->Q[0][l][k] >= 0) && (sbr_ptr->Q[0][l][k] <= 5/*30*/) &&
            (sbr_ptr->Q[1][l][k] >= 0) && (sbr_ptr->Q[1][l][k] <= 24))
        {
            if (ch == 0)
            {
                return log_Qplus1_pan[sbr_ptr->Q[0][l][k]][sbr_ptr->Q[1][l][k] >> 1];
            } else {
                return log_Qplus1_pan[sbr_ptr->Q[0][l][k]][12 - (sbr_ptr->Q[1][l][k] >> 1)];
            }
        } 
        else if ((sbr_ptr->Q[0][l][k] > 5) && (sbr_ptr->Q[0][l][k] <= 16+6/*30*/) &&
            (sbr_ptr->Q[1][l][k] >= 0) && (sbr_ptr->Q[1][l][k] <= 24))
        {
            if (ch == 0)
            {
                return log_Qplus1_pan1[sbr_ptr->Q[0][l][k]-6][sbr_ptr->Q[1][l][k] >> 1];
        } else {
                return log_Qplus1_pan1[sbr_ptr->Q[0][l][k]-6][12 - (sbr_ptr->Q[1][l][k] >> 1)];
            }
        } 
        else 
        {
            return 0;
        }
    } else {
        if (sbr_ptr->Q[ch][l][k] >= 0 && sbr_ptr->Q[ch][l][k] <= 30)
        {
            return log_Qplus1[sbr_ptr->Q[ch][l][k]];
        } else {
            return 0;
        }
    }
}

static int32 AAC_FindLog2Q(AAC_SBR_INFO_T *sbr_ptr, uint8 k, uint8 l, uint8 ch)
{
    /* check for coupled energy/noise data */
    if (sbr_ptr->bs_coupling == 1)
    {
        int32 tmp = (114688) - ((int32)sbr_ptr->Q[0][l][k] << REAL_BITS);
        int32 pan;
		
        int16 Q = sbr_ptr->Q[1][l][k];
		
        if (ch == 0)
        {
            if (Q > 12)
            {
                /* negative */
                pan = pan_log2_tab[-12 + Q];
            } else {
                /* positive */
                pan = pan_log2_tab[12 - Q] + ((int32)(12 - Q)<<REAL_BITS);
            }
        } else {
            if (Q < 12)
            {
                /* negative */
                pan = pan_log2_tab[-Q + 12];
            } else {
                /* positive */
                pan = pan_log2_tab[Q - 12] + ((int32)(Q - 12)<<REAL_BITS);
            }
        }
		
        /* tmp / pan in log2 */
        return tmp - pan;
    } else {
        return (98304) - ((int32)sbr_ptr->Q[ch][l][k] << REAL_BITS);
    }
}


/************************************************************************/
/* the function HfAdjustment description is shown below:                */
/* sbr_ptr : the relative information for HF adjustment                     */
/* Xsbr: the high frequency data from HF generation model,              */
/*       fixed-point: S28.3                                             */
/* deg : the degree of aliasing. fixed-point: S0.15                     */
/* ch  : the current channel                                            */
/************************************************************************/
//#define PRINT_TEST_DATA
void AAC_HfAdjustment(AAC_SBR_INFO_T *sbr_ptr, 
					  aac_complex     Xsbr[MAX_NTSRHFG][64],      // fixed-point: S21.0 
					  uint8   ch,
					  int32  *tmp_buf_ptr)
{
    if (sbr_ptr->bs_frame_class[ch] == AAC_FIXFIX)
    {
        sbr_ptr->l_A[ch] = -1;
    } else if (sbr_ptr->bs_frame_class[ch] == AAC_VARFIX) 
    {
        if (sbr_ptr->bs_pointer[ch] > 1)
            sbr_ptr->l_A[ch] = -1;
         else
            sbr_ptr->l_A[ch] = (uint8) (sbr_ptr->bs_pointer[ch] - 1);
     } else 
     {
            if (sbr_ptr->bs_pointer[ch] == 0)
                 sbr_ptr->l_A[ch] = -1;
            else
        sbr_ptr->l_A[ch] = (uint8) (sbr_ptr->L_E[ch] + 1 - sbr_ptr->bs_pointer[ch]);
    }	
    //AAC_DEC_MEMSET(tmp_buf_ptr, 0, 1024 * sizeof(int32));
    sbr_ptr->E_curr = (uint32 (*)[64]) (tmp_buf_ptr + 735);
    AAC_DEC_MEMSET((tmp_buf_ptr), 0, (320  + 735)* sizeof(int32));
    /* calculate the parameter E_curr1, fixed-point: S60.3 */
    AAC_EstimateCurrentEnvelope(  sbr_ptr,
                                                        Xsbr,
                                                        ch);	
#if 0//def GEN_TEST_DATA
	//if (0 == ch)
    {
        FILE *fp = fopen("Energe.txt", "wb");
        int i, j;
        SBR_HFADJ_INFO_T *sbr_adj_ptr = (SBR_HFADJ_INFO_T *) (tmp_buf_ptr);
        for (i = 0; i < sbr_ptr->L_E[ch]; i++)
        {
            for (j = 0; j < 49; j++)
            {
                fprintf(fp, "%5d,\n", sbr_ptr->E_curr[i][j]);
            }
        }
        fclose(fp);
    }
#endif
    /* calculate the gain parameter and the fixed-point define is shown below, */
    /* G_lim_boost:   Q50.9 Q_M_lim_boost: Q11.7 iS_M_boost:     Q11.7 */
    //AAC_MEMSET(tmp_buf_ptr, 0, 192 * 10 * sizeof(int32));
    AAC_CalculateGain( sbr_ptr, 				   
                                     ch,
                                     tmp_buf_ptr);
#if 0//def GEN_TEST_DATA
    if (g_frm_counter > TEST_FRAME)
    {
        FILE *fp = fopen("paramemter.txt", "wb");
        int i, j;
        SBR_HFADJ_INFO_T *sbr_adj_ptr = (SBR_HFADJ_INFO_T *) (tmp_buf_ptr);

        for (i = 0; i < sbr_ptr->L_E[ch]; i++)
        {
            for (j = 0; j < sbr_ptr->M; j++)
            {
                uint32 t0, t1, t2;
                t0 = sbr_adj_ptr->G_lim_boost[i][j];
                t1 = sbr_adj_ptr->Q_M_lim_boost[i][j];
                t2 = sbr_adj_ptr->S_M_boost[i][j];
                fprintf(fp, "energe:%5d,  G_lim_boost: %5d, Q_M_lim_boost: %5d, S_M_boost: %5d, \n", sbr_ptr->E_curr[i][j], t0, t1, t2);
            }
        }
        fclose(fp);
    }    
#endif	
    AAC_HfAssemblyFilter( sbr_ptr, 
    Xsbr,// fixed-point: S22.3  
    ch,
    tmp_buf_ptr);

}
/* calculate the parameter E_curr1, fixed-point: S60.3 */
static void AAC_EstimateCurrentEnvelope(AAC_SBR_INFO_T       *sbr_ptr,
                                                                  aac_complex          Xsbr[MAX_NTSRHFG][64], // high frequent data, fixed-point: S32.0  
                                                                  uint8        ch)
{
    uint8 l;
    int32 i_div;
    uint8 L_E    = sbr_ptr->L_E[ch];
    uint8 kx     = sbr_ptr->kx;
    if (sbr_ptr->bs_interpol_freq == 1)
    {		
        for (l = 0; l < L_E; l++)
        {
            uint8 l_i, u_i, t;
            uint8 M         = sbr_ptr->M;
            uint32 *tE_curr     = sbr_ptr->E_curr[l];			
            l_i   = (uint8) (sbr_ptr->t_E[ch][l]   + AAC_T_HFADJ);
            u_i   = (uint8) (sbr_ptr->t_E[ch][l+1] + AAC_T_HFADJ);
            i_div = INV_tab[u_i - l_i - 1];  // Q 0. 12
            t     = (uint8) (u_i - l_i);
            /* ARM9 asm code implementation */
            AAC_SBRAdj_CalcInterpolFreq1Enrg(Xsbr[l_i][kx], (int32 *)tE_curr, (((int32)M << 16) | t), (int16) i_div);
        }

    } else 
    {
		
        for (l = 0; l < L_E; l++)
        {
            uint16 k_l, k_h;
            uint8 sbr_n = sbr_ptr->n[sbr_ptr->f[ch][l]], p;
            for (p = 0; p < sbr_n; p++)
            {
                uint8 l_i, u_i;
                k_l = sbr_ptr->f_table_res[sbr_ptr->f[ch][l]][p];
                k_h = sbr_ptr->f_table_res[sbr_ptr->f[ch][l]][p+1];
                l_i = sbr_ptr->t_E[ch][l];
                u_i = sbr_ptr->t_E[ch][l+1];

                i_div = ((int32)u_i - l_i)* ((int32)k_h - k_l) - 1;
                i_div = INV_tab[i_div];
                AAC_SBRAdj_CalcInterpolFreq0Enrg(Xsbr[l_i + AAC_T_HFADJ][k_l], (int32 *) (sbr_ptr->E_curr[l] + k_l - kx), ((((int32)u_i-l_i) << 16) | ((int32)k_h-k_l)), (int16)i_div);
            }
        }
    }
}

/* 
calculating the parameters which is shown below: 
// G_lim_boost:    square value,      fixed-point: Q22.5
// G_M_lim_boost:  square root value, fixed-point: Q11.7
// iS_M_boost:     square root value, fixed-point: Q11.0
*/
#define TMP_BIT 4
#define G_BIT_T 12
static void AAC_CalculateGain( AAC_SBR_INFO_T         *sbr_ptr, 						   
						       uint8           ch,
							   int32         *G_lim_boost_ptr1)
{			
    int32      E64[64];
    uint8 m, l, k;	
    uint8 current_t_noise_band = 0;
    uint8 S_mapped;
    int32  Q_orig, Q_orig_plus1;
    ALIGN int32 G_boost;
    uint8 bs_limiter_bands = sbr_ptr->bs_limiter_bands;
    SBR_HFADJ_INFO_T *sbr_adj_ptr = (SBR_HFADJ_INFO_T *) (G_lim_boost_ptr1);	

    for (l = 0; l < sbr_ptr->L_E[ch]; l++)
    {
        uint8 sbr_f                 = sbr_ptr->f[ch][l];
        uint32 *E_curr_ptr          = sbr_ptr->E_curr[l];
        uint8  current_f_noise_band = 0;
        uint8  current_res_band     = 0;
        uint8  current_res_band2    = 0;
        uint8  current_hi_res_band  = 0;		
        int8   delta = (int8)( (l == sbr_ptr->l_A[ch] || l == sbr_ptr->prevEnvIsShort[ch]) ? 0 : 1);
        S_mapped = AAC_GetSMapped(sbr_ptr, ch, l, current_res_band2);		
        if (sbr_ptr->t_E[ch][l+1] > sbr_ptr->t_Q[ch][current_t_noise_band+1])
        {
            current_t_noise_band++;
        }
        for (k = 0; k < sbr_ptr->N_L[bs_limiter_bands]; k++)
        {
            int32  Q_M = 0;
            int32  G_max, E_orig;
            int32  acc1 = 0;
            int32  acc2 = 0;
            uint8 current_res_band_size = 0;
            uint8 Q_M_size = 0;
            uint8 ml1, ml2;
            /* bounds of current limiter bands */
            ml1 = sbr_ptr->f_table_lim[bs_limiter_bands][k];
            ml2 = sbr_ptr->f_table_lim[bs_limiter_bands][k+1];
            /* calculate the accumulated E_orig and E_curr over the limiter band */
            for (m = ml1; m < ml2; m++)
            {
                if ((m + sbr_ptr->kx) < sbr_ptr->f_table_res[sbr_f][current_res_band+1])
                {
                    current_res_band_size++;
                } else 
            {
                int32 t0;
                // the out put t0 bit: Q18.14
                t0                    = log2_int_tab[current_res_band_size];
                E64[current_res_band] = AAC_FindLog2E(sbr_ptr, current_res_band, l, ch) - 163840;
                t0                    = t0 + E64[current_res_band];
                // the out put acc1 bit: Q18.0
                if (t0 >= 0)
                {
                      acc1 += AAC_DEC_ARM_Pow2IntAsm(t0, (int16*)pow2_tab);
                 }
                 current_res_band++;
                 current_res_band_size = 1;
            }
                acc2 += (int32 ) E_curr_ptr[m];
      }
      E64[current_res_band] = AAC_FindLog2E(sbr_ptr, current_res_band, l, ch)-163840;
      Q_M   =  log2_int_tab[current_res_band_size] + E64[current_res_band];
      acc1 +=	 AAC_DEC_ARM_Pow2IntAsm(Q_M, (int16*)pow2_tab);
      /* the out put acc1 bit: Q18.14 and adopting the log calculation to avoiding the data which access 32 bit*/
      if (acc1 == 0)
           acc1 = -163840000;
      else
          acc1 = AAC_DEC_ARM_Log2IntAsm(acc1, (int16*)log2_tab);
      /* calculate the maximum gain,  ratio of the energy of the original
      signal and the energy, of the HF generated signal 
      the G_max fixed-point is: Q18.14 */
      G_max = acc1 - AAC_DEC_ARM_Log2IntAsm(acc2, (int16*)log2_tab) + limGain[sbr_ptr->bs_limiter_gains];
      G_max = AAC_DEC_MIN(G_max, 544260);
      acc2 = 0;
      E_orig       = E64[current_res_band2];
      Q_orig       = AAC_FindLog2Q(sbr_ptr, current_f_noise_band, current_t_noise_band, ch);
      Q_orig_plus1 = AAC_FindLog2Qplus1(sbr_ptr, current_f_noise_band, current_t_noise_band, ch);
      for (m = ml1; m < ml2; m++)
      {
             int32  G;
             int32  tE_curr;
            uint8 S_index_mapped;
             /* check if m is on a noise band border */
             if ((m + sbr_ptr->kx) == sbr_ptr->f_table_noise[current_f_noise_band+1])
            {
                   /* step to next noise band */
                   current_f_noise_band++;
                   Q_orig       =  AAC_FindLog2Q(sbr_ptr, current_f_noise_band, current_t_noise_band, ch);
                   Q_orig_plus1 =  AAC_FindLog2Qplus1(sbr_ptr, current_f_noise_band, current_t_noise_band, ch);
            }
            /* check if m is on a HI_RES band border */
            if ((m + sbr_ptr->kx) == sbr_ptr->f_table_res[AAC_HI_RES][current_hi_res_band+1])
           {
                  /* step to next HI_RES band */
                  current_hi_res_band++;
           }
           /* check if m is on a resolution band border */
           if ((m + sbr_ptr->kx) == sbr_ptr->f_table_res[sbr_f][current_res_band2+1])
           {
                  /* accumulate a whole range of equal Q_Ms */
                  if (Q_M_size > 0)
                        acc2 += AAC_DEC_ARM_Pow2IntAsm(log2_int_tab[Q_M_size] + Q_M, (int16*)pow2_tab);
                  Q_M_size = 0;
                   /* step to next resolution band */
                   current_res_band2++;
                    /* if we move to a new resolution band, we should check if we are
                     * going to add a sinusoid in this band */
					/* E_orig: Q18.14 */
                    S_mapped = AAC_GetSMapped(sbr_ptr, ch, l, current_res_band2);
					E_orig   = E64[current_res_band2];
                }                
                /* find S_index_mapped
                 * S_index_mapped can only be 1 for the m in the middle of the
                 * current HI_RES band    */
                S_index_mapped = 0;
                if ((l >= sbr_ptr->l_A[ch]) ||
                    (sbr_ptr->bs_add_harmonic_prev[ch][current_hi_res_band] && sbr_ptr->bs_add_harmonic_flag_prev[ch]))
                {
                    /* find the middle subband of the HI_RES frequency band */
                    if ((m + sbr_ptr->kx) == (sbr_ptr->f_table_res[AAC_HI_RES][current_hi_res_band+1] + 
						sbr_ptr->f_table_res[AAC_HI_RES][current_hi_res_band]) >> 1)
					{
						S_index_mapped = sbr_ptr->bs_add_harmonic[ch][current_hi_res_band];
					}                        
                }
                /* find bitstream parameters, E_curr: Q18.14 */
                if (E_curr_ptr[m])
                    tE_curr = AAC_DEC_ARM_Log2IntAsm(E_curr_ptr[m], (int16*)log2_tab);
                else
                    tE_curr = -163840000;	
                
                /* Q_M only depends on E_orig and Q_div2:
                 * since N_Q <= N_Low <= N_High we only need to recalculate Q_M on
                 * a change of current res band (HI or LO) */
				/* Q_M Q18.14 */
                Q_M = E_orig + Q_orig - Q_orig_plus1;                
                /* calculate gain */
                /* ratio of the energy of the original signal and the energy
                 * of the HF generated signal */
                /* E_curr here is officially E_curr+1 so the log2() of that can never be < 0 */
                /* scaled by -10 */
                G = E_orig - AAC_DEC_MAX(-163840, tE_curr);
                if ((S_mapped == 0) && (delta == 1))
                {
                    /* G = G * 1/(1+Q) */
                    G -= Q_orig_plus1;
                } else if (S_mapped == 1) {
                    /* G = G * Q/(1+Q) */
                    G += Q_orig - Q_orig_plus1;
                }
                /* limit the additional noise energy level */
                /* and apply the limiter */
                if (G_max > G)
                {
					sbr_adj_ptr->G_lim_boost[l][m]   = G;
                    sbr_adj_ptr->Q_M_lim_boost[l][m] = Q_M; 
					/* accumulate the total energy */
					/* E_curr changes for every m so we do need to accumulate every m */
					acc2 += AAC_DEC_ARM_Pow2IntAsm(tE_curr + G, (int16*)pow2_tab);
                    if ((S_index_mapped == 0) && (l != sbr_ptr->l_A[ch]))
                    {
                        Q_M_size++;
                    }
					/* S_M only depends on E_orig, Q_div and S_index_mapped:
					* S_index_mapped can only be non-zero once per HI_RES band*/
					if (S_index_mapped == 0)
					{
						sbr_adj_ptr->S_M_boost[l][m] = -163840000; /* -inf */
					} else {
						sbr_adj_ptr->S_M_boost[l][m] = E_orig - Q_orig_plus1;
						/* accumulate sinusoid part of the total energy */
						acc2 += AAC_DEC_ARM_Pow2IntAsm(sbr_adj_ptr->S_M_boost[l][m], (int16*)pow2_tab);
					}
                } else 
				{
                    /* G > G_max */
					sbr_adj_ptr->G_lim_boost[l][m]   = G_max;
                    sbr_adj_ptr->Q_M_lim_boost[l][m] = Q_M + G_max - G;  
					/* accumulate the total energy */
					/* E_curr changes for every m so we do need to accumulate every m */
					acc2 += AAC_DEC_ARM_Pow2IntAsm(tE_curr + G_max, (int16*)pow2_tab);
                    /* accumulate limited Q_M */
                    if ((S_index_mapped == 0) && (l != sbr_ptr->l_A[ch]))
                    {
                        acc2 += AAC_DEC_ARM_Pow2IntAsm(sbr_adj_ptr->Q_M_lim_boost[l][m], (int16*)pow2_tab);
                    }
					/* S_M only depends on E_orig, Q_div and S_index_mapped:
					* S_index_mapped can only be non-zero once per HI_RES band*/
					if (S_index_mapped == 0)
					{
						sbr_adj_ptr->S_M_boost[l][m] = -163840000; /* -inf */
					} else {
						sbr_adj_ptr->S_M_boost[l][m] = E_orig - Q_orig_plus1;
						/* accumulate sinusoid part of the total energy */
						acc2 += AAC_DEC_ARM_Pow2IntAsm(sbr_adj_ptr->S_M_boost[l][m], (int16*)pow2_tab);
					}
                }    //////////////////////////////////////////////////////////////////////////
            }
			/* accumulate last range of equal Q_Ms */
            if (Q_M_size > 0)
            {
                acc2 += AAC_DEC_ARM_Pow2IntAsm(log2_int_tab[Q_M_size] + Q_M, (int16*)pow2_tab);
            }
            /* calculate the final gain */
            /* G_boost: [0..2.51188643] */
            G_boost = acc1 - AAC_DEC_ARM_Log2IntAsm(acc2, (int16*)log2_tab);
            G_boost = AAC_DEC_MIN(G_boost, 21771);
             for (m = ml1; m < ml2; m++)
            {
             	int32 tmp;
                /* apply compensation to gain, noise floor sf's and sinusoid levels */                
               sbr_adj_ptr->G_lim_boost[l][m]   = AAC_DEC_ARM_Pow2IntOut14bitAsm(((sbr_adj_ptr->G_lim_boost[l][m] + G_boost) >> 1) + 16384 * 2, (int16*)pow2_tab);    // S16.16 + 81920
               sbr_adj_ptr->Q_M_lim_boost[l][m] = AAC_DEC_ARM_Pow2IntOut14bitAsm(((sbr_adj_ptr->Q_M_lim_boost[l][m] + G_boost) >> 1), (int16*)pow2_tab);  // S18.14 + 114688
               tmp = sbr_adj_ptr->S_M_boost[l][m] + G_boost;
                if (tmp < 0 )
                {
                    sbr_adj_ptr->S_M_boost[l][m] = 0;
                } else {
                    sbr_adj_ptr->S_M_boost[l][m] = AAC_DEC_ARM_Pow2IntAsm((tmp) >> 1, (int16*) pow2_tab);  // S32.0;
                }
            }
        }
    }

}
/************************************************************************/
// calculate the variaty Xsbr and the fixed-point: Q28.3
/************************************************************************/
int32 AAC_SBR_h_smooth[11] = 
{
      0x55550826,   0x1D7B37DA,           // idx 0:   f4,  f0, f1, f2, f3
      0x4D2F5555,  0x08261D7B,            // idx 1:   f3,  f4, f0, f1, f2
      0x37DA4D2F, 0x55550826,             // idx 2:   f2,  f3,  f4, f0, f1
      0x1D7B37DA, 0x4D2F5555,            // idx 3:   f1,  f2,  f3,  f4, f0  
	0x08261D7B,  0x37DA4D2F,   0x55550000        // idx 4:   f0,   f1,  f2,  f3,  f4
};
extern int32 AAC_DecHfAssemblyFilterAsm(int32                                idx_noise_sine_ch_l,                               // [31:16]: noise,   [15:12]: sine,      [11:8]: ch,    [7:4]:  l,    [3:0]:  noise
                                                                      int32                               *XBuf_ptr, 
                                                                      int32                               *G_lim_boost_ptr,
                                                                      AAC_SBR_INFO_T     *sbr_ptr);
extern int32 AAC_DecHfAssemblyFilter1Asm(int32                                idx_noise_sine_ch_l,                             // [31:16]: noise,   [15:12]: sine,      [11:8]: ch,    [7:4]:  l,    [3:0]:  noise
                                                                        int32                               *XBuf_ptr, 
                                                                        int32                               *G_lim_boost_ptr,
                                                                        AAC_SBR_INFO_T     *sbr_ptr);
int32 AAC_DecHfAssemblyFilter_h_SL4(int32                       idx_noise_sine_ch_l,
                                                                 aac_complex          Xsbr[MAX_NTSRHFG][64], // fixed-point: S22.3  
                                                                 SBR_HFADJ_INFO_T *sbr_adj_ptr,
                                                                 AAC_SBR_INFO_T      *sbr_ptr                                                                
                                                                 );
static void AAC_HfAssemblyFilter( AAC_SBR_INFO_T      *sbr_ptr, 
                                                       aac_complex          Xsbr[MAX_NTSRHFG][64], // fixed-point: S22.3  
                                                       uint8        ch,
                                                       int32       *G_lim_boost_ptr)
{
    uint8  l;    
    int32   idx_noise_sine_ch_l = 0;
    SBR_HFADJ_INFO_T *sbr_adj_ptr = (SBR_HFADJ_INFO_T *) (G_lim_boost_ptr);	
    uint8 h_SL = 0;
    uint8 assembly_reset = 0;
    if (sbr_ptr->Reset == 1)
    {
        assembly_reset = 1;
        idx_noise_sine_ch_l = 0;
    } else 
    {
        idx_noise_sine_ch_l = (int32)sbr_ptr->index_noise_prev[ch] << 16;
    }
    idx_noise_sine_ch_l |= ((int32)sbr_ptr->psi_is_prev[ch] << 12);    
  
    for (l = 0; l < sbr_ptr->L_E[ch]; l++)   
    {
        uint8 no_noise =  (uint8) ((l == sbr_ptr->l_A[ch] || l == sbr_ptr->prevEnvIsShort[ch]) ? 1 : 0);
        h_SL =  (uint8) ((sbr_ptr->bs_smoothing_mode == 1) ? 0 : 4);
        h_SL =  (uint8) ((no_noise ? 0 : h_SL));
#ifdef GEN_TEST_DATA
        h_SL = 4;
#endif
        if (assembly_reset)
        {
              int8 n;
              for (n = 0; n < 4; n++)
              {
                    AAC_DEC_MEMCPY(sbr_ptr->G_temp_prev[ch][n], sbr_adj_ptr->G_lim_boost[l], sbr_ptr->M*sizeof(int32));
                    AAC_DEC_MEMCPY(sbr_ptr->Q_temp_prev[ch][n], sbr_adj_ptr->Q_M_lim_boost[l], sbr_ptr->M*sizeof(int32));
              }
              sbr_ptr->GQ_ringbuf_index[ch] = 4;
              assembly_reset = 0;
          }          
          idx_noise_sine_ch_l = idx_noise_sine_ch_l | ((int32)ch << 8) |  ((int32)l << 4) | (no_noise);  
          if (0 == h_SL)
          {
               idx_noise_sine_ch_l = AAC_DecHfAssemblyFilterAsm(idx_noise_sine_ch_l, 
                                                                  (int32  *)Xsbr,
                                                                  (int32  *)sbr_adj_ptr->G_lim_boost,
                                                                  sbr_ptr);
          }else
          {
               idx_noise_sine_ch_l = AAC_DecHfAssemblyFilter_h_SL4(idx_noise_sine_ch_l, 
                                                                  Xsbr,
                                                                  sbr_adj_ptr,
                                                                  sbr_ptr
                                                                        );
          }
              
          }
       sbr_ptr->index_noise_prev[ch] = (uint16)(idx_noise_sine_ch_l >> 16);
       sbr_ptr->psi_is_prev[ch]          = (uint8) ((idx_noise_sine_ch_l >> 12) & 0xF);
          
       }
extern int32  SBR_ADJ_Noise_tab[512];
extern const int32 SBR_DEC_phi_re_im[4];
extern int32 AAC_DecHfAssemblySmoothFilterAsm(int32    *G_temp_prev_ptr,
                                                                                  int32     *ptr,
                                                                                  int32      ri,
                                                                                  int32     *Xsbr_ptr);
int32 AAC_DecHfAssemblyFilter_h_SL4(int32                       idx_noise_sine_ch_l,
                                                                 aac_complex          Xsbr[MAX_NTSRHFG][64], // fixed-point: S22.3  
                                                                 SBR_HFADJ_INFO_T *sbr_adj_ptr,
                                                                 AAC_SBR_INFO_T      *sbr_ptr                                                                
                                                                 )
{
    int32  i = 0,  m = 0;
    int32 G_filt = 0, Q_filt = 0, S_M_boost = 0;    
    int32 no_noise = 0,  fIndexNoise = 0, fIndexSine = 0, l = 0, ch = 0;
    int32 GQ_ringbuf_index;
    no_noise     = idx_noise_sine_ch_l & 0xF;
    l                  = (idx_noise_sine_ch_l >> 4) & 0xF;
    ch               = (idx_noise_sine_ch_l >> 8) & 0xF;
    fIndexSine   = (idx_noise_sine_ch_l >> 12) & 0xF;
    fIndexNoise = idx_noise_sine_ch_l >> 16;
    GQ_ringbuf_index = sbr_ptr->GQ_ringbuf_index[ch];
    for (i = sbr_ptr->t_E[ch][l]; i < sbr_ptr->t_E[ch][l+1]; i++)
    {
        int32    data[3];
        int32    rev = (((sbr_ptr->kx) & 1) ? 1 : -1);
        int32  *Xsbr_ptr        = Xsbr[i + 2][sbr_ptr->kx];
        AAC_DEC_MEMCPY(sbr_ptr->G_temp_prev[ch][GQ_ringbuf_index], sbr_adj_ptr->G_lim_boost[l], sbr_ptr->M*sizeof(int32));
        AAC_DEC_MEMCPY(sbr_ptr->Q_temp_prev[ch][GQ_ringbuf_index], sbr_adj_ptr->Q_M_lim_boost[l], sbr_ptr->M*sizeof(int32));				
        //////////////////////////////////////////////////////////////////////////		
        data[0] = SBR_DEC_phi_re_im[fIndexSine];
        for (m = 0; m < sbr_ptr->M; m++)
        {
           data[1]  = sbr_adj_ptr->S_M_boost[l][m];                
           /* add noise to the output */
           fIndexNoise = (fIndexNoise + 1) & 511;	
           rev = -rev; 
           data[2] = SBR_ADJ_Noise_tab[fIndexNoise];
           AAC_DecHfAssemblySmoothFilterAsm(sbr_ptr->G_temp_prev[ch][0]+m,  
                                                                          data,  
                                                                          ((GQ_ringbuf_index<<16) | no_noise)*rev,
                                                                          Xsbr_ptr);
           Xsbr_ptr += 2;
        }
        //////////////////////////////////////////////////////////////////////////			
        fIndexSine = (fIndexSine + 1) & 3;      
        /* update the ringbuffer index used for filtering G and Q with h_smooth */
        GQ_ringbuf_index++;
        if (GQ_ringbuf_index >= 5)
            GQ_ringbuf_index = 0;
    }
    idx_noise_sine_ch_l = (fIndexNoise << 16) | (fIndexSine << 12);
   sbr_ptr->GQ_ringbuf_index[ch] = GQ_ringbuf_index;
    return idx_noise_sine_ch_l;
}

