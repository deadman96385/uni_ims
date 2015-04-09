/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef __CSM_EVENT_H__
#define __CSM_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <osal.h>

/* Generally used definitions */
#define CSM_OUTPUT_INVALID_ID           (-1)
#define CSM_EVENT_STRING_SZ             (128)
#define CSM_EVENT_LONG_STRING_SZ        (511)
#define CSM_EVENT_MAX_CALL_LIST_SIZE    (6)
#define CSM_AUDIO_CODER_NUM             (2)
#define CSM_VIDEO_CODER_NUM             (2)
#define CSM_CODER_NUM                   (CSM_AUDIO_CODER_NUM + CSM_VIDEO_CODER_NUM)
#define CSM_CODER_STRING_SZ             (31)
#define CSM_CODER_DESCRIPTION_STRING_SZ (255)
#define CSM_NUMBER_STRING_SZ            (80)
#define CSM_ALPHA_STRING_SZ             (128)
#define CSM_SMS_STRING_SZ               (512)
#define CSM_FQDN_STRING_SZ              (255)
#define CSM_SUPSRV_PARAM_STRING_SZ      (16)
#define CSM_CGI_STRING_SZ               (24)
#define CSM_URI_STRING_SZ               (128)
#define CSM_USSD_STRING_SZ              (1024)

#define CSM_AKA_RAND_STRING_SZ          (16)
#define CSM_AKA_AUTN_STRING_SZ          (16)
#define CSM_AKA_RESP_STRING_SZ          (16)
#define CSM_AKA_IK_STRING_SZ            (16)
#define CSM_AKA_CK_STRING_SZ            (16)
#define CSM_AKA_AUTS_STRING_SZ          (14)

/* Max media number for each resource reservation */
#define CSM_MAX_MEDIA_NUM               (2)
/* Max ip flow number for each media */
#define CSM_MAX_IP_FLOW_NUM             (2)
/* String size for ip address */
#define CSM_IP_ADDRESS_STRING_SZ        (46) 
#define CSM_HISTORY_INDEX_STRING_SZ     (8)
#define CSM_HISTORY_MAX_LIST_SIZE       (3)
#define CSM_INSTANCE_STRING_SZ          (64)
#define CSM_ERROR_CODE_SIP_START        (300)
#define CSM_ERROR_CODE_SIP_UNAUTH       (401)
#define CSM_ERROR_CODE_SIP_END          (699)

/* Queue and IPC definitions */
#ifdef VPORT_4G_PLUS_APROC
/*
 * Define different message queue name for CSM running in application processor.
 */
#define CSM_INPUT_EVENT_QUEUE_NAME          "aproc.csm.evt.in.q"
#define CSM_OUTPUT_EVENT_QUEUE_NAME         "aproc.csm.evt.out.q"
#define CSM_OUTPUT_ISIM_EVENT_QUEUE_NAME    "aproc.csm.evt.out.isim.q"
#else
#define CSM_INPUT_EVENT_QUEUE_NAME          "csm.evt.in.q"
#define CSM_OUTPUT_EVENT_QUEUE_NAME         "csm.evt.out.q"
#define CSM_OUTPUT_ISIM_EVENT_QUEUE_NAME    "csm.evt.out.isim.q"
#endif
#define CSM_INPUT_EVENT_MSGQ_LEN            (16)
#define CSM_OUTPUT_EVENT_MSGQ_LEN           (8)
#define CSM_OUTPUT_ISIM_EVENT_MSGQ_LEN      (8)

#define IR92_DEBUG_TAG "IR92"


/*
 ******************************************************************************
 * Enumeration definitions
 ******************************************************************************
 */

/* What type of event this is. */
typedef enum {
    CSM_EVENT_TYPE_CALL = 0,
    CSM_EVENT_TYPE_SMS,
    CSM_EVENT_TYPE_SERVICE,
    CSM_EVENT_TYPE_RADIO,
    CSM_EVENT_TYPE_SUPSRV,
    CSM_EVENT_TYPE_USSD
} CSM_EventType;

/*
 **************************************
 * CSM input event Reason enumeration
 **************************************
 */
/* Reason for this call event. */
typedef enum {
    /*
     * The following events are commands from AT interface send as input to CSM
     */
    CSM_CALL_REASON_AT_CMD_DIAL = 0,        /* ATD 'Dial' Command. */
    CSM_CALL_REASON_AT_CMD_REPORT,          /* AT+CLCC 'call list' command. */
    CSM_CALL_REASON_AT_CMD_ANSWER,          /* ATA 'answer' command. */
    CSM_CALL_REASON_AT_CMD_END,             /* ATH 'Hangup' or reject a single call. */
    CSM_CALL_REASON_AT_CMD_SWAP,            /* AT+CHLD=2.  Swap calls or accept a call waiting. */
    CSM_CALL_REASON_AT_CMD_END_ALL_ACTIVE,  /* AT+CHLD=1. End all active calls and accept held or call waiting. */
    CSM_CALL_REASON_AT_CMD_END_ALL_HELD_OR_WAITING, /* AT+CHLD=0. Release all held calls or reject a call waiting. */
    CSM_CALL_REASON_AT_CMD_HOLD_ALL_EXCEPT_X, /* AT+CHLD=2x Request private consultation.  Hold all calls except the one referenced by 'x' (index) */
    CSM_CALL_REASON_AT_CMD_DIGIT,           /* AT+VTS genereate a digit tone on the line, */
    CSM_CALL_REASON_AT_CMD_RELEASE_AT_X,    /* AT+CHLD=1x release the call referenced at 'x' (index). */
    CSM_CALL_REASON_AT_CMD_CONFERENCE,      /* AT+CHLD=3 merge in all held calls to current call. */
    CSM_CALL_REASON_AT_CMD_SRVCC_START,     /* +CIREPH:0 SRVCC handover starts. */
    CSM_CALL_REASON_AT_CMD_SRVCC_SUCCESS,   /* +CIREPH:1 SRVCC handover is successful, call need to release. */
    CSM_CALL_REASON_AT_CMD_SRVCC_FAILED,    /* +CIREPH:2 SRVCC handover is failed or canceled, call need to re-invite. */
    CSM_CALL_REASON_AT_CMD_CONF_DIAL,       /* AT%CGU=1 extended conf-call dial */
    CSM_CALL_REASON_AT_CMD_CONF_ADHOC,      /* AT%CGU=2 extended conf-call ad-hoc create */
    CSM_CALL_REASON_AT_CMD_CONF_MODIFY,     /* AT%CGU=3 extended conf-call modify */
    CSM_CALL_REASON_AT_CMD_CONF_ADD_PARTY,  /* AT%CGU=4 extended conf-call add another */
    /*
     * Call resource reservation event.
     */
    CSM_CALL_REASON_RESOURCE_INDICATION,
    CSM_CALL_REASON_AT_CMD_MEDIA_CONTROL, /* AT+CCMMD media control */
    /* VE control */
    CSM_CALL_REASON_VE_AEC_CMD,
    CSM_CALL_REASON_VE_GAIN_CTRL,
    CSM_CALL_REASON_VE_CN_GAIN_CTRL,
} CSM_CallReason;

/* Reason for this SMS event. */
typedef enum {
    CSM_SMS_REASON_AT_CMD_SEND_MESSAGE,
    CSM_SMS_REASON_SET_SMSC,
} CSM_SmsReason;

/* Reason for this SMS Data encoding. */
typedef enum {
    CSM_SMS_DCS_DC_ASCII          = 0x04, /* General Data Coding indication */
    CSM_SMS_DCS_DC_UTF8           = 0x08,
    CSM_SMS_DCS_DC_RESERVED       = 0x0C,
    CSM_SMS_DCS_ME_VOICE_MAIL     = 0xD0, /* ME indicator */
    CSM_SMS_DCS_ME_OTHER_MSG      = 0xD3,
} CSM_SmsDcs;


/* Reason for this Service event. */
typedef enum {    
    CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS,
    CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE,
    CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE,
    CSM_SERVICE_REASON_SET_IMPU,
    CSM_SERVICE_REASON_SET_IMPI,
    CSM_SERVICE_REASON_SET_DOMAIN,
    CSM_SERVICE_REASON_SET_PASSWORD,
    CSM_SERVICE_REASON_SET_PCSCF,
    CSM_SERVICE_REASON_SET_EMGCY_PCSCF,
    CSM_SERVICE_REASON_SET_PORTS,
    CSM_SERVICE_REASON_SET_IPSEC,
    CSM_SERVICE_REASON_UPDATE_CGI,
    CSM_SERVICE_REASON_SET_IMEI_URI,
    CSM_SERVICE_REASON_APPS_PROVISION,
    CSM_SERVICE_REASON_SET_INSTANCE_ID,
    CSM_SERVICE_REASON_IMS_ENABLE,
    CSM_SERVICE_REASON_IMS_DISABLE,
    CSM_SERVICE_REASON_SET_REREGISTER_PERIOD,
    CSM_SERVICE_REASON_SET_RETRY_TIMER_PERIOD,
    CSM_SERVICE_REASON_SET_REG_CAPABILITIES,
    CSM_SERVICE_REASON_SET_CODERS,
    CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER,
    CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER,
} CSM_ServiceReason;

/* Reason for Radio event. */
typedef enum {
    CSM_RADIO_REASON_IP_CHANGE = 0,
    CSM_RADIO_REASON_IP_CHANGE_EMERGENCY,
    CSM_RADIO_REASON_RADIO_CHANGE_EMERGENCY,
} CSM_RadioReason;

/* Reason for this Supplementary Service event. */
typedef enum {
    CSM_EVENT_SUPSRV_REASON_AT_CMD_OIP = 0,  /* AT+CLIP */
    CSM_EVENT_SUPSRV_REASON_AT_CMD_OIR,      /* AT+CLIR */
    CSM_EVENT_SUPSRV_REASON_AT_CMD_TIP,      /* AT+COLP */
    CSM_EVENT_SUPSRV_REASON_AT_CMD_TIR,      /* AT+COLR */
    CSM_EVENT_SUPSRV_REASON_AT_CMD_CW,       /* AT+CCWA */
    CSM_EVENT_SUPSRV_REASON_AT_CMD_CF,       /* AT+CCFC */
    CSM_EVENT_SUPSRV_REASON_AT_CMD_CB,       /* AT+CLCK */
} CSM_SupSrvReason;

/* Reason for this USSD event. */
typedef enum {
    CSM_USSD_REASON_AT_CMD_SEND_USSD = 0,
    CSM_USSD_REASON_AT_CMD_REPLY_USSD,
    CSM_USSD_REASON_AT_CMD_DISCONNECT_USSD,
} CSM_UssdReason;

/*
 **************************************
 * CSM output response reason enumeration
 **************************************
 */
typedef enum {
    CSM_OUTPUT_REASON_OK,        /* '0' or 'OK' response to a command. */
    CSM_OUTPUT_REASON_ERROR,     /* +CME ERROR response to a command. The error code will be in the payload. */
    CSM_OUTPUT_REASON_CALL_LIST, /* +CLCC response to a AT+CLCC command. */
    CSM_OUTPUT_REASON_DISCONNECT_EVENT, /* Event indicating that a remote party has disconnected. */
    CSM_OUTPUT_REASON_INCOMING_EVENT, /* Event indicating that a new incoming call is being requested. */
    CSM_OUTPUT_REASON_WAITING_EVENT,  /* Event indicating that a call is waiting. */
    CSM_OUTPUT_REASON_INITIALIZING_EVENT, /*
                                           * Event indicating that a new incoming
                                           * call is initializing but the
                                           * resource is not ready yet.
                                           */
    CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT, /* Event indicating the result of SRVCC. Now support re-invite result. */
    CSM_OUTPUT_REASON_CALL_EARLY_MEDIA, /* Event indicating that there is early media for outgoing call. */
    CSM_OUTPUT_REASON_SMS_SENT,  /* Event indicating that a SMS was sent and that the mr value should be populated. */
    CSM_OUTPUT_REASON_SMS_RECEIVED,  /* Event indicating that a SMS was received. */
    CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED,  /* Event indicating that a SMS Status report was received. */
    /* Event indicating that a 3GPP2 SMS Delivery Acknowledagement message was received. */
    CSM_OUTPUT_REASON_SMS_3GPP2_DELIVERY_ACK_RECEIVED,
    /* Event indicating that a 3GPP2 SMS User Acknowledagement message was received. */
    CSM_OUTPUT_REASON_SMS_3GPP2_USER_ACK_RECEIVED,
    /* Event indicating that a 3GPP3 SMS VMN was received. */
    CSM_OUTPUT_REASON_SMS_3GPP2_VOICE_MAIL_NOTIFICATION,
    CSM_OUTPUT_REASON_SERVICE_STATE, /* Event indicating a network registration state change */
    CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE,  /* AKA Auth Challenge to isim app */
    CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP,    /* IPSec ports and SPIs information when IPSec setup */
    CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE,  /* IPSec ports and SPIs information when IPSec release */
    CSM_OUTPUT_REASON_SERVICE_SHUTDOWN, /* The event to indicate that CSM is going to shutdown. */
    CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT,
    CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT, /* Event indicating a new USSD connection.*/
    CSM_OUTPUT_REASON_USSD_REQUEST_EVENT, /* Event indicating a USSD request.*/
    CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT, /* Disconnect USSD connection.*/
    CSM_OUTPUT_REASON_CALL_EMERGENCY_INDICATION, /* Event to indicate the outgoing call is an emergency call. */
    CSM_OUTPUT_REASON_CALL_EXTRA_INFO, /* extra info for forwarded call history */
    CSM_OUTPUT_REASON_CALL_MODIFY_EVENT, /* Got a call modification event. */
    CSM_OUTPUT_REASON_CALL_INDEX, /* Event indicating the call index to AT+CDU. */
    CSM_OUTPUT_REASON_CALL_DTMF_DETECT, /* detect dtmf from remote */
    CSM_OUTPUT_REASON_CALL_MONITOR, /* unsolicited call monitoring report */
    CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY, /* Video Key frame reqeust */
    CSM_OUTPUT_REASON_SERVICE_NOTIFY, /* Event indicating service related notification. */
} CSM_OutputReason;

/*
 **************************************
 * CSM output error code enumeration
 **************************************
 */
/*
 * CSM error code definitions.  CSM output events that indicate
 * an error will also have a 'errorCode' member populated with
 * either an HTTP error code (i.e. 3xx-6xx) or a D2 Error code
 * (0-99).  The values in the CSM_ErrorCode enumeration represent
 * the D2 error code definitions.  NOTE, these values should match
 * a semantically equivalent ISI value in isi_errors.h
 */
typedef enum {
    CSM_ERROR_CODE_NONE                  = -1,
    CSM_ERROR_CODE_NO_NETWORK_AVAILABLE  = 0,
    CSM_ERROR_CODE_REQUEST_TIMED_OUT     = 14,
    CSM_ERROR_CODE_ACK_RECEIPT_TIMED_OUT = 15,
    CSM_ERROR_CODE_RTP_RTCP_TIMEOUT      = 16,
    CSM_ERROR_CODE_XPORT_INIT_FAILURE    = 17,
} CSM_ErrorCodes;

/* SMS Error Code. */
typedef enum {
    CSM_SMS_ERROR_PDU_MODE        = 304,
    CSM_SMS_ERROR_NET_SERVICE     = 331,
    CSM_SMS_ERROR_NETWORK_TIMEOUT = 332,
    CSM_SMS_ERROR_UNKNOWN         = 500,
    CSM_SMS_ERROR_SIP             = 512,
} CSM_SmsErrorCode;

/* SupSrv Error Code. */
typedef enum {
    CSM_SUPSRV_ERROR_NONE           = 0,
    CSM_SUPSRV_ERROR_NO_NET_SERVICE = 30,  /*err =30 : no network service */
    CSM_SUPSRV_ERROR_NET_TIMEOUT    = 31,  /*err =31 :network timeout */
    CSM_SUPSRV_ERROR_UNKNOWN        = 100, /* err=100 :unknown */
    CSM_SUPSRV_ERROR_HTTP           = 601, /* http error header in reasonDesc */
    CSM_SUPSRV_ERROR_XCAP_CONFLICT  = 602, /* 409 with xcap-error+xml in reasonDesc */
    CSM_SUPSRV_ERROR_XML_PARSING    = 603, /* XML content invalid or failed to parse */
} CSM_SupSrvErrorCode;

/* USSD Error Code. */
typedef enum {
    CSM_USSD_ERROR_NONE                 = 0,
    CSM_USSD_ERROR_SIP                  = 501,
    CSM_USSD_ERROR_USSD_UNKNOWN,
    CSM_USSD_ERROR_USSD_LANG,
    CSM_USSD_ERROR_USSD_UNEXPECTED_DATA,
} CSM_UssdErrorCode;


/*
 ******************
 * Call enumeration
 ******************
 */
typedef enum {
   CSM_CALL_EVENT_TYPE_AT = 0,
   CSM_CALL_EVENT_TYPE_ISI
} CSM_CallEventType;

typedef enum {
    CSM_CALL_CID_TYPE_NONE = 0,
    CSM_CALL_CID_TYPE_INVOCATION,
    CSM_CALL_CID_TYPE_SUPPRESSION,
    CSM_CALL_CID_TYPE_USE_ALIAS, /* Use alias URI for the call. */
} CSM_CallCidType;

typedef enum {
    CSM_EMERGENCY_SUB_TYPE_NONE = 0,
    CSM_EMERGENCY_SUB_TYPE_GENERIC,
    CSM_EMERGENCY_SUB_TYPE_AMBULANCE,
    CSM_EMERGENCY_SUB_TYPE_POLICE,
    CSM_EMERGENCY_SUB_TYPE_FIRE,
    CSM_EMERGENCY_SUB_TYPE_MARINE,
    CSM_EMERGENCY_SUB_TYPE_MOUNTAIN
} CSM_EmergencyType;

typedef enum {
    CSM_CALL_DIR_MOBILE_ORIGINATED = 0,
    CSM_CALL_DIR_MOBILE_TERMINATED,
} CSM_CallDirection;

typedef enum {
    CSM_CALL_STATE_INVALID = -1,
    CSM_CALL_STATE_ACTIVE,
    CSM_CALL_STATE_HOLD,
    CSM_CALL_STATE_DIALING,
    CSM_CALL_STATE_ALERTING,
    CSM_CALL_STATE_INCOMING,
    CSM_CALL_STATE_WAITING,
    CSM_CALL_STATE_TRYING,
    CSM_CALL_STATE_INITIALIZING,
} CSM_CallState;

typedef enum {
    CSM_CALL_ADDRESS_NATIONAL = 129,
    CSM_CALL_ADDRESS_INTERNATIONAL = 145,
    CSM_CALL_ADDRESS_TYPE_3,
} CSM_CallAddressType;

typedef enum {
    CSM_CALL_SINGLE_PARTY = 0,
    CSM_CALL_CONFERENCE,
    CSM_CALL_IMS_CONF
} CSM_CallMultiParty;

typedef enum {
    CSM_CALL_MODE_VOICE = 0,
    CSM_CALL_MODE_DATA
} CSM_CallMode;

typedef enum {
    CSM_CALL_SESSION_TYPE_NONE  = 0,
    CSM_CALL_SESSION_TYPE_AUDIO = 1,
    CSM_CALL_SESSION_TYPE_VIDEO = 2,
} CSM_CallSessionType;

/*
 * Enum for extended call status report.
 */

/* Negotiation status for +CLCCS */
typedef enum {
    CSM_CALL_NEG_STATUS_INVALID = 0,
    CSM_CALL_NEG_STATUS_UNCONDITIONAL,
    CSM_CALL_NEG_STATUS_PROPOSED,
    CSM_CALL_NEG_STATUS_ACCEPTED,
    CSM_CALL_NEG_STATUS_REJECTED,
} CSM_CallNegStatus;

/* Call status for +CLCCS */
typedef enum {
    CSM_CALL_STATUS_IDLE = 1,
    CSM_CALL_STATUS_CALLING_MO,
    CSM_CALL_STATUS_CONNECTING_MO,
    CSM_CALL_STATUS_ALERTING_MO,
    CSM_CALL_STATUS_ALERTING_MT,
    CSM_CALL_STATUS_ACTIVE,
    CSM_CALL_STATUS_RELEASED_MO,
    CSM_CALL_STATUS_RELEASED_MT,
    CSM_CALL_STATUS_USER_BUSY,
    CSM_CALL_STATUS_USER_DTMD_USER_BUSY, /* User Determined User Busy. */
    CSM_CALL_STATUS_CALL_WAITING_MO,
    CSM_CALL_STATUS_CALL_WAITING_MT,
    CSM_CALL_STATUS_CALL_HOLD_MO,
    CSM_CALL_STATUS_CALL_HOLD_MT,
} CSM_CallStatus;

/* Number type for +CLCCS */
typedef enum {
    CSM_CALL_NUMBER_TYPE_INVALID = 0,
    CSM_CALL_NUMBER_TYPE_URI,
    CSM_CALL_NUMBER_TYPE_TON, /* See  CSM_CallAddressType. */
} CSM_CallNumberType;

/* Define the SRVCC re-invite result. */
typedef enum {
    CSM_CALL_SRVCC_REINVITE_OK = 1,
    CSM_CALL_SRVCC_REINVITE_FAILED,
} CSM_SrvccResult;

typedef enum {
    CSM_CALL_CAUSE_CODE_INVALID         = -1,
    CSM_CALL_CAUSE_CODE_NONE            = 0,
    CSM_CALL_CAUSE_CODE_UNKNOWN         = 404,
    CSM_CALL_CAUSE_CODE_BUSY            = 486,
    CSM_CALL_CAUSE_CODE_NOREPLY         = 408,
    CSM_CALL_CAUSE_CODE_UNCONDITIONAL   = 302,
    CSM_CALL_CAUSE_CODE_NOTREACHABLE    = 503
} CSM_CallCauseCode;

/*
 ******************
 * Sms enumeration
 ******************
 */
typedef enum {
    CSM_SMS_TYPE_TEXT        = 0,
    CSM_SMS_TYPE_PDU_3GPP,
    CSM_SMS_TYPE_PDU_3GPP2
} CSM_SmsType;

typedef enum {
    CSM_SMS_REPORT_TYPE_NONE = 0,
    /* Delivery Ack. Currently used for 3GPP2 SMS. */
    CSM_SMS_REPORT_TYPE_DELIVERY_ACK = 1,
    /* User Ack. Currently used for 3GPP2 SMS. */
    CSM_SMS_REPORT_TYPE_USER_ACK = 2,
} CSM_SmsReportType;
/*
 ******************
 * Service enumeration
 ******************
 */
/* Service access network type enumeration */
typedef enum {
    CSM_SERVICE_ACCESS_TYPE_NONE,
    CSM_SERVICE_ACCESS_TYPE_3GPP_GERAN,
    CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_FDD,
    CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_TDD,
    CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_FDD,
    CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_TDD,
    CSM_SERVICE_ACCESS_TYPE_IEEE_802_11,
} CSM_ServiceNetworkAccessType;

typedef enum {
    CSM_SERVICE_APP_AUTHTYPE_AUTO = 0, /* only auto detect the first 2 types */
    CSM_SERVICE_APP_AUTHTYPE_DIGEST = 1,
    CSM_SERVICE_APP_AUTHTYPE_GAA_DIGEST = 2,
    CSM_SERVICE_APP_AUTHTYPE_GAA_DIGEST_UNKNOWN = 3
    /* future supported Ua security protocol identifiers to be added here */
}  CSM_ServiceAppAuthType;


typedef enum {
    CSM_SERVICE_STATE_INACTIVE = 0, // similar +CREG:0
    CSM_SERVICE_STATE_ACTIVE,       // similar +CREG:1
    CSM_SERVICE_STATE_IN_PROGRESS,  // similar +CREG:2
    CSM_SERVICE_STATE_FAILED,       // similar +CREG:3
    CSM_SERVICE_STATE_UNKNOWN,      // similar +CREG:4
    CSM_SERVICE_STATE_ROAMING,      // similar +CREG:5
    CSM_SERVICE_STATE_DEREGISTERING = 10, /* Specific state to indicte we are doing deregister. No mapping CREG value. */
} CSM_ServiceState;

/* Service notification type. */
typedef enum {
    /* Indicate the alias URI provided by network. */
    CSM_SERVICE_NOTIFY_ALIAS_URI = 1,
} CSM_ServiceNotifyType;

/* The enumeration of registration capabilities */
typedef enum {
    CSM_REG_CAPS_NONE                     = 0x0000,
    CSM_REG_CAPS_IP_VOICE_CALL            = 0x0002, /* voice call */
    CSM_REG_CAPS_IP_VIDEO_CALL            = 0x0004, /* video call */
    CSM_REG_CAPS_SMS                      = 0x0010, /* SMS over IP */
} CSM_RegCapabilities;

/*
 ******************
 * Radio enumeration
 ******************
 */
typedef enum {
    CSM_RADIO_NETWORK_TYPE_NONE,
    CSM_RADIO_NETWORK_TYPE_LTE,
    CSM_RADIO_NETWORK_TYPE_WIFI,
    CSM_RADIO_NETWORK_TYPE_LTE_SS,
} CSM_RadioNetworkType;

/*
 ******************
 * SUPSRV enumeration
 ******************
 */
typedef enum {
    CSM_SUPSRV_OIR_DEFAULT = 0,
    CSM_SUPSRV_OIR_INVOCATION = 1,
    CSM_SUPSRV_OIR_SUPPRESSION = 2,
    CSM_SUPSRV_OIR_QUERY = 3
} CSM_SupSrvOirEventStatus;

typedef enum {
    CSM_SUPSRV_DISABLE = 0,
    CSM_SUPSRV_ENABLE,
    CSM_SUPSRV_QUERY,
    CSM_SUPSRV_REGISTRATION,
    CSM_SUPSRV_ERASURE
} CSM_SupSrvGenReqStatus;

typedef union {
    CSM_SupSrvOirEventStatus  oirReqStatus;
    CSM_SupSrvGenReqStatus    genReqStatus;
} CSM_SupSrvReqStatus;

typedef enum {
   CSM_EVENT_SUPSRV_CB_MODE_BAOC = 0, /* Barring of All Outgoing Calls */
   CSM_EVENT_SUPSRV_CB_MODE_BOIC,     /* Barring of Outgoing International Calls */
   CSM_EVENT_SUPSRV_CB_MODE_BAIC,     /* Barring of All Incoming Callsl */
   CSM_EVENT_SUPSRV_CB_MODE_BICR,     /* Barring of Incoming Calls - When Roaming */
   CSM_EVENT_SUPSRV_CB_MODE_BOIC_EXHC,/* Bar Outgoing International Calls except to Home Country */
   CSM_EVENT_SUPSRV_CB_MODE_QUERY
} CSM_SupSrvCbMode;

typedef enum {
   CSM_EVENT_SUPSRV_CF_MODE_UNCONDITION = 0,   /* Communication Forwarding Unconditional */
   CSM_EVENT_SUPSRV_CF_MODE_BUSY,      /* Communication Forwarding on Busy */
   CSM_EVENT_SUPSRV_CF_MODE_NOREPLY,   /* Communication Forwarding on No Reply */
   CSM_EVENT_SUPSRV_CF_MODE_NOTREACH,  /* Communication Forwarding on not Reachable */
   CSM_EVENT_SUPSRV_CF_MODE_NOTLOGIN,  /* Communication Forwarding on not Logged in */
   CSM_EVENT_SUPSRV_CF_MODE_TIME = 9   /* CF on time of the day, CMCC only */
} CSM_SupSrvCFMode;

typedef union {
    CSM_SupSrvCbMode cbMode;
    CSM_SupSrvCFMode cfMode;
} CSM_SupSrvMode;


typedef enum {
   CSM_SUPSRV_CMD_NONE = 0,
   CSM_SUPSRV_CMD_GET_SIMSERVS,
   CSM_SUPSRV_CMD_GET_OIP,
   CSM_SUPSRV_CMD_OIP_OPERATION,
   CSM_SUPSRV_CMD_GET_OIR,
   CSM_SUPSRV_CMD_OIR_OPERATION,
   CSM_SUPSRV_CMD_GET_CD,
   CSM_SUPSRV_CMD_CD_OPERATION,
   CSM_SUPSRV_CMD_GET_CBIC,
   CSM_SUPSRV_CMD_CBIC_OPERATION,
   CSM_SUPSRV_CMD_GET_CBICR,
   CSM_SUPSRV_CMD_CBICR_OPERATION,
   CSM_SUPSRV_CMD_GET_CBOG,
   CSM_SUPSRV_CMD_CBOG_OPERATION,
   CSM_SUPSRV_CMD_GET_CBOIC,
   CSM_SUPSRV_CMD_CBOIC_OPERATION,
   CSM_SUPSRV_CMD_GET_TIP,
   CSM_SUPSRV_CMD_TIP_OPERATION,
   CSM_SUPSRV_CMD_GET_TIR,
   CSM_SUPSRV_CMD_TIR_OPERATION,
   CSM_SUPSRV_CMD_GET_CW,
   CSM_SUPSRV_CMD_CW_OPERATION,
   CSM_SUPSRV_CMD_LAST
} CSM_SupSrvCmdType;

/* CF NoReplyTimer */
typedef enum {
    CSM_SUPSRV_NOREPLY_NONE = 0,
    CSM_SUPSRV_NOREPLY_MIN = 5,
    CSM_SUPSRV_NOREPLY_MAX = 180
} CSM_SupSrvNoReplyRange;

/* General Response status */
typedef enum {
    CSM_SUPSRV_RES_DISABLE = 0,
    CSM_SUPSRV_RES_ENABLE = 1
} CSM_SupSrvGenResStatus;

typedef union {
    CSM_SupSrvOirEventStatus  oirResStatus;
    CSM_SupSrvGenResStatus    genResStatus;
} CSM_SupSrvResStatus;

/* supsrv provision status/mode */
typedef enum {
    CSM_SUPSRV_OIR_NOT_PROVISIONED = 0,
    CSM_SUPSRV_OIR_PERM_MODE = 1,
    CSM_SUPSRV_OIR_UNKNOWN = 2,
    CSM_SUPSRV_OIR_TEMP_MODE_RESTRICTED = 3,
    CSM_SUPSRV_OIR_TEMP_MODE_ALLOWED = 4,
} CSM_SupSrvOirProvision;

/* supsrv provision status/mode */
typedef enum {
    CSM_SUPSRV_TIR_NOT_PROVISIONED = 0,
    CSM_SUPSRV_TIR_PERM_MODE = 1,
    CSM_SUPSRV_TIR_UNKNOWN = 2,
    CSM_SUPSRV_TIR_TEMP_MODE_RESTRICTED = 3,
    CSM_SUPSRV_TIR_TEMP_MODE_ALLOWED = 4,
} CSM_SupSrvTirProvision;

typedef enum {
    CSM_SUPSRV_NOT_PROVISIONED = 0,
    CSM_SUPSRV_PROVISIONED = 1,
    CSM_SUPSRV_UNKNOWN = 2,
} CSM_SupSrvGenProvision;

typedef union {
    CSM_SupSrvOirProvision  oirProv;
    CSM_SupSrvTirProvision  tirProv;
    CSM_SupSrvGenProvision  genProv;
} CSM_SupSrvProvision;

typedef enum {
    CSM_PARTICIPANT_STATUS_CONNECTED = 0,
    CSM_PARTICIPANT_STATUS_DISCONNECTED,
    CSM_PARTICIPANT_STATUS_ONHOLD,
    CSM_PARTICIPANT_STATUS_MUTEDVIAFOCUS,
    CSM_PARTICIPANT_STATUS_PENDING,
    CSM_PARTICIPANT_STATUS_ALERTING,
    CSM_PARTICIPANT_STATUS_DIALING,
    CSM_PARTICIPANT_STATUS_DISCONNECTING,
} CSM_ParticipantStatus;

/*
 * CSSI/CSSU report supported code
 * +CSSI
 *      2 call has been forwarded
 *      3 call is waiting
 * +CSSU
 *      0 this is a forwarded call
 *      2 call has been put on hold
 *      3 call has been retrieved
 *      4 multiparty call entered
 */
typedef enum {
    CSM_SUPSRV_CALL_NONE = 0,
    CSM_SUPSRV_MO_CALL_CODE_BASE = 0,
    CSM_SUPSRV_MO_CALL_BEING_FORWARDED = 1 << (2+CSM_SUPSRV_MO_CALL_CODE_BASE),
    CSM_SUPSRV_MO_CALL_IS_WAITING      = 1 << (3+CSM_SUPSRV_MO_CALL_CODE_BASE),
    CSM_SUPSRV_MO_CALL_VIRTUAL_RING    = 1 << (4+CSM_SUPSRV_MO_CALL_CODE_BASE),
    CSM_SUPSRV_MO_CALL_MEDIA_CONVERT   = 1 << (5+CSM_SUPSRV_MO_CALL_CODE_BASE),
    CSM_SUPSRV_MT_CALL_CODE_BASE = 16,
    CSM_SUPSRV_MT_CALL_IS_FORWARDED    = 1 << (0+CSM_SUPSRV_MT_CALL_CODE_BASE),
    CSM_SUPSRV_MT_CALL_IS_HELD         = 1 << (2+CSM_SUPSRV_MT_CALL_CODE_BASE),
    CSM_SUPSRV_MT_CALL_IS_UNHELD       = 1 << (3+CSM_SUPSRV_MT_CALL_CODE_BASE),
    CSM_SUPSRV_MT_CALL_JOINED_CONFCALL = 1 << (4+CSM_SUPSRV_MT_CALL_CODE_BASE),
    /* Indicate the MT call is called to alias URI. */
    CSM_SUPSRV_MT_CALL_ALIAS_URI       = 1 << (5+CSM_SUPSRV_MT_CALL_CODE_BASE),
    CSM_SUPSRV_MT_CALL_MEDIA_CONVERT   = 1 << (6+CSM_SUPSRV_MT_CALL_CODE_BASE),
    /* Indicate the status of participant */
    CSM_SUPSRV_CALL_PARTICIPANT_STATUS = 1 << (7+CSM_SUPSRV_MT_CALL_CODE_BASE),
} CSM_SupsrvCallNotice;

/*
 ******************
 * Ussd enumeration
 ******************
 */
/* Encoding type for USSD. */
typedef enum {
    CSM_USSD_ENCTYPE_ASCII = 0,
    CSM_USSD_ENCTYPE_UCS2,
    CSM_USSD_ENCTYPE_UTF8,
} CSM_UssdEncType;


/*
 ******************************************************************************
 * Structure definitions
 ******************************************************************************
 */
typedef struct {
    CSM_CallAddressType   type;
    CSM_CallNumberType    numberType;
    char                  number[CSM_NUMBER_STRING_SZ + 1];
    char                  alpha[CSM_ALPHA_STRING_SZ + 1];
} CSM_Address;

typedef struct {
    int     callId;
    int     audioRtpPort; /* port info of audio */
    int     videoRtpPort; /* port info of video */
} CSM_RadioResourceMedia;

typedef struct {
    int     callId;
    int     audioReady;
    int     videoReady;
} CSM_ResourceStatus;
/*
 **************************************
 *  CSM input event structure
 **************************************
 */
typedef struct {
    CSM_CallEventType   type;
    CSM_CallReason      reason;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    int                 extraArgument;
    int                 isEmergency;
    CSM_EmergencyType   emergencyType;
    CSM_CallCidType     cidType;
    int                 isRsrcReady;
    CSM_CallNegStatus   negStatus;
    CSM_CallSessionType callSessionType; /*Indicate this call is a video or voice call.*/
    union {
        char            remoteAddress[CSM_ALPHA_STRING_SZ + 1];
        char            remoteParticipants[CSM_EVENT_LONG_STRING_SZ];
        int             errorCode;
        char            digit;
        int             callIndex;
        int             aecEnable;
        struct {
            int         tx;
            int         rx;
        } gain;
        CSM_ResourceStatus resourceStatus;
    } u;
} CSM_InputCall;

typedef struct {
    CSM_SmsReason   reason;
    CSM_SmsType     type;
    char            reasonDesc[CSM_EVENT_STRING_SZ + 1];
    char            remoteAddress[CSM_ALPHA_STRING_SZ + 1];
    char            message[CSM_SMS_STRING_SZ + 1];
    char            pdu[CSM_SMS_STRING_SZ + 1];
    int             msgLen;
    char            smsc[CSM_ALPHA_STRING_SZ + 1];
} CSM_InputSms;

typedef struct {
    char appName[CSM_EVENT_STRING_SZ];       /* e.g. xcap */
    char appUri[CSM_FQDN_STRING_SZ];         /* e.g. xcap root uri */
    CSM_ServiceAppAuthType appAuthType;
    char appAuthName[CSM_EVENT_STRING_SZ];   /* for digest mode only */ 
    char appAuthSecret[CSM_EVENT_STRING_SZ]; /* for digest mode only */
} CSM_ServiceAppObj;

typedef struct {
    char bsf[CSM_FQDN_STRING_SZ];
    CSM_ServiceAppObj xcapAppInfo;
} CSM_ServiceAppsProvision;

typedef struct {
    CSM_ServiceReason reason;
    char              reasonDesc[CSM_EVENT_STRING_SZ + 1];
    union {
        struct {
            CSM_ServiceNetworkAccessType type;
            char                         id[CSM_CGI_STRING_SZ + 1];
        } cgi;
        struct {
            char      response[CSM_AKA_RESP_STRING_SZ]; /* size is 16 */
            int       resLength;
            char      auts[CSM_AKA_AUTS_STRING_SZ];     /* size is 14 */
            char      ik[CSM_AKA_IK_STRING_SZ];         /* size is 16 */
            char      ck[CSM_AKA_CK_STRING_SZ];         /* size is 16 */
        } aka;
        struct {
            int       protectedPort;        /* protected port */
            int       protectedPortPoolSz;  /* protected port pool size */
            int       spi;                  /* SPI start */
            int       spiPoolSz;            /* spi pool size */
        } ipsec;
        struct {
            int       sip;
            int       audio;                /* audio rtp port */
            int       audioPoolSize;        /* audio rtp pool size */
            int       video;                /* video rtp port */
            int       videoPoolSize;        /* video rtp pool size */
        } port;
        struct {
            int       coderNum;
            int       coderPayloadType[CSM_CODER_NUM];
            char      coderName[CSM_CODER_NUM][CSM_CODER_STRING_SZ + 1];
            char      coderDescription[CSM_CODER_NUM]
                        [CSM_CODER_DESCRIPTION_STRING_SZ + 1];
        } coder;
                    /*
                     * Accecpt multiple P-CSCF addresses up to 5.
                     * Seperate muliple P-CSCF address by ','.
                     * Example:
                     *     pcscf1.home1.net, pcscf2.home1.net, pcscf3.home1.net
                     */
        char        pcscf[CSM_EVENT_LONG_STRING_SZ + 1];
        char        domain[CSM_EVENT_STRING_SZ + 1];
        char        impi[CSM_EVENT_STRING_SZ + 1];
        char        impu[CSM_EVENT_STRING_SZ + 1];
        char        password[CSM_EVENT_STRING_SZ + 1];
        char        imeiUri[CSM_EVENT_STRING_SZ + 1];
        char        instanceId[CSM_INSTANCE_STRING_SZ + 1];
        char        audioConfServer[CSM_EVENT_STRING_SZ + 1];
        char        videoConfServer[CSM_EVENT_STRING_SZ + 1];
        CSM_ServiceAppsProvision appsProvision;
        int         extraArgument;
    } u;
} CSM_InputService;

typedef struct {
    CSM_RadioReason         reason;
    CSM_RadioNetworkType    networkType;
    char                    address[CSM_IP_ADDRESS_STRING_SZ];
    char                    infcName[CSM_EVENT_STRING_SZ + 1];
    int                     isEmergencyFailoverToCs;
    int                     isEmergencyRegRequired;
} CSM_InputRadio;

typedef struct {
    int                 noReplyTimer;
    char                mediaType[CSM_SUPSRV_PARAM_STRING_SZ];
    char                timeRangeOfTheDay[CSM_SUPSRV_PARAM_STRING_SZ]; /* "HH:MM,HH:MM" */
    char                cfwNumber[CSM_URI_STRING_SZ + 1];
    CSM_CallAddressType addrType; /* type of address octet in integer format 
                                   * (refer 3GPP TS 24.008 subclause 10.5.4.7);
                                   * default 145 when dialling string includes
                                   * international access code character "+",
                                   * otherwise 129
                                   */
} CSM_SupSrvRuleParams;

typedef struct {
    CSM_SupSrvReason         reason;
    char                     reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_SupSrvMode           mode;
    CSM_SupSrvReqStatus      status;
    CSM_SupSrvRuleParams     ruleParams;
} CSM_InputSupSrv;

typedef struct {
    CSM_UssdReason    reason;
    char              reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_UssdEncType   encType; /* CSM_USSD_ENCTYPE_ASCII: ASCII encoding. CSM_USSD_ENCTYPE_UCS2: UCS-2 encoding on message field */
    char              message[CSM_USSD_STRING_SZ + 1]; /*HexString encoding */
} CSM_InputUssd;

/*
 * This is a event send to CSM to signal some sort of command, response, 
 * or event from the CS or IP layers.
 */
typedef struct {
    CSM_EventType type; /* Type of message */
    union {
        CSM_InputCall    call;
        CSM_InputSms     sms;
        CSM_InputService service;
        CSM_InputRadio   radio;
        CSM_InputSupSrv  supSrv;
        CSM_InputUssd    ussd;
    } evt;
} CSM_InputEvent;

/*
 **************************************
 *  CSM output event struct
 **************************************
 */

/*
 * CLCC responses are of the form:
 *
 * +CLCC: <index>,<dir>,<state>,<mode>,<mpty>[,<number>,<type>[,<alpha>]]
 * <index> is the index of the call
 * <dir> is the direction of the mobile originated or terminated
 * <state> is the state of the call
 * <mode> is a number representing whether the call is voice, data, etc.
 * <mpty> is a flag indicating whether the call is multi-party
 * <number> is the phone number (address)
 * <type> is the type of phone number
 * <alpha> is the CID name associated with the number.
 *
 */
typedef struct {
    int                  idx; /* The 'identifier or index of the call. */
    CSM_CallDirection    direction; /* The direction of the call e.g. mobile originated or terminated. */
                         /* Negotiation status. */
    CSM_CallNegStatus    negStatus;
    CSM_CallStatus       status;
    CSM_CallState        state; /* The state of the call. */
    CSM_CallMode         mode;
    CSM_CallMultiParty   isMultiParty; /* 0 = is a single call.  >=1 = is a multiparty call. */
    CSM_CallNumberType   numberType;
    char                 number[CSM_NUMBER_STRING_SZ + 1]; /* The address of the remote party. */
    char                 normalizedAddress[CSM_ALPHA_STRING_SZ + 1];
    CSM_CallAddressType  type; /* The 'type' of address, I.e. ton */
    char                 alpha[CSM_ALPHA_STRING_SZ + 1];
    int                  callSessionType; /*Indicate the call is video or voice call.*/
    char                 coderSdpMd[CSM_CODER_DESCRIPTION_STRING_SZ + 1];   
} CSM_CallSummary;

typedef struct {
    int               numCalls;
    CSM_CallSummary   calls[CSM_EVENT_MAX_CALL_LIST_SIZE];
} CSM_CallReport;

typedef struct {
    char                  number[CSM_NUMBER_STRING_SZ + 1]; /* The address of the remote party. */
    CSM_CallAddressType   type;                             /* The 'type' of address. */
    char                  alpha[CSM_ALPHA_STRING_SZ + 1];
    int                 callSessionType; /*Indicate the call is video or voice call.*/
} CSM_CallClip;

typedef struct {
    CSM_SrvccResult       result;  /* The result of SRVCC related command. */
    int                   callId;  /* The call ID for this SRVCC result. */
} CSM_CallSrvcc;

typedef struct {
    CSM_CallCauseCode   cause;
    char                hiIndex[CSM_HISTORY_INDEX_STRING_SZ]; /* hi-index, e.g. "1.1" */
    char                number[CSM_NUMBER_STRING_SZ + 1]; /* The address of the party. */
    CSM_CallAddressType type; /* The 'type' of address. */
    char                alpha[CSM_ALPHA_STRING_SZ + 1];
} CSM_CallHistory;

typedef struct {
    int             idx; /* The identifier or index of the main call */
    int             supsrvNotification; /* valid values: CSM_SupsrvCallNotice */
    int             numHistories;
    CSM_CallHistory callHistories[CSM_HISTORY_MAX_LIST_SIZE];
    struct {
        CSM_ParticipantStatus   status;
        char                    number[CSM_NUMBER_STRING_SZ + 1];
    } participant;
} CSM_CallSupsrvInfo;

/*
 * CSM_OutputCall
 *
 * Structure containing report information about active calls in the system
 */
typedef struct {
    CSM_OutputReason   reason;
    char               reasonDesc[CSM_EVENT_STRING_SZ + 1];
    union {
        char           address[CSM_NUMBER_STRING_SZ + 1];
        CSM_CallReport report;
        int            errorCode;
        CSM_CallClip   clipReport;
        CSM_CallSrvcc  srvcc;
        int            callIdx; /* ccidx for AT+CDU response. */
        char           digit;
        CSM_CallSupsrvInfo supsrvInfo;
        CSM_RadioResourceMedia resourceMedia; /* Indicate the resource status */
    } u;
} CSM_OutputCall;

/*
 * CSM_OutputSms
 *
 * Structure containing information about SMS
 */
typedef struct {
    CSM_OutputReason  reason;
    char              reasonDesc[CSM_EVENT_STRING_SZ + 1];
    char              address[CSM_ALPHA_STRING_SZ + 1];
    unsigned short    mr; /* The <mr> 'message reference' */
    union {
        struct {
        char              body[CSM_SMS_STRING_SZ + 1];
        int               len;
        int               numOfMessage; /* Number of unread voice mail. */
        /* 3GPP TS 23.040 TP-Service-Centre-Time-Stamp in time-string format */
        char              scts[CSM_ALPHA_STRING_SZ + 1];
        CSM_SmsDcs        dcs; /* Data Coding Scheme */
        } msg;
        CSM_SmsErrorCode  errorCode;
        CSM_SmsReportType reportType; /*
                                       * Indicate report type when SMS report
                                       * received.
                                       */
    } u;
} CSM_OutputSms;

/*
 * CSM_OutputService
 *
 * Structure containing service info like Network Registration
 */
typedef struct {
    CSM_OutputReason     reason;
    char                 reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_ServiceState     state;
    int                  errorCode;
    int                  isEmergency;
    union {
        struct {
            char         akaRand[CSM_AKA_RAND_STRING_SZ];
            char         akaAutn[CSM_AKA_AUTN_STRING_SZ];
        } aka;
        struct {
            char         localIpAddress[CSM_IP_ADDRESS_STRING_SZ + 1];
            char         remoteIpAddress[CSM_IP_ADDRESS_STRING_SZ + 1];
            int          portUc; /* UE client port */
            int          portUs; /* UE server port */
            int          portPc; /* Proxy client port */
            int          portPs; /* proxy server port */
            int          spiUc;  /* SPI for inbound SA to UE client port */
            int          spiUs;  /* SPI for inbound SA to UE server port */
            int          spiPc;  /*
                                  * SPI for outbound SA to proxy client
                                  * port.
                                  */
            int          spiPs;  /*
                                  * SPI for outbound SA to proxy server
                                  * port.
                                  */
        } ipsec;
        struct {
            CSM_ServiceNotifyType type;
        } notify;
    } u;
} CSM_OutputService;

/*
 * CSM_OutputSupSrv
 *
 * Structure containing information about SUPSRV
 */
typedef struct {
    CSM_OutputReason    reason;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_SupSrvCmdType   cmdType;
    CSM_SupSrvMode      mode;
    CSM_SupSrvResStatus queryEnb;
    CSM_SupSrvProvision prov;
    CSM_SupSrvErrorCode errorCode;
    union {
        CSM_SupSrvRuleParams ruleParams;
    } u;
} CSM_OutputSupSrv;

/*
 * CSM_OutputUssd
 *
 * Structure containing information about USSD
 */
typedef struct {
    CSM_OutputReason    reason;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    CSM_UssdEncType     encType; /* always CSM_USSD_ENCTYPE_UCS2. UCS-2 encoding on message field */
    char                message[CSM_USSD_STRING_SZ + 1]; /*HexString encoding */
    CSM_UssdErrorCode   errorCode;
} CSM_OutputUssd;

/*
 * This is a event send to CSM to signal some sort of command, response,
 * or event from the CS or IP layers.
 */
typedef struct {
    CSM_EventType type; /* Type of message */
    union {
        CSM_OutputCall    call; /* Details of the call. */
        CSM_OutputSms     sms;
        CSM_OutputService service; /* Network Registration output */
        CSM_OutputSupSrv  supSrv;
        CSM_OutputUssd    ussd;
    } evt;
} CSM_OutputEvent;

#ifdef __cplusplus
}
#endif

#endif //__CSM_EVENT_H__

