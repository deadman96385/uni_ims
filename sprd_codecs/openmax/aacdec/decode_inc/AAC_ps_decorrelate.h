/*************************************************************************
** File Name:      ps_decorrelate.h                                      *
** Author:         Reed zhang                                            *
** Date:           21/04/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file is to do PS mix phase processing            *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 21/04/2006     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __PS_DECORRELATION__
#define __PS_DECORRELATION__

#ifdef __cplusplus
extern "C" {
#endif
#include "aac_common.h"
#include "AAC_ps_dec.h"

#define PS_DECORRELATION_ADJ 8
void PsDecorrelate(AAC_PS_INFO_T  *ps_ptr, 
                              aac_complex     *X_left,          
                              aac_complex     *X_hybrid_left);
                              
                              

#ifdef __cplusplus
}
#endif
#endif