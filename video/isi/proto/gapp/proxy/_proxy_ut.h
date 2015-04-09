/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2007 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 26935 $ $Date: 2014-06-13 17:10:27 +0800 (Fri, 13 Jun 2014) $
 */

#ifndef _PROXY_UT_H_
#define _PROXY_UT_H_

/*
 * Public routines
 */

void PRXY_printAT(
    const char *prefix,
    const char *at_ptr);

void PXRY_testAtParse(
    void);

void PRXY_printCsmInputEvent(
    const CSM_InputEvent* csmEvt_ptr);
#endif
