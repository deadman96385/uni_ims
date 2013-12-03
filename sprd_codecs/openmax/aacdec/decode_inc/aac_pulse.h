/*************************************************************************
** File Name:      pulse.h                                               *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is for mp4 container parsing
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __PULSE_H__
#define __PULSE_H__

#ifdef __cplusplus
extern "C" {
#endif

uint8 AAC_PulseDecode(AAC_ICS_STREAM_T *ics, int32 *spec_coef);

#ifdef __cplusplus
}
#endif
#endif
