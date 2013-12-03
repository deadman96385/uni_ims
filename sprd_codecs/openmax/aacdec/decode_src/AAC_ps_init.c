/*************************************************************************
** File Name:      ps_init.c                                             *
** Author:         Reed zhang                                            *
** Date:           18/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize SBR signal analyzing                          *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 18/04/2006     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "AAC_ps_dec.h"
#include "AAC_ps_tables.h"

void AAC_HybridInit(AAC_HYB_INFO_T *hyb_ptr)
{
    uint8 i;	
    hyb_ptr->resolution20[0] = 8;
    hyb_ptr->resolution20[1] = 2;
    hyb_ptr->resolution20[2] = 2;	
    hyb_ptr->frame_len       = 32;

    for (i = 0; i < 5; i++)
    {
        AAC_DEC_MEMSET(hyb_ptr->buffer[i], 0, 12 * sizeof(aac_complex));
    }
    //return hyb_ptr;
}

void AAC_PsInit(uint8 sr_index,
		     AAC_PS_INFO_T *ps_ptr)
{
    uint8 i;
    uint8 short_delay_band;
    AAC_DEC_MEMSET(ps_ptr, 0, sizeof(AAC_PS_INFO_T));
    AAC_HybridInit(&ps_ptr->hyb);
    ps_ptr->ps_data_available = 0;		
	
    /* delay stuff*/
    ps_ptr->saved_delay = 0;
    /* THESE ARE CONSTANTS NOW */
    short_delay_band     = 35;
    ps_ptr->nr_allpass_bands = 22;

    /* THESE ARE CONSTANT NOW IF PS IS INDEPENDANT OF SAMPLERATE */
    
    

    /* mixing and phase */
    for (i = 0; i < 50; i++)
    {
        ps_ptr->h11_prev[i] = 536870912;
        ps_ptr->h12_prev[i] = 536870912;
    }
    ps_ptr->phase_hist = 0;
#ifndef _AAC_PS_BASE_LINE_
    for (i = 0; i < 20; i++)
    {
        RE(ps_ptr->ipd_prev[i][0]) = 0;
        IM(ps_ptr->ipd_prev[i][0]) = 0;
        RE(ps_ptr->ipd_prev[i][1]) = 0;
        IM(ps_ptr->ipd_prev[i][1]) = 0;
        RE(ps_ptr->opd_prev[i][0]) = 0;
        IM(ps_ptr->opd_prev[i][0]) = 0;
        RE(ps_ptr->opd_prev[i][1]) = 0;
        IM(ps_ptr->opd_prev[i][1]) = 0;
    }
#endif
}