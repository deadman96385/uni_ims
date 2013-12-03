/*************************************************************************
** File Name:      sbr_tf_grid.h                                         *
** Author:         Reed zhang                                            *
** Date:           09/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    Time/Frequency grid
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_TF_GRID_H__
#define __SBR_TF_GRID_H__

#ifdef __cplusplus
extern "C" {
#endif


uint8 AAC_EnvelopeTimeBorderVector(AAC_SBR_INFO_T *sbr_ptr, 
									uint8   ch);
void AAC_NoiseFloorTimeBorderVector(AAC_SBR_INFO_T  *sbr_ptr, 
									uint8   ch);


#ifdef __cplusplus
}
#endif
#endif

