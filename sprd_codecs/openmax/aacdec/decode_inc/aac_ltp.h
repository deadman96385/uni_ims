/*************************************************************************
** File Name:      aac_ltp.h                                             *
** Author:         Reed zhang                                            *
** Date:           09/11/2010                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:                                                          *
**      This file is used to do LTP mode.                                *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 09/11/2010     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __AAC_LTP_H__
#define __AAC_LTP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "aac_common.h"
#include "aac_structs.h"


/* FUNCTION DESCRIPTION 
    parameter: 
      spec_ptr:        input/output, spectrum data
      ics_ptr:         input, stream side information
      hdecoder_ptr:    input, stream information
      pred_sample_ptr: temporary buffer for mid-memory , size, 2048
      tmp_buf_ptr:     temporary buffer for mid-memory , size, 1024
*/
int16 AAC_DEC_LtpModel(int32              *spec_ptr,
                       AAC_ICS_STREAM_T   *ics_ptr,
                       int16               window_shape_prev,
                       int32              *pred_sample_ptr,
                       int32              *tmp_buf_ptr,                       

                       int16              *last_first_frm_pcm_ptr,
                       int32              *last_first_frm_overlap_ptr);

/* LTP mpde side information stream parsing */
/* FUNCTION DESCRIPTION 
    parameter:       
      ics_ptr:    input/output, stream side information
      ld_ptr:     input, stream information
*/
int16 AAC_DEC_LTPSideInfoParsing(AAC_ICS_STREAM_T      *ics_ptr,
                                 AAC_BIT_FIFO_FORMAT_T *ld_ptr);                                 

/* FUNCTION DESCRIPTION 
    parameter:       
      ics_ptr:          input/output, stream side information
      pcm_data_ptr:     input, time domain pcm data
      overlap_data_ptr: input, filterbank overlap data
*/
void AAC_DEC_LTPOverlapUpdate(AAC_ICS_STREAM_T      *ics_ptr,
                              int16                 *pcm_data_ptr,
                              int32                 *overlap_data_ptr);

#ifdef __cplusplus
}
#endif
#endif

