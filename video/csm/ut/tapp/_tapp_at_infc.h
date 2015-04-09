/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20785 $ $Date: 2013-05-23 07:23:39 +0800 (Thu, 23 May 2013) $
 *
 */

#ifndef _TAPP_AT_INFC_H_
#define _TAPP_AT_INFC_H_

TAPP_Return TAPP_atInfcInit(
    TAPP_GlobalObj *global_ptr);

void TAPP_atInfcShutdown(
    TAPP_GlobalObj *global_ptr);

TAPP_Return TAPP_atInfcIssueAt(
    TAPP_GlobalObj *global_ptr,
    char           *at_ptr);

TAPP_Return TAPP_atInfcValidateAt(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

#endif // _TAPP_AT_INFC
