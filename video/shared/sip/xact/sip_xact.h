/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SIP_XACT_H_
#define _SIP_XACT_H_

void TRANS_Init(void);

/* External transaction event types. 
   These are the types sent out to the TU (The UA in this case 
*/
enum 
{
    eTU_FAILED,
    eTU_REQUEST,
    eTU_RESPONSE,
    eTU_DEAD,
    eTU_LAST_EVENT,
};

/* internal transaction event types */
typedef enum eTransEvent
{
    eTRANS_EVENT_NONE = 0,
    eTRANS_EVENT_RESPONSE,
    eTRANS_EVENT_REQUEST,
    eTRANS_EVENT_TIMEOUT_A,
    eTRANS_EVENT_TIMEOUT_B,
    eTRANS_EVENT_TIMEOUT_C,
    eTRANS_EVENT_TIMEOUT_D,
    eTRANS_EVENT_TIMEOUT_E,
    eTRANS_EVENT_TIMEOUT_F,
    eTRANS_EVENT_TIMEOUT_K,
    eTRANS_EVENT_TIMEOUT_G, /* G and greater are for the server side */
    eTRANS_EVENT_TIMEOUT_H,
    eTRANS_EVENT_TIMEOUT_I,
    eTRANS_EVENT_TIMEOUT_J,
    eTRANS_EVENT_TIMEOUT_FIREWALL,
    eTRANS_EVENT_TRANSPORT_ERROR,
    eTRANS_EVENT_ISDEAD,
    eTRANS_LAST_EVENT,
}tTransEvent;

typedef enum eTransState
{
    eTRANS_NONE = 0,
    eTRANS_INIT,
    eTRANS_CALLING,
    eTRANS_TRYING,
    eTRANS_PROCEEDING,
    eTRANS_CONFIRMED,
    eTRANS_COMPLETED,
    eTRANS_TERMINATED,
    eTRANS_LAST_STATE,
}tTransState;

typedef int (*tpfTransportCB)     (tSipHandle hTransport, tSipIntMsg*);
typedef vint(*tpfAppCB)           (tSipHandle hOwner, uint32 event, tSipIntMsg* pMsg, tSipHandle hTransaction);


typedef struct sTransTimer
{
    tSipHandle hTimer;
    tTransEvent event;
    struct sTrans *pOwner;
}tTransTimer;

typedef struct sTrans
{
    tDLListEntry dll;    /* Must always be first in any DLL managed structure */

    /* the type of transaction */
    vint isServer;

    uint32 id; /* used to determine who owns the this memory object */
    
    tSipMethod      method;
    tSipHandle      hTransport;
    tTransportType  transType;
    tTransState     state;
    int (*pfState)(tTransEvent,struct sTrans*,tSipIntMsg*);
    
    uint32             t1;

    /* link to the transport layer */
    tpfTransportCB    pfTransport;
    tpfAppCB          pfApp;
    tSipHandle        hOwner; 
    tSipHandle        hContext;

    
    tTransTimer     timer1;
    tTransTimer     timer2;
    tTransTimer     timer3;

    /* Server transactions are identified by... 
     * topmost via branch and send-by, for rfc3261
     * and a bunch of other header fields for 2543 */
    tViaHFE         via;
    tUri            requestUri;
    tToHF           to;
    tFromHF         from;
    char            szCallId[SIP_CALL_ID_HF_STR_SIZE];
    tCSeq           cseq;

    /* Is the original messge for clients.
     * Is the last response for servers.
     */
    tSipIntMsg  *pMsg;

    /*
     * When ACK's are sent they are cached just in case
     * they have to be resent.
     */
    tSipIntMsg  *pAckMsg;

    /* client side only */
    char identifier[SIP_BRANCH_HF_STR_SIZE];

} tTrans;

typedef int (*tpfTransState)(tTransEvent,struct sTrans*,tSipIntMsg*);

/* Module prototypes */

void TRANS_DumpLists(void);

tSipHandle TRANS_ClientCreate(
    tSipHandle              hTransport,
    tpfTransportCB          pfTransport,
    tTransportType          transType,
    tSipIntMsg             *pMsg,
    tpfAppCB                pfApp,
    tSipHandle              hOwner,
    tSipHandle              hContext);

tSipHandle TRANS_ServerCreate(
    tSipHandle              hTransport,
    tpfTransportCB          pfTransport,
    tTransportType          transType,
    tSipIntMsg             *pMsg,
    tpfAppCB                pfApp,
    tSipHandle              hOwner,
    tSipHandle              hContext);

tSipHandle TRANS_ServerSearch(tSipIntMsg *pMsg);

vint TRANS_ServerReq(
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction);

vint TRANS_ServerResp(
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction);

tSipHandle TRANS_ClientSearch(tSipIntMsg *pMsg);

vint TRANS_ClientReq(
    tSipIntMsg *pMsg, 
    tSipHandle  hTransaction);

vint TRANS_ClientResp(
    tSipIntMsg *pMsg,
    tSipHandle  hTransaction);


tSipHandle TRANS_SearchForServerInvite(tSipIntMsg *pCancelMsg);

tSipHandle TRANS_GetTransport(tSipHandle hTransaction);
tSipHandle TRANS_GetContext(tSipHandle hTransaction);
vint TRANS_CheckId(tSipHandle hTransaction, uint32 id);
uint32 TRANS_GetId(tSipHandle hTransaction);
void TRANS_StartInviteCancelTimer(tSipHandle hTransaction);
void TRANS_KillModule(void);

void TRANS_setTimers(
    uint32 t1,
    uint32 t2,
    uint32 t4);

#if defined(PROVIDER_CMCC)
void TRANS_setTcallTimer(
    uint32  tcall);
#endif

uint32 TRANS_getTimerT1(void);

uint32 TRANS_getTimerT2(void);

void TRANS_stopTimerTA(
    tSipHandle  hTransaction);

int TRANS_deassociate(
    tSipHandle  hTransaction);
#endif

