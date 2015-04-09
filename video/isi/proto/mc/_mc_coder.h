/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 */
  
#ifndef _MC_CODER_H_
#define _MC_CODER_H_

void MC_parseCoder(
    char     *name_ptr,
    char     *desc_ptr,
    MC_Coder *encoder_ptr,
    MC_Coder *decoder_ptr);

vint MC_isH263(
    char *coderName_ptr,
    vint  maxCoderLen);

vint MC_isH264(
    char *coderName_ptr,
    vint  maxCoderLen);

#endif
