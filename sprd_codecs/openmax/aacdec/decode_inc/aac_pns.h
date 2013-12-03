/*************************************************************************
** File Name:      aac_pns.h                                                                                       *
** Author:         Reed zhang                                                                                      *
** Date:           30/11/2005                                                                                      *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.                            *
** Description:                                                                                                         *
**                        Edit History                                                                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                                                              *
** 30/11/2005     Reed zhang       Create.                                                                 *
*************************************************************************/

#ifndef __AAC_PNS_H__
#define __AAC_PNS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aac_syntax.h"

#define NOISE_OFFSET 90

void AAC_PnsDecode(AAC_ICS_STREAM_T *ics_left, AAC_ICS_STREAM_T *ics_right,
                int32 *spec_left, int32 *spec_right, uint16 frame_len,
                uint8 channel_pair, uint8 object_type);

#define AAC_IS_NOISE(ics, group, sfb) (ics->sfb_cb[group][sfb] == AAC_NOISE_HCB) ? (1) :(0)


#ifdef __cplusplus
}
#endif
#endif
