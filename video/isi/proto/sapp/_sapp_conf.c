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

#include "_sapp.h"
#include "_sapp_xml.h"
#include "_sapp_conf.h"
#include "_sapp_parse_helper.h"
#include "_sapp_conf.h"
#include "_sapp_ipsec.h"
#include "_sapp_dialog.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "./simple/_simple_types.h"
#include "./simple/_simple_manage.h"
#endif

/* 
 * This file handles Subscriptions to the conference event package
 * as specified in RFC4575.
 */

/*
 * ======== _SAPP_confSub() ========
 * This function sends a SUBSCRIBE request to subscribe to the conference
 * event package.
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
 */
static vint _SAPP_confSub(
    tSipHandle      *hDialod_ptr,
    SAPP_ServiceObj *service_ptr,
    char            *to_ptr,
    char            *from_ptr,
    char            *acceptContact_ptr,
    vint             expiresSecs)
{
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;
    vint             status;

    SAPP_dbgPrintf("%s: Sending SUBSCRIBE request for conference events\n",
            __FUNCTION__);
    
    numHdrFlds = 0;
    
    /* 
     * Add a 'preconfigured route' if the registrar reported one during the 
     * registration process
     */
    if (SAPP_OK == SAPP_sipAddPreconfiguredRoute(&service_ptr->registration,
            &service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ)) {
        /* Then a "Route" header field was added */
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    /* Build up a SIP subscribe request and send */
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_EVENT_HF, _SAPP_CONF_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
        SAPP_ACCEPT_HF, _SAPP_CONF_EVENT_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Note that the 'default' expires for RFC4575 is 1 hour */
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %d",
            SAPP_EXPIRES_HF, expiresSecs);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    
    
    if (NULL != acceptContact_ptr && 0 != *acceptContact_ptr) {
        /* Add the Accept-Contact header field per OMA SAPPE V1 
         * section 7.1.1.11 */
        OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, 
                "%s *;%s", SAPP_ACCEPT_CONTACT_HF, acceptContact_ptr);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_SUPPORTED_HF, _SAPP_EVENTLIST_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Add P-Preferred-Identity header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF,
            from_ptr);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    
    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }

    /* Add SRVCC capabilities. */
    
    status = UA_Subscribe(service_ptr->sipConfig.uaId,
            hDialod_ptr, to_ptr, from_ptr, hdrFlds_ptr,
            numHdrFlds, NULL, service_ptr->srvccCapabilitiesBitmap,
            &service_ptr->sipConfig.localConn);
    if (SIP_DONE == status || SIP_OK == status) {
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== _SAPP_confSubProcessContent() ========
 * This routine will process the message bodies of NOTIFY requests related to 
 * subscriptions to the conference event package RFC4575.  ISI events are 
 * typically generated from calling this routine.
 *
 * Returns:
 *  Nothing.
 */
static void _SAPP_confSubProcessContent(
    ISI_Id           isiId,
    SAPP_ServiceObj *service_ptr,
    const char      *from_ptr,
    char            *contentType_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    SAPP_Event      *evt_ptr) 
{
    SAPP_xmlDecodeConferenceInfoDoc(isiId, service_ptr, from_ptr,
            contentType_ptr, payload_ptr, payloadLen, evt_ptr);
    return;
}

/*
 * ======== _SAPP_confSubNotifyEvent() ========
 * This function is the entry point for SIP notifications that pertain to 
 * subscriptions to the conference event package (RFC4575).
 *
 * Returns:
 *  SAPP_ERR: The SIP notification was not handled by this routine.
 *            Further processing of this SIP event should continue.
 *
 *  SAPP_OK:  The SIP notification was handled by this routine and no further
 *            processing of this SIP event is needed.
 */
static vint _SAPP_confSubNotifyEvent(
    ISI_Id           isiId,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    char            *contentType_ptr;
    
    SAPP_dbgPrintf(
            "%s: Received NOTIFY request for the conference event package\n",
            __FUNCTION__);

    if (1 == SAPP_parseHfValueExist(_SAPP_SUB_STATE_HF,
            _SAPP_ACTIVE_ARG, sipEvt_ptr)) {

        /* Check that there is a mandatory msg body */
        if (NULL == sipEvt_ptr->msgBody.payLoad.data ||
                0 == sipEvt_ptr->msgBody.payLoad.length) {
           /* 
            * Let's just return and not process this. Return OK since
            * technically we did process it for the conference event package.
            */
            return (SAPP_OK);
        }

        /* Parse the Content-Type and message body (XML doc) */
        if (NULL == (contentType_ptr = 
                SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, sipEvt_ptr))) {
            /* 
             * Then there is no conten-type, hence no "content" to process
             * quitely ignore.
             */
            return (SAPP_OK);
        }
            
        _SAPP_confSubProcessContent(isiId, service_ptr,
                sipEvt_ptr->szRemoteUri, contentType_ptr,
                sipEvt_ptr->msgBody.payLoad.data,
                sipEvt_ptr->msgBody.payLoad.length, evt_ptr);
    }
    /* Return "OK" indicating that this NOTIFY was successfully processed */
    return (SAPP_OK);
}

/*
 * ======== SAPP_conferenceSubscribe() ========
 * Subscribe to the conference event pacakge.
 * This function will stimulate a SIP SUBSCRIBE request.
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
 */
vint SAPP_conferenceSubscribe(
    tSipHandle      *hDialog_ptr,
    SAPP_ServiceObj *service_ptr,
    char            *to_ptr,
    char            *from_ptr,
    char            *acceptContact_ptr)
{
    SAPP_dbgPrintf("%s: Attempting to Subscribe to an event package at :%s\n",
            __FUNCTION__, to_ptr);

    if (SAPP_OK != _SAPP_confSub(hDialog_ptr, service_ptr, to_ptr, from_ptr,
            acceptContact_ptr, service_ptr->confKeepAlive)) {
        /* Could even send the subscription  */
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== SAPP_conferenceUnsubscribe() ========
 * This function will stimulate a SIP SUBSCRIBE request that will 
 * cancel/terminate the subscription.
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.  The dialog/subscription
 *            was terminated.
 */
vint SAPP_conferenceUnsubscribe(
    tSipHandle      *hDialog_ptr,
    SAPP_ServiceObj *service_ptr,
    char            *to_ptr,
    char            *from_ptr,
    char            *acceptContact_ptr)
{
    SAPP_dbgPrintf("%s: Attempting to unsubscribe from event package\n",
            __FUNCTION__);
    
    if (SAPP_OK != _SAPP_confSub(hDialog_ptr, service_ptr,
            to_ptr, from_ptr, acceptContact_ptr, 0)) {
        /* Could even send the subscription */
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/*
 * ======== SAPP_conferenceSubscribeEvent() ========
 * This function is the entry point for SIP events that pertain to conference
 * events.
 *
 * Returns:
 *  SAPP_ERR: The SIP subscription related event was not handled by this
 *            routine.  Further processing of this SIP event should continue.
 *  SAPP_ERR: The SIP subscription related event was handled by this
 *            routine and no further processing of this SIP event is needed.
 */
vint SAPP_conferenceSubscribeEvent(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr)
{
    SAPP_CallObj *call_ptr = NULL;
    char         *reason_ptr;
#ifdef INCLUDE_SIMPLE
    SIMPL_ImSession *imSess_ptr = NULL;
#endif

    if (NULL == (call_ptr = SAPP_findCallConfEventViaDialogId(service_ptr, 
                    sipEvt_ptr->header.hOwner))) {
        /* This events doesn't belong to the conference event package for 
         * conference calls */
//#ifdef INCLUDE_SIMPLE
#if 0 /* Bug 11127. Disabling the processing of events for IM conferences here since the code
       * in _simpl_conf.c is used for this.  In the future conferenced calls and IM
       * will share the same code.
       */
        if (NULL == (imSess_ptr = SIMPL_mngrSessionGetViaDialogId(
                &service_ptr->simple.imSessions, sipEvt_ptr->header.hOwner))) {
            /* This events doesn't belong to the conference event package 
             * for conference IM sessions */
            return (SAPP_ERR);
        }
#else
        return (SAPP_ERR);
#endif
    }

    SAPP_dbgPrintf(
            "%s: Recv'd subscribe related conference event :%d for dialog:%x\n",
            __FUNCTION__, sipEvt_ptr->header.type, 
            (uint32)sipEvt_ptr->header.hOwner);

    switch (sipEvt_ptr->header.type) {
        case eUA_NOTIFY_EVENT_NO_SUBS: 
        case eUA_SUBSCRIBE_FAILED_NO_SUBS:
            if (call_ptr) {
                call_ptr->conf.dialogId = 0;
            }
#ifdef INCLUDE_SIMPLE
            else {
                imSess_ptr->u.conf.confEventDialogId = 0;
            }
#endif
            SAPP_dbgPrintf(
            "%s: Recv'd 'eUA_SUBSCRIBE_FAILED_NO_SUBS' local sub destroyed\n",
            __FUNCTION__);
            break;
        case eUA_SUBSCRIBE_FAILED:
            /*
             * Note that we don't care about eUA_SUBSCRIBE_FAILED because
             * we will also get a "eUA_SUBSCRIBE_FAILED_NO_SUBS" if the
             * subscription fails.
             */
            if (sipEvt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
                if (SAPP_OK == _SAPP_sipServerTimeOut(service_ptr,
                        &reason_ptr, sipEvt_ptr, sip_ptr)) {
                    return (SAPP_OK);
                }
            }
            break;
        case eUA_SUBSCRIBE_COMPLETED:
            /*
             * Note that we don't care about eUA_SUBSCRIBE_COMPLETED because
             * we assume that the subscription is successful from the
             * beginning and only look for e"UA_NO_MORE_SUBSCRIPTIONS" when
             * the subscription fails.
             */
            break;
        case eUA_NOTIFY_EVENT:
            if (call_ptr) {
                return (_SAPP_confSubNotifyEvent(call_ptr->isiCallId, 
                            service_ptr, sipEvt_ptr, evt_ptr));
            }
#ifdef INCLUDE_SIMPLE
            else {
                 return (_SAPP_confSubNotifyEvent(imSess_ptr->isiId, 
                             service_ptr, sipEvt_ptr, evt_ptr));
            }
#endif
        default:
            return (SAPP_ERR);
    }
    return (SAPP_OK);
}
