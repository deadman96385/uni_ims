/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30363 $ $Date: 2014-12-11 18:17:53 +0800 (Thu, 11 Dec 2014) $
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
#include <sip_auth.h>
#include <sr.h>

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
#include "_sapp_xml.h"
#include "_sapp_reg.h"
#include "_sapp_ipsec.h"
#include "_sapp_dialog.h"
#include "_sapp_radio_update.h"
#include "_sapp_mwi.h"
#include "_sapp_emergency.h"
#include "_sapp_capabilities.h"

#include <ims_net.h>
/*
 * bug 6824: revert to one reg using private ip
 *
#define SAPP_REG_USE_RPORT (1)
*/

static const char _SAPP_REG_NO_DATA_CONN[]      =  "No data connection available.";
static const char _SAPP_REG_ERR_DATA_CONN[]     =  "Data connection error.";
static const char _SAPP_REG_LOGGED_IN[]         =  "Logged in on another phone?";
static const char _SAPP_REG_ACCT_DISABLED[]     =  "Account disabled.";
static const char _SAPP_REG_RETRY[]             =  "Can't register now.";
static const char _SAPP_REG_BAD_SETTING[]       =  "Invalid settings.";
static const char _SAPP_REG_REJECTED[]          =  "Credentials Rejected.";

typedef enum {
    REG_EVT_NONE,
    REG_EVT_UNREG_DONE,
    REG_EVT_COMPLETED,
    REG_EVT_FAILED,
    REG_EVT_CANCELLED,
    REG_EVT_CANCELLED_AUTH,
    REG_EVT_START,
    REG_EVT_STOP,
    REG_EVT_RESTART,
    REG_EVT_NO_NET,
    REG_EVT_RESET,
    REG_EVT_YES_NET,
    REG_EVT_RE_REG,
} SAPP_regSmEvent;

#ifdef SAPP_DEBUG
static const char *SAPP_regStateStr[SAPP_REG_STATE_LAST + 1] = {
    "SAPP_REG_STATE_NONE",
    "SAPP_REG_STATE_OFF",
    "SAPP_REG_STATE_TRYING",
    "SAPP_REG_STATE_RE_REG",
    "SAPP_REG_STATE_ON",
    "SAPP_REG_STATE_NO_NET",
    "SAPP_REG_STATE_DE_REG_TRYING",
    "SAPP_REG_STATE_RESTART",
    "SAPP_REG_STATE_LAST",
};

static const char *SAPP_regCmdStr[REG_EVT_RE_REG + 1] = {
    "REG_EVT_NONE",
    "REG_EVT_UNREG_DONE",
    "REG_EVT_COMPLETED",
    "REG_EVT_FAILED",
    "REG_EVT_CANCELLED",
    "REG_EVT_CANCELLED_AUTH",
    "REG_EVT_START",
    "REG_EVT_STOP",
    "REG_EVT_RESTART",
    "REG_EVT_NO_NET",
    "REG_EVT_RESET",
    "REG_EVT_YES_NET",
    "REG_EVT_RE_REG",
};
#endif

/*
 * ======== _SAPP_isiPresenceEvt() ========
 * This function is a helper to populate an ISI event indicating the
 * number of registered devices on the network for this endpoint.  This info
 * is past via a presence update. 
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_regIsiPresenceEvt(
    ISI_Id              presId,
    SAPP_ServiceObj    *service_ptr,
    ISIP_PresReason     reason,
    const char         *to_ptr,
    const char         *from_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = presId;
    isi_ptr->code = ISIP_CODE_PRESENCE;
    isi_ptr->protocol = service_ptr->protocolId;
    isi_ptr->msg.presence.reason = reason;
    isi_ptr->msg.presence.serviceId = service_ptr->isiServiceId;
    if (NULL != to_ptr) {
        OSAL_snprintf(isi_ptr->msg.presence.to, ISI_ADDRESS_STRING_SZ, "%s",
                to_ptr);
    }
    else {
        isi_ptr->msg.presence.to[0] = 0;
    }
    if (NULL != from_ptr) {
        OSAL_snprintf(isi_ptr->msg.presence.from, ISI_ADDRESS_STRING_SZ, "%s",
                from_ptr);
    }
    else {
        isi_ptr->msg.presence.from[0] = 0;
    }
    return;
}

static void _SAPP_regGetReasonDesc(
    tUaAppEvent  *uaEvt_ptr,
    ISIP_Service *isi_ptr)
{
    const char *reason_ptr = NULL;
    vint  respCode = SIP_RSP_CODE_UNKNOWN;

    SAPP_dbgPrintf("%s %d: reason:%s\n", __FUNCTION__, __LINE__,
            uaEvt_ptr->resp.szReasonPhrase);

    if (NULL != uaEvt_ptr) {
        /* Let's see if there's a reason phrase */
        reason_ptr = SAPP_parseHfValue(SAPP_REASON_HF, uaEvt_ptr);
        respCode = uaEvt_ptr->resp.respCode;
    }

    /* Look for 'D2' SIP error codes and convert to ISI terminology. */
    if (SIP_RSP_CODE_UNKNOWN == respCode) {
        respCode = ISI_NO_NETWORK_AVAILABLE;
        reason_ptr = _SAPP_REG_ERR_DATA_CONN;
    }
    else if (SIP_RSP_CODE_XACT_TIMEOUT == respCode) {
        respCode = ISI_REQUEST_TIMED_OUT;
        reason_ptr = ISI_REQUEST_TIMED_OUT_STR;
    }
    else if (SIP_RSP_CODE_INTERNAL_ERROR == respCode) {
        respCode = ISI_NO_AVAILABLE_RESOURCES;
        reason_ptr = ISI_NO_AVAILABLE_RESOURCES_STR;
    }
    else if (NULL == reason_ptr) {
        reason_ptr = (char*)SAPP_GetResponseReason(respCode);
    }

    OSAL_snprintf(isi_ptr->reasonDesc, ISI_EVENT_DESC_STRING_SZ,
            "REG FAILED: CODE:%d REASON:%s", respCode, reason_ptr);
    return;
}

/*
 * ======== _SAPP_regSendSub() ========
 * This function sends a SUBSCRIBE request to subscribe to regsitration events. 
 *
 * Returns:
 *  SAPP_ERR: The subscription request could not be made.
 *  SAPP_OK : The subscription request was successful.
 */
static vint _SAPP_regSendSub(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr,
    vint             expiresSecs,
    tSipHandle      *dialogId_ptr)
{
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;
    vint             status;

    SAPP_dbgPrintf("%s: Sending SUBSCRIBE request for Reg event pkg\n",
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

    /* Add P-Preferred-Identity header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF,
            service_ptr->sipConfig.config.aor[0].szUri);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    
    /* Build up a SIP subscribe request and send */
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_EVENT_HF, _SAPP_REG_EVENT_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_ACCEPT_HF, _SAPP_REG_ACCEPT_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %d",
            SAPP_EXPIRES_HF, expiresSecs);
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
            &reg_ptr->lclConn);
    if (SIP_DONE == status || SIP_OK == status) {
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

vint _SAPP_regSubscribe(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr)
{
    SAPP_dbgPrintf("%s: Attempting to SUBSCRIBE\n", __FUNCTION__);
    reg_ptr->subscription.dialogId = 0;
    
    if (SAPP_OK != _SAPP_regSendSub(reg_ptr, service_ptr,
            reg_ptr->timeoutSecs, &reg_ptr->subscription.dialogId)) {
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static void _SAPP_regUnsubscribe(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr)
{
    SAPP_dbgPrintf("%s: Attempting to UNSUBSCRIBE\n", __FUNCTION__);
    if (0 != reg_ptr->subscription.dialogId) {
        /* Then kill off the subscription for the "reg" event package */
        _SAPP_regSendSub(reg_ptr, service_ptr, 0,
                &reg_ptr->subscription.dialogId);
        reg_ptr->subscription.dialogId = 0;
    }
    return;
}

static vint _SAPP_checkLocalConnValidity(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr)
{
    OSAL_NetAddress addr;

    /* Convert address to network byte order */
    OSAL_netAddrHton(&addr, &reg_ptr->lclConn.addr);

    if (!_SAPP_sipServiceIsTransportReady(service_ptr)) {
        return (SAPP_ERR);
    }

    /* Don't regeister if the connection info does not have any valid info */
    if (OSAL_netIsAddrZero(&addr) || OSAL_netIsAddrLoopback(&addr)) {
        /* Tell ISI */
        OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ,
                "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                _SAPP_REG_NO_DATA_CONN);
        SAPP_serviceIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                    ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
        SAPP_sendEvent(evt_ptr);
        /* Send another event up for deactivate */
        SAPP_serviceIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                &evt_ptr->isiMsg);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regRetryTimerCb() ========
 *
 * This function is the callback functio of registration retry delay time expires.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
static int32 _SAPP_regRetryTimerCb(
    void  *arg_ptr)
{
    SAPP_TmrEvent       tmrEvt;
    SAPP_ServiceObj    *service_ptr = (SAPP_ServiceObj*) arg_ptr;

    /* Setup the event to send */
    tmrEvt.type = SAPP_TMR_EVENT_REG_RETRY;
    tmrEvt.arg_ptr = service_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(service_ptr->registration.tmrEvtQ,
            (char *)&tmrEvt, sizeof(SAPP_TmrEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED\n", __FUNCTION__, __LINE__);
        OSAL_tmrStart(service_ptr->registration.regMsgTmrId,
                _SAPP_regRetryTimerCb, arg_ptr, SAPP_MESSAGE_RETRY_TIMER_MS);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regDeRegTimerCb() ========
 *
 * This function is the callback functio of de-registration timeout. 
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
static int32 _SAPP_regDeRegTimerCb(
    void  *arg_ptr)
{
    SAPP_TmrEvent       tmrEvt;
    SAPP_ServiceObj    *service_ptr = (SAPP_ServiceObj*) arg_ptr;

    /* Setup the event to send */
    tmrEvt.type = SAPP_TMR_EVENT_DE_REG;
    tmrEvt.arg_ptr = service_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(service_ptr->registration.tmrEvtQ,
            (char *)&tmrEvt, sizeof(SAPP_TmrEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED\n", __FUNCTION__, __LINE__);
        OSAL_tmrStart(service_ptr->registration.regMsgTmrId,
                _SAPP_regDeRegTimerCb, arg_ptr, SAPP_MESSAGE_RETRY_TIMER_MS);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regSubscribeRetryTimerCb() ========
 *
 * This function is the callback functio of registration subscribe retry delay time expires.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
static int32 _SAPP_regSubscribeRetryTimerCb(
    void  *arg_ptr)
{
    SAPP_TmrEvent       tmrEvt;
    SAPP_ServiceObj    *service_ptr = (SAPP_ServiceObj*) arg_ptr;

    /* Setup the event to send */
    tmrEvt.type = SAPP_TMR_EVENT_REG_SUBSCRIBE_RETRY;
    tmrEvt.arg_ptr = service_ptr;
    SAPP_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);

    if (OSAL_SUCCESS != OSAL_msgQSend(service_ptr->registration.tmrEvtQ,
            (char *)&tmrEvt, sizeof(SAPP_TmrEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED\n", __FUNCTION__, __LINE__);
        OSAL_tmrStart(service_ptr->registration.regTmrId,
                _SAPP_regSubscribeRetryTimerCb, arg_ptr, SAPP_MESSAGE_RETRY_TIMER_MS);
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regCalRetryDelayTime() ========
 *
 * This function is to compute the retry delay time.
 *
 * Returns: 
 *  The retry delay time.
 */
static uint32 _SAPP_regCalRetryDelayTime(
    SAPP_RegObj      *reg_ptr)
{
    uint32  upperWaitTime = 0;

     /*
     * The upper-bound wait time (W) is computed by
     * W = min (max-time, (base-time * (2 ^ consecutive-failures)))
     */   
    upperWaitTime = reg_ptr->regRetryBaseTime * (1 << reg_ptr->regFailCount);
    if (upperWaitTime > reg_ptr->regRetryMaxTime) {
        upperWaitTime = reg_ptr->regRetryMaxTime;
    }
    
    /*
     * The retry delay time is computed by selecting a uniform random 
     * time between 50 and 100% of the upper-bound wait time
     */
    return (SIP_randInt(upperWaitTime >> 1, upperWaitTime));
}

/* 
 * ======== _SAPP_regStartDeRegTmr() ========
 *
 * This function is to start de-deregistration timer. 
 *
 * Returns: 
 *   SAPP_OK  : Process done.
 *   SAPP_ERR : Error in start timer.
 */
vint _SAPP_regStartDeRegTmr(
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr)
{
    SAPP_RegObj  *reg_ptr;

    reg_ptr = &service_ptr->registration;

    if (0 == reg_ptr->regTmrId) {
        return (SAPP_ERR);
    }
    else {
        /* Stop timer anyway. */
        OSAL_tmrStop(reg_ptr->regTmrId);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrStart(reg_ptr->regTmrId,
            _SAPP_regDeRegTimerCb, service_ptr, SAPP_REG_DE_REG_TIMER_MS)) {
        OSAL_tmrDelete(reg_ptr->regTmrId);
        reg_ptr->regTmrId = 0;
        return (SAPP_ERR);
    }
    
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regStartRetryTmr() ========
 *
 * This function is to start the registration retry timer.
 *
 * Returns: 
 *   SAPP_OK  : Process done.
 *   SAPP_ERR : Error in start timer.
 */
vint _SAPP_regStartRetryTmr(
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr)
{
    SAPP_RegObj  *reg_ptr;
    uint32        expires;

    reg_ptr = &service_ptr->registration;

    /* Don't need to retry if timeout value is zero. */
    if ((reg_ptr->regRetryBaseTime == 0) || (reg_ptr->regRetryMaxTime == 0)) {
        return (SAPP_OK);
    }
    
    if (0 == reg_ptr->regTmrId) {
        return (SAPP_ERR);
    }
    else {
        OSAL_tmrStop(reg_ptr->regTmrId);
    }


    if (sipEvt_ptr) {
        /* Increment fail count only when we got error response */
        reg_ptr->regFailCount++;
        
        /* See the there is Retry-After header field. */
        if (sipEvt_ptr->resp.retryAfterPeriod) {
            /* 
             * If Retry-After header field exist, using that value as the retry
             * delay time.
             */
            expires = sipEvt_ptr->resp.retryAfterPeriod;
        }
        else {
            expires = _SAPP_regCalRetryDelayTime(reg_ptr);
        }
    }
    else {
        expires = _SAPP_regCalRetryDelayTime(reg_ptr);
    }

    SAPP_dbgPrintf("Start Reg retry timer %d seconds\n", expires);

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrStart(reg_ptr->regTmrId,
            _SAPP_regRetryTimerCb, service_ptr, expires * 1000)) {
        OSAL_tmrDelete(reg_ptr->regTmrId);
        reg_ptr->regTmrId = 0;
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regStopRegTmr() ========
 *
 * This function is to stop the registration retry timer or de-registration
 * timer.
 *
 * Returns: 
 *   SAPP_OK  : Process done.
 *   SAPP_ERR : Error in start timer.
 */
vint _SAPP_regStopRegTmr(
    SAPP_ServiceObj  *service_ptr)
{
    SAPP_RegObj     *reg_ptr = &service_ptr->registration;

    if (0 != reg_ptr->regTmrId) {
        OSAL_tmrStop(reg_ptr->regTmrId);
    }
    if (0 != reg_ptr->regMsgTmrId) {
        OSAL_tmrStop(reg_ptr->regMsgTmrId);
    }

    return (SAPP_OK);
}

/* 
 * ======== _SAPP_regAdvancePcscf() ========
 *
 * This function is to select a different P-CSCF address.
 *
 * Returns: 
 *   Nothing
 */
void _SAPP_regAdvancePcscf(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegObj     *reg_ptr = &service_ptr->registration;
    char             proxyAddrStr[SAPP_LONG_STRING_SZ];
    vint index;

    if ((SIP_RSP_USE_PROXY == service_ptr->registration.regFailCode) ||
            (SIP_RSP_SERVER_TIMEOUT ==
            service_ptr->registration.regFailCode)) {
        /* Find a different pcscf from last tried one. */
        for (index = 0; index < SAPP_MAX_PCSCF_NUM; index++) {
            if (reg_ptr->pcscfList[index][0]) {
                if (OSAL_strcmp(reg_ptr->pcscfTrying,
                        &reg_ptr->pcscfList[index][0])) {
                    reg_ptr->pcscfIndex = index;
                    break;
                }
            }
            else {
                break;
            }
        }
        service_ptr->registration.regFailCode = 0;
    }
    else {
        reg_ptr->pcscfIndex++;
        if ((reg_ptr->pcscfIndex >= SAPP_MAX_PCSCF_NUM) || (0 ==
                service_ptr->registration.pcscfList[reg_ptr->pcscfIndex][0])) {
            reg_ptr->pcscfIndex = 0;
        }
    }

    SAPP_dbgPrintf("%s: PCSCF (%d): %s\n", __FUNCTION__, reg_ptr->pcscfIndex,
            &reg_ptr->pcscfList[reg_ptr->pcscfIndex][0]);
    if (0 != service_ptr->registration.pcscfList[reg_ptr->pcscfIndex][0]) {
        SAPP_setHostAndPort(&service_ptr->registration.pcscfList[
                reg_ptr->pcscfIndex][0], proxyAddrStr, SAPP_STRING_SZ,
                &service_ptr->ipsecObj.defaultProxyPort);
        if (0 != service_ptr->registration.pcscfList[reg_ptr->pcscfIndex][0]) {
            UA_Modify(service_ptr->sipConfig.uaId, NULL,
                    &service_ptr->registration.pcscfList[
                    reg_ptr->pcscfIndex][0],
                    0, NULL, NULL, NULL, NULL, NULL, 20, 0);
        }
        /* Clean transport. */
        SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
    }
}


/*
 * ======== _SAPP_regSend() ========
 * This function sends a REGISTER request to a registrar. 
 *
 * NOTE, THE RETURN VALUES RETURN SIP STACK RETURN VALUES.
 *
 * Returns:
 *  SAPP_OK:  A REGISTER request was successfully sent.
 *  SAPP_ERR:  A REGISTER request was NOT sent.
 */
static vint _SAPP_regSend(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr)
{
    vint       ret;
    char      *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint       numHdrFlds;

    SAPP_dbgPrintf("%s: Attempting to REGISTER\n", __FUNCTION__);

     _SAPP_regStopRegTmr(service_ptr);

    /*
     * Cancel previous registration if there is any unfinished registration
     * since we don't have the old de-registering registration.
     * If there is ongoing de-registration, it causes UA_Register() failed
     * and we have to wait to next retry to have successful registration.
     *
     * If there is no regisration, it does nothing.
     */
    UA_CancelRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
           &reg_ptr->lclConn);

    numHdrFlds = 0;

    /* IR.92 Requirement. Authorization in first REGISTER */
    if (SAPP_OK == SAPP_sipAddAuthorization(service_ptr,
            &service_ptr->hfStratch[numHdrFlds][0], SAPP_LONG_STRING_SZ)) {
        /* Then a "Authorization" header field was added */
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_SUPPORTED_HF, SAPP_SUPPORTED_OPT_PATH);
    if (0 != service_ptr->instanceId[0]) {
        /* There is the device id, uuid, so support gruu */
        OSAL_snprintf(service_ptr->hfStratch[numHdrFlds] +
                OSAL_strlen(service_ptr->hfStratch[numHdrFlds]), SAPP_STRING_SZ,
                ", %s", SAPP_SUPPORTED_OPT_GRUU);
    }
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /*
     * Check if we should support IPSEC
     */
    if (service_ptr->sipConfig.useIpSec) {
        /* Set up the "Require" header field for IPSEC */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "Require: %s", "sec-agree");
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
        /* Set up the "Proxy-Require" header field for IPSEC */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "Proxy-Require: %s", "sec-agree");
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Add Security-Client header field */
        _SAPP_ipsecAddSecurityClient(service_ptr,
            &service_ptr->hfStratch[numHdrFlds][0], SAPP_LONG_STRING_SZ);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }
    /* Check emergency register */
    if (OSAL_TRUE == service_ptr->isEmergency) {
        /* Set SAPP_CAPS_EMERGENCY_REG for adding "sos" arg in Contact hf */
        service_ptr->regCapabilitiesBitmap |= SAPP_CAPS_EMERGENCY_REG;
    }


    /* Cache the P-CSCF we tried. */
    OSAL_strncpy(reg_ptr->pcscfTrying,
            &reg_ptr->pcscfList[reg_ptr->pcscfIndex][0], SAPP_LONG_STRING_SZ);
    /* Otherwise the user has explicitly set the registrar, try to register */
    ret = UA_Register(service_ptr->sipConfig.uaId, NULL, 
            reg_ptr->timeoutSecs,  service_ptr->natKeepaliveEnable,
            reg_ptr->natRefreshRateSecs, hdrFlds_ptr, numHdrFlds,
            service_ptr->regCapabilitiesBitmap, &reg_ptr->lclConn,
            service_ptr->isEmergency, service_ptr->instanceId,
            service_ptr->qValue);

    switch (ret) {
        case SIP_BUSY:
        case SIP_FAILED:
            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);

            /*
             * Could not register the endpoint due to a reason with
             * the network interface.
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_RETRY);
            /*
             * Also tell the ISI layer that we are now deactivated due to
             * the failure.
             */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);

            /* Start the registration retry timer */
            _SAPP_regStartRetryTmr(service_ptr, NULL);
            return (SAPP_ERR);
        case SIP_BADPARM:
            /*
             * Then there's some bad parameters.  Tell the UA it's because
             * of mis-configuration by sending an event related to
             * authentication failure
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_BAD_SETTING);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            /* 
             * Also tell the ISI layer that we are now deactivated due to 
             * the failure.
             */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            
            /* Start the registration retry timer */
            _SAPP_regStartRetryTmr(service_ptr, NULL);            
            return (SAPP_ERR);
        default:
            break;
    }
    return (SAPP_OK);
}

/*
 * ======== _SAPP_deRegSend() ========
 * This function sends a UNREGISTER request to a registrar. 
 *
 * NOTE, THE RETURN VALUES RETURN SIP STACK RETURN VALUES.
 *
 * Returns:
 *  SAPP_OK:  A UNREGISTER request was successfully sent.
 *  SAPP_ERR:  A UNREGISTER request was NOT sent.
 */
static vint _SAPP_deRegSend(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr)
{
    vint       ret;

    SAPP_dbgPrintf("%s: Attempting to de-REGISTER.\n", __FUNCTION__);

    ret = UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
            &reg_ptr->lclConn);
    /* We have a previous unregister.*/
    if (SIP_BUSY == ret) {
        UA_CancelUnRegister(service_ptr->sipConfig.uaId, NULL,
                &reg_ptr->lclConn);

        ret = UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                &reg_ptr->lclConn);
    }

    SAPP_dbgPrintf("%s: de-REGISTER ret:%d\n", __FUNCTION__, ret);
    switch (ret) {
        case SIP_BUSY:
        case SIP_FAILED:
            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);

            /*
             * Could not de-register the endpoint due to a reason with
             * the network interface.
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_RETRY);
            /*
             * Also tell the ISI layer that we are now deactivated due to
             * the failure.
             */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            return (SAPP_ERR);
        case SIP_BADPARM:
            /*
             * Then there's some bad parameters.  Tell the UA it's because
             * of mis-configuration by sending an event related to
             * authentication failure
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_BAD_SETTING);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            /* 
          * Also tell the ISI layer that we are now deactivated due to 
          * the failure.
          */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            return (SAPP_ERR);
        default:
            break;
    }


    /* Clear any preconfigured route */
    reg_ptr->preconfiguredRoute[0] = 0;
    /* Kill any active subscriptions to the "reg" event if they exist */
    _SAPP_regUnsubscribe(reg_ptr, service_ptr);
    /* send MWI UNSUBSCRIBE */
    if (0 != service_ptr->mwiObj.useMwiEvt) {
        SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
    }

    return (ret);
}
/*
 * ======== _SAPP_reRegSend() ========
 * This function sends a re-REGISTER request to a registrar.
 *
 * NOTE, THE RETURN VALUES RETURN SIP STACK RETURN VALUES.
 *
 * Returns:
 *  SAPP_OK:  A REGISTER request was successfully sent.
 *  SAPP_ERR:  A REGISTER request was NOT sent.
 */
static vint _SAPP_reRegSend(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr)
{
    vint        ret;
    char       *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint        numHdrFlds;
    uint8       akaAuthResp[SAPP_AKA_AUTH_RESP_SZ + SAPP_AKA_AUTH_IK_SZ +
                        SAPP_AKA_AUTH_CK_SZ];
    uint8      *akaAuthAuts_ptr = NULL;
    vint        akaAuthResLength = 0;
    
    SAPP_dbgPrintf("%s: Attempting to re-REGISTER.\n", __FUNCTION__);
    numHdrFlds = 0;
    /*
     * Check if ISIM authentication is set or not.
     * If not, set isimAuthResp_ptr to NULL.
      */
    if (service_ptr->akaAuthRespSet) {
        if (ISI_SERVICE_AKA_RESPONSE_SYNC_FAILURE ==
                service_ptr->akaAuthResult) {
            akaAuthAuts_ptr  = service_ptr->akaAuthAuts;
        }
        else if (ISI_SERVICE_AKA_RESPONSE_NETWORK_FAILURE ==
                service_ptr->akaAuthResult) {
            akaAuthResLength = 0;
        }
        else {
            akaAuthResLength = service_ptr->akaAuthResLength;
        }
        if ((4 <= akaAuthResLength) && (16 >= akaAuthResLength)) {
            if (1 == reg_ptr->akaVersion) {
                OSAL_memCpy(akaAuthResp, service_ptr->akaAuthResp,
                        service_ptr->akaAuthResLength);
            }
            else {
                OSAL_memCpy(akaAuthResp, service_ptr->akaAuthResp,
                        service_ptr->akaAuthResLength);
                OSAL_memCpy(akaAuthResp + service_ptr->akaAuthResLength,
                        service_ptr->akaAuthIk, SAPP_AKA_AUTH_IK_SZ);
                OSAL_memCpy(akaAuthResp + service_ptr->akaAuthResLength +
                        SAPP_AKA_AUTH_IK_SZ,
                        service_ptr->akaAuthCk, SAPP_AKA_AUTH_CK_SZ);
                akaAuthResLength += (SAPP_AKA_AUTH_IK_SZ + SAPP_AKA_AUTH_CK_SZ);
            }
        }
    }
    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }

    /* Re-register with AKA auth response or shortened expires */
    ret = UA_ReRegister(service_ptr->sipConfig.uaId, NULL,
            hdrFlds_ptr, numHdrFlds, akaAuthResp, akaAuthResLength,
            akaAuthAuts_ptr, &reg_ptr->lclConn, service_ptr->isEmergency,
            reg_ptr->shortenExpires);
    if (service_ptr->akaAuthRespSet) {
        /* Clear akaAuthRespSet flag */
        service_ptr->akaAuthRespSet = 0;
    }

    /* reset the shortenExpires */
    reg_ptr->shortenExpires = 0;
    switch (ret) {
        case SIP_BUSY:
        case SIP_FAILED:
            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);

            /*
             * Could not register the endpoint due to a reason with
             * the network interface.
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_RETRY);
            /*
             * Also tell the ISI layer that we are now deactivated due to
             * the failure.
             */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            return (SAPP_ERR);
        case SIP_BADPARM:
            /*
             * Then there's some bad parameters.  Tell the UA it's because
             * of mis-configuration by sending an event related to
             * authentication failure
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_BAD_SETTING);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            /* 
          * Also tell the ISI layer that we are now deactivated due to 
          * the failure.
          */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            return (SAPP_ERR);
        default:
            break;
    }
    return (SAPP_OK);
}

/*
 * ======== _SAPP_regProcessAKAChallege() ========
 * This function process the AKA challenge from sip
 *
 * Returns:
 *  SAPP_OK:  Successfully processed.
 *  SAPP_ERR: Failed
 */

vint _SAPP_regProcessAKAChallege(
    SAPP_ServiceObj  *service_ptr,
    SAPP_RegObj      *reg_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    uint8         rand[SIP_AUTH_AKA_RANDLEN];
    uint8         autn[SIP_AUTH_AKA_AUTNLEN];

    if (SAPP_ERR != SAPP_parseSecSrvHfValue(service_ptr, sipEvt_ptr)) {
        /* Parse and set ipsec parameters */
        if (SAPP_OK != _SAPP_ipsecParseSecurityServer(service_ptr)) {
            return (SAPP_ERR);
        }
    }
    else {
        /* No Security-Server hf or hf parsed failed, disable ipsec */
        service_ptr->sipConfig.useIpSec = 0;
    }

    /* Cache AKA version */
    if (SIP_RSP_CODE_AUTH_AKA_V1 == sipEvt_ptr->resp.respCode) {
        reg_ptr->akaVersion = 1;
    }
    else {
        reg_ptr->akaVersion = 2;
    }
    
    /* Decode nonce */
    if (SAPP_OK != _SAPP_sipDecodeNonce(sipEvt_ptr->resp.szReasonPhrase,
            rand, sizeof(rand), autn, sizeof(autn))) {
        /* Still need to send REGISTER with empty response */
        SAPP_dbgPrintf("%s %d: AKA response decode nonce failed\n",
                __FILE__, __LINE__);
    }
    else {
        /* Decode nonce successfully */
        if (service_ptr->useIsim) {
            /* Send AKA auth event to ISI */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            OSAL_memCpy(evt_ptr->isiMsg.msg.service.settings.akaAuthChallenge.rand,
                    rand, ISI_AKA_AUTH_RAND_STRING_SZ);
            OSAL_memCpy(evt_ptr->isiMsg.msg.service.settings.akaAuthChallenge.autn,
                    autn, ISI_AKA_AUTH_AUTN_STRING_SZ);
            SAPP_sendEvent(evt_ptr);

            /* Do not send event again */
            evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
            return (SAPP_OK);
        }
        else {
            /* No ISIM, calculate the response, ik and ck */
            if (SAPP_OK != _SAPP_ipsecCalculateAKAResp(service_ptr, rand, autn,
                        sip_ptr)) {
                /* Still need to send REGISTER with empty response */
                SAPP_dbgPrintf("%s %d: AKA response calculation failed\n",
                        __FILE__, __LINE__);
            }
            /* Set IPSec protected port and create SAs */
            if (SAPP_OK != _SAPP_ipsecSetProtectedPort(service_ptr, sip_ptr)) {
                SAPP_dbgPrintf("%s %d: Update protected port failed.\n",
                        __FILE__, __LINE__);
                /*
                 * Change back to default unprotected port and
                 * do not need to send register.
                 */
                _SAPP_ipsecSetDefaultPort(service_ptr, sip_ptr);
                return (SAPP_ERR);
            }
        }
    }

    /* Send REGISTER */
    if (SAPP_OK == _SAPP_reRegSend(reg_ptr, service_ptr, evt_ptr)) {
        /*
         * State is not change so set code to invalid to prevent sending
         * ISI msg again.
         */
        evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
    }
    else {
        /*
          * Select a different P-CSCF address.
          */
        _SAPP_regAdvancePcscf(service_ptr, sip_ptr);

        /* Break and send REG fail event to ISI */
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static vint _SAPP_regNotifyEvent(
    SAPP_ServiceObj   *service_ptr,
    tUaAppEvent       *sipEvt_ptr,
    SAPP_xmlRegStatus *status_ptr,
    vint              *numDev_ptr)
{
    char            *contentType_ptr;

    SAPP_dbgPrintf("%s: Received NOTIFY request 'reg' event package\n",
            __FUNCTION__);

    /* Check that there is a mandatory msg body */
    if (NULL == sipEvt_ptr->msgBody.payLoad.data ||
            0 == sipEvt_ptr->msgBody.payLoad.length) {
        /* 
         * Let's just return and not process this. Return ERROR
         * since technically we did process it for presence.
         */
        return (SAPP_ERR);
    }

    /* Parse the Content-Type and message body (XML doc) */
    if (NULL == (contentType_ptr = 
            SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, sipEvt_ptr))) {
        /* 
         * Then there is no content-type, hence no "content" to process
         * quitely ignore.
         */
        return (SAPP_ERR);
    }

    /* Check if it's "application/reginfo+xml" only */
    if (0 != OSAL_strncasecmp(contentType_ptr, _SAPP_REG_ACCEPT_ARG,
            sizeof(_SAPP_REG_ACCEPT_ARG) - 1)) {
        /* We don't understand, quietly ignore */
        return (SAPP_ERR);
    }

    /* 
    * Check the payload to get the registration status for this endpoint.
    */
    if (SAPP_OK != SAPP_xmlDecodeRegEventDoc(
            service_ptr->sipConfig.config.aor[0].szUri,
            service_ptr->sipConfig.config.szFqdn, 
            sipEvt_ptr->msgBody.payLoad.data,
            sipEvt_ptr->msgBody.payLoad.length,
            status_ptr, numDev_ptr,
            &service_ptr->registration.shortenExpires)) {
        /* Then there's no status so return error */
        return (SAPP_ERR);
    }
    
    /*
    * Return "OK" indicating that we have registration status
    * in status_ptr and the number of registered devices in numDev_ptr.
    */
    return (SAPP_OK);
}

/*
 * ======== _SAPP_regUpdateExternalAddr() ========
 * This function will examine REGISTER responses and see if out external address
 * info (Our NATTED address info) has changed.  If so, then the port in the 
 * service objects's localconn info is updated to reflect our NATTED 
 * address/port info.
 *
 * Returns:
 *  SAPP_ERR: No change in our NATTED address/port has been detected.
 *  SAPP_OK : Our NATTED address/port has changed.
 */
#ifdef SAPP_REG_USE_RPORT
static vint _SAPP_regUpdateExternalAddr(
    tUaAppEvent     *sipEvt_ptr,
    tLocalIpConn    *lclConn_ptr,
    tLocalIpConn    *newConn_ptr)
{
    int ret = SAPP_ERR;
    /*
     * We will attempt to use SIP to learn our external IP address and port
     * pairs via 'rport' port and 'received' address.  Let's check and see if
     * these values are different then what we think our SIP interface is.
     * If so, we will update and re-reg.
     */
    if (0 != sipEvt_ptr->resp.rport && 0 != sipEvt_ptr->resp.receivedAddr) {
        *newConn_ptr = *lclConn_ptr;
       if (lclConn_ptr->port != sipEvt_ptr->resp.rport) {
           newConn_ptr->port = sipEvt_ptr->resp.rport;
           ret = SAPP_OK;
       }
       if (lclConn_ptr->addr.ul != sipEvt_ptr->resp.receivedAddr) {
            newConn_ptr->addr.ul = sipEvt_ptr->resp.receivedAddr;
           ret = SAPP_OK;
       }
    }
    return (ret);
}
#endif
/*
 * ======== _SAPP_regUpdateServiceRoute() ========
 * This function will examine REGISTER responses and see if there are any
 * specified "Service Routes" that the registrar wants us to use.  
 * If so, then we cache the route(s) to use in future INVITEs, SUBSCRIBE's, and
 * MESSAGEs.
 *
 * Returns:
 *  Nothing
 */
static void _SAPP_regUpdateServiceRoute(
    SAPP_ServiceObj  *service_ptr,
    SAPP_RegObj   *reg_ptr,
    tUaAppEvent   *sipEvt_ptr)
{
    char *route_ptr;
    char *pos_ptr;
    char *pcscf_ptr;
    vint  hasPort;
    char  szPcscf[SIP_URI_STRING_MAX_SIZE];

    /*
     * TS 24.229. Add PCSCF URI in preconfigured route.
     */
    hasPort = 0;
    if (0 !=  reg_ptr->pcscfList[reg_ptr->pcscfIndex][0]) {
        /* Outbound proxy is not empty, consider it as PCSCF */
        pcscf_ptr = &reg_ptr->pcscfList[reg_ptr->pcscfIndex][0];
    }
    else {
        /* Outbound proxy is empty, consider proxy as PCSCF */
        pcscf_ptr = service_ptr->sipConfig.config.szProxy;
    }

    /* Check if "sip:" is included in the proxy string */
    if (NULL != (pos_ptr = OSAL_strncasescan(pcscf_ptr,
            SIP_URI_STRING_MAX_SIZE, SAPP_SIP_SCHEME))) {
        /* Advance off "sip:" */
        pos_ptr += OSAL_strlen(SAPP_SIP_SCHEME);
    }
    else {
        pos_ptr = pcscf_ptr;
    }

    /* Copy to local string for later process */
    OSAL_snprintf(szPcscf, SIP_URI_STRING_MAX_SIZE, "%s", pos_ptr);

    /* Get rid of the rest parameters */
    if (NULL != (pos_ptr = OSAL_strncasescan(szPcscf, SIP_URI_STRING_MAX_SIZE,
            ";"))) {
        *pos_ptr = '\0';
    }

    /*
     * Check if port is included in the proxy string.
     * If no, give default port 5060.
     */
    if (NULL != OSAL_strncasescan(szPcscf, SIP_URI_STRING_MAX_SIZE, ":")) {
        hasPort = 1;
    }

    /* 
    * Check if the event has any "Service-Route" header field info, 
    * if so then cache it.
    */
    if (NULL != (route_ptr = SAPP_parseHfValue(SAPP_SERVICE_ROUTE_HF,
            sipEvt_ptr))) {
        /* 
         * Then we have a "Service-Route", save it and PCSCF's sip uri
         * to use in other SIP requests
         */
        OSAL_snprintf(reg_ptr->preconfiguredRoute, SAPP_STRING_SZ, "<%s%s%s;%s>,%s",
                SAPP_SIP_SCHEME, szPcscf, hasPort? "": ":5060",
                SIP_LR_URI_PARM_STR, route_ptr);
    }
    else {
        /* Only add PCSCF's sip uri */
        OSAL_snprintf(reg_ptr->preconfiguredRoute, SAPP_STRING_SZ, "<%s%s%s;%s>",
                SAPP_SIP_SCHEME, szPcscf, hasPort? "": ":5060",
                SIP_LR_URI_PARM_STR);
    }

    return;
}

/* 
 * ======== SAPP_regInit() ========
 *
 * This function is used to initialize an object used to manage registration
 * state.  This function is typically called when ISI has instructed SAPP to 
 * create a service.
 *
 * Returns: 
 *   Nothing.
 */
void SAPP_regInit(
    SAPP_RegObj     *reg_ptr,
    uint32           reRegTimeoutSecs,
    uint32           natRefreshRateSecs,
    uint32           regRetryBaseTime,
    uint32           regRetryMaxTime,   
    vint             useRegEvt,
    char            *preconfigRoute_ptr,
    SAPP_ServiceObj *service_ptr,
    OSAL_MsgQId      tmrEvtQ)
{
    reg_ptr->state = SAPP_REG_STATE_NONE;
    reg_ptr->timeoutSecs = reRegTimeoutSecs;
    reg_ptr->natRefreshRateSecs = natRefreshRateSecs;
    reg_ptr->preconfiguredRoute[0] = 0;
    reg_ptr->useRegEvt = useRegEvt;
    if (NULL != preconfigRoute_ptr && 0 != preconfigRoute_ptr[0]) {
        OSAL_strncpy(reg_ptr->preconfiguredRoute, preconfigRoute_ptr,
                SAPP_STRING_SZ);
    }
    else {
        reg_ptr->preconfiguredRoute[0] = 0;
    }
    reg_ptr->regRetryBaseTime = regRetryBaseTime;
    reg_ptr->regRetryMaxTime = regRetryMaxTime;
    /* Create timer at init time */
    if (0 == (reg_ptr->regTmrId = OSAL_tmrCreate())) {
        SAPP_dbgPrintf("%s %d: Create registration retry timer failed. \n", 
                __FUNCTION__, __LINE__);
    }
    if (0 == (reg_ptr->regMsgTmrId = OSAL_tmrCreate())) {
        SAPP_dbgPrintf("%s %d: Create message sending retry timer failed. \n", 
                __FUNCTION__, __LINE__);
    }
    reg_ptr->regFailCount = 0;
    reg_ptr->pcscfIndex = 0;
    reg_ptr->tmrEvtQ = tmrEvtQ;
    reg_ptr->regFailCode = 0;
    reg_ptr->pcscfTrying[0] = 0;
    reg_ptr->subscription.dialogId = 0;
    reg_ptr->subscription.service_ptr = service_ptr;
    reg_ptr->subscription.timer.cnt = 0;
    reg_ptr->subscription.timer.id = 0;
    OSAL_memSet(&reg_ptr->lclConn, 0, sizeof(tLocalIpConn));
    return;
}

/* 
 * ======== SAPP_regDestroy() ========
 *
 * This function is used to destroy SAPP_RegObj
 *
 * Returns: 
 *   Nothing.
 */
void SAPP_regDestroy(
    SAPP_RegObj     *reg_ptr)
{
    /* Delete timer */
    if (0 != reg_ptr->regTmrId) {
        OSAL_tmrDelete(reg_ptr->regTmrId);
        reg_ptr->regTmrId = 0;
    }
    if (0 != reg_ptr->regMsgTmrId) {
        OSAL_tmrDelete(reg_ptr->regMsgTmrId);
        reg_ptr->regMsgTmrId = 0;
    }

    return;
}

/* 
 * ======== _SAPP_regSubNotEvent() ========
 * 
 * This function is the handler for SIP events that pertian to the "reg" event
 * package as described in RFC3680.  In other words this is the handler for all
 * SIP stack events related to the SUBSCRIBE/NOTIFY methods for the "reg" event 
 * package.
 *
 * Returns: 
 *  SAPP_ERR: The SIP event was not handled by this routine.  Further 
 *            processing of this SIP event should continue.
 *  SAPP_OK: The SIP event was handled by this routine and no further 
 *            processing of this SIP event is needed.
 */
static SAPP_regSmEvent _SAPP_regSubNotEvent(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    SAPP_SipObj     *sip_ptr)
{
    SAPP_xmlRegStatus status;
    vint              numDevices;
    SAPP_regSmEvent   event = REG_EVT_NONE;
    char             *reason_ptr;

    if (0 == sipEvt_ptr->header.hOwner) {
        /*
         * Then this definately doesn't belong to us.  This
         * can happen when we get NOTIFY's that don't belong to a dialog.
         * Like those for MWI on Asterisk.
         */
        return (event);
    }
    
    /* Check if this event belongs to the "reg" event package */
    if (sipEvt_ptr->header.hOwner != reg_ptr->subscription.dialogId) {
        /* This events doesn't belong to the "reg" event package */
        return (event);
    }
    
    switch (sipEvt_ptr->header.type) {
        case eUA_SUBSCRIBE_FAILED_NO_SUBS:
            if ((sipEvt_ptr->resp.respCode == SIP_RSP_SERVICE_UNAVAIL) && 
                    (SAPP_REG_STATE_ON == reg_ptr->state)) {
                if (SAPP_OK != _SAPP_regServiceUnavailable(reg_ptr, service_ptr,
                        sipEvt_ptr)) {
                    SAPP_dbgPrintf("%s:%d Fail to re-send subscribe\n", 
                            __FUNCTION__, __LINE__);
                    break;
                }
            }
            else {
                /* Then the subscription could not be installed.  Do nothing */
                reg_ptr->subscription.dialogId = 0;
            }
            break;
        case eUA_NOTIFY_EVENT_NO_SUBS:
            /*
             * Then our subscription to the registration package was
             * cancelled by the server! So let's find out why.
             */
            if (SAPP_OK == _SAPP_regNotifyEvent(service_ptr,
                    sipEvt_ptr, &status, &numDevices)) {
                /* Then there's more info! let's see what we have */
                if (status == SAPP_XML_REG_STATUS_REJECTED) {
                    /* 
                     * Then our registration was 'rejected' by the server.
                     * We must have problems with login credentials
                     */
                    event = REG_EVT_CANCELLED_AUTH;
                }
                else if (status == SAPP_XML_REG_STATUS_TERMINATED) {
                    /* 
                     * Then our registration was 'terminated'
                     * (cancelled) by the server, maybe we have multiple
                     * D2 devices logged into the network.
                     */
                    event = REG_EVT_CANCELLED;
                }
                else if (status == SAPP_XML_REG_STATUS_DEACTIVATED) {
                    event = REG_EVT_RESTART;
                }
            }

            /*
             * If there is no event set, or basically no REAL reason why our
             * subscription to the registration packege was cancelled, then
             * let's re-subscribe but not transition any state.
             */
            if (REG_EVT_NONE == event) {
                _SAPP_regSubscribe(reg_ptr, service_ptr);
            }
            break;
        case eUA_SUBSCRIBE_FAILED:
            /* See if got 504 server time out. */
            if (sipEvt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
                /* Server response 504 server time out. */
                if (SAPP_OK == _SAPP_sipServerTimeOut(service_ptr, &reason_ptr,
                        sipEvt_ptr, sip_ptr)) {
                    event = REG_EVT_NONE;
                }
            }
            /*
             * Then this belongs to a current subscription but
             * note that we don't care about eUA_SUBSCRIBE_FAILED because
             * we will also get a "eUA_SUBSCRIBE_FAILED_NO_SUBS" if the
             * subscription fails.
             */
            break;
        case eUA_SUBSCRIBE_COMPLETED:
            /*
             * Then this belongs to a current presence subscription but
             * note that we don't care about eUA_SUBSCRIBE_COMPLETED because
             * we assume that the subscription is successful from the
             * beginning and only look for e"UA_NO_MORE_SUBSCRIPTIONS" when
             * the subscription fails.
             */
            break;
        case eUA_NOTIFY_EVENT:
            /* 
             * Process the payload to find out the state of registration is
             * still valid.
             */
            if (SAPP_OK != _SAPP_regNotifyEvent(service_ptr,
                    sipEvt_ptr, &status, &numDevices)) {
                /* Then there's no registration status, so do nothing */
                break;
            }

            SAPP_dbgPrintf(
                    "%s: 'reg' rpt. Our status:%d Num of devices registered:%d\n",
                    __FUNCTION__, status, numDevices);

            /*
             * Then we have a report.  status will have our registration status
             * ("ON", "TERIMINATED", "REJECTED).  The numDev will be populated
             * with the number of registered contacts for this AOR
             */
            switch (status) {
                case SAPP_XML_REG_STATUS_TERMINATED:
                    event = REG_EVT_CANCELLED;
                    break;
                case SAPP_XML_REG_STATUS_REJECTED:
                    event = REG_EVT_CANCELLED_AUTH;
                    break;
                case SAPP_XML_REG_STATUS_ACTIVE_SHORTENED:
                    event = REG_EVT_RE_REG;
                    break;
                case SAPP_XML_REG_STATUS_DEACTIVATED:
                    event = REG_EVT_RESTART;
                    break;
                case SAPP_XML_REG_STATUS_ON:
                default:
                    break;
            }
            
            /*
             * The 'numDev' represents the number of registered contacts
             * for this endpoint.  Let's tell ISI about it via a
             * presence update.
             */
            _SAPP_regIsiPresenceEvt(0, service_ptr,
                    ISIP_PRES_REASON_PRESENCE, sipEvt_ptr->szToUri,
                    sipEvt_ptr->szRemoteUri, &evt_ptr->isiMsg);
            /* Let's encode the presence xml doc itself */
            SAPP_xmlEncodeRegEventDoc(
                    service_ptr->sipConfig.config.aor[0].szUri, numDevices,
                    evt_ptr->isiMsg.msg.presence.presence,
                    ISI_PRESENCE_STRING_SZ);
            break;
        default:
            break;
    }
    return (event);
}

static SAPP_RegState _SAPP_regNoneState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
    switch (cmd) {
        case REG_EVT_START:
            if (0 == service_ptr->sipConfig.config.szRegistrarProxy[0]) {
                /*
                 * Then the user has no register set.  This is possible.
                 * The user may just want to use the UA as a peer-to-peer with
                 * no registrar. Since this is a possible configuration, then
                 * just immediately return a 'successful' event to ISI.
                 */
                SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_ACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
                state = SAPP_REG_STATE_ON;
                /* Process emergency registration complete */
                if (service_ptr->isEmergency) {
                    SAPP_emgcyRegComplete(service_ptr, sip_ptr, NULL, 0);
                    _SAPP_regUpdateServiceRoute(service_ptr, reg_ptr, NULL);
                }
                /* Get uri for App Isi event */
                OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "URI:\"%s\"",
                        service_ptr->sipConfig.config.aor[0].szUri);

                /*
                 * Get Register info, IPSec and route from MSAPP.
                 * This routine will be empty for SAPP in modem processor
                 * or non-4G+.
                 */
                SR_getRegSecData(service_ptr->registration.preconfiguredRoute,
                        SAPP_STRING_SZ, &service_ptr->secSrvHfs[0][0],
                        SAPP_SEC_SRV_HF_MAX_NUM, SAPP_STRING_SZ,
                        &service_ptr->ipsecObj.pPortUS,
                        &service_ptr->ipsecObj.pPortUC,
                        &service_ptr->ipsecObj.inboundSAc,
                        &service_ptr->ipsecObj.inboundSAs,
                        &service_ptr->ipsecObj.outboundSAc);
                /* Reset IPSec protected port and bind them again */
                if (service_ptr->sipConfig.useIpSec) {
                    _SAPP_ipsecResetPort(service_ptr, sip_ptr,
                            service_ptr->ipsecObj.pPortUC,
                            service_ptr->ipsecObj.pPortUS);
                }
                return state;
            }

            /* Update any new new connection information */
            reg_ptr->lclConn = service_ptr->sipConfig.localConn;

            /* Come out of the 'none' state to 'off' */
            state = SAPP_REG_STATE_OFF;
            
            if (SAPP_OK == _SAPP_checkLocalConnValidity(
                    reg_ptr, service_ptr, evt_ptr)) {
               /*
                * Reset any preconfigured routes.
                * We will see if this registration specifies any routes.
                */
                reg_ptr->preconfiguredRoute[0] = 0;
                if (SAPP_OK == _SAPP_regSend(reg_ptr, service_ptr, evt_ptr)) {
                    /* All is okay, set the registration as on */
                    state = SAPP_REG_STATE_TRYING;
                }
                else {
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                }
            }
            break;
        case REG_EVT_COMPLETED:
        case REG_EVT_FAILED:
        case REG_EVT_UNREG_DONE:
            break;
        case REG_EVT_CANCELLED:
        case REG_EVT_CANCELLED_AUTH:
        case REG_EVT_NO_NET:
        case REG_EVT_RESET:
        case REG_EVT_RESTART:
        case REG_EVT_STOP:
        default:
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regNoNetState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
    switch (cmd) {
        case REG_EVT_YES_NET:
            /* Update any new new connection information */
            reg_ptr->lclConn = service_ptr->sipConfig.localConn;

            /* Come out of the 'no net' state to 'off' */
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_START:
        case REG_EVT_COMPLETED:
        case REG_EVT_FAILED:
        case REG_EVT_CANCELLED:
        case REG_EVT_CANCELLED_AUTH:
        case REG_EVT_NO_NET:
        case REG_EVT_RESET:
        case REG_EVT_RESTART:
        case REG_EVT_STOP:
        case REG_EVT_UNREG_DONE:
        default:
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regOffState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
    switch (cmd) {
        case REG_EVT_RESTART:
        case REG_EVT_START:
            if (0 == service_ptr->sipConfig.config.szRegistrarProxy[0]) {
                /*
                 * Then the user has no register set.  This is possible.
                 * The user may just want to use the UA as a peer-to-peer with
                 * no registrar. Since this is a possible configuration, then
                 * just immediately return a 'successful' event to ISI.
                 */
                SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_ACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
                state = SAPP_REG_STATE_ON;
                /* Process emergency registration complete */
                if (service_ptr->isEmergency) {
                    SAPP_emgcyRegComplete(service_ptr, sip_ptr, NULL, 0);
                }
                /*
                 * Get Register info, IPSec and route from MSAPP.
                 * This routine will be empty for SAPP in modem processor
                 * or non-4G+.
                 */
                SR_getRegSecData(service_ptr->registration.preconfiguredRoute,
                        SAPP_STRING_SZ, &service_ptr->secSrvHfs[0][0],
                        SAPP_SEC_SRV_HF_MAX_NUM, SAPP_STRING_SZ,
                        &service_ptr->ipsecObj.pPortUS,
                        &service_ptr->ipsecObj.pPortUC,
                        &service_ptr->ipsecObj.inboundSAc,
                        &service_ptr->ipsecObj.inboundSAs,
                        &service_ptr->ipsecObj.outboundSAc);
                return state;
            }
            /* Update any new new connection information */
            reg_ptr->lclConn = service_ptr->sipConfig.localConn;
            
            if (SAPP_OK == _SAPP_checkLocalConnValidity(
                    reg_ptr, service_ptr, evt_ptr)) {
               /*
                * Reset any preconfigured routes.
                * We will see if this registration specifies any routes.
                */
                reg_ptr->preconfiguredRoute[0] = 0;
                /*
                 * When receiving a 305 (Use Proxy) response to
                 * the unprotected REGISTER request, to find
                 * another P-CSCF address which is different from
                 * previous failure one when receiving
                 * a 305 (Use Proxy) response.
                 */
                if (SIP_RSP_USE_PROXY ==
                        service_ptr->registration.regFailCode) {
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                }
                if (SAPP_OK == _SAPP_regSend(reg_ptr, service_ptr, evt_ptr)) {
                    /* All is okay, set the registration as on */
                    state = SAPP_REG_STATE_TRYING;
                }
                else {
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                }
            }
            break;
        case REG_EVT_STOP:
            state = SAPP_REG_STATE_NONE;
            break;
        case REG_EVT_NO_NET:
            /* Nothing to do in SIP/SAPP, but let's tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_NO_DATA_CONN);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);
            /* Send another event up for deactivate */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            break;
        case REG_EVT_COMPLETED:
        case REG_EVT_FAILED:
        case REG_EVT_CANCELLED:
        case REG_EVT_CANCELLED_AUTH:
        case REG_EVT_RESET:
        case REG_EVT_UNREG_DONE:
        default:
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regTryingState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
#ifdef SAPP_REG_USE_RPORT
    LocalIpConn  nattedConn;
#endif
    char         *lastPath_ptr;
    char         pauFirstUri[SAPP_STRING_SZ];
    int          ret;

    switch (cmd) {
        case REG_EVT_COMPLETED:
            _SAPP_regUpdateServiceRoute(service_ptr, reg_ptr, sipEvt_ptr);
            /* Update our NATTED address/port if our NATTED info changed */
#ifdef SAPP_REG_USE_RPORT
            if (SAPP_OK == _SAPP_regUpdateExternalAddr(sipEvt_ptr,
                    &reg_ptr->lclConn, &nattedConn)) {
                /* Then we need to re-reg with new info */
                if (SIP_OK == UA_UnRegister(service_ptr->sipConfig.uaId,
                        NULL, NULL, 0, &reg_ptr->lclConn)) {
                    state = SAPP_REG_STATE_RE_REG;
                    /* Let's use the new values now!*/
                    reg_ptr->lclConn = nattedConn;
                    service_ptr->sipConfig.localConn = nattedConn;
                    break;
                }
                /*
                 * If we are here then we could not send the UnRegister.
                 * Let's go to 'OFF' and retry later.  Tell ISI.
                 */

                /* Since we now the NATTED address let's save it */
                reg_ptr->lclConn = nattedConn;
                service_ptr->sipConfig.localConn = nattedConn;

                OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_RETRY);
                SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
                state = SAPP_REG_STATE_OFF;
                break;
            }
#endif
            /* Parse P-Associated-URI to get alias URI. */
            ret = SAPP_parsePauHfValue(service_ptr, sipEvt_ptr, pauFirstUri);
            if (ret > 0) {
                /*
                 * Then we have alias URI.
                 * Report the first one for now and the primary URI to app.
                 * Primary URI is PAU's first element, if received PAU.
                 * TODO: It may call UA_Modify to set aor.
                 */
                OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "%s:\"%s\",URI:\"%s\"",
                        SAPP_ALIAS_URI_STR,
                        service_ptr->aliasUriList[0],
                        pauFirstUri);
            }
            else if (ret == 0) {
                /*
                 * No alias URI, but with URI from PAU.
                 * Report the PAU's first URI to app.
                 */
                OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "URI:\"%s\"",
                        pauFirstUri);
            }
            else {
                /* No alias URI, just report the primary URI to app. */
                OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "URI:\"%s\"",
                        service_ptr->sipConfig.config.aor[0].szUri);
            }

            /*
             * If there is public gruu in contact and instance Id is not NULL,
             * update UA and modify the contact
             */
            if ((0 != service_ptr->instanceId[0]) &&
                    (0 != sipEvt_ptr->szPublicGruu[0])) {
                UA_Modify(service_ptr->sipConfig.uaId, NULL, 0, 0, NULL,
                        sipEvt_ptr->szPublicGruu, NULL, NULL, NULL, 20, 0);
            }

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_ACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            /* If emergency registeration ,do not need subscribe */
            if (0 != reg_ptr->useRegEvt && !service_ptr->isEmergency) {
                _SAPP_regSubscribe(reg_ptr, service_ptr);
            }
            /* send MWI SUBSCRIBE after register successfully */
            if (0 != service_ptr->mwiObj.useMwiEvt) {
                SAPP_mwiSubscribe(&service_ptr->mwiObj, service_ptr);
            }

            /*
             * Look for the last entry in Path header field and store it for 
             * later use.
             */
            if (NULL != (lastPath_ptr = SAPP_parsePathHfValue(sipEvt_ptr))) {
                OSAL_strcpy(service_ptr->lastPathUri, lastPath_ptr);
            }
            state = SAPP_REG_STATE_ON;
            /* Stop the registration retry timer and reset regFailCount */
            _SAPP_regStopRegTmr(service_ptr);
            reg_ptr->regFailCount = 0;

            /* Send ISI event for registration completed */
            SAPP_sendEvent(evt_ptr);
            /* Reset event for next time */
            evt_ptr->isiMsg.code = ISIP_CODE_INVALID;

            /* Update call if there call active for IP to IP handover*/
            SAPP_radioUpdateCall(sip_ptr, service_ptr, evt_ptr);
            if (service_ptr->isEmergency) {
                SAPP_emgcyRegComplete(service_ptr, sip_ptr, NULL,
                        sipEvt_ptr->expires);
            }

            /*
             * Notify register info, IPSec and route to SAPP in application
             * processor.
             * This routine will be empty for SAPP in application processor
             * or non-4G+.
             */
            SR_setRegSecData(service_ptr->registration.preconfiguredRoute,
                    &service_ptr->secSrvHfs[0][0],
                    SAPP_SEC_SRV_HF_MAX_NUM, SAPP_STRING_SZ,
                    &service_ptr->ipsecObj.inboundSAc,
                    &service_ptr->ipsecObj.inboundSAs,
                    &service_ptr->ipsecObj.outboundSAc);

            break;
        case REG_EVT_FAILED:
            /* Start the registration retry timer */
            _SAPP_regStartRetryTmr(service_ptr, sipEvt_ptr);
            service_ptr->registration.regFailCode = sipEvt_ptr->resp.respCode;
            /* If it's a AUTH error then send an additional message */
            switch (sipEvt_ptr->resp.respCode) {
                case SIP_RSP_CODE_AUTH_AKA_V1:
                case SIP_RSP_CODE_AUTH_AKA_V2:
                    /* AKAv1/2 is required. */
                    if (SAPP_OK == _SAPP_regProcessAKAChallege(
                            service_ptr, reg_ptr, sipEvt_ptr,
                            evt_ptr, sip_ptr)) {
                        /* Stay in this state. */
                        return (state);
                    }
                    else {
                        state = SAPP_REG_STATE_OFF;
                    }
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case 401:
                case 407:
                case 403:
                case 404:
                case 402:
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case SIP_RSP_USE_PROXY:
                    _SAPP_regStopRegTmr(service_ptr);
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    break;
                case SIP_RSP_CODE_XACT_TIMEOUT:
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                    /* Fall through */
                default:
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    break;
            }
            if (service_ptr->isEmergency) {
                SAPP_emgcyRegFailed(service_ptr, sip_ptr, NULL);
            }

            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);

            /* Send another ISI event indicating the DEACTIVATION * */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            /* Reset the state */
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_STOP:
            UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                    &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_ACCT_DISABLED);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_NONE;
            break;
        case REG_EVT_CANCELLED:
            UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                    &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_LOGGED_IN);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_CANCELLED_AUTH:
            UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                    &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", 403, _SAPP_REG_REJECTED);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_NO_NET:
            /* 
             * Bug 7442  : remove unregister
             */
            UA_CancelRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                   &reg_ptr->lclConn);
             /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_NO_DATA_CONN);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);
            /* Send another event up for deactivate */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_RE_REG:
            if (!service_ptr->akaAuthRespSet) {
                /*
                 * Authenticate response is not set. It's not re-register
                 * with AKA resp.
                 */
                break;
            }

            /* Must be retry register with AKA auth response */
            if (SAPP_OK == _SAPP_reRegSend(reg_ptr, service_ptr, evt_ptr)) {
                state = SAPP_REG_STATE_TRYING;
                /*
                 * State is not change so set code to invalid to prevent
                 *sending ISI msg again
                 */
                evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
            }
            else {
                /*
                  * Select a different P-CSCF address.
                  */
                _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                   
                state = SAPP_REG_STATE_OFF;
            }

            break;
        case REG_EVT_START:
            /*
             * If receive REG_EVT_START in trying,
             * send a new registration again.
             */
        case REG_EVT_RESTART:
            /*
             * There is a NIC error and trasnport is re-inited.
             * Let's tell ISI and start a retry timer. Fall through.
             */
        case REG_EVT_RESET:
            /* Nic error and transport initilization failed. */
            UA_CancelRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                   &reg_ptr->lclConn);

            state = SAPP_REG_STATE_OFF;
            /* To start a new registration. */
            _SAPP_regRetryTimerCb(service_ptr);
            break;
        case REG_EVT_UNREG_DONE:
        default:
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regOnState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    OSAL_NetAddress addr;
    SAPP_RegState   state = SAPP_REG_STATE_LAST;
#ifdef SAPP_REG_USE_RPORT
    tLocalIpConn    nattedConn;
#endif

    switch (cmd) {
        case REG_EVT_CANCELLED:
            UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                    &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Kill any active subscriptions to the "reg" event if they exist */
            _SAPP_regUnsubscribe(reg_ptr, service_ptr);
            /* send MWI UNSUBSCRIBE */
            if (0 != service_ptr->mwiObj.useMwiEvt) {
                SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
            }
            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_LOGGED_IN);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_CANCELLED_AUTH:
            UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                    &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Kill any active subscriptions to the "reg" event if they exist */
            _SAPP_regUnsubscribe(reg_ptr, service_ptr);
            /* send MWI UNSUBSCRIBE */
            if (0 != service_ptr->mwiObj.useMwiEvt) {
                SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
            }
            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", 403, _SAPP_REG_REJECTED);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_RESET:
            /*
             * There is a NIC error and trasnport initialization failed.
             * This would NOT de-register registration and clear the 
             * registration record.
             */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);
            /* Cancel existing registration. */
            UA_CancelRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                   &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Kill any active subscriptions to the "reg" event if they exist */
            _SAPP_regUnsubscribe(reg_ptr, service_ptr);
            /* send MWI UNSUBSCRIBE */
            if (0 != service_ptr->mwiObj.useMwiEvt) {
                SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
            }
            state = SAPP_REG_STATE_OFF; 
            _SAPP_regStartRetryTmr(service_ptr, NULL);
            break;
        case REG_EVT_RESTART:
            /*
             * There is a NIC error and trasnport is re-inited.
             * Send deactivate event.
             * This would de-register registration.
             */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc, 
                    ISI_EVENT_DESC_STRING_SZ, 
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_NO_DATA_CONN); 
            SAPP_serviceIsiEvt(service_ptr->isiServiceId, 
                    service_ptr->protocolId, 
                    ISIP_SERVICE_REASON_DEACTIVATE, 
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg); 
            /* De-register previous registration. */
            if (SIP_OK != UA_UnRegister(service_ptr->sipConfig.uaId, NULL,
                    NULL, 0, &reg_ptr->lclConn)) {
                /* Failed to de-register. */
                state = SAPP_REG_STATE_OFF; 
                /* Restore the exact port we bound. */
                IMS_NET_GET_SOCKET_ADDRESS(&service_ptr->sipInfcFd, &addr);
                service_ptr->registration.lclConn.addr.port =
                        OSAL_netNtohl(addr.port);
                /* Start the registration retry timer */ 
                _SAPP_regStartRetryTmr(service_ptr, NULL); 
                break;
            }

            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Kill any active subscriptions to the "reg" event if they exist */
            _SAPP_regUnsubscribe(reg_ptr, service_ptr);
            /* send MWI UNSUBSCRIBE */
            if (0 != service_ptr->mwiObj.useMwiEvt) {
                SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
            }

            /* Restore the exact port we bound. */
            IMS_NET_GET_SOCKET_ADDRESS(&service_ptr->sipInfcFd, &addr);
            service_ptr->registration.lclConn.addr.port =
                    OSAL_netNtohl(addr.port);
            /* Let restart. */
            state = SAPP_REG_STATE_RESTART;
            break;
        case REG_EVT_STOP:
            /* no need to do de-register processes when it's not really registered. */
            if (0 == service_ptr->sipConfig.config.szRegistrarProxy[0]) {
                /* Tell ISI */
                SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
                state = SAPP_REG_STATE_OFF;
            }
            else {
                /* Terminate all calls */
                SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);
                UA_UnRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                        &reg_ptr->lclConn);
                /* Clear any preconfigured route */
                reg_ptr->preconfiguredRoute[0] = 0;
                /* Kill any active subscriptions to the "reg" event if they exist */
                _SAPP_regUnsubscribe(reg_ptr, service_ptr);
                /* send MWI UNSUBSCRIBE */
                if (0 != service_ptr->mwiObj.useMwiEvt) {
                    SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
                }

                /* State de-register timer. */
                _SAPP_regStartDeRegTmr(service_ptr, NULL);

                /* Don't tell ISI now, send it when de-regsitration done. */
                state = SAPP_REG_STATE_DE_REG_TRYING;
            }
            break;
        case REG_EVT_FAILED:
            /* Start the registration retry timer */
            _SAPP_regStartRetryTmr(service_ptr, sipEvt_ptr);
            _SAPP_regGetReasonDesc(sipEvt_ptr, &evt_ptr->isiMsg.msg.service);
            service_ptr->registration.regFailCode = sipEvt_ptr->resp.respCode;

            /* If it's a AUTH error then send an additional message */
            switch (sipEvt_ptr->resp.respCode) {
                case SIP_RSP_CODE_AUTH_AKA_V1:
                case SIP_RSP_CODE_AUTH_AKA_V2:
                    /* AKAv1/2 is required. */
                    if (SAPP_OK == _SAPP_regProcessAKAChallege(
                            service_ptr, reg_ptr, sipEvt_ptr,
                            evt_ptr, sip_ptr)) {
                        _SAPP_regStopRegTmr(service_ptr);
                        /* Stay in this state. */
                        return (state);
                    }
                    else {
                        state = SAPP_REG_STATE_OFF;
                    }
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case 401:
                case 407:
                case 403:
                case 404:
                case 402:
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case SIP_RSP_USE_PROXY:
                    _SAPP_regStopRegTmr(service_ptr);
                    break;
                case SIP_RSP_CODE_XACT_TIMEOUT:
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                    break; 
                default:
                    break;
            }
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;
            /* Kill any active subscriptions to the "reg" event if they exist */
            _SAPP_regUnsubscribe(reg_ptr, service_ptr);
            /* send MWI UNSUBSCRIBE */
            if (0 != service_ptr->mwiObj.useMwiEvt) {
                SAPP_mwiUnsubscribe(&service_ptr->mwiObj, service_ptr);
            }

            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);
            /* Send another ISI event indicating the DEACTIVATION */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            /* Reset the state */
            state = SAPP_REG_STATE_OFF;

            /* Clear all IPSEC SAs if necessary */
            _SAPP_ipsecClearSAs(service_ptr, evt_ptr);
            break;
        case REG_EVT_NO_NET:
            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);

            /* 
             * Bug 7442  : remove unsubscribe too
             */
            /* Kill any active subscriptions to the "reg" event if they exist */
            //_SAPP_regUnsubscribe(reg_ptr, service_ptr);

            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_NO_DATA_CONN);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                        ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                        &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);
            break;
        case REG_EVT_COMPLETED:
            if (service_ptr->isEmergency) {
                SAPP_emgcyRegComplete(service_ptr, sip_ptr, NULL, sipEvt_ptr->expires);
            }
 #ifdef SAPP_REG_USE_RPORT
            if (SAPP_OK == _SAPP_regUpdateExternalAddr(sipEvt_ptr,
                    &reg_ptr->lclConn, &nattedConn)) {
                /* Then we need to re-reg with new info */
                if (SIP_OK == UA_UnRegister(service_ptr->sipConfig.uaId,
                        NULL, NULL, 0, &reg_ptr->lclConn)) {
                    state = SAPP_REG_STATE_RE_REG;
                    /* Let's use the new values now!*/
                    reg_ptr->lclConn = nattedConn;
                    service_ptr->sipConfig.localConn = nattedConn;
                    break;
                }
                /*
                 * If we are here then we could not send the UnRegister.
                 * Let's go to 'OFF' and retry later.  Tell ISI.
                 */

                /* Since we now the NATTED address let's save it */
                reg_ptr->lclConn = nattedConn;
                service_ptr->sipConfig.localConn = nattedConn;

                OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                         ISI_EVENT_DESC_STRING_SZ,
                        "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                        _SAPP_REG_RETRY);
                SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                        &evt_ptr->isiMsg);
                state = SAPP_REG_STATE_OFF;
                break;
            }
#endif
            break;
        case REG_EVT_START:
        case REG_EVT_RE_REG:
            /* Must be retry register */
            if (SAPP_OK == _SAPP_reRegSend(reg_ptr, service_ptr, evt_ptr)) {
                state = SAPP_REG_STATE_ON;
                /* State is not change so set code to invalid to prevent sending ISI msg again */
                evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
            }
            else {
                /*
                 * Select a different P-CSCF address.
                 */
                _SAPP_regAdvancePcscf(service_ptr, sip_ptr);

                state = SAPP_REG_STATE_OFF;
            }
            break;
        case REG_EVT_YES_NET:
            /* Update any new new connection information */
            reg_ptr->lclConn = service_ptr->sipConfig.localConn;
            break;
        case REG_EVT_UNREG_DONE:
        default:
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regReRegState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
    switch (cmd) {
        case REG_EVT_UNREG_DONE:
            /* Then let's reg with new setting */
            if (SAPP_OK != _SAPP_regSend(reg_ptr, service_ptr, evt_ptr)) {
                /* Then we are off */
                state = SAPP_REG_STATE_OFF;
            }
            else {
                state = SAPP_REG_STATE_TRYING;
            }
            break;
        case REG_EVT_STOP:
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;

            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_ACCT_DISABLED);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);

            state = SAPP_REG_STATE_NONE;
            break;
        case REG_EVT_CANCELLED:
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;

            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_LOGGED_IN);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);

            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_CANCELLED_AUTH:
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;

            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", 403, _SAPP_REG_REJECTED);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);

            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);

            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_RESTART:
            /*
             * There is a NIC error and trasnport is re-inited.
             * Let's register again later. Fall trhought intentionally.
             */
        case REG_EVT_RESET:
            /* Nic error and transport initilization failed. */

            /* Cancel existing registration. */
            UA_CancelRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                   &reg_ptr->lclConn);
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;

            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_NO_DATA_CONN);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);

            state = SAPP_REG_STATE_OFF;
            /* Start the registration retry timer */
            _SAPP_regStartRetryTmr(service_ptr, NULL);
            break;
        case REG_EVT_NO_NET:
            /* Clear any preconfigured route */
            reg_ptr->preconfiguredRoute[0] = 0;

            /* Tell ISI */
            OSAL_snprintf(evt_ptr->isiMsg.msg.service.reasonDesc,
                    ISI_EVENT_DESC_STRING_SZ,
                    "REG FAILED: CODE:%d REASON:%s", ISI_NO_NETWORK_AVAILABLE,
                    _SAPP_REG_NO_DATA_CONN);
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                    &evt_ptr->isiMsg);
            SAPP_sendEvent(evt_ptr);
            /* Send another event up for deactivate */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);

            state = SAPP_REG_STATE_OFF;
            break;
        case REG_EVT_START:
        case REG_EVT_FAILED:
        case REG_EVT_COMPLETED:
        default:
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regDeRegTryingState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
#ifdef SAPP_REG_USE_RPORT
    tLocalIpConn  nattedConn;
#endif

    switch (cmd) {
        case REG_EVT_START:
            if (0 != service_ptr->akaAuthRespSet) {
                if (SAPP_OK == _SAPP_reRegSend(reg_ptr, service_ptr, evt_ptr)) {
                    break;
                }
            }
            /* Fall through. */
        case REG_EVT_STOP:
            /* De-Reg times out, fall through. */
        case REG_EVT_COMPLETED:
            /* Stop de-reg timer. */
            _SAPP_regStopRegTmr(service_ptr);
            /* Cancel registration which cancel retransmission o UDP. */
            UA_CancelRegister(service_ptr->sipConfig.uaId, NULL, NULL, 0,
                   &reg_ptr->lclConn);
            /* Clean transport. */
            SAPP_sipServiceTransportClean(service_ptr, sip_ptr);
            /* Send another ISI event indicating the DEACTIVATION * */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            state = SAPP_REG_STATE_OFF;
            break;

       case REG_EVT_FAILED:
           /* If it's a AUTH error then send an additional message */
           switch (sipEvt_ptr->resp.respCode) {
               case SIP_RSP_CODE_AUTH_AKA_V1:
                case SIP_RSP_CODE_AUTH_AKA_V2:
                    /* AKAv1/2 is required. */
                    if (SAPP_OK == _SAPP_regProcessAKAChallege(
                            service_ptr, reg_ptr, sipEvt_ptr,
                            evt_ptr, sip_ptr)) {
                        /* Stay in this state. */
                        return (state);
                    }
                    else {
                        state = SAPP_REG_STATE_OFF;
                    }
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case 401:
                case 407:
                case 403:
                case 404:
                case 402:
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                default:
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    break;
            }
            /* Stop de-reg timer. */
            _SAPP_regStopRegTmr(service_ptr);

            /* ISI event indicating the DEACTIVATION * */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);

            /* Reset the state */
            state = SAPP_REG_STATE_OFF;
            break;
        default:
            /* Should not be here. */
            SAPP_dbgPrintf("Receive %s in DeRegTryingState\n",
                    SAPP_regCmdStr[cmd]);
            /* ISI event indicating the DEACTIVATION * */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);

            /* Reset the state */
            state = SAPP_REG_STATE_OFF;
            break;
    }
    return state;
}

static SAPP_RegState _SAPP_regRestartState(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
#ifdef SAPP_REG_USE_RPORT
    tLocalIpConn  nattedConn;
#endif

    switch (cmd) {
        case REG_EVT_COMPLETED:
            /* We've de-registered. Register again. */

            /* Update any new new connection information */
            reg_ptr->lclConn = service_ptr->sipConfig.localConn;

            if (SAPP_OK == _SAPP_checkLocalConnValidity(
                    reg_ptr, service_ptr, evt_ptr)) {
               /*
                * Reset any preconfigured routes.
                * We will see if this registration specifies any routes.
                */
                reg_ptr->preconfiguredRoute[0] = 0;
                if (SAPP_OK == _SAPP_regSend(reg_ptr, service_ptr, evt_ptr)) {
                    /* All is okay, set the registration as on */
                    state = SAPP_REG_STATE_TRYING;
                }
                else {
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                }
            }
            else {
                state = SAPP_REG_STATE_OFF;
                /* Start retry timer. */
                _SAPP_regStartRetryTmr(service_ptr, NULL);
            }
            break;
       case REG_EVT_FAILED:
            _SAPP_regStartRetryTmr(service_ptr, sipEvt_ptr);
           /* If it's a AUTH error then send an additional message */
           switch (sipEvt_ptr->resp.respCode) {
               case SIP_RSP_CODE_AUTH_AKA_V1:
               case SIP_RSP_CODE_AUTH_AKA_V2:
                    /* AKAv1/2 is required. */
                    if (SAPP_OK == _SAPP_regProcessAKAChallege(
                            service_ptr, reg_ptr, sipEvt_ptr,
                            evt_ptr, sip_ptr)) {
                        /* Stay in this state. */
                        return (state);
                    }
                    else {
                        state = SAPP_REG_STATE_OFF;
                    }
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case 401:
                case 407:
                case 403:
                case 404:
                case 402:
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_AUTH, ISIP_STATUS_FAILED,
                            &evt_ptr->isiMsg);
                    SAPP_sendEvent(evt_ptr);
                    break;
                case SIP_RSP_USE_PROXY:
                    _SAPP_regStopRegTmr(service_ptr);
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    break;
                case SIP_RSP_CODE_XACT_TIMEOUT:
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                    /* Fall through */
                default:
                    _SAPP_regGetReasonDesc(sipEvt_ptr,
                            &evt_ptr->isiMsg.msg.service);
                    break;
            }

            /* Terminate all calls */
            SAPP_sipTerminateAllCalls(service_ptr, evt_ptr);

            /* Send another ISI event indicating the DEACTIVATION * */
            SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    ISIP_SERVICE_REASON_DEACTIVATE,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);

            /* Reset the state */
            state = SAPP_REG_STATE_OFF;
            /* Try to register.*/
            if (SAPP_OK == _SAPP_checkLocalConnValidity(
                    reg_ptr, service_ptr, evt_ptr)) {
               /*
                * Reset any preconfigured routes.
                * We will see if this registration specifies any routes.
                */
                reg_ptr->preconfiguredRoute[0] = 0;
                if (SAPP_OK == _SAPP_regSend(reg_ptr, service_ptr, evt_ptr)) {
                    /* All is okay, set the registration as on */
                    state = SAPP_REG_STATE_TRYING;
                }
                else {
                    /*
                     * Select a different P-CSCF address.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                }
            }
            break;
        case REG_EVT_START:
            if (SAPP_OK == _SAPP_deRegSend(reg_ptr, service_ptr, evt_ptr)) {
                /* State is not change so set code to invalid to prevent sending ISI msg again */
                evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
            }
            break;
        case REG_EVT_STOP:
            if (SAPP_OK == _SAPP_deRegSend(reg_ptr, service_ptr, evt_ptr)) {
                /* State is not change so set code to invalid to prevent sending ISI msg again */
                evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
            }
            break;
        case REG_EVT_CANCELLED:
        case REG_EVT_CANCELLED_AUTH:
        case REG_EVT_NO_NET:
        case REG_EVT_RESTART:
        case REG_EVT_UNREG_DONE:
        case REG_EVT_RESET:
            break;
        default:
            break;
    }
    return state;
}

static vint _SAPP_regStateMachine(
    SAPP_regSmEvent   cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    tUaAppEvent      *sipEvt_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    SAPP_RegState state = SAPP_REG_STATE_LAST;
    SAPP_dbgPrintf("%s: SM Begin state:%s cmd:%s\n", __FUNCTION__,
            SAPP_regStateStr[reg_ptr->state], SAPP_regCmdStr[cmd]);
    switch (reg_ptr->state) {
        case SAPP_REG_STATE_OFF:
            state = _SAPP_regOffState(cmd, reg_ptr, service_ptr, evt_ptr,
                    sip_ptr);
            break;
        case SAPP_REG_STATE_TRYING:
            state = _SAPP_regTryingState(cmd, reg_ptr, service_ptr, sipEvt_ptr,
                    evt_ptr, sip_ptr);
            break;
        case SAPP_REG_STATE_RE_REG:
            state = _SAPP_regReRegState(cmd, reg_ptr, service_ptr, sipEvt_ptr,
                    evt_ptr, sip_ptr);
            break;
        case SAPP_REG_STATE_ON:
            state = _SAPP_regOnState(cmd, reg_ptr, service_ptr, sipEvt_ptr,
                    evt_ptr, sip_ptr);
            break;
        case SAPP_REG_STATE_NONE:
            state = _SAPP_regNoneState(cmd, reg_ptr, service_ptr, evt_ptr,
                    sip_ptr);
            break;
        case SAPP_REG_STATE_NO_NET:
            state = _SAPP_regNoNetState(cmd, reg_ptr, service_ptr, evt_ptr);
            break;
        case SAPP_REG_STATE_DE_REG_TRYING:
            state = _SAPP_regDeRegTryingState(cmd, reg_ptr, service_ptr,
                    sipEvt_ptr, evt_ptr, sip_ptr);
            break;
        case SAPP_REG_STATE_RESTART:
            state = _SAPP_regRestartState(cmd, reg_ptr, service_ptr, sipEvt_ptr,
                    evt_ptr, sip_ptr);
            break;
        default:
            break;
    }
    if (state != SAPP_REG_STATE_LAST && state != reg_ptr->state) {
        reg_ptr->state = state;
        /* There a state change */
        SAPP_dbgPrintf("%s: SM End state:%s \n", __FUNCTION__,
                SAPP_regStateStr[reg_ptr->state]);
        return (SAPP_OK);
    }
    SAPP_dbgPrintf("%s: SM End state:%s \n", __FUNCTION__,
                SAPP_regStateStr[reg_ptr->state]);
    return (SAPP_ERR);
}

void SAPP_regStart(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_START, reg_ptr, service_ptr, NULL, evt_ptr,
            sip_ptr);
    return;
}

void SAPP_regStop(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_STOP, reg_ptr, service_ptr, NULL, evt_ptr,
            sip_ptr);
    return;
}

void SAPP_regRestart(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_RESTART, reg_ptr, service_ptr, NULL, evt_ptr,
            sip_ptr);
    return;
}

void SAPP_regReReg(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_RE_REG, reg_ptr, service_ptr, NULL, evt_ptr,
            sip_ptr);
    return;
}
void SAPP_regReset(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_RESET, reg_ptr, service_ptr, NULL, evt_ptr,
            sip_ptr);
    return;
}

void SAPP_regNoNet(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_NO_NET, reg_ptr, service_ptr, NULL, evt_ptr,
            sip_ptr);
    return;
}

void SAPP_regYesNet(
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    SAPP_SipObj      *sip_ptr)
{
    _SAPP_regStateMachine(REG_EVT_YES_NET, reg_ptr, service_ptr, NULL, evt_ptr,
        NULL);
    return;
}

vint SAPP_regEvent(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr,
    vint            *isNewReg_ptr,
    SAPP_SipObj     *sip_ptr)
{
    vint ret = SAPP_ERR;
    SAPP_regSmEvent event;
    vint stateChange;
    
    *isNewReg_ptr = SAPP_ERR;

    if (eUA_REGISTRATION_FAILED == sipEvt_ptr->header.type) {
        _SAPP_regStateMachine(REG_EVT_FAILED, reg_ptr,
                service_ptr, sipEvt_ptr, evt_ptr, sip_ptr);
        ret = SAPP_OK;
    }
    else if (eUA_REGISTRATION_COMPLETED == sipEvt_ptr->header.type) {
        if (sipEvt_ptr->resp.respCode != 0) {
            stateChange = _SAPP_regStateMachine(REG_EVT_COMPLETED, reg_ptr,
                    service_ptr, sipEvt_ptr, evt_ptr, sip_ptr);
        }
        else {
            stateChange = _SAPP_regStateMachine(REG_EVT_UNREG_DONE, reg_ptr,
                    service_ptr, sipEvt_ptr, evt_ptr, sip_ptr);
        }
        if (SAPP_OK == stateChange && reg_ptr->state == SAPP_REG_STATE_ON) {
            *isNewReg_ptr = SAPP_OK;
        }
        ret = SAPP_OK;
    }
    else {
        if (REG_EVT_NONE != (event = (SAPP_regSmEvent)_SAPP_regSubNotEvent(
                reg_ptr, service_ptr, sipEvt_ptr, evt_ptr, sip_ptr))) {
            _SAPP_regStateMachine(event, reg_ptr,
                    service_ptr, sipEvt_ptr, evt_ptr, sip_ptr);
            ret = SAPP_OK;
        }
    }
    return (ret);
}

/*
 * ========_SAPP_regServiceUnavailable() ========
 * This function is used when getting 503 Service Unavailable.
 * From 3GPP TS 24.229 5.1.2.2 General SUBSCRIBE requirements, if the UE
 * receives a 503 (Service Unavailable) response to an initial SUBSCRIBE
 * request containing a Retry-After header field, then the UE shall not
 * automaticall reattempt the request until after the period indicated by
 * the Retry-After header field contents.
 *
 * return: SAPP_OK
 *         SAPP_ERR
 */
vint _SAPP_regServiceUnavailable(
    SAPP_RegObj     *reg_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *uaEvt_ptr)
{
    uint32 expires;

    if (uaEvt_ptr) {
        SAPP_dbgPrintf("%s:%d\n", __FUNCTION__, __LINE__);
        /* check if the retry-after header field is included. */
        if (uaEvt_ptr->resp.retryAfterPeriod) {
            expires = uaEvt_ptr->resp.retryAfterPeriod;
            if (OSAL_SUCCESS == OSAL_tmrIsRunning(reg_ptr->regTmrId)) {
                SAPP_dbgPrintf("%s:%d Fail to start the subscribe timer\n", 
                        __FUNCTION__, __LINE__);
                return (SAPP_ERR);    
            }
            SAPP_dbgPrintf("Start Reg Subscribe retry timer %d seconds\n", expires);
            /* Now start the timer */
            if (OSAL_SUCCESS != OSAL_tmrStart(reg_ptr->regTmrId,
                    _SAPP_regSubscribeRetryTimerCb, service_ptr, expires * 1000)) {
                OSAL_tmrDelete(reg_ptr->regTmrId);
                reg_ptr->regTmrId = 0;
                return (SAPP_ERR);
            }
            return (SAPP_OK);
        }
    }
    
    return (SAPP_ERR);

}

