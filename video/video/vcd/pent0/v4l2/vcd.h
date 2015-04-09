/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _VCD_H_
#define _VCD_H_


#include <video.h>

int VCD_init(
    int width,
    int height,
    int id,
    int flags);

int VCD_shutdown(
    void);

int VCD_mute(
    void);

int VCD_unmute(
    void);

int VCD_videoIn(
    Video_Picture *pic_ptr);

int VCD_requestResolution(
    int width,
    int height);

#endif
