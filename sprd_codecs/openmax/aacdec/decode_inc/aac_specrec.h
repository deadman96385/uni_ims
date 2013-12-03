/*************************************************************************
** File Name:      specrec.h                                             *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is to do
					Spectral reconstruction:
					- grouping/sectioning
					- inverse quantization
					- applying scalefactors

**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __SPECREC_H__
#define __SPECREC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aac_syntax.h"

// add by reed at 2005-10-10
#if 0
#define  IQ(q, tab, error, result) {result = tab; if (q < 0) {result = -tab;} }
#else
#define  IQ(q, tab) (q > 0)? (tab[q]) : (-tab[-q])
#endif

uint8 AAC_WindowGroupingInfoSingleParsing(NeAACDecHandle hDecoder_ptr, AAC_ICS_STREAM_T *ics1);
uint8 AAC_WindowGroupingInfoPairParsing(NeAACDecHandle hDecoder, AAC_ICS_STREAM_T *ics1, AAC_ICS_STREAM_T *ics);
uint8 AAC_ReconstructChannelPair(NeAACDecHandle hDecoder, 
                                 AAC_ICS_STREAM_T *ics1,
                                 AAC_ICS_STREAM_T *ics2,
                                 AAC_ELEMENT_T *cpe_ptr, 
                                 int32 *spec_data1_ptr, 
                                 int32 *spec_data2_ptr,
								 void           *aac_dec_mem_ptr);
uint8 AAC_ReconstructSingleChannel(NeAACDecHandle hDecoder, AAC_ICS_STREAM_T *ics, AAC_ELEMENT_T *sce,
                                int32 *spec_data,
								void          *aac_dec_mem_ptr);

#ifdef __cplusplus
}
#endif
#endif
