/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#include "sip_sip.h"

#include "sip_port.h"
#include "sip_dbase_sys.h"
#include "sip_timers.h"
#include "sip_port.h"
#include "sip_list.h"
#include "sip_xact.h"
#include "sip_mem_pool.h"
#include "../xport/sip_xport.h"

/*
 * Declairation of sip timer variables and assign default values.
 */
static uint32 _TRANS_timerT1 = TRANS_TIMER_T1;
static uint32 _TRANS_timerT2 = TRANS_TIMER_T2;
static uint32 _TRANS_timerT4 = TRANS_TIMER_T4;
static uint32 _TRANS_timerA = TRANS_TIMER_A;
#if defined(PROVIDER_CMCC)
static uint32 _TRANS_timerB = TRANS_TIMER_TCALL;
#else
static uint32 _TRANS_timerB = TRANS_TIMER_B;
#endif
static uint32 _TRANS_timerC = TRANS_TIMER_C;
static uint32 _TRANS_timerD = TRANS_TIMER_D;
static uint32 _TRANS_timerE = TRANS_TIMER_E;
static uint32 _TRANS_timerF = TRANS_TIMER_F;
static uint32 _TRANS_timerH = TRANS_TIMER_H;
static uint32 _TRANS_timerI = TRANS_TIMER_I;
static uint32 _TRANS_timerJ = TRANS_TIMER_J;
static uint32 _TRANS_timerK = TRANS_TIMER_K;

/*
 *Unused: 
 *static uint32 _TRANS_timerG = TRANS_TIMER_G;
 */

/*
 * Define the Client's ACTIVE transaction lists (per Method)
 */
static tDLList _ClientHashTable[eSIP_LAST_METHOD] = {
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Invite", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Ack", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Cancel", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Options", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Register", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Bye", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Notify", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Refer", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Message", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Subscribe", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Info", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Prack", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Update", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Publish", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ClientXact Error", 0, },
};

/*
 * Define the Server's ACTIVE transaction lists (per Method)
 */
static tDLList _ServerHashTable[eSIP_LAST_METHOD] = {
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Invite", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Ack", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Cancel", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Options", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Register", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Bye", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Notify", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Refer", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Message", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Subscribe", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Info", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Prack", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Update", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Publish", 0, },
    { NULL, NULL, 0, SIP_PORT_LOCK_INIT, "ServerXact Error", 0, },
};

/* 
 * This is a look up table that translates the internal
 * transaction events to external event type sent out to the
 * TU (UA is this case)
 */
static const uint32 _Int2ExtEventTable[eTRANS_LAST_EVENT] = 
{
    eTU_FAILED,  /* This is just empty space because eTRANS_EVENT_RESPONSE is '1' */
    eTU_RESPONSE,/* eTRANS_EVENT_RESPONSE */
    eTU_REQUEST, /* eTRANS_EVENT_REQUEST */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_A */
    eTU_FAILED,  /* eTRANS_EVENT_TIMEOUT_B */
    eTU_FAILED,  /* eTRANS_EVENT_TIMEOUT_C */ 
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_D */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_E */
    eTU_FAILED,  /* eTRANS_EVENT_TIMEOUT_F */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_K */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_G */
    eTU_FAILED,  /* eTRANS_EVENT_TIMEOUT_H */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_I */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_J */
    eTU_FAILED,  /* Never goes to TU eTRANS_EVENT_TIMEOUT_FIREWALL */
    eTU_FAILED,  /* eTRANS_EVENT_TRANSPORT_ERROR */
    eTU_DEAD     /* eTRANS_EVENT_ISDEAD */
};

#if (SIP_DEBUG_LOG)

static const char* _TRANS_TuEvent[eTU_LAST_EVENT] = {
    "TU_FAILED",
    "TU_REQUEST",
    "TU_RESPONSE",
    "TU_DEAD",
};

static const char* _TRANS_State[eTRANS_LAST_STATE] = {
    "NO STATE",
    "INIT",
    "CALLING",
    "TRYING",
    "PROCEEDING",
    "CONFIRMED",
    "COMPLETED",
    "TERMINATED",
};

static const char* _TRANS_Event[eTRANS_LAST_EVENT] = {
    "NONE",
    "RESPONSE",
    "REQUEST",
    "TIMEOUT_A",
    "TIMEOUT_B",
    "TIMEOUT_C",
    "TIMEOUT_D",
    "TIMEOUT_E",
    "TIMEOUT_F",
    "TIMEOUT_K",
    "TIMEOUT_G", /* G and greater are for the server side */
    "TIMEOUT_H",
    "TIMEOUT_I",
    "TIMEOUT_J",
    "TIMEOUT_FIREWALL",
    "TRANSPORT_ERROR",
    "IS_DEAD",
};

#endif /* if (SIP_DEBUG_LOG) */

/* local prototypes */
static void _SetOrgInfo(
    tTrans       *pTrans,
    tSipIntMsg   *pMsg);

static vint _InviteClientSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg);

static vint _MethodClientSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg);

static vint _InviteServerSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg);

static vint _MethodServerSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg);

static vint _ClientSendAck(
    tTrans       *pTrans,
    tToHF        *pTo);

static vint _FindTransClient(
    tSipMethod     method,
    char          *pIdentifier,
    tTrans       **ppTrans);

static tTrans* _ServerSearch(
    tSipIntMsg   *pMsg, 
    tSipMethod    method);

static vint _FindTransServerRfc2543(
    tTrans       *pTrans, 
    tViaHFE      *pVia, 
    tSipIntMsg   *pMsg);

static vint _FindTransServerRfc3261(
    tTrans       *pTrans, 
    tViaHFE      *pVia,
    uint32        cseqNum);

static vint _ServerSendTrying(
    tTrans       *pTrans,
    tSipIntMsg   *pMsg);

static vint _FindDestroy(tTrans *pTargetTrans);

static vint _Destroy(
    tTrans       *pTrans, 
    vint          notifyApp);

static vint _SendLastResponse(tTrans *pTrans);

static void _TRANS_Cb(
    tTransEvent   event, 
    tSipIntMsg   *pMsg, 
    tTrans       *pTrans);

/* timer handler call back functions (called indirectly via SIPTIMER_ISR()) */
static void _transTimer(
    tSipHandle    hOwner, 
    void         *pArg);

static void _TRANS_TimerStart(
    tTransTimer  *pTimer, 
    tTransEvent   event, 
    uint32        time);

static vint _ClientSendMethod(tTrans *pTrans);

static void _TRANS_SetState(
    tTrans       *pTrans,
    tTransState   state);

static vint _TRANS_FailOver(
    tTrans *pTrans, 
    vint    isInvite);

void TRANS_Init(void) {
    int i;

    for (i = 0; i < eSIP_LAST_METHOD; i++) {
        SIP_MutexInit(&_ClientHashTable[i].lock);
        SIP_MutexInit(&_ServerHashTable[i].lock);
    }
}

/* 
 *****************************************************************************
 * ================TRANS_ClientCreate()===================
 *
 * This function creates a new Client transaction object
 * and initilaizes it.  It also initilaizes the Client Trans 
 * statemachine. 
 *
 * If !NULL is returned, then the caller MUST NOT
 * do anything with pMsg. Meaning, the caller must not 
 * free (deallocate) the original message (pMsg).
 * that created the transaction.
 * If the function fails (returns NULL then pMsg is the 
 * resposibility of the caller.
 *
 * RETURNS:
 *      handle to the transaction object
 *      NULL Could not create the transaction
 *
 ******************************************************************************
 */
tSipHandle TRANS_ClientCreate(
    tSipHandle              hTransport,
    tpfTransportCB          pfTransport,
    tTransportType          transType,
    tSipIntMsg             *pMsg,
    tpfAppCB                pfApp,
    tSipHandle              hOwner,
    tSipHandle              hContext)
{
    vint          status;
    tTrans       *pTrans;
    tDLListEntry *pEntry;
    tDLList      *xactList;

    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Creating New Client Transaction", 0, 0, 0);

    if(NULL == (pTrans = (tTrans *)SIP_memPoolAlloc(eSIP_OBJECT_TRANS))) {
        goto error_exit;
    }

    /* Generate a unique id for this transaction. */
    pTrans->id = SIP_randInt(1, SIP_MAX_POSTIVE_INT);

    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Allocated New Client Transaction (%X)", (int)pTrans, 0, 0);

    pTrans->isServer = FALSE;

    /* get the topmost via */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        if (((tViaHFE *)pEntry)->szBranch[0] != '\0') {
            OSAL_strcpy(pTrans->identifier, ((tViaHFE *)pEntry)->szBranch);    
        }
    }
    else {
        goto error_exit;
    }

    /* init owners to timers */
    pTrans->timer1.pOwner = pTrans;
    pTrans->timer2.pOwner = pTrans;
    pTrans->timer3.pOwner = pTrans;

    /* save the owner of the transaction */
    pTrans->hOwner = hOwner;
    pTrans->hContext = hContext;
    pTrans->hTransport = hTransport;

    pTrans->method = pMsg->method;

    /* set up the network address */
    pTrans->transType = transType;
    pTrans->pfApp = pfApp;
    pTrans->pfTransport = pfTransport;

    /* start timer F even for reliable transports */
    if ((pTrans->timer2.hTimer = SIPTIMER_Create(hContext)) == NULL) {
        /* clean house and return */
        goto error_exit;
    }
    
    /* set up the state machine */
    _TRANS_SetState(pTrans, eTRANS_INIT);
    if (transType == eTransportUdp) {
        if ((pTrans->timer1.hTimer = SIPTIMER_Create(hContext)) == NULL) {
            /* clean house and return */
            goto error_exit;
        }
    }

    if (pTrans->method == eSIP_INVITE) {
        pTrans->pfState = _InviteClientSM;
        _SetOrgInfo(pTrans, pMsg);
        if (transType == eTransportUdp) {
            /* set up timer values */
            pTrans->t1 = _TRANS_timerT1;
        }
        /* Timer3 is used for both reliable and datagram transports for INVITE only !! */
        if ((pTrans->timer3.hTimer = SIPTIMER_Create(hContext)) == NULL) {
            goto error_exit;
        }
    }
    else {
        pTrans->pfState = _MethodClientSM;
        if (transType == eTransportUdp) {
            /* set up timer values */
            pTrans->t1 = _TRANS_timerT1;
        }
    }

    /* 
     * Enqueue the transaction onto the Client's (per method) ACTIVE list
     */
    DLLIST_InitEntry(&pTrans->dll);
    xactList = &_ClientHashTable[pTrans->method];
    SIP_Lock(xactList->lock);
    status = DLLIST_Enqueue(xactList, &pTrans->dll);
    SIP_Unlock(xactList->lock);
    if (status != SIP_OK) {
        goto error_exit;
    }
    goto done;

error_exit:
    SIP_DebugLog(SIP_DB_TRANS_LVL_1, "ERROR Creating New Client Transaction", 0, 0, 0);
    if (pTrans) {
        _Destroy(pTrans, FALSE);
        pTrans = NULL;
    }

done:
    return (tSipHandle)pTrans;
}

/*
 *****************************************************************************
 * ================TRANS_ServerCreate()===================
 *
 * This function creates a new Server transaction object
 * and initializes it.  It also initilaizes the Server Trans 
 * statemachine. 
 * If this function returns a value of other than NULL, then the pMsg 
 * should be freed (deallocated) by the caller.
 *
 * If !NULL is returned then the pMsg was successfully passed 
 * back to the TU via the pfApp routine.
 *
 * RETURNS:
 *      handle to the transactiopn object
 *      NULL Could not create the transaction
 ******************************************************************************
 */
tSipHandle TRANS_ServerCreate(
    tSipHandle              hTransport,
    tpfTransportCB          pfTransport,
    tTransportType          transType,
    tSipIntMsg             *pMsg,
    tpfAppCB                pfApp,
    tSipHandle              hOwner,
    tSipHandle              hContext)

{
    tTrans       *pTrans;
    tDLList      *xactList;
    vint status = SIP_OK;

    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Creating New Server Transaction: pMsg=%X", (int)pMsg, 0, 0);

    if (NULL == (pTrans = (tTrans *)SIP_memPoolAlloc(eSIP_OBJECT_TRANS))) {
        goto error_exit;
    }

    /* Generate a unique id for this transaction. */
    pTrans->id = SIP_randInt(1, SIP_MAX_POSTIVE_INT);

    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Allocated New Server Transaction (%X)", (int)pTrans, 0, 0);

    pTrans->isServer = TRUE;
    
    /* init owners to timers */
    pTrans->timer1.pOwner = pTrans;
    pTrans->timer2.pOwner = pTrans;
    pTrans->timer3.pOwner = pTrans;

    pTrans->hOwner = hOwner;
    pTrans->hContext = hContext;

    /* set up the network address */
    pTrans->transType   = transType;
    pTrans->hTransport = hTransport;
     
    /* now save the following info from the original message */
    _SetOrgInfo(pTrans, pMsg);
    
    /* set the call back routine to handle event back to TU */
    pTrans->pfApp = pfApp;
    pTrans->pfTransport = pfTransport;
    
    /* Timer1 is used for both reliable and datagram transports */
    if ((pTrans->timer1.hTimer = SIPTIMER_Create(hContext)) == NULL) {
        goto error_exit;
    }
    
    _TRANS_SetState(pTrans, eTRANS_INIT);
    if (pMsg->method == eSIP_INVITE) {
        pTrans->pfState = _InviteServerSM;
        if ((pTrans->timer2.hTimer = SIPTIMER_Create(hContext)) == NULL) {
            goto error_exit;
        }
        pTrans->t1 = _TRANS_timerT1;
    }
    else {
         pTrans->pfState = _MethodServerSM;
    }

    /* 
     * Enqueue the transaction onto the Server's (per method) ACTIVE list
     */
    DLLIST_InitEntry(&pTrans->dll);
    xactList = &_ServerHashTable[pMsg->method];
    SIP_Lock(xactList->lock);
    status = DLLIST_Enqueue(xactList, &pTrans->dll);
    SIP_Unlock(xactList->lock);
    if (status != SIP_OK) {
        goto error_exit;
    }
    goto done;

error_exit:
    SIP_DebugLog(SIP_DB_TRANS_LVL_1, "ERROR Creating New Server Transaction", 0, 0, 0);
    if (pTrans) {
        _Destroy(pTrans, FALSE);
        pTrans = NULL;
    }

done:
    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Created New Server Transaction: pTrans=%X", (int)pTrans, 0, 0);
    return (tSipHandle)pTrans;
}

static void _Transporvintor(tTrans *pTrans)
{
    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
    _TRANS_Cb(eTRANS_EVENT_TRANSPORT_ERROR, NULL, pTrans);
    _FindDestroy(pTrans);  
    return;
}

/*
 *****************************************************************************
 * ================_InviteServerSM()===================
 *
 * This is the state machine handler for Inbound "Invite" message 
 * transactions.  The state machine is described in section 17 of RFC3261
 *
 * If SIP_OK is returned then the pMsg was successfully passed back to the TU
 * via the pfApp routine.
 *
 *
 * RETURNS:
 *  SIP_OK       -Function was succesful and passed the pMsg back to TU.
 *  SIP_FREE_MEM -Function was succesful but pMsg needs to be freed by caller.
 ******************************************************************************
 */
static vint _InviteServerSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg)
{
    vint status = SIP_OK; 

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_InviteServerSM: hTransaction:%X state:%s, event:%s", 
            (int)pTrans, (int)_TRANS_State[pTrans->state], (int)_TRANS_Event[event]);

    if (pTrans->isServer == FALSE) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_InviteServerSM: ERROR client transaction in serverFSM hTransaction:%X", 
            (int)pTrans, 0, 0);
        return (SIP_FREE_MEM);
    }


    switch (pTrans->state)
    {
    case eTRANS_INIT:
        if (event == eTRANS_EVENT_REQUEST) {
            _TRANS_SetState(pTrans, eTRANS_PROCEEDING);
            if (_ServerSendTrying(pTrans, pMsg) != SIP_OK) {
                _Transporvintor(pTrans);
                status = SIP_FREE_MEM;
            }
            else {
                _TRANS_Cb(eTRANS_EVENT_REQUEST, pMsg, pTrans);
            }
        }
    break;
    case eTRANS_PROCEEDING:
        if (event == eTRANS_EVENT_REQUEST) {
            if (_SendLastResponse(pTrans) != SIP_OK) {
                _Transporvintor(pTrans);
            }
            status = SIP_FREE_MEM;
        }
        else if (event == eTRANS_EVENT_RESPONSE) {
            /* this must be from the TU */

            /* update the tag if the tu has one */
            if (pMsg->To.szTag[0] != 0)
                OSAL_strcpy(pTrans->to.szTag, pMsg->To.szTag);
            
            if (MSGCODE_ISPROV(pMsg->code)) {
                SIP_freeMsg(pTrans->pMsg);
                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
            }
            else if (MSGCODE_ISSUCCESS(pMsg->code)) { /* it's a 2xx */
                SIP_freeMsg(pTrans->pMsg);
                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
                else {
                    if (pTrans->transType == eTransportUdp) {
                        /*
                         * For 200OK we will violate the spec and enter the
                         * "Confirmed" state for a duration so we can catch an
                         * duplicate INVITE. This is to fix a bug in the spec.
                         */
                        _TRANS_SetState(pTrans, eTRANS_CONFIRMED);
                        /* start timer I */
                        _TRANS_TimerStart(&pTrans->timer1,
                                eTRANS_EVENT_TIMEOUT_I,
                                TRANS_TIMER_INVITE_CATCH_BUG);
                    }
                    else {
                        _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                        _FindDestroy(pTrans);
                    }
                }
            }
            else { /* it's 3xx - 6xx */
                _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                SIP_freeMsg(pTrans->pMsg);
                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
                else {
                    if (pTrans->transType == eTransportUdp) {
                        /* Start timer G @ t1 */
                        _TRANS_TimerStart(&pTrans->timer1,
                                eTRANS_EVENT_TIMEOUT_G, pTrans->t1);
                    }
                    /* start timer H for ALL transports */
                    _TRANS_TimerStart(&pTrans->timer2, eTRANS_EVENT_TIMEOUT_H,
                            _TRANS_timerH);
                }
            }
        }
        else {
            status = SIP_FREE_MEM;
        }
        break;
    case eTRANS_COMPLETED:
        if (event == eTRANS_EVENT_TIMEOUT_G) {
            pTrans->t1 = pTrans->t1 * 2;
            if (pTrans->t1 > _TRANS_timerT2)
                pTrans->t1 = _TRANS_timerT2;
            
            if (_SendLastResponse(pTrans) != SIP_OK) {
                _Transporvintor(pTrans);
            }
            else {
                SIP_DebugLog(SIP_DB_TRANS_LVL_3, "Handling G timeout", 0, 0, 0);
                /* start timer G @ t1 */
                _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_G, pTrans->t1);
            }
        }
        else if (event == eTRANS_EVENT_TIMEOUT_H) {
            /* kill all, since NO ACK */
            _TRANS_SetState(pTrans, eTRANS_TERMINATED);
            SIPTIMER_Stop(pTrans->timer1.hTimer);
            /* let TU know about it */
            _TRANS_Cb(event, NULL, pTrans);
            _FindDestroy(pTrans);
        }
        else if (event == eTRANS_EVENT_REQUEST) {
            if (pMsg->method == eSIP_ACK) {
                _TRANS_SetState(pTrans, eTRANS_CONFIRMED);
                /* stop timer G */
                SIPTIMER_Stop(pTrans->timer1.hTimer);
                /* stop timer h */
                SIPTIMER_Stop(pTrans->timer2.hTimer);

                if (pTrans->transType == eTransportUdp) {
                    /* start timer I */
                    _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_I, _TRANS_timerI);
                }
                else {
                    /* terminate */
                    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                    _FindDestroy(pTrans);
                }
                /* this will dispose of the ACK, it should not be passed back see 17.2.1 */
                status = SIP_FREE_MEM;
            }
            else { /* must be duplicated request */
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
                status = SIP_FREE_MEM;
            }
        }
        else {
            status = SIP_FREE_MEM;
        }
        break;
    case eTRANS_CONFIRMED:
        if (event == eTRANS_EVENT_TIMEOUT_I) {
            /* kill all */
            _TRANS_SetState(pTrans, eTRANS_TERMINATED);
            _FindDestroy(pTrans);
        }
        else {
            status = SIP_FREE_MEM;
        }
        break;
    case eTRANS_TERMINATED:
        status = SIP_FREE_MEM;
        break;
    default:
        _TRANS_SetState(pTrans, eTRANS_TERMINATED);
        _FindDestroy(pTrans);
        break;
    }

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_InviteServerSM transaction (%X) done, status= %d, new state= %d", (int)pTrans, status, (int)pTrans->state);
    return status;
}

/*
 *****************************************************************************
 * ================ _MethodServerSM()===================
 *
 * This is the state machine handler for Inbound messages other than 'Invite' 
 * transactions.  The state machine is described in section 17 of RFC3261
 *
 * If SIP_OK is returned then the pMsg was successfully passed back to the TU
 * via the pfApp routine.
 *
 *
 * RETURNS:
 *  SIP_OK       -Function was succesful and passed the pMsg back to TU.
 *  SIP_FREE_MEM -Function was succesful but pMsg needs to be freed by caller.
 ******************************************************************************
 */
static vint _MethodServerSM(
    tTransEvent     event,
    tTrans          *pTrans,
    tSipIntMsg      *pMsg)
{
    vint status = SIP_OK;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_MethodServerSM: hTransaction:%X state:%s, event:%s", 
            (int)pTrans, (int)_TRANS_State[pTrans->state], (int)_TRANS_Event[event]);

    if (pTrans->isServer == FALSE) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_InviteServerSM: ERROR client transaction in serverFSM hTransaction:%X", 
            (int)pTrans, 0, 0);
        return (SIP_FREE_MEM);
    }

    switch (pTrans->state)
    {
    case eTRANS_INIT:
        if (event == eTRANS_EVENT_REQUEST) {
            _TRANS_SetState(pTrans, eTRANS_TRYING);
            _TRANS_Cb(eTRANS_EVENT_REQUEST, pMsg, pTrans);
        }
        break;
    case eTRANS_TRYING:
        if (event == eTRANS_EVENT_REQUEST) {
            /* duplicate request then discard */
            status = SIP_FREE_MEM;
        }
        else if (event == eTRANS_EVENT_RESPONSE) {
            if (MSGCODE_ISPROV(pMsg->code))
            {
                _TRANS_SetState(pTrans, eTRANS_PROCEEDING);
                
                if (pTrans->pMsg) {
                    SIP_DebugLog(SIP_DB_TRANS_LVL_1,
                        "_MethodServerSM: ERROR released msg that should have "
                        "already been released",
                        (int)pTrans->pMsg, 0, 0);
                    SIP_freeMsg(pTrans->pMsg);
                }

                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
            }
            else if (MSGCODE_ISFINAL(pMsg->code)) {
                _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                    
                if (pTrans->pMsg) {
                    SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_MethodServerSM: ERROR "
                        "released msg that should have already been released", 
                        (int)pTrans->pMsg, 0, 0);
                    SIP_freeMsg(pTrans->pMsg);
                }

                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                } 
                else {
                    /* start timers */
                    if (pTrans->transType == eTransportUdp) {
                        /* start timer I */
                        _TRANS_TimerStart(&pTrans->timer1,
                        eTRANS_EVENT_TIMEOUT_J, _TRANS_timerJ);
                    }
                    else {
                        /* terminate */
                        _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                        _FindDestroy(pTrans);
                    }
                }
            }
        }
        else {
            status = SIP_FREE_MEM;
        }
        break;
    case eTRANS_PROCEEDING:
        if (event == eTRANS_EVENT_REQUEST) {
            /* it's a request retransmission */
            if (_SendLastResponse(pTrans) != SIP_OK) {
                _Transporvintor(pTrans);
            }
            status = SIP_FREE_MEM;
        }
        else if (event == eTRANS_EVENT_RESPONSE) {
            if (MSGCODE_ISPROV(pMsg->code)) {
                SIP_freeMsg(pTrans->pMsg);
                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
            }
            else if(MSGCODE_ISFINAL(pMsg->code)) {
                _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                SIP_freeMsg(pTrans->pMsg);
                pTrans->pMsg = pMsg;
                if (_SendLastResponse(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
                else {
                    /* start timers */
                    if (pTrans->transType == eTransportUdp) {
                        /* start timer I */
                        _TRANS_TimerStart(&pTrans->timer1,
                        eTRANS_EVENT_TIMEOUT_J, _TRANS_timerJ);
                    }
                    else {
                        /* terminate */
                        _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                        _FindDestroy(pTrans);
                    }
                }
            }
        }
        else {
            status = SIP_FREE_MEM;
        }
        break;
    case eTRANS_COMPLETED:
        if (event == eTRANS_EVENT_REQUEST) {
            /* duplicate message */
            if (_SendLastResponse(pTrans) != SIP_OK) {
                _Transporvintor(pTrans);
            }
            status = SIP_FREE_MEM;
        }
        else if (event == eTRANS_EVENT_TIMEOUT_J) {
            /* kill it */
            _TRANS_SetState(pTrans, eTRANS_TERMINATED);
            _FindDestroy(pTrans);
        }
        else { /* must be a "eTRANS_EVENT_RESPONSE" */
            /* discard */
            status = SIP_FREE_MEM;
        }
        break;
    case eTRANS_TERMINATED:
        status = SIP_FREE_MEM;
        break;
    default:
         _TRANS_SetState(pTrans, eTRANS_TERMINATED);
         _FindDestroy(pTrans);
         break;
    }

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_MethodServerSM transaction (%X) done, "
            "status= %d", (int)pTrans, status, 0);
    return status;
}

/*
 *****************************************************************************
 * ================ _InviteClientSM()===================
 *
 * This is the state machine handler for Outbound 'Invite' message 
 * transactions.  The state machine is described in section 17 of RFC3261
 *
 * If SIP_OK is returned then the pMsg was successfully passed back to 
 * The Transport layer via the pfApp routine.
 *
 *
 * RETURNS:
 *  SIP_OK       -Function was succesful and passed the pMsg back to TU.
 *  SIP_FREE_MEM -Function was succesful but pMsg needs to be freed by caller.
 ******************************************************************************
 */
static vint _InviteClientSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg)
{
    vint status = SIP_OK;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_InviteClientSM: hTransaction:%X state:%s, event:%s", 
            (int)pTrans, (int)_TRANS_State[pTrans->state], (int)_TRANS_Event[event]);

    if (pTrans->isServer == TRUE) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_InviteServerSM: ERROR server transaction in client FSM hTransaction:%X", 
            (int)pTrans, 0, 0);
        return (SIP_FREE_MEM);
    }
    
    switch (pTrans->state)
    {
    case eTRANS_INIT:
        /* It's a new client transaction */
        if (event == eTRANS_EVENT_REQUEST) {
            _TRANS_SetState(pTrans, eTRANS_CALLING);
            pTrans->pMsg = pMsg;
            status = _ClientSendMethod(pTrans);
            if (status != SIP_OK) {
                pTrans->pMsg = NULL;
                _Transporvintor(pTrans);
                status = SIP_FREE_MEM;
            }
            else {
                /* Start timer b even for reliable transports 
                 * NOTE, timer B is described in section 17.1.1 of RFC3261.  Please note that 
                 * timer B is also used to accomidate a transaction where an original invite 
                 * never receives a final response after receiving a provisional response 
                 * as described in section 9.1 (CANCELLING) in RFC3261. 
                 */
                _TRANS_TimerStart(&pTrans->timer2, eTRANS_EVENT_TIMEOUT_B,
                        _TRANS_timerB);
                
                if (pTrans->transType == eTransportUdp) {
                    _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_A,
                            pTrans->t1);
                }            
            }
        }
        else {
            status = SIP_FREE_MEM;
        }
        break;
        case eTRANS_CALLING:
            if (event == eTRANS_EVENT_TIMEOUT_A) {
                /* restart the timer */
                pTrans->t1 = (pTrans->t1 * 2);
                _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_A,
                        pTrans->t1);
                /* pMsg should be the org method msg */
                if (_ClientSendMethod(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
            }
            else if (event == eTRANS_EVENT_TIMEOUT_B) {
                if (_TRANS_FailOver(pTrans, 1) != SIP_OK) {
                    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                    _TRANS_Cb(event, NULL, pTrans);
                    /* destroy the trans object */
                    _FindDestroy(pTrans);    
                }
            }
            else if (event == eTRANS_EVENT_RESPONSE) {
                if (pTrans->transType == eTransportUdp) {
                    SIPTIMER_Stop(pTrans->timer1.hTimer);
                }

                if (MSGCODE_ISPROV(pMsg->code)) {
                    /* Stop timer B */
                    SIPTIMER_Stop(pTrans->timer2.hTimer);
                    _TRANS_SetState(pTrans, eTRANS_PROCEEDING);
#ifdef TRANSACTION_WITH_FIREWALL
                    if (pTrans->transType == eTransportUdp) {
                        /* start a new timer to resend the INVITE. 
                         * This should only be used if you need to send 
                         * INVITE's to kkep a firewall open 
                         */
                        _TRANS_TimerStart(&pTrans->timer2,
                                eTRANS_EVENT_TIMEOUT_FIREWALL, 
                                TRANS_FIREWALL_REFRESH);
                    }
#endif

                    _TRANS_Cb(event, pMsg, pTrans);
                }
                else if (!MSGCODE_ISSUCCESS(pMsg->code)) {
                    /* stop timer B even for reliable transports */
                    SIPTIMER_Stop(pTrans->timer2.hTimer);
                    /* transition the 300 - 699 reposnses to 'Completed' */
                    _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                    /* generate the ACK */
                    if (_ClientSendAck(pTrans, &pMsg->To) != SIP_OK) {
                        _Transporvintor(pTrans);
                        status = SIP_FREE_MEM;
                    }
                    else {
                        _TRANS_Cb(event, pMsg, pTrans);
                        if (pTrans->transType == eTransportUdp) {
                            /* start timer D should be T2 or 32000ms */
                            _TRANS_TimerStart(&pTrans->timer1,
                                    eTRANS_EVENT_TIMEOUT_D, _TRANS_timerD);
                        }
                        else {
                            _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                            /* destroy the trans object */
                            _FindDestroy(pTrans);
                        }
                    }
                }
                else { /* it must be a 200 */
                    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                    _TRANS_Cb(event, pMsg, pTrans);
                    /* destroy the trans object */
                    _FindDestroy(pTrans);
                }
            }
            else {
                status = SIP_FREE_MEM;
            }
        break;
        case eTRANS_PROCEEDING:
            if (event == eTRANS_EVENT_RESPONSE) {
                if (MSGCODE_ISPROV(pMsg->code))
                     _TRANS_Cb(event, pMsg, pTrans);
                else if (!MSGCODE_ISSUCCESS(pMsg->code)) {
#ifdef TRANSACTION_WITH_FIREWALL
                    if (pTrans->transType == eTransportUdp) {
                        /* then stop the timer that resends the INVITE */
                        SIPTIMER_Stop(pTrans->timer2.hTimer);
                    }
#endif
                    /* stop timer C even for reliable transports */
                    SIPTIMER_Stop(pTrans->timer3.hTimer);
                    /* transition the 300 - 699 reposnses to 'Completed' */
                    _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                    if (_ClientSendAck(pTrans, &pMsg->To) != SIP_OK) {
                        _Transporvintor(pTrans);
                        status = SIP_FREE_MEM;
                    }
                    else {
                        _TRANS_Cb(event, pMsg, pTrans);
                        if (pTrans->transType == eTransportUdp) {
                             /* start timer D should be T2  or 32000ms */
                             _TRANS_TimerStart(&pTrans->timer1,
                                    eTRANS_EVENT_TIMEOUT_D, _TRANS_timerD);
                        }
                        else {
                             _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                             /* destroy the trans object */
                             _FindDestroy(pTrans);
                        }
                    }
                }
                else { /* it must be a 200 */
                    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                    _TRANS_Cb(event, pMsg, pTrans);
                    /* destroy the trans object */
                    _FindDestroy(pTrans);
                }
            }
#ifdef TRANSACTION_WITH_FIREWALL
            else if (event == eTRANS_EVENT_TIMEOUT_FIREWALL) {
                /* restart the timer */
                _TRANS_TimerStart(&pTrans->timer2, eTRANS_EVENT_TIMEOUT_FIREWALL, 
                        TRANS_FIREWALL_REFRESH);
                /* then resend the last INVITE request to keep the firewall open */
                if (_ClientSendMethod(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
            }
#endif
            else if (event == eTRANS_EVENT_TIMEOUT_C) {
                _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                _TRANS_Cb(event, NULL, pTrans);
                /* destroy the trans object */
                _FindDestroy(pTrans);    
            }
            else {
                status = SIP_FREE_MEM;
            }
        break;
        case eTRANS_COMPLETED:
            if (event == eTRANS_EVENT_TIMEOUT_D) {
                _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                /* destroy the trans object */
                _FindDestroy(pTrans);
            }
            else if (event == eTRANS_EVENT_RESPONSE) {
                if (MSGCODE_ISFINAL(pMsg->code)) {
                    /* resend the ack */
                    if (_ClientSendAck(pTrans, &pMsg->To) != SIP_OK) {
                        _Transporvintor(pTrans);
                    }
                }
                status = SIP_FREE_MEM;
            }
        break;
        case eTRANS_TERMINATED:
            status = SIP_FREE_MEM;
        break;
        default:
            _TRANS_SetState(pTrans, eTRANS_TERMINATED);
            _FindDestroy(pTrans);
        break;
    }

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_InviteClientSM transaction (%X) done, status= %d", (int)pTrans, status, 0);
    return status;
}

/*
 *****************************************************************************
 * ================ _MethodClientSM()===================
 *
 * This is the state machine handler for Outbound message other than 'Invite' 
 * transactions.  The state machine is described in section 17 of RFC3261
 *
 * If SIP_OK is returned then the pMsg was successfully passed back to 
 * The Transport layer via the pfApp routine.
 *
 *
 * RETURNS:
 *  SIP_OK       -Function was succesful and passed the pMsg back to TU.
 *  SIP_FREE_MEM -Function was succesful but pMsg needs to be freed by caller.
 ******************************************************************************
 */
static vint _MethodClientSM(
    tTransEvent   event,
    tTrans       *pTrans,
    tSipIntMsg   *pMsg)
{
    vint status = SIP_OK;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_MethodClientSM: hTransaction:%X state:%s, event:%s", 
            (int)pTrans, (int)_TRANS_State[pTrans->state], (int)_TRANS_Event[event]);

    if (pTrans->isServer == TRUE) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_InviteServerSM: ERROR server transaction in client FSM hTransaction:%X", 
            (int)pTrans, 0, 0);
        return (SIP_FREE_MEM);
    }

    switch (pTrans->state)
    {
        case eTRANS_INIT:
            /* It's a new client transaction */
            if (event == eTRANS_EVENT_REQUEST) {
                _TRANS_SetState(pTrans, eTRANS_TRYING);
                pTrans->pMsg = pMsg;
                status = _ClientSendMethod(pTrans);
                if (status != SIP_OK) {
                    pTrans->pMsg = NULL;
                    /*
                     * Leaving this code here for historical reason.
                     * See bug 6004 for more details about it's history.
                     * Failed non-invite transactions should fail and clean
                     * up immediately and NOT call the callback.
                     */
                    //_Transporvintor(pTrans);
                    //status = SIP_FREE_MEM;
                    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                    _FindDestroy(pTrans);
                    status = SIP_FAILED;
                }
                else {
                    /* start timer F even for reliable transports */
                    _TRANS_TimerStart(&pTrans->timer2, eTRANS_EVENT_TIMEOUT_F,
                            _TRANS_timerF);
        
                    if (pTrans->transType == eTransportUdp) {
                        _TRANS_TimerStart(&pTrans->timer1,
                                eTRANS_EVENT_TIMEOUT_E, pTrans->t1);
                    }            
                }
            }
            else {
                status = SIP_FREE_MEM;
            }
        break;
        case eTRANS_TRYING:
            if (event == eTRANS_EVENT_TIMEOUT_E) {
                /* restart the timer */
                pTrans->t1 = (pTrans->t1 * 2);
                if (pTrans->t1 > _TRANS_timerT2) {
                    pTrans->t1 = _TRANS_timerT2;
                }
                
                /* pMsg should be the org method msg */
                if (_ClientSendMethod(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
                else {
                    /* interval should be t1 */
                    _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_E,
                            pTrans->t1);
                }
            }
            else if (event == eTRANS_EVENT_TIMEOUT_F) {
                if (_TRANS_FailOver(pTrans, 0) != SIP_OK) {
                    _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                    _TRANS_Cb(event, NULL, pTrans);
                    /* destroy the trans object */
                    _FindDestroy(pTrans);
                }
            }
            else if (event == eTRANS_EVENT_RESPONSE) {
                /* stop the timer */
                if (pTrans->transType == eTransportUdp) {
                    SIPTIMER_Stop(pTrans->timer1.hTimer);
                }

                if (MSGCODE_ISPROV(pMsg->code)) {
                    _TRANS_SetState(pTrans, eTRANS_PROCEEDING);
                    _TRANS_Cb(event, pMsg, pTrans);
                    /* start timer at 4 seconds */
                    if (pTrans->transType == eTransportUdp) {
                        _TRANS_TimerStart(&pTrans->timer1,
                                eTRANS_EVENT_TIMEOUT_E, _TRANS_timerE);
                    }
                }
                else if (MSGCODE_ISFINAL(pMsg->code)) {
                    /* stop timer F for both transports */
                    SIPTIMER_Stop(pTrans->timer2.hTimer);
                    _TRANS_Cb(event, pMsg, pTrans);
                    if (pTrans->transType == eTransportUdp) {
                        /* interval should be t4 5 seconds*/
                        _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                        _TRANS_TimerStart(&pTrans->timer1,
                                eTRANS_EVENT_TIMEOUT_K, _TRANS_timerK);
                    }
                    else {
                        _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                        /* destroy the trans object */
                        _FindDestroy(pTrans);
                    }
                }
                else status = SIP_FREE_MEM;
            }
            else status = SIP_FREE_MEM;
        break;
        case eTRANS_PROCEEDING:
            if (event == eTRANS_EVENT_TIMEOUT_E) {
                if (_ClientSendMethod(pTrans) != SIP_OK) {
                    _Transporvintor(pTrans);
                }
                else {
                    /* interval should be T1 4 seconds */
                    _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_E,
                            _TRANS_timerT2);
                }
            }
            else if (event == eTRANS_EVENT_TIMEOUT_F) {
                 _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                 _TRANS_Cb(event, NULL, pTrans);
                 /* destroy the trans object */
                 _FindDestroy(pTrans);
            }
            else if (event == eTRANS_EVENT_RESPONSE) {
                if (pTrans->transType == eTransportUdp) {
                    SIPTIMER_Stop(pTrans->timer1.hTimer);
                }
                
                if (MSGCODE_ISFINAL(pMsg->code)) {
                    _TRANS_SetState(pTrans, eTRANS_COMPLETED);
                    _TRANS_Cb(event, pMsg, pTrans);

                    if (pTrans->transType == eTransportUdp) {
                        /* stop timer F */
                        SIPTIMER_Stop(pTrans->timer2.hTimer);
                 
                        /* interval should be t4 5 seconds*/
                        _TRANS_TimerStart(&pTrans->timer1,
                                eTRANS_EVENT_TIMEOUT_K, _TRANS_timerK);
                    }
                    else {
                        _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                        /* destroy the trans object */
                        _FindDestroy(pTrans);
                    }
                }
                else status = SIP_FREE_MEM;
            }
            else status = SIP_FREE_MEM;
        break;
        case eTRANS_COMPLETED:
            if (event == eTRANS_EVENT_TIMEOUT_K) {
                _TRANS_SetState(pTrans, eTRANS_TERMINATED);
                /* destroy the trans object */
                _FindDestroy(pTrans);
            }
            else if (event == eTRANS_EVENT_RESPONSE) {
                /* This hsould not even be here 
                if (MSGCODE_ISFINAL(pMsg->code)) {
                    if (_ClientSendAck(pTrans, &pMsg->To) != SIP_OK) {
                        _Transporvintor(pTrans);
                    }
                }*/
                status = SIP_FREE_MEM;
            }
        break;
        case eTRANS_TERMINATED:
            status = SIP_FREE_MEM;
        break;
        default:
            _TRANS_SetState(pTrans, eTRANS_TERMINATED);
            _FindDestroy(pTrans);
        break;
    }

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_MethodClientSM transaction (%X) done, status= %d", (int)pTrans, status, 0);
    return status;
}

/*
 *****************************************************************************
 * ================ _SendLastResponse()===================
 *
 * This function is used by the Server and will make a copy of the last 
 * response message sent, and pass it the transport layer for transmission.
 *  IT's used to send reponses and retransmissions of responses.
 *
 * If SIP_OK is returned then the pMsg was successfully passed back to 
 * The Transport layer via the pfApp routine.
 *
 *
 * RETURNS:
 *  SIP_OK       -Function was succesful and passed the pMsg back to 
 *                Transport layer.
 *  SIP_FAILED   -FAiled because there was no reponse to send or because 
 *                transport layer failed.
 ******************************************************************************
 */
static vint _SendLastResponse(tTrans *pTrans)
{
    /* If there are any problems then 
     * send event back indication of a transport error
     */
    vint status = SIP_FAILED;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_SendLastResponse pTrans=%X", (int)pTrans, 0, 0);

    /* just retransmit the last response */
    if (pTrans->pMsg) {
        /* If there is no via then use the one from the server trasnport object */
        if (!(HF_PresenceExists(&pTrans->pMsg->x.ECPresenceMasks, eSIP_VIA_HF))) {
            /* get the top most via */
            tViaHFE *pVia = (tViaHFE *)SIP_memPoolAlloc(eSIP_OBJECT_VIA_HF);

            SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_SendLastResponse Error no Via is populated pTrans=%X, pMsg=%X", 
                    (int)pTrans, (int)pTrans->pMsg, 0);
            if (pVia) {
                *pVia = pTrans->via;
                DLLIST_InitEntry(&pVia->dll);
                DLLIST_Enqueue(&pTrans->pMsg->ViaList, &pVia->dll);
                HF_SetPresence(&pTrans->pMsg->x.ECPresenceMasks, eSIP_VIA_HF);
            }
        }
        /*
         * Responses being sent for server side transactions must use
         * the CSeq associated with this transaction.
         */
        pTrans->pMsg->CSeq = pTrans->cseq;
        HF_SetPresence(&pTrans->pMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
        status = pTrans->pfTransport(pTrans->hTransport, pTrans->pMsg);
    }

    if (status != SIP_OK) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_SendLastResponse Failed pTrans=%X, pMsg=%X", (int)pTrans, (int)pTrans->pMsg, 0);
    }
    return status;
}

/*
 *****************************************************************************
 * ================ _ServerSendTrying()===================
 *
 * This function is used by the Server and sends a "100 trying response. 
 *
 *
 * RETURNS:
 *  SIP_NO_MEM   -Could not allocate memory for the message.
 *  SIP_FAILED   -General failure with 'copy' reutine or possibly linked list 
 *                Issues, or a transport error.
 *  SIP_OK       -Function was succesful.
 ******************************************************************************
 */
static vint _ServerSendTrying(tTrans *pTrans, tSipIntMsg *pMsg)
{
    tSipIntMsg *pResp;
    vint status;
    
    if (NULL == (pResp = SIP_allocMsg()) ) 
        return (SIP_NO_MEM);

    pResp->msgType = eSIP_RESPONSE;
    pResp->code = eSIP_RSP_TRYING;
    pResp->method = pMsg->method;

    /* copy the via list, the whole thing and in same order */
    status = DLLIST_Copy(&pMsg->ViaList, &pResp->ViaList, eDLLIST_VIA_HF); 
    if (status != SIP_OK) {
        SIP_freeMsg(pResp);
        return status;
    }
    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_VIA_HF);
   
    pResp->From = pMsg->From;
    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_FROM_HF);

    pResp->CSeq = pTrans->cseq;
    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_CSEQ_HF);

    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);

    OSAL_strcpy(pResp->szCallId, pMsg->szCallId);
    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_CALL_ID_HF);

    /* copy the to but don't include the 'tag' */
    pResp->To = pMsg->To;
    /* pResp->To.szTag[0] = '\0'; */
    HF_SetPresence(&pResp->x.ECPresenceMasks, eSIP_TO_HF);
    /* set up the presence bits for encoding */
    
    if (pTrans->pMsg) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_ServerSendTrying: ERROR released msg that should have already been released", 
            (int)pTrans->pMsg, 0, 0);
        SIP_freeMsg(pTrans->pMsg);
    }

    pTrans->pMsg = pResp;
    return _SendLastResponse(pTrans);
}


/*
 *****************************************************************************
 * ================ TRANS_ClientSearch()===================
 *
 * This function looks for a matching transaction to the pMsg 
 *
 *
 * RETURNS:
 *  NULL:      Could not find or process the transaction.  If this is 
 *                 Returned then the caller needs to handle the pMsg.
 *  tSipHandle:A trasnaction matching the pMsg was found and the handle 
 *             to the trasnaction is returned.
 ******************************************************************************
 */
tSipHandle TRANS_ClientSearch(tSipIntMsg *pMsg)
{
    tTrans       *pTrans;
    tDLListEntry *pEntry;
   
    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_ClientSearch: ", 0, 0, 0);

    /* find the transaction! */

    /* get the branch from the topmost via */
    pEntry = NULL;
    if (!(DLLIST_GetNext(&pMsg->ViaList, &pEntry))) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_ClientSearch: Error - ViaList is empty", 0, 0, 0);
        return (NULL);
    }
    if (_FindTransClient(pMsg->CSeq.method, ((tViaHFE *)pEntry)->szBranch, &pTrans) != SIP_OK) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_ClientSearch: Error - couldn't find transaction", 0, 0, 0);
        return (NULL); /* could not find the trans */
    }
    
    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_ClientRecvFromTransport: found transaction %X", (int)pTrans, 0, 0);
    return ((tSipHandle)pTrans);
}

/*
 *****************************************************************************
 * ================TRANS_ClientReq()===================
 *
 * RETURNS:
 *  SIP_NOT_FOUND -no matching transaction was found, caller is responsible 
 *                 for handling pMsg.
 *  SIP_OK        -Function was succesful and has assumed responsibility of 
 *                 pMsg.  In other words, don't use pMsg if SIP_OK is returned.
 ******************************************************************************
 */
vint TRANS_ClientReq(
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    tTrans *pTrans;
    int status;
    if (hTransaction) {
        pTrans = (tTrans*)hTransaction;
        SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_ClientReq: -hTrans:%x", (int)hTransaction, 0, 0);
        status = pTrans->pfState(eTRANS_EVENT_REQUEST, pTrans, pMsg);
        if (status == SIP_FREE_MEM) {
            SIP_freeMsg(pMsg);
            status = SIP_OK;
        }
    }
    else {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_ClientReq: Failed transaction handle was NULL", 0, 0, 0);
        return (SIP_FAILED);
    }
    
    return (status);
}          

/*
 *****************************************************************************
 * ================ TRANS_ClientResp()===================
 *
 * This function is the entry point into the client FSM for responses 
 * from the network.
 *
 *
 * RETURNS:
 *  SIP_FAILED   -Could not find or process the transaction.  If this is 
 *                 Returned then the caller needs to handle the pMsg.
 *  SIP_OK       -Function was succesful.  Caller should not attempt to handle 
 *                the pMsg.  In other words, pMsg was passed to another module.
 ******************************************************************************
 */
vint TRANS_ClientResp(
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    tTrans *pTrans;
   
    if (hTransaction) {
        pTrans = (tTrans*)hTransaction;
        SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_ClientResp: -hTrans:%x", (int)hTransaction, 0, 0);
        if (pTrans->pfState(eTRANS_EVENT_RESPONSE, pTrans, pMsg) == SIP_FREE_MEM) {
            SIP_freeMsg(pMsg);
        }
    }
    else {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_ClientResp: Failed transaction handle was NULL", 0, 0, 0);
        return (SIP_FAILED);
    }
    
    return (SIP_OK);
}              

/*
 *****************************************************************************
 * ================ TRANS_ServerResp()===================
 *
 * This function, handles message traffic from the TU to the Server 
 * transaction state machine.
 *
 *
 * RETURNS:
 *  SIP_FAILED   -Could not find or process the transaction.  If this is 
 *                 returned then the caller needs to handle the pMsg.
 *  SIP_OK       -Function was succesful.  Caller should not attempt to handle 
 *                the pMsg.  In other words, pMsg was passed to another module.
 ******************************************************************************
 */
vint TRANS_ServerResp(
   tSipIntMsg  *pMsg,
   tSipHandle   hTransaction)
{
    tTrans *pTrans;

    if (hTransaction) {
        pTrans = (tTrans*)hTransaction;  
        SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_ServerResp: -hTrans:%x", (int)hTransaction, 0, 0);
        if ((pTrans->pfState(eTRANS_EVENT_RESPONSE, pTrans, pMsg)) == SIP_FREE_MEM) {
            SIP_freeMsg(pMsg);
        }
    }
    else {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_ServerResp: Failed transaction handle was NULL", 0, 0, 0);
        return (SIP_FAILED);
    }
    return (SIP_OK);
}

/*
 *****************************************************************************
 * ================TRANS_ServerReq()===================
 *
 * RETURNS:
 *  SIP_NOT_FOUND -no matching transaction was found, caller is responsible 
 *                 for handling pMsg.
 *  SIP_OK        -Function was succesful and has assumed responsibility of 
 *                 pMsg.  In other words, don't use pMsg if SIP_OK is returned.
 ******************************************************************************
 */
vint TRANS_ServerReq(
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction)
{
    tTrans *pTrans;
    
    if (hTransaction) {
        pTrans = (tTrans*)hTransaction;
        SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_ServerReq: -hTrans:%x", (int)hTransaction, 0, 0);
        if ((pTrans->pfState(eTRANS_EVENT_REQUEST, pTrans, pMsg)) == SIP_FREE_MEM) {
            SIP_freeMsg(pMsg);
        }
    }
    else {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_ServerReq: Failed transaction handle was NULL", 0, 0, 0);
        return (SIP_FAILED);
    }
    
    return (SIP_OK);
}              

/*
 *****************************************************************************
 * ================_ServerSearch()===================
 *
 * This function will first try to match the request to existing 
 * server transactions to see if there is a match.
 *
 * NOTE: that this function handles matching messages for RFC3261 and RFC2543
 *
 * RETURNS:
 *  SIP_NOT_FOUND -no matching transaction was found, caller is responsible 
 *                 for handling pMsg.
 *  SIP_OK        -The transaction was successful found
 ******************************************************************************
 */
static tTrans* _ServerSearch(tSipIntMsg *pMsg, tSipMethod method)
{
    /* first try to find it */
    vint isRFC3261 = FALSE;
    tDLList      *xactList;
    tDLListEntry *pEntry;
    tViaHFE      *pVia;
    tTrans       *pTrans;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_ServerSearch: Incoming msg", 0, 0, 0);
            
    pEntry = NULL;
    pVia = NULL;
    
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry) == 0) {
        /* via's are mandatory in SIP */
        return (NULL);
    }
    else {
        pVia = (tViaHFE *)pEntry;
        /* check if rfc3261 see section 17.2.3 of RFC3261 for matching rules */
        isRFC3261 = HF_MagicCookieExists(pVia->szBranch);
    }
    
    /* now try to find the transaction */
    pEntry = NULL;
    xactList = &_ServerHashTable[method];
    SIP_Lock(xactList->lock);
    while (DLLIST_GetNext(xactList, &pEntry)) {
        pTrans = (tTrans *)pEntry;
        if (isRFC3261) {
            if (_FindTransServerRfc3261(pTrans, pVia, pMsg->CSeq.seqNum) == SIP_OK) {
                SIP_Unlock(xactList->lock);
                return (pTrans);
            }
        }
        else { 
            SIP_DebugLog(SIP_DB_TRANS_LVL_2, "_ServerSearch: using RFC 2543 rules to find transaction (pTrans=%X)", (int)pTrans, 0, 0);
            if (_FindTransServerRfc2543(pTrans, pVia, pMsg) == SIP_OK) {
                SIP_Unlock(xactList->lock);
                return (pTrans);
            }
        }
    }
    SIP_Unlock(xactList->lock);
    return (NULL);
}

/*
 *****************************************************************************
 * ================TRANS_ServerSearch()===================
 *
 * This function handles a message form the transport layer.
 * This function will first try to match the request to existing 
 * server transactions to see if there is a match.
 *
 * RETURNS:
 *  tSipHandle* : A handle to the transaction found
 *  NULL        : No transaction was found
 ******************************************************************************
 */
tSipHandle TRANS_ServerSearch(tSipIntMsg *pMsg)
{
    /* first try to find it */
    tSipMethod method = pMsg->CSeq.method;
    
    if (method == eSIP_ACK)
        method = eSIP_INVITE;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_SearchServer: Incoming msg", 0, 0, 0);
            
    return _ServerSearch(pMsg, method);
}

        
tSipHandle TRANS_SearchForServerInvite(
    tSipIntMsg *pCancelMsg)
{
    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "TRANS_SearchForServerInvite: ", 0, 0, 0);
        
    /* make sure that this msg is a cancel */
    if (pCancelMsg->method != eSIP_CANCEL) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_SearchForServerInvite: Failed, the pMsg is NOT a CANCEL hMsg:%X", 
                (int)pCancelMsg, 0, 0);
        return (NULL);
    }
    
    return _ServerSearch(pCancelMsg, eSIP_INVITE);
}

/*
 *****************************************************************************
 * ================_FindTransServerRfc2543()===================
 *
 * This routine matches the pMsg to a transaction as per the criteria
 * described in RFC 2543.  This is for backwards compatibility.
 *
 *
 * RETURNS:
 *  SIP_NOT_FOUND -no matching transaction was found.
 *  SIP_OK        -A match was found
 ******************************************************************************
 */
static vint _FindTransServerRfc2543(tTrans *pTrans, tViaHFE *pVia, tSipIntMsg *pMsg)
{
    /* the order of comparison is optimized here for best results in the field */
    if (HF_IsEqualVia(pVia, &pTrans->via) && 
            HF_IsEqualUri(&pMsg->requestUri, &pTrans->requestUri) &&
            OSAL_strcmp(pMsg->From.szTag, pTrans->from.szTag) == 0 &&
            OSAL_strcmp(pMsg->szCallId, pTrans->szCallId) == 0 &&
            pMsg->CSeq.seqNum == pTrans->cseq.seqNum ) {

        if (pMsg->method != eSIP_INVITE) {
            if (OSAL_strcmp(pMsg->To.szTag, pTrans->to.szTag) == 0) {
                SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Found Previous Transaction via 2543", 0, 0, 0);
                return (SIP_OK);
            }
        }
        else {
            return (SIP_OK);
        }
    }
    return (SIP_NOT_FOUND);
}

/*
 *****************************************************************************
 * ================_FindTransServerRfc3261()===================
 *
 * This routine matches the pMsg to a transaction as per the criteria
 * described in RFC 3261.  This is for backwards compatibility.
 *
 *
 * RETURNS:
 *  SIP_NOT_FOUND -no matching transaction was found.
 *  SIP_OK        -A match was found
 ******************************************************************************
 */
static vint _FindTransServerRfc3261(
    tTrans       *pTrans, 
    tViaHFE      *pVia,
    uint32           cseqNum)
{
    
    if (cseqNum == pTrans->cseq.seqNum) {
        if (!OSAL_strcmp(pVia->szBranch, pTrans->via.szBranch)) {
            if (pVia->uri.host.addressType == pTrans->via.uri.host.addressType) {
                if (pVia->uri.host.addressType == eNwAddrDomainName) {
                    if (OSAL_strcasecmp(pVia->uri.host.x.domainName, pTrans->via.uri.host.x.domainName) != 0) {
                        return (SIP_NOT_FOUND);
                    }
                }
                else if (eNwAddrIPv6 == pVia->uri.host.addressType) {
                    if (OSAL_memCmp(pVia->uri.host.x.ip.v6, pTrans->via.uri.host.x.ip.v6,
                            sizeof(pVia->uri.host.x.ip.v6))) {
                        return (SIP_NOT_FOUND);
                    }
                }
                else {
                    if (pVia->uri.host.x.ip.v4.ul != pTrans->via.uri.host.x.ip.v4.ul)
                        return (SIP_NOT_FOUND);
                }
                return (SIP_OK);
            }
        }
    }
    return (SIP_NOT_FOUND);
}

/*
 *****************************************************************************
 * ================_ClientSendAck()===================
 *
 * This routine creates and sends an ACK message for an
 * 'Invite' Client transaction.
 *
 *
 * RETURNS:
 *  SIP_NO_MEM    -Could not allocate memory.
 *  SIP_OK        -ACK was successfully sent to the Transport layer.
 *  SIP_FAILED    -Did not successfully send to transport layer.
 ******************************************************************************
 */
static vint _ClientSendAck(
    tTrans      *pTrans,
    tToHF       *pTo)
{
    vint status;
    /* note: that the pMsg here is a response to the original req */
    tSipIntMsg *pAckMsg;
    tViaHFE *pVia;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "_ClientSendAck: ", 0, 0, 0);
    
    if (NULL == pTrans->pAckMsg) {
        if (NULL == (pAckMsg = SIP_allocMsg())) return (SIP_NO_MEM);

        /* these come from the original message */
        pAckMsg->msgType = eSIP_REQUEST;
        pAckMsg->method = eSIP_ACK;
        pAckMsg->requestUri = pTrans->requestUri;
    
        pAckMsg->From = pTrans->from;
        if (NULL == (pVia = (tViaHFE *)SIP_memPoolAlloc(eSIP_OBJECT_VIA_HF))) {
            SIP_freeMsg(pAckMsg);
            return (SIP_NO_MEM);
        }
        (*pVia) = pTrans->via;
        DLLIST_InitEntry(&pVia->dll);
        DLLIST_Enqueue(&pAckMsg->ViaList, &pVia->dll);
        OSAL_strcpy(pAckMsg->szCallId, pTrans->szCallId);
        pAckMsg->CSeq.seqNum = pTrans->cseq.seqNum;
        pAckMsg->CSeq.method = eSIP_ACK;
        /* these come from the response */
        pAckMsg->To = *pTo;

        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_FROM_HF);
        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_CALL_ID_HF);
        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_VIA_HF);
        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_CSEQ_HF);
        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_TO_HF);
    
        HF_CopyInsert(&pAckMsg->pHFList, eSIP_MAX_FORWARDS_HF, SYSDB_MAX_FORWARDS_DFLT, 0);
        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_MAX_FORWARDS_HF);

        pAckMsg->ContentLength = 0;
        HF_SetPresence(&pAckMsg->x.ECPresenceMasks, eSIP_CONTENT_LENGTH_HF);

        pTrans->pAckMsg = pAckMsg;
    }

    /* send to the transport layer */
    status = pTrans->pfTransport(pTrans->hTransport, pTrans->pAckMsg);
    return status;        
}

/*
 *****************************************************************************
 * ================_ClientSendMethod()===================
 *
 * This routine clones and and sends a Request message for an
 *
 *
 * RETURNS:
 *  SIP_OK        -pTrans->pMsg was succesfully sent.
 *  SIP_FAILED    -Could NOT send pTrans->pMsg.  There was an error from the 
 *                 underlying IP stack or the message could not be encoded.
 ******************************************************************************
 */
static vint _ClientSendMethod(tTrans *pTrans)
{
    vint status;
    if (pTrans->pMsg == NULL) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_ClientSendMethod:Trying to send non-existent pMsg hTransaction%X hMsg:%X", 
                (int)pTrans, (int)pTrans->pMsg, 0);
        return (SIP_FAILED);
    }
    status = SIP_OK;    
    while (status == SIP_OK) {
        /* send to transport layer */
        status = pTrans->pfTransport(pTrans->hTransport, pTrans->pMsg);
        if (status != SIP_OK) {
            /* Then try the next address */
            status = TRANSPORT_AdvanceRmtAddr(pTrans->hTransport);
            if (status != SIP_OK) {
                break;
            }
            else {
                /* then we will try a new address/port set.  So make a new via */
                HF_GenerateBranch(pTrans->identifier);
                HF_MakeViaBranch(&pTrans->pMsg->ViaList, pTrans->identifier);
                OSAL_strcpy(pTrans->via.szBranch, pTrans->identifier);
            }
        }
        else {
            break;
        }
    }
    return (status);
}

/*
 *****************************************************************************
 * ================_FindTransClient()===================
 *
 * This routine looks for a preexisting transaction for incoming messages
 *
 *
 * RETURNS:
 *  SIP_NOT_FOUND -Could not match message to Tranaction.
 *  SIP_OK        -Transaction was found.
 ******************************************************************************
 */
static vint _FindTransClient(
    tSipMethod     method,
    char          *pIdentifier,
    tTrans       **ppTrans)
{
    tDLListEntry *pEntry;
    tDLList *xactList = &_ClientHashTable[method];
    int identifierLen = OSAL_strlen(pIdentifier);
 
    pEntry = NULL;
    SIP_Lock(xactList->lock);
    while (DLLIST_GetNext(xactList, &pEntry)) {
        if (0 == OSAL_memCmp(((tTrans*)pEntry)->identifier, pIdentifier,
                identifierLen)) {
            *ppTrans = (tTrans*)pEntry;
            SIP_Unlock(xactList->lock);
            return (SIP_OK);
        }
    }
    SIP_Unlock(xactList->lock);
    *ppTrans = NULL;
    return (SIP_NOT_FOUND);
}

/*
 *****************************************************************************
 * ================_SetOrgInfo()===================
 *
 * Copies information needed through out the transaction from the 
 * original meesage that instantiated the transaction.
 *
 *
 * RETURNS: Nothing
 *  
 ******************************************************************************
 */
static void _SetOrgInfo(
    tTrans      *pTrans,
    tSipIntMsg  *pMsg)
{
    tDLListEntry *pEntry;

    pTrans->method = pMsg->method;
        
    /* set the call ID */
    OSAL_strcpy(pTrans->szCallId, pMsg->szCallId);
    /* set the From */
    pTrans->from = pMsg->From;
    /* set the request uri */
    pTrans->requestUri = pMsg->requestUri;
    /* set the sequence number from the CSeq field */
    pTrans->cseq = pMsg->CSeq;

    /* set the topmost via from the orginal msg */
    pEntry = NULL;
    if (DLLIST_GetNext(&pMsg->ViaList, &pEntry)) {
        pTrans->via = (*((tViaHFE *)pEntry));
    }
    
    if (pTrans->isServer == TRUE)
        pTrans->to = pMsg->To;
    
    return;
}

/*
 *****************************************************************************
 * ================_FindDestroy()===================
 *
 * Deallocates and detroys Server transaction objects and all 
 *  members, timers, etc.
 *
 *
 * RETURNS: 
 *  SIP_OK:  Transaction was found and destroyed
 *  SIP_FAILED: Transaction could not be found.
 *  
 ******************************************************************************
 */
static vint _FindDestroy(tTrans *pTargetTrans)
{
    tDLListEntry *pEntry;
    tDLList *xactList;

    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "_FindDestroy tId=%X", (uint32)pTargetTrans, 0, 0);

    pEntry = NULL;
    
    if (pTargetTrans->isServer == TRUE) {
        xactList = &_ServerHashTable[pTargetTrans->method];
    }
    else {
        xactList = &_ClientHashTable[pTargetTrans->method];
    }
    SIP_Lock(xactList->lock);
    while (DLLIST_GetNext(xactList, &pEntry)) {
        if ((tTrans *)pEntry == pTargetTrans) {
            /* remove it form the LL */
            DLLIST_Remove(xactList, pEntry);
            SIP_Unlock(xactList->lock);
            _Destroy((tTrans *)pEntry, TRUE);
            return (SIP_OK);
        }
    }
    SIP_Unlock(xactList->lock);

    /* couldn't find anything */
    SIP_DebugLog(SIP_DB_TRANS_LVL_1, "_FindDestroy couldn't find tId=%X", (uint32)pTargetTrans, 0, 0);
    return (SIP_FAILED);
}

static vint _Destroy(tTrans *pTrans, vint notifyApp)
{
    if (pTrans) {
        /* destroy the timers */
        if (pTrans->timer1.hTimer) {
            SIPTIMER_Destroy(pTrans->timer1.hTimer);
            pTrans->timer1.hTimer = NULL;
        }

        if (pTrans->timer2.hTimer) {
            SIPTIMER_Destroy(pTrans->timer2.hTimer);
            pTrans->timer2.hTimer = NULL;
        }

        if (pTrans->timer3.hTimer) {
            SIPTIMER_Destroy(pTrans->timer3.hTimer);
            pTrans->timer3.hTimer = NULL;
        }

        if (pTrans->pMsg) {
            SIP_freeMsg(pTrans->pMsg);
            pTrans->pMsg = NULL;
        }
        
        if (pTrans->pAckMsg) {
            SIP_freeMsg(pTrans->pAckMsg);
            pTrans->pAckMsg = NULL;
        }

        if (notifyApp == TRUE)
            _TRANS_Cb(eTRANS_EVENT_ISDEAD, NULL, pTrans);

        SIP_memPoolFree(eSIP_OBJECT_TRANS, (tDLListEntry *)pTrans);

        return (SIP_OK);
    }

    SIP_DebugLog(SIP_DB_TRANS_LVL_2, "Transaction NOT Destroyed", 0, 0, 0);
    return (SIP_FAILED);
}


void TRANS_StartInviteCancelTimer(tSipHandle hTransaction)
{
    tTrans *pTrans;

    /* This function turns on Timer C.  You will not find timer C in the 
     * spec but please read the following found in Section 9.1 of RFC3261
     *
     * ...a UAC cancelling a request connot rely on receiving a 487
     * response for the original invite, as RFC2543 will not generate a such a response.
     * If there is no such fianl response for the original request in t1 * 64, the original
     * Invite transaction should be considered cancelled and destroyed.
     */

    /* In other words... In the event that an INVITE is cancelled and no final
     * response comes from the UAS within 32 seconds, we need to consider that 
     * Invite transaction as dead and destroy it.
     */
    
    pTrans = (tTrans*)hTransaction;

    if (pTrans) {
        if (pTrans->state == eTRANS_PROCEEDING) {
            if (pTrans->timer3.hTimer) {
                SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_StartInviteCancelTimer: Starting C Timer for hTransaction:%X", 
                        (int)pTrans, 0, 0);
                _TRANS_TimerStart(&pTrans->timer3, eTRANS_EVENT_TIMEOUT_C, _TRANS_timerC);
            }
        }
    }
    return;
}

/*
 *****************************************************************************
 * ================TRANS_KillModule()===================
 *
 * Loops through the server and client transactions and terminates them
 * and frees memory and detroys timer timers.  Use it when the whole
 * stack is being detroyed or is closing down.
 *
 * RETURNS: Nothing
 *  
 ******************************************************************************
 */
void TRANS_KillModule()
{
    int x;
    tDLListEntry *pEntry;
    tDLList *xactList;

    /* kill all the clients */
    for (x = 0 ; x < eSIP_LAST_METHOD ; x++) {
        pEntry = NULL;
        xactList = &_ClientHashTable[x];
        SIP_Lock(xactList->lock);
        while (DLLIST_GetNext(xactList, &pEntry)) {
            tTrans *pCtrans = (tTrans *)pEntry; 

            _TRANS_SetState(pCtrans, eTRANS_TERMINATED);
            /* destroy the timers */
            if (pCtrans->timer1.hTimer) {
                SIPTIMER_Destroy(pCtrans->timer1.hTimer);
                pCtrans->timer1.hTimer = NULL;
            }

            if (pCtrans->timer2.hTimer) {
                SIPTIMER_Destroy(pCtrans->timer2.hTimer);
                pCtrans->timer2.hTimer = NULL;
            }

            if (pCtrans->timer3.hTimer) {
                SIPTIMER_Destroy(pCtrans->timer3.hTimer);
                pCtrans->timer3.hTimer = NULL;
            }

            if (pCtrans->pMsg) {
                SIP_freeMsg(pCtrans->pMsg);
                pCtrans->pMsg = NULL;
            }
            if (pCtrans->hTransport) {
                TRANSPORT_Dealloc(pCtrans->hTransport);
                pCtrans->hTransport = NULL;
            }
        }
        DLLIST_Empty(xactList, eSIP_OBJECT_TRANS);
        SIP_Unlock(xactList->lock);

        pEntry = NULL;
        xactList = &_ServerHashTable[x];
        SIP_Lock(xactList->lock);
        while (DLLIST_GetNext(xactList, &pEntry)) {
            tTrans *pStrans = (tTrans *)pEntry; 
            _TRANS_SetState(pStrans, eTRANS_TERMINATED);
            if (pStrans->timer1.hTimer) {
                SIPTIMER_Destroy(pStrans->timer1.hTimer);
                pStrans->timer1.hTimer = NULL;
            }
            if (pStrans->timer2.hTimer) {
                SIPTIMER_Destroy(pStrans->timer2.hTimer);
                pStrans->timer2.hTimer = NULL;
            }
            if (pStrans->timer3.hTimer) {
                SIPTIMER_Destroy(pStrans->timer3.hTimer);
                pStrans->timer3.hTimer = NULL;
            }

            /* destroy the the pLastReponse */
            if (pStrans->pMsg) {
                SIP_freeMsg(pStrans->pMsg);
                pStrans->pMsg = NULL;
            }
            if (pStrans->hTransport) {
                TRANSPORT_Dealloc(pStrans->hTransport);
                pStrans->hTransport = NULL;
            }
        }
        DLLIST_Empty(xactList, eSIP_OBJECT_TRANS);
        SIP_Unlock(xactList->lock);
    }
    /* Now free up the transaction 'free' lists */

    for (x = 0; x < eSIP_LAST_METHOD; x++) {
        SIP_MutexDestroy(_ClientHashTable[x].lock);
        SIP_MutexDestroy(_ServerHashTable[x].lock);
    }

    return;
}


/*
 *****************************************************************************
 * ================_TRANS_DebugPrintTimer()===================
 *
 *  This is a function used to print timer firing debug logging.
 * It cuts down on the number of constant string allocations 
 *
 * RETURNS:Nothing
 ******************************************************************************
 */

static void _TRANS_DebugPrintTimer(
    tTransEvent event, 
    tSipHandle hOwner, 
    tSipHandle hTransaction)
{
    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "Firing timer:%s hOwner:%X, hTransaction:%X", 
            (int)_TRANS_Event[event], (int)hOwner, (int)hTransaction);
    return;
}

static void _TRANS_TimerStart(
    tTransTimer *pTimer, 
    tTransEvent  event, 
    uint32          time)
{
    pTimer->event = event;
    SIPTIMER_Start(pTimer->hTimer, _transTimer, pTimer, time, FALSE);

}

static void _transTimer(tSipHandle hOwner, void *pArg)
{
    
    tTransTimer *pTimer = (tTransTimer*)pArg;
    tTrans *pTrans = pTimer->pOwner;

    _TRANS_DebugPrintTimer(pTimer->event, hOwner, pTrans);

    if (pTrans->pfState) {
        pTrans->pfState(pTimer->event, pTrans, NULL);
    }
    else {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "%s fired but no state machine set (owner %X)", 
                (int)_TRANS_Event[pTimer->event], (int)hOwner, 0);
        SIP_TaskExit();
    }
    
    return;
}

static void _TRANS_Cb(tTransEvent event, tSipIntMsg *pMsg, tTrans *pTrans)
{
    /* convert internal to external */
    uint32 ExtEvt = _Int2ExtEventTable[event];

    /* If hOwner is NULL and pfAPP is NULL, trans deassociate. */
    if ((NULL == pTrans->hOwner) && (NULL == pTrans->pfApp)) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_Cb: no owner.",
                0, 0, 0);
        /* Free pMsg if exists. */
        if (pMsg) {
            SIP_freeMsg(pMsg);
        }
        /* Free transport when xact dead. */
        if (eTU_DEAD == ExtEvt) {
            TRANSPORT_Dealloc(TRANS_GetTransport(pTrans));
        }
        return;
    }

    if (pTrans->pfApp) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_3,
                "TRANS_Cb: hTransaction:%X sending event:%s to hOwner:%X", 
                (int)pTrans, (int)_TRANS_TuEvent[ExtEvt], (int)pTrans->hOwner);

        pTrans->pfApp(pTrans->hOwner, ExtEvt, pMsg, pTrans);
    }
    return;
}

tSipHandle TRANS_GetTransport(tSipHandle hTransaction)
{
    tTrans *pTrans = (tTrans*)hTransaction;
    if (!hTransaction) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_GetTransport: failed no Transaction ID", 0,0,0);
        return (NULL);
    }
    else 
        return pTrans->hTransport;
}

tSipHandle TRANS_GetContext(tSipHandle hTransaction)
{
    tTrans *pTrans;
    if (!hTransaction) {
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_GetContext: failed no Transaction ID", 0,0,0);
        return (NULL);
    }
    else {
        pTrans = (tTrans*)hTransaction;
        return pTrans->hContext;
    }
}

vint TRANS_CheckId(
    tSipHandle hTransaction, 
    uint32        id)
{
    tTrans *pTrans;
    if (hTransaction) {
        pTrans = (tTrans*)hTransaction;
        if (pTrans->id == id) {
            return TRUE;
        }
    }
    return FALSE;
}

uint32 TRANS_GetId(tSipHandle hTransaction)
{
    tTrans *pTrans;
    if (hTransaction) {
        pTrans = (tTrans*)hTransaction;
        return pTrans->id;
    }
    return 0;    
}

static void _TRANS_SetState(
    tTrans       *pTrans,
    tTransState   state)
{
#ifdef SIP_DEBUG_LOG 
    tTransState oldState;
#endif
    if (pTrans) {
#ifdef SIP_DEBUG_LOG
        oldState = pTrans->state;
#endif
        pTrans->state = state;
        SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_SetState: hTransaction:%X changing state from %s to %s", 
            (int)pTrans, (int)_TRANS_State[oldState], (int)_TRANS_State[state]);
        return;
    }
    SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_SetState: Error -trying to change of a NULL transaction", 
            0, 0, 0);
    return;
}

static vint _TRANS_FailOver(
    tTrans *pTrans, 
    vint    isInvite)
{
    /* We timed out.  Let's see if there's 
     * another available address 
     */
    if (TRANSPORT_AdvanceRmtAddr(pTrans->hTransport) == SIP_OK) {
        /* create a new branch */
        HF_GenerateBranch(pTrans->identifier);
        HF_MakeViaBranch(&pTrans->pMsg->ViaList, pTrans->identifier);
        OSAL_strcpy(pTrans->via.szBranch, pTrans->identifier);
        /* then reset the timers and try again */
        if (isInvite) {
            /* start timer B even for reliable transports */
            _TRANS_TimerStart(&pTrans->timer2, eTRANS_EVENT_TIMEOUT_B, _TRANS_timerB);
            if (pTrans->transType == eTransportUdp) {
                /* Timer 'A' retry timer for UDP only */
                pTrans->t1 = _TRANS_timerA;
                _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_A, pTrans->t1);
            }
        }
        else {
             /* start timer F even for reliable transports */
            _TRANS_TimerStart(&pTrans->timer2, eTRANS_EVENT_TIMEOUT_F, _TRANS_timerF);
            if (pTrans->transType == eTransportUdp) {
                /* Timer 'E' retry timer for UDP only */
                pTrans->t1 = _TRANS_timerE;
                _TRANS_TimerStart(&pTrans->timer1, eTRANS_EVENT_TIMEOUT_E, pTrans->t1);
            }
        }

        /* now resend the message */
        return _ClientSendMethod(pTrans);
    }
    return (SIP_FAILED);
}

/*
 * ======== TRANS_setTimers() ========
 * This function is to configure sip timers T1, T2 and T4.
 * All other timers will change as T1/T2/T4 changes.
 *
 * t1: Value of sip timer t1 in milliseconds.
 * t2: Value of sip timer t2 in milliseconds.
 * t4: Value of sip timer t4 in milliseconds.
 *
 * Returns
 *   None.
 */
void TRANS_setTimers(
    uint32 t1,
    uint32 t2,
    uint32 t4)
{
    /* Ignore timer if it's less then zero */
    if (0 < t1) {
        _TRANS_timerT1 = t1;
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_setTimers: t1:%d", 
                 _TRANS_timerT1, 0, 0);
        /* Apply to other sip timers */
        _TRANS_timerA = _TRANS_timerT1;
#if defined(PROVIDER_CMCC)
        /* CMCC use Tcall to replace timer B */
#else
        _TRANS_timerB = _TRANS_timerT1 * 64;
#endif
        _TRANS_timerF = _TRANS_timerT1 * 64;
        _TRANS_timerE = _TRANS_timerT1;
        /* _TRANS_timerG = _TRANS_timerT1;   unused now */
        _TRANS_timerH = _TRANS_timerT1 * 64;
        _TRANS_timerJ = _TRANS_timerT1 * 64;
    }

    /* Ignore timer if it's less then zero */
    if (0 < t2) {
        _TRANS_timerT2 = t2;
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_setTimers: t2:%d",
                 _TRANS_timerT2, 0, 0);
    }

    /* Ignore timer if it's less then zero */
    if (0 < t4) {
        _TRANS_timerT4 = t4;
        SIP_DebugLog(SIP_DB_TRANS_LVL_1, "TRANS_setTimers: t4:%d", 
                 _TRANS_timerT4, 0, 0);
        /* Apply to other sip timers */
        _TRANS_timerI = _TRANS_timerT4;
        _TRANS_timerK = _TRANS_timerT4;
    }
    
    return;
}

#if defined(PROVIDER_CMCC)
/*
 * ======== TRANS_setTregTimer() ========
 * This function is to configure sip timers Tcall which timer is
 * defined by CMCC. CMCC use Tcall to replace timer B.
 *
 * tcall: Value of sip timer Tcall in millisecond.
 *
 * Returns
 *   None.
 */
void TRANS_setTcallTimer(
    uint32  tcall)
{
    _TRANS_timerB = tcall;
}
#endif
/*
 * ======== TRANS_getTimerT1() ========
 * This function is to get the value of sip timer t1.
 *
 * Returns
 *   The value of sip timer t1 in milliseconds.
 */
uint32 TRANS_getTimerT1()
{
    return _TRANS_timerT1;
}

/*
 * ======== TRANS_getTimerT2() ========
 * This function is to get the value of sip timer t2.
 *
 * Returns
 *   The value of sip timer t2 in milliseconds.
 */
uint32 TRANS_getTimerT2()
{
    return _TRANS_timerT2;
}

/*
 * ======== TRANS_stopTimerTA() ========
 * This function is to stop the timer1 when the transaction state is 
 * eTRANS_CALLING.
 * Returns
 *   none.
 */
void TRANS_stopTimerTA(
    tSipHandle  hTransaction)
{
    tTrans *pTrans;

    if (NULL == hTransaction){
        return;
    }

    pTrans = (tTrans*)hTransaction;
    if (eTRANS_CALLING == pTrans->state) {
        if (eTransportUdp == pTrans->transType) {
            SIPTIMER_Stop(pTrans->timer1.hTimer);
        }
    }
}

/*
 * ======== TRANS_deassociate() ========
 * This function is de-associate the owner from a transcation.
 * This function should be called when the owner no longer exists,
 * so that the transaction will not maps to wrong owner and callback function.
 *
 * Returns
 *   SIP_OK: De-associated owner from transaction.
 *   SIP_FAILED: Otherwise.
 */
int TRANS_deassociate(
    tSipHandle  hTransaction)
{
    tTrans *pTrans;

    SIP_DebugLog(SIP_DB_TRANS_LVL_3, "TRANS_deassociate: hTransaction:%X",
            (int)hTransaction, 0, 0);

    if (NULL == hTransaction){
        return (SIP_FAILED);
    }

    pTrans = (tTrans*)hTransaction;

    /* Stopping retry on UDP when application terminate the request. */
    TRANS_stopTimerTA(hTransaction);

    /* Clear the owner and callback function pointer. */
    pTrans->hOwner = NULL;
    pTrans->pfApp = NULL;
    
    return (SIP_OK);
}
