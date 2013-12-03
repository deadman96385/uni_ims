/*************************************************************************
** File Name:      syntax.h                                              *
** Author:         Reed zhang                                            *
** Date:           30/11/2005                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    the file is to do Reads the AAC bitstream as          *
**                 defined in 14496-3 (MPEG-4 Audio)					 *
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 30/11/2005     Reed zhang       Create.                               *
**************************************************************************/
#ifndef __SYNTAX_H__
#define __SYNTAX_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "aac_decoder.h"
#include "aac_bits.h"

int8 GASpecificConfig(AAC_BIT_FIFO_FORMAT_T *ld,
                        AAC_mp4AudioSpecificConfig *mp4ASC,
                        AAC_PROGRAM_CONFIG_T *pce_out);

uint8 AAC_AdtsFrameParsing(AAC_ADTS_HEADER_T *adts, 
				   AAC_BIT_FIFO_FORMAT_T *ld);

int16 AAC_RawDataBlockParsing(NeAACDecHandle    hDecoder_ptr, 				
                    AAC_BIT_FIFO_FORMAT_T           *ld, 
					AAC_PROGRAM_CONFIG_T    *pce,
					void   *aac_dec_mem_ptr,
					AAC_ELEMENT_T * tmp_cpe_ptr);

#ifdef __cplusplus
}
#endif
#endif
