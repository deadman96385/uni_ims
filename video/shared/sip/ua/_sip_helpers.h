/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28617 $ $Date: 2014-09-01 12:27:55 +0800 (Mon, 01 Sep 2014) $
 */

#ifndef _SIP_HELPERS_H_
#define _SIP_HELPERS_H_

typedef enum sUaListState
{
    eUA_LIST_STATE_NONE,
    eUA_LIST_STATE_ACTIVE,
    eUA_LIST_STATE_INACTIVE,
    eUA_LIST_STATE_BUSY,
} tUaListState;

typedef struct sUaReg 
{
    tSipHandle      hReRegTimer;
    uint32          reRegInterval;
    tSipHandle      hRefreshTimer;
    OSAL_Boolean    refreshEnable;
    uint32          refreshInterval;
    tSipKeepalives  refreshArgs;
    tSipIntMsg     *pMsg;
    vint            isBusy;
    vint            hasTried;
    vint            retryCnt;
    tSipHandle      hTransaction;
    /* This is set at init time 
     * and should never change 
     */
    struct sUa     *pUa;
} tUaReg;

typedef struct sUaPub 
{
    tSipHandle   hTimer;
    tSipIntMsg  *pMsg;
    vint         isBusy;
    vint         hasTried;
    tSipHandle   hTransaction;
    /* This is set at init time 
     * and should never change 
     */
    struct sUa *pUa;
} tUaPub;

typedef struct sUaTrans
{
    tDLListEntry dll;

    tSipIntMsg  *pMsg;
    vint         hasTried;
    tSipHandle   hTransaction;
    struct sUa  *pUa;
} tUaTrans;

typedef struct sUaProxyReg
{
    tSipIntMsg  *pMsg;
    tSipHandle   hTransaction;
} tUaProxyReg;

typedef struct sUaDialogs
{
    uint8       numDialogs;
    tSipDialog  dialogs[SIP_DIALOGS_PER_UA_MAX];
} tUaDialogs;

typedef struct {
    OSAL_MsgQId  msgQId;
    tUaAppEvent  event;
} tUaEvent;

typedef struct sUa
{
    tDLListEntry dll;
    tUaListState listState;
    uint32       taskId;
    vint         useCompactForms;
    tUaDialogs   dialogList;
    tLocalIpConn regLclConn;
    tLocalIpConn pubLclConn;
    tLocalIpConn txtLclConn;
    tLocalIpConn lclConn;
    tUaProxyReg  ProxyReg;
    tUaReg       Reg[SIP_MAX_NUM_AOR];
    tUaPub       Pub[SIP_MAX_NUM_AOR];
    tEPDB_Entry  epdb[eEPDB_LAST_ENTRY];
    uint32       capabilitiesBitmap; /* bitmap of capabilities */
    /* Area used for handling events */
    tUaEvent     event;
} tUa;

void UA_Entry(
    tSipIpcMsg* pIpc);

vint UA_InitModule(
    vint maxDialogsPerUa);

void UA_KillModule(void);

tUa *UA_Alloc(void);

void UA_Dealloc(
    tUa* pUa);

vint UA_Insert(
    tUa *pUa);

tUa* UA_Search(
    const tUri *pTargetUri);

tUa* UA_NicErr(
    tLocalIpConn    *pLclConn,
    tRemoteIpConn   *pRmtConn);

tSipDialog *UA_DialogInit(
    tUa *pUa);

tSipDialog* UA_DialogSearch(
    tUa        *pUa, 
    tSipMsgType msgType,
    tSipMethod  method,
    char       *pTo,
    char       *pFrom,
    char       *pCallId);

tSipDialog* UA_DialogSearchAll(
    tSipIntMsg *pMsg);

void UA_SessionInit(
    tUa        *pUa, 
    tSipDialog *pDialog,
    char       *pName);

vint UA_SendRequest(
    tUa        *pUa,
    tUri       *pUri, 
    tSipHandle  hOwner,
    tSipIntMsg *pMsg, 
    tpfAppCB    pfApp,
    tSipHandle  hTransport,
    tSipHandle *hTransaction);

void UA_AppEvent(
    tUa         *pUa, 
    tSipDialog  *pDialog, 
    tUaEvtType   type, 
    tSipIntMsg  *pMsg,
    tSipHandle   hArg);

void UA_CleanPublish(
    tUaPub *pPub);

void UA_CleanRegistration(
    tUaReg *pReg);

void UA_CleanTrans(
    tUaTrans *pTrans);

void UA_PopulateContact(
    tUa        *pUa, 
    tSipIntMsg *pMsg);

void UA_PopulateRoute(
    tUa        *pUa, 
    tSipIntMsg *pMsg);

vint UA_PopulateRegister(
    tUa             *pUa, 
    tSipIntMsg      *pMsg);

vint UA_WWWAuthenticate(
    tUa     *pUa,
    char    *pUserName,
    tDLList *pAuthChallenge, 
    tDLList *pAuthResponse,
    char    *pAkaAuthResp,
    vint     akaAuthResLen,
    char    *pAkaAuthAuts);

vint UA_Authenicate(
    tUa        *pUa,
    char       *pMethodStr,
    tSipIntMsg *pIn,
    tSipIntMsg *pOut);

vint UA_LoadHeaderFields(
    tSipIntMsg *pMsg, 
    char       *pHdrFlds[],
    vint        numHdrFlds);

char* UA_GetHeaderField(
    char   *pHfStr, 
    char   *pHdrFlds[],
    vint    numHdrFlds);

vint UA_SetFromField(
    tUa        *pUa,
    char       *pFrom,
    char       *pDisplayName,
    tUriPlus   *pUri);

vint UA_SetMsgBody(
    tSipIntMsg *pMsg,
    char       *pBody,
    vint        length);

vint UA_GetTimerMs(
    vint timeoutSecs);

#endif
