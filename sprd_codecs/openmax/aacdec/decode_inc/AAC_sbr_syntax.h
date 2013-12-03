/*************************************************************************
** File Name:      sbr_syntax.h                                          *
** Author:         Reed zhang                                            *
** Date:           10/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    this file is use to Calculate frequency band tables

**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 10/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_SYNTAX_H__
#define __SBR_SYNTAX_H__

#ifdef __cplusplus
extern "C" {
#endif
	
#include "aac_common.h"
#include "aac_structs.h"
#include "AAC_sbr_dec.h"

uint8 AAC_SbrExtensionData(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
						   AAC_SBR_INFO_T *sbr_ptr,
						   uint16 cnt, int32 *tmp_buf_ptr);

#ifdef __cplusplus
}
#endif
#endif /* __SBR_SYNTAX_H__ */

