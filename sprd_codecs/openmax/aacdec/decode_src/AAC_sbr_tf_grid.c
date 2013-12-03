/*************************************************************************
** File Name:      sbr_tf_grid.c                                         *
** Author:         Reed zhang                                            *
** Date:           09/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    Time/Frequency grid
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#ifdef AAC_HE_AAC_EXTENSION
#include "AAC_sbr_syntax.h"
#include "AAC_sbr_tf_grid.h"

/* static function declarations */
static uint8 AAC_MiddleBorder(AAC_SBR_INFO_T *sbr_ptr, uint8 ch);
uint8 AAC_EnvelopeTimeBorderVector(AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    uint8 l, border, temp;

    for (l = 0; l <= sbr_ptr->L_E[ch]; l++)
    {
        sbr_ptr->t_E[ch][l] = 0;
    }
    sbr_ptr->t_E[ch][0] = (uint8) (sbr_ptr->rate * sbr_ptr->abs_bord_lead[ch]);
    sbr_ptr->t_E[ch][sbr_ptr->L_E[ch]] = (uint8) (sbr_ptr->rate * sbr_ptr->abs_bord_trail[ch]);
    switch (sbr_ptr->bs_frame_class[ch])
    {
    case AAC_FIXFIX:
        switch (sbr_ptr->L_E[ch])
        {
        case 4:
            temp = (uint8) ((sbr_ptr->numTimeSlots / 4));
            sbr_ptr->t_E[ch][3] = (uint8) (sbr_ptr->rate * 3 * temp);
            sbr_ptr->t_E[ch][2] = (uint8) (sbr_ptr->rate * 2 * temp);
            sbr_ptr->t_E[ch][1] = (uint8) (sbr_ptr->rate * temp);
            break;
        case 2:
            sbr_ptr->t_E[ch][1] = (uint8) (sbr_ptr->rate *  (sbr_ptr->numTimeSlots / 2));
            break;
        default:
            break;
        }
        break;

    case AAC_FIXVAR:
        if (sbr_ptr->L_E[ch] > 1)
        {
            int8 i = sbr_ptr->L_E[ch];
            border = sbr_ptr->abs_bord_trail[ch];

            for (l = 0; l < (sbr_ptr->L_E[ch] - 1); l++)
            {
                if (border < sbr_ptr->bs_rel_bord[ch][l])
                    return 1;

                border = (uint8) (border -sbr_ptr->bs_rel_bord[ch][l]);
                sbr_ptr->t_E[ch][--i] = (uint8) (sbr_ptr->rate * border);
            }
        }
        break;

    case AAC_VARFIX:
        if (sbr_ptr->L_E[ch] > 1)
        {
            int8 i = 1;
            border = sbr_ptr->abs_bord_lead[ch];

            for (l = 0; l < (sbr_ptr->L_E[ch] - 1); l++)
            {
                border = (uint8) (border +sbr_ptr->bs_rel_bord[ch][l]);

                if (sbr_ptr->rate * border + AAC_T_HFADJ > sbr_ptr->numTimeSlotsRate+AAC_T_HFGEN)
                    return 1;

                sbr_ptr->t_E[ch][i++] = (uint8) (sbr_ptr->rate * border);
            }
        }
        break;

    case AAC_VARVAR:
        if (sbr_ptr->bs_num_rel_0[ch])
        {
            int8 i = 1;
            border = sbr_ptr->abs_bord_lead[ch];
            for (l = 0; l < sbr_ptr->bs_num_rel_0[ch]; l++)
            {
                border =(uint8) ( border +sbr_ptr->bs_rel_bord_0[ch][l]);

                if (sbr_ptr->rate * border + AAC_T_HFADJ > sbr_ptr->numTimeSlotsRate+AAC_T_HFGEN)
                    return 1;
                sbr_ptr->t_E[ch][i++] = (uint8) (sbr_ptr->rate * border);
            }
        }
        if (sbr_ptr->bs_num_rel_1[ch])
        {
            int8 i = sbr_ptr->L_E[ch];
            border = sbr_ptr->abs_bord_trail[ch];

            for (l = 0; l < sbr_ptr->bs_num_rel_1[ch]; l++)
            {
                if (border < sbr_ptr->bs_rel_bord_1[ch][l])
                    return 1;

                border = (uint8) (border -sbr_ptr->bs_rel_bord_1[ch][l]);
                sbr_ptr->t_E[ch][--i] = (uint8) (sbr_ptr->rate * border);
            }
        }
        break;
    }

    return 0;
}

void AAC_NoiseFloorTimeBorderVector(AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    sbr_ptr->t_Q[ch][0] = sbr_ptr->t_E[ch][0];

    if (sbr_ptr->L_E[ch] == 1)
    {
        sbr_ptr->t_Q[ch][1] = sbr_ptr->t_E[ch][1];
        sbr_ptr->t_Q[ch][2] = 0;
    } else {
        uint8 index = AAC_MiddleBorder(sbr_ptr, ch);
        sbr_ptr->t_Q[ch][1] = sbr_ptr->t_E[ch][index];
        sbr_ptr->t_Q[ch][2] = sbr_ptr->t_E[ch][sbr_ptr->L_E[ch]];
    }
}



static uint8 AAC_MiddleBorder(AAC_SBR_INFO_T *sbr_ptr, uint8 ch)
{
    int8 retval = 0;

    switch (sbr_ptr->bs_frame_class[ch])
    {
    case AAC_FIXFIX:
        retval = (uint8) (sbr_ptr->L_E[ch]/2);
        break;
    case AAC_VARFIX:
        if (sbr_ptr->bs_pointer[ch] == 0)
            retval = 1;
        else if (sbr_ptr->bs_pointer[ch] == 1)
            retval = (uint8) (sbr_ptr->L_E[ch] - 1);
        else
            retval = (uint8) (sbr_ptr->bs_pointer[ch] - 1);
        break;
    case AAC_FIXVAR:
    case AAC_VARVAR:
        if (sbr_ptr->bs_pointer[ch] > 1)
            retval = (uint8) (sbr_ptr->L_E[ch] + 1 - sbr_ptr->bs_pointer[ch]);
        else
            retval = (uint8) (sbr_ptr->L_E[ch] - 1);
        break;
    }

    return (uint8) ((retval > 0) ? retval : 0);
}
#endif
