/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29252 $ $Date: 2014-10-10 08:15:06 +0800 (Fri, 10 Oct 2014) $
 *
 */

#ifndef __TAPP_REPORT_H_
#define __TAPP_REPORT_H_

void TAPP_rptInit();

void TAPP_rptShutdown();

void TAPP_rptOutputAction(
    TAPP_Action *act_ptr);

void TAPP_rptOutputActionResult(
    TAPP_Action *act_ptr,
    TAPP_Event  *evt_ptr,
    TAPP_Return  result);

void TAPP_rptOutput(
    const char     *format_ptr,
    int             arg1,
    int             arg2,
    int             arg3);

void TAPP_rptXmlOutput(
    const char     *format_ptr,
    int             arg1,
    int             arg2,
    int             arg3);

void TAPP_rptGenXmlReport(
    TAPP_GlobalObj *global_ptr);

#endif
