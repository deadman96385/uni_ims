/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28791 $ $Date: 2014-09-11 15:28:46 +0800 (Thu, 11 Sep 2014) $
 */

#ifndef _SIP_TU_H_
#define _SIP_TU_H_

vint TU_SendResponse(
    tSipIntMsg  *pMsg,
    tSipHandle   hTrans);

vint TU_SendRequest(
    tUri         *pTargetUri,
    tSipIntMsg   *pMsg,
    tpfAppCB      pfApp,
    tSipHandle    hOwner,
    uint32        taskId,
    tSipHandle    hTransport,
    tSipHandle   *hTransaction,
    tLocalIpConn *pLclConn);


#endif

