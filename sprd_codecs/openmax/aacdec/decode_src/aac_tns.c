/*************************************************************************
** File Name:      tns.c                                                 *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:                                                          *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#include "aac_common.h"
#include "aac_structs.h"
#include "aac_tns.h"

/* static function declarations */
void AAC_TnsDecCoef(uint8 order, 
							uint8 coef_res_bits,
							uint8 coef_compress,
                            uint8 *coef, 
							int32  *a);

#ifdef _AACARM_
extern void AACTnsTnsDecodeCoefAsm(
							       int32   *a_ptr,
                                   int32   *tm_buf_ptr,
                                   int8     order1);
void AACTnsArFilterAsm(
					   int32   *spectrum_ptr,
                       int32   *lpc_ptr,
                       int32   *state_ptr,
                       int32    para);
#endif

#define SBR_CLIP(val, min, max) ((val>max) ? (val = max):((val < min) ? (val = min) : (val)))
#define  TNSBIT 11
/* the following table fixed-point is: S0.12*/
const int32 tns_coef_0_3[]
=
{
	0, 
		116469779, 
		209871291, 
		261705219, 
		-264357318, 
		-232471924, 
		-172546985, 
		-91810333, 
		-116469779, 
		-209871291, 
		-261705219, 
		-261705219, 
		-264357318, 
		-232471924, 
		-172546985, 
		-91810333,  
};

const int32 tns_coef_0_4[] 
=
{
	0, 
		55810870, 
		109182536, 
		157782402, 
		199486420, 
		232471924, 
		255297290, 
		266964938, 
		-267290358, 
		-258188105, 
		-240293566, 
		-214216118, 
		-180843797, 
		-141313058, 
		-96970071, 
		-49324886, 
};

const int32 tns_coef_1_3[] 
=
{
	0, 
		116469779, 
		-172546985, 
		-91810333, 
		261705219, 
		209871291, 
		-172546985, 
		-91810333, 
		-116469779, 
		-209871291, 
		-172546985, 
		-91810333, 
		-209871291, 
		-116469779, 
		-172546985, 
		-91810333, 
};

const int32 tns_coef_1_4[] 
=
{
	0, 
		55810870, 
		109182536, 
		157782402, 
		-180843797, 
		-141313058, 
		-96970071, 
		-49324886, 
		266964938, 
		255297290, 
		232471924, 
		199486420, 
		-180843797, 
		-141313058, 
		-96970071, 
		-49324886, 
};

/* TNS decoding for one channel and frame */
void AAC_TnsDecodeFrame(AAC_ICS_STREAM_T *ics_ptr, 
                                              AAC_TNS_INFO_T  *tns_ptr,
                                              uint8   sr_index,
                                              uint8   object_type, 
                                              int32    *spec_ptr 
                                              )
{
    uint8  w, f, tns_order;
    int8   inc;
    int16  size;
    uint16 bottom, top, start, end;
    int32  lpc[TNS_MAX_ORDER+4] = {0};
#ifdef _AACARM_
    int32  tmp_state[2*TNS_MAX_ORDER], para;
#endif

    
    if (!ics_ptr->tns_data_present)
        return;

    for (w = 0; w < ics_ptr->num_windows; w++)
    {
        uint8 n_filt;
        bottom = ics_ptr->num_swb;
        n_filt = tns_ptr->n_filt[w];
        for (f = 0; f < n_filt; f++)
        {
            uint8 ls = 0;
            top = bottom;
            bottom = (uint16)(AAC_DEC_MAX(top - tns_ptr->length[w][f], 0));
            tns_order = (uint8) (AAC_DEC_MIN(tns_ptr->order[w][f], TNS_MAX_ORDER));
            if (!tns_order)
                continue;			
            AAC_TnsDecCoef(tns_order,
                                          (uint8)(tns_ptr->coef_res[w]+3),
                                          tns_ptr->coef_compress[w][f], 
                                          tns_ptr->coef[w][f], 
                                          lpc);

            if (ics_ptr->window_sequence == AAC_EIGHT_SHORT_SEQUENCE) 
           {
                  ls = 1;
           }
            start =  (uint16) (AAC_DEC_MIN(bottom,
                             AAC_MaxTnsSfb((uint8)sr_index, (uint8)object_type, (uint8)ls)));
            start = (uint16) (AAC_DEC_MIN(start, ics_ptr->max_sfb));
            start = ics_ptr->swb_offset[start];

            end = (uint16) (AAC_DEC_MIN(top, AAC_MaxTnsSfb(sr_index, object_type, ls)));
            end = (uint16) (AAC_DEC_MIN(end, ics_ptr->max_sfb));
            end = ics_ptr->swb_offset[end];

            size = (int16) (end - start);
            if (size <= 0)
                continue;

            if (tns_ptr->direction[w][f])
            {
                inc = -1;
                start = (uint16) (end - 1);
            } else 
            {
                inc = 1;
            }
            para = size;
            para |= ((int32)inc << 24);
            para |= ((int32)tns_order << 16);
            AAC_DEC_MEMSET(tmp_state, 0, 40 * sizeof(int32));
            AACTnsArFilterAsm(spec_ptr + (w*128)+start, lpc, tmp_state, para);
        }
    }
}

/* Decoder transmitted coefficients for one TNS filter */
void AAC_TnsDecCoef(uint8 order, 
							uint8 coef_res_bits,
							uint8 coef_compress,
                            uint8 *coef_ptr,
							int32 *a_ptr)
{

    uint8 i=0;
	int32 tmp2[42] = {0};
#ifndef _AACARM_	
	int32  *tmp2_ptr = tmp2;
	int32  *b_ptr = tmp2 + TNS_MAX_ORDER+1;
	uint8   m=0;
#endif	
    /* Conversion to signed integer */
    for (i = 0; i < order; i++)
    {
        if (coef_compress == 0)
        {
            if (coef_res_bits == 3)
            {
                tmp2[i] = tns_coef_0_3[coef_ptr[i]];
            } else 
			{
                tmp2[i] = tns_coef_0_4[coef_ptr[i]];
            }
        } else 
		{
            if (coef_res_bits == 3)
            {
                tmp2[i] = tns_coef_1_3[coef_ptr[i]];
            } else {
                tmp2[i] = tns_coef_1_4[coef_ptr[i]];
            }
        }
    }
    AACTnsTnsDecodeCoefAsm(a_ptr, tmp2, order);

}
extern const uint8 AAC_DEC_tns_sbf_max[][4];
extern void AAC_DEC_TnsAnalyzeFilterAsm(
                                        int32   *spectrum_ptr,
                                        int32   *lpc_ptr,
                                        int32   *state_ptr,
                                        int32    para);



int16 AAC_DEC_TNSFrmAnalysisFilter(int32              *spec_ptr,
                                   AAC_DEC_LTP_INFO_T *ltp_info_ptr,
                                   AAC_ICS_STREAM_T   *ics_ptr
                                  )
{
    int16 sf_index    = ltp_info_ptr->sf_index;
    int16 win = 0, filt = 0;
    int16 bottom, top, tns_order;
    int32 lpc_coef[TNS_MAX_ORDER+1]= {0};
    int16 f_start, f_end, f_size;
    int32  tmp_state[2*TNS_MAX_ORDER], para;
    
    if (!ics_ptr->tns_data_present)
    {
        /* no TNS processing */
        return 0;
    }
    /* TNS analysis filter */
    for (win = 0; win < ics_ptr->num_windows; win++)
    {
        bottom = ics_ptr->num_swb;
        for (filt = 0; filt < ics_ptr->tns.n_filt[win]; filt++)
        {
            int16 inc = 1;
            
            top       = bottom;
            bottom    = (int16) (AAC_DEC_MAX((top-ics_ptr->tns.length[win][filt]), 0));
            tns_order = (int16) (AAC_DEC_MIN(ics_ptr->tns.order[win][filt], TNS_MAX_ORDER));
            /* tns order number is equal to zero, will do next band */
            if (0 == tns_order)
                continue;
            /* get LPC coefficient */
            AAC_TnsDecCoef((uint8) tns_order,
                            (uint8) (ics_ptr->tns.coef_res[win]+3),
                            ics_ptr->tns.coef_compress[win][filt],
                            ics_ptr->tns.coef[win][filt],
                            lpc_coef);

            f_start = (int16) (AAC_DEC_MIN(bottom, AAC_DEC_tns_sbf_max[sf_index][0]));
            f_start = (int16) (AAC_DEC_MIN(f_start, ics_ptr->max_sfb));
            f_start = (int16) (ics_ptr->swb_offset[f_start]);

            f_end = (int16) (AAC_DEC_MIN(top, AAC_DEC_tns_sbf_max[sf_index][0]));
            f_end = (int16) (AAC_DEC_MIN(f_end, ics_ptr->max_sfb));
            f_end = (int16) (ics_ptr->swb_offset[f_end]);
            
            f_size = (int16) (f_end - f_start);
            if (f_size <= 0)  // next sub-band
                continue;

            if (ics_ptr->tns.direction[win][filt])
            {                
                f_start = (int16) (f_end - 1);
                inc = -1;
            }
            /* do tns analyze processing*/
            para = f_size;
            para |= ((int32)inc << 24);
            para |= ((int32)tns_order << 16);
           AAC_DEC_MEMSET(tmp_state, 0, 40 * sizeof(int32));
            AAC_DEC_TnsAnalyzeFilterAsm(spec_ptr+f_start,
                                        lpc_coef,
                                        tmp_state,
                                        para);                 
            
        }
    }
    
    return 0;

}                                  

