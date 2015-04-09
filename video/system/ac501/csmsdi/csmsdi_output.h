/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#ifndef __CSMSDI_OUTPUT_H__
#define __CSMSDI_OUTPUT_H__

#include "csm_event.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef union CSMSDI_OutputCallUnion{
    char                address[CSM_NUMBER_STRING_SZ + 1];
    int                 errorCode;
    CSM_CallReport      report;
    int                 dummyDisconnect;
    CSM_CallClip        clipReport;
    int                 dummyWait;
    int                 dummyInit;
    CSM_CallSrvcc       srvcc;
    int                 dummyMedia;
    int                 dummySmsSent;
    int                 dummySmsRecv;
    int                 dummySmsRepRecv;
    int                 dummySms3gpp2Delivery;
    int                 dummySms3gpp2User;
    int                 dummySms3gpp2Voice;
    int                 dummySrvState;
    int                 dummySrvAuthChallenge;
    int                 dummySrvIpsecSetup;
    int                 dummySrvIpsecRelease;
    int                 dummySrvIpsecShutdown;
    CSM_CallSupsrvInfo  supsrvInfo;
    int                 dummyUssdNotify;
    int                 dummyUssdReq;
    int                 dummyUssdDisconnect;
    int                 dummyCallEmgcyInd;
    int                 dummyCallExtraInfo;
    int                 dummyCallModify;
    int                 callIdx; 
    char                digit;
    int                 dummyCallMonitor;
    int                 dummyVideoReqKey;
    int                 dummySrvNotify;
} CSMSDI_OutputCallUnion;

typedef struct CSMSDI_OutputCall {
    CSM_OutputReason            outputcall_tag;
    char                        reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSMSDI_OutputCallUnion      outputcall_value;
} CSMSDI_OutputCall;

#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_OK, address);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_ERROR, errorCode);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_LIST, report);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_DISCONNECT_EVENT, dummyDisconnent);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_INCOMING_EVENT, clipReport);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_WAITING_EVENT, dummyWait);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_INITIALIZING_EVENT, dummyInit);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT, srvcc);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_EARLY_MEDIA, dummyMedia);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SMS_SENT, dummySmsSent);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SMS_RECEIVED, dummySmsRecv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED, dummySmsRecv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SMS_3GPP2_DELIVERY_ACK_RECEIVED, dummySms3gpp2Delivery);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SMS_3GPP2_USER_ACK_RECEIVED, dummySms3gpp2User);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SMS_3GPP2_VOICE_MAIL_NOTIFICATION, dummySms3gpp2Voice);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SERVICE_STATE, dummySrvState);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE, dummySrvAuthChallenge);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP, dummySrvIpsecSetup);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE, dummySrvIpsecRelease);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SERVICE_SHUTDOWN, dummySrvIpsecShutdown);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT, supsrvInfo);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT, dummyUssdNotify);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_USSD_REQUEST_EVENT, dummyUssdReq);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT, dummyUssdDisconnect);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_EMERGENCY_INDICATION, dummyCallEmgcyInd);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_EXTRA_INFO, dummyCallExtraInfo);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_MODIFY_EVENT, dummyCallModify);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_INDEX, callIdx);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_DTMF_DETECT, digit);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_MONITOR, dummyCallMonitor);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY, dummyVideoReqKey);
    MTA_TAG_FLD_MAP(CSMSDI_OutputCallUnion, CSM_OUTPUT_REASON_SERVICE_NOTIFY, dummySrvNotify);
#endif

typedef struct CSMSDI_OutputSmsMsg {
    char              body[CSM_SMS_STRING_SZ + 1];
    int               len;
    int               numOfMessage; /* Number of unread voice mail. */
    /* 3GPP TS 23.040 TP-Service-Centre-Time-Stamp in time-string format */
    char              scts[CSM_ALPHA_STRING_SZ + 1];
    CSM_SmsDcs        dcs; /* Data Coding Scheme */
} CSMSDI_OutputSmsMsg;

typedef union CSMSDI_OutputSmsUnion{
    CSMSDI_OutputSmsMsg msg;
    CSM_SmsErrorCode    errorCode;
    CSM_SmsReportType   reportType;
    int                 dummyDisconnect;
    int                 dummyIncoming;
    int                 dummyWait;
    int                 dummyInit;
    int                 dummySrvccResult;
    int                 dummyMedia;
    int                 dummySmsSent;
    int                 dummySmsRecv;
    int                 dummySmsRepRecv;
    int                 dummySms3gpp2Delivery;
    int                 dummySms3gpp2User;
    int                 dummySms3gpp2Voice;
    int                 dummySrvState;
    int                 dummySrvAuthChallenge;
    int                 dummySrvIpsecSetup;
    int                 dummySrvIpsecRelease;
    int                 dummySrvIpsecShutdown;
    int                 dummySupsrvQuery;
    int                 dummyUssdNotify;
    int                 dummyUssdReq;
    int                 dummyUssdDisconnect;
    int                 dummyCallEmgcyInd;
    int                 dummyCallExtraInfo;
    int                 dummyCallModify;
    int                 dummyCallIdx; /* ccidx for AT+CDU response. */
    char                dummyDtmf;
    int                 dummyCallMonitor;
    int                 dummyVideoReqKey;
    int                 dummySrvNotify;
} CSMSDI_OutputSmsUnion;

typedef struct CSMSDI_OutputSms {
    CSM_OutputReason            outputsms_tag;
    char                        reasonDesc[CSM_EVENT_STRING_SZ + 1];
    char                        address[CSM_ALPHA_STRING_SZ + 1];
    unsigned short              mr; /* The <mr> 'message reference' */
    CSMSDI_OutputSmsUnion       outputsms_value;
} CSMSDI_OutputSms;

#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_OK, msg);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_ERROR, errorCode);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_LIST, reportType);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_DISCONNECT_EVENT, dummyDisconnent);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_INCOMING_EVENT, dummyIncoming);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_WAITING_EVENT, dummyWait);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_INITIALIZING_EVENT, dummyInit);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT, dummySrvccResult);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_EARLY_MEDIA, dummyMedia);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SMS_SENT, dummySmsSent);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SMS_RECEIVED, dummySmsRecv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED, dummySmsRecv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SMS_3GPP2_DELIVERY_ACK_RECEIVED, dummySms3gpp2Delivery);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SMS_3GPP2_USER_ACK_RECEIVED, dummySms3gpp2User);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SMS_3GPP2_VOICE_MAIL_NOTIFICATION, dummySms3gpp2Voice);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SERVICE_STATE, dummySrvState);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE, dummySrvAuthChallenge);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP, dummySrvIpsecSetup);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE, dummySrvIpsecRelease);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SERVICE_SHUTDOWN, dummySrvIpsecShutdown);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT, dummySupsrvQuery);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT, dummyUssdNotify);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_USSD_REQUEST_EVENT, dummyUssdReq);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT, dummyUssdDisconnect);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_EMERGENCY_INDICATION, dummyCallEmgcyInd);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_EXTRA_INFO, dummyCallExtraInfo);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_MODIFY_EVENT, dummyCallModify);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_INDEX, dummyCallIdx);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_DTMF_DETECT, dummyDtmf);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_MONITOR, dummyCallMonitor);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY, dummyVideoReqKey);
    MTA_TAG_FLD_MAP(CSMSDI_OutputSmsUnion, CSM_OUTPUT_REASON_SERVICE_NOTIFY, dummySrvNotify);
#endif

typedef struct CSMSDI_OutputServiceAka{
    char     akaRand[CSM_AKA_RAND_STRING_SZ];
    char     akaAutn[CSM_AKA_AUTN_STRING_SZ];
} CSMSDI_OutputServiceAka;

typedef struct CSMSDI_OutputServiceIpsec{
    char     localIpAddress[CSM_IP_ADDRESS_STRING_SZ + 1];
    char     remoteIpAddress[CSM_IP_ADDRESS_STRING_SZ + 1];
    int      portUc;
    int      portUs;
    int      portPc;
    int      portPs;
    int      spiUc;
    int      spiUs;
    int      spiPc;  /*
                      * SPI for outbound SA to proxy client
                      * port.
                      */
    int      spiPs;  /*
                      * SPI for outbound SA to proxy server
                      * port.
                      */
} CSMSDI_OutputServiceIpsec;

typedef struct CSMSDI_OutputServiceNotify{
    CSM_ServiceNotifyType type;
} CSMSDI_OutputServiceNotify;

typedef union CSMSDI_OutputServiceUnion{
    int                 dummyOk;
    int                 dummyError;
    int                 dummyCallList;
    int                 dummyDisconnect;
    int                 dummyIncoming;
    int                 dummyWait;
    int                 dummyInit;
    int                 dummySrvccResult;
    int                 dummyMedia;
    int                 dummySmsSent;
    int                 dummySmsRecv;
    int                 dummySmsRepRecv;
    int                 dummySms3gpp2Delivery;
    int                 dummySms3gpp2User;
    int                 dummySms3gpp2Voice;
    int                 dummySrvState;
    CSMSDI_OutputServiceAka   aka;
    CSMSDI_OutputServiceIpsec ipsec;
    int                 dummySrvIpsecRelease;
    int                 dummySrvIpsecShutdown;
    int                 dummySupsrvQuery;
    int                 dummyUssdNotify;
    int                 dummyUssdReq;
    int                 dummyUssdDisconnect;
    int                 dummyCallEmgcyInd;
    int                 dummyCallExtraInfo;
    int                 dummyCallModify;
    int                 dummyCallIdx; /* ccidx for AT+CDU response. */
    char                dummyDtmf;
    int                 dummyCallMonitor;
    int                 dummyVideoReqKey;
    CSMSDI_OutputServiceNotify notify;
} CSMSDI_OutputServiceUnion;
typedef struct CSMSDI_OutputService {
    CSM_OutputReason    service_tag;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_ServiceState    state;    
    int                 errorCode;    
    int                 isEmergency;
    CSMSDI_OutputServiceUnion service_value;
} CSMSDI_OutputService;
#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_OK, dummyOk);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_ERROR, dummyError);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_LIST, dummyCallList);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_DISCONNECT_EVENT, dummyDisconnent);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_INCOMING_EVENT, dummyIncoming);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_WAITING_EVENT, dummyWait);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_INITIALIZING_EVENT, dummyInit);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT, dummySrvccResult);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_EARLY_MEDIA, dummyMedia);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SMS_SENT, dummySmsSent);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SMS_RECEIVED, dummySmsRecv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED, dummySmsRecv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SMS_3GPP2_DELIVERY_ACK_RECEIVED, dummySms3gpp2Delivery);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SMS_3GPP2_USER_ACK_RECEIVED, dummySms3gpp2User);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SMS_3GPP2_VOICE_MAIL_NOTIFICATION, dummySms3gpp2Voice);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SERVICE_STATE, dummySrvState);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE, aka);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP, ipsec);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE, dummySrvIpsecRelease);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SERVICE_SHUTDOWN, dummySrvIpsecShutdown);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT, dummySupsrvQuery);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT, dummyUssdNotify);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_USSD_REQUEST_EVENT, dummyUssdReq);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT, dummyUssdDisconnect);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_EMERGENCY_INDICATION, dummyCallEmgcyInd);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_EXTRA_INFO, dummyCallExtraInfo);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_MODIFY_EVENT, dummyCallModify);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_INDEX, dummyCallIdx);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_DTMF_DETECT, dummyDtmf);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_MONITOR, dummyCallMonitor);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY, dummyVideoReqKey);
    MTA_TAG_FLD_MAP(CSMSDI_OutputServiceUnion, CSM_OUTPUT_REASON_SERVICE_NOTIFY, notify);
#endif
typedef enum CSMSDI_SupSrvCbCfModeOut{
   CSMSDI_EVENT_SUPSRV_CB_MODE_BAOC_OUT = 0, /* Barring of All Outgoing Calls */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BOIC_OUT,     /* Barring of Outgoing International Calls */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BAIC_OUT,     /* Barring of All Incoming Callsl */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BICR_OUT,     /* Barring of Incoming Calls - When Roaming */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BOIC_EXHC_OUT,/* Bar Outgoing International Calls except to Home Country */
   CSMSDI_EVENT_SUPSRV_CB_MODE_QUERY_OUT,
   CSMSDI_EVENT_SUPSRV_CF_MODE_UNCONDITION_OUT = 0,   /* Communication Forwarding Unconditional */
   CSMSDI_EVENT_SUPSRV_CF_MODE_BUSY_OUT = 1,      /* Communication Forwarding on Busy */
   CSMSDI_EVENT_SUPSRV_CF_MODE_NOREPLY_OUT = 2,   /* Communication Forwarding on No Reply */
   CSMSDI_EVENT_SUPSRV_CF_MODE_NOTREACH_OUT = 3,  /* Communication Forwarding on not Reachable */
   CSMSDI_EVENT_SUPSRV_CF_MODE_NOTLOGIN_OUT = 4 /* Communication Forwarding on not Logged in */
} CSMSDI_SupSrvCbCfModeOut;


typedef enum CSMSDI_SupSrvResStatusOut{
    CSMSDI_SUPSRV_OIR_DEFAULT_OUT = 0,
    CSMSDI_SUPSRV_OIR_INVOCATION_OUT = 1,
    CSMSDI_SUPSRV_OIR_SUPPRESSION_OUT = 2,
    CSMSDI_SUPSRV_OIR_QUERY_OUT = 3,
    CSMSDI_SUPSRV_RES_DISABLE_OUT = 0,
    CSMSDI_SUPSRV_RES_ENABLE_OUT = 1,
} CSMSDI_SupSrvResStatusOut;

typedef enum CSMSDI_SupSrvProvisionOut{
    CSMSDI_SUPSRV_OIR_NOT_PROVISIONED_OUT = 0,
    CSMSDI_SUPSRV_OIR_PERM_MODE_OUT = 1,
    CSMSDI_SUPSRV_OIR_UNKNOWN_OUT = 2,
    CSMSDI_SUPSRV_OIR_TEMP_MODE_RESTRICTED_OUT = 3,
    CSMSDI_SUPSRV_OIR_TEMP_MODE_ALLOWED_OUT = 4,
    CSMSDI_SUPSRV_TIR_NOT_PROVISIONED_OUT = 0,
    CSMSDI_SUPSRV_TIR_PERM_MODE_OUT = 1,
    CSMSDI_SUPSRV_TIR_UNKNOWN_OUT = 2,
    CSMSDI_SUPSRV_TIR_TEMP_MODE_RESTRICTED_OUT = 3,
    CSMSDI_SUPSRV_TIR_TEMP_MODE_ALLOWED_OUT = 4,
    CSMSDI_SUPSRV_NOT_PROVISIONED_OUT = 0,
    CSMSDI_SUPSRV_PROVISIONED_OUT = 1,
    CSMSDI_SUPSRV_UNKNOWN_OUT = 2,
} CSMSDI_SupSrvProvisionOut;
typedef struct CSMSDI_OutputSupSrv {
    CSM_OutputReason            reason;
    char                        reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_SupSrvCmdType           cmdType;
    CSM_SupSrvErrorCode         errorCode;
    CSMSDI_SupSrvCbCfModeOut       mode;
    CSMSDI_SupSrvResStatusOut      resStatus;
    CSMSDI_SupSrvProvisionOut      provision;
    CSM_SupSrvRuleParams        ruleParams;
} CSMSDI_OutputSupSrv;


typedef struct CSMSDI_OutputUssd {
    CSM_OutputReason    reason;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_UssdEncType     messageEncType;
    char                message[CSM_USSD_STRING_SZ + 1];
    CSM_UssdErrorCode   errorCode;
} CSMSDI_OutputUssd;

typedef union CSMSDI_OutputEventUnion {
    CSMSDI_OutputCall    call; /* Details of the call. */
    CSMSDI_OutputSms     sms;
    CSMSDI_OutputService service; /* Network Registration output */
    int                  dummyRadio;
    CSMSDI_OutputSupSrv  supSrv;
    CSMSDI_OutputUssd    ussd;
} CSMSDI_OutputEventUnion;

typedef struct CSMSDI_OutputEvent {
    CSM_EventType           evt_tag; /* Type of message */
    CSMSDI_OutputEventUnion evt_value;
} CSMSDI_OutputEvent;

#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_OutputEventUnion, CSM_EVENT_TYPE_CALL, call);
    MTA_TAG_FLD_MAP(CSMSDI_OutputEventUnion, CSM_EVENT_TYPE_SMS, sms);
    MTA_TAG_FLD_MAP(CSMSDI_OutputEventUnion, CSM_EVENT_TYPE_SERVICE, service);
    MTA_TAG_FLD_MAP(CSMSDI_OutputEventUnion, CSM_EVENT_TYPE_RADIO, dummyRadio);
    MTA_TAG_FLD_MAP(CSMSDI_OutputEventUnion, CSM_EVENT_TYPE_SUPSRV, supSrv);
    MTA_TAG_FLD_MAP(CSMSDI_OutputEventUnion, CSM_EVENT_TYPE_USSD, ussd);
#endif
#ifdef __cplusplus
}
#endif

#endif // __CSMSDI_OUTPUT_H__
