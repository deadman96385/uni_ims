/*************************************************************************
** File Name:      sbr_hfgen.h                                           *
** Author:         Reed zhang                                            *
** Date:           17/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    define the high frequency function interface          *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 17/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __SBR_HFGEN_H__
#define __SBR_HFGEN_H__

#ifdef __cplusplus
extern "C" {
#endif
/************************************************************************/
/* hf_generation: be used to geverate high frequency information base on*/
/*	              the Low frequency and the SBR info. from bitstream    */
/************************************************************************/
void AAC_HfGeneration(AAC_SBR_INFO_T *sbr_ptr,                 // relative information from bitstream
				   aac_complex    Xlow[MAX_NTSRHFG][64] // the low frequency information base on the data from aac dec.
#ifdef AAC_SBR_LOW_POWER
                   ,int32 *deg_ptr
#endif
                   ,uint8 ch,
				   int32 *shared_buffer_ptr);

#ifdef __cplusplus
}
#endif
#endif

