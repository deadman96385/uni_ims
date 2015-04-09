/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29962 $ $Date: 2014-11-20 12:36:51 +0800 (Thu, 20 Nov 2014) $
 */

#include "sip_sip.h"

#include "sip_timers.h"
#include "sip_msgcodes.h"
#include "sip_auth.h"
#include "sip_dbase_sys.h"
#include "sip_dbase_endpt.h"
#include "sip_parser_enc.h"
#include "sip_parser_dec.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "sip_tu.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_ua.h"
#include "sip_ua_client.h"
#include "sip_app.h"
#include "sip_ua_server.h"
#include "sip_mem_pool.h"

#include "_sip_helpers.h"
#include "_sip_fsm.h"

/* registry of method handlers for that the application layer uses */
static tpfUaServer      _UAS_handlers[eSIP_LAST_METHOD] = {0};
static tpfSipDispatcher _UAS_dispatcher = NULL;
static tpfSipProxy      _UAS_proxyServer = 0;
static tUri             _UAS_proxyUri; 
static tSipMatchType    _UAS_MatchType = SIP_REQUEST_MATCH_REQUEST_URI;

static vint _UAS_dispatch(
    tSipHandle  hUa,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction)
{
    tSipIpcMsg ipcMsg;
    tSipHandle hContext;
    
    /* build up the message for Inter Process Communication */
    ipcMsg.type = eSIP_IPC_SERVER_MSG;
    ipcMsg.hTransaction = hTransaction;
    ipcMsg.hOwner = hUa;
    ipcMsg.pMsg = pMsg;
    ipcMsg.id = TRANS_GetId(hTransaction);

    if (hUa != NULL) {
        hContext = (tSipHandle)((tUa*)hUa)->taskId;
    }
    else {
        hContext = TRANS_GetContext(hTransaction);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_3,
      "_UAS_dispatch: dispatching to hContext:%x, hTransaction %x owner:%x", 
      (int)hContext, (int)hTransaction, (int)hUa);

    if ((*_UAS_dispatcher)(hContext, &ipcMsg) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "_UAS_dispatch: Could not dispatch msg to hContext:%x, hTransaction:%x owner:%x", 
            (int)hContext, (int)hTransaction, (int)hUa);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

static vint _UAS_failure(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    SIP_DebugLog(SIP_DB_UA_LVL_1,
            "_UAS_failure: Error MSG has internal error code", 0, 0, 0);

    if (event == eTU_REQUEST) {
        /* return a failure */
        if (pMsg->internalErrorCode != 0) {
            MSGCODE_Create(pMsg, NULL, pMsg->internalErrorCode);
            SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
            return TU_SendResponse(pMsg, hTransaction);
        }
    }
    else if (event == eTU_DEAD) {
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
    }    
    else {
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

static vint _UAS_dontExist(
    tSipHandle  hOwner, 
    uint32         event, 
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    SIP_DebugLog(SIP_DB_UA_LVL_2,
            "_UAS_dontExist: Got a request that belongs to no one",
            0, 0, 0);

    if (event == eTU_REQUEST) {
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_NOT_FOUND);
        return TU_SendResponse(pMsg, hTransaction);       
    }
    else if (event == eTU_DEAD) {
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
    }    
    else {
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

static vint _UAS_send2Method(
    tSipHandle  hOwner, 
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction)
{
    tSipMethod method;
    tpfUaServer pfMethod = NULL;
    vint status;

    /* in this case the owner is a UA */

    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UAS_send2Method: hOwner:%x hTransaction:%x",
            (int)hOwner, (int)hTransaction, 0);

    method = pMsg->method;
    if (method < eSIP_LAST_METHOD) {
        pfMethod = _UAS_handlers[method];
        if (pfMethod) {
            status = pfMethod(hOwner /*or UA */, pMsg, hTransaction);
        }
        else {
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UAS_send2Method: ERROR no method handler defined -method:%d",
                (int)method, 0, 0);
            return (SIP_NOT_SUPPORTED);
        }
    }
    else {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UAS_send2Method: Method not supported -method:%d",
                (int)method, 0, 0);
        return (SIP_NOT_SUPPORTED);
    }
    return (status);
}

static tSipHandle _UAS_buildResources(
    tSipHandle      hContext,
    tSipHandle      hUa,
    tSipIntMsg     *pMsg,
    tpfAppCB        pfHandler,
    tTransportType  transType,
    tLocalIpConn   *pLclConn,
    tRemoteIpConn  *pRmtConn)
{
    tSipHandle hTransport;
    tSipHandle hTransaction;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UAS_buildResources: Making resources hUa:%x for hContext:%x", 
            (int)hUa, (int)hContext, 0);

    if (transType == eTransportUdp || transType == eTransportTcp ||
             transType == eTransportTls) {
        /* allocate a new server transport resource*/
        hTransport = TRANSPORT_ServerAlloc(pLclConn, pRmtConn, transType);
    }
    else {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UAS_buildResources: Failed transport not supported!",
                0, 0, 0);
        return (NULL);
    }

    if (!hTransport) {
        /* for TCP the hTransport should have already been set */
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UAS_buildResources: no transport resource available!",
                0, 0, 0);
        return (NULL);
    }

    /* create a new server transaction */
    hTransaction = TRANS_ServerCreate(hTransport, TRANSPORT_Send, transType, 
            pMsg, pfHandler, hUa, hContext);
    if (!hTransaction) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UAS_buildResources: could not create new transaction",
                0, 0, 0);
        TRANSPORT_Dealloc(hTransport);
        return (NULL);
    }

    return hTransaction;
}


/* 
 *****************************************************************************
 * ================UA_Server()===================
 *
 * This is the main part (or entry) into the UAS.
 * It's called when a request is recieved and matched to a current transaction
 * or a new transaction has been created to handle it.
 * This function will dispatch the message to the thread that owns the UA
 * the the request was destined for. The remainder of the UAS is handled
 * on the UA specific thread or task.
 *
 * hOwner  = UNUSED
 *
 * event = The event fromt he TU (Transaction User). Possible values are
 *      eTU_FAILED,
 *      eTU_REQUEST,
 *      eTU_RESPONSE,
 *      eTU_DEAD,
 *
 * pMsg = a pointer a sip message object, this is the internal response.
 *
 * hTransaction = A handle to the client side transaction that "owns" this 
 *                response.
 *
 * hTransport =   A handle tot he trasnport resource that was used to recieve 
 *                this response
 *
 * RETURNS: 
 *        SIP_OK:       The function properly passed back the incoming request
 *        SIP_FREE_MEM: There was an error and the pMsg should be "freed"
 *        SIP_FAILED:   Could not pass back the requset through the UAS. 
 ******************************************************************************
 */
static vint _UA_server(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    vint status;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UA_server: hTransaction:%x hOwner:%x pMsg:%x",
            (int)hTransaction, (int)hOwner, (int)pMsg);

    switch (event) {
    case eTU_REQUEST:
        /* send it to the appropriate handler */
        if ((status = _UAS_send2Method(hOwner, pMsg, hTransaction)) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
        break;
    case eTU_RESPONSE: 
        /* 
         * Then there is a serious issue, the server should never receive a 
         * response. 
         */
        SIP_freeMsg(pMsg);
        SIP_DebugLog(SIP_DB_UA_LVL_1, "_UA_server: ERROR - eTU_RESPONSE event",
                0, 0, 0);
        status = SIP_NOT_SUPPORTED;
        break;
    case eTU_DEAD: 
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        SIP_DebugLog(SIP_DB_UA_LVL_2,
                "_UA_server: eTU_DEAD hTransaction:%x hOwner:%x pMsg:%x",
                (int)hTransaction, (int)hOwner, (int)pMsg);
        status = SIP_OK;
        break;
    default:
        SIP_DebugLog(SIP_DB_UA_LVL_1, "_UA_server: ERROR - bad event:%d",
                event, 0, 0);
        if (pMsg) {
            /* pMsg should NEVER be NULL */
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "_UA_server: ERROR hTransaction:%x hOwner:%x pMsg:%x",
                    (int)hTransaction, (int)hOwner, (int)pMsg);
            SIP_freeMsg(pMsg);
        }
        status = SIP_NOT_SUPPORTED;
        break;
    } /* end of switch */

    return status;
}

/* 
 *****************************************************************************
 * ================_UAS_proxyValidiateReq()===================
 * 
 * This function validates the request by examining the "MaxForwards" scheme
 * and the uri scheme in the requestURI.  This is described in section 16
 * of RFC3261.
 *
 * pMsg = A pointer to the request
 * 
 * RETURNS: 
 *    SIP_OK:  The request passed the "validation" test. 
 *    SIP_FAILED: The request failed the validation.  Either the URI 
 *                scheme in the requestURI or the value of the MaxForwards
 *                header field was not valid. 
 ******************************************************************************
 */
static vint _UAS_proxyValidiateReq(tSipIntMsg *pMsg)
{
    tHdrFldList *pHf;
    char        *pNum;
    
    if (pMsg->requestUri.scheme == eURI_SCHEME_SIPS) {
        /* return a 416 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_UNSUPP_URI_SCHEME);
        return (SIP_FAILED);
    }
    /* Find max forwards */
    if (NULL != (pHf = 
            HF_Find(&pMsg->pHFList, eSIP_MAX_FORWARDS_HF))) {
        if (OSAL_strtoul(pHf->pField, &pNum, 10) == 0) {
            /* return a 483 */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_TOO_MANY_HOPS);
            return (SIP_FAILED);
        }
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================_UAS_proxyReqForward()===================
 * 
 * This function prepares the SIP request to be sent to the next hop.
 * A 'Via' header field entry is added and the maxforwards value is 
 * decremented. This is described in section 16 of RFC3261.
 *
 * pMsg = A pointer to the request
 * 
 * RETURNS: 
 *    SIP_OK:  The request was successfully prepared. 
 *    SIP_FAILED: The request can not be forwarded becuase the stack 
 *                could not add a 'Via' header field. 
 ******************************************************************************
 */
static vint _UAS_proxyReqForward(tSipIntMsg *pMsg)
{
    tHdrFldList  *pHf;
    char         *pNum;
    tViaHFE      *pVia;
    tDLListEntry *pEntry;
    uint32        maxForwards;
    tIPAddr       addr;
    
    /* Find max forwards */
    if (NULL != (pHf = 
            HF_Find(&pMsg->pHFList, eSIP_MAX_FORWARDS_HF))) {
        maxForwards = OSAL_strtoul(pHf->pField, &pNum, 10);
        if (maxForwards) {
            /* decrement */
            maxForwards--;
            /* rewrite it */
            OSAL_itoa(maxForwards, pHf->pField, OSAL_strlen(pHf->pField));
        }
    }
    else {
        /* doesn't exist so add it */
        HF_CopyInsert(&pMsg->pHFList, eSIP_MAX_FORWARDS_HF,
                SYSDB_MAX_FORWARDS_DFLT, 0);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
    }
    /* Set up the 'Via'. 
     * Get the first via because we need it's branch 
     */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        pVia = (tViaHFE*)pEntry;
        /* Add a VIA */
        OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
        addr.v4.ul = 0;
        if (HF_MakeVia(&pMsg->ViaList, &_UAS_proxyUri,
                pVia->szBranch, addr, OSAL_FALSE) == SIP_OK) {
            DLLIST_Reverse(&pMsg->ViaList);
            /* Get the new Via */
            pEntry = NULL;
            DLLIST_GetNext(&pMsg->ViaList, &pEntry);
            pVia = (tViaHFE*)pEntry;
            /* Now we have to add a unique identifier 
             * per RFC3261 section 16.11 */
            HF_CatViaBranch(pVia->szBranch);
            return (SIP_OK);
        }
    }
    return (SIP_FAILED);
}

static void _UAS_proxyWriteReqUri(
    tSipIntMsg *pMsg,
    tUri       *pTargetUri)
{
    pMsg->requestUri = *pTargetUri;
    return;
}

/* 
 *****************************************************************************
 * ================_UAS_proxy()===================
 * 
 * This function is used to proxy forward a request.  If the _UAS_proxyServer
 * static variable is TRUE then this function will fire the callback to 
 * determine if the SIP application wishes that the request be 
 * "proxy forwarded".  So if _UAS_proxyServer returns TRUE, then the SIP
 * stack will act like a stateless proxy and forward the request to the
 * next hop.
 *
 * transType = The transport type that this request was recevied on 
 *             (i.e. UDP or TCP).
 *
 * pLclConn = A pointer to a tLocalIpConn object that describes the interface
 *            that the request was received on.
 *
 * pRmtConn = A pointer to a tRemoteIpConn object that describes the IP stack
 *            interface of the peer that sent the request.
 *
 * pMsg = A pointer to the decoded request in question of "proxy forwarding"
 * 
 * RETURNS: 
 *         SIP_OK:  The request was successfully handled by the proxy 
 *                  mechanism.  If this function returns SIP_OK.  Then 
 *                  the stack should make no more attempts to process
 *                  the request pointed to by pMsg.
 *                 
 *         SIP_FAILED: The proxy mechanism did not (should not) process the 
 *                     request.  Therefore, if this function returns 
 *                    SIP_FAILED, then the stack shold continue to process the 
 *                    request as per normal.
 ******************************************************************************
 */
static vint _UAS_proxy(
    tTransportType transType,
    tLocalIpConn  *pLclConn,
    tRemoteIpConn *pRmtConn,
    tSipIntMsg    *pMsg)
{
    uint32        len;
    vint          isReg;
    tUri          targetUri;
    char         *pFqdn;
    char          aor[SIP_URI_STRING_MAX_SIZE];
    
    /* perform all basic tests for max forwards, uri scheme etc...*/
    if (_UAS_proxyValidiateReq(pMsg) != SIP_OK) {
        /* Then return the response */
        TRANSPORT_ProxyError(transType, pLclConn, pRmtConn, pMsg);
        return (SIP_OK);
    }
    
    /* encode the aor to string format for the application */
    len = SIP_URI_STRING_MAX_SIZE;
    if (ENC_Uri(&pMsg->To.uri, aor, &len, 0) == SIP_OK) {
        aor[len] = 0;
        pFqdn = NULL;
        isReg = ((pMsg->method == eSIP_REGISTER) ? 1 : 0);
        if (_UAS_proxyServer(isReg, aor, &pFqdn) == SIP_OK) {
            /* If the SIP application says "SIP_OK", then that 
             * means to proxy the request forward */
            if (pFqdn != NULL) {
                HF_CleanUri(&targetUri);
                if (DEC_Uri(pFqdn, OSAL_strlen(pFqdn), &targetUri) == SIP_OK) {
                    /* Now send it */
                    if (_UAS_proxyReqForward(pMsg) != SIP_OK) {
                        MSGCODE_Create(pMsg, NULL, eSIP_RSP_SERVER_INT_ERR);
                        TRANSPORT_ProxyError(transType, pLclConn, pRmtConn,
                                pMsg);
                    }
                    else {
                        _UAS_proxyWriteReqUri(pMsg, &targetUri);
                        if (TRANSPORT_ProxyMsg(pLclConn, pMsg, &targetUri) !=
                                SIP_OK) {
                            MSGCODE_Create(pMsg, NULL, eSIP_RSP_AMBIGUOUS);
                            TRANSPORT_ProxyError(transType, pLclConn, pRmtConn,
                                    pMsg);
                        }
                    }
                    return (SIP_OK);
                }
            }
        }
    }
    else {
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_AMBIGUOUS);
        TRANSPORT_ProxyError(transType, pLclConn, pRmtConn, pMsg);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

/* 
 *****************************************************************************
 * ================UAS_Entry()===================
 *
 * This is the entry point to the UA's Server side behavior.
 * Once a SIP event (i.e. new sip message or timer event) is dispatched to 
 * the appropriate thread
 *
 * hOwner  = The owner of the event, could be a handle to a dialog or NULL
 *
 * pMsg = a pointer a sip message object, This is NULL for timer events.
 *
 * hTransaction = A handle to the server side transaction .
 *
 * id = the id paramtere from the timer event used to determine if the timer 
 *      event is 'old' or 'stale'
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UAS_Entry(
    tSipHandle  hOwner,    
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction,
    uint32      id)
{
    /* 
     * In the case of the server side, the hOwner is a handle to the UA 
     * NOTE: for clients the hOwner is a dialog 
     */
    if (hTransaction) {
        /* then we go into the transaction FSM */
        if (TRANS_CheckId(hTransaction, id) == TRUE) {
            
            SIP_DebugLog(SIP_DB_UA_LVL_3,
              "UAS_Entry: sending pMsg:%x into hTransaction:%x FSM with id:%d", 
              (int)pMsg, (int)hTransaction, (int)id);
            
            if (TRANS_ServerReq(pMsg, hTransaction) != SIP_OK) {
                SIP_freeMsg(pMsg);
            }
        }
        else {
            SIP_DebugLog(SIP_DB_UA_LVL_2,
                    "UAS_Entry: received old MSG for hTransaction:%x id:%d", 
                    (int)hTransaction, (int)id, 0);
            SIP_freeMsg(pMsg);
        }
    }
    else if (hOwner) {
        /* 
         * Then there is no transaction associated with this message so send 
         * it to _UA_server to resolve it. Note, this handles ACKs to INVITEs.
         */
        SIP_DebugLog(SIP_DB_UA_LVL_3,
                "UAS_Entry: we have no transaction sending pMsg:%x to UA:%x", 
                (int)pMsg, (int)hOwner, 0);
        
        _UA_server(hOwner, eTU_REQUEST, pMsg, hTransaction);
    }
    else {
        /* we should never be in here */
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UAS_Entry: ERROR no dialog or transaction associated with event",
            0, 0, 0);
        SIP_freeMsg(pMsg);
    }
    return;
}

/* 
 *****************************************************************************
 * ================UAS_Invite()===================
 *
 * This function is called when an INVITE request is received.
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FAILED:   The error response could not be returned
 *         SIP_OK:       An error response was successfully sent.
 ******************************************************************************
 */
vint UAS_Invite(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog  *pDialog;
    tReplacesHF *pReplaces;
    tDLList     *pCredList;
    tEPDB_Entry *pEntry;
    vint status;

    /* in this case the hOwner is the UA object */
    tUa *pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Invite: hTransaction:%x",
            (int)hTransaction, 0, 0);

    /* search the list of current dialogs for the endpoint */
    pDialog = UA_DialogSearch(pUa, pMsg->msgType, pMsg->method, 
                  pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId);
    
    if (pDialog == NULL) {
        /* 
         * If we couldn't find the dialog and there's a 
         * populated 'to' tag then pMsg is probably misrouted!
        */
        if (pMsg->To.szTag[0] != 0) {
            /* return a 481 error here */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
            return TU_SendResponse(pMsg, hTransaction);
        }
        
        /* 
         * According to RFC 3891, responds 481 if there is replaces header field but
         * we cannot find the dialog to replace.
         */
        if (HF_PresenceExists(&pMsg->x.ECPresenceMasks, eSIP_REPLACES_HF)) {
            pReplaces = &pMsg->Replaces;
            if (NULL == UA_DialogSearch(pUa, pMsg->msgType,
                    pMsg->method, pReplaces->szToTag, pReplaces->szFromTag,
                    pReplaces->szCallId)) {
                /* return a 481 error here */
                MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
                return TU_SendResponse(pMsg, hTransaction);
            }
        }

        /* create a new dialog */
        pDialog = UA_DialogInit(pUa);
        if (!pDialog) {
            /* serious issue, send back a error */
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UAS_Invite: Error creating new dialog hTransaction:%x)",
                    (int)hTransaction, 0, 0);
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_SERVER_INT_ERR);
            return TU_SendResponse(pMsg, hTransaction);
        }
        else {
            SIP_DebugLog(SIP_DB_UA_LVL_3,
                    "UAS_Invite: Initializing new dialog hTransaction:%x)",
                    (int)hTransaction, 0, 0);
        
            pCredList = NULL;
            if (NULL != (pEntry = EPDB_Get(eEPDB_CREDENTIALS, pUa->epdb))) {
                pCredList = &pEntry->x.dll;
            }

            /* initialize the new dialog */
            DIALOG_InitServer(pUa, (tSipHandle)pUa->taskId, pDialog, pMsg, pCredList);
            /* Fall through and bind (overwrite?) transaction */
            goto bindDialog;
        }
    }
    else {
        SIP_DebugLog(SIP_DB_UA_LVL_3,
                "UAS_Invite: found existing dialog...binding hTransaction:%x",
                (int)hTransaction, 0, 0);
        /* Fall through and bind (overwrite?) transaction */
        goto bindDialog;
    }
        
bindDialog:
    /* bind the trans to the dialog */
    pDialog->hTrans = hTransaction;
    /* Now drive through the S.M. */
    status = UASM_DialogServer(pDialog, pMsg, hTransaction);

    return (status);
}

/* 
 *****************************************************************************
 * ================UAS_Update()===================
 *
 * This function is called when an UPDATE request is received.
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FAILED:   The error response could not be returned
 *         SIP_OK:       An error response was successfully sent.
 ******************************************************************************
 */
vint UAS_Update(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;

    /* in this case the hOwner is the UA object */
    tUa *pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Update: hTransaction:%x",
            (int)hTransaction, 0, 0);

    /* search the list of current dialogs for the endpoint */
    pDialog = UA_DialogSearch(pUa, pMsg->msgType, pMsg->method, 
                  pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId);
    
    if (pDialog == NULL) {
        /* return a 481 error here */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        return TU_SendResponse(pMsg, hTransaction);
    }
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UAS_Update: found existing dialog...binding hTransaction:%x",
            (int)hTransaction, 0, 0);
    
    /* bind the trans to the dialog */
    pDialog->hUpdateTrans = hTransaction;
    /* Now drive through the S.M. */
    return (UASM_DialogServer(pDialog, pMsg, hTransaction));
}

/* 
 *****************************************************************************
 * ================UAS_Cancel()===================
 *
 * This function is called when a CANCEL request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed"
 *         SIP_FAILED: There was a problem passing the CANCEL request.
 *         SIP_OK: Function was successfull
 ******************************************************************************
 */
vint UAS_Cancel(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Cancel: ", 0, 0, 0);

    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch((tUa*)hOwner, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
        /* Then return a 481 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        return TU_SendResponse(pMsg, hTransaction);
    }
    
    /* bind the trans to the dialog */
    pDialog->hTrans = hTransaction;
    /* then pDialog should be the dialog found */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Prack()===================
 *
 * This function is called when a Prack request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed"
 *         SIP_FAILED: There was a problem passing the PRACK request.
 *         SIP_OK: Function was successfull
 ******************************************************************************
 */
vint UAS_Prack(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Prack: ", 0, 0, 0);

    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch((tUa*)hOwner, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
        /* Then return a 481 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        return TU_SendResponse(pMsg, hTransaction);
    }
    
    /* bind the trans to the dialog */
    pDialog->hPrackTrans = hTransaction;
    /* then pDialog should be the dialog found */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Bye()===================
 *
 * This function is called when a BYE request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed"
 *         SIP_FAILED: There was a problem passing the BYE request.
 *         SIP_OK: Function was successfull
 ******************************************************************************
 */
vint UAS_Bye(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;
    
    /* in this case the hOwner is the UA object */

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Bye: ", 0, 0, 0);
    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch((tUa*)hOwner, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
         /* Then return a 481 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        return TU_SendResponse(pMsg, hTransaction);
    }

    /* bind the trans to the dialog */
    pDialog->hTrans = hTransaction;
    /* then pDialog should be the dialog found */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Ack()===================
 *
 * This function is called when an ACK request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed".
 *         SIP_FAILED:   There was a problem passing the ACK request.
 *         SIP_OK:       Function was successfull.
 ******************************************************************************
 */
vint UAS_Ack(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;
    
    /* in this case the hOwner is the UA object */
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Ack: hOwner:%x", (int)hOwner, 0, 0);

    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch((tUa*)hOwner, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
        return (SIP_FAILED);
    }

    /* bind the trans to the dialog */
    pDialog->hTrans = hTransaction;
    /* then pDialog should be the dialog found */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}


/* 
 *****************************************************************************
 * ================UAS_Notify()===================
 *
 * This function is called when an NOTIFY request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed".
 *         SIP_FAILED:   There was a problem passing the NOTIFY request
 *                       through the dialog state machine.
 *         SIP_OK:       Function was successfull.
 ******************************************************************************
 */
vint UAS_Notify(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;
    
    /* in this case the hOwner is the UA object */
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Notify: hOwner:%x", (int)hOwner, 0, 0);

    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch((tUa*)hOwner, pMsg->msgType, 
            pMsg->method, pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
        
        /* clear out the message body before returning the response */
#ifdef SIP_UAS_ALLOW_BLIND_NOTIFY
        UA_AppEvent(hOwner, NULL, eUA_NOTIFY_EVENT, pMsg, NULL);
        
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        pMsg->ContentType = eCONTENT_TYPE_NONE;
        pMsg->sipfragCode = (tSipMsgCodes)0;
        pMsg->ContentLength = 0;
#else
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        pMsg->ContentType = eCONTENT_TYPE_NONE;
        pMsg->sipfragCode = (tSipMsgCodes)0;
        pMsg->ContentLength = 0;
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
#endif
        return TU_SendResponse(pMsg, hTransaction);
    }
    else {
        /* send it to the dialog state machine */
        return UASM_DialogServer(pDialog, pMsg, hTransaction);
    }
}

/* 
 *****************************************************************************
 * ================UAS_Info()===================
 *
 * This function is called when an INFO request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed".
 *         SIP_FAILED:   There was a problem passing the INFO request through 
 *                       the dialog state machine.
 *         SIP_OK:       Function was successfull.
 ******************************************************************************
 */
vint UAS_Info(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;
    
    /* in this case the hOwner is the UA object */

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Info: hOwner:%x", (int)hOwner, 0, 0);
    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch((tUa*)hOwner, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
         /* Then return a 481 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        /* get rid of content type and message body */
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        pMsg->ContentType = eCONTENT_TYPE_NONE;
        pMsg->ContentLength = 0;
                
        return TU_SendResponse(pMsg, hTransaction);
    }

    /* then pDialog should be the dialog found */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Subscribe()===================
 *
 * This function is called when an SUBSCRIBE request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed".
 *         SIP_FAILED:   There was a problem passing the ACK request.
 *         SIP_OK:       Function was successfull.
 ******************************************************************************
 */
vint UAS_Subscribe(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog  *pDialog;
    tDLList     *pCredList;
    tEPDB_Entry *pEntry;
    vint         status;

    /* in this case the hOwner is the UA object */
    tUa *pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Subscribe: hTransaction:%x",
            (int)hTransaction, 0, 0);

    /* search the list of current dialogs for the endpoint */
    pDialog = UA_DialogSearch(pUa, pMsg->msgType, pMsg->method, 
        pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId);
    
    if (pDialog == NULL) {
        /* 
         * If we couldn't find the dialog and there's a 
         * populated 'to' tag then pMsg is probably misrouted!
        */
        if (pMsg->To.szTag[0] != 0) {
            /* return a 481 error here */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
            return TU_SendResponse(pMsg, hTransaction);
        }
        
        /* create a new dialog */
        pDialog = UA_DialogInit(pUa);
        if (!pDialog) {
            /* serious issue, send back a error */
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UAS_Subscribe: Error creating new dialog hTransaction:%x",
                    (int)hTransaction, 0, 0);
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_SERVER_INT_ERR);
            return TU_SendResponse(pMsg, hTransaction);
        }
        else {
            SIP_DebugLog(SIP_DB_UA_LVL_3,
                    "UAS_Subscribe: Initializing new dialog hTransaction:%x",
                    (int)hTransaction, 0, 0);
        
            pCredList = NULL;
            if (NULL != (pEntry = EPDB_Get(eEPDB_CREDENTIALS, pUa->epdb))) {
                pCredList = &pEntry->x.dll;
            }

            /* initialize the new dialog */
            DIALOG_InitServer(pUa, (tSipHandle)pUa->taskId, pDialog, pMsg, pCredList);
            /* Fall through and bind (overwrite?) transaction */
            goto bindDialog;
        }
    }
    
bindDialog:
    /* bind the trans to the dialog */
    pDialog->hTrans = hTransaction;
    /* Now drive through the S.M. */
    status = UASM_DialogServer(pDialog, pMsg, hTransaction);

    return (status);
}


/* 
 *****************************************************************************
 * ================UAS_Message()===================
 *
 * This function is called when an MESSAGE request is received.
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed".
 *         SIP_FAILED:   There was a problem passing the ACK request.
 *         SIP_OK:       Function was successfull.
 ******************************************************************************
 */
vint UAS_Message(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog   *pDialog;
    tDLListEntry *pDLLEntry;
    tContactHFE  *pContact;
    
    /* in this case the hOwner is a UA object */
    tUa *pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Message: hOwner:%x", (int)hOwner, 0, 0);

    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch(pUa, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
        /* MESSAGEs are typically sent outside the context of a dialog.
         * So tell the application and return a 200.
         */
        /* MESSAGE Evt */
        UA_AppEvent(pUa, NULL, eUA_TEXT_MESSAGE, pMsg, NULL);
        
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
        /* get rid of content type and message body */
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        pMsg->ContentType = eCONTENT_TYPE_NONE;
        pMsg->ContentLength = 0;
        /* Add tag value on To header. */
        HF_GenerateTag(pMsg->To.szTag);        
        

#ifdef SIP_CUSTOM_CAPABILITY_EXCHANGE
        if (NULL != pMsg->pMsgBody && 0 == OSAL_strncmp(pMsg->pMsgBody->msg,
                SIP_CUSTOM_CAPABILITY_EXCHANGE, sizeof(SIP_CUSTOM_CAPABILITY_EXCHANGE)-1)) {
            /* clear out Contact list before filling it with our own info */
            DLLIST_Empty(&pMsg->ContactList, eSIP_OBJECT_CONTACT_HF);

            /* Set up the contact */
            UA_PopulateContact(pUa, pMsg);

            /* Add the capabilities */
            if (0 != pUa->capabilitiesBitmap) {
                pDLLEntry = NULL;
                while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
                    pContact = (tContactHFE*)pDLLEntry;
                    pContact->capabilitiesBitmap = pUa->capabilitiesBitmap;
                }
            }
        }
#endif
        return TU_SendResponse(pMsg, hTransaction);
    }
    else {
        /* send it to the dialog state machine */
        return UASM_DialogServer(pDialog, pMsg, hTransaction);
    }
}

/* 
 *****************************************************************************
 * ================UAS_Publish()===================
 *
 * This function is called when a PUBLISH request is received.
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed".
 *         SIP_FAILED:   There was a problem passing the ACK request.
 *         SIP_OK:       Function was successfull.
 ******************************************************************************
 */
vint UAS_Publish(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* in this case the hOwner is a UA object */
    tUa *pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Publish: hOwner:%x", (int)hOwner, 0, 0);

    /* PUBLISH requests are typically sent outside the context of a dialog.
     * So tell the application and return a 200.
     */
    UA_AppEvent(pUa, NULL, eUA_PUBLISH, pMsg, NULL);
    
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    /* get rid of content type and message body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->ContentLength = 0;
    
    return TU_SendResponse(pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Refer()===================
 *
 * This function is called when a REFER request is received.
 *
 * hOwner = A handle to a tUa (User Agent).
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed"
 *         SIP_FAILED: There was a problem passing the BYE request.
 *         SIP_OK: Function was successfull
 ******************************************************************************
 */
vint UAS_Refer(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;
    
    /* in this case the hOwner is the UA object */
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Refer: ", 0, 0, 0);
    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = UA_DialogSearch((tUa*)hOwner, pMsg->msgType, 
            pMsg->method, pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {
         /* Then return a 481 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        return TU_SendResponse(pMsg, hTransaction);
    }

    /* then pDialog should be the dialog found */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Register()===================
 *
 * This function is called when an REGISTER request is received.
 * A UA shouldn't handle register requests, so an error response is always
 * returned.
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FAILED:   The error response could not be returned
 *         SIP_OK:       An error response was successfully sent, or
 *                       the SIP message was past to the application.
 ******************************************************************************
 */
vint UAS_Register(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* in this case the hOwner is a UA object */
    tUa *pUa = (tUa*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_RegisterServer: ", 0, 0, 0);
    /* REGISTERs are sent outside the context of a dialog. */
    if (_UAS_proxyServer) {
        /* Then we are acting like a proxy so notify the app */
        
        /* cache the REGISTER message */
        if (pUa->ProxyReg.pMsg == NULL) {
            pUa->ProxyReg.pMsg = pMsg;
            pUa->ProxyReg.hTransaction = hTransaction;
            UA_AppEvent(pUa, NULL, eUA_REGISTRATION, pMsg, NULL);
            return (SIP_OK);
        }
        else {
            /* Then we are busy so return an error */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_BUSY_EVERYWHERE);
        }
    }
    else {
        /* Then we are a UA just return a 200OK */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    }
    return TU_SendResponse(pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Options()===================
 *
 * This function is called when a OPTIONS request is received
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * RETURNS: 
 *         SIP_FREE_MEM: The callback was unsuccessful and the pMsg should 
 *                       be "freed"
 *         SIP_FAILED: There was a problem passing the BYE request.
 *         SIP_OK: Function was successfull
 ******************************************************************************
 */
vint UAS_Options(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog;
    uint32 capabilitiesBitmap;
    tDLListEntry   *pDLLEntry;
    tContactHFE    *pContact;
   
    /* in this case the hOwner is the UA object */
    tUa *pUa = (tUa*)hOwner;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Options: ", 0, 0, 0);
    /* search the list of current dialogs for the endpoint */
    if (NULL == (pDialog = 
            UA_DialogSearch(pUa, pMsg->msgType, pMsg->method, 
            pMsg->To.szTag, pMsg->From.szTag, pMsg->szCallId))) {

        /*
         * notify application that OPTIONS have arrived so that any
         * capabilities present may be processed
         */
        UA_AppEvent(hOwner, NULL, eUA_OPTIONS, pMsg, NULL);

        /* 
         * If there an available dialog then return a 200 otherwise return a 
         * 486 busy here this is from section 11 of RFC3261.
         */
        if (UA_DialogInit(pUa) == NULL) {
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_BUSY_HERE);
        }
        else {
            /* clear out Contact list before filling it with our own info */
            DLLIST_Empty(&pMsg->ContactList, eSIP_OBJECT_CONTACT_HF);

            /* Set up the contact */
            UA_PopulateContact(pUa, pMsg);

            capabilitiesBitmap = pUa->capabilitiesBitmap;

            /* Add the capabilities */
            if (0 != capabilitiesBitmap) {
                pDLLEntry = NULL;
                while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
                    pContact = (tContactHFE*)pDLLEntry;
                    pContact->capabilitiesBitmap = capabilitiesBitmap;
                }
            }


            /* load these per section 11 of RFC3261 */
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_ENCODING_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_LANGUAGE_HF);
            SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
        }
        return TU_SendResponse(pMsg, hTransaction);
    }

    /* then OPTIONS request was received inside dialog (pDialog) */
    return UASM_DialogServer(pDialog, pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_Error()===================
 *
 * This function is called when an unsupported request is received.
 *
 * hOwner = A handle to a tUa (User Agent)
 *
 * pMsg = A pointer to the internal sip message.
 *
 * hTransaction = A handle to the server transaction that is handling this
 *                request transaction.
 *
 * hTransport = A handle to the transport resource that it handling the 
 *              transaction.
 *
 * RETURNS: 
 *         SIP_FAILED:   The error response could not be returned
 *         SIP_OK:       An error response was successfully sent.
 ******************************************************************************
 */
vint UAS_Error(
    tSipHandle   hOwner, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* in this case the hOwner is the UA object */
    UNUSED(hOwner);
   
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_Error: ", 0, 0, 0);

    MSGCODE_Create(pMsg, NULL, eSIP_RSP_METHOD_NOT_ALLOWED);
    SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
    return TU_SendResponse(pMsg, hTransaction);
}

/* 
 *****************************************************************************
 * ================UAS_TrafficCB()===================
 *
 * This is the function that was registered with the transport module to pass 
 * incoming traffic destined for the UAS back through the sip 
 * stack. In otherwords, when traffic is received by the transport module, the 
 * next train stop is here! It attempts to match the sip message to a current
 * transaction or creates a new one (unless it's an ACK).
 *
 * hTransport = A handle to the transport resource that was used to recieve 
 *             this response.
 *
 * hOwner  = The owner of the transport resource, in this case it's nobody
 *
 * conn = An object popualted with ip address \ port and transport type 
 *        (i.e. udp, tcp, etc...).  See transport.h for the definition of this
 *        object
 *
 * pMsg = A pointer to the sip message (the response).
 *
 *
 * RETURNS: 
 *         SIP_OK: The message was successfully send back through the UAC
 *         SIP_FAILED: There was an error passing the message back.
 ******************************************************************************
 */
vint UAS_TrafficCB(
    tTransportType   transType,
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn,
    tSipIntMsg      *pMsg)
{
    tSipHandle hTransaction;
    tSipHandle hContext;
    tUa       *pUa;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAS_TrafficCB:", 0, 0, 0);

    if (pMsg->msgType == eSIP_REQUEST) {
        hTransaction = TRANS_ServerSearch(pMsg);
        /* try to find it in the transaction module */
        if (!hTransaction) {
            if (_UAS_proxyServer) {
                if (_UAS_proxy(transType, pLclConn, pRmtConn, pMsg) == SIP_OK) {
                    /* Then the request was handled by the "proxy" handler.
                     * We always return FAILED here so pMsg will be 'freed'
                     */
                    return (SIP_FAILED);
                }
            }
            /* Else, process as per normal.
             * Check and see if the endpoint exists 
             */ 
            switch (_UAS_MatchType) {
            case SIP_REQUEST_MATCH_REQUEST_URI:
                pUa = UA_Search(&pMsg->requestUri);
                break;
            case SIP_REQUEST_MATCH_FROM_AOR:
                pUa = UA_Search(&pMsg->From.uri);
                break;
            case SIP_REQUEST_MATCH_ANY:
                pUa = UA_Search(NULL);
                break;
            default:
                pUa = UA_Search(&pMsg->requestUri);
                break;
            }
            if (!pUa) {
                SIP_DebugLog(SIP_DB_UA_LVL_2,
                        "UAS_TrafficCB: Warning Could not find a UA",  0, 0, 0);
                hTransaction = _UAS_buildResources(NULL, NULL, pMsg,
                        _UAS_dontExist, transType, pLclConn, pRmtConn);
                if (hTransaction) {
                    return TRANS_ServerReq(pMsg, hTransaction);
                }
            }
            else {
                /* we found a ua */
                if (pMsg->method == eSIP_ACK) {
                     SIP_DebugLog(SIP_DB_UA_LVL_3,
                             "UAS_TrafficCB: dispatching an ACK to hUa:%x",
                             (int)pUa, 0, 0);
                    /* ACK's without a transaction are just past back to 
                     * the UA for handling as per RFC */
                    return _UAS_dispatch(pUa, pMsg, NULL);
                }
                else {
                    hContext = (tSipHandle)pUa->taskId;
                    if (SYSDB_ExamineRequest(pMsg) != SIP_OK) {
                        hTransaction = _UAS_buildResources(hContext, pUa,
                                pMsg, _UAS_failure, transType, pLclConn,
                                pRmtConn);
                        if (hTransaction) {
                            return TRANS_ServerReq(pMsg, hTransaction);
                        }
                    }
                    else {
                        hTransaction = _UAS_buildResources(hContext, pUa,
                                pMsg, _UA_server, transType, pLclConn,
                                pRmtConn);
                        if (hTransaction) {
                            /* all is well so distribute */
                            return _UAS_dispatch(pUa, pMsg, hTransaction);
                        }
                    }
                }    
            }
            /* there was a problem getting a transaction */
            return (SIP_FAILED);
        }
        else {
            /* It looks like there is a transaction already in existance */
            return _UAS_dispatch(NULL, pMsg, hTransaction);
        }
    }
    else {
        return (SIP_FAILED);
    }
}

/* 
 *****************************************************************************
 * ================UAS_RegisterMethodCallBack()===================
 *
 * This function is used to register function handlers called when 
 * Server requests are recieved.  The UA_InitModule function uses
 * this function to set the default sip method handlers described in rfc3261.
 * Let't say you were adding a new method to this SIP stack, you would have to 
 * call this function to register the function used to handle the server 
 * requests.  For example UA_RegisterMethodCallBack(eSIP_REFER, UA_MyRefer);
 *
 * tSipMethod the enumerated value of the method type. This is defined in 
 * sipcnst.h
 *
 * pfHandler = A pointer to a function that will be called to handle the 
 *             sip request
 *
 * RETURNS: 
 *         Nothing
 ******************************************************************************
 */
void UAS_RegisterMethodCallBack(
    tSipMethod  method, 
    tpfUaServer pfHandler)
{
    if (method >= eSIP_LAST_METHOD) {
        return;
    }
    _UAS_handlers[method] = pfHandler;
    return;
}

/* 
 *****************************************************************************
 * ================UAS_RegisterDispatcher()===================
 *
 * This function is used to register a functio that is used to dispatch
 * server requests to the appropriate UA's task that should handle the 
 * message.  In otherwords, if the voip architecture has one task to handle 
 * each UA, then this function needs to be called at initialization time.
 * Then, once a server request is received and the UA that shold handle the 
 * the request is determined, the function that was registered via this 
 * function call will be called.
 *
 * pfHandler = The function that will perform the dispatching of the sip 
 * message to the appropriate task.  Note that the task that the message 
 * should be sent to is in the Ua object (i.e. the "taskId" memeber of tUa). 
 * 
 * RETURNS: 
 *         Nothing 
 ******************************************************************************
 */
void UAS_RegisterDispatcher(tpfSipDispatcher pfHandler)
{
    _UAS_dispatcher = pfHandler;
    return;
}

/* 
 *****************************************************************************
 * ================UAS_SetupProxy()===================
 * 
 * This function is used to register a function and set a proxy URI 
 * that is used if SIP applications want the stack to act like a stateless 
 * proxy. If _UAS_proxyServer is NULL, then the stack will make no attempt to 
 * perform any proxy functions.  Otherwise, the callback function will be 
 * called everytime the stack receives a reeusts other than REGISTER.
 *
 * pfHandler = The function that will detemine whether or not to proxy 
 *             forward a SIP request. 
 *
 * pProxyFqdn = A NULL terminated string to use as the URI of this proxy
 *              i.e. "sip:212.20.1.1"
 * 
 * RETURNS: 
 *         SIP_OK:  The values were successfully set.
 *         SIP_FAILED: There was an issue with decoding the URI of the proxy.
 ******************************************************************************
 */
vint UAS_SetupProxy(
    tpfSipProxy pfHandler, 
    char       *pProxyFqdn)
{
    _UAS_proxyServer = pfHandler;
    if (_UAS_proxyServer != NULL) {
        if (DEC_Uri(pProxyFqdn, 
                    OSAL_strlen(pProxyFqdn), &_UAS_proxyUri) != SIP_OK) {
            _UAS_proxyServer = NULL;
            return (SIP_FAILED);
        }
    }
    return (SIP_OK);
}

void UAS_SetMatch(tSipMatchType matchType)
{
    if (matchType == SIP_REQUEST_MATCH_NONE) {
        _UAS_MatchType = SIP_REQUEST_MATCH_REQUEST_URI;
    }
    else {
        _UAS_MatchType = matchType;
    }
}

