/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29964 $ $Date: 2014-11-20 13:26:23 +0800 (Thu, 20 Nov 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isi_errors.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_dialog.h"
#include "_sapp_im_page.h"
#include "_sapp_cpim_page.h"
#include "_sapp_mwi.h"
#include "_sapp_te.h"
#include "_sapp_ipsec.h"

#ifdef INCLUDE_SIMPLE
#include "_simple.h"
#include "simple/_simple_types.h"
#include "simple/_simple_broadcast.h"
#endif

static const char SAPP_IMDN_ACCEPT_CONTACT[] = "*;+g.oma.sip-im";

/* 
 * ======== _SAPP_imIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for "im" (instant messaging) related events. These events will be passed 
 * from SAPP to the ISI module. 
 *
 * Returns: 
 *   Nothing.
 */  
static void _SAPP_imIsiEvt(
    ISI_Id              textId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              chatId,
    ISIP_TextReason     reason,
    char               *to_ptr,
    char               *from_ptr,
    char               *msg_ptr,
    vint                msgLen,
    char               *datetime_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = textId;
    isi_ptr->code = ISIP_CODE_MESSAGE;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.message.reason = reason;
    isi_ptr->msg.message.serviceId = serviceId;
    /* We populate the callID.  However in most cases it will be '0' */
    isi_ptr->msg.message.chatId = chatId;

    if (to_ptr) {
        OSAL_snprintf(isi_ptr->msg.message.to, ISI_ADDRESS_STRING_SZ, "%s",
                to_ptr);
    }

    if (from_ptr) {
        OSAL_snprintf(isi_ptr->msg.message.from, ISI_ADDRESS_STRING_SZ, 
                "%s", from_ptr);
    }

    if (datetime_ptr) {
        OSAL_snprintf(isi_ptr->msg.message.dateTime, ISI_DATE_TIME_STRING_SZ,
                "%s", datetime_ptr);
    }

    if (msg_ptr && msgLen != 0) {
        /* 
         * Copy the message.  In SIP it may be a string or data, 
         * so there's no guarantee that pData is NULL terminated.
         */
        
        /* Copy as much as you can, but don't overrun any buffers */
        if (msgLen >= ISI_TEXT_STRING_SZ) {
            msgLen = (ISI_TEXT_STRING_SZ - 1);
        }
        OSAL_memCpy(isi_ptr->msg.message.message, msg_ptr, msgLen);
        /* Just for safety... NULL terminate */
        isi_ptr->msg.message.message[msgLen] = 0;
    }
    return;
}


/* 
 * ======== _SAPP_imPage() ========
 * 
 * This function will send a instant message in "page" mode via the SIP MESSAGE
 * request.
 *
 * Returns: 
 *   SAPP_OK: The IM was successfully sent.
 *   SAPP_ERR: Could not successfully send the IM.
 */
static vint _SAPP_imPage(
    SAPP_ServiceObj *service_ptr,
    char            *from_ptr,
    ISIP_Text       *text_ptr,
    ISI_Id           id,
    tSipHandle       dialogId)
{
    tUaMsgBody       body;
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds = 0;
    SAPP_ImObj      *im_ptr = &service_ptr->im;
    char            *payload_ptr;
    vint             payloadLen;
        
    /* 
     * Add a 'preconfigured route' if the registrar reported one during the 
     * registration process
     */
    if (0 == dialogId) {
        if (SAPP_OK == SAPP_sipAddPreconfiguredRoute(&service_ptr->registration,
                &service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ)) {
            /* Then a "Route" header field was added */
            hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
            numHdrFlds++;
        }
    }

    /* Set up the "To" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0], 
        SAPP_STRING_SZ, "%s %s", SAPP_TO_HF, text_ptr->to);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Set up the "From" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0], 
        SAPP_STRING_SZ, "%s %s", SAPP_FROM_HF, from_ptr);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Add P-Preferred-Identity header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF, from_ptr);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    
    /* Set up the "Accept-Contact" header field */
    /* Abdi server don't like '+g.oma.sip-im'. Bug 8124 */
    if (NULL == OSAL_strncasescan(service_ptr->sipConfig.config.szRegistrarProxy,
            SIP_URI_STRING_MAX_SIZE, "foundry.att")) {
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_ACCEPT_CONTACT_HF, SAPP_IMDN_ACCEPT_CONTACT);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    if (ISI_MSG_TYPE_PDU_3GPP == text_ptr->type) {
        /* Set up the "Request-Disposition" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_REQUEST_DISPOSITION_HF, SAPP_NO_FORK_ARG);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Set up the "Content-Type" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_CONTENT_TYPE_HF, SAPP_CONTENT_TYPE_3GPP_SMS);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Prepare the SIP object for sending IM */
        body.pBody = text_ptr->message;
        body.length = text_ptr->pduLen;
    }
    else if (ISI_MSG_TYPE_PDU_3GPP2 == text_ptr->type) {
        /* Set up the "Request-Disposition" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_REQUEST_DISPOSITION_HF, SAPP_NO_FORK_ARG);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Set up the "Content-Type" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_CONTENT_TYPE_HF, SAPP_CONTENT_TYPE_3GPP2_SMS);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Set up the "Content-Transfer-Encoding" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "%s %s", SAPP_CONTENT_TRANSFER_ENCODING_HF,
                SAPP_BINARY_ARG);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
        /* Prepare the SIP object for sending IM */
        body.pBody = text_ptr->message;
        body.length = text_ptr->pduLen;
    }
    else if (0 != service_ptr->sipConfig.useCpim) {
        /* Set up the "Content-Type" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_CONTENT_TYPE_HF, SAPP_CONTENT_TYPE_CPIM);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Prepare the SIP object for sending IM */
        payload_ptr = service_ptr->payloadStratch;
        payloadLen = SAPP_PAYLOAD_SZ;
        if (SAPP_OK != SAPP_cpimEncodeIm(text_ptr->to, from_ptr,
                text_ptr->report, text_ptr->messageId, text_ptr->message,
                &payload_ptr, &payloadLen)) {
            /* Not enough room */
            return (SAPP_ERR);
        }
        body.length = payload_ptr - service_ptr->payloadStratch;
        body.pBody = service_ptr->payloadStratch;
    }
    else {
        /* Set up the "Content-Type:" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_CONTENT_TYPE_HF, SAPP_IM_CONTENT_TYPE);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
        /* Prepare the SIP object for sending IM */
        body.pBody = text_ptr->message;
        body.length = OSAL_strlen(text_ptr->message);
    }

    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }
    /* cache the ID from ISI */
    im_ptr->id = id;
    
    if (UA_Message(service_ptr->sipConfig.uaId, dialogId, 
            &im_ptr->hTransaction, hdrFlds_ptr, numHdrFlds, 
            &body, 0, &service_ptr->sipConfig.localConn) != SIP_OK) {
        /* Can't send it, tell ISI about the failure */
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static vint _SAPP_imNotification(
    SAPP_ServiceObj *service_ptr,
    char            *from_ptr,
    ISIP_Text       *text_ptr,
    ISI_Id           id,
    tSipHandle       dialogId)
{
    tUaMsgBody       body;
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds = 0;
    SAPP_ImObj      *im_ptr = &service_ptr->im;
    vint             bytes;

    /*
     * Add a 'preconfigured route' if the registrar reported one during the
     * registration process
     */
    if (0 == dialogId) {
        if (SAPP_OK == SAPP_sipAddPreconfiguredRoute(&service_ptr->registration,
                &service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ)) {
            /* Then a "Route" header field was added */
            hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
            numHdrFlds++;
        }
    }

    /* Set up the "To" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
        SAPP_STRING_SZ, "%s %s", SAPP_TO_HF, text_ptr->to);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Set up the "From" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
        SAPP_STRING_SZ, "%s %s", SAPP_FROM_HF, from_ptr);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Add P-Preferred-Identity header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF, from_ptr);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Set up the "Accept-Contact" header field */
    /* Abdi server don't like '+g.oma.sip-im'. Bug 8124 */
    if (NULL == OSAL_strncasescan(service_ptr->sipConfig.config.szRegistrarProxy,
            SIP_URI_STRING_MAX_SIZE, "foundry.att")) {
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s %s", SAPP_ACCEPT_CONTACT_HF, SAPP_IMDN_ACCEPT_CONTACT);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }
    
    /* Set up the "Content-Type" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
        SAPP_STRING_SZ, "%s %s", SAPP_CONTENT_TYPE_HF, SAPP_CONTENT_TYPE_CPIM);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }

    /* Prepare the SIP object for sending IM */
    if (0 == (bytes = SAPP_cpimEncodeImNotification(text_ptr->to, from_ptr,
            text_ptr->report, text_ptr->messageId,
            service_ptr->payloadStratch, SAPP_PAYLOAD_SZ))) {
        return (SAPP_ERR);
    }

    body.pBody = service_ptr->payloadStratch;
    body.length = bytes;

    /* cache the ID from ISI */
    im_ptr->id = id;

    if (UA_Message(service_ptr->sipConfig.uaId, dialogId,
            &im_ptr->hTransaction, hdrFlds_ptr, numHdrFlds,
            &body, 0, &service_ptr->sipConfig.localConn) != SIP_OK) {
        /* Can't send it, tell ISI about the failure */
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static void _SAPP_imGetReasonDesc(
    tUaAppEvent *uaEvt_ptr,
    ISIP_Text   *isi_ptr)
{
    char *reason_ptr = NULL;
    vint  respCode = ISI_NO_NETWORK_AVAILABLE;
    
    if (NULL != uaEvt_ptr) {
        /* Let's see if there's a reason phrase */
        reason_ptr = SAPP_parseHfValue(SAPP_REASON_HF, uaEvt_ptr);
        respCode = uaEvt_ptr->resp.respCode;
    }

    if (ISI_NO_NETWORK_AVAILABLE == respCode) {
        reason_ptr = ISI_NO_NETWORK_AVAILABLE_STR;
    }
    else if (NULL == reason_ptr) {
        reason_ptr = (char*)SAPP_GetResponseReason(respCode);
    }

    OSAL_snprintf(isi_ptr->reasonDesc, ISI_EVENT_DESC_STRING_SZ,
            "SMS FAILED: CODE:%d REASON:%s", respCode, reason_ptr);
    return;
}

static char* _SAPP_imGetPayload(
    char *payload_ptr,
    vint  payloadLen)
{
    char *pos_ptr;
    if (NULL == (pos_ptr = OSAL_strnscan(payload_ptr, payloadLen, "\r\n\r\n"))) {
        return NULL;
    }
    pos_ptr += 4;
    return pos_ptr;
}

static vint _SAPP_imParseTextPlain(
    SAPP_ServiceObj   *service_ptr,
    char              *to_ptr,
    char              *from_ptr,
    char              *dateTime_ptr,
    char              *contentType_ptr,
    vint               contentTypeLen,
    char              *payload_ptr,
    vint               payloadLen,
    ISIP_Message      *isi_ptr)
{
    if (SAPP_CONTENT_TEXT_PLAIN != SAPP_parseIntFileType(contentType_ptr,
            contentTypeLen)) {
        return (SAPP_ERR);
    }
    _SAPP_imIsiEvt(0, service_ptr->isiServiceId, service_ptr->protocolId,
            0, ISIP_TEXT_REASON_NEW, to_ptr, from_ptr, payload_ptr,
            payloadLen, dateTime_ptr, isi_ptr);
    return (SAPP_OK);
}

static vint _SAPP_imParseCpim(
    SAPP_ServiceObj   *service_ptr,
    char              *contentType_ptr,
    vint               contentTypeLen,
    char              *payload_ptr,
    vint               payloadLen,
    SAPP_Event        *evt_ptr)
{
    if (SAPP_CONTENT_MSG_CPIM != SAPP_parseIntFileType(contentType_ptr,
            contentTypeLen)) {
        return (SAPP_ERR);
    }
    SAPP_cpimDecode(service_ptr, payload_ptr, payloadLen, evt_ptr);
    return (SAPP_OK);
}

static vint _SAPP_imParseSipMessage(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    vint             contentTypeLen,
    char            *payload_ptr,
    vint             payloadLen,
    SAPP_Event      *evt_ptr)
{
    char *ct_ptr;
    vint ctLen;
    char *cd_ptr;
    vint cdLen;
    char *from_ptr;
    vint fromLen;
    char *cl_ptr;
    vint len;
    char *p_ptr;
    
    if (0 != OSAL_strncasecmp(contentType_ptr, "message/sip", contentTypeLen)) {
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parsePayloadValue(payload_ptr, SAPP_CONTENT_TYPE_HF,
            &ct_ptr, &ctLen)) {
        /*  This is mandatory */
        return (SAPP_ERR);
    }

    if (SAPP_OK != SAPP_parsePayloadValue(payload_ptr, SAPP_CONTENT_LENGTH_HF,
            &cl_ptr, &len)) {
        /*  This is mandatory */
        return (SAPP_ERR);
    }
    if (0 >= (len = OSAL_atoi(cl_ptr))) {
        /*  This is mandatory */
        return (SAPP_ERR);
    }

    if (NULL == (p_ptr = _SAPP_imGetPayload(payload_ptr, payloadLen))) {
        /*  This is mandatory */
        return (SAPP_ERR);
    }

    /* Content disposition and from are optional */
    cd_ptr = NULL;
    cdLen = 0;
    from_ptr = NULL;
    fromLen = 0;
    SAPP_parsePayloadValue(payload_ptr, SAPP_CONTENT_DISP_HF, &cd_ptr, &cdLen);
    /* Get the remote party */
    if (SAPP_OK != SAPP_parsePayloadValue(payload_ptr, SAPP_P_ASSERTED_ID_HF,
            &from_ptr, &fromLen)) {
        /* Then let's get the 'from' field */
        SAPP_parsePayloadValue(payload_ptr, SAPP_FROM_HF,  &from_ptr, &fromLen);
    }

    if (SAPP_OK == _SAPP_imParseCpim(service_ptr, ct_ptr, ctLen,
            p_ptr, len, evt_ptr)) {
        return (SAPP_OK);
    }
    if (SAPP_OK == SAPP_mwiParseTextMessage(service_ptr, ct_ptr,
            p_ptr, len, &evt_ptr->isiMsg)) {
        return (SAPP_OK);
    }
    /* ADD MORE HERE IF NEED BE */

    // Watch out dateTime will not be NULL terminated here */
    //else if (SAPP_OK == _SAPP_imParseTextPlain(service_ptr, to_ptr, from_ptr,
    //        dateTime_ptr, contentType_ptr, contentTypeLen, p_ptr, len,
    //        &evt_ptr->isi_ptr)) {
    //    return (SAPP_OK);
    //}
    return (SAPP_ERR);
}

/* 
 * ======== _SAPP_imParse3gpp() ========
 * 
 * Private function to parse 3GPP SMS.
 * request.
 *
 * Returns: 
 *   SAPP_OK: The IM was successfully parsed as 3GPP SMS.
 *   SAPP_ERR: The IM is not 3GPP SMS.
 */
static vint _SAPP_imParse3gpp(
    SAPP_ServiceObj   *service_ptr,
    char              *to_ptr,
    char              *from_ptr,
    char              *dateTime_ptr,
    char              *contentType_ptr,
    vint               contentTypeLen,
    char              *payload_ptr,
    vint               payloadLen,
    ISIP_Message      *isi_ptr)
{
    if (SAPP_CONTENT_SMS_3GPP != SAPP_parseIntFileType(contentType_ptr,
            contentTypeLen)) {
        return (SAPP_ERR);
    }

    _SAPP_imIsiEvt(0, service_ptr->isiServiceId, service_ptr->protocolId,
            0, ISIP_TEXT_REASON_NEW, to_ptr, from_ptr, payload_ptr,
            payloadLen, dateTime_ptr, isi_ptr);

    isi_ptr->msg.message.type = ISI_MSG_TYPE_PDU_3GPP;
    isi_ptr->msg.message.pduLen = payloadLen;

    return (SAPP_OK);
}

/* 
 * ======== _SAPP_imParse3gpp2() ========
 * 
 * Private function to parse 3GPP2 SMS.
 * request.
 *
 * Returns: 
 *   SAPP_OK: The IM was successfully parsed as 3GPP2 SMS.
 *   SAPP_ERR: The IM is not 3GPP2 SMS.
 */
static vint _SAPP_imParse3gpp2(
    SAPP_ServiceObj   *service_ptr,
    char              *to_ptr,
    char              *from_ptr,
    char              *dateTime_ptr,
    char              *contentType_ptr,
    vint               contentTypeLen,
    char              *payload_ptr,
    vint               payloadLen,
    ISIP_Message      *isi_ptr)
{
    if (SAPP_CONTENT_SMS_3GPP2 != SAPP_parseIntFileType(contentType_ptr,
            contentTypeLen)) {
        return (SAPP_ERR);
    }

    _SAPP_imIsiEvt(0, service_ptr->isiServiceId, service_ptr->protocolId,
            0, ISIP_TEXT_REASON_NEW, to_ptr, from_ptr, payload_ptr,
            payloadLen, dateTime_ptr, isi_ptr);

    isi_ptr->msg.message.type = ISI_MSG_TYPE_PDU_3GPP2;
    isi_ptr->msg.message.pduLen = payloadLen;

    return (SAPP_OK);
}

static void _SAPP_imMulipartMixedDecode(
    SAPP_ServiceObj   *service_ptr,
    char              *to_ptr,
    char              *from_ptr,
    char              *dateTime_ptr,
    char              *contentType_ptr,
    char              *payload_ptr,
    vint               payloadLen,
    SAPP_Event        *evt_ptr)
{
    char         boundary_ary[SAPP_STRING_SZ];
    char        *start_ptr;
    char        *end_ptr;
    char        *hf_ptr;
    vint         size;
    vint         hfLen;
    vint         len;
    vint         skipBytes;
    OSAL_Boolean notDone;

    /* Get the 'boundary=' value in the content-type */
    if (NULL == (start_ptr = OSAL_strnscan(contentType_ptr,
            SAPP_STRING_SZ, "boundary="))) {
        return;
    }
    /* Advance past boundary=" */
    start_ptr += 10;
    /* Find Trailing quote \" */
    if (NULL == (end_ptr = OSAL_strnscan(start_ptr,
            SAPP_STRING_SZ, "\""))) {
        return;
    }
    /* Find boundary length and copy to scratch */
    len = end_ptr - start_ptr;
    SAPP_parseCopy(boundary_ary, sizeof(boundary_ary), start_ptr, len);

    /* Find all Content-Types and process */
    while (SAPP_OK == SAPP_parsePayloadValue(payload_ptr,
            SAPP_CONTENT_TYPE_HF, &hf_ptr, &hfLen)) {

        /* Get the start and stop of the part of the payload to process */
        if (NULL == (start_ptr = OSAL_strnscan(payload_ptr, payloadLen,
                SAPP_END_OF_DOC))) {
            break;
        }
        /* Advance off the \r\n\r\n */
        start_ptr += 4;
         /* update the payload size */
        payloadLen -= (start_ptr - payload_ptr);

        notDone = OSAL_TRUE;
        skipBytes = 0;

        while (OSAL_TRUE == notDone) {
            if (NULL == (end_ptr = OSAL_strnscan(start_ptr + skipBytes,
                    payloadLen - skipBytes, boundary_ary))) {
                /* Set the payload pointer to the end */
                payload_ptr = (start_ptr + payloadLen);
                size = payloadLen;
                payloadLen = 0;
                notDone = OSAL_FALSE;
            }
            else if (0 == OSAL_strncmp(end_ptr - 4,"\r\n--",4)) {
                payload_ptr = end_ptr + len;
                size = (end_ptr - start_ptr) - 4;
                payloadLen -= (payload_ptr - start_ptr);
                notDone = OSAL_FALSE;
            }
            else if (0 == OSAL_strncmp(end_ptr - 2,"\r\n",2)) {
                payload_ptr = end_ptr + len;
                size = (end_ptr - start_ptr) - 2;
                payloadLen -= (payload_ptr - start_ptr);
                notDone = OSAL_FALSE;
            }
            else {
                skipBytes = (end_ptr - start_ptr) + len;
                notDone = OSAL_TRUE;
            }
        }

        if (SAPP_OK == _SAPP_imParseTextPlain(service_ptr,
                to_ptr, from_ptr, dateTime_ptr, hf_ptr, hfLen,
                start_ptr, size, &evt_ptr->isiMsg)) {
            SAPP_sendEvent(evt_ptr);
            /* Since we just sent an event to isi, let's clear the code */
            OSAL_memSet(evt_ptr, 0, sizeof(ISIP_Message));
            continue;
        }
        if (SAPP_OK == _SAPP_imParseSipMessage(service_ptr, hf_ptr, hfLen,
                start_ptr, size, evt_ptr)) {
            SAPP_sendEvent(evt_ptr);
            /* Since we just sent an event to isi, let's clear the code */
            OSAL_memSet(evt_ptr, 0, sizeof(ISIP_Message));
            continue;
        }
        if (SAPP_OK == _SAPP_imParseCpim(service_ptr, hf_ptr, hfLen,
                start_ptr, size, evt_ptr)) {
            SAPP_sendEvent(evt_ptr);
            /* Since we just sent an event to isi, let's clear the code */
            OSAL_memSet(evt_ptr, 0, sizeof(ISIP_Message));
            continue;
        }
        if (SAPP_OK == _SAPP_imParse3gpp(service_ptr,
                to_ptr, from_ptr, dateTime_ptr, hf_ptr, hfLen,
                start_ptr, size, &evt_ptr->isiMsg)) {
            SAPP_sendEvent(evt_ptr);
            /* Since we just sent an event to isi, let's clear the code */
            OSAL_memSet(evt_ptr, 0, sizeof(ISIP_Message));
            continue;
        }
        if (SAPP_OK == _SAPP_imParse3gpp2(service_ptr,
                to_ptr, from_ptr, dateTime_ptr, hf_ptr, hfLen,
                start_ptr, size, &evt_ptr->isiMsg)) {
            SAPP_sendEvent(evt_ptr);
            /* Since we just sent an event to isi, let's clear the code */
            OSAL_memSet(evt_ptr, 0, sizeof(ISIP_Message));
            continue;
        }
        /* ADD ANY MORE HERE */
    }
    return;
}

static vint _SAPP_imParseMultipartMessage(
    SAPP_ServiceObj *service_ptr,
    char            *to_ptr,
    char            *from_ptr,
    char            *dateTime_ptr,
    char            *contentType_ptr,
    char            *messageType_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    SAPP_Event      *evt_ptr)
{
    if (SAPP_CONTENT_MULTI_PART != SAPP_parseIntFileType(contentType_ptr,
            OSAL_strlen(contentType_ptr))) {
        return (SAPP_ERR);
    }

     if (NULL == messageType_ptr || 0 != OSAL_strncasecmp(
             messageType_ptr,"deliver_sm", sizeof("deliver_sm") - 1)) {
         return (SAPP_ERR);
     }

    _SAPP_imMulipartMixedDecode(service_ptr, to_ptr, from_ptr, dateTime_ptr,
            contentType_ptr, payload_ptr, payloadLen, evt_ptr);
    return (SAPP_OK);
}

vint SAPP_imParseMultipartMessage(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    SAPP_Event      *evt_ptr)
{
    if (SAPP_CONTENT_MULTI_PART != SAPP_parseIntFileType(contentType_ptr,
            OSAL_strlen(contentType_ptr))) {
        return (SAPP_ERR);
    }

    _SAPP_imMulipartMixedDecode(service_ptr, NULL, NULL, NULL,
            contentType_ptr, payload_ptr, payloadLen, evt_ptr);
    return (SAPP_OK);
}

/*
 * ======== _SAPP_imPageEvent() ========
 * This function is the entry point for SIP events that pertain to page mode
 * IM.  In otherwords events stimualted by SIP MESSAGE transactions.
 *
 * Returns:
 *  SAPP_ERR: The SIP MESSAGE related event was not handled by this
 *            routine.  Further processing of this SIP event should continue.
 *  SAPP_ERR: The SIP MESSAGE related event was handled by this
 *            routine and no further processing of this SIP event is needed.
 */
static vint _SAPP_imPageEvent(
    SAPP_ImObj        *im_ptr,
    SAPP_ServiceObj   *service_ptr,
    tUaAppEvent       *sipEvt_ptr,
    SAPP_Event        *evt_ptr,
    SAPP_SipObj       *sip_ptr)
{
    char *contentType_ptr;
    char *messageType_ptr;
    char *dateTime_ptr;
    char *from_ptr;
    char *tel_ptr;
    char *reason_ptr;
    int ret = SAPP_ERR;
    switch (sipEvt_ptr->header.type) {
        case eUA_TEXT_MESSAGE:
            if (NULL == (contentType_ptr =
                SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, sipEvt_ptr))) {
                /*
                 * Then there is no content-type, hence no "content" to process
                 * quitely ignore.
                 */
                break;
            }
            if (NULL != (from_ptr = SAPP_parsePaiHfValue(
                    SAPP_SIP_SCHEME, sipEvt_ptr))) {
                /* Then rewrite the from field */
                OSAL_strncpy(sipEvt_ptr->szRemoteUri, from_ptr,
                        SIP_URI_STRING_MAX_SIZE);
            }

            /* Get other tid bits of info */
            dateTime_ptr = SAPP_parseHfValue(SAPP_DATETIME_HF, sipEvt_ptr);
            messageType_ptr = SAPP_parseHfValue(SAPP_MESSAGE_TYPE_HF, sipEvt_ptr);
    
            if (SAPP_OK == _SAPP_imParseTextPlain(service_ptr,
                    sipEvt_ptr->szToUri, sipEvt_ptr->szRemoteUri,
                    dateTime_ptr, contentType_ptr, SIP_EVT_STR_SIZE_BYTES,
                    sipEvt_ptr->msgBody.payLoad.data,
                    sipEvt_ptr->msgBody.payLoad.length, &evt_ptr->isiMsg)) {

                /*
                 * For YTL, so we can learn telephone numbers
                 * Let's get the phone number of there is one
                 */
                tel_ptr = SAPP_parsePaiHfValue(SAPP_TEL_SCHEME, sipEvt_ptr);
                if (NULL != tel_ptr && 0 != tel_ptr[0]) {
                    OSAL_snprintf(evt_ptr->isiMsg.msg.message.reasonDesc,
                            ISI_EVENT_DESC_STRING_SZ, "ALIAS:%s", tel_ptr);
                }
                ret = SAPP_OK;
            }
            else if (SAPP_OK == _SAPP_imParseCpim(service_ptr,
                    contentType_ptr, SIP_EVT_STR_SIZE_BYTES,
                    sipEvt_ptr->msgBody.payLoad.data,
                    sipEvt_ptr->msgBody.payLoad.length, evt_ptr)) {
                /*
                 * For YTL, so we can learn telephone numbers
                 * Let's get the phone number of there is one
                 */
                tel_ptr = SAPP_parsePaiHfValue(SAPP_TEL_SCHEME, sipEvt_ptr);
                if (NULL != tel_ptr && 0 != tel_ptr[0]) {
                    OSAL_snprintf(evt_ptr->isiMsg.msg.message.reasonDesc,
                            ISI_EVENT_DESC_STRING_SZ, "ALIAS:%s", tel_ptr);
                }
                ret = SAPP_OK;
            }
            if (SAPP_OK == _SAPP_imParse3gpp(service_ptr,
                    sipEvt_ptr->szToUri, sipEvt_ptr->szRemoteUri,
                    dateTime_ptr, contentType_ptr, SIP_EVT_STR_SIZE_BYTES,
                    sipEvt_ptr->msgBody.payLoad.data,
                    sipEvt_ptr->msgBody.payLoad.length, &evt_ptr->isiMsg)) {
                    /* We got 3gpp message */
                ret = SAPP_OK;
            }
            if (SAPP_OK == _SAPP_imParse3gpp2(service_ptr,
                    sipEvt_ptr->szToUri, sipEvt_ptr->szRemoteUri,
                    dateTime_ptr, contentType_ptr, SIP_EVT_STR_SIZE_BYTES,
                    sipEvt_ptr->msgBody.payLoad.data,
                    sipEvt_ptr->msgBody.payLoad.length, &evt_ptr->isiMsg)) {
                    /* We got 3gpp message */
                ret = SAPP_OK;
            }
            else if (SAPP_OK == _SAPP_imParseMultipartMessage(service_ptr,
                    sipEvt_ptr->szToUri, sipEvt_ptr->szRemoteUri,
                    dateTime_ptr, contentType_ptr, messageType_ptr,
                    sipEvt_ptr->msgBody.payLoad.data,
                    sipEvt_ptr->msgBody.payLoad.length, evt_ptr)) {
                ret = SAPP_OK;
            }
            break;
        case eUA_TEXT_MESSAGE_COMPLETED:
            if (sipEvt_ptr->resp.hTransaction == im_ptr->hTransaction) {
                /* we're good, tell ISI */
                _SAPP_imIsiEvt(im_ptr->id, service_ptr->isiServiceId,
                        service_ptr->protocolId, 0,
                        ISIP_TEXT_REASON_COMPLETE, NULL, NULL, NULL, 0,
                        NULL, &evt_ptr->isiMsg);
                ret = SAPP_OK;
                im_ptr->hTransaction = 0;
            }
            break;
        case eUA_TEXT_MESSAGE_FAILED:
            
            /* See if this event is for the recent IM request */
            if (sipEvt_ptr->resp.hTransaction == im_ptr->hTransaction) {
                /* we're good, tell ISI */
                _SAPP_imGetReasonDesc(sipEvt_ptr, &evt_ptr->isiMsg.msg.message);
                _SAPP_imIsiEvt(im_ptr->id, service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        0, ISIP_TEXT_REASON_ERROR,
                        NULL, NULL, NULL, 0, NULL, &evt_ptr->isiMsg);
                /* See if this event is 504 server time out. */
                if (sipEvt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
                    if (SAPP_OK == _SAPP_sipServerTimeOut(service_ptr,
                            &reason_ptr, sipEvt_ptr, sip_ptr)) {
                        /* Do nothing. */
                    }
                }    
                ret = SAPP_OK;
                im_ptr->hTransaction = 0;
            }

            
            break;
        default:
            break;
    } /* End of switch */
    return (ret);
}

/*
 * ======== SAPP_imPageInit() ========
 * This function will initialize a SAPP_ImObj.  This is typically called at 
 * system init time.
 *
 * Returns:
 *  Nothing.
 */
void SAPP_imPageInit(
    SAPP_ImObj     *im_ptr)
{
    im_ptr->hTransaction = 0;
    im_ptr->id = 0;
    return;
}


/*
 * ======== SAPP_imEvent() ========
 * This function is the entry point for SIP events that pertain to 
 * Instant Messaging.
 *
 * Returns:
 *  SAPP_ERR: The SIP event was not handled by this routine.
 *            Further processing of this SIP event should continue.
 *  SAPP_OK:  The SIP event was handled by this routine and no further
 *            processing of this SIP event is needed.
 */
vint SAPP_imEvent(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr)
{
    if (SAPP_OK == _SAPP_imPageEvent(&service_ptr->im, service_ptr,
            sipEvt_ptr, evt_ptr, sip_ptr)) {
        /* Then it was processed */
        return (SAPP_OK);
    }
    /* Return "ERROR" indicating that this Instant Message was not processed */
    return (SAPP_ERR);
}

/* 
 * ======== SAPP_isiImPageCmd() ========
 * 
 * This function is the entry point for commands that come from ISI 
 * that pertain to page mode "IM" (Instant Messaging).  It's the first place
 * the IM command begins to be processed.
 *
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 *
 * sip_ptr : A pointer to an object used internally in SAPP to manage services
 * 
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 *
 * Returns: 
 *   Nothing.
 */
void SAPP_isiImPageCmd(
    ISIP_Message    *cmd_ptr, 
    SAPP_SipObj     *sip_ptr,
    ISIP_Message    *isi_ptr)
{
    SAPP_ServiceObj *service_ptr;
    SAPP_CallObj    *call_ptr;
    ISIP_Text       *text_ptr;
    tSipHandle       dialogId;
    char            *from_ptr;
#ifdef INCLUDE_SIMPLE
    vint             numRecipients;
#endif
 
    text_ptr = &cmd_ptr->msg.message;

    /* Find the service */
    service_ptr = SAPP_findServiceViaServiceId(sip_ptr, text_ptr->serviceId);
    
    /* build from_ptr */
    from_ptr = service_ptr->sipConfig.config.aor[0].szUri;
    if ((NULL != service_ptr->telUri_ptr) &&
            (0 != service_ptr->telUri_ptr[0])) {
        from_ptr = service_ptr->telUri_ptr;
    }

    if (service_ptr == NULL) {
        /* Then nothing to perform, tell ISI about this error */
        _SAPP_imIsiEvt(cmd_ptr->id, text_ptr->serviceId, 
            service_ptr->protocolId, text_ptr->chatId,
            ISIP_TEXT_REASON_ERROR, NULL, NULL, NULL, 0, NULL, isi_ptr);
        return;
    }
    
    /* 
     * Check if it's for a call and if so retrieve the dialog 
     * handle for the call.  If we set a dialog handle then the IM will 
     * be sent within the context of a SIP dialog.  Otherwise, it is send
     * outside the context of a SIP dialog.
     */
    dialogId = 0;
    if (text_ptr->chatId != 0) {
        call_ptr = SAPP_findCallViaIsiId(service_ptr, text_ptr->chatId);
        if (call_ptr) {
            /* get the dialog handle to use in the call to UA_Message() */
            dialogId = call_ptr->dialogId;
        }
    }

    /* Make sure we are doing this for the right 'reason' */
    if (text_ptr->reason == ISIP_TEXT_REASON_NEW) {
        /* 
         * Check how many participants their are.
         * First populate the list of recipients.  If there is just one then
         * perform the broadcast.  Otherwise return so we send a regular
         * MESSAGE.
         */
#ifdef INCLUDE_SIMPLE
        numRecipients = SIMPL_broadcastLoadList(text_ptr->to,
            &service_ptr->simple.rlsList);
        if (1 < numRecipients) {
            /* then let's attempt a broadcast */
            if (SAPP_OK != SIMPL_broadcastIm(service_ptr,
                    from_ptr,
                    &service_ptr->simple.rlsList, text_ptr, cmd_ptr->id)) {
                _SAPP_imGetReasonDesc(NULL, &isi_ptr->msg.message);
                /* Can't send it, tell ISI about the failure */
                _SAPP_imIsiEvt(cmd_ptr->id, text_ptr->serviceId, 
                        service_ptr->protocolId, text_ptr->chatId,
                        ISIP_TEXT_REASON_ERROR, NULL, NULL, NULL, 0,
                        NULL, isi_ptr);
            }
        }
        else {
#endif
            if (SAPP_OK != _SAPP_imPage(service_ptr,
                    from_ptr,
                    text_ptr, cmd_ptr->id, dialogId)) {
                _SAPP_imGetReasonDesc(NULL, &isi_ptr->msg.message);
                /* Can't send it, tell ISI about the failure */
                _SAPP_imIsiEvt(cmd_ptr->id, text_ptr->serviceId, 
                        service_ptr->protocolId, text_ptr->chatId,
                        ISIP_TEXT_REASON_ERROR, NULL, NULL, NULL, 0,
                        NULL, isi_ptr);
            }
#ifdef INCLUDE_SIMPLE
        }
#endif
    }
    else if (text_ptr->reason == ISIP_TEXT_REASON_REPORT) {

        if (SAPP_OK != _SAPP_imNotification(service_ptr,
                from_ptr,
                text_ptr, cmd_ptr->id, dialogId)) {
            /* Can't send it, tell ISI about the failure */
            _SAPP_imIsiEvt(cmd_ptr->id, text_ptr->serviceId, 
                    service_ptr->protocolId, text_ptr->chatId,
                    ISIP_TEXT_REASON_ERROR, NULL, NULL, NULL, 0, NULL, isi_ptr);
        }
    }
    else {
        /*
         * Then we are not interested in this event.
         * Tell ISI about the error
         */
        _SAPP_imIsiEvt(cmd_ptr->id, text_ptr->serviceId, 
                service_ptr->protocolId, text_ptr->chatId,
                ISIP_TEXT_REASON_ERROR, NULL, NULL, NULL, 0, NULL, isi_ptr);
    }
    return;
}

