/*
 * THIS IS AN UNPUBLISHED WORK CONTAING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20785 $ $Date: 2013-05-23 07:23:39 +0800 (Thu, 23 May 2013) $
 *
 */

#ifndef _TAPP_MOCK_GSM_H_
#define _TAPP_MOCK_GSM_H_

#define TAPP_AT_CMD_CLCC "AT+CLCC"

TAPP_Return TAPP_mockGsmInit(
    TAPP_GlobalObj *global_ptr);

void TAPP_mockGsmShutdown(
    TAPP_GlobalObj *global_ptr);

TAPP_Return TAPP_mockGsmIssueAt(
    TAPP_GlobalObj *global_ptr,
    char           *at_ptr);

TAPP_Return TAPP_mockGsmValidateAt(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

#endif //_TAPP_MOCK_GSM_H_
