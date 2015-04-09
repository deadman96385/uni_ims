/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29738 $ $Date: 2014-11-10 14:31:33 +0800 (Mon, 10 Nov 2014) $
 */

#include <csm_event.h>
#include <isi.h>
#include <osal.h>
#include "_csm.h"
#include "_csm_isi.h"
#include "_csm_isi_tel.h"

/*
 *  ======== _CSM_isiTelTypeEventHandler() ========
 *
 *  handler for ISI "tel" type events
 */
void _CSM_isiTelTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          messageId,
    ISI_Event       event,
    const char     *desc_ptr)
{
    CSM_PrivateInputEvt    *event_ptr = &isiMngr_ptr->csmInputEvent;
    CSM_InputIsiCall       *callEvt_ptr;
    CSM_InputIsiSms        *smsEvt_ptr;

    ISI_Id       callId;
    vint         arg0;
    vint         arg1;
    ISI_TelEvent telEvt;
    char         from[ISI_ADDRESS_STRING_SZ + 1];
    char         dateTime[ISI_DATE_TIME_STRING_SZ + 1];

    CSM_dbgPrintf("\n");

    /* Switch on the TEL event type */
    switch (event) {
        case ISI_EVENT_TEL_EVENT_SEND_OK:
        case ISI_EVENT_TEL_EVENT_SEND_FAILED:
            /* Send ok/fail.  Just relay it to CSM */
            event_ptr->type        = CSM_PRIVATE_EVENT_TYPE_CALL;
            callEvt_ptr            = &event_ptr->evt.call;
            callEvt_ptr->reason    = CSM_CALL_REASON_EVT_DIGIT_DONE;
            callEvt_ptr->type      = CSM_CALL_EVENT_TYPE_ISI;
            callEvt_ptr->id        = messageId;
            callEvt_ptr->serviceId = isiServiceId;
            OSAL_strncpy(callEvt_ptr->reasonDesc, desc_ptr, 
                    sizeof(callEvt_ptr->reasonDesc));
            break;
        case ISI_EVENT_TEL_EVENT_RECEIVED:
            /* Tel event received.  Get the details. */
            ISI_getTelEventFromRemote(messageId, &callId, &telEvt, 
                    &arg0, &arg1, from, dateTime);
            if (ISI_TEL_EVENT_VOICEMAIL == telEvt) {
                event_ptr->type       = CSM_PRIVATE_EVENT_TYPE_SMS;
                smsEvt_ptr            = &event_ptr->evt.sms;
                smsEvt_ptr->type      = CSM_SMS_TYPE_TEXT;
                smsEvt_ptr->serviceId = isiServiceId;
                smsEvt_ptr->id        = messageId;
                if (0 == arg0) {
                    smsEvt_ptr->reason = CSM_SMS_REASON_EVT_MWI_INACTIVE;
                }
                else {
                    smsEvt_ptr->reason = CSM_SMS_REASON_EVT_MWI_ACTIVE;
                }
                OSAL_strncpy(smsEvt_ptr->reasonDesc, desc_ptr, 
                        sizeof(smsEvt_ptr->reasonDesc));
                OSAL_strncpy(smsEvt_ptr->remoteAddress, from, 
                        sizeof(smsEvt_ptr->remoteAddress));
            }
            else if (ISI_TEL_EVENT_DTMF_DETECT == telEvt) {
                event_ptr->type         = CSM_PRIVATE_EVENT_TYPE_CALL;
                callEvt_ptr             = &event_ptr->evt.call;
                if (ISI_DTMFDECT_TYPE_LEADING == arg1) {
                    callEvt_ptr->reason =
                            CSM_CALL_REASON_EVT_DTMFDECT_LEADING;
                }
                else {
                    callEvt_ptr->reason =
                            CSM_CALL_REASON_EVT_DTMFDECT_TRAILING;
                }
                callEvt_ptr->u.digit    = arg0;
                callEvt_ptr->type       = CSM_CALL_EVENT_TYPE_ISI;
                callEvt_ptr->id         = messageId;
                callEvt_ptr->serviceId  = isiServiceId;
                OSAL_strncpy(callEvt_ptr->reasonDesc, desc_ptr, 
                        sizeof(callEvt_ptr->reasonDesc));
            }
            else if (ISI_TEL_EVENT_DTMF_OOB == telEvt) {
                /* Get SIP INFO */
                /* Contruct CSM internal input event */
                event_ptr->type            = CSM_PRIVATE_EVENT_TYPE_CALL;
                callEvt_ptr                = &event_ptr->evt.call;
                callEvt_ptr->reason        = CSM_CALL_REASON_EVT_DTMFDECT_OOB;
                callEvt_ptr->extraArgument = arg1; /* Duration */
                callEvt_ptr->type          = CSM_CALL_EVENT_TYPE_ISI;
                callEvt_ptr->id            = messageId;
                callEvt_ptr->serviceId     = isiServiceId;
                OSAL_strncpy(callEvt_ptr->reasonDesc, desc_ptr, 
                        sizeof(callEvt_ptr->reasonDesc));
                /* Convert ASCII to digit number */
                switch (arg0) {
                    case '*':
                        callEvt_ptr->u.digit = 10;
                        break;
                    case '#':
                        callEvt_ptr->u.digit = 11;
                        break;
                    case 'A':
                    case 'a':
                        callEvt_ptr->u.digit = 12;
                        break;
                    case 'B':
                    case 'b':
                        callEvt_ptr->u.digit = 13;
                        break;
                    case 'C':
                    case 'c':
                        callEvt_ptr->u.digit = 14;
                        break;
                    case 'D':
                    case 'd':
                        callEvt_ptr->u.digit = 15;
                        break;
                    default:
                        if (48 <= arg0) {
                            callEvt_ptr->u.digit = (arg0 - 48);
                        }
                        else {
                            callEvt_ptr->u.digit = arg0;
                        }
                        break;
                }
            }
            else {
                OSAL_logMsg("%s:%d Unknown Tel event received", __FUNCTION__,
                        __LINE__);
            }
            break;
        default:
            OSAL_logMsg("%s:%d unknown TEL event type:%d", __FUNCTION__,
                    __LINE__, event);
            return;
    };
    /* Notify Account package */
    CSM_isiSendEvent(isiMngr_ptr, event_ptr);
}

