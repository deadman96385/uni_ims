/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
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

#include "_sip_callback.h"
#include "_sip_helpers.h"
#include "_sip_fsm.h"
#include "sip_parser_dec.h"

/* 
 *****************************************************************************
 * ================UA_OkayTimeoutCB()===================
 *
 * When an INVITE response is receeved by a UAS and the UA ansers with a 200
 * the 200 is continually sent until an ACK is recieved.  This function
 * is the callback function called by the timer mechanism that the retry timed
 * out.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a dialog object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_OkayTimeoutCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tSipDialog *pDialog = (tSipDialog*)pArg;
    tSipIntMsg *pMsg;
    tSipHandle  hTransport;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_OkayTimeoutCB: calling callback ", 0, 0, 0);

    if (DIALOG_IS_CONFIRMED(pDialog->currentState)) {
        DIALOG_Stop2xx(pDialog);
    
        if (NULL != pDialog->pCancel) {
            /* The dialog is already terminated by user, now send BYE. */
            hTransport = TRANS_GetTransport(pDialog->hTrans);
            /* 
             * Then add a user to this transport.  We are 
             * going to use the same transport as the INVITE
             */
            if (UA_SendRequest(pDialog->hOwner, NULL, 
                    pDialog, pDialog->pCancel, UAC_Bye, hTransport, NULL) !=
                    SIP_OK) {
                SIP_freeMsg(pDialog->pCancel);
            }
            pDialog->pCancel = NULL;
            /* now kill the dialog */
            DIALOG_Destroy(pDialog);
            return;
        }
        /* Then we have to do a bye here */
        if (NULL == (pMsg = SIP_allocMsg())) {
            return;
        }

        pMsg->msgType = eSIP_REQUEST;
        pMsg->method = eSIP_BYE;
       
        /* send it through the state machine */
        if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }

        UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL,
                (tSipHandle)eSIP_RSP_CODE_ACK_TIMEOUT);
    }
}

/* 
 *****************************************************************************
 * ================UA_OkayRetryCB()===================
 *
 * When an INVITE response is receeved by a UAS and the UA ansers with a 200
 * the 200 is continually sent until an ACK is recieved.  This function
 * is the callback function called by the timer mechanism that continually
 * sends the 200 until that ACK is received.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a dialog object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_OkayRetryCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tSipDialog *pDialog = (tSipDialog*)pArg;
    tDialog2xx *pOk;

    pOk = &pDialog->ok;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_OkayRetryCB: calling callback ", 0, 0, 0);

    if (DIALOG_IS_CONFIRMED(pDialog->currentState)) {
        /* update the next interval t1 * 2 */
        pOk->t1 <<= 1;
        if (pOk->t1 > pOk->retryMax) {
            pOk->t1 = pOk->retryMax;
        }
        /* send the retry and start timer again */
        if (TRANSPORT_Send(pOk->hTransport, pOk->p2xxMsg) != SIP_OK) {
            DIALOG_Stop2xx(pDialog);    
        }
        else {
            SIPTIMER_Start(hOwner, UA_OkayRetryCB, pArg, pOk->t1, FALSE);
        }
    }
}

/* 
 *****************************************************************************
 * ================UA_PrackTimeoutCB()===================
 *
 * When an provisional response to an invite is sent reliably, then the 
 * dialog will resend the provisional response until a PRACK is received.
 * This function is the callback function called by the timer mechanism
 * that the resending of the provisional response timed out.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a dialog object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_PrackTimeoutCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tSipDialog   *pDialog = (tSipDialog*)pArg;
    tSipIntMsg   *pMsg;
    tDialogPrack *pPrack;

    pPrack = &pDialog->prack;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_PrackTimeoutCB: calling callback ", 0, 
            0, 0);

    if ((pPrack->respCode != 0) && pPrack->pMsg) {       
        /* then we timed out, we must return 5xx per RFC3262 section 3 */
        pMsg = pPrack->pMsg;
        pPrack->pMsg = NULL;
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_SERVER_TIMEOUT);
            
        /* send it through the state machine */
        if (UASM_DialogServer(pDialog, pMsg, pDialog->hTrans) != SIP_OK) {
            SIP_freeMsg(pMsg);
        }

        UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL, NULL);
    }
}

/* 
 *****************************************************************************
 * ================UA_PrackRetryCB()===================
 *
 * When an provisional response to an invite is sent reliably, then the 
 * dialog will resend the provisional response until a PRACK is received.
 * This function handles the resending of the provisional response.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a dialog object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_PrackRetryCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tSipDialog   *pDialog = (tSipDialog*)pArg;
    tSipIntMsg   *pMsg;
    tDialogPrack *pPrack;

    pPrack = &pDialog->prack;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_PrackRetryCB: calling callback ", 0, 0, 0);

    if (pPrack->respCode != 0 && pPrack->pMsg) {
        /* Then we need to send another provisional response */
        pPrack->t1 <<= 1;

        /* make a new copy of the message */
        if (NULL != (pMsg = SIP_copyMsg(pPrack->pMsg))) {
            if (TU_SendResponse(pMsg, pDialog->hTrans) != SIP_OK) {
                SIP_freeMsg(pMsg);
            }
        }
        SIPTIMER_Start(hOwner, UA_PrackRetryCB, pArg, pPrack->t1, FALSE);
    }
}

/* 
 *****************************************************************************
 * ================UA_SubscriberTimeoutCB()===================
 *
 * This function is used when the timer used for active subscriptions
 * (at the subscriber end a.k.a. the client sending the subscribe message)
 * expires.  If it does expire then the subscription is removed and the 
 * application is notified.  If there are no more subscriptions then
 * the application is sent an additional event.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a tDialogEvent that owns this timer.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_SubscriberTimeoutCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tDialogEvent *pEvent = (tDialogEvent*)pArg;
    tSipDialog   *pDialog;
    tSipIntMsg   *pMsg;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_SubscriberTimeoutCB: calling callback ",
            0, 0, 0);

    pDialog = pEvent->pDialog;
    
    /* 
     * Make sure that the payload related stuff is not included, it's not 
     * needed when we 'refresh'.
     */
    HF_Delete(&pDialog->pHFList, eSIP_CONTENT_DISP_HF);
    HF_Delete(&pDialog->pHFList, eSIP_CONTENT_LENGTH_HF);
    HF_Delete(&pDialog->pHFList, eSIP_CONTENT_TYPE_HF);

    /* Create another pMsg and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        /* 
         * Can't do it, remove the event and report that 
         * the subscription is over 
         */
        UASM_SubscribeFailed(pDialog, (tSipIntMsg *)0, pEvent->hTransaction);
        return;
    }

    pMsg->method = eSIP_SUBSCRIBE;
    pMsg->msgType = eSIP_REQUEST;

    /* 
     * Copy the known header fields that were cached when the 
     * dialog was created 
     */
    if (SIP_OK != HF_CopyAll(&pMsg->pHFList, pDialog->pHFList,
            &pMsg->x.ECPresenceMasks)) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_SubscriberTimeoutCB: Failed in copying HF", 0, 0, 0);
        UASM_SubscribeFailed(pDialog, (tSipIntMsg *)0, pEvent->hTransaction);
        return;
    }
    
    /* Copy the event */
    pMsg->Event = pEvent->evt;

    /* Copy the timeout value ("Expires" HF) */
    pMsg->Expires = pEvent->timeout;

    /* run it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_SubscriberTimeoutCB: Failed in dialog FSM", 0, 0, 0);
        SIP_freeMsg(pMsg);
        UASM_SubscribeFailed(pDialog, (tSipIntMsg *)0, pEvent->hTransaction);
    }
    return;
}

/* 
 *****************************************************************************
 * ================UA_OkayRecvCB()===================
 *
 * This timer call back is used to clean up the client 2xx retry mechanisms 
 * in the dialog.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a dialog object.
 *
 * id = A value past in when the timer expiry fires.  
 *      It's used to identify 'old' timer events.
 *           
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_OkayRecvCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tSipDialog *pDialog = (tSipDialog*)pArg;
    
    UNUSED(hOwner);

    DIALOG_Stop2xx(pDialog);
}

void UA_SessionRefresherCB(
    tSipHandle hOwner,
    void      *pArg)
{
    tSipIntMsg  *pMsg;
    tSipDialog *pDialog = (tSipDialog*)pArg;
    UNUSED(hOwner);

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_SessionTimerCB: callback kicked for hDialog:%X",
            (int)pDialog, 0, 0);

    if (NULL == (pMsg = SIP_allocMsg())) {
        return;
    }

    pMsg->msgType = eSIP_REQUEST;
    pMsg->method = eSIP_UPDATE;

    /* send it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_SessionTimerCB: Failed to send UPDATE as a session refresh hDialog:%X",
            (int)pDialog, 0, 0);
        SIP_freeMsg(pMsg);
    }
    return;
}

void UA_SessionExpiryCB(
    tSipHandle hOwner,
    void      *pArg)
{
    tSipIntMsg  *pMsg;
    tSipDialog *pDialog = (tSipDialog*)pArg;
    UNUSED(hOwner);

    SIP_DebugLog(SIP_DB_UA_LVL_3, 
            "UA_SessionExpiryCB: callback kicked for hDialog:%X",
            (int)pDialog, 0, 0);

    if (NULL == (pMsg = SIP_allocMsg())) {
        return;
    }

    UA_AppEvent(pDialog->hOwner, pDialog, eUA_CALL_DROP, NULL, NULL);

    pMsg->msgType = eSIP_REQUEST;
    pMsg->method = eSIP_BYE;

    /* send it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_SessionTimerCB: Failed to end a session that did'nt efresh hDialog:%X",
            (int)pDialog, 0, 0);
        SIP_freeMsg(pMsg);
    }

    return;
}

/* 
 *****************************************************************************
 * ================UA_ReRegisterCB()===================
 *
 * When a UA is registered it will continually retry to register with a proxy
 * as a solution to a "keep alive" mechanism.  This callback is kicked
 * by a timer when it's time to re-register
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a tUaReg object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_ReRegisterCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tUaReg     *pReg = (tUaReg*)pArg;
    tSipIntMsg *pMsg;

    UNUSED(hOwner);

    pReg->isBusy = TRUE;
    pReg->hasTried = 0;
    pMsg = pReg->pMsg;
    pMsg->CSeq.seqNum++;
    /* make a new branch */
    HF_MakeViaBranch(&pMsg->ViaList, NULL);

    /* set the register expiry.  It may have changed after every re-register */
    pMsg->Expires = pReg->reRegInterval;
   
    if (NULL == (pReg->pMsg = SIP_copyMsg(pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_ReRegisterCB: Failed to cooy pMsg", 0, 0, 0);
        UA_CleanRegistration(pReg);
        UA_AppEvent(pReg->pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                (tSipHandle)eSIP_RSP_CODE_INTERNAL_ERROR);
        SIP_freeMsg(pMsg);
    }

    /* Pass it to the TU */
    if (UA_SendRequest(pReg->pUa, &pMsg->requestUri, pArg, pMsg, UAC_Register,
                NULL, &pReg->hTransaction) != SIP_OK) {
        UA_CleanRegistration(pReg);
        UA_AppEvent(pReg->pUa, NULL, eUA_REGISTRATION_FAILED, pMsg,
                (tSipHandle)eSIP_RSP_CODE_UNKNOWN);
        SIP_freeMsg(pMsg);
    }
    return;
}

/* 
 *****************************************************************************
 * ================ UA_DialogKeepalivesCB() ===================
 *
 * This results in a UA thinking it's registered however the proxy can not
 * communicate with the UA due to a stale NAT mapping.
 * This callback is used to periodically send a dummy packet
 * to refresh the NAT mapping.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a tSipKeepalives object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_KeepalivesCB(
    tSipHandle hOwner,
    void      *pArg)
{
    tSipKeepalives *pKeep = (tSipKeepalives*)pArg;
    vint size;
    UNUSED(hOwner);

    size = sizeof(SIP_DUMMY_PACKET) - 1;
    /* Send a dummy packet */
    OSAL_netSocketSendTo(&pKeep->refreshFd, SIP_DUMMY_PACKET, &size,
            &pKeep->refreshAddr);
    return;
}

/* 
 *****************************************************************************
 * ================UA_RefreshPublishCB()===================
 *
 * When a UA sends an "initial" PUBLISH request it will periodically refresh 
 * the publication.  This callback is kicked by a timer when it's time to 
 * refresh.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a tUaPub object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_RefreshPublishCB(
    tSipHandle hOwner, 
    void      *pArg)
{
    tUaPub         *pPub = (tUaPub*)pArg;
    tSipIntMsg     *pMsg;

    UNUSED(hOwner);

    pPub->isBusy = TRUE;
    pPub->hasTried = FALSE; 
    pPub->pMsg->CSeq.seqNum++;
    /* make a new branch */
    HF_MakeViaBranch(&pPub->pMsg->ViaList, NULL);
    /* make a new 'from' 'tag' */
    HF_GenerateTag(pPub->pMsg->From.szTag);
    /* make a new callId */
    HF_GenerateCallId(NULL, pPub->pMsg->szCallId);

    
    if (NULL == (pMsg = SIP_copyMsg(pPub->pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_RefreshPublishCB: Failed to cooy pMsg", 0, 0, 0);
        UA_AppEvent(pPub->pUa, NULL, eUA_PUBLISH_FAILED, pMsg, 
                pPub->hTransaction);
        SIP_freeMsg(pMsg);
        UA_CleanPublish(pPub);
    }

    /* Pass it to the TU */
    if (UA_SendRequest(pPub->pUa, &pMsg->requestUri, pArg, pMsg,
            UAC_PublishNoDialog, NULL, NULL) != SIP_OK) {
        UA_AppEvent(pPub->pUa, NULL, eUA_PUBLISH_FAILED, pMsg, 
                pPub->hTransaction);
        SIP_freeMsg(pMsg);
        UA_CleanPublish(pPub);
    }
    return;
}

/* 
 *****************************************************************************
 * ================UA_InviteExpiryCB()===================
 *
 * UAS should send 487(Request Terminated) when timeout. RFC3261 13.3.1.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a tSipDialog object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_InviteExpiryCB(
    tSipHandle hOwner,
    void      *pArg)
{
    tSipIntMsg *pMsg;
    tSipDialog *pDialog = (tSipDialog*)pArg;
    UNUSED(hOwner);

    SIP_DebugLog(SIP_DB_UA_LVL_3, 
            "UA_InviteExpiryCB: callback kicked for hDialog:%X",
            (int)pDialog, 0, 0);

    if (NULL == (pMsg = SIP_allocMsg())) {
        return;
    }

    UA_AppEvent(pDialog->hOwner, pDialog, eUA_CALL_DROP, NULL, NULL);

    pMsg->msgType     = eSIP_RESPONSE;
    pMsg->CSeq.method = eSIP_INVITE;

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);

    MSGCODE_Create(pMsg, NULL, eSIP_RSP_REQUEST_TERMINATED);

    /* send it through the state machine */
    if (UASM_DialogServer(pDialog, pMsg, pDialog->hTrans) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_InviteTimerCB: Failed FSM hDialog:%X", (int)pDialog, 0, 0);
        SIP_freeMsg(pMsg);
    }
}

/* 
 *****************************************************************************
 * ================UA_reqPendingExpiryCB()===================
 *
 * UAC should send request again, if get 491 request pending of previouse
 * request. This is defined in RFC 3261 14.1.
 *
 * hOwner  = A handle to the timer object that is kicking this function.
 *
 * pArg = a pointer to a tSipDialog object.
 *
 * RETURNS: Nothing
 ******************************************************************************
 */
void UA_reqPendingExpiryCB(
    tSipHandle hOwner,
    void      *pArg)
{
    tSipDialog *pDialog = (tSipDialog*)pArg;

    if (NULL == pDialog->pReInviteUpdate) {
        return;
    }

    pDialog->pReInviteUpdate->CSeq.seqNum++;

    /* send it through the state machine */
    if (UASM_DialogClient(pDialog, pDialog->pReInviteUpdate, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_reqPendingExpiryCB: Failed FSM hDialog:%X", (int)pDialog, 0, 0);
        SIP_freeMsg(pDialog->pReInviteUpdate);
    }
}

