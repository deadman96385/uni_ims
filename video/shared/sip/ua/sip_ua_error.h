/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-07 15:10:49 -0700 (Wed, 07 Jul 2010) $
 */

#ifndef _SIP_UA_ERROR_H_
#define _SIP_UA_ERROR_H_

vint UAE_dispatch(
    tSipHandle  hUa,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction);

void UAE_Entry(
    tSipHandle  hOwner,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction,
    uint32      id);

vint UAE_TrafficCB(
    tTransportType   transType,
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn,
    tSipIntMsg      *pMsg);

void UAE_RegisterDispatcher(
    tpfSipDispatcher pfHandler);


#endif
