/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */
#include "sip_cfg.h"
#include "sip_types.h"
#include "sip_sip_const.h"
#include "sip_voipnet.h"
#include "sip_sip.h"
#include "sip_mem.h"
#include "sip_msgcodes.h"
#include "sip_mem_pool.h"

static tTokenizer _SipReturnCodes[] =
{
    /* 1xx */
    {   "",                               0,                            0   },
    {   SIP_RSP_TRYING_STR,               SIP_RSP_TRYING,               0   },
    {   SIP_RSP_RINGING_STR,              SIP_RSP_RINGING,              0   },
    {   SIP_RSP_CALL_IS_BEING_FORW_STR,   SIP_RSP_CALL_IS_BEING_FORW,   0   },
    {   SIP_RSP_QUEUED_STR,               SIP_RSP_QUEUED,               0   },
    {   SIP_RSP_SESSION_PROGRESS_STR,     SIP_RSP_SESSION_PROGRESS,     0   },
    /* 2xx */
    {   SIP_RSP_OK_STR,                   SIP_RSP_OK,                   0   },
    {   SIP_RSP_ACCEPTED_STR,             SIP_RSP_ACCEPTED,             0   },
    {   SIP_RSP_NO_NOTIFICATION_STR,      SIP_RSP_NO_NOTIFICATION,      0   },
    /* For eSIP_RSP_SUCCESS_2XX */
    {   0,                                0,                            0   },
    /* 3xx */
    {   SIP_RSP_MULTI_CHOICES_STR,        SIP_RSP_MULTI_CHOICES,        0   },
    {   SIP_RSP_MOVED_PERM_STR,           SIP_RSP_MOVED_PERM,           0   },
    {   SIP_RSP_MOVED_TEMP_STR,           SIP_RSP_MOVED_TEMP,           0   },
    {   SIP_RSP_USE_PROXY_STR,            SIP_RSP_USE_PROXY,            0   },
    {   SIP_RSP_ALTER_SERVICE_STR,        SIP_RSP_ALTER_SERVICE,        0   },
    /* 4xx */
    {   SIP_RSP_BAD_REQUEST_STR,          SIP_RSP_BAD_REQUEST,          0   },
    {   SIP_RSP_UNAUTH_STR,               SIP_RSP_UNAUTH,               0   },
    {   SIP_RSP_PAYMENT_REQUIRED_STR,     SIP_RSP_PAYMENT_REQUIRED,     0   },
    {   SIP_RSP_FORBIDDEN_STR,            SIP_RSP_FORBIDDEN,            0   },
    {   SIP_RSP_NOT_FOUND_STR,            SIP_RSP_NOT_FOUND,            0   },
    {   SIP_RSP_METHOD_NOT_ALLOWED_STR,   SIP_RSP_METHOD_NOT_ALLOWED,   0   },
    {   SIP_RSP_NOT_ACCEPT_STR,           SIP_RSP_NOT_ACCEPT,           0   },
    {   SIP_RSP_PROXY_AUTH_REQUIRED_STR,  SIP_RSP_PROXY_AUTH_REQUIRED,  0   },
    {   SIP_RSP_REQUEST_TIMEOUT_STR,      SIP_RSP_REQUEST_TIMEOUT,      0   },
    {   SIP_RSP_GONE_STR,                 SIP_RSP_GONE,                 0   },
    {   SIP_RSP_REQ_ENT_TOO_LARGE_STR,    SIP_RSP_REQ_ENT_TOO_LARGE,    0   },
    {   SIP_RSP_REQ_URI_TOO_LONG_STR,     SIP_RSP_REQ_URI_TOO_LONG,     0   },
    {   SIP_RSP_UNSUPP_MEDIA_TYPE_STR,    SIP_RSP_UNSUPP_MEDIA_TYPE,    0   },
    {   SIP_RSP_UNSUPP_URI_SCHEME_STR,    SIP_RSP_UNSUPP_URI_SCHEME,    0   },
    {   SIP_RSP_BAD_EXT_STR,              SIP_RSP_BAD_EXT,              0   },
    {   SIP_RSP_EXT_REQUIRED_STR,         SIP_RSP_EXT_REQUIRED,         0   },
    {   SIP_RSP_SESSION_TOO_SMALL_STR,    SIP_RSP_SESSION_TOO_SMALL,    0   },
    {   SIP_RSP_INTERVAL_TOO_BRIEF_STR,   SIP_RSP_INTERVAL_TOO_BRIEF,   0   },
    {   SIP_RSP_SEND_FAILED_STR,          SIP_RSP_SEND_FAILED,          0   },
    {   SIP_RSP_NO_PROCESS_URI_STR,       SIP_RSP_NO_PROCESS_URI,       0   },
    {   SIP_RSP_TEMP_UNAVAIL_STR,         SIP_RSP_TEMP_UNAVAIL,         0   },
    {   SIP_RSP_CALL_TRANS_NO_EXIST_STR,  SIP_RSP_CALL_TRANS_NO_EXIST,  0   },
    {   SIP_RSP_LOOP_DETECTED_STR,        SIP_RSP_LOOP_DETECTED,        0   },
    {   SIP_RSP_TOO_MANY_HOPS_STR,        SIP_RSP_TOO_MANY_HOPS,        0   },
    {   SIP_RSP_ADDR_INCOMPLETE_STR,      SIP_RSP_ADDR_INCOMPLETE,      0   },
    {   SIP_RSP_AMBIGUOUS_STR,            SIP_RSP_AMBIGUOUS,            0   },
    {   SIP_RSP_BUSY_HERE_STR,            SIP_RSP_BUSY_HERE,            0   },
    {   SIP_RSP_REQUEST_TERMINATED_STR,   SIP_RSP_REQUEST_TERMINATED,   0   },
    {   SIP_RSP_NOT_ACCEPTABLE_HERE_STR,  SIP_RSP_NOT_ACCEPTABLE_HERE,  0   },
    {   SIP_RSP_BAD_EVENT_STR,            SIP_RSP_BAD_EVENT,            0   },
    {   SIP_RSP_REQUEST_PENDING_STR,      SIP_RSP_REQUEST_PENDING,      0   },
    {   SIP_RSP_UNDECIPHERABLE_STR,       SIP_RSP_UNDECIPHERABLE,       0   },
    /* 5xx */
    {   SIP_RSP_SERVER_INT_ERR_STR,       SIP_RSP_SERVER_INT_ERR,       0   },
    {   SIP_RSP_NOT_IMPLEMENTED_STR,      SIP_RSP_NOT_IMPLEMENTED,      0   },
    {   SIP_RSP_BAD_GATEWAY_STR,          SIP_RSP_BAD_GATEWAY,          0   },
    {   SIP_RSP_SERVICE_UNAVAIL_STR,      SIP_RSP_SERVICE_UNAVAIL,      0   },
    {   SIP_RSP_SERVER_TIMEOUT_STR,       SIP_RSP_SERVER_TIMEOUT,       0   },
    {   SIP_RSP_VERSION_NO_SUPPORT_STR,   SIP_RSP_VERSION_NO_SUPPORT,   0   },
    {   SIP_RSP_MSG_TOO_LARGE_STR,        SIP_RSP_MSG_TOO_LARGE,        0   },
    {   SIP_RSP_PRECONDITION_FAILURE_STR, SIP_RSP_PRECONDITION_FAILURE, 0   },
    /* 6xx Global failures */
    {   SIP_RSP_BUSY_EVERYWHERE_STR,      SIP_RSP_BUSY_EVERYWHERE,      0   },
    {   SIP_RSP_DECLINE_STR,              SIP_RSP_DECLINE,              0   },
    {   SIP_RSP_DOES_NOT_EXIST_STR,       SIP_RSP_DOES_NOT_EXIST,       0   },
    {   SIP_RSP_NOT_ACCEPTABLE_STR,       SIP_RSP_NOT_ACCEPTABLE,       0   },
    /* D2 Error Codes are anything under 100 */
    {   SIP_RSP_CODE_XACT_TIMEOUT_STR,    SIP_RSP_CODE_XACT_TIMEOUT,    0   },
    {   SIP_RSP_CODE_INTERNAL_ERROR_STR,  SIP_RSP_CODE_INTERNAL_ERROR,  0   },
    {   SIP_RSP_CODE_ACK_TIMEOUT_STR,     SIP_RSP_CODE_ACK_TIMEOUT,     0   },
    {   SIP_RSP_CODE_AUTH_AKA_V1_STR,     SIP_RSP_CODE_AUTH_AKA_V1,     0   },
    {   SIP_RSP_CODE_AUTH_AKA_V2_STR,     SIP_RSP_CODE_AUTH_AKA_V2,     0   },
    {   SIP_RSP_CODE_UNKNOWN_STR,         SIP_RSP_CODE_UNKNOWN,         0   },
};

/* 
 ******************************************************************************
 * ================MSGCODE_Create()===================
 *
 * This function populates a tSipIntMsg with a response code.  It will also 
 * automatically make a copy of a message.
 *
 * pSourceMsg = A pointer to the source msg to populate with the response
 *              If NULL, they a new sip message is allocated and the response
 *              code is popluated.
 *
 * ppTarget = If pSourceMsg is NULL, then a pointer to the newly allocated 
 *            SIP message is populated here.
 *
 * code = The response code to populate in the message
 *
 * RETURNS: 
 *       SIP_OK: Function successful.
 *       SIP_BADPARM: The pSrc was NULL and ppTargetMsg was also NULL
 *       SIP_NO_MEM: Could not allocate the new memory off the heap
 *         
 ******************************************************************************
 */
int MSGCODE_Create(
    tSipIntMsg  *pSourceMsg,
    tSipIntMsg **ppTargetMsg,
    tSipMsgCodes code)
{
    tSipIntMsg *pOutputMsg = NULL;
        
    if (!pSourceMsg) {
        
        if (!ppTargetMsg) return (SIP_BADPARM);

        /* then make a fresh msg based on no source */
        if (!*ppTargetMsg) {
            *ppTargetMsg = SIP_allocMsg();
            if (!*ppTargetMsg) {
                return (SIP_NO_MEM);
            }
        }
        (*ppTargetMsg)->msgType = eSIP_RESPONSE;
        (*ppTargetMsg)->code = code;

        pOutputMsg = *ppTargetMsg;
    }
    else {

        /* clear out SDP if there is any in the source */
        if (MSGCODE_ISFAILURE(code)) {
            if (pSourceMsg->pSessDescr) {
                SDP_DeallocMsg(pSourceMsg->pSessDescr);
                pSourceMsg->pSessDescr = 0;
            }
            /* Clear out any other body data if there is any. */
            if (pSourceMsg->pMsgBody) {
                SIP_memPoolFree(eSIP_OBJECT_SIP_MSG_BODY,
                        (tDLListEntry *)pSourceMsg->pMsgBody);
                pSourceMsg->pMsgBody = 0;
            }
            pSourceMsg->ContentLength = 0;
            pSourceMsg->ContentType = eCONTENT_TYPE_NONE;
            HF_ClrPresence(&pSourceMsg->x.ECPresenceMasks, eSIP_CONTENT_TYPE_HF);
        }

        if (!ppTargetMsg) {
            /* then populatate the source rather than the target */
            pSourceMsg->msgType = eSIP_RESPONSE;
            pSourceMsg->code = code;
            pOutputMsg = pSourceMsg;
        }
        else if (!*ppTargetMsg) {
            *ppTargetMsg = SIP_copyMsg(pSourceMsg);
            if (!*ppTargetMsg) {
                return (SIP_NO_MEM);
            }
            (*ppTargetMsg)->msgType = eSIP_RESPONSE;
            (*ppTargetMsg)->code = code;
            pOutputMsg = *ppTargetMsg;
        }
        else { /* There is a source and target so poulate both */
            pSourceMsg->msgType = eSIP_RESPONSE;
            pSourceMsg->code = code;
            (*ppTargetMsg)->msgType = eSIP_RESPONSE;
            (*ppTargetMsg)->code = code;
            pOutputMsg = *ppTargetMsg;
        }
    }
    
    /* special error code handling */
    if (code == eSIP_RSP_METHOD_NOT_ALLOWED) {
        HF_SetPresence(&pOutputMsg->x.ECPresenceMasks, eSIP_ALLOW_HF);
        /*SYSDB_HF_Load(&pOutputMsg->x.ECPresenceMasks, pOutputMsg); */
    }
    
    return (SIP_OK);
}

/* 
 ******************************************************************************
 * ================MSGCODE_GetStr()===================
 *
 * This function returns the default string for the specified message code.
 *
 * code = The internal code enumeration of the desiered string
 * 
 * RETURNS: 
 *       NULL: internal code doesn't exist.
 *       char*: A pointer to a string
 *         
 ******************************************************************************
 */
const char* MSGCODE_GetStr(tSipMsgCodes code)
{
    if (code > eSIP_RSP_LAST_RESPONSE_CODE) {
        return (NULL);
    }
    else {
        return _SipReturnCodes[code].pExt;
    }
}

/* 
 ******************************************************************************
 * ================MSGCODE_GetNum()===================
 *
 * This function returns the numerical value for the specified enumerated code 
 * value.  For example, eSIP_RSP_RINGING returns '180'
 *
 * code = The internal code enumeration of the desired number
 * 
 * RETURNS: 
 *         0: internal code doesn't exist.
 *       int: The interger value defined in the RFC3261 spec.
 *         
 ******************************************************************************
 */
int MSGCODE_GetNum(tSipMsgCodes code)
{
    if (code > eSIP_RSP_LAST_RESPONSE_CODE) {
        return 0;
    }
    else {
        return _SipReturnCodes[code].Int;
    }
}

/* 
 ******************************************************************************
 * ================MSGCODE_GetNum()===================
 *
 * This function returns the enumerated value for the specified integer value 
 * For example, '180' returns eSIP_RSP_RINGING
 *
 * code = The integer value of the desired enumeration
 * 
 * RETURNS: 
 *                  0: internal code doesn't exist.
 *       tSipMsgCodes: The enumerated value of the specified integer value.
 *         
 ******************************************************************************
 */
tSipMsgCodes MSGCODE_GetInt(int code)
{
    int x = 0;

    /* a little optimization here */
    if (code > 399)
        x = eSIP_FAILURE_RSP_FIRST;
    if (code > 499)
        x = eSIP_SERVER_RSP_FIRST;
    
    for ( ; x < eSIP_RSP_LAST_RESPONSE_CODE ; x++) {
        if ((int)_SipReturnCodes[x].Int == code) {
            return (tSipMsgCodes)x;
        }
    }
    if (code >= 200 && code < 299) {
        return eSIP_RSP_SUCCESS_2XX;
    }
    return eSIP_RSP_LAST_RESPONSE_CODE;
}


