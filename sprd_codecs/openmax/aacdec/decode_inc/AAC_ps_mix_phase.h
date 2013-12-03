/*************************************************************************
** File Name:      ps_mix_phase.h                                        *
** Author:         Reed zhang                                            *
** Date:           24/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file defines the common function and variation   *
**                 for ps decorrelation.                                 *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 24/04/2006     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __PS_MIX_PHASE__
#define __PS_MIX_PHASE__

#ifdef __cplusplus
extern "C" {
#endif
#include "aac_common.h"
#include "AAC_ps_dec.h"
	typedef struct AAC_PS_MIX_PHASETMP_VAR_T_tag
	{
		int32 *H11;//[50][4];
		int32 *deltaH11;//[50][4];      
		int32 *H12;//[50][4];
		int32 *deltaH12;//[50][4];
		int32 *H21;//[50][4];
		int32 *deltaH21;//[50][4];   
		int32 *H22;//[50][4]; 
		int32 *deltaH22;//[50][4];
		int32 *opt_h11;
		int32 *opt_h12;
		int32 *opt_h21;
		int32 *opt_h22;
	}AAC_PS_MIX_PHASETMP_VAR_T;


void AAC_ClacPsMixPhaseEnvelope(AAC_PS_INFO_T *ps_ptr, 
								AAC_PS_MIX_PHASETMP_VAR_T *ps_mix_phase_t_ptr,
								int16  band_No,
								int16  env_start);

#ifdef __cplusplus
}
#endif
#endif