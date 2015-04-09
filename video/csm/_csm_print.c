/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29644 $ $Date: 2014-11-03 19:15:40 +0800 (Mon, 03 Nov 2014) $
 */

#include <osal.h>
#include <isi.h>
#include "_csm_print.h"
#include "_csm.h"

static char _CSM_atCmdDial[]           = "AT DIAL CMD";
static char _CSM_atCmdReport[]         = "AT REPORT CMD";
static char _CSM_atCmdAnswer[]         = "AT ANSWER CMD";
static char _CSM_atCmdEnd[]            = "AT END CMD";
static char _CSM_atCmdSwap[]           = "AT SWAP CMD";
static char _CSM_atCmdEndAllActive[]   = "AT END ALL ACTIVE CMD";
static char _CSM_atCmdEndAllHeldOrWaiting[] = "AT END ALL HELD OR WAITING";
static char _CSM_atCmdHoldAllExceptX[] = "AT HOLD ALL EXCEPT X";
static char _CSM_atCmdDigit[]          = "AT GENERATE DIGIT";
static char _CSM_atCmdReleaseAtX[]     = "AT RELEASE AT X";
static char _CSM_atCmdConference[]     = "AT CONFERENCE";
static char _CSM_atCmdSrvccStart[]     = "AT SRVCC Start";
static char _CSM_atCmdSrvccSuccess[]   = "AT SRVCC Success";
static char _CSM_atCmdSrvccFailed[]    = "AT SRVCC Failed";

static char _CSM_evtIncoming[]         = "ISI EVENT - INCOMING";
static char _CSM_evtTrying[]           = "ISI EVENT - TRYING";
static char _CSM_evtAcknowledged[]     = "ISI EVENT - ACKNOWLEDGED";
static char _CSM_evtFailed[]           = "ISI EVENT - FAILED";
static char _CSM_evtDisconnect[]       = "ISI EVENT - DISCONNECT";
static char _CSM_evtHold[]             = "ISI EVENT - HOLD";
static char _CSM_evtResume[]           = "ISI EVENT - RESUME";
static char _CSM_evtAccepted[]         = "ISI EVENT - ACCEPTED";
static char _CSM_evtRejected[]         = "ISI EVENT - REJECTED";
static char _CSM_evtVccHandoff[]       = "ISI EVENT - VCC HANDOFF";
static char _CSM_evtDigitDone[]        = "ISI EVENT - Digit Sent";
static char _CSM_evtXferDone[]         = "ISI EVENT - Conference Xfer Complete";
static char _CSM_evtParticipantInfo[]  = "ISI EVENT - Participant Info";
static char _CSM_evtModify[]           = "ISI EVENT - MODIFY";
static char _CSM_evtForward[]          = "ISI EVENT - Being Forwarded";
static char _CSM_evtSrvccStart[]     = "ISI EVENT - SRVCC Start";
static char _CSM_evtSrvccSuccess[]     = "ISI EVENT - SRVCC Success";
static char _CSM_evtSrvccFailed[]      = "ISI EVENT - SRVCC Failed";
static char _CSM_evtOtherDisconnect[]  = "Other Call disconnect";

static char _CSM_evtResourceReady[]    = "Resource EVENT - Ready";
static char _CSM_evtResourceFailed[]   = "Resource EVENT - Failed";

static char *_CSM_callReasons[] = {
    _CSM_atCmdDial,
    _CSM_atCmdReport,
    _CSM_atCmdAnswer,
    _CSM_atCmdEnd,
    _CSM_atCmdSwap,
    _CSM_atCmdEndAllActive,
    _CSM_atCmdEndAllHeldOrWaiting,
    _CSM_atCmdHoldAllExceptX,
    _CSM_atCmdDigit,
    _CSM_atCmdReleaseAtX,
    _CSM_atCmdConference,
    _CSM_atCmdSrvccStart,
    _CSM_atCmdSrvccSuccess,
    _CSM_atCmdSrvccFailed,

    _CSM_evtIncoming,
    _CSM_evtTrying,
    _CSM_evtAcknowledged,
    _CSM_evtFailed,
    _CSM_evtDisconnect,
    _CSM_evtHold,
    _CSM_evtResume,
    _CSM_evtAccepted,
    _CSM_evtRejected,
    _CSM_evtVccHandoff,
    _CSM_evtDigitDone,
    _CSM_evtXferDone,
    _CSM_evtParticipantInfo,
    _CSM_evtModify,
    _CSM_evtForward,
    _CSM_evtSrvccStart,
    _CSM_evtSrvccSuccess,
    _CSM_evtSrvccFailed,
    _CSM_evtOtherDisconnect,

    _CSM_evtResourceReady,
    _CSM_evtResourceFailed
};

/* Look up table for call ISI event type categories */
static const char *_CSM_isiPrintIdType[ISI_ID_TYPE_INVALID + 1] = {
    "ISI_ID_TYPE_NONE",
    "ISI_ID_TYPE_CALL",
    "ISI_ID_TYPE_TEL_EVENT",
    "ISI_ID_TYPE_SERVICE",
    "ISI_ID_TYPE_PRESENCE",
    "ISI_ID_TYPE_MESSAGE",
    "ISI_ID_TYPE_GROUPCHAT",
    "ISI_ID_TYPE_FILE",
    "ISI_ID_TYPE_USSD",
    "ISI_ID_TYPE_INVALID",
};

/* Look up table for call ISI events */
static const char *_CSM_isiPrintEvent[ISI_EVENT_INVALID + 1] = {
    "ISI_EVENT_NONE",
    "ISI_EVENT_NET_UNAVAILABLE",
    "ISI_EVENT_CONTACT_RECEIVED",
    "ISI_EVENT_CONTACT_SEND_OK",
    "ISI_EVENT_CONTACT_SEND_FAILED",
    "ISI_EVENT_SUB_TO_PRES_RECEIVED",
    "ISI_EVENT_SUB_TO_PRES_SEND_OK",
    "ISI_EVENT_SUB_TO_PRES_SEND_FAILED",
    "ISI_EVENT_SUBSCRIPTION_RESP_RECEIVED",
    "ISI_EVENT_SUBSCRIPTION_RESP_SEND_OK",
    "ISI_EVENT_SUBSCRIPTION_RESP_SEND_FAILED",
    "ISI_EVENT_PRES_RECEIVED",
    "ISI_EVENT_PRES_SEND_OK",
    "ISI_EVENT_PRES_SEND_FAILED",
    "ISI_EVENT_MESSAGE_RECEIVED",
    "ISI_EVENT_MESSAGE_REPORT_RECEIVED",
    "ISI_EVENT_MESSAGE_SEND_OK",
    "ISI_EVENT_MESSAGE_SEND_FAILED",
    "ISI_EVENT_CREDENTIALS_REJECTED",
    "ISI_EVENT_TEL_EVENT_RECEIVED",
    "ISI_EVENT_TEL_EVENT_SEND_OK",
    "ISI_EVENT_TEL_EVENT_SEND_FAILED",
    "ISI_EVENT_SERVICE_ACTIVE",
    "ISI_EVENT_SERVICE_INACTIVE",
    "ISI_EVENT_SERVICE_INIT_OK",
    "ISI_EVENT_SERVICE_INIT_FAILED",
    "ISI_EVENT_SERVICE_HANDOFF",
    "ISI_EVENT_CALL_FAILED",
    "ISI_EVENT_CALL_XFER_PROGRESS",
    "ISI_EVENT_CALL_XFER_REQUEST",
    "ISI_EVENT_CALL_XFER_FAILED",
    "ISI_EVENT_CALL_XFER_COMPLETED",
    "ISI_EVENT_CALL_MODIFY_FAILED",
    "ISI_EVENT_CALL_MODIFY_COMPLETED",
    "ISI_EVENT_CALL_HOLD",
    "ISI_EVENT_CALL_RESUME",
    "ISI_EVENT_CALL_REJECTED",
    "ISI_EVENT_CALL_ACCEPTED",
    "ISI_EVENT_CALL_ACKNOWLEDGED",
    "ISI_EVENT_CALL_DISCONNECTED",
    "ISI_EVENT_CALL_INCOMING",
    "ISI_EVENT_CALL_TRYING",
    "ISI_EVENT_CALL_HANDOFF",
    "ISI_EVENT_PROTOCOL_READY",
    "ISI_EVENT_PROTOCOL_FAILED",

    "ISI_EVENT_CHAT_INCOMING",
    "ISI_EVENT_CHAT_TRYING",
    "ISI_EVENT_CHAT_ACKNOWLEDGED",
    "ISI_EVENT_CHAT_ACCEPTED",
    "ISI_EVENT_CHAT_DISCONNECTED",
    "ISI_EVENT_CHAT_FAILED",
    "ISI_EVENT_GROUP_CHAT_INCOMING",
    "ISI_EVENT_GROUP_CHAT_PRES_RECEIVED",
    "ISI_EVENT_GROUP_CHAT_NOT_AUTHORIZED",
    "ISI_EVENT_DEFERRED_CHAT_INCOMING",

    "ISI_EVENT_CALL_MODIFY",
    "ISI_EVENT_FILE_SEND_PROGRESS",
    "ISI_EVENT_FILE_SEND_PROGRESS_COMPLETED",
    "ISI_EVENT_FILE_SEND_PROGRESS_FAILED",
    "ISI_EVENT_FILE_RECV_PROGRESS",
    "ISI_EVENT_FILE_RECV_PROGRESS_COMPLETED",
    "ISI_EVENT_FILE_RECV_PROGRESS_FAILED",
    "ISI_EVENT_FRIEND_REQ_RECEIVED",
    "ISI_EVENT_FRIEND_REQ_SEND_OK",
    "ISI_EVENT_FRIEND_REQ_SEND_FAILED",
    "ISI_EVENT_FRIEND_RESP_RECEIVED",
    "ISI_EVENT_FRIEND_RESP_SEND_OK",
    "ISI_EVENT_FRIEND_RESP_SEND_FAILED",
    "ISI_EVENT_CALL_FLASH_HOLD",
    "ISI_EVENT_CALL_FLASH_RESUME",
    "ISI_EVENT_PRES_CAPS_RECEIVED",
    "ISI_EVENT_MESSAGE_COMPOSING_ACTIVE",
    "ISI_EVENT_MESSAGE_COMPOSING_IDLE",
    "ISI_EVENT_FILE_REQUEST",
    "ISI_EVENT_FILE_ACCEPTED",
    "ISI_EVENT_FILE_REJECTED",
    "ISI_EVENT_FILE_PROGRESS",
    "ISI_EVENT_FILE_COMPLETED",
    "ISI_EVENT_FILE_FAILED",
    "ISI_EVENT_FILE_CANCELLED",
    "ISI_EVENT_FILE_ACKNOWLEDGED",
    "ISI_EVENT_FILE_TRYING",
    "ISI_EVENT_AKA_AUTH_REQUIRED",
    "ISI_EVENT_IPSEC_SETUP",
    "ISI_EVENT_IPSEC_RELEASE",
    "ISI_EVENT_USSD_REQUEST",
    "ISI_EVENT_USSD_DISCONNECT",
    "ISI_EVENT_USSD_SEND_OK",
    "ISI_EVENT_USSD_SEND_FAILED",
    "ISI_EVENT_CALL_HANDOFF_START",
    "ISI_EVENT_CALL_HANDOFF_SUCCESS",
    "ISI_EVENT_CALL_HANDOFF_FAILED",
    "ISI_EVENT_CALL_BEING_FORWARDED",
    "ISI_EVENT_RCS_PROVISIONING",
    "ISI_EVENT_CALL_ACCEPT_ACK",
    "ISI_EVENT_CALL_RTP_INACTIVE_TMR_DISABLE",
    "ISI_EVENT_CALL_RTP_INACTIVE_TMR_ENABLE",
    "ISI_EVENT_SERVICE_ACTIVATING",
    "ISI_EVENT_CALL_VIDEO_REQUEST_KEY",
    "ISI_EVENT_CALL_CANCEL_MODIFY",
    "ISI_EVENT_INVALID"
};

#ifdef CSM_DEBUG
/* Look up table for all telephone events (a.k.a. tel events) */
static const char *_CSM_isiPrintTelEvt[ISI_TEL_EVENT_INVALID + 1] = {
    "ISI_TEL_EVENT_DTMF",
    "ISI_TEL_EVENT_DTMF_OOB",
    "ISI_TEL_EVENT_VOICEMAIL",
    "ISI_TEL_EVENT_CALL_FORWARD",
    "ISI_TEL_EVENT_FEATURE",
    "ISI_TEL_EVENCSM_dbgPrintfT_FLASHHOOK",
    "ISI_TEL_EVENT_RESERVED_1",
    "ISI_TEL_EVENT_RESERVED_2",
    "ISI_TEL_EVENT_RESERVED_3",
    "ISI_TEL_EVENT_RESERVED_4",
    "ISI_TEL_EVENT_RESERVED_5",
    "ISI_TEL_EVENT_INVALID"
};
#endif

/* 
 * ======== CSM_getCallReasonString() ========
 *
 * This function maps an a CSM Call reason to a string
 *
 * Returns: 
 *   char* : A pointer to a NULL terminated string representing the
 *           value in the "r" parameter.
 *   NULL : "RETURN CODE UNKNOWN" - the value in "r" is unknown.
 */
char *CSM_getCallReasonString(
    CSM_CallReason reason)
{
    return _CSM_callReasons[reason];
}

/* 
 * ======== CSM_isiPrintReturnString() ========
 *
 * This function maps an ISI return value into a string for printing
 * and diagnostic needs.
 *
 * Returns: 
 *   char* : A pointer to a NULL terminated string representing the
 *           value in the "r" parameter.
 *   NULL : "RETURN CODE UNKNOWN" - the value in "r" is unknown.
 */
char* CSM_isiPrintReturnString(
    ISI_Return r)
{
    char *str_ptr = NULL;
    switch (r) {
    case ISI_RETURN_FAILED:
        str_ptr = "ISI_RETURN_FAILED";
        break;
    case ISI_RETURN_TIMEOUT:
        str_ptr = "ISI_RETURN_TIMEOUT";
        break;
    case ISI_RETURN_INVALID_CONF_ID:
        str_ptr = "ISI_RETURN_INVALID_CONF_ID";
        break;
    case ISI_RETURN_INVALID_CALL_ID:
        str_ptr = "ISI_RETURN_INVALID_CALL_ID";
        break;
    case ISI_RETURN_INVALID_TEL_EVENT_ID:
        str_ptr = "ISI_RETURN_INVALID_TEL_EVENT_ID";
        break;
    case ISI_RETURN_INVALID_SERVICE_ID:
        str_ptr = "ISI_RETURN_INVALID_SERVICE_ID";
        break;
    case ISI_RETURN_INVALID_PRESENCE_ID:
        str_ptr = "ISI_RETURN_INVALID_PRESENCE_ID";
        break;
    case ISI_RETURN_INVALID_MESSAGE_ID:
        str_ptr = "ISI_RETURN_INVALID_MESSAGE_ID";
        break;
    case ISI_RETURN_INVALID_COUNTRY:
        str_ptr = "ISI_RETURN_INVALID_COUNTRY";
        break;
    case ISI_RETURN_INVALID_PROTOCOL:
        str_ptr = "ISI_RETURN_INVALID_PROTOCOL";
        break;
    case ISI_RETURN_INVALID_CREDENTIALS:
        str_ptr = "ISI_RETURN_INVALID_CREDENTIALS";
        break;
    case ISI_RETURN_INVALID_SESSION_DIR:
        str_ptr = "ISI_RETURN_INVALID_SESSION_DIR";
        break;
    case ISI_RETURN_INVALID_SERVER_TYPE:
        str_ptr = "ISI_RETURN_INVALID_SERVER_TYPE";
        break;
    case ISI_RETURN_INVALID_ADDRESS:
        str_ptr = "ISI_RETURN_INVALID_ADDRESS";
        break;
    case ISI_RETURN_INVALID_TEL_EVENT:
        str_ptr = "ISI_RETURN_INVALID_TEL_EVENT";
        break;
    case ISI_RETURN_INVALID_CODER:
        str_ptr = "ISI_RETURN_INVALID_CODER";
        break;
    case ISI_RETURN_NOT_SUPPORTED:
        str_ptr = "ISI_RETURN_NOT_SUPPORTED";
        break;
    case ISI_RETURN_DONE:
        str_ptr = "ISI_RETURN_DONE";
        break;
    case ISI_RETURN_SERVICE_ALREADY_ACTIVE:
        str_ptr = "ISI_RETURN_SERVICE_ALREADY_ACTIVE";
        break;
    case ISI_RETURN_SERVICE_NOT_ACTIVE:
        str_ptr = "ISI_RETURN_SERVICE_NOT_ACTIVE";
        break;
    case ISI_RETURN_SERVICE_BUSY:
        str_ptr = "ISI_RETURN_SERVICE_BUSY";
        break;
    case ISI_RETURN_NOT_INIT:
        str_ptr = "ISI_RETURN_NOT_INIT";
        break;
    case ISI_RETURN_MUTEX_ERROR:
        str_ptr = "ISI_RETURN_MUTEX_ERROR";
        break;
    case ISI_RETURN_OK:
        str_ptr = "ISI_RETURN_OK";
        break;
    case ISI_RETURN_INVALID_TONE:
        str_ptr = "ISI_RETURN_INVALID_TONE";
        break;
    case ISI_RETURN_INVALID_CHAT_ID:
        str_ptr = "ISI_RETURN_INVALID_GROUPCHAT_ID";
        break;
    default:
        str_ptr = "RETURN CODE UNKNOWN";
        break;
    }
    return str_ptr;
};

/* 
 * ======== CSM_isiPrintEvent() ========
 *
 * This function prints events as received by application from ISI_getEvent().
 *
 * Returns: 
 *   Nothing.
 *
 */
void CSM_isiPrintEvent(
    ISI_Id     serviceId,
    ISI_Id     id,
    ISI_IdType idType,
    ISI_Event  event,
    char      *eventDesc_ptr)
{
    char buffer[CSM_STRING_SZ << 1];

    /* First print the id type */
    if (idType > ISI_ID_TYPE_INVALID) {
        idType = ISI_ID_TYPE_INVALID;
    }
    if (event > ISI_EVENT_INVALID) {
        event = ISI_EVENT_INVALID;
    }

    if (NULL == eventDesc_ptr || 0 == *eventDesc_ptr) {
        OSAL_snprintf(buffer, CSM_STRING_SZ << 1,
                "APP EVENT RECV FOR ServiceID:%d ID:%d ID Type:%s\n"
                "  The Event :%s\n", serviceId, id, _CSM_isiPrintIdType[idType],
                _CSM_isiPrintEvent[event]);
    }
    else {
        OSAL_snprintf(buffer, CSM_STRING_SZ << 1,
                "APP EVENT RECV FOR ServiceID:%d ID:%d ID Type:%s\n"
                "The Event :%s (%s)\n", serviceId, id, 
                _CSM_isiPrintIdType[idType], _CSM_isiPrintEvent[event], 
                eventDesc_ptr);
    }
    CSM_dbgPrintf("%s", buffer);
    return;
}

/* 
 * ======== CSM_isiPrintTelEvt() ========
 *
 * This function prints telephone events (a.k.a. "tel events") 
 * as received by applications from a call to ISI_getEvent().
 *
 * Returns: 
 *   Nothing.
 *
 */
void CSM_isiPrintTelEvt(
    ISI_TelEvent event,
    char        *from_ptr,
    char        *dateTime_ptr,
    uint32       arg0,
    uint32       arg1)
{
    CSM_dbgPrintf("TELEVT: Evt:%s arg0:%d arg1:%d From:%s Date:%s\n",
            _CSM_isiPrintTelEvt[event], arg0, arg1, from_ptr, dateTime_ptr);
}

/*
 * ======== CSM_isiPrintIm() ========
 *
 * This function prints instant messages (a.k.a. "IM" or "text messages") 
 * that come from ISI.
 *
 * Returns: 
 *   Nothing.
 *
 */
void CSM_isiPrintIm(
    char  *from_ptr,
    ISI_Id messageId,
    char  *subject_ptr,
    char  *message_ptr,
    char  *dateTime_ptr,
    int    report,
    char  *reportId_ptr)
{
    CSM_dbgPrintf("IM MSG: from:%s messageId:%d subject:%s date:%s\n%s\n",
            from_ptr, messageId, subject_ptr, dateTime_ptr, message_ptr);
    CSM_dbgPrintf("IM REPORT: report:%x reportId:%s\n",
            report, reportId_ptr);
}

/* 
 * ======== CSM_isiPrintImReport() ========
 *
 * This function prints the IM Report.
 *
 * Returns: 
 *   Nothing.
 *
 */
void CSM_isiPrintImReport(
    char  *from_ptr,
    ISI_Id chatId,
    char  *dateTime_ptr,
    int    report,
    char  *reportId_ptr)
{
    CSM_dbgPrintf("IM REPORT: report:%x reportId:%s date:%s "
            "from:%s chatId:%d\n", report, reportId_ptr, dateTime_ptr, 
            from_ptr, chatId);
}

/* 
 * ======== CSM_isiPrintCallerID() ========
 *
 * This function prints the caller ID info of a call.
 *
 * Returns: 
 *   Nothing.
 *
 */
void CSM_isiPrintCallerId(
    char *from_ptr,
    char *subject_ptr)
{
    CSM_dbgPrintf("CALLER ID: from:%s subject:%s\n", 
            from_ptr, subject_ptr);
}

