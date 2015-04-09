/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30186 $ $Date: 2014-12-03 12:04:17 +0800 (Wed, 03 Dec 2014) $
 */

#include <osal.h>
#include <isi.h>
#include <isip.h>
#include <csm_event.h>
#include "_tapp.h"
#include <ezxml.h>
#include <settings.h>
#include <sip_sip.h>
#include <sip_xport.h>
extern TAPP_GlobalObj *global_ptr;

/* 
 * ======== _TAPP_mockSappSendMessage() ========
 *
 * This function is send ISI message
 *
 * Return Values: 
 *   TAPP_PASS: Send ISI message successfully.
 *   TAPP_FAIL: Failed to send ISI message
 */
static  TAPP_Return _TAPP_mockSappSendMessage(
    TAPP_GlobalObj *global_ptr,
    ISIP_Message   *msg_ptr)
{

    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.isiEvt,
            msg_ptr, sizeof(ISIP_Message), OSAL_WAIT_FOREVER, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, global_ptr->queue.isiEvt);
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_mockSappSetServiceId() ========
 *
 * This function is used to set service id to global_ptr->serviceId[]
 * 
 * Return Values:
 *   service id index
 *   -1 : failed.
 */
vint _TAPP_mockSappSetServiceId(
    TAPP_GlobalObj *global_ptr,
    vint            id)
{
    vint idx;

    for (idx =0; idx < TAPP_MAX_SERVICE_NUMS; idx++) {
        if (global_ptr->serviceId[idx] == 0) {
            global_ptr->serviceId[idx] = id;
            TAPP_dbgPrintf("%s id:%d, idx:%d\n", __FUNCTION__, id, idx);
            return (idx);
        }
    }

    return (-1);
}

/*
 * ======== _TAPP_mockSappGetServiceIdByIndex() ========
 *
 * This function is used to get service id from global_ptr->serviceId[] by index.
 * 
 * Return Values:
 *   service id number.
 *   -1 : failed.
 */
vint _TAPP_mockSappGetServiceIdByIndex(
    TAPP_GlobalObj *global_ptr,
    vint index)
{
    if (TAPP_MAX_SERVICE_NUMS < index) {
        return (-1);
    }
    return (global_ptr->serviceId[index]);
}
/*
 * ======== _TAPP_mockSappGetIdByIndex() ========
 *
 * This function is used to get call id by from global_ptr->callId by index.
 * 
 * Return Values:
 *   call id number.
 *   -1 : failed.
 */
vint _TAPP_mockSappGetIdByIndex(
    TAPP_GlobalObj *global_ptr,
    vint index)
{
    if (TAPP_MAX_CALL_NUMS < index) {
        return (-1);
    }
    return (global_ptr->callId[index]);
}

/* 
 * ======== _TAPP_mockSappTimerCb() ========
 *
 * This function is registered with a timer used for mock SAPP registration
 * with ISI.  The timer calls this function when it expires.  IT will send 
 * an ISI event indicating that it wants to register with ISI.
 *
 * Return Values: 
 *   None
 */
static int32 _TAPP_mockSappTimerCb(
    void *arg_ptr)
{
    TAPP_GlobalObj *global_ptr;

    global_ptr = arg_ptr;

    /* This is a periodic timer, do not need to wait forever for sending msg. */
    if (OSAL_SUCCESS != OSAL_msgQSend(global_ptr->queue.isiEvt,
            &global_ptr->tmr.event, sizeof(ISIP_Message),
            OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, global_ptr->queue.isiEvt);
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}

/* 
 * ======== _TAPP_mockSappTimerInit() ========
 *
 * This function initializes a timer used to register mock SAPP with
 * ISI.  The timer fires at the specified interval until 
 * ISI returns a Command to mock SAPP indicating that it received
 * this event.
 *
 * Return Values: 
 *   TAPP_PASS The timer was succesfully initialized.
 *   TAPP_FAIL The timer failed to initialize
 */
static vint _TAPP_mockSappTimerInit(
    TAPP_GlobalObj   *tapp_ptr)
{
    if (0 == tapp_ptr->tmr.id) {
        /* Launch a timer that will attempt to register to ISI */
        if (0 == (tapp_ptr->tmr.id = OSAL_tmrCreate())) {
            /* Then we can't register with ISI! */
            return (TAPP_FAIL);
        }
    }
    else {
        OSAL_tmrStop(tapp_ptr->tmr.id);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrPeriodicStart(tapp_ptr->tmr.id, _TAPP_mockSappTimerCb,
            tapp_ptr, 5000)) {
        OSAL_tmrDelete(tapp_ptr->tmr.id);
        tapp_ptr->tmr.id = 0;
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_systemIsiEvt() ========
 *
 * This function is used to send ISIP system event.
 * 
 * Return Values:
 *   TAPP_PASS: send event succdessful
 *   TAPP_FAIL: send event failed.
 */
static vint _TAPP_systemIsiEvt(
    TAPP_GlobalObj *global_ptr,
    char           *ipcName_ptr,
    char           *audioName_ptr,
    char           *streamName_ptr,
    ISIP_ServiceReason  reason,
    ISIP_Status         status)
{
    ISIP_Message *inputEvt_ptr;

    inputEvt_ptr = &global_ptr->tmr.event;

    OSAL_memSet(inputEvt_ptr, 0, sizeof(ISIP_Message));
    inputEvt_ptr->id         = 0;
    inputEvt_ptr->code       = ISIP_CODE_SYSTEM;
    inputEvt_ptr->protocol   = ISI_PROTOCOL_SIP;
    inputEvt_ptr->msg.system.reason = reason;
    inputEvt_ptr->msg.system.status = status;
    /* get the IPC name */
    inputEvt_ptr->msg.system.protocolIpc[0] = 0;
    if (NULL != ipcName_ptr) {
        OSAL_snprintf(inputEvt_ptr->msg.system.protocolIpc,
                ISI_ADDRESS_STRING_SZ, "%s", ipcName_ptr);
    }
    inputEvt_ptr->msg.system.mediaIpc[0] = 0;
    if (NULL != audioName_ptr) {
        OSAL_snprintf(inputEvt_ptr->msg.system.mediaIpc,
                ISI_ADDRESS_STRING_SZ, "%s", audioName_ptr);
    }
    
    inputEvt_ptr->msg.system.streamIpc[0] = 0;
    if (NULL != streamName_ptr) {
        OSAL_snprintf(inputEvt_ptr->msg.system.streamIpc,
                ISI_ADDRESS_STRING_SZ, "%s", streamName_ptr);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_sappIsipTextValidate() ========
 *
 * This function is used to validate SAPP ISIP Text message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappIsipTextValidate(
    ISIP_Text *tappMsg_ptr,
    ISIP_Text *isipMsg_ptr)
{
    int i;

    /* Comparing reason */
    if (tappMsg_ptr->reason != isipMsg_ptr->reason) {
        return(TAPP_FAIL);
    }
    /* Comparing report */
    if (tappMsg_ptr->report != isipMsg_ptr->report) {
        return(TAPP_FAIL);
    }
    /* Comparing type */
    if (tappMsg_ptr->type != isipMsg_ptr->type) {
        return(TAPP_FAIL);
    }

    /* Convert serviceId from index */
    tappMsg_ptr->serviceId = 
            _TAPP_mockSappGetServiceIdByIndex(global_ptr,
            tappMsg_ptr->serviceId);
    if (tappMsg_ptr->serviceId != isipMsg_ptr->serviceId) {
        return(TAPP_FAIL);
    }

    /* Comparing pduLen */
    if (tappMsg_ptr->pduLen != (isipMsg_ptr->pduLen)) {
        return(TAPP_FAIL);
    }

    /* Comparing the fist byte */
    if (tappMsg_ptr->message[0] != isipMsg_ptr->message[0]) {
        return(TAPP_FAIL);
    }
    /* ignore 2nd bytes(mr) */
    /* Comparing pdu content */
    for (i = 2; i < isipMsg_ptr->pduLen; i++) {
        if (tappMsg_ptr->message[i] != isipMsg_ptr->message[i]) {
            return(TAPP_FAIL);
        } 
    }
    /* Comparing text to and from URI */
    if (0 != tappMsg_ptr->to[0]) {
        if (OSAL_strcmp(tappMsg_ptr->to, isipMsg_ptr->to)) {
            return(TAPP_FAIL);
        }
    }
    if (0 != tappMsg_ptr->from[0]) {
        if (OSAL_strcmp(tappMsg_ptr->from, isipMsg_ptr->from)) {
            return(TAPP_FAIL);
        }
    }

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_sappIsipUssdValidate() ========
 *
 * This function is used to validate SAPP ISIP Ussd message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappIsipUssdValidate(
    ISIP_Ussd *tappMsg_ptr,
    ISIP_Ussd *isipMsg_ptr)
{
    int i;

    /* Comparing reason */
    if (tappMsg_ptr->reason != isipMsg_ptr->reason) {
        return(TAPP_FAIL);
    }

    /* Comparing the fist byte */
    if (tappMsg_ptr->message[0] != isipMsg_ptr->message[0]) {
        return(TAPP_FAIL);
    }
    /* Comparing message */
    if (OSAL_strlen(tappMsg_ptr->message) != OSAL_strlen(isipMsg_ptr->message)) {
        return(TAPP_FAIL);
    }
    for (i = 0; i < OSAL_strlen(isipMsg_ptr->message); i++) {
        if (tappMsg_ptr->message[i] != isipMsg_ptr->message[i]) {
            return(TAPP_FAIL);
        } 
    }
    /* Comparing text to and from URI */
    if (0 != tappMsg_ptr->to[0]) {
        if (OSAL_strcmp(tappMsg_ptr->to, isipMsg_ptr->to)) {
            return(TAPP_FAIL);
        }
    }

    return (TAPP_PASS);
}

/*
 * ======== _TAPP_sappTelEvtValidate() ========
 *
 * This function is used to validate SAPP ISIP telephone 
 *  event message content.
 *
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappTelEvtValidate(
    ISIP_TelEvent *tappSrv_ptr,
    ISIP_TelEvent *inputSrv_ptr)
{   
    switch(tappSrv_ptr->evt) {
        case ISI_TEL_EVENT_DTMF:
        case ISI_TEL_EVENT_DTMF_OOB:
        case ISI_TEL_EVENT_VOICEMAIL:
        case ISI_TEL_EVENT_DTMF_STRING:
            if (tappSrv_ptr->settings.args.arg0 !=
                    inputSrv_ptr->settings.args.arg0) {
                return(TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.args.arg1 !=
                    inputSrv_ptr->settings.args.arg1) {
                return(TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.args.string,
                    inputSrv_ptr->settings.args.string)) {
                return(TAPP_FAIL);
            }
            break;
        case ISI_TEL_EVENT_CALL_FORWARD:
            /* Comparing forward items */
            if (tappSrv_ptr->settings.forward.condition !=
                    inputSrv_ptr->settings.forward.condition) {
                return(TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.forward.enable !=
                    inputSrv_ptr->settings.forward.enable) {
                return(TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.forward.timeout !=
                    inputSrv_ptr->settings.forward.timeout) {
                return(TAPP_FAIL);
            }
            break;
        case ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE:
            /* Comparing tel event service attibutes */
            if (tappSrv_ptr->settings.service.cmd !=
                    inputSrv_ptr->settings.service.cmd) {
                return(TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.service.arg1,
                    inputSrv_ptr->settings.service.arg1)) {
                return(TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.service.arg2,
                    inputSrv_ptr->settings.service.arg2)) {
                return(TAPP_FAIL);
            }
            break;
        case ISI_TEL_EVENT_FEATURE:
        case ISI_TEL_EVENT_FLASHHOOK:
        case ISI_TEL_EVENT_SEND_USSD:
        default:
            break;
    }
    return (TAPP_PASS);

}

/*
 * ======== _TAPP_sappServiceValidate() ========
 *
 * This function is used to validate SAPP service message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappServiceValidate(
    ISIP_Service *tappSrv_ptr,
    ISIP_Service *inputSrv_ptr) {

    if (tappSrv_ptr->status != inputSrv_ptr->status) {
        return (TAPP_FAIL);
    }

    /* Comparing reasonDesc if it's not empty */
    if (0 != tappSrv_ptr->reasonDesc[0]) {
        if (OSAL_strncmp(tappSrv_ptr->reasonDesc,
                inputSrv_ptr->reasonDesc, ISI_EVENT_DESC_STRING_SZ)) {
            return (TAPP_FAIL);
        }
    }
    switch (tappSrv_ptr->reason) {
        case ISIP_SERVICE_REASON_NET:
            /* Comparing actived inrface name and address */
            if (OSAL_strncmp(tappSrv_ptr->settings.interface.name,
                inputSrv_ptr->settings.interface.name,
                ISI_ADDRESS_STRING_SZ)) {
                return (TAPP_FAIL);
            }
            if (OSAL_FALSE == OSAL_netIsAddrEqual(
                    &tappSrv_ptr->settings.interface.address,
                    &inputSrv_ptr->settings.interface.address)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_BLOCKUSER:
            /* Comparing block user uri */
            if (OSAL_strncmp(tappSrv_ptr->settings.uri,
                inputSrv_ptr->settings.uri, ISI_ADDRESS_STRING_SZ)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_IDENTITY:
            /* Comparing the identity Hide */
            if (tappSrv_ptr->settings.identityHide !=
                    inputSrv_ptr->settings.identityHide) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_SERVER:
            /* Comparing server address */
            if (OSAL_strncmp(tappSrv_ptr->settings.server,
                inputSrv_ptr->settings.server, ISI_ADDRESS_STRING_SZ)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_AUTH:
            /* Comparing credentials items */
            if (OSAL_strcmp(tappSrv_ptr->settings.credentials.username,
                    inputSrv_ptr->settings.credentials.username)) {
                return (TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.credentials.password,
                    inputSrv_ptr->settings.credentials.password)) {
                return (TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.credentials.realm,
                    inputSrv_ptr->settings.credentials.realm)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_URI:
            /* Comparing uri */
            if (OSAL_strcmp(tappSrv_ptr->settings.uri,
                    inputSrv_ptr->settings.uri)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_BSID:
            /* Comparing bsid */
            if (tappSrv_ptr->settings.bsId.type !=
                    inputSrv_ptr->settings.bsId.type) {
                return (TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.bsId.szBsId,
                    inputSrv_ptr->settings.bsId.szBsId)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_FILE:
            /* Compaing file path and prepend */
            if (OSAL_strcmp(tappSrv_ptr->settings.file.filePath,
                    inputSrv_ptr->settings.file.filePath)) {
                return (TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.file.filePrepend,
                    inputSrv_ptr->settings.file.filePrepend)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE:
            /* Comparing aka authentication items */
            if (OSAL_strcmp(tappSrv_ptr->settings.akaAuthResp.resp,
                    inputSrv_ptr->settings.akaAuthResp.resp)) {
                return (TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.akaAuthResp.ck,
                    inputSrv_ptr->settings.akaAuthResp.ck)) {
                return (TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->settings.akaAuthResp.ik,
                    inputSrv_ptr->settings.akaAuthResp.ik)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_CAPABILITIES:
            /* Comparing capabilities */
            if (OSAL_strcmp(tappSrv_ptr->settings.capabilities,
                    inputSrv_ptr->settings.capabilities)) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_PORT:
            /* Comparing port setting items */
            if (tappSrv_ptr->settings.port.portType !=
                    inputSrv_ptr->settings.port.portType) {
                return (TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.port.portNum !=
                    inputSrv_ptr->settings.port.portNum) {
                return (TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.port.poolSize !=
                    inputSrv_ptr->settings.port.poolSize) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_EMERGENCY:
            if (tappSrv_ptr->settings.isEmergency !=
                    inputSrv_ptr->settings.isEmergency) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_IMEI_URI:
            if (OSAL_strcmp(tappSrv_ptr->settings.imeiUri,
                    inputSrv_ptr->settings.imeiUri)) {
                return (TAPP_FAIL);
            }
            break;    
        case ISIP_SERVICE_REASON_IPSEC:
            if (tappSrv_ptr->settings.ipsec.cfg.protectedPort !=
                    inputSrv_ptr->settings.ipsec.cfg.protectedPort) {
                return (TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.ipsec.cfg.protectedPortPoolSz !=
                    inputSrv_ptr->settings.ipsec.cfg.protectedPortPoolSz) {
                return (TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.ipsec.cfg.spi !=
                    inputSrv_ptr->settings.ipsec.cfg.spi) {
                return (TAPP_FAIL);
            }
            if (tappSrv_ptr->settings.ipsec.cfg.spiPoolSz !=
                    inputSrv_ptr->settings.ipsec.cfg.spiPoolSz) {
                return (TAPP_FAIL);
            }
            break;
        case ISIP_SERVICE_REASON_DESTROY:
        case ISIP_SERVICE_REASON_DEACTIVATE:
        case ISIP_SERVICE_REASON_HANDOFF:
        case ISIP_SERVICE_REASON_ACTIVATE:
        case ISIP_SERVICE_REASON_CODERS:
        case ISIP_SERVICE_REASON_SET_PROVISIONING_DATA:
        default:
            break;
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_sappCallSessionValidate() ========
 *
 * This function is used to validate SAPP call session message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappCallSessionValidate(
    ISIP_Call *tappSrv_ptr,
    ISIP_Call *inputSrv_ptr)
{
    
    /* Comparing audio security keys*/
    if (tappSrv_ptr->type & ISI_SESSION_TYPE_SECURITY_AUDIO &&
            OSAL_memCmp(&tappSrv_ptr->audioKeys, &inputSrv_ptr->audioKeys,
            sizeof(ISIP_SecurityKeys))) {
        return(TAPP_FAIL);
    }
    /* Comparing video security keys*/
    if (tappSrv_ptr->type & ISI_SESSION_TYPE_SECURITY_VIDEO &&
            OSAL_memCmp(&tappSrv_ptr->videoKeys, &inputSrv_ptr->videoKeys,
            sizeof(ISIP_SecurityKeys))) {
         return(TAPP_FAIL);
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_sappRemoteRtpValidate() ========
 *
 * This function is used to validate SAPP remote RTP message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappRemoteRtpValidate(
    ISIP_Call *tappSrv_ptr,
    ISIP_Call *inputSrv_ptr)
{
    /* Comparing remote audio address */
    if (tappSrv_ptr->type) {
        if (OSAL_memCmp(&tappSrv_ptr->rmtAudioAddr, &inputSrv_ptr->rmtAudioAddr, 
                sizeof(OSAL_NetAddress))) {
            return(TAPP_FAIL);
        }
        if (tappSrv_ptr->rmtAudioCntlPort != inputSrv_ptr->rmtAudioCntlPort) {
            return(TAPP_FAIL);
        }
        
    }
    /* Comparing remote video address */
    if ((tappSrv_ptr->type & ISI_SESSION_TYPE_SECURITY_VIDEO ||
            tappSrv_ptr->type & ISI_SESSION_TYPE_VIDEO)) {
        if (tappSrv_ptr->rmtVideoCntlPort != inputSrv_ptr->rmtVideoCntlPort) {
            return(TAPP_FAIL);
        }
        if (OSAL_memCmp(&tappSrv_ptr->rmtVideoAddr, &inputSrv_ptr->rmtVideoAddr, 
                sizeof(OSAL_NetAddress))) {
            return(TAPP_FAIL);
        }
    }
    return(TAPP_PASS);
}

/*
 * ======== _TAPP_sappIsipCallValidate() ========
 *
 * This function is used to validate ISIP call message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappIsipCallValidate(
    TAPP_GlobalObj *global_ptr,
    ISIP_Call      *tappSrv_ptr,
    ISIP_Call      *inputSrv_ptr)
{ 
    vint index;

    if (tappSrv_ptr->reason != inputSrv_ptr->reason) {
        return(TAPP_FAIL);
    }
    if (tappSrv_ptr->status != inputSrv_ptr->status) {
        return(TAPP_FAIL);
    }
   
    if (tappSrv_ptr->type != inputSrv_ptr->type) {
        return(TAPP_FAIL);
    }
  
    /* Convert serviceId from index */
    tappSrv_ptr->serviceId = 
            _TAPP_mockSappGetServiceIdByIndex(global_ptr,
            tappSrv_ptr->serviceId);
    if (tappSrv_ptr->serviceId != inputSrv_ptr->serviceId) {
        return(TAPP_FAIL);
    }
    if (ISI_SESSION_TYPE_AUDIO == inputSrv_ptr->type) {
        if (tappSrv_ptr->audioDirection != inputSrv_ptr->audioDirection) {
            return(TAPP_FAIL);
        }
    }
    else if (ISI_SESSION_TYPE_VIDEO == inputSrv_ptr->type) {
        if (tappSrv_ptr->videoDirection != inputSrv_ptr->videoDirection) {
            return(TAPP_FAIL);
        }
    }
    /*  If type is VIDEO or VIDEO SECURITY , check the video direction */
    if ((tappSrv_ptr->type & ISI_SESSION_TYPE_SECURITY_VIDEO ||
            tappSrv_ptr->type & ISI_SESSION_TYPE_VIDEO) &&
            tappSrv_ptr->videoDirection != inputSrv_ptr->videoDirection) {
        return(TAPP_FAIL);
    }
    /* Validat the "to" URI */
    if (OSAL_strcmp(tappSrv_ptr->to, inputSrv_ptr->to)) {
        return(TAPP_FAIL);
    }
 
    switch (tappSrv_ptr->reason) {
        case ISIP_CALL_REASON_INITIATE:
            /* Validat the "from" URI */
            if (OSAL_strcmp(tappSrv_ptr->from, inputSrv_ptr->from)) {
                return(TAPP_FAIL);
            }
            if (tappSrv_ptr->cidType != inputSrv_ptr->cidType) {
                return(TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_MODIFY:
            /* Comparing from uri */
            if (OSAL_strcmp(tappSrv_ptr->from, inputSrv_ptr->from)) {
                return(TAPP_FAIL);
            }
            /* Validate remote rtp date */
            if (TAPP_PASS != _TAPP_sappRemoteRtpValidate(tappSrv_ptr,
                    inputSrv_ptr)) {
                return(TAPP_FAIL);
            }
            /* Validate session data */
            return (_TAPP_sappCallSessionValidate(tappSrv_ptr, inputSrv_ptr));
        case ISIP_CALL_REASON_ACCEPT:
            /* Validate remote rtp date */
            if (TAPP_PASS != _TAPP_sappRemoteRtpValidate(tappSrv_ptr,
                    inputSrv_ptr)) {
                return(TAPP_FAIL);
            }
            /* Validate session data */
            return (_TAPP_sappCallSessionValidate(tappSrv_ptr, inputSrv_ptr));    
        case ISIP_CALL_REASON_TRANSFER_CONSULT:
        case ISIP_CALL_REASON_TRANSFER_COMPLETED:
            /* Get transfer target call ID from global object */
            index = tappSrv_ptr->transferTargetCallId;
            if ( 0 >= (tappSrv_ptr->transferTargetCallId =
                    _TAPP_mockSappGetIdByIndex(global_ptr, index))) {
                return (TAPP_FAIL);
            }

            if (tappSrv_ptr->transferTargetCallId != 
                    inputSrv_ptr->transferTargetCallId) {
                return(TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_TRANSFER_CONF:
            if (OSAL_strcmp(tappSrv_ptr->participants, inputSrv_ptr->participants)) {
                return(TAPP_FAIL);
            }
            break;
         case ISIP_CALL_REASON_INITIATE_CONF:
            if (OSAL_strcmp(tappSrv_ptr->from, inputSrv_ptr->from)) {
                return(TAPP_FAIL);
            }
            if (tappSrv_ptr->cidType != inputSrv_ptr->cidType) {
                return(TAPP_FAIL);
            }
            if (OSAL_strcmp(tappSrv_ptr->participants, inputSrv_ptr->participants)) {
                return(TAPP_FAIL);
            }
            break;
        case ISIP_CALL_REASON_REJECT:
            if (('\0' != tappSrv_ptr->reasonDesc[0]) && 
                    OSAL_strcmp(tappSrv_ptr->reasonDesc, inputSrv_ptr->reasonDesc)) {
                return(TAPP_FAIL);
            }
            break;
        default:
            break;        
    }
    return (TAPP_PASS);
}

/*
 * ======== _TAPP_sappIsipPresenceValidate() ========
 *
 * This function is used to validate ISIP presence content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappIsipPresenceValidate(
    ISIP_Presence *tappSrv_ptr,
    ISIP_Presence *inputSrv_ptr)
{
    /* Comparing reason */
    if (tappSrv_ptr->reason != inputSrv_ptr->reason) {
        return(TAPP_FAIL);
    }
    /* Comparing serviceId*/
    if (tappSrv_ptr->serviceId != inputSrv_ptr->serviceId) {
        return(TAPP_FAIL);
    }
    /* Comparing chatId */
    if (tappSrv_ptr->chatId != inputSrv_ptr->chatId) {
        return(TAPP_FAIL);
    }
    /* Comparing reasonDesc */
    if (OSAL_strcmp(tappSrv_ptr->reasonDesc, inputSrv_ptr->reasonDesc)) {
        return(TAPP_FAIL);
    }
    /* Comparing presence from and to uri */
    if (OSAL_strcmp(tappSrv_ptr->to, inputSrv_ptr->to)) {
        return(TAPP_FAIL);
    }
    /* Comparing from uri */
    if (OSAL_strcmp(tappSrv_ptr->from, inputSrv_ptr->from)) {
        return(TAPP_FAIL);
    }
    /* Comparing roster items */
    if (OSAL_strcmp(tappSrv_ptr->roster.group, inputSrv_ptr->roster.group)) {
        return(TAPP_FAIL);
    }
    if (OSAL_strcmp(tappSrv_ptr->roster.name, inputSrv_ptr->roster.name)) {
        return(TAPP_FAIL);
    }
    /* Comparing presence */
    if (OSAL_strcmp(tappSrv_ptr->presence, inputSrv_ptr->presence)) {
        return(TAPP_FAIL);
    }
    return(TAPP_PASS);
}

/*
 * ======== _TAPP_sappIsipChatValidate() ========
 *
 * This function is used to validate ISIP chat message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappIsipChatValidate(
    ISIP_Chat *tappMsg_ptr,
    ISIP_Chat *inputSrv_ptr)
{
    /* Comparing serviceId */
    if (tappMsg_ptr->serviceId != inputSrv_ptr->serviceId) {
        return(TAPP_FAIL);
    }
    /* Comparing reason*/
    if (tappMsg_ptr->reason != inputSrv_ptr->reason) {
        return(TAPP_FAIL);
    }
    /* Comparing stauts */
    if (tappMsg_ptr->status != inputSrv_ptr->status) {
        return(TAPP_FAIL);
    }
    /* Comparing passwordRequired */
    if (tappMsg_ptr->passwordRequired != inputSrv_ptr->passwordRequired) {
        return(TAPP_FAIL);
    }
    /* Validate text part */
    if (TAPP_PASS != _TAPP_sappIsipTextValidate(&tappMsg_ptr->text,
            &inputSrv_ptr->text)) {
        return(TAPP_FAIL);
    }
    /* Comparing reasonDesc */
    if (OSAL_strcmp(tappMsg_ptr->reasonDesc, inputSrv_ptr->reasonDesc)) {
        return(TAPP_FAIL);
    }
    /* Comparing remote address */
    if (OSAL_strcmp(tappMsg_ptr->remoteAddress, inputSrv_ptr->remoteAddress)) {
        return(TAPP_FAIL);
    }
    /* Comparing local address */
    if (OSAL_strcmp(tappMsg_ptr->localAddress,
            inputSrv_ptr->localAddress)) {
        return(TAPP_FAIL);
    }
    /* Comparing participants */
    if (OSAL_strcmp(tappMsg_ptr->participants, inputSrv_ptr->participants)) {
        return(TAPP_FAIL);
    }
    /* Comparing subject */
    if (OSAL_strcmp(tappMsg_ptr->subject, inputSrv_ptr->subject)) {
        return(TAPP_FAIL);
    }
    /* Comparing password */
    if (OSAL_strcmp(tappMsg_ptr->password,
            inputSrv_ptr->password)) {
        return(TAPP_FAIL);
    }
    return(TAPP_PASS);
}

/*
 * ======== _TAPP_sappIsipFileValidate() ========
 *
 * This function is used to validate ISIP file message content.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
static vint _TAPP_sappIsipFileValidate(
    ISIP_File *tappMsg_ptr,
    ISIP_File *inputSrv_ptr)
{
    /* Comparing reason */
    if (tappMsg_ptr->reason != inputSrv_ptr->reason) {
        return(TAPP_FAIL);
    }
    /* Comparing serivceId*/
    if (tappMsg_ptr->serviceId != inputSrv_ptr->serviceId) {
        return(TAPP_FAIL);
    }
    /* Comparing chatId*/
    if (tappMsg_ptr->chatId != inputSrv_ptr->chatId) {
        return(TAPP_FAIL);
    }
    /* Comparing fileType*/
    if (tappMsg_ptr->fileType!= inputSrv_ptr->fileType) {
        return(TAPP_FAIL);
    }
    /* Comparing the file attribute*/
    if (tappMsg_ptr->fileAttribute!= inputSrv_ptr->fileAttribute) {
        return(TAPP_FAIL);
    }
    /* Comparing fileSize*/
    if (tappMsg_ptr->fileSize!= inputSrv_ptr->fileSize) {
        return(TAPP_FAIL);
    }
    /* Comparing progress */
    if (tappMsg_ptr->progress != inputSrv_ptr->progress) {
        return(TAPP_FAIL);
    }
    /* Comparing reasonDesc */
    if (OSAL_strcmp(tappMsg_ptr->reasonDesc, inputSrv_ptr->reasonDesc)) {
        return(TAPP_FAIL);
    }
    /* Comparing file's from and to uri */
    if (OSAL_strcmp(tappMsg_ptr->to, inputSrv_ptr->to)) {
        return(TAPP_FAIL);
    }
    if (OSAL_strcmp(tappMsg_ptr->from, inputSrv_ptr->from)) {
        return(TAPP_FAIL);
    }
    /* Comparing filePath */
    if (OSAL_strcmp(tappMsg_ptr->filePath, inputSrv_ptr->filePath)) {
        return(TAPP_FAIL);
    }
    /* Comparing subject */
    if (OSAL_strcmp(tappMsg_ptr->subject, inputSrv_ptr->subject)) {
        return(TAPP_FAIL);
    }
    return(TAPP_PASS);
}

/*
 * ======== TAPP_mockSappInit() ========
 *
 * This function is used to initialize and configure Mock SAPP.
 * 
 * 
 * Return Values:
 *   TAPP_PASS: initialize successfully.
 *   TAPP_FAIL: initialize fialed.
 */
vint TAPP_mockSappInit(
    void           *cfg_ptr,
    TAPP_GlobalObj *global_ptr) 
{
    char           *ipc_ptr;
    char           *isiIpc_ptr;
    char           *mediaIpc_ptr;
    char           *streamIpc_ptr;

    if (NULL == global_ptr) {
        return (TAPP_FAIL);
    }

    /* Get the names of the ipc interfaces */
    ipc_ptr = NULL;
    isiIpc_ptr = NULL;
    mediaIpc_ptr = NULL;
    streamIpc_ptr = NULL;
    if (NULL == (ipc_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        return (TAPP_FAIL);
    }
    if (NULL == (isiIpc_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_ISI))) {
        return (TAPP_FAIL);
    }
    if (NULL == (mediaIpc_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_AUDIO))) {
        return (TAPP_FAIL);
    }
    if (NULL == (streamIpc_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_STREAM))) {
        return (TAPP_FAIL);
    }
    
    /* Command queue */
    if (0 == (global_ptr->queue.isiCmd = OSAL_msgQCreate(ipc_ptr, 
            OSAL_MODULE_ISI, OSAL_MODULE_TAPP, OSAL_DATA_STRUCT_ISIP_Message,
            16, sizeof(ISIP_Message), 0))) {
        OSAL_logMsg("%s:%d Create isi CmdQ fail\n", __FUNCTION__,
                __LINE__);
        return (TAPP_FAIL);
    }
    /* ISI Application queue */
    if (0 == (global_ptr->queue.isiEvt = OSAL_msgQCreate(isiIpc_ptr,
            OSAL_MODULE_TAPP, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
            16, sizeof(ISIP_Message), 0))) {
        OSAL_msgQDelete(global_ptr->queue.isiCmd);
        return (TAPP_FAIL);
    }

    if (TAPP_PASS != _TAPP_systemIsiEvt(global_ptr, ipc_ptr,
            mediaIpc_ptr,streamIpc_ptr,
            ISIP_SYSTEM_REASON_START, ISIP_STATUS_TRYING)) {
        return (TAPP_FAIL);
    }

    /* Launch the timer used to register with ISI */
    if (TAPP_PASS != _TAPP_mockSappTimerInit(global_ptr)) {
        /* Failed to init timer */
        return (TAPP_FAIL);
    }

    return (TAPP_PASS);
}

/*
 * ======== TAPP_mockSappShutdown() ========
 *
 * This function is used to shutdown Mock SAPP.
 * 
 * Return Values:
 *   TAPP_PASS: shutdown sucessfully.
 *   TAPP_FAIL: shutdown failed
 */
vint TAPP_mockSappShutdown(
    TAPP_GlobalObj *global_ptr) 
{
    if (NULL == global_ptr) {
        return (TAPP_FAIL);
    }
    /* Tell ISI that we are going down */
    if (TAPP_PASS != _TAPP_systemIsiEvt(global_ptr, NULL, NULL, NULL,
            ISIP_SYSTEM_REASON_SHUTDOWN, ISIP_STATUS_DONE)) {
        return (TAPP_FAIL);
    }

    _TAPP_mockSappSendMessage(global_ptr, &global_ptr->tmr.event);

    /* Delete queues. */
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.isiCmd)) {
        return (TAPP_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(global_ptr->queue.isiEvt)) {
        return (TAPP_FAIL);
    }
    return (TAPP_PASS);
}
/*
 * ======== _TAPP_sappSetAvilableId() ========
 *
 * This function is used to set call id to avilable index of global_ptr->callId.
 * 
 * Return Values:
 *   TAPP_PASS: can get avilable id index.
 *   TAPP_FAIL: no avilable id index.
 */
static vint _TAPP_sappSetAvilableId(
    TAPP_GlobalObj *global_ptr,
    vint          callId)
{
    vint index;
    vint ret = TAPP_FAIL;

    for (index = 0; index < TAPP_MAX_CALL_NUMS; index ++) {
        if (0 == global_ptr->callId[index]) {
            /* get empty and set call ID  */
            global_ptr->callId[index] = callId;
            ret = TAPP_PASS;
            break;
        }
    }
    return (ret);
}
/*  
 * ======== _TAPP_sappGetUniqCallId() ========
 *
 * This function is used to get uniq call id by ramdom.
 *
*/
static vint _TAPP_sappGetUniqCallId(
    uint32 l,
    uint32 h)
{
    uint32 x;
    uint32 mod;
    static int inited = 0;
    /* get random seed */
    if (!inited) {
        inited = 1;
        OSAL_randomReseed();
    }
    if (l < h) {
        OSAL_randomGetOctets((void *)&x, sizeof(x));
        mod = (h - l);
        x %= mod;
        x += l;
        return x;
    }
    else {
        return l;
    }
}

/*
 * ======== TAPP_mockSappIssueIsip() ========
 *
 * This function is used to issue an SAPP ISIP event.
 * 
 * Return Values:
 *   TAPP_PASS:send isip message sucessfully.
 *   TAPP_FAIL: send isip message failed.
 */
vint TAPP_mockSappIssueIsip(
    TAPP_GlobalObj *global_ptr,
    ISIP_Message   *isip_ptr)
{
    int index;
    ISIP_Message isip;

    if (NULL == isip_ptr || NULL == global_ptr) {
        return (TAPP_FAIL);
    }

    /* Duplicate original one in case it's modifed */
    isip = *isip_ptr;

    if (ISIP_CODE_SERVICE == isip.code) {
        /* Temp set to 2 */
        isip.id = _TAPP_mockSappGetServiceIdByIndex(global_ptr, isip.id);
    }
    if (ISIP_CODE_CALL == isip.code) {
        isip.msg.call.serviceId = _TAPP_mockSappGetServiceIdByIndex(global_ptr,
                isip.msg.call.serviceId);
        index = isip.id;
        /* If the type is initiate call, we need to generate a uniq call id 
         * and send response to CSM
         */
        if (ISIP_CALL_REASON_INITIATE == isip.msg.call.reason &&
                ISIP_STATUS_DONE != isip.msg.call.status) {
            /* generate a call id */
            isip.id = _TAPP_sappGetUniqCallId(1, 0x7fffffff);
            /* set to global */
            if (TAPP_PASS != _TAPP_sappSetAvilableId(global_ptr,
                    isip.id)) {
                return(TAPP_FAIL);
            }
        }
        /* else we get the call id form the global object we have set */
        else if (0 >= (isip.id = _TAPP_mockSappGetIdByIndex(global_ptr,
                index))) {
            return (TAPP_FAIL);
        }
        /* If transfer consult or complete, we get call id  from global object */
        if (ISIP_CALL_REASON_TRANSFER_CONSULT == isip.msg.call.reason ||
                ISIP_CALL_REASON_TRANSFER_COMPLETED == 
                isip.msg.call.reason) {
            index = isip.msg.call.transferTargetCallId;
            if (0 >= (isip.msg.call.transferTargetCallId =
                    _TAPP_mockSappGetIdByIndex(global_ptr, index))) {
                return (TAPP_FAIL);
            }    

        }
        /* If call is ended ,need to clean the call id. */
        if (ISIP_CALL_REASON_TERMINATE == isip.msg.call.reason ||
            ISIP_CALL_REASON_FAILED == isip.msg.call.reason ||
            ISIP_CALL_REASON_REJECT == isip.msg.call.reason ||
            ISIP_CALL_REASON_ERROR == isip.msg.call.reason) {
            /* Clear the call ID */
            global_ptr->callId[index] = 0;
        }
    }
    if (ISIP_CODE_MESSAGE == isip.code) {
        isip.msg.message.serviceId = _TAPP_mockSappGetServiceIdByIndex(global_ptr,
                isip.msg.message.serviceId);
        if ((ISIP_TEXT_REASON_COMPLETE == isip.msg.message.reason) || 
                (ISIP_TEXT_REASON_INVALID == isip.msg.message.reason) ||
                (ISIP_TEXT_REASON_ERROR == isip.msg.message.reason)) {
            isip.id = global_ptr->textId;
        }
    }
    if (ISIP_CODE_USSD == isip.code) {
        isip.msg.ussd.serviceId = global_ptr->ussdServiceId;
        if ((ISIP_USSD_REASON_INVALID == isip.msg.ussd.reason) ||
                (ISIP_USSD_REASON_SEND_OK == isip.msg.ussd.reason)) {
            isip.id = global_ptr->ussdId;
        }
    }
    if (ISIP_CODE_PRESENCE == isip.code) {
        isip.msg.presence.serviceId = _TAPP_mockSappGetServiceIdByIndex(global_ptr,
                isip.msg.presence.serviceId);
        if (0 >= (isip.msg.presence.chatId = _TAPP_mockSappGetIdByIndex(global_ptr,
                isip.id))) {
            return (TAPP_FAIL);
        }
        isip.id = 0;
    }
    if (ISIP_CODE_TEL_EVENT == isip.code) {
        isip.msg.event.serviceId =
                _TAPP_mockSappGetServiceIdByIndex(global_ptr,
                isip.msg.event.serviceId);
        isip.id = 0;
    }
    return (_TAPP_mockSappSendMessage(global_ptr, &isip));
}

/*
 * ======== TAPP_mockSappValidateIsip() ========
 *
 * This function is used to validate ISIP message.
 * 
 * Return Values:
 *   TAPP_PASS: the result is same the expected value.
 *   TAPP_FAIL: the result is different with the expected value.
 */
vint TAPP_mockSappValidateIsip(
    TAPP_GlobalObj *global_ptr,
    TAPP_Action    *action_ptr) 
{
    TAPP_Event   *tappEvt_ptr;
    ISIP_Message *isipMsg_ptr;  /* for getting input ISIP message */
    ISIP_Message  tappMsg;      /* for expect ISIP message */
    vint          index;

    if (NULL == action_ptr || NULL == global_ptr) {
        return (TAPP_FAIL);
    }

    tappEvt_ptr = &global_ptr->tappEvt;
    /* Duplicate original one in case it's modifed */
    tappMsg     = action_ptr->msg.isip;
    
    if (TAPP_PASS != TAPP_getInputEvent(global_ptr, action_ptr,
            tappEvt_ptr, action_ptr->u.timeout)) {
        /* 
         * If the clean isip action, we expect to get time
         * out response.
         * Otherwise ,return fail
         */
        if (TAPP_ACTION_TYPE_CLEAN_ISIP == action_ptr->type) {
            /* we expect to get timeout event that we clean the ISIP up */
            if (TAPP_EVENT_TYPE_TIMEOUT == tappEvt_ptr->type) {
                return(TAPP_PASS);
            }
            else {
                /* If got others event type, return fail */
                return(TAPP_FAIL);
            }
        }
        else {
            return (TAPP_FAIL);
        }
    }
    
    if (TAPP_EVENT_TYPE_ISIP != tappEvt_ptr->type) {
        return (TAPP_FAIL);
    }
    isipMsg_ptr = &tappEvt_ptr->msg.isip;
 
    if (tappMsg.code != isipMsg_ptr->code) {
        return (TAPP_FAIL);
    }
    switch (tappMsg.code) {
        case ISIP_CODE_SERVICE:
            /* Validate the service message */
            if (isipMsg_ptr->msg.service.reason != 
                    tappMsg.msg.service.reason) {
                return(TAPP_FAIL);
            }
            if (ISIP_SERVICE_REASON_CREATE ==
                    isipMsg_ptr->msg.service.reason) {
                _TAPP_mockSappSetServiceId(global_ptr, isipMsg_ptr->id);
            }
            if (TAPP_PASS == _TAPP_sappServiceValidate(&tappMsg.msg.service,
                    &isipMsg_ptr->msg.service)) {
                return(TAPP_PASS);
            }
            else {
                return(TAPP_FAIL);
            }
        case ISIP_CODE_CALL:
            /* Validate call */
            if (ISIP_CALL_REASON_INITIATE == tappMsg.msg.call.reason || ISIP_CALL_REASON_INITIATE_CONF == tappMsg.msg.call.reason) {
                /* It's new outgoing call, set the call ID to global_ptr */
                _TAPP_sappSetAvilableId(global_ptr, isipMsg_ptr->id);
            }
            else {
                /* Mapping the correct call id */
                index = tappMsg.id;
                tappMsg.id = _TAPP_mockSappGetIdByIndex(global_ptr, index);
                if (tappMsg.id != isipMsg_ptr->id) {
                    return (TAPP_FAIL);
                }
            }
            /* Validate others fields */
            return (_TAPP_sappIsipCallValidate(global_ptr, 
                    &tappMsg.msg.call,
                    &isipMsg_ptr->msg.call));
        case ISIP_CODE_SYSTEM:
            /* Validate the system message */
            if (tappMsg.msg.system.reason !=
                    isipMsg_ptr->msg.system.reason) {
                return(TAPP_FAIL);
            }
            if (tappMsg.msg.system.status !=
                    isipMsg_ptr->msg.system.status) {
                return(TAPP_FAIL);
            }            
            break;
        case ISIP_CODE_MESSAGE:
            /* Validate text */
            if (TAPP_PASS == _TAPP_sappIsipTextValidate(&tappMsg.msg.message,
                    &isipMsg_ptr->msg.message)) {
                global_ptr->textId = isipMsg_ptr->id;
                return (TAPP_PASS);
            }
            else {
                return (TAPP_FAIL);
            }
        case ISIP_CODE_PRESENCE:
            /* Validate presence */
            return (_TAPP_sappIsipPresenceValidate(&tappMsg.msg.presence,
                    &isipMsg_ptr->msg.presence));
        case ISIP_CODE_TEL_EVENT:
            /* Validate tel event */
            if (tappMsg.msg.event.reason !=
                    isipMsg_ptr->msg.event.reason) {
                return(TAPP_FAIL);
            }
            /* Get call ID for telEvent */
            index = tappMsg.msg.event.callId;
            tappMsg.msg.event.callId = _TAPP_mockSappGetIdByIndex(global_ptr,
                    index);
            if (tappMsg.msg.event.callId !=
                    isipMsg_ptr->msg.event.callId) {
                return(TAPP_FAIL);
            }
            if (tappMsg.msg.event.evt !=
                    isipMsg_ptr->msg.event.evt) {
                return(TAPP_FAIL);
            }
            return (_TAPP_sappTelEvtValidate(&tappMsg.msg.event,
                    &isipMsg_ptr->msg.event));
        case ISIP_CODE_CHAT:
            /* Validate chat message */
            return (_TAPP_sappIsipChatValidate(&tappMsg.msg.groupchat,
                    &isipMsg_ptr->msg.groupchat));
        case ISIP_CODE_FILE:
            /* Validate file message */
            return (_TAPP_sappIsipFileValidate(&tappMsg.msg.file,
                    &isipMsg_ptr->msg.file));
        case ISIP_CODE_DIAGNOSTIC:
            /* Validate diagnostic */
            if (tappMsg.msg.diag.reason !=
                    isipMsg_ptr->msg.diag.reason) {
                return(TAPP_FAIL);
            }
            if (OSAL_strcmp(tappMsg.msg.diag.audioFile,
                    isipMsg_ptr->msg.diag.audioFile)) {
                return(TAPP_FAIL);
            }
            break;
        case ISIP_CODE_USSD:
            /* Validate ussd */
            if (TAPP_PASS == _TAPP_sappIsipUssdValidate(&tappMsg.msg.ussd,
                    &isipMsg_ptr->msg.ussd)) {
                global_ptr->ussdId = isipMsg_ptr->id;
                global_ptr->ussdServiceId = isipMsg_ptr->msg.ussd.serviceId;
                return (TAPP_PASS);
            }
            else {
                return (TAPP_FAIL);
            }
        default:
            break;
    }   
    return (TAPP_PASS);
}

/*
 * ======== SAPP_init() ========
 *
 * This function is Mock SAPP initial..
 * This function will call the mock sapp initial function
 * 
 * Return Values:
 *   0: initialize successfully.
 *   -1: initialize fialed.
 */
int SAPP_init(
    void *cfg_ptr,
    char *hostname_ptr)
{
    if (TAPP_PASS != TAPP_mockSappInit(cfg_ptr, global_ptr)) {
        return(-1);
    }
    return (0);
}

/*
 * ======== SAPP_shutdown() ========
 *
 * This function is to shutdown Mock SAPP .
 * This function will call the TAPP_mockSappShutdown()  function
 * 
 * Return Values:
 *   0: initialize successfully.
 *   -1: initialize fialed.
 */
int SAPP_shutdown()
{
    if (TAPP_PASS != TAPP_mockSappShutdown(global_ptr)) {
        return(-1);
    }
    return (0);
}

/* mock functions to satisfy linker */
vint SAPP_allocate()
{
    return (OSAL_SUCCESS);
}

vint SAPP_start(void)
{
    return (OSAL_SUCCESS);
}

vint SAPP_destroy(void)
{
    return (OSAL_SUCCESS);
}
