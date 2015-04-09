#ifndef _SUPSRV_H_
#define _SUPSRV_H_

#include <osal.h>
#include <xcap.h>

/* Return codes. */
#define SUPSRV_OK                               (0)
#define SUPSRV_ERR                              (-1)

/*
 * XCAP DOC definition for supplemetary service.
 */
#define SUPSRV_XCAP_HEADER "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<simservs \
xmlns:cp=\"urn:ietf:params:xml:ns:common-policy\"\n\
xmlns=\"http://uri.etsi.org/ngn/params/xml/simservs/xcap\"\n\
xmlns:ocp=\"urn:oma:xml:xdm:common-policy\">"

/* The condition of media */
#define SUPSRV_XCAP_MEDIA "<ss:media>%s</ss:media>"

#define SUPSRV_XCAP_OIP_DOC SUPSRV_XCAP_HEADER"<originating-identity-presentation active=\"%s\">\n\
</originating-identity-presentation>\n\
</simservs>"

#define SUPSRV_XCAP_OIR_DOC SUPSRV_XCAP_HEADER"<originating-identity-presentation-restriction active=\"%s\">\n\
</originating-identity-presentation-restriction>\n\
</simservs>"

#define SUPSRV_XCAP_TIP_DOC SUPSRV_XCAP_HEADER"<terminating-identity-presentation active=\"%s\">\n\
</terminating-identity-presentation>\n\
</simservs>"

#define SUPSRV_XCAP_TIR_DOC SUPSRV_XCAP_HEADER"<terminating-identity-presentation-restriction active=\"%s\">\n\
</terminating-identity-presentation-restriction>\n\
</simservs>"
#define SUPSRV_XCAP_CW_DOC SUPSRV_XCAP_HEADER"<communication-waiting active=\"%s\"/>\n\
</simservs>"

/* incoming-communication-barring */
#define SUPSRV_XCAP_CBIC_ACTIVATE_DOC SUPSRV_XCAP_HEADER"<incoming-communication-barring active=\"true\">\n\
  <cp:ruleset>\n\
      <cp:rule id=\"%s\">\n\
          <cp:conditions>\n\
              <communication-barring-serv-cap>\n\
                  <serv-cap-roaming>%s</serv-cap-roaming>\n\
              </communication-barring-serv-cap>\n\
          </cp:conditions>\n\
          <cp:actions>\n\
              <allow>false</allow>\n\
          </cp:actions>\n\
      </cp:rule>\n\
  </cp:ruleset>\n\
  </incoming-communication-barring>\n\
</simservs>"

#define SUPSRV_XCAP_CBIC_DEACTIVATE_DOC SUPSRV_XCAP_HEADER"<incoming-communication-barring active=\"false\"/>\n\
</simservs>"

/* Call Barring Outgoing Call */
#define SUPSRV_XCAP_CBOC_DEACTIVATE_DOC SUPSRV_XCAP_HEADER"<outgoing-communication-barring active=\"false\"/>\n\
</simservs>"

#define SUPSRV_XCAP_CBOG_ACTIVATE_DOC SUPSRV_XCAP_HEADER"<outgoing-communication-barring active=\"true\">\n\
  <cp:ruleset>\n\
      <cp:rule id=\"%s\">\n\
          <cp:conditions>\n\
              <communication-barring-serv-cap>\n\
                  %s\
              </communication-barring-serv-cap>\n\
          </cp:conditions>\n\
          <cp:actions>\n\
              <allow>false</allow>\n\
          </cp:actions>\n\
      </cp:rule>\n\
  </cp:ruleset>\n\
  </outgoing-communication-barring>\n\
</simservs>"

/* communication-diversion */
#define SUPSRV_XCAP_CD_DEACTIVATE_DOC SUPSRV_XCAP_HEADER"<communication-diversion active=\"false\">\n\
    <cp:ruleset>\n\
        <cp:rule id=\"rule1\">\n\
            <cp:conditions>\n\
                %s\n\
            </cp:conditions>\n\
        </cp:rule>\n\
    </cp:ruleset>\n\
    </communication-diversion>\n\
</simservs>"

#define SUPSRV_XCAP_CD_NO_REPLY_TIMER "<NoReplyTimer>%d</NoReplyTimer>"

#define SUPSRV_XCAP_CD_DOC SUPSRV_XCAP_HEADER"<communication-diversion active=\"true\">\n\
    %s\n\
    <cp:ruleset>\n\
        <cp:rule id=\"%s\">\n\
            <cp:conditions>\n\
                %s\n\
                %s\n\
            </cp:conditions>\n\
            <cp:actions>\n\
                <forward-to>\n\
                    <target>%s</target>\n\
                </forward-to>\n\
            </cp:actions>\n\
        </cp:rule>\n\
    </cp:ruleset>\n\
    </communication-diversion>\n\
</simservs>"

#define SUPSRV_XCAP_CD_TIME_RANGE "<time>%s</time>"

#define SUPSRV_PARAM_STRING_SZ                  (16)
#define SUPSRV_URI_STRING_SZ                    (128)

/* XCAP constants */
#define SUPSRV_XCAP_INIT_CUNTER                 (100)
#define SUPSRV_MAX_TRANSACTIONS                 (10)
#define SUPSRV_MAX_SCRATCH_PADS                 (2)
#define SUPSRV_SCRATCH_SZ                       (1024)
#define SUPSRV_STR_SZ                           (128)

#define SUPSRV_XCAP_TASK_NAME                   "SupSrvXCAP"
#define SUPSRV_XCAP_TASK_SIZE                   (4096)
#define SUPSRV_EVENT_STRING_SZ                  (128)

#define SUPSRV_EVENT_TYPE                       (5)
#define SUPSRV_NOREPLY_TIMER_DEFAULT            (20)
#define SUPSRV_TIME_STR_SZ                      (32)

/*
 ******************
 * SUPSRV enumeration
 ******************
 */
typedef enum {
    SUPSRV_OIR_DEFAULT = 0,
    SUPSRV_OIR_INVOCATION = 1,
    SUPSRV_OIR_SUPPRESSION = 2,
    SUPSRV_OIR_QUERY = 3
} SUPSRV_OirEventStatus;

typedef enum {
    SUPSRV_DISABLE = 0,
    SUPSRV_ENABLE,
    SUPSRV_QUERY,
    SUPSRV_REGISTRATION,
    SUPSRV_ERASURE
} SUPSRV_GenReqStatus;

typedef union {
    SUPSRV_OirEventStatus  oirReqStatus;
    SUPSRV_GenReqStatus    genReqStatus;
} SUPSRV_ReqStatus;

typedef enum {
   SUPSRV_EVENT_CB_MODE_BAOC = 0, /* Barring of All Outgoing Calls */
   SUPSRV_EVENT_CB_MODE_BOIC,     /* Barring of Outgoing International Calls */
   SUPSRV_EVENT_CB_MODE_BAIC,     /* Barring of All Incoming Callsl */
   SUPSRV_EVENT_CB_MODE_BICR,     /* Barring of Incoming Calls - When Roaming */
   SUPSRV_EVENT_CB_MODE_BOIC_EXHC,/* Bar Outgoing International Calls except to Home Country */
   SUPSRV_EVENT_CB_MODE_QUERY
} SUPSRV_CbMode;

typedef enum {
   SUPSRV_EVENT_CF_MODE_UNCONDITION = 0,   /* Communication Forwarding Unconditional */
   SUPSRV_EVENT_CF_MODE_BUSY,      /* Communication Forwarding on Busy */
   SUPSRV_EVENT_CF_MODE_NOREPLY,   /* Communication Forwarding on No Reply */
   SUPSRV_EVENT_CF_MODE_NOTREACH,  /* Communication Forwarding on not Reachable */
   SUPSRV_EVENT_CF_MODE_NOTLOGIN,  /* Communication Forwarding on not Logged in */
   SUPSRV_EVENT_CF_MODE_TIME = 9   /* CF on time of the day, CMCC only */
} SUPSRV_CFMode;

typedef union {
    SUPSRV_CbMode cbMode;
    SUPSRV_CFMode cfMode;
}SUPSRV_Mode;


typedef enum {
   SUPSRV_CMD_NONE = 0,
   SUPSRV_CMD_GET_SIMSERVS,
   SUPSRV_CMD_GET_OIP,
   SUPSRV_CMD_OIP_OPERATION,
   SUPSRV_CMD_GET_OIR,
   SUPSRV_CMD_OIR_OPERATION,
   SUPSRV_CMD_GET_CD,
   SUPSRV_CMD_CD_OPERATION,
   SUPSRV_CMD_GET_CBIC,
   SUPSRV_CMD_CBIC_OPERATION,
   SUPSRV_CMD_GET_CBICR,
   SUPSRV_CMD_CBICR_OPERATION,
   SUPSRV_CMD_GET_CBOG,
   SUPSRV_CMD_CBOG_OPERATION,
   SUPSRV_CMD_GET_CBOIC,
   SUPSRV_CMD_CBOIC_OPERATION,
   SUPSRV_CMD_GET_TIP,
   SUPSRV_CMD_TIP_OPERATION,
   SUPSRV_CMD_GET_TIR,
   SUPSRV_CMD_TIR_OPERATION,
   SUPSRV_CMD_GET_CW,
   SUPSRV_CMD_CW_OPERATION,
   SUPSRV_CMD_LAST
} SUPSRV_CmdType;

/* CF NoReplyTimer */
typedef enum {
    SUPSRV_NOREPLY_NONE = 0,
    SUPSRV_NOREPLY_MIN = 5,
    SUPSRV_NOREPLY_MAX = 180
} SUPSRV_NoReplyRange;

/* General Response status */
typedef enum {
    SUPSRV_RES_DISABLE = 0,
    SUPSRV_RES_ENABLE = 1
} SUPSRV_GenResStatus;

/* Reason for this Supplementary Service event. */
typedef enum {
    SUPSRV_EVENT_REASON_AT_CMD_OIP = 0,  /* AT+CLIP */
    SUPSRV_EVENT_REASON_AT_CMD_OIR,      /* AT+CLIR */
    SUPSRV_EVENT_REASON_AT_CMD_TIP,      /* AT+COLP */
    SUPSRV_EVENT_REASON_AT_CMD_TIR,      /* AT+COLR */
    SUPSRV_EVENT_REASON_AT_CMD_CW,       /* AT+CCWA */
    SUPSRV_EVENT_REASON_AT_CMD_CF,       /* AT+CCFC */
    SUPSRV_EVENT_REASON_AT_CMD_CB,       /* AT+CLCK */
    SUPSRV_EVENT_REASON_RADIO_INFC,      /* ip change */
} SUPSRV_EventReason;

typedef union {
    SUPSRV_OirEventStatus  oirResStatus;
    SUPSRV_GenResStatus    genResStatus;
} SUPSRV_ResStatus;

/* supsrv provision status/mode */
typedef enum {
    SUPSRV_OIR_NOT_PROVISIONED = 0,
    SUPSRV_OIR_PERM_MODE = 1,
    SUPSRV_OIR_UNKNOWN = 2,
    SUPSRV_OIR_TEMP_MODE_RESTRICTED = 3,
    SUPSRV_OIR_TEMP_MODE_ALLOWED = 4,
} SUPSRV_OirProvision;

/* supsrv provision status/mode */
typedef enum {
    SUPSRV_TIR_NOT_PROVISIONED = 0,
    SUPSRV_TIR_PERM_MODE = 1,
    SUPSRV_TIR_UNKNOWN = 2,
    SUPSRV_TIR_TEMP_MODE_RESTRICTED = 3,
    SUPSRV_TIR_TEMP_MODE_ALLOWED = 4,
} SUPSRV_TirProvision;

typedef enum {
    SUPSRV_NOT_PROVISIONED = 0,
    SUPSRV_PROVISIONED = 1,
    SUPSRV_UNKNOWN = 2,
} SUPSRV_GenProvision;

typedef union {
    SUPSRV_OirProvision  oirProv;
    SUPSRV_TirProvision  tirProv;
    SUPSRV_GenProvision  genProv;
} SUPSRV_Provision;

/* SUPSRV output response reason enumveration. */
typedef enum {
    SUPSRV_OUTPUT_REASON_OK,        /* '0' or 'OK' response to a command. */
    SUPSRV_OUTPUT_REASON_ERROR,     /* +CME ERROR response to a command. The error code will be in the payload. */
    SUPSRV_OUTPUT_REASON_QUERY_RESULT 
}SUPSRV_OutputReason;

/* SupSrv Error Code. */
typedef enum {
    SUPSRV_ERROR_NONE           = 0,
    SUPSRV_ERROR_NO_NET_SERVICE = 30,  /*err =30 : no network service */
    SUPSRV_ERROR_NET_TIMEOUT    = 31,  /*err =31 :network timeout */
    SUPSRV_ERROR_UNKNOWN        = 100, /* err=100 :unknown */
    SUPSRV_ERROR_HTTP           = 601, /* http error header in reasonDesc */
    SUPSRV_ERROR_XCAP_CONFLICT  = 602, /* 409 with xcap-error+xml in reasonDesc */
    SUPSRV_ERROR_XML_PARSING    = 603, /* XML content invalid or failed to parse */
} SUPSRV_ErrorCode;

/* XCAP resaon */
typedef enum {
    SUPSRV_REASON_XCAP_EVT,
    SUPSRV_NONE
} SUPSRV_XcapReason;

typedef struct {
    SUPSRV_XcapReason  reason;
    XCAP_Evt           evt;
} SUPSRV_XcapEvt;

typedef struct {
    vint                 type;
    SUPSRV_XcapEvt       xcap;
} SUPSRV_Evt;

typedef struct {
    uint32              cmdCnt;
    SUPSRV_CmdType      cmdType;    
    char                scratch[SUPSRV_MAX_SCRATCH_PADS][SUPSRV_SCRATCH_SZ];
} SUPSRV_XcapTrans;

typedef vint (*pfonResultCB)(void *arg_ptr, SUPSRV_Evt *evt_ptr);

typedef struct {
    XCAP_Obj             xcap;
    vint                 timeout;
    uint32               cmdCnt;
    vint                 protoType;
    char                 server[SUPSRV_STR_SZ];
    char                 username[SUPSRV_STR_SZ];
    char                 password[SUPSRV_STR_SZ];
    char                 uri[SUPSRV_STR_SZ];
    char                 impu[SUPSRV_STR_SZ];
    SUPSRV_XcapTrans     trans[SUPSRV_MAX_TRANSACTIONS];
    OSAL_NetAddress      infcAddress; /* LTE APN ip for supsrv/xcap access */
} SUPSRV_XcapObj;

typedef struct {
    int     noReplyTimer;
    char    mediaType[SUPSRV_PARAM_STRING_SZ];
    char    timeRangeOfTheDay[SUPSRV_PARAM_STRING_SZ]; /* time range "HH:MM,HH:MM" */
    char    cfwNumber[SUPSRV_URI_STRING_SZ + 1];
    uint32  addrType; /* type of address octet in integer format
                    * (refer 3GPP TS 24.008 subclause 10.5.4.7);
                    * default 145 when dialling string includes
                    * international access code character "+",
                    * otherwise 129
                    */
} SUPSRV_RuleParams;

typedef struct {
    SUPSRV_EventReason    reason;
    char                  reasonDesc[SUPSRV_EVENT_STRING_SZ + 1];
    SUPSRV_Mode           mode;
    SUPSRV_ReqStatus      status;
    SUPSRV_RuleParams     ruleParams;
} SUPSRV_Input;

/*
 * SUPSRV_Output
 *
 * Structure containing information about SUPSRV
 */
typedef struct {
    SUPSRV_OutputReason    reason;
    char                   reasonDesc[SUPSRV_EVENT_STRING_SZ + 1];
    SUPSRV_CmdType         cmdType;
    SUPSRV_Mode            mode;
    SUPSRV_ResStatus       queryEnb;
    SUPSRV_Provision       prov;
    SUPSRV_ErrorCode       errorCode;
    union {
        SUPSRV_RuleParams ruleParams;
    } u;
} SUPSRV_Output;

#if defined(PROVIDER_CMCC)
/* for supsrv config that are implemented in local UE */
typedef struct {
    SUPSRV_GenResStatus cwSetting;
} SUPSRV_LocalSettings;
#endif

typedef struct {
    OSAL_TaskId      tId;
    void            *arg_ptr; 
    pfonResultCB     onResultCB;    
    SUPSRV_XcapObj   supSrvXcap;
    SUPSRV_Input     inEvt; /* Scratch for processing input event */
    SUPSRV_Output    outEvt; /* Scratch for processing output event */
#if defined(PROVIDER_CMCC)
    SUPSRV_LocalSettings localSettings;
#endif
} SUPSRV_Mngr;

vint SUPSRV_init(
    SUPSRV_Mngr  *supsrvMngr_ptr,
    pfonResultCB  funcCB);

vint SUPSRV_shutdown(
    SUPSRV_Mngr *supSrv_ptr);

void SUPSRV_initTrans(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_simServsGet(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_queryOip(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_activateOip(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_deactivateOip(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_queryOir(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_activateOir(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_deactivateOir(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_queryCd(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_activateCd(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_deactivateCd(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_updateCd(
    SUPSRV_XcapObj *xcap_ptr,
    SUPSRV_Input   *evt_ptr);

vint SUPSRV_updateCbic(
    SUPSRV_XcapObj *xcap_ptr,
    SUPSRV_Input   *evt_ptr);

vint SUPSRV_updateCboc(
    SUPSRV_XcapObj *xcap_ptr,
    SUPSRV_Input   *evt_ptr);

vint SUPSRV_queryTip(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_activateTip(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_deactivateTip(
    SUPSRV_XcapObj *xcap_ptr);

vint _SUPSRV_updateTip(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate);

vint SUPSRV_queryTir(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_activateTir(
    SUPSRV_XcapObj *xcap_ptr);

vint SUPSRV_deactivateTir(
    SUPSRV_XcapObj *xcap_ptr);

vint _SUPSRV_updateTir(
    SUPSRV_XcapObj  *xcap_ptr,
    vint             activate);

vint SUPSRV_updateCw(
    SUPSRV_XcapObj  *xcap_ptr,
    SUPSRV_Input    *evt_ptr);

vint SUPSRV_isCfwModeExist(
    SUPSRV_CFMode  mode,
    char             *res_ptr);

vint SUPSRV_parseCfwResult(
    char            *res_ptr,
    SUPSRV_Output   *out_ptr);

vint SUPSRV_parseCbResult(
    char            *res_ptr,
    SUPSRV_Output   *out_ptr);

vint SUPSRV_getXcapEvtOwner(
    SUPSRV_Mngr  *mngr_ptr);

vint SUPSRV_getXcapResponseCode(
    XCAP_Evt *xcapEvt_ptr);

vint SUPSRV_parseSupSrvQueryResult(
    const XCAP_Evt  *xcapEvt_ptr,
    SUPSRV_Output   *out_ptr,
    SUPSRV_CmdType   cmdType);

vint SUPSRV_xcapEventProcess(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    XCAP_Evt        *xcapEvt_ptr,
    SUPSRV_Output   *out_ptr);
    
vint SUPSRV_allocate(
    SUPSRV_Mngr  *supsrvMngr_ptr,
    pfonResultCB  funcCB);

vint SUPSRV_start(
     SUPSRV_Mngr *supsrvMngr_ptr);

vint SUPSRV_destroy(
    SUPSRV_Mngr *supSrvMngr_ptr);

vint SUPSRV_processIpChange(
    SUPSRV_Mngr     *supSrvMngr_ptr,
    OSAL_NetAddress *ipAddr_ptr);

#if defined(PROVIDER_CMCC)

vint SUPSRV_setLocalCwSetting(
    SUPSRV_GenResStatus status);

SUPSRV_GenResStatus SUPSRV_getLocalCwSetting(void);

#endif

#endif

