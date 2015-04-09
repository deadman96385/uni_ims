/*
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include "sip_sip.h"
#include "sip_timers.h"
#include "sip_dbase_sys.h"
#include "sip_xport.h"
#include "sip_xact.h"
#include "sip_auth.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_mem_pool.h"

#include "../ua/_sip_callback.h"

#if (SIP_DEBUG_LOG)
#include "sip_parser_enc.h"
#endif

#include <sr.h>

static void _DIALOG_Destroy2xx(tSipDialog *pDialog);
static void DIALOG_FreeList(tDLList *pList);
static tDialogEvent* DIALOG_MallocEvent(void);

/* 
 *****************************************************************************
 * ================DIALOG_Init()===================
 *
 * This function initiailizes the dialog module.  Specifically mutexes used 
 * in linked list that are used to cache freed dialog event objects that 
 * originally came from the heap.
 *
 * RETURNS: 
 *      NOTHING
 *
 ******************************************************************************
 */
void DIALOG_Init(void) {
    /* Do nothing */
}

/* 
 *****************************************************************************
 * ================DIALOG_KillModule()===================
 *
 * This function initiailizes the dialog module.  Specifically mutexes used 
 * in linked list that are used to cache freed dialog event objects that 
 * originally came from the heap.
 *
 * RETURNS: 
 *      NOTHING
 *
 ******************************************************************************
 */
void DIALOG_KillModule(void) {
    /* Do nothing */
}

/* 
 *****************************************************************************
 * ================DIALOG_InitClient()===================
 *
 * This function initiailizes a dialog for a UAC.  It will initialize 
 * data members in the dialog specified in pDialog.  The pieces of state 
 * and data memebers set can be better described in seciton 12 of RFC3261
 *
 * hOwner = A handle to "owner" of the dialog.  In this case it's always a 
 *          handle to a UA.
 *
 * hContext = A handle to the context that owns the dialog.  This is typically 
 *            a thread or task ID number.
 *
 * pDialog = A pointer to the dialog object that should be initiailized
 *
 * pMsg = A pointer to an internal sip object.  This is the INVITE request
 *
 * pCred = a pointer to a set of user credentials used for authentication
 *
 * RETURNS: 
 *      NOTHING
 *
 ******************************************************************************
 */
void DIALOG_InitClient(
    tSipHandle         hOwner,
    tSipHandle         hContext,
    tSipDialog        *pDialog,
    tSipIntMsg        *pMsg,
    tDLList           *pCredList)
{
    char ipStr[MAX_IPV6_STR_LEN];
    tUri *pUri=NULL;
    tDLListEntry *pEntry;
    tDialog2xx *pOk;
    
    DIALOG_ChangeState(pDialog, eSIP_DIALOG_CLIENT_EARLY_STATE);

    /* 
     * store a pointer to the credentials just in case we are 'challenged' to 
     * authenticate
     */
    pDialog->authCache.pCredList = pCredList;

    /* cache the header field list just in case */
    HF_CopyAll(&pDialog->pHFList, pMsg->pHFList, NULL);

    /* Cache session timer stuff */
    pDialog->sessionTimer.expires = pMsg->SessionTimer.expires;
    pDialog->sessionTimer.minSeHf = pMsg->MinSE;
    if (pMsg->SessionTimer.refresher == eSIP_REFRESHER_UAC) {
        pDialog->sessionTimer.isRefresher = 1;
    }
    else {
        pDialog->sessionTimer.isRefresher = 0;
    }

    /* Cache the Expires HF. We only cache this for requests so we can
     * reuse the value if we get challenged. */
    pDialog->expires = pMsg->Expires;

    /* init the sequence number, leave the remoteSeqNum as empty or zero */
    pDialog->localSeqNum = HF_GenerateSeqNum();
    pDialog->remoteSeqNum = 0;

    /* 
     * Generate a unique CallId, the domain name will be part of the
     * Call-ID as a recommendation from the RFC, so that's why the
     * domain name is retrieved from the 'from' header field
     */
    pUri = &pMsg->From.uri;
    if (pUri->host.addressType == eNwAddrIPv6) {
        VoIP_IpV6Int2Ext(pUri->host.x.ip, ipStr);
        HF_GenerateCallId(ipStr, pDialog->id.szCallId);
    }
    else if (pUri->host.addressType == eNwAddrIPv4) {
        VoIP_IpV4Int2Ext(pUri->host.x.ip, ipStr);
        HF_GenerateCallId(ipStr, pDialog->id.szCallId);
    }
    else {
        HF_GenerateCallId(pUri->host.x.domainName, pDialog->id.szCallId);
    }
    /*
     * Set this dialog is the owner of Call-ID of the dialog ID.
     * This is requested by RFC 3261 14.1
     */
    pDialog->id.isCallIdOwner = TRUE;

    /* 
     * Now generate a local tag one
     */
    HF_GenerateTag(pDialog->id.szLocalTag);
    
    /* 
     * Now store the remote URI (from the 'To' HF)
     * and the local URI (from the 'From' HF)
     * and initialize the remoteTargetUri to the RequestUri.
     * If there is a contact header field in the response to 
     * this client transaction, or in a Re-INVITE then this value 
     * will change.
     */
    pDialog->remoteUri  = pMsg->To;
    pDialog->localUri   = pMsg->From;
    pDialog->remoteTargetUri = pMsg->requestUri;
    /* if there is a route (pre-existing), then copy it */
    DLLIST_Copy(&pMsg->RouteList, &pDialog->routeList, eDLLIST_ROUTE_HF);
        
    /* Now determine if the isSecure flag should be set and also store
     * the via branch just in case the the transaction gets cancelled 
     * See 9.1 of rfc 3261 */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        if (((tViaHFE *)pEntry)->uri.transport == eTransportTls &&  
                pMsg->requestUri.scheme == eURI_SCHEME_SIPS) {
            pDialog->isSecure = TRUE;
        }
    }

    /* 
     * if there is replaces info then cache it to use in future if auth 
     * challenged
     */
    if (HF_PresenceExists(&pMsg->x.ECPresenceMasks, eSIP_REPLACES_HF)) {
        pDialog->replaces = pMsg->Replaces;
    }

    /* set the owner of the dialog */
    pDialog->hOwner = hOwner;
    /* set the context the dialog is with in */
    pDialog->hContext = hContext;  
    /* This is the Context that the owner UA wants/needs in Callbacks */
    
    /* create the timer to receive 2xx response retrying coming from servers */
    pOk = &pDialog->ok;
    if (pOk->hTimeoutTimer) {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_2,
                "DIALOG_InitClient: Warning timeout Timer already exists for"
                "this dialog hDialog:%x -hTimer:%x", 
                (int)pDialog, (int)pOk->hTimeoutTimer, 0);
    }
    else {
        if ((pOk->hTimeoutTimer = SIPTIMER_Create(hContext)) == NULL) {
            SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                    "DIALOG_InitClient: Can't create a timer"
                    "object hDialog:%x", (int)pDialog, 0, 0);    
        }
    }

    pDialog->isMarkedForDeletion = FALSE;

    /*
     * Nodify VPR to add dialog id for sip packet filtering
     * This function only uses under 4G+ for VPR.
     */
    SR_addCallId(pDialog->id.szCallId);

    return; 
}

/* 
 *****************************************************************************
 * ================DIALOG_InitServer()===================
 *
 * This function creates and initiailizes a dialog for a UAS.  When a Server
 * is first created it is placed in to a early state until establishment
 * of the dialog is successfull, at which point the dialog is placed in to a 
 * confirmed state.  This happens when a 200 (OK) response is returned to an 
 * INVITE.
 *
 * hOwner = A handle to "owner" of the dialog.  In this case it's always a 
 *          handle to a UA.
 *
 * hContext = A handle to the context that owns the dialog.  This is typically 
 *            a thread or task ID number.
 *
 * pDialog = A pointer to the dialog object that should be initiailized
 *
 * pMsg = A pointer to an internal sip object.  This is the INVITE request
 *
 * RETURNS: 
 *      NOTHING
 *
 ******************************************************************************
 */
void DIALOG_InitServer(
    tSipHandle  hOwner,
    tSipHandle  hContext,
    tSipDialog *pDialog,
    tSipIntMsg *pMsg,
    tDLList    *pCredList)
{
    tDLListEntry *pEntry;
    tDialog2xx   *pOk;
    
    DIALOG_ChangeState(pDialog, eSIP_DIALOG_SERVER_EARLY_STATE);

    /*
     * store a pointer to the credentials just in case we are 'challenged' to
     * authenticate
     */
    pDialog->authCache.pCredList = pCredList;

    /* copy the Record-Route info in the same order */
    DLLIST_Copy(&pMsg->RecRouteList, &pDialog->routeList, eDLLIST_REC_ROUTE_HF);

    /* set the remote target to the contact of the request */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
        pDialog->remoteTargetUri = ((tContactHFE *)pEntry)->uri;
        pDialog->haveRemoteFqdn = TRUE;
    }

    /* set remoteSeqNum */
    pDialog->remoteSeqNum = pMsg->CSeq.seqNum;

    /* init the sequence number for use later */
    pDialog->localSeqNum = HF_GenerateSeqNum();

    /* init the sequence number used for reliable provisional responses */
    pDialog->rseq = HF_GenerateSeqNum();

    /* now copy the Call-ID */
    OSAL_strcpy(pDialog->id.szCallId, pMsg->szCallId);

    /* set remoteTag value from 'From' header field if it exists */
    OSAL_strcpy(pDialog->id.szRemoteTag, pMsg->From.szTag);

    HF_GenerateTag(pDialog->id.szLocalTag);
    
    /* set the entire remoteURI to what's in the 'From' HF */
    pDialog->remoteUri = pMsg->From;

    /* set the entire localURI to what's in the 'To' HF */
    pDialog->localUri = pMsg->To;

    /* Cache any session timer values */
    DIALOG_ServerReqSessionExpires(pDialog, pMsg);

    /* set the secure flag and store the topmost via branch param, see 12.1.1 of rfc3261 */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        OSAL_strcpy(pDialog->szBranch, ((tViaHFE *)pEntry)->szBranch);
        if (((tViaHFE *)pEntry)->uri.transport == eTransportTls && pMsg->requestUri.scheme == eURI_SCHEME_SIPS) {
            pDialog->isSecure = TRUE;
        }
    }
    
    /* set the owner of the dialog */
    pDialog->hOwner = hOwner;
    /* set the context the dialog is with in */
    pDialog->hContext = hContext; /* used to identify threads */

    /* Store the via list from the request that created this server dialog
     * so later we can use it in responses.  First clear out any old via info.
     */
    DLLIST_Copy(&pMsg->ViaList, &pDialog->via, eDLLIST_VIA_HF);

    /* create the timer for 2xx responses retrying */
    pOk = &pDialog->ok;
    if (pOk->hRetryTimer) {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                "DIALOG_InitServer: Warning Retry Timer already exists"
                "for this dialog hDialog:%x -hTimer:%x", 
                (int)pDialog, (int)pOk->hRetryTimer, 0);
    }
    else {
        pOk->hRetryTimer = SIPTIMER_Create(hContext);
        if (!pOk->hRetryTimer) {
            SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                    "DIALOG_InitServer: Can't create a timer hDialog:%x", 
                     (int)pDialog, 0, 0);
        }
    }
    if (pOk->hTimeoutTimer) {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                "DIALOG_InitServer: Warning timeout Timer already exists"
                "for this dialog hDialog:%x -hTimer:%x", 
                (int)pDialog, (int)pOk->hTimeoutTimer, 0);
    }
    else {
        pOk->hTimeoutTimer = SIPTIMER_Create(hContext);
        if (!pOk->hTimeoutTimer) {
            SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                    "DIALOG_InitServer: Can't create a timer hDialog:%x", 
                     (int)pDialog, 0, 0);
        }
    }

    pDialog->isMarkedForDeletion = FALSE;
        
    /*
     * Nodify VPR to add dialog id for sip packet filtering
     * This function only uses under 4G+ for VPR.
     */
    SR_addCallId(pDialog->id.szCallId);

    return;
}   



/* 
 *****************************************************************************
 * ================DIALOG_IsMatched()===================
 *
 * When a SIP message is received it needs to be matched to a dialog if one
 * exists.  the function will compare the fields in the pMsg with the ones 
 * in pDialog to determine if a match exists.  This fucntion accomidates both 
 * Dialog idetification for 3261 and also 2543 (Which does not mandate for 
 * 'To tags' and 'From tags'.  Please refer to 12.1.1 regarding 2543 tags and
 * also the dialog matching rules.
 *
 * pDialog = A pointer to the dialog to compare the pMsg to
 *
 * msgType = the type of message we are using to match to a dialog
 *
 * pTo, pFrom, pCallId = NULL terminated strings that represent the dialog
 *                       identifiers.
 *
 *
 * RETURNS: 
 *      SIP_OK: The pMsg belongs to the dialog indictated via pDialog
 *      SIP_NOT_FOUND: pMsg does NOT belong to dialog indictated by pDialog
 *      SIP_BADPARM: Either the pMsg of pDialog paramters were NULL.
 *                   This should never happen.
 *
 ******************************************************************************
 */
vint DIALOG_IsMatched(
    tSipDialog   *pDialog,
    tSipMsgType   msgType,
    tSipMethod    method,
    char         *pTo,
    char         *pFrom,
    char         *pCallId)
{

    if (OSAL_strcmp(pDialog->id.szCallId, pCallId) == 0) {
        if (DIALOG_IS_CONFIRMED(pDialog->currentState)) {
            if (msgType == eSIP_REQUEST) {
                if (OSAL_strcmp(pDialog->id.szLocalTag, pTo) == 0) {
                    if (OSAL_strcmp(pDialog->id.szRemoteTag, pFrom) == 0) {   
                        return (SIP_OK);
                    }
                }
            }
            else { /* must be a response */
                if (OSAL_strcmp(pDialog->id.szLocalTag, pFrom) == 0) {
                    if (OSAL_strcmp(pDialog->id.szRemoteTag, pTo) == 0) {   
                        return (SIP_OK);
                    }
                }
            }
        }
        else {
            /* 
             * It's early.  The only messages we want during 
             * an early state are CANCEL or NOTIFY or PRACK requests
             */
            if (msgType == eSIP_REQUEST) {
                if (method == eSIP_CANCEL || method == eSIP_PRACK) {
                    /* don't check the to field */
                    if (OSAL_strcmp(pDialog->id.szRemoteTag, pFrom) == 0) {
                        return (SIP_OK);
                    }
                }
                else if (method == eSIP_NOTIFY) {
                    if (OSAL_strcmp(pDialog->id.szLocalTag, pTo) == 0) {
                        return (SIP_OK);
                    }
                }
                else if (method == eSIP_UPDATE) {
                    /* If this is an update then we assume that 
                     * both side have traded 'To' & 'From' 'tags'.
                     * Therefore we check them both.
                     */
                    if (OSAL_strcmp(pDialog->id.szLocalTag, pTo) == 0) {
                        if (OSAL_strcmp(pDialog->id.szRemoteTag, pFrom) == 0) {   
                            return (SIP_OK);
                        }
                    }
                }
            }
        }
    }
    return (SIP_NOT_FOUND);
}

/* 
 *****************************************************************************
 * ================DIALOG_Activate()===================
 *
 * When a 200 (OK) response is returned to an INVITE the state of the dialog
 * must change, and pieces of the dialog state need to be updated based
 * on fields within the 200 response message.  This function handles both those 
 * things.
 *
 * pDialog = A pointer to a dialog.
 *
 * pMsg = A pointer to the internal sip message. This should be the 200 (OK) 
 *        response.
 *
 * RETURNS: 
 *      SIP_OK: Dailog was activated
 *      SIP_FAILED: There was an error updating the routeList.
 *
 ******************************************************************************
 */
vint DIALOG_Activate(
    tSipDialog  *pDialog,
    tSipIntMsg  *pMsg)
{
       
    /* clean out any route list in the dialog.  
     * It will be replaced with what's in the response 1xx 02 2xx.
     * If there is no Rec-route then the dialog shold be clear also.  
     * See section 12 of RFC3261.
     */
    DLLIST_Empty(&pDialog->routeList, eSIP_OBJECT_ROUTE_HF);
    
    /* Copy the route, if one exists */
    if (!DLLIST_IsEmpty(&pMsg->RecRouteList)) {
        /* Now replace it with the new route list if one exists */
        DLLIST_Copy(&pMsg->RecRouteList, &pDialog->routeList, eDLLIST_REC_ROUTE_HF);
        /* reverse the order of the route list, as per rfc3261 section 12 */
        DLLIST_Reverse(&pDialog->routeList);
    }

    if (pMsg->To.szTag[0] != 0) {
        /* copy the remote tag value to the dialog */
        OSAL_strcpy(pDialog->id.szRemoteTag, pMsg->To.szTag);
    }

    return SIP_OK;    
}

/* 
 *****************************************************************************
 * ================DIALOG_TargetRefresh()===================
 *
 * This function is called to update the dialog with target refresh 
 * information.
 *
 * pDialog = A pointer to a dialog.
 *
 * pContactList = A pointer to the contact list in the Re-INVITE with the 
 *                new target information.
 *
 * RETURNS: 
 *      SIP_BADPARM: The pDialog was NULL.
 *      SIP_OK: The remote taret information was updated 
 *      SIP_NOT_FOUND: There was no target refresh information.
 *
 *****************************************************************************
 */
vint DIALOG_TargetRefresh(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg)
{
    tDLListEntry *pEntry;

    if (!pDialog || !pMsg) 
        return (SIP_BADPARM);

    /* Copy the contact info to the remote target, Note there may be a list of 
     * contacts but we are only interested in the top one
     */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
        pDialog->remoteTargetUri = ((tContactHFE *)pEntry)->uri;
        pDialog->haveRemoteFqdn = TRUE;
        return (SIP_OK);
    }
    return (SIP_NOT_FOUND);
}

/* 
 *****************************************************************************
 * ================DIALOG_CacheVia()===================
 *
 * This function is called to cache (save) a via from a inbound request to the
 * dialog 
 *
 * pDialog = A pointer to a dialog.
 *
 * pMsg = A pointer to the internal SIP request that the via is coming from  
 *
 * RETURNS: 
 *     Nothing
 *
 *****************************************************************************
 */
void DIALOG_CacheVia(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg)
{
    /* Store the via list from the request so later we can use it in 
     * responses.  First clear out any old via info.
     */
    if (pMsg->method == eSIP_UPDATE) {
        DLLIST_Empty(&pDialog->updateVia, eSIP_OBJECT_VIA_HF);
        DLLIST_Copy(&pMsg->ViaList, &pDialog->updateVia, eDLLIST_VIA_HF);
    }
    else {
        DLLIST_Empty(&pDialog->via, eSIP_OBJECT_VIA_HF);
        DLLIST_Copy(&pMsg->ViaList, &pDialog->via, eDLLIST_VIA_HF);
    }
    return;
}


/* 
 *****************************************************************************
 * ================DIALOG_Destroy()===================
 *
 * This function destroys (cleans) a sip dialog object
 *
 * pDialog = A pointer to a dialog.
 *
 * NOTE: This function assumes the pDialog has already been verified non-NULL
 *
 * RETURNS: 
 *      Nothing
 *
 *****************************************************************************
 */
void  DIALOG_Destroy(tSipDialog *pDialog)
{
    /*
     * Nodify VPR to remove dialog id for sip packet filtering
     * This function only uses under 4G+ for VPR.
     */
    SR_removeCallId(pDialog->id.szCallId);

    /* first destroy the session */
    SESSION_Destroy(&pDialog->session);

    /* free up the contact List */
    DLLIST_Empty(&pDialog->contactList, eSIP_OBJECT_CONTACT_HF);

    /* free up the route List */
    DLLIST_Empty(&pDialog->routeList, eSIP_OBJECT_ROUTE_HF);
    /* empty out the via list */
    DLLIST_Empty(&pDialog->via, eSIP_OBJECT_VIA_HF);
    /* empty out the via list for update requests */
    DLLIST_Empty(&pDialog->updateVia, eSIP_OBJECT_VIA_HF);
    /* empty out the authenticate list */
    DLLIST_Empty(&pDialog->authCache.authList, eSIP_OBJECT_AUTH_HF);
    /* empty out the events lists */
    DIALOG_FreeList(&pDialog->subscribeList);
    DIALOG_FreeList(&pDialog->notifyList);

    /* kill off any timer is they exist */
    _DIALOG_Destroy2xx(pDialog);
    DIALOG_DestroySessionExpires(pDialog);
    DIALOG_DestroyInviteExpires(pDialog);
    DIALOG_DestroyReqPendingRetry(pDialog);

    /* free any cache header fields */
    HF_DeleteAll(&pDialog->pHFList);

    /* kill the prack timer if it exists */
    if (pDialog->prack.hRetryTimer) {
        SIPTIMER_Destroy(pDialog->prack.hRetryTimer);
    }
    if (pDialog->prack.hTimeoutTimer) {
        SIPTIMER_Destroy(pDialog->prack.hTimeoutTimer);
    }    
    if (pDialog->prack.pMsg) {
        SIP_freeMsg(pDialog->prack.pMsg);

    }
    /* clear up the stuff used for PRACK */
    OSAL_memSet(&pDialog->prack, 0, sizeof(tDialogPrack));

    /* If there is still a waiting cancel message, free that up as well */
    if (pDialog->pCancel) {
        SIP_freeMsg(pDialog->pCancel);
        pDialog->pCancel = NULL;
    }

    if (pDialog->pReInviteUpdate) {
        SIP_freeMsg(pDialog->pReInviteUpdate);
        pDialog->pReInviteUpdate = NULL;
    }

    /* zero out all fields that should be clear for next time */ 
    pDialog->id.szRemoteTag[0] = 0;
    pDialog->id.szLocalTag[0]  = 0;
    pDialog->id.szCallId[0]    = 0;
    pDialog->id.isCallIdOwner  = FALSE;

    /* clean out old resplaces info */
    pDialog->replaces.szCallId[0] = 0;
        
    HF_CleanUriPlus(&pDialog->localUri);
    HF_CleanUriPlus(&pDialog->remoteUri);
    HF_CleanUri(&pDialog->remoteTargetUri);

    pDialog->authCache.count = 0;
    
    DIALOG_ChangeState(pDialog, eSIP_DIALOG_IDLE_STATE);
    //pDialog->isMarkedForDeletion = FALSE;
    /* Deassociate owner from transacation. */
    TRANS_deassociate(pDialog->hTrans);
    pDialog->hTrans = NULL;
    /* Deassociate owner from transacation. */
    TRANS_deassociate(pDialog->hUpdateTrans);
    pDialog->hUpdateTrans = NULL;
    TRANS_deassociate(pDialog->hPrackTrans);
    pDialog->hPrackTrans = NULL;
    pDialog->transferor.IsValid = FALSE;
    pDialog->haveRemoteFqdn = FALSE;
    pDialog->isStrict = FALSE;
    pDialog->inviteHasResponse = FALSE;
    pDialog->isEarlyBusy = FALSE;

    /* Clear nat keep-alives flag and timer */
    pDialog->natKeepaliveEnable = OSAL_FALSE;
    if (pDialog->keepaliveTimer.hTimer) {
        SIPTIMER_Destroy(pDialog->keepaliveTimer.hTimer);
    }

    if (pDialog->isMarkedForDeletion == TRUE) {
        pDialog->isMarkedForDeletion = FALSE;
        /* send it back to heap */
        SIP_free(pDialog);
    }
}

/* 
 *****************************************************************************
 * ================_BuildRouteHelper()===================
 *
 * This function is used to build a route and request URI 
 * based on state of the routelist in the dialog.  
 * The requestUri of the pMsg and the pMsg routeList may be modified based 
 * on the dialog's route information. See section 12 in RFC3261.  
 *
 * pRouteList = A pointer to the list of routes in a dialog 
 *
 * pMsg = A pointer to the sip message who's route list is being manipulated
 *
 * pRemoteTargetUri = A pointer to the Uri of the target UA
 *
 * RETURNS: 
 *      Nothing
 *
 *****************************************************************************
 */
static vint _BuildRouteHelper(
    tDLList    *pRouteList, 
    tSipIntMsg *pMsg, 
    tUri       *pRemoteTargetUri)
{
    tDLListEntry *pEntry;

    vint isStrict;
    /* now build the route based on section 12 of RFC3261 */
    pEntry = NULL;
    if (DLLIST_GetNext(pRouteList, &pEntry)) {
        
        if (!DLLIST_IsEmpty(&pMsg->RouteList)) {
            /* Then something previously existed so get rid of it */
            SIP_DebugLog(SIP_DB_DIALOG_LVL_2,
                    "_BuildRouteHelper: WARNING clearing previous route"
                    "entries in pMsg:%X", (int)pMsg, 0, 0);
            DLLIST_Empty(&pMsg->RouteList, eSIP_OBJECT_ROUTE_HF); 
        }
        
        if (((tRouteHFE *)pEntry)->uri.lr) {/* loose routing... */
            pMsg->requestUri = *pRemoteTargetUri;
            DLLIST_Copy(pRouteList, &pMsg->RouteList, eDLLIST_ROUTE_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ROUTE_HF);
            isStrict = FALSE;

        }
        else {
            DLLIST_Copy(pRouteList, &pMsg->RouteList, eDLLIST_ROUTE_HF);
            pEntry = NULL;
            if (DLLIST_GetNext(&pMsg->RouteList, &pEntry)) {
                DLLIST_Remove(&pMsg->RouteList, pEntry);
                pMsg->requestUri = ((tRouteHFE *)pEntry)->uri; 
                /* strip off parameters not allowed in Request URI's */
                pMsg->requestUri.lr = FALSE;
                /* disable the transport param */
                pMsg->requestUri.transport = eTransportNone;
                ((tRouteHFE *)pEntry)->uri = *pRemoteTargetUri;
                /* now reinsert it into the route list */
                DLLIST_InitEntry(pEntry);
                DLLIST_Enqueue(&pMsg->RouteList, pEntry);
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ROUTE_HF);
            }
            isStrict = TRUE;
        }
    }
    else {
        /* DO NOT enable even an empty route entry */
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_ROUTE_HF);
        isStrict = FALSE;
    }
    return isStrict;
}

/* 
 *****************************************************************************
 * ================DIALOG_PopulateRequest()===================
 *
 * This function is used as a helper function to populate requests that are 
 * sent within the context of a dialog.  These reqeusts are populated with 
 * pieces of state in the dialog.  This is described in section
 * 12.2.1 of RFC3261
 *
 * pDialog = A pointer to a dialog.
 *
 * pMsg = A pointer to the sip message to be populated (The reqeust being sent)
 *
 * RETURNS: 
 *      SIP_BADPARM: The pDialog was NULL
 *      SIP_FAILED: The dialog is not in a "confirmed" state
 *      SIP_OK: The pMSg was correctly populated
 *
 *****************************************************************************
 */
vint DIALOG_PopulateRequest(
    tSipDialog  *pDialog, 
    tSipIntMsg  *pMsg)
{
    tSipMethod      method;
    vint            status;
    tIPAddr         addr;
    char           *pMethodStr;
    tUri           *pTargetUri;
    char           *pBranch;
    OSAL_Boolean    keep;

    keep   = OSAL_FALSE;
    method = pMsg->method;

    /* Let this function automatically generate a branch
     * for new requests for other than CANCEL */
    if (method == eSIP_PRACK) {
        pBranch = NULL;
    }
    else if (method == eSIP_CANCEL) {
        pBranch = pDialog->szBranch;
    }
    else {
        HF_GenerateBranch(pDialog->szBranch);
        pBranch = pDialog->szBranch;
    }

    if ((method != eSIP_ACK) && (method != eSIP_PRACK)) {
        /* if there is no contact information in the pMsg, then use
         * the contact information cached in the dialog.
         * This is for INVITE ONLY!
         */
        if (DLLIST_IsEmpty(&pMsg->ContactList)) {
            if (!DLLIST_IsEmpty(&pDialog->contactList)) {
                DLLIST_Copy(&pDialog->contactList, &pMsg->ContactList,
                            eDLLIST_CONTACT_HF);
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
            }
        }
        else {
            /* cache the contact to the dialog for use later */
            DLLIST_Empty(&pDialog->contactList, eSIP_OBJECT_CONTACT_HF);
            DLLIST_Copy(&pMsg->ContactList, &pDialog->contactList,
                    eDLLIST_CONTACT_HF);
        }

        /* turn on the 'Allow' header field */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    }

    /*
     * If request is INVITE and this is non-register emergency call,
     * need send "keep" in Via that depend on if enable "Keep_Alive_Enabled"
     */
    if (method == eSIP_INVITE) {
        keep = pDialog->natKeepaliveEnable;
    }

    /* Cache the expires just in case we get challenged. */
    pDialog->expires = pMsg->Expires;

    /* build a via */
    OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
    addr.v4.ul = 0;
    if ((status = HF_MakeVia(&pMsg->ViaList, NULL, pBranch, addr, keep)) !=
            SIP_OK) {
        return status;
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);

    /* set the 'To' field */
    pMsg->To = pDialog->remoteUri;
    /* kill the display name */
    pMsg->To.szDisplayName[0] = 0;
    if (pMsg->method == eSIP_CANCEL) {
        pMsg->To.szTag[0] = 0;
    }
    else {
        OSAL_strcpy(pMsg->To.szTag, pDialog->id.szRemoteTag);
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);

    /* set the 'From' header field */
    pMsg->From = pDialog->localUri;
    /* set the 'From' tag value */
    OSAL_strcpy(pMsg->From.szTag, pDialog->id.szLocalTag);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

    /* set the call ID */
    OSAL_strcpy(pMsg->szCallId, pDialog->id.szCallId);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);

    /* set the seq number on the 'CSeq' header field */
    if (method == eSIP_ACK || method == eSIP_CANCEL) {
        pMsg->CSeq.seqNum = pDialog->localSeqNumInvite;
    }
    else {
        /* cache the sequence number specifically for INVITEs
         * This is because we need it for CANCEL's and INVITEs
         */
        if (method == eSIP_INVITE) {
            pDialog->localSeqNumInvite = pDialog->localSeqNum;
        }
        pMsg->CSeq.seqNum = pDialog->localSeqNum++;
    }

    pMsg->CSeq.method = method;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);

    /* Set the requestUri, it may be changed after going through the
     * call to DIALOG_BuildRouteHelper.  Note, that for cancel the
     * Request-URI must be the same as the invite it's canceling.
     */
    if (pMsg->method == eSIP_CANCEL || (!(pDialog->haveRemoteFqdn))) {
        pTargetUri = &pDialog->remoteUri.uri;
    }
    else {
        pTargetUri = &pDialog->remoteTargetUri;
    }

    /* Add any session timer stuff */
    if (pMsg->method == eSIP_INVITE || pMsg->method == eSIP_UPDATE) {
        if (pDialog->sessionTimer.expires != 0) {
            pMsg->SessionTimer.expires = pDialog->sessionTimer.expires;
            if (pDialog->sessionTimer.isRefresher) {
                pMsg->SessionTimer.refresher = eSIP_REFRESHER_UAC;
            }
            else {
                pMsg->SessionTimer.refresher = eSIP_REFRESHER_UAS;
            }
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SESSION_EXPIRES_HF);
            pMsg->MinSE = pDialog->sessionTimer.minSeHf;
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MIN_SE_HF);
        }
    }

    /* now set the 'sips' scheme if the isSecure is true */
    /* NOTE: I'm not sure of this is right, see 12.2 of RFC */
    if (pDialog->isSecure)
        pMsg->requestUri.scheme = eURI_SCHEME_SIPS;

    pMsg->requestUri = *pTargetUri;
    /* builds the route\request uri based on the route set in the dialog */

    /* Can't use route semantics and reconstruction methods for CANCEL.
     * You are not supposed to do this at all before 200ok
     *
     */
    if (pMsg->method != eSIP_CANCEL) {
        pDialog->isStrict = _BuildRouteHelper(&pDialog->routeList, pMsg, pTargetUri);
    }

    /* Include Supported header field in INVITE, UPDATE and REFER. */
    if ((eSIP_INVITE == pMsg->method) || (eSIP_UPDATE == pMsg->method) ||
            (eSIP_REFER == pMsg->method)) {
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);
    }
    /* flip on some defaults */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);
    SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);

    /* if there is an authorization list then include it */
    if (!DLLIST_IsEmpty(&pDialog->authCache.authList)) {
        /* get the right method string for authentication */
        switch (pMsg->method) {
        case eSIP_INVITE:
            pMethodStr = SIP_INVITE_METHOD_STR;
            break;
        case eSIP_CANCEL:
            pMethodStr = SIP_CANCEL_METHOD_STR;
            break;
        case eSIP_BYE:
            pMethodStr = SIP_BYE_METHOD_STR;
            break;
        case eSIP_ACK:
            pMethodStr = SIP_ACK_METHOD_STR;
            break;
        case eSIP_SUBSCRIBE:
            pMethodStr = SIP_SUBSCRIBE_METHOD_STR;
            break;
        case eSIP_NOTIFY:
            pMethodStr = SIP_NOTIFY_METHOD_STR;
            break;
        case eSIP_MESSAGE:
            pMethodStr = SIP_MESSAGE_METHOD_STR;
            break;
        case eSIP_INFO:
            pMethodStr = SIP_INFO_METHOD_STR;
            break;
        case eSIP_PRACK:
            pMethodStr = SIP_PRACK_METHOD_STR;
            break;
        case eSIP_UPDATE:
            pMethodStr = SIP_UPDATE_METHOD_STR;
            break;
        case eSIP_PUBLISH:
            pMethodStr = SIP_PUBLISH_METHOD_STR;
            break;
        case eSIP_REFER:
            pMethodStr = SIP_REFER_METHOD_STR;
            break;
        default:
            pMethodStr = SIP_DUMMY_STR;
            break;
        }
        if (AUTH_Response(pMsg->From.uri.user, pDialog->authCache.pCredList,
                &pDialog->remoteTargetUri, &pDialog->authCache.authList,
                pMethodStr, &pMsg->AuthorizationList, NULL, 0, NULL) ==
                SIP_OK) {
            HF_SetPresence(&pMsg->x.ECPresenceMasks, pDialog->authCache.credHf);
        }
        else {
            SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                    "DIALOG_PopulateRequest: FAILED to authenticate hDialog:"
                    "%X pMsg:%X", (int)pDialog, (int)pMsg, 0);
            return (SIP_FAILED);
        }
    }

    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================DIALOG_PopulateResponse()===================
 *
 * This function is used as a helper function to populate responses that are 
 * sent within the context of a dialog.  These responses are populated with 
 * pieces of state in the dialog.  This is described in section
 * 12.1.1 of RFC3261
 *
 * pDialog = A pointer to a dialog.
 *
 * pMsg = A pointer to the sip message to be populated 
 *        (The response being sent)
 *
 * RETURNS: 
 *      NOTHING
 *
 *****************************************************************************
 */
void DIALOG_PopulateResponse(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg)
{

    /* 
     * populate the dialog's route info into the RecRoute for 1xx and 2xx
     * only as per rfc3261 seciton 12
     */
    if (MSGCODE_ISPROV(pMsg->code) || MSGCODE_ISSUCCESS(pMsg->code)) {
        if (DLLIST_IsEmpty(&pMsg->RecRouteList)) {
            /* then copy the dialogs route information */
            if (!DLLIST_IsEmpty(&pDialog->routeList)) {
                DLLIST_Copy(&pDialog->routeList, &pMsg->RecRouteList,
                        eDLLIST_REC_ROUTE_HF);
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_RECORD_ROUTE_HF);
            }
        }
    }

    /* We want to use the dialog's copy of the 
     * contact list if it's a 200 OK to an INVITE.
     * But, only if there is a contact List available.
     * Otherwise save the one in pMsg as the dialog's contactList.
     * This is because this is probably the first 200 to an INVITE.
     */

    if (MSGCODE_ISSUCCESS(pMsg->code)) {
        /*
         * Include Supported header field in 200 OK of INVITE, UPDATE and
         * OPTIONS.
         */
        if ((eSIP_INVITE == pMsg->CSeq.method) ||
                (eSIP_OPTIONS == pMsg->CSeq.method) ||
                (eSIP_UPDATE == pMsg->CSeq.method)) {
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);
        }
        if (pMsg->CSeq.method == eSIP_INVITE || pMsg->CSeq.method == eSIP_UPDATE) {
            if (DLLIST_IsEmpty(&pDialog->contactList)) {
                DLLIST_Copy(&pMsg->ContactList, &pDialog->contactList,
                        eDLLIST_CONTACT_HF);
            }
            else {
                /* else there is contact information currently in the dialog 
                 * so use that.  But, first get rid of anything already inthe pMsg
                 */
                DLLIST_Empty(&pMsg->ContactList, eSIP_OBJECT_CONTACT_HF);
                DLLIST_Copy(&pDialog->contactList, &pMsg->ContactList,
                        eDLLIST_CONTACT_HF);
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
            }
            /* Load session timer stuff if session timers are negotiated */
            if (pDialog->sessionTimer.expires != 0) {
                if (pDialog->sessionTimer.isRefresher) {
                    pMsg->SessionTimer.refresher = eSIP_REFRESHER_UAS;
                }
                else {
                    pMsg->SessionTimer.refresher = eSIP_REFRESHER_UAC;
                }
                pMsg->SessionTimer.expires = pDialog->sessionTimer.expires;
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SESSION_EXPIRES_HF);
                HF_CopyInsert(&pMsg->pHFList, eSIP_REQUIRE_HF, "timer", 0);
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_REQUIRE_HF);
            }
            /*
             * If session timer is not supported in UAC,
             * load session timer stuff from MT side.
             */
            else if (pMsg->SessionTimer.expires != 0) {
                if (eSIP_REFRESHER_UAS == pMsg->SessionTimer.refresher) {
                    pDialog->sessionTimer.isRefresher = 1;
                }
                else {
                    pDialog->sessionTimer.isRefresher = 0;
                }
                pDialog->sessionTimer.expires = pMsg->SessionTimer.expires;
                HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SESSION_EXPIRES_HF);
            }
        }
    }


    /* set the 'To' field */
    pMsg->To = pDialog->localUri;
    OSAL_strcpy(pMsg->To.szTag, pDialog->id.szLocalTag);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);

    /* set the 'From' header field */
    pMsg->From = pDialog->remoteUri;
    /* set the 'From' tag value */
    OSAL_strcpy(pMsg->From.szTag, pDialog->id.szRemoteTag);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

    /* set the call ID */
    OSAL_strcpy(pMsg->szCallId, pDialog->id.szCallId);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);

    /* if there is not a via in the message then copy 
     * in the one from the dialog object 
     */
    if (!(HF_PresenceExists(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF))) {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_2,
                "DIALOG_PopulateResponse: Warning there is no via in "
                "hMsg:%X hDialog:%X will use local cached copy", 
                (int)pMsg, (int)pDialog, 0);
        if (pMsg->CSeq.method == eSIP_UPDATE) {
            DLLIST_Copy(&pDialog->updateVia, &pMsg->ViaList, eDLLIST_VIA_HF);
        }
        else {
            DLLIST_Copy(&pDialog->via, &pMsg->ViaList, eDLLIST_VIA_HF);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
    }

    /* flip on some defaults*/
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);
    SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
}

/* 
 *****************************************************************************
 * ================DIALOG_Start2xx()===================
 *
 * When a 200 (OK) response is returned to an INVITE reqeust.  The UAS
 * must continue to send the 200 response until an ACK is received from the 
 * remote device's UAC.  This function initalizes the mechanisms
 * that will resend the 200 OK. 
 * But, if the dialog is a client it starts a timer to keep the transport 
 * resource open to receive 2xx responses form the peer's server.
 *
 * pDialog = A pointer to the dialog.
 *
 * hTransport = A handle to the transport resource used to send the response
 *
 * pMsg = A pointer to the 200 response message
 *
 * retryCB = a pointer to a function that is kicked when the retry timer 
 *           expires.
 *
 * timeoutCB = a pointer to a function that is kicked when the timeout timer 
 *           expires.
 *
 * RETURNS: 
 *      Nothing
 *
 *****************************************************************************
 */
void DIALOG_Start2xx(
    tSipDialog     *pDialog, 
    tSipHandle      hTransport,
    tSipIntMsg     *pMsg,
    tpfSipTimerCB   retryTimerCB,
    tpfSipTimerCB   timeoutTimerCB)
{
    tDialog2xx *pOk;
    uint32 time;
    
    if (!pDialog || !hTransport) {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                "DIALOG_Start2xx: couldn't start 2xx retry timer "
                "hDialog:%X, hTransport:%X", (int)pDialog, (int)hTransport, 0);
        return;
    }

    pOk = &pDialog->ok;

    /* init the timer, reinit it for server and cleint behavior for safety */
    pOk->t1 = TRANS_getTimerT1();
    /* Timer H is T1 * 64. */
    pOk->retryMax = pOk->t1 * 64;
    
    /* deallocate any old transport resources if they exist */
    if (pOk->hTransport) {
        TRANSPORT_Dealloc(pOk->hTransport);
    }
    /* We don't want to delete the transport resource 
     * when the transaction dies so we add a new user
     */
    TRANSPORT_AddUser(hTransport);
    pOk->hTransport = hTransport;
    
    if (pMsg) {
        if (pOk->p2xxMsg) {
            /* 
             * the p2xxMsg pointer should be clean, if it's not keep going 
             * put there is still a cleaning up problem 
             */
            SIP_DebugLog(SIP_DB_DIALOG_LVL_2,
                    "DIALOG_Start2xx: warning server thinks the p2xxMsg is "
                    "still in use hDialog:%X, pMsg:%X", 
                    (int)pDialog, (int)pOk->p2xxMsg, 0);
            SIP_freeMsg(pOk->p2xxMsg);
            pOk->p2xxMsg = NULL;
        }
    }

    /* Start timeout timer */
    if (pOk->hTimeoutTimer && (NULL != timeoutTimerCB)) {
        /* timeout value is timer H, i.e. t1 * 64 */
        SIPTIMER_Start(pOk->hTimeoutTimer, timeoutTimerCB, pDialog,
                pOk->retryMax, FALSE);
    }
    else {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                "DIALOG_Start2xx: could not start 2xx timeout timer, "
                "timer never created hDialog:%X", (int)pDialog, 0, 0);
        /* clean house */
        TRANSPORT_Dealloc(pOk->hTransport);
        pOk->hTransport = NULL;
        return;
    }

    /* Start retry timer */
    if (pOk->hRetryTimer && (NULL != retryTimerCB)) {
        if (pMsg) {
            pOk->p2xxMsg = SIP_copyMsg(pMsg);
            /*
             * Need to get the CSeq num since this retry msg will not go
             * through xact. 
             */
            pOk->p2xxMsg->CSeq.seqNum = ((tTrans*)(pDialog->hTrans))->cseq.seqNum;
            HF_SetPresence(&pOk->p2xxMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
            if (!pOk->p2xxMsg) {
                SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                        "DIALOG_Start2xx: server could not start 2xx retry "
                        "timer hDialog:%X, failed to COPY pMsg",
                        (int)pDialog, 0, 0);
                /* clean house */
                TRANSPORT_Dealloc(pOk->hTransport);
                pOk->hTransport = NULL;
                return;
            }
            time = pOk->t1;
            SIPTIMER_Start(pOk->hRetryTimer, retryTimerCB, pDialog, time, FALSE);
        }
        else {
            SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                    "DIALOG_Start2xx: could not start 2xx retry timer, "
                    "pMsg is NULL hDialog:%X", (int)pDialog, 0, 0);
            /* clean house */
            TRANSPORT_Dealloc(pOk->hTransport);
            pOk->hTransport = NULL;
        }
        return;
    }
    else {
        /* Retry timer is not active. Clear t1. */
        pOk->t1 = 0;
    }

}

static void _DIALOG_StartSessionExpires(
    tSipDialog     *pDialog,
    tpfSipTimerCB   timerCB,
    uint32          timeoutSecs)
{
    tDialogSessionTimer *stimer_ptr;
    uint32 time;

    stimer_ptr = &pDialog->sessionTimer;
    time =  timeoutSecs * 1000;

    /*
     * We don't want to delete the transport resource
     * when the transaction dies so we add a new user
     */

    if (NULL == stimer_ptr->hTimer) {
        if (NULL == (stimer_ptr->hTimer = SIPTIMER_Create(pDialog->hContext))) {
            return;
        }
    }

    SIPTIMER_Start(stimer_ptr->hTimer, timerCB, pDialog, time, TRUE);
    
    SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                "DIALOG_StartSessionTimer: started! for hDialog:%x",
            (int)pDialog, 0, 0);
    return;
}

void DIALOG_ServerReqSessionExpires(tSipDialog *pDialog, tSipIntMsg *pMsg)
{
    if (SIP_OK == HF_CheckSupportedHF(&pMsg->pHFList, "timer")
            && 0 != pMsg->SessionTimer.expires) {
        if (eSIP_REFRESHER_UAC != pMsg->SessionTimer.refresher) {
            /* Then let's start a timer to re-INVITE periodically */
            pDialog->sessionTimer.isRefresher = 1;
            pDialog->sessionTimer.expires = pMsg->SessionTimer.expires;
            pDialog->sessionTimer.minSeHf = (0 == pMsg->MinSE) ? 90 : pMsg->MinSE;
        }
        else {
            pDialog->sessionTimer.isRefresher = 0;
            pDialog->sessionTimer.expires = pMsg->SessionTimer.expires;
            pDialog->sessionTimer.minSeHf = (0 == pMsg->MinSE) ? 90 : pMsg->MinSE;
        }
    }
}

void DIALOG_DestroyInviteExpires(
    tSipDialog *pDialog)
{
    if (pDialog->inviteTimer) {
        SIPTIMER_Stop(pDialog->inviteTimer);
        SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
            "DIALOG_DestroyInviteTimer: Has destroyed the timer:%X hTimer:%X",
            (int)pDialog, (int)pDialog->inviteTimer, 0);
        SIPTIMER_Destroy(pDialog->inviteTimer);
        pDialog->inviteTimer = NULL;
    }
}

void DIALOG_ServerReqInviteExpires(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg)
{
    /*
     * If there is expires header in INVITE, start a INVITE timer.
     * (RFC3261 13.3.1)
     */
    if (0 != pMsg->Expires) {
        /* Cache the Expires HF. */
        pDialog->expires = pMsg->Expires;
        /* Start INVITE expires timer */
        if (NULL == pDialog->inviteTimer) {
            if (NULL == (pDialog->inviteTimer =
                    SIPTIMER_Create(pDialog->hContext))) {
                return;
            }
        }

        SIPTIMER_Start(pDialog->inviteTimer, UA_InviteExpiryCB, pDialog,
                (pMsg->Expires * 1000), FALSE);

        SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                "DIALOG_StartInviteTimer: started! for hDialog:%x, Expires=%d",
                (int)pDialog, pMsg->Expires, 0);
    }
}

void DIALOG_DestroyReqPendingRetry(
    tSipDialog *pDialog)
{
    if (pDialog->reqPendingTmr) {
        SIPTIMER_Stop(pDialog->reqPendingTmr);
        SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
            "DIALOG_DestroyReqPendingRetry: Has destroyed the timer:%X "
            "hTimer:%X", (int)pDialog, (int)pDialog->reqPendingTmr, 0);
        SIPTIMER_Destroy(pDialog->reqPendingTmr);
        pDialog->reqPendingTmr = NULL;
    }

    /* Free pMsg */
    if (pDialog->pReInviteUpdate) {
        SIP_freeMsg(pDialog->pReInviteUpdate);
        pDialog->pReInviteUpdate = NULL;
    }
}

void DIALOG_ClientReqPendingRetry(
    tSipDialog *pDialog)
{
    uint32  timeout;

    /*
     * RFC 3261 14.1
     * Create a timer to send re-INVITE again, if receive 491 request pending.
     */
    if (NULL == pDialog->reqPendingTmr) {
        if (NULL == (pDialog->reqPendingTmr =
                SIPTIMER_Create(pDialog->hContext))) {
            return;
        }
    }

    /*
     * RFC 3261 14.1
     * 1. If the UAC is the owner of the Call-ID of the dialog ID
     *    (meaning it generated the value), T has a randomly chosen value
     *    between 2.1 and 4 seconds in units of 10 ms.
     * 2. If the UAC is not the owner of the Call-ID of the dialog ID, T
     *    has a randomly chosen value of between 0 and 2 seconds in units
     *    of 10 ms.
     */
    if (TRUE == pDialog->id.isCallIdOwner) {
        timeout = SIP_randInt(210, 400) * 10;
    }
    else {
        if (0 == (timeout = SIP_randInt(0, 200) * 10)) {
            /* If timeout is 0, send retry immediately. */
            UA_reqPendingExpiryCB(NULL, pDialog);
            return;
        }
    }

    SIPTIMER_Start(pDialog->reqPendingTmr, UA_reqPendingExpiryCB, pDialog,
            timeout, FALSE);

    SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
            "DIALOG_ClientReqPendingRetry: started! for hDialog:%x, Expires=%d",
            (int)pDialog, timeout, 0);
}

void DIALOG_ServerRespSessionExpires(
    tSipDialog     *pDialog,
    tSipIntMsg     *pMsg)
{
    if (0 != pDialog->sessionTimer.expires) {
        if (1 == pDialog->sessionTimer.isRefresher) {
            _DIALOG_StartSessionExpires(pDialog, UA_SessionRefresherCB,
                    pDialog->sessionTimer.expires / 2);
            SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                "DIALOG_ServerRespSessionExpires: started refresher timer for hDialog:%x expires:%d",
                (int)pDialog, (int)(pDialog->sessionTimer.expires / 2), 0);
        }
        else {
            _DIALOG_StartSessionExpires(pDialog, UA_SessionExpiryCB,
                    pDialog->sessionTimer.expires -
                    CALC_MIN(32, pDialog->sessionTimer.expires / 3));
            SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                "DIALOG_ServerRespSessionExpires: started expiry timer for hDialog:%x expires:%d",
                (int)pDialog, (int)pDialog->sessionTimer.expires -
                CALC_MIN(32, pDialog->sessionTimer.expires / 3), 0);
        }
        /*
         * Add the 'Require' for Timer STEVE -This seems to have no
         * impact so commenting out
         */
        /* HF_CopyInsert(&pMsg->pHFList, eSIP_REQUIRE_HF, "timer", 0); */
        /* HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_REQUIRE_HF); */
    }
    return;
}

void DIALOG_ClientRespSessionExpires(
    tSipDialog     *pDialog,
    tSipIntMsg     *pMsg)
{
    if (SIP_OK == HF_CheckRequiredHF(&pMsg->pHFList, "timer")
            && 0 != pMsg->SessionTimer.expires) {
        if (eSIP_REFRESHER_UAS != pMsg->SessionTimer.refresher) {
            /* Then let's start a timer to re-INVITE periodically */
            pDialog->sessionTimer.isRefresher = 1;
            pDialog->sessionTimer.expires = pMsg->SessionTimer.expires;
            pDialog->sessionTimer.minSeHf = (0 == pMsg->MinSE) ? 90 : pMsg->MinSE;
            _DIALOG_StartSessionExpires(pDialog, UA_SessionRefresherCB,
                    pDialog->sessionTimer.expires / 2);
            SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                "DIALOG_ClientRespSessionExpires: started refresher timer for hDialog:%x expires:%d",
                (int)pDialog, (int)(pDialog->sessionTimer.expires / 2), 0);
        }
        else {
            pDialog->sessionTimer.isRefresher = 0;
            pDialog->sessionTimer.expires = pMsg->SessionTimer.expires;
            pDialog->sessionTimer.minSeHf = (0 == pMsg->MinSE) ? 90 : pMsg->MinSE;
            _DIALOG_StartSessionExpires(pDialog, UA_SessionExpiryCB,
                    pDialog->sessionTimer.expires -
                    CALC_MIN(32, pDialog->sessionTimer.expires / 3));
            SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                "DIALOG_ClientRespSessionExpires: started expiry timer for hDialog:%x expires:%d",
                (int)pDialog, (int)pDialog->sessionTimer.expires -
                CALC_MIN(32, pDialog->sessionTimer.expires / 3), 0);
        }
    }
    else {
        /* No session timer stuff so reset the values */
        pDialog->sessionTimer.expires = 0;
        pDialog->sessionTimer.minSeHf = 0;
    }
    return;
}

/* 
 *****************************************************************************
 * ================DIALOG_Stop2xx()===================
 *
 * This function will turn off and clean up the mechanisms for both server 
 * and client dialogs to handle 2xx retry
 *
 * pDialog = A pointer to the dialog sending the 200 response.
 *
 * RETURNS: 
 *      SIP_OK: The timer was stopped.
 *      SIP_FAILED: No timer was stopped. It was already stopped or 
 *                  the dialog handle is invalid.
 *
 *****************************************************************************
 */
vint DIALOG_Stop2xx(tSipDialog *pDialog)
{
    tDialog2xx *pOk;
    
    if (!pDialog)
        return (SIP_FAILED);

    pOk = &pDialog->ok;

    /* stop the timer mechanism */
    if (pOk->hTimeoutTimer) {
        SIPTIMER_Stop(pOk->hTimeoutTimer);
    }
    else {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                "DIALOG_Stop2xx: Error no active timer hDialog:%X hTimer:%X", 
                (int)pDialog, (int)pOk->hTimeoutTimer, 0);
    }
    if (pOk->hRetryTimer) {
        SIPTIMER_Stop(pOk->hRetryTimer);
    }

    if (pOk->hTransport) {
        TRANSPORT_Dealloc(pOk->hTransport);
        pOk->hTransport = NULL;
    }
    else {
        SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
                "DIALOG_Stop2xx: Error no transport resource to deallocate hDialog:%X", (int)pDialog, 0, 0);
    }

    /* Clear out retryMax, it's for UAC to absort 2xx retries. */
    pOk->retryMax = 0;

    /* Clear out t1, this is used to indicate if the timer is active or not */
    if (0 != pOk->t1) {
        pOk->t1 = 0;
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

void DIALOG_DestroySessionExpires(tSipDialog *pDialog)
{
    tDialogSessionTimer *stimer_ptr;

    stimer_ptr = &pDialog->sessionTimer;

    /* stop the timer mechanism */
    if (stimer_ptr->hTimer) {
        SIPTIMER_Stop(stimer_ptr->hTimer);
        SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
            "DIALOG_DestroySessionTimer: Has destroyed the timer:%X hTimer:%X",
            (int)pDialog, (int)stimer_ptr->hTimer, 0);
        SIPTIMER_Destroy(stimer_ptr->hTimer);
        stimer_ptr->hTimer = NULL;
    }
    stimer_ptr->expires = 0;
    stimer_ptr->isRefresher = 0;
    stimer_ptr->minSeHf = 0;
    return;
}

/* 
 *****************************************************************************
 * ================DIALOG_GetContext()===================
 *
 * This function returns the 'context' member of the dialog object passed in
 *
 * RETURNS: 
 *      the value in the 'context' member of the object pointed to by pDialog
 *
 *****************************************************************************
 */
tSipHandle DIALOG_GetContext(tSipHandle hDialog)
{
    return (((tSipDialog *)hDialog)->hContext);
}

/* 
 *****************************************************************************
 * ================DIALOG_ChangeState()===================
 *
 * This function is the interface called when the dialog needs to change state
 *
 * RETURNS: 
 *      Nothing
 *
 *****************************************************************************
 */
void DIALOG_ChangeState(
    tSipDialog     *pDialog, 
    tSipDialogState state)
{
    pDialog->currentState = state;
    /* Everytime we change state then reset the authenication counter. */
    pDialog->authCache.count = 0;
}

/* 
 *****************************************************************************
 * ================DIALOG_Authenticate()===================
 *
 * If a request is challenged to provide authentication details
 * This function will build the request including the authentication 
 * details.  These credentials used to authenticate aret he same ones
 * passed in when the Client dialog was initialized (via DIALOG_InitClient())
 *
 * pDialog = A pointer to the dialog
 *
 * pMsg = A pointer to the sip message containing the response with the 
 *        'challenge'.
 *
 * RETURNS: 
 *     tSipIntMsg* = A pointer to the new INVITE reqeust containg the 
 *                   authentication details.
 *     NULL = Could not authnticate.
 *
 *****************************************************************************
 */
tSipIntMsg* DIALOG_Authenticate(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg)
{
    tSipIntMsg *pResp;
    
    if (pDialog->authCache.count >= 1 
            || DLLIST_IsEmpty(&pMsg->AuthorizationList)) {
        /* We already attempted to authenticate so we need to prevent 
         * a infinite loop of continually retrying to authenticate 
         */
        return (NULL);
    }
    else {
        /* cache the 'authorization' list */
        if (pMsg->code == eSIP_RSP_PROXY_AUTH_REQUIRED) {
            pDialog->authCache.credHf = eSIP_PROXY_AUTHORIZATION_HF;
        }
        else {
            pDialog->authCache.credHf = eSIP_AUTHORIZATION_HF;
        }
        pDialog->authCache.authList = pMsg->AuthorizationList;
        DLLIST_InitList(&pMsg->AuthorizationList);
        pDialog->authCache.count++;
        if ((pResp = SIP_allocMsg()) != NULL) {
            pResp->msgType = eSIP_REQUEST;
            pResp->method = pMsg->CSeq.method;
            if (pResp->method == eSIP_INVITE) {
                if (pDialog->replaces.szCallId[0] != 0) {
                    pResp->Replaces = pDialog->replaces;
                    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_REPLACES_HF);
                }
                /* copy in any cached header field values */
                if (pDialog->pHFList) {
                    if (SIP_OK != HF_CopyAll(&pResp->pHFList,
                            pDialog->pHFList, &pResp->x.ECPresenceMasks)) {
                        goto errorExit;
                    }
                }

                /* Add any cached expires */
                if (0 != pDialog->expires) {
                    pResp->Expires = pDialog->expires;
                    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_EXPIRES_HF);
                }

                /* set up the sdp params */
                if ((pResp->pSessDescr = SESSION_MakeSdp(&pDialog->session)) != NULL) {
                    /* set the content type */
                    pResp->ContentType = eCONTENT_TYPE_SDP;
                    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
                    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
                }
                /* Set up other payloads to include in the body of the request. */
                if (0 != pDialog->session.sess.otherPayload[0]) {
                    if (NULL != (pResp->pMsgBody = (tSipMsgBody *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_MSG_BODY))) {
                        OSAL_snprintf(pResp->pMsgBody->msg, SIP_MAX_TEXT_MSG_SIZE, "%s",
                                pDialog->session.sess.otherPayload);
                        pResp->ContentType = eCONTENT_TYPE_MULTIPART;
                    }
                }

                if (DIALOG_PopulateRequest(pDialog, pResp) == SIP_OK) {
                    return (pResp);
                }
            }
            else if (pResp->method == eSIP_SUBSCRIBE) {
                /* copy in any cached header field values */
                if (pDialog->pHFList) {
                    if (SIP_OK != HF_CopyAll(&pResp->pHFList,
                            pDialog->pHFList, &pResp->x.ECPresenceMasks)) {
                        goto errorExit;
                    }
                }
                if (DIALOG_PopulateRequest(pDialog, pResp) == SIP_OK) {
                    return (pResp);
                }
            }
            else {       
                if (DIALOG_PopulateRequest(pDialog, pResp) == SIP_OK) {
                    return (pResp);
                }
            }
        }
    }   

errorExit:
    SIP_DebugLog(SIP_DB_DIALOG_LVL_1,
            "DIALOG_Authenticate: Failed to make method with "
            "authorization for hDialog:%X", (int)pDialog, 0, 0);
    
    if (pResp) {
        SIP_freeMsg(pResp);
    }
    return (NULL);
}

/* 
 *****************************************************************************
 * ================DIALOG_Destroy2xx()===================
 *
 * When a 200 (OK) response is returned to an INVITE reqeust.  The UAS
 * must continue to send the 200 response until an ACK is received from the 
 * remote device's UAC.  Once the ACK is received the retry mechanism
 * must be stopped.  This function kills (deactivates) both the client and 
 * server mechanisms needed to support 2xx retry handling.
 *
 * pDialog = A pointer to the dialog sending the 200 response.
 *
 * RETURNS: 
 *     Nothing
 *
 *****************************************************************************
 */
static void _DIALOG_Destroy2xx(tSipDialog  *pDialog)
{
    tDialog2xx *pOk;

    pOk = &pDialog->ok;

    if (pOk->p2xxMsg) {
        SIP_freeMsg(pOk->p2xxMsg);
        pOk->p2xxMsg = NULL;
    }
        
    /* destroy the 2xx recv mechanism */
    if (pOk->hTimeoutTimer) {
        SIPTIMER_Destroy(pOk->hTimeoutTimer);
        pOk->hTimeoutTimer= NULL;
    }
    if (pOk->hRetryTimer) {
        SIPTIMER_Destroy(pOk->hRetryTimer);
        pOk->hRetryTimer = NULL;
    }
        
    if (pOk->hTransport) {
        TRANSPORT_Dealloc(pOk->hTransport);
        pOk->hTransport = NULL;
    }
    return;
}

tUri* DIALOG_GetUri(
    tSipDialog *pDialog, 
    tSipIntMsg *pMsg)
{
    tRouteHFE *pRoute;
    tDLListEntry *pEntry;
    
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->RouteList, &pEntry)) {
        pRoute = (tRouteHFE *)pEntry;
        if (pDialog->isStrict) {
            return &pMsg->requestUri;
        }
        else {
            return &pRoute->uri;
        }
    }
    else {
        if (pDialog->haveRemoteFqdn) {
            return &pDialog->remoteTargetUri;
        }
    }
    return NULL;
}

vint DIALOG_PopulateReplaces(
    tSipHandle   hDialog, 
    tReplacesHF *pReplaces)
{
    tSipDialog *pDialog;
    pDialog = (tSipDialog*)hDialog;
    if (DIALOG_IS_CONFIRMED(pDialog->currentState)) {
        OSAL_strcpy(pReplaces->szCallId, pDialog->id.szCallId);
        OSAL_strcpy(pReplaces->szToTag, pDialog->id.szRemoteTag);
        OSAL_strcpy(pReplaces->szFromTag, pDialog->id.szLocalTag);
        return (SIP_OK);
    }
    return (SIP_FAILED);
}

vint DIALOG_PopulateReferTo(
    tSipHandle   hDialog, 
    tSipIntMsg  *pMsg)
{
    vint status;
    tSipDialog *pDialog;
    pDialog = (tSipDialog*)hDialog;
    status = DIALOG_PopulateReplaces(hDialog, &pMsg->ReferTo.replaces);
    if (status == SIP_OK) {
        pMsg->ReferTo.uriPlus.uri = pDialog->remoteUri.uri;
    }
    return status;
}

void DIALOG_CleanPrack(tDialogPrack *pPrack)
{
    if (pPrack->hRetryTimer) {
        SIPTIMER_Destroy(pPrack->hRetryTimer);
    }
    if (pPrack->hTimeoutTimer) {
        SIPTIMER_Destroy(pPrack->hTimeoutTimer);
    }    
    if (pPrack->pMsg) {
        SIP_freeMsg(pPrack->pMsg);
    }
    OSAL_memSet(pPrack, 0, sizeof(tDialogPrack));
    return;
}

tDialogEvent* DIALOG_CacheEvent(
    tSipDialog   *pDialog,
    OSAL_Boolean  isSubscriber, 
    tEventHF     *pEvt,
    uint32        timeout)
{
    tDialogEvent *pEvent;
    tDLList      *pList;

    if (OSAL_TRUE == isSubscriber) {
        pList = &pDialog->subscribeList;
    }
    else {
        pList = &pDialog->notifyList;
    }
    
    /* 
     * Only cache it if it doesn't exist.  This could be called because
     * the application is trying to refresh the subscription
     */
    if (NULL != (pEvent = DIALOG_SearchEvtByEvt(pDialog, isSubscriber, pEvt))) {
        if (OSAL_TRUE == isSubscriber) {
            pEvent->timeout = timeout;
        }
        /* this must be a re-subscription so set a flag */
        pEvent->isResent = TRUE;
    }
    else {
        if (NULL != (pEvent = DIALOG_MallocEvent())) {
            pEvent->evt = *pEvt;
            if (OSAL_TRUE == isSubscriber) {
                pEvent->pDialog = pDialog;
                /* create a timer */
                pEvent->hTimer = SIPTIMER_Create(pDialog->hContext);
                pEvent->timeout = timeout;
            }
            DLLIST_Enqueue(pList, &pEvent->dll);
        }
    }
    return (pEvent);
}

tDialogEvent* DIALOG_SearchEvtByEvt(
    tSipDialog   *pDialog,
    OSAL_Boolean  isSubscriber, 
    tEventHF     *pEvt)
{
    tDLList      *pList;
    tDLListEntry *pEntry;
    tDialogEvent *pEvent;
    
    if (OSAL_TRUE == isSubscriber) {
        pList = &pDialog->subscribeList;
    }
    else {
        pList = &pDialog->notifyList;
    }
        
    pEntry = NULL;
    while (0 != DLLIST_GetNext(pList, &pEntry)) {
        pEvent = (tDialogEvent *)pEntry;
        if ((OSAL_strcasecmp(pEvent->evt.szPackage, pEvt->szPackage) == 0) && 
                (OSAL_strcmp(pEvent->evt.szId, pEvt->szId) == 0)) {
            /* found it */
            return (pEvent);
        }
    }
    return (NULL);
}

tDialogEvent* DIALOG_SearchEvtByTrans(
    tSipDialog   *pDialog,
    OSAL_Boolean  isSubscriber,  
    tSipHandle    hTransaction)
{
    tDLListEntry *pEntry;
    tDialogEvent *pEvent;
    tDLList      *pList;
    
    if (OSAL_TRUE == isSubscriber) {
        pList = &pDialog->subscribeList;
    }
    else {
        pList = &pDialog->notifyList;
    }
    
    pEntry = NULL;
    while (0 != DLLIST_GetNext(pList, &pEntry)) {
        pEvent = (tDialogEvent *)pEntry;
        if (pEvent->hTransaction == hTransaction) {
            /* found it */
            return (pEvent);
        }
    }
    return (NULL);
}

void DIALOG_WakeUpSubscriptions(
    tSipDialog   *pDialog)
{
    tDLListEntry *pEntry;
    tDialogEvent *pEvent;
    tDLList      *pList;

    pList = &pDialog->subscribeList;
    pEntry = NULL;
    while (0 != DLLIST_GetNext(pList, &pEntry)) {
        pEvent = (tDialogEvent *)pEntry;
        if (0 != pEvent->hTimer) {
            SIPTIMER_WakeUp(pEvent->hTimer);
        }
    }
    return;
}

vint DIALOG_RemoveEventByTrans(
    tSipDialog    *pDialog, 
    OSAL_Boolean   isSubscriber, 
    tSipHandle     hTransaction,
    tDialogEvent **ppEvt)
{
    tDLList *pList;
    
    if (OSAL_TRUE == isSubscriber) {
        pList = &pDialog->subscribeList;
    }
    else {
        pList = &pDialog->notifyList;
    }
  
    /* first find the subscription */
    *ppEvt = DIALOG_SearchEvtByTrans(pDialog, isSubscriber, hTransaction);
    if (NULL != *ppEvt) {
        /* found it */
        DLLIST_Remove(pList, &(*ppEvt)->dll);
        if (DLLIST_IsEmpty(&pDialog->subscribeList) && 
                DLLIST_IsEmpty(&pDialog->notifyList)) {
            return (SIP_NO_DATA);
        }
        return (SIP_OK);
    }
    return (SIP_NOT_FOUND);
}

vint DIALOG_RemoveEventByEvt(
    tSipDialog    *pDialog, 
    OSAL_Boolean   isSubscriber, 
    tEventHF      *pEvent,
    tDialogEvent **ppEvt)
{
    tDLList      *pList;
    
    if (OSAL_TRUE == isSubscriber) {
        pList = &pDialog->subscribeList;
    }
    else {
        pList = &pDialog->notifyList;
    }

    /* first find the subscription */
    *ppEvt = DIALOG_SearchEvtByEvt(pDialog, isSubscriber, pEvent);
    if (NULL != *ppEvt) {
        /* found it */
        DLLIST_Remove(pList, &(*ppEvt)->dll);
        if (DLLIST_IsEmpty(&pDialog->subscribeList) && 
                DLLIST_IsEmpty(&pDialog->notifyList)) {
            return (SIP_NO_DATA);
        }
        return (SIP_OK);
    }
    return (SIP_NOT_FOUND);
}


void DIALOG_FreeEvent(tDialogEvent *pEvent)
{
    pEvent->evt.szId[0] = 0;
    pEvent->hTransaction = 0;
    pEvent->isResent = FALSE;
    pEvent->pDialog = NULL;
    DLLIST_InitEntry(&pEvent->dll);
    if (NULL != pEvent->hTimer) {
        SIPTIMER_Destroy(pEvent->hTimer);
        pEvent->hTimer = NULL;
    }
    SIP_memPoolFree(eSIP_OBJECT_DIALOG_EVENT, &pEvent->dll);
}

static tDialogEvent* DIALOG_MallocEvent(void)
{
    return ((tDialogEvent *)SIP_memPoolAlloc(eSIP_OBJECT_DIALOG_EVENT));
}

static void DIALOG_FreeList(tDLList *pList)
{
    tDLListEntry *pEntry;
    
    while (NULL != (pEntry = DLLIST_Dequeue(pList))) {
        DIALOG_FreeEvent((tDialogEvent *)pEntry);
    }
}

void DIALOG_CacheBranch(
    tSipDialog  *pDialog,
    tSipIntMsg  *pMsg)
{
    /* Get the topmost via branch value 
     * from the message and save it in the 
     * dialog.
     */
    tDLListEntry *pEntry;
    
    /* get the topmost via and change the branch parameter */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        OSAL_strcpy(pDialog->szBranch, ((tViaHFE *)pEntry)->szBranch);
    }
    return;
}

/*
 *****************************************************************************
 * ================DIALOG_StartKeepalives()===================
 *
 * This function is used to start a timer that is sending nat keep-alives.
 * If "Keep_Alive_Enabled" is enable and this session is Emergency session
 * set-up in case of no registration and there is the response value of 'keep',
 * starting a timer to send keep-alives.
 *
 * pDialog = A pointer to the dialog
 *
 * pMsg = A pointer to the sip message containing the response with the
 *        value of 'keep' of Via header.
 *
 * RETURNS:
 *
 *****************************************************************************
 */
void DIALOG_StartKeepalives(
    tSipDialog *pDialog,
    tSipIntMsg *pMsg)
{
    tDLListEntry   *pEntry;
    uint32          interval;
    tSipHandle      hTransport;

    /* get the value of 'keep' of topmost via */
    pEntry   = NULL;
    interval = 0;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        interval = ((tViaHFE *)pEntry)->keepaliveFeq;
    }

    if ((OSAL_TRUE == pDialog->natKeepaliveEnable) && (interval > 0)) {
        /* create a timer */
        if (NULL == pDialog->keepaliveTimer.hTimer) {
            if (NULL == (pDialog->keepaliveTimer.hTimer =
                    SIPTIMER_Create(pDialog->hContext))) {
                SIP_DebugLog(SIP_DB_DIALOG_LVL_3,
                        "DIALOG_StartKeepalives: Failed to create timer! "
                        "for hDialog:%x",
                        (int)pDialog, 0, 0);
                return;
            }
        }

        /* Get target Fd and Addr */
        if (NULL != (hTransport = TRANS_GetTransport(pDialog->hTrans))) {
            if (SIP_OK == TRANSPORT_GetTransportAddr(
                    hTransport,
                    &pDialog->keepaliveTimer.args.refreshAddr,
                    &pDialog->keepaliveTimer.args.refreshFd)) {
                /* Start the timer */
                SIPTIMER_Start(pDialog->keepaliveTimer.hTimer, UA_KeepalivesCB,
                        &pDialog->keepaliveTimer.args, interval*1000, TRUE);
            }
        }
    }
}

