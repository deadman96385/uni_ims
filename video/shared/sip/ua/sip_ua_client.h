/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_UA_CLIENT_H_
#define _SIP_UA_CLIENT_H_

void UAC_Entry(
    tSipHandle  hOwner, 
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction,
    uint32      id);

vint UAC_Invite(
    tSipHandle  hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_ReInvite(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Update(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Refer(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg  *pMsg, 
    tSipHandle  hTransaction);

vint UAC_Notify(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Subscribe(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Message(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Info(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Prack(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_MessageNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_NotifyNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Cancel(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Bye(
    tSipHandle  hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Register(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_UnRegister(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction);

vint UAC_PublishNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Publish(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_Options(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_OptionsNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_TrafficCB(
    tTransportType   transType,
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn,
    tSipIntMsg      *pMsg);

void UAC_RegisterDispatcher(tpfSipDispatcher pfHandler);

vint UAC_SetupProxy(
    tpfSipProxy pfHandler, 
    char       *pProxyFqdn);

#endif
