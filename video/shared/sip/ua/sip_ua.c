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
#include "sip_parser_dec.h"
#include "sip_parser_enc.h"
#include "sip_session.h"
#include "sip_dialog.h"
#include "sip_xact.h"
#include "sip_xport.h"
#include "sip_tu.h"
#include "sip_app.h"
#include "sip_ua_server.h"
#include "sip_ua_client.h"
#include "sip_ua.h"
#include "_sip_helpers.h"
#include "_sip_fsm.h"
#include "_sip_callback.h"
#include "sip_mem_pool.h"

#if SIP_DEBUG_LOG
#include <stdio.h>
#endif

static char _UA_wmProxy[128];

void UA_setWmProxy(
    char *proxy_ptr)
{
    if (NULL != proxy_ptr && *proxy_ptr) {
        OSAL_strncpy(_UA_wmProxy, proxy_ptr, sizeof(_UA_wmProxy));
    }
    else {
        _UA_wmProxy[0] = '\0';
    }
}

/* 
 *****************************************************************************
 * ================UA_GetEvent()===================
 *
 * This function processes SIP IPC messages and also checks if there are any 
 * events that need to be processed by the application.
 * Applications should call this function in the same task that issues SIP 
 * API's and when applications detect SIP messages that are in a UA queue
 * (The OSAL queue specified when a UA was created).  If the application
 * detects a message in the UA message queue then it should pass a copy of the 
 * message into this function and at the same time provide a pointer to an
 * application event object (tUaAppEvent). In the event that an IPC message
 * turns out to stimulate an application event then this function will 
 * write the application event to the pEvent object and return SIP_OK.
 * If this function returns any thing but a SIP_OK, then there is NO event 
 * for the application to process.
 *
 * pIpcMsg = A pointer to a SIP IPC message that was just read from the UA
 *           queue that was specified when the UA was originally created 
 *           via UA_Create().
 *
 * pEvent = A pointer to a tUaAppEvent object that this function will write 
 *          application events.
 *
 * RETURNS:
 *      SIP_OK: Function successful and there is an application event that needs
 *              processing that was written to pEvent.
 *      SIP_NO_DATA: Function processed SIP IPC message but there are no 
 *                   application events to process.
 *      SIP_BADPARM: One or both of the function's parameters was invalid.
 *
 ******************************************************************************
 */
vint UA_GetEvent(
    void        *pIpcMsg, 
    tUaAppEvent *pEvent)
{
    tSipIpcMsg *pIpc;
    tUaEvent   *pEvt;

    if (pIpcMsg == NULL || pEvent == NULL) {
        /* Bad params return an error */
        return (SIP_BADPARM);
    }

    pIpc= (tSipIpcMsg*)pIpcMsg; 
    /* Here the hContext is a handle to the UA's tUaEvent object */
    pEvt = (tUaEvent*)pIpc->hContext;

    if (pEvt) {
        /* Clear the event type to indicate that the event buffer is empty */
        OSAL_memSet((char*)&pEvt->event, 0, sizeof(tUaAppEvent));
        pEvt->event.header.type = eUA_LAST_EVENT;
    }

    UA_Entry(pIpcMsg);

    if (pEvt) {
        /* Clear the event type to indicate that the event buffer is empty */
        if (pEvt->event.header.type != eUA_LAST_EVENT) {
            /* Then there is an event for the application. Copy it. */
            OSAL_memCpy((char*)pEvent, (char*)&pEvt->event, 
                    sizeof(tUaAppEvent));
            return (SIP_OK);
        }
    }
    return (SIP_NO_DATA);
}

/* 
 *****************************************************************************
 * ================UA_Create()===================
 *
 * This function creates an instance of a user agent.  This function is called
 * at initialization time.  The returned handle is used in future UA module 
 * calls to place, teardown, and modify phone calls.
 *
 * pConfig = A pointer to an object that describes the UA configuration 
 *           information.
 *
 * RETURNS:
 *      NULL: Could not successfully create a User Agent
 *      tSipHandle: If successfully then a handle to the UA is returned.
 *
 ******************************************************************************
 */
tSipHandle UA_Create(
    tUaConfig *pConfig)
{
    tUa        *pUa;
    vint        x;
    tSipDialog *pDialog;

    /* Allocate a UA object */
    pUa = UA_Alloc();
    if (!pUa) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_Create: Could not allocate a UA",
                0, 0, 0);
        return (NULL);
    }

    /* Set user defined configuration data */
    if (UA_Modify(pUa, pConfig->szProxy, pConfig->szOutboundProxy, NULL,
            pConfig->szRegistrarProxy, pConfig->szFqdn, pConfig->aor, 
            pConfig->authCred, pConfig->szCoders, pConfig->packetRate,
            pConfig->capabilitiesBitmap) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_Create: Could not configure UA database", 0, 0, 0);
        UA_Dealloc(pUa);
        return (NULL);
    }

    /* cache the setting for using compact SIP forms */
    pUa->useCompactForms = pConfig->useCompactSipMsgs;

    pDialog = pUa->dialogList.dialogs;
    /* all is good so initialize all the dialog states */
    for (x = 0 ; x < pUa->dialogList.numDialogs ; x++) {
        DIALOG_ChangeState(pDialog, eSIP_DIALOG_IDLE_STATE);
        pDialog++;
    }

    /*
     * Initialize all the tUaReg and tUaPub objects.  Let them know who their
     * parent pUa is.
     */
    for (x = 0; x < SIP_MAX_NUM_AOR ; x++) {
        pUa->Reg[x].pUa = pUa;
        pUa->Pub[x].pUa = pUa;
    }
    
    /* Init internals used for SIP event passing to the calling application */
    pUa->event.event.header.type = eUA_LAST_EVENT;
    pUa->taskId = (uint32)&pUa->event;

    /* 
     * Set the msg Queue that the applicaiton wants to use to receive SIP 
     * events.
     */
    pUa->event.msgQId = pConfig->msgQId;
    
    /* 
     * Now insert this UA to the list of UA's. 
     * Lock the list of UA's before adding.
     */
    if (UA_Insert(pUa) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_Create: Could not add UA to database",
                0, 0, 0);
        UA_Dealloc(pUa);
        return (NULL);
    }
    return ((tSipHandle)pUa);
}

/* 
 *****************************************************************************
 * ================UA_Modify()===================
 *
 * This function will modify configuration data belonging to the UA 
 * specified in hUa. Note, that all pointers to strings MUST BE NULL
 * terminated.
 * 
 * If a pointer to a configuration parameter is NULL, then no attempt is 
 * made to change that particular configuration field.
 * 
 * If a pointer to a configuration parameter is NOT NULL BUT, it is an
 * empty string (i.e. pProxy[0] == 0), then that configuration field
 * will be cleared (made 'empty' or 'no value').
 *
 * RETURNS:
 *      SIP_BADPARM: On of the parameters is invalid.  This is due to 'hUa'
 *                   being invalid or because a string containing a URI 
 *                   had an invalid format.
 *      SIP_NO_MEM:  Memory could not be allocated from the heap to modify 
 *                   the database.
 *      SIP_OK:      The UA's database was successfully modified.
 *
 ******************************************************************************
 */
vint UA_Modify(
    tSipHandle hUa,
    char      *pProxy,
    char      *pOutboundProxy,
    char      *pWimaxProxy,
    char      *pRegProxy,
    char      *pFqdn,
    tAor       aor[],
    tAuthCred  authCred[],
    char      *pCoders,
    vint       packetRate,
    uint32     capabilitiesBitmap)
{
    tUa           *pUa;
    tDLList        dll;
    tUri          *pUri;
    tContactHFE   *pContact;
    tAUTH_Cred    *pCred;
    vint           x;
    tPacketRate    prate;
    tFSM           FSM;
    tL4Packet      Buff;
    char          *pEnd;
    vint           temp;
    char           numStr[SIP_MAX_BASETEN_NUM_STRING + 1];
    char           coders[SYSDB_MAX_NUM_CODERS + 1];
    tEPDB_Entry   *pEntry;

    if (!hUa) {
        return (SIP_BADPARM);
    }
    pUa = (tUa*)hUa;

    /* Initialize dll */
    OSAL_memSet(&dll, 0, sizeof(tDLList));

    /* SET THE FQDN */
    if (pFqdn) {
        if (pFqdn[0] != 0) {
            /* set a default 'contact' header field entry to be the fqdn */
            DLLIST_InitList(&dll);
            if (NULL == (pContact = (tContactHFE *)SIP_memPoolAlloc(eSIP_OBJECT_CONTACT_HF))) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Contact HF pool has no free entries to decode pFqdn:%s",
                    (int)pFqdn, 0, 0);
                return (SIP_NO_MEM);
            }
            DLLIST_InitEntry(&pContact->dll);
            HF_CleanContact(pContact);
            if (DEC_Uri(pFqdn, OSAL_strlen(pFqdn), &pContact->uri) != SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1, 
                    "UA_Modify: Could not decode the fqdn:%s", 
                    (int)pFqdn, 0, 0);
                SIP_memPoolFree(eSIP_OBJECT_CONTACT_HF, (tDLListEntry *)pContact);
                return (SIP_BADPARM);
            }
            if (DLLIST_Enqueue(&dll, &pContact->dll) != SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Could not enqueue pFqdn:%s",
                    (int)pFqdn, 0, 0);
                SIP_memPoolFree(eSIP_OBJECT_CONTACT_HF, (tDLListEntry *)pContact);
                return (SIP_BADPARM);
            }
            EPDB_Set(eEPDB_CONTACTS, (void*)&dll, pUa->epdb);
            SIP_memPoolFree(eSIP_OBJECT_CONTACT_HF, (tDLListEntry *)pContact);
        }
        else {
            /* Then the fqdn should be cleared */
            EPDB_Set(eEPDB_CONTACTS, NULL, pUa->epdb);
        }
    }

    /*
     * allocate a tUri object from the pool.  we'll use it for
     * the next few configuration entries that could be modified,
     * and free it below when it will no longer be needed.
     */
    if (NULL == (pUri = (tUri *)SIP_memPoolAlloc(eSIP_OBJECT_URI))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_Modify: URI pool has no free entries %d",
            (int)__LINE__, 0, 0);
        return (SIP_NO_MEM);
    }

    /* SET THE PROXY */
    if (pProxy) {
        if (pProxy[0] != 0) {
            /* decode the proxy */
            HF_CleanUri(pUri);
            if (DEC_Uri(pProxy, OSAL_strlen(pProxy), pUri) !=
                    SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Could not decode the proxy:%s",
                    (int)pProxy, 0, 0);
                SIP_memPoolFree(eSIP_OBJECT_URI, (tDLListEntry *)pUri);
                return (SIP_BADPARM);
            }
            EPDB_Set(eEPDB_PROXY_URI, (void*)pUri, pUa->epdb);
        }
        else {
            /* nothing so just clear it */
            EPDB_Set(eEPDB_PROXY_URI, NULL, pUa->epdb);
        }
    }
    
    /* SET THE OUTBOUND PROXY */
    if (pOutboundProxy) {
        if (pOutboundProxy[0] != 0) {
            /* decode the proxy */
            HF_CleanUri(pUri);
            if (DEC_UriNoScheme(pOutboundProxy,
                    OSAL_strlen(pOutboundProxy), pUri) != SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Could not decode the outbound proxy:%s",
                    (int)pOutboundProxy, 0, 0);
                SIP_memPoolFree(eSIP_OBJECT_URI, (tDLListEntry *)pUri);
                return (SIP_BADPARM);
            }
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Decode the outbound proxy:%s",
                    (int)pOutboundProxy, 0, 0);
            EPDB_Set(eEPDB_OUTBOUND_PROXY_URI, (void*)pUri, pUa->epdb);
        }
        else {
            /* nothing so just clear it */
            EPDB_Set(eEPDB_OUTBOUND_PROXY_URI, NULL, pUa->epdb);
        }
    }

    /* SET THE WIMAX PROXY */
    if (NULL == pWimaxProxy) {
        pWimaxProxy = _UA_wmProxy;
    }
    if (pWimaxProxy) {
        if (pWimaxProxy[0] != 0) {
            /* decode the proxy */
            HF_CleanUri(pUri);
            if (DEC_UriNoScheme(pWimaxProxy,
                    OSAL_strlen(pWimaxProxy), pUri) != SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Could not decode the wimax proxy:%s",
                    (int)pWimaxProxy, 0, 0);
                SIP_memPoolFree(eSIP_OBJECT_URI, (tDLListEntry *)pUri);
                return (SIP_BADPARM);
            }
            if ((pEntry = EPDB_Get(eEPDB_OUTBOUND_PROXY_URI, pUa->epdb)) != NULL &&
                    pEntry->x.pUri) {
                pUri->transport = pEntry->x.pUri->transport;
            }
            EPDB_Set(eEPDB_WIMAX_PROXY_URI, (void*)pUri, pUa->epdb);
        }
        else {
            /* nothing so just clear it */
            EPDB_Set(eEPDB_WIMAX_PROXY_URI, NULL, pUa->epdb);
        }
    }

    /* SET THE REGISTRAR */
    if (pRegProxy) {
        if (pRegProxy[0] != 0) {
            /* decode the proxy */
            HF_CleanUri(pUri);
            if (DEC_Uri(pRegProxy, OSAL_strlen(pRegProxy), pUri) !=
                    SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1,
                    "UA_Modify: Could not decode the registrar:%s",
                    (int)pRegProxy, 0, 0);
                SIP_memPoolFree(eSIP_OBJECT_URI, (tDLListEntry *)pUri);
                return (SIP_BADPARM);
            }
            EPDB_Set(eEPDB_REG_PROXY_URI, (void*)pUri, pUa->epdb);
        }
        else {
            /* nothing so just clear it */
            EPDB_Set(eEPDB_REG_PROXY_URI, NULL, pUa->epdb);
        }
    }

    /*
     * we're done processing any configuration parameters that require
     * the pUri object, so release it to the pool
     */
    SIP_memPoolFree(eSIP_OBJECT_URI, (tDLListEntry *)pUri);

    /* SET THE AOR's */
    if (aor) {
        DLLIST_InitList(&dll);
        for (x = 0 ; aor->szUri[0] != 0 && x < SIP_MAX_NUM_AOR ; x++, aor++) {
            if (NULL == (pContact = (tContactHFE *)SIP_memPoolAlloc(eSIP_OBJECT_CONTACT_HF))) {
                DLLIST_Empty(&dll, eSIP_OBJECT_CONTACT_HF);
                return (SIP_NO_MEM);
            }
            SIP_DebugLog(SIP_DB_UA_LVL_3, "_SetAor: setting up aor:%s", 
                (int)aor->szUri, 0, 0);
            
            if (DEC_Uri(aor->szUri, OSAL_strlen(aor->szUri), &pContact->uri) !=
                    SIP_OK) {
                SIP_DebugLog(SIP_DB_UA_LVL_1, 
                    "_SetAor: Warning Could not decode aor:%s", 
                    (int)aor->szUri, 0, 0);
                DLLIST_Empty(&dll, eSIP_OBJECT_CONTACT_HF);
                SIP_memPoolFree(eSIP_OBJECT_CONTACT_HF, (tDLListEntry *)pContact);
                return (SIP_BADPARM);
            }

            if (DLLIST_Enqueue(&dll, &pContact->dll) != SIP_OK) {
                DLLIST_Empty(&dll, eSIP_OBJECT_CONTACT_HF);
                SIP_memPoolFree(eSIP_OBJECT_CONTACT_HF, (tDLListEntry *)pContact);
                return (SIP_BADPARM);
            }
        }

        if (x != 0) {
            EPDB_Set(eEPDB_ADDR_OF_REC, (void*)&dll, pUa->epdb);
            DLLIST_Empty(&dll, eSIP_OBJECT_CONTACT_HF);
        }
        else {
            /* Then the user just wants to clear the entry */
            EPDB_Set(eEPDB_ADDR_OF_REC, NULL, pUa->epdb);
        }
    }

    /* SET THE CREDENTIALS */
    if (authCred) {
        DLLIST_InitList(&dll);
        for (x = 0; authCred->szAuthRealm[0] != 0 && x < SIP_MAX_NUM_AUTH_CRED;
                x++, authCred++) {
            if (NULL == (pCred = (tAUTH_Cred *)SIP_memPoolAlloc(eSIP_OBJECT_AUTH_CRED))) {
                DLLIST_Empty(&dll, eSIP_OBJECT_AUTH_CRED);
                return (SIP_NO_MEM);
            }
            OSAL_strcpy(pCred->szUsername, authCred->szAuthUsername);
            OSAL_strcpy(pCred->szRealm,    authCred->szAuthRealm);            
            if (authCred->authType == eUA_AUTH_TYPE_AKA) {
                pCred->types = SIP_AUTH_AKA_KEY;
                /* Copy the 'key', and watch out for buffer overflowing */
                temp = (authCred->u.aka.kLen > SIP_AUTH_KEY_SIZE) ? 
                        SIP_AUTH_KEY_SIZE : authCred->u.aka.kLen;
                if (0 < temp) {
                    OSAL_memCpy(pCred->aka.k, authCred->u.aka.k, temp);
                    pCred->aka.kLen = temp;
                }
                /* Copy the optional 'op', and watch out for buffer overflowing */
                temp = (authCred->u.aka.opLen > SIP_AUTH_OP_SIZE) ? 
                        SIP_AUTH_OP_SIZE : authCred->u.aka.opLen;
                if (0 < temp) {
                    OSAL_memCpy(pCred->aka.op, authCred->u.aka.op, temp);
                    pCred->aka.opLen = temp;
                }
                /* Copy the optional 'amf', and watch out for buffer overflowing */
                temp = (authCred->u.aka.amfLen > SIP_AUTH_AMF_SIZE) ? 
                        SIP_AUTH_AMF_SIZE : authCred->u.aka.amfLen;
                if (0 < temp) {
                    OSAL_memCpy(pCred->aka.amf, authCred->u.aka.amf, temp);
                    pCred->aka.amfLen = temp;
                }
            }
            else {
                /* The default is a plain old NULL terminated password string */
                pCred->types = SIP_AUTH_AKA_PASSWORD;
                OSAL_strcpy(pCred->szPassword, authCred->u.szAuthPassword);
                
            }
            SIP_DebugLog(SIP_DB_UA_LVL_3, "AuthCred: user:%s realm:%s", 
                        (int)authCred->szAuthUsername, 
                        (int)authCred->szAuthRealm, 0);
            DLLIST_Enqueue(&dll, &pCred->dll);
        }
        if (x != 0) {
            EPDB_Set(eEPDB_CREDENTIALS, (void*)&dll, pUa->epdb);
            DLLIST_Empty(&dll, eSIP_OBJECT_AUTH_CRED);
        }
        else {
            EPDB_Set(eEPDB_CREDENTIALS, NULL, pUa->epdb);
        }
    }

    /* SET PACKET RATE */
    if (packetRate != 0) {
        prate.target = (uint16)packetRate;
        prate.high = 60;
        prate.low = 10;
        EPDB_Set(eEPDB_PACKET_RATE, (void*)&prate, pUa->epdb);
    }

    /* SET CODER DATA */
    if (pCoders) {
        if (pCoders[0] != 0) {
            OSAL_memSet(&FSM, 0, sizeof (tFSM));
            Buff.frame = pCoders;
            Buff.length = (uint16)OSAL_strlen(pCoders);
            Buff.pCurr = pCoders;
            Buff.pStart = pCoders;
            Buff.isOutOfRoom = FALSE;
            x = 0;
            while (TOKEN_Get(&FSM, &Buff," ,") == SIP_OK && 
                    x < SYSDB_MAX_NUM_CODERS) {
                if (FSM.CurrToken.length > 0) {
                    TOKEN_copyToBuffer(numStr, sizeof(numStr), &FSM.CurrToken);
                    temp = (vint)OSAL_strtoul(numStr, &pEnd, 10);
                    coders[x+1] = (uint8)temp;
                    x++;
                }
            } /* end of while */
            coders[0] = x;
            EPDB_Set(eEPDB_CODER_TYPES, (void*)&coders, pUa->epdb);
        }
        else {
            EPDB_Set(eEPDB_CODER_TYPES, NULL, pUa->epdb);
        }
    }

    /* SET CAPABILITIES */
    if (0 != capabilitiesBitmap) {
        /*
         * capabilitiesBitmap used in response to SIP OPTIONS request added for
         * Vegas1 project in support of Rich Communications Suite (RCS 5.0)
         */
        SIP_DebugLog(SIP_DB_UA_LVL_1,
            "UA_Modify: Setting capabilitiesBitmap: 0x%x",
            (int)capabilitiesBitmap, 0, 0);

        pUa->capabilitiesBitmap = capabilitiesBitmap;
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_MakeCall()===================
 *
 * This function places a phone via the SIP INVITE method transaction.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * pTo   = A pointer to a NULL terminated string representing the remote 
 *         target uri i.e. "sip:mrandmaa@d2tech.com"
 *           
 * pFrom = A pointer to a NULL terminated string representing the AOR of the 
 *         UA that you wish to make the call on behave of. This should be one 
 *         of the AOR's that was set during UA_Create().  If NULL, 
 *         (or if the string is empty), the stack will use the first AOR in 
 *         the list of AOR's set when UA_Create() was called.
 *
 * pDisplayName = A pointer to a NULL terminated string representing the 
 *                the "caller-ID" of the caller.  If NULL, the stack will use
 *                the username from the AOR.  If the string is empty
 *                (i.e. pDisplayName[0] == 0) then no display name will be 
 *                used.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string (i.e. "Server: vport")
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMediaSess = A pointer to an object that contains information regarding 
 *              the media connection.  i.e. RTP IP addr\port pairs, codec 
 *              information, the duplicity of the connection.  
 *              The RTP IP addr\port pairs must be populated, but if no
 *              information regarding the codecs, packet rate or duplicity
 *              are provided then default values set during UA_Create() will 
 *              be used.  This object is defined in Ua.h.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * keep = The parameter that explicitly indicate willingness to send keep-alives
 *        towards its adjacent downstream SIP entity.
 *
 * RETURNS:
 *      NULL: Could not successfully create an instance of a dialog
 *      tSipHandle: If successful then a handle to the Dialog is returned.
 *
 ******************************************************************************
 */
tSipHandle UA_MakeCall(
    tSipHandle    hUa, 
    char         *pTo,
    char         *pFrom,
    char         *pDisplayName,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn,
    OSAL_Boolean  keep)
{
    /* build an invite per RFC3261 section 12.1.2 */
    tSipIntMsg     *pMsg;
    tSipDialog     *pDialog;
    tEPDB_Entry    *pEntry;
    tUa            *pUa;
    tDLList        *pCredList;
    char           *pToReplaces;
    int             len;
    tContactHFE    *pContact;
    tDLListEntry   *pDLLEntry;

    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_MakeCall: -hUa:%X, to %s (%s)", 
            (int)hUa, (int)pTo, (int)pDisplayName);

    /* pre-initialize for common error exit support */
    pDialog = NULL;
    pMsg = NULL;

    if (!hUa || !pTo) {
        return (NULL);
    }

    pUa = (tUa*)hUa;

    /* create an invite and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (NULL);
    }

    pMsg->method = eSIP_INVITE;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        goto makeCallErrorExit;
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);

    HF_CleanUriPlus(&pMsg->To);
    /*
     * Handle the 'To' header field check and see if
     * there is a '?' this means it's a 'replace'
     */
    pToReplaces = OSAL_strchr(pTo, '?');
    if (pToReplaces) {
        len = (int)(pToReplaces - pTo);
        /* then we also have to populate the 'replaces' header field */
        pToReplaces++;
        if (DEC_ReferTo2Replaces(pToReplaces, OSAL_strlen(pToReplaces), 
                &pMsg->Replaces) == SIP_OK) {
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_REPLACES_HF);
        }
    }
    else {
        len = OSAL_strlen(pTo);
    }

    if (DEC_Uri(pTo, (uint16)len, &pMsg->To.uri) != SIP_OK) {
        goto makeCallErrorExit;
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);

    /* handle the from */
    if (UA_SetFromField(pUa, pFrom, pDisplayName, &pMsg->From) == -1) {
        goto makeCallErrorExit;
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

    /* build the request uri */
    pMsg->requestUri = pMsg->To.uri;
    
    /* Set up the contact */
    UA_PopulateContact(pUa, pMsg);

    /* Add the capabilitiesBitmap */
    if (0 != capabilitiesBitmap) {
        pDLLEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
            pContact = (tContactHFE*)pDLLEntry;
            pContact->capabilitiesBitmap = capabilitiesBitmap;
        }
    }

    /* Check if the media is for a IM session and it's NOT a file transfer. */
    if (pMediaSess && (eSdpMediaMsMessage == pMediaSess->media[0].mediaType) &&
        (0 == pMediaSess->media[0].fileSelectorType[0])) {
        /* Then this is strictly for a IM session */
        pDLLEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
            pContact = (tContactHFE*)pDLLEntry;
            pContact->isImSession = 1;
        }
    }

    /* Set up the route header fields */
    UA_PopulateRoute(pUa, pMsg);

    pDialog = UA_DialogInit(pUa);
    if (!pDialog) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_MakeCall failed, no available dialogs:", 0, 0, 0);
        goto makeCallErrorExit;
    }
    else {
        /* set if this dialog need include keep-alives */
        pDialog->natKeepaliveEnable = keep;
        /* get the credentials.  If none exist just keep going */
        pCredList = NULL;
        if ((pEntry = EPDB_Get(eEPDB_CREDENTIALS, pUa->epdb)) != NULL) {
            pCredList = &pEntry->x.dll;
        }
        DIALOG_InitClient(pUa, (tSipHandle)pUa->taskId, pDialog, pMsg, 
            pCredList);
    }

    UA_SessionInit(pUa, pDialog, pMsg->From.uri.user);
    if (pMediaSess) {
        SIP_DebugLog(SIP_DB_UA_LVL_3,
            "UA_MakeCall Creating Session Offer:", 0, 0, 0);
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_MakeCall FAILED! bad session data hUa:%X hDialog:%X",
                        (int)pUa, (int)pDialog, 0);
            goto makeCallErrorExit;
        }
        /* set the content type */
        pMsg->ContentType = eCONTENT_TYPE_SDP;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        /* HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF); */
        if (0 != pMediaSess->otherPayload[0]) {
            len = OSAL_strlen(pMediaSess->otherPayload);
            if (NULL != (pMsg->pMsgBody =
                    (tSipMsgBody *)SIP_memPoolAlloc(
                    eSIP_OBJECT_SIP_MSG_BODY))) {
                OSAL_snprintf(pMsg->pMsgBody->msg, len + 1, "%s",
                        pMediaSess->otherPayload);
                /*
                 * Also copy to the dialog just in case we get
                 * challenged to authenticate.
                 */
                OSAL_snprintf(pDialog->session.sess.otherPayload, len + 1, "%s",
                        pMediaSess->otherPayload);
                pMsg->ContentType = eCONTENT_TYPE_MULTIPART;
            }
        }
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* run it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_MakeCall: Failed in FSM -hUa:%X",
            (int)hUa, 0, 0);
        goto makeCallErrorExit;
    }

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_MakeCall created dialog: %X",
        (int)pDialog, 0, 0);

    return (tSipHandle)pDialog;

makeCallErrorExit:
    SIP_freeMsg(pMsg);

    if (pDialog) {
        DIALOG_Destroy(pDialog);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_MakeCall failed :", 0, 0, 0);
    return (NULL);
}

/* 
 *****************************************************************************
 * ================UA_Subscribe()===================
 *
 * This function is used to send a SUBSCRIBE request to subscribe to an event.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog  = A POINTER TO A HANDLE containing the value of a dialog handle.
 *            This value MUST NOT BE NULL. If the pointer is dereferenced 
 *            and the value is NULL then a new dialog will be created on 
 *            behalf of the SUBSCRIBE request and the POINTER will be populated
 *            with the value of the new dialog handle when the function 
 *            successfully returns. If the dereferenced value is non-NULL 
 *            then the SUBSCRIBE request will be made within a pre-existing 
 *            dialog.
 *
 * pTo   = A pointer to a NULL terminated string representing the remote 
 *         target uri i.e. "sip:mrandmaa@d2tech.com"
 *           
 * pFrom = A pointer to a NULL terminated string representing the AOR of the 
 *         UA that you wish to make the call on behave of. This should be one 
 *         of the AOR's that was set during UA_Create().  If NULL, 
 *         (or if the string is empty), the stack will use the first AOR in 
 *         the list of AOR's set when UA_Create() was called.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Event: message-summary"
 *            pHdrFlds[1] = "Expires: 180"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMsgBody = A pointer to data to be inserted into the SIP message body of
 *            the SUBSCRIBE request.
 *
 * msgBodyLen = The length of the string specified in pMsgBody.
 * 
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      The subscribe request was successfully sent.
 *      SIP_DONE:    The subscribe request was successfull and the dialog was
 *                   destroyed.  This return code happens when an event was
 *                   successfully UN-subscribed and the dialog destroyed.
 *      SIP_BADPARM: Function failed because one of following occurred
 *                   1) The hUa, or hDialog, or pTo parameter was NULL.
 *                   2) One of the header field values in the pHdrFlds array 
                        could not be understood.
                     3) Could not decode the pTo or pFrom URI strings
 *      SIP_NO_MEM:  Could not allocate memory necessary to send the 
 *                   SUBSCRIBE request.
 *      SIP_FAILED:  Could not send the request because of a UA configuration
 *                   issue, or because the dialog was in an invalid state. 
 *
 ******************************************************************************
 */
vint UA_Subscribe(
    tSipHandle     hUa,
    tSipHandle    *hDialog,
    char          *pTo,
    char          *pFrom,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tUaMsgBody    *pMsgBody,
    uint32         capabilitiesBitmap,
    tLocalIpConn  *pLocalConn)
{
    /* build a subscribe per RFC3265 */
    tSipIntMsg     *pMsg;
    tSipDialog     *pDialog;
    tEPDB_Entry    *pEntry;
    tUa            *pUa;
    tDLList        *pCredList;
    vint            status;
    char           *pDisplayName = NULL;
    tContactHFE    *pContact;
    tDLListEntry   *pDLLEntry;
    
    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Subscribe: -hUa:%X, to %s from %s",
            (int)hUa, (int)pTo, (int)pFrom);

    if (NULL == hUa || NULL == pTo || NULL == hDialog) {
        return (SIP_BADPARM);
    }

    pUa = (tUa*)hUa;

    /* create a SUBSCRIBE and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_2, "allocated a message", 0, 0, 0);

    pMsg->method = eSIP_SUBSCRIBE;
    pMsg->msgType = eSIP_REQUEST;

    /* Populate any header fields from the function caller */
    if (SIP_OK != UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds)) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_2, "loaded header fields", 0, 0, 0);

    /* handle the from */

    if (-1 == UA_SetFromField(pUa, pFrom, pDisplayName, &pMsg->From)) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_2, "set From Field", 0, 0, 0);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);
    
    HF_CleanUriPlus(&pMsg->To);
    if (SIP_OK != DEC_Uri(pTo, OSAL_strlen(pTo), &pMsg->To.uri)) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    SIP_DebugLog(SIP_DB_UA_LVL_2, "set To Field", 0, 0, 0);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);
    
    /* build the request uri */
    pMsg->requestUri = pMsg->To.uri;
    
    /* Set the 'contact' header fields */
    UA_PopulateContact(pUa, pMsg);

    /* Add the capabilitiesBitmap into Contact */
    if (0 != capabilitiesBitmap) {
        pDLLEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
            pContact = (tContactHFE*)pDLLEntry;
            pContact->capabilitiesBitmap = capabilitiesBitmap;
        }
    }

    /* Set the 'route' header fields */
    UA_PopulateRoute(pUa, pMsg);
    
    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    
    /* Include and msg body */
    if (pMsgBody && pMsgBody->pBody) {
        if (SIP_OK != (UA_SetMsgBody(pMsg, pMsgBody->pBody,
                pMsgBody->length))) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
    }
        
    if (NULL == (*hDialog)) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_Subscribe Creating Dialog Client:",
                0, 0, 0);
        pDialog = UA_DialogInit(pUa);
        if (NULL == pDialog) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                    "UA_Subscribe failed, no available dialogs:", 0, 0, 0);
            SIP_freeMsg(pMsg);
            return (SIP_FAILED);
        }
        else {
            /* get the credentials.  If none exist just keep going */
            pCredList = NULL;
            if (NULL != (pEntry = EPDB_Get(eEPDB_CREDENTIALS, pUa->epdb))) {
                pCredList = &pEntry->x.dll;
            }
            DIALOG_InitClient(pUa, (tSipHandle)pUa->taskId, pDialog, 
                pMsg, pCredList);
        }
    }
    else {
        pDialog = (tSipDialog*)(*hDialog);
    }
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    status = UASM_DialogClient(pDialog, pMsg, NULL);
    if (SIP_DONE == status) {
        /* 
         * This happens when "unsubscribing" from an event. It indicates that 
         * the subscription request was successful and there are no more active
         * event subscriptions. The dialog needs to be destroyed.
         */
        DIALOG_Destroy(pDialog);
    }
    else if (SIP_OK != status) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_Subscribe failed : %d", status, 0, 0);
        if (NULL == (*hDialog)) {
            /* Then this error happened for a new dialog, not an existing one */
            DIALOG_Destroy(pDialog);
        }
        SIP_freeMsg(pMsg);
    }
    else {
        /* All is okay, tell the application what the new dialog handle is */
        if (NULL == (*hDialog)) {
            *hDialog = pDialog;
        }
    }
    return (status);
}


/* 
 *****************************************************************************
 * ================UA_HungUp()===================
 *
 * This function is used when the User Agents wishes to destroy an active 
 * dialog.  This is typically called when the application detects that the
 * user has hung up the phone.  Please note that if the caller has 
 * more than one active dialog (like a three way call), This function
 * must be called for each of those active dialogs.
 * 
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create(). 
 *
 * hDialog = A handle to the dialog that was returned when the call was 
 *           created via UA_MakeCall()
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: 99 red ballons"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:       Dialog was successfully destroyed
 *      SIP_FAILED:   There was an error destroying the dialog or the 
 *                    applicable SIP messages could not be sent.  This is 
 *                    typically because the dialog was in the wrong state.
 *      SIP_BADPARM:  Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *      SIP_NO_MEM:   Could not allocate heap memory needed for the transaction
 *
 ******************************************************************************
 */
vint UA_HungUp(
    tSipHandle     hUa, 
    tSipHandle     hDialog,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tLocalIpConn  *pLocalConn)
{
    tSipDialog *pDialog;
    tSipIntMsg *pMsg;
    tUa *pUa;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_HungUp: hUa%X, hDialog:%X ", 
            (int)hUa, (int)hDialog, 0);
    
    if (!hDialog || !hUa) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_HungUp: Failed hUa:%X, hDialog:%X ", 
                (int)hUa, (int)hDialog, 0);
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;

    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    /* 
     * Assume that the request is a BYE first, the dialog 
     * FSM will handle the rest 
     */
    pMsg->msgType = eSIP_REQUEST;
    pMsg->method = eSIP_BYE;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* send it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_freeMsg(pMsg);
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_HungUp: CANCEL-BYE request failed in FSM", 
            0, 0, 0);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/**
 * This routine will wake up and reset timers used by SIP to manage
 * Registrations and presence publications.  This routine is typically
 * called by the application after a device's hardware had been in a
 * suspend mode or sleep mode.  It is used to awaken and reset timers
 * for the speicified UA that pertain to to the UA's re-registration
 * state-machines, registration subscriptions and presence publications.
 */


/*
 *****************************************************************************
 * ================UA_WakeUp()===================
 *
* This routine will wake up and reset timers used by SIP to manage
 * Registrations and presence publications.  This routine is typically
 * called by the application after a device's hardware had been in a
 * suspend mode or sleep mode.  It is used to awaken and reset timers
 * for the speicified UA that pertain to to the UA's re-registration
 * state-machines, registration subscriptions and presence publications.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created
 *        via UA_Create().
 *
 * RETURNS:
 *      SIP_OK:       Command was successfully issued.
 *      SIP_BADPARM:  Function failed because the hUa parameter was invalid.
 *
 ******************************************************************************
 */
vint UA_WakeUp(tSipHandle hUa)
{
    tUa *pUa;
    vint x;
    tSipDialog *pDialog;

    SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_WakeUp: hUa%X", (int)hUa, 0, 0);

    if (!hUa) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_WakeUp: Failed hUa:%X", (int)hUa, 0, 0);
        return (SIP_BADPARM);
    }

    pUa = (tUa*)hUa;

    for (x = 0; x < SIP_MAX_NUM_AOR; x++) {
        if (0 != pUa->Reg[x].hReRegTimer) {
            SIPTIMER_WakeUp(pUa->Reg[x].hReRegTimer);
        }
        if (0 != pUa->Reg[x].hRefreshTimer) {
            SIPTIMER_WakeUp(pUa->Reg[x].hRefreshTimer);
        }
        if (0 != pUa->Pub[x].hTimer) {
            SIPTIMER_WakeUp(pUa->Pub[x].hTimer);
        }
    }

    pDialog = pUa->dialogList.dialogs;
    for(x = 0; x < pUa->dialogList.numDialogs; x++) {
        DIALOG_WakeUpSubscriptions(pDialog);
        pDialog++;
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Answer()===================
 *
 * This function is used when the User Agents wishes to answer an incoming 
 * phone call.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create(). 
 *
 * hDialog = A handle to the dialog that was originated from a eUA_CALL_ATTEMPT
 *           event.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: WiFi phone"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMediaSess = A pointer to an object that contains information regarding the 
 *              media connection.  i.e. RTP IP addr\port pairs, codec 
 *              information, the duplexity of the connection.  
 *              The RTP IP addr\port pairs must be populated, but if no
 *              information regarding the codecs, packet rate or duplexity
 *              are provided then default values set during UA_Create() are 
 *              used.  This object is defined in Ua.h.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this response.
 *              If NULL then the last IP Stack interface used to send any 
 *              previous request or response will be used.
 *
 *
 * RETURNS:
 *      SIP_OK:      Call was successfully answered and the dialog is in a 
 *                   state of "confirmed".
 *      SIP_FAILED:  There was an error trying to answer the call. The dialog 
 *                   state machine may have been in an invalid state.
 *      SIP_BADPARM:  Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *      SIP_NO_MEM: Could not allocate memory needed to answer the call
 *
 ******************************************************************************
 */
vint UA_Answer(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg   *pMsg;
    tSipDialog   *pDialog;
    tUa          *pUa;
    tEPDB_Entry  *pEntry;
    vint          status;
    tContactHFE  *pContact;
    tDLListEntry *pDLLEntry;
      
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_Answer: hUa:%X, hDialog:%X", 
            (int)hUa, (int)hDialog, 0);

    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;
    status = SIP_OK;

    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    /* construct a 200 ok message */
    pMsg->msgType = eSIP_RESPONSE;
    pMsg->code = eSIP_RSP_OK;
    
    /* set the method in the CSeq */
    pMsg->CSeq.method = eSIP_INVITE;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ACCEPT_HF);

    /* build the contact for 200 responses as per rfc, if no still send it */ 
    pEntry = EPDB_Get(eEPDB_CONTACTS, pUa->epdb);
    if (pEntry) {
        DLLIST_Copy(&pEntry->x.dll, &pMsg->ContactList, eDLLIST_CONTACT_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    }
    /* Add the capabilitiesBitmap */
    if (0 != capabilitiesBitmap) {
        pDLLEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
            pContact = (tContactHFE*)pDLLEntry;
            pContact->capabilitiesBitmap = capabilitiesBitmap;
        }
    } 
    
    /* Check for session data, See RFC 3261 section 13.1 */
    if (pMediaSess) {
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_2, 
                   "UA_Answer Warning! did not encode SDP hUa:%X hDialog:%X",
                   (int)pUa, (int)pDialog, 0);
            /* 
             * If the 'if' statements above fails then send an error response.
             * We don't return an error here to the app because the dialog 
             * 'fsm' will send the app an error event once it get's the pMsg.
             */

            /* send error */
            MSGCODE_Create(pMsg, NULL, eSIP_RSP_NOT_ACCEPT);
        }
        else {
            /* set the content type */
            pMsg->ContentType = eCONTENT_TYPE_SDP;
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
        }
    } /* end of pMediaSess */

    /* 
     * Setup the pLocalConn info in the transport layer if it exists 
     * This will take care of correctly populating the 'contact' info in the 2xx
     */
    if (pLocalConn) {
        TRANSPORT_SetLocalConn(TRANS_GetTransport(pDialog->hTrans), 
            pLocalConn);
        /* 
         * Save the local connection settings if they exist.
         * This is for sending when ACK receipt timeout.
         */
        pUa->lclConn = *pLocalConn;
    }
        
    /* pass it through the state machine */
    if (UASM_DialogServer(pDialog, pMsg, pDialog->hTrans) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_Answer: Couldn't send 200", 0, 0, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (status);
}

/* 
 *****************************************************************************
 * ================UA_UpdateCall()===================
 *
 * This function is used to change media connection information and perform 
 * target refreshes (similar to performing a Re-INVITE) on dialogs that 
 * are in an 'early' state. In other words this is how you send a SIP UPDATE 
 * request.  Please refer to RFC3311 for more details regarding UPDATE 
 * handling.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog = A handle to the dialog retrieved from a eUA_CALL_ATTEMPT
 *           event or returned from a UA_MakeCall() function call.
 *
 * pTargetRefresh = A pointer to a string representing the "target refresh" 
 *               uri. If NULL, then no target refresh value is populated in 
 *               the 'Contact' header field, just like it says to do in 
 *               RFC3261.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMediaSess = A pointer to an object that contains information regarding 
 *              the media connection.  i.e. RTP IP addr\port pairs, codec 
 *              information, the duplicity of the connection and the 
 *              packetRate. 
 *              If this value is NULL then a copy of the original SDP
 *              (or session) data is re-sent.  Otherwise it will populate 
 *              the SDP payload with the new media connection information.
 *              See sip_ua.h for the definition of the tSession object.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_BADPARM: Function failed due to one of the following:
 *                    1) Bad Ua handle or dialog handle. 
 *                    2) The pTargetRefresh string could not be decoded.
 *                    3) One of the header fields in the pHdrFldsarray could 
 *                       not be understood.
 *                    4) The pMediaSess data could not be encoded.
 *      SIP_NO_MEM:  Could not allocate any memory for the request.
 *      SIP_FAILED:  Could not send the Re-Invite because the dialog was in 
 *                   the wrong state. 
 *      SIP_OK:      Request was successfully sent.
 *
 ******************************************************************************
 */
vint UA_UpdateCall(
    tSipHandle     hUa, 
    tSipHandle     hDialog, 
    char          *pTargetRefresh,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tSession      *pMediaSess,
    tLocalIpConn  *pLocalConn)
{
    tSipDialog  *pDialog;
    tSipIntMsg  *pMsg;
    tUri         target;
    tContactHFE *pContact;
    tUa         *pUa;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_UpdateCall: -hUa:%X hDialog:%X", 
            (int)hUa, (int)hDialog, 0);
        
    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;

    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->msgType = eSIP_REQUEST;
    pMsg->method = eSIP_UPDATE;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    
    /* if pTargetRefresh is not null try to decode it */
    if (pTargetRefresh) {
        HF_CleanUri(&target);
        if (DEC_Uri(pTargetRefresh, (uint16)OSAL_strlen(pTargetRefresh),
                &target) != SIP_OK) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* then place it in the contact header field */
        if (NULL == (pContact = (tContactHFE *)SIP_memPoolAlloc(eSIP_OBJECT_CONTACT_HF))) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        DLLIST_InitEntry(&pContact->dll);
        pContact->uri = target;
        DLLIST_Enqueue(&pMsg->ContactList, &pContact->dll);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    }

    /* rtp connection info */
    pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
    if (!pMsg->pSessDescr) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, 
            "UA_UpdateCall: FAILED could not encode SDP data", 0, 0, 0);
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    /* set the content type */
    pMsg->ContentType = eCONTENT_TYPE_SDP;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* Save a copy of the UPDATE message */
    if (pDialog->pReInviteUpdate) {
        SIP_freeMsg(pDialog->pReInviteUpdate);
    }
    if (NULL == (pDialog->pReInviteUpdate = SIP_copyMsg(pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_MakeCall: Failed to cooy pMsg", 0, 0, 0);
    }

    /* send it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_UpdateCall: Failed in FSM -hUa:%X hDialog:%X",
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_UpdateResp()===================
 * 
 * This function is used to respond to an UPDATE request.  When a SIP 
 * application receives a eUA_UPDATE event, it can use this function to 
 * send back a response to the UPDATE request.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create(). 
 *
 * hDialog = A handle to the dialog that the developer is sending the 
 *          UPDATE response within.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: WiFi phone"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMediaSess = A pointer to an object that contains information regarding the 
 *              media connection.  i.e. RTP IP addr\port pairs, codec 
 *              information, the duplicity of the connection. If NULL, then 
 *              the stack will include in the response the last session data 
 *              sent for this UA.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this response.
 *              If NULL then the last IP Stack interface used to send any 
 *              previous request or response will be used.
 *
 * RETURNS:
 *      SIP_OK:      UPDATE response was successfully sent.
 *      SIP_FAILED:  There was an error trying to send the response. The dialog 
 *                   state machine may have been in an illegal state to send
 *                   as UPDATE response.
 *      SIP_BADPARM: Function failed due to one of the following reasons:
 *                    1) One of the header fields in the pHdrFlds array could 
 *                       not be understood.
 *                    2) The hUA or hDialog parameter was invalid
 *                    3) The pMediaSess could not be encoded in the response
 *                    4) The pReasonPhrase string was too long.
 *      SIP_NO_MEM: Could not allocate memory needed to answer the call
 *
 ******************************************************************************
 */
vint UA_UpdateResp(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg  *pMsg;
    tSipDialog  *pDialog;
    tUa         *pUa;
    tEPDB_Entry *pEntry;
    tSipMsgCodes intCode;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_UpdateResp: hUa:%X, hDialog:%X", 
            (int)hUa, (int)hDialog, 0);

    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;
    
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    /* construct a 200 ok message */
    pMsg->msgType = eSIP_RESPONSE;
    /* set the method in the CSeq */
    pMsg->CSeq.method = eSIP_UPDATE;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    
    intCode = MSGCODE_GetInt(responseCode);
    if (intCode == eSIP_RSP_LAST_RESPONSE_CODE) {
        SIP_freeMsg(pMsg);
        return (SIP_NOT_SUPPORTED);
    }
    MSGCODE_Create(pMsg, NULL, intCode);

    if (pReasonPhrase) {
        if (OSAL_strlen(pReasonPhrase) >= SIP_MAX_REASON_PHRASE_STR_LEN) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        SIP_copyStringToSipText(pReasonPhrase, &pMsg->pReasonPhrase);
    }

    /* build the contact per rfc, if none, still send it */ 
    pEntry = EPDB_Get(eEPDB_CONTACTS, pUa->epdb);
    if (pEntry) {
        DLLIST_Copy(&pEntry->x.dll, &pMsg->ContactList, eDLLIST_CONTACT_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    }
    
    if (pMediaSess) {
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_UpdateResp: Could not encode SDP -hUa:%X hDialog:%X",
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
    }
    /* set the content type */
    pMsg->ContentType = eCONTENT_TYPE_SDP;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);

    /* 
     * Setup the pLocalConn info in the transport layer if it exists 
     * This will take care of correctly populating the 'contact' info in the 2xx
     */
    if (pLocalConn) {
        TRANSPORT_SetLocalConn(TRANS_GetTransport(pDialog->hTrans),
            pLocalConn);
    }
        
    /* pass it through the state machine */
    if (UASM_DialogServer(pDialog, pMsg, pDialog->hUpdateTrans) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_UpdateResp: Couldn't send response", 0, 0, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_PrackResp()===================
 * 
 * This function is used to respond to an PRACK request.  When a SIP 
 * application receives a eUA_PRACK event, it can use this function to 
 * send back a response to the PRACK request.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create(). 
 *
 * hDialog = A handle to the dialog that the developer is sending the 
 *          UPDATE response within.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: WiFi phone"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMediaSess = A pointer to an object that contains information regarding the 
 *              media connection.  i.e. RTP IP addr\port pairs, codec 
 *              information, the duplicity of the connection. If NULL, then 
 *              the stack will include in the response the last session data 
 *              sent for this UA.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this response.
 *              If NULL then the last IP Stack interface used to send any 
 *              previous request or response will be used.
 *
 * RETURNS:
 *      SIP_OK:      UPDATE response was successfully sent.
 *      SIP_FAILED:  There was an error trying to send the response. The dialog 
 *                   state machine may have been in an illegal state to send
 *                   as UPDATE response.
 *      SIP_BADPARM: Function failed due to one of the following reasons:
 *                    1) One of the header fields in the pHdrFlds array could 
 *                       not be understood.
 *                    2) The hUA or hDialog parameter was invalid
 *                    3) The pMediaSess could not be encoded in the response
 *                    4) The pReasonPhrase string was too long.
 *      SIP_NO_MEM: Could not allocate memory needed to answer the call
 *
 ******************************************************************************
 */
vint UA_PrackResp(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg  *pMsg;
    tSipDialog  *pDialog;
    tUa         *pUa;
    tEPDB_Entry *pEntry;
    tSipMsgCodes intCode;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_PrackResp: hUa:%X, hDialog:%X", 
            (int)hUa, (int)hDialog, 0);

    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;
    
    if (NULL == (pMsg = pDialog->prack.pMsg)) {
        SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_PrackResp: No pMsg in prack!", 
                (int)hUa, (int)hDialog, 0);
        return (SIP_NO_MEM);
    }

    /* construct a 200 ok message */
    pMsg->msgType = eSIP_RESPONSE;
    /* set the method in the CSeq */
    pMsg->CSeq.method = eSIP_PRACK;

    /* Ditch the contact and the message body */
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    pMsg->ContentType = eCONTENT_TYPE_NONE;
    pMsg->sipfragCode = (tSipMsgCodes)0;
    pMsg->ContentLength = 0;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    
    intCode = MSGCODE_GetInt(responseCode);
    if (intCode == eSIP_RSP_LAST_RESPONSE_CODE) {
        SIP_freeMsg(pMsg);
        return (SIP_NOT_SUPPORTED);
    }
    MSGCODE_Create(pMsg, NULL, intCode);

    if (pReasonPhrase) {
        if (OSAL_strlen(pReasonPhrase) >= SIP_MAX_REASON_PHRASE_STR_LEN) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        SIP_copyStringToSipText(pReasonPhrase, &pMsg->pReasonPhrase);
    }

    /* build the contact per rfc, if none, still send it */ 
    pEntry = EPDB_Get(eEPDB_CONTACTS, pUa->epdb);
    if (pEntry) {
        DLLIST_Copy(&pEntry->x.dll, &pMsg->ContactList, eDLLIST_CONTACT_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    }

    if (pMediaSess) {
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_PrackResp: Could not encode SDP -hUa:%X hDialog:%X",
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* set the content type */
        pMsg->ContentType = eCONTENT_TYPE_SDP;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
    }

    /* 
     * Setup the pLocalConn info in the transport layer if it exists 
     * This will take care of correctly populating the 'contact' info in the 2xx
     */
    if (pLocalConn) {
        TRANSPORT_SetLocalConn(TRANS_GetTransport(pDialog->hTrans),
            pLocalConn);
    }
        
    /* pass it through the state machine */
    if (UASM_DialogServer(pDialog, pMsg, pDialog->hPrackTrans) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_PrackResp: Couldn't send response", 0, 0, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_TransferCall()===================
 *
 * This function places a phone via the SIP REFER method transaction.
 * If successful, then the eUA_TRANSFER_COMPLETED SIP event message will 
 * be returned.  If it can't then a eUA_TRANSFER_FAILED will be returned.
 * Note, that subsequent NOTIFY messages are handled internally for your
 * convenience
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog   = A handle to the dialog (call leg) that you wish to transfer
 *           
 * pTransferTarget = A pointer to a string containing the URI of where you want 
 *                   the call to go (i.e. sip@sparrish@awesome.sip.com).
 *
 * hDialogReplaces = A handle to another dialog that the call is being 
 *                   transferred to.  NOTE, this parameter is used for 
 *                   "Call Transfers with Call Consultations".  If performing
 *                   a "blind: call transfer this parameter should be NULL.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Expires: 60"
 *            pHdrFlds[1] = "User-Agent: WiFi phone"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * pmethod = A pointer to method string which is appended to Refer-To.
 *
 * pMsgBody = A pointer to extra xml message body to the refer request
 *
 * RETURNS:
 *      SIP_OK:       REFER method and transfer request were successful sent.
 *      SIP_BADPARM:  Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) The pTransferTarget URI string could not be understood
 *                    4) The 'ReferTo' could not be included in the SIP REFER 
 *                       request.
 *      SIP_NO_MEM:   Could not allocate necessary memory.
 *      SIP_FAILED:   Could not transfer the call.  The dialog was in an invalid 
 *                    state for call transfer.
 *
 ******************************************************************************
 */
vint UA_TransferCall(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pTransferTarget,
    tSipHandle    hDialogReplaces,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn,
    tSipMethod    method,
    tUaMsgBody   *pMsgBody)
{
    /* build a REFER per RFC3515 section 12.1.2 */
    tSipIntMsg *pMsg;
    tUa        *pUa;
    tSipDialog *pDialog;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_TransferCall: hUa:%X, hDialog:%X to %s)",
            (int)hUa, (int)hDialog, (int)pTransferTarget);

    if (!hUa || !hDialog) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;

    /* create an invite and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_REFER;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUPPORTED_HF);


    HF_CleanUriPlus(&pMsg->ReferTo.uriPlus);
    if (!hDialogReplaces) {
        if (pTransferTarget && (DEC_Uri(pTransferTarget,
                OSAL_strlen(pTransferTarget),
                &pMsg->ReferTo.uriPlus.uri) != SIP_OK)) {
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_TransferCall: FAILED could not decode URI:%s",
                (int)pTransferTarget, 0, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
    }
    else {
        /* now get the uri populated */
        if (DIALOG_PopulateReferTo(hDialogReplaces, pMsg) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1,
               "UA_TransferCall: FAILED couldn't populate referto from dialog",
                0, 0, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_REFER_TO_HF);

    /* method in refer-to populated */
    pMsg->ReferTo.method = method;

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    if (NULL != pMsgBody) {
        /*  receipient list xml body */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
        pMsg->ContentType = eCONTENT_TYPE_RSRC_LISTS;

        if (pMsgBody && pMsgBody->pBody) {
            if (SIP_OK != (UA_SetMsgBody(pMsg, pMsgBody->pBody,
                    pMsgBody->length))) {
                SIP_freeMsg(pMsg);
                return (SIP_BADPARM);
            }
        }
    }

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_TransferCall: refer to %s)",
            (int)pMsg->ReferTo.uriPlus.uri.user, 0, 0);

    /* run it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_TransferCall: Failed in FSM hUa:%X hDialog:%X to %s",
            (int)hUa, (int)hDialog, (int)pTransferTarget);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_TransferStatus()===================
 *
 * This function is used to report the status of an incoming transfer (REFER)
 * request.  This status ultimately sends a NOTIFY request.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog = A handle to the dialog (call leg) that you reporting the status on
 *           
 * responseCode = The response code to return in the payload of the NOTIFY
 *                (as specified in RFC3515 section 2.4.5).
 *           For example,  
 *           100 = "100 Trying"
 *           200 = "200 OK"
 *           503 = "503 Service Unavailable"
 *           603 = "603 Declined"
 * 
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Event: refer"
 *            pHdrFlds[1] = "Subscription-State: active"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      NOTIFY method and transfer request were successfully sent.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *      SIP_NO_MEM:  Could not allocate necessary memory.
 *      SIP_FAILED:  Could not sent the transfer status (NOTIFY) request.  
 *                   The dialog was in an invalid state
 *      SIP_NOT_SUPPORTED: The responseCode is not understood by the stack
 *
 ******************************************************************************
 */
vint UA_TransferStatus(
    tSipHandle     hUa, 
    tSipHandle     hDialog,
    uint32         responseCode,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tLocalIpConn  *pLocalConn)
{
    /* build a NOTIFY per RFC3515 */
    tSipIntMsg  *pMsg;
    tSipMsgCodes intCode;
    tSipDialog  *pDialog;
    tUa         *pUa;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_TransferStatus: hUa:%X, hDialog:%X ", 
            (int)hUa, (int)hDialog, 0);

    if (!hUa || !hDialog) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;

    /* create a NOTIFY Request and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_NOTIFY;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    
    intCode = MSGCODE_GetInt(responseCode);
    if (intCode == eSIP_RSP_LAST_RESPONSE_CODE) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_TransferStatus: FAILED resp code %d!? for hUa:%X, hDialog:%X",
                (int)responseCode, (int)hUa, (int)hDialog);
        SIP_freeMsg(pMsg);
        return (SIP_NOT_SUPPORTED);
    }
    
    /* 
     * Set the content-type and the content to be 
     * included in the NOTIFY payload
     */
    pMsg->sipfragCode = intCode;
    pMsg->ContentType = eCONTENT_TYPE_SIPFRAG;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);

    /* Set the event to "refer" */
    OSAL_strcpy(pMsg->Event.szPackage, SIP_EVENT_HF_REFER_PKG_STR);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_EVENT_HF);

    /* Set the sub state */
    if (MSGCODE_ISFINAL(intCode)) {
        pMsg->SubState.arg = eSIP_SUBS_HF_TERM_ARG;
    }
    else { 
        pMsg->SubState.arg = eSIP_SUBS_HF_ACTIVE_ARG;
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_SUB_STATE_HF);
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* run it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_TransferStatus: Failed in dialog FSM hUa:%X hDialog:%X",
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_NotifyEvent()===================
 *
 * This function is used to send a NOTIFY to a SUBSCRIBE request.  If the 
 * expires value is zero then the NOTIFY will be sent and the subscription 
 * will be terminated.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog = A handle to the dialog that contains the subscription that the
 *           NOTIFY is being sent on.
 * 
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Event: message-summary"
 *            pHdrFlds[1] = "Subscription-State: terminated;reason=timeout"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMsgBody = A pointer to data to be inserted into the SIP message body of
 *            the NOTIFY Request.
 *
 * msgBodyLen = The length of the data specified in pMsgBody.
 *           
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      NOTIFY request was successfully sent.
 *      SIP_DONE:    NOTIFY request was successfully sent and the dialog used
 *                   to service this subscription event has been destroyed. This
 *                   happens when this routine is used to send a NOTIFY that 
 *                   will "terminate" the subscription.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *      SIP_NO_MEM:  Could not allocate necessary memory to send the NOTIFY. 
 *      SIP_FAILED:  The NOTIFY request could not be sent because the 
 *                   subscription does not exist.
 *
 ******************************************************************************
 */
vint UA_NotifyEvent(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    tLocalIpConn *pLocalConn)
{
    /* build a NOTIFY per RFC3515 */
    tSipIntMsg    *pMsg;
    tSipDialog    *pDialog;
    tUa           *pUa;
    tUaTrans      *pT;
    tIPAddr        addr; 
    vint           status;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_NotifyEvent: hUa:%X, hDialog:%X ", 
            (int)hUa, (int)hDialog, 0);

    if (!hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;
    
    /* create a NOTIFY Request and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_NOTIFY;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    
    /* Include a msg body if one is specified */
    if (pMsgBody && pMsgBody->pBody) {
        if (SIP_OK != UA_SetMsgBody(pMsg, pMsgBody->pBody, pMsgBody->length)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* 
     * If Dialog is NULL then we will send the request 
     * outside the context of the dialog
     */
    if (pDialog == NULL) {
        /* The "To" header field is mandatory */
        if (!(UA_GetHeaderField(SIP_TO_HF_STR, pHdrFlds, numHdrFlds))) {
            /* Then there is no "To" header field and it's mandatory */
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* 
         * Set the from header field.  If one isn't specified then 
         * use the default AOR.
         */
        if (UA_SetFromField(pUa, 
                UA_GetHeaderField(SIP_FROM_HF_STR, pHdrFlds, numHdrFlds),
                NULL, &pMsg->From) == -1) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* Make a 'from' 'tag' */
        HF_GenerateTag(pMsg->From.szTag);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

        /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
        
        /* copy the 'to' to the request uri */
        pMsg->requestUri = pMsg->To.uri;
        
        /* now populate the Cseq */
        pMsg->CSeq.method = eSIP_NOTIFY;
        pMsg->CSeq.seqNum = HF_GenerateSeqNum();
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
        
        /* now populate the callid */
        HF_GenerateCallId(NULL, pMsg->szCallId);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);

        /* Make a via */
        OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
        addr.v4.ul = 0;
        if ((status = HF_MakeVia(&pMsg->ViaList, NULL, NULL, addr, OSAL_FALSE)) != SIP_OK) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
        /* load default and send it */
        SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
        if ((pT = (tUaTrans *)SIP_memPoolAlloc(eSIP_OBJECT_UA_TRANS) ) == NULL) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        pT->hasTried = FALSE;

        if (NULL == (pT->pMsg = SIP_copyMsg(pMsg))) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_NotifyEvent: Failed to cooy pMsg", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanTrans(pT);
            return (SIP_FAILED);
        }

        pT->pUa = pUa;
        if (UA_SendRequest(pUa, &pMsg->requestUri, pT, pMsg, 
                UAC_NotifyNoDialog, NULL, &pT->hTransaction) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_NotifyEvent: Couldn't send NOTIFY method", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanTrans(pT);
            return (SIP_FAILED);
        }
        if (hTransaction) {
            *hTransaction = pT->hTransaction;
        }
    } /* end of pDialog */
    else {
        /* Then the NOTIFY is being sent within the context of a dialog */
        status = UASM_DialogClient(pDialog, pMsg, NULL);
        if (SIP_DONE == status) {
            /* 
             * Then the NOTIFY was succesfull and the dialog needs to be 
             * destroyed.
             */
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_NotifyEvent: Dialog is destroyed hUa:%X hDialog:%X", 
                (int)hUa, (int)hDialog, 0);
            DIALOG_Destroy(pDialog);
            return (SIP_DONE);
        }
        if (SIP_OK != status) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_NotifyEvent: Failed in dialog FSM hUa:%X hDialog:%X", 
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            return (SIP_FAILED);
        }
        /* 
         * Then the handle the user should use for the transaction 
         * should really be the dialog.
         */
        if (hTransaction) {
            *hTransaction = hDialog;
        }
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Register()===================
 *
 * This function is used to register a UA with a registrar proxy as set in 
 * the DCDB (device configuration database).
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * pAor = A pointer to a NULL terminated string representing the AOR that
 *        you wish to register.  If NULL or if the string is empty 
 *        (i.e. pAor[0] == 0), then the first AOR in the 
 *        list of AOR's that were set during UA_Create() are used.
 *
 * reRegInterval = The time in seconds in which the UA will periodically 
 *                 re-register itself with the registrar.  If zero, then 
 *                 this will de-register the UA.
 *                 If there is an error with the registration process, the 
 *                 application will see a eUA_REGISTRATION_FAILED event
 *                 otherwise, if successful, a eUA_REGISTRATION_COMPLETED
 *                 event is passed down to the application.
 *
 * keep = The parameter that explicitly indicate willingness to send keep-alives
 *        towards its adjacent downstream SIP entity.
 *
 * refreshInterval = The time in seconds in which the UA will periodically 
 *                 send a "dummy" packet.  This dummy packet is used to refresh
 *                 NAT mappings when the UA is behind a Firewall/NAT enabled 
 *                 router.  If zero, then the UA will not attempt to send any
 *                 "dummy" packets. Typically, when this value is used it should
 *                 be set to 30 seconds.  Note, if the reRegInterval is less 
 *                 than this value, then this value is ignored and no "dummy" 
 *                 packets are sent.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: Special Agent VoIP"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * pInstanceId = The instance id of ua. If there is it, add +sip.instance
 *
 * pQValue = A pointer to the q-value. If there is it, add ;q=
 *
 * RETURNS:
 *      SIP_OK:      A register method attempt was sent
 *      SIP_FAILED:  There was an error sending the registration method
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) There was a problem with the AOR specified in pAor.
 *      SIP_BUSY:    The AOR is currently in the middle of 
 *                   registration. Please try again later.
 *
 ******************************************************************************
 */
vint UA_Register(
    tSipHandle    hUa,
    char         *pAor,
    uint32        reRegInterval,
    OSAL_Boolean  keep,
    uint32        refreshInterval,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn,
    vint          isEmergency,
    char         *pInstanceId,
    char         *pQValue)
{
    /* build a register */
    tSipIntMsg      *pMsg;
    tUaReg          *pReg;
    tIPAddr          addr; 
    int              idx;
    tUa             *pUa;
    tContactHFE     *pContact;
    tDLListEntry    *pEntry;
    char             ipAddr[16];
    char             ipv6Addr[39];
    
    /* you only generate one Register call-ID per UA per boot cycle */
    if (!hUa) {
        return (SIP_BADPARM);
    }
    pUa = (tUa*)hUa;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_Register: hUa:%X hLocalConn:%X", 
        (int)hUa, (int)pLocalConn, 0);

    /* create a register and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_REGISTER;
    pMsg->msgType = eSIP_REQUEST;

    
    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);

    /* build the 'from' */
    if (-1 == (idx = UA_SetFromField(pUa, pAor, "", &pMsg->From))) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);
    
    /* build the 'To', same as 'From' for registration */
    pMsg->To = pMsg->From;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);
    /* make a 'from' 'tag' */
    HF_GenerateTag(pMsg->From.szTag);
    
    /* 
     * Get the tUaReg object that corresponds 
     * to the aor specified in pAor
     */
    pReg = &pUa->Reg[idx];
    if (pReg->isBusy == TRUE) {
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_BUSY);
    }
    /* place the register FSM into a valid state and set the expiry */
    pReg->isBusy          = TRUE;
    pReg->hasTried        = 0;
    pReg->retryCnt        = 1;
    pReg->reRegInterval   = reRegInterval;
    pReg->refreshEnable   = keep;
    pReg->refreshInterval = refreshInterval;
    
     /* build the request uri, get the info from the configuration */
    if (UA_PopulateRegister(pUa, pMsg) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Register: Error Registrar could not be configured for hUa:%X",
            (int)hUa, 0, 0);
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_BADPARM);
    }

    OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
    addr.v4.ul = 0;
    if (SIP_OK != HF_MakeVia(&pMsg->ViaList, NULL, NULL, addr, keep)) {
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_NO_MEM);
    }
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
    
    /* Add the 'Contact' header field */
    UA_PopulateContact(pUa, pMsg);

    /* Add the capabilitiesBitmap */
    if (0 != capabilitiesBitmap) {
        pEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
            pContact = (tContactHFE*)pEntry;
            pContact->capabilitiesBitmap = capabilitiesBitmap;
        }
    }

    /* Add sip-instance */
    if ((NULL != pInstanceId) && (0 != *pInstanceId)) {
        pEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
            pContact = (tContactHFE*)pEntry;
            OSAL_snprintf(pContact->szInstance, SIP_INSTANCE_STR_SIZE, "%s",
                    pInstanceId);
        }
    }

    /* Add q value */
    if (NULL != pQValue && 0 != *pQValue) {
        pEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pEntry)) {
            pContact = (tContactHFE*)pEntry;
            OSAL_snprintf(pContact->q, SIP_Q_VALUE_STR_SIZE, "%s",
                    pQValue);
        }
    }    

    /* Add the 'Route' header field */
    UA_PopulateRoute(pUa, pMsg);
        
    /* now populate the Cseq */
    pMsg->CSeq.method = eSIP_REGISTER;
    pMsg->CSeq.seqNum = 1;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
    
    HF_GenerateCallId(NULL, pMsg->szCallId); 
    /* now populate the callid */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);
    
    /* Set the content length */
    pMsg->ContentLength = 0;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);

    /* Set the register expiry */
    pMsg->Expires = pReg->reRegInterval;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);
    
    /* load the defaults if the presence above were set */
    SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
    
    /* 
     * Save a copy of the register message for the re-register
     * interval expiry 
     */
    if (pReg->pMsg) {
        SIP_freeMsg(pReg->pMsg);
    }
    if (NULL == (pReg->pMsg = SIP_copyMsg(pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Register: Failed to cooy pMsg", 0, 0, 0);
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_FAILED);
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->regLclConn = *pLocalConn;
        if ((pLocalConn->addr.type == OSAL_NET_SOCK_UDP_V6) ||
                (pLocalConn->addr.type == OSAL_NET_SOCK_TCP_V6)) {
            OSAL_netAddressToString((int8 *)ipv6Addr, &pLocalConn->addr);
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Register: using local conn info port:%d ip(v6):%s fd:%d",
                (uint32)(pLocalConn->addr.port), (uint32)ipv6Addr,
                (uint32)(pLocalConn->fd));
        }
        else {
            OSAL_netAddressToString((int8 *)ipAddr, &pLocalConn->addr);
            SIP_DebugLog(SIP_DB_UA_LVL_1,
                "UA_Register: using local conn info port:%d ip(v4):%s fd:%d",
                (uint32)(pLocalConn->addr.port), (uint32)ipAddr,
                (uint32)(pLocalConn->fd));
        }
    }
        
    /* send it */
    if (UA_SendRequest(pUa, &pMsg->requestUri, pReg, pMsg, UAC_Register, NULL,
            &pReg->hTransaction) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Register: Couldn't send REGISTER method", 0, 0, 0);
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_FAILED);
    }

    /* 
     * create the re-register timer 
     * If it was emergency call we would not create timer to re-register.
     */
    if (pReg->hReRegTimer == NULL && !isEmergency) {
        pReg->hReRegTimer = SIPTIMER_Create((tSipHandle)pUa->taskId);
    }
    /* create the NAT mapping refresh timer stuff */
    if (pReg->hRefreshTimer == NULL) {
        pReg->hRefreshTimer = SIPTIMER_Create((tSipHandle)pUa->taskId);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_ReRegister()===================
 *
 * This function is used re-register or to update SIP timer to re-register of a UA.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * pAor = A pointer to a NULL terminated string representing the AOR that
 *        you wish to register.  If NULL or if the string is empty 
 *        (i.e. pAor[0] == 0), then the first AOR in the 
 *        list of AOR's that were set during UA_Create() are used.
 *
 * pAkaAuthResp = A pointer to null terminated string of AKA authentication
 *              response. If it's not NULL, then do registration retry with
 *              existing registration and pass the pAkaAuthResp to
 *              UA_WWWAuthenticat() to calculate digest.
 *
 * pAkaAuthAuts = A pointer to AUTS.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 * expires = A expire time from the shortened NOTIFY event. UA would update the SIP timer
 *               and  to re-register when the SIP timer timeout.
 *
 * RETURNS:
 *      SIP_OK:      A register method attempt was sent
 *      SIP_FAILED:  There was an error sending the registration method
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) There was a problem with the AOR specified in pAor.
 *      SIP_BUSY:    The AOR is currently in the middle of 
 *                   registration. Please try again later.
 *
 ******************************************************************************
 */
vint UA_ReRegister(
    tSipHandle    hUa,
    char         *pAor,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    uint8        *pAkaAuthResp,
    vint          akaAuthResLen,
    uint8        *pAkaAuthAuts,
    tLocalIpConn *pLocalConn,
    vint          isEmergency,
    vint          expires)
{
    /* build a register */
    tSipIntMsg       *pMsg;
    tUaReg           *pReg;
    int               idx;
    tUa              *pUa;
    tUriPlus          from;
    tDLList           tmpList;
    
    /* you only generate one Register call-ID per UA per boot cycle */
    if (!hUa) {
        return (SIP_BADPARM);
    }
    pUa = (tUa*)hUa;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_ReRegister: hUa:%X hLocalConn:%X", 
        (int)hUa, (int)pLocalConn, 0);

    /* build the 'from' */
    if (-1 == (idx = UA_SetFromField(pUa, pAor, "", &from))) {
        return (SIP_BADPARM);
    }
        
    /* 
     * Get the tUaReg object that corresponds 
     * to the aor specified in pAor
     */
    pReg = &pUa->Reg[idx];

    if (pReg->isBusy == TRUE) {
        return (SIP_BUSY);
    }   

    if (NULL == (pMsg = pReg->pMsg)) {
        /* No registration exists, something wrong here */
        SIP_DebugLog(SIP_DB_UA_LVL_3, 
                "UA_ReRegister: No registration exists for hUa:%X",
                (int)hUa, 0, 0);
        UA_CleanRegistration(pReg);
        return (SIP_FAILED);
    }


    /* kill any existing re-reg timers */
    if (pReg->hReRegTimer) {
        SIPTIMER_Destroy(pReg->hReRegTimer);
        SIPTIMER_AddWakeUpTime(0); //reset watchdog timer
        pReg->hReRegTimer = NULL;
    }
    /* kill any existing re-reg timers */
    if (pReg->hRefreshTimer) {
        SIPTIMER_Destroy(pReg->hRefreshTimer);
        pReg->hRefreshTimer = NULL;
    }
    
    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* increment the sequence number */
    pMsg->CSeq.seqNum++;
    /* new transaction so new via branch */
    HF_MakeViaBranch(&pMsg->ViaList, NULL);
    /* generate a new 'fromTag' */
    HF_GenerateTag(pMsg->From.szTag);

    /* if there is an authorization list then re-calulate the response */
    if ((NULL != pAkaAuthResp) && !DLLIST_IsEmpty(&pMsg->AuthorizationList)) {
        /* Copy auth list to a temp list */
        DLLIST_InitList(&tmpList);
        if (SIP_OK != DLLIST_Copy(&pMsg->AuthorizationList,
                    &tmpList, eDLLIST_WWW_AUTH_HF)) {
            SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_ReRegister: Fail to copy "
                    "authorization list.", 0, 0, 0);
            UA_CleanRegistration(pReg);
            return (SIP_FAILED);
        }
        /*  
         * Sync failure case for AKA authentication.
         * Give it one more time to retry
         */
        if (pAkaAuthAuts != NULL) {
            if (pReg->retryCnt == 1) {
                pReg->retryCnt = 2;
            }
            else {
                /* Second Time of Sync failure. Treat it as Failure */
                UA_CleanRegistration(pReg);
                return (SIP_FAILED);
            }
        }
        /* Do authentication */
        if (SIP_OK != UA_WWWAuthenticate(pUa, pMsg->From.uri.user,
                    &tmpList,
                    &pMsg->AuthorizationList,
                    (char *)pAkaAuthResp,
                    akaAuthResLen,
                    (char *)pAkaAuthAuts)) {
            SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_ReRegister: UA_AkaAuthenticate "
                    "failed.", 0, 0, 0);
            /* clear the temp list */
            DLLIST_Empty(&tmpList, eSIP_OBJECT_AUTH_HF);
            UA_CleanRegistration(pReg);
            return (SIP_FAILED);
        }
        if (pMsg->code == eSIP_RSP_UNAUTH) {
            HF_SetPresence(&pMsg->x.ECPresenceMasks,
                    eSIP_AUTHORIZATION_HF);
        }
        else {
            HF_SetPresence(&pMsg->x.ECPresenceMasks,
                    eSIP_PROXY_AUTHORIZATION_HF);
        }
        /* clear the temp list */
        DLLIST_Empty(&tmpList, eSIP_OBJECT_AUTH_HF);
    }

    if (NULL == (pReg->pMsg = SIP_copyMsg(pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_ReRegister: Failed to cooy pMsg", 0, 0, 0);
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_FAILED);
    }
    pReg->isBusy = TRUE;
    pReg->hasTried++;

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->regLclConn = *pLocalConn;
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_ReRegister: using local conn info port:%d ip:%X fd:%d",
            (int)pLocalConn->addr.port, (int)pLocalConn->addr.ipv4,
            pLocalConn->fd);
    }
    /* If expires is 0, send Re-register directly. */
    if (0 == expires) {    
        /* send it */
        if (UA_SendRequest(pUa, &pMsg->requestUri, pReg, pMsg, UAC_Register, NULL,
                &pReg->hTransaction) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_ReRegister: Couldn't send REGISTER method", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanRegistration(pReg);
            return (SIP_FAILED);
        }
    }
    /* 
     * create the re-register timer 
     * If it was emergency call we would not create timer to re-register.
     */
    if (pReg->hReRegTimer == NULL && !isEmergency) {
        pReg->hReRegTimer = SIPTIMER_Create((tSipHandle)pUa->taskId);
    }
    /* create the NAT mapping refresh timer stuff */
    if (pReg->hRefreshTimer == NULL) {
        pReg->hRefreshTimer = SIPTIMER_Create((tSipHandle)pUa->taskId);
    }
    /* If expires is not 0, to start the SIP timer to re-Register */
    if ((expires != 0) && (pReg->hReRegTimer != NULL)) {
        expires = (uint32)UA_GetTimerMs(expires);
        SIPTIMER_Start(pReg->hReRegTimer, UA_ReRegisterCB, 
                pReg, expires, FALSE);
        /* store it for watchdog timer , we need this information to take care lazy timer */
        SIPTIMER_AddWakeUpTime(expires + SIPTIMER_REGISTER_BUFFER_TIMER_MS);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Publish()===================
 *
 * This function is used to publish event states to a remote endpoint or server. 
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * hDialog = A handle to a dialog.  If this is NULL (Zero), then the publication 
 *           will be sent outside the context of a dialog.  Else, it will be 
 *           sent to the remote device using the information currently inside 
 *           the dialog.  Typically this value should be NULL as there is no 
 *           behavior defined in any IETF RFC regarding sending PUBLISH 
 *           requests within a dialog.
 *
 * hTransaction = A pointer to a 'void*' (void pointer).  If Not NULL then 
 *                the stack will populate this address to a pointer with 
 *                a unique transaction that applications can use later 
 *                to match PUBLISH requests with publish related events.  
 *                If NULL, then this value is NOT USED.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "To: sip:mypresenceserver.com"
 *            pHdrFlds[1] = "From: sip:me@d2tech.com"
 *            pHdrFlds[2] = "Event: presence"
 *            pHdrFlds[3] = "Expires: 600"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      A PUBLISH method was successfully sent.
 *      SIP_FAILED:  There was an error sending the PUBLISH method.
 *      SIP_NO_MEM:  Could not allocate memory needed to send this request.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *      SIP_BUSY:    This UA is currently in the middle of sending a PUBLISH
 *                   request. Please try again later.
 *
 ******************************************************************************
 */
vint UA_Publish(
    tSipHandle    hUa,
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg       *pMsg;
    tUaPub           *pPub;
    tIPAddr           addr; 
    int               idx;
    tUa              *pUa;
    tUriPlus          from;
    char             *pFrom;
    char             *pTo;
    char             *pExpires;

    if (NULL == hUa) {
        return (SIP_BADPARM);
    }
    pUa = (tUa*)hUa;

    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Pubish: hUa:%X hLocalConn:%X",
        (int)hUa, (int)pLocalConn, 0);
    
    /* 
     * Check if the PUBLISH should be sent within a dialog. This is legal in SIP 
     * however, there is not an industry definition of why anyone would ever 
     * do this.
     */
    if (NULL != hDialog) {
        /* create a PUBLISH request and populate */
        if (NULL == (pMsg = SIP_allocMsg())) {
            SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
            return (SIP_NO_MEM);
        }        
        /* Populate the rest of the request */
        pMsg->method = eSIP_PUBLISH;
        pMsg->msgType = eSIP_REQUEST;
        
        /* Set the refresh rate */
        SIP_DebugLog(SIP_DB_UA_LVL_2, "Setting refresh rate", 0, 0, 0);
        pExpires = UA_GetHeaderField(SIP_EXPIRES_HF_STR, pHdrFlds, numHdrFlds);
        if (NULL != pExpires) {
            pMsg->Expires = OSAL_atoi(pExpires);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);    
        }
        
        /* populate any header fields from the function caller */
        if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
            SIP_freeMsg(pMsg);
            SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
            return (SIP_BADPARM);
        }

        /* load the defaults if the presence above were set */
        SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);

        /* Include and msg body, if there is one */
        if (pMsgBody && pMsgBody->pBody) {
            if (SIP_OK != UA_SetMsgBody(pMsg, pMsgBody->pBody,
                    pMsgBody->length)) {
                SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
                SIP_freeMsg(pMsg);
                return (SIP_NO_MEM);
            }
        }
        
        /* run it through the state machine */
        if (UASM_DialogClient(hDialog, pMsg, NULL) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Publish: Failed in dialog FSM hUa:%X hDialog:%X", 
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
            return (SIP_FAILED);
        }
        if (NULL != hTransaction) {
            *hTransaction = hDialog;
        }
        return (SIP_OK);
    }
    
    /* IF WE ARE HERE THEN THE PUBLISH IS BEING SENT OUTSIDE A DIALOG */
    
    pFrom = UA_GetHeaderField(SIP_FROM_HF_STR, pHdrFlds, numHdrFlds);
    pTo = UA_GetHeaderField(SIP_TO_HF_STR, pHdrFlds, numHdrFlds);
    if (NULL == pFrom || NULL == pTo) {
        /* Both of these are mandatory, so if they don't exist then return */
        SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
        return (SIP_BADPARM);
    }
    
    /* build a 'From' header field */
    if (-1 == (idx = UA_SetFromField(pUa, pFrom, "", &from))) {
        /* Then the from field is not valid */
        SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
        return (SIP_BADPARM);
    }
    
    /* Get the tUaPub object that corresponds to the aor specified in pFrom */
    pPub = &pUa->Pub[idx];
    if (pPub->isBusy == TRUE) {
        /* Then we are currently in the middle of a PUBLISH Request */ 
        SIP_DebugLog(SIP_DB_UA_LVL_2, "Error: %s : %d", (uint32)__FUNCTION__, (uint32)__LINE__, 0);
        return (SIP_BUSY);
    }
    
    /* 
     * If we are currently active (pMsg is not NULL) then this is either a 
     * "modified" PUBLISH, meaning we are updating presence state or a 
     * "cancelation" to a PUBLISH. Eitherway we will reuse the existing SIP
     * message used for this PUBLISH session. 
     */
    if (pPub->pMsg != NULL) {
        pMsg = pPub->pMsg;
        /* Increment the CSeq */
        pMsg->CSeq.seqNum++;
        /* Set the refresh rate */
        pExpires = UA_GetHeaderField(SIP_EXPIRES_HF_STR, pHdrFlds, numHdrFlds);
        if (NULL == pExpires || 0 == pExpires[0]) {
            /* 
             * Then the user does not want an 'Expires' value but we need it to
             * be non-zero, so set the value to something positive BUT DISABLE
             * the "Encode bit".
             */ 
            pMsg->Expires = 3600;
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);
        }
        else {
            pMsg->Expires = OSAL_atoi(pExpires);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);    
        }
        /* make a new branch */
        HF_MakeViaBranch(&pMsg->ViaList, NULL);
        /* make a new 'from' 'tag' */
        HF_GenerateTag(pMsg->From.szTag);
        /* make a new callId */
        HF_GenerateCallId(NULL, pMsg->szCallId);

        /* Clear any left over SIP-If-Tag (from SIP-ETag) */
        HF_Delete(&pMsg->pHFList, eSIP_IF_MATCH_HF);
        HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_IF_MATCH_HF); 
    }
    else {
        /* create a PUBLISH request and populate */
        if (NULL == (pMsg = SIP_allocMsg())) {
            return (SIP_NO_MEM);
        }        
        /* Populate the rest of the request */
        pMsg->method = eSIP_PUBLISH;
        pMsg->msgType = eSIP_REQUEST;
        
        /* Set the refresh rate */
        pExpires = UA_GetHeaderField(SIP_EXPIRES_HF_STR, pHdrFlds, numHdrFlds);
        if (NULL == pExpires || 0 == pExpires[0]) {
            /* 
             * Then the user does not want an 'Expires' value but we need it to
             * be non-zero, so set the value to something positive BUT DISABLE
             * the "Encode bit".
             */ 
            pMsg->Expires = 3600;
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);
        }
        else {
            pMsg->Expires = OSAL_atoi(pExpires);
            HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);    
        }
        
        /* Set the "From" header field */
        pMsg->From = from;
        /* make a 'from' 'tag' */
        HF_GenerateTag(pMsg->From.szTag);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);
        
        /* Build the 'To' header field */
        HF_CleanUriPlus(&pMsg->To);
        if (DEC_Uri(pTo, OSAL_strlen(pTo), &pMsg->To.uri) != SIP_OK) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);
        
        /* Add the 'Contact' header field */
        UA_PopulateContact(pUa, pMsg);
        
        /* Add the 'Route' header field */
        UA_PopulateRoute(pUa, pMsg);
        
        /* build the request uri, based on the to */
        pMsg->requestUri = pMsg->To.uri;

        OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
        addr.v4.ul = 0;
        if (SIP_OK != HF_MakeVia(&pMsg->ViaList, NULL, NULL, addr, OSAL_FALSE)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
        
        /* Add a 'route' header field if need be */
        UA_PopulateRoute(pUa, pMsg);

        /* now populate the Cseq */
        pMsg->CSeq.method = eSIP_PUBLISH;
        pMsg->CSeq.seqNum = 1;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);

        /* now populate the callid */
        HF_GenerateCallId(NULL, pMsg->szCallId);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);
        
        /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
        
        /* Make a copy to send */
        pPub->pMsg = pMsg;
    }

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        UA_CleanPublish(pPub);
        return (SIP_BADPARM);
    }

    /* load the defaults if the presence above were set */
    SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
    
    /* Include and msg body, if there is one */
    if (pMsgBody && pMsgBody->pBody) {
        /* 
         * Make sure that if there's already a message bosy that it's first 
         * freed 
         */
        if (NULL != pMsg->pMsgBody) {
            SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY,
                    (tDLListEntry *)pMsg->pMsgBody);
            pMsg->pMsgBody = NULL;
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
            pMsg->ContentType = eCONTENT_TYPE_NONE;
            pMsg->ContentLength = 0;
            HF_ClrPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);
        }
        if (SIP_OK != UA_SetMsgBody(pMsg, pMsgBody->pBody, pMsgBody->length)) {
            SIP_freeMsg(pMsg);
            UA_CleanPublish(pPub);
            return (SIP_NO_MEM);
        }
    }
    
    if (NULL == (pMsg = SIP_copyMsg(pMsg))) {
        UA_CleanPublish(pPub);
        return(SIP_NO_MEM);
    }
        
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->pubLclConn = *pLocalConn;
    }
    
    /* Place the PUBLISH FSM into a valid state and set the expiry */
    pPub->isBusy = TRUE;
    pPub->hasTried = FALSE;
    
    /* send it */
    if (UA_SendRequest(pUa, NULL, pPub, pMsg, UAC_PublishNoDialog, NULL,
            &pPub->hTransaction) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Publish: Couldn't send PUBLISH method", 0, 0, 0);
        SIP_freeMsg(pMsg);
        UA_CleanPublish(pPub);
        return (SIP_FAILED);
    }

    /* start the refresh timer stuff */
    if (pPub->hTimer == NULL) {
        pPub->hTimer = SIPTIMER_Create((tSipHandle)pUa->taskId);
    }
    if (NULL != hTransaction) {
        *hTransaction = pPub->hTransaction;
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_RegisterResp()===================
 *
 * This function is used to return a SIP response to a register request.
 * When a REGISTER request is received a eUA_REGISTRATION event is generated
 * to a SIP application.  Applications use this interface to return a response 
 * to eUA_REGISTRATION events.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * responseCode = The response code you would like to return.
 *                For example '200' would be "Okay".
 *
 * pReasonPhrase = This is the string included at the top of the response 
 *                 that describes the type of responseCode.
 *                 IF NULL, then the system will use a default string for the 
 *                 response code specified in responseCode.  For example,
 *                 If responseCode was '200' then the string would be 
 *                 "OK"
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Expires: 60"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array. 
 *
 * RETURNS:
 *   SIP_OK:            Response was successfully sent.
 *   SIP_FAILED:        Could not send the response.  The server registration
 *                      mechanism was in a bad state..
 *   SIP_BADPARM:       Function failed because of one of the following:
 *                       1) The hUa parameter was invalid.
 *                       2) One of the header fields in the pHdrFlds array 
 *                          could not be understood.
 *                      3) The pReasonPhrase string was too long
 *   SIP_NOT_SUPPORTED:  The SIP stack can't identify the response code
 ******************************************************************************
 */
vint UA_RegisterResp(
    tSipHandle    hUa, 
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds)
{
    tUa         *pUa;
    tSipIntMsg  *pMsg;
    tSipMsgCodes intCode;
    tSipHandle   hTransaction;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_RegisterResp: UA:%X", (int)hUa, 0, 0);
        
    if (!hUa) {
        return (SIP_BADPARM);
    }
    
    pUa = (tUa*)hUa;
    if (!pUa->ProxyReg.pMsg || !pUa->ProxyReg.hTransaction) {
        return (SIP_FAILED);
    }

    pMsg = pUa->ProxyReg.pMsg;
    pUa->ProxyReg.pMsg = NULL;
    hTransaction = pUa->ProxyReg.hTransaction;
    pUa->ProxyReg.hTransaction = NULL;
    
    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
       /* reinstate everything and return */
       pUa->ProxyReg.pMsg = pMsg;
       pUa->ProxyReg.hTransaction = hTransaction;
       return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    
    pMsg->msgType = eSIP_RESPONSE;
    intCode = MSGCODE_GetInt(responseCode);
    if (intCode == eSIP_RSP_LAST_RESPONSE_CODE) {
       /* reinstate everything and return */
       pUa->ProxyReg.pMsg = pMsg;
       pUa->ProxyReg.hTransaction = hTransaction;
       return (SIP_NOT_SUPPORTED);
    }
    MSGCODE_Create(pMsg, NULL, intCode);

    if (pReasonPhrase) {
        if (OSAL_strlen(pReasonPhrase) >= SIP_MAX_REASON_PHRASE_STR_LEN) {
            pUa->ProxyReg.pMsg = pMsg;
            pUa->ProxyReg.hTransaction = hTransaction;
            return (SIP_BADPARM);
        }
        SIP_copyStringToSipText(pReasonPhrase, &pMsg->pReasonPhrase);
    }

    /* Send the response */
    if (TU_SendResponse(pMsg, hTransaction) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_RegisterResp: Failed to send response UA:%X", 
            (int)hUa, 0, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_UnRegister()===================
 *
 * This function is used to un-register a UA with a registrar proxy
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * pAor = A pointer to a NULL terminated string representing the AOR that
 *        you wish to un-register.  If NULL or if the string is empty 
 *        (i.e. pAor[0] == 0), then the first AOR in the 
 *        list of AOR's that were set during UA_Create() are used.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: Special Agent VoIP"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      A un-register method attempt was sent
 *      SIP_FAILED:  There was an error sending the registration method
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) There was a problem with the AOR specified in pAor.
 *      SIP_BUSY:    The AOR is currently in the middle of 
 *                   registration. Please try again later.
 *      SIP_NO_MEM:  Could not allocate memory needed to build and send the 
 *                   REGISTER request.
 *
 ******************************************************************************
 */
vint UA_UnRegister(
    tSipHandle    hUa,
    char         *pAor,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg       *pMsg;
    tUaReg           *pReg;
    tUriPlus          from;
    int               idx;
    tIPAddr           addr;

    tUa *pUa = (tUa*)hUa;
    if (!hUa) {
        return (SIP_BADPARM);
    }

    SIP_DebugLog(SIP_DB_UA_LVL_3, 
        "UA_UnRegister: hUa:%X hLocalConn:%X", (int)hUa, (int)pLocalConn, 0);

    /* build the 'from' */
    if (-1 == (idx = UA_SetFromField(pUa, pAor, "", &from))) {
        return (SIP_BADPARM);
    }
    
    /* 
     * Get the tUaReg object that corresponds 
     * to the aor specified in pAor
     */
    pReg = &pUa->Reg[idx];

    if (0 == pReg->reRegInterval) {
        /* Already de-registered, no need to de-register again */
        return (SIP_FAILED);
    }

    /* kill any existing re-reg timers */
    if (pReg->hReRegTimer) {
        SIPTIMER_Destroy(pReg->hReRegTimer);
        SIPTIMER_AddWakeUpTime(0); //reset watchdog timer
        pReg->hReRegTimer = NULL;
    }
    /* kill any existing re-reg timers */
    if (pReg->hRefreshTimer) {
        SIPTIMER_Destroy(pReg->hRefreshTimer);
        pReg->hRefreshTimer = NULL;
    }

    if (pReg->isBusy == TRUE) {
        return (SIP_BUSY);
    }
#ifdef SIP_UN_REG_WITH_AUTH
    /* place the register FSM into a valid state */
    pReg->isBusy = TRUE;
#endif
    pReg->hasTried = 0;

    if ((pMsg = pReg->pMsg) == NULL) {
        /* then make a new register message */
        if ((pMsg = SIP_allocMsg()) == NULL) {
            return (SIP_NO_MEM);
        }
        pMsg->method = eSIP_REGISTER;
        pMsg->msgType = eSIP_REQUEST;
        /* build the 'To' and 'From' for de-registration */
        pMsg->To.uri = from.uri;
        pMsg->From.uri = from.uri;
        /* make a 'from' 'tag' */
        HF_GenerateTag(pMsg->From.szTag);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_TO_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

        /* build the request uri, get the info from the configuration */
        if (UA_PopulateRegister(pUa, pMsg) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_UnRegister: Error couldn't configure reg info for hUa:%X",
                (int)hUa, 0, 0);
            SIP_freeMsg(pMsg);
            return (SIP_FAILED);
        }
        
        UA_PopulateContact(pUa, pMsg);
        
        UA_PopulateRoute(pUa, pMsg);
        
        /* now populate the callid */
        HF_GenerateCallId(NULL, pMsg->szCallId);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);

        /* Make sure expires is enabled.  The actual value will be set below */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_EXPIRES_HF);
        
        /* now populate the Cseq */
        pMsg->CSeq.method = eSIP_REGISTER;
        pMsg->CSeq.seqNum = 0;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
    
        /* Set the content length */
        pMsg->ContentLength = 0;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);

        /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);

        /* load the defaults if the presence above were set */
        SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);

        OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
        addr.v4.ul = 0;
        if (HF_MakeVia(&pMsg->ViaList, NULL, NULL, addr, OSAL_FALSE) != SIP_OK) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
    }
    else {
        /* 
         * clear out the pMsg from the pReg because 
         * we won't use it again.  The reason we don't 
         * have to free it here is because we will use 
         * it for the last De-registration.
         */
        pReg->pMsg = NULL;
        if (HF_MakeViaBranch(&pMsg->ViaList, NULL) != SIP_OK) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
    }
    /* Zero will kill the re-reg timing */
    pMsg->Expires = 0;
    /* increment the sequence number */
    pMsg->CSeq.seqNum++;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->regLclConn = *pLocalConn;
    }
    
#ifdef SIP_UN_REG_WITH_AUTH
    /* make a copy because we may need to authenticate */
    if (NULL == (pReg->pMsg = SIP_copyMsg(pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_UnRegister: Failed to cooy pMsg", 0, 0, 0);
        SIP_freeMsg(pMsg);
        UA_CleanRegistration(pReg);
        return (SIP_FAILED);
    }

    /* send it */
    if (UA_SendRequest(pUa, &pMsg->requestUri, pReg, pMsg, 
            UAC_Register, NULL, &pReg->hTransaction) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
#else 
    if (UA_SendRequest(pUa, &pMsg->requestUri, pReg, pMsg, 
            UAC_UnRegister, NULL, &pReg->hTransaction) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    UA_CleanRegistration(pReg);
#endif
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_CancelRegister()===================
 *
 * This function is used to un-register a UA with a registrar proxy
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created
 *        via UA_Create().
 *
 * pAor = A pointer to a NULL terminated string representing the AOR that
 *        you wish to un-register.  If NULL or if the string is empty
 *        (i.e. pAor[0] == 0), then the first AOR in the
 *        list of AOR's that were set during UA_Create() are used.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "User-Agent: Special Agent VoIP"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      A un-register method attempt was sent
 *      SIP_FAILED:  There was an error sending the registration method
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array
 *                       could not be understood.
 *                    3) There was a problem with the AOR specified in pAor.
 *      SIP_BUSY:    The AOR is currently in the middle of
 *                   registration. Please try again later.
 *      SIP_NO_MEM:  Could not allocate memory needed to build and send the
 *                   REGISTER request.
 *
 ******************************************************************************
 */
vint UA_CancelRegister(
    tSipHandle    hUa,
    char         *pAor,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn)
{
    tUaReg           *pReg;
    tUriPlus          from;
    int               idx;

    tUa *pUa = (tUa*)hUa;
    if (!hUa) {
        return (SIP_BADPARM);
    }

    SIP_DebugLog(SIP_DB_UA_LVL_3,
        "UA_CancelRegister: hUa:%X hLocalConn:%X", (int)hUa, (int)pLocalConn, 0);

    /* build the 'from' */
    if (-1 == (idx = UA_SetFromField(pUa, pAor, "", &from))) {
        return (SIP_BADPARM);
    }

    /*
     * Get the tUaReg object that corresponds
     * to the aor specified in pAor
     */
    pReg = &pUa->Reg[idx];

    UA_CleanRegistration(pReg);

    return (SIP_OK);
}

/* 
 * ======== UA_CancelUnRegister() ========
 * To cancel the unregister
 */

vint UA_CancelUnRegister(
    tSipHandle    hUa,
    char         *pAor,
    tLocalIpConn *pLocalConn)
{
    tUaReg           *pReg;
    tUriPlus          from;
    int               idx;

    tUa *pUa = (tUa*)hUa;
    if (!hUa) {
        return (SIP_BADPARM);
    }

    SIP_DebugLog(SIP_DB_UA_LVL_3,
        "UA_CancelUnRegister: hUa:%X hLocalConn:%X", (int)hUa, (int)pLocalConn, 0);

    /* build the 'from' */
    if (-1 == (idx = UA_SetFromField(pUa, pAor, "", &from))) {
        return (SIP_BADPARM);
    }

    /*
     * Get the tUaReg object that corresponds
     * to the aor specified in pAor
     */
    pReg = &pUa->Reg[idx];

    pReg->isBusy = FALSE;

    return (SIP_OK);
}

/*
 *****************************************************************************
 * ================UA_Message()===================
 *
 * This function is used to send a TEXT message.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * hDialog = A handle to a dialog.  If this is NULL (Zero), then the text 
 *           message will be sent outside the context of a dialog.  Else,
 *           It will be sent to the remote device using the information
 *           currently inside the dialog.
 *
 * hTransaction = A pointer to a 'void*' (void pointer).  If Not NULL then 
 *                the stack will populate this address to a pointer with 
 *                a unique transaction that applications can use later 
 *                to match MESSAGE requests with events.  If NULL, then this
 *                value is NOT USED.
 * 
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMsgBody = A pointer to a tUaMsgBody object that contains information 
 *           about the text message.  Note, that the stack will "copy out" 
 *           all the data found in the tUaMsgBody.  For example, in the
 *           tUaMsgBody object is a pointer to a string containing the 
 *           actual message, the stack will NOT use that pointer, rather, 
 *           it will allocate memory from the heap and make a copy of the 
 *           message.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      The MESSAGE method was successfully sent
 *      SIP_FAILED:  Failed to sent the message.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa or pMsgBody parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) The "To" or "From" heder fields were missing when 
 *                       mandatory.
 *      SIP_NO_MEM:  Could not allocate memory needed to send the message
 *
 ******************************************************************************
 */
vint UA_Message(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg    *pMsg;
    tSipDialog    *pDialog;
    tUa           *pUa;
    tUaTrans      *pT;
    tIPAddr        addr; 
    tDLListEntry   *pDLLEntry;
    tContactHFE    *pContact;

    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Message: -hUa:%X", (int)hUa, 0, 0);

    if (!hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;
    
    /* create a MESSAGE Request and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_MESSAGE;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
    
    /* Include a msg body if one is specified */
    if (pMsgBody && pMsgBody->pBody) {
        if (SIP_OK != UA_SetMsgBody(pMsg, pMsgBody->pBody, pMsgBody->length)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->txtLclConn = *pLocalConn;
    }

    /* 
     * If Dialog is NULL then we will send the request 
     * outside the context of the dialog
     */
    if (pDialog == NULL) {
        /* The "To" header field is mandatory */
        if (!(UA_GetHeaderField(SIP_TO_HF_STR, pHdrFlds, numHdrFlds))) {
            /* Then there is no "To" header field and it's mandatory */
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* 
         * Set the from header field.  If one isn't specified then 
         * use the default AOR.
         */
        if (UA_SetFromField(pUa, 
                UA_GetHeaderField(SIP_FROM_HF_STR, pHdrFlds, numHdrFlds),
                NULL, &pMsg->From) == -1) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* Make a 'from' 'tag' */
        HF_GenerateTag(pMsg->From.szTag);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

        /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
    
        /* copy the 'to' to the request uri */
        pMsg->requestUri = pMsg->To.uri;
        
        /* now populate the Cseq */
        pMsg->CSeq.method = eSIP_MESSAGE;
        pMsg->CSeq.seqNum = HF_GenerateSeqNum();
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
        
        /* now populate the callid */
        HF_GenerateCallId(NULL, pMsg->szCallId);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);

        /* Make a via */
        OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
        addr.v4.ul = 0;
        if (SIP_OK != HF_MakeVia(&pMsg->ViaList, NULL, NULL, addr, OSAL_FALSE)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
        /* load defaults. */
        SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);

        /* Add the capabilties */
        if (0 != capabilitiesBitmap) {
            UA_PopulateContact(pUa, pMsg);
            pDLLEntry = NULL;
            while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
                pContact = (tContactHFE*)pDLLEntry;
                pContact->capabilitiesBitmap = capabilitiesBitmap;
            }
        }


        if ((pT = (tUaTrans *)SIP_memPoolAlloc(eSIP_OBJECT_UA_TRANS) ) == NULL) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        pT->hasTried = FALSE;
        if (NULL == (pT->pMsg = SIP_copyMsg(pMsg))) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Message: Failed to cooy pMsg", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanTrans(pT);
            return (SIP_FAILED);
        }
        pT->pUa = pUa;
        if (UA_SendRequest(pUa, &pMsg->requestUri, pT, pMsg, 
                UAC_MessageNoDialog, NULL, &pT->hTransaction) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Message: Couldn't send MESSAGE method", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanTrans(pT);
            return (SIP_FAILED);
        }
        if (hTransaction) {
            *hTransaction = pT->hTransaction;
        }
    } /* end of pDialog */
    else {
        /* Then the MESSAGE is being sent within the context of a dialog */
        if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Message: Failed in dialog FSM hUa:%X hDialog:%X", 
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            return (SIP_FAILED);
        }
        /* 
         * Then the handle the user should use for the transaction 
         * should really be the dialog.
         */
        if (hTransaction) {
            *hTransaction = hDialog;
        }
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Info()===================
 *
 * This function is used to send an INFO request within the context 
 * of a dialog.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * hDialog = A handle to a dialog.  
 * 
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMsgBody = A pointer to data to include in the msg body
 *
 * msgBodyLen = The length in bytes of the data pointed to by pMsgBody
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      The INFO method was successfully sent
 *      SIP_FAILED:  Failed to sent the message.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *      SIP_NO_MEM:  Could not allocate memory needed to send the INFO request
 *
 ******************************************************************************
 */
vint UA_Info(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg   *pMsg;
    tUa          *pUa;
    
    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Info: -hUa:%X", (int)hUa, 0, 0);

    if (!hUa || !hDialog) {
        return (SIP_BADPARM);
    }

    pUa = (tUa*)hUa;
    
    /* create an INFO and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_INFO;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    
    /* Add a msg body if one exists */
    if (pMsgBody && pMsgBody->pBody) {
        if (SIP_OK != UA_SetMsgBody(pMsg, pMsgBody->pBody, pMsgBody->length)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* run it through the state machine */
    if (UASM_DialogClient(hDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Info: Failed in dialog FSM hUa:%X hDialog:%X", 
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Options()===================
 *
 * This function is used to send an OPTIONS request.  If successful this 
 * function returns a handle to the instance of an OPTION transaction.  
 * This enables a UA to send more than one OPTIONS request at any time both 
 * inside or outside the context of a dialog. If the transaction fails then 
 * the application will receive a eUA_OPTIONS_FAILED event.  If successful, 
 * the application will see a eUA_OPTIONS_COMPLETED event.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created
 *         via UA_Create().
 *  
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * hDialog = A handle to a dialog.  If NULL then the OPTIONS request will be 
 *           sent outside the context of a dialog.  Otherwise the stack will
 *           attempt to send the request within the context of a dialog.
 *
 * pTo   = A pointer to a string representing the remote target uri 
 *         "sip:mrandmaa@d2tech.com".  when sending to a proxy it could be
 *         "sip:d2tech.com".  If this is NULL it defaults to the proxy address.
 *
 * pFrom = A pointer to a NULL terminated string representing the AOR that
 *        is sending the request.  If NULL or if the string is empty 
 *        (i.e. pAor[0] == 0), then the first AOR in the 
 *        list of AOR's that were set during UA_Create() are used.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *        
 * RETURNS:
 *      SIP_FAILED:  Could not send an OPTIONS request method
 *      SIP_NO_MEM:  Could not allocate necessary memory from the heap to 
 *                    send the OPTIONS request.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) The pTo or pFrom URI strings could not be understood
 *      SIP_OK:      The OPTIONS request was successfully sent
 *
 ******************************************************************************
 */
vint UA_Options(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg    *pMsg;
    tSipDialog    *pDialog;
    tUa           *pUa;
    tUaTrans      *pT;
    tIPAddr        addr; 
    tContactHFE    *pContact;
    tDLListEntry   *pDLLEntry;

    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Options: -hUa:%X", (int)hUa, 0, 0);

    if (!hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;
    
    /* create a OPTIONS Request and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_OPTIONS;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);

    /* Include a msg body if one is specified */
    if (pMsgBody && pMsgBody->pBody) {
        if (SIP_OK != UA_SetMsgBody(pMsg, pMsgBody->pBody, pMsgBody->length)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
    }

    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* 
     * If Dialog is NULL then we will send the request 
     * outside the context of the dialog
     */
    if (pDialog == NULL) {
        /* The "To" header field is mandatory */
        if (!(UA_GetHeaderField(SIP_TO_HF_STR, pHdrFlds, numHdrFlds))) {
            /* Then there is no "To" header field and it's mandatory */
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* 
         * Set the from header field.  If one isn't specified then 
         * use the default AOR.
         */
        if (UA_SetFromField(pUa, 
                UA_GetHeaderField(SIP_FROM_HF_STR, pHdrFlds, numHdrFlds),
                NULL, &pMsg->From) == -1) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* Make a 'from' 'tag' */
        HF_GenerateTag(pMsg->From.szTag);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_FROM_HF);

        /* Set up the contact */
        UA_PopulateContact(pUa, pMsg);
    
        /* Add the capabilties */
        if (0 != capabilitiesBitmap) {
            pDLLEntry = NULL;
            while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
                pContact = (tContactHFE*)pDLLEntry;
                pContact->capabilitiesBitmap = capabilitiesBitmap;
            }
        }

        /* Set the content length */
        pMsg->ContentLength = 0;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);

        /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);
    
        /* copy the 'to' to the request uri */
        pMsg->requestUri = pMsg->To.uri;
        
        /* now populate the Cseq */
        pMsg->CSeq.method = eSIP_OPTIONS;
        pMsg->CSeq.seqNum = HF_GenerateSeqNum();
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
        
        /* now populate the callid */
        HF_GenerateCallId(NULL, pMsg->szCallId);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);

        /* Make a via */
        OSAL_memSet(addr.v6, 0, sizeof(addr.v6));
        addr.v4.ul = 0;
        if (SIP_OK != HF_MakeVia(&pMsg->ViaList, NULL, NULL, addr, OSAL_FALSE)) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
        /* load default and send it */
        SYSDB_HF_Load(&pMsg->x.ECPresenceMasks, pMsg);
        if ((pT = (tUaTrans *)SIP_memPoolAlloc(eSIP_OBJECT_UA_TRANS) ) == NULL) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        pT->hasTried = FALSE;
        if (NULL == (pT->pMsg = SIP_copyMsg(pMsg))) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Options: Failed to cooy pMsg", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanTrans(pT);
            return (SIP_FAILED);
        }
        pT->pUa = pUa;
        if (UA_SendRequest(pUa, &pMsg->requestUri, pT, pMsg, 
                UAC_OptionsNoDialog, NULL, &pT->hTransaction) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Options: Couldn't send MESSAGE method", 0, 0, 0);
            SIP_freeMsg(pMsg);
            UA_CleanTrans(pT);
            return (SIP_FAILED);
        }
        if (hTransaction) {
            *hTransaction = pT->hTransaction;
        }
    } /* end of pDialog */
    else {
        /* Then the OPTIONs is being sent within the context of a dialog */
        if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_Options: Failed in dialog FSM hUa:%X hDialog:%X", 
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            return (SIP_FAILED);
        }
        /* 
         * Then the handle the user should use for the transaction 
         * should really be the dialog.
         */
        if (hTransaction) {
            *hTransaction = hDialog;
        }
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_ModifyCall()===================
 *
 * This function is used to change media connection information and also the 
 * remote uri target of the remote dialog.  IN OTHER WORDS AND IN SIP TERMS
 * THIS IS HOW YOU DO A "Re-Invite" or "target refresh" as described in 
 * Section 12 and 13 of RFC 3261.  
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog = A handle to the dialog that was originated from a eUA_CALL_ATTEMPT
 *           event or returned from a UA_MakeCall() function call.
 *           
 *
 * pTargetRefresh = A pointer to a string representing the "target refresh" 
 *               uri. If NULL, then no target refresh value is populated in the 
 *               'Contact' header field, just like it says to do in RFC3261.
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * pMediaSess = A pointer to an object that contains information regarding 
 *              the media connection.  i.e. RTP IP addr\port pairs, codec 
 *              information, the duplexity of the connection and the 
 *              packetRate. 
 *              If this value is NULL then an copy of the previously known SDP
 *              (or session) data is sent.  Otherwise it will populate the SDP 
 *              payload with the new media connection information.
 *              See sip_ua.h for the definition of the tSession object.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 *
 * RETURNS:
 *      SIP_BADPARM: Function failed because of one of the following:
 *                    1) The hUa or hDialog parameter was invalid.
 *                    2) One of the header fields in the pHdrFlds array 
 *                       could not be understood.
 *                    3) The pTargetRefresh URI string could not be understood
 *                    4) The pMediaSess data could not be included (encoded) 
 *                       in the request.
 *      SIP_NO_MEM:  Could not allocate memory necessary to send the request.
 *      SIP_FAILED:  Could not send the Re-Invite.  The dialog may be in the 
 *                   wrong state. 
 *      SIP_OK:      Request was successfully sent.
 *
 ******************************************************************************
 */
vint UA_ModifyCall(
    tSipHandle     hUa, 
    tSipHandle     hDialog, 
    char          *pTargetRefresh,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tSession      *pMediaSess,
    tLocalIpConn  *pLocalConn)
{
    tSipDialog  *pDialog;
    tSipIntMsg  *pMsg;
    tUri         target;
    tContactHFE *pContact = NULL;
    tUa *pUa;

    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_ModifyCall: -hUa:%X hDialog:%X", (int)hUa, 
            (int)hDialog, 0);
        
    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }
    pDialog = (tSipDialog*)hDialog;
    pUa = (tUa*)hUa;

    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->msgType = eSIP_REQUEST;
    pMsg->method = eSIP_INVITE;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    
    /* if pTargetRefresh is not null try to decode it */
    if (pTargetRefresh) {
        HF_CleanUri(&target);
        if (DEC_Uri(pTargetRefresh, (uint16)OSAL_strlen(pTargetRefresh), 
                &target) != SIP_OK) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* then place it in the contact header field */
        if (NULL == (pContact = (tContactHFE *)SIP_memPoolAlloc(eSIP_OBJECT_CONTACT_HF))) {
            SIP_freeMsg(pMsg);
            return (SIP_NO_MEM);
        }
        DLLIST_InitEntry(&pContact->dll);
        pContact->uri = target;
        DLLIST_Enqueue(&pMsg->ContactList, &pContact->dll);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTACT_HF);
    }

    /* rtp connection info */
    pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
    if (!pMsg->pSessDescr) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_ModifyCall: Could not make SDP offer -hUa:%X hDialog:%X",
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    /* set the content type */
    pMsg->ContentType = eCONTENT_TYPE_SDP;
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* Save a copy of the INVITE message */
    if (pDialog->pReInviteUpdate) {
        SIP_freeMsg(pDialog->pReInviteUpdate);
    }
    if (NULL == (pDialog->pReInviteUpdate = SIP_copyMsg(pMsg))) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_MakeCall: Failed to cooy pMsg", 0, 0, 0);
    }

    /* send it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_ModifyCall: Failed in FSM -hUa:%X hDialog:%X", 
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}


/* 
 *****************************************************************************
 * ================UA_Respond()===================
 *
 * This function is used to inject responses into the SIP stack.  It's used in
 * server-side application to return responses to transactions.
 * For, example let's say an INVITE request is received and the application 
 * receives a eUA_CALL_ATTEMPT event.  But, the user has left his office and 
 * wishes to temporarily forward calls.  Then the application would use 
 * UA_Respond() to return a response number "302" (Moved Temporarily)
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog = A handle to the dialog that was originated from a eUA_CALL_ATTEMPT
 *           event.
 *           
 * responseCode = The response code you would like to return.
 *                For example '180' would be "Ringing".
 *
 * sendReliably = if '1' or TRUE, AND the response code is a provisional 
 *                response, then send the provisional response reliably
 *                as specified in RFC3262. 
 *
 * alterTone    = if '1' or TRUE,  AND the response code is a Ringing
 *                response, it is alter tone for Communication Waiting
 *                as specified in 3GPP TS 24.615.
 *
 * pReasonPhrase = This is the string included at the top of the response 
 *                 that describes the type of responseCode.
 *                 IF NULL, then the system will use a default string for the 
 *                 response code specified in responseCode.  For example,
 *                 If responseCode was '180' then the string would be 
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Date: 12:12:12 12/12/2005"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array. 
 *
 * pMediaSess = A pointer to a tSession object that specifies 
 *              SDP (media session info).  If NULL, then no SDP info is
 *              included in the response, otherwise it will be included.   
 *
 * RETURNS:
 *   SIP_OK:            Response was successfully sent.
 *   SIP_FAILED:        Could not pass the response through the stack.  
 *                       This is probably due to a problem with the current 
 *                       state of the dialog.
 *   SIP_BADPARM:       Function failed because of one of the following:
 *                       1) The hUa or hDialog parameter was invalid.
 *                       2) One of the header fields in the pHdrFlds array 
 *                          could not be understood.
 *                      3) The pReasonPhrase string was too long
 *                      4) The pMediaSess data could not be included (encoded) 
 *                         in the response.
 *   SIP_NO_MEM:         Could not allocate memory needed to send the response
 *   SIP_NOT_SUPPORTED:  The SIP stack can't identify the response code
 ******************************************************************************
 */
vint UA_Respond(
    tSipHandle    hUa, 
    tSipHandle    hDialog, 
    uint32        responseCode,
    vint          sendReliably,
    vint          alterTone,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    uint32        capabilitiesBitmap)
{
    tSipIntMsg  *pMsg;
    tSipDialog  *pDialog = (tSipDialog*)hDialog;
    tSipMsgCodes intCode;
    tUa          *pUa;
    tContactHFE  *pContact;
    tDLListEntry *pDLLEntry;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_Respond: hDialog:%X", 
        (int)hDialog, 0, 0);
        
    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }

    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pUa = (tUa*)hUa;
    pMsg->msgType = eSIP_RESPONSE;
    /* set the method in the CSeq */
    pMsg->CSeq.method = eSIP_INVITE;
    
    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);

    intCode = MSGCODE_GetInt(responseCode);
    if (intCode == eSIP_RSP_LAST_RESPONSE_CODE) {
        SIP_freeMsg(pMsg);
        return (SIP_NOT_SUPPORTED);
    }
    MSGCODE_Create(pMsg, NULL, intCode);

    if (pReasonPhrase) {
        if (OSAL_strlen(pReasonPhrase) >= SIP_MAX_REASON_PHRASE_STR_LEN) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        SIP_copyStringToSipText(pReasonPhrase, &pMsg->pReasonPhrase);
    }

    /* Set up the contacts if there is no contact provided by the response. */
    if (TRUE == DLLIST_IsEmpty(&pMsg->ContactList)) {
        UA_PopulateContact(pUa, pMsg);
    }
    /* Add the capabilitiesBitmap */
    if (0 != capabilitiesBitmap) {
        pDLLEntry = NULL;
        while (DLLIST_GetNext(&pMsg->ContactList, &pDLLEntry)) {
            pContact = (tContactHFE*)pDLLEntry;
            pContact->capabilitiesBitmap = capabilitiesBitmap;
        }
    }

    if (sendReliably && MSGCODE_ISPROV(intCode)) {
        /* then send this provisional response reliably (RFC3262) */
        
        /* Enable 'Require' HF.  If the user didn't specify this in
         * the pHdrFlds parameter then the default in sysdb will 
         * be used 
         */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_REQUIRE_HF);

        /* Setting the RSeq to 1 here will tell the 
         * dialog layer that we want to use reliable 
         * provisional responses.
         */
        pMsg->RSeq = 1;
    }

    if (alterTone && (intCode == eSIP_RSP_RINGING)) {
        /* set alter-info filed to indicat CW (3GPP TS.615)
         *
         * Enable Alter-Info field. the Alter-Info only used for CW tone so far.
         */
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALERT_INFO_HF);
    }
    
    if (pMediaSess) {
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_1, 
                "UA_ModifyCall: Could not make SDP offer -hUa:%X hDialog:%X",
                (int)hUa, (int)hDialog, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* set the content type */
        pMsg->ContentType = eCONTENT_TYPE_SDP;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
    }
        
    /* pass it through the state machine */
    if (UASM_DialogServer(pDialog, pMsg, pDialog->hTrans) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, "UA_Respond: Failed FSM -hDialog:%X", 
                (int)hDialog, 0, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Prack()===================
 *
 * This function is used to send a PRACK request within the context 
 * of an !!!EARLY!!! dialog.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * hDialog = A handle to a dialog.  
 * 
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Required: 100rel"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.  
 *
 * pMediaSess = A pointer to a tSession object that specifies 
 *              SDP (media session info).  RFC3262 says that PRACK may be 
 *              used to send another SDP 'offer' or an answer to an offer
 *              that was in a reliable provisional response  But, typically 
 *              this should be NULL.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      The PRACK method was successfully sent
 *      SIP_FAILED:  Failed to sent the message.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                     1) The hUa or hDialog parameter was invalid.
 *                     2) One of the header fields in the pHdrFlds array 
 *                        could not be understood.
 *                     3) The pMediaSess data could not be included (encoded) 
 *                        in the response.
 *      SIP_NO_MEM:  Could not allocate memory needed to send the PRACK request
 *
 ******************************************************************************
 */
vint UA_Prack(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg       *pMsg;
    tUa              *pUa;
    tSipDialog       *pDialog;
    
    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Prack: -hUa:%X", (int)hUa, 0, 0);

    if (!hUa || !hDialog) {
        return (SIP_BADPARM);
    }

    pUa = (tUa*)hUa;
    pDialog = (tSipDialog*)hDialog;
    
    /* create a PRACK and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_PRACK;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }
    
    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);

    if (pMediaSess) {
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_3, 
            "UA_Prack: FAILED could no add Session (SDP) data", 0, 0, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* set the content type */
        pMsg->ContentType = eCONTENT_TYPE_SDP;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
    } /* end of pMediaSess */
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* run it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Prack: Failed in dialog FSM hUa:%X hDialog:%X", 
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_Ack()===================
 *
 * This function is used to send a ACK request within the context 
 * of an dialog.
 *
 * hUa   = A handle to the UA agent that was returned when the UA was created 
 *        via UA_Create().
 *
 * hDialog = A handle to a dialog.  
 * 
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Required: 100rel"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.  
 *
 * pMediaSess = A pointer to a tSession object that specifies 
 *              SDP (media session info).  RFC3262 says that PRACK may be 
 *              used to send another SDP 'offer' or an answer to an offer
 *              that was in a reliable provisional response  But, typically 
 *              this should be NULL.
 *
 * pLocalConn = A pointer to a tLocalIpConn object that specifies which IP
 *              stack interface to use for when sending this request.
 *              If NULL then the last IP Stack interface used to send the 
 *              previous request will be used.
 *
 * RETURNS:
 *      SIP_OK:      The PRACK method was successfully sent
 *      SIP_FAILED:  Failed to sent the message.
 *      SIP_BADPARM: Function failed because of one of the following:
 *                     1) The hUa or hDialog parameter was invalid.
 *                     2) One of the header fields in the pHdrFlds array 
 *                        could not be understood.
 *                     3) The pMediaSess data could not be included (encoded) 
 *                        in the response.
 *      SIP_NO_MEM:  Could not allocate memory needed to send the PRACK request
 *
 ******************************************************************************
 */
vint UA_Ack(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn)
{
    tSipIntMsg       *pMsg;
    tUa              *pUa;
    tSipDialog       *pDialog;
    
    SIP_DebugLog(SIP_DB_UA_LVL_2, "UA_Ack: -hUa:%X", (int)hUa, 0, 0);

    if (!hUa || !hDialog) {
        return (SIP_BADPARM);
    }

    pUa = (tUa*)hUa;
    pDialog = (tSipDialog*)hDialog;
    
    /* create a ACK and populate */
    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->method = eSIP_ACK;
    pMsg->msgType = eSIP_REQUEST;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);

    if (pMediaSess) {
        /* rtp connection info */
        pMsg->pSessDescr = SESSION_Encode(&pDialog->session, pMediaSess);
        if (!pMsg->pSessDescr) {
            SIP_DebugLog(SIP_DB_UA_LVL_3, 
            "UA_Prack: FAILED could no add Session (SDP) data", 0, 0, 0);
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        /* set the content type */
        pMsg->ContentType = eCONTENT_TYPE_SDP;
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_CONTENT_DISP_HF);
    } /* end of pMediaSess */
    
    /* save the local connection settings if they exist */
    if (pLocalConn) {
        pUa->lclConn = *pLocalConn;
    }

    /* run it through the state machine */
    if (UASM_DialogClient(pDialog, pMsg, NULL) != SIP_OK) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_Ack: Failed in dialog FSM hUa:%X hDialog:%X", 
            (int)hUa, (int)hDialog, 0);
        SIP_freeMsg(pMsg);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/* 
 *****************************************************************************
 * ================UA_SubscribeResp()===================
 *
 * This function is used to return (send) a response to a SUBSCRIBE request.
 *
 * hUa  = A handle to a UA that was returned when the UA was created via the 
 *        UA_Create() function.
 *
 * hDialog = A handle to the dialog that the subscription is from.
 *           
 * responseCode = The response code you would like to return.
 *                for example '202' would be "accepted".
 *
 * pReasonPhrase = This is the string included at the top of the response 
 *                 that describes the type of responseCode
 *                 IF NULL, then the system will use a default string for the 
 *                 response code specified in responseCode.  For example,
 *                 If responseCode was 202 then the string would be "Accepted"
 *
 * pHdrFlds = An array of pointers to strings. Where each pointer points to 
 *            a NULL terminated header field string.
 *            For Example...
 *            pHdrFlds[0] = "Subscription-State: active"
 *            pHdrFlds[1] = "Event: message-summary"
 *            pHdrFlds[2] = "Expires: 180"
 *
 * numHdrFlds = The number of valid pointers in the 'pHdrFlds' array.   
 *
 * RETURNS:
 *      SIP_BADPARM:       Function failed because of one of the following:
 *                          1) The hUa or hDialog parameter was invalid.
 *                          2) One of the header fields in the pHdrFlds array 
 *                             could not be understood.
 *                          3) The pReasonPhrase string was too long.
 *      SIP_NO_MEM:        Could allocate enough memory to generate the 
 *                          response to the SUBSCRIBE request.
 *      SIP_NOT_SUPPORTED: The responseCode is unknown.
 *      SIP_FAILED:        The response could not be sent.  This may be due 
 *                          to the Dialog being in an invalid state.
 *      SIP_OK:            Response was successfully sent.
 *      SIP_DONE:          Response was successfully sent and the dialog has
 *                         no further subscriptions so the dialog is destroyed.
 ******************************************************************************
 */
vint UA_SubscribeResp(
    tSipHandle        hUa, 
    tSipHandle        hDialog, 
    uint32            responseCode,
    char             *pReasonPhrase,
    char             *pHdrFlds[],
    vint              numHdrFlds)
{
    tSipIntMsg   *pMsg;
    tSipDialog   *pDialog;
    tSipMsgCodes  intCode;
    vint          status;
    
    SIP_DebugLog(SIP_DB_UA_LVL_3, "UA_SubscribeResp: hDialog:%X", 
        (int)hDialog, 0, 0);
        
    if (!hDialog || !hUa) {
        return (SIP_BADPARM);
    }

    pDialog = (tSipDialog*)hDialog;

    if (NULL == (pMsg = SIP_allocMsg())) {
        return (SIP_NO_MEM);
    }

    pMsg->msgType = eSIP_RESPONSE;
    /* set the method in the CSeq */
    pMsg->CSeq.method = eSIP_SUBSCRIBE;

    /* populate any header fields from the function caller */
    if (UA_LoadHeaderFields(pMsg, pHdrFlds, numHdrFlds) != SIP_OK) {
        SIP_freeMsg(pMsg);
        return (SIP_BADPARM);
    }

    /* DECLARE ANY DEFAULT PRESENCE BITS HERE IF YOU SO DESIRE */
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_USER_AGENT_HF);
    HF_SetPresence(&pMsg->x.ECPresenceMasks, eSIP_ALLOW_EVENTS_HF);

    intCode = MSGCODE_GetInt(responseCode);
    if (intCode == eSIP_RSP_LAST_RESPONSE_CODE) {
        SIP_freeMsg(pMsg);
        return (SIP_NOT_SUPPORTED);
    }
    MSGCODE_Create(pMsg, NULL, intCode);

    if (pReasonPhrase) {
        if (OSAL_strlen(pReasonPhrase) >= SIP_MAX_REASON_PHRASE_STR_LEN) {
            SIP_freeMsg(pMsg);
            return (SIP_BADPARM);
        }
        SIP_copyStringToSipText(pReasonPhrase, &pMsg->pReasonPhrase);
    }
    
    /* pass it through the state machine */
    status = UASM_DialogServer(pDialog, pMsg, pDialog->hTrans);
    if (SIP_DONE == status) {
        /* 
         * This happens when we decide to reject the subscription request and 
         * there are no more vents within a dialog so it's must be destroyed 
         */
        DIALOG_Destroy(pDialog);
    }
    if (SIP_OK != status) {
        SIP_DebugLog(SIP_DB_UA_LVL_1, 
            "UA_SubscribeResp: Failed FSM -hDialog:%X", 
            (int)hDialog, 0, 0);
        SIP_freeMsg(pMsg);
    }
    return (status);
}


/* 
 *****************************************************************************
 * ================UA_Destroy()===================
 *
 * This function will destroy a UA that was originally created via 
 * UA_Create().  Any memory that used to create and service the UIA will 
 * be freed.  Note , that if a UA was registered to proxies then 
 * UA_UnRegister() should be called before calling UA_Destroy().
 *
 * RETURNS:
 *    SIP_OK: UA was successfully destroyed
 *    SIP_BADPARM: The hUa parameter was invalid
 *
 ******************************************************************************
 */
vint UA_Destroy(
    tSipHandle hUa)
{
    tUa *pUa;
    
    if (!hUa) {
        return (SIP_BADPARM);
    }
    pUa = (tUa*)hUa;
    /* free up the UA object */
    UA_Dealloc(pUa);
    return (SIP_OK);
}
