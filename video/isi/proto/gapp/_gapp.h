/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */

#ifndef _GAPP_H_
#define _GAPP_H_

#define GAPP_PROTO_DIRECTORY    "../isi/proto/"

#include "../../../csm/csm_event.h"

#include "proxy.h"
#ifndef GAPP_DISABLE_GSM
#include "gsm.h"
#endif

/* Return codes. */
#define GAPP_OK                     (0)
#define GAPP_ERR                    (-1)

#define GAPP_MAX_ID                 (0x00FFFFFF)

#define GAPP_MAX_QUEUE_DEPTH        (16)

#define GAPP_GSM_TASK_NAME          "Gapp"

/* Task Related settings */

#define GAPP_TASK_STACK_BYTES       (4096)

/* Queue and IPC definitions */
#define GAPP_MSGQ_LEN               (8)

/* Generally used definitions */
#define GAPP_STRING_SZ              (128)
#define GAPP_IM_MAX_SZ              (1024) 
#define GAPP_MAX_BLOCKED_USERS      (25)

/* Max number of allowable calls per GSM Endpoint (Per Service) */
#define GAPP_CALL_NUM               (6)

/* Retry timer in ms when registering GAPP to ISI */
#define GAPP_REGISTER_TIMER_MS      (5000)

/* 
 * Offset (in number of bytes) into MWI string that indicates the number of 
 * unread messages and the length of the entire MWI string.  
 */
#define GAPP_MWI_STATUS_OFFSET_START (6)
#define GAPP_MWI_STATUS_OFFSET_END   (10)

#ifndef GAPPD_DEBUG
#define GAPP_dbgPrintf(x, ...)
#else
#ifdef VPORT_4G_PLUS_APROC
#define GAPP_dbgPrintf(fmt, args...) \
         OSAL_logMsg("AP:" fmt, ## args)
#else
#define GAPP_dbgPrintf(fmt, args...) \
         OSAL_logMsg("MP:" fmt, ## args)
#endif
#endif

#define GAPP_GSM_SMS_CMD_TO_SECS      (20)

#define GAPP_GSM_CFWD_CMD_TO_SECS     (20)

#define GAPP_GSM_REG_CMD_TO_SECS      (60)

#define GAPP_FMC_MAX_ASSD_STR_SZ      (8)

#define GAPP_FMC_MAX_ASSD_FEATURES    (12)

#define GAPP_FMC_CMD_TO_SECS          (25)

#define GAPP_REG_TABLE_SIZE           (6)
#define GAPP_AT_COMMAND_SIZE          (512)

/* 
 * The duration of this command should be at least as long as the maximum 
 * DTMF digit duration. For GSM that's 3 seconds.
 */
#define GAPP_GSM_DTMF_CMD_TO_SECS     (3)

#define GAPP_GSM_SUBSTITUTE_CHAR      (0x1A)

#define GAPP_GSM_DTMF_DURATION_DIVISOR (100)

//#define GAPP_GSM_OK                       "OK"
#define GAPP_GSM_OK                       "0"
//#define GAPP_GSM_ERROR                    "ERROR"
#define GAPP_GSM_ERROR                    "4"
#define GAPP_GSM_CRLF                     "\r\n"
#define GAPP_GSM_CR                       "\r"
#define GAPP_GSM_SERVICE_OPERATOR         "AT+COPS="
#define GAPP_GSM_SERVICE_REG              "+CREG?"
#define GAPP_GSM_SERVICE_REG_REPORT       "+CREG:"
#define GAPP_GSM_EMGCY_SERVICE_REG_REPORT  "%EREG:"
#define GAPP_GSM_SIGNAL_REPORT            "+CSQ:"
#define GAPP_GSM_SERVICE_SMS_SEND         "AT+CMGS="
#define GAPP_GSM_SERVICE_SMS_PROMPT       "> "
#define GAPP_GSM_SERVICE_CALL_FORWARD     "AT+CCFC="
#define GAPP_GSM_CALL_DTMF                "AT+VTS="
#define GAPP_GSM_CALL_CLIP                "+CLIP:"
#define GAPP_GSM_CALL_WAITING             "+CCWA:"
#define GAPP_GSM_CALL_DIAL_URI            "+CDU:"
#define GAPP_GSM_CALL_RINGSTOP            "STOP RING"
#define GAPP_GSM_CALL_RING1               "RING"
#define GAPP_GSM_CALL_RING2               "+CRING: VOICE"
//#define GAPP_GSM_CALL_NO_CARRIER          "NO CARRIER"
#define GAPP_GSM_CALL_NO_CARRIER          "3"
#define GAPP_GSM_SMS_TP_MR                "TP-MR:"

#ifdef GSM_NOKIA
#define GAPP_GSM_CALL_HANGUP              "AT+CHUP"
#else
#define GAPP_GSM_CALL_HANGUP              "ATH"
#endif

#define GAPP_GSM_CALL_REPORT_CMD          "AT+CLCC"
#define GAPP_GSM_CALL_STATUS              "+CLCC:"
#define GAPP_GSM_CALL_EXT_STATUS          "+CLCCS:"
#define GAPP_GSM_CALL_MONITOR             "+CMCCSI:"
#define GAPP_GSM_CALL_REPORT              "CALL REPORT"
#define GAPP_GSM_CALL_DIAL                "ATD"
#define GAPP_GSM_CALL_ANSWER              "ATA"
#define GAPP_GSM_CALL_HOLD_0              "AT+CHLD=0"
#define GAPP_GSM_CALL_HOLD_1              "AT+CHLD=1"
#define GAPP_GSM_CALL_HOLD_2              "AT+CHLD=2"
#define GAPP_GSM_CALL_HOLD_3              "AT+CHLD=3"
#define GAPP_GSM_CALL_BLIND_TRANSFER      "AT+CHLD=4"
#define GAPP_GSM_CALL_FORWARD             "AT+CTFR="
#define GAPP_GSM_SMS_RECVD                "+CMT:"
#define GAPP_GSM_SMS_REPORT_RECVD         "+CMT REPORT:"
#define GAPP_GSM_SMS_MEM_RECVD            "+CMTI:"
#define GAPP_GSM_SMS_MEM_READ_CMD         "AT+CMGR="
#define GAPP_GSM_SMS_MEM_DEL_CMD          "AT+CMGD="
#define GAPP_GSM_SMS_MEM_READ_RESP        "+CMGR:"
#define GAPP_GSM_SMS_SENT                 "+CMGS:"
#define GAPP_GSM_NO_OP                    "AT"

#define GAPP_GSM_CALL_SRVCC_START         "+CIREPH:0" // XXX Custom Pete-1 command
#define GAPP_GSM_CALL_SRVCC_SUCCESS       "+CIREPH:1"
#define GAPP_GSM_CALL_SRVCC_FAILURE       "+CIREPH:2"

/* 
 * The following is +CHLD argument logic.  The AT+CHLD command is described
 * in GSM AT Command specifications.
 *
 * 0 = Releases all held calls or sets User Determined User Busy (UDUB) for
 *     a waiting call.
 *
 * 1 = Releases all active calls (if any exist) and accepts the other (held or
 *     waiting) call.
 *
 * 1<idx> = Releases specified active call only (<idx>).
 *
 * 2 = Places all active calls (if any exist) on hold and accepts the other
 *     (held or waiting) call.
 *
 * 2<idx> = Request private consultation mode with specified call (<idx>).
 *          (Place all calls on hold EXCEPT the call indicated by <idx>.)
 *
 * 3 = Adds a held call to the conversation.
 *
 * 4 = Connects the two calls and disconnects the subscriber from both calls
 *   (Explicit Call Transfer). Support for this value and its associated
 *   functionality is optional for the HF.
 */

/* 
 * The following is the AT command logic used in this application.  This matrix
 * represents which AT commands were used for the various call progress events.
 * "arh" are bit maps for each call and the bits represent
 * "Active", "Ring", "Hold".
 */
/*
arh | arh  Ans A  Ans B     Hold A  Hold B  Resume A  Resume B  Disc A   Disc B   Reject A  Reject B
000 | 000  X      X      X       X       X         X         X        X        X         X
100 | 000  X      X      CLHD=2  X       X         X         ATH      X        X         X
101 | 000  X      X      X       X       CHLD=2    X         ATH      X        X         X
110 | 000  ATA    X      X       X       X         X         ATH      X        ATH       X
000 | 100  X      X      X       CHLD=2  X         X         ATH      X        ATH       X
100 | 100  X      X      CHLD=2X CHLD=2X X         X         CHLD=1X  CHLD=1X  X         X
101 | 100  X      X      X       CHLD=2  CHLD=2    X         CHLD=0   CHLD=1X  X         X
110 | 100  CHLD=2 X      X       CHLD=2  X         X         CHLD=0   CHLD=1X  CHLD=0    X
000 | 101  X      X      X       X       X         CHLD=2    X        ATH      X         X
100 | 101  X      X      CHLD=2  X       X         CHLD=2    CHLD=1X  CHLD=0   X         X
110 | 101  ATA    X      X       X       X         X         CHLD=0   X        CHLD=0    X
000 | 110  X      ATA    X       X       X         X         X        ATH      X         ATH
100 | 110  X      CHLD=2 CHLD=2  X       X         X         CHLD=1X  CHLD=0   X         CHLD=0
101 | 110  X      ATA    X       X       X         X         X        CHLD=0   X         CHLD=0
*/

typedef struct {
   char        *start_ptr;  /* points to start of token */
   uint32       length;     /* token's length (except delimiter) */
   char        *dmtr_ptr;   /* points to start of delimiter */
} GAPP_Token;

typedef struct {
   char        *start_ptr;
   char        *curr_ptr;
   uint16       length;
   OSAL_Boolean isEnd;
   GAPP_Token   token;
} GAPP_Buffer;

/* Information about a created task is placed here. */
typedef struct {
    OSAL_TaskId tId;
    uvint       stackSz;
    uvint       pri;
    void      *func_ptr;
    char       name[16];
    void      *arg_ptr;
} GAPP_TaskObj;

/* GAPP struct to exchange ISI commands and GSM events. */
typedef struct {
    union {
       ISIP_Message   isi;
#ifndef GAPP_DISABLE_GSM
       GSM_Event      gsm;
#endif
       char           proxy[PRXY_AT_COMMAND_SIZE + 1];
       CSM_OutputEvent   response;
    } u;
} GAPP_Msg;

typedef struct {
    vint maxUsers;
    char aUsers[GAPP_MAX_BLOCKED_USERS][GAPP_STRING_SZ];
} GAPP_BlockUsers;

typedef struct {
    ISI_Id          isiCallId;
    OSAL_Boolean    isFmc;
    vint            state;
    GSM_Id          gsmId;
    char            vdi[GAPP_STRING_SZ];
    char            to[GAPP_STRING_SZ];
    vint            idx;
    vint            isVcc;
    ISI_SessionType sessionType; /* Cache session type remote proposed */
    vint            negStatus; /* Media negotiation status. */
    vint            isLocalModified; /* Local modified the session. */
} GAPP_CallObj;

typedef struct {
    char    type[GAPP_FMC_MAX_ASSD_STR_SZ];
    vint    isiEnum;
    char    pre[GAPP_FMC_MAX_ASSD_STR_SZ];
    char    post[GAPP_FMC_MAX_ASSD_STR_SZ];
    char    complete[GAPP_FMC_MAX_ASSD_STR_SZ];
    char    outboundPrefix[GAPP_FMC_MAX_ASSD_STR_SZ];
    vint    outboundLength;
} GAPP_FmcFeature;

typedef struct {
    vint             digitDuration;
    char             delimiter[GAPP_FMC_MAX_ASSD_STR_SZ];
    GAPP_FmcFeature  features[GAPP_FMC_MAX_ASSD_FEATURES];
    char             scratch[GAPP_STRING_SZ];
} GAPP_Fmc;

typedef struct {
    ISI_Id             isiServiceId;
    vint               protocolId;
    char               fmcServer[GAPP_STRING_SZ];
    char               fmcDisaPassword[GAPP_STRING_SZ];
    GAPP_CallObj       aCall[GAPP_CALL_NUM];
    struct {
        uint32         active;
        uint32         hold;
        uint32         ring;
        uint32         conf;
        uint32         modify; /* Indicate if local modified session media. */
    } callBits;
    char               vdi[GAPP_STRING_SZ];
    char               networkOperator[GAPP_STRING_SZ];
    vint               ringTemplate;
    char               scratch[GAPP_STRING_SZ];
    GAPP_BlockUsers    blockedUsers;
    vint               blockCid;
    struct {         
        OSAL_Boolean   isRegistered;
        GSM_Id         regGsmId;
        GSM_Id         unregGsmId;
    } reg;
    struct {         
        ISI_Id         isiId;
        char           msg[GAPP_IM_MAX_SZ + 1];
        GSM_Id         gsmId;
        OSAL_Boolean   isConstructed;
    } sms;
    struct {         
        ISI_Id         isiId;
        GSM_Id         gsmId;
        ISI_TelEvent   evt;
    } telEvt;
    struct {         
        GSM_Id         gsmId;
    } smsRead;
    void              *daemon;
    PRXY_NetReg       *netReg;
    OSAL_Boolean       extDialCmdEnabled;
} GAPP_ServiceObj;

typedef struct {
    ISIP_Message   isiMsg;
    OSAL_MsgQId    isiEvtQId;
} GAPP_Event;

/* GAPP Object to manage all everything */
typedef struct {
    GAPP_TaskObj         task;
    struct {
        OSAL_MsgQId      isiCmd;
        OSAL_MsgQId      gsmEvt;
        OSAL_MsgQId      respEvt;
        OSAL_MsgQId      proxyCmdEvt;
        OSAL_MsgQGrpId  *group_ptr;
        OSAL_MsgQGrpId   groupAll;
        OSAL_MsgQGrpId   groupResponsesOnly;
        GAPP_Msg         msg;

    } queue;
    struct {
        OSAL_TmrId       id;
        GAPP_Event       event;
    } tmr;
    GAPP_Event           event;
    GAPP_ServiceObj      service;
    GAPP_Fmc             fmc;
    char                 gsmDrvrFile[GAPP_STRING_SZ];
    char                 proxyTerminalName[GAPP_STRING_SZ];
    PRXY_CommandMngr     proxyCmdMngr;
} GAPP_GsmObj;


/*
 * This is the GAPP global object.
 * Anything allocated statically must be placed in this object.
 * It get allocated in .bss.
 */
typedef struct {
    vint        protocolId;
    GAPP_GsmObj gsmObj;
} GAPP_GlobalObj;
        
vint _GAPP_mixNetRegStatus(    
    GAPP_ServiceObj *service_ptr);

vint GAPP_searchBlockedUser(
    GAPP_BlockUsers *block_ptr,
    char            *uri_ptr,
    vint             shouldRemove);

vint GAPP_sendEvent(
    GAPP_Event *evt_ptr);

vint GAPP_getToken(
    GAPP_Buffer  *buff_ptr, 
    const char   *esc_ptr);

void GAPP_initBuffer(
    char        *result_ptr,
    vint         resultLen,
    GAPP_Buffer *buff_ptr);

vint GAPP_chkResult(
    GAPP_Buffer  *result_ptr,
    const char   *compare_ptr,
    vint          compareSize);

void GAPP_initService(
    GAPP_ServiceObj *service_ptr,
    GAPP_GsmObj     *gsm_ptr,
    ISI_Id           isiServiceId,
    vint             protocolId);
#ifndef GAPP_DISABLE_GSM
void GAPP_isiServiceCmd(
    GAPP_GsmObj     *gsm_ptr, 
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr, 
    ISIP_Message    *isi_ptr);

void GAPP_isiSmsCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr, 
    ISIP_Message    *isi_ptr);

void GAPP_isiTelEventCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    ISIP_Message    *isi_ptr);

vint GAPP_serviceResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    GAPP_Event      *evt_ptr);

vint GAPP_serviceUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr);

vint GAPP_smsResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    ISIP_Message    *isi_ptr);

vint GAPP_telEvtResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    ISIP_Message    *isi_ptr);

vint GAPP_smsUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    ISIP_Message    *isi_ptr);

vint GAPP_networkRegSolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr);
#endif

vint GAPP_processCsmRegOutputEvent(
    GAPP_ServiceObj *service_ptr,
    char            *out_ptr);

void GAPP_telEvtIsiEvt(
    ISI_Id              evtId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_TelEvtReason   reason,
    ISI_TelEvent        event,
    ISIP_Message       *isi_ptr);

vint getAddress(
    char *source_ptr,
    vint  maxSourceLen,
    char *destination_ptr,
    vint  maxDestinationLen,
    char *delimiter_ptr,
    vint  numDelimiter);

vint getAddressAndIndex(
    char *source_ptr,
    vint  maxSourceLen,
    char *destination_ptr,
    vint  maxDestinationLen,
    vint *callIdx_ptr,
    char *delimiter_ptr,
    vint  numDelimiter);

vint GAPP_mcmNotify(
    GAPP_GsmObj *obj_ptr);

ISI_Id GAPP_getUniqueIsiId(ISI_Id serviceId);



#endif
