/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SIP_DIALOG_H_
#define _SIP_DIALOG_H_

typedef enum eSipDialogState
{
    eSIP_DIALOG_IDLE_STATE,
    eSIP_DIALOG_SERVER_EARLY_STATE,
    eSIP_DIALOG_CLIENT_EARLY_STATE,
    eSIP_DIALOG_CONFIRMED_STATE,
    eSIP_DIALOG_CONFIRMED_BUSY_STATE,
    eSIP_DIALOG_LAST_STATE,
}tSipDialogState;

#define DIALOG_IS_ACTIVE(s) ((s > eSIP_DIALOG_IDLE_STATE) && (s < eSIP_DIALOG_LAST_STATE))
#define DIALOG_IS_IDLE(s) (s == eSIP_DIALOG_IDLE_STATE)
#define DIALOG_IS_CONFIRMED(s) (s == eSIP_DIALOG_CONFIRMED_STATE || s == eSIP_DIALOG_CONFIRMED_BUSY_STATE)
#define DIALOG_IS_SERVER_EARLY(s)(s == eSIP_DIALOG_SERVER_EARLY_STATE)

typedef struct sSipDialogId
{
    char szCallId[SIP_CALL_ID_HF_STR_SIZE];
    char szLocalTag[SIP_TAG_HF_STR_SIZE];
    char szRemoteTag[SIP_TAG_HF_STR_SIZE];
    vint isCallIdOwner;
}tSipDialogId;

typedef struct sDialog2xx
{
    tSipHandle  hRetryTimer;
    tSipHandle  hTimeoutTimer;
    tSipHandle  hTransport;
    uint32      t1;
    uint32      retryMax;
    tSipIntMsg *p2xxMsg;
}tDialog2xx;

typedef struct sDialogSessionTimer
{
    tSipHandle        hTimer;
    uint32            expires;
    tMinSeHF          minSeHf;
    vint              isRefresher;
}tDialogSessionTimer;

typedef struct sDialogAuthCache
{
    tDLList     authList;
    int         count;
    tHdrFld     credHf;
    tDLList    *pCredList;
}tDialogAuthCache;

typedef struct sDialogTransfer
{
    vint IsValid;
    char szId[SIP_MAX_BASETEN_NUM_STRING];
}tDialogTransfer;

typedef struct sDialogPrack
{
    tRAck        rack;
    tSipMsgCodes respCode;
    tSipHandle   hRetryTimer;
    tSipHandle   hTimeoutTimer;
    uint32       t1; /* same t1 value as in the transaction */
    uint32       retryMax; /* same t2 value as in the transaction */
    tSipIntMsg  *pMsg;
}tDialogPrack;

typedef struct sSipDialog
{
    /* pieces of state as described in section 12 of rfc3261 */
    tSipDialogId    id;
    tSipDialogState currentState;
    uint32          localSeqNum;
    uint32          localSeqNumInvite; /* this caches the seq num for invites
                                        * we may need this same value later for 
                                        * CANCEL's and ACK's
                                        */
    uint32          remoteSeqNum;
    tUriPlus        localUri;
    tUriPlus        remoteUri;
    tUri            remoteTargetUri;
    vint            haveRemoteFqdn;
    vint            isSecure;
    tDLList         routeList;
    tDLList         contactList;
    char            szBranch[SIP_BRANCH_HF_STR_SIZE];
    tDLList         via; /* store the via list so responses know where to go */
    /* end of RFC defined state pieces */
    tDLList         updateVia; /* Store via's for UPDATE requests seperate from INVITE.
                                  This is because they can happen at the same time */ 

    /* these are for internal SIP stack use */
    tSipHandle       hOwner; /* a handle to the owner of the dialog, this implementation defines that as a tUa */
    tSipHandle       hTrans; /* handle to a transaction for an INVITE request */
    tSipHandle       hUpdateTrans; /* handle to a transaction for an UPDATE request */
    tSipHandle       hPrackTrans; /* handle to a transaction for an PRAK request */
    vint             isMarkedForDeletion;
    vint             inviteHasResponse;
    vint             isEarlyBusy; /*
                                   * If SDP offer received or sent in early
                                   * state.
                                   */
    tSipIntMsg      *pCancel;
    tSipIntMsg      *pReInviteUpdate; /* Cache pMsg for re-INVITE or UPDATE. */
    tDialog2xx       ok;
    vint             isStrict;

    tSipHandle       hContext; /* used for thread message queue identification */
    tDialogAuthCache authCache; /* used to cache authentication info for 
                                 * requests within dialog, see section 22.3 on RFC 3261.
                                 */

    tHdrFldList     *pHFList; /* used to cache header fields from 
                               * messages just in case we get challenged 
                               * to authenticate and we need to build a new request 
                               */
    /* Expires HF for invite and re-invite just in case we get challenged
     * to authenticate and we need to build a new request
     */
    tExpiresHF expires;

    /* used for subscribe/notify behavior */
    tDLList           notifyList;
    tDLList           subscribeList;
    
    /* used for PRACK support (RFC3262) */
    uint32            rseq;
    tDialogPrack      prack;
    
    tReplacesHF       replaces;
    tSessionEntry     session;
    tDialogTransfer   transferor;
    tDialogTransfer   transferee;

    /* RFC3261 13.3.1 */
    tSipHandle        inviteTimer;

    /* RFC 3261 14.1 re-INVITE retry timer */
    tSipHandle        reqPendingTmr;

    tDialogSessionTimer sessionTimer;

    /* TS 24.229 and RFC6223 */
    OSAL_Boolean        natKeepaliveEnable;

    struct {
        tSipHandle      hTimer;
        tSipKeepalives  args;
    } keepaliveTimer;
}tSipDialog;

typedef struct sDialogEvent
{
    tDLListEntry dll;
    tEventHF     evt;
    tSipHandle   hTransaction;
    tSipHandle   hTimer;
    uint32       timeout;
    vint         isResent;
    tSipDialog  *pDialog;
}tDialogEvent;

void DIALOG_Init(void);

void DIALOG_KillModule(void);

void DIALOG_InitClient(
    tSipHandle         hOwner,
    tSipHandle         hContext,
    tSipDialog        *pDialog,
    tSipIntMsg        *pMsg,
    tDLList           *pCredList);

void DIALOG_InitServer(
    tSipHandle  hOwner,
    tSipHandle  hContext,
    tSipDialog *pDialog,
    tSipIntMsg *pMsg,
    tDLList    *pCredList);

vint DIALOG_IsMatched(
    tSipDialog   *pDialog,
    tSipMsgType   msgType,
    tSipMethod    method,
    char         *pTo,
    char         *pFrom,
    char         *pCallId);

vint DIALOG_Activate(
    tSipDialog  *pDialog,
    tSipIntMsg  *pMsg);

vint DIALOG_TargetRefresh(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg);

void DIALOG_Destroy(tSipDialog *pDialog);

vint DIALOG_PopulateRequest(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg);

void DIALOG_PopulateResponse(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg);

void DIALOG_Start2xx(
    tSipDialog     *pDialog, 
    tSipHandle      hTransport,
    tSipIntMsg     *pMsg,
    tpfSipTimerCB   retryCB,
    tpfSipTimerCB   timeoutCB);

vint DIALOG_Stop2xx(tSipDialog *pDialog);

void DIALOG_DestroySessionExpires(tSipDialog *pDialog);

void DIALOG_ServerReqSessionExpires(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg);

void DIALOG_DestroyInviteExpires(
    tSipDialog *pDialog);

void DIALOG_ServerReqInviteExpires(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg);

void DIALOG_ServerRespSessionExpires(
    tSipDialog     *pDialog,
    tSipIntMsg     *pMsg);

void DIALOG_ClientRespSessionExpires(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg);

void DIALOG_DestroyReqPendingRetry(
    tSipDialog *pDialog);

void DIALOG_ClientReqPendingRetry(
    tSipDialog *pDialog);

tSipHandle DIALOG_GetContext(tSipHandle hDialog);

void DIALOG_ChangeState(
    tSipDialog     *pDialog, 
    tSipDialogState state);

tSipIntMsg* DIALOG_Authenticate(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg);

tUri* DIALOG_GetUri(
    tSipDialog  *pDialog, 
    tSipIntMsg  *pMsg);

vint DIALOG_PopulateReplaces(
    tSipHandle   hDialog, 
    tReplacesHF *pReplaces);

vint DIALOG_PopulateReferTo(
    tSipHandle   hDialog, 
    tSipIntMsg  *pMsg);

tDialogEvent* DIALOG_CacheEvent(
    tSipDialog   *pDialog,
    OSAL_Boolean  isSubscriber, 
    tEventHF     *pEvt,
    uint32        timeout);

tDialogEvent* DIALOG_SearchEvtByEvt(
    tSipDialog   *pDialog,
    OSAL_Boolean  isSubscriber, 
    tEventHF     *pEvt);

tDialogEvent* DIALOG_SearchEvtByTrans(
    tSipDialog   *pDialog,
    OSAL_Boolean  isSubscriber,  
    tSipHandle    hTransaction);

vint DIALOG_RemoveEventByTrans(
    tSipDialog    *pDialog, 
    OSAL_Boolean   isSubscriber, 
    tSipHandle     hTransaction,
    tDialogEvent **ppEvt);

vint DIALOG_RemoveEventByEvt(
    tSipDialog    *pDialog, 
    OSAL_Boolean   isSubscriber, 
    tEventHF      *pEvent,
    tDialogEvent **ppEvt);

void DIALOG_FreeEvent(tDialogEvent *pEvent);

void DIALOG_CacheVia(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg);

void DIALOG_CleanPrack(tDialogPrack *pPrack);

void DIALOG_CacheBranch(
    tSipDialog  *pDialog,
    tSipIntMsg  *pMsg);

void DIALOG_WakeUpSubscriptions(
    tSipDialog   *pDialog);

void DIALOG_StartKeepalives(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg);

#endif
