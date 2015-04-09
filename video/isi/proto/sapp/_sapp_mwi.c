/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
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
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_mwi.h"
#include "_sapp_ipsec.h"
#include "_sapp_dialog.h"

static const char _SAPP_XML_TAG_UMS_SERVICES[]  = "ums-services";
static const char _SAPP_XML_TAG_UMS_SERVICE[]   = "ums-service";
static const char _SAPP_XML_ATTR_ACTION[]       = "action";
static const char _SAPP_XML_VAL_NOTIFICATION[]  = "notification";
static const char _SAPP_XML_TAG_NI[]            = "ni";
static const char _SAPP_XML_TAG_NI_DATA[]       = "ni-data";
//static const char _SAPP_XML_ATTR_SENDER[]       = "sender";       //unused
//static const char _SAPP_XML_ATTR_RECIPIENT[]    = "recipient";    //unused
//static const char _SAPP_XML_ATTR_TIME[]         = "time";         //unused
static const char _SAPP_XML_ATTR_CONTENT[]      = "content";
static const char _SAPP_CONTENT_TYPE_OMA_PUSH_STR[] = "application/vnd.oma.push";

static const char _SAPP_YOU_HAVE_STR[]          = "You have ";
static const char _SAPP_THERE_ARE_STR[]         = "There are ";
static const char _SAPP_THERE_IS_NO_STR[]       = "There is no ";
static const char _SAPP_AND_STR[]               = "and ";

/* 
 * ======== _SAPP_mwiIsiEvt() ========
 *
 * This function is used to populate a ISI event for "telephone event" 
 * specifically for mwi (Message Waiting Indication) related events. 
 * These events will be passed to the ISI module. 
 *
 * Returns: 
 *   Nothing.
 */  
static void _SAPP_mwiIsiEvt(
    ISI_Id              telEvtId,
    ISI_Id              serviceId,
    vint                protocolId,
    uint32              arg0,
    uint32              arg1,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = telEvtId;
    isi_ptr->code = ISIP_CODE_TEL_EVENT;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.event.reason = ISIP_TEL_EVENT_REASON_NEW;
    isi_ptr->msg.event.serviceId = serviceId;
    isi_ptr->msg.event.callId = 0;
    isi_ptr->msg.event.evt = ISI_TEL_EVENT_VOICEMAIL;
    isi_ptr->msg.event.settings.args.arg0 = arg0;
    isi_ptr->msg.event.settings.args.arg1 = arg1;
    return;
}

/*
 * ======== _SAPP_mwiParseTextMessage() ========
 *
 * This function is a helper for MESSAGE requests
 * that are related to mwi (Message Waiting Indication).
 *
 * Returns:
 *  SAPP_ERR: The SIP MESSAGE was not handled by this routine.  Further
 *            processing of this SIP event should continue.
 *  SAPP_OK: The SIP MESSAGE event was handled by this routine and no further
 *            processing of this SIP event is needed.
 */
static vint _SAPP_mwiParseTextMessage(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    char       *contentType_ptr;

    /* Parse the Content-Type and message body (XML doc) */
    if (NULL == (contentType_ptr =
            SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, sipEvt_ptr))) {
        /*
         * Then there is no content-type, hence no "content" to process
         * quitely ignore.
         */
        return (SAPP_ERR);
    }

    return SAPP_mwiParseTextMessage(
            service_ptr, contentType_ptr,
            sipEvt_ptr->msgBody.payLoad.data,
            sipEvt_ptr->msgBody.payLoad.length, &evt_ptr->isiMsg);
}


static vint _SAPP_mwiParseIot(
    const char *content_ptr,
    uint32     *readMsg_ptr,
    uint32     *unreadMsg_ptr)
{
    vint   total = 0;
    vint   unread = 0;
    const char *str1_ptr;
    const char *str2_ptr;

    if (NULL != (str1_ptr = OSAL_strncasescan(content_ptr,
            SAPP_STRING_SZ, _SAPP_YOU_HAVE_STR))) {
        str1_ptr += (sizeof(_SAPP_YOU_HAVE_STR) - 1);
        if (0 >= (total = OSAL_atoi(str1_ptr))) {
             *readMsg_ptr = 0;
             *unreadMsg_ptr = 0;
             return (SAPP_OK);
        }
        unread = 0;
        if (NULL != (str2_ptr = OSAL_strncasescan(str1_ptr,
                SAPP_STRING_SZ, _SAPP_AND_STR))) {
            str2_ptr += (sizeof(_SAPP_AND_STR) - 1);
            if (0 > (unread = OSAL_atoi(str2_ptr))) {
                unread = 0;
            }
        }
        *readMsg_ptr = (uint32)total - unread;
        *unreadMsg_ptr = (uint32)unread;
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

static vint _SAPP_mwiParsePdc(
    const char *content_ptr,
    uint32     *readMsg_ptr,
    uint32     *unreadMsg_ptr)
{
    vint        read = 0;
    vint        unread = 0;
    const char *str1_ptr;
    const char *str2_ptr;

    if (NULL != (str1_ptr = OSAL_strncasescan(content_ptr,
            SAPP_STRING_SZ, _SAPP_THERE_IS_NO_STR))) {
        *readMsg_ptr = 0;
        *unreadMsg_ptr = 0;
        return (SAPP_OK);
    }

    /* For other PDC updates let's step off the first sentence. */
    if (NULL == (str1_ptr = OSAL_strchr(content_ptr, '.'))) {
        str1_ptr = content_ptr;
    }

    if (NULL != (str2_ptr = OSAL_strncasescan(str1_ptr,
            SAPP_STRING_SZ, _SAPP_THERE_ARE_STR))) {
        str2_ptr += (sizeof(_SAPP_THERE_ARE_STR) - 1);
        if (0 > (unread = OSAL_atoi(str2_ptr))) {
            unread = 0;
        }
        if (NULL != (str1_ptr = OSAL_strncasescan(str2_ptr,
                SAPP_STRING_SZ, _SAPP_AND_STR))) {
            str1_ptr += (sizeof(_SAPP_AND_STR) - 1);
            if (0 > (read = OSAL_atoi(str1_ptr))) {
                read = 0;
            }
        }
        *readMsg_ptr = (uint32)read;
        *unreadMsg_ptr = (uint32)unread;
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== _SAPP_mwiSendSub() ========
 * This function sends a SUBSCRIBE request to subscribe to MWI events.
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
 */
static vint _SAPP_mwiSendSub(
    SAPP_MwiObj     *mwi_ptr,
    SAPP_ServiceObj *service_ptr,
    vint             expiresSecs,
    tSipHandle      *dialogId_ptr)
{
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;
    vint             status;

    SAPP_dbgPrintf("%s: Sending SUBSCRIBE request for MWI event pkg\n",
            __FUNCTION__);

    numHdrFlds = 0;

    /* Build up a SIP subscribe request and send */
    /* Event: message-summaryEvent: message-summary */
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_EVENT_HF, _SAPP_MWI_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Expires:  */
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %d",
            SAPP_EXPIRES_HF, expiresSecs);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Accept: application/simple-message-summary */    
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
        SAPP_ACCEPT_HF, _SAPP_MWI_EVENT_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }
    status = UA_Subscribe(
            service_ptr->sipConfig.uaId, 
            dialogId_ptr,
            service_ptr->sipConfig.config.aor[0].szUri, 
            service_ptr->sipConfig.config.aor[0].szUri,
            hdrFlds_ptr, numHdrFlds, NULL, 0,
            &service_ptr->sipConfig.localConn);
    if (SIP_DONE == status || SIP_OK == status) {
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== SAPP_mwiSubscribe() ========
 * This function sends a SUBSCRIBE request to subscribe to MWI events.
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
 */
vint SAPP_mwiSubscribe(
    SAPP_MwiObj     *mwi_ptr,
    SAPP_ServiceObj *service_ptr)
{
    SAPP_dbgPrintf("%s: Attempting to SUBSCRIBE\n", __FUNCTION__);
    mwi_ptr->subscription.dialogId = 0;

    if (SAPP_OK != _SAPP_mwiSendSub(mwi_ptr, service_ptr,
            mwi_ptr->timeoutSecs, &mwi_ptr->subscription.dialogId)) {
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/*
 * ======== SAPP_mwiUnsubscribe() ========
 * This function sends a UNSUBSCRIBE request to subscribe to MWI events.
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
 */
vint SAPP_mwiUnsubscribe(
    SAPP_MwiObj     *mwi_ptr,
    SAPP_ServiceObj *service_ptr)
{
    SAPP_dbgPrintf("%s: Attempting to UNSUBSCRIBE\n", __FUNCTION__);

    if (0 != mwi_ptr->subscription.dialogId) {
        _SAPP_mwiSendSub(mwi_ptr, service_ptr, 
                0, &mwi_ptr->subscription.dialogId);
        mwi_ptr->subscription.dialogId = 0;
    }
    return (SAPP_OK);
}

/* 
 * ======== SAPP_mwiInit() ========
 *
 * This function is used to initialize an object used to manage MWI state.
 * This function is typically called when ISI has instructed SAPP to 
 * create a service.
 *
 * Returns: 
 *   Nothing.
 */
void SAPP_mwiInit(
    SAPP_MwiObj     *mwi_ptr,
    uint32           reMwiTimeoutSecs,
    vint             useMwiEvt,
    SAPP_ServiceObj *service_ptr)
{
    mwi_ptr->timeoutSecs              = reMwiTimeoutSecs;
    mwi_ptr->useMwiEvt                = useMwiEvt;
    mwi_ptr->subscription.dialogId    = 0;
    mwi_ptr->subscription.service_ptr = service_ptr;
}

vint SAPP_mwiParseTextMessage(
    SAPP_ServiceObj *service_ptr,
    char            *contentType_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    ISIP_Message    *isi_ptr)
{
    uint32      arg0;
    uint32      arg1;
    ezxml_t     xml_ptr;
    ezxml_t     child_ptr;
    const char *value_ptr;
    
    /* Check if it's "application/vnd.oma.push" only */
    if (0 != OSAL_strncasecmp(contentType_ptr, _SAPP_CONTENT_TYPE_OMA_PUSH_STR,
            sizeof(_SAPP_CONTENT_TYPE_OMA_PUSH_STR) - 1)) {
        /* We don't understand, quietly ignore */
        return (SAPP_ERR);
    }

    /*
     * Decode mwiEvent values from XML
     */
    OSAL_memCpy(service_ptr->payloadStratch, payload_ptr, payloadLen);
    if (NULL == (xml_ptr = ezxml_parse_str(service_ptr->payloadStratch,
            payloadLen))) {
        return (SAPP_ERR);
    }

    /* Check for the mandatory 'ums-services' root tag */
    if (NULL == (value_ptr = ezxml_name(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncasecmp(value_ptr, _SAPP_XML_TAG_UMS_SERVICES,
            sizeof(_SAPP_XML_TAG_UMS_SERVICES - 1))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Get the "ums-service" child element */
    if (NULL == (child_ptr = ezxml_child(xml_ptr,
            _SAPP_XML_TAG_UMS_SERVICE))) {
        /* can't find it, return */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }

    /* Verify the 'action' attr equals 'notification' */
    if (NULL == (value_ptr = ezxml_attr(child_ptr, _SAPP_XML_ATTR_ACTION))) {
        /* This is mandatory so return if it doesn't exist */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }
    if (0 != OSAL_strncasecmp(value_ptr, _SAPP_XML_VAL_NOTIFICATION, SAPP_STRING_SZ)) {
        /* Then this doesn't belong to us */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }

    /* Move down to the "ni" child element */
    if (NULL == (child_ptr = ezxml_child(child_ptr,
            _SAPP_XML_TAG_NI))) {
        /* can't find it, return */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }
    /* Move down to the "ni-data" child element */
    if (NULL == (child_ptr = ezxml_child(child_ptr,
            _SAPP_XML_TAG_NI_DATA))) {
        /* can't find it, return */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }

    /* Parse the 'content' attr, which gives total and unread. */
    if (NULL == (value_ptr = ezxml_attr(child_ptr, _SAPP_XML_ATTR_CONTENT))) {
        /* This is mandatory so return error if it doesn't exist */
        ezxml_free(xml_ptr);
        return (SAPP_OK);
    }

    /* arg0 = unread arg1 = read */

    /* Let's parse the content.  There are 2 different flavors from YTL */
    if (SAPP_OK == _SAPP_mwiParsePdc(value_ptr, &arg1, &arg0)) {
        /* Populate the ISI message and send to ISI */
        _SAPP_mwiIsiEvt(0, service_ptr->isiServiceId,
                service_ptr->protocolId, arg0, arg1, isi_ptr);
    }
    else if (SAPP_OK == _SAPP_mwiParseIot(value_ptr, &arg1, &arg0)) {
        /* Populate the ISI message and send to ISI */
        _SAPP_mwiIsiEvt(0, service_ptr->isiServiceId,
                service_ptr->protocolId, arg0, arg1, isi_ptr);
    }
    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}

/* 
 * ======== SAPP_mwiEvent() ========
 * 
 * This function is the handler for NOTIFY or MESSAGE requests
 * that are related to mwi (Message Waiting Indication).
 *
 * Returns: 
 *  SAPP_ERR: The SIP NOTIFY was not handled by this routine.  Further 
 *            processing of this SIP event should continue.
 *  SAPP_OK: The SIP NOTIFY event was handled by this routine and no further
 *            processing of this SIP event is needed.
 */
vint SAPP_mwiEvent(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr)
{
    vint             value;
    char            *value_ptr;
    char            *pos_ptr;
    uint32           arg0;
    uint32           arg1;
    char            *reason_ptr;

    if (eUA_TEXT_MESSAGE == sipEvt_ptr->header.type) {
        return _SAPP_mwiParseTextMessage(service_ptr, sipEvt_ptr, evt_ptr);
    }

    if (eUA_SUBSCRIBE_FAILED == sipEvt_ptr->header.type) {
        if (sipEvt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
            /* Server response 504 server time out. */
            if (SAPP_OK == _SAPP_sipServerTimeOut(service_ptr, &reason_ptr,
                    sipEvt_ptr, sip_ptr)) {
                return (SAPP_OK);
            }
        }
    }

    if (eUA_NOTIFY_EVENT != sipEvt_ptr->header.type) {
        /* Then this is not for us */
        return (SAPP_ERR);
    }
    
    /* Parse the message to find if a message is waiting. */
    if (0 == sipEvt_ptr->msgBody.payLoad.length) {
        /* Then there is no payload info. Quietly ignore */
        return (SAPP_ERR);
    }
        
    /* Check for the message waiting indication */
    if (SAPP_OK != SAPP_parsePayloadValue(sipEvt_ptr->msgBody.payLoad.data,
            "Messages-Waiting: ", &value_ptr, &value)) {
        /* Didn't find and info that we are intersted in. Quietly ignore */
        return (SAPP_ERR);
    }
    
    /* Calculate the max size, copy, NULL terminate payload value */
    value = (value >= SAPP_PAYLOAD_SZ) ? (SAPP_PAYLOAD_SZ - 1) : value;
    OSAL_memCpy(service_ptr->payloadStratch, value_ptr, value);
    service_ptr->payloadStratch[value] = 0;
        
    /* 
     * Get the mailbox statistics.  arg0 == 'number of unread messages' 
     * arg1 = 'number of read messages'. Report statisics to ISI.
     */

    /* 
     * Check if 'yes' is there and set default values just in case there is
     * no mailbox statistics included in this mwi notification.
     */
    if (NULL != OSAL_strnscan(service_ptr->payloadStratch, SAPP_PAYLOAD_SZ,
            "yes")) {
        /* 1 unread, 0 read */
        arg0 = 1;
        arg1 = 0;
    }
    else {
        /* 0 unread, 0 read, the mailbox is empty */
        arg0 = 0;
        arg1 = 0;
    }
        
    /* Check for real mailbox statistics */
    if (SAPP_OK == SAPP_parsePayloadValue(sipEvt_ptr->msgBody.payLoad.data,
            "Voice-Message: ", &value_ptr, &value)) {
        /* 
         * Then we have statistics. A.K.A. a summary of the mailbox. 
         * Copy them and NULL terminate for further processing. 
         */
        value = (value >= SAPP_PAYLOAD_SZ) ? (SAPP_PAYLOAD_SZ - 1) : value;
        OSAL_memCpy(service_ptr->payloadStratch, value_ptr, value);
        service_ptr->payloadStratch[value] = 0;
        pos_ptr = OSAL_strnscan(service_ptr->payloadStratch, value, "/");
        if (NULL != pos_ptr) {
            /* Then we the number of 'unread' messages */
            *pos_ptr = 0;
            arg0 = OSAL_atoi(service_ptr->payloadStratch);
            pos_ptr++;
            arg1 = OSAL_atoi(pos_ptr);
        }
    }
    /* Get From address */
    OSAL_snprintf(evt_ptr->isiMsg.msg.event.from, ISI_ADDRESS_STRING_SZ, 
            "%s", sipEvt_ptr->szRemoteUri);

    SAPP_dbgPrintf("%s: serviceId:%d mwi summary unread:%d read:%d\n",
            __FUNCTION__, service_ptr->isiServiceId, arg0, arg1);
            
    /* Populate the ISI message and send to ISI */
    _SAPP_mwiIsiEvt(0, service_ptr->isiServiceId,
            service_ptr->protocolId, arg0, arg1, &evt_ptr->isiMsg);
    
    return (SAPP_OK);
}

