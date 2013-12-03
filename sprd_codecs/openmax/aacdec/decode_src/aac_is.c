/*************************************************************************
** File Name:      is.c                                                *
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
#include "aac_structs.h"
#include "AAC_sbr_common.h"
#include "aac_is.h"

extern const int32 AAC_DEC_pow05_table[];

void AAC_IsDecode(AAC_ICS_STREAM_T *ics,
			   AAC_ICS_STREAM_T *icsr, 
			   int32    *l_spec,
			   int32    *r_spec)
{
    uint8 g, sfb, b;
    uint16 i;
    int32 exp, frac;
    uint8 max_sfb0 = icsr->max_sfb;
	
    uint16 nshort = 1024/8;
    uint8 group = 0;
	
    for (g = 0; g < icsr->num_window_groups; g++)
    {
        /* Do intensity stereo decoding */
        for (b = 0; b < icsr->window_group_length[g]; b++)
        {            
            uint8 *sfb_cb0 = icsr->sfb_cb[g];
            for (sfb = 0; sfb < max_sfb0; sfb++)
            {
                uint8 t_sfb = *(sfb_cb0++);				
				
                if (t_sfb >= 14)//(is_intensity(t_sfb)) // == AAC_INTENSITY_HCB || AAC_INTENSITY_HCB2
                {
                    exp = (int32)(icsr->scale_factors[g][sfb]) >> 2;
                    frac = (int32)icsr->scale_factors[g][sfb] & 3;
                    /* Scale from left to right channel,
				       do not touch left channel */
                    for (i = icsr->swb_offset[sfb]; i < icsr->swb_offset[sfb+1]; i++)
                    {
                        int32 t_data = 0;
                        if (exp < 0)
                            t_data = l_spec[(group*nshort)+i] << -exp;
                        else
                            t_data = l_spec[(group*nshort)+i] >> exp;						
                        t_data = AAC_DEC_MultiplyShiftR32((t_data<<2), AAC_DEC_pow05_table[frac + 3]);
						
                        if (is_intensity(icsr->sfb_cb[g][sfb]) != invert_intensity(ics, g, sfb))
                            t_data = -t_data;
                        r_spec[(group*nshort)+i] = t_data;
                    }
                }
            }
            group++;
        }
    }
}
