/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20382 $ $Date: 2013-04-11 12:17:02 -0700 (Thu, 11 Apr 2013) $
 *
 */
#include <csm_event.h>
#include <pdu_hlpr.h>
#include <pduconv.h>
#include <utf8_to_utf16.h>
#include "_csm.h"
#include "_csm_isi_ussd.h"
#include "_csm_response.h"

/*
 * ======== _CSM_ussdReset() ========
 *
 * Reset CSM_UssdMngr.
 *
 * Returns: 
 *      Nothing
 */
static void _CSM_ussdReset(CSM_UssdMngr *ussdMngr_ptr)
{
    OSAL_logMsg("%s\n", __FUNCTION__);
    ussdMngr_ptr->state = CSM_USSD_STATE_NONE;
    ussdMngr_ptr->remoteAddress[0] = '\0';
    ussdMngr_ptr->serviceId = 0;
}

/*
 * ======== _CSM_ussdMessageToUtf8() ========
 *
 * Translation encoding type from ASCII or UCS2 to UTF8
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */

static vint _CSM_ussdMessageToUtf8(
    CSM_UssdEvt     *ussdEvt_ptr, 
    CSM_UssdMngr    *ussdMngr_ptr) 
{
    
    int totalBytes;
    /*Convert Hex String into byte array*/
    totalBytes = PDU_pduHexStringToBytes(ussdEvt_ptr->message, 
        (unsigned char*) ussdMngr_ptr->buff);
    ussdMngr_ptr->buff[totalBytes] = '\0';
    
    if (ussdEvt_ptr->encType == CSM_USSD_ENCTYPE_UCS2) {
        //Convert UCS2 to UTF8
        totalBytes = pdu_utf16_to_utf8(
            (unsigned short *)ussdMngr_ptr->buff, OSAL_strlen(ussdEvt_ptr->message)/2,
            (unsigned char  *)ussdEvt_ptr->message, CSM_USSD_STRING_SZ);
        ussdEvt_ptr->message[totalBytes] = '\0';
    }
    else if (ussdEvt_ptr->encType == CSM_USSD_ENCTYPE_ASCII) {
        //Convert ASCII to UTF8
        OSAL_strcpy(ussdEvt_ptr->message, (char*)ussdMngr_ptr->buff);
    }
    else {
        return (CSM_ERR);
    }
    return (CSM_OK);
}


/*
 * ======== CSM_ussdInit() ========
 *
 * Initialization routine for the CSM USSD manager package
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_ussdInit(
    CSM_UssdMngr       *ussdMngr_ptr,
    CSM_IsiMngr        *isiMngr_ptr)
{
    OSAL_logMsg("%s:%d", __FUNCTION__, __LINE__);
    ussdMngr_ptr->isiMngr_ptr = isiMngr_ptr;
    _CSM_ussdReset(ussdMngr_ptr);
    return (CSM_OK);
}

/*
 * ======== CSM_ussdProcessEvent() ========
 *
 * Main entry point into the USSD Package.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_ussdProcessEvent(
    CSM_UssdMngr       *ussdMngr_ptr,
    CSM_UssdEvt        *ussdEvt_ptr,
    CSM_OutputEvent    *csmOutput_ptr)
{
    CSM_IsiService   *service_ptr;
    CSM_OutputUssd   *ussdResp_ptr;
    ISI_UssdType      type;
    vint              maxLen;
    vint              bytes;

    OSAL_logMsg("%s:%d,reason=%d, state=%d\n", __FUNCTION__, __LINE__, 
        ussdEvt_ptr->reason, ussdMngr_ptr->state);
    switch (ussdEvt_ptr->reason) {
        case CSM_USSD_EVT_REASON_AT_CMD_SEND_USSD:
        case CSM_USSD_EVT_REASON_AT_CMD_REPLY_USSD:
        case CSM_USSD_EVT_REASON_AT_CMD_DISCONNECT_USSD:
            //Convert Hex String in ussdEvt_ptr->message to UTF8
            if (_CSM_ussdMessageToUtf8(ussdEvt_ptr, ussdMngr_ptr) != CSM_OK) {
                break;
            }
            if (ussdMngr_ptr->state == CSM_USSD_STATE_NONE) {
                if (ussdEvt_ptr->reason == CSM_USSD_EVT_REASON_AT_CMD_DISCONNECT_USSD) {
                    /* USSD is not in ONGOING state and got a DISCONNECT Ussd event.
                                  * Should reply an error
                                  */
                    break;
                }
                /* Get the service to use and set the serviceId. */
                service_ptr = CSM_isiNormalizeOutboundAddress(
                    ussdMngr_ptr->isiMngr_ptr, ussdEvt_ptr->message,
                    ussdMngr_ptr->remoteAddress, ISI_ADDRESS_STRING_SZ, 
                    RPM_FEATURE_TYPE_CALL_NORMAL);
                if (service_ptr == NULL) {
                    break;
                }

                /*Add user=dialstring to remote address. A requirement of USSD spec*/
                OSAL_strcpy(ussdMngr_ptr->remoteAddress + 
                    OSAL_strlen(ussdMngr_ptr->remoteAddress), ";user=dialstring");
                ussdMngr_ptr->serviceId = service_ptr->serviceId;
                ussdMngr_ptr->state = CSM_USSD_STATE_ACTIVE;
                type = ISI_USSD_TYPE_SEND;
            }
            else if (ussdEvt_ptr->reason == CSM_USSD_EVT_REASON_AT_CMD_DISCONNECT_USSD) {
                type = ISI_USSD_TYPE_DISCONNECT;
            }
            else {
                type = ISI_USSD_TYPE_REPLY;
            }
            OSAL_logMsg("%s, type=%d\n", __FUNCTION__, type);
            _CSM_isiUssdSend(ussdEvt_ptr, type, ussdMngr_ptr->remoteAddress,
                ussdMngr_ptr->serviceId, csmOutput_ptr);
            csmOutput_ptr->type = CSM_EVENT_TYPE_USSD;
            ussdResp_ptr = &csmOutput_ptr->evt.ussd;
            ussdResp_ptr->reason = CSM_OUTPUT_REASON_OK;
            ussdResp_ptr->errorCode = CSM_USSD_ERROR_NONE;
            CSM_sendOutputEvent(csmOutput_ptr);
            return (CSM_OK);
        case CSM_USSD_EVT_REASON_EVT_REQUEST_USSD:
        case CSM_USSD_EVT_REASON_EVT_DISCONNECT_USSD:
            /* 
                     * We've receive a new incoming USSD.  Process it. 
                     */
            service_ptr = CSM_isiGetServiceViaId(ussdMngr_ptr->isiMngr_ptr, 
                    ussdEvt_ptr->serviceId);
            if (NULL == service_ptr) {
                /* Service unknown! just return. */
                break;
            }
            /* Construct response message */
            csmOutput_ptr->type = CSM_EVENT_TYPE_USSD;
            ussdResp_ptr = &csmOutput_ptr->evt.ussd;
            if (ussdEvt_ptr->reason == CSM_USSD_EVT_REASON_EVT_REQUEST_USSD) {
                ussdResp_ptr->reason = CSM_OUTPUT_REASON_USSD_REQUEST_EVENT;
            }
            else {
                ussdResp_ptr->reason = CSM_OUTPUT_REASON_USSD_DISCONNECT_EVENT;
            }
            OSAL_strncpy(ussdResp_ptr->reasonDesc, ussdEvt_ptr->reasonDesc, 
                    CSM_EVENT_STRING_SZ);
            ussdResp_ptr->encType = CSM_USSD_ENCTYPE_UCS2;
            ussdResp_ptr->errorCode = CSM_USSD_ERROR_NONE;

            maxLen = CSM_USSD_STRING_SZ;

            /* Read USSD message from database of Ussds*/
            bytes = maxLen;
            if (ISI_RETURN_OK != ISI_readUssd(ussdEvt_ptr->id, ussdResp_ptr->message, &bytes)) {
                /* Fail to read ussd message*/
                bytes = 0;
            }
            ussdResp_ptr->message[bytes] = '\0';
            OSAL_logMsg("%s:%d, len=%d, recv=%s\n", __FUNCTION__, __LINE__, 
                OSAL_strlen(ussdResp_ptr->message), ussdResp_ptr->message);

            /* Convert USSD Message to UCS2 format*/
            bytes = utf8_to_utf16((unsigned short*)ussdMngr_ptr->buff, 
                ussdResp_ptr->message, bytes, 1);
            ussdMngr_ptr->buff[bytes*2] = '\0';

            /* Convert USSD message to Hex string formt*/
            bytes = PDU_pduBytesToHexString((unsigned char *)ussdMngr_ptr->buff, 
                bytes*2, ussdResp_ptr->message);
            ussdResp_ptr->message[bytes] = '\0';

            /* Send event out*/
            CSM_sendOutputEvent(csmOutput_ptr);
            if (ussdEvt_ptr->reason == CSM_USSD_EVT_REASON_EVT_DISCONNECT_USSD){
                _CSM_ussdReset(ussdMngr_ptr);
            }
            return (CSM_OK);
        case CSM_USSD_EVT_REASON_EVT_NOTIFY_USSD:
            return (CSM_OK);
        case CSM_USSD_EVT_REASON_EVT_SENT_USSD:
            OSAL_logMsg("USSD sent. No need to report again\n");
            return (CSM_OK);
        case CSM_USSD_EVT_REASON_EVT_SEND_ERROR:
            CSM_sendUssdError(CSM_USSD_ERROR_SIP, ussdEvt_ptr->reasonDesc, csmOutput_ptr);
            _CSM_ussdReset(ussdMngr_ptr);
            return (CSM_OK);
        default:
            break;
    }
    CSM_sendUssdError(CSM_USSD_ERROR_SIP, ussdEvt_ptr->reasonDesc, csmOutput_ptr);
    _CSM_ussdReset(ussdMngr_ptr);
    return (CSM_ERR);
}

/*
 * CSM_ussdShutdown()
 */
vint CSM_ussdShutdown(
    CSM_UssdMngr *ussdMngr_ptr)
{
    OSAL_logMsg("%s:%d", __FUNCTION__, __LINE__);

    return (CSM_OK);
}

/*
 * ======== CSM_callsConvertToInternalEvt() ========
 *
 * This function is used to convert CSM input event to CSM call internal event.
 *
 * Returns:
 *
 */
void CSM_ussdConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputUssdEvt_ptr,
    CSM_UssdEvt        *csmUssdEvt_ptr)
{
    CSM_InputUssd       *csmExtUssdEvt_ptr;
    CSM_InputIsiUssd    *csmIntUssdEvt_ptr;
    CSM_UssdEvtReason    reason; 

    OSAL_memSet(csmUssdEvt_ptr, 0, sizeof(CSM_UssdEvt));

    if (CSM_INPUT_EVT_TYPE_EXT == type) {
        csmExtUssdEvt_ptr = (CSM_InputUssd *) inputUssdEvt_ptr;
        switch (csmExtUssdEvt_ptr->reason) {
            case CSM_USSD_REASON_AT_CMD_SEND_USSD:
                reason = CSM_USSD_EVT_REASON_AT_CMD_SEND_USSD;
                break;
            case CSM_USSD_REASON_AT_CMD_REPLY_USSD:
                reason = CSM_USSD_EVT_REASON_AT_CMD_REPLY_USSD;
                break;
            case CSM_USSD_REASON_AT_CMD_DISCONNECT_USSD:
                reason = CSM_USSD_EVT_REASON_AT_CMD_DISCONNECT_USSD;
                break;
            default:
                reason = CSM_USSD_EVT_REASON_INVALID;
                OSAL_logMsg("Invalid CSM ussd reason: %d\n",
                        csmExtUssdEvt_ptr->reason);
        }
        csmUssdEvt_ptr->reason  = reason;
        csmUssdEvt_ptr->encType = csmExtUssdEvt_ptr->encType;
        OSAL_memCpy(csmUssdEvt_ptr->reasonDesc, csmExtUssdEvt_ptr->reasonDesc,
                sizeof(csmUssdEvt_ptr->reasonDesc));
        OSAL_memCpy(csmUssdEvt_ptr->message, csmExtUssdEvt_ptr->message,
                sizeof(csmUssdEvt_ptr->message));
    }
    else if (CSM_INPUT_EVT_TYPE_INT == type) {
        csmIntUssdEvt_ptr = (CSM_InputIsiUssd *) inputUssdEvt_ptr;
        switch (csmIntUssdEvt_ptr->reason) {
            case CMS_USSD_REASON_EVT_SEND_ERROR:
                reason = CSM_USSD_EVT_REASON_EVT_SEND_ERROR;
                break;
            case CSM_USSD_REASON_EVT_SENT_USSD:
                reason = CSM_USSD_EVT_REASON_EVT_SENT_USSD;
                break;
            case CSM_USSD_REASON_EVT_REQUEST_USSD:
                reason = CSM_USSD_EVT_REASON_EVT_REQUEST_USSD;
                break;
            case CSM_USSD_REASON_EVT_NOTIFY_USSD:
                reason = CSM_USSD_EVT_REASON_EVT_NOTIFY_USSD;
                break;
            case CSM_USSD_REASON_EVT_DISCONNECT_USSD:
                reason = CSM_USSD_EVT_REASON_EVT_DISCONNECT_USSD;
                break;
            default:
                reason = CSM_USSD_EVT_REASON_INVALID;
                OSAL_logMsg("Invalid CSM ussd reason: %d\n",
                        csmIntUssdEvt_ptr->reason);
        }
        csmUssdEvt_ptr->reason    = reason;
        csmUssdEvt_ptr->id        = csmIntUssdEvt_ptr->id;
        csmUssdEvt_ptr->serviceId = csmIntUssdEvt_ptr->serviceId;
        csmUssdEvt_ptr->encType   = csmIntUssdEvt_ptr->encType;
        OSAL_memCpy(csmUssdEvt_ptr->reasonDesc, csmIntUssdEvt_ptr->reasonDesc,
                sizeof(csmUssdEvt_ptr->reasonDesc));
    }
}

