/*************************************************************************
** File Name:      sbr_huff.c                                            *
** Author:         Reed zhang                                            *
** Date:           10/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file is to do sbr_ptr envelope and noise huffman     *
**                 decoding                                              *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 10/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#ifdef AAC_SBR_DEC
#include "AAC_sbr_syntax.h"
#include "aac_bits.h"
#include "AAC_sbr_huff.h"
#include "AAC_sbr_e_nf.h"
extern const int8 t_huffman_env_1_5dB[120][2];
extern const int8 f_huffman_env_1_5dB[120][2];
extern const int8 t_huffman_env_bal_1_5dB[48][2];
extern const int8 f_huffman_env_bal_1_5dB[48][2];
extern const int8 t_huffman_env_3_0dB[62][2];
extern const int8 f_huffman_env_3_0dB[62][2];
extern const int8 t_huffman_env_bal_3_0dB[24][2];
extern const int8 f_huffman_env_bal_3_0dB[24][2];
extern const int8 t_huffman_noise_3_0dB[62][2];
extern const int8 t_huffman_noise_bal_3_0dB[24][2];


typedef const int8 (*sbr_huff_tab)[2];




static  int16 AAC_SbrHuffDec(AAC_BIT_FIFO_FORMAT_T *ld_ptr, sbr_huff_tab t_huff)
{
    uint8 bit;
    int16 index = 0;

    while (index >= 0)
    {
        bit = (uint8)AAC_GetBits(ld_ptr, 1);
        index = t_huff[index][bit];
    }

    return (int16) (index + 64);
}

/* table 10 */
void AAC_SbrEnvelope(AAC_BIT_FIFO_FORMAT_T *ld_ptr,
				  AAC_SBR_INFO_T *sbr_ptr,
				  uint8 ch)
{
    uint8      env, band;
    int8       delta = 0;
    sbr_huff_tab t_huff, f_huff;

    if ((sbr_ptr->L_E[ch] == 1) && (sbr_ptr->bs_frame_class[ch] == AAC_FIXFIX))
        sbr_ptr->amp_res[ch] = 0;
    else
        sbr_ptr->amp_res[ch] = sbr_ptr->bs_amp_res;

    if ((sbr_ptr->bs_coupling) && (ch == 1))
    {
        delta = 1;
        if (sbr_ptr->amp_res[ch])
        {
            t_huff = t_huffman_env_bal_3_0dB;
            f_huff = f_huffman_env_bal_3_0dB;
        } else {
            t_huff = t_huffman_env_bal_1_5dB;
            f_huff = f_huffman_env_bal_1_5dB;
        }
    } else {
        delta = 0;
        if (sbr_ptr->amp_res[ch])
        {
            t_huff = t_huffman_env_3_0dB;
            f_huff = f_huffman_env_3_0dB;
        } else {
            t_huff = t_huffman_env_1_5dB;
            f_huff = f_huffman_env_1_5dB;
        }
    }

    for (env = 0; env < sbr_ptr->L_E[ch]; env++)
    {
		uint8 sbr_n;
        if (sbr_ptr->bs_df_env[ch][env] == 0)
        {
            if ((sbr_ptr->bs_coupling == 1) && (ch == 1))
            {
                if (sbr_ptr->amp_res[ch])
                {
                    sbr_ptr->E[ch][env][0] = (int16) ((int32)(AAC_GetBits(ld_ptr, 5) << delta));
                } else {
                    sbr_ptr->E[ch][env][0] = (int16)(AAC_GetBits(ld_ptr, 6) << delta);
                }
            } else {
                if (sbr_ptr->amp_res[ch])
                {
                    sbr_ptr->E[ch][env][0] = (int16)(AAC_GetBits(ld_ptr, 6) << delta);
                } else {
                    sbr_ptr->E[ch][env][0] = (int16)(AAC_GetBits(ld_ptr, 7) << delta);
                }
            }

            sbr_n = sbr_ptr->n[sbr_ptr->f[ch][env]];			
            for (band = 1; band < sbr_n; band++)
            {
                sbr_ptr->E[ch][env][band] =  (int16)(AAC_SbrHuffDec(ld_ptr, f_huff) << delta);
            }

        } else 
		{
			sbr_n = sbr_ptr->n[sbr_ptr->f[ch][env]];
            for (band = 0; band < sbr_n; band++)
            {
                sbr_ptr->E[ch][env][band] = (int16)(AAC_SbrHuffDec(ld_ptr, t_huff) << delta);
            }
        }
    }

    AAC_ExtractEnvelopeData(sbr_ptr,
						  ch);
}

/* table 11 */
void AAC_SbrNoise(AAC_BIT_FIFO_FORMAT_T *ld_ptr, AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    uint8 noise, band;
    int8 delta = 0;
    sbr_huff_tab t_huff, f_huff;

    if ((sbr_ptr->bs_coupling == 1) && (ch == 1))
    {
        delta = 1;
        t_huff = t_huffman_noise_bal_3_0dB;
        f_huff = f_huffman_env_bal_3_0dB;
    } else {
        delta = 0;
        t_huff = t_huffman_noise_3_0dB;
        f_huff = f_huffman_env_3_0dB;
    }

    for (noise = 0; noise < sbr_ptr->L_Q[ch]; noise++)
    {
        if(sbr_ptr->bs_df_noise[ch][noise] == 0)
        {
            if ((sbr_ptr->bs_coupling == 1) && (ch == 1))
            {
                sbr_ptr->Q[ch][noise][0] = (int16)(AAC_GetBits(ld_ptr, 5) << delta);
            } else {
                sbr_ptr->Q[ch][noise][0] = (int16)(AAC_GetBits(ld_ptr, 5) << delta);
            }
            for (band = 1; band < sbr_ptr->N_Q; band++)
            {
                sbr_ptr->Q[ch][noise][band] = (int16)(AAC_SbrHuffDec(ld_ptr, f_huff) << delta);
            }
        } else {
            for (band = 0; band < sbr_ptr->N_Q; band++)
            {
                sbr_ptr->Q[ch][noise][band] = (int16)(AAC_SbrHuffDec(ld_ptr, t_huff) << delta);
            }
        }
    }

    AAC_ExtractNoiseFloorData(sbr_ptr, ch);
}

#endif
