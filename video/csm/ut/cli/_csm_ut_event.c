/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7544 $ $Date: 2008-09-05 19:45:05 -0400 (Fri, 05 Sep 2008) $
 *
 */

/*
 * Test control functions
 * Assumes the VTSP has already been initialized
 */
#include <osal.h>
#include <csm_event.h>
#include "_csm_ut.h"
#include "_csm_ut_call.h"
#include "_csm_ut_event.h"

typedef struct {
    CSM_OutputReason  num;
    char               *name;
} CSMUT_EventCodes;

vint _CSMUT_constructAtEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize);
    
vint _CSMUT_constructAtResult(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize);

vint _CSMUT_constructSupsrvQueryResult(
    const CSM_OutputSupSrv *supSrv_ptr,
    char                   *out_ptr,
    vint                    maxOutSize);

vint _CSMUT_constructSupsrvCallEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize);

CSMUT_EventCodes  CSMUT_responseReasonTable[] = {
    { CSM_OUTPUT_REASON_OK,                   "CSM_OUTPUT_REASON_OK" },
    { CSM_OUTPUT_REASON_ERROR,                "CSM_OUTPUT_REASON_ERROR" },
    { CSM_OUTPUT_REASON_CALL_LIST,            "CSM_OUTPUT_REASON_CALL_LIST" },
    { CSM_OUTPUT_REASON_DISCONNECT_EVENT,     "CSM_OUTPUT_REASON_DISCONNECT_EVENT" },
    { CSM_OUTPUT_REASON_INCOMING_EVENT,       "CSM_OUTPUT_REASON_INCOMING_EVENT" },
    { CSM_OUTPUT_REASON_WAITING_EVENT,        "CSM_OUTPUT_REASON_WAITING_EVENT" },
    { CSM_OUTPUT_REASON_SMS_SENT,             "CSM_OUTPUT_REASON_SMS_SENT" },
    { CSM_OUTPUT_REASON_SMS_RECEIVED,         "CSM_OUTPUT_REASON_SMS_RECEIVED" },
    { CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED,  "CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED" },
    { CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT,    "CSM_OUTPUT_REASON_USSD_NOTIFY_EVENT" },
    { CSM_OUTPUT_REASON_USSD_REQUEST_EVENT,   "CSM_OUTPUT_REASON_USSD_REQUEST_EVENT" },
    { CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT,"CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT" },
    { CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT,  "CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT"},
    { CSM_OUTPUT_REASON_CALL_EXTRA_INFO,      "CSM_OUTPUT_REASON_CALL_EXTRA_INFO"},
    { CSM_OUTPUT_REASON_CALL_MODIFY_EVENT,    "CSM_OUTPUT_REASON_CALL_MODIFY_EVENT"},
    { CSM_OUTPUT_REASON_CALL_DTMF_DETECT,     "CSM_OUTPUT_REASON_CALL_DTMF_DETECT:"},
    { CSM_OUTPUT_REASON_CALL_MONITOR,         "CSM_OUTPUT_REASON_CALL_MONITOR" },
};

enum {
    CSM_EVENT_TYPE_MAX = CSM_EVENT_TYPE_USSD+1
};
static CSMUT_EventFilter _CSMUT_eventFilters[CSM_EVENT_TYPE_MAX] = {NULL};

/* 
 * Look up event msg constant in table
 */

/*
 * Look up event msg code in table
 */
unsigned char *CSMUT_eventReason(
    uvint arg)
{
    uvint code;

    for (code = 0; code < sizeof(CSMUT_responseReasonTable); code++) {
        if (CSMUT_responseReasonTable[code].num == arg) {
            return ((unsigned char *)CSMUT_responseReasonTable[code].name);
        }
    }
    return (NULL);
}

/*
 * Look up event msg code in table
 */
UT_Return _CSMUT_processEvent(
    CSM_OutputEvent  *response_ptr)
{
    CSM_OutputCall *call;
    CSM_OutputSms *sms;
    CSM_OutputUssd *ussd;

    unsigned char  *reason_ptr = NULL;
    char            scratch[CSM_UT_AT_COMMAND_SIZE + 1];

    switch (response_ptr->type) {
        case CSM_EVENT_TYPE_CALL:
            call = &response_ptr->evt.call;
            reason_ptr = CSMUT_eventReason(call->reason);
            OSAL_logMsg("CSMUT_processEvent : %s\n", reason_ptr);
            if (UT_PASS == _CSMUT_constructAtEvent(response_ptr,
                    scratch, CSM_UT_AT_COMMAND_SIZE)) {
                OSAL_logMsg("%s:%d Wrote event %s to RILD %s\n",
                        __FUNCTION__, __LINE__, reason_ptr, scratch);
                break;
            }
            /* Otherwise let's see if it's a result to a previous command. */
            if (UT_PASS == _CSMUT_constructAtResult(response_ptr,
                    scratch, CSM_UT_AT_COMMAND_SIZE)) {
                OSAL_logMsg("%s:%d Wrote cmd result %s to RILD %s\n",
                        __FUNCTION__, __LINE__, reason_ptr, scratch);
                break;
            }
            break;
        case CSM_EVENT_TYPE_SMS:
            sms = &response_ptr->evt.sms;
            /* XXX: How to get message from PDU ?? */
            if (CSM_OUTPUT_REASON_SMS_RECEIVED == sms->reason) {
                OSAL_logMsg("\n================ Receiving a SMS message "
                        "================\n");
                OSAL_logMsg(" From    : %s\n", sms->address);
                OSAL_logMsg(" Message : %s\n", sms->u.msg.body);
                OSAL_logMsg(" Message Id: %d\n", sms->mr);
                if (-1 != sms->u.msg.numOfMessage) {
                    OSAL_logMsg(" Number of messages(VMN): %d\n",
                            sms->u.msg.numOfMessage);
                }
                OSAL_logMsg("============================="
                        "============================\n");
            }
            else if (CSM_OUTPUT_REASON_SMS_SENT == sms->reason) {
                OSAL_logMsg("\n================ SMS message sent"
                        "================\n");
                OSAL_logMsg(" Message Id: %d\n", sms->mr);
                OSAL_logMsg("============================="
                        "============================\n");
            }
            else if (CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED == sms->reason) {
                OSAL_logMsg("\n================ Receiving a SMS Report"
                        "================\n");
                OSAL_logMsg(" Message Id: %d\n", sms->mr);
                OSAL_logMsg(" Report type: %d\n", sms->u.reportType);
                OSAL_logMsg("============================="
                        "============================\n");
            }
            else if (CSM_OUTPUT_REASON_ERROR == sms->reason) {
                OSAL_logMsg("\n================ Receiving a SMS Error"
                        "================\n");
                OSAL_logMsg(" Error Code: %d\n", sms->u.errorCode);
                OSAL_logMsg("============================="
                        "============================\n");
            }
            
            break;
        case CSM_EVENT_TYPE_USSD:
            ussd = &response_ptr->evt.ussd;
            reason_ptr = CSMUT_eventReason(ussd->reason);
            if (UT_PASS == _CSMUT_constructAtEvent(response_ptr,
                    scratch, CSM_UT_AT_COMMAND_SIZE)) {
                OSAL_logMsg("%s:%d Wrote event %s to RILD %s\n",
                        __FUNCTION__, __LINE__, reason_ptr, scratch);
                break;
            }
            /* Otherwise let's see if it's a result to a previous command. */
            if (UT_PASS == _CSMUT_constructAtResult(response_ptr,
                    scratch, CSM_UT_AT_COMMAND_SIZE)) {
                OSAL_logMsg("%s:%d Wrote cmd result %s to RILD %s\n",
                        __FUNCTION__, __LINE__, reason_ptr, scratch);
                break;
            }
            break;
        case CSM_EVENT_TYPE_SUPSRV:
            /* dynamic filter processing first */
            if ( (NULL != _CSMUT_eventFilters[CSM_EVENT_TYPE_SUPSRV]) &&
                    OSAL_TRUE == (_CSMUT_eventFilters[CSM_EVENT_TYPE_SUPSRV])(response_ptr) ) {
                OSAL_logMsg("CSM_EVENT_TYPE_SUPSRV processed by filter\n");
                break;
            }
            /* fixed processing logic here */
            if (UT_PASS == _CSMUT_constructAtResult(response_ptr,
                    scratch, CSM_UT_AT_COMMAND_SIZE)) {
                OSAL_logMsg("%s:%d Wrote cmd result %s to RILD %s\n",
                        __FUNCTION__, __LINE__, reason_ptr, scratch);
                break;
            }
            break;
        case CSM_EVENT_TYPE_SERVICE:
            if (CSM_OUTPUT_REASON_SERVICE_STATE == response_ptr->evt.service.reason) {
                OSAL_logMsg("%s %d :%s state = %d\n", __FILE__, __LINE__, 
                    "CSM_OUTPUT_REASON_SERVICE_STATE", response_ptr->evt.service.state);
            }
            break;
        default:
            OSAL_logMsg("%s:%d ERROR bad response type\n", __func__, __LINE__);
            break;
    }

    return (UT_PASS);
}

/**
 * ======== _CSMUT_constructAtEvent ========
 * This routine will construct an AT unsolicited events that came from CSM
 * back up through the AT interface.
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details event to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated unsolicited AT event
 * will be written. Ultimately the AT response written here is then sent up
 * through the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * UT_PASS The AT event was constructed in out_ptr.
 * UT_FAIL The details of the event indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _CSMUT_constructAtEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize)
{
    const CSM_OutputCall *call_ptr;
    const CSM_OutputSms  *sms_ptr;
    const CSM_OutputUssd  *ussd_ptr;

    if (CSM_EVENT_TYPE_CALL == response_ptr->type) {
        call_ptr = &response_ptr->evt.call;
        switch (call_ptr->reason) {

            case CSM_OUTPUT_REASON_DISCONNECT_EVENT:
                /* Event indicating that a remote party has disconnected. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        CSM_UT_AT_RESPONSE_REMOTE_DISCONNECT,
                        CSM_UT_AT_RESPONSE_CR);
                return (UT_PASS);

            case CSM_OUTPUT_REASON_INCOMING_EVENT:
                OSAL_logMsg("\n=============== Receiving an incoming call "
                            "==============\n");
                OSAL_logMsg(" From    : %s, Call Session Type=%d\n", call_ptr->u.clipReport.number, call_ptr->u.clipReport.callSessionType);
                OSAL_logMsg("============================="
                            "============================\n");
                return (UT_PASS);

            case CSM_OUTPUT_REASON_WAITING_EVENT:
                /*
                 * Event indicating that a new incoming call waiting
                 * is being requested.
                 */
                // EXample...
                //+CCWA: "13122099351",131,,,""
                if (0 != call_ptr->u.clipReport.alpha[0]) {
                    OSAL_snprintf(out_ptr, maxOutSize,
                            "%s \"%s\",%d,,,\"%s\"%s",
                            CSM_UT_AT_RESPONSE_WAITING,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            call_ptr->u.clipReport.alpha,
                            CSM_UT_AT_RESPONSE_CR);
                }
                else {
                    OSAL_snprintf(out_ptr, maxOutSize, "%s \"%s\",%d,,,%s",
                            CSM_UT_AT_RESPONSE_WAITING,
                            call_ptr->u.clipReport.number,
                            call_ptr->u.clipReport.type,
                            CSM_UT_AT_RESPONSE_CR);
                }
                return (UT_PASS);
            case CSM_OUTPUT_REASON_INITIALIZING_EVENT:
                /* Send ready to csm .*/
                _CSM_UT_callResourceReady();
                return (UT_PASS);
            case CSM_OUTPUT_REASON_CALL_MODIFY_EVENT:
                OSAL_logMsg("\n=============== Got an call modification"
                            "==============\n");
                OSAL_logMsg(" From    : %s, Call Session Type=%d\n", call_ptr->u.clipReport.number, call_ptr->u.clipReport.callSessionType);
                OSAL_logMsg("============================="
                            "============================\n");
                return (UT_PASS);
            default:
                /* passed on next construct chain that handling supsrv call events */
                return _CSMUT_constructSupsrvCallEvent(response_ptr, out_ptr, maxOutSize);
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SMS == response_ptr->type) {
        sms_ptr = &response_ptr->evt.sms;
        switch (sms_ptr->reason) {

            case CSM_OUTPUT_REASON_SMS_RECEIVED:
                OSAL_snprintf(out_ptr, maxOutSize, "%s ,%d%s%s%s",
                        CSM_UT_AT_RESPONSE_SMS_RECV,
                        OSAL_strlen(sms_ptr->u.msg.body) / 2,
                        CSM_UT_AT_RESPONSE_CR_LN, sms_ptr->u.msg.body,
                        CSM_UT_AT_RESPONSE_CR_LN);
                return (UT_PASS);

            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_USSD == response_ptr->type) {
        ussd_ptr = &response_ptr->evt.ussd;
        switch (ussd_ptr->reason) {
            case CSM_OUTPUT_REASON_USSD_REQUEST_EVENT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s1,\"%s\",72%s%s%s",
                    CSM_UT_AT_RESPONSE_USSD_SENT, ussd_ptr->message,
                    CSM_UT_AT_RESPONSE_CR_LN, CSM_UT_AT_RESPONSE_STR_OK, 
                    CSM_UT_AT_RESPONSE_CR_LN);
                return (UT_PASS);
            case CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s2,\"%s\",72%s%s%s",
                    CSM_UT_AT_RESPONSE_USSD_SENT, ussd_ptr->message,
                    CSM_UT_AT_RESPONSE_CR_LN, CSM_UT_AT_RESPONSE_STR_OK, 
                    CSM_UT_AT_RESPONSE_CR_LN);
                return (UT_PASS);
            default:
                break;
        }
    }
    return (UT_FAIL);
}
/**
 * ======== _CSMUT_cmdMngrConstructAtResult ========
 * This routine will construct an AT solicited response that came from CSM
 * back up through the AT interface.
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details response to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated AT response will be
 * written. Ultimately the AT response written here is then sent up through
 * the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * UT_PASS The AT response was constructed in out_ptr.
 * UT_FAIL The details of the response indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _CSMUT_constructAtResult(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize)
{
    const CSM_OutputCall *call_ptr;
    const CSM_OutputSms  *sms_ptr;
    const CSM_CallReport   *rpt_ptr;
    const CSM_CallSummary  *sum_ptr;
    const CSM_OutputUssd  *ussd_ptr;
    const CSM_OutputSupSrv *supSrv_ptr;
    vint              x;
    vint              errorCode;

    if (CSM_EVENT_TYPE_CALL == response_ptr->type) {
        call_ptr = &response_ptr->evt.call;
        switch (call_ptr->reason) {
            case CSM_OUTPUT_REASON_OK:        
                /* '0' or 'OK' response to a command. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        CSM_UT_AT_RESPONSE_OK, CSM_UT_AT_RESPONSE_CR);
                return (UT_PASS);

            case CSM_OUTPUT_REASON_ERROR:     
                /* +CME ERROR response to a command. 
                 * The error code will be in the payload. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                        CSM_UT_AT_RESPONSE_ERROR, call_ptr->u.errorCode, 
                        CSM_UT_AT_RESPONSE_CR);
                return (UT_PASS);

            case CSM_OUTPUT_REASON_CALL_LIST:
            case CSM_OUTPUT_REASON_CALL_MONITOR:
                /* +CLCC response to a AT+CLCC command. */
                rpt_ptr = &call_ptr->u.report;
                /* Loop and set all the states of the calls. */

                for (x = 0 ; x < rpt_ptr->numCalls ; x++) {
                    sum_ptr = &rpt_ptr->calls[x];
                    if (0 != sum_ptr->alpha[0]) {                        
                        OSAL_logMsg("%s %d,%d,%d,%d,%d,\"%s\",%d,\"%s\"\n",
                            CSM_UT_AT_RESPONSE_REPORT,
                            sum_ptr->idx, sum_ptr->direction, sum_ptr->state,
                            sum_ptr->mode, sum_ptr->isMultiParty,
                            sum_ptr->number, sum_ptr->type, sum_ptr->alpha);
                    }
                    else if (0 != sum_ptr->number[0]) {
                        /* Then don't include the alpha */
                        OSAL_logMsg("%s %d,%d,%d,%d,%d,\"%s\",%d\n",
                            CSM_UT_AT_RESPONSE_REPORT,
                            sum_ptr->idx, sum_ptr->direction, sum_ptr->state,
                            sum_ptr->mode, sum_ptr->isMultiParty,
                            sum_ptr->number, sum_ptr->type);
                    }
                    else {
                        /* Then don't include a phone number or alpha */
                        OSAL_logMsg("%s %d,%d,%d,%d,%d\n",
                            CSM_UT_AT_RESPONSE_REPORT,
                            sum_ptr->idx, sum_ptr->direction, sum_ptr->state,
                            sum_ptr->mode, sum_ptr->isMultiParty);
                    }
                    // xxx exit type/cause for CMCCSI
                }
                return (UT_PASS);
            case CSM_OUTPUT_REASON_CALL_DTMF_DETECT:
                OSAL_logMsg("%s:%c\n", CSM_UT_AT_RESPONSE_DTMF_DETECT,
                        call_ptr->u.digit);
                return (UT_PASS);
            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SMS == response_ptr->type) {
        sms_ptr = &response_ptr->evt.sms;
        switch (sms_ptr->reason) {
            case CSM_OUTPUT_REASON_SMS_SENT:
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s%s%s",
                        CSM_UT_AT_RESPONSE_SMS_SENT, sms_ptr->mr,
                        CSM_UT_AT_RESPONSE_CR_LN, CSM_UT_AT_RESPONSE_OK,
                        CSM_UT_AT_RESPONSE_CR);
                return (UT_PASS);
            case CSM_OUTPUT_REASON_ERROR:
                /* +CMS ERROR response to a command.
                 * The error code will be in the payload. */
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                        CSM_UT_AT_RESPONSE_SMS_ERROR, sms_ptr->u.errorCode,
                        CSM_UT_AT_RESPONSE_CR);
                return (UT_PASS);
            default:
                break;
        }
    }
    else if (CSM_EVENT_TYPE_USSD == response_ptr->type) {
            ussd_ptr = &response_ptr->evt.ussd;
            OSAL_logMsg("%s, reason=%d, errorCode=%d\n", 
                __FUNCTION__, ussd_ptr->reason, ussd_ptr->errorCode);
            switch (ussd_ptr->reason) {
                case CSM_OUTPUT_REASON_ERROR:
                    OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                        CSM_UT_AT_RESPONSE_ERROR, ussd_ptr->errorCode,
                        CSM_UT_AT_RESPONSE_CR);
                    return (UT_PASS);
                case CSM_OUTPUT_REASON_OK:
                    OSAL_snprintf(out_ptr, maxOutSize, "%s%s", 
                        CSM_UT_AT_RESPONSE_STR_OK, CSM_UT_AT_RESPONSE_CR_LN);
                    return (UT_PASS);
                default:
                    return (UT_FAIL);
            }
    }
    else if (CSM_EVENT_TYPE_SUPSRV == response_ptr->type) {
        supSrv_ptr = &response_ptr->evt.supSrv;
        switch (supSrv_ptr->reason) {
            case CSM_OUTPUT_REASON_OK:
                OSAL_snprintf(out_ptr, maxOutSize, "%s%s",
                        CSM_UT_AT_RESPONSE_OK,
                        CSM_UT_AT_RESPONSE_CR_LN);
                return (UT_PASS);
            case CSM_OUTPUT_REASON_ERROR:
                switch (supSrv_ptr->errorCode) {
                    case CSM_SUPSRV_ERROR_NO_NET_SERVICE:
                        errorCode = 30; /* No network */
                        break;
                    case CSM_SUPSRV_ERROR_NET_TIMEOUT:
                        errorCode = 31; /* Network timeout*/
                        break;
                    case CSM_SUPSRV_ERROR_HTTP:
                        OSAL_logMsg("CSM_SUPSRV_ERROR_HTTP: %s\n",
                                supSrv_ptr->reasonDesc);
                    case CSM_SUPSRV_ERROR_NONE:
                    case CSM_SUPSRV_ERROR_UNKNOWN:
                    case CSM_SUPSRV_ERROR_XCAP_CONFLICT:
                    case CSM_SUPSRV_ERROR_XML_PARSING:
                    default:
                        errorCode = 100; /* Unknown */
                        break;
                }
                OSAL_snprintf(out_ptr, maxOutSize, "%s %d%s",
                        CSM_UT_AT_RESPONSE_ERROR, errorCode,
                        CSM_UT_AT_RESPONSE_CR);
                return (UT_PASS);
            case CSM_OUTPUT_REASON_SUPSRV_QUERY_RESULT:
                return (_CSMUT_constructSupsrvQueryResult(
                        supSrv_ptr, out_ptr, maxOutSize));
            default:
                break;
        }
    }
    return (UT_FAIL);
}

/*
 * ======== CSMUT_eventTaskBlock() ========
 *
 * This is a task function
 * Read event queues independently in 'ANY' blocking method
 */
OSAL_TaskReturn CSMUT_eventTaskBlock(
    OSAL_TaskArg taskArg)
{
    CSM_OutputEvent  event;
    OSAL_MsgQId      *qId_ptr;
    OSAL_Boolean     timeout;

    qId_ptr = (OSAL_MsgQId *)taskArg;
    while (1) {
        if (OSAL_msgQRecv(*qId_ptr, (char *)&event, sizeof(CSM_OutputEvent),
                OSAL_WAIT_FOREVER, &timeout) > 0) {
            _CSMUT_processEvent(&event);
        }
    }

    OSAL_logMsg("evTask: exit.\n");
    return(0);
}


/**
 * ======== _CSMUT_constructSupsrvQueryResult ========
 * This routine will construct an AT queried response that came from CSM
 * back up through the AT interface.
 *
 * supSrv_ptr A pointer to a CSM_OutputSupSrv object.  This object contains the
 * details response to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated AT response will be
 * written. Ultimately the AT response written here is then sent up through
 * the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 */
vint _CSMUT_constructSupsrvQueryResult(
    const CSM_OutputSupSrv *supSrv_ptr,
    char                   *out_ptr,
    vint                    maxOutSize)
{
    vint              type;

    switch (supSrv_ptr->cmdType) {
        case CSM_SUPSRV_CMD_GET_OIP:
             OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    CSM_UT_AT_RESPONSE_OIP,
                    supSrv_ptr->queryEnb.genResStatus,
                    supSrv_ptr->prov.genProv,
                    CSM_UT_AT_RESPONSE_CR_LN,
                    CSM_UT_AT_RESPONSE_OK,
                    CSM_UT_AT_RESPONSE_CR_LN);
             break;
        case CSM_SUPSRV_CMD_GET_OIR:
             OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    CSM_UT_AT_RESPONSE_OIR,
                    supSrv_ptr->queryEnb.oirResStatus,
                    supSrv_ptr->prov.oirProv,
                    CSM_UT_AT_RESPONSE_CR_LN,
                    CSM_UT_AT_RESPONSE_OK,
                    CSM_UT_AT_RESPONSE_CR_LN);
             break;
        case CSM_SUPSRV_CMD_GET_TIP:
            OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    CSM_UT_AT_RESPONSE_TIP,
                    supSrv_ptr->queryEnb.genResStatus,
                    supSrv_ptr->prov.genProv,
                    CSM_UT_AT_RESPONSE_CR_LN,
                    CSM_UT_AT_RESPONSE_OK,
                    CSM_UT_AT_RESPONSE_CR_LN);
            break;
        case CSM_SUPSRV_CMD_GET_TIR:
            OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%d%s%s%s",
                    CSM_UT_AT_RESPONSE_TIR,
                    supSrv_ptr->queryEnb.genResStatus,
                    supSrv_ptr->prov.genProv,
                    CSM_UT_AT_RESPONSE_CR_LN,
                    CSM_UT_AT_RESPONSE_OK,
                    CSM_UT_AT_RESPONSE_CR_LN);
            break;
        case CSM_SUPSRV_CMD_GET_CW:
            OSAL_snprintf(out_ptr, maxOutSize,
                    "%s %d,%s%s%s%s",
                    CSM_UT_AT_RESPONSE_WAITING,
                    supSrv_ptr->queryEnb.genResStatus,
                    CSM_UT_AT_RESPONSE_CLASS,
                    CSM_UT_AT_RESPONSE_CR_LN,
                    CSM_UT_AT_RESPONSE_OK,
                    CSM_UT_AT_RESPONSE_CR_LN);
            break;
        case CSM_SUPSRV_CMD_GET_CD:
            if (0 == supSrv_ptr->queryEnb.genResStatus) {
                OSAL_snprintf(out_ptr, maxOutSize,
                        "%s %d,%s%s%s%s",
                        CSM_UT_AT_RESPONSE_CF,
                        0,
                        CSM_UT_AT_RESPONSE_CLASS,
                        CSM_UT_AT_RESPONSE_CR_LN,
                        CSM_UT_AT_RESPONSE_OK,
                        CSM_UT_AT_RESPONSE_CR_LN);
            }
            else {
                if (NULL != supSrv_ptr->u.ruleParams.cfwNumber) {
                    /* default 145 when dialling string
                     * includes international
                     * access code character "+", otherwise 129
                     * (refer GSM 04.08)
                     */
                    if (OSAL_strchr(supSrv_ptr->u.ruleParams.cfwNumber, '+')){
                        type = CSM_UT_AT_RESPONSE_CF_TYPE_145;
                    }
                    else {
                        type = CSM_UT_AT_RESPONSE_CF_TYPE_129;
                    }
                    if (CSM_EVENT_SUPSRV_CF_MODE_NOREPLY ==
                            supSrv_ptr->mode.cfMode){
                        OSAL_snprintf(out_ptr, maxOutSize,
                            "%s %d,%s,%s,%d,,,%d%s%s%s",
                            CSM_UT_AT_RESPONSE_CF,
                            1,
                            CSM_UT_AT_RESPONSE_CLASS,
                            supSrv_ptr->u.ruleParams.cfwNumber,
                            type,
                            supSrv_ptr->u.ruleParams.noReplyTimer,
                            CSM_UT_AT_RESPONSE_CR_LN,
                            CSM_UT_AT_RESPONSE_OK,
                            CSM_UT_AT_RESPONSE_CR_LN);
                    }
                    OSAL_snprintf(out_ptr, maxOutSize,
                            "%s %d,%s,%s,%d%s%s%s",
                            CSM_UT_AT_RESPONSE_CF,
                            1,
                            CSM_UT_AT_RESPONSE_CLASS,
                            supSrv_ptr->u.ruleParams.cfwNumber,
                            type,
                            CSM_UT_AT_RESPONSE_CR_LN,
                            CSM_UT_AT_RESPONSE_OK,
                            CSM_UT_AT_RESPONSE_CR_LN);
                }
                else {
                    OSAL_snprintf(out_ptr, maxOutSize,
                            "%s %d,,,%s%s%s",
                            CSM_UT_AT_RESPONSE_CF,
                            1,
                            CSM_UT_AT_RESPONSE_CR_LN,
                            CSM_UT_AT_RESPONSE_OK,
                            CSM_UT_AT_RESPONSE_CR_LN);
                }
            }
            break;
        case CSM_SUPSRV_CMD_GET_CBOG:
        case CSM_SUPSRV_CMD_GET_CBOIC:
        case CSM_SUPSRV_CMD_GET_CBIC:
        case CSM_SUPSRV_CMD_GET_CBICR:
            OSAL_snprintf(out_ptr, maxOutSize,
                "%s %d%s%s%s",
                CSM_UT_AT_RESPONSE_CB,
                supSrv_ptr->queryEnb.genResStatus,
                CSM_UT_AT_RESPONSE_CR_LN,
                CSM_UT_AT_RESPONSE_OK,
                CSM_UT_AT_RESPONSE_CR_LN);
            break;
        default:
            break;
    }
    OSAL_logMsg("\n%s %d SUPSRV AT output=%s\n", __FILE__,
            __LINE__, out_ptr);
    return (UT_PASS);
}

/*
 * ======== CSMUT_eventRegisterFilter() ========
 *
 * Save the event filter for specific event type
 *
 * Returns:
 *   OSAL_TRUE if ok or OSAL_FALSE if failed.
 */
OSAL_Boolean CSMUT_eventRegisterFilter(
    CSM_EventType       type,
    CSMUT_EventFilter  eventFilter)
{
    if (NULL != _CSMUT_eventFilters[type]) {
        return (OSAL_FALSE);
    } else {
        _CSMUT_eventFilters[type] = eventFilter;
    }
    return (OSAL_TRUE);
}

/*
 * ======== CSMUT_eventUnRegisterFilter() ========
 *
 * Remove the filter for that event type.
 *
 * Returns:
 *   OSAL_TRUE or OSAL_FALSE
 */
OSAL_Boolean CSMUT_eventUnRegisterFilter(
    CSM_EventType   type)
{
    /* xxx thread-safe check */
    _CSMUT_eventFilters[type] = NULL;

    return (OSAL_TRUE);
}

typedef enum {
    CSM_UT_AT_CSSI_CODE_BEING_FORWARDED = 2,
    CSM_UT_AT_CSSI_CODE_BEING_WAITING = 3,
    CSM_UT_AT_CSSU_CODE_FORWARDED_CALL = 0,
    CSM_UT_AT_CSSU_CODE_BEING_HOLD = 2,
    CSM_UT_AT_CSSU_CODE_BEING_UNHOLD = 3,
    CSM_UT_AT_CSSU_CODE_JOIN_CONFCALL = 4,
} CSM_UT_AtSupsrvCallEventCode;

/*
 * ======== _CSMUT_constructSupsrvCallEvent ========
 * This routine will construct an AT unsolicited supsrv events that came from CSM
 * back up through the AT interface CSSI/CSSU reporting.
 *
 * xxx should also support CSSN to turn on/off the reporting
 *
 * response_ptr A pointer to a CSM_Response object.  This object contains the
 * details event to generate.
 *
 * out_ptr A pointer to a buffer where a NULL terminated unsolicited AT event
 * will be written. Ultimately the AT response written here is then sent up
 * through the AT interface.
 *
 * maxOutSize The size of the buffer pointed to by out_ptr.
 *
 * Returns:
 * UT_PASS The AT event was constructed in out_ptr.
 * UT_FAIL The details of the event indicated in
 * response_ptr are unknown. The out_ptr buffer was NOT written to.
 */
vint _CSMUT_constructSupsrvCallEvent(
    const CSM_OutputEvent *response_ptr,
    char               *out_ptr,
    vint                maxOutSize)
{
    const CSM_OutputCall     *call_ptr;
    const CSM_CallSupsrvInfo *supsrvInfo_ptr;
    const CSM_CallHistory    *historyInfo_ptr;
    vint                      bytes;
    vint                      bytesLeft;
    vint                      x;

    call_ptr = &response_ptr->evt.call;
    if ((CSM_EVENT_TYPE_CALL != response_ptr->type) ||
            (CSM_OUTPUT_REASON_CALL_EXTRA_INFO != call_ptr->reason)) {
        return (UT_FAIL);
    }

    supsrvInfo_ptr = &call_ptr->u.supsrvInfo;
    /* Let's save space for the last '\r\n0\r' in the report. */
    bytesLeft = maxOutSize - 4;

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_BEING_FORWARDED) {
        /*
         * Event indicating that the MO call is being forwarded.
         * +CSSI: 2
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            CSM_UT_AT_RESPONSE_SUPSRV_MO_CALL,
            CSM_UT_AT_CSSI_CODE_BEING_FORWARDED,
            CSM_UT_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MO_CALL_IS_WAITING) {
        /*
         * Event indicating that the MO call is being waited.
         * +CSSI: 3
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            CSM_UT_AT_RESPONSE_SUPSRV_MO_CALL,
            CSM_UT_AT_CSSI_CODE_BEING_WAITING,
            CSM_UT_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;
    }

    if (supsrvInfo_ptr->supsrvNotification &
            CSM_SUPSRV_MT_CALL_IS_FORWARDED) {
        /*
         * Event indicating that the MT call is forwarded to me.
         * +CSSU: 0
         */
        bytes = OSAL_snprintf(out_ptr, bytesLeft,
            "%s %d%s",
            CSM_UT_AT_RESPONSE_SUPSRV_MT_CALL,
            CSM_UT_AT_CSSU_CODE_FORWARDED_CALL,
            CSM_UT_AT_RESPONSE_CR);
        out_ptr += bytes;
        bytesLeft -= bytes;

        /* do our proprietary command to show the history info */
        if (0 != supsrvInfo_ptr->numHistories) {
            /*
             * Event listing the history info using new at command
             * +CSSH: hi-idx1, cause1, number1, type1[, alpha1]
             * +CSSH: hi-idx2, cause2, number2, type2[, alpha2]
             * ...
             */

            for (x = 0 ; x < supsrvInfo_ptr->numHistories ; x++) {
                historyInfo_ptr = &supsrvInfo_ptr->callHistories[x];
                if (0 != historyInfo_ptr->alpha[0]) {
                    bytes = OSAL_snprintf(out_ptr, bytesLeft,
                            "%s %s,%d,\"%s\",%d,\"%s\"%s",
                            CSM_UT_AT_RESPONSE_SUPSRV_HISTORY,
                            historyInfo_ptr->hiIndex,
                            historyInfo_ptr->cause,
                            historyInfo_ptr->number,
                            historyInfo_ptr->type,
                            historyInfo_ptr->alpha,
                            CSM_UT_AT_RESPONSE_CR);
                }
                else if (0 != historyInfo_ptr->number[0]) {
                    /* Then don't include the alpha */
                    bytes = OSAL_snprintf(out_ptr, bytesLeft,
                            "%s %s,%d,\"%s\",%d%s",
                            CSM_UT_AT_RESPONSE_SUPSRV_HISTORY,
                            historyInfo_ptr->hiIndex,
                            historyInfo_ptr->cause,
                            historyInfo_ptr->number,
                            historyInfo_ptr->type,
                            CSM_UT_AT_RESPONSE_CR);
                }
                else {
                    /*
                     * Then don't include a phone number or alpha,
                     * illegal case
                     */
                }

                out_ptr += bytes;
                bytesLeft -= bytes;
            }
        }
    }

    return (UT_PASS);
}
