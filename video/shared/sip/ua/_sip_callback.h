/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30075 $ $Date: 2014-11-27 10:31:32 +0800 (Thu, 27 Nov 2014) $
 */

#ifndef _SIP_CALLBACK_H_
#define _SIP_CALLBACK_H_


void UA_OkayTimeoutCB(
    tSipHandle hOwner, 
    void      *pArg);

void UA_OkayRecvCB(
    tSipHandle hOwner, 
    void      *pArg);

void UA_OkayRetryCB(
    tSipHandle hOwner, 
    void      *pArg);

void UA_PrackTimeoutCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_PrackRetryCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_ReRegisterCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_KeepalivesCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_RefreshPublishCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_SubscriberTimeoutCB(
    tSipHandle hOwner, 
    void      *pArg);

void UA_SessionRefresherCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_SessionExpiryCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_InviteExpiryCB(
    tSipHandle hOwner,
    void      *pArg);

void UA_reqPendingExpiryCB(
    tSipHandle hOwner,
    void      *pArg);

#endif
