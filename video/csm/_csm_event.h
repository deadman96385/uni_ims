/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 21411 $ $Date: 2013-07-15 09:07:07 +0800 (Mon, 15 Jul 2013) $
 */

#ifndef __CSM_EVENT_H_
#define __CSM_EVENT_H_

#include <csm_event.h>
#include <isi.h>
#include <isi_errors.h>
#include <osal.h>

#ifdef INCLUDE_SUPSRV
#include <xcap.h>
#include <supsrv.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Queue and IPC definitions */
#define CSM_PRIVATE_INPUT_EVT_MSGQ_LEN      (8)
#ifdef VPORT_4G_PLUS_APROC
/*
 * Define different message queue name for CSM running in application processor.
 */
#define CSM_PRIVATE_INPUT_EVT_MSGQ_NAME     "aproc.csm.private.evt.in.q"
#else
#define CSM_PRIVATE_INPUT_EVT_MSGQ_NAME     "csm.private.evt.in.q"
#endif

/*
 * Enumeration definitions
 */

typedef enum {
    CSM_INPUT_EVT_TYPE_EXT, // external csm input events, AT...
    CSM_INPUT_EVT_TYPE_INT  // internal csm input events, ISI, XCAP...
} CSM_InputEvtType;

/* What type of event this is. */
typedef enum {
    CSM_PRIVATE_EVENT_TYPE_CALL = 0,
    CSM_PRIVATE_EVENT_TYPE_SMS,
    CSM_PRIVATE_EVENT_TYPE_SERVICE,
    CSM_PRIVATE_EVENT_TYPE_RADIO,
    CSM_PRIVATE_EVENT_TYPE_SUPSRV,
    CSM_PRIVATE_EVENT_TYPE_USSD,
    CSM_PRIVATE_EVENT_TYPE_INTERNAL,
} CSM_PrivateEventType;

/* Transport protocol enumeration */
typedef enum {
    CSM_TRASPORT_PROTO_NONE,
    CSM_TRASPORT_PROTO_UDP,
    CSM_TRASPORT_PROTO_TCP,
    CSM_TRASPORT_PROTO_MSRP,
    CSM_TRASPORT_PROTO_MSRP_O_TLS,
    CSM_TRASPORT_PROTO_TLS,
    CSM_TRASPORT_PROTO_RT_MEDIA_RTP,
    CSM_TRASPORT_PROTO_RT_MEDIA_SRTP,
    CSM_TRASPORT_PROTO_LAST = CSM_TRASPORT_PROTO_TLS,
} CSM_TransportProto;

/*
 * The following declarations are from ISI or XCAP to CSM
 */

/* service events from ISI send as input to CSM */
typedef enum {
    CSM_SERVICE_REASON_PROTOCOL_REGISTERED = 0,
    CSM_SERVICE_REASON_PROTOCOL_DIED,
    CSM_SERVICE_REASON_SERVICE_INIT_OK,
    CSM_SERVICE_REASON_SERVICE_INIT_FAILED,
    CSM_SERVICE_REASON_SERVICE_ACTIVE,
    CSM_SERVICE_REASON_SERVICE_ACTIVATING,
    CSM_SERVICE_REASON_SERVICE_INACTIVE,
    CSM_SERVICE_REASON_SERVICE_HANDOFF,
    CSM_SERVICE_REASON_SERVICE_RETRY,
    CSM_SERVICE_REASON_SERVICE_AUTH_FAIL,
    CSM_SERVICE_REASON_ISIM_AKA_CHALLENGE,
    CSM_SERVICE_REASON_IPSEC_SETUP,
    CSM_SERVICE_REASON_IPSEC_RELEASE,
    CSM_SERVICE_REASON_RCS_PROVISIONING,
    CSM_SERVICE_REASON_ACTIVATE,
    CSM_SERVICE_REASON_DEACTIVATE,
} CSM_IsiServiceReason;

typedef struct {
    CSM_IsiServiceReason    reason;
    char                    reasonDesc[CSM_EVENT_STRING_SZ + 1];
    int                     protocol;
    int                     serviceId;
} CSM_InputIsiService;

/* call events from ISI send as input to CSM */
typedef enum {
    CSM_CALL_REASON_EVT_START,
    CSM_CALL_REASON_EVT_NEW_INCOMING = CSM_CALL_REASON_EVT_START,
    CSM_CALL_REASON_EVT_TRYING,
    CSM_CALL_REASON_EVT_ACKNOWLEDGED,
    CSM_CALL_REASON_EVT_FAILED,
    CSM_CALL_REASON_EVT_DISCONNECT,
    CSM_CALL_REASON_EVT_HOLD,
    CSM_CALL_REASON_EVT_RESUME,
    CSM_CALL_REASON_EVT_ACCEPTED,
    CSM_CALL_REASON_EVT_REJECTED,
    CSM_CALL_REASON_EVT_VCC_HANDOFF,
    CSM_CALL_REASON_EVT_DIGIT_DONE,
    CSM_CALL_REASON_EVT_XFER_FAILED,
    CSM_CALL_REASON_EVT_XFER_DONE,
    CSM_CALL_REASON_EVT_PARTICIPANT_INFO,
    CSM_CALL_REASON_EVT_MODIFY,
    CSM_CALL_REASON_EVT_MODIFY_COMPLETED,
    CSM_CALL_REASON_EVT_MODIFY_FAILED,
    CSM_CALL_REASON_EVT_BEING_FORWARDED,
    CSM_CALL_REASON_EVT_SRVCC_START,
    CSM_CALL_REASON_EVT_SRVCC_SUCCESS,
    CSM_CALL_REASON_EVT_SRVCC_FAILURE,
    CSM_CALL_REASON_EVT_ACCEPT_ACK,
    CSM_CALL_REASON_EVT_SERVICE_ACTIVE,
    CSM_CALL_REASON_EVT_SERVICE_INACTIVE,
    CSM_CALL_REASON_EVT_DTMFDECT_OOB,
    CSM_CALL_REASON_EVT_DTMFDECT_LEADING,
    CSM_CALL_REASON_EVT_DTMFDECT_TRAILING,
    CSM_CALL_REASON_EVT_VIDEO_REQUEST_KEY,
    CSM_CALL_REASON_EVT_EARLY_MEDIA,
} CSM_IsiCallReason;

typedef struct {
    CSM_IsiCallReason   reason;
    CSM_CallEventType   type;
    ISI_Id              id;
    ISI_Id              serviceId;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    int                 extraArgument;
    union {
        char            digit;
    } u;
} CSM_InputIsiCall;

/* SMS events from ISI send as input to CSM */
typedef enum {
    CSM_SMS_REASON_EVT_SEND_OK,
    CSM_SMS_REASON_EVT_SEND_ERROR,
    CSM_SMS_REASON_EVT_NEW_INCOMING,
    CSM_SMS_REASON_EVT_MWI_ACTIVE,
    CSM_SMS_REASON_EVT_MWI_INACTIVE,
    CSM_SMS_REASON_EVT_SENT_TIMEOUT
} CSM_IsiSmsReason;

typedef struct {
    CSM_IsiSmsReason    reason;
    ISI_Id              id;
    ISI_Id              serviceId;
    CSM_SmsType         type;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    char                remoteAddress[CSM_ALPHA_STRING_SZ + 1];
} CSM_InputIsiSms;

/* USSD events from ISI send as input to CSM */
typedef enum {
    CMS_USSD_REASON_EVT_SEND_ERROR,
    CSM_USSD_REASON_EVT_SENT_USSD,
    CSM_USSD_REASON_EVT_REQUEST_USSD,
    CSM_USSD_REASON_EVT_NOTIFY_USSD,
    CSM_USSD_REASON_EVT_DISCONNECT_USSD,
} CSM_IsiUssdReason;

typedef struct {
    CSM_IsiUssdReason   reason;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    ISI_Id              id;
    ISI_Id              serviceId;
    CSM_UssdEncType     encType;
} CSM_InputIsiUssd;

/* CSM internal event */
typedef enum {
    CMS_INT_REASON_EVT_RINGING_TIMEOUT_CB,
    CMS_INT_REASON_EVT_SRVCC_SUCCESS_TIMEOUT_CB,
} CSM_InternalReason;

typedef struct {
    CSM_PrivateEventType    type;
    union {
        CSM_InputIsiService service;
        CSM_InputIsiCall    call;
        CSM_InputIsiSms     sms;
        CSM_InputIsiUssd    ussd;
#ifdef INCLUDE_SUPSRV
        SUPSRV_XcapEvt      xcap;
#endif
    } evt;
} CSM_PrivateInputEvt;


/*
 * The following declarations are CSM internal using
 */

typedef enum {
    CSM_SERVICE_EVT_REASON_INVALID = -1,
    /* map to service events from ISI */
    CSM_SERVICE_EVT_REASON_PROTOCOL_REGISTERED = 0,
    CSM_SERVICE_EVT_REASON_PROTOCOL_DIED,
    CSM_SERVICE_EVT_REASON_SERVICE_INIT_OK,
    CSM_SERVICE_EVT_REASON_SERVICE_INIT_FAILED,
    CSM_SERVICE_EVT_REASON_SERVICE_ACTIVE,
    CSM_SERVICE_EVT_REASON_SERVICE_ACTIVATING,
    CSM_SERVICE_EVT_REASON_SERVICE_INACTIVE,
    CSM_SERVICE_EVT_REASON_SERVICE_HANDOFF,
    CSM_SERVICE_EVT_REASON_SERVICE_RETRY,
    CSM_SERVICE_EVT_REASON_SERVICE_AUTH_FAIL,
    CSM_SERVICE_EVT_REASON_ISIM_AKA_CHALLENGE,
    CSM_SERVICE_EVT_REASON_IPSEC_SETUP,
    CSM_SERVICE_EVT_REASON_IPSEC_RELEASE,
    CSM_SERVICE_EVT_REASON_RCS_PROVISIONING,
    CSM_SERVICE_EVT_REASON_ACTIVATE,
    CSM_SERVICE_EVT_REASON_DEACTIVATE,

    /* map to service events from external */
    CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SUCCESS,
    CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE,
    CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE,
    CSM_SERVICE_EVT_REASON_SET_IMPU,
    CSM_SERVICE_EVT_REASON_SET_IMPI,
    CSM_SERVICE_EVT_REASON_SET_DOMAIN,
    CSM_SERVICE_EVT_REASON_SET_PASSWORD,
    CSM_SERVICE_EVT_REASON_SET_PCSCF,
    CSM_SERVICE_EVT_REASON_SET_EMGCY_PCSCF,
    CSM_SERVICE_EVT_REASON_SET_PORTS,
    CSM_SERVICE_EVT_REASON_SET_IPSEC,
    CSM_SERVICE_EVT_REASON_UPDATE_CGI,
    CSM_SERVICE_EVT_REASON_SET_IMEI_URI,
    CSM_SERVICE_EVT_REASON_APPS_PROVISION,
    CSM_SERVICE_EVT_REASON_SET_INSTANCE_ID,
    CSM_SERVICE_EVT_REASON_IMS_ENABLE,
    CSM_SERVICE_EVT_REASON_IMS_DISABLE,
    CSM_SERVICE_EVT_REASON_SET_REREGISTER_PERIOD,
    CSM_SERVICE_EVT_REASON_SET_RETRY_TIMER_PERIOD,
    CSM_SERVICE_EVT_REASON_SET_REG_CAPABILITIES,
    CSM_SERVICE_EVT_REASON_SET_CODERS,
    CSM_SERVICE_EVT_REASON_SET_AUDIO_CONF_SERVER,
    CSM_SERVICE_EVT_REASON_SET_VIDEO_CONF_SERVER,
} CSM_ServiceEvtReason;

typedef struct {
    CSM_ServiceEvtReason    reason;
    char                    reasonDesc[CSM_EVENT_STRING_SZ + 1];
    int                     protocol;
    int                     serviceId;
    union {
        struct {
            CSM_ServiceNetworkAccessType type;
            char                         id[CSM_CGI_STRING_SZ + 1];
        } cgi;
        struct {
            char            response[CSM_AKA_RESP_STRING_SZ]; /* size is 16 */
            int             resLength;
            char            auts[CSM_AKA_AUTS_STRING_SZ];     /* size is 14 */
            char            ik[CSM_AKA_IK_STRING_SZ];         /* size is 16 */
            char            ck[CSM_AKA_CK_STRING_SZ];         /* size is 16 */
        } aka;
        struct {
            int             protectedPort;        /* protected port */
            int             protectedPortPoolSz;  /* protected port pool size */
            int             spi;                  /* SPI start */
            int             spiPoolSz;            /* spi pool size */
        } ipsec;
        struct {
            int             sip;
            int             audio;                /* audio rtp port */
            int             audioPoolSize;        /* audio rtp pool size */
            int             video;                /* video rtp port */
            int             videoPoolSize;        /* video rtp pool size */
        } port;
        struct {
            int             coderNum;
            int             coderPayloadType[CSM_CODER_NUM];
            char            coderName[CSM_CODER_NUM][CSM_CODER_STRING_SZ + 1];
            char            coderDescription[CSM_CODER_NUM]
                                [CSM_CODER_DESCRIPTION_STRING_SZ + 1];
        } coder;
        char                pcscf[CSM_EVENT_LONG_STRING_SZ + 1];
        char                domain[CSM_EVENT_STRING_SZ + 1];
        char                impi[CSM_EVENT_STRING_SZ + 1];
        char                impu[CSM_EVENT_STRING_SZ + 1];
        char                password[CSM_EVENT_STRING_SZ + 1];
        char                imeiUri[CSM_EVENT_STRING_SZ + 1];
        char                instanceId[CSM_INSTANCE_STRING_SZ + 1];
        char                audioConfServer[CSM_EVENT_STRING_SZ + 1];
        char                videoConfServer[CSM_EVENT_STRING_SZ + 1];
        CSM_ServiceAppsProvision appsProvision;
        int                 extraArgument;
    } u;
} CSM_ServiceEvt;

typedef enum {
    CSM_CALL_EVT_REASON_INVALID = -1,
    /*
     * The following events are commands from AT interface send as input to CSM
     */
    CSM_CALL_EVT_REASON_AT_CMD_DIAL = 0,
    CSM_CALL_EVT_REASON_AT_CMD_REPORT,
    CSM_CALL_EVT_REASON_AT_CMD_ANSWER,
    CSM_CALL_EVT_REASON_AT_CMD_END,
    CSM_CALL_EVT_REASON_AT_CMD_SWAP,
    CSM_CALL_EVT_REASON_AT_CMD_END_ALL_ACTIVE,
    CSM_CALL_EVT_REASON_AT_CMD_END_ALL_HELD_OR_WAITING,
    CSM_CALL_EVT_REASON_AT_CMD_HOLD_ALL_EXCEPT_X,
    CSM_CALL_EVT_REASON_AT_CMD_DIGIT,
    CSM_CALL_EVT_REASON_AT_CMD_RELEASE_AT_X,
    CSM_CALL_EVT_REASON_AT_CMD_CONFERENCE,
    CSM_CALL_EVT_REASON_AT_CMD_SRVCC_START,
    CSM_CALL_EVT_REASON_AT_CMD_SRVCC_SUCCESS,
    CSM_CALL_EVT_REASON_AT_CMD_SRVCC_FAILED,
    CSM_CALL_EVT_REASON_AT_CMD_CONF_DIAL,
    CSM_CALL_EVT_REASON_AT_CMD_CONF_ADHOC,
    CSM_CALL_EVT_REASON_AT_CMD_CONF_MODIFY,
    CSM_CALL_EVT_REASON_AT_CMD_CONF_ADD_PARTY,
    /*
     * The following events are events from ISI send as input to CSM
     */
    CSM_CALL_EVT_REASON_START,
    CSM_CALL_EVT_REASON_NEW_INCOMING = CSM_CALL_EVT_REASON_START,
    CSM_CALL_EVT_REASON_TRYING,
    CSM_CALL_EVT_REASON_ACKNOWLEDGED,
    CSM_CALL_EVT_REASON_FAILED,
    CSM_CALL_EVT_REASON_DISCONNECT,
    CSM_CALL_EVT_REASON_HOLD,
    CSM_CALL_EVT_REASON_RESUME,
    CSM_CALL_EVT_REASON_ACCEPTED,
    CSM_CALL_EVT_REASON_REJECTED,
    CSM_CALL_EVT_REASON_VCC_HANDOFF,
    CSM_CALL_EVT_REASON_DIGIT_DONE,
    CSM_CALL_EVT_REASON_XFER_FAILED,
    CSM_CALL_EVT_REASON_XFER_DONE,
    CSM_CALL_EVT_REASON_PARTICIPANT_INFO,
    CSM_CALL_EVT_REASON_MODIFY,
    CSM_CALL_EVT_REASON_MODIFY_COMPLETED,
    CSM_CALL_EVT_REASON_MODIFY_FAILED,
    CSM_CALL_EVT_REASON_BEING_FORWARDED,
    CSM_CALL_EVT_REASON_SRVCC_START,
    CSM_CALL_EVT_REASON_SRVCC_SUCCESS,
    CSM_CALL_EVT_REASON_SERVICE_ACTIVE,
    CSM_CALL_EVT_REASON_SERVICE_INACTIVE,
    CSM_CALL_EVT_REASON_SRVCC_FAILURE,
    CSM_CALL_EVT_REASON_DTMFDECT_OOB,
    CSM_CALL_EVT_REASON_DTMFDECT_LEADING,
    CSM_CALL_EVT_REASON_DTMFDECT_TRAILING,
    /* An event indicating that another call disconnected. */
    CSM_CALL_EVT_REASON_OTHER_DISCONNECT,    
    /*
     * Call resource reservation event.
     */
    CSM_CALL_EVT_REASON_RESOURCE_INDICATION,
    CSM_CALL_EVT_REASON_ACCEPT_ACK,
    CSM_CALL_EVT_REASON_AT_CMD_MEDIA_CONTROL,
    CSM_CALL_EVT_REASON_VE_AEC_CMD,
    CSM_CALL_EVT_REASON_VE_GAIN_CTRL,
    CSM_CALL_EVT_REASON_VE_CN_GAIN_CTRL,
    CSM_CALL_EVT_REASON_VIDEO_REQUEST_KEY,
    CSM_CALL_EVT_REASON_EARLY_MEDIA,
    /* CSM internal event */
    CSM_CALL_EVT_REASON_RINGING_TIMEOUT_CB,
    CSM_CALL_EVT_REASON_SRVCC_SUCCESS_TIMEOUT_CB,
} CSM_CallEvtReason;

typedef struct {
    CSM_CallEventType   type;
    CSM_CallEvtReason   reason;
    ISI_Id              id;
    ISI_Id              serviceId;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    int                 extraArgument;
    int                 isEmergency;
    CSM_EmergencyType   emergencyType;
    CSM_CallCidType     cidType;
    int                 isRsrcReady;
    CSM_CallSessionType callSessionType;
    CSM_CallNegStatus   negStatus;
    union {
        char            remoteAddress[CSM_EVENT_STRING_SZ + 1];
        char            remoteParticipants[CSM_EVENT_LONG_STRING_SZ];
        int             errorCode;
        char            digit;
        int             callIndex;
        struct {
            vint                    isAdded;
            CSM_Address             address;
            CSM_ParticipantStatus   status;
        } remoteParty;
        int             aecEnable;
        struct {
            vint        tx;
            vint        rx;
        } gain;
        CSM_ResourceStatus resourceStatus;
    } u;
} CSM_CallEvt;

typedef CSM_CallEvt *CSM_EventCall_Ptr;

typedef enum {
    CMS_SMS_EVT_REASON_EVT_INVALID = -1,
    /* map to sms events from external */
    CSM_SMS_EVT_REASON_AT_CMD_SEND_MESSAGE = 0,
    CSM_SMS_EVT_REASON_SET_SMSC,

    /* map to sms events from ISI */
    CMS_SMS_EVT_REASON_EVT_SEND_OK,
    CMS_SMS_EVT_REASON_EVT_SEND_ERROR,
    CMS_SMS_EVT_REASON_EVT_NEW_INCOMING,
    CMS_SMS_EVT_REASON_EVT_MWI_ACTIVE,
    CMS_SMS_EVT_REASON_EVT_MWI_INACTIVE,
    CSM_SMS_EVT_REASON_EVT_SENT_TIMEOUT

} CSM_SmsEvtReason;

typedef struct {
    CSM_SmsEvtReason    reason;
    ISI_Id              id;
    ISI_Id              serviceId;
    CSM_SmsType         type;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    char                remoteAddress[CSM_ALPHA_STRING_SZ + 1];
    char                message[CSM_SMS_STRING_SZ + 1];
    char                pdu[CSM_SMS_STRING_SZ + 1];
    int                 msgLen;
    char                smsc[CSM_ALPHA_STRING_SZ + 1];
} CSM_SmsEvt;

typedef enum {
    CSM_USSD_EVT_REASON_INVALID = -1,
    CSM_USSD_EVT_REASON_AT_CMD_SEND_USSD,
    CSM_USSD_EVT_REASON_AT_CMD_REPLY_USSD,
    CSM_USSD_EVT_REASON_AT_CMD_DISCONNECT_USSD,
    CSM_USSD_EVT_REASON_EVT_SEND_ERROR,
    CSM_USSD_EVT_REASON_EVT_SENT_USSD,
    CSM_USSD_EVT_REASON_EVT_REQUEST_USSD,
    CSM_USSD_EVT_REASON_EVT_NOTIFY_USSD,
    CSM_USSD_EVT_REASON_EVT_DISCONNECT_USSD,
} CSM_UssdEvtReason;

typedef struct {
    CSM_UssdEvtReason   reason;
    char                reasonDesc[CSM_EVENT_STRING_SZ + 1];
    ISI_Id              id;
    ISI_Id              serviceId;
    /*
     * CSM_USSD_ENCTYPE_ASCII: ASCII encoding.
     * CSM_USSD_ENCTYPE_UCS2: UCS-2 encoding on message field
     */
    CSM_UssdEncType     encType;
    char                message[CSM_USSD_STRING_SZ + 1]; /*HexString encoding */
} CSM_UssdEvt;

#ifdef __cplusplus
}
#endif

#endif //__CSM_EVENT_H__
