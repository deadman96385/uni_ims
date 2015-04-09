/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24709 $ $Date: 2014-02-20 17:27:12 +0800 (Thu, 20 Feb 2014) $
 */

#ifndef _SIP_MSGCODES_H_
#define _SIP_MSGCODES_H_


/* macro definitions */
#define MSGCODE_ISPROV(code) (code <= eSIP_PROV_RSP_LAST)
#define MSGCODE_ISSUCCESS(code) (code >= eSIP_SUCCESS_RSP_FIRST && code <= eSIP_SUCCESS_RSP_LAST)
#define MSGCODE_ISREDIRECT(code) (code >= eSIP_REDIRECT_RSP_FIRST && code <= eSIP_REDIRECT_RSP_LAST)
#define MSGCODE_ISSERVER(code) (code >= eSIP_SERVER_RSP_FIRST && code <= eSIP_SERVER_RSP_LAST)
#define MSGCODE_ISGLOBAL(code) (code >= eSIP_GLOBAL_RSP_FIRST && code <= eSIP_GLOBAL_RSP_LAST)
#define MSGCODE_ISFINAL(code) (code >= eSIP_SUCCESS_RSP_FIRST && code <= eSIP_RSP_LAST_RESPONSE_CODE)
#define MSGCODE_ISFAILURE(code) (code >= eSIP_FAILURE_RSP_FIRST && code <= eSIP_RSP_LAST_RESPONSE_CODE)
#define MSGCODE_ISCHALLENGE(code) (code == eSIP_RSP_UNAUTH || code == eSIP_RSP_PROXY_AUTH_REQUIRED)


/* 1xx Provisional responses */
#define SIP_RSP_TRYING               100
#define SIP_RSP_RINGING              180
#define SIP_RSP_CALL_IS_BEING_FORW   181
#define SIP_RSP_QUEUED               182
#define SIP_RSP_SESSION_PROGRESS     183
/* 2xx Successful */
#define SIP_RSP_OK                   200
#define SIP_RSP_ACCEPTED             202
#define SIP_RSP_NO_NOTIFICATION      204
/* 3xx redirection */
#define SIP_RSP_MULTI_CHOICES        300
#define SIP_RSP_MOVED_PERM           301
#define SIP_RSP_MOVED_TEMP           302
#define SIP_RSP_USE_PROXY            305
#define SIP_RSP_ALTER_SERVICE        380
/* 4xx */
#define SIP_RSP_BAD_REQUEST          400   
#define SIP_RSP_UNAUTH               401
#define SIP_RSP_PAYMENT_REQUIRED     402
#define SIP_RSP_FORBIDDEN            403
#define SIP_RSP_NOT_FOUND            404
#define SIP_RSP_METHOD_NOT_ALLOWED   405
#define SIP_RSP_NOT_ACCEPT           406
#define SIP_RSP_PROXY_AUTH_REQUIRED  407
#define SIP_RSP_REQUEST_TIMEOUT      408
#define SIP_RSP_GONE                 410
#define SIP_RSP_REQ_ENT_TOO_LARGE    413
#define SIP_RSP_REQ_URI_TOO_LONG     414
#define SIP_RSP_UNSUPP_MEDIA_TYPE    415
#define SIP_RSP_UNSUPP_URI_SCHEME    416
#define SIP_RSP_BAD_EXT              420
#define SIP_RSP_EXT_REQUIRED         421
#define SIP_RSP_SESSION_TOO_SMALL    422
#define SIP_RSP_INTERVAL_TOO_BRIEF   423
#define SIP_RSP_SEND_FAILED          477
#define SIP_RSP_NO_PROCESS_URI       479
#define SIP_RSP_TEMP_UNAVAIL         480
#define SIP_RSP_CALL_TRANS_NO_EXIST  481
#define SIP_RSP_LOOP_DETECTED        482
#define SIP_RSP_TOO_MANY_HOPS        483
#define SIP_RSP_ADDR_INCOMPLETE      484
#define SIP_RSP_AMBIGUOUS            485
#define SIP_RSP_BUSY_HERE            486
#define SIP_RSP_REQUEST_TERMINATED   487
#define SIP_RSP_NOT_ACCEPTABLE_HERE  488
#define SIP_RSP_BAD_EVENT            489
#define SIP_RSP_REQUEST_PENDING      491
#define SIP_RSP_UNDECIPHERABLE       493
/* 5xx are responses from errors from the server */  
#define SIP_RSP_SERVER_INT_ERR       500
#define SIP_RSP_NOT_IMPLEMENTED      501
#define SIP_RSP_BAD_GATEWAY          502
#define SIP_RSP_SERVICE_UNAVAIL      503
#define SIP_RSP_SERVER_TIMEOUT       504
#define SIP_RSP_VERSION_NO_SUPPORT   505
#define SIP_RSP_MSG_TOO_LARGE        513
#define SIP_RSP_PRECONDITION_FAILURE 580
/* 6xx Global failures */
#define SIP_RSP_BUSY_EVERYWHERE      600
#define SIP_RSP_DECLINE              603
#define SIP_RSP_DOES_NOT_EXIST       604
#define SIP_RSP_NOT_ACCEPTABLE       606
/* D2 Error Codes are values under 100 */
#define SIP_RSP_CODE_UNKNOWN           0
#define SIP_RSP_CODE_XACT_TIMEOUT     99
#define SIP_RSP_CODE_INTERNAL_ERROR   98
#define SIP_RSP_CODE_ACK_TIMEOUT      97
#define SIP_RSP_CODE_AUTH_AKA_V1      96 /* Requires AKAv1 authentication. */
#define SIP_RSP_CODE_AUTH_AKA_V2      95 /* Requires AKAv2 authentication. */

/* THE EQUIVALENT STRINGS */
/* 1xx Provisional responses */
#define SIP_RSP_TRYING_STR                "Trying"
#define SIP_RSP_RINGING_STR               "Ringing"
#define SIP_RSP_CALL_IS_BEING_FORW_STR    "Call Is Being Forwarded"
#define SIP_RSP_QUEUED_STR                "Queued"
#define SIP_RSP_SESSION_PROGRESS_STR      "Session Progress"
/* 2xx Successful */
#define SIP_RSP_OK_STR                    "OK"
#define SIP_RSP_ACCEPTED_STR              "Accepted"
#define SIP_RSP_NO_NOTIFICATION_STR       "No Notification"
/* 3xx redirection */
#define SIP_RSP_MULTI_CHOICES_STR         "Multiple Choices"
#define SIP_RSP_MOVED_PERM_STR            "Moved Permanently"
#define SIP_RSP_MOVED_TEMP_STR            "Moved Temporarily"
#define SIP_RSP_USE_PROXY_STR             "Use Proxy"
#define SIP_RSP_ALTER_SERVICE_STR         "Alternative Service"
/* 4xx */
#define SIP_RSP_BAD_REQUEST_STR           "Bad Request"
#define SIP_RSP_UNAUTH_STR                "Unauthorized"
#define SIP_RSP_PAYMENT_REQUIRED_STR      "Payment Required"
#define SIP_RSP_FORBIDDEN_STR             "Forbidden"
#define SIP_RSP_NOT_FOUND_STR             "Not Found"
#define SIP_RSP_METHOD_NOT_ALLOWED_STR    "Method Not Allowed"
#define SIP_RSP_NOT_ACCEPT_STR            "Not Acceptable"
#define SIP_RSP_PROXY_AUTH_REQUIRED_STR   "Proxy Authentication Required"
#define SIP_RSP_REQUEST_TIMEOUT_STR       "Request Timeout"
#define SIP_RSP_GONE_STR                  "Gone"
#define SIP_RSP_REQ_ENT_TOO_LARGE_STR     "Request Entity Too Large"
#define SIP_RSP_REQ_URI_TOO_LONG_STR      "Request-URI Too Long"
#define SIP_RSP_UNSUPP_MEDIA_TYPE_STR     "Unsupported Media Type"
#define SIP_RSP_UNSUPP_URI_SCHEME_STR     "Unsupported URI Scheme"
#define SIP_RSP_BAD_EXT_STR               "Bad Extension"
#define SIP_RSP_EXT_REQUIRED_STR          "Extension Required"
#define SIP_RSP_SESSION_TOO_SMALL_STR     "Session Interval Too Small"
#define SIP_RSP_INTERVAL_TOO_BRIEF_STR    "Interval Too Brief"
#define SIP_RSP_SEND_FAILED_STR           "Send Failed"
#define SIP_RSP_NO_PROCESS_URI_STR        "Can't process URI"
#define SIP_RSP_TEMP_UNAVAIL_STR          "Temporarily Unavailable"
#define SIP_RSP_CALL_TRANS_NO_EXIST_STR   "Call/Transaction Does Not Exist"
#define SIP_RSP_SUBSCRIPTION_NO_EXIST_STR "Subscription Does Not Exist"
#define SIP_RSP_LOOP_DETECTED_STR         "Loop Detected"
#define SIP_RSP_TOO_MANY_HOPS_STR         "Too Many Hops"
#define SIP_RSP_ADDR_INCOMPLETE_STR       "Address Incomplete"
#define SIP_RSP_AMBIGUOUS_STR             "Ambiguous"
#define SIP_RSP_BUSY_HERE_STR             "Busy Here"
#define SIP_RSP_REQUEST_TERMINATED_STR    "Request Terminated"
#define SIP_RSP_NOT_ACCEPTABLE_HERE_STR   "Not Acceptable Here"
#define SIP_RSP_BAD_EVENT_STR             "Bad Event"
#define SIP_RSP_REQUEST_PENDING_STR       "Request Pending"
#define SIP_RSP_UNDECIPHERABLE_STR        "Undecipherable"
/* 5xx are responses from errors from the server */  
#define SIP_RSP_SERVER_INT_ERR_STR        "Server Internal Error"
#define SIP_RSP_NOT_IMPLEMENTED_STR       "Not Implemented"
#define SIP_RSP_BAD_GATEWAY_STR           "Bad Gateway"
#define SIP_RSP_SERVICE_UNAVAIL_STR       "Service Unavailable"
#define SIP_RSP_SERVER_TIMEOUT_STR        "Server Time-out"
#define SIP_RSP_VERSION_NO_SUPPORT_STR    "Version Not Supported"
#define SIP_RSP_MSG_TOO_LARGE_STR         "Message Too Large"
#define SIP_RSP_PRECONDITION_FAILURE_STR  "Precondition Failure"
/* 6xx Global failures */
#define SIP_RSP_BUSY_EVERYWHERE_STR       "Busy Everywhere"
#define SIP_RSP_DECLINE_STR               "Decline"
#define SIP_RSP_DOES_NOT_EXIST_STR        "Does Not Exist Anywhere"
#define SIP_RSP_NOT_ACCEPTABLE_STR        "Not Acceptable"
/* D2 Error codes */
#define SIP_RSP_CODE_UNKNOWN_STR          ""
#define SIP_RSP_CODE_XACT_TIMEOUT_STR     "Transaction Timeout"
#define SIP_RSP_CODE_INTERNAL_ERROR_STR   "Internal Error"
#define SIP_RSP_CODE_ACK_TIMEOUT_STR      "ACK Receipt Timeout"
#define SIP_RSP_CODE_AUTH_AKA_V1_STR      "AKAv1 Required"
#define SIP_RSP_CODE_AUTH_AKA_V2_STR      "AKAv2 Required"

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
    tSipMsgCodes code);

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
const char* MSGCODE_GetStr(tSipMsgCodes code);

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
int MSGCODE_GetNum(tSipMsgCodes code);

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
tSipMsgCodes MSGCODE_GetInt(int code);

#endif
