/*************************************************************************
** File Name:      sbr_hfadj.h                                           *
** Author:         Reed zhang                                            *
** Date:           19/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    define the interface for HF ADJUSTMENT model          *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 19/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_HFADJ_H__
#define __SBR_HFADJ_H__

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/* the function HfAdjustment description is shown below:                */
/* sbr : the relative information for HF adjustment                     */
/* Xsbr: the high frequency data from HF generation model               */
/* deg : the degree of aliasing                                         */
/* ch  : the current channel                                            */
/************************************************************************/
void AAC_HfAdjustment( AAC_SBR_INFO_T *sbr_ptr,
				   aac_complex    Xsbr[MAX_NTSRHFG][64]  
#ifdef AAC_SBR_LOW_POWER
                   ,int32 *deg_ptr
#endif
                   ,uint8 ch,
				   int32  *tmp_buf_ptr);

#ifdef __cplusplus
}
#endif
#endif

