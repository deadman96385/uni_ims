/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include "sip_sip.h"
#include "sip_clib.h"
#include "sip_mem.h"
#include "sip_timers.h"
#include "sip_msgcodes.h"
#include "sip_auth.h"
#include "sip_dbase_sys.h"
#include "sip_dbase_endpt.h"
#include "sip_xport.h"
#include "sip_xact.h"
#include "sip_tu.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_ua.h"
#include "sip_app.h"
#include "sip_ua_client.h"
#include "sip_ua_server.h"

#include "_sip_helpers.h"
#include "_sip_callback.h"
#include "_sip_fsm.h"
#include "sip_parser_enc.h"

/* *************************************************************
 * NOTE about naming convention of static functions. 
 * Function with "Conf" refers to the "Confirmed" dialog state. 
 * Function with "Erly" refers to the "Early" dialog state.
 * These are terms used in section 12 of RFC3261 
 ***************************************************************
 */

static vint _UASM_ErlyClientByeReq       (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyClientInviteReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyClientUpdateReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyClientSubscribeReq (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyClientInviteResp   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyClientSubscribeResp(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyClientUpdateResp   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);

static vint _UASM_ConfClientAckReq      (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientByeReq      (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientInviteReq   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientSubscribeReq(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientReferReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientNotifyReq   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientMessageReq  (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientPublishReq  (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientInfoReq     (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientUpdateReq     (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientMessageResp (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientPublishResp (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientInfoResp    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientInviteResp  (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientSubscribeResp(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientReferResp   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientNotifyResp  (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfClientUpdateResp  (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerOtherReq     (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerCancelReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerInviteReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerSubscribeReq (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerNotifyReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerInviteResp   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerSubscribeResp(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerUpdateReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerUpdateResp   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ErlyServerPrackResp    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);

static vint _UASM_ConfServerAckReq       (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerCancelReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerByeReq       (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerInviteReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerSubscribeReq (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerMessageReq   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerPublishReq   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerNotifyReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerReferReq     (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerInfoReq      (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerSubscribeResp(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerInviteResp   (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfServerUpdateReq    (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);

static vint _UASM_ServerPrackReq         (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ClientPrackReq         (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ClientPrackResp        (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ServerOptionsReq       (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ConfBusyServerInviteReq(tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ClientOptionsReq       (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);
static vint _UASM_ClientOptionsResp      (tSipDialog *pDialog, tSipIntMsg *pMsg, tSipHandle hTrans);

static tpfUASM_Server _UASM_ServerTable[2][eSIP_DIALOG_LAST_STATE][eSIP_LAST_METHOD];
static tpfUASM_Client _UASM_ClientTable[2][eSIP_DIALOG_LAST_STATE][eSIP_LAST_METHOD];

/* 
 *****************************************************************************
 * ================UASM_Init()===================
 *
 * This function initializes the Finite State Machine used for the dialog 
 * handling.  this function needs to be called when the stack is initialized.
 * This function will initalize the state machine table of functions.
 *
 * If you add a method or need to add handling of an event in a certain state 
 * the function used to handle that needs to be added here.
 *
 * There are two tables and three dimensions to the table of state machine 
 * handlers.  There is one table for client side activity and 
 * one for server activity.
 * then the dimensions of the tables are message type (REQUEST\RESPONSE)
 *
 * RETURNS: 
 *      Nothing
 ******************************************************************************
 */
void UASM_Init(void)
{
    
    tpfUASM_Server *pServer;
    tpfUASM_Client *pClient;

    OSAL_memSet(_UASM_ServerTable, 0, sizeof(_UASM_ServerTable));
    
    /* init the server table */
    pServer = _UASM_ServerTable[eSIP_REQUEST][eSIP_DIALOG_SERVER_EARLY_STATE];
    pServer[eSIP_INVITE]    = _UASM_ErlyServerInviteReq;
    pServer[eSIP_SUBSCRIBE] = _UASM_ErlyServerSubscribeReq;
    pServer[eSIP_ACK]       = _UASM_ErlyServerOtherReq;
    pServer[eSIP_CANCEL]    = _UASM_ErlyServerCancelReq;
    pServer[eSIP_OPTIONS]   = _UASM_ServerOptionsReq; /* same regardless of state */
    pServer[eSIP_REGISTER]  = _UASM_ErlyServerOtherReq;
    pServer[eSIP_BYE]       = _UASM_ErlyServerOtherReq;
    pServer[eSIP_NOTIFY]    = _UASM_ErlyServerNotifyReq;
    pServer[eSIP_REFER]     = _UASM_ErlyServerOtherReq;
    pServer[eSIP_PRACK]     = _UASM_ServerPrackReq;
    pServer[eSIP_UPDATE]    = _UASM_ErlyServerUpdateReq;
    pServer[eSIP_ERROR]     = _UASM_ErlyServerOtherReq;
    
    pServer = _UASM_ServerTable[eSIP_RESPONSE][eSIP_DIALOG_SERVER_EARLY_STATE];
    pServer[eSIP_INVITE]   = _UASM_ErlyServerInviteResp;
    pServer[eSIP_SUBSCRIBE]= _UASM_ErlyServerSubscribeResp;
    pServer[eSIP_UPDATE]   = _UASM_ErlyServerUpdateResp;
    pServer[eSIP_PRACK]    = _UASM_ErlyServerPrackResp;

    pServer = _UASM_ServerTable[eSIP_REQUEST][eSIP_DIALOG_CONFIRMED_STATE];
    pServer[eSIP_INVITE]   = _UASM_ConfServerInviteReq;
    pServer[eSIP_SUBSCRIBE]= _UASM_ConfServerSubscribeReq;
    pServer[eSIP_ACK]      = _UASM_ConfServerAckReq;
    pServer[eSIP_CANCEL]   = _UASM_ConfServerCancelReq;
    pServer[eSIP_OPTIONS]  = _UASM_ServerOptionsReq; /* same regardless of state */
    pServer[eSIP_BYE]      = _UASM_ConfServerByeReq;
    pServer[eSIP_INFO]     = _UASM_ConfServerInfoReq;
    pServer[eSIP_NOTIFY]   = _UASM_ConfServerNotifyReq;
    pServer[eSIP_REFER]    = _UASM_ConfServerReferReq;
    pServer[eSIP_MESSAGE]  = _UASM_ConfServerMessageReq;
    pServer[eSIP_PRACK]    = _UASM_ServerPrackReq;
    pServer[eSIP_PUBLISH]  = _UASM_ConfServerPublishReq;
    pServer[eSIP_UPDATE]   = _UASM_ConfServerUpdateReq;

    pServer = _UASM_ServerTable[eSIP_REQUEST][eSIP_DIALOG_CONFIRMED_BUSY_STATE];
    pServer[eSIP_INVITE]   = _UASM_ConfBusyServerInviteReq; 
    pServer[eSIP_UPDATE]   = _UASM_ConfBusyServerInviteReq; /* Do the same thing as invite here */
    pServer[eSIP_CANCEL]   = _UASM_ConfServerCancelReq; 
    pServer[eSIP_OPTIONS]  = _UASM_ServerOptionsReq; /* same regardless of state */
    pServer[eSIP_BYE]      = _UASM_ConfServerByeReq;
    pServer[eSIP_INFO]     = _UASM_ConfServerInfoReq;
    pServer[eSIP_NOTIFY]   = _UASM_ConfServerNotifyReq;
    pServer[eSIP_REFER]    = _UASM_ConfServerReferReq;
    pServer[eSIP_PRACK]    = _UASM_ServerPrackReq;
    pServer[eSIP_UPDATE]   = _UASM_ConfServerUpdateReq;

    pServer = _UASM_ServerTable[eSIP_RESPONSE][eSIP_DIALOG_CONFIRMED_BUSY_STATE];
    pServer[eSIP_INVITE] = _UASM_ConfServerInviteResp;

    pServer = _UASM_ServerTable[eSIP_RESPONSE][eSIP_DIALOG_CONFIRMED_STATE];
    pServer[eSIP_SUBSCRIBE]= _UASM_ConfServerSubscribeResp;
    pServer[eSIP_UPDATE]   = _UASM_ErlyServerUpdateResp;
    
    pServer = _UASM_ServerTable[eSIP_REQUEST][eSIP_DIALOG_CLIENT_EARLY_STATE];
    pServer[eSIP_OPTIONS]   = _UASM_ServerOptionsReq;
    pServer[eSIP_UPDATE]    = _UASM_ErlyServerUpdateReq;

    pServer = _UASM_ServerTable[eSIP_RESPONSE][eSIP_DIALOG_CLIENT_EARLY_STATE];
    pServer[eSIP_UPDATE]    = _UASM_ErlyServerUpdateResp;

    /* init the client table */
    OSAL_memSet(_UASM_ClientTable, 0, sizeof(_UASM_ClientTable));

    pClient = _UASM_ClientTable[eSIP_REQUEST][eSIP_DIALOG_CLIENT_EARLY_STATE];
    pClient[eSIP_INVITE]    = _UASM_ErlyClientInviteReq;
    pClient[eSIP_SUBSCRIBE] = _UASM_ErlyClientSubscribeReq;
    pClient[eSIP_CANCEL]    = _UASM_ErlyClientByeReq;
    pClient[eSIP_BYE]       = _UASM_ErlyClientByeReq;
    pClient[eSIP_OPTIONS]   = _UASM_ClientOptionsReq; /* same regardless of state */
    pClient[eSIP_PRACK]     = _UASM_ClientPrackReq;
    pClient[eSIP_UPDATE]    = _UASM_ErlyClientUpdateReq;
    
    pClient = _UASM_ClientTable[eSIP_RESPONSE][eSIP_DIALOG_CLIENT_EARLY_STATE];
    pClient[eSIP_INVITE]    = _UASM_ErlyClientInviteResp;
    pClient[eSIP_SUBSCRIBE] = _UASM_ErlyClientSubscribeResp;
    pClient[eSIP_OPTIONS]   = _UASM_ClientOptionsResp;
    pClient[eSIP_PRACK]     = _UASM_ClientPrackResp;
    pClient[eSIP_UPDATE]    = _UASM_ErlyClientUpdateResp;
    
    pClient = _UASM_ClientTable[eSIP_REQUEST][eSIP_DIALOG_SERVER_EARLY_STATE];
    pClient[eSIP_OPTIONS]  = _UASM_ClientOptionsReq;
    pClient[eSIP_UPDATE]  = _UASM_ErlyClientUpdateReq;
        
    pClient = _UASM_ClientTable[eSIP_REQUEST][eSIP_DIALOG_CONFIRMED_STATE];
    pClient[eSIP_INVITE]   = _UASM_ConfClientInviteReq;
    pClient[eSIP_SUBSCRIBE]= _UASM_ConfClientSubscribeReq;
    pClient[eSIP_BYE]      = _UASM_ConfClientByeReq;
    pClient[eSIP_NOTIFY]   = _UASM_ConfClientNotifyReq;
    pClient[eSIP_REFER]    = _UASM_ConfClientReferReq;
    pClient[eSIP_OPTIONS]  = _UASM_ClientOptionsReq; /* same regardless of state */
    pClient[eSIP_MESSAGE]  = _UASM_ConfClientMessageReq;
    pClient[eSIP_INFO]     = _UASM_ConfClientInfoReq;
    pClient[eSIP_PRACK]    = _UASM_ClientPrackReq;
    pClient[eSIP_PUBLISH]  = _UASM_ConfClientPublishReq;
    pClient[eSIP_UPDATE]   = _UASM_ConfClientUpdateReq;
    pClient[eSIP_ACK]      = _UASM_ConfClientAckReq;

    pClient = _UASM_ClientTable[eSIP_RESPONSE][eSIP_DIALOG_CONFIRMED_STATE];
    pClient[eSIP_INVITE]   = _UASM_ConfClientInviteResp;
    pClient[eSIP_SUBSCRIBE]= _UASM_ConfClientSubscribeResp;
    pClient[eSIP_NOTIFY]   = _UASM_ConfClientNotifyResp;
    pClient[eSIP_REFER]    = _UASM_ConfClientReferResp;
    pClient[eSIP_OPTIONS]  = _UASM_ClientOptionsResp;
    pClient[eSIP_MESSAGE]  = _UASM_ConfClientMessageResp;
    pClient[eSIP_INFO]     = _UASM_ConfClientInfoResp;
    pClient[eSIP_PRACK]    = _UASM_ClientPrackResp;
    pClient[eSIP_PUBLISH]  = _UASM_ConfClientPublishResp;
    pClient[eSIP_UPDATE]   = _UASM_ConfClientUpdateResp;

    pClient = _UASM_ClientTable[eSIP_RESPONSE][eSIP_DIALOG_SERVER_EARLY_STATE];
    pClient[eSIP_OPTIONS]  = _UASM_ClientOptionsResp;
    pClient[eSIP_UPDATE]   = _UASM_ErlyClientUpdateResp;

    pClient = _UASM_ClientTable[eSIP_REQUEST][eSIP_DIALOG_CONFIRMED_BUSY_STATE];
    pClient[eSIP_BYE]      = _UASM_ConfClientByeReq;
    pClient[eSIP_NOTIFY]   = _UASM_ConfClientNotifyReq;
    pClient[eSIP_REFER]    = _UASM_ConfClientReferReq;
    pClient[eSIP_OPTIONS]  = _UASM_ClientOptionsReq; /* same regardless of state */
    pClient[eSIP_PRACK]    = _UASM_ClientPrackReq;  
    pClient = _UASM_ClientTable[eSIP_RESPONSE][eSIP_DIALOG_CONFIRMED_BUSY_STATE];
    pClient[eSIP_INVITE]  = _UASM_ConfClientInviteResp;
    pClient[eSIP_NOTIFY]  = _UASM_ConfClientNotifyResp;
    pClient[eSIP_REFER]   = _UASM_ConfClientReferResp;
    pClient[eSIP_OPTIONS] = _UASM_ClientOptionsResp;
    pClient[eSIP_PRACK]   = _UASM_ClientPrackResp;
          
    return;
};

/* 
 *****************************************************************************
 * ================UASM_DialogClient()===================
 *
 * This function is the entry point into the dialog state machine of a UA.
 *
 * pDialog = A pointer to the dialog that we are operating on.
 *
 * pMsg = a pointer a sip message object, this is the internal request.
 *
 * RETURNS: 
 *      SIP_FREE_MEM: Sip message (pMsg) was processed and should be "freed".
 *                      This doesn't mean there was a failure.
 *      SIP_OK:       Message was successfully processed and pMsg should 
 *                      NOT be "freed" by the caller.
 *      SIP_FAILED:   Processing the message failed and the message (pMsg) 
 *                      should be "freed".
 ******************************************************************************
 */
vint UASM_DialogClient(
    tSipDialog  *pDialog, 
    tSipIntMsg  *pMsg,
    tSipHandle   hTransaction)
{
    tpfUASM_Client pfClient;
    tSipMethod method;

    SIP_DebugLog(SIP_DB_UA_LVL_2, "UASM_DialogClient: hDialog:%X hTransaction:%X",
                 (int)pDialog, (int)hTransaction, 0);

    if (pMsg->msgType == eSIP_RESPONSE) {
        method = pMsg->CSeq.method;
    }
    else {
        method = pMsg->method;
    }
    
    pfClient = _UASM_ClientTable[pMsg->msgType][pDialog->currentState][method];
    if (pfClient) {
        return pfClient(pDialog, pMsg, hTransaction);
    }
    else {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "UASM_DialogClient: No action defined for this event hDialog:%X", 
            (int)pDialog, 0, 0);
        return (SIP_FREE_MEM);
    }
}

/* 
 *****************************************************************************
 * ================UASM_DialogServer()===================
 *
 * This function is the entry point into the server dialog state machine of a UA.
 *
 * pDialog = A pointer to the dialog that we are operating on.
 *
 * pMsg = a pointer a sip message object, this is the internal request.
 *
 * hTransaction = A handle to the transaction that was used to recv this pMsg
 *
 * RETURNS: 
 *      SIP_FREE_MEM: Sip message (pMsg) was processed and should be "freed".
 *                      This doesn't mean there was a failure.
 *      SIP_OK:       Message was successfully processed and pMsg should 
 *                      NOT be "freed" by the caller.
 *      SIP_FAILED:   Processing the message failed and the message (pMsg) 
 *                      should be "freed".
 ******************************************************************************
 */
vint UASM_DialogServer(
    tSipDialog  *pDialog, 
    tSipIntMsg  *pMsg, 
    tSipHandle   hTransaction)
{
    tpfUASM_Server pfServer;
    tSipMethod method;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UASM_DialogServer: hDialog:%X pMsg:%X hTransaction:%X", 
            (int)pDialog, (int)pMsg, (int)hTransaction);

    if (pMsg->msgType == eSIP_RESPONSE) {
        method = pMsg->CSeq.method;
    }
    else {
        method = pMsg->method;
    }
    
    pfServer = _UASM_ServerTable[pMsg->msgType][pDialog->currentState][method];
    if (pfServer) {
        return pfServer(pDialog, pMsg, hTransaction);
    }
    else {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "UASM_DialogServer: No action defined for this event hDialog:%X pMsg:%X hTransaction:%X", 
            (int)pDialog, (int)pMsg, (int)hTransaction);
        return (SIP_FREE_MEM);
    }
}

static vint _UASM_ErlyClientInviteReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientInviteReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the route per section 12 */
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest((tUa*)pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Invite, NULL, &pDialog->hTrans);
}

static vint _UASM_ErlyClientUpdateReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientUpdateReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the route per section 12 */
    DIALOG_PopulateRequest(pDialog, pMsg);
    /* Set flag to indicate SDP offer sent. */
    pDialog->isEarlyBusy = OSAL_TRUE;
    return UA_SendRequest((tUa*)pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Update, NULL, NULL);
}

static vint _UASM_ConfClientUpdateReq(
    tSipDialog      *pDialog,
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientUpdateReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the route per section 12 */
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest((tUa*)pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Update, NULL, NULL);
}

static vint _UASM_ErlyClientSubscribeReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tDialogEvent *pEvt;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ErlyClientSubscribeReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    /* populate the route per section 12 */
    DIALOG_PopulateRequest(pDialog, pMsg);

    if (NULL != (pEvt = DIALOG_CacheEvent(pDialog, OSAL_TRUE, &pMsg->Event,
            pMsg->Expires))) {
        return UA_SendRequest((tUa*)pDialog->hOwner,
                DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Subscribe,
                NULL, &pEvt->hTransaction);
    }
    return (SIP_FAILED);
}

static vint _UASM_ClientOptionsReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ClientOptionsReq: hDialog:%X pMsg:%X hTrans:%X", (int)pDialog, (int)pMsg, (int)hTrans);
    /* populate the route per section 12 */
    /* flip on some defaults */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest((tUa*)pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Options, NULL, hTrans);
}

static vint _UASM_ClientPrackReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    /* NOTE, this same function is used for all 
     * prack handling irregardless of state
     */
    vint status;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientPrackReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    
    DIALOG_PopulateRequest(pDialog, pMsg);
    /* Populate the RAck header field */
    pMsg->RAck = pDialog->prack.rack;
    /* flip on the bit */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_RACK_HF);
    status = UA_SendRequest((tUa*)pDialog->hOwner, NULL, pDialog, pMsg, UAC_Prack, NULL, NULL);
    if (status != SIP_OK) {
       DIALOG_CleanPrack(&pDialog->prack);
    }
    return status;
}

static vint _UASM_ConfClientAckReq(
    tSipDialog      *pDialog,
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    vint status;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientAckReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    DIALOG_PopulateRequest(pDialog, pMsg);

    status = UA_SendRequest((tUa*)pDialog->hOwner, NULL, NULL, pMsg, NULL, NULL, NULL);
    if (status != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientAckReq: Fail to send ACK. hDialog:%X pMsg:%X",
                (int)pDialog, (int)pMsg, 0);
        SIP_freeMsg(pMsg);
    }
    return status;
}

static vint _UASM_ErlyClientByeReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    vint status;
    tSipHandle hTransport;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientByeReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* then it's too early for a BYE so change it to a CANCEL */
    pMsg->method = eSIP_CANCEL;
    DIALOG_PopulateRequest(pDialog, pMsg);
    if (pDialog->inviteHasResponse == FALSE) {
        pDialog->pCancel = pMsg;
        /* Stop retry timer */
        TRANS_stopTimerTA(pDialog->hTrans);
        /* 
         * Then you must wait until the invite has a response
         * so don't go through the state machine 
         */
        SIP_DebugLog(SIP_DB_DIALOG_LVL_2, "_UASM_ErlyClientByeReq: CANCELING but waiting for prov resp", 0, 0, 0);
        return (SIP_OK);
    }
    SIP_DebugLog(SIP_DB_DIALOG_LVL_3, "_UASM_ErlyClientByeReq: CANCELING request for Invite hTransaction:%X", 
            (int)pDialog->hTrans, 0, 0);
    TRANS_StartInviteCancelTimer(pDialog->hTrans);
    hTransport = TRANS_GetTransport(pDialog->hTrans);
    status = UA_SendRequest((tUa*)pDialog->hOwner, NULL, pDialog, pMsg, UAC_Cancel, hTransport, NULL);
    DIALOG_Destroy(pDialog);
    return status;
}

static vint _UASM_InitPrack(
    tSipIntMsg   *pMsg, 
    tSipDialog   *pDialog,
    tDialogPrack *pPrack)
{
    
    /* enable the bit to include the RSeq header field */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_RSEQ_HF);
    /* then we are doing prack, check and see if this response exists */
    if (pPrack->respCode == 0) {
        /* it's empty, so use the rseq from the dialog */
        pMsg->RSeq = pDialog->rseq;
        pPrack->respCode = pMsg->code;
        pPrack->rack.seqNum = pDialog->rseq;
        pPrack->rack.cseq.method = pMsg->CSeq.method;
        pPrack->rack.cseq.seqNum = pDialog->remoteSeqNum;
        /* Retry init time is t1 */
        pPrack->t1 = TRANS_getTimerT1();
        pPrack->retryMax = TRANS_getTimerT2();
        /* save a copy of the prack for retransmission */
        pPrack->pMsg = SIP_copyMsg(pMsg);
        if (!pPrack->pMsg) {
            /* clean up */
            OSAL_memSet(pPrack, 0, sizeof(tDialogPrack));
            return (SIP_FAILED);
        }
        /* update the dialog's rseq */
        pDialog->rseq++;
        /* create the retry timer and start it */
        if (NULL == (pDialog->prack.hRetryTimer = 
                SIPTIMER_Create(pDialog->hContext))) {
            /* clean up */
            SIP_freeMsg(pPrack->pMsg);
            OSAL_memSet(pPrack, 0, sizeof(tDialogPrack));
            return (SIP_FAILED);
        }
        else {
            /* timer successful so start it */
            SIPTIMER_Start(pPrack->hRetryTimer, UA_PrackRetryCB, pDialog, pPrack->t1, FALSE);
        }

        /* create the timeout timer and start it */
        if (NULL == (pDialog->prack.hTimeoutTimer = 
                SIPTIMER_Create(pDialog->hContext))) {
            /* clean up */
            SIP_freeMsg(pPrack->pMsg);
            OSAL_memSet(pPrack, 0, sizeof(tDialogPrack));
            return (SIP_FAILED);
        }
        else {
            /* timer successful so start it */
            SIPTIMER_Start(pPrack->hTimeoutTimer, UA_PrackTimeoutCB, pDialog, 
                    pPrack->retryMax, FALSE);
        }        
        return (SIP_OK);
    }
    else if (pPrack->respCode == pMsg->code) {
        pMsg->RSeq = pPrack->rack.seqNum;
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

static vint _UASM_ErlyServerInviteResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    vint status = SIP_FAILED;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerInviteResp: hDialog:%X pMsg:%X", 
                 (int)pDialog, (int)pMsg, 0);

    /* change the state */
    DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
    
    if (MSGCODE_ISPROV(pMsg->code)) {
        /* Check if we are attempting Realiable Provisional response (PRACK) */
        if (pMsg->RSeq) {
            if (_UASM_InitPrack(pMsg, pDialog, &pDialog->prack) != SIP_OK) {
                return status;
            }
        }
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
            status = SIP_OK;
        }
    }
    else /* is final response */ {
        /*
         * RFC3261 13.3.1.
         * If this is final response, stop invite expires timer.
         */
        DIALOG_DestroyInviteExpires(pDialog);
        /* kill off any left over and unfinished reliable 
         * provisional responses
         */
        OSAL_memSet(&pDialog->prack, 0, sizeof(tDialogPrack));
        
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
           /* set up the 2xx retry mechanism */
            DIALOG_Start2xx(pDialog, TRANS_GetTransport(pDialog->hTrans), pMsg,
                    UA_OkayRetryCB, UA_OkayTimeoutCB);

            DIALOG_ServerRespSessionExpires(pDialog, pMsg);
            /* Start keep-alives timer if necessary */
            DIALOG_StartKeepalives(pDialog, pMsg);
            
            if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
                status = SIP_OK;
            }
        }
        else {
            /* not good so send response and kill the dialog */
            if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
                status = SIP_OK;
            }
            /* Kill the dialog, no need to notify the app, 
             * the app already knows what response it's returning 
             */
            DIALOG_Destroy(pDialog);
        }
    }
    return status;
}

static vint _UASM_ErlyServerSubscribeResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    tDialogEvent *pEvt;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ErlyServerSubscribeResp: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    /* Verify the event exists */
    if (NULL == (pEvt = DIALOG_SearchEvtByEvt(pDialog, OSAL_FALSE, 
            &pMsg->Event))) {
        /* Event doesn't exist, return error */
        return (SIP_FAILED);
    }
    
    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
        DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
            return (SIP_OK);
        }
        return (SIP_FAILED);
    }
    else {
        /* not good so send response and kill the dialog */
        DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
        TU_SendResponse(pMsg, hTrans);
                
        DLLIST_Remove(&pDialog->notifyList, &pEvt->dll);
        if (DLLIST_IsEmpty(&pDialog->subscribeList) &&
                DLLIST_IsEmpty(&pDialog->notifyList)) {
            DIALOG_FreeEvent(pEvt);
            return (SIP_DONE);
        }
        DIALOG_FreeEvent(pEvt);
    }  
    return (SIP_OK);
}

static vint _UASM_ConfServerSubscribeResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    tDialogEvent *pEvt;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerSubscribeResp: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    /* Verify the event exists */
    if (NULL == (pEvt = DIALOG_SearchEvtByEvt(pDialog, OSAL_FALSE, 
            &pMsg->Event))) {
        /* Event doesn't exist, return error */
        return (SIP_FAILED);
    }
    
    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
            return (SIP_OK);
        }
        return (SIP_FAILED);
    }
    else {
        /* not good so send response and kill the dialog */
        DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
        TU_SendResponse(pMsg, hTrans);
                
        DLLIST_Remove(&pDialog->notifyList, &pEvt->dll);
        if (DLLIST_IsEmpty(&pDialog->subscribeList) &&
                DLLIST_IsEmpty(&pDialog->notifyList)) {
            DIALOG_FreeEvent(pEvt);
            return (SIP_DONE);
        }
        DIALOG_FreeEvent(pEvt);
    }  
    return (SIP_OK);
}

static vint _UASM_ErlyClientInviteResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tSipIntMsg *pReq;
    tSipHandle  hTransport;
    tUaEvtType  evtType;
    UNUSED(hTrans);
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientInviteResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
    }

    /*
     * Process provisonal response except 100Trying. 100Trying will not be sent to 
     * application.
     */
    if (MSGCODE_ISPROV(pMsg->code) && (eSIP_RSP_TRYING != pMsg->code)) {
        /*
         * The branch in the via may have 
         * changed due to fail over.  So 
         * let's update the dialog szBranch. 
         */
        DIALOG_CacheBranch(pDialog,pMsg);

        DIALOG_Activate(pDialog, pMsg);
        
        if (pDialog->pCancel) {
            TRANS_StartInviteCancelTimer(pDialog->hTrans);
            hTransport = TRANS_GetTransport(pDialog->hTrans);
            /* then add a user to this transport.  We are 
             * going to use the same transport as the INVITE
             */
            if (UA_SendRequest(pDialog->hOwner, NULL, 
                    pDialog, pDialog->pCancel, UAC_Cancel, hTransport, NULL) != SIP_OK) {
                SIP_freeMsg(pDialog->pCancel);
            }
            pDialog->pCancel = NULL;
            /* now kill the dialog */
            DIALOG_Destroy(pDialog);
        }
        else {
            pDialog->inviteHasResponse = TRUE;
            /* Update the target URI.  We need this for PRACK & UPDATES. */
            DIALOG_TargetRefresh(pDialog, pMsg);

            /* Now determine the event to generate to the application */ 
            if (pMsg->code == eSIP_RSP_RINGING) {
                evtType = eUA_RINGING;
            }
            else if (pMsg->code == eSIP_RSP_SESSION_PROGRESS) {
                evtType = eUA_SESSION;
            }
            else if (pMsg->code == eSIP_RSP_CALL_IS_BEING_FORW) {
                evtType = eUA_CALL_IS_BEING_FORW;
            }
            else {
                /* it some other provisional response */
                evtType = eUA_RESPONSE;
            }
            /* now see if it's a reliable provision response */
            if (pMsg->RSeq != 0) {
                /* check if we already got one of these reliable 
                 * provisional responses 
                 */
                if ((pDialog->prack.respCode == 0) ||
                            (pMsg->RSeq != pDialog->prack.rack.seqNum)) {
                    /* then it's new so cache its information */
                    pDialog->rseq = pMsg->RSeq;
                    pDialog->prack.respCode = pMsg->code;
                    pDialog->prack.rack.seqNum = pMsg->RSeq;
                    pDialog->prack.rack.cseq = pMsg->CSeq;
                    UA_AppEvent(pDialog->hOwner, pDialog, evtType, pMsg, (tSipHandle)pMsg->code);
                }
            }
            else {
                UA_AppEvent(pDialog->hOwner, pDialog, evtType, pMsg, (tSipHandle)pMsg->code);
            }
        }
    }
    else if (MSGCODE_ISFINAL(pMsg->code)) {
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            /* keep the transport resource alive and start a timer to free it once it expires.  
             * This is used  to receive all 2xx retries */
            DIALOG_Start2xx(pDialog, TRANS_GetTransport(pDialog->hTrans), NULL,
                    NULL, UA_OkayRecvCB);

            /* dialog established */
            DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
            DIALOG_Activate(pDialog, pMsg);
            /* Cache remote target uri if the response is final. */
            DIALOG_TargetRefresh(pDialog, pMsg);

            DIALOG_ClientRespSessionExpires(pDialog, pMsg);
            /* Start keep-alives timer if necessary */
            DIALOG_StartKeepalives(pDialog, pMsg);

            /* see if there is a CANCEL waiting around if so turn it into a BYE and send it.
             * This handles the event when we send an INVITE, then we try to CANCEL but we haven't gotten a 
             * response to the INVITE yet so we are waiting for that, but then a 2xx is immediately
             * returned, so we transitioned to confirmed state, and we never CANCEL on a confirmed 
             * state, rather we change it to a BYE message and handled it like any other BYE.
             */
            if (pDialog->pCancel) {
                SIP_freeMsg(pDialog->pCancel);
                pDialog->pCancel = NULL;
                if (NULL != (pReq = SIP_allocMsg())) {
                    pReq->msgType = eSIP_REQUEST;
                    pReq->method = eSIP_BYE;
                    DIALOG_PopulateRequest(pDialog, pReq);

                    if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                            pDialog, pReq, UAC_Bye, NULL, NULL) != SIP_OK) {
                        SIP_freeMsg(pReq);
                    }
                }
                DIALOG_Destroy(pDialog);
            }
            else {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_ANSWERED, pMsg, (tSipHandle)pMsg->code);
            }
        }
        else {
            /* it's a 3xx-6xx response */
            if (pDialog->pCancel) {
               /* 
                * Cancel should have no effect if a 
                * final response is received
                */
                SIP_freeMsg(pDialog->pCancel);
                pDialog->pCancel = NULL;
                DIALOG_Destroy(pDialog);
                SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ErlyClientInviteResp: CANCEL request not sent because we have a final response anyway",
                     0, 0, 0);
            }
            else {
                if (MSGCODE_ISCHALLENGE(pMsg->code)) {
                    if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
                        /* send new invite with authorization */
                        if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                                pDialog, pReq, UAC_Invite, TRANS_GetTransport(pDialog->hTrans), NULL) != SIP_OK) {
                            SIP_freeMsg(pReq);
                            UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL, NULL);
                            DIALOG_Destroy(pDialog);
                        }
                    }
                    else {
                        UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL, NULL);
                        DIALOG_Destroy(pDialog);
                    }
                }
                else {
                    SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ErlyClientInviteResp: "
                            "Warning non-OK response %d", pMsg->code, 0, 0);
                    UA_AppEvent(pDialog->hOwner, pDialog, eUA_RESPONSE, pMsg,
                            (tSipHandle)pMsg->code);
                    DIALOG_Destroy(pDialog);
                }
            }
        }
    }
    else if (eSIP_RSP_TRYING == pMsg->code) {
        pDialog->inviteHasResponse = TRUE;
    }

    return (SIP_FREE_MEM);
}

static vint _UASM_ErlyClientUpdateResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tSipIntMsg *pReq;
    
    UNUSED(hTrans);
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientUpdateResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
        /* Received SDP answer, clear flag. */
        pDialog->isEarlyBusy = OSAL_FALSE;
    }

    if (MSGCODE_ISFINAL(pMsg->code)) {
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            DIALOG_Activate(pDialog, pMsg);
            /*
             * ZK: cache remote target uri if the response is final.
             * Note: we should not cache the remote target uri for provisional
             * responses as above.
             */
            DIALOG_TargetRefresh(pDialog, pMsg);
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_UPDATE_COMPLETED, pMsg, (tSipHandle)pMsg->code);
        }
        else {
            /* it's a 3xx-6xx response */
            if (MSGCODE_ISCHALLENGE(pMsg->code)) {
                if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
                    /* send new invite with authorization */
                    if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                            pDialog, pReq, UAC_Update, NULL, NULL) == SIP_OK) {
                        return (SIP_FREE_MEM);
                    }
                    else {
                        SIP_freeMsg(pReq);
                    }
                }
            }
            else if (eSIP_RSP_REQUEST_PENDING == pMsg->code) {
                /* RFC 3261 14.1: create a timer T to send UPDATE again. */
                DIALOG_ClientReqPendingRetry(pDialog);
                return (SIP_FREE_MEM);
            }
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_UPDATE_FAILED, pMsg, (tSipHandle)pMsg->code);
        }
    }
    return (SIP_FREE_MEM);
}


/*
 * ======== _UASM_ErlyClientSubscribeResp() ========
 * User Agent (Client) State Machine's handler for responses to SUBSCRIBE
 * messages sent prior to establishment of a Dialog (i.e.-Early State).
 *
 * pDialog : ISIP_Message object received from ISI containing the
 *           details about the command.
 *
 * pMsg    : pointer to the internal SIP message object that was received.
 *
 * hTrans  :
 *
 * Returns:
 *  SIP_FREE_MEM
 */

static vint _UASM_ErlyClientSubscribeResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tSipIntMsg    *pReq;
    tDialogEvent  *pEvt;
    vint           timeout;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ErlyClientSubscribeResp: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    if (pMsg != 0) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "_UASM_ErlyClientSubscribeResp: pMsg->code:%d",
                (int)pMsg->code, (int)0, 0);
    }
    
    /* Verify that the event is part of this dialog */
    if (NULL == (pEvt = DIALOG_SearchEvtByTrans(pDialog, OSAL_TRUE, hTrans))) {
        /* Quietly ignore this response */
        return (SIP_FREE_MEM);
    }

    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        /* Dialog established. Update the dialog state */
        DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
        DIALOG_Activate(pDialog, pMsg);
        DIALOG_TargetRefresh(pDialog, pMsg);
            
        /* Set up the retry mechanism */
        if (HF_PresenceExists(&pMsg->x.DCPresenceMasks, eSIP_EXPIRES_HF)) {
            if (pMsg->Expires < pEvt->timeout) {
                pEvt->timeout = pMsg->Expires;
            }
        }
        /* Then start timer */
        if (pEvt->timeout != 0) {
            timeout = UA_GetTimerMs(pEvt->timeout);
            SIPTIMER_Start(pEvt->hTimer, UA_SubscriberTimeoutCB, pEvt,
                    timeout, FALSE);
        }
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_SUBSCRIBE_COMPLETED, pMsg,
                &pEvt->evt);
    }
    else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
        if (NULL == (pReq = DIALOG_Authenticate(pDialog, pMsg))) {
            /* Can't authenticate, so consider event as dead */
            UASM_SubscribeFailed(pDialog, pMsg, hTrans);
            return (SIP_FREE_MEM);
        }
        /* send new subscribe with authorization */
        pReq->Event = pEvt->evt;
        pReq->Expires = pEvt->timeout;
        HF_SetPresence(&pReq->x.ECPresenceMasks, eSIP_EVENT_HF);
        HF_SetPresence(&pReq->x.ECPresenceMasks, eSIP_EXPIRES_HF);
        if (SIP_OK != UA_SendRequest(pDialog->hOwner,
                DIALOG_GetUri(pDialog, pReq), pDialog, pReq, UAC_Subscribe,
                NULL, &pEvt->hTransaction)) {
            SIP_freeMsg(pReq);
            UASM_SubscribeFailed(pDialog, pMsg, pEvt->hTransaction);
        }
    }
    else {
        SIP_DebugLog(SIP_DB_UA_LVL_2,
                "_UASM_ErlyClientSubscribeResp: Warning non-OK response %d",
                pMsg->code, 0, 0);
        UASM_SubscribeFailed(pDialog, pMsg, hTrans);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ClientPrackResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    /* Note, this function is used for all PRACK response 
     * handling irregardless of dialog state
     */
    
    tSipIntMsg *pReq;
    
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyClientPrackResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
    }
    
    if (MSGCODE_ISFINAL(pMsg->code)) {
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            /* If there is sdp info then it must be an 
             * answer to a offer that was in a prack 
             */
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_PRACK_COMPLETED, pMsg, (tSipHandle)pMsg->code);
        }
        else {
            if (MSGCODE_ISCHALLENGE(pMsg->code)) {
                if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
                    if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                                       pDialog, pReq, UAC_Prack, NULL, NULL) != SIP_OK) {
                        SIP_DebugLog(SIP_DB_UA_LVL_1, "_UASM_ErlyClientPrackResp: Failed could not authenticate", 0, 0, 0);
                        UA_AppEvent(pDialog->hOwner, pDialog, eUA_PRACK_FAILED, pMsg, (tSipHandle)pMsg->code);
                        SIP_freeMsg(pReq);
                    }
                }
                else {
                    SIP_DebugLog(SIP_DB_UA_LVL_1, "_UASM_ErlyClientPrackResp: Failed could not authenticate", 0, 0, 0);
                    UA_AppEvent(pDialog->hOwner, pDialog, eUA_PRACK_FAILED, pMsg, (tSipHandle)pMsg->code);
                }
            }
            else {
                SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ErlyClientPrackResp: Warning non-OK response %d", pMsg->code, 0, 0);
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_PRACK_FAILED, pMsg, (tSipHandle)pMsg->code);
            }
        }
    }
    return (SIP_FREE_MEM);
}


static vint _UASM_ErlyServerInviteReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerInviteReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    /* create and set session information if there is some */
    UA_SessionInit((tUa*)pDialog->hOwner, pDialog, pMsg->To.uri.user);
    if (pMsg->pSessDescr) {
        /* Then there's SDP data */
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
    }
    /*
     * If there is also something in the text field,
     * then include it with the tSession object.  It was another part to
     * a multi-part body in the invite.
     */
    if (pMsg->pMsgBody) {
        OSAL_snprintf(pDialog->session.sess.otherPayload, MAX_SESSION_PAYLOAD_SIZE,
                "%s", pMsg->pMsgBody->msg);
    }
    /* Cache any session expires (timers) stuff */
    DIALOG_ServerReqSessionExpires(pDialog, pMsg);

    /* If there is expires in INVITE, start a expires timer. */
    DIALOG_ServerReqInviteExpires(pDialog, pMsg);

    UA_AppEvent(pDialog->hOwner, pDialog, eUA_CALL_ATTEMPT, pMsg, NULL);
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerInviteReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfServerInviteReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    /* Then this is a re-invite */
    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr); 
    }
    /*
     * If there is something in the text field,
     * then include it with the tSession object.  It was another part to
     * a multi-part body in the invite.
     */
    if (pMsg->pMsgBody) {
        OSAL_snprintf(pDialog->session.sess.otherPayload, MAX_SESSION_PAYLOAD_SIZE,
                "%s", pMsg->pMsgBody->msg);
    }

    /* If there is expires in INVITE, start a expires timer. */
    DIALOG_ServerReqInviteExpires(pDialog, pMsg);

    /* Cache session timer */
    DIALOG_ServerReqSessionExpires(pDialog, pMsg);
    /* cache the new via parameter */
    DIALOG_CacheVia(pDialog, pMsg);
    /* check & cache a new remoteUri */
    DIALOG_TargetRefresh(pDialog, pMsg);
    /* Stop re-invite retry timer, because receive new re-invire from others */
    DIALOG_DestroyReqPendingRetry(pDialog);

    /* Handle session update using INVITE. */
    if ((!pMsg->pSessDescr) && (!pMsg->pMsgBody)) { 
        /* No content, consider it as a session update. */
        DIALOG_ServerRespSessionExpires(pDialog, pMsg);
        /* return a error response */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);

        /* populate contact header in response */
        DIALOG_PopulateResponse(pDialog, pMsg);

        /* if we are here then there was an error so we are sending a response */
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
            return (SIP_OK);
        }
        else {
            return (SIP_FREE_MEM);
        }
    }
    else {
        /* change to the busy state */
        DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_BUSY_STATE);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_MEDIA_INFO, pMsg, NULL);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ErlyServerUpdateReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerUpdateReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr); 
        if (pDialog->isEarlyBusy) {
            SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerUpdateReq "
                    "SDP offer sent hDialog:%X pMsg:%X", (int)pDialog,
                    (int)pMsg, 0);
            /* SDP offer sent, respond 491. */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_REQUEST_PENDING);
            if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
                return (SIP_OK);
            }
            return (SIP_FREE_MEM);
        }
    }
    DIALOG_DestroyReqPendingRetry(pDialog);
    /* cache the new via parameter */
    DIALOG_CacheVia(pDialog, pMsg);
    /* check & cache a new remoteUri */
    DIALOG_TargetRefresh(pDialog, pMsg);
    /* change to the busy state */
    UA_AppEvent(pDialog->hOwner, pDialog, eUA_UPDATE, pMsg, NULL);
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerUpdateReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfServerUpdateReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    DIALOG_ServerReqSessionExpires(pDialog, pMsg);
    DIALOG_ServerRespSessionExpires(pDialog, pMsg);

    DIALOG_CacheVia(pDialog, pMsg);
    /* check & cache a new remoteUri */
    DIALOG_TargetRefresh(pDialog, pMsg);

    /* return a error response */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);

    /* populate contact header in response */
    DIALOG_PopulateResponse(pDialog, pMsg);

    /* if we are here then there was an error so we are sending a response */
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    else {
        return (SIP_FREE_MEM);
    }
}

static vint _UASM_ErlyServerSubscribeReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ErlyServerSubscribeReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    /* return an bad event response */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_BAD_EVENT);
    /* send a response */
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }

    /* kill the dialog */
    DIALOG_Destroy(pDialog);
       return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerSubscribeReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerSubscribeReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    /* cache the new via parameter */
    DIALOG_CacheVia(pDialog, pMsg);
    /* check & cache a new remoteUri */
    DIALOG_TargetRefresh(pDialog, pMsg);
    
    /* return an bad event response */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_BAD_EVENT);
    /* sending a response */
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}


static vint _UASM_ErlyServerCancelReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    vint status = SIP_FREE_MEM;
    tSipHandle hInviteTrans;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerCancelReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    /* then try to find the invite transaction */
    hInviteTrans = TRANS_SearchForServerInvite(pMsg);
    if (hInviteTrans) {
        tSipIntMsg *pRespMsg;
        /* prepare 'ok' response for the cancel */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
            /* Now prepare the 'request terminated' for the 
             * original invite */
            if (NULL != (pRespMsg = SIP_allocMsg())) {
                MSGCODE_Create(pRespMsg, NULL, eSIP_RSP_REQUEST_TERMINATED);
                DIALOG_PopulateResponse(pDialog, pRespMsg);
                /* send the response to the invite */
                if (TU_SendResponse(pRespMsg, hInviteTrans) != SIP_OK) {
                    SIP_freeMsg(pRespMsg);
                    /* we must return ok here so the calling 
                     * layers don't think there was something wrong with pMsg
                     */
                    status = SIP_OK;
                }
                else {
                    /* we must return ok here so the calling 
                     * layers don't think there was something wrong with pMsg
                     */
                    status = SIP_OK;    
                }
            }
            else {
                /* we must return ok here so the calling 
                 * layers don't think there was something wrong with pMsg
                 */
                status = SIP_OK;
            }
        }
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_CALL_DROP, NULL, NULL);
        /* kill the dialog */
        DIALOG_Destroy(pDialog);
    }
    else {
        /* return 481 */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK)
            status = SIP_OK;
    }
    return status;
}

static vint _UASM_ErlyServerOtherReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerOtherReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ErlyServerNotifyReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    tDialogEvent *pEvt;
    /* 
     * It is possible to receive a NOTIFY request for an early dialog 
     * before a 200 response is received to the initial subscribe.  
     * See 3.3.4 of RFC3265
     */
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ErlyServerNotifyReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    if (NULL == (pEvt = DIALOG_SearchEvtByEvt(pDialog, OSAL_TRUE,
            &pMsg->Event))) {
        /* Then the event doesn't exist so return response */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        /* override the reason string see 3.2.4 in RFC3265 */
        SIP_copyStringToSipText(SIP_RSP_SUBSCRIPTION_NO_EXIST_STR,
                &pMsg->pReasonPhrase);
    }
    else {
        if (pMsg->SubState.arg == eSIP_SUBS_HF_TERM_ARG) {
            /* must be terminated so remove it */
            DIALOG_RemoveEventByEvt(pDialog, OSAL_TRUE, &pMsg->Event, &pEvt);
            if (DLLIST_IsEmpty(&pDialog->subscribeList) &&
                    DLLIST_IsEmpty(&pDialog->notifyList)) {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT_NO_SUBS,
                        pMsg, &pEvt->evt);
                DIALOG_FreeEvent(pEvt);
                DIALOG_Destroy(pDialog);
            }
            else {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT, pMsg,
                        NULL);
                DIALOG_FreeEvent(pEvt);
            }
        }
        else {
            /* All is well so transition to the dialog state */
            DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
            DIALOG_Activate(pDialog, pMsg);
            DIALOG_TargetRefresh(pDialog, pMsg);
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT, pMsg, NULL);
        }
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    }
    
    /* ditch the contact and the message body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->sipfragCode = (tSipMsgCodes)0;
    pMsg->ContentLength = 0;
        
    if (SIP_OK == TU_SendResponse(pMsg, hTrans)) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ServerPrackReq(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    tDialogPrack *pPrack;

    /* Note, this function is used for all Server 
     * Prack handling irregardless of dialog state 
     */
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ErlyServerPrackReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
    }
    
    /* See if there is an outstanding reliable 
     * provisional response for this PRACK 
     */
    pPrack = &pDialog->prack;
    if (pMsg->RAck.seqNum == pPrack->rack.seqNum &&
            pMsg->RAck.cseq.seqNum == pPrack->rack.cseq.seqNum &&
            pMsg->RAck.cseq.method == pPrack->rack.cseq.method) {
        /* found it! so stop the prack retransmission timer and clean house */
        DIALOG_CleanPrack(pPrack);
        /* 
         * Cache the pMsg for sending response.
         * Sending OK is triggered by application.
         */
        pPrack->pMsg = pMsg;
    }
    else {
        /* then this prack is invalid */
        
        /* ditch the contact and the message body */
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        pMsg->ContentType = eCONTENT_TYPE_NONE;
        pMsg->sipfragCode = (tSipMsgCodes)0;
        pMsg->ContentLength = 0;
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        if (SIP_OK != TU_SendResponse(pMsg, hTrans)) {
            SIP_DebugLog(SIP_DB_UA_LVL_3,
                    "_UASM_ErlyServerPrackReq: PRACK response send failed. "
                    "hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
        }
        return (SIP_FREE_MEM);
    }
        
    /* Send eUA_PRACK to UA */
    UA_AppEvent(pDialog->hOwner, pDialog, eUA_PRACK, pMsg, NULL);
    /* Don't free the pMsg, free it when sent response. */
    return (SIP_OK);
}

static vint _UASM_ConfServerInviteResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    vint status = SIP_FAILED;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfServerInviteResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
    
    if (MSGCODE_ISPROV(pMsg->code)) {
        /* Check if we are attempting Realiable Provisional response (PRACK) */
        if (pMsg->RSeq) {
            if (_UASM_InitPrack(pMsg, pDialog, &pDialog->prack) != SIP_OK) {
                return status;
            }
        }
        if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
            status = SIP_OK;
        }
    }
    else /* is final response */ {
        /*
         * RFC3261 13.3.1.
         * If this is final response, stop invite expires timer.
         */
        DIALOG_DestroyInviteExpires(pDialog);
        
        /* kill off any left over and unfinished reliable 
         * provisional responses
         */
        OSAL_memSet(&pDialog->prack, 0, sizeof(tDialogPrack));
        /* change the state whether the re-invite works or not */
        DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            /* set up the 2xx retry mechanism */
            DIALOG_Start2xx(pDialog, TRANS_GetTransport(pDialog->hTrans), pMsg,
                    UA_OkayRetryCB, UA_OkayTimeoutCB);

            DIALOG_ServerRespSessionExpires(pDialog, pMsg);
            
            if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
                status = SIP_OK;
            }
        }
        else {
            /* not good so send response and kill the dialog */
            if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
                status = SIP_OK;
            }
            /* DO NOT KILL THE DIALOG here. Dialog don't die because
             * a RE-Invite failed.
             */
        }
    }
    return status;
}

/*
 * ======== _UASM_ErlyServerPrackResp() ========
 * Function to process Prack response.
 *
 * Return:
 *  SIP_OK: Done without error.
 *  SIP_FAILED: Failed to process the response.
 */
static vint _UASM_ErlyServerPrackResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ErlyServerPrackResp: hDialog:%X "
            "pMsg:%X", (int)pDialog, (int)pMsg, 0);
    DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return SIP_FAILED;
}

static vint _UASM_ErlyServerUpdateResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg, 
    tSipHandle       hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ServerUpdateResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    DIALOG_PopulateResponse((tSipDialog*)pDialog, pMsg);
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return SIP_FAILED;
}

static vint _UASM_ConfServerByeReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    vint status =  SIP_FREE_MEM;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerByeReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    UA_AppEvent(pDialog->hOwner, pDialog, eUA_CALL_DROP, pMsg, NULL);
    
    /* send response */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        status = SIP_OK;
    }
    /* kill the dialog */
    DIALOG_Destroy(pDialog);
    return status;
}

static vint _UASM_ConfServerInfoReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerInfoReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    UA_AppEvent(pDialog->hOwner, pDialog, eUA_INFO, pMsg, NULL);
    
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    /* get rid of content type and message body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->ContentLength = 0;
    /* Clear out the contact header field */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfBusyServerInviteReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerBusyInviteReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    /* send response */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_REQUEST_PENDING);
    
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ServerOptionsReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
   SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ServerOptionsReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    /* cache the remote sequence number to use in the response */
    pDialog->remoteSeqNum = pMsg->CSeq.seqNum;
    /* send response */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    /* load these per section 11 of RFC3261 */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_ENCODING_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_LANGUAGE_HF);
    DIALOG_PopulateResponse(pDialog, pMsg);
        
    if (SIP_OK == TU_SendResponse(pMsg, hTrans)) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerMessageReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
   
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerMessageReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    /* Notify the app */
    UA_AppEvent(pDialog->hOwner, pDialog, eUA_TEXT_MESSAGE, pMsg, NULL);
    
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    /* get rid of content type and message body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->ContentLength = 0;
        
    DIALOG_PopulateResponse(pDialog, pMsg);
    
    if (SIP_OK == TU_SendResponse(pMsg, hTrans)) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerPublishReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
   
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerPublishReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    /* Notify the app */
    UA_AppEvent(pDialog->hOwner, pDialog, eUA_PUBLISH, pMsg, NULL);
    
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    /* get rid of content type and message body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->ContentLength = 0;
        
    DIALOG_PopulateResponse(pDialog, pMsg);
    
    if (SIP_OK == TU_SendResponse(pMsg, hTrans)) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerNotifyReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    tDialogEvent *pEvt;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfServerNotifyReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);

    /* notifies must include an event */
    if (!HF_PresenceExists(&pMsg->x.DCPresenceMasks, eSIP_EVENT_HF)) {
        /* then the event doesn't exist so return response */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
        /* override the reason string see 3.2.4 in RFC3265 */
        SIP_copyStringToSipText(SIP_RSP_SUBSCRIPTION_NO_EXIST_STR,
                &pMsg->pReasonPhrase);
        goto _UASM_ConfServerNotifyReq_label;
    }
    
    if (OSAL_strcasecmp(pMsg->Event.szPackage, SIP_EVENT_HF_REFER_PKG_STR) ==
            0) {
        /* 
         * Is the NOTIFY is meant for the current REFER? see section 2.4.6 of 
         * RFC3515 
         */
        if (pMsg->Event.szId[0] != 0) {
            if (OSAL_strcmp(pMsg->Event.szId, pDialog->transferor.szId) == 0) {
                /* send response, it's always a 2xx */
                MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
                /* SJP -commented out to fix transfer bug 
                goto _UASM_ConfServerNotifyReq_label; */
            }
        }
        
        /* 
         * See if the transfer is valid, meaning that at least the REFER 
         * transaction was successful 
         */
        if (pDialog->transferor.IsValid) {
            /* check the body message type of the NOTIFY, 
             * if successful then notify app that all is well 
             */
            if (pMsg->sipfragCode == eSIP_RSP_TRYING) {
                /* send response, it's always a 2xx */
                MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
            }
            else if (pMsg->sipfragCode == eSIP_RSP_RINGING || 
                    pMsg->sipfragCode == eSIP_RSP_SESSION_PROGRESS) {
                /* all is well */
                 UA_AppEvent(pDialog->hOwner, pDialog, eUA_TRANSFER_RINGING,
                         pMsg, NULL);
                 MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
            }
            else if (MSGCODE_ISSUCCESS(pMsg->code)) {
                /* all is well */
                 UA_AppEvent(pDialog->hOwner, pDialog, eUA_TRANSFER_COMPLETED,
                         pMsg, NULL);
                 /* send response, it's always a 2xx */
                 MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
            }
            else {
                /* then the transfer was not supported or refused */
                pDialog->transferor.IsValid = FALSE;
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_TRANSFER_FAILED,
                        pMsg, NULL);
                /* send response, it's always a 2xx */
                MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
            }
        }
    }
    else {
        if (NULL == (pEvt = DIALOG_SearchEvtByEvt(pDialog, OSAL_TRUE,
                &pMsg->Event))) {
            /*
             * If we can't find the subscription for the event let's check to
             * see if the event is 'conference'.  Some conference servers
             * send NOTIFY's containing conference info without a subscription.
             * This technically is a violation in the spec but it's not a
             * bad idea on the part of conference bridge vendors.
             */
            if (0 == OSAL_strcasecmp(pMsg->Event.szPackage,
                    SIP_EVENT_HF_CONFERENCE_PKG_STR)) {
                /* All is well so transition to the dialog state */
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT, pMsg, NULL);
                MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
            }
            else {
                /* then the event doesn't exist so return response */
                MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
                /* override the reason string see 3.2.4 in RFC3265 */
                SIP_copyStringToSipText(SIP_RSP_SUBSCRIPTION_NO_EXIST_STR,
                        &pMsg->pReasonPhrase);
            }
            goto _UASM_ConfServerNotifyReq_label;
        }

        if (pMsg->SubState.arg == eSIP_SUBS_HF_TERM_ARG) {
            /* must be terminated so remove it */
            DIALOG_RemoveEventByEvt(pDialog, OSAL_TRUE, &pMsg->Event, &pEvt);
            if (DLLIST_IsEmpty(&pDialog->subscribeList) &&
                    DLLIST_IsEmpty(&pDialog->notifyList)) {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT_NO_SUBS,
                        pMsg, &pEvt->evt);
                DIALOG_FreeEvent(pEvt);
                DIALOG_Destroy(pDialog);
            }
            else {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT, pMsg,
                        NULL);
                DIALOG_FreeEvent(pEvt);
            }
        }
        else {
            /* All is well so transition to the dialog state */
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT, pMsg, NULL);
        }
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_OK);
    }
        
_UASM_ConfServerNotifyReq_label:
    /* ditch the contact and the mesage body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->sipfragCode = (tSipMsgCodes)0;
    pMsg->ContentLength = 0;
       
    if (SIP_OK == TU_SendResponse(pMsg, hTrans)) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerReferReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    vint status =  SIP_FREE_MEM;
    tSipIntMsg *pNotify;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfServerReferReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    if (pDialog->transferee.IsValid == FALSE) {
        pDialog->transferee.IsValid = TRUE;
    }
    else {
        /* Then we received more than one REFER, so we stash the CSeq to 
         * populate later in the NOTIFY's 'Event' Header Field's 'id' parameter
         */
        OSAL_itoa(pMsg->CSeq.seqNum, pDialog->transferee.szId,
                SIP_MAX_BASETEN_NUM_STRING);
    }
    /* send back an immediate NOTIFY with 'Trying' */
    if ((pNotify = SIP_allocMsg()) == NULL) {
        /* send an error back on the response to the REFER */
        MSGCODE_Create(pMsg, NULL, eSIP_RSP_SERVER_INT_ERR);
    }
    else {
        pNotify->msgType = eSIP_REQUEST;
        pNotify->method = eSIP_NOTIFY;
        
        OSAL_strcpy(pNotify->Event.szPackage, SIP_EVENT_HF_REFER_PKG_STR);
        OSAL_strcpy(pNotify->Event.szId, pDialog->transferee.szId);
        HF_SetPresence(&pNotify->x.ECPresenceMasks, eSIP_EVENT_HF);

        pNotify->SubState.arg = eSIP_SUBS_HF_ACTIVE_ARG;
        pNotify->SubState.expires = 0;/*pDialog->transferee.expiration; */
        
        pNotify->ContentType = eCONTENT_TYPE_SIPFRAG;
        HF_SetPresence(&pNotify->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);

        pNotify->sipfragCode = eSIP_RSP_TRYING;
        HF_SetPresence(&pNotify->x.ECPresenceMasks, eSIP_SUB_STATE_HF);
        
        DIALOG_PopulateRequest(pDialog, pNotify);
        status = UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pNotify), 
                pDialog, pNotify, UAC_Notify, NULL, NULL);
        if (status != SIP_OK) {
            SIP_freeMsg(pNotify);
            /* send an error back on the response to the REFER */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_SERVER_INT_ERR);
        }
        else {
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_TRANSFER_ATTEMPT, pMsg, NULL);
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_ACCEPTED);
            /* clear out some of the header fields in the request 
             * that don't need to be in the response 
             */
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_REFER_TO_HF);
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_REFERRED_BY_HF);
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);
        }
    }
    
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        status = SIP_OK;
    }
    return status;
}

static vint _UASM_ConfServerCancelReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfServerCancelReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    UA_AppEvent(pDialog->hOwner, pDialog, eUA_CANCELED, pMsg, NULL);
    DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
    /* return 481 */
    MSGCODE_Create(pMsg, NULL, eSIP_RSP_CALL_TRANS_NO_EXIST);
    if (TU_SendResponse(pMsg, hTrans) == SIP_OK) {
        return (SIP_OK);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfServerAckReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg, 
    tSipHandle    hTrans)
{
    tSipHandle hTransport;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfServerAckReq:hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    if (SIP_OK == DIALOG_Stop2xx(pDialog)) {
        if (NULL != pDialog->pCancel) {
            SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ConfServerAckReq. "
                    "Got ACK but the dialog is terminated, send BYE",
                    0, 0, 0);
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
            return (SIP_FREE_MEM);
        }
        /*
         * If "DIALOG_Stop2xx" returned okay then we stopped the timer.
         * This means that this ACK is fresh.  If we get an ACK and the
         * timer has already been stopped then it means the ACK is a
         * retransmission, and we should not give it to the application but
         * instead quietly discard.
         */
        SESSION_ResetIsNew(&pDialog->session);
        if (pMsg->pSessDescr) {
            /* then this must be an answer to an offer we provided in the 200OK */
            SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
        }
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_ACK, pMsg, NULL);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfClientInviteResp(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    tSipIntMsg *pReq;

    SESSION_ResetIsNew(&pDialog->session);
    if (pMsg->pSessDescr) {
        SESSION_Decode(&pDialog->session, pMsg->pSessDescr);
    }
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientInviteResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        tSipIntMsg *pAck;

        DIALOG_ClientRespSessionExpires(pDialog, pMsg);

        /* Refresh the target */
        DIALOG_TargetRefresh(pDialog, pMsg);

        /* Send an ACK back, we dealloc and realloc to clean up the msg
         * but copy the via list first. 
         */
        if (NULL == (pAck = SIP_allocMsg()))
            return (SIP_FREE_MEM);

        pAck->msgType = eSIP_REQUEST;
        pAck->method = eSIP_ACK;
        DIALOG_PopulateRequest(pDialog, pAck);
        /* 
         * This ack is not associated with a transaction so pass NULL
         * as the hTrans parameter in UA_SendRequest
         */
        
        if (!hTrans) {
            /* If hTrans is NULL then this is a resent 200ok so just 
             * send the ACK to the transport layer */
            if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pAck), 
                               NULL, pAck, NULL, NULL, NULL) != SIP_OK) {
                SIP_freeMsg(pAck);
            }
        }
        else {
            DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
            
            /* send it to the transport layer */
            if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pAck), 
                    NULL, pAck, NULL, NULL, NULL) != SIP_OK) {
                SIP_freeMsg(pAck);
            }
            else {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_MEDIA_INFO, pMsg, (tSipHandle)pMsg->code);
            }
        }
    }
    else if (MSGCODE_ISFINAL(pMsg->code)){
        if (MSGCODE_ISCHALLENGE(pMsg->code)) {
            if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
                /* send new invite with authorization */
                if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq),
                        pDialog, pReq, UAC_Invite, TRANS_GetTransport(pDialog->hTrans), NULL) != SIP_OK) {
                    SIP_freeMsg(pReq);
                    UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL, NULL);
                    DIALOG_Destroy(pDialog);
                }
            }
            else {
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_ERROR, NULL, NULL);
                DIALOG_Destroy(pDialog);
            }
            return (SIP_FREE_MEM);
        }
        else if (eSIP_RSP_REQUEST_PENDING == pMsg->code) {
            /* RFC 3261 14.1: create a timer T to send re-INVITE again. */
            DIALOG_ClientReqPendingRetry(pDialog);
            DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
            return (SIP_FREE_MEM);
        }
        else if ((eSIP_RSP_CALL_TRANS_NO_EXIST == pMsg->code) ||
                (eSIP_RSP_REQUEST_TIMEOUT == pMsg->code)) {
            /*
             * RFC 3261 12.2.1.2 and 14.1
             * If the response for a request within a dialog is
             * a 481 (Call/Transaction Does Not Exist) or
             * a 408 (Request Timeout), the UAC SHOULD terminate the dialog.
             */
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_CALL_DROP, pMsg,
                    (tSipHandle)pMsg->code);
            DIALOG_Destroy(pDialog);
            return (SIP_FREE_MEM);
        }

        if (pDialog->pReInviteUpdate) {
            SIP_freeMsg(pDialog->pReInviteUpdate);
            pDialog->pReInviteUpdate = NULL;
        }

        DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_STATE);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_MEDIA_INFO, pMsg, (tSipHandle)pMsg->code);
    }
    else {
        /* The branch in the via may have 
         * changed due to fail over.  So 
         * let's update the dialog szBranch. 
         */
        DIALOG_CacheBranch(pDialog,pMsg);

        /* must be provisional */
         if (pMsg->RSeq != 0) {
            /* check if we already got one of these reliable 
             * provisional responses 
             */
            if (pDialog->prack.respCode == 0) {
                /* then it's new so cache its information */
                pDialog->rseq = pMsg->RSeq;
                pDialog->prack.respCode = pMsg->code;
                pDialog->prack.rack.seqNum = pMsg->RSeq;
                pDialog->prack.rack.cseq = pMsg->CSeq;
            }
        }
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_MEDIA_INFO, pMsg, (tSipHandle)pMsg->code);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfClientUpdateResp(
    tSipDialog   *pDialog,
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfClientUpdateResp: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        DIALOG_TargetRefresh(pDialog, pMsg);
        DIALOG_ClientRespSessionExpires(pDialog, pMsg);
    }
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfClientSubscribeResp(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    tDialogEvent   *pEvt;
    vint            timeout;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfClientSubscribeResp: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    if (NULL == (pEvt = DIALOG_SearchEvtByTrans(pDialog, OSAL_TRUE, hTrans))) {
        /* Event doesn't exist.  Quitely ignore response then */
        return (SIP_FREE_MEM);
    }
    
    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        /* then start timer */
        if (HF_PresenceExists(&pMsg->x.DCPresenceMasks, eSIP_EXPIRES_HF)) {
            if (pMsg->Expires < pEvt->timeout) {
                pEvt->timeout = pMsg->Expires;
            }
        }
        /* then start timer */
        if (pEvt->timeout != 0) {
            timeout = UA_GetTimerMs(pEvt->timeout);
            SIPTIMER_Start(pEvt->hTimer, UA_SubscriberTimeoutCB, pEvt, timeout,
                    FALSE);
        }
    }
    else if (MSGCODE_ISFAILURE(pMsg->code)){
        /* 
         * Dont kill the subscription if it's a subscribe refresh per 3.1.4.2
         * RFC3265 
         */
        if (!pEvt->isResent) {
            UASM_SubscribeFailed(pDialog, pMsg, hTrans);
        }
    }
    /* do nothing is it's a provisional response */
    return (SIP_FREE_MEM);
}

static vint _UASM_ConfClientReferResp(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientReferResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    if (!(MSGCODE_ISSUCCESS(pMsg->code))) {
        /* Then mark the tranfer as invalid and notify the APP.
         * We want to mark the the tranfer as invalid so we ignore subseqeuent
         * NOTIFY requests. 
         */
        pDialog->transferor.IsValid = FALSE;
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_TRANSFER_FAILED, pMsg, NULL);
    }

    return SIP_FREE_MEM;
}

static vint _UASM_ClientOptionsResp(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ClientOptionsResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    UA_AppEvent(pDialog->hOwner, pDialog, 
            MSGCODE_ISSUCCESS(pMsg->code) ? eUA_OPTIONS_COMPLETED : eUA_OPTIONS_FAILED, pMsg, hTrans);
    return SIP_FREE_MEM;
}

static vint _UASM_ConfClientNotifyResp(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    tDialogEvent *pEvt;
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfClientNotifyResp: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    /* Search for the transaction */
    if (NULL == (pEvt = DIALOG_SearchEvtByTrans(pDialog, OSAL_FALSE, hTrans))) {
        /* This event is for call transfer */
        if (MSGCODE_ISSUCCESS(pMsg->code)) {
            /* 
             * Then mark the transferee as invalid so we ignore subseqeuent
             * NOTIFY request attempts from the app. 
             */
            pDialog->transferor.IsValid = FALSE;
        }
        return (SIP_FREE_MEM); 
    }
    
    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT_COMPLETED, pMsg,
                &pEvt->hTransaction);
    }
    else if (MSGCODE_ISFAILURE(pMsg->code)) {
        /* then remove the subscription */
        UASM_NotifyFailed(pDialog, hTrans);
    }

    return (SIP_FREE_MEM);
}

static vint _UASM_ConfClientInviteReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientInviteReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the message */
    DIALOG_PopulateRequest(pDialog, pMsg);
    if (SIP_OK != UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), 
            pDialog, pMsg, UAC_ReInvite, NULL, NULL)) {
        return (SIP_FAILED);
    }
    /* Request was successfully sent so Transition state */
    DIALOG_ChangeState(pDialog, eSIP_DIALOG_CONFIRMED_BUSY_STATE);
    return (SIP_OK);
}

static vint _UASM_ConfClientSubscribeReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    tDialogEvent *pEvt;
    vint          status;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfClientSubscribeReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    if (0 == pMsg->Expires) {
        
        SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfClientSubscribeReq: UN-subscribing hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
        
        if (SIP_NOT_FOUND == (status = DIALOG_RemoveEventByEvt(pDialog,
                OSAL_TRUE, &pMsg->Event, &pEvt))) {
            return (SIP_FAILED); 
        }
        
        /* populate the message */
        DIALOG_PopulateRequest(pDialog, pMsg);
    
        /* Send the request */
        if (SIP_OK != UA_SendRequest(pDialog->hOwner,
                DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Subscribe,
                NULL, NULL)) {
            /* 
             * We are returning SIP_OK anyway when we are un-subscribing so 
             * make sure the pMsg is freed here 
             */
            SIP_freeMsg(pMsg);
        }
        DIALOG_FreeEvent(pEvt);
        if (SIP_NO_DATA == status) {
            return (SIP_DONE);
        }
        return (SIP_OK);
    }
    else {
        /* populate the message */
        DIALOG_PopulateRequest(pDialog, pMsg);

        /* cache the event */
        if (NULL != (pEvt = DIALOG_CacheEvent(pDialog, OSAL_TRUE, &pMsg->Event,
                pMsg->Expires))) {
            return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), 
                    pDialog, pMsg, UAC_Subscribe, NULL, &pEvt->hTransaction);
        }
    }
    return (SIP_FAILED);
}

static vint _UASM_ConfClientMessageReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientMessageReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the message */
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Message, NULL, NULL);
}

static vint _UASM_ConfClientPublishReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientPublishReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the message */
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Publish, NULL, NULL);
}

static vint _UASM_ConfClientInfoReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientInfoReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the message */
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), 
            pDialog, pMsg, UAC_Info, NULL, NULL);
}


static vint _UASM_ConfClientByeReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    vint status;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientByeReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    /* populate the message */
    DIALOG_PopulateRequest(pDialog, pMsg);
    /* Check if the ACK is received or not. */
    if ((NULL != pDialog->ok.hRetryTimer) && (0 != pDialog->ok.t1)) {
        /*
         * We are stilling waiting the ACK. Cache the pMsg of BYE and don't
         * destroy the dialog.
         */
        pDialog->pCancel = pMsg;
        SIP_DebugLog(SIP_DB_DIALOG_LVL_2, "_UASM_ConfClientByeReq."
                "BYE but waiting for ACK", 0, 0, 0);
        return (SIP_OK);
    }
    status = UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), 
            pDialog, pMsg, UAC_Bye, NULL, NULL);
    return status;
}

static vint _UASM_ConfClientReferReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    char   str[SIP_URI_STRING_MAX_SIZE];
    uint32 len = SIP_URI_STRING_MAX_SIZE;
    UNUSED(hTrans);
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientReferReq: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
    
    /* populate the referredby header field */
    if (ENC_Uri(&pDialog->localUri.uri, str, &len, 1) == SIP_OK) {
        str[len] = 0;
        HF_CopyInsert(&pMsg->pHFList, eSIP_REFERRED_BY_HF, str, 0);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_REFERRED_BY_HF);
    }

    /* populate the message */
    DIALOG_PopulateRequest(pDialog, pMsg);
    /* save the Cseq number to accomidate multiple REFER's 
     * in the same dialog see section 2.4.6 in RFC3515
     */
    OSAL_itoa(pMsg->CSeq.seqNum, pDialog->transferor.szId,
            SIP_MAX_BASETEN_NUM_STRING);
    pDialog->transferor.IsValid = TRUE;
    return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), 
        pDialog, pMsg, UAC_Refer, NULL, NULL);
}

static vint _UASM_ConfClientNotifyReq(
    tSipDialog   *pDialog, 
    tSipIntMsg   *pMsg,
    tSipHandle    hTrans)
{
    tDialogEvent *pEvt;
    vint          status;
    
    UNUSED(hTrans);

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "_UASM_ConfClientNotifyReq: hDialog:%X pMsg:%X",
            (int)pDialog, (int)pMsg, 0);
    
    /* Frist see if this NOTIFY is pertains to call transfer */
    if (OSAL_strcasecmp(pMsg->Event.szPackage, SIP_EVENT_HF_REFER_PKG_STR) ==
            0) {

        /* if the transferee is not valid then perform a no-op */
        if (pDialog->transferee.IsValid) {
            /* populate the message */
            OSAL_strcpy(pMsg->Event.szId, pDialog->transferee.szId);
               
            DIALOG_PopulateRequest(pDialog, pMsg);
            return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg),
                pDialog, pMsg, UAC_Notify, NULL, NULL);
        }
        return (SIP_FAILED);
    }
    
    /* Handle all other NOTIFY's pertaining to subscriptions */
    if (NULL == (pEvt = DIALOG_SearchEvtByEvt(pDialog, OSAL_FALSE,
            &pMsg->Event))) {
        /* 
         * Then this notify does not pertain to any event in the dialog, 
         * return failed 
         */
        return (SIP_FAILED);
    }
    
    /* Check if this NOTIFY will terminate the subscription */
    if (pMsg->SubState.arg == eSIP_SUBS_HF_TERM_ARG) {
        /* Then the subscription should be removed */
        status = DIALOG_RemoveEventByEvt(pDialog, OSAL_FALSE, &pMsg->Event, 
                &pEvt);

        DIALOG_PopulateRequest(pDialog, pMsg);
        if (SIP_OK != UA_SendRequest(pDialog->hOwner, 
                DIALOG_GetUri(pDialog, pMsg), pDialog, pMsg, UAC_Notify,
                NULL, &pEvt->hTransaction)) {
            /* 
             * Then free he pMsg but still return SIP_OK or SIP_DONE to 
             * indicate that the event subscription was destroyed.
             */
            SIP_freeMsg(pMsg);
        }
        DIALOG_FreeEvent(pEvt);
        if (SIP_NO_DATA == status) {
            /* Then free free the event */
            return (SIP_DONE);
        }
        return (SIP_OK);
    }
    
    /* Otherwise, just send the NOTIFY */
    DIALOG_PopulateRequest(pDialog, pMsg);
    return UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pMsg), 
            pDialog, pMsg, UAC_Notify, NULL, &pEvt->hTransaction);
}

static vint _UASM_ConfClientMessageResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tSipIntMsg *pReq;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientMessageResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientMessageResp: received final response", 0, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_TEXT_MESSAGE_COMPLETED, pMsg, pDialog);
    }
    else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
        if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
            /* send new message with authorization */
            if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                               pDialog, pReq, UAC_Message, NULL, NULL) != SIP_OK) {
                SIP_freeMsg(pReq);
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_TEXT_MESSAGE_FAILED, pMsg, pDialog);
            }
        }
        else {
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_TEXT_MESSAGE_FAILED, pMsg, pDialog);
        }
    }
    else if (!(MSGCODE_ISPROV(pMsg->code))) {
        SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ConfClientMessageResp: Warning non-OK response %d", pMsg->code, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_TEXT_MESSAGE_FAILED, pMsg, pDialog);
    }
    /* for provisional responses we simply perform a no-op */
    return SIP_FREE_MEM;
}

static vint _UASM_ConfClientPublishResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tSipIntMsg *pReq;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientPublishResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientPublishResp: received final response", 0, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_PUBLISH_COMPLETED, pMsg, pDialog);
    }
    else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
        if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
            /* send new message with authorization */
            if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                               pDialog, pReq, UAC_Publish, NULL, NULL) != SIP_OK) {
                SIP_freeMsg(pReq);
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_PUBLISH_FAILED, pMsg, pDialog);
            }
        }
        else {
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_PUBLISH_FAILED, pMsg, pDialog);
        }
    }
    else if (!(MSGCODE_ISPROV(pMsg->code))) {
        SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ConfClientPublishResp: Warning non-OK response %d", pMsg->code, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_PUBLISH_FAILED, pMsg, pDialog);
    }
    /* for provisional responses we simply perform a no-op */
    return SIP_FREE_MEM;
}

static vint _UASM_ConfClientInfoResp(
    tSipDialog      *pDialog, 
    tSipIntMsg      *pMsg,
    tSipHandle       hTrans)
{
    tSipIntMsg *pReq;
    UNUSED(hTrans);
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientInfoResp: hDialog:%X pMsg:%X", (int)pDialog, (int)pMsg, 0);

    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "_UASM_ConfClientInfoResp: received final response", 0, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_INFO_COMPLETED, pMsg, (tSipHandle)pMsg->code);
    }
    else if (MSGCODE_ISCHALLENGE(pMsg->code)) {
        if ((pReq = DIALOG_Authenticate(pDialog, pMsg)) != NULL) {
            /* send new invite with authorization */
            if (UA_SendRequest(pDialog->hOwner, DIALOG_GetUri(pDialog, pReq), 
                               pDialog, pReq, UAC_Info, NULL, NULL) != SIP_OK) {
                SIP_freeMsg(pReq);
                UA_AppEvent(pDialog->hOwner, pDialog, eUA_INFO_FAILED, pMsg, (tSipHandle)pMsg->code);
            }
        }
        else {
            UA_AppEvent(pDialog->hOwner, pDialog, eUA_INFO_FAILED, pMsg, (tSipHandle)pMsg->code);
        }
    }
    else if (!(MSGCODE_ISPROV(pMsg->code))) {
        SIP_DebugLog(SIP_DB_UA_LVL_2, "_UASM_ConfClientInfoResp: Warning non-OK response %d", pMsg->code, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_INFO_FAILED, pMsg, (tSipHandle)pMsg->code);
    }
    /* for provisional responses we simply perform a no-op */
    return SIP_FREE_MEM;
}

void UASM_SubscribeFailed(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction)
{
    vint          status;
    tDialogEvent *pEvt;
    status = DIALOG_RemoveEventByTrans(pDialog, OSAL_TRUE, hTransaction, &pEvt);
    if (status == SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_2, "UASM_SubscribeFailed: eUA_SUBSCRIBE_FAILED", 0, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_SUBSCRIBE_FAILED, pMsg,
                &pEvt->evt);
        DIALOG_FreeEvent(pEvt);
    }
    else if (status == SIP_NO_DATA) {
        SIP_DebugLog(SIP_DB_UA_LVL_2, "UASM_SubscribeFailed: eUA_SUBSCRIBE_FAILED_NO_SUBS", 0, 0, 0);
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_SUBSCRIBE_FAILED_NO_SUBS,
                pMsg, &pEvt->evt);
        DIALOG_FreeEvent(pEvt);
        DIALOG_Destroy(pDialog);
    }
    return;
}

void UASM_NotifyFailed(
    tSipDialog *pDialog,
    tSipHandle  hTransaction)
{
    vint          status;
    tDialogEvent *pEvt;
    status = DIALOG_RemoveEventByTrans(pDialog, OSAL_FALSE, hTransaction,
            &pEvt);
    if (status == SIP_OK) {
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_NOTIFY_EVENT_FAILED, NULL,
                &pEvt->hTransaction);
        DIALOG_FreeEvent(pEvt);
    }
    else if (status == SIP_NO_DATA) {
        UA_AppEvent(pDialog->hOwner, pDialog, eUA_SUBSCRIBE_FAILED_NO_SUBS,
                NULL, &pEvt->evt);
        DIALOG_FreeEvent(pEvt);
        DIALOG_Destroy(pDialog);
    }
    return;
}

