/*************************************************************************
** File Name:      aac_pns.c                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "AAC_sbr_common.h"
#include "aac_structs.h"
#include "aac_pns.h"
/* static function declarations */
static void AAC_GenRandVector(int32 *spec, int16 scale_factor, int16 size,
                            uint8 sub);
extern int32 AAC_ARM_LongByLongAsm(
							  int32   d0,
                              int32   d1,
                              int16   bitw);

#ifdef FIXED_POINT
static int32 pow2_table[] =
{
        1073741824,
		1276901416,
		1518500248,
		1805811300
};
#endif

/* The function AAC_GenRandVector(addr, size) generates a vector of length
   <size> with signed random values of average energy MEAN_NRG per random
   value. A suitable random number generator can be realized using one
   multiplication/accumulation per random value.
*/

static int32  AAC_DEC_random_seek = 1;
#define AAC_DEC_RANDOM_COEF  1677995L   
#define AAC_DEC_RANDOM_CONST 752038887L 
#define AAC_DEC_PNS_ONE      (1L << 28)   //268435456L
static void AAC_GenRandVector(int32 *spec, 
                              int16 scale_factor, 
                              int16 size,
                              uint8 sub)
{
    uint16 i;
    int32 energy = 0, scale;
    int32 exp, frac;

    for (i = 0; i < size; i++)
    {        
        int32 tmp;        
        AAC_DEC_random_seek = (AAC_DEC_RANDOM_COEF * AAC_DEC_random_seek) + AAC_DEC_RANDOM_CONST;
        tmp     = AAC_DEC_random_seek >> 17; // S2.14
        energy += (tmp*tmp >> 14);
        spec[i] = tmp;
    }
    energy = AAC_DEC_Sqrt(energy); 
    if (energy > 0)
    {
        scale = AAC_DEC_Normalize(28, energy) ;
        exp   = (int32)scale_factor >> 2;
        frac  = (int32)scale_factor & 3;
        exp  -= sub;

        if (exp < 0)
            scale >>= -exp;
        else
            scale <<= exp;

        if (frac)
            scale = AAC_DEC_MultiplyShiftR32(scale << 2, pow2_table[frac]);

        for (i = 0; i < size; i++)
        {
            spec[i] = AAC_ARM_LongByLongAsm(scale, spec[i], 14);
        }
    }
}

void AAC_PnsDecode(AAC_ICS_STREAM_T *ics_left, 
                AAC_ICS_STREAM_T *ics_right,
                int32 *spec_left, 
				int32 *spec_right, 
				uint16 frame_len,
                uint8 channel_pair,
				uint8 object_type)
{
    uint8 g, sfb, b;
    int16 size, offs;

    uint8 group = 0;
    uint16 nshort = (uint16)(frame_len >> 3);

    uint8 sub = 0;


    if (ics_left->window_sequence == AAC_EIGHT_SHORT_SEQUENCE)
        sub = 7 /*7*/;
    else
        sub = 10 /*10*/;

    for (g = 0; g < ics_left->num_window_groups; g++)
    {
        /* Do perceptual noise substitution decoding */
        for (b = 0; b < ics_left->window_group_length[g]; b++)
        {
            for (sfb = 0; sfb < ics_left->max_sfb; sfb++)
            {
                if (AAC_IS_NOISE(ics_left, g, sfb))
                {
                    offs = ics_left->swb_offset[sfb];
                    size = (uint16)(ics_left->swb_offset[sfb+1] - offs);

                    /* Generate random vector */
                    AAC_GenRandVector(&spec_left[(group*nshort)+offs],
                        ics_left->scale_factors[g][sfb], size, sub);
                }

                if (channel_pair)
                {
                    if (AAC_IS_NOISE(ics_right, g, sfb))
                    {
                        if (((ics_left->ms_mask_present == 1) &&
                            (ics_left->ms_used[g][sfb])) ||
                            (ics_left->ms_mask_present == 2))
                        {
                            uint16 c;

                            offs = ics_right->swb_offset[sfb];
                            size = (uint16) (ics_right->swb_offset[sfb+1] - offs);

                            for (c = 0; c < size; c++)
                            {
                                spec_right[(group*nshort) + offs + c] =  spec_left[(group*nshort) + offs + c];
                            }
                        } else /*if (ics_left->ms_mask_present == 0)*/ {

                            offs = ics_right->swb_offset[sfb];
                            size = (uint16) (ics_right->swb_offset[sfb+1] - offs);

                            /* Generate random vector */
                            AAC_GenRandVector(&spec_right[(group*nshort)+offs],
                                ics_right->scale_factors[g][sfb], size, sub);
                        }
                    }
                }
            } /* sfb */
            group++;
        } /* b */
    } /* g */
}
 