/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29738 $ $Date: 2014-11-10 14:31:33 +0800 (Mon, 10 Nov 2014) $
 */

#include <csm_event.h>
#include <isi.h>
#include "_csm.h"
#include "_csm_isi_sms.h"
#include "_csm_sms.h"
#include "_csm_print.h"
#include "_csm_response.h"

/*
 *  ======== _CSM_isiSmsSend() ========
 *
 *  helper method for sending SMS message via ISI.
 *
 *  RETURN:
 *      None
 */
void _CSM_isiSmsSend(
    CSM_SmsEvt      *smsEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    ISI_Return      ret;
    ISI_Id          smsId; 

    CSM_dbgPrintf("\n");

    switch (smsEvt_ptr->type) {
        /* Handle PDU type messages */
        case CSM_SMS_TYPE_PDU_3GPP:
            ret = ISI_sendMessage(&smsId, smsEvt_ptr->serviceId, 
                    ISI_MSG_TYPE_PDU_3GPP, smsEvt_ptr->remoteAddress, "", 
                    smsEvt_ptr->pdu, smsEvt_ptr->msgLen, ISI_MSG_RPT_NONE, "");
            break;
        case CSM_SMS_TYPE_PDU_3GPP2:
            ret = ISI_sendMessage(&smsId, smsEvt_ptr->serviceId, 
                    ISI_MSG_TYPE_PDU_3GPP2, smsEvt_ptr->remoteAddress, "", 
                    smsEvt_ptr->pdu, smsEvt_ptr->msgLen, ISI_MSG_RPT_NONE, "");
            break;
        /* Handle TEXT type messages */
        case CSM_SMS_TYPE_TEXT:
        default:
            CSM_dbgPrintf("Sending:%s", smsEvt_ptr->message);
            ret = ISI_sendMessage(&smsId, smsEvt_ptr->serviceId, 
                    ISI_MSG_TYPE_TEXT, smsEvt_ptr->remoteAddress, "",
                    smsEvt_ptr->message, OSAL_strlen(smsEvt_ptr->message),
                    ISI_MSG_RPT_NONE, "");
            break;
    }
    /* Process ISI Errors */
    if (ret != ISI_RETURN_OK) {
        OSAL_logMsg("%s:%d ISI_sendMessage ERROR ret=%s",
                __FUNCTION__, __LINE__, CSM_isiPrintReturnString(ret));
        CSM_sendSmsError(331, NULL, csmOutput_ptr);
    }
}

/*
 *  ======== _CSM_isiSmsTypeEventHandler() ========
 *
 *  handler for ISI "message" type events (only SMS in vPort4G)
 *
 *  RETURN:
 *      None
 */
void _CSM_isiSmsTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          messageId,
    ISI_Event       event,
    const char     *desc_ptr)
{
    ISI_Return              ret;
    char                    remoteAddress[ISI_ADDRESS_STRING_SZ + 1];
    char                    dateTime[ISI_DATE_TIME_STRING_SZ + 1];
    char                    reportId[ISI_ID_STRING_SZ + 1];
    ISI_MessageReport       reports;
    ISI_Id                  chatId;
    CSM_InputIsiSms        *smsEvt_ptr;
    CSM_PrivateInputEvt    *event_ptr = &isiMngr_ptr->csmInputEvent;

    CSM_dbgPrintf("\n");

    switch (event) {
        case ISI_EVENT_MESSAGE_SEND_OK:
            /* SMS send ok, relay to CSM */
            event_ptr->type = CSM_PRIVATE_EVENT_TYPE_SMS;
            smsEvt_ptr = &event_ptr->evt.sms;
            smsEvt_ptr->reason = CSM_SMS_REASON_EVT_SEND_OK;
            smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP;
            smsEvt_ptr->serviceId = isiServiceId;
            smsEvt_ptr->id = messageId;
            OSAL_strncpy(smsEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(smsEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        case ISI_EVENT_MESSAGE_SEND_FAILED:
            /* SMS send error, relay to CSM */
            event_ptr->type = CSM_PRIVATE_EVENT_TYPE_SMS;
            smsEvt_ptr = &event_ptr->evt.sms;
            smsEvt_ptr->reason = CSM_SMS_REASON_EVT_SEND_ERROR;
            smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP;
            smsEvt_ptr->serviceId = isiServiceId;
            smsEvt_ptr->id = messageId;
            OSAL_strncpy(smsEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(smsEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        case ISI_EVENT_MESSAGE_RECEIVED:
            /* New SMS message received, relay to CSM */
            event_ptr->type = CSM_PRIVATE_EVENT_TYPE_SMS;
            smsEvt_ptr = &event_ptr->evt.sms;
            smsEvt_ptr->reason = CSM_SMS_REASON_EVT_NEW_INCOMING;
            smsEvt_ptr->type = CSM_SMS_TYPE_PDU_3GPP;
            smsEvt_ptr->serviceId = isiServiceId;
            smsEvt_ptr->id = messageId;
            OSAL_strncpy(smsEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(smsEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        case ISI_EVENT_MESSAGE_REPORT_RECEIVED:
            /* New Message (delivery) report received */
            ret = ISI_readMessageReport(messageId, &chatId, remoteAddress, 
                    dateTime, &reports, reportId);
            if (ISI_RETURN_OK != ret) {
                OSAL_logMsg("%s:%d ERROR in ISI_getMessageHeader ret=%s", 
                        __FUNCTION__, __LINE__, CSM_isiPrintReturnString(ret));
                break;
            }
            /*
             * Currently IR.92 SMS uses RP (PDU) data in a incoming SMS
             * to traffic message reports. These are handled by
             * ISI_EVENT_MESSAGE_RECEIVED event processing so there is
             * nothing further that needs to be done with
             * ISI_EVENT_MESSAGE_REPORT_RECEIVED events.  In the future
             * if CPIM formated SMS over IP messages are required then
             * this event will need to be processed.
             */
            break;
        default:
            break;
    }
}
