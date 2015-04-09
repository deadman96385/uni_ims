/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include "sip_sip.h"
#include "sip_timers.h"
#include "sip_auth.h"
#include "sip_dbase_sys.h"
#include "sip_dbase_endpt.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "sip_ua.h"
#include "sip_tu.h"
#include "sip_parser_enc.h"
#include "sip_parser_dec.h"
#include "_sip_callback.h"
#include "_sip_helpers.h"
#include "_sip_fsm.h"
#include "sip_app.h"
#include "sip_ua_server.h"
#include "sip_ua_client.h"
#include "sip_ua_error.h"
#include "sip_mem_pool.h"

/* registry of endpoint or TU users */
static tDLList _UA_Users;
static vint    _UA_maxDialogsPerUa;

static tContactHFE* _UA_getFirstAor(
    tUa *pUa)
{
    tDLListEntry *pAorEntry;
    tEPDB_Entry  *p;
    
    p = EPDB_Get(eEPDB_ADDR_OF_REC, pUa->epdb);
    if (p != NULL) {
         pAorEntry = NULL;
         if (DLLIST_GetNext(&p->x.dll, &pAorEntry)) {
            return ((tContactHFE *)pAorEntry);
         }
            
    } /* end of first while */
    return (NULL);
}

static vint _UA_buildVia(
    tUa        *pUa, 
    tSipIntMsg *pMsg) 
{
    tDLListEntry *pListEntry;
    tEPDB_Entry  *pDbEntry;
    tViaHFE      *pVia;
    tContactHFE  *pFqdn;

    /* build the via using the FQDN of the UA. This is RECOMMENDED in RFC3261 */
    if (HF_PresenceExists(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF)) {
        /* get the top most via */
        pListEntry = NULL;
        if (DLLIST_GetNext(&pMsg->ViaList, &pListEntry)) {
            pVia = (tViaHFE *)pListEntry;
           /* This function was added after interop.  The FQDN
            * should always be used regardless of if it's going to a 
            * target or proxy.
            * This is also a fix for MR1227 & 1226.
            */       
            pDbEntry = EPDB_Get(eEPDB_CONTACTS, pUa->epdb);
            if (pDbEntry) {
                /* get the top most fqdn in the list */
                pListEntry = NULL;
                if (DLLIST_GetNext(&pDbEntry->x.dll, &pListEntry)) {
                    pFqdn = (tContactHFE *)pListEntry;
                    pVia->uri.host = pFqdn->uri.host;
                    return (SIP_OK);
                }
            }
        }
    }
    
    return (SIP_FAILED);
}
static vint _UA_send2Target(
    tUa        *pUa,
    tUri       *pUri, 
    tSipHandle  hOwner,
    tSipIntMsg *pMsg, 
    tpfAppCB    pfApp,
    tSipHandle  hTransport,
    tSipHandle *hTransaction)
{
    tLocalIpConn *pLclConn;
    /* when sending to a target, make a copy of the uri 
     * because the transport layer will change the object
     * and those changes should only be temporary 
     */
    tUri uri = *pUri;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "_UA_send2Target:", 0, 0, 0);

    if (pMsg->msgType != eSIP_REQUEST) {
        return (SIP_BADPARM);           
    }

    /* build the via from the target proxy */
    if (_UA_buildVia(pUa, pMsg) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_2,
                "_UA_send2Target: Failed Could not build via ", 0, 0, 0);
        return (SIP_FAILED);
    }

    if (pMsg->method == eSIP_REGISTER) {
        pLclConn = &pUa->regLclConn;
    }
    else if (pMsg->method == eSIP_MESSAGE) {
        pLclConn = &pUa->txtLclConn;
    }
    else if (pMsg->method == eSIP_PUBLISH) {
        pLclConn = &pUa->pubLclConn;
    }
    else {
        pLclConn = &pUa->lclConn;
    }
    return TU_SendRequest(&uri, pMsg, pfApp, hOwner, pUa->taskId, hTransport,
            hTransaction, pLclConn);
}

static void _UA_populateEvt(
    tSipIntMsg  *pMsg,
    tUa         *pUa, 
    tSipDialog  *pDialog,
    tUaAppEvent *pE)
{
    uint32        len;
    uint32        maxLen;
    tDLListEntry *pEntry;
    tHdrFldList  *pCurr;
    vint          x;
    vint          numHFs;
    tViaHFE      *pVia;
    tContactHFE  *pContact;
    
    if (pMsg->msgType == eSIP_REQUEST) {
        /* populate more info about the caller */
        
        /* Bug 4161.  We shouldn't need this. */
        pE->resp.respCode = 0;
        OSAL_strncpy(pE->szDisplayName, pMsg->From.szDisplayName,
                sizeof(pE->szDisplayName));
        /* now the uri from who the call is being made */
        len = SIP_URI_STRING_MAX_SIZE;
        ENC_Uri(&pMsg->From.uri, pE->szRemoteUri, &len, 0);
        pE->szRemoteUri[len] = 0;
        /* and now the aor that the URI that the call attempt is for */
        len = SIP_URI_STRING_MAX_SIZE;
        ENC_Uri(&pMsg->To.uri, pE->szToUri, &len, 0);
        pE->szToUri[len] = 0;
        /* now encode the contact */
        pEntry = NULL;
        if (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
            pContact = (tContactHFE*)pEntry;
            len = SIP_URI_STRING_MAX_SIZE;
            ENC_Uri(&pContact->uri, pE->szContactUri, &len, 0);
            pE->szContactUri[len] = 0;

            /*
             * populate any capability info that may appear
             * as part of the Contact header (RCS 5.0)
             */
            pE->capabilitiesBitmap = pContact->capabilitiesBitmap;

            /*
             * if isFocus was in the Contact: header, then the event will indicate
             * that this is related to an IM Conference (RFC4579)
             */
            pE->isConference = (OSAL_Boolean)pContact->isFocus;
        }
    }
    else {
        /* must be a response */

        /* populate more info about the caller */
        OSAL_strncpy(pE->szDisplayName, pMsg->To.szDisplayName,
                sizeof(pE->szDisplayName));
        /* now the uri from who the call is being made */
        len = SIP_URI_STRING_MAX_SIZE;
        ENC_Uri(&pMsg->To.uri, pE->szRemoteUri, &len, 0);
        pE->szRemoteUri[len] = 0;
        /* and now the aor that the URI that the call attempt is for */
        len = SIP_URI_STRING_MAX_SIZE;
        ENC_Uri(&pMsg->From.uri, pE->szToUri, &len, 0);
        pE->szToUri[len] = 0;

        /* now encode the contact */
        pEntry = NULL;
        if (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
            pContact = (tContactHFE*)pEntry;
            len = SIP_URI_STRING_MAX_SIZE;
            ENC_Uri(&pContact->uri, pE->szContactUri, &len, 0);
            pE->szContactUri[len] = 0;

            /*
             * populate any capability info that may appear
             * as part of the Contact header (RCS 5.0)
             */
            pE->capabilitiesBitmap = pContact->capabilitiesBitmap;

            /* Get expire time */
            if (pUa->regLclConn.addr.port == 0) {
                /*
                 * If the port according to the application is zero
                 * then we don't know what port to look for, so
                 * just grab the expires.
                 */
                pE->expires = pContact->expires;
            }
            else if (pContact->uri.host.port == pUa->regLclConn.addr.port) {
                pE->expires = pContact->expires;
            }
            else if (pUa->regLclConn.addr.port == SIP_DEFAULT_IPADDR_PORT && 
                  (pContact->uri.host.port == 0 || 
                  pContact->uri.host.port == SIP_DEFAULT_IPADDR_PORT)) {
                pE->expires = pContact->expires;
            }

            /*
             * if isFocus was in the Contact: header, then the event will indicate
             * that this is related to an IM Conference (RFC4579)
             */
            pE->isConference = (OSAL_Boolean)pContact->isFocus;

            /* check if there is public gruu, if find contact. */
            if (0 != pContact->szPublicGruu[0]) {
                OSAL_strncpy(pE->szPublicGruu, pContact->szPublicGruu,
                        sizeof(pContact->szPublicGruu));
            }
        }
        
        /* Report a reason phrase if there is one. */
        if (pMsg->pReasonPhrase) {
            OSAL_strncpy(pE->resp.szReasonPhrase, pMsg->pReasonPhrase->msg,
                    SIP_EVT_STR_SIZE_BYTES - 1);
        }

        pEntry = NULL;
        if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
            pVia = (tViaHFE *)pEntry;
            pE->resp.rport = pVia->rport;
            if (eNwAddrIPv4 == pVia->uri.host.addressType) {
                pE->resp.receivedAddr.type = OSAL_NET_SOCK_UDP;
                pE->resp.receivedAddr.ipv4 = pVia->received.v4.ul;
            }
            else {
                pE->resp.receivedAddr.type = OSAL_NET_SOCK_UDP_V6;
                OSAL_memCpy(pE->resp.receivedAddr.ipv6, pVia->received.v6, sizeof(pVia->received.v6));
            }
        }
    }
    
    /* 
     * Check if there's any service-route header field values. 
     * If so then include them in the event destined for the application.
     */
    x = 0;
    if (!DLLIST_IsEmpty(&pMsg->ServiceRouteList)) {
        len = OSAL_snprintf(&pE->szHeaderFields[x][0], SIP_EVT_STR_SIZE_BYTES,
                "%s: ", SIP_SERVICE_ROUTE_HF_STR);
        maxLen = SIP_EVT_STR_SIZE_BYTES - len;
        ENC_Route(&pMsg->ServiceRouteList, &pE->szHeaderFields[x][len],
                &maxLen);
        x++;
    }

    /* Check if theres is Min-SE for 422 Session Interval too Short */
    if (0 != pMsg->MinSE) {
        len = OSAL_snprintf(&pE->szHeaderFields[x][0], SIP_EVT_STR_SIZE_BYTES,
                "%s: %d", SIP_MIN_SE_HF_STR, pMsg->MinSE);
        x++;
    }
    /* 
     * now populate all the other header fields.
     * Because HF_Insert() would insert the last parsed item to the first one,
     * it makes the HF's order is reversed. So needs to reverse it here .
     */
    pCurr = pMsg->pHFList;
    numHFs = 0;
    /* Find number of the HFs from pHFList for reversing the order */
    while (pCurr) {
        if (numHFs > SIP_MAX_HEADER_FIELDS) {
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "_UA_populateEvt: the HF list size is too large",
                    0, 0, 0);
            break;
        }
        if (pCurr->pStart) {
            numHFs++;
        }
        pCurr = pCurr->pNext;
    }
    /* Copy the HF to szHeaderFields[] from the end */
    pCurr = pMsg->pHFList;
    x += numHFs;
    numHFs = x;
    while (NULL != pCurr) {
        x--;
        OSAL_strncpy(&pE->szHeaderFields[x][0], pCurr->pStart,
                    SIP_EVT_STR_SIZE_BYTES);
        pCurr = pCurr->pNext;

    }
    /* NULL terminate the array of char pointers */
    pE->szHeaderFields[numHFs][0] = 0;
    return;
}

static int _UA_search4Uri(
    tUa        *pUa,
    const tUri *pTargetUri,
    vint        isAor)
{
    tDLListEntry *pEntry;
    tEPDB_Entry  *p;
    tUri         *pUri;
    int           idx;

    idx = 0;

    if (isAor) {
        p = EPDB_Get(eEPDB_ADDR_OF_REC, pUa->epdb);
    }
    else {
        p = EPDB_Get(eEPDB_CONTACTS, pUa->epdb);
    }
    if (p != NULL) {
        /* loop through all the aor's looking for the right dude */
        pEntry = NULL;
        while(DLLIST_GetNext(&p->x.dll, &pEntry)) {
            pUri = &((tContactHFE *)pEntry)->uri;
#ifdef UA_FIND_UA_VIA_USERNAME_ONLY
            if (!OSAL_strcasecmp(pUri->user, pTargetUri->user))
                return idx;
#else
            if (pTargetUri->host.addressType == pUri->host.addressType) {
                if (pTargetUri->host.addressType == eNwAddrDomainName) {
                    if(!OSAL_strcasecmp(pUri->host.x.domainName,
                            pTargetUri->host.x.domainName)) {
                        if (!OSAL_strcasecmp(pUri->user, pTargetUri->user))
                            return idx;
                    }
                }
                else if (eNwAddrIPv6 == pTargetUri->host.addressType) {
                    if (0 == OSAL_memCmp(pUri->host.x.ip.v6,
                            pTargetUri->host.x.ip.v6,
                            sizeof(pUri->host.x.ip.v6))) {
                        if (!OSAL_strcasecmp(pUri->user, pTargetUri->user))
                            return idx;
                    }
                }
                else {
                    if (pUri->host.x.ip.v4.ul == pTargetUri->host.x.ip.v4.ul) {
                        if (!OSAL_strcasecmp(pUri->user, pTargetUri->user))
                            return idx;
                    }
                }
            } /* end of if */
#endif
            idx++;
        } /* end of while */
    } 
    return -1;
}


/* 
 *****************************************************************************
 * ================UA_Entry()===================
 *
 * This function is the entry of the UA handler that processes 
 * inter-task messages.
 * There are currently 3 types of messages that a UA needs
 * 1) Timer event
 * 2) an incoming request (server msg)
 * 3) an incoming response (client msg)
 *
 * pIpc = A pointer to the interprocess communication message.
 *           see the definition of tSipIpcMsg for more details.
 *
 * RETURNS:
 *      Nothing.
 *
 ******************************************************************************
 */
void UA_Entry(tSipIpcMsg* pIpc)
{
    switch (pIpc->type) {
    case eSIP_IPC_TIMER_MSG:
        SIPTIMER_Expiry(pIpc->hOwner, pIpc->id);
        break;
    case eSIP_IPC_SERVER_MSG:
        UAS_Entry(pIpc->hOwner, pIpc->pMsg, pIpc->hTransaction, pIpc->id);
        break;
    case eSIP_IPC_CLIENT_MSG:
        UAC_Entry(pIpc->hOwner, pIpc->pMsg, pIpc->hTransaction, pIpc->id);
        break;
    case eSIP_IPC_ERROR_MSG:
        UAE_Entry(pIpc->hOwner, pIpc->pMsg, pIpc->hTransaction, pIpc->id);
        break;
    default:
        break;
    }
    return;
}

/* 
 *****************************************************************************
 * ================UA_InitModule()===================
 *
 * This function is used to initialize the UA module.  Mainly, it's sets all the
 * callback functions for each of the server side method handlers and
 * initializes the UA linked list.
 *
 * maxDialogsPerUa : An integer value representing the maximum number of dialogs
 *                   that can be created for any UA at any given time.
 *
 * RETURNS:
 *         SIP_OK:  Module was initialized and all supported method handlers
 *                  were registered with the UA_Server.
 *
 ******************************************************************************
 */
vint UA_InitModule(
    vint maxDialogsPerUa)
{
    SIP_MutexInit(&_UA_Users.lock);
    _UA_Users.isBackwards = 0;
    _UA_Users.pHead = NULL;
    _UA_Users.pTail = NULL;
#if (SIP_DEBUG_LOG)
    _UA_Users.listName = "_UA_Users";
    _UA_Users.cnt = 0;
#endif

    /* No dymanic allocate dialogs, set it to max */
    _UA_maxDialogsPerUa = SIP_DIALOGS_PER_UA_MAX;
    
    /* register the Server CallBacks */
    UAS_RegisterMethodCallBack(eSIP_INVITE, UAS_Invite);
    UAS_RegisterMethodCallBack(eSIP_ACK, UAS_Ack);
    UAS_RegisterMethodCallBack(eSIP_BYE, UAS_Bye);
    UAS_RegisterMethodCallBack(eSIP_CANCEL, UAS_Cancel);
    UAS_RegisterMethodCallBack(eSIP_REGISTER, UAS_Register);
    UAS_RegisterMethodCallBack(eSIP_REFER, UAS_Refer);
    UAS_RegisterMethodCallBack(eSIP_NOTIFY, UAS_Notify);
    UAS_RegisterMethodCallBack(eSIP_MESSAGE, UAS_Message);
    UAS_RegisterMethodCallBack(eSIP_PUBLISH, UAS_Publish);
    UAS_RegisterMethodCallBack(eSIP_SUBSCRIBE, UAS_Subscribe);
    UAS_RegisterMethodCallBack(eSIP_INFO, UAS_Info);
    UAS_RegisterMethodCallBack(eSIP_PRACK, UAS_Prack);
    UAS_RegisterMethodCallBack(eSIP_UPDATE, UAS_Update);
    UAS_RegisterMethodCallBack(eSIP_ERROR, UAS_Error);
    UAS_RegisterMethodCallBack(eSIP_OPTIONS, UAS_Options);
    /* add new methods here! */

    /* init the UA state machine */
    UASM_Init();
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_KillModule()===================
 *
 * This function will free all UA, dialog, session, EPDB objects
 * associated with them.
 *
 * RETURNS:
 *      Nothing.
 *
 ******************************************************************************
 */
void UA_KillModule(void)
{
    tUa          *pUa;
    tDLListEntry *pEntry;
    
    pEntry = NULL;
    SIP_Lock(_UA_Users.lock);
    while (DLLIST_GetNext(&_UA_Users, &pEntry)) {
        pUa = (tUa*)pEntry;
        /* Clean up registration stuff */
        UA_Dealloc(pUa);
    }
    /* Now kill off the UA themselves */
    DLLIST_Empty(&_UA_Users, eSIP_OBJECT_UA);
    SIP_Unlock(_UA_Users.lock);

    /* Free the mutex */
    SIP_MutexDestroy(_UA_Users.lock);
    return;
}

/* 
 *****************************************************************************
 * ================UA_Alloc()===================
 *
 * This function allocates a UA object and initializes it.  If there is an
 * unused UA object in the UA linked list then it will reuse it, otherwise it 
 * will get a new object from heap.
 *
 * RETURNS:
 *      tUa: A pointer to a UA object.
 *      NULL: No object could be allocated or reused.
 *
 ******************************************************************************
 */
tUa *UA_Alloc(void)
{
    tUa          *pUa;
    tDLListEntry *pEntry;

    pEntry  = NULL;
    pUa     = NULL;

    /* lock out the _UA_Users list */
    SIP_Lock(_UA_Users.lock);
    while (DLLIST_GetNext(&_UA_Users, &pEntry)) {
        pUa = (tUa*)pEntry;
        if (pUa->listState == eUA_LIST_STATE_INACTIVE) {
            /* 
             * Found a free one. Reset the UA and mark is as busy so no other
             * attempt is made to use this ua object.
             */
            pUa->listState = eUA_LIST_STATE_BUSY;
            /* 
             * Clean up the object, but don't touch the permenant data.  
             * i.e. dll data, dialog list 
             */
            OSAL_memSet(&pUa->epdb, 0, sizeof(pUa->epdb));
            OSAL_memSet(pUa->dialogList.dialogs, 0,
                (sizeof(tSipDialog) * pUa->dialogList.numDialogs));
            OSAL_memSet(&pUa->lclConn, 0, sizeof (tLocalIpConn));
            OSAL_memSet(&pUa->txtLclConn, 0, sizeof (tLocalIpConn));
            OSAL_memSet(&pUa->regLclConn, 0, sizeof (tLocalIpConn));
            OSAL_memSet(&pUa->pubLclConn, 0, sizeof (tLocalIpConn));
            OSAL_memSet(&pUa->Reg, 0, sizeof (tUaReg));
            OSAL_memSet(&pUa->Pub, 0, sizeof (tUaPub));
            OSAL_memSet(&pUa->ProxyReg, 0, sizeof (tUaProxyReg));
            pUa->taskId = 0;
            pUa->useCompactForms = 0;
            break;
        }
        pUa = NULL;
    }
    SIP_Unlock(_UA_Users.lock);
    
    if (!pUa) {
        /* grab one from memory pool */
        if (NULL == (pUa = (tUa *)SIP_memPoolAlloc(eSIP_OBJECT_UA))) {
            return (NULL);
        }
        pUa->listState = eUA_LIST_STATE_NONE;
        /* 
         * Clean up the object, but don't touch the permenant data.  
         * i.e. dll data, dialog list 
         */
        OSAL_memSet(&pUa->epdb, 0, sizeof(pUa->epdb));
        OSAL_memSet(pUa->dialogList.dialogs, 0,
            (sizeof(tSipDialog) * pUa->dialogList.numDialogs));
        OSAL_memSet(&pUa->lclConn, 0, sizeof (tLocalIpConn));
        OSAL_memSet(&pUa->txtLclConn, 0, sizeof (tLocalIpConn));
        OSAL_memSet(&pUa->regLclConn, 0, sizeof (tLocalIpConn));
        OSAL_memSet(&pUa->pubLclConn, 0, sizeof (tLocalIpConn));
        OSAL_memSet(&pUa->Reg, 0, sizeof (tUaReg));
        OSAL_memSet(&pUa->Pub, 0, sizeof (tUaPub));
        OSAL_memSet(&pUa->ProxyReg, 0, sizeof (tUaProxyReg));
        pUa->taskId = 0;
        pUa->useCompactForms = 0;

        /* Set the dll object */
        DLLIST_InitEntry(&pUa->dll);
        pUa->dialogList.numDialogs = _UA_maxDialogsPerUa;
    }
    return pUa;
}

/* 
 *****************************************************************************
 * ================UA_Dealloc()===================
 *
 * This function will clean up any resources used internally in the UA object 
 * and place the UA object into a state where it will be available for reuse.
 *
 * RETURNS:
 *      Nothing.
 *
 ******************************************************************************
 */
void UA_Dealloc(tUa* pUa)
{
    vint x;
    
    if (pUa->listState == eUA_LIST_STATE_NONE) {
        /* Then it was never inserted in the list so send it back to memory pool */
        EPDB_Empty(pUa->epdb);
        SIP_memPoolFree(eSIP_OBJECT_UA, (tDLListEntry *)pUa);
    }
    else {
        /* it's still on the _UA_Users list */
        EPDB_Empty(pUa->epdb);
        for (x = 0 ; x < SIP_MAX_NUM_AOR ; x++) {
            UA_CleanRegistration(&pUa->Reg[x]);
            UA_CleanPublish(&pUa->Pub[x]);
        }
        /* Free up resources and then place in state to be reused */
        pUa->listState = eUA_LIST_STATE_INACTIVE;
    }
    return;
}

vint UA_Insert(
    tUa *pUa)
{
    SIP_Lock(_UA_Users.lock);
    if (pUa->listState == eUA_LIST_STATE_NONE) {
        DLLIST_InitEntry(&pUa->dll);
        if (DLLIST_Enqueue(&_UA_Users, &pUa->dll) != SIP_OK) {
            SIP_Unlock(_UA_Users.lock);
            return (SIP_FAILED);
        }
        pUa->listState = eUA_LIST_STATE_ACTIVE;
    }
    else {
        /* then it's already inserted on the list, 
         * so just set active 
         */
        pUa->listState = eUA_LIST_STATE_ACTIVE;
    }
    SIP_Unlock(_UA_Users.lock);
    return (SIP_OK);
}

tUa* UA_Search(
    const tUri *pTargetUri)
{
    tDLListEntry *pUaEntry;
    tUa          *pUa;
    
    pUaEntry = NULL;
    SIP_Lock(_UA_Users.lock);
    /* first search the list of fqdn's */
    while (DLLIST_GetNext(&_UA_Users, &pUaEntry)) {
        pUa = ((tUa *)pUaEntry);
        if (pUa->listState == eUA_LIST_STATE_ACTIVE) {
            if (pTargetUri == NULL) {
                /* Then we the caller wishes to return 'ANY' UA */
                SIP_Unlock(_UA_Users.lock);
                return (pUa);
            }
            if (_UA_search4Uri(pUa, pTargetUri, 0) >= 0) {
                SIP_Unlock(_UA_Users.lock);
                return (pUa);
            }
        }
    } /* end of while */

    /* If we are here then we could not find the UA via 
     * in the FQDN.  Now try the AOR's.  We do this 
     * because many peer-to-peer modes (meaning no proxy)
     * may populate the Request-Uri with the AOR rather
     * than the FQDN
     */
    pUaEntry = NULL;
    while (DLLIST_GetNext(&_UA_Users, &pUaEntry)) {
        pUa = ((tUa *)pUaEntry);
        if (pUa->listState == eUA_LIST_STATE_ACTIVE) {
            if (_UA_search4Uri(pUa, pTargetUri, 1) >= 0) {
                SIP_Unlock(_UA_Users.lock);
                return (pUa);
            }
        }
    } /* end of while */
    SIP_Unlock(_UA_Users.lock);
    SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_SearchEndpoints: Error could not find UA", 0, 0, 0);
    return (NULL);
}

tUa* UA_NicErr(
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn)
{
    tDLListEntry *pUaEntry;
    tUa          *pUa;
    vint          x;
    vint          y;
    vint          foundMatch;

    pUaEntry = NULL;
    SIP_Lock(_UA_Users.lock);
    /* first search the list of fqdn's */
    while (DLLIST_GetNext(&_UA_Users, &pUaEntry)) {
        pUa = ((tUa *)pUaEntry);
        if (pUa->listState == eUA_LIST_STATE_ACTIVE) {
            foundMatch = 0;
            for (x = 0; x < SIP_MAX_NUM_AOR; ++x) {
                if (OSAL_netIsAddrZero(&pUa->Reg[x].refreshArgs.refreshAddr)) {
                    /* Ignore zero address. */
                    continue;
                }
                for (y = 0; y < MAX_DNS_IP_ADDRESSES; ++y) {
                    if (OSAL_netIsAddrPortEqual(
                            &pUa->Reg[x].refreshArgs.refreshAddr,
                            &pRmtConn->addrSet[y])) {
                        SIP_DebugLog(SIP_DB_UA_LVL_1,
                                "!!!!!!!UA_SearchAddr: match.\n", 0, 0, 0); 
                        SIP_DebugLogAddr(SIP_DB_UA_LVL_1,
                                &pUa->Reg[x].refreshArgs.refreshAddr);
                        SIP_DebugLogAddr(SIP_DB_UA_LVL_1,
                                &pRmtConn->addrSet[y]);
                        foundMatch = 1;
                    }
                }
            }
            if (foundMatch) {
                UAE_dispatch(pUa, NULL, NULL);
            }
        }
    } /* end of while */

    SIP_Unlock(_UA_Users.lock);

    return (NULL);
}

tSipDialog *UA_DialogInit(tUa *pUa)
{
    uint8 numDialogs;
    uint8 x;
    tSipDialog *pDialogs;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "%s: hUa:%X", (int)__FUNCTION__, (int)pUa, 0);

    if (pUa) {
        numDialogs = pUa->dialogList.numDialogs;
        pDialogs = pUa->dialogList.dialogs;
        for (x = 0 ; x < numDialogs ; x++, pDialogs++) {
            if (DIALOG_IS_IDLE(pDialogs->currentState)) {
                /* found an available dialog object */
                SIP_DebugLog(SIP_DB_UA_LVL_3, "found idle dialog", 0,0,0);
                return pDialogs;
            }
        }
    }
    return (NULL);
}

tSipDialog* UA_DialogSearch(
    tUa        *pUa, 
    tSipMsgType msgType,
    tSipMethod  method,
    char       *pTo,
    char       *pFrom,
    char       *pCallId)
{
    tSipDialog *pDialogs;
    uint8 numDialogs;
    uint8 x;
    
    numDialogs = pUa->dialogList.numDialogs;
    pDialogs = pUa->dialogList.dialogs;
    for (x = 0 ; x < numDialogs && pDialogs ; x++, pDialogs++) {
        if (DIALOG_IS_ACTIVE(pDialogs->currentState)) {
            if (DIALOG_IsMatched(pDialogs, msgType, method, pTo, pFrom,
                    pCallId) == SIP_OK) {
                return pDialogs;
            }
        }
    }
    return (NULL);
}

tSipDialog* UA_DialogSearchAll(
        tSipIntMsg *pMsg)
{
    tDLListEntry *pEntry;
    tSipDialog   *pDialog;
    
    pEntry = NULL;
    SIP_Lock(_UA_Users.lock);
    while (DLLIST_GetNext(&_UA_Users, &pEntry)) {
        if (((tUa*)pEntry)->listState == eUA_LIST_STATE_ACTIVE) {
            pDialog = UA_DialogSearch((tUa *)pEntry, pMsg->msgType, 
                    pMsg->CSeq.method, pMsg->To.szTag, pMsg->From.szTag,
                    pMsg->szCallId);
            if (pDialog) {
                SIP_Unlock(_UA_Users.lock);
                return (pDialog);
            }
        }
    }
    SIP_Unlock(_UA_Users.lock);
    return (NULL);
}

void UA_SessionInit(
    tUa        *pUa, 
    tSipDialog *pDialog,
    char       *pName)
{
    char           *pCoders;
    tPacketRate    *pPrate;
    tEPDB_Entry    *pEntry;
        
    /* use the defaults from the DCDB */
    pEntry = EPDB_Get(eEPDB_CODER_TYPES, pUa->epdb);
    if (pEntry) {
        pCoders = pEntry->x.cparm;
    }
    else {
        pCoders = NULL;
    }

    /* use the defaults from the DCDB */
    pEntry = EPDB_Get(eEPDB_PACKET_RATE, pUa->epdb);
    if (pEntry) {
        pPrate = &pEntry->x.packetRate;
    }
    else {
        pPrate = NULL;
    }
    SESSION_Init(&pDialog->session, pName, pCoders, pPrate);
    SESSION_ResetIsNew(&pDialog->session);
    return;
    
}

vint UA_SendRequest(
    tUa        *pUa,
    tUri       *pUri, 
    tSipHandle  hOwner,
    tSipIntMsg *pMsg, 
    tpfAppCB    pfApp,
    tSipHandle  hTransport,
    tSipHandle *hTransaction)
{
    /* 
     * The logic found in this function is 
     * specified in RFC3261 section 8.1.2 
     */
    
    tEPDB_Entry *pEntry;
    tUri        *pOutboundProxy;
    tUri        *pWimaxProxy;
    tUri        *pProxy; 

    if (pUa == NULL || pMsg == NULL) {
        return (SIP_BADPARM);
    }
    
    /* Check if there's a proxy configured.  */
    pProxy = NULL;
    if ((pEntry = EPDB_Get(eEPDB_PROXY_URI, pUa->epdb)) != NULL) {
        pProxy = pEntry->x.pUri;
    }
    
    /* 
     * Check if there's an outbound proxy configured.  If so that's the
     * destination of this request.
     */
    pOutboundProxy = NULL;
    if ((pEntry = EPDB_Get(eEPDB_OUTBOUND_PROXY_URI, pUa->epdb)) != NULL) {
        pOutboundProxy = pEntry->x.pUri;
    }
    
    /*
     * Check if there's a wimax proxy configured.  If so that's the
     * destination of this request.
     */
    pWimaxProxy = NULL;
    if ((pEntry = EPDB_Get(eEPDB_WIMAX_PROXY_URI, pUa->epdb)) != NULL) {
        pWimaxProxy = pEntry->x.pUri;
    }

    pMsg->isCompactForm = pUa->useCompactForms;
        
    if (pUri == NULL) {
        /* 
         * We are not sure where to send it. This typically happens during an 
         * early dialog, when the dialog route has not been established yet.
         */
        if (pWimaxProxy != NULL) {
            /* A wimax proxy is configured, this must be the next hop */
            return _UA_send2Target(pUa, pWimaxProxy, hOwner, pMsg,
                    pfApp, hTransport, hTransaction);
        }
        else if (pOutboundProxy != NULL) {
            /* An outbound proxy is configured, this must be the next hop */
            return _UA_send2Target(pUa, pOutboundProxy, hOwner, pMsg,
                    pfApp, hTransport, hTransaction);
        }
        else if (pProxy != NULL) {
            /* Then send it to the proxy since one is configuered */
            return _UA_send2Target(pUa, pProxy, hOwner, pMsg, pfApp, 
                    hTransport, hTransaction);
        }
        else {
            /* 
             * Otherwise, no available outbound proxy or proxy, 
             * try the endpoint directly.
             */
            return _UA_send2Target(pUa, &pMsg->requestUri, hOwner, pMsg,
                    pfApp, hTransport, hTransaction);
        }
    }
    else {
        /* Then we know exactly where to send it */
        if (pWimaxProxy != NULL) {
            /* A wimax proxy is configured, this must be the next hop */
            return _UA_send2Target(pUa, pWimaxProxy, hOwner, pMsg,
                    pfApp, hTransport, hTransaction);
        }
        else if (pOutboundProxy != NULL) {
            /* The outbound proxy is configured so this must be the next hop */
            return _UA_send2Target(pUa, pOutboundProxy, hOwner, pMsg, pfApp,
                    hTransport, hTransaction);
        }
        else {
            /* Send directly to the known remote target */
            return _UA_send2Target(pUa, pUri, hOwner, pMsg, pfApp,
                    hTransport, hTransaction);
        }
    }
}

void UA_AppEvent(
    tUa         *pUa, 
    tSipDialog  *pDialog, 
    tUaEvtType   type, 
    tSipIntMsg  *pMsg,
    tSipHandle   hArg)
{
    tUaAppEvent    *pEvent;
    uint32          len;
    vint            x;
    tDLListEntry   *pEntry; 
    tReplacesHF    *pReplaces;
    tContactHFE    *pContact;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_AppEvent: hUa:%X event %d", (int)pUa, (int)type, 0);

    if (pUa == NULL) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_AppEvent: FAILED hUa:%X is NULL! event %d", 
                (int)pUa, (int)type, 0);
        return;
    }

    /* 
     * Write the tUaEvent object needed to signal to 
     * the application that an event is ready for the app to process.
     * First clean it before we use it.
     */
    /* BUG 4161. This needs investigation of why we can't zero this out */
    /* OSAL_memSet(&pUa->event.event, 0, sizeof(tUaAppEvent)); */
    pUa->event.event.header.type = type;
    pUa->event.event.header.hOwner = pDialog;
    pUa->event.event.header.hUa = pUa;
    /* The rest of the function will write the tUaAppEvent object */
    pEvent = &pUa->event.event;

    if (pMsg) {
        _UA_populateEvt(pMsg, pUa, pDialog, pEvent);    
    }

    switch (type) {
        case eUA_CALL_ATTEMPT:
            /* find the session info */
            pEvent->sessNew = pDialog->session.isNew;
            OSAL_memCpy(&pEvent->msgBody.session, &pDialog->session.sess,
                    sizeof(tSession));
            pReplaces = &pMsg->Replaces;
            pEvent->hDialogReplaces = UA_DialogSearch(pUa, eSIP_REQUEST,
                    eSIP_INVITE, pReplaces->szToTag, pReplaces->szFromTag,
                    pReplaces->szCallId);
            break;

        case eUA_RESPONSE:
            /* 3xx codes will populate the contact header field if it exists */
            if (MSGCODE_ISREDIRECT(pMsg->code)) {
                pEntry = NULL;
                if (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
                    pContact = (tContactHFE *)pEntry;
                    len = SIP_URI_STRING_MAX_SIZE;
                    ENC_Uri(&pContact->uri, pEvent->szToUri, &len, 0);
                    pEvent->szToUri[len] = 0;
                }
            }
            /* find the session info */
            pEvent->sessNew = pDialog->session.isNew;
            OSAL_memCpy(&pEvent->msgBody.session, &pDialog->session.sess,
                    sizeof(tSession));
            pEvent->resp.respCode = (uint32)MSGCODE_GetNum((tSipMsgCodes)hArg);
            /*
             * Only handle the 503 (Service Unavailable) response to
             * an initial INVITE request.
             */
            if (SIP_RSP_SERVICE_UNAVAIL == pEvent->resp.respCode) {
                if ((eSIP_INVITE == pMsg->CSeq.method) &&
                        (eSIP_DIALOG_CLIENT_EARLY_STATE == 
                        pDialog->currentState)) {
                    pEvent->resp.retryAfterPeriod = pMsg->retryAfterPeriod;
                }
            }
            /* invite with both SDP and CWI xml multipart body */
            if (pMsg->pMsgBody) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                        "UA_AppEvent: invite with msgbody ContentLength:%d str-leng:%d",
                        (int)pMsg->ContentLength, OSAL_strlen(pMsg->pMsgBody->msg), 0);
                pEvent->msgBody.payLoad.length = OSAL_strlen(pMsg->pMsgBody->msg);
                OSAL_memCpy(pEvent->msgBody.payLoad.data, pMsg->pMsgBody->msg,
                        pEvent->msgBody.payLoad.length);
            }
            break;
        case eUA_MEDIA_INFO:
        case eUA_ANSWERED:
        case eUA_RINGING:
        case eUA_SESSION:
        case eUA_UPDATE:
        case eUA_UPDATE_COMPLETED:
        case eUA_PRACK:
        case eUA_PRACK_COMPLETED:
        case eUA_ACK:
            /* find the session info */
            pEvent->sessNew = pDialog->session.isNew;
            OSAL_memCpy(&pEvent->msgBody.session, &pDialog->session.sess,
                    sizeof(tSession));
        /* SLIP THROUGH HERE */
        case eUA_PRACK_FAILED:
        case eUA_UPDATE_FAILED:
        case eUA_REGISTRATION_COMPLETED:
        case eUA_REGISTRATION_FAILED:
        case eUA_INFO_COMPLETED:
        case eUA_INFO_FAILED:
            if (hArg) {
                pEvent->resp.respCode =
                        (uint32)MSGCODE_GetNum((tSipMsgCodes)hArg);
                if (eUA_REGISTRATION_FAILED == type && pMsg != NULL) {
                    pEvent->resp.retryAfterPeriod = pMsg->retryAfterPeriod;
                }
            }
            break;
        case eUA_OPTIONS:
        case eUA_OPTIONS_COMPLETED:
        case eUA_OPTIONS_FAILED:
            if (pMsg) {
                pEvent->resp.respCode =
                        (uint32)MSGCODE_GetNum(pMsg->code);
            }
            pEvent->resp.hTransaction = (tSipHandle)hArg;
            break;

        case eUA_SUBSCRIBE_FAILED:
        case eUA_SUBSCRIBE_COMPLETED:
        case eUA_SUBSCRIBE_FAILED_NO_SUBS:
            /* then we have to populate the event */
            for (x = 0; x < SIP_MAX_HEADER_FIELDS; x++) {
                if (pEvent->szHeaderFields[x][0] == 0) {
                    len = SIP_EVT_STR_SIZE_BYTES - 1;
                    ENC_Event((tEventHF*)(hArg), &pEvent->szHeaderFields[x][0],
                            &len);
                    pEvent->szHeaderFields[x][len] = 0;
                    /* ensure the last cell in the array is '0' */
                    x++;
                    pEvent->szHeaderFields[x][0] = 0;
                    break;
                }
            }

            /*
             * For RCS 5.0 capabilities support using PRESENCE methods, we need
             * the response code in the event in order to determine how to
             * treat the user whose capabilities we attempted to anonymously
             * fetch.  We'll also use the contents of the To: field to determine
             * where to send the OPTIONS.
             */
            if (pMsg) {
                SIP_DebugLog(SIP_DB_UA_LVL_3,
                        "UA_AppEvent: filled responseCode field: %d : ", (int)pMsg->code, (int)MSGCODE_GetNum(pMsg->code), 0);

                pEvent->resp.respCode =
                        (uint32)MSGCODE_GetNum(pMsg->code);

                /*
                 * Only handle the 503 (Service Unavailable) response to
                 * an initial INVITE request.
                 */
                if (SIP_RSP_SERVICE_UNAVAIL == pEvent->resp.respCode) {
                    pEvent->resp.retryAfterPeriod = pMsg->retryAfterPeriod;
                }
                OSAL_strncpy(pEvent->szRemoteUri, pMsg->From.szUser, sizeof(pEvent->szRemoteUri));
                OSAL_strncpy(pEvent->szToUri, pMsg->To.szUser, sizeof(pEvent->szToUri));
            }
            else {
                SIP_DebugLog(SIP_DB_UA_LVL_3,
                        "UA_AppEvent: pMsg was null", 0, 0, 0);
            }

            break;
        
        case eUA_TEXT_MESSAGE:
        case eUA_NOTIFY_EVENT:
        case eUA_NOTIFY_EVENT_NO_SUBS:
        case eUA_SUBSCRIBE:
        case eUA_INFO:
        case eUA_PUBLISH:
            if (pMsg->pMsgBody) {
                pEvent->msgBody.payLoad.length = pMsg->ContentLength;
                OSAL_memCpy(pEvent->msgBody.payLoad.data, pMsg->pMsgBody->msg,
                        pMsg->ContentLength);
            }
            break;

        case eUA_NOTIFY_EVENT_COMPLETED:
        case eUA_NOTIFY_EVENT_FAILED:
        case eUA_TEXT_MESSAGE_COMPLETED:
        case eUA_TEXT_MESSAGE_FAILED:
        case eUA_PUBLISH_COMPLETED:
        case eUA_PUBLISH_FAILED:
            if (pMsg) {
                pEvent->resp.respCode = (uint32)MSGCODE_GetNum(pMsg->code);
                if (pMsg->pMsgBody) {
                    pEvent->msgBody.payLoad.length = pMsg->ContentLength;
                    OSAL_memCpy(pEvent->msgBody.payLoad.data, pMsg->pMsgBody->msg,
                        pMsg->ContentLength);
                }
            }
            pEvent->resp.hTransaction = (tSipHandle)hArg;
            break;

        case eUA_TRANSFER_ATTEMPT:
        len = SIP_URI_STRING_MAX_SIZE - 1;
            ENC_ReferTo(&pMsg->ReferTo, pEvent->szToUri, &len);
            pEvent->szToUri[len] = 0;
            SIP_DebugLog(SIP_DB_UA_LVL_3, "TRANSFER ATTEMPT: %s",
                    (int)pEvent->szToUri, 0, 0);
            break;
        case eUA_CALL_DROP:
            if (pMsg && pMsg->ContentLength != 0) {
                pEvent->msgBody.payLoad.length = pMsg->ContentLength;
                OSAL_memCpy(pEvent->msgBody.payLoad.data, pMsg->pMsgBody->msg,
                        pMsg->ContentLength);
                pEvent->msgBody.payLoad.data[pMsg->ContentLength] = '\0';
            }
            break;
        case eUA_ERROR:
            pEvent->resp.respCode = (uint32)MSGCODE_GetNum((tSipMsgCodes)hArg);
            break;
        case eUA_REGISTRATION:
        case eUA_TRANSFER_RINGING:
        case eUA_TRANSFER_COMPLETED:
        case eUA_TRANSFER_FAILED:
        default:
            break;
    } /* End of switch */
    return; 
}

void UA_CleanPublish(
    tUaPub *pPub)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_CleanRegistration: Cleaning the Publish for pPub:%X",
            (int)pPub, 0, 0);
    
    pPub->isBusy = FALSE;
    pPub->hasTried = FALSE;
    /* De-associate from transaction. */
    TRANS_deassociate(pPub->hTransaction);
    pPub->hTransaction = 0;
    if (pPub->hTimer) {
        SIPTIMER_Destroy(pPub->hTimer);
        pPub->hTimer = NULL;
    }
    if (pPub->pMsg) {
        SIP_freeMsg(pPub->pMsg);
        pPub->pMsg = NULL;
    }
    return;
}

void UA_CleanRegistration(
    tUaReg *pReg)
{
    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_CleanRegistration: Cleaning the Registration for pReg:%X",
            (int)pReg, 0, 0);
    
    pReg->isBusy = FALSE;
    pReg->reRegInterval = 0;
    if (pReg->hReRegTimer) {
        SIPTIMER_Destroy(pReg->hReRegTimer);
        SIPTIMER_AddWakeUpTime(0); //reset watchdog timer
        pReg->hReRegTimer = NULL;
    }
    pReg->refreshEnable                = OSAL_FALSE;
    pReg->refreshInterval              = 0;
    OSAL_netAddrClear(&pReg->refreshArgs.refreshAddr);
    pReg->refreshArgs.refreshFd        = 0;
    if (pReg->hRefreshTimer) {
        SIPTIMER_Destroy(pReg->hRefreshTimer);
        pReg->hRefreshTimer = NULL;
    }
    if (pReg->pMsg) {
        SIP_freeMsg(pReg->pMsg);
        pReg->pMsg = NULL;
    }
    /* De-associate from transaction. */
    TRANS_deassociate(pReg->hTransaction);
    /* Reset transcation */
    pReg->hTransaction = 0;
    return;
}

void UA_CleanTrans(
    tUaTrans *pTrans)
{
    if (pTrans->pMsg) {
        SIP_freeMsg(pTrans->pMsg);
    }
    /* De-associate from transaction. */
    TRANS_deassociate(pTrans->hTransaction);
    SIP_memPoolFree(eSIP_OBJECT_UA_TRANS, (tDLListEntry *)pTrans);
    return;    
}

void UA_PopulateContact(
    tUa        *pUa, 
    tSipIntMsg *pMsg)
{
    tEPDB_Entry    *pEntry;
    tDLListEntry   *pListEntry;
    
    /* build the contact(s) if any exist.  They should but it's not critical */
    pEntry = EPDB_Get(eEPDB_CONTACTS, pUa->epdb);
    if (pEntry) {
        DLLIST_Copy(&pEntry->x.dll, &pMsg->ContactList, eDLLIST_CONTACT_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
        /* set the contact scheme as per rfc 3261 */
        if (pMsg->requestUri.scheme == eURI_SCHEME_SIPS) {
            pListEntry = NULL;
            while(DLLIST_GetNext(&pMsg->ContactList, &pListEntry))
                ((tContactHFE *)pListEntry)->uri.scheme = eURI_SCHEME_SIPS;  
        }
    }
    return;
}

void UA_PopulateRoute(
    tUa        *pUa, 
    tSipIntMsg *pMsg)
{
    tSYSDB_Entry   *pRouteList;
    
    /* now populate any configured pre-existing routes */
    if (SYSDB_HF_Get(eSIP_ROUTE_HF, &pRouteList) == SIP_OK) {
        if (!DLLIST_IsEmpty(&pRouteList->u.dll)) {
            DLLIST_Copy(&pRouteList->u.dll, &pMsg->RouteList, eDLLIST_ROUTE_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ROUTE_HF);
        }
    }
    return;
}

vint UA_PopulateRegister(
    tUa             *pUa, 
    tSipIntMsg      *pMsg)
{
    tEPDB_Entry *pEntry;
   
    /* 
     * Register with the configured registrar if on exists otherwise
     * try using the domain of the address of record.  
     * This is said in section 10.2.6 of RFC3261
     */
    if ((pEntry = EPDB_Get(eEPDB_REG_PROXY_URI, pUa->epdb)) != NULL) {
        SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_PopulateRegister: setting URI to configured register -hUA:%X", 
            (int)pUa, 0, 0);
        pMsg->requestUri = *(pEntry->x.pUri);
        return (SIP_OK);
    }
    else {
        pMsg->requestUri = pMsg->From.uri;
        /* except we don't want to use the user name */
        pMsg->requestUri.user[0] = 0;
        return (SIP_OK);
    }
}

vint UA_WWWAuthenticate(
    tUa     *pUa,
    char    *pUserName,
    tDLList *pAuthChallenge,
    tDLList *pAuthResponse,
    char    *pAkaAuthResp,
    vint     akaAuthResLen,
    char    *pAkaAuthAuts)
{
    tEPDB_Entry *pEntry;
    tUri        *pUri;
    tDLList     *pCredList;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_WWWAuthenicate: performing www-authentication for hUa:%X", 
            (int)pUa, 0, 0);
    /* get the registrar uri */
    if (NULL != (pEntry = 
                EPDB_Get(eEPDB_REG_PROXY_URI, pUa->epdb))) {
        pUri = pEntry->x.pUri;
        if (NULL != (pEntry = 
                    EPDB_Get(eEPDB_CREDENTIALS, pUa->epdb))) {
            pCredList = &pEntry->x.dll;
        }
        else {
            pCredList = NULL;
        }
        return AUTH_Response(pUserName, pCredList, pUri, pAuthChallenge,
                SIP_REGISTER_METHOD_STR, pAuthResponse,
                pAkaAuthResp, akaAuthResLen, pAkaAuthAuts);
    }

    SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_WWWAuthenicate: FAILED trying to www-authenticate hUa:%X", 
            (int)pUa, 0, 0);
    
    return (SIP_FAILED); 
}

vint UA_Authenicate(
    tUa        *pUa,
    char       *pMethodStr,
    tSipIntMsg *pIn,
    tSipIntMsg *pOut)
{
    tEPDB_Entry *pEntry;
    tDLList     *pCredList;
    vint         status;

    SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_Authenicate: performing www-authentication for hUa:%X", 
            (int)pUa, 0, 0);
    /* get the registrar uri */
    if (NULL != (pEntry = 
                EPDB_Get(eEPDB_CREDENTIALS, pUa->epdb))) {
        pCredList = &pEntry->x.dll;
    }
    else {
        pCredList = NULL;
    }
    status = AUTH_Response(pIn->From.uri.user, pCredList, &pOut->requestUri,
                           &pIn->AuthorizationList, pMethodStr,
                           &pOut->AuthorizationList, NULL, 0, NULL);
    if (status == SIP_OK) {
        if (pIn->code == eSIP_RSP_PROXY_AUTH_REQUIRED) {
            HF_SetPresence(&pOut->x.ECPresenceMasks,
                    eSIP_PROXY_AUTHORIZATION_HF);
        }
        else {
            HF_SetPresence(&pOut->x.ECPresenceMasks, eSIP_AUTHORIZATION_HF);
        }
    }
    return status;
}

vint UA_LoadHeaderFields(
    tSipIntMsg *pMsg, 
    char       *pHdrFlds[],
    vint        numHdrFlds)
{
    vint x;
    vint status;
    /* populate any header fields from the function caller */
    x = 0;
    if (pHdrFlds) {
        while (x < numHdrFlds && pHdrFlds[x] && x < SIP_MAX_HEADER_FIELDS) {
            status = DEC_HeaderFields(pMsg, pHdrFlds[x]);
            if (status != SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                        "UA_LoadHeaderFields: Could not decode %s", 
                        (int)pHdrFlds[x], 0, 0);
                return status;
            }
            x++;
        }
    }
    if (x == SIP_MAX_HEADER_FIELDS) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_LoadHeaderFields: no more room", 0, 0, 0);
        return (SIP_FAILED);
    }
    else {
        return (SIP_OK);
    }
}

char* UA_GetHeaderField(
    char   *pHfStr, 
    char   *pHdrFlds[],
    vint    numHdrFlds)
{
    vint      x;
    tFSM      fsm;
    tL4Packet buff;
    
    /* populate any header fields from the function caller */
    x = 0;
    while (pHdrFlds && pHdrFlds[x] && x < numHdrFlds && 
            x < SIP_MAX_HEADER_FIELDS) {
        /* Set up the tokenizer */
        OSAL_memSet(&fsm, 0, sizeof(tFSM));
        buff.frame = 0;
        buff.isOutOfRoom = 0;
        buff.length = OSAL_strlen(pHdrFlds[x]);
        buff.pStart = buff.pCurr = pHdrFlds[x];
        
        if (TOKEN_Get(&fsm, &buff, ":") == SIP_OK) {
            /* Then we have a header field */
            if (TOKEN_iCmpToken(&fsm.CurrToken, pHfStr) == TRUE) {
                /* Found it.  Return the value */
                if (TOKEN_Get(&fsm, &buff, "") == SIP_OK) {
                    return (fsm.CurrToken.pStart);
                }
            }
        }
        x++;
    }
    return (NULL);
}

vint UA_SetFromField(
    tUa        *pUa,
    char       *pFrom,
    char       *pDisplayName,
    tUriPlus   *pUri)
{
    tContactHFE  *pContact;
    vint          idx;
    
    HF_CleanUriPlus(pUri);
    if (pFrom && pFrom[0] != 0) {
        if (DEC_Uri(pFrom, OSAL_strlen(pFrom), &pUri->uri) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_2, "%s: DEC_Uri failed", (int)__FUNCTION__, 0, 0);
            return (-1);
        }
        /* see if this aor is valid for this UA */
        if ((idx = _UA_search4Uri(pUa, &pUri->uri, 1)) < 0) {
            SIP_DebugLog(SIP_DB_UA_LVL_2, "%s: _UA_search4Uri failed", (int)__FUNCTION__, 0, 0);
            /* aor doesn't belong to this UA */
            return (-1);
        }
    }
    else {
        /* pFrom is NULL or empty so set default to the first aor for the UA */
        if (NULL == (pContact = _UA_getFirstAor(pUa))) {
            return (-1);
        }
        pUri->uri = pContact->uri;
        idx = 0;
    }
    /* Do the display name (callerId) */
    if (pDisplayName) {
        OSAL_strncpy(pUri->szDisplayName, pDisplayName, 
            sizeof(pUri->szDisplayName));
    }
    else {
        /* Then set a default */
        OSAL_strcpy(pUri->szDisplayName, pUri->uri.user);
    }
    return (idx);
}

vint UA_SetMsgBody(
    tSipIntMsg *pMsg,
    char       *pBody,
    vint        length)
{
    /* Make sure the length is within max length */
    length = (length > SIP_MAX_TEXT_MSG_SIZE) ? SIP_MAX_TEXT_MSG_SIZE : length;
    
    if (NULL == (pMsg->pMsgBody =
            (tSipMsgBody *)SIP_memPoolAlloc(eSIP_OBJECT_SIP_MSG_BODY))) {
        return (SIP_NO_MEM);
    }
        
    pMsg->ContentLength = length;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);
    /* 
     * Set the default content type if the caller didn't 
     * specify any in pHFlist 
     */
    if (pMsg->ContentType == eCONTENT_TYPE_NONE) {
        pMsg->ContentType = eCONTENT_TYPE_TEXT;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    }
    OSAL_memCpy(pMsg->pMsgBody->msg, pBody, length);
    return (SIP_OK);
}

vint UA_GetTimerMs(vint timeoutSecs) {
    vint value;

    if (SIP_RESEND_BACKOFF_TS_24_229 == SIP_RESEND_BACKOFF) {
        /* this value is special and used for 3GPP TS 24.229
         * If the negotiated value is less than 1200, use 1/2 the time to reregister
         * If the negotiated value is 1200 or more, reregister 600 sec. before expiration
         */
        value = (timeoutSecs > (SIP_RESEND_BACKOFF*2)) ?
            (timeoutSecs - SIP_RESEND_BACKOFF) : timeoutSecs / 2;
    }
    else if (4 >= SIP_RESEND_BACKOFF) {
        /* Then divide */
        value = timeoutSecs / SIP_RESEND_BACKOFF;
        /* Convert to milliseconds */

    }
    else {
        value = (timeoutSecs > SIP_RESEND_BACKOFF) ?
            (timeoutSecs - SIP_RESEND_BACKOFF) : timeoutSecs;
    }
    return (value * 1000);
}

