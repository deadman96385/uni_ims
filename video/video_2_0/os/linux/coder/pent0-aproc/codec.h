/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#ifndef CODEC_H_
#define CODEC_H_

#include <codec_object.h>

/*
 * Encoder decoder.
 */
#define CODEC_ENCODER (0)
#define CODEC_DECODER (1)

/*
 * Public methods
 */

void Codec_init(
    CODEC_Ptr);

void codecInit(
    CODEC_Ptr,
    vint,
    vint);

void codecModify(
    CODEC_Ptr);

void codecRelease(
    CODEC_Ptr,
    vint);

void requestFIR(
    CODEC_Ptr);

#endif /* CODEC_H_ */
