/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30364 $ $Date: 2014-12-11 18:20:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _SAPP_H_
#define _SAPP_H_


#include <ezxml.h>
#include <mns.h>

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "simple/_simple_types.h"
#endif

/* Return codes. */
#define SAPP_OK                     (0)
#define SAPP_ERR                    (-1)

#define SAPP_MAX_ID                 (0x00FFFFFF)

/* Task Related settings */

#define SAPP_TASK_STACK_BYTES       (16384)

/* Queue and IPC definitions */
#define SAPP_MSGQ_LEN               (8)

/* Make queue name different for ASAPP and MSAPP */
#ifdef VPORT_4G_PLUS_APROC
#define SAPP_SIP_TASK_NAME                "aproc.Sapp"
#define SAPP_SIP_EVENT_QUEUE_NAME         "aproc.sapp.sip.event.q"
#define SAPP_SIMPLE_TMR_EVENT_QUEUE_NAME  "aproc.sapp.simple.tmr.event.q"
#define SAPP_IP_EVENT_QUEUE_NAME          "aproc.sapp.ip"
#else
#define SAPP_SIP_TASK_NAME                "Sapp"
#define SAPP_SIP_EVENT_QUEUE_NAME         "sapp.sip.event.q"
#define SAPP_SIMPLE_TMR_EVENT_QUEUE_NAME  "sapp.simple.tmr.event.q"
#define SAPP_IP_EVENT_QUEUE_NAME          "sapp.ip"
#endif

/* Generally used definitions */
#define SAPP_STRING_SZ              (256)
#define SAPP_LONG_STRING_SZ         (384)
#define SAPP_FILE_PATH_SZ           (1024)
#define SAPP_MAX_BLOCKED_USERS      (25)

/* URI for anonymous fetch of capabilities, per RCS 5.0 and RFC3323 specs */
#define SAPP_ANONYMOUS_URI                  "sip:anonymous@anonymous.invalid"

/* Definitions for AKA authentication */
#define SAPP_AKA_AUTH_RESP_SZ      (16)
#define SAPP_AKA_AUTH_AUTS_SZ      (14)
#define SAPP_AKA_AUTH_CK_SZ        (16)
#define SAPP_AKA_AUTH_IK_SZ        (16)
/*
 * The size in bytes of the scratch pad area used for constructing 
 * message body's that are used in signalling messages.
 * For example, the text message inside a SIP MESSAGE body.
 */
#define SAPP_PAYLOAD_SZ             (4096)

/*
 * Max number of extra (non-mandatory) header fields that can be included
 * in SIP Api calls.
 */
#define SAPP_HF_MAX_NUM_SZ          (16)

/*
 * Max number of Security-Server header fields that can be contained 
 * in re-register requests.
 */
#define SAPP_SEC_SRV_HF_MAX_NUM          (3)

/* Maximum alias URI table size. */
#define SAPP_ALIAS_URI_MAX_NUM           (2)

/* Max number of allowable SIP UA's (Services). */
#define SAPP_SIP_MAX_UA             (3)

/* Max number of allowable calls per SIP UA (Per Service) */
#define SAPP_CALL_NUM               (3)

/* SIP max transactions per UA/INFC as recommended by the SIP API doc. */
#define SAPP_SIP_TRANS_PER_INFC     (8)

/* Retry timer in ms when registering SAPP to ISI */
#define SAPP_REGISTER_TIMER_MS      (5000)

/* Watch dog timer to determine when device is in sleep mode */
#define SAPP_SLEEP_WATCH_DOG_TIMER_MS (1000)

/* Retry timer ins ms when send message failed in timer callback */
#define SAPP_MESSAGE_RETRY_TIMER_MS   (10)

/*
 * Resource reservation timer in ms.
 * This timer is used for precondition and INVITE w/o SDP, prevneting
 * sapp stucked in resource not ready state.
 * The value could be changed depends on the real condition.
 */
#define SAPP_RSRC_RSRV_TIMER_MS     (5000)
/* De-registration timeout value in milli-second. */
#define SAPP_REG_DE_REG_TIMER_MS    (500)
/*
 * Amount of sleep in seconds to look for when trying to determine if
 * the device had been sleeping.
 */
#define SAPP_SLEEP_TIMEOUT_SECS       (5)

/* The rate at which stun requests */
#define SAPP_STUN_RETRANS_RATE_MS   (200)

/* Default packet rate */
#define SAPP_PRATE_DEFAULT_MS       (20)
/*
 * SIT transaction timeout.  The timeout when SIT considers a stun
 * transaction to be terminated.
 */
#define SAPP_STUN_TRANS_EXPIRY_MS   (6000)

/*
 * The max register retry failed count.
 */
#define _SAPP_RETRY_REG_MAX_FAIL_COUNT  (4)

/* Values used in a bit mask to represent an SRTP encryption type */
#define SRTP_AES_80 (1)
#define SRTP_AES_32 (2)

/* 
 * The size of an SRTP key.  This is the size of a randomly generated string
 * used as a key when establishing an SRTP data stream.
 */
#define SRTP_SECURITY_KEY_SZ (30)

/*
 * Cell Id string size
 */
#define SAPP_BSID_STRING_SZ (24)

/* Phone context string size */
#define SAPP_PHONE_CONTEXT_STRING_SZ (MAX_DOMAIN_NAME_LEN)
/*
 * Cell Id string size
 */
#define SAPP_IMEI_URI_STRING_SZ (128)
/*
 * q-value string size
 */
#define SAPP_Q_VALUE_STRING_SZ  (5)

/*
 * Max P-CSCF addresses in the list.
 */
#define SAPP_MAX_PCSCF_NUM      (5)

/* Maximum mode set value */
#define _SAPP_MAX_MODE_SET         (8)
/* Mode set string size. */
#define _SAPP_MODE_SET_STRING_SIZE (32)
/* Values used to indicate the SRTP security scheme in SIP/SDP parameters */
#define SRTP_AES_80_STR ("AES_CM_128_HMAC_SHA1_80")
#define SRTP_AES_32_STR ("AES_CM_128_HMAC_SHA1_32")

#define SAPP_END_OF_LINE            "\r\n"
#define SAPP_END_OF_DOC             "\r\n\r\n"

#define SAPP_ANONYMOUS_CALL_ID      "Anonymous"
#define SAPP_ANONYMOUS_CALL_ID_URI  "sip:Anonymous@Anonymous.invalid"

#ifndef SAPP_DEBUG
#define SAPP_dbgPrintf(x, ...)
#else
#ifdef VPORT_4G_PLUS_APROC
#define SAPP_dbgPrintf(fmt, args...) \
         OSAL_logMsg("AP:" fmt, ## args)
#else
#define SAPP_dbgPrintf(fmt, args...) \
         OSAL_logMsg("MP:" fmt, ## args)
#endif
#endif

/* Header Field and argument values used through out SAPP */
#define SAPP_ACCEPT_HF              "Accept:"
#define SAPP_ACCEPT_CONTACT_HF      "Accept-Contact:"
#define SAPP_PRIVACY_HF             "Privacy:"
#define SAPP_ALLOW_HF               "Allow:"
#define SAPP_CONTENT_TYPE_HF        "Content-Type:"
#define SAPP_CONTENT_LENGTH_HF      "Content-Length:"
#define SAPP_CONTENT_DISP_HF        "Content-Disposition:"
#define SAPP_SUB_STATE_HF           "Subscription-State:"
#define SAPP_EXPIRES_HF             "Expires:"
#define SAPP_SUPPORTED_HF           "Supported:"
#define SAPP_SERVICE_ROUTE_HF       "Service-Route:"
#define SAPP_ROUTE_HF               "Route:"
#define SAPP_EVENT_HF               "Event:"
#define SAPP_PATH_HF                "Path:"
#define SAPP_DATETIME_HF            "DateTime:"
#define SAPP_FROM_HF                "From:"
#define SAPP_TO_HF                  "To:"
#define SAPP_MIN_SE_HF              "Min-SE:"
#define SAPP_P_PREFERRED_ID_HF      "P-Preferred-Identity:"
#define SAPP_P_ASSERTED_ID_HF       "P-Asserted-Identity:"
#define SAPP_P_ASSOCIATED_URI_HF    "P-Associated-URI:"
#define SAPP_P_CALLED_PARTY_ID_HF   "P-Called-Party-ID:"
#define SAPP_P_CHARING_FUNC_ADR_HF  "P-Charging-Function-Addresses:"
#define SAPP_REFERRED_BY_HF         "Referred-By:"
#define SAPP_PANI_VAL_STR           "IEEE-802.16;wimax-location="
#define SAPP_REQUIRE_HF             "Require:"
#define SAPP_REASON_HF              "Reason:"
#define SAPP_SUBJECT_HF             "Subject:"
#define SAPP_HISTORY_INFO_HF        "History-Info:"
#define SAPP_ALERT_INFO_HF          "Alert-Info:"
#define SAPP_MESSAGE_TYPE_HF        "Message-Type:"
#define SAPP_SECURITY_CLIENT_HF     "Security-Client:"
#define SAPP_SECURITY_SERVER_HF     "Security-Server:"
#define SAPP_SECURITY_VERIFY_HF     "Security-Verify:"
#define SAPP_RECV_INFO_HF           "Recv-Info:"
#define SAPP_INFO_PACKAGE_HF        "Info-Package:"
#define SAPP_RETRY_AFTER_HF         "Retry-After:"
#define SAPP_SERVER_HF              "Server:"
#define SAPP_CONTENT_TRANSFER_ENCODING_HF "Content-Transfer-Encoding:"
#define SAPP_REFER_SUB_HF           "Refer-Sub:"
#define SAPP_CONTENT_ID_HF          "Content-ID:"
#define SAPP_FEATURE_CAPS_HF        "Feature-Caps:"
#define SAPP_P_EARLY_MEDIA_HF       "P-Early-Media:"

#define SAPP_TEL_SCHEME             "tel:"
#define SAPP_SIP_SCHEME             "sip:"
#define SAPP_MSRP_SCHEME            "msrp:"
#define SAPP_MSRPS_SCHEME           "msrps:"
#define SAPP_SUBSCRIBE_ALLOW        "NOTIFY"
#define SAPP_MESSAGE_TYPE_SUBMIT    "submit_sm"
#define SAPP_CONTENT_TYPE_CPIM      "Message/CPIM"
#define SAPP_IM_CONTENT_TYPE        "text/plain"
#define SAPP_IPSEC_3GPP_ARG         "ipsec-3gpp"
#define SAPP_ALG_ARG                "alg="
#define SAPP_PROT_ARG               "prot="
#define SAPP_MOD_ARG                "mod="
#define SAPP_EALG_ARG               "ealg="
#define SAPP_SPI_C_ARG              "spi-c="
#define SAPP_SPI_S_ARG              "spi-s="
#define SAPP_PORT_C_ARG             "port-c="
#define SAPP_PORT_S_ARG             "port-s="
#define SAPP_Q_ARG                  "q="

#define _SAPP_ACTIVE_ARG            "active"
#define _SAPP_INACTIVE_ARG          "inactive"
#define _SAPP_REG_EVENT_ARG         "reg"
#define _SAPP_REG_ACCEPT_ARG        "application/reginfo+xml"
#define _SAPP_CONF_EVENT_ARG        "application/conference-info+xml"
#define _SAPP_CONF_ARG              "conference"
#define _SAPP_EVENTLIST_ARG         "eventlist"
#define _SAPP_SUB_STATE_HF          "Subscription-State:"
#define _SAPP_MWI_ARG               "message-summary"
#define _SAPP_MWI_EVENT_ARG         "application/simple-message-summary"
#define _SAPP_3GPP_IMS_ARG          "application/3gpp-ims+xml"
#define _SAPP_3GPP_ACCEPT_ARG       "application/sdp, application/3gpp-ims+xml"
#define _SAPP_USSD_ACCEPT_ARG       "application/sdp; application/3gpp-ims+xml; " \
                                    "application/vnd.3gpp.ussd+xml"
#define _SAPP_USSD_RECV_INFO_ARG    "g.3gpp.ussd"

#define _SAPP_DIR_SNEDREV_ARG       "sendrecv"
#define _SAPP_DIR_SENDONLY_ARG      "sendonly"
#define _SAPP_DIR_RECVONLY_ARG      "recvonly"
#define _SAPP_GATED_ARG             "gated"
#define _SAPP_SUPPORTED_ARG         "supported"

#define SAPP_REQUEST_DISPOSITION_HF "Request-Disposition:"
#define SAPP_NO_FORK_ARG            "no-fork"
#define SAPP_BINARY_ARG             "binary"
#define SAPP_CONTENT_TYPE_3GPP_SMS  "application/vnd.3gpp.sms"
#define SAPP_CONTENT_TYPE_3GPP2_SMS "application/vnd.3gpp2.sms"
#define SAPP_URI_SOS_STR            "urn:service:sos"
#define SAPP_URN_ALERT_INFO_CW_STR  "urn:alert:service:call-waiting"

#define SAPP_DTMF_ENABLED_STR       "DTMF-enabled"

/* Supported header field options */
#define SAPP_SUPPORTED_OPT_PATH     "path"
#define SAPP_SUPPORTED_OPT_GRUU     "gruu"
#define SAPP_SUPPORTED_OPT_REPLACES "replace"
#define SAPP_SUPPORTED_OPT_100REL   "100rel"
#define SAPP_SUPPORTED_OPT_TIMER    "timer"
#define SAPP_SUPPORTED_OPT_PRECONDITION     "precondition"

#define SAPP_SUPPORTED_OPT_NOREFSUB     "norefsub"

/* Network interface name */
#define SAPP_NETWORK_INFC_NAME_WIFI "wifi"
#define SAPP_NETWORK_INFC_NAME_LTE  "lte"

#define SAPP_ALIAS_URI_STR               ("ALIAS")
#define SAPP_RECIPIENT_LIST       "recipient-list"
#define SAPP_APP_RESOURCE_LISTS   "application/resource-lists+xml"
#define SAPP_BOUNDRY_ARG          "boundary="
#define SAPP_PAYLOAD_BOUNDRY      "++"
#define SAPP_FALSE                "false"

/* Reject call reason */
#define SAPP_CALL_REJECT_REASON_NOT_ACCEPTABLE  "488 Not Acceptable Here"
#define SAPP_CALL_REJECT_REASON_BUSY            "486 Busy Here"
/*
 * Information about a created task is placed here.
 */
typedef struct {
    OSAL_TaskId tId;
    uvint       stackSz;
    uvint       pri;
    void       *func_ptr;
    char        name[16];
    void       *arg_ptr;
} SAPP_TaskObj;

typedef enum {
    SAPP_TMR_EVENT_NONE = 0,
    SAPP_TMR_EVENT_PUBLISH,
    SAPP_TMR_EVENT_RLS,
    SAPP_TMR_EVENT_RC_RLS,
    SAPP_TMR_EVENT_WATCHER,
    SAPP_TMR_EVENT_DEFERRED_MSGS,
    SAPP_TMR_SLEEP_DETECTED,
    SAPP_TMR_EVENT_LCL_REFRESH,
    SAPP_TMR_EVENT_RMT_REFRESH,
    SAPP_TMR_EVENT_REG_EXPIRED,
    SAPP_TMR_EVENT_IDLE,
    SAPP_TMR_EVENT_REG_RETRY,
    SAPP_TMR_EVENT_REG_SUBSCRIBE_RETRY,
    SAPP_TMR_EVENT_DE_REG,
    SAPP_TMR_EVENT_INVITE_RETRY,
} SAPP_TmrEventType;

typedef struct {
    SAPP_TmrEventType type;
    uint32            cnt;
    void             *arg_ptr;
} SAPP_TmrEvent;

/*
 * SAPP struct to exchange messages.
 */
typedef struct {
    union {
        tSipIpcMsg    sip;
        ISIP_Message  isi;
        SAPP_TmrEvent tmr;
#ifdef INCLUDE_SIMPLE
        XCAP_Evt      xcap;
        MSRP_Event    msrp;
#endif
    } u;
} SAPP_Msg;

typedef enum {
    SAPP_CONTENT_NONE = 0,
    SAPP_CONTENT_TEXT_PLAIN,
    SAPP_CONTENT_PHOTO_JPEG,
    SAPP_CONTENT_PHOTO_GIF,
    SAPP_CONTENT_PHOTO_BMP,
    SAPP_CONTENT_PHOTO_PNG,
    SAPP_CONTENT_VIDEO_3GP,
    SAPP_CONTENT_VIDEO_MP4,
    SAPP_CONTENT_VIDEO_WMV,
    SAPP_CONTENT_VIDEO_AVC,
    SAPP_CONTENT_VIDEO_MPEG,
    SAPP_CONTENT_AUDIO_AMR,
    SAPP_CONTENT_AUDIO_AAC,
    SAPP_CONTENT_AUDIO_MP3,
    SAPP_CONTENT_AUDIO_WMA,
    SAPP_CONTENT_AUDIO_M4A,
    SAPP_CONTENT_AUDIO_3GPP,
    SAPP_CONTENT_MSG_CPIM,
    SAPP_CONTENT_MULTI_PART,
    SAPP_CONTENT_COMPOSING,
    SAPP_CONTENT_SMS_3GPP,
    SAPP_CONTENT_SMS_3GPP2,
    SAPP_CONTENT_LAST,
} SAPP_ContentType;

typedef enum {
    SAPP_FILE_ATTR_NONE       = 0,
    SAPP_FILE_ATTR_RENDER     = 1,
    SAPP_FILE_ATTR_ATTACHMENT = 2,
    SAPP_FILE_ATTR_LAST
} SAPP_FileAttribute;

typedef struct
{
   vint          internal;
   const char   *ext_ptr;
} SAPP_IntExt;

typedef enum {
    SAPP_MODIFY_NONE = 0,
    SAPP_MODIFY_HOLD,
    SAPP_MODIFY_RESUME,
    SAPP_MODIFY_OTHER,
} SAPP_ModifyType;

typedef enum {
    SAPP_NETWORK_MODE_4G = 0,
    SAPP_NETWORK_MODE_WIFI,
} SAPP_NetworkMode;

typedef struct {
    vint maxUsers;
    char aUsers[SAPP_MAX_BLOCKED_USERS][SAPP_STRING_SZ];
}SAPP_BlockUsers;

typedef struct {
    uint16          startPort;
    OSAL_NetAddress nextInfc;
    uint16          endPort;
}SAPP_RtpInfc;

typedef struct {
    OSAL_Boolean    isEnable; /* cache if ipsec is enable */
    uint16          defaultPort; /* Default unprotected sip port */ 
    uint16          pPortUC; /* Protected UE client port */
    uint16          pPortUS; /* Protected UE server port */
    vint            pPortPoolSize; /* Poolsize of Protected port */
    uint16          startProtectedPort; /* Start port of protected pool */
    uint16          lastProtectedPort; /* Last used protected port */
    vint            startSpi; /* Start spi of spi pool */
    vint            lastSpi; /* Last used spi */
    vint            spiPoolSize; /* Spi pool size */
    uint16          defaultProxyPort; /* Default unprotected proxy port */ 
    OSAL_IpsecSa    inboundSAc; /* Inbound SA for protected client port */
    OSAL_IpsecSp    inboundSPc;
    OSAL_IpsecSa    inboundSAs; /* Inbound SA for protected server port */
    OSAL_IpsecSp    inboundSPs;
    OSAL_IpsecSa    outboundSAc; /* Outbound SA for protected client port */
    OSAL_IpsecSp    outboundSPc;
    OSAL_IpsecSa    outboundSAs; /* Outbound SA for protected server port */
    OSAL_IpsecSp    outboundSPs;
}SAPP_IpsecObj;

typedef int32 (*SAPP_TmrCb)(void*);

typedef struct {
    tSipHandle         hTransaction;
    ISI_Id             id;
} SAPP_ImObj;

typedef struct {
    tSipHandle         hTransaction;
    ISI_Id             id;
} SAPP_TransObj;

typedef struct {
    ISIP_Message isiMsg;
    OSAL_MsgQId  isiEvt;
} SAPP_Event;

typedef struct {
    uint8 SQCIF;        /* 1 - 32 */
    uint8 QCIF;         /* 1 - 32 */
    uint8 CIF;          /* 1 - 32 */
    uint8 CIF4;         /* 1 - 32 */
    uint8 CIF16;        /* 1 - 32 */
    char framesize[MAX_SDP_ATTR_STR_LEN + 1];
    struct {
        uint8 left;     /* 1 - 255 */
        uint8 right;    /* 1 - 255 */
    } PAR_ratio;
    struct {
        uint8 cd;       /* 1 - 127 */
        int   cf;       /* 1000, 1001 */
        int SQCIFMPI;   /* 1 to 2048 */
        int QCIFMPI;    /* 1 to 2048 */
        int CIFMPI;     /* 1 to 2048 */
        int CIF4MPI;    /* 1 to 2048 */
        int CIF16MPI;   /* 1 to 2048 */
        int CUSTOMMPI;  /* 1 to 2048 */
    } CPCF;
    int BPP;            /* 0 - 65536 */
    uint8 HRD;          /* 0 or 1 */
    struct {
        int Xmax;
        int Ymax;
        int MPI;
    } CUSTOM;
    uint8 PROFILE;      /* 0 - 10 */
    uint8 LEVEL;        /* 0 - 100 */
    uint8 INTERLACE;    /* 0 or 1 */
    struct {
        uint8 F;        /* Possible values are 0 - 1 */
        uint8 I;        /* Possible values are 0 - 1 */
        uint8 J;        /* Possible values are 0 - 1 */
        uint8 T;        /* Possible values are 0 - 1 */
        uint8 K;        /* posible values 1 - 4 */
        uint8 N;     /* Possible values are 1 - 4 */
        uint8 P;     /* Possible values are 1 - 4 */
    } annex;            /* "F", "I", "J", "T", "K", "N", and "P". */
    int maxBr;
    int maxMbps;
} H263_Params;

typedef struct {
    char           szCoderName[ISI_CODER_STRING_SZ + 1];
    int            coderNum;
    int            decoderNum;
    int            rate;
    int            maxPacketRate;
    tSdpMediaType  relates;
    union {
        struct {
            int     annexb;
        } g729;
        struct {
            int     octetAlign;
            int     modeSet; /* Mode set bit mask. */
        } amr;
        struct {
            int pmode;
            char plevel[8];
            struct {
                int id;
                char uri[MAX_SDP_ATTR_STR_LEN];
            } extmap;
            int maxBr;
            int maxMbps;
            char framesize[MAX_SDP_ATTR_STR_LEN + 1];
            int  framerate;
            char sps[MAX_SDP_ATTR_STR_LEN + 1];
        } h264;
        H263_Params h263;
    } props;
} SAPP_Coder;

typedef struct {
    vint               type;
    char               lclAes80[MAX_SESSION_SRTP_PARAMS];
    char               lclAes32[MAX_SESSION_SRTP_PARAMS];
    char               rmtAes80[MAX_SESSION_SRTP_PARAMS];
    char               rmtAes32[MAX_SESSION_SRTP_PARAMS];
} SAPP_SecurityKeys;



typedef struct {
    ISI_Id             isiCallId;
    tSipHandle         dialogId;
    MNS_SessionObj     mnsSession;
    SAPP_ModifyType    modify;
    vint               usePrack;
    vint               useIpSec;
    vint               waitingTone; /*0 is default, 1 for Communication Waiting tone*/
    vint               usePrecondition;                 /* 0 for Not support. 1 for support. others for required. */
    char               vccHandoffAddr[SAPP_STRING_SZ];
    SAPP_Coder         coders[ISI_CODER_NUM];
    SAPP_SecurityKeys  audioKeys;
    SAPP_SecurityKeys  videoKeys;
    vint               blockCid;
    struct {
        tSipHandle   dialogId;
        char         identity[SAPP_STRING_SZ];  /* The id of the conference. Also called the 'Conference URI' */
    } conf;
    SAPP_Event         event;
    OSAL_TmrId         rsrcRsrvTimerId;
    OSAL_TmrId         rsrcRsrvRetryTimerId;  /* send message retry timer */
    vint               isEmergency;
    tSipHandle         replaceDialogId; /*
                                         * Cache the dialog id which attempt to
                                         * replace this call.
                                         */
    OSAL_Boolean       useAlias; /* If use alias uri for the call. */
    OSAL_TmrId         retryAfterTimerId;
    OSAL_TmrId         retryAfterMsgTmrId; /* send message retry timer */
    OSAL_MsgQId        tmrEvtQ;
} SAPP_CallObj;

typedef enum {
    SAPP_REG_STATE_NONE = 0,
    SAPP_REG_STATE_OFF,
    SAPP_REG_STATE_TRYING,
    SAPP_REG_STATE_RE_REG,
    SAPP_REG_STATE_ON,
    SAPP_REG_STATE_NO_NET,
    SAPP_REG_STATE_DE_REG_TRYING,
    SAPP_REG_STATE_RESTART,
    SAPP_REG_STATE_LAST,
} SAPP_RegState;

typedef enum {
    SAPP_EMGERGENCY_STATE_IDLE,
    SAPP_EMGERGENCY_STATE_ACTIVE,
    SAPP_EMGERGENCY_STATE_EXPIRED_REG,
    SAPP_EMGERGENCY_STATE_WAIT_FOR_REG,
    SAPP_EMGERGENCY_STATE_IN_CALL,
    SAPP_EMGERGENCY_STATE_LAST,
} SAPP_EmgcyState;


typedef struct {
    SAPP_RegState  state;
    uint32         timeoutSecs;
    uint32         natRefreshRateSecs;
    char           preconfiguredRoute[SAPP_STRING_SZ];
    vint           useRegEvt;
    tLocalIpConn   lclConn;
    uint32         regRetryBaseTime;
    uint32         regRetryMaxTime;
    OSAL_TmrId     regTmrId;
    OSAL_TmrId     regMsgTmrId; /* send message retry timer */
    uint32         regFailCount;
    char           pcscfList[SAPP_MAX_PCSCF_NUM][SAPP_LONG_STRING_SZ];
    vint           pcscfIndex;
    OSAL_MsgQId    tmrEvtQ;
    vint           akaVersion; /* AKAv1 or AKAv2 */
    vint           shortenExpires;
    vint           regFailCode;
    char           pcscfTrying[SAPP_LONG_STRING_SZ];
    struct {
        tSipHandle dialogId;
        void      *service_ptr;
        struct {
            uint32         cnt;
            OSAL_TmrId     id;
        } timer;
    } subscription;
} SAPP_RegObj;

typedef struct {
    SAPP_EmgcyState  state;
    SAPP_CallObj    *call_ptr;
    OSAL_MsgQId      tmrEvtQ;
    OSAL_TmrId       emergencyTmrId;
    OSAL_TmrId       emergencyMsgTmrId; /* send message retry timer */
    char             imeiUri[SAPP_IMEI_URI_STRING_SZ];
} SAPP_EmgcyObj;

typedef struct {
    tSipHandle   hTransaction;
    ISI_Id       id;
    ISI_TelEvent evt;
} SAPP_TelEvt; /* Telephone event for sending DTMF via SIP */

typedef struct {
    uint32         timeoutSecs;
    vint           useMwiEvt;
    struct {
        tSipHandle dialogId;
        void      *service_ptr;
    } subscription;
} SAPP_MwiObj;

typedef struct {
    ISI_Id              id;
    SAPP_CallObj        aUssd;
} SAPP_UssdObj;

typedef enum {
    SAPP_CAPABILITIES_DISCOVERY_NONE = 0,
    SAPP_CAPABILITIES_DISCOVERY_OPTIONS,
    SAPP_CAPABILITIES_DISCOVERY_PRESENCE
} SAPP_CapabilityDiscoveryType;

typedef uint32  _SAPP_CapabilityBitmapType;


typedef struct {
    ISI_Id             isiServiceId;
    OSAL_NetAddress    sipInfc;
    tTransportType     sipTransportType;
    OSAL_Boolean       isTransportReady;
    OSAL_NetSockId     sipInfcFd;
    OSAL_NetSockId     sipInfcTcpServerFd;
    OSAL_NetSockId     sipInfcTcpClientFd;    
    OSAL_NetSockId     sipInfcProtectedServerFd;
    OSAL_NetSockId     sipInfcProtectedClientFd;
    OSAL_NetSockId     sipInfcProtectedTcpServerFd;
    OSAL_NetSockId     sipInfcProtectedTcpClientFd;
    vint               defaultPort;                    /* Default sip port for ipsec */
    SAPP_RtpInfc       artpInfc;
    SAPP_RtpInfc       vrtpInfc;
    vint               blockCid;
    char               stunAddr[SAPP_STRING_SZ];
    uint16             stunPort;
    SAPP_BlockUsers    blockedUsers;
    vint               confKeepAlive;
    vint               ringTemplate;
    char               userAgent[SAPP_STRING_SZ];
    vint               capDiscoveryMethod;
    vint               capDiscoveryViaCommonStack;
    uint32             regCapabilitiesBitmap;
    uint32             exchangeCapabilitiesBitmap;
    uint32             srvccCapabilitiesBitmap;
    char               vdn[SAPP_STRING_SZ];
    char               payloadStratch[SAPP_PAYLOAD_SZ];
    char               hfStratch[SAPP_HF_MAX_NUM_SZ][SAPP_LONG_STRING_SZ];
                       /* Array for caching security service header field. */
    char               secSrvHfs[SAPP_SEC_SRV_HF_MAX_NUM][SAPP_STRING_SZ];
    char               callForward[SAPP_STRING_SZ];
    vint               akaAuthRespSet;
    vint               akaAuthResult;
    uint8              akaAuthResp[SAPP_AKA_AUTH_RESP_SZ];
    vint               akaAuthResLength;
    uint8              akaAuthAuts[SAPP_AKA_AUTH_AUTS_SZ];
    uint8              akaAuthCk[SAPP_AKA_AUTH_CK_SZ];
    uint8              akaAuthIk[SAPP_AKA_AUTH_IK_SZ];
    vint               useIsim;
    struct {
        vint           type;
                       /* LTE Cell ID for P-Access-Network-Info header field */
        char           szBsId[SAPP_BSID_STRING_SZ]; 
    } bsId;
    char              *telUri_ptr;
    char               telUri[SIP_URI_STRING_MAX_SIZE];      /* Tel-Uri, copy from P-Associated-URI header field*/
                       /* Alias URI table. */
    char               aliasUriList[SAPP_ALIAS_URI_MAX_NUM][SIP_URI_STRING_MAX_SIZE];
#ifdef INCLUDE_SIMPLE
    SIMPL_Obj          simple;
#endif
    MNS_ServiceObj     mnsService;
    SAPP_RegObj        registration;
    SAPP_EmgcyObj      emergencyObj;
    SAPP_MwiObj        mwiObj;
    SAPP_TelEvt        telEvt;
    SAPP_ImObj         im;
    SAPP_UssdObj       ussd;
    SAPP_TransObj      transObj;
    struct {
        tSipHandle     uaId;
        tUaConfig      config;
        SAPP_CallObj   aCall[SAPP_CALL_NUM];
        tLocalIpConn   localConn;
        vint           usePrack;
        vint           useCpim; /*
                                 * Use CPIM format when sending page mode SMS,
                                 * otherwise send plain/text.
                                 */
        vint           useIpSec;    
        vint           usePrecondition; /*
                                         * 0 for Not support. 1 for support 
                                         * others for required.
                                         */
        char           sessionTimer[SAPP_STRING_SZ];
        char           forceSessionTimer[SAPP_STRING_SZ];
    } sipConfig;
    SAPP_IpsecObj      ipsecObj;
    SAPP_Coder         coders[ISI_CODER_NUM];
    vint               isEmergency;            /* if emergency call, set to 1 */
    char               lastPathUri[SAPP_STRING_SZ]; /* Last Path URI */
    SAPP_NetworkMode   networkMode; /* 4G or Wifi */
    vint               protocolId; /* Also need a copy for each service */
    char               instanceId[SIP_INSTANCE_STR_SIZE];
    char               qValue[SIP_Q_VALUE_STR_SIZE];
    OSAL_Boolean       natKeepaliveEnable;
    char               phoneContext[SAPP_PHONE_CONTEXT_STRING_SZ];
    OSAL_Boolean       isNoNet;
} SAPP_ServiceObj;

/*
 * SIP protocol object.
 */
typedef struct {
    SAPP_TaskObj       task;
    struct {
        OSAL_MsgQId    isiCmd;
        OSAL_MsgQId    isiEvt;
        OSAL_MsgQId    sipEvt;
        OSAL_MsgQId    tmrEvt;
        OSAL_MsgQId    xcapEvt;
        OSAL_MsgQId    msrpEvt;
        OSAL_MsgQGrpId group;
        SAPP_Msg       msg;
    } queue;
    struct {
        OSAL_TmrId      id;
        SAPP_Event      event;
    } tmr;
    struct {
        uint32         keepAlive;
    } presence;
#ifdef INCLUDE_SIMPLE
    SIMPL_XcapObj      xcap;
#endif
    SAPP_ServiceObj    service[SAPP_SIP_MAX_UA];
    SAPP_Event         event;
    uint32             keepAlive;
    uint32             refresh;
    uint32             mwiRefresh;
    OSAL_Boolean       keepAliveEnable;
    char               sessionTimer[SAPP_STRING_SZ];
    char               forceSessionTimer[SAPP_STRING_SZ];

    tUaAppEvent        sipEvt;

    char               name[SAPP_STRING_SZ];
    char               qValue[SIP_Q_VALUE_STR_SIZE];
    vint               capDiscoveryMethod;
    vint               capDiscoveryViaCommonStack;
    uint32             regCapabilitiesBitmap; /* bitmask */
    uint32             exchangeCapabilitiesBitmap; /* bitmask */
    uint32             srvccCapabilitiesBitmap; /* bitmask */
    char               supported[SAPP_STRING_SZ];
    char               vdn[SAPP_STRING_SZ];
    char               hostname[SAPP_STRING_SZ];

    char               filePath[SAPP_FILE_PATH_SZ];
    char               filePrepend[SAPP_STRING_SZ];
    vint               msrpUseTls;                     /* If MSRP use TLS */ 
    vint               msrpUseTlsFor4g; /* If MSRP use TLS under 4G network */ 
    vint               msrpUseTlsForWifi; /* If MSRP use TLS under wifi network */ 
    vint               usePrack;
    vint               useCpim;
    vint               useIpSec;
    vint               useIsim;
    vint               usePrecondition;
    vint               useRegEvt;
    vint               useMwiEvt;
    vint               ringTemplate;
    uint32             regRetryBaseTime;
    uint32             regRetryMaxTime;    
    struct {
        OSAL_TmrId      id;
    } sleepWatchDog;
    vint               fileFlowControlDepth;
    OSAL_NetSslCert    cert;  /*
                               * Certificate for MSRP/TLS.
                               * Keep only one cert for all session
                               */ 
    uint32             t1; /* sip timer t1 */
    uint32             t2; /* sip timer t2 */
    uint32             t4; /* sip timer t4 */
    uint32             mtu;
} SAPP_SipObj;

/*
 * This is the SAPP global object.
 * Anything allocated statically must be placed in this object.
 * Information used by modules in SAPP is placed here.
 */
typedef struct {
    SAPP_SipObj   sipObj;
    vint          protocolId;
} SAPP_GlobalObj;

/*
 * SIP max dialogs per INFC.
 * You need dialogs for calls, presence subscriptions, conference event
 * subscriptions, and IM sessions.
 */
#define SAPP_SIP_MAX_DIALOGS  (6)

void SAPP_setHostAndPort(
    char     *host_ptr,
    char     *name_ptr,
    vint      maxNameLen,
    uint16   *port_ptr);

SAPP_ServiceObj* SAPP_findServiceViaServiceId(
    SAPP_SipObj *sip_ptr,
    ISI_Id       serviceId);

SAPP_CallObj* SAPP_findCallViaIsiId(
    SAPP_ServiceObj *service_ptr,
    ISI_Id           callId);

SAPP_CallObj* SAPP_findCallViaDialogId(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       dialogId);

SAPP_CallObj* SAPP_findCallConfEventViaDialogId(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       dialogId);

vint SAPP_searchBlockedUser(
    SAPP_BlockUsers *block_ptr,
    char          *uri_ptr,
    vint           shouldRemove);

vint SAPP_sendEvent(
    SAPP_Event *evt_ptr);

void SAPP_isiServiceCmd(
    ISIP_Message    *cmd_ptr, 
    SAPP_SipObj     *sip_ptr,
    SAPP_Event      *evt_ptr);

vint SAPP_sipEvent(
    SAPP_SipObj   *sip_ptr,
    tUaAppEvent   *evt_ptr);

void SAPP_setHostAndPort(
    char     *host_ptr,
    char     *name_ptr,
    vint      maxNameLen,
    uint16   *port_ptr);

char* SAPP_getXmlNestedTagAttr(
    ezxml_t     xml_ptr, 
    const char *parentTag_ptr, 
    const char *childTag_ptr,
    const char *attr_ptr);

char* SAPP_getXmlNestedTagText(
    ezxml_t     xml_ptr, 
    const char *parentTag_ptr, 
    const char *childTag_ptr);

char* SAPP_getXml2NestedTagAttr(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr,
    const char *attr_ptr);

char* SAPP_getXml2NestedTagText(
    ezxml_t     xml_ptr,
    const char *parentTag_ptr,
    const char *childOneTag_ptr,
    const char *childTwoTag_ptr);

void SAPP_serviceIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_ServiceReason  reason,
    ISIP_Status         status,
    ISIP_Message       *isi_ptr);

vint SAPP_sipAddPreconfiguredRoute(
    SAPP_RegObj   *reg_ptr,
    char          *target_ptr,
    vint           targetLen);

vint SAPP_sipAddAuthorization(
    SAPP_ServiceObj *service_ptr,
    char            *target_ptr,
    vint             targetLen);

ISI_Id SAPP_getUniqueIsiId(ISI_Id serviceId);

const char* SAPP_GetResponseReason(int code);

vint _SAPP_sipDecodeNonce(
    char  *nonce_ptr,
    uint8 *rand_ptr,
    vint   randLen,
    uint8 *autn_ptr,
    vint   autnLen);

vint SAPP_sipServiceTransportInit(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr);

vint SAPP_sipServiceTransportClean(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr);

vint _SAPP_sipServiceIsTransportReady(
    SAPP_ServiceObj  *service_ptr);

vint SAPP_sipServiceTransportDestroy(
    SAPP_ServiceObj  *service_ptr);

void SAPP_sipServiceTransportSwitch(
    SAPP_ServiceObj  *service_ptr,
    vint            onOff);

void SAPP_sipServiceDestroyAll(
    SAPP_SipObj *sip_ptr);

void _SAPP_getSipTimerSettings(
    void        *cfg_ptr);

void _SAPP_getImTransportProto(
    SAPP_SipObj *sip_ptr,
    void        *cfg_ptr);

vint _SAPP_sipServerTimeOut(
    SAPP_ServiceObj     *service_ptr,
    char               **reason_ptr,
    tUaAppEvent         *uaEvt_ptr,
    SAPP_SipObj         *sip_ptr);

#endif
