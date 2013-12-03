/*************************************************************************
** File Name:      ps_decorrelate.h                                      *
** Author:         Reed zhang                                            *
** Date:           21/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file is to do PS mix phase processing            *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 21/04/2006     Reed zhang       Create.                               *
** 26/04/2006     Reed zhang       finish the baseline version MixPhase  *
**************************************************************************/
#include "AAC_ps_mix_phase.h"
#include "AAC_ps_tables.h"
#include "AAC_sbr_common.h"
extern const int16 AAC_ps_sf_iid_fine[31];
extern const int16 AAC_ps_cos_betas_fine[16][8];
extern const int16 AAC_ps_sin_betas_fine[16][8];
extern const int16 AAC_ps_sf_iid_normal[15];
extern const int16 AAC_ps_cos_betas_normal[8][8];
extern const int16 AAC_ps_sin_betas_normal[8][8];
extern const int16 scaleFactorsFine[31] ;
extern const int16 ps_mix_phase_tab[16][32];
extern const int16 AAC_ps_sincos_alphas_B_fine[31][8];
extern const int32 AAC_ps_cos_alphas[8];
extern const int32 AAC_ps_sin_alphas[8];



void AAC_ClacPsMixPhaseEnvelope(AAC_PS_INFO_T *ps_ptr, 
                                                           AAC_PS_MIX_PHASETMP_VAR_T *ps_mix_phase_t_ptr,
                                                           int16  band_No,
                                                           int16  env_start)
{    
    int16 gr;
    int16 bk = 0;
    int32 L;
    const int16 *sf_iid = NULL;
    int16 no_iid_steps;	
    int16 *pScaleFactors = NULL;
    int16 *pcos_sin = NULL;
    //////////////////////////////////////////////////////////////////////////
    int16 *cosbg_ptr = NULL, *sinbg_ptr = NULL, *sincos_ptr = NULL;
    int16 sincos_size;
    L = (int32)(ps_ptr->border_position[env_start + 1] - ps_ptr->border_position[env_start]);
    if (ps_ptr->icc_mode < 3)
    {
            if (ps_ptr->iid_mode >= 3)
            {
                no_iid_steps = 15;
                //pScaleFactors = scaleFactorsFine;
                //pcos_sin     = ps_mix_phase_tab;
                sf_iid = AAC_ps_sf_iid_fine;
                cosbg_ptr =  (int16 *)AAC_ps_cos_betas_fine;
                sinbg_ptr =  (int16 *)AAC_ps_sin_betas_fine;						
           } else 
	    {
                 no_iid_steps = 7;
                 //pScaleFactors = scaleFactors;
                sf_iid = AAC_ps_sf_iid_normal;
                cosbg_ptr =  (int16 *)AAC_ps_cos_betas_normal;
                sinbg_ptr =  (int16 *)AAC_ps_sin_betas_normal;						
           }
    }
    else
    {
            no_iid_steps = 15;
            pScaleFactors = (int16 *) scaleFactorsFine;
            pcos_sin     = (int16 *) ps_mix_phase_tab;
            sf_iid = AAC_ps_sf_iid_fine;
            sincos_ptr = (int16 *)AAC_ps_sincos_alphas_B_fine;
            sincos_size = 30;				
      }
    //////////////////////////////////////////////////////////////////////////        
    for (gr = 0; gr < ps_ptr->num_groups; gr++)
    {
        bk = (int16) (ps_ptr->map_group2bk[gr]);   // (~NEGATE_IPD_MASK) & ps_ptr->map_group2bk_ptr[gr];		

        /* use one channel per group in the subqmf domain */
        //for (tmp_env = 0; tmp_env < ps_ptr->num_env; tmp_env++)
        {
            if (ps_ptr->icc_mode < 3)
            {
                    /* type 'A' mixing as described in 8.6.4.6.2.1 */
                    int32 c_1 = 0, c_2 = 0;
                    int32 cosa = 0, sina = 0;
                    int32 cosbg = 0, sinbg = 0;
                    int32 ab1 = 0, ab2 = 0;

                    int32 ab3 = 0, ab4 = 0;
                    int16 iid_index = ps_ptr->iid_index[env_start][bk];
                    int16 icc_index = ps_ptr->icc_index[env_start][bk];
                    /* calculate the scalefactors c_1 and c_2 from the intensity differences */				
                    c_1 = sf_iid[no_iid_steps + iid_index];
                    c_2 = sf_iid[no_iid_steps - iid_index];
                    /* calculate alpha and beta using the ICC parameters */
                    cosa = AAC_ps_cos_alphas[icc_index];
                    sina = AAC_ps_sin_alphas[icc_index];
                    if (iid_index < 0)
                            iid_index = (int16) (- iid_index);
                    cosbg = cosbg_ptr[iid_index* 8 +icc_index];
                    sinbg = sinbg_ptr[iid_index* 8 +icc_index];
                    if (ps_ptr->iid_index[env_start][bk] < 0)
                    {							
                            sinbg = -sinbg;//AAC_ps_sin_betas_fine[iid_index][icc_index];
                    }
                    ab1 = AAC_DEC_MultiplyShiftR32(cosbg << 16, cosa) << 1;
                    ab2 = AAC_DEC_MultiplyShiftR32(sinbg << 16, sina) << 1;
                    ab3 = AAC_DEC_MultiplyShiftR32(sinbg << 16, cosa) << 1;
                    ab4 = AAC_DEC_MultiplyShiftR32(cosbg << 16, sina) << 1;
                    /* h_xy: COEF */
                    ps_mix_phase_t_ptr->opt_h11[gr] = AAC_DEC_MultiplyShiftR32(((c_2 + 0) << 16), (ab1 - ab2)) << 3;
                    ps_mix_phase_t_ptr->opt_h12[gr] = AAC_DEC_MultiplyShiftR32(((c_1 + 0) << 16), (ab1 + ab2)) << 3;
                    ps_mix_phase_t_ptr->opt_h21[gr] = AAC_DEC_MultiplyShiftR32(((c_2 + 0) << 16), (ab3 + ab4)) << 3;
                    ps_mix_phase_t_ptr->opt_h22[gr] = AAC_DEC_MultiplyShiftR32(((c_1 + 0) << 16), (ab3 - ab4)) << 3;
               }else
               {
               /* type 'B' mixing as described in 8.6.4.6.2.2 */
                     int32 cos512_v0, sin512_v0,cos512_v1, sin512_v1, scaleL, scaleR;
                     int16 icc_id = (int16) (ps_ptr->icc_index[env_start][bk] * 4);
                     int16 idd_id = (int16) (abs(ps_ptr->iid_index[env_start][bk]));
                     scaleR = pScaleFactors[no_iid_steps+ps_ptr->iid_index[env_start][bk]];
                     scaleL = pScaleFactors[no_iid_steps-ps_ptr->iid_index[env_start][bk]]; 
                     if (ps_ptr->iid_index[env_start][bk] > 0)
                     {
                             //scaleR = pScaleFactors[no_iid_steps+idd_id];
                             //scaleL = pScaleFactors[no_iid_steps-idd_id]; 
                             cos512_v0 = pcos_sin[idd_id * 32 + icc_id + 0];
                             cos512_v1 = pcos_sin[idd_id * 32 + icc_id + 1];
                             sin512_v0 = pcos_sin[idd_id * 32 + icc_id + 2];
                             sin512_v1 = pcos_sin[idd_id * 32 + icc_id + 3];
                       } else 
                       {
                       //scaleL = pScaleFactors[no_iid_steps+idd_id];
                       //scaleR = pScaleFactors[no_iid_steps-idd_id]; 
                       cos512_v1 = pcos_sin[idd_id * 32 + icc_id + 0];
                       cos512_v0 = pcos_sin[idd_id * 32 + icc_id + 1];
                       sin512_v1 = (int16) (-pcos_sin[idd_id * 32 + icc_id + 2]);
                       sin512_v0 = (int16) (-pcos_sin[idd_id * 32 + icc_id + 3]);
                }
                ps_mix_phase_t_ptr->opt_h11[gr] = (cos512_v0  * scaleL) >> 1;
                ps_mix_phase_t_ptr->opt_h12[gr] = (cos512_v1  * scaleR) >> 1;
                ps_mix_phase_t_ptr->opt_h21[gr] = (sin512_v0  * scaleL) >> 1;
                ps_mix_phase_t_ptr->opt_h22[gr] = (sin512_v1  * scaleR) >> 1;
           }
           //////////////////////////////////////////////////////////////////////////
           /* obtain final H_xy by means of linear interpolation */
           // 1
           ps_mix_phase_t_ptr->deltaH11[gr]    = (int32) ((ps_mix_phase_t_ptr->opt_h11[gr] - ps_ptr->h11_prev[gr]) / L);
           ps_mix_phase_t_ptr->H11[gr]         = (ps_ptr->h11_prev[gr]);
           ps_ptr->h11_prev[gr] = ps_mix_phase_t_ptr->opt_h11[gr];
           // 2
           ps_mix_phase_t_ptr->deltaH12[gr]    = (int32) ((ps_mix_phase_t_ptr->opt_h12[gr] - ps_ptr->h12_prev[gr]) / L);
           ps_mix_phase_t_ptr->H12[gr]         = ps_ptr->h12_prev[gr];
           ps_ptr->h12_prev[gr] = ps_mix_phase_t_ptr->opt_h12[gr];
           // 3
           ps_mix_phase_t_ptr->deltaH21[gr]    = (int32) ((ps_mix_phase_t_ptr->opt_h21[gr] - ps_ptr->h21_prev[gr]) / L);
           ps_mix_phase_t_ptr->H21[gr]         = ps_ptr->h21_prev[gr];
           ps_ptr->h21_prev[gr] = ps_mix_phase_t_ptr->opt_h21[gr];
           // 4
           ps_mix_phase_t_ptr->deltaH22[gr]    = (int32) ((ps_mix_phase_t_ptr->opt_h22[gr] - ps_ptr->h22_prev[gr]) / L);				
           ps_mix_phase_t_ptr->H22[gr]         = ps_ptr->h22_prev[gr];
           ps_ptr->h22_prev[gr] = ps_mix_phase_t_ptr->opt_h22[gr];			
      }
    }
}