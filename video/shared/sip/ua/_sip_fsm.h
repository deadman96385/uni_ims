/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_FSM_H_
#define _SIP_FSM_H_


typedef vint (*tpfUASM_Client)(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTransaction);
typedef vint (*tpfUASM_Server)(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTransaction);

void UASM_Init(void);

vint UASM_DialogClient(
    tSipDialog  *pDialog, 
    tSipIntMsg  *pMsg,
    tSipHandle   hTransaction);

vint UASM_DialogServer(
    tSipDialog  *pDialog, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

void UASM_NotifyFailed(
    tSipDialog *pDialog,
    tSipHandle  hTransaction);

void UASM_SubscribeFailed(
    tSipDialog *pDialog,
    tSipIntMsg  *pMsg,
    tSipHandle  hTransaction);

#endif
