/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30280 $ $Date: 2014-12-09 14:24:57 +0800 (Tue, 09 Dec 2014) $
 */

#ifndef _SIP_UA_H_
#define _SIP_UA_H_

#include "sip_sip.h"
#include "sip_dbase_sys.h"
#include "sip_session.h"

typedef enum eUaEvtType
{ 
    eUA_RESPONSE,
    eUA_ERROR,
    eUA_TRYING,
    eUA_RINGING,
    eUA_CALL_IS_BEING_FORW,
    eUA_CALL_ATTEMPT,
    eUA_ANSWERED,
    eUA_CALL_DROP,
    eUA_MEDIA_INFO,
    eUA_REGISTRATION,
    eUA_REGISTRATION_COMPLETED,
    eUA_REGISTRATION_FAILED,
    eUA_OPTIONS,
    eUA_OPTIONS_COMPLETED,
    eUA_OPTIONS_FAILED,
    eUA_TRANSFER_ATTEMPT,
    eUA_TRANSFER_RINGING,
    eUA_TRANSFER_COMPLETED,
    eUA_TRANSFER_FAILED,
    eUA_SUBSCRIBE,
    eUA_SUBSCRIBE_COMPLETED,
    eUA_SUBSCRIBE_FAILED,
    eUA_SUBSCRIBE_FAILED_NO_SUBS,
    eUA_NOTIFY_EVENT,
    eUA_NOTIFY_EVENT_NO_SUBS,
    eUA_NOTIFY_EVENT_COMPLETED,
    eUA_NOTIFY_EVENT_FAILED,
    eUA_TEXT_MESSAGE,
    eUA_TEXT_MESSAGE_FAILED,
    eUA_TEXT_MESSAGE_COMPLETED,
    eUA_INFO,
    eUA_INFO_FAILED,
    eUA_INFO_COMPLETED,
    eUA_PRACK,
    eUA_PRACK_FAILED,
    eUA_PRACK_COMPLETED,
    eUA_UPDATE,
    eUA_UPDATE_FAILED,
    eUA_UPDATE_COMPLETED,
    eUA_SESSION,
    eUA_PUBLISH_FAILED,
    eUA_PUBLISH_COMPLETED,
    eUA_PUBLISH,
    eUA_NIC_ERROR,
    eUA_ACK, /* Indicate UAS received ACK to INVITE 200 OK */
    eUA_CANCELED,
    eUA_LAST_EVENT,
}tUaEvtType;

typedef struct sUaAppEvent
{
    struct {
        tSipHandle      hUa;
        tSipHandle      hOwner;
        tUaEvtType      type; 
    }header;
    
    char                szDisplayName[SIP_URI_STRING_MAX_SIZE];
    char                szRemoteUri[SIP_URI_STRING_MAX_SIZE];
    char                szToUri[SIP_URI_STRING_MAX_SIZE];
    char                szContactUri[SIP_URI_STRING_MAX_SIZE];
    uint32              capabilitiesBitmap; /* bitmaps of capabilities */
    char                szPublicGruu[SIP_PUB_GRUU_STR_SIZE];
    OSAL_Boolean        isConference; /* OSAL_TRUE=in conference */
    tSipHandle          hDialogReplaces;
    vint                expires;
    struct {
        uint32          respCode;
        uint16          rport;
        OSAL_NetAddress receivedAddr;
        char            szReasonPhrase[SIP_EVT_STR_SIZE_BYTES];
        tSipHandle      hTransaction;
        uint32          retryAfterPeriod;
    }resp;
    vint                sessNew;
    struct { // xxx short term for Invite with both SDP and XML multipart
        struct {
            char        data[SIP_MAX_TEXT_MSG_SIZE];
            uint32      length;
        }payLoad;
        tSession        session;
    } msgBody;
    char szHeaderFields[SIP_MAX_HEADER_FIELDS + 1][SIP_EVT_STR_SIZE_BYTES];
    /* Note that the last cell in the array 
     * above should always be NULL */
}tUaAppEvent;

typedef struct sUaMsgBody
{
    char  *pBody;
    uint32 length;
}tUaMsgBody;

typedef struct tAor {
    char szUri[SIP_URI_STRING_MAX_SIZE];
}tAor;

typedef enum eUaAuthType
{ 
    eUA_AUTH_TYPE_PASSWORD = 0,
    eUA_AUTH_TYPE_AKA,
} tUaAuthType;

typedef struct sAuthCred {
    char            szAuthUsername[SIP_USERNAME_ARG_STR_SIZE];
    char            szAuthRealm[SIP_REALM_ARG_STR_SIZE];
    tUaAuthType     authType;
    union {
        char        szAuthPassword[SIP_PASSWORD_ARG_STR_SIZE];
        struct {
            char    k[SIP_AUTH_KEY_SIZE];
            vint    kLen;
            char    op[SIP_AUTH_OP_SIZE];
            vint    opLen;
            char    amf[SIP_AUTH_AMF_SIZE];
            vint    amfLen;
        } aka;
    } u;
}tAuthCred;

typedef struct sUaConfig {
    char        szProxy[SIP_URI_STRING_MAX_SIZE];
    char        szRegistrarProxy[SIP_URI_STRING_MAX_SIZE];
    char        szOutboundProxy[SIP_URI_STRING_MAX_SIZE];
    uint32      capabilitiesBitmap;
    char        szFqdn[SIP_URI_STRING_MAX_SIZE];
    tAor        aor[SIP_MAX_NUM_AOR]; 
    char        szCoders[SIP_MAX_CODER_STR_SIZE];
    int         packetRate;
    tAuthCred   authCred[SIP_MAX_NUM_AUTH_CRED];
    vint        useCompactSipMsgs;
    OSAL_MsgQId msgQId;
    /* 
     * For future releases, individual transaction 
     * timer values can be passed into the stack here 
     */
}tUaConfig;

void UA_setWmProxy(
    char *proxy_ptr);

vint UA_GetEvent(
    void        *pIpcMsg, 
    tUaAppEvent *pEvent);

tSipHandle UA_Create(
    tUaConfig *pConfig);

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
    uint32     capabilitiesBitmap);

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
    OSAL_Boolean  keep);

vint UA_HungUp(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn);

vint UA_Answer(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    uint32        capabilitiesBitmap,    
    tLocalIpConn *pLocalConn);

vint UA_TransferCall(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pTransferTarget,
    tSipHandle    hDialogReplaces,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn,
    tSipMethod    method,
    tUaMsgBody   *pMsgBody);

vint UA_TransferStatus(
    tSipHandle     hUa, 
    tSipHandle     hDialog,
    uint32         responseCode,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tLocalIpConn  *pLocalConn);

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
    char         *pQValue);

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
    vint          expires);

vint UA_UnRegister(
    tSipHandle    hUa,
    char         *pAor,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn);

vint UA_CancelRegister(
    tSipHandle    hUa,
    char         *pAor,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tLocalIpConn *pLocalConn);

vint UA_CancelUnRegister(
    tSipHandle    hUa,
    char         *pAor,
    tLocalIpConn *pLocalConn);

vint UA_RegisterResp(
    tSipHandle    hUa, 
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds);

vint UA_Publish(
    tSipHandle    hUa,
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    tLocalIpConn *pLocalConn);

vint UA_Message(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn);

vint UA_Info(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    tLocalIpConn *pLocalConn);

vint UA_Options(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    uint32        capabilitiesBitmap,
    tLocalIpConn *pLocalConn);

vint UA_ModifyCall(
    tSipHandle    hUa, 
    tSipHandle    hDialog, 
    char         *pTargetRefresh,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn);


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
    uint32        capabilitiesBitmap);

vint UA_Prack(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn);

vint UA_Subscribe(
    tSipHandle     hUa,
    tSipHandle    *hDialog,
    char          *pTo,
    char          *pFrom,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tUaMsgBody    *pMsgBody,
    uint32         capabilitiesBitmap,
    tLocalIpConn  *pLocalConn);

vint UA_SubscribeResp(
    tSipHandle    hUa, 
    tSipHandle    hDialog, 
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds);

vint UA_NotifyEvent(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    tSipHandle   *hTransaction,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tUaMsgBody   *pMsgBody,
    tLocalIpConn *pLocalConn);

vint UA_UpdateCall(
    tSipHandle     hUa, 
    tSipHandle     hDialog, 
    char          *pTargetRefresh,
    char          *pHdrFlds[],
    vint           numHdrFlds,
    tSession      *pMediaSess,
    tLocalIpConn  *pLocalConn);

vint UA_UpdateResp(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn);

vint UA_Ack(
    tSipHandle    hUa,
    tSipHandle    hDialog,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn);

vint UA_Destroy(
    tSipHandle hUa);

vint UA_WakeUp(
    tSipHandle hUa);

vint UA_PrackResp(
    tSipHandle    hUa, 
    tSipHandle    hDialog,
    uint32        responseCode,
    char         *pReasonPhrase,
    char         *pHdrFlds[],
    vint          numHdrFlds,
    tSession     *pMediaSess,
    tLocalIpConn *pLocalConn);

#endif

