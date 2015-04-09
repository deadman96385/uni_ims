/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29252 $ $Date: 2014-10-10 08:15:06 +0800 (Fri, 10 Oct 2014) $
 *
 */

#ifndef __TAPP_H_
#define __TAPP_H_

#include <isi_rpc.h>
#include <isi_xdr.h>
#include <xcap.h>

#ifndef TAPP_DEBUG
#define TAPP_dbgPrintf(fmt, args...)
#else
#define TAPP_dbgPrintf(fmt, args...) \
         OSAL_logMsg("[%s:%d] " fmt, __FUNCTION__, __LINE__, ## args)
#endif


/* TAPP constants definition */
#define TAPP_MAX_CALL_NUMS              (3)
#define TAPP_MAX_SERVICE_NUMS           (3)
#define TAPP_AT_COMMAND_STRING_SZ       (512)
#define TAPP_SCRATCH_STRING_SZ          (1024)
#define TAPP_XML_DOC_PATH_NAME_SZ       (128)
#define TAPP_DEVICE_NAME_SZ             (128)
#define TAPP_MAX_ACTION_SZ              (8192)
#define TAPP_MAX_TESTCASE_SZ            (1024)
#define TAPP_TESTCASE_NAME_STRING_SZ    (128)
#define TAPP_TESTSUITE_NAME_STRING_SZ   (64)
#define TAPP_FAILURE_STR_SZ             (128)
#define TAPP_ACTION_STR_SZ              (64)
#define TAPP_MAX_ISIP_ENUM_SZ           (64)
#define TAPP_ISI_RPC_DATA_TYPE_INT      (1)
#define TAPP_ISI_RPC_DATA_TYPE_STR      (2)

#define TAPP_XML_TESTCASE_TAG           "testcase"
#define TAPP_XML_INCLUDE_TAG            "include"
#define TAPP_XML_ACTION_TAG             "action"

/* ideally, these should be in _tapp_mock_xcap.h, 
 * but would cause circular reference. refactor later
 */
#define TAPP_MOCK_XCAP_HEADER_SZ  (1024)
#define TAPP_MOCK_XCAP_BODY_SZ    (4096)
#define TAPP_MOCK_XCAP_STRING_SZ  (128)

/** 
 * xcap used pointers to buffers but we need real buffers for storing the actions
 * so here we define mock command and event objects similar 
 * to xcap_cmd and xcap_evt but with buffers
 */
typedef struct {
    XCAP_Operation      op;         /**< Set an op */
    XCAP_OperationType  opType;     /**< Set an op type */
    XCAP_Condition      cond;       /**< Set a condition */
    char                uri[TAPP_MOCK_XCAP_HEADER_SZ];    /* Set target URI */
    char                username[TAPP_MOCK_XCAP_STRING_SZ]; /* Set username for authentcation */
    char                password[TAPP_MOCK_XCAP_STRING_SZ]; /* Set password for authentcation */
    char                x3gpp[TAPP_MOCK_XCAP_STRING_SZ];    /* Set ID for the 3GPP network */
    char                auid[TAPP_MOCK_XCAP_STRING_SZ];   /* Set to AUID as in above uri */
    char                etag[TAPP_MOCK_XCAP_STRING_SZ];   /* ETag if .cond is set */
    char                body[TAPP_MOCK_XCAP_BODY_SZ];
} TAPP_mockXcapCmd;

typedef struct {
    XCAP_EvtErr   error;    /* Error code */
    char         hdr[TAPP_MOCK_XCAP_HEADER_SZ];  /*  HTTP header, unparsed */
    char         body[TAPP_MOCK_XCAP_BODY_SZ]; /*  HTTP body, unparsed */
} TAPP_mockXcapEvt;

/* TAPP return enumeration */
typedef enum {
    TAPP_FAIL = -1,
    TAPP_PASS = 0,
} TAPP_Return;

/* TAPP Data type enumeration */
typedef enum {
    TAPP_DATA_TYPE_INTEGER,
    TAPP_DATA_TYPE_STRING,
    TAPP_DATA_TYPE_HEX,
    TAPP_DATA_TYPE_ENUM,
    TAPP_DATA_TYPE_ADDR,
    TAPP_DATA_TYPE_XML_DOC,
    TAPP_DATA_TYPE_MEDIA_ATTR,
    TAPP_DATA_TYPE_LAST
} TAPP_DataType;

typedef struct {
    vint index;
    char string[TAPP_MAX_ISIP_ENUM_SZ];
} TAPP_EnumObj;

/* TAPP action type enumeration */
typedef enum {
    TAPP_ACTION_TYPE_ISSUE_AT,
    TAPP_ACTION_TYPE_VALIDATE_AT,
    TAPP_ACTION_TYPE_ISSUE_ISIP,
    TAPP_ACTION_TYPE_VALIDATE_ISIP,
    TAPP_ACTION_TYPE_ISSUE_GSM_AT,
    TAPP_ACTION_TYPE_VALIDATE_GSM_AT,
    TAPP_ACTION_TYPE_ISSUE_CSM,
    TAPP_ACTION_TYPE_ISSUE_XCAP,
    TAPP_ACTION_TYPE_VALIDATE_XCAP,
    TAPP_ACTION_TYPE_ISSUE_ISI_RPC,
    TAPP_ACTION_TYPE_VALIDATE_ISI_RPC_RETURN,
    TAPP_ACTION_TYPE_VALIDATE_ISI_GET_EVT,
    TAPP_ACTION_TYPE_CLEAN_ISI_EVT,
    TAPP_ACTION_TYPE_PAUSE,
    TAPP_ACTION_TYPE_CLEAN_ISIP,
    TAPP_ACTION_TYPE_REPEAT_START,
    TAPP_ACTION_TYPE_REPEAT_END,
    TAPP_ACTION_TYPE_LAST,
} TAPP_ActionType;

/* TAPP event type enumeration */
typedef enum {
    TAPP_EVENT_TYPE_ISIP,
    TAPP_EVENT_TYPE_AT_INFC,
    TAPP_EVENT_TYPE_GSM_DEV,
    TAPP_EVENT_TYPE_TIMEOUT,
    TAPP_EVENT_TYPE_XCAP,
    TAPP_EVENT_TYPE_ISI_RPC,
    TAPP_EVENT_TYPE_LAST,
} TAPP_EventType;

typedef enum {
    TAPP_ID_TYPE_NONE,
    TAPP_ID_TYPE_SERVICE,
    TAPP_ID_TYPE_CALL,
} TAPP_IdType;


typedef struct {
    ISI_ApiName   funcType;
    ISI_Xdr       isiXdr;
    OSAL_Boolean  isDontCare[ISI_DATA_SIZE];
    char          dataType[ISI_DATA_SIZE];
    struct {
        TAPP_IdType   idType;
        int           idIdx;
        unsigned char paramIdx;
    } id;
    
} RPC_Message;

/* TAPP event struct */
typedef struct {
    TAPP_EventType type;
    union {
        ISIP_Message isip;
        char at[TAPP_AT_COMMAND_STRING_SZ];
        CSM_InputEvent csm;
        XCAP_Cmd xcapCmd;
        RPC_Message rpc;
    } msg;
} TAPP_Event;

/*
 * TAPP action strcut
 * TAPP_Action is the basic element of Malibu unit test
 * which defines the action type and the data for issuing or validaing
 */
typedef struct {
    TAPP_ActionType type;
    vint            stop;  /* while testing result is failed ,test stop or not */
    union {
        vint            timeout; /* Timeout value for this action, in ms unit */
        vint            pause;   /* Pause in ms unit */
        vint            repeat;  /* To indicate the repeat time if the type is
                                    TAPP_ACTION_TYPE_REPEAT_START.
                                    And to indicate the loop start index if the
                                    type is TAPP_ACTION_TYPE_REPEAT_END. */
    } u;
    union {
        ISIP_Message isip;
        char at[TAPP_AT_COMMAND_STRING_SZ];
        CSM_InputEvent csm;
        TAPP_mockXcapCmd mockXcapCmd;
        TAPP_mockXcapEvt mockXcapEvt;
        RPC_Message      rpcMsg;
    } msg;
    char            testCaseName[TAPP_TESTCASE_NAME_STRING_SZ];
    OSAL_Boolean    endOfTestCase;
} TAPP_Action;

/* Report result. */
typedef struct {
    struct {
        char name[TAPP_TESTCASE_NAME_STRING_SZ];
        OSAL_Boolean pass;
    } testCase[TAPP_MAX_TESTCASE_SZ];
    char testsuiteName[TAPP_TESTSUITE_NAME_STRING_SZ];
    int  totalCases;
    int  failureCases;
    int  curTestCase;
} TAPP_ReportResult;

/* TAPP global object */
typedef struct {
    char testsuiteName[TAPP_TESTSUITE_NAME_STRING_SZ];
    struct {
        OSAL_FileId     atInfc;
        OSAL_FileId     gsm;
        OSAL_FileId     isiCmd;
        OSAL_FileId     xcapCmd;
        OSAL_FileId     rpcCmd;
        OSAL_FileId     rpcEvtCmd;
        OSAL_SelectSet  set;
    } fd;
    struct {
        OSAL_MsgQId isiCmd;
        OSAL_MsgQId isiEvt;
        OSAL_MsgQId csmEvt;
        OSAL_MsgQId xcapCmd;
        OSAL_MsgQId xcapEvt;
        OSAL_MsgQId rpcWrite;
        OSAL_MsgQId rpcRead;
        OSAL_MsgQId rpcWEvt;
        OSAL_MsgQId rpcREvt;
    } queue;
    struct {
        TAPP_Action actions[TAPP_MAX_ACTION_SZ];
        vint        size;
        vint        curIdx;
    } actionList;
    struct {
        OSAL_TmrId      id;
        ISIP_Message    event;
    } tmr;
    struct {
        OSAL_FileId fileId;
        char fileName[TAPP_XML_DOC_PATH_NAME_SZ];
    } report;
    struct {
        OSAL_FileId fileId;
        char fileName[TAPP_XML_DOC_PATH_NAME_SZ];
    } xmlReport;
    TAPP_Event  tappEvt;
    char        csmXmlFileName[TAPP_XML_DOC_PATH_NAME_SZ];
    char        gsmDevName[TAPP_DEVICE_NAME_SZ];
    char        scratch[TAPP_SCRATCH_STRING_SZ];
    ISI_Id      callId[TAPP_MAX_CALL_NUMS];
    ISI_Id      textId;
    ISI_Id      serviceId[TAPP_MAX_SERVICE_NUMS];
    ISI_Id      isiCCallId[TAPP_MAX_CALL_NUMS];
    ISI_Id      isiCServiceId[TAPP_MAX_SERVICE_NUMS];
    ISI_Id      ussdId;
    ISI_Id      ussdServiceId;
    TAPP_ReportResult reportResult; 
} TAPP_GlobalObj;

/* TAPP functions prototype */

TAPP_Return TAPP_readTestCase(
    TAPP_GlobalObj *global_ptr,
    char           *filename_tr);

TAPP_Return TAPP_init(
    TAPP_GlobalObj **pGlobal_ptr,
    char            *filename_ptr);

TAPP_Return TAPP_shutdown(
    TAPP_GlobalObj *global_ptr);

TAPP_Return TAPP_getNextAction(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action   **pAction_ptr);

TAPP_Return TAPP_getInputEvent(
    TAPP_GlobalObj  *global_ptr,
    TAPP_Action     *action_ptr,
    TAPP_Event      *evt_ptr,
    vint             timeout);

TAPP_Return _TAPP_addAction(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr,
    vint           *idx);

TAPP_Return TAPP_processRepeatStart(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

TAPP_Return TAPP_processRepeatEnd(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr);

void _TAPP_printEvent(
    TAPP_Event *event_ptr);

void _TAPP_printAction(
    TAPP_Action    *action_ptr);

void TAPP_printBytes(
    unsigned char *s_ptr,
    int len);
#endif
