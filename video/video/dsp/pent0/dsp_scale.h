/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12262 $ $Date: 2010-06-10 18:55:56 -0400 (Thu, 10 Jun 2010) $
 */

#ifndef _DSP_SCALE_H_
#define _DSP_SCALE_H_

#include <video.h>

/*
 * ======== Dsp_scale() ========
 * Scales/color converts
 * Returns:
 */
void Dsp_scale(
    void         *dstData_ptr,
    int           dstW,
    int           dstH,
    VideoFormat   dstFormat,
    void         *srcData_ptr,
    int           srcW,
    int           srcH,
    VideoFormat   srcFormat);

#endif

