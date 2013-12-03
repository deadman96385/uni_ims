/*************************************************************************
** File Name:      sbr_qmf.c                                             *
** Author:         Reed zhang                                            *
** Date:           12/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    realize SBR signal analyzing                          *               
**                                                                       *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 12/01/2006     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __SBR_QMF_H__
#define __SBR_QMF_H__

#ifdef __cplusplus
extern "C" {
#endif

void AAC_SbrQmfAnalysis(     AAC_SBR_INFO_T     *sbr_ptr, 
						 int32    *qmfa_ptr,
						 int32 *input_ptr,
                         aac_complex        X[MAX_NTSRHFG][64], 
						 uint8      kx,
						 int32       *tmp_buf_ptr);

void AAC_SbrQmfSynthesis( int16 numTimeSlotsRate, 
						  int32   *qmfs_ptr, 
						  aac_complex     X[MAX_NTSRHFG][64],
                          uint16  *output_ptr,
						  int32   *tmp_buf_ptr);

#ifdef __cplusplus
}
#endif
#endif

