/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_UA_SERVER_H_
#define _SIP_UA_SERVER_H_


typedef vint (*tpfUaServer)(
    tSipHandle, 
    tSipIntMsg*, 
    tSipHandle);

void UAS_Entry(
    tSipHandle  hOwner,    
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction,
    uint32      id);

vint UAS_Invite(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Update(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Cancel(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Bye(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Ack(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Notify(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Refer(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Message(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Publish(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Prack(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Register(
    tSipHandle  hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAC_UnRegister(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction);

vint UAS_Options(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Error(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Subscribe(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);

vint UAS_Info(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction);
                                        
vint UAS_TrafficCB(
    tTransportType   transType,
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn,
    tSipIntMsg      *pMsg);

void UAS_RegisterMethodCallBack(
    tSipMethod  method, 
    tpfUaServer pfHandler);

void UAS_RegisterDispatcher(
    tpfSipDispatcher pfHandler);

vint UAS_SetupProxy(
    tpfSipProxy pfHandler, 
    char       *pProxyFqdn);

void UAS_SetMatch(
    tSipMatchType matchType);

#endif
