/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 18551 $ $Date: 2012-10-23 01:55:53 -0700 (Tue, 23 Oct 2012) $
 *
 */

#ifndef __CSM_UT_UTILS_H_
#define __CSM_UT_UTILS_H_

#include "csm_event.h"
#include "_csm_ut.h"

int _CSM_UT_getLine(
    char *buf,
    unsigned int max);

int CSM_UT_getBuffer(
    char *buf,
    unsigned int max);

UT_Return CSM_UT_writeCsmEvent(
    const CSM_InputEvent *csmEvt_ptr);

#endif
