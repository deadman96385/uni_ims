/*************************************************************************
** File Name:      filtbank.h                                            *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __FILTBANK_H__
#define __FILTBANK_H__
#include "aac_common.h"




#ifdef __cplusplus
extern "C" {
#endif	
void AAC_IfilterBank(uint8  window_sequence, 
				  uint8  window_shape,
                  uint8  window_shape_prev, 
				  int32  *freq_in,   //the data before IMDCT                  
				  int32  *overlap,    //the saving data for next frame imdct 
				  int16 *tmp_sample_buffer,
				  int16  aac_format_sign,  // 5, sbr , 2 aac_lc, 4, AAC_LTP
				  void    *aac_dec_mem_ptr,
				  int16 *ltp_sample_rec_ptr);
#ifdef __cplusplus
}
#endif
#endif
