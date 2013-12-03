/*************************************************************************
** File Name:      ms.c                                                *
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
#include "aac_ms.h"
#include "aac_is.h"
extern void AAC_MSAsm(int32 *l_spec_ptr, int32 *r_spec_ptr, uint16 offset_start, uint16 offset_end);

void AAC_MsDecode(AAC_ICS_STREAM_T *icsl_ptr,
			   AAC_ICS_STREAM_T *icsr_ptr, 
			   int32    *l_spec, 
			   int32    *r_spec)
{
    uint8 g, b, sfb;
    uint16 group  = 0;
    uint8 ms_mask = icsl_ptr->ms_mask_present;
	
    if (ms_mask >= 1)
    {
        if (AAC_EIGHT_SHORT_SEQUENCE !=icsl_ptr->window_sequence)
        {
            /* long block */
            uint8 max_sfb0  = icsl_ptr->max_sfb;
            for (sfb = 0; sfb < max_sfb0; sfb++)
            {	   
                if ((icsl_ptr->ms_used[0][sfb] || ms_mask == 2) && icsr_ptr->sfb_cb[0][sfb] < 14 && (icsl_ptr->sfb_cb[0][sfb] != AAC_NOISE_HCB))
                {
                    AAC_MSAsm(l_spec, r_spec, icsl_ptr->swb_offset[sfb], icsl_ptr->swb_offset[sfb+1]);
            
                }
            }
                
        }else
        {
            for (g = 0; g < icsl_ptr->num_window_groups; g++) 
	        {
				uint8 win0 = icsl_ptr->window_group_length[g];
	            for (b = 0; b < win0; b++)
	            {
					uint8 *tms_used = icsl_ptr->ms_used[g];
					uint8 max_sfb0  = icsl_ptr->max_sfb;
	                for (sfb = 0; sfb < max_sfb0; sfb++)
	                {
					    /* If intensity stereo coding or noise substitution is on
					       for a particular scalefactor band, no M/S stereo decoding
						   is carried out.  */
						//if ((*(tms_used++) || ms_mask == 2) && !is_intensity(icsr->sfb_cb[g][sfb]) && (ics->sfb_cb[group][sfb] != AAC_NOISE_HCB))
						
						if ((*(tms_used++) || ms_mask == 2) && icsr_ptr->sfb_cb[g][sfb] < 14 && (icsl_ptr->sfb_cb[group][sfb] != AAC_NOISE_HCB))
	                    {

	                        AAC_MSAsm(l_spec + (group << 7), r_spec + (group << 7), icsl_ptr->swb_offset[sfb], icsl_ptr->swb_offset[sfb+1]);
	            
	                    }
	                }
	                group++;
	            }
	        }
        }
        
    }
}
