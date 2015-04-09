/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 *
 */

#include <osal.h>
#include "http.h"

/* from IDH */
#include <http_api.h>
#include "http_api.h"
#include "IN_Message.h"
#include "cfl_mem.h"
#include <mn_api.h>


#ifdef HTTP_DBG_LOG
#define HTTP_dbgPrintf    OSAL_logMsg
#else
#define HTTP_dbgPrintf(x, ...)
#endif

/* Queue and IPC definitions */
#define _HTTP_MSGQ_LEN                (8)
#define _HTTP_TASK_NAME               "HTTP_DAPS"
#define _HTTP_TASK_STACK_BYTES        (1000)
#define _HTTP_EVENT_QUEUE_NAME        "http.event.q"

#define _HTTP_HEADER_NAME_LEN       32
#define _HTTP_RECV_TIMEOUT          (10*1000)
#define _HTTP_MAX_BODY_SZ           (4096)

/* http header fields */
static const char _HTTP_acceptString[] = "*/*";
static const char _HTTP_acceptCharset[] = "utf-8";
static const char _HTTP_acceptLanaguageString[] = "en-US";
static const char _HTTP_userAgentString[] = "D2Tech HTTP_DAPS";
static const char _HTTP_emptyString[] = " ";
static const char _HTTP_contenTypeString[] = "application/octet-stream";

typedef enum {
    _HTTP_EVENT_UNKNOWN,
    _HTTP_EVENT_INIT_CONF,
    _HTTP_EVENT_REQUEST_ID_IND,
    _HTTP_EVENT_SSL_CERT_UNTRUST_IND,
    _HTTP_EVENT_HEADER_IND,
    _HTTP_EVENT_NEED_AUTH_IND,
    _HTTP_EVENT_DATA_IND,
    _HTTP_EVENT_ERROR_IND,
    _HTTP_EVENT_GET_CNF,
    _HTTP_EVENT_PUT_CNF,
    _HTTP_EVENT_DELETE_CNF,
    _HTTP_EVENT_EXIT,
} _HTTP_EventType;

typedef union {
        HTTP_INIT_CNF_SIG_T         initCnfSig;
        HTTP_REQUEST_ID_IND_SIG_T   requestIdIndSig;
        HTTP_SSL_CERT_UNTRUST_IND_SIG_T sslCertUntrustSig;
        HTTP_HEADER_IND_SIG_T       headerIndSig;
        HTTP_AUTH_IND_SIG_T         authIndSig;
        HTTP_DATA_IND_SIG_T         dataIndSig;
        HTTP_ERROR_IND_SIG_T        errorIndSig;
        HTTP_GET_CNF_SIG_T          getCnfSig;
        HTTP_PUT_CNF_SIG_T          putCnfSig;
        HTTP_DELETE_CNF_SIG_T       deleteCnfSig;
        HTTP_CLOSE_CNF_SIG_T        closeCnfSig;
        HTTP_CANCEL_CNF_SIG_T       cancelCnfSig;
} _HTTP_Signal;

typedef union {
    HTTP_AUTH_PARAM_T   authParam;
    HTTP_INIT_PARAM_T   initParam;
    HTTP_GET_PARAM_T    getParam;
    HTTP_DELETE_PARAM_T delParam;
    HTTP_PUT_PARAM_T    putParam;
    HTTP_POST_PARAM_T   postParam;
    HTTP_HEAD_PARAM_T   headParam;
} _HTTP_Param;

typedef struct {
    _HTTP_Signal    *sig_ptr;
    uint8            padding[12];
} _HTTP_EventMsg;

/*
 * Follows HTTP private data. Dont read/write.
 */
typedef struct {
    HTTP_APP_MODULE_ID_T moduleId;
    HTTP_APP_PARAM_T     appParam;
    HTTP_APP_INSTANCE_T  appInstance;
    HTTP_CONTEXT_ID_T    contextId;
    HTTP_REQUEST_ID_T    requestId;
    HTTP_ERROR_E         lastResult;
    HTTP_OTHER_HEADER_T  otherHeaders[HTTP_MAX_CUSTOM_HEADERS];
    xSignalHeaderRec     errorSig;
    vint                 lastResponseCode;
    vint                 expectingSignalCode;
    _HTTP_Param     httpParam;
    HTTP_Obj       *httpObj_ptr;
    char            customHeaderNameList[HTTP_MAX_CUSTOM_HEADERS][_HTTP_HEADER_NAME_LEN];
    OSAL_TaskId     tId;
    OSAL_MsgQId     eventQ;
    OSAL_SemId      taskLock;
    OSAL_Boolean    isBusy;
} _HTTP_Internal;

/* local function prototype */
void _HTTP_httpSigCallbackFunc(
    xSignalHeader sig_ptr, 
    BLOCK_ID sender);

vint _HTTP_prepare(
    _HTTP_Internal *obj_ptr);

OSAL_TaskReturn _HTTP_processEvent(
    _HTTP_Internal  *obj_ptr,
    _HTTP_Signal     *msg_ptr);

OSAL_TaskReturn _HTTP_task(
    OSAL_TaskArg arg_ptr);

/* local static global */
static _HTTP_Internal *_HTTP_internal_ptr;
/* 
 * ======== HTTP_alloc() ========
 * Public routine for allocating the module resource
 *
 * Returns: 
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
 vint HTTP_allocate()
{
    _HTTP_internal_ptr = OSAL_memCalloc(1, sizeof(_HTTP_Internal), 0);
    if (NULL == _HTTP_internal_ptr) {
        OSAL_logMsg("HTTP_allocate failed memCalloc\n");
        return (OSAL_FAIL);
    }
    
    /* Create event queue */
    _HTTP_internal_ptr->eventQ = OSAL_msgQCreate(_HTTP_EVENT_QUEUE_NAME,
            OSAL_MODULE_HTTP, OSAL_MODULE_HTTP, OSAL_DATA_STRUCT_HTTP_Event,
            _HTTP_MSGQ_LEN,
            sizeof(_HTTP_EventMsg),
            0);
    if (NULL == _HTTP_internal_ptr->eventQ) {
        OSAL_logMsg("HTTP_allocate failed msgQCreate\n");
        OSAL_memFree(_HTTP_internal_ptr, 0);
        _HTTP_internal_ptr = NULL;
        return (OSAL_FAIL);
    }
    
    _HTTP_internal_ptr->taskLock = OSAL_semBinaryCreate(OSAL_SEMB_UNAVAILABLE);
    if (NULL == _HTTP_internal_ptr->taskLock) {
        OSAL_logMsg("HTTP_allocate failed semBinaryCreate\n");
        OSAL_msgQDelete(_HTTP_internal_ptr->eventQ);
        OSAL_memFree(_HTTP_internal_ptr, 0);
        _HTTP_internal_ptr = NULL;
        return (OSAL_FAIL); 
    }
    
    _HTTP_internal_ptr->moduleId = OSAL_MODULE_HTTP;
    _HTTP_internal_ptr->appParam = 0;
    
    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_start() ========
 *
 * Public routine for starting the module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
 vint HTTP_start()
{
    /* Create task */
    _HTTP_internal_ptr->tId = OSAL_taskCreate(_HTTP_TASK_NAME,
            OSAL_TASK_PRIO_NRT,
            _HTTP_TASK_STACK_BYTES,
            _HTTP_task,
            (void *)_HTTP_internal_ptr);
    if (NULL == _HTTP_internal_ptr->tId) {
        OSAL_logMsg("HTTP_allocate failed taskCreate\n");
        return (OSAL_FAIL);
    }
    
    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_applyAuthInfo() ========
 * 
 * This function setup the user/pass to the underlining http layer
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_applyAuthInfo(
    HTTP_Obj *httpObj_ptr,
    char     *akaAuts_ptr)
{
    HTTP_AUTH_PARAM_T *authParam_ptr;

    switch (_HTTP_internal_ptr->expectingSignalCode) {
        case HTTP_SIG_GET_CNF:
            authParam_ptr = &_HTTP_internal_ptr->httpParam.getParam.auth;
            break;
        case HTTP_SIG_PUT_CNF:
            authParam_ptr = &_HTTP_internal_ptr->httpParam.delParam.auth;
            break;
        case HTTP_SIG_DELETE_CNF:
            authParam_ptr = &_HTTP_internal_ptr->httpParam.delParam.auth;
            break;
    }

    if (HTTP_AUTH_DIGEST == httpObj_ptr->authProtocol) {
        authParam_ptr->user_name_ptr = httpObj_ptr->username_ptr;
        authParam_ptr->password_ptr = httpObj_ptr->password_ptr;
        authParam_ptr->entity_ptr = NULL;
        authParam_ptr->entity_len = 0;
    }
}

/* 
 * ======== _HTTP_headerFilter() ========
 * 
 * This function is a filter for parsing a header data.
 * Calls HTTP application callback if necessary.
 *
 * Returns: 
 *   NONE.
 */
static void _HTTP_headerFilter(
    HTTP_RSP_HEADER_INFO_T  *headerInfo_ptr,
    char                    *header_ptr)
{
    HTTP_Obj *httpObj_ptr = _HTTP_internal_ptr->httpObj_ptr;
    char *pos_ptr;

    OSAL_snprintf(httpObj_ptr->lastHttpStatusLine, "HTTP/1.1 %d", 
            headerInfo_ptr->response_code);
    OSAL_snprintf(httpObj_ptr->lastContentType, "Content-Type: %s", 
            headerInfo_ptr->content_type_ptr);

    /* parse the authentication line */
    pos_ptr = OSAL_strscan(header_ptr, "WWW-Authenticate");
    if (NULL != pos_ptr) {
        if (httpObj_ptr->authHandler != NULL) {
            OSAL_Boolean updateUserPass;
            char *eolPos_ptr;
            eolPos_ptr = OSAL_strchr(pos_ptr, '\n');
            updateUserPass = (httpObj_ptr->authHandler)(httpObj_ptr, 
                                                    pos_ptr,
                                                    eolPos_ptr-pos_ptr);
            if (updateUserPass) {
                HTTP_applyAuthInfo(httpObj_ptr, NULL);
            }
        }
    }

    // TBD: other customized header parsing needs?
}

/* 
 * ======== _HTTP_dataBodyFilter() ========
 * 
 * This function is a callback for reading body data.
 * Calls HTTP application callback.
 *
 * Returns: 
 *   NONE.
 */
static size_t _HTTP_dataBodyFilter(
     char   *buf_ptr,
     size_t  size)
{
    HTTP_Obj *httpObj_ptr = _HTTP_internal_ptr->httpObj_ptr;
    void *newbuf_ptr;

    if ( (httpObj_ptr->bufs.bodyBufSz - httpObj_ptr->bufs.bodyBufIndex) <=
            (int)size ) {
        /* out of memory! */
        OSAL_logMsg("http: not enough memory in bufs\n");
        httpObj_ptr->bufs.failed = 1;
        return 0;
    }
    OSAL_memCpy(&(httpObj_ptr->bufs.body_ptr[httpObj_ptr->bufs.bodyBufIndex]),
                buf_ptr, 
                size);
    httpObj_ptr->bufs.bodyBufIndex += size;
    httpObj_ptr->bufs.body_ptr[httpObj_ptr->bufs.bodyBufIndex] = 0;

    HTTP_dbgPrintf("_HTTP_dataBodyFilter http body size: %d\n", size);
    return size;
}

 /*
 * ======== _HTTP_applyCustomHeaders() ========
 *
 * Internal routine for coverting the custom headers into daps list
 * also handled the special case of body type header for daps,
 * which extract it from customer header and put into toBodyType_ptr
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _HTTP_applyCustomHeaders(
    HTTP_Obj                *httpObj_ptr,
    HTTP_EXTERN_HEADER_T    *toExterHeader_ptr,
    char                    **toBodyTypePtr_ptr)
{
    /* copy httpObj_ptr->customHeaders to _HTTP_internal_ptr->otherHeaders */
    toExterHeader_ptr->header_num = httpObj_ptr->customHeadersCount;
    if (0 != httpObj_ptr->customHeadersCount) {
        int len, i = 0;
        char *colonPos_ptr;
        char *customerHeader_ptr;
        while (i<httpObj_ptr->customHeadersCount) {
            customerHeader_ptr = httpObj_ptr->customHeaders[i];
            colonPos_ptr = OSAL_strchr(customerHeader_ptr, ':');
            len = colonPos_ptr-customerHeader_ptr;
            OSAL_strncpy(_HTTP_internal_ptr->customHeaderNameList[i],
                    customerHeader_ptr,
                    len+1);
            _HTTP_internal_ptr->customHeaderNameList[i][len] = '\0';
            _HTTP_internal_ptr->otherHeaders[i].header_name_ptr = 
                    _HTTP_internal_ptr->customHeaderNameList[i];

            colonPos_ptr += 1;
            while (' ' == *colonPos_ptr) {
                colonPos_ptr += 1;
            }
            _HTTP_internal_ptr->otherHeaders[i].header_value_ptr = colonPos_ptr;
            
            /* special filter for "Content-Type" header */
            if (0 == OSAL_strcmp("Content-Type", 
                    _HTTP_internal_ptr->otherHeaders[i].header_name_ptr)) {
                if (NULL != toBodyTypePtr_ptr) {
                    *toBodyTypePtr_ptr = 
                            _HTTP_internal_ptr->otherHeaders[i].header_value_ptr;
                    HTTP_dbgPrintf("custome content type:%s\n", *toBodyTypePtr_ptr);
                }
                /* don't increase i here to skip this content-type in custom header */
            }
            else {
                i++;
            }
        }
        HTTP_dbgPrintf("custom headers: n,h1n,h1v: %d,%s,%s\n",
                httpObj_ptr->customHeadersCount,
                _HTTP_internal_ptr->otherHeaders[0].header_name_ptr,
                _HTTP_internal_ptr->otherHeaders[0].header_value_ptr);
        /* pass list of custom made headers */
        toExterHeader_ptr->other_header_ptr = _HTTP_internal_ptr->otherHeaders;
    } 
    else {
        toExterHeader_ptr->other_header_ptr = NULL;
    }
}
 /*
 * ======== _HTTP_prepare() ========
 *
 * Internal routine for doing the HTTP startup actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _HTTP_prepare(
    _HTTP_Internal *obj_ptr)
{
    /* at task thread time and all modules are allocated */

    /*register http signal callback function*/
    HTTP_RegSigCallbackFunc(_HTTP_httpSigCallbackFunc);
    HTTP_dbgPrintf("_HTTP_prepare registered HTTP signal cb\n");

    return (OSAL_SUCCESS);
}

OSAL_TaskReturn _HTTP_processEvent(
    _HTTP_Internal  *obj_ptr,
    _HTTP_Signal     *msg_ptr)
{
    xSignalHeader sig_ptr = (xSignalHeader)msg_ptr;

    if (sig_ptr->SignalCode >= HTTP_SIGNAL_BEG) {
        HTTP_dbgPrintf("_HTTP_processEvent HTTP event:%d\n", 
                sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
    }
    else {
        HTTP_dbgPrintf("_HTTP_processEvent Other Signal:%d, sig_ptr:0x%x\n", 
            sig_ptr->SignalCode, sig_ptr);
    }

    switch(sig_ptr->SignalCode)
    {
        case HTTP_SIG_INIT_CNF:
            if (HTTP_SIG_INIT_CNF == obj_ptr->expectingSignalCode) {
                obj_ptr->lastResult = msg_ptr->initCnfSig.result;
                obj_ptr->contextId = msg_ptr->initCnfSig.context_id;
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_INIT_CNF res:%d, cxtId:0x%x",
                        obj_ptr->lastResult, obj_ptr->contextId );
                OSAL_semGive(obj_ptr->taskLock);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_REQUEST_ID_IND:
            if (0 != obj_ptr->expectingSignalCode) {
                obj_ptr->requestId = msg_ptr->requestIdIndSig.request_id;
                /* assert obj_ptr->contextId == msg_ptr->requestIdIndSig.context_id; */
                /* continute wait for request CNF, OSAL_semGive(obj_ptr->taskLock);*/
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_REQUEST_ID_IND reqId:%d",
                        obj_ptr->requestId);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_SSL_CERT_UNTRUST_IND:
            HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_SSL_CERT_UNTRUST_IND reqId:%d",
                        obj_ptr->requestId);
            break;
        case HTTP_SIG_HEADER_IND:
            if (0 != obj_ptr->expectingSignalCode) {
                /* assert obj_ptr->requestId = msg_ptr->headerIndSig.request_id; */
                /* assert obj_ptr->contextId == msg_ptr->headerIndSig.context_id; */
                /* handle header */
                _HTTP_headerFilter(&msg_ptr->headerIndSig.rsp_header_info,
                        msg_ptr->headerIndSig.header_ptr);
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_HEADER_IND resp:%d",
                        msg_ptr->headerIndSig.rsp_header_info.response_code);
                /* continute wait for request CNF, OSAL_semGive(obj_ptr->taskLock);*/
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_NEED_AUTH_IND:
            if (0 != obj_ptr->expectingSignalCode) {
                /* assert obj_ptr->requestId = msg_ptr->authIndSig.request_id; */
                /* assert obj_ptr->contextId == msg_ptr->authIndSig.context_id; */
                /* handle auth */
                HTTP_AUTH_PARAM_T *authParam_ptr;
                
                HTTP_dbgPrintf("HTTP HTTP_SIG_NEED_AUTH_IND: authType=%d, algType=%d", 
                        msg_ptr->authIndSig.auth_type, msg_ptr->authIndSig.alg_type);
                /* continute wait for request CNF, OSAL_semGive(obj_ptr->taskLock);*/
                /* auth param is prepared in HTTP_applyAuthInfo by _HTTP_headerFilter */
                HTTP_AuthResponse(_HTTP_internal_ptr->contextId,
                        authParam_ptr,
                        _HTTP_internal_ptr->appInstance,
                        _HTTP_internal_ptr->requestId);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_DATA_IND:
            if (0 != obj_ptr->expectingSignalCode) {
                /* assert obj_ptr->requestId = msg_ptr->dataIndSig.request_id; */
                /* assert obj_ptr->contextId == msg_ptr->dataIndSig.context_id; */
                /* handle header */
                HTTP_dbgPrintf("HTTP HTTP_SIG_DATA_IND: dataPtr=%x, len=%d", 
                        msg_ptr->dataIndSig.data_ptr, msg_ptr->dataIndSig.data_len);
                _HTTP_dataBodyFilter(msg_ptr->dataIndSig.data_ptr,
                        msg_ptr->dataIndSig.data_len);
                /* continute wait for request CNF, OSAL_semGive(obj_ptr->taskLock);*/
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_ERROR_IND:
            OSAL_logMsg("HTTP error ind signal: result=%d", 
                        msg_ptr->errorIndSig.result);
            obj_ptr->lastResult = msg_ptr->errorIndSig.result;
            if (0 != obj_ptr->expectingSignalCode) {
                /* wake up pending action with error in lastResult, 
                 * may need to clear ctx since the daps seems kill cxt after error
                 * obj_ptr->contextId = 0;
                 */
                OSAL_semGive(obj_ptr->taskLock);
            }
            break;
        case HTTP_SIG_GET_CNF:
            if (HTTP_SIG_GET_CNF == obj_ptr->expectingSignalCode) {
                /* assert obj_ptr->requestId = msg_ptr->getCnfSig.request_id; */
                /* assert obj_ptr->contextId == msg_ptr->getCnfSig.context_id; */
                obj_ptr->lastResult = msg_ptr->getCnfSig.result;
                obj_ptr->lastResponseCode = 
                        msg_ptr->getCnfSig.rsp_header_info.response_code;
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_GET_CNF res:%d, resp:%d",
                        msg_ptr->getCnfSig.result,
                        msg_ptr->getCnfSig.rsp_header_info.response_code);
                OSAL_semGive(obj_ptr->taskLock);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_PUT_CNF:
            if (HTTP_SIG_PUT_CNF == obj_ptr->expectingSignalCode) {
                /* assert obj_ptr->requestId = msg_ptr->putCnfSig.request_id; */
                /* assert obj_ptr->contextId == msg_ptr->putCnfSig.context_id; */
                obj_ptr->lastResult = msg_ptr->putCnfSig.result;
                obj_ptr->lastResponseCode = 
                        msg_ptr->putCnfSig.rsp_header_info.response_code;
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_PUT_CNF res:%d, resp:%d",
                        msg_ptr->putCnfSig.result,
                        msg_ptr->putCnfSig.rsp_header_info.response_code);
                OSAL_semGive(obj_ptr->taskLock);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_DELETE_CNF:
            if (HTTP_SIG_DELETE_CNF == obj_ptr->expectingSignalCode) {
                /* assert obj_ptr->requestId = msg_ptr->deleteCnfSig.request_id; */
                /* assert obj_ptr->contextId == msg_ptr->deleteCnfSig.context_id; */
                obj_ptr->lastResult = msg_ptr->deleteCnfSig.result;
                obj_ptr->lastResponseCode = 
                        msg_ptr->deleteCnfSig.rsp_header_info.response_code;
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_DELETE_CNF res:%d, resp:%d",
                        msg_ptr->deleteCnfSig.result,
                        msg_ptr->deleteCnfSig.rsp_header_info.response_code);
                OSAL_semGive(obj_ptr->taskLock);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_CLOSE_CNF:
            if (HTTP_SIG_CLOSE_CNF == obj_ptr->expectingSignalCode) {
                obj_ptr->lastResult = msg_ptr->closeCnfSig.result;
                obj_ptr->contextId = msg_ptr->closeCnfSig.context_id;
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_CLOSE_CNF res:%d, cxtId:0x%x",
                        msg_ptr->closeCnfSig.result,
                        msg_ptr->closeCnfSig.context_id);
                OSAL_semGive(obj_ptr->taskLock);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
            break;
        case HTTP_SIG_CANCEL_CNF:
            if (HTTP_SIG_CANCEL_CNF == obj_ptr->expectingSignalCode) {
                obj_ptr->requestId = msg_ptr->cancelCnfSig.request_id;
                obj_ptr->contextId = msg_ptr->cancelCnfSig.context_id;
                HTTP_dbgPrintf("_HTTP_processEvent HTTP_SIG_CANCEL_CNF reqId:%d, cxtId:0x%x",
                        msg_ptr->cancelCnfSig.request_id,
                        msg_ptr->cancelCnfSig.context_id);
                OSAL_semGive(obj_ptr->taskLock);
            } else {
                OSAL_logMsg("unexpected HTTP event: expect=%d, got=%d",
                        obj_ptr->expectingSignalCode - HTTP_SIGNAL_BEG,
                        sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
            }
        default:
            OSAL_logMsg("unexpected HTTP signal");
            break;
    }
    /* free the signal resource, == SCI_FREE */
    if (sig_ptr->SignalCode) {
        CFL_FREE(msg_ptr);
    }
    return (0);
}

/*
 * ======== _HTTP_task() ========
 *
 * This is one and only task of HTTP. It receives sequentially, event from
 * daps signal callback, processes these event and trigger other http actions in the 
 * http task context.
 *
 * Returns:
 *  Never returns.
 *
 */
OSAL_TaskReturn _HTTP_task(
    OSAL_TaskArg arg_ptr)
{
    _HTTP_Internal *obj_ptr = (_HTTP_Internal *)arg_ptr;
    _HTTP_EventMsg  msg;
    vint            len;

    _HTTP_prepare(obj_ptr);

    while (1) {
        /* Receive a valid event. */
        len = OSAL_msgQRecv(obj_ptr->eventQ,
                (char *)&msg,
                sizeof(_HTTP_EventMsg),
                OSAL_WAIT_FOREVER,
                NULL);

        /* validate */
        if (len != sizeof(_HTTP_EventMsg)) {
            HTTP_dbgPrintf("http got wrong length msg len:%d, sig_ptr:0x%x\n",
                    len, msg.sig_ptr);
            OSAL_taskDelay(100);
            continue;
        }
        
        /* Check if exit command. */
        if ((HTTP_SIGNAL_BEG == msg.sig_ptr->errorIndSig.SignalCode) ||
                (0 == msg.sig_ptr->errorIndSig.SignalCode)) {
            break;
        }

        _HTTP_processEvent(obj_ptr, msg.sig_ptr);
    }

    /*
     * Task exit, better than killing it.
     */
    HTTP_dbgPrintf("%s:%d http exiting...\n", __FILE__, __LINE__);

    return (0);
}


/*
 * ======== HTTP_destroy() ========
 * Destroy the module
 *
 * Return Values:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *
 */
vint HTTP_destroy(void)
{
    _HTTP_EventMsg msg;

    if (NULL != _HTTP_internal_ptr->tId) {
        HTTP_dbgPrintf("HTTP_destroy tId:0x%x\n", _HTTP_internal_ptr->tId);
        msg.sig_ptr = (_HTTP_Signal *)&_HTTP_internal_ptr->errorSig;

        /* gracefully or forcefully kill the task */
        msg.sig_ptr->errorIndSig.SignalCode = HTTP_SIGNAL_BEG; /* dummy to exit */
        msg.sig_ptr->errorIndSig.SignalSize = 0;

        if (OSAL_SUCCESS != OSAL_msgQSend(_HTTP_internal_ptr->eventQ,
                (char *)&msg,
                sizeof(_HTTP_EventMsg), /* random data is fine here */
                OSAL_WAIT_FOREVER,
                NULL)) {
            HTTP_dbgPrintf("%s:%d ERROR msgQ send FAILED, http event=%d\n",
                    __FUNCTION__, __LINE__, HTTP_SIGNAL_BEG);
            OSAL_taskDelete(_HTTP_internal_ptr->tId);
        }
        _HTTP_internal_ptr->tId = 0;
    }

    /* free resources */
    OSAL_semDelete(_HTTP_internal_ptr->taskLock);
    OSAL_msgQDelete(_HTTP_internal_ptr->eventQ);
    OSAL_memFree(_HTTP_internal_ptr, 0);
    _HTTP_internal_ptr = NULL;
    return (OSAL_SUCCESS);
}

/* 
 * ======== HTTP_setup() ========
 * 
 * This function is called to setup http connection parameters.
 * Must be called before any http request.
*
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
 OSAL_Status HTTP_setup(
    HTTP_Obj *httpObj_ptr)
{
    HTTP_INIT_PARAM_T   *initParam_ptr;
    MN_GPRS_PDP_ADDR_T   ipAddr;
    OSAL_NetAddress     *address_ptr;
    int32                ret;

    httpObj_ptr->priv_ptr = _HTTP_internal_ptr;
    _HTTP_internal_ptr->httpObj_ptr = httpObj_ptr;

    /* 
     * app provided buffer or we allocated for them
     * but app is always responsible to OSAL_free() it after use
     * because we don't know when it is done
     */
    if (httpObj_ptr->bufs.body_ptr == NULL) {
        httpObj_ptr->bufs.body_ptr = OSAL_memAlloc(_HTTP_MAX_BODY_SZ, 
                OSAL_MEM_ARG_DYNAMIC_ALLOC);
        httpObj_ptr->bufs.bodyBufSz = _HTTP_MAX_BODY_SZ;
        httpObj_ptr->bufs.bodyBufIndex = 0;
    }

    initParam_ptr = &_HTTP_internal_ptr->httpParam.initParam;
    
#ifdef HTTP_DBG_LOG
    {
        char hostAddrString[48];

        /* Convert to printable string */
        OSAL_netAddressToString(hostAddrString, &httpObj_ptr->infcAddress);
        HTTP_dbgPrintf("HTTP_setup: local ip: %s\n", hostAddrString);
    }
#endif

    initParam_ptr->is_cookie_enable = FALSE;
    initParam_ptr->is_cache_enable = FALSE;
    initParam_ptr->cache_path[0] = 0;
    initParam_ptr->cookie_file_name[0] = 0;
    initParam_ptr->proxy.is_use_proxy = FALSE;
    initParam_ptr->time_zone = 0.0; /* XXX */

    address_ptr = &httpObj_ptr->infcAddress;
    /* query netid to real one if it's not loopback */

    if (OSAL_netIsAddrLoopback(address_ptr)) {
        initParam_ptr->net_id = sci_getLoopBackNetid();
    }
    else if (!OSAL_netIsAddrZero(address_ptr)) {
        if (OSAL_netIsAddrIpv6(address_ptr)) {
            /* ipv6 */
            ipAddr.length = sizeof(address_ptr->ipv6);
            OSAL_memCpy(ipAddr.value_arr, address_ptr->ipv6,
                    ipAddr.length);
        }
        else {
            /* ipv4 */
            ipAddr.length = sizeof(address_ptr->ipv4);
            OSAL_memCpy(ipAddr.value_arr, &address_ptr->ipv4,
                    ipAddr.length);
        }
        /* Look up netid from ip address. */
        if ((ret = MN_getNetIdByIpAddr(MN_DUAL_SYS_1, ipAddr)) <= 0) {
            OSAL_logMsg("Failed to get net id. ret:%d\n", ret);
            return (OSAL_FAIL);
        }
        initParam_ptr->net_id = ret;
        HTTP_dbgPrintf("HTTP_setup: to use net id: %d\n", initParam_ptr->net_id);
    }
    else {
        /* all zero ip addr case */
        return (OSAL_FAIL);
    }

    _HTTP_internal_ptr->expectingSignalCode = HTTP_SIG_INIT_CNF;
    HTTP_InitRequest(_HTTP_internal_ptr->moduleId, _HTTP_internal_ptr->appParam,
            initParam_ptr);

    /* blocking wait */
    OSAL_semAcquire(_HTTP_internal_ptr->taskLock, OSAL_WAIT_FOREVER);
    _HTTP_internal_ptr->expectingSignalCode = 0;
    if (HTTP_SUCCESS == _HTTP_internal_ptr->lastResult) {
        HTTP_dbgPrintf("HTTP_setup:got init cnf sig ok\n");
        return (OSAL_SUCCESS);
    }
    else {
        OSAL_logMsg("Failed http init req. err:%d\n", _HTTP_internal_ptr->lastResult);
        return (OSAL_FAIL);
    }
}

/* 
 * ======== HTTP_delete() ========
 * 
 * Sends HTTP DELETE request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_delete(
    HTTP_Obj *httpObj_ptr,
    char     *url_ptr)
{

    HTTP_DELETE_PARAM_T *delParam_ptr;
    HTTP_ERROR_E result;

    delParam_ptr = &_HTTP_internal_ptr->httpParam.delParam;
    OSAL_memSet(delParam_ptr, 0, sizeof(HTTP_DELETE_PARAM_T));
    
    HTTP_dbgPrintf("HTTP_delete url: %s\n", url_ptr);
    delParam_ptr->connection = FALSE;
    delParam_ptr->need_net_prog_ind = FALSE;

    delParam_ptr->accept.accept_ptr = _HTTP_acceptString;
    delParam_ptr->accept_charset.accept_charset_ptr = _HTTP_acceptCharset;
    delParam_ptr->accept_language.accept_language_ptr = _HTTP_acceptLanaguageString;
    delParam_ptr->user_agent.user_agent_ptr = _HTTP_userAgentString;

    /* copy httpObj_ptr->customHeaders to _HTTP_internal_ptr->otherHeaders */
    _HTTP_applyCustomHeaders(httpObj_ptr, &delParam_ptr->extern_header, NULL);

    delParam_ptr->uri.uri_ptr = url_ptr;
    
    delParam_ptr->ua_profile.ua_profile_ptr = NULL;
    delParam_ptr->referer.referer_ptr = NULL;
    delParam_ptr->accept_encoding = HTTP_ENCODING_TYPE_NULL;
    delParam_ptr->get_data.style = HTTP_DATA_TRANS_STYLE_BUFFER;
    delParam_ptr->get_data.is_header_use_file = FALSE;
    delParam_ptr->get_data.content_file_name[0] = NULL;
    delParam_ptr->get_data.header_file_name[0] = NULL;
    
    delParam_ptr->is_use_relative_url = FALSE;
    delParam_ptr->recv_timeout = _HTTP_RECV_TIMEOUT;

    if (HTTP_AUTH_DIGEST == httpObj_ptr->authProtocol) {
        delParam_ptr->auth.user_name_ptr = httpObj_ptr->username_ptr;
        delParam_ptr->auth.password_ptr = httpObj_ptr->password_ptr;
        delParam_ptr->auth.entity_ptr = NULL;
        delParam_ptr->auth.entity_len = 0;
    }

    _HTTP_internal_ptr->expectingSignalCode = HTTP_SIG_DELETE_CNF;
    result = HTTP_DeleteRequest(
            _HTTP_internal_ptr->contextId,
            delParam_ptr,
            _HTTP_internal_ptr->appInstance);

    if (HTTP_SUCCESS != result) {
        HTTP_dbgPrintf("http delRequest failed: %d\n", result);
        return (OSAL_FALSE);
    }

    /* blocking wait */
    OSAL_semAcquire(_HTTP_internal_ptr->taskLock, OSAL_WAIT_FOREVER);
    _HTTP_internal_ptr->expectingSignalCode = 0;


    if (HTTP_SUCCESS == _HTTP_internal_ptr->lastResult) {
        return (OSAL_SUCCESS);
    }
    else {
        OSAL_logMsg("Failed http del req. err:%d\n", _HTTP_internal_ptr->lastResult);
        return (OSAL_FAIL);
    }
}

/* 
 * ======== HTTP_get() ========
 * 
 * Sends HTTP GET request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_get(
    HTTP_Obj *httpObj_ptr,
    char     *url_ptr)
{
    HTTP_GET_PARAM_T *getParam_ptr;
    HTTP_ERROR_E result;

    getParam_ptr = &_HTTP_internal_ptr->httpParam.getParam;
    OSAL_memSet(getParam_ptr, 0, sizeof(HTTP_GET_PARAM_T));
    
    HTTP_dbgPrintf("HTTP_get url: %s\n", url_ptr);
    getParam_ptr->connection = FALSE;
    getParam_ptr->need_net_prog_ind = FALSE;

    getParam_ptr->accept.accept_ptr = _HTTP_acceptString;
    getParam_ptr->accept_charset.accept_charset_ptr = _HTTP_acceptCharset;
    getParam_ptr->accept_language.accept_language_ptr = _HTTP_acceptLanaguageString;
    getParam_ptr->user_agent.user_agent_ptr = _HTTP_userAgentString;

    /* copy httpObj_ptr->customHeaders to _HTTP_internal_ptr->otherHeaders */
    _HTTP_applyCustomHeaders(httpObj_ptr, &getParam_ptr->extern_header, NULL);

    getParam_ptr->uri.uri_ptr = url_ptr;
    
    getParam_ptr->ua_profile.ua_profile_ptr = _HTTP_emptyString;
    getParam_ptr->referer.referer_ptr = _HTTP_emptyString;
    getParam_ptr->accept_encoding = HTTP_ENCODING_TYPE_NULL;
    getParam_ptr->get_data.style = HTTP_DATA_TRANS_STYLE_BUFFER;
    getParam_ptr->get_data.is_header_use_file = FALSE;
    getParam_ptr->get_data.content_file_name[0] = NULL;
    getParam_ptr->get_data.header_file_name[0] = NULL;
    
    getParam_ptr->is_use_relative_url = FALSE;
    getParam_ptr->recv_timeout = _HTTP_RECV_TIMEOUT;

    if (HTTP_AUTH_DIGEST == httpObj_ptr->authProtocol) {
        getParam_ptr->auth.user_name_ptr = httpObj_ptr->username_ptr;
        getParam_ptr->auth.password_ptr = httpObj_ptr->password_ptr;
        getParam_ptr->auth.entity_ptr = NULL;
        getParam_ptr->auth.entity_len = 0;
    }

    _HTTP_internal_ptr->expectingSignalCode = HTTP_SIG_GET_CNF;
    result = HTTP_GetRequest(
            _HTTP_internal_ptr->contextId,
            getParam_ptr,
            _HTTP_internal_ptr->appInstance);

    if (HTTP_SUCCESS != result) {
        HTTP_dbgPrintf("http getRequest failed: %d\n", result);
        return (OSAL_FALSE);
    }

    /* blocking wait */
    OSAL_semAcquire(_HTTP_internal_ptr->taskLock, OSAL_WAIT_FOREVER);
    _HTTP_internal_ptr->expectingSignalCode = 0;


    if (HTTP_SUCCESS == _HTTP_internal_ptr->lastResult) {
        HTTP_dbgPrintf("HTTP_get:got cnf sig ok\n");
        return (OSAL_SUCCESS);
    }
    else {
        OSAL_logMsg("Failed http get req. err:%d\n", _HTTP_internal_ptr->lastResult);
        return (OSAL_FAIL);
    }
}


/* 
 * ======== HTTP_put() ========
 * 
 * Sends HTTP PUT request.
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_put(
    HTTP_Obj *httpObj_ptr,
    void     *buf_ptr,
    int       bufSz,
    char     *url_ptr)
{
    HTTP_PUT_PARAM_T *putParam_ptr;
    HTTP_ERROR_E result;

    putParam_ptr = &_HTTP_internal_ptr->httpParam.putParam;
    OSAL_memSet(putParam_ptr, 0, sizeof(HTTP_PUT_PARAM_T));
    
    HTTP_dbgPrintf("HTTP_put url:%s\n", url_ptr);
    putParam_ptr->connection = FALSE;
    putParam_ptr->need_net_prog_ind = FALSE;

    putParam_ptr->accept.accept_ptr = _HTTP_acceptString;
    putParam_ptr->accept_charset.accept_charset_ptr = _HTTP_acceptCharset;
    putParam_ptr->accept_language.accept_language_ptr = _HTTP_acceptLanaguageString;
    putParam_ptr->user_agent.user_agent_ptr = _HTTP_userAgentString;

    /* copy httpObj_ptr->customHeaders to _HTTP_internal_ptr->otherHeaders */
    putParam_ptr->extern_header.header_num = httpObj_ptr->customHeadersCount;
    /* copy httpObj_ptr->customHeaders to _HTTP_internal_ptr->otherHeaders */
    _HTTP_applyCustomHeaders(httpObj_ptr, 
        &putParam_ptr->extern_header,
        &putParam_ptr->put_body.body_type_ptr);

    putParam_ptr->uri.uri_ptr = url_ptr;
    
    putParam_ptr->ua_profile.ua_profile_ptr = NULL;
    putParam_ptr->referer.referer_ptr = NULL;
    putParam_ptr->accept_encoding = HTTP_ENCODING_TYPE_NULL;
    putParam_ptr->get_data.style = HTTP_DATA_TRANS_STYLE_BUFFER;
    putParam_ptr->get_data.is_header_use_file = FALSE;
    putParam_ptr->get_data.content_file_name[0] = NULL;
    putParam_ptr->get_data.header_file_name[0] = NULL;
    
    putParam_ptr->is_use_relative_url = FALSE;
    putParam_ptr->recv_timeout = _HTTP_RECV_TIMEOUT;

    if (HTTP_AUTH_DIGEST == httpObj_ptr->authProtocol) {
        putParam_ptr->auth.user_name_ptr = httpObj_ptr->username_ptr;
        putParam_ptr->auth.password_ptr = httpObj_ptr->password_ptr;
        putParam_ptr->auth.entity_ptr = NULL;
        putParam_ptr->auth.entity_len = 0;
    }

    putParam_ptr->put_body.is_use_file = FALSE;
    if (NULL == putParam_ptr->put_body.body_type_ptr) {
        putParam_ptr->put_body.body_type_ptr = _HTTP_contenTypeString;
    }
    putParam_ptr->put_body.u.post_buffer.buffer_ptr = buf_ptr;
    putParam_ptr->put_body.u.post_buffer.buffer_len = bufSz;
    putParam_ptr->put_body.u.post_buffer.is_copied_by_http = TRUE;

    _HTTP_internal_ptr->expectingSignalCode = HTTP_SIG_PUT_CNF;
    result = HTTP_PutRequest(
            _HTTP_internal_ptr->contextId,
            putParam_ptr,
            _HTTP_internal_ptr->appInstance);

    if (HTTP_SUCCESS != result) {
        HTTP_dbgPrintf("http putRequest failed: %d\n", result);
        return (OSAL_FALSE);
    }

    /* blocking wait */
    OSAL_semAcquire(_HTTP_internal_ptr->taskLock, OSAL_WAIT_FOREVER);
    _HTTP_internal_ptr->expectingSignalCode = 0;


    if (HTTP_SUCCESS == _HTTP_internal_ptr->lastResult) {
        HTTP_dbgPrintf("HTTP_put:got cnf sig ok\n");
        return (OSAL_SUCCESS);
    }
    else {
        OSAL_logMsg("Failed http put req. err:%d\n", _HTTP_internal_ptr->lastResult);
        return (OSAL_FAIL);
    }
}


/* 
 * ======== HTTP_cleanup() ========
 * 
 * Deallocates all resources allocated by HTTP_setup().
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status HTTP_cleanup(
    HTTP_Obj *httpObj_ptr)
{
 
    if (0 != _HTTP_internal_ptr->contextId) {
        HTTP_dbgPrintf("HTTP_cleanup contextId:%d\n", _HTTP_internal_ptr->contextId);
        _HTTP_internal_ptr->expectingSignalCode = HTTP_SIG_CLOSE_CNF;
        HTTP_CloseRequest(_HTTP_internal_ptr->contextId);
        /* blocking wait */
        OSAL_semAcquire(_HTTP_internal_ptr->taskLock, OSAL_WAIT_FOREVER);
        _HTTP_internal_ptr->expectingSignalCode = 0;
        _HTTP_internal_ptr->contextId = 0;
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== HTTP_copyHeaderValue() ========
 *
 * Strip off leading and trailing whitespace from the value in the
 * given HTTP header line and copy to the supplied buffer.
 * Set buffer to an empty string if the header value
 * consists entirely of whitespace.
 * assumption: destBuf_ptr is big enough to store the value string
 * code base is from libcurl
 *
 * Returns: 
 *   OSAL_SUCCESS or OSAL_FAIL.
 */
OSAL_Status  HTTP_copyHeaderValue(
    const char  *hl_ptr,
    char        *destBuf_ptr)
{
    const char *start;
    const char *end;
    size_t len;

    /* Find the end of the header name */
    while (*hl_ptr && (*hl_ptr != ':')) {
        ++hl_ptr;
    }
    if(*hl_ptr) {
        /* Skip over colon */
        ++hl_ptr;
    }

    /* Find the first non-space letter */
    start = hl_ptr;
    while (*start && isspace(*start)) {
        start++;
    }
    
    /* data is in the host encoding so
     use '\r' and '\n' instead of 0x0d and 0x0a */
    end = OSAL_strchr(start, '\r');
    if(!end) {
        end = OSAL_strchr(start, '\n');
    }
    if(!end) {
        end = OSAL_strchr(start, '\0');
    }
    if(!end) {
        return OSAL_FAIL;
    }

    /* skip all trailing space letters */
    while((end > start) && isspace(*end)) {
        end--;
    }
    
    /* get length of the type */
    len = end-start+1;

    OSAL_memCpy(destBuf_ptr, start, len);
    destBuf_ptr[len] = 0; /* zero terminate */

    return OSAL_SUCCESS;
}

void HTTP_freshBuffer(
    HTTP_Obj *httpObj_ptr)
{
    httpObj_ptr->bufs.bodyBufIndex = 0;
    if (NULL != httpObj_ptr->bufs.body_ptr) {
        httpObj_ptr->bufs.body_ptr[0] = 0;
    }
    httpObj_ptr->lastHttpStatusLine[0] = '\0';
    httpObj_ptr->lastContentType[0] = '\0';
}


void _HTTP_httpSigCallbackFunc(xSignalHeader sig_ptr, BLOCK_ID sender)
{
    _HTTP_EventMsg msg;

    msg.sig_ptr = (_HTTP_Signal *)sig_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(_HTTP_internal_ptr->eventQ,
            (char *)&msg,
            sizeof(_HTTP_EventMsg),
            OSAL_WAIT_FOREVER,
            NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED, http event=%d\n",
                __FUNCTION__, __LINE__, sig_ptr->SignalCode - HTTP_SIGNAL_BEG);
    }
}
