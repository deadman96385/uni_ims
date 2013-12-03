/*************************************************************************
** File Name:      sbr_e_nf.h                                            *
** Author:         Reed zhang                                            *
** Date:           10/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 10/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_E_NF_H__
#define __SBR_E_NF_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifndef FIXED_POINT
#define P2_TABLE_OFFSET 35
#define P2_TABLE_MAX 91
#else
#define P2Q_TABLE_OFFSET 24
#define P2Q_TABLE_MAX 7
#define P2_TABLE_OFFSET 35
#define P2_TABLE_MAX 31
#endif
#define P2_TABLE_RCP_OFFSET 12
#define P2_TABLE_RCP_MAX 21


void AAC_ExtractEnvelopeData(AAC_SBR_INFO_T *sbr_ptr,
						   uint8 ch);
void AAC_ExtractNoiseFloorData(AAC_SBR_INFO_T *sbr_ptr, 
							  uint8 ch);

#ifdef __cplusplus
}
#endif
#endif

