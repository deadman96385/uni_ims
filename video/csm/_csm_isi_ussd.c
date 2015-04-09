/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20360 $ $Date: 2013-04-10 13:21:13 -0700 (Wed, 10 Apr 2013) $
 */

#include <csm_event.h>
#include <isi.h>
#include "_csm.h"
#include "_csm_isi_ussd.h"
#include "_csm_ussd.h"
#include "_csm_print.h"
#include "_csm_response.h"

/*
 *  ======== _CSM_isiUssdSend() ========
 *
 *  helper method for sending USSD message via ISI.
 *
 *  RETURN:
 *      None
 */
void _CSM_isiUssdSend(
    CSM_UssdEvt        *ussdEvt_ptr, 
    ISI_UssdType        type, 
    char               *remoteAddress, 
    ISI_Id              serviceId,
    CSM_OutputEvent    *csmOutput_ptr)
{
    ISI_Return ret;
    ISI_Id     ussdId; 

    ret = ISI_sendUssd(&ussdId, serviceId, type,
            remoteAddress, ussdEvt_ptr->message);
    /* Process ISI Errors */
    if (ret != ISI_RETURN_OK) {
        OSAL_logMsg("%s:%d ISI_sendMessage ERROR ret=%s\n",
                __FUNCTION__, __LINE__, CSM_isiPrintReturnString(ret));
        CSM_sendUssdError(CSM_USSD_ERROR_SIP, NULL, csmOutput_ptr);
    }
}

/*
 *  ======== _CSM_isiUssdTypeEventHandler() ========
 *
 *  handler for ISI ussd type events (only USSD in vPort4G)
 *
 *  RETURN:
 *      None
 */
void _CSM_isiUssdTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          messageId,
    ISI_Event       event,
    const char     *desc_ptr)
{
    CSM_InputIsiUssd    *ussdEvt_ptr;
    CSM_PrivateInputEvt *event_ptr = &isiMngr_ptr->csmInputEvent;

    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_USSD;
    ussdEvt_ptr     = &event_ptr->evt.ussd;
    switch (event) {
        case ISI_EVENT_USSD_SEND_OK:
            /* USSD send ok, relay to CSM */
            ussdEvt_ptr->reason = CSM_USSD_REASON_EVT_SENT_USSD;
            ussdEvt_ptr->id = messageId;
            ussdEvt_ptr->serviceId = isiServiceId;
            OSAL_strncpy(ussdEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(ussdEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        case ISI_EVENT_USSD_SEND_FAILED:
            /* USSD send error, relay to CSM */
            ussdEvt_ptr->reason = CMS_USSD_REASON_EVT_SEND_ERROR;
            ussdEvt_ptr->id = messageId;
            ussdEvt_ptr->serviceId = isiServiceId;
            OSAL_strncpy(ussdEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(ussdEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        case ISI_EVENT_USSD_REQUEST:
            /* New USSD message received, relay to CSM */
            ussdEvt_ptr->reason = CSM_USSD_REASON_EVT_REQUEST_USSD;
            ussdEvt_ptr->encType = CSM_USSD_ENCTYPE_UTF8;
            ussdEvt_ptr->id = messageId;
            ussdEvt_ptr->serviceId = isiServiceId;
            OSAL_strncpy(ussdEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(ussdEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        case ISI_EVENT_USSD_DISCONNECT:
            /* Receive a BYE USSD request, relay to CSM */
            ussdEvt_ptr->reason = CSM_USSD_REASON_EVT_DISCONNECT_USSD;
            ussdEvt_ptr->encType = CSM_USSD_ENCTYPE_UTF8;
            ussdEvt_ptr->id = messageId;
            ussdEvt_ptr->serviceId = isiServiceId;
            OSAL_strncpy(ussdEvt_ptr->reasonDesc, desc_ptr,
                    sizeof(ussdEvt_ptr->reasonDesc));
            CSM_isiSendEvent(isiMngr_ptr, event_ptr);
            break;
        default:
            break;
    }
}
