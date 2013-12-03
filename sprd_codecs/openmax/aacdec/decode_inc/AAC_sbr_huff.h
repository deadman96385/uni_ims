/*************************************************************************
** File Name:      sbr_huff.h                                            *
** Author:         Reed zhang                                            *
** Date:           10/01/2006                                            *
** Copyright:      2001 Spreatrum, Incoporated. All Rights Reserved.     *
** Description:    This file is to do SBR envelope and noise huffman     *
**                 decoding                                              *
**                 
**                        Edit History                                   *
** ----------------------------------------------------------------------*
** DATE           NAME             DESCRIPTION                           *
** 10/01/2006     Reed zhang       Create.                               *
**************************************************************************/

#ifndef __SBR_HUFF_H__
#define __SBR_HUFF_H__

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
//    the function sbr_envelope is used for envelope huffman decoding and 
//    the parameter is shown below:
//    ld:  the SBR stream
//    sbr: the relative information for decoding
//    ch:  the channel No.
/************************************************************************/
void AAC_SbrEnvelope(AAC_BIT_FIFO_FORMAT_T *ld_ptr, 
				  AAC_SBR_INFO_T *sbr_ptr, 
				  uint8 ch);

/************************************************************************/
//    the function sbr_noise is used for noise huffman decoding and 
//    the parameter is shown below:
//    ld:  the SBR stream
//    sbr: the relative information for decoding
//    ch:  the channel No.
/************************************************************************/
void AAC_SbrNoise(AAC_BIT_FIFO_FORMAT_T *ld_ptr,
			   AAC_SBR_INFO_T *sbr_ptr,
			   uint8 ch);

#ifdef __cplusplus
}
#endif
#endif

