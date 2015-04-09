/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2013 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#ifndef __CSMSDI_INPUT_H__
#define __CSMSDI_INPUT_H__

#include "csm_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CSMSDI_InputCallGain {
    int         tx;
    int         rx;
} CSMSDI_InputCallGain;

typedef struct CSMSDI_InputCallResource {
    int         callId;
    int         audioReady;
    int         videoReady;
} CSMSDI_InputCallResource;

typedef union CSMSDI_InputCallUnion {
    char                 remoteAddress[CSM_ALPHA_STRING_SZ + 1];
    int                  errorCode;
    int                  dummyAnswer;
    int                  callIndex;
    int                  dummySwap;
    int                  dummyActive;
    int                  dummyHold;
    int                  dummyExceptX;
    char                 digit;
    int                  dummyAtX;
    int                  dummyConf;
    int                  dummySrvccSuccess;
    int                  dummySrvccFail;
    int                  dummyConfDial;
    int                  dummyConfAdhoc;
    int                  dummyConfModify;
    int                  dummyRcsIndication;
    int                  dummyMediaControl;
    int                  aecEnable;
    CSMSDI_InputCallGain gain;
    CSMSDI_InputCallResource resourceStatus;
} CSMSDI_InputCallUnion;

typedef struct CSMSDI_InputCall{
    CSM_CallEventType           type;
    CSM_CallReason              inputCall_tag;
    char                        reasonDesc[CSM_EVENT_STRING_SZ + 1];
    int                         extraArgument;
    int                         isEmergency;
    CSM_EmergencyType           emergencyType;
    CSM_CallCidType             cidType;
    int                         isRsrcReady;
    CSM_CallNegStatus           negStatus;
    CSM_CallSessionType         callSessionType; /* Indicate this call is a video/voice call */
    CSMSDI_InputCallUnion       inputCall_value;
} CSMSDI_InputCall;

#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_DIAL, remoteAddress);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_REPORT, errorCode);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_ANSWER, dummyAnswer);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_END, callIndex);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_SWAP, dummy_swap);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE, dummyActive);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING, dummyHold);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X, dummyExceptX);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_DIGIT, digit);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_RELEASE_AT_X, dummyAtX);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_CONFERENCE, dummyConf);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS, dummySrvccSuccess);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_SRVCC_FAILED, dummySrvccFail);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_CONF_DIAL, dummyConfDial);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_CONF_ADHOC, dummyConfAdhoc);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_CONF_MODIFY, dummyConfModify);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_RESOURCE_INDICATION, dummyRcsIndication);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL, dummyMediaControl);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_VE_AEC_CMD, aecEnable);
    MTA_TAG_FLD_MAP(CSMSDI_InputCallUnion, CSM_CALL_REASON_VE_GAIN_CTRL, gain);
#endif

typedef struct CSMSDI_InputSms {
    CSM_SmsReason   reason;
    CSM_SmsType     type;
    char            reasonDesc[CSM_EVENT_STRING_SZ + 1];
    char            remoteAddress[CSM_ALPHA_STRING_SZ + 1];
    char            message[CSM_SMS_STRING_SZ + 1];
    char            pdu[CSM_SMS_STRING_SZ + 1];
    int             msgLen;
    char            smsc[CSM_ALPHA_STRING_SZ + 1];
} CSMSDI_InputSms;

typedef struct CSMSDI_InputServiceCgi {
    CSM_ServiceNetworkAccessType type;
    char                         id[CSM_CGI_STRING_SZ + 1];
} CSMSDI_InputServiceCgi;

typedef struct CSMSDI_InputServiceAka {
    char      response[CSM_AKA_RESP_STRING_SZ]; /* size is 16 */
    int       resLength;
    char      auts[CSM_AKA_AUTS_STRING_SZ];     /* size is 14 */
    char      ik[CSM_AKA_IK_STRING_SZ];         /* size is 16 */
    char      ck[CSM_AKA_CK_STRING_SZ];         /* size is 16 */
} CSMSDI_InputServiceAka;

typedef struct CSMSDI_InputServicIpsec {
    int       protectedPort;        /* protected port */
    int       protectedPortPoolSz;  /* protected port pool size */
    int       spi;                  /* SPI start */
    int       spiPoolSz;            /* spi pool size */
} CSMSDI_InputServicIpsec;

typedef struct CSMSDI_InputServicPort {
    int       sip;
    int       audio;                /* audio rtp port */
    int       audioPoolSize;        /* audio rtp pool size */
    int       video;                /* video rtp port */
    int       videoPoolSize;        /* video rtp pool size */
} CSMSDI_InputServicPort;

typedef struct CSMSDI_InputServicCoder {
    int       coderNum;
    int       coderPayloadType[CSM_CODER_NUM];
    char      coderName[CSM_CODER_NUM][CSM_CODER_STRING_SZ + 1];
    char      coderDescription[CSM_CODER_NUM][CSM_CODER_DESCRIPTION_STRING_SZ + 1];
} CSMSDI_InputServicCoder;

typedef union CSMSDI_InputServiceUnion {
    CSMSDI_InputServiceAka      aka;
    int                         dummyAkaNetFail;
    int                         dummyAkaSyncFail;
    char                        impu[CSM_EVENT_STRING_SZ + 1];
    char                        impi[CSM_EVENT_STRING_SZ + 1];
    char                        domain[CSM_EVENT_STRING_SZ + 1];
    char                        password[CSM_EVENT_STRING_SZ + 1];
    char                        pcscf[CSM_EVENT_LONG_STRING_SZ + 1];
    int                         dummyEmgcyPcscf;
    CSMSDI_InputServicPort      port;
    CSMSDI_InputServicIpsec     ipsec;
    CSMSDI_InputServiceCgi      cgi;
    char                        imeiUri[CSM_EVENT_STRING_SZ + 1];
    CSM_ServiceAppsProvision    appsProvision;
    char                        instanceId[CSM_INSTANCE_STRING_SZ + 1];
    int                         dummyIMSEnable;
    int                         dummyIMSDisable;
    int                         dummyRegPeriod;
    int                         dummyRetryTmrPeriod;
    int                         extraArgument;
    CSMSDI_InputServicCoder     coder;
} CSMSDI_InputServiceUnion;

typedef struct CSMSDI_InputService {
    CSM_ServiceReason               inputService_tag;
    char                            reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSMSDI_InputServiceUnion        inputService_value;
    
} CSMSDI_InputService;

#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS, aka);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE, dummyAkaNetFail);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE, dummyAkaSyncFail);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_IMPU, impu);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_IMPI, impi);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_DOMAIN, domain);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_PASSWORD, password);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_PCSCF, pcscf);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_EMGCY_PCSCF, dummyEmgcyPcscf);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_PORTS, port);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_IPSEC, ipsec);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_UPDATE_CGI, cgi);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_IMEI_URI, imeiUri);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_APPS_PROVISION, appsProvision);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_INSTANCE_ID, instanceId);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_IMS_ENABLE, dummyIMSEnable);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_IMS_DISABLE, dummyIMSDisable);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_REREGISTER_PERIOD, dummyRegPeriod);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_RETRY_TIMER_PERIOD, dummyRetryTmrPeriod);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_REG_CAPABILITIES, extraArgument);
    MTA_TAG_FLD_MAP(CSMSDI_InputServiceUnion, CSM_SERVICE_REASON_SET_CODERS, coder);
#endif

typedef struct CSMSDI_InputRadio {
    CSM_RadioReason         reason;
    CSM_RadioNetworkType    networkType;
    char                    address[CSM_IP_ADDRESS_STRING_SZ];
    char                    infcName[CSM_EVENT_STRING_SZ + 1];
    int                     isEmergencyFailoverToCs;
    int                     isEmergencyRegRequired;
} CSMSDI_InputRadio;


typedef enum CSMSDI_SupSrvCbCfMode{
   CSMSDI_EVENT_SUPSRV_CB_MODE_BAOC = 0, /* Barring of All Outgoing Calls */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BOIC,     /* Barring of Outgoing International Calls */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BAIC,     /* Barring of All Incoming Callsl */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BICR,     /* Barring of Incoming Calls - When Roaming */
   CSMSDI_EVENT_SUPSRV_CB_MODE_BOIC_EXHC,/* Bar Outgoing International Calls except to Home Country */
   CSMSDI_EVENT_SUPSRV_CB_MODE_QUERY,
   CSMSDI_EVENT_SUPSRV_CF_MODE_UNCONDITION = 0,   /* Communication Forwarding Unconditional */
   CSMSDI_EVENT_SUPSRV_CF_MODE_BUSY = 1,      /* Communication Forwarding on Busy */
   CSMSDI_EVENT_SUPSRV_CF_MODE_NOREPLY = 2,   /* Communication Forwarding on No Reply */
   CSMSDI_EVENT_SUPSRV_CF_MODE_NOTREACH = 3,  /* Communication Forwarding on not Reachable */
   CSMSDI_EVENT_SUPSRV_CF_MODE_NOTLOGIN = 4 /* Communication Forwarding on not Logged in */
} CSMSDI_SupSrvCbCfMode;


typedef enum CSMSDI_SupSrvGenReqStatus{
    CSMSDI_SUPSRV_OIR_DEFAULT = 0,
    CSMSDI_SUPSRV_OIR_INVOCATION = 1,
    CSMSDI_SUPSRV_OIR_SUPPRESSION = 2,
    CSMSDI_SUPSRV_OIR_QUERY = 3,
    CSMSDI_SUPSRV_DISABLE = 0,
    CSMSDI_SUPSRV_ENABLE = 1,
    CSMSDI_SUPSRV_QUERY = 2,
    CSMSDI_SUPSRV_REGISTRATION = 3 ,
    CSMSDI_SUPSRV_ERASURE = 4,
} CSMSDI_SupSrvGenReqStatus;


typedef struct CSMSDI_InputSupSrv {
    CSM_SupSrvReason                    reason;
    char                                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSMSDI_SupSrvCbCfMode               mode;
    CSMSDI_SupSrvGenReqStatus           status;
    CSM_SupSrvRuleParams                ruleParams;
} CSMSDI_InputSupSrv;


typedef struct CSMSDI_InputUssd {
    CSM_UssdReason    reason;
    char              reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_UssdEncType   encType; /* CSM_USSD_ENCTYPE_ASCII: ASCII encoding. CSM_USSD_ENCTYPE_UCS2: UCS-2 encoding on message field */
    char              message[CSM_USSD_STRING_SZ + 1]; /*HexString encoding */
} CSMSDI_InputUssd;
/*
 * This is a event send to CSM to signal some sort of command, response,
 * or event from the CS or IP layers.
 */
typedef union CSMSDI_InputEventUnion {
    CSMSDI_InputCall    call;
    CSMSDI_InputSms     sms;
    CSMSDI_InputService service;
    CSMSDI_InputRadio   radio;
    CSMSDI_InputSupSrv  supSrv;
    CSMSDI_InputUssd    ussd;
} CSMSDI_InputEventUnion;

typedef struct CSMSDI_InputEvent {
    CSM_EventType           evt_tag; /* Type of message */
    CSMSDI_InputEventUnion  evt_value;
} CSMSDI_InputEvent;

#ifdef MTA_TAG_FLD_MAP 
    MTA_TAG_FLD_MAP(CSMSDI_InputEventUnion, CSM_EVENT_TYPE_CALL, call);
    MTA_TAG_FLD_MAP(CSMSDI_InputEventUnion, CSM_EVENT_TYPE_SMS, sms);
    MTA_TAG_FLD_MAP(CSMSDI_InputEventUnion, CSM_EVENT_TYPE_SERVICE, service);
    MTA_TAG_FLD_MAP(CSMSDI_InputEventUnion, CSM_EVENT_TYPE_RADIO, radio);
    MTA_TAG_FLD_MAP(CSMSDI_InputEventUnion, CSM_EVENT_TYPE_SUPSRV, supSrv);
    MTA_TAG_FLD_MAP(CSMSDI_InputEventUnion, CSM_EVENT_TYPE_USSD, ussd);
#endif
#ifdef __cplusplus
}
#endif

#endif //__CSMSDI_INPUT_H__
