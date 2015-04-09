/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 5312 $ $Date: 2008-02-18 19:05:41 -0600 (Mon, 18 Feb 2008) $
 */

#ifndef _VDD_H_
#define _VDD_H_


#include <video.h>

/*
 * To init the show for a particular call.
 * Note that surface size is unrelated to width, height arguments.
 * width and height arguments are the dimensions of the picture that will be
 * sent to the surface.
 */
int VDD_init(
    int               callId,
    int               flags);

int VDD_shutdown(
    int callId);


/*
 * Note: Use callId as
 * 0 ... camera, 1 ... stream 0, 2 ... stream 1
 */

int VDD_videoOut(
    Video_Picture *pic_ptr);

int VDD_clear(
    int  callId);


#endif

