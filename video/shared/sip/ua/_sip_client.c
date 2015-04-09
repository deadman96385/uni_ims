/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 */
#include "sip_cfg.h"
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_voipnet.h"
#include "sip_sip.h"
#include "sip_clib.h"
#include "sip_timers.h"
#include "sip_mem.h"
#include "sip_msgcodes.h"
#include "sip_parser_dec.h"
#include "sip_debug.h"
#include "sip_auth.h"
#include "sip_dbase_sys.h"
#include "sip_dbase_endpt.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "sip_tu.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_ua.h"
#include "sip_ua_client.h"

#include "_sip_helpers.h"
#include "_sip_callback.h"
#include "_sip_fsm.h"
#include "sip_mem_pool.h"

static tpfSipDispatcher _UAC_dispatcher = NULL;
static tpfSipProxy      _UAC_proxyClient = 0;
static tUri             _UAC_proxyUri;

static vint _UAC_dispatch(
    tSipHandle  hContext,
    tSipHandle  hDialog,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction)
{
    tSipIpcMsg ipcMsg;

    /* build up the message for Inter Process Communication */
    ipcMsg.type = eSIP_IPC_CLIENT_MSG;
    ipcMsg.hTransaction = hTransaction;
    ipcMsg.hOwner = hDialog;
    ipcMsg.pMsg = pMsg;
    ipcMsg.id = TRANS_GetId(hTransaction);

    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UAC_dispatch: dispatching to hContext:%x ",
            (int)hContext, 0, 0);
    if ((*_UAC_dispatcher)(hContext, &ipcMsg) != SIP_OK) {
        /* error so free up the allocated IPC message */
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UAC_dispatch: Could not dispatch msg to context:%x ",
                (int)hContext, 0, 0);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================_UAC_proxy()===================
 * 
 * This function will statelessly proxy back a response as per RFC3261 
 * section 16.  This function wil proxy back the response if the response 
 * meets the following criteria...
 * 1) The response could be matched to any existing transaction or dialog
 * 2) The topmost via matches the proxu URI settings known in this file
 *    (i.e. _ServerProxyUri)
 * 3) There is still another 'Via' headerfield that indicates who the 
 *    the receipiant.
 *
 * pLclConn = The local IP interface that the response was recieved on.
 *
 * pMsg = A pointer to the response to be proxied back.
 *
 * RETURNS: 
 *         SIP_OK:  The response was succesfully sent.
 *         SIP_FAILED: Could not proxy back the response.
 ******************************************************************************
 */
static vint _UAC_proxy(
    tLocalIpConn  *pLclConn,
    tSipIntMsg    *pMsg)
{
    tDLListEntry *pEntry;
    tViaHFE      *pVia;
    
    /* pull off the top via if there are more than one via */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry) > 1) {
        pVia = (tViaHFE*)DLLIST_Dequeue(&pMsg->ViaList);
        if (pVia) {
            /* see if this via is this proxy */
            if (VoIP_IsEqualHost(&pVia->uri.host, &_UAC_proxyUri.host) ==
                    SIP_OK) {
                /* Then this via belongs to us and there's still one left so 
                 * send the response to that 'hop' 
                 */
                pEntry = NULL;
                if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
                    pVia = (tViaHFE*)pEntry;
                    return TRANSPORT_ProxyMsg(pLclConn, pMsg, &pVia->uri);
                }
            }
        }
    }
    return (SIP_FAILED);
}


/* 
 *****************************************************************************
 * ================UAC_Entry()===================
 *
 * This is the entry point to the UA's Client side behavior.
 * Once a SIP event (i.e. new sip message or timer event) is dispatched to 
 * the appropriate thread
 *
 * hOwner  = The owner of the event, could be a handle to a dialog or NULL
 *
 * pMsg = a pointer a sip message object, This is NULL for timer events.
 *
 * hTransaction = A handle to the client side transaction .
 *
 * id = the id paramtere from the timer event used to determine if the timer 
 *      event is 'old' or 'stale'
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UAC_Entry(
    tSipHandle  hOwner, 
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction,
    uint32         id)
{
    /* in this case the hOwner is a handle to a dialog.  
     * This may exist for 200 retry's to an INVITE since 200 
     * retry's are sent outside the context of transaction.  
     * Otherwise, it's NULL.
     */
    tSipDialog *pDialog;
    
    if (hTransaction) {
        /* then we go into the transaction FSM */
        if (TRANS_CheckId(hTransaction, id) == TRUE) {
            
            SIP_DebugLog(SIP_DB_UA_LVL_3,
                "UAC_Entry: sending pMsg:%x into transaction:%x FSM with id:%d", 
                (int)pMsg, (int)hTransaction, (int)id);
            
            if (TRANS_ClientResp(pMsg, hTransaction) != SIP_OK) {
                SIP_freeMsg(pMsg);   
            }
        }
        else {
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UAC_Entry: received old MSG for transaction:%x with id:%d", 
                    (int)hTransaction, (int)id, 0);
            SIP_freeMsg(pMsg);    
        }
    }
    else if (hOwner) {
        /* then there is no transaction associated 
         * with this message so send it to the dialog FSM 
         */
        pDialog = (tSipDialog*)hOwner;
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UAC_Entry: No transaction when sending pMsg:%x into dialog:%x FSM", 
            (int)pMsg, (int)hOwner, 0);
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);    
        }
    }
    else {
        /* we should never be in here */
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UAC_Entry: ERROR No dialog or transaction associated with event",
            0, 0, 0);
        SIP_freeMsg(pMsg);    
    }
    return;
}

/* 
 *****************************************************************************
 * ================UAC_Invite()===================
 *
 * This is the callback that's called for an initial 'INVITE' request
 * (by 'initial' we mean an INVITE that initializes a dialog)
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Invite(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    /* tTransClient *pTrans = (tTransClient*)hTrans; */
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Invite: event:%d ", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        /* bind the trans to the dialog */
        pDialog->hTrans = hTransaction;
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL,
                (tSipHandle)eSIP_RSP_CODE_XACT_TIMEOUT);
        DIALOG_Destroy(pDialog);
    break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_ReInvite()===================
 *
 * This is the callback that's called when a response or event is received 
 * to an 'INVITE' request that has been sent.
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_ReInvite(
    tSipHandle  hOwner, 
    uint32         event, 
    tSipIntMsg  *pMsg, 
    tSipHandle  hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    /* tTransClient *pTrans = (tTransClient*)hTrans; */
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_ReInvite: event:%d ", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        /* bind the trans to the dialog */
        pDialog->hTrans = hTransaction;
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        /*
         * If the Re-INVITE failed then the remote party was not reachable
         * assume network error.  Kill the call.
         */
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL, NULL);
        DIALOG_Destroy(pDialog);
    break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Update()===================
 *
 * This is the callback that's called when a response or event is received 
 * to an 'UPDATE' request that has been sent.
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Update(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Update: event:%d ", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        /* bind the trans to the dialog */
        pDialog->hUpdateTrans = hTransaction;
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        /*
         * If the Re-INVITE failed then the remote party was not reachable
         * assume network error.  Kill the call.
         */
        UA_SessionExpiryCB(NULL, pDialog);
        break;
    default:
        break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Refer()===================
 *
 * This is the callback that's called when a response or event is received for 
 * a REFER request.
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Refer(
    tSipHandle  hOwner, 
    uint32         event, 
    tSipIntMsg  *pMsg, 
    tSipHandle  hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    /* tTransClient *pTrans = (tTransClient*)hTrans; */
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Refer: event:%d ", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        /* bind the trans to the dialog */
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        /* bad news, mark the transfer as invalid so we 
         * properly ignore subseqeunt NOTIFY's
         */
        pDialog->transferor.IsValid = FALSE;
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_TRANSFER_FAILED, NULL, NULL);
    break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Notify()===================
 *
 * This is the callback that's called when a response or event is recevied for 
 * a NOTIFY request.
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Notify(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Notify: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        /* bind the trans to the dialog */
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        /* then we should remove the subscription from the dialog */
        UASM_NotifyFailed(pDialog, hTransaction);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Subscribe()===================
 *
 * This is the callback that's called when a response or event is received 
 * for a Subscribe request.
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Subscribe(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Subscribe: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        /* bind the trans to the dialog */
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        /* Remove it from the list of subscriptions, if there are no 
         * more subscriptions then notify app of no more subscriptions */
        UASM_SubscribeFailed(pDialog, pMsg, hTransaction);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}


/* 
 *****************************************************************************
 * ================UAC_Cancel()===================
 *
 * This is the callback that's called when a response or event is recevied for 
 * a CANCEL request.
 *
 * This includes any responses to the CANCEL or any events from the 
 * transaction layer, as a result of a CANCEL transaction should not affect 
 * the dialog state or anything else.  CANCEL requests basically are attempted 
 * and if for any reason they don't succeed then no action should be taken.
 *
 * If an INVITE transaction is cancelled via a CANCEL request.  The 
 * state of the dialog is 'markedForDeletion' by that CANCEL request.  
 * Then the dialog will be deleted when the INVITE transaction has completed.
 * AND NOT WHEN THE CANCEL COMPLETES.
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
 *      eTU_FAILED,
 *      eTU_REQUEST,
 *      eTU_RESPONSE,
 *      eTU_DEAD,
 *
 * pMsg = a pointer a sip message object, this is the internal response.
 *
 * hTransaction = A handle to the client side transaction that "owns" this 
 *                response.
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Cancel(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* simply get rid of anything here */
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Cancel: event:%d ", event, 0, 0);
    if (event == eTU_DEAD) {
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
    }
    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Bye()===================
 *
 * This is the callback that's called whena response or event is recevied for 
 * a BYE request. 
 *
 * hOwner  = A handle to a current dialog
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Bye(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    /* simply get rid of anything here */
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Bye: event:%d ", event, 0, 0);
    /* 
     * we invoke DIALOG_Destroy here but not in _UASM_ConfClientByeReq 
     * because if we destroy it in _UASM_ConfClientByeReq, we might lose
     * events or responses.
     */
    if (event == eTU_DEAD) {
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
    }
    else if (DIALOG_IS_ACTIVE(pDialog->currentState)) {
        /* bind the trans to the dialog */
        pDialog->hTrans = hTransaction;
        DIALOG_Destroy(pDialog);
    }

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Register()===================
 *
 * This is a function specifically for handling UAC REGISTER method responses.
 * Since the transaction module allows you to register handlers
 * for transactions when they are created, we register special handlers for
 * responses to requests that are not associated with any instances of 
 * dialogs.  
 *
 * hOwner  = A handle to a tUaReg object inside of a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Register(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tUaReg       *pReg;
    tUa          *pUa;
    tContactHFE  *pContact;
    tViaHFE      *pVia;
    tDLListEntry *pEntry;
    uint32        reReg;
    tSipIntMsg   *pNewMsg;
    tSipHandle    hTransport;
    vint          stale = FALSE;
    vint          retVal;

    pReg = (tUaReg*)hOwner;
    pUa = pReg->pUa;
        
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Register: hUa:%x hReg:%x event:%d", 
        (int)pUa, (int)pReg, (int)event);

    if (hTransaction != pReg->hTransaction) {
        /* It's an old transaction, ignore it. */
        SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Register: Got old transaction %x, "
                "current transaction %x",
                (int)hTransaction, (int)pReg->hTransaction, 0);
        if (pMsg) {
            SIP_freeMsg(pMsg);
        }
        if (eTU_DEAD == event) {
            TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        }
        return (SIP_OK);
    }

    /* In this case the owner is a handle to the UA */
    reReg = pReg->reRegInterval;
    switch (event) {
    case eTU_RESPONSE:
        if (pReg->pMsg == NULL || pMsg->CSeq.method != eSIP_REGISTER) {
            /* 
             * Then registration was killed or this is a misrouted 
             * message.
             */
            break;
        }
        
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            /* Calculate a re-registration expiry */
            if (HF_PresenceExists(&pMsg->x.DCPresenceMasks, eSIP_EXPIRES_HF) &&
                    pMsg->Expires != 0) {
                reReg = (uint32)pMsg->Expires;
            }
            /* 
             * Make sure that this binding matches our current one and then 
             * go through the contacts and see if 'expires' is populated 
             */
            else if (OSAL_strcmp(pMsg->From.szTag, pReg->pMsg->From.szTag) == 0
                    && HF_PresenceExists(&pMsg->x.DCPresenceMasks, 
                    eSIP_CONTACT_HF)) {
                pEntry = NULL;
                while (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
                    pContact = (tContactHFE*)pEntry;
                    if (pContact->expires == 0) {
                        /* Skip this entry */
                        continue;
                    }
                    /* 
                     * Make sure the port is correct to just in case there
                     * are several different bindings from STUN remappings
                     */
                    if (pUa->regLclConn.addr.port == 0) {
                        /*
                         * If the port according to the application is zero
                         * then we don't know what port to look for, so
                         * just grab the expires.
                         */
                        reReg = pContact->expires;
                        break;
                    }
                    if (pContact->uri.host.port == pUa->regLclConn.addr.port) {
                        reReg = pContact->expires;
                        break;
                    }
                    if (pUa->regLclConn.addr.port == SIP_DEFAULT_IPADDR_PORT && 
                          (pContact->uri.host.port == 0 || 
                          pContact->uri.host.port == SIP_DEFAULT_IPADDR_PORT)) {
                        reReg = pContact->expires;
                        break;
                    }
                } /* end of while */
            }

            if ((reReg != 0) && (pReg->hReRegTimer != NULL)) {
                reReg = (uint32)UA_GetTimerMs(reReg);
                SIPTIMER_Start(pReg->hReRegTimer, UA_ReRegisterCB, 
                        pReg, reReg, FALSE);
                /* store it for watchdog timer , we need this information to take care 
                   lazy timer */
                SIPTIMER_AddWakeUpTime(reReg + SIPTIMER_REGISTER_BUFFER_TIMER_MS);
            }

            /*
             * TS 24.229 and RFC6223:
             * If Keep_Alive_Enabled is enable and receive 'keep' response,
             * start a timer to send keep-alives.
             */
            pEntry = NULL;
            if (0 == pReg->refreshInterval) {
                if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
                    pVia = (tViaHFE *)pEntry;
                    pReg->refreshInterval = pVia->keepaliveFeq;
                }
            }

            if ((OSAL_TRUE == pReg->refreshEnable) &&
                    (pReg->refreshInterval != 0) &&
                    (pReg->hRefreshTimer != NULL) &&
                    (pReg->refreshInterval < reReg)) {
                /*
                 * Then cache the remote address of the proxy and start a timer
                 * to periodically send a "dummy" packet to preserve and NAT
                 * mappings.
                 */
                if (NULL != (hTransport = TRANS_GetTransport(hTransaction))) {
                    if (SIP_OK == TRANSPORT_GetTransportAddr(
                            hTransport, &pReg->refreshArgs.refreshAddr,
                            &pReg->refreshArgs.refreshFd)) {
                        SIPTIMER_Start(pReg->hRefreshTimer,
                                UA_KeepalivesCB, &pReg->refreshArgs,
                                pReg->refreshInterval * 1000, TRUE);
                    }
                }
            }

            /* Cache remote address in ua */
            if (NULL != (hTransport = TRANS_GetTransport(hTransaction))) {
                TRANSPORT_GetTransportAddr(hTransport,
                        &pReg->refreshArgs.refreshAddr,
                        &pReg->refreshArgs.refreshFd);
            }

            pReg->isBusy = FALSE;
            UA_AppEvent(pUa, NULL, eUA_REGISTRATION_COMPLETED, pMsg,
                    (tSipHandle)pMsg->code);
        }
        else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
            /* Get the nonce 'stale' value */
            pEntry = NULL;
            if (DLLIST_GetNext(&pMsg->AuthorizationList, &pEntry)) {
                    tAuthorizationHFE *pAuth = (tAuthorizationHFE *)pEntry;
                stale = pAuth->stale;
                if (0 != pAuth->domain[0]) {
                    /* Then we want to retry at least one more time */
                    pReg->retryCnt = 2;
                }
            }
                
            if(pReg->hasTried < pReg->retryCnt || stale == TRUE) {
                /* 
                 * If we haven't already tried to authenticate or if
                 * the 'nonce' is stale, let's try to authenticate or
                 * re-authenticate. 
                 */

                /* first clear out any old authentication entries in history */
                if (SIP_OK != (retVal = UA_WWWAuthenticate(pUa, pMsg->From.uri.user,
                        &pMsg->AuthorizationList,
                        &pReg->pMsg->AuthorizationList,
                        NULL, 0, NULL))) {
                    /* If AKA failed, try to pass nonce to app for ISIM AKA */
                    if ((SIP_AUTH_AKA_V1 == retVal) ||
                            (SIP_AUTH_AKA_V2 == retVal)) {
                        /* Just get the nonce from first challenge in the list */
                        pEntry = NULL;
                        if (DLLIST_GetNext(&pMsg->AuthorizationList, &pEntry)) {
                            tAuthorizationHFE *pAuth =
                                    (tAuthorizationHFE *)pEntry;
                            SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Register: ISIM"
                                    " AKA AUTH Required nonce=%s.",
                                    (uint32)(pAuth->szNonce), 0, 0);
                            /*
                             * Put reason phrase and nonce for ISIM auth. Clean
                             * pReasonPhrase if it exsits
                             */
                            SIP_copyStringToSipText(pAuth->szNonce,
                                    &pMsg->pReasonPhrase);
                            /*
                             * Copy the authirization list to pReg->pMsg for
                             * later registration retry with auth resp.
                             */
                            if (SIP_OK != DLLIST_Copy(&pMsg->AuthorizationList,
                                        &pReg->pMsg->AuthorizationList,
                                        eDLLIST_WWW_AUTH_HF)) {
                                SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Register:"
                                        "Copy authirization list failed.",
                                        0, 0, 0);
                                UA_CleanRegistration(pReg);
                            }
                            /*
                             * Copy the response code to pReg->pMsg for retry
                             * register with aka auth response to set presence.
                             */
                            pReg->pMsg->code = pMsg->code;

                            /* Put response code to application. */
                            if (SIP_AUTH_AKA_V1 == retVal) {
                                pMsg->code = eSIP_RSP_CODE_AUTH_AKA_V1;
                            }
                            else {
                                /* Suppose to be AKAv2 */
                                pMsg->code = eSIP_RSP_CODE_AUTH_AKA_V2;
                            }
                            pReg->isBusy = FALSE;
                        }
                        else {                        
                            SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Register: Cannot"
                                    " find challenge in AuthList.", 0, 0, 0);
                            UA_CleanRegistration(pReg);
                        }
                    }
                    else {
                        /* if you can't then tell the APP */
                        UA_CleanRegistration(pReg);
                    }
                    UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                            (tSipHandle)pMsg->code);
                }
                else {
                    /* Pass it to the TU */
                    pNewMsg = pReg->pMsg;
                    pNewMsg->CSeq.seqNum++;
                    if (pMsg->code == eSIP_RSP_UNAUTH) {
                        HF_SetPresence(&pNewMsg->x.ECPresenceMasks,
                                eSIP_AUTHORIZATION_HF);
                    }
                    else {
                        HF_SetPresence(&pNewMsg->x.ECPresenceMasks,
                                eSIP_PROXY_AUTHORIZATION_HF);
                    }
                    /* new transaction so new via branch */
                    HF_MakeViaBranch(&pNewMsg->ViaList, NULL);
                    /* generate a new 'fromTag' */
                    HF_GenerateTag(pNewMsg->From.szTag);

                    if (NULL == (pReg->pMsg = SIP_copyMsg(pNewMsg))) {
                        SIP_DebugLog(SIP_DB_UA_LVL_1,
                            "UAC_Register: Failed to cooy pMsg", 0, 0, 0);
                        SIP_freeMsg(pNewMsg);
                        UA_CleanRegistration(pReg);
                        UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                                (tSipHandle)pMsg->code);
                        break;
                    }

                    pReg->hasTried++;
                    if (UA_SendRequest(pUa, &pNewMsg->requestUri, pReg, pNewMsg,
                            UAC_Register, TRANS_GetTransport(hTransaction),
                            &pReg->hTransaction) != SIP_OK) {
                        SIP_freeMsg(pNewMsg);
                        UA_CleanRegistration(pReg);
                        UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                                (tSipHandle)pMsg->code);
                    }
                }
            }
            else {
                UA_CleanRegistration(pReg);
                UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                        (tSipHandle)pMsg->code);
            }
        }
        else if (MSGCODE_ISPROV(pMsg->code)) {
            /* no-op just break */
            break;
        }
        else if (eSIP_RSP_INTERVAL_TOO_BRIEF == pMsg->code) {
            /*
             * Handle 423 Interval Too Brief.
             */
            if (HF_PresenceExists(&pMsg->x.DCPresenceMasks,
                    eSIP_MIN_EXPIRES_HF)) {
                /*
                 * If Min-Expires header is presented in response
                 * then set it to reRegInterval.
                 */
                pReg->reRegInterval = pMsg->MinExpires;
                /*
                 * Just call UA_ReRegisterCB() to re-register.
                 */
                UA_ReRegisterCB(NULL, pReg);
            }
            else {
                /*
                 * If Min-Expires is not presented.
                 */
                UA_CleanRegistration(pReg);
                UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                        (tSipHandle)pMsg->code);
            }
        }
        else {
            UA_CleanRegistration(pReg);
            UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                    (tSipHandle)pMsg->code);
        }
        break;

    case eTU_FAILED:
        /* we timed out, check if the transaction is valid or not */
        UA_CleanRegistration(pReg);
        UA_AppEvent(pUa, NULL, eUA_REGISTRATION_FAILED, NULL,
                (tSipHandle)eSIP_RSP_CODE_XACT_TIMEOUT);
        break;

    case eTU_DEAD: 
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    default:
        break;
    } /* end of switch */

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_PublishNoDialog()===================
 *
 * This is a function specifically for handling UAC PUBLISH method responses 
 * for PUBLISH requests that are sent outside the context of a dialog.
 * Since the transaction module allows you to register handlers
 * for transactions when they are created, we register special handlers for
 * responses to requests that are not associated with any instances of 
 * dialogs.  
 *
 * hOwner  = A handle to a tUaPub object inside of a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_PublishNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tUaPub       *pPub;
    tUa          *pUa;
    vint          refresh;
    tSipIntMsg   *pNewMsg;
    tHdrFldList  *pETag;  
    
    pPub = (tUaPub*)hOwner;
    pUa = pPub->pUa;
        
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UAC_PublishNoDialog: hUa:%x hPub:%x event:%d", 
            (int)pUa, (int)pPub, (int)event);

    /* 
     * In this case the owner is a handle to the tUaPub object inside a
     * UA Object.
     */
    switch (event) {
    case eTU_RESPONSE:
        if (pPub->pMsg == NULL || pMsg->CSeq.method != eSIP_PUBLISH) {
            /* Then the publish mechanism was deactivated or this response 
             * does not really belong to us.
             */
            break;
        }
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            if (pPub->pMsg->Expires == 0) {
                /* Then we are terminating the PUBLISH mechanism */
                UA_AppEvent(pUa, NULL, eUA_PUBLISH_COMPLETED, pMsg, 
                        pPub->hTransaction);
                UA_CleanPublish(pPub);
                break;
            }
            
            /* 
             * Throw out the message body since we will not need it for 
             * 'refreshing' the Publication.
             */
            if (pPub->pMsg->pMsgBody) {
                SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY,
                    (tDLListEntry *)pPub->pMsg->pMsgBody);
                pPub->pMsg->pMsgBody = NULL;
                /* Clear the content-type and content length as well */
                HF_ClrPresence(&pPub->pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
                pPub->pMsg->ContentType = eCONTENT_TYPE_NONE;
                HF_ClrPresence(&pPub->pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);
                pPub->pMsg->ContentLength = 0;
            }
            
            /* 
             * Check for a new "SIP_ETag" header field.  If it exists then
             * we have to include the value in subsequent PUBLISH requests in 
             * the "SIP-If-Match" header field.
             */
            if (NULL != (pETag = HF_Find(&pMsg->pHFList, eSIP_ETAG_HF))) {
                /* 
                 * Delete any existing SIP-If-Match header fields and then copy
                 * this header value to a new "SIP-If-Match" header field.
                 */
                HF_Delete(&pPub->pMsg->pHFList, eSIP_IF_MATCH_HF);
                /* Then add the 'SIP-If-Match' header field */
                HF_CopyInsert(&pPub->pMsg->pHFList, eSIP_IF_MATCH_HF,
                         pETag->pField, 0);
                HF_SetPresence(&pPub->pMsg->x.ECPresenceMasks,
                        eSIP_IF_MATCH_HF); 
            }
            
            /* 
             * Check/Start the "refresh" mechanism but after the refresh rate 
             * is calculated.
             */
            if (HF_PresenceExists(&pMsg->x.DCPresenceMasks, eSIP_EXPIRES_HF)) {
                if (pMsg->Expires != 0)
                    pPub->pMsg->Expires = pMsg->Expires;
            }
            if (pPub->hTimer != NULL) {
                /* 
                 * Calculate the refresh rate expiry.  Always make it a few 
                 * seconds less than what is specified to account for race 
                 * conditions.
                 */
                refresh = UA_GetTimerMs(pPub->pMsg->Expires);
                /* Launch the timer used to refresh */
                SIPTIMER_Start(pPub->hTimer, UA_RefreshPublishCB, pPub, refresh,
                        FALSE);
            }
            /* Change/reset the state since the transaction is now over */
            pPub->isBusy = FALSE;
            UA_AppEvent(pUa, NULL, eUA_PUBLISH_COMPLETED, pMsg, 
                    pPub->hTransaction);
        }
        else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
            if(pPub->hasTried == TRUE) {
                /* 
                 * Then we have already tried to authenticate once,
                 * this flag prevents repeatedly trying to authenticate.
                 */
                UA_AppEvent(pUa, NULL, eUA_PUBLISH_FAILED, pMsg,
                        pPub->hTransaction);
                UA_CleanPublish(pPub);
                break;
            }
            
            /* 
             * Try to authenticate. First clear out any old authentication 
             * entries in the history.
             */
            if (UA_Authenicate(pUa, SIP_PUBLISH_METHOD_STR, pMsg,
                    pPub->pMsg) != SIP_OK) {
                /* if you can't, then tell the APP that we failed */
                UA_AppEvent(pUa, NULL, eUA_PUBLISH_FAILED, pMsg,
                        pPub->hTransaction);
                UA_CleanPublish(pPub);
                break;
            }
            
            pNewMsg = pPub->pMsg;
            pNewMsg->CSeq.seqNum++;
            if (pMsg->code == eSIP_RSP_UNAUTH) {
                HF_SetPresence(&pNewMsg->x.ECPresenceMasks,
                        eSIP_AUTHORIZATION_HF);
            }
            else {
                HF_SetPresence(&pNewMsg->x.ECPresenceMasks,
                        eSIP_PROXY_AUTHORIZATION_HF);
            }
            /* new transaction so new via branch */
            HF_MakeViaBranch(&pNewMsg->ViaList, NULL);
            /* generate a new 'fromTag' */
            HF_GenerateTag(pNewMsg->From.szTag);

            if (NULL == (pPub->pMsg = SIP_copyMsg(pNewMsg))) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UAC_PublishNoDialog: Failed to cooy pMsg", 0, 0, 0);
                SIP_freeMsg(pNewMsg);
                UA_AppEvent(pUa, NULL, eUA_PUBLISH_FAILED, pMsg,
                        pPub->hTransaction);
                UA_CleanPublish(pPub);
                break;
            }
            pPub->hasTried = TRUE;
            if (UA_SendRequest(pUa, &pNewMsg->requestUri, pPub, pNewMsg,
                    UAC_PublishNoDialog, TRANS_GetTransport(hTransaction),
                    NULL) != SIP_OK) {
                SIP_freeMsg(pNewMsg);
                UA_AppEvent(pUa, NULL, eUA_PUBLISH_FAILED, pMsg,
                        pPub->hTransaction);
                UA_CleanPublish(pPub);
            }
        }
        else if (MSGCODE_ISPROV(pMsg->code)) {
            /* no-op just break */
            break;
        }
        else {
            UA_AppEvent(pUa, NULL, eUA_PUBLISH_FAILED, pMsg,
                    pPub->hTransaction);
            UA_CleanPublish(pPub);
        }
        break;

    case eTU_FAILED:
        /* we timed out */
        UA_AppEvent(pUa, NULL, eUA_PUBLISH_FAILED, NULL, pPub->hTransaction);
        UA_CleanPublish(pPub);
        break;

    case eTU_DEAD: 
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    default:
        break;
    } /* end of switch */

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_UnRegister()===================
 *
 * This is the callback that's called when a response or event is recevied 
 * for REGISTER request with an 'expires' header field of '0' (un-register). 
 *
 * hOwner  = A handle to a tUAReg object that this RESISTER transaction 
 * belongs tp.
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_UnRegister(
    tSipHandle  hOwner, 
    uint32      event, 
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    tUaReg *pReg = (tUaReg*)hOwner;

    /* simply get rid of anything here */
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_UnRegister: event:%d ", event, 0, 0);
    switch (event) {
        case eTU_RESPONSE:
            if (MSGCODE_ISSUCCESS(pMsg->code)) {
                UA_AppEvent(pReg->pUa, NULL, eUA_REGISTRATION_COMPLETED, pMsg,
                    (tSipHandle)0);
            }
            else if (MSGCODE_ISFINAL(pMsg->code)) {
                UA_AppEvent(pReg->pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                    (tSipHandle)0);
            }
            break;
        case eTU_FAILED:
            UA_AppEvent(pReg->pUa, NULL, eUA_REGISTRATION_FAILED, NULL,
                (tSipHandle)0);
            break;
        case eTU_DEAD:
            TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
            break;
        default:
            break;
    }

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_MessageNoDialog()===================
 *
 * This is a function specifically for handling UAC MESSAGE method responses
 * when the request was sent outside the context of a dialog.
 * Since the transaction module allows you to register handlers
 * for transactions when they are created, we register special handlers for
 * responses to requests that are not associated with any instances of 
 * dialogs.  
 *
 * hOwner  = A handle to a tUaTrans object inside of a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_MessageNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tUaTrans     *pT;
    tUa          *pUa;
    tSipIntMsg   *pRetry;
    tSipHandle    hNothing;
    
    pT = (tUaTrans*)hOwner;
    pUa = pT->pUa;
        
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UAC_MessageNoDialog: hUa:%x hTrans:%x event:%d", 
            (int)pUa, (int)pT, (int)event);

    /* In this case the owner is a handle to a UA 'transaction'*/
    switch (event) {
    case eTU_RESPONSE:
        if (pMsg->CSeq.method == eSIP_MESSAGE) {
            if (MSGCODE_ISSUCCESS(pMsg->code)) {
                /* MESSAGE Evt */
                UA_AppEvent(pUa, NULL, eUA_TEXT_MESSAGE_COMPLETED, pMsg,
                        pT->hTransaction);
                UA_CleanTrans(pT);
            }
            else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
                if(pT->hasTried == FALSE) {
                    /* Then try to authenticate but only once,
                     * this flag prevents repeatedly trying to authenticate */
                    pRetry = pT->pMsg;
                    if (UA_Authenicate(pUa, SIP_MESSAGE_METHOD_STR, pMsg, 
                            pRetry) != SIP_OK) {
                        /* if you can't then tell the APP */
                        goto errorExit;
                    }
                    else {
                        pRetry->CSeq.seqNum++;
                        /* new transaction so new via branch */
                        HF_MakeViaBranch(&pRetry->ViaList, NULL);
                        /* generate a new 'fromTag' */
                        HF_GenerateTag(pRetry->From.szTag);
                        
                        pT->hasTried = TRUE;
                        if (UA_SendRequest(pUa, &pRetry->requestUri, pT, pRetry,
                                UAC_MessageNoDialog, NULL, &hNothing) != 
                                SIP_OK) {
                            goto errorExit;
                        }
                        else {
                            /* 
                             * We successfully sent the pRetry therefore 
                             * we no longer own the pMsg in pTxt so clear it out
                             */
                            pT->pMsg = NULL;
                        }
                    }
                }
                else {
                    goto errorExit;
                }
            }
            else if (MSGCODE_ISPROV(pMsg->code)) {
                /* no-op just break */
                break;
            }
            else {
                goto errorExit;
            }
        }
        break;

    case eTU_FAILED:
        /* we timed out */
errorExit:
        /* MESSAGE Evt */
        UA_AppEvent(pUa, NULL, eUA_TEXT_MESSAGE_FAILED, pMsg, pT->hTransaction);
        UA_CleanTrans(pT);
        break;

    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    default:
        break;
    } /* end of switch */

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_NotifyNoDialog()===================
 *
 * This is a function specifically for handling UAC NOTIFY method responses
 * when the request was sent outside the context of a dialog.
 * Since the transaction module allows you to register handlers
 * for transactions when they are created, we register special handlers for
 * responses to requests that are not associated with any instances of 
 * dialogs.  
 *
 * hOwner  = A handle to a tUaTrans object
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_NotifyNoDialog(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tUaTrans     *pT;
    tUa          *pUa;
    tSipIntMsg   *pRetry;
    tSipHandle    hNothing;
    
    pT = (tUaTrans*)hOwner;
    pUa = pT->pUa;
        
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UAC_NotifyNoDialog: hUa:%x hTrans:%x event:%d", 
            (int)pUa, (int)pT, (int)event);

    /* In this case the owner is a handle to a UA_Trans object */
    switch (event) {
    case eTU_RESPONSE:
        if (pMsg->CSeq.method == eSIP_NOTIFY) {
            if (MSGCODE_ISSUCCESS(pMsg->code)) {
                /* NOTIFY Evt */
                UA_AppEvent(pUa, NULL, eUA_NOTIFY_EVENT_COMPLETED,
                        pMsg, pT->hTransaction);
                UA_CleanTrans(pT);
            }
            else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
                if(pT->hasTried == FALSE) {
                    /* Then try to authenticate but only once,
                     * this flag prevents repeatedly trying to authenticate */
                    pRetry = pT->pMsg;
                    if (UA_Authenicate(pUa, SIP_NOTIFY_METHOD_STR, 
                            pMsg, pRetry) != SIP_OK) {
                        /* if you can't then tell the APP */
                        goto errorExit;
                    }
                    else {
                        pRetry->CSeq.seqNum++;
                        /* new transaction so new via branch */
                        HF_MakeViaBranch(&pRetry->ViaList, NULL);
                        /* generate a new 'fromTag' */
                        HF_GenerateTag(pRetry->From.szTag);
                        
                        pT->hasTried = TRUE;
                        if (UA_SendRequest(pUa, &pRetry->requestUri, pT, pRetry,
                                UAC_NotifyNoDialog, NULL, &hNothing) !=
                                SIP_OK) {
                            goto errorExit;
                        }
                        else {
                            /* 
                             * We successfully sent the pRetry therefore 
                             * we no longer own the pMsg in pTxt so clear it out
                             */
                            pT->pMsg = NULL;
                        }
                    }
                }
                else {
                    goto errorExit;
                }
            }
            else if (MSGCODE_ISPROV(pMsg->code)) {
                /* no-op just break */
                break;
            }
            else {
                goto errorExit;
            }
        }
        break;

    case eTU_FAILED:
        /* we timed out */
errorExit:
        /* NOTIFY Evt */
        UA_AppEvent(pUa, NULL, eUA_NOTIFY_EVENT_FAILED, pMsg, pT->hTransaction);
        UA_CleanTrans(pT);
        break;

    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    default:
        break;
    } /* end of switch */

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Message()===================
 *
 * This is a function specifically for handling UAC MESSAGE method responses
 * and events sent inside the context of a dialog.
 *
 * hOwner  = A handle to a tSipDialog object inside of a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Message(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Message: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_TEXT_MESSAGE_FAILED, NULL,
                hOwner);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Publish()===================
 *
 * This is a function specifically for handling UAC PUBLISH method responses 
 * for PUBLISH requests that are sent within the context of a dialog.
 *
 * hOwner  = A handle to a dialog object.
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Publish(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Publish: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_PUBLISH_FAILED, NULL, hOwner);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Info()===================
 *
 * This is a function specifically for handling UAC INFO method responses
 * and events sent inside the context of a dialog.
 *
 * hOwner  = A handle to a tSipDialog object inside of a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Info(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Info: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_INFO_FAILED, NULL, 
                     (tSipHandle)eSIP_RSP_CODE_XACT_TIMEOUT);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Prack()===================
 *
 * This is a function specifically for handling UAC PRACK method responses
 * and events sent inside the context of an !!!EARLY!!! dialog.
 *
 * hOwner  = A handle to a tSipDialog object inside of a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Prack(
    tSipHandle   hOwner, 
    uint32       event, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Prack: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        pDialog->hPrackTrans = hTransaction;
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        /* clean up the dialog's prack stuff */
        DIALOG_CleanPrack(&pDialog->prack);
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_PRACK_FAILED, NULL, 
                     (tSipHandle)eSIP_RSP_CODE_XACT_TIMEOUT);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_Options()===================
 *
 * This is a function specifically for handling UAC OPTIONS method responses
 * and events WHEN AN OPTIONS IS SENT WITHIN THE CONTEXT OF THE DIALOG ONLY!.
 *
 * hOwner  = A handle to DIALOG
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_Options(
    tSipHandle   hOwner, 
    uint32       event,
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    /* In this case the hOwner is a dialog */
    tSipDialog *pDialog = (tSipDialog*)hOwner;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UAC_Options: event:%d", event, 0, 0);

    switch (event) {
    case eTU_RESPONSE:
        if (UASM_DialogClient(pDialog, pMsg, hTransaction) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }
    break;
    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    case eTU_FAILED:
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_OPTIONS_FAILED, NULL, 
                hTransaction);
        break;
    default:
    break;
    } /* end of switch */
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_OptionsNoDialog()===================
 *
 * This is a function specifically for handling UAC OPTIONS method responses
 * and events when an OPTIONS request is NOT sent within the context of a 
 * dialog.  Since the transaction module allows you to register handlers
 * for transactions when they are created, we register special handlers for
 * responses to requests that are not associated with any instances of 
 * dialogs.  
 *
 * hOwner  = A handle to a UA
 *
 * event = The event from the TU (Transaction User). Possible values are
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
 * RETURNS: SIP_OK
 ******************************************************************************
 */
vint UAC_OptionsNoDialog(
    tSipHandle   hOwner,
    uint32       event,
    tSipIntMsg  *pMsg,
    tSipHandle   hTransaction)
{
    tUaTrans     *pT;
    tUa          *pUa;
    tSipIntMsg   *pRetry;
    tSipHandle    hNothing;

    pT = (tUaTrans*)hOwner;
    pUa = pT->pUa;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UAC_OptionsNoDialog: hUa:%x hTrans:%x event:%d",
            (int)pUa, (int)pT, (int)event);

    /* In this case the owner is a handle to a UA 'transaction'*/
    switch (event) {
    case eTU_RESPONSE:
        if (pMsg->CSeq.method == eSIP_OPTIONS) {
            if (MSGCODE_ISSUCCESS(pMsg->code)) {
                /* MESSAGE Evt */
                UA_AppEvent(pT->pUa, NULL, eUA_OPTIONS_COMPLETED, pMsg, pT->hTransaction);
                UA_CleanTrans(pT);
            }
            else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
                if(pT->hasTried == FALSE) {
                    /* Then try to authenticate but only once,
                     * this flag prevents repeatedly trying to authenticate */
                    pRetry = pT->pMsg;
                    if (UA_Authenicate(pUa, SIP_OPTIONS_METHOD_STR, pMsg, pRetry) != SIP_OK) {
                        /* if you can't then tell the APP */
                        goto errorExit;
                    }
                    else {
                        pRetry->CSeq.seqNum++;
                        /* new transaction so new via branch */
                        HF_MakeViaBranch(&pRetry->ViaList, NULL);
                        /* generate a new 'fromTag' */
                        HF_GenerateTag(pRetry->From.szTag);

                        pT->hasTried = TRUE;
                        if (UA_SendRequest(pUa, &pRetry->requestUri, pT, pRetry,
                                UAC_OptionsNoDialog, NULL, &hNothing) != SIP_OK) {
                            goto errorExit;
                        }
                        else {
                            /*
                             * We successfully sent the pRetry therefore
                             * we no longer own the pMsg in pTxt so clear it out
                             */
                            pT->pMsg = NULL;
                        }
                    }
                }
                else {
                    goto errorExit;
                }
            }
            else if (MSGCODE_ISPROV(pMsg->code)) {
                /* no-op just break */
                break;
            }
            else {
                goto errorExit;
            }
        }
        break;

    case eTU_FAILED:
        /* we timed out */
errorExit:
        UA_AppEvent(pT->pUa, NULL, eUA_OPTIONS_FAILED, pMsg, pT->hTransaction);
        UA_CleanTrans(pT);
        break;

    case eTU_DEAD:
        TRANSPORT_Dealloc(TRANS_GetTransport(hTransaction));
        break;
    default:
        break;
    } /* end of switch */

    if (pMsg) {
        SIP_freeMsg(pMsg);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UAC_TrafficCB()===================
 *
 * This is the fucntion that was registered with the transport module to pass 
 * incoming traffic destined for the client side behavior back through the sip 
 * stack. In otherwords, when traffic is receved by the transport module the 
 * next train stop is here!  It attempts to match the sip message to a current
 * transaction.
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
vint UAC_TrafficCB(
    tTransportType   transType,
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn,
    tSipIntMsg      *pMsg)
{
    tSipHandle hTransaction;
    tSipHandle hContext;
    UNUSED(pRmtConn);
    UNUSED(transType);
    
    hTransaction = TRANS_ClientSearch(pMsg);
    if (!hTransaction) {
         if (MSGCODE_ISSUCCESS(pMsg->code)) {
            /* If there's no transaction and it's a 200 to invite
             * pass it back to the UA as per the RFC. 
             * Try to find an owner via the dialog identifier.
             */
            tSipDialog *pDialog = UA_DialogSearchAll(pMsg);
            if (pDialog) {
                hContext = DIALOG_GetContext(pDialog);
                return _UAC_dispatch(hContext, pDialog, pMsg, NULL);
            }
         }
        SIP_DebugLog(SIP_DB_UA_LVL_2, "UAC_TrafficCB: no DIALOG", 0, 0, 0);
        if (_UAC_proxyClient) {
            /* Then proxy the response back.  Note that 
             * _UAC_proxyClient is not actually called.  We simply use it 
             * to determine if the SIP application wants use to 'act' like
             * a proxy.  In the future this could be used to communicate
             * additiaonal info to the SIP application when we get responses
             * to proxy back to some UA's server 
             */
            _UAC_proxy(pLclConn, pMsg);
            /* we always return SIP_FAILED to indicate to the caller 
             * that the pMsg should be free'd
             */

        }
        return (SIP_FAILED);
    }
    else {
        hContext = TRANS_GetContext(hTransaction);
        return _UAC_dispatch(hContext, NULL, pMsg, hTransaction);
    }
}

/* 
 *****************************************************************************
 * ================UAC_RegisterDispatcher()===================
 *
 * This function is used to register a functio that is used to dispatch
 * client requests to the appropriate UA's task that should handle the 
 * message.  In otherwords, if the voip architecture has one task to handle 
 * each UA, then this function needs to be called at initialization time.
 * Then, once a client response is received and the transaction the owns 
 * the response is determined, the function that was registered via this 
 * function call will be called.
 *
 * pfHandler = The function that will perform the dispatching of the sip 
 * message to the appropriate task.  Note that the task that the message 
 * should be sent to is in the transaction object that owns the response. 
 * 
 * RETURNS: 
 *         SIP_OK: 
 ******************************************************************************
 */
void UAC_RegisterDispatcher(tpfSipDispatcher pfHandler)
{
    _UAC_dispatcher = pfHandler;
    return;
} 

/* 
 *****************************************************************************
 * ================UAC_SetupProxy()===================
 * 
 * This function sets a pointer to a function and a proxy URI to indicate 
 * that SIP responses not belonging to any transaction should be proxy'd back
 * to the next hop specified in the 'Via' of a SIP message.
 * Note that the callback routine here is not called at this time. 
 * It's simply here to indicate that the SIP applicaiton wants the stack to 
 * use this "proxy" behavior.
 *
 * pfHandler = a pointer that detemines whether or not to proxy 
 *             back a SIP response. If TRUE, then it will.  If FALSE
 *             then the stack will quickly discard misrouted responses
 *
 * pProxyFqdn = A NULL terminated string to use as the URI of this proxy
 *              i.e. "sip:212.20.1.1"
 * 
 * RETURNS: 
 *         SIP_OK:  The values were successfully set.
 *         SIP_FAILED: There was an issue with decoding the URI of the proxy.
 ******************************************************************************
 */
vint UAC_SetupProxy(
    tpfSipProxy pfHandler, 
    char       *pProxyFqdn)
{
    _UAC_proxyClient = pfHandler;
    if (_UAC_proxyClient != NULL) {
        if (DEC_Uri(pProxyFqdn, 
                    OSAL_strlen(pProxyFqdn), &_UAC_proxyUri) != SIP_OK) {
            _UAC_proxyClient = NULL;
            return (SIP_FAILED);
        }
    }
    return (SIP_OK);
}

