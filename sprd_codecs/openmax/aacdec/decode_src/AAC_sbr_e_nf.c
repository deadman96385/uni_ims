/*************************************************************************
** File Name:      sbr_e_nf.c                                            *
** Author:         Reed zhang                                            *
** Date:           10/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize SBR dequatization algorithm                   *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 10/01/2006     Reed zhang       Create.                               *
** 12/01/2006     reed zhang       modify the dequantization algorithm   * 
**                                 according to the standard ISO/IEC     *
**       						   14496-3:2001 and rewrite the          *
**								   fixed-point code                      *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"

#ifdef AAC_SBR_DEC

#include "AAC_sbr_syntax.h"
#include "AAC_sbr_e_nf.h"
#define E31_TOP 2147483648


/************************************************************************/
/*                   the dequantisation table for SBR                   */
/************************************************************************/
/* create table according to the following function:
         1.0 / (1.0 + pow(2.0, x - 12)                                  */
void AAC_ExtractEnvelopeData(AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    uint8 l, k;

    for (l = 0; l < sbr_ptr->L_E[ch]; l++)
    {
        if (sbr_ptr->bs_df_env[ch][l] == 0)
        {
            for (k = 1; k < sbr_ptr->n[sbr_ptr->f[ch][l]]; k++)
            {
                sbr_ptr->E[ch][l][k] = (int16) (sbr_ptr->E[ch][l][k - 1] + sbr_ptr->E[ch][l][k]);
            }

        } else { /* bs_df_env == 1 */

            uint8 g =  (uint8) ( (l == 0) ? sbr_ptr->f_prev[ch] : sbr_ptr->f[ch][l-1]);
            int16 E_prev;

            if (sbr_ptr->f[ch][l] == g)
            {
                for (k = 0; k < sbr_ptr->n[sbr_ptr->f[ch][l]]; k++)
                {
                    if (l == 0)
                        E_prev = sbr_ptr->E_prev[ch][k];
                    else
                        E_prev = sbr_ptr->E[ch][l - 1][k];

                    sbr_ptr->E[ch][l][k] = (int16) (E_prev + sbr_ptr->E[ch][l][k]);
                }

            } else if ((g == 1) && (sbr_ptr->f[ch][l] == 0)) {
                uint8 i;

                for (k = 0; k < sbr_ptr->n[sbr_ptr->f[ch][l]]; k++)
                {
                    for (i = 0; i < sbr_ptr->N_high; i++)
                    {
                        if (sbr_ptr->f_table_res[AAC_HI_RES][i] == sbr_ptr->f_table_res[AAC_LO_RES][k])
                        {
                            if (l == 0)
                                E_prev = sbr_ptr->E_prev[ch][i];
                            else
                                E_prev = sbr_ptr->E[ch][l - 1][i];

                            sbr_ptr->E[ch][l][k] =  (int16) (E_prev + sbr_ptr->E[ch][l][k]);
                        }
                    }
                }

            } else if ((g == 0) && (sbr_ptr->f[ch][l] == 1)) {
                uint8 i;

                for (k = 0; k < sbr_ptr->n[sbr_ptr->f[ch][l]]; k++)
                {
                    for (i = 0; i < sbr_ptr->N_low; i++)
                    {
                        if ((sbr_ptr->f_table_res[AAC_LO_RES][i] <= sbr_ptr->f_table_res[AAC_HI_RES][k]) &&
                            (sbr_ptr->f_table_res[AAC_HI_RES][k] < sbr_ptr->f_table_res[AAC_LO_RES][i + 1]))
                        {
                            if (l == 0)
                                E_prev = sbr_ptr->E_prev[ch][i];
                            else
                                E_prev = sbr_ptr->E[ch][l - 1][i];

                            sbr_ptr->E[ch][l][k] =  (int16) (E_prev + sbr_ptr->E[ch][l][k]);
                        }
                    }
                }
            }
        }
    }
}

void AAC_ExtractNoiseFloorData(AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    uint8 l, k;

    for (l = 0; l < sbr_ptr->L_Q[ch]; l++)
    {
        if (sbr_ptr->bs_df_noise[ch][l] == 0)
        {
            for (k = 1; k < sbr_ptr->N_Q; k++)
            {
                sbr_ptr->Q[ch][l][k] =  (int16) (sbr_ptr->Q[ch][l][k] + sbr_ptr->Q[ch][l][k-1]);
            }
        } else {
            if (l == 0)
            {
                for (k = 0; k < sbr_ptr->N_Q; k++)
                {
                    sbr_ptr->Q[ch][l][k] =  (int16) (sbr_ptr->Q_prev[ch][k] + sbr_ptr->Q[ch][0][k]);
                }
            } else {
                for (k = 0; k < sbr_ptr->N_Q; k++)
                {
                    sbr_ptr->Q[ch][l][k] =  (int16) (sbr_ptr->Q[ch][l - 1][k] + sbr_ptr->Q[ch][l][k]);
                }
            }
        }
    }
}
#endif
