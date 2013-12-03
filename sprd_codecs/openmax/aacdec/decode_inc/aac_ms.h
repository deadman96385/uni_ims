/*************************************************************************
** File Name:      ms.c                                                *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __MS_H__
#define __MS_H__

#ifdef __cplusplus
extern "C" {
#endif

void AAC_MsDecode(AAC_ICS_STREAM_T *ics, AAC_ICS_STREAM_T *icsr, int32 *l_spec, int32 *r_spec);

#ifdef __cplusplus
}
#endif
#endif
