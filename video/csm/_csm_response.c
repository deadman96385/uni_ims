/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30186 $ $Date: 2014-12-03 12:04:17 +0800 (Wed, 03 Dec 2014) $
 *
 */

#include <osal.h>
#include "_csm_calls.h"
#include "_csm_response.h"
#include "call_fsm/cfsm.h"
#include "_csm.h"

/*
 * ======== _CSM_writeCsmResponse() ========
 *
 * Private helper routine for writing a CSM Response message to the message Q.
 *
 * Returns: 
 *      none
 */
static void _CSM_writeCsmResponse(
    const CSM_OutputEvent *csmOutEvt_ptr)
{
    CSM_GlobalObj *_CSM_globalObj_ptr;
    OSAL_Status ret;

    _CSM_globalObj_ptr = CSM_getObject();

    ret = OSAL_msgQSend(_CSM_globalObj_ptr->queue.outEvtQ, 
            (void *)csmOutEvt_ptr, sizeof(CSM_OutputEvent), OSAL_NO_WAIT, NULL);
    if (OSAL_SUCCESS != ret) {
        OSAL_logMsg("%s: Failed to write command", __FUNCTION__);
    }
}

/*
 * ======== _CSM_writeCsmIsimResponse() ========
 *
 * Private helper routine for writing a CSM Response message to the isim
 * output event Q.
 *
 * Returns: 
 *      none
 */
static void _CSM_writeCsmIsimResponse(
    const CSM_OutputEvent *csmOutEvt_ptr)
{
    CSM_GlobalObj *_CSM_globalObj_ptr;
    OSAL_Status ret;

    _CSM_globalObj_ptr = CSM_getObject();

    ret = OSAL_msgQSend(_CSM_globalObj_ptr->queue.isimOutEvtQ, 
            (void *)csmOutEvt_ptr, sizeof(CSM_OutputEvent), OSAL_NO_WAIT, NULL);
    if (OSAL_SUCCESS != ret) {
        OSAL_logMsg("%s: Failed to write ISIM response", __FUNCTION__);
    }
    return;
}

/*
 * ======== CSM_sendOutputEvent() ========
 *
 * Public helper routine for sending a CSM reponse message.
 *
 * Returns: 
 *      none
 */
void CSM_sendOutputEvent(
    CSM_OutputEvent *outEvt_ptr)
{
    CSM_OutputCall    *callEvt_ptr = &outEvt_ptr->evt.call;
    CSM_OutputSms     *smsEvt_ptr = &outEvt_ptr->evt.sms;
    CSM_OutputService *srvEvt_ptr = &outEvt_ptr->evt.service;
#ifdef CSM_DEBUG
    CSM_OutputSupSrv  *supSrvEvt_ptr = &outEvt_ptr->evt.supSrv;
#endif
    CSM_CallSummary   *summary_ptr;
    CSM_OutputUssd    *ussd_ptr = &outEvt_ptr->evt.ussd;

    vint               index;
    vint               numCalls;

    /* Switch on the reponse type */
    if (CSM_EVENT_TYPE_CALL == outEvt_ptr->type) {
        /* Switch on the Call Reason */
        switch (callEvt_ptr->reason) {
            case CSM_OUTPUT_REASON_ERROR:
                CSM_dbgPrintf("CAPP: +CME ERROR %d [%s]\n",
                        callEvt_ptr->u.errorCode, 
                        callEvt_ptr->reasonDesc);
                break;
            case CSM_OUTPUT_REASON_OK:
                CSM_dbgPrintf("CAPP: OK\n");
                break;
            case CSM_OUTPUT_REASON_DISCONNECT_EVENT:
                CSM_dbgPrintf("CAPP: 3\n");
                break;
            case CSM_OUTPUT_REASON_CALL_LIST:
                CSM_dbgPrintf("CAPP:\n");
                numCalls = callEvt_ptr->u.report.numCalls;
                if (numCalls > 0) {
                    summary_ptr = callEvt_ptr->u.report.calls;
                    for (index = 0; index < numCalls; index++) {
                        CSM_dbgPrintf("\t+CLCC: %d,%d,%d,%d,%d,%s,%d\n", summary_ptr->idx, 
                                summary_ptr->direction, summary_ptr->state, 
                                summary_ptr->mode, summary_ptr->isMultiParty, 
                                summary_ptr->number, summary_ptr->type);
                        CSM_dbgPrintf("\t+CLCCS: %d,%d,%d,%d,\"%s\",%d,%d,%d,1,0,\"%s\"\n", summary_ptr->idx,
                            summary_ptr->direction, 
                            summary_ptr->negStatus == CSM_CALL_NEG_STATUS_INVALID?0:1,
                            summary_ptr->negStatus, 
                            summary_ptr->callSessionType & CSM_CALL_SESSION_TYPE_VIDEO? "m=audio%xD%xAm=video":"m=audio",
                            summary_ptr->mode, summary_ptr->status, summary_ptr->isMultiParty,
                            summary_ptr->normalizedAddress);
                        summary_ptr++;
                    }
                }
                else {
                    CSM_dbgPrintf("+CLCC:0\n");
                }
                break;
            case CSM_OUTPUT_REASON_CALL_MONITOR:
                CSM_dbgPrintf("CAPP:\n");
                numCalls = callEvt_ptr->u.report.numCalls;
                if (numCalls > 0) {
                    summary_ptr = callEvt_ptr->u.report.calls;
                    for (index = 0; index < numCalls; index++) {
                        CSM_dbgPrintf("\t+CMCCSI: %d,%d,%d,%d,\"%s%s\",%d,%d,%d,1,0,\"%s\"\n", summary_ptr->idx,
                            summary_ptr->direction,
                            summary_ptr->negStatus == CSM_CALL_NEG_STATUS_INVALID?0:1,
                            summary_ptr->negStatus,
                            summary_ptr->callSessionType & CSM_CALL_SESSION_TYPE_VIDEO? "m=audio\\0D\\0Am=video\\0D\\0A":"m=audio",
                            summary_ptr->coderSdpMd,
                            summary_ptr->mode, summary_ptr->status, summary_ptr->isMultiParty,
                            summary_ptr->normalizedAddress);
                        summary_ptr++;
                    }
                }
                break;
            case CSM_OUTPUT_REASON_INCOMING_EVENT:
                CSM_dbgPrintf("CAPP: +CRING: %s\n+CLIP: %s,%d",
                        callEvt_ptr->u.clipReport.callSessionType & 
                        CSM_CALL_SESSION_TYPE_VIDEO?"VOICE/VIDEO":"VOICE",
                        callEvt_ptr->u.clipReport.number, 
                        callEvt_ptr->u.clipReport.type);
                break;
            case CSM_OUTPUT_REASON_WAITING_EVENT:
                CSM_dbgPrintf("CAPP: +CCWA: %s,%d",
                        callEvt_ptr->u.clipReport.number, 
                        callEvt_ptr->u.clipReport.type);
                break;
            case CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT:
                CSM_dbgPrintf("CAPP: SRVCC result\n");
                break;
            case CSM_OUTPUT_REASON_INITIALIZING_EVENT:
                CSM_dbgPrintf("CAPP: Initializing, resource is not ready.\n");
                break;
            case CSM_OUTPUT_REASON_CALL_EXTRA_INFO:
                CSM_dbgPrintf("CAPP: Supsrv Call events for CSSI/CSSU.\n");
                break;
            case CSM_OUTPUT_REASON_CALL_INDEX:
                CSM_dbgPrintf("CAPP: Call Index = %d\n", callEvt_ptr->u.callIdx);
                break;
            case CSM_OUTPUT_REASON_CALL_DTMF_DETECT:
                CSM_dbgPrintf("DTMF detect digit=%c\n", callEvt_ptr->u.digit);
                break;
            case CSM_OUTPUT_REASON_CALL_VIDEO_REQUEST_KEY:
                CSM_dbgPrintf("Video Key frame reqeust\n");
                break;
            case CSM_OUTPUT_REASON_CALL_EARLY_MEDIA:
                CSM_dbgPrintf("There is early media.\n");
                break;
            default:
                CSM_dbgPrintf("%s:%s:%d: Illegal outEvt type\n", __FILE__, 
                        __FUNCTION__, __LINE__);
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SMS == outEvt_ptr->type) {
        /* Switch on the SMS Reason */
        switch(smsEvt_ptr->reason) {
            case CSM_OUTPUT_REASON_ERROR:
                CSM_dbgPrintf("CAPP: +CMS ERROR %d [%s]\n",
                        smsEvt_ptr->u.errorCode, 
                        smsEvt_ptr->reasonDesc);
                break;
            case CSM_OUTPUT_REASON_SMS_SENT:
                CSM_dbgPrintf("CAPP: +CMGS: %d", smsEvt_ptr->mr);
                break;
            case CSM_OUTPUT_REASON_SMS_RECEIVED:
                CSM_dbgPrintf("CAPP: +CMT: %s\n", smsEvt_ptr->u.msg.body);
                break;
            case CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED:
                CSM_dbgPrintf("CAPP: +CDS: %d\r\n%s",
                        OSAL_atoi(smsEvt_ptr->reasonDesc),
                        smsEvt_ptr->u.msg.body);
                break;
            case CSM_OUTPUT_REASON_OK:
                CSM_dbgPrintf("CAPP: SMS Got a OK response\n");
                break;
            default:
                CSM_dbgPrintf("%s:%s:%d: Illegal outEvt type\n", __FILE__,
                        __FUNCTION__, __LINE__);
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SERVICE == outEvt_ptr->type) {
        switch(srvEvt_ptr->reason) {
            case CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE:
                CSM_dbgPrintf("%s:%s:%d: AKA auth challenge\n", __FILE__,
                        __FUNCTION__, __LINE__);
                _CSM_writeCsmIsimResponse(outEvt_ptr);
                return;
            case CSM_OUTPUT_REASON_SERVICE_STATE:
                CSM_dbgPrintf("Service state changed. State:%d errorCode:%d\n",
                        srvEvt_ptr->state, srvEvt_ptr->errorCode);
                if (CSM_SERVICE_STATE_DEREGISTERING == srvEvt_ptr->state) {
                    return;
                }
                break;
            case CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP:
            case CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE:
                CSM_dbgPrintf("Service IPSec reason:%d info: "
                        "portUc=%d, portUs=%d, portPc=%d, portPs=%d, "
                        "spiUc=%d, spiUs=%d, spiPc=%d, spiPs=%d\n",
                        srvEvt_ptr->reason,
                        srvEvt_ptr->u.ipsec.portUc, srvEvt_ptr->u.ipsec.portUs,
                        srvEvt_ptr->u.ipsec.portPc, srvEvt_ptr->u.ipsec.portPs,
                        srvEvt_ptr->u.ipsec.spiUc, srvEvt_ptr->u.ipsec.spiUs,
                        srvEvt_ptr->u.ipsec.spiPc, srvEvt_ptr->u.ipsec.spiPs);
                break;
            case CSM_OUTPUT_REASON_ERROR:
                CSM_dbgPrintf("Service errorCode:%d reasonDesc:%s\n",
                        srvEvt_ptr->errorCode, srvEvt_ptr->reasonDesc); 
                break;
            default:
                OSAL_logMsg("%s:%s:%d: ERROR Illegal outEvt type\n", __FILE__,
                        __FUNCTION__, __LINE__);
                break;
        }
    }
    else if (CSM_EVENT_TYPE_SUPSRV == outEvt_ptr->type) {
        CSM_dbgPrintf("CSM_SUPSRV:%d Result : %d error code : %d\n",
                supSrvEvt_ptr->cmdType,
                supSrvEvt_ptr->reason,
                supSrvEvt_ptr->errorCode);
    }
     else if (CSM_EVENT_TYPE_USSD == outEvt_ptr->type) {
        switch(ussd_ptr->reason) {
            default:
            CSM_dbgPrintf("CSM_USSD: Result : %d error code : %d\n",
                    ussd_ptr->reason,
                    ussd_ptr->errorCode);
            break;
        }
    }
    else {
        OSAL_logMsg("%s:%s:%d: ERROR Illegal outEvt type\n", __FILE__, 
                __FUNCTION__, __LINE__);
    }
    _CSM_writeCsmResponse(outEvt_ptr);
}

/*
 * ======== CSM_sendError() ========
 *
 * Public helper routine for sending a CSM call error message.
 *
 * Returns: 
 *      none
 */
void CSM_sendError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutput_ptr->evt.call.reason = CSM_OUTPUT_REASON_ERROR;
    if (NULL == description_ptr) {
        description_ptr = "Error";
    }

    CSM_dbgPrintf("Error desc:%s", description_ptr);

    OSAL_strncpy(csmOutput_ptr->evt.call.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.call.reasonDesc));
    csmOutput_ptr->evt.call.u.errorCode = errorCode;
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendSmsError() ========
 *
 * Public helper routine for sending a CSM SMS error message.
 *
 * Returns: 
 *      none
 */
void CSM_sendSmsError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_SMS;
    csmOutput_ptr->evt.sms.reason = CSM_OUTPUT_REASON_ERROR;

    if (NULL == description_ptr) {
        description_ptr = "Error";
    }
    CSM_dbgPrintf("Error desc:%s", description_ptr);

    OSAL_strncpy(csmOutput_ptr->evt.sms.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.sms.reasonDesc));
    csmOutput_ptr->evt.sms.u.errorCode = (CSM_SmsErrorCode)errorCode;
    CSM_sendOutputEvent(csmOutput_ptr);
}
/*
 * ======== CSM_sendServiceError() ========
 *
 * Public helper routine for sending a CSM Service error message.
 *
 * Returns: 
 *      none
 */
void CSM_sendServiceError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_SERVICE;
    csmOutput_ptr->evt.service.reason = CSM_OUTPUT_REASON_ERROR;

    if (NULL == description_ptr) {
        description_ptr = "Error";
    }
    CSM_dbgPrintf("Error desc:%s\n", description_ptr);

    OSAL_strncpy(csmOutput_ptr->evt.service.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.service.reasonDesc));
    csmOutput_ptr->evt.service.errorCode = errorCode;
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendSupSrvError() ========
 *
 * Public helper routine for sending a CSM SUPSRV error message.
 *
 * Returns: 
 *      none
 */
void CSM_sendSupSrvError(
    int              errorCode,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_SUPSRV;
    csmOutput_ptr->evt.supSrv.reason = CSM_OUTPUT_REASON_ERROR;
    if (NULL == description_ptr) {
        description_ptr = "Error";
    }
    OSAL_strncpy(csmOutput_ptr->evt.supSrv.reasonDesc, description_ptr,
            CSM_EVENT_STRING_SZ);
    csmOutput_ptr->evt.supSrv.errorCode = (CSM_SupSrvErrorCode)errorCode;
    CSM_sendOutputEvent(csmOutput_ptr);
}
/*
 * ======== CSM_sendOk() ========
 *
 * Public helper routine for sending a CSM OK outEvt message.
 *
 * Returns: 
 *      none
 */
void CSM_sendOk(
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutput_ptr->evt.call.reason = CSM_OUTPUT_REASON_OK;

    if (NULL == description_ptr) {
        description_ptr = "OK";
    }

    OSAL_strncpy(csmOutput_ptr->evt.call.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.call.reasonDesc));
    csmOutput_ptr->evt.call.u.errorCode = 0;
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendRemoteDisconnect() ========
 *
 * Public helper routine for sending a CSM a remote disconnect event.
 *
 * Returns: 
 *      none
 */
void CSM_sendRemoteDisconnect(
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutput_ptr->evt.call.reason = CSM_OUTPUT_REASON_DISCONNECT_EVENT;

    if (NULL == description_ptr) {
        description_ptr = "Remote Disconnect";
    }

    OSAL_strncpy(csmOutput_ptr->evt.call.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.call.reasonDesc));
    csmOutput_ptr->evt.call.u.errorCode = 0;
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendAkaChallenge() ========
 *
 * Public helper routine for sending a AKA challenge to ISIM outEvt Queue.
 *
 * Returns: 
 *      none
 */
void CSM_sendAkaChallenge(
    char            *rand_ptr,
    char            *autn_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_SERVICE;
    csmOutput_ptr->evt.service.reason = CSM_OUTPUT_REASON_SERVICE_AUTH_CHALLENGE;
    OSAL_memCpy(csmOutput_ptr->evt.service.u.aka.akaRand, rand_ptr,
            CSM_AKA_RAND_STRING_SZ);
    OSAL_memCpy(csmOutput_ptr->evt.service.u.aka.akaAutn, autn_ptr,
            CSM_AKA_AUTN_STRING_SZ);
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendSrvccResult() ========
 *
 * Public helper routine for sending a CSM SRVCC outEvt message.
 *
 * Returns:
 *      none
 */
void CSM_sendSrvccResult(
    int              result,
    int              callId,
    char            *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutput_ptr->evt.call.reason = CSM_OUTPUT_REASON_SRVCC_RESULT_EVENT;

    if (NULL == description_ptr) {
        description_ptr = "SRVCC";
    }

    OSAL_strncpy(csmOutput_ptr->evt.call.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.call.reasonDesc));
    csmOutput_ptr->evt.call.u.srvcc.result = (CSM_SrvccResult)result;
    csmOutput_ptr->evt.call.u.srvcc.callId = callId;
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendIpsecEvent() ========
 *
 * Public helper routine for sending IPSec ports and SPI information
 * to outEvt Queue.
 *
 * Returns: 
 *      none
 */
void CSM_sendIpsecEvent(
    int              reason,
    int              portUc,
    int              portUs,
    int              portPc,
    int              portPs,
    int              spiUc,
    int              spiUs,
    int              spiPc,
    int              spiPs,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_SERVICE;
    if (CSM_SERVICE_REASON_IPSEC_SETUP == reason) {
        csmOutput_ptr->evt.service.reason = CSM_OUTPUT_REASON_SERVICE_IPSEC_SETUP;
    }
    else if (CSM_SERVICE_REASON_IPSEC_RELEASE == reason) {
        csmOutput_ptr->evt.service.reason = CSM_OUTPUT_REASON_SERVICE_IPSEC_RELEASE;
    }
    else {
        /* Unknown reasone */
        CSM_dbgPrintf("Unknown reason:%d", reason);
        return;
    }

    csmOutput_ptr->evt.service.u.ipsec.portUc = portUc;
    csmOutput_ptr->evt.service.u.ipsec.portUs = portUs;
    csmOutput_ptr->evt.service.u.ipsec.portPc = portPc;
    csmOutput_ptr->evt.service.u.ipsec.portPs = portPs;
    csmOutput_ptr->evt.service.u.ipsec.spiUc  = spiUc;
    csmOutput_ptr->evt.service.u.ipsec.spiUs  = spiUs;
    csmOutput_ptr->evt.service.u.ipsec.spiPc  = spiPc;
    csmOutput_ptr->evt.service.u.ipsec.spiPs  = spiPs;

    /* Send event */
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*cd bu 
 * ======== CSM_sendUssdError() ========
 *
 * Public helper routine for sending a CSM USSD error message.
 *
 * Returns: 
 *      none
 */
void CSM_sendUssdError(
    int   errorCode,
    char *description_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_USSD;
    csmOutput_ptr->evt.ussd.reason = CSM_OUTPUT_REASON_ERROR;

    if (NULL == description_ptr) {
        description_ptr = "Error";
    }
    CSM_dbgPrintf("Error desc:%s", description_ptr);

    OSAL_strncpy(csmOutput_ptr->evt.ussd.reasonDesc, description_ptr,
            sizeof(csmOutput_ptr->evt.ussd.reasonDesc));
    csmOutput_ptr->evt.ussd.errorCode = (CSM_UssdErrorCode)errorCode;
    CSM_sendOutputEvent(csmOutput_ptr);
}

/*
 * ======== CSM_sendEarlyMedia() ========
 *
 * Public helper routine for sending call is in early media state
 *
 * Returns: 
 *      none
 */
void CSM_sendEarlyMedia(
    CSM_OutputEvent *csmOutput_ptr)
{
    csmOutput_ptr->type = CSM_EVENT_TYPE_CALL;
    csmOutput_ptr->evt.call.reason = CSM_OUTPUT_REASON_CALL_EARLY_MEDIA;
    CSM_sendOutputEvent(csmOutput_ptr);
}

