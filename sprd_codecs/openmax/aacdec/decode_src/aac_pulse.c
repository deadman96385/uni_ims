/*************************************************************************
** File Name:      pulse.c                                                                                                           *
** Author:         Reed zhang                                                                                                      *
** Date:           30/11/2005                                                                                                       *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.                                        *
** Description:    the file is for mp4 container parsing                                                                 *
**                        Edit History                                                                                                   *
** ---------------------------------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                                                                     *
** 30/11/2005     Reed zhang       Create.                                                                                 *
*************************************************************************/
#include "AAC_sbr_common.h"
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_pulse.h"
extern const int32 AAC_DEC_pow05_table[];
extern const int16 pow2_tab[];
extern const int16 log2_tab[];
extern const int32 AAC_iq_table[257];
extern int32 AAC_DEC_SinglePtReQuanAsm(
                                              int32                       spec,
                                              int32                       scf,
                                              int32                       sgn
                                              );
uint8 AAC_PulseDecode(AAC_ICS_STREAM_T *ics_ptr, 
                                        int32 *spec_data_ptr     // S28.4
                                        )
{
    int32 i;
    int32 pos,  cur_sfb_w;
    AAC_PULSE_INFO_T *pul_ptr;
    int32  start_sfb;
    int32  scf;
    int32  spec;
    int16 *scf_ptr = (int16*) ics_ptr->scale_factors; 
    int32   sign = 1;   
    pul_ptr     = &(ics_ptr->pul);
    start_sfb   = pul_ptr->pulse_start_sfb;   
    scf            = scf_ptr[start_sfb] - 100 + 8;    
    pos           = ics_ptr->swb_offset[start_sfb];
    cur_sfb_w = ics_ptr->swb_offset[start_sfb+1];   
    for (i = 0; i <= pul_ptr->number_pulse; i++)
    {
        pos = (uint16) (pos + pul_ptr->pulse_offset[i]);
        if (pos >= 1024)
        {
             return 0x1002;
        }
        spec = spec_data_ptr[pos];
        if (pos >= cur_sfb_w)
        {
             scf = scf_ptr[start_sfb+1] - 100 + 8;
        }      
        sign = 1;     
        if  (spec <= 0)
        {
            sign = -1;
            spec = -spec;
        }
        if (spec)
        {
            /* Quantlization */
            int32 exp = (scf) >> 2;
            int32 rest = scf  - (exp << 2);
            spec = AAC_DEC_MultiplyShiftR32(spec,  AAC_DEC_pow05_table[3+rest]);
            spec = (spec+(1<<(exp-1))) >> exp;
            spec = AAC_DEC_ARM_Log2IntAsm(spec, (int16*)log2_tab);
            spec = (spec *3 + 2) >> 2;
            spec = AAC_DEC_ARM_Pow2IntAsm(spec, (int16*)pow2_tab);            
        }
        spec = spec + pul_ptr->pulse_amp[i];
        /* Quantlization */
        spec = AAC_DEC_SinglePtReQuanAsm(spec,  scf,  sign);      
        spec_data_ptr[pos] = spec;     
    }    
    return 0;
}
