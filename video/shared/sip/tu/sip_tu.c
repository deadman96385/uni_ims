/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28728 $ $Date: 2014-09-05 17:01:31 +0800 (Fri, 05 Sep 2014) $
 */

#include <stdio.h>
#include "sip_cfg.h"
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_voipnet.h"
#include "sip_sip.h"
#include "sip_debug.h"
#include "sip_clib.h"
#include "sip_mem.h"
#include "sip_dbase_sys.h"
#include "sip_hdrflds.h"
#include "sip_parser_enc.h"

#include "sip_xport.h"
#include "sip_xact.h"
#include "sip_tu.h"

/* 
 *****************************************************************************
 * ================TU_SendRequest()===================
 *
 * This function sends a request.  It will obtain an transport resource for
 * sending the request.  It will also allocate a transaciton for handling
 * the request transaction.
 *
 * pTargetUri = The target uri that the reqeust is destined for.
 *
 * pMsg = A pointer to an internal sip message object to send. 
 *
 * pfApp = A pointer to a function that is called when responses for the 
 *         transaction are received.  These are typically pointer to the 
 *         functions defined in UaClient.c.
 *
 * hOwner = A handle to "owner" of the request.  In this case it's always a 
 *          handle to a UA. unless it's an ACK then it's NULL.
 *
 * taskId = The task Identifier of the task that is handling the request 
 *          transaction.  This is typically the task Id of the UA belongs to.
 *
 * hTransport = A handle to a transport object.  If this is NOT-NULL, then 
 *              The SIP stack will use this transport resource to send the 
 *              request.  If NULL, then a new transport object (resource)
 *              is created.
 *
 * hTransaction = A pointer to a handle that will be populated with the handle
 *                to the transaction used to send this request.
 *
 * pLclConn = A pointer to a tLocalIpConn object that sepcifies the IP Stack 
 *           Interface to use when sending the request.  If this is NULL
 *           then the default IP Stack interface is used.  Typically this
 *           IP Stack interface is port 5060.
 *
 * RETURNS: 
 *      SIP_OK: Successfully send the request
 *      SIP_FAILED: Could not send the request
 *
 ******************************************************************************
 */
vint TU_SendRequest(
    tUri         *pTargetUri,
    tSipIntMsg   *pMsg,
    tpfAppCB      pfApp,
    tSipHandle    hOwner,
    uint32        taskId,
    tSipHandle    hTransport,
    tSipHandle   *hTransaction,
    tLocalIpConn *pLclConn)
{
    vint status = SIP_FAILED;
    tSipHandle hTrans = NULL;

    SIP_DebugLog(SIP_DB_TU_LVL_2, "TU_SendRequest: hOwner=%X, ptargetUri=%X",
            (int)hOwner, (int)pTargetUri, 0);

    if (!hTransport) {
        /* set up a transport instance */
        hTransport = TRANSPORT_ClientAlloc(pTargetUri, pLclConn,
                eTransportNone);
    }
    else {
        /* reusing an old transport so increase 
         * the numbers of users for this trasnport resource
         */
        TRANSPORT_AddUser(hTransport);
    }

    if (hTransport) {
        if (!hOwner) {
            /* send it to the transport layer */
            if ((status = TRANSPORT_Send(hTransport, pMsg)) == SIP_OK) {
                SIP_freeMsg(pMsg);
            }
            TRANSPORT_Dealloc(hTransport);    /* Immediately, and unconditionally, deallocate the transport resource */
        }
        else {
            tTransportType transport = TRANSPORT_GetTransportType(hTransport);
            if (transport != eTransportNone) {
                if (NULL == (hTrans = TRANS_ClientCreate(hTransport, TRANSPORT_Send, transport, pMsg, pfApp, hOwner, (tSipHandle)taskId))) {
                    SIP_DebugLog(SIP_DB_TU_LVL_1, "Could not create a Transaction for :%d", pMsg->method, 0, 0);
                }
                else {
                    status = TRANS_ClientReq(pMsg, hTrans);
                    //status = SIP_OK;
                }
            }
        }
    }
#if SIP_DEBUG_LOG
    if (status != SIP_OK)
        SIP_DebugLog(SIP_DB_TU_LVL_1, "TU_SendRequest: FAILED!  hTransaction:%X hOwner=%X, ptargetUri=%X", 
            (int)hTransport, (int)hOwner, (int)pTargetUri);
#endif
    if (hTransaction) {
        *hTransaction = hTrans;
    }
    return status;
}


/* 
 *****************************************************************************
 * ================TU_SendResponse()===================
 *
 * This function sends a response to a request (server transaction). 
 *
 * pMsg = A pointer to an internal sip object. This is the response to send
 *
 * hTrans = A handle to a Server transaction.
 *
 * RETURNS: 
 *      SIP_OK: Successfully send the response
 *      SIP_FAILED: Could not send the response
 *
 ******************************************************************************
 */
vint TU_SendResponse(
    tSipIntMsg  *pMsg,
    tSipHandle   hTransaction)
{
    vint status;
    status = TRANS_ServerResp(pMsg, hTransaction);
    if (status != SIP_OK) {
        SIP_DebugLog(SIP_DB_TU_LVL_1, "Could not send a response for :%d", pMsg->code, 0, 0);
    }
    return status;
}

