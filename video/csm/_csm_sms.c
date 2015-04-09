/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29866 $ $Date: 2014-11-17 14:42:10 +0800 (Mon, 17 Nov 2014) $
 *
 */
#include <ezxml.h>
#include <settings.h>
#include <pdu_hlpr.h>
#include "csm_event.h"
#include <rpm.h>
#include "_csm_utils.h"
#include "_csm_response.h"
#include "_csm_isi_sms.h"
#include "_csm_print.h"
#include "_csm_sms.h"
#include "_csm.h"
#include <pdu_3gpp.h>
#include <pdu_3gpp2.h>

/* Global class pointer for the SMS sub-module */
CSM_SmsMngr *mSmsMngr_ptr;
OSAL_Boolean isWaitForTmr;

/*
 ======== _CSM_smsGetMsgRef() ========
 *
 * Private method to get message reference id for sending SMS.
 *
 * Returns:
 *     Message reference id
 */
unsigned char _CSM_smsGetMsgRef(
    CSM_SmsMngr *smsMngr_ptr)
{
    return (smsMngr_ptr->msgRefId++);
}

/*
 * ======== _CSM_writeCsmEvent() ========
 *
 * This private helper routine is used to write an event to the CSM
 * private Input Event queue
 *
 * RETURN:
 *     CSM_OK : success.
 *     CSM_ERR: failed.
 */
static vint _CSM_smsWriteCsmEvent(
    const CSM_PrivateInputEvt   *csmEvt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(mSmsMngr_ptr->evtQId, (void *)csmEvt_ptr,
            sizeof(CSM_PrivateInputEvt), OSAL_NO_WAIT, NULL)) {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_smsSubmitTimeout() ========
 *
 * This function send an error output event when waitting
 * RP-ACK timeout
 *
 * Return Values:
 *     NONE
 */
static vint  _CSM_smsSubmitTimeout(
    OSAL_TmrArg arg)
{
    CSM_PrivateInputEvt csmEvt;

    csmEvt.type = CSM_PRIVATE_EVENT_TYPE_SMS;
    csmEvt.evt.sms.reason = CSM_SMS_REASON_EVT_SENT_TIMEOUT;
    return (_CSM_smsWriteCsmEvent(&csmEvt));
}

/*
 * ======== _CSM_smsTimerInit() ========
 *
 * This function initializes a timer used to send sms error output
 * event when SMS SUBMIT timeout.
 *
 * Return Values:
 *     CSM_OK: The timer was succesfully initialized.
 *     CSM_ERR: The timer failed to initialize
 */
static vint _CSM_smsTimerInit(
    CSM_SmsMngr *smsMngr_ptr)
{
    /* Launch a timer that will response error if ACK timeout */
    if (0 == smsMngr_ptr->tmrId) {
        /* Launch a timer that will attempt to register to ISI */
        if (0 == (smsMngr_ptr->tmrId = OSAL_tmrCreate())) {
            /* Then we can't register with ISI! */
            return (CSM_ERR);
        }
    }
    else {
        OSAL_tmrStop(smsMngr_ptr->tmrId);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrStart(smsMngr_ptr->tmrId,
            (OSAL_TmrPtr)_CSM_smsSubmitTimeout, (OSAL_TmrArg)NULL,
            CSM_SMS_RT_ACK_TIMER_MS)) {
        OSAL_tmrDelete(smsMngr_ptr->tmrId);
        smsMngr_ptr->tmrId = 0;
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_timerDestroy() ========
 *
 * This function will destroy the SMS timer
 *
 * Return Values:
 *     NONE
 */
static void _CSM_smsTimerDestroy(
    CSM_SmsMngr *smsMngr_ptr)
{
    /* Kill/Free the timer */
    if (0 != smsMngr_ptr->tmrId) {
        OSAL_tmrStop(smsMngr_ptr->tmrId);
        OSAL_tmrDelete(smsMngr_ptr->tmrId);
        smsMngr_ptr->tmrId = 0;
    }
    return;
}

/*
 * ======== _CSM_smsProcessPsIncomingMsg() ========
 *
 * Process incoming message with PDU APIs when the protocol is SIP
 *
 * Returns:
 *    CSM_OK: function exits normally.
 *    CSM_ERR: in case of error
 */
static vint _CSM_smsProcessPsIncomingMsg(
    CSM_SmsMngr     *smsMngr_ptr,
    CSM_OutputSms   *smsResp_ptr,
    CSM_SmsEvt      *smsEvt_ptr,
    char            *message,
    vint             bytes,
    ISI_MessageType  type,
    CSM_OutputEvent *csmOutput_ptr)
{
    char   errCode;
    char   pduTpye;
    vint   msgType;
    int8   replySeq;
    OSAL_TimeLocal  timeLocal;
    int    msgCoding;

    /* Get system time */
    OSAL_timeLocalTime(&timeLocal);
    OSAL_snprintf(smsResp_ptr->u.msg.scts, CSM_ALPHA_STRING_SZ,
            "%d/%d/%d,%d:%d:%d+00", (timeLocal.year - 100), timeLocal.mon + 1,
            timeLocal.mday, timeLocal.hour, timeLocal.min, timeLocal.sec);

    errCode = 0;
    switch (type) {
        case ISI_MSG_TYPE_TEXT:
            OSAL_logMsg("The message is:%s", smsResp_ptr->u.msg.body);
            PDU_3gppEncodeDeliverySms(smsResp_ptr->address,
                    smsResp_ptr->u.msg.body, smsMngr_ptr->scratch,
                    CSM_SMS_STRING_SZ, PDU_3GPP_DCS_DEFAULT);
            /* Now copy it back into the response. */
            OSAL_snprintf(smsResp_ptr->u.msg.body, CSM_SMS_STRING_SZ,
                    "%s", smsMngr_ptr->scratch);
            break;
        case ISI_MSG_TYPE_PDU_3GPP:
            /* XXX Currently doesn't support TEXT for 3GPP SMS. */
            if (CSM_SMS_PDU_MODE_TEXT == smsMngr_ptr->pduMode) {
                return (CSM_ERR);
            }
            /* decode SMS to get SMS type */
            if (0 != PDU_3gppDecodeSms(message, bytes, smsResp_ptr->u.msg.body,
                    CSM_SMS_STRING_SZ , smsMngr_ptr->scratch,
                    CSM_EVENT_STRING_SZ, &pduTpye, (char*)&smsResp_ptr->mr,
                    &errCode)) {
                /* Failed to decode the message, ignore it */
                return (CSM_ERR);
            }

            if (PDU_3GPP_TP_MTI_SMS_DELIVER == pduTpye) {
                /* Here is the incoming SMS, then send ACK back */
                if (0 != (smsEvt_ptr->msgLen = PDU_3gppEncodeDeliverReport(
                        smsMngr_ptr->scratch, smsResp_ptr->mr, errCode,
                        smsEvt_ptr->pdu, CSM_SMS_STRING_SZ))) {
                    /* Rewrite the address to use. */
                    _CSM_isiSmsSend(smsEvt_ptr, csmOutput_ptr);
                    if (0 != errCode) {
                        /*
                         * Failed to decode incoming SMS,
                         * don't send event to GAPP
                         */
                        break;
                    }
                }
            }
            else if (PDU_3GPP_TP_MTI_SMS_SUBMIT_REPORT == pduTpye) {
                /* We got ack or error response from SMSC */
                if (-1 == smsMngr_ptr->msgIdForWaitingAck) {
                    /* Nothing if we are not in wait-for-ack state */
                    break;
                }
                _CSM_smsTimerDestroy(smsMngr_ptr);
                smsMngr_ptr->msgIdForWaitingAck = -1;
                /* Construct event to notify the ack or error of SMS sending */
                smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_SENT;
                if (0 != errCode){
                    CSM_dbgPrintf("SMS submit errCode = %d\n", errCode);
                    CSM_sendSmsError(errCode, NULL, csmOutput_ptr);
                    break;
                }
                CSM_dbgPrintf("SMS submit OK mr = %d\n", smsResp_ptr->mr);
                break;
            }
            else if (PDU_3GPP_TP_MTI_SMS_STATUS_REPORT == pduTpye) {
                /* Got status report, reply an ack */
                if (0 != (smsEvt_ptr->msgLen = PDU_3gppEncodeDeliverReport(
                        smsMngr_ptr->scratch, smsResp_ptr->mr, errCode,
                        smsEvt_ptr->pdu, CSM_SMS_STRING_SZ))) {
                    /* Rewrite the address to use. */
                    _CSM_isiSmsSend(smsEvt_ptr, csmOutput_ptr);
                    if (0 != errCode) {
                        /*
                         * Failed to decode incoming SMS, don't send event
                         * to GAPP
                         */
                        break;
                    }
                }
                /* Construct event to notify the status report */
                smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED;
                OSAL_strncpy(smsResp_ptr->u.msg.body, message,
                        CSM_SMS_STRING_SZ);
            }

            break;
        case ISI_MSG_TYPE_PDU_3GPP2:
            /* XXX Currently only support TEXT for 3GPP2 SMS. */
            if (CSM_SMS_PDU_MODE_TEXT != smsMngr_ptr->pduMode) {
                return (CSM_ERR);
            }
            /* Decode the SMS. */
            if (PDU_ERR == PDU_3gpp2DecodeSms(
                    smsResp_ptr->u.msg.body,
                    bytes,
                    smsMngr_ptr->scratch,
                    sizeof(smsMngr_ptr->scratch),
                    &msgCoding,
                    smsResp_ptr->address, /* From addr */
                    sizeof(smsResp_ptr->address),
                    &msgType,
                    &smsResp_ptr->mr,
                    &replySeq,
                    (uint8 *)&errCode,
                    &smsResp_ptr->u.msg.numOfMessage)) {
                CSM_dbgPrintf("Failed to decode 3GPP2 SMS.\n");
                return (CSM_ERR);
            }
            /* Process depends on message type. */
            switch (msgType) {
                case (PDU_3GPP2_TS_MSG_TYPE_DELIVER):
                    /* New incoming SMS. */
                    smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_RECEIVED;
                    /* Copy decoded message. */
                    OSAL_strncpy(smsResp_ptr->u.msg.body,
                            smsMngr_ptr->scratch,
                            sizeof(smsResp_ptr->u.msg.body));
                    if (msgCoding == PDU_3GPP2_DATA_CODING_ASCII) {
                        smsResp_ptr->u.msg.dcs = CSM_SMS_DCS_DC_ASCII;
                    }
                    else if (msgCoding == PDU_3GPP2_DATA_CODING_UTF8) {
                        smsResp_ptr->u.msg.dcs = CSM_SMS_DCS_DC_UTF8;
                    }
                    else if (msgCoding == PDU_3GPP2_TELESERVICE_ID_VM) {
                        smsResp_ptr->u.msg.dcs = CSM_SMS_DCS_ME_VOICE_MAIL;
                        OSAL_snprintf(smsResp_ptr->u.msg.body, CSM_SMS_STRING_SZ,
                                "%d", smsResp_ptr->u.msg.numOfMessage);
                    }
                    else if (msgCoding == PDU_3GPP2_TELESERVICE_ID_PAGING) {
                        smsResp_ptr->u.msg.dcs = CSM_SMS_DCS_ME_OTHER_MSG;
                    }
                    else {
                        smsResp_ptr->u.msg.dcs = CSM_SMS_DCS_DC_RESERVED;
                    }
                    break;
                case (PDU_3GPP2_TS_MSG_TYPE_DELIVERY_ACK):
                    /* 
                     * Note: According to [C.S0015], if the incoming SMS message is an SMS
                     * “Delivery Acknowledgement” message or an SMS “User Acknowledgement”
                     * message, the actions to be taken are implementation dependent.
                     * Currently we only send CSM_Event to notify there is a "Delivery Ack"
                     * or "User Ack" message. 
                     */
                    smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED;
                    smsResp_ptr->u.reportType = CSM_SMS_REPORT_TYPE_DELIVERY_ACK;
                    break;
                case (PDU_3GPP2_TS_MSG_TYPE_USER_ACK):
                    smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_REPORT_RECEIVED;
                    smsResp_ptr->u.reportType = CSM_SMS_REPORT_TYPE_USER_ACK;
                    break;
                case (PDU_3GPP2_TS_MSG_TYPE_NONE):
                    /* Check if it's SMS ACK for Submit. */
                    if (0 <= replySeq) {
                        /*
                         * Type NONE and non-zero replySqe means it's a SMS ACK.
                         */
                        if (-1 == smsMngr_ptr->msgIdForWaitingAck) {
                            /* Nothing if we are not in wait-for-ack state */
                            break;
                        }
                        _CSM_smsTimerDestroy(smsMngr_ptr);
                        smsMngr_ptr->msgIdForWaitingAck = -1;
                        /* Construct event to notify the ack or error of SMS sending */
                        smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_SENT;
                        smsResp_ptr->mr = replySeq;
                        if (0 != errCode){
                            CSM_dbgPrintf("SMS submit errCode = %d\n", errCode);
                            CSM_sendSmsError(errCode, NULL, csmOutput_ptr);
                            break;
                        }
                        CSM_dbgPrintf("SMS submit OK mr = %d\n", smsResp_ptr->mr);
                        return (CSM_OK);
                    }
                    return (CSM_ERR);
                default:
                    CSM_dbgPrintf("Invalid 3GPP2 msg type: %d\n", msgType);
                    return (CSM_ERR);
            }

            if (0 <= replySeq) {
                /* Send SMS Acknowledge. */
                if (0 != smsMngr_ptr->smsc[0]) {
                    /* We have smsc configured, send to it. */
                    OSAL_strncpy(smsEvt_ptr->remoteAddress, smsMngr_ptr->smsc,
                            sizeof(smsEvt_ptr->remoteAddress));
                }
                if (0 < (smsEvt_ptr->msgLen = PDU_3gpp2EncodeSmsAck(
                        smsResp_ptr->address,
                        replySeq,
                        0, /* Cause code. */
                        smsEvt_ptr->pdu,
                        sizeof(smsEvt_ptr->pdu)))) {
                    _CSM_isiSmsSend(smsEvt_ptr, csmOutput_ptr);
                }
            }
            break;
        default:
            /* Do nothing */
            return (CSM_OK);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_smsConvertSIPOutPayload() ========
 *
 * Process outgoing message with PDU APIs when the protocol is SIP
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint _CSM_smsProcessPsOutgoingMsg(
    CSM_SmsMngr     *smsMngr_ptr,
    CSM_SmsEvt      *smsEvt_ptr,
    char            *addr_ptr,
    char            *realm_ptr,
    char            *callBackNumber_ptr)
{
    char *smscUri_ptr;
    char  smscNormalized[CSM_SMS_SMSC_STR_SZ];
    vint  msgRef;
    OSAL_Boolean bearerReplyOption;

    msgRef = -1;
    /* get SMSC */
    OSAL_memSet(smscNormalized, 0, CSM_SMS_SMSC_STR_SZ);

    if (0 != smsMngr_ptr->smsc[0]) {
        /* We have smsc configured, use it as destination address */
        smscUri_ptr = smsMngr_ptr->smsc;
        CSM_dbgPrintf("SMSC configured: %s\n", smscUri_ptr);
        if (CSM_OK != CSM_utilOutBoundCsCallNormalize(
                realm_ptr, smsMngr_ptr->smsc,
                smscNormalized, CSM_EVENT_STRING_SZ)){
            smscNormalized[0] = '0';
            CSM_dbgPrintf("OutBoundCsCallNormalize Failed\n");
        }
    }
    else {
        /* No smsc configured, use the remote address as destination address */
        smscUri_ptr = addr_ptr;
        CSM_dbgPrintf("SMSC is not configured, use remote address: %s\n",
                smscUri_ptr);
        if (CSM_OK != CSM_utilOutBoundCsCallNormalize(
                realm_ptr, addr_ptr,
                smscNormalized, CSM_EVENT_STRING_SZ)){
            smscNormalized[0] = '0';
            CSM_dbgPrintf("OutBoundCsCallNormalize Failed\n");
        }
    }

    if (CSM_SMS_PDU_MODE_TEXT == smsMngr_ptr->pduMode) {
        /* Plain text, encode it. */
        switch (smsEvt_ptr->type) {
            case CSM_SMS_TYPE_TEXT:
                /* Do nothing for TEXT type */
                break;
            case CSM_SMS_TYPE_PDU_3GPP:
                /* XXX Encode 3GPP format. Currently not support. */
                return (CSM_ERR);
            case CSM_SMS_TYPE_PDU_3GPP2:
                /* Normalize to address. */
                if (CSM_OK != CSM_utilOutBoundCsCallNormalize(
                        realm_ptr, addr_ptr,
                        smsMngr_ptr->scratch, CSM_EVENT_STRING_SZ)){
                    smscNormalized[0] = '0';
                    CSM_dbgPrintf("OutBoundCsCallNormalize Failed\n");
                    return (CSM_ERR);
                }
                /* Encode 3GPP2 format */
                msgRef = _CSM_smsGetMsgRef(smsMngr_ptr);
                bearerReplyOption = OSAL_TRUE;
                if (0 == (smsEvt_ptr->msgLen = PDU_3gpp2EncodeSubmit(
                        smsEvt_ptr->message,
                        smsMngr_ptr->scratch,
                        smscNormalized,
                        (uint8)msgRef,
                        smsEvt_ptr->pdu,
                        sizeof(smsEvt_ptr->pdu),
                        bearerReplyOption, 
                        callBackNumber_ptr))) {
                    CSM_dbgPrintf("Fail to encode 3GPP2 SMS.\n");
                }
                break;
            default:
                CSM_dbgPrintf("Invalid SMS type %d\n", smsEvt_ptr->type);
                return (CSM_ERR);
        }
    }
    else if (CSM_SMS_PDU_MODE_TPDU == smsMngr_ptr->pduMode) {
        /* Only support TPUD mode for 3GPP */
        if (CSM_SMS_TYPE_PDU_3GPP != smsEvt_ptr->type) {
            CSM_dbgPrintf("TPDU mode is not support for text or 3GPP2 SMS\n");
            return (CSM_ERR);
        }
        /*
         * Convert the PDU from a PDU in HEX string to a raw pdu excludes the
         * end Ctrl-Z.
         */
        smsEvt_ptr->msgLen =
                PDU_pduHexStringToBytes(smsEvt_ptr->pdu,
                (unsigned char *)smsMngr_ptr->scratch) - 1;

        /* Get unique message reference */
        msgRef = _CSM_smsGetMsgRef(smsMngr_ptr);
        /* Encode Submit to RP data */
        smsEvt_ptr->msgLen = PDU_3gppEncodeSubmit(smsMngr_ptr->scratch,
                smsEvt_ptr->msgLen, smscNormalized, (char)msgRef,
                smsEvt_ptr->pdu, CSM_SMS_STRING_SZ);
    }

    OSAL_snprintf(smsEvt_ptr->remoteAddress, CSM_EVENT_STRING_SZ,
            "%s", smscUri_ptr);

    /* Initiate a timer for RP-ACK(3GPP) or SMS ACK(3GPP2) */
    if (-1 != msgRef) {
        smsMngr_ptr->msgIdForWaitingAck = msgRef;
        if (CSM_OK != _CSM_smsTimerInit(smsMngr_ptr)) {
            /* Return an error. */
            smsMngr_ptr->msgIdForWaitingAck = -1;
            return (CSM_ERR);
        }
    }
    return (CSM_OK);
}

/*
 * ======== CSM_smsInit() ========
 *
 * Initialization routine for the CSM SMS manager package
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_smsInit(
    CSM_SmsMngr        *smsManager_ptr,
    CSM_IsiMngr        *isiMngr_ptr,
    void               *cfg_ptr)
{
    char      *value_ptr;

    OSAL_logMsg("%s:%d\n", __FUNCTION__, __LINE__);

    mSmsMngr_ptr = smsManager_ptr;
    mSmsMngr_ptr->isiMngr_ptr = isiMngr_ptr;
    mSmsMngr_ptr->msgRefId    = 0;
    mSmsMngr_ptr->evtQId      = isiMngr_ptr->eventQ;

    /* Get SMS PDU type, TPDU, RPDU or plain text. Plain text is default */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SMS, NULL, NULL,
            SETTINGS_PARM_SMS_PDU_FMT))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_PDU_FMT_RPDU,
                sizeof(SETTINGS_PARM_VALUE_PDU_FMT_RPDU))) {
            smsManager_ptr->pduMode = CSM_SMS_PDU_MODE_RPDU;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_PDU_FMT_TPDU,
                sizeof(SETTINGS_PARM_VALUE_PDU_FMT_TPDU))) {
            smsManager_ptr->pduMode = CSM_SMS_PDU_MODE_TPDU;
        }
        else {
            /* Default is plain text. */
            smsManager_ptr->pduMode = CSM_SMS_PDU_MODE_TEXT;
        }
    }
    else {
        /* CSM will handle PDU defualt type is plain text. */
        smsManager_ptr->pduMode = CSM_SMS_PDU_MODE_TEXT;
    }

    return (CSM_OK);
}

/*
 * ======== CSM_smsProcessEvent() ========
 *
 * Main entry point into the SMS Package.
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_smsProcessEvent(
    CSM_SmsMngr     *smsMngr_ptr,
    CSM_SmsEvt      *smsEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_IsiService  *service_ptr;
    CSM_OutputSms   *smsResp_ptr;

    ISI_MessageReport   reports;
    ISI_MessageType     type;
    vint                bytes;
    vint                maxLen;
    char                subject[ISI_SUBJECT_STRING_SZ + 1];
    char                dateTime[ISI_DATE_TIME_STRING_SZ + 1];
    char                reportId[ISI_ID_STRING_SZ + 1];
    char                address[ISI_ADDRESS_STRING_SZ + 1];
    CSM_CallAddressType addressType;
    char                callBackNumber[ISI_ADDRESS_STRING_SZ + 1];
    ISI_Id              chatId;
    char               *pos_ptr;

    ISI_Return ret;

    CSM_dbgPrintf("Reason: %d\n", smsEvt_ptr->reason);

    switch (smsEvt_ptr->reason) {
        case CSM_SMS_EVT_REASON_AT_CMD_SEND_MESSAGE:
            /*
             * User wants to send an SMS
             */
            /* Get the service to use and set the serviceId. */
            service_ptr = CSM_isiNormalizeOutboundAddress(
                    smsMngr_ptr->isiMngr_ptr, smsEvt_ptr->remoteAddress,
                    address, ISI_ADDRESS_STRING_SZ, RPM_FEATURE_TYPE_SMS);
            if (NULL != service_ptr) {
                smsEvt_ptr->serviceId = service_ptr->serviceId;
                    /*
                     * If this is going over SIP then make a decision about
                     * whether to send it as plain 'text' or a PDU.
                     */

#ifndef SIP_SMS_USE_PDU
                /* Then over write the type to enforce 'text'. */
                smsEvt_ptr->type = CSM_SMS_TYPE_TEXT;
                OSAL_snprintf(smsEvt_ptr->remoteAddress, CSM_EVENT_STRING_SZ,
                        "%s", address);
#else
                CSM_utilGetNameFromAddress(service_ptr->uri, callBackNumber, 
                    ISI_ADDRESS_STRING_SZ);
                if (CSM_ISI_PROTOCOL_SIP == service_ptr->protocol) {
                    if (CSM_ERR == _CSM_smsProcessPsOutgoingMsg(smsMngr_ptr,
                            smsEvt_ptr, address, service_ptr->realm, 
                            callBackNumber)) {
                        CSM_sendSmsError(CSM_SMS_ERROR_SIP, NULL, 
                                csmOutput_ptr);
                        return (CSM_ERR);
                    }
                }
                else {
                    /* Rewrite the address to use. */
                    OSAL_snprintf(smsEvt_ptr->remoteAddress,
                            CSM_EVENT_STRING_SZ, "%s", address);
                }
#endif
                _CSM_isiSmsSend(smsEvt_ptr, csmOutput_ptr);
            }
            else {
                /* Return an error. */
                CSM_sendSmsError(CSM_SMS_ERROR_SIP, NULL, csmOutput_ptr);
            }
            break;
        case CMS_SMS_EVT_REASON_EVT_NEW_INCOMING:
            /*
             * We've receive a new incoming SMS.  Process it.
             */
            service_ptr = CSM_isiGetServiceViaId(smsMngr_ptr->isiMngr_ptr,
                    smsEvt_ptr->serviceId);
            if (NULL == service_ptr) {
                /* Service unknown! just return. */
                break;
            }
            /* Construct response message */
            csmOutput_ptr->type = CSM_EVENT_TYPE_SMS;
            smsResp_ptr = &csmOutput_ptr->evt.sms;
            smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_RECEIVED;
            OSAL_strncpy(smsResp_ptr->reasonDesc, smsEvt_ptr->reasonDesc,
                    CSM_EVENT_STRING_SZ);
            smsResp_ptr->mr = 0;

            /* Let's get the rest of the message. */
            ret = ISI_getMessageHeader(smsEvt_ptr->id, &type,
                    subject, address, dateTime, &reports, reportId);
            if (ISI_RETURN_OK != ret) {
                OSAL_logMsg("%s:%d ERROR in ISI_getMessageHeader ret=%s",
                        __FUNCTION__, __LINE__, CSM_isiPrintReturnString(ret));
                break;
            }
            /* Normalize address by inbound rules */
            CSM_isiNormalizeInboundAddress(service_ptr, address,
                smsResp_ptr->address, CSM_ALPHA_STRING_SZ, &addressType,
                subject, ISI_SUBJECT_STRING_SZ);

            pos_ptr = smsResp_ptr->u.msg.body;
            maxLen = CSM_SMS_STRING_SZ;

            // read the whole message
            while (ret == ISI_RETURN_OK) {
                bytes = maxLen;
                ret = ISI_readMessage(smsEvt_ptr->id, &chatId,
                        pos_ptr, &bytes);
                if (ISI_RETURN_OK == ret) {
                    pos_ptr += bytes;
                    if (bytes >= maxLen) {
                        /* then we are done. */
                        break;
                    }
                    maxLen -= bytes;
                }
                else if (ISI_RETURN_DONE == ret) {
                    pos_ptr += bytes;
                }
            }
            /* NULL Terminate the buffer and send the response. */
            *pos_ptr = 0;
            /* If it's from SIP then convert as needed. */
            if (CSM_ISI_PROTOCOL_SIP == service_ptr->protocol) {
                switch (type) {
                    case ISI_MSG_TYPE_TEXT:
                        smsEvt_ptr->type = CSM_SMS_TYPE_TEXT;
                        break;
                    case ISI_MSG_TYPE_PDU_3GPP:
                        smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP;
                        break;
                    case ISI_MSG_TYPE_PDU_3GPP2:
                        smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP2;
                        break;
                }
                OSAL_snprintf(smsEvt_ptr->remoteAddress, CSM_EVENT_STRING_SZ,
                        "%s", address);

                if ((CSM_SMS_PDU_MODE_TPDU == smsMngr_ptr->pduMode) ||
                        ((CSM_SMS_PDU_MODE_TEXT == smsMngr_ptr->pduMode) &&
                        (ISI_MSG_TYPE_TEXT != type))) {
                    /* Now convert the payload based on the type. */
                    if (CSM_ERR == _CSM_smsProcessPsIncomingMsg(smsMngr_ptr,
                            smsResp_ptr, smsEvt_ptr, smsResp_ptr->u.msg.body,
                            bytes, type, csmOutput_ptr)) {
                        return (CSM_ERR);
                    }
                    smsResp_ptr->u.msg.len = OSAL_strlen(smsResp_ptr->u.msg.body);
                }
                else {
                    /*
                     * If PDU mode is RPDU, do nothing only passthrough raw PDU
                     * and set the length of RPDATA. The length need excludes
                     * the end 0x0d 0x0a
                     */
                    smsResp_ptr->u.msg.len = bytes;
                }
            }
            CSM_isiPrintIm(smsResp_ptr->address, smsEvt_ptr->id, subject,
                    smsResp_ptr->u.msg.body, dateTime, reports, reportId);
            CSM_sendOutputEvent(csmOutput_ptr);
            break;
        case CMS_SMS_EVT_REASON_EVT_MWI_INACTIVE:
        case CMS_SMS_EVT_REASON_EVT_MWI_ACTIVE:
            service_ptr = CSM_isiGetServiceViaId(smsMngr_ptr->isiMngr_ptr,
                    smsEvt_ptr->serviceId);
            if (NULL == service_ptr) {
                /* Service unknown! just return. */
                break;
            }
            /* Construct response message */
            csmOutput_ptr->type    = CSM_EVENT_TYPE_SMS;
            smsResp_ptr            = &csmOutput_ptr->evt.sms;
            smsResp_ptr->reason    = CSM_OUTPUT_REASON_SMS_RECEIVED;
            smsResp_ptr->mr  = 0;
            OSAL_strncpy(smsResp_ptr->reasonDesc, smsEvt_ptr->reasonDesc,
                    CSM_EVENT_STRING_SZ);
            /* Normalize the remote address */
            CSM_isiNormalizeInboundAddress(service_ptr,
                    smsEvt_ptr->remoteAddress,
                    smsResp_ptr->address, CSM_ALPHA_STRING_SZ, &addressType,
                    subject, ISI_SUBJECT_STRING_SZ);

            /* If it's from SIP then convert as needed. */
            if (CSM_ISI_PROTOCOL_SIP == service_ptr->protocol) {
                if (CMS_SMS_EVT_REASON_EVT_MWI_ACTIVE == smsEvt_ptr->reason) {
                    PDU_3gppEncodeDeliverySms(smsResp_ptr->address,
                            smsResp_ptr->u.msg.body, smsMngr_ptr->scratch,
                            CSM_SMS_STRING_SZ, PDU_3GPP_DCS_VOICEMAIL_ON);
                }
                else {
                    PDU_3gppEncodeDeliverySms(smsResp_ptr->address,
                            smsResp_ptr->u.msg.body, smsMngr_ptr->scratch,
                            CSM_SMS_STRING_SZ, PDU_3GPP_DCS_VOICEMAIL_OFF);
                }
                /* Now copy it back into the response. */
                OSAL_snprintf(smsResp_ptr->u.msg.body, CSM_SMS_STRING_SZ,
                        "%s", smsMngr_ptr->scratch);
            }
            CSM_isiPrintIm(smsResp_ptr->address, smsEvt_ptr->id, subject,
                    smsResp_ptr->u.msg.body, dateTime, reports, reportId);
            CSM_sendOutputEvent(csmOutput_ptr);
            break;

        case CMS_SMS_EVT_REASON_EVT_SEND_OK:
             /* Get protocol*/
            service_ptr = CSM_isiGetServiceViaId(smsMngr_ptr->isiMngr_ptr,
                    smsEvt_ptr->serviceId);
#ifdef SIP_SMS_USE_PDU
            /*
             * Ack the SMS send when receiving RP-ACK for 3GPP SMS.
             * So if using PDU and the protocol is SIP then ignore it.
             */
            if (CSM_ISI_PROTOCOL_GSM == service_ptr->protocol) {
#endif
                /* SMS sent sucessfully by GSM, notify user */
                csmOutput_ptr->type = CSM_EVENT_TYPE_SMS;
                smsResp_ptr = &csmOutput_ptr->evt.sms;
                smsResp_ptr->reason = CSM_OUTPUT_REASON_SMS_SENT;
                /* get real 'mr' handling for delivery reports. */
                smsResp_ptr->mr = OSAL_atoi(smsEvt_ptr->reasonDesc);
                CSM_dbgPrintf("mr=%d", smsResp_ptr->mr);
                CSM_sendOutputEvent(csmOutput_ptr);
                break;
#ifdef SIP_SMS_USE_PDU
            }
#endif
#ifdef SIP_SMS_USE_PDU
            /*
             * If PDU mode is RPDU, we should send OK
             * to notify command is completed
             */
            if (CSM_SMS_PDU_MODE_RPDU == smsMngr_ptr->pduMode) {
                csmOutput_ptr->type = CSM_EVENT_TYPE_SMS;
                smsResp_ptr = &csmOutput_ptr->evt.sms;
                smsResp_ptr->reason = CSM_OUTPUT_REASON_OK;
                CSM_sendOutputEvent(csmOutput_ptr);
            }
            break;
#endif
        case CMS_SMS_EVT_REASON_EVT_SEND_ERROR:
            /* Stop timer and Send failed.  Alert the user */
            _CSM_smsTimerDestroy(smsMngr_ptr);
            smsMngr_ptr->msgIdForWaitingAck = -1;
            CSM_sendSmsError(CSM_SMS_ERROR_SIP, smsEvt_ptr->reasonDesc, csmOutput_ptr);
            break;
        case CSM_SMS_EVT_REASON_SET_SMSC:
            /* Set smsc address to sms manager */
            OSAL_strncpy(smsMngr_ptr->smsc, smsEvt_ptr->smsc,
                    sizeof(smsMngr_ptr->smsc));
            CSM_dbgPrintf("SMSC address:%s\n", smsMngr_ptr->smsc);
            break;
        case CSM_SMS_EVT_REASON_EVT_SENT_TIMEOUT:
            /* Stop timer and Send failed.  Alert the user */
            _CSM_smsTimerDestroy(smsMngr_ptr);
            smsMngr_ptr->msgIdForWaitingAck = -1;
            CSM_sendSmsError(CSM_SMS_ERROR_NETWORK_TIMEOUT, NULL, csmOutput_ptr);
            break;
        default:
            return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * CSM_smsShutdown()
 */
vint CSM_smsShutdown(
    CSM_SmsMngr *smsMngr_ptr)
{
    OSAL_logMsg("%s:%d", __FUNCTION__, __LINE__);

    /* Destroy timer */
    if (0 != smsMngr_ptr->tmrId) {
        _CSM_smsTimerDestroy(smsMngr_ptr);
        smsMngr_ptr->tmrId = 0;
    }
    return (CSM_OK);
}

/*
 * ======== CSM_smsConvertToInternalEvt() ========
 *
 * This function is used to convert CSM input event to CSM call internal event.
 *
 * Returns:
 *
 */
void CSM_smsConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputSmsEvt_ptr,
    CSM_SmsEvt         *csmSmsEvt_ptr)
{
    CSM_InputSms       *csmExtSmsEvt_ptr;
    CSM_InputIsiSms    *csmIntSmsEvt_ptr;
    CSM_SmsEvtReason    reason;

    OSAL_memSet(csmSmsEvt_ptr, 0, sizeof(CSM_SmsEvt));

    if (CSM_INPUT_EVT_TYPE_EXT == type) {
        csmExtSmsEvt_ptr = (CSM_InputSms*)inputSmsEvt_ptr;
        switch (csmExtSmsEvt_ptr->reason) {
            case CSM_SMS_REASON_AT_CMD_SEND_MESSAGE:
                reason = CSM_SMS_EVT_REASON_AT_CMD_SEND_MESSAGE;
                break;
            case CSM_SMS_REASON_SET_SMSC:
                reason = CSM_SMS_EVT_REASON_SET_SMSC;
                break;
            default:
                reason = CMS_SMS_EVT_REASON_EVT_INVALID;
                OSAL_logMsg("Invalid CSM SMS reason.");
        }
        csmSmsEvt_ptr->reason  = reason;
        csmSmsEvt_ptr->type    = csmExtSmsEvt_ptr->type;
        csmSmsEvt_ptr->msgLen  = csmExtSmsEvt_ptr->msgLen;
        OSAL_memCpy(csmSmsEvt_ptr->reasonDesc, csmExtSmsEvt_ptr->reasonDesc,
                sizeof(csmSmsEvt_ptr->reasonDesc));
        OSAL_memCpy(csmSmsEvt_ptr->remoteAddress,
                csmExtSmsEvt_ptr->remoteAddress,
                sizeof(csmSmsEvt_ptr->remoteAddress));
        OSAL_memCpy(csmSmsEvt_ptr->message, csmExtSmsEvt_ptr->message,
                sizeof(csmSmsEvt_ptr->message));
        OSAL_memCpy(csmSmsEvt_ptr->pdu, csmExtSmsEvt_ptr->pdu,
                sizeof(csmSmsEvt_ptr->pdu));
        OSAL_memCpy(csmSmsEvt_ptr->smsc, csmExtSmsEvt_ptr->smsc,
                sizeof(csmSmsEvt_ptr->smsc));
    }
    else if (CSM_INPUT_EVT_TYPE_INT == type) {
        csmIntSmsEvt_ptr = (CSM_InputIsiSms*)inputSmsEvt_ptr;
        switch (csmIntSmsEvt_ptr->reason) {
            case CSM_SMS_REASON_EVT_SEND_OK:
                reason = CMS_SMS_EVT_REASON_EVT_SEND_OK;
                break;
            case CSM_SMS_REASON_EVT_SEND_ERROR:
                reason = CMS_SMS_EVT_REASON_EVT_SEND_ERROR;
                break;
            case CSM_SMS_REASON_EVT_NEW_INCOMING:
                reason = CMS_SMS_EVT_REASON_EVT_NEW_INCOMING;
                break;
            case CSM_SMS_REASON_EVT_MWI_ACTIVE:
                reason = CMS_SMS_EVT_REASON_EVT_MWI_ACTIVE;
                break;
            case CSM_SMS_REASON_EVT_MWI_INACTIVE:
                reason = CMS_SMS_EVT_REASON_EVT_MWI_INACTIVE;
                break;
            case CSM_SMS_REASON_EVT_SENT_TIMEOUT:
                reason = CSM_SMS_EVT_REASON_EVT_SENT_TIMEOUT;
                break;
            default:
                reason = CMS_SMS_EVT_REASON_EVT_INVALID;
                OSAL_logMsg("Invalid CSM SMS reason.");
        }
        csmSmsEvt_ptr->reason    = reason;
        csmSmsEvt_ptr->id        = csmIntSmsEvt_ptr->id;
        csmSmsEvt_ptr->serviceId = csmIntSmsEvt_ptr->serviceId;
        csmSmsEvt_ptr->type      = csmIntSmsEvt_ptr->type;
        OSAL_memCpy(csmSmsEvt_ptr->reasonDesc, csmIntSmsEvt_ptr->reasonDesc,
                sizeof(csmSmsEvt_ptr->reasonDesc));
        OSAL_memCpy(csmSmsEvt_ptr->remoteAddress,
                csmIntSmsEvt_ptr->remoteAddress,
                sizeof(csmSmsEvt_ptr->remoteAddress));
    }
}
