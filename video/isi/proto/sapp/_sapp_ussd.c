/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20219 $ $Date: 2013-03-26 23:26:48 -0700 (Tue, 26 Mar 2013) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <ezxml.h>
#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include "isi.h"
#include "isi_errors.h"
#include "isip.h"
#include "mns.h"
#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_coder_helper.h"
#include "_sapp_ipsec.h"

/* USSD XML patterns*/
static const char _USSD_XML_HEADER[]             = "<?xml version='1.0' encoding='UTF-8'?>";
static const char _USSD_XML_TAG_USSD_DATA[]      = "ussd-data";
static const char _USSD_XML_TAG_USSD_STRING[]    = "ussd-string";
static const char _USSD_XML_TAG_USSD_LANGUAGE[]  = "language";
static const char _USSD_CONTENT_TYPE[]           = "application/vnd.3gpp.ussd+xml";

/*
 * ======== SAPP_findUssdViaIsiId() ========
 *
 * This function will search for a ussd with in the service specified by
 * service_ptr. If a ussd was found that matches the call ID specified
 * in "callId" then a pointer to the call object is returned.
 * Otherwise if the ussdId can not be found then NULL is returned.
 *
 * Returns:
 *   SAPP_CallObj *: A pointer to the call object that has the same
 *                      call ID as "ussdId"
 *   NULL : The call specified by callId does not exist.
 */
static SAPP_CallObj* _SAPP_findUssdViaIsiId(
    SAPP_ServiceObj *service_ptr,
    ISI_Id           ussdId)
{
    if (service_ptr->ussd.aUssd.isiCallId == ussdId) {
        return (&service_ptr->ussd.aUssd);
    }
    return (NULL);
}

/*
 * ======== SAPP_findCallViaDialogId() ========
 *
 * This function will search for a ussd with in the service specified by
 * service_ptr. If a ussd was found that matches the dialog ID specified
 * in "dialogId" then a pointer to the call object is returned.
 * Otherwise if the dialogId can not be found then NULL is returned.
 *
 * Returns:
 *   SAPP_CallObj *: A pointer to the call object that has the same
 *                      dialog ID as "dialogId"
 *   NULL : The call specified by dialogId does not exist.
 */
SAPP_CallObj* _SAPP_findUssdViaDialogId(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       dialogId)
{
    if (service_ptr->ussd.aUssd.dialogId == dialogId) {
        return (&service_ptr->ussd.aUssd);
    }
    return (NULL);
}

/*
 * ======== _SAPP_sipUssdDecodeXML() ========
 *
 * This function is to decode USSD XML content and extract message from <ussd-string> tag.
 *
 * service_ptr : A pointer to the service that this ussd event belongs to.
 *
 * payload_ptr : A pointer to USSD XML content
 *
 * payloadLen : Length of USSD XML content
 *
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 * Returns:
 *  SAPP_OK : Can get  message from <ussd-string> tag.
 *  SAPP_ERR : Function failed
 */

static vint _SAPP_sipUssdDecodeXML(
    SAPP_ServiceObj *service_ptr,
    char            *payload_ptr,
    vint             payloadLen,
    ISIP_Message    *isi_ptr)
{
    ezxml_t     xml_ptr;
    ezxml_t     child_ptr;
    const char *value_ptr;

    if (payloadLen > SAPP_PAYLOAD_SZ) {
        return (SAPP_ERR);
    }
    OSAL_memCpy(service_ptr->payloadStratch, payload_ptr, payloadLen);
    if (NULL == (xml_ptr = ezxml_parse_str(service_ptr->payloadStratch,
            payloadLen))) {
        return (SAPP_ERR);
    }

    /* Check for the mandatory 'ussd-data' root tag */
    if (NULL == (value_ptr = ezxml_name(xml_ptr))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    if (0 != OSAL_strncasecmp(value_ptr, _USSD_XML_TAG_USSD_DATA,
            OSAL_strlen(_USSD_XML_TAG_USSD_DATA))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }

    /* Get the "ussd-string" child element */
    if (NULL == (child_ptr = ezxml_child(xml_ptr,
            _USSD_XML_TAG_USSD_STRING))) {
        ezxml_free(xml_ptr);
        return (SAPP_ERR);
    }
    OSAL_strncpy(isi_ptr->msg.ussd.message, ezxml_txt(child_ptr), 
        ISI_TEXT_STRING_SZ);

    /* Free the xml object since we don't need it anymore */
    ezxml_free(xml_ptr);
    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipUssdEncodeXML() ========
 *
 * This function is to encode ussdString as a USSD XML content.
 *
 * ussdString : A pointer to content for <ussd-string> tag.
 *
 * doc_ptr : buffer to put USSD XML content
 *
 * docLength : Buffer length of doc_ptr
 *
 * Returns:
 *  SAPP_OK : Success to encode XML
 *  SAPP_ERR : Function failed
 */

static vint _SAPP_sipUssdEncodeXML(char* ussdString, char* doc_ptr, int docLength) {
    ezxml_t xml_ptr;
    ezxml_t language_ptr;
    ezxml_t ussd_string_ptr;
    char    *str_ptr;
    
    xml_ptr = ezxml_new(_USSD_XML_TAG_USSD_DATA);

    /* Add language tag*/
    language_ptr = ezxml_add_child(xml_ptr, _USSD_XML_TAG_USSD_LANGUAGE, 0);
    ezxml_set_txt(language_ptr, "en");

    /* Add ussd-string tag*/
    ussd_string_ptr = ezxml_add_child(xml_ptr, _USSD_XML_TAG_USSD_STRING, 0);
    ezxml_set_txt(ussd_string_ptr, ussdString);
    str_ptr = ezxml_toxml(xml_ptr);

    /* Always free it here, even if it couldn't be constructed */
    ezxml_free(xml_ptr);
    if (str_ptr == NULL) {
        return SAPP_ERR;
    }

    /* Add XML header */
    OSAL_snprintf(doc_ptr, docLength, "%s%s%s", _USSD_XML_HEADER,
            SAPP_END_OF_LINE, str_ptr);
    OSAL_memFree(str_ptr, OSAL_MEM_ARG_DYNAMIC_ALLOC);
    return SAPP_OK;
}

/*
 * ======== _SAPP_sipUssdIsiEvt() ========
 *
 * This function is used to populate a ISI event
 * related to ussd. These events will be passed from SAPP
 * to the ISI module.
 *
 * service_ptr : A pointer to the service that this ussd event belongs to.
 *
 * callId: ISID number
 *
 * reason: reason for ISIP
 *
 * isi_ptr: A pointer to a buffer where ISI events are written.  Events
 *          written here all ultimately sent up to the ISI module.
 *
 * Returns:
 * Nothing.
 */
static void _SAPP_sipUssdIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              callId,
    ISIP_UssdReason     reason,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = callId;
    isi_ptr->code = ISIP_CODE_USSD;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.ussd.reason = reason;
    isi_ptr->msg.ussd.serviceId = serviceId;
    return;
}

/*
 * ======== _SAPP_sipUssdIsiEvtHelper() ========
 *
 * A helper function for _SAPP_sipUssdIsiEvt. Fill id in ISIP_Message 
 * with the one got from SAPP_isiUssdCmd.
 *
 * service_ptr : A pointer to the service that this ussd event belongs to.
 *
 * reason: reason for ISIP
 *
 * isi_ptr: A pointer to a buffer where ISI events are written.  Events
 *          written here all ultimately sent up to the ISI module.
 * Returns:
 * Nothing.
 */

static void _SAPP_sipUssdIsiEvtHelper(
    SAPP_ServiceObj *service_ptr,
    ISIP_UssdReason     reason,
    ISIP_Message       *isi_ptr)
{
    _SAPP_sipUssdIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
        service_ptr->ussd.id, reason, isi_ptr);
    service_ptr->ussd.id = 0;
    return;
}

/*
 * ======== _SAPP_sipUssdRsrcRsrvTimerStop() ========
 * Stop resource reservation timer.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
static vint _SAPP_sipUssdRsrcRsrvTimerStop(
    SAPP_CallObj     *call_ptr)
{
    if ((0 == call_ptr->rsrcRsrvTimerId) &&
            (0 == call_ptr->rsrcRsrvRetryTimerId)) {
        /* Timer doesn't exist. */
        return (SAPP_ERR);
    }

    if (0 != call_ptr->rsrcRsrvTimerId) {
        OSAL_tmrStop(call_ptr->rsrcRsrvTimerId);
        OSAL_tmrDelete(call_ptr->rsrcRsrvTimerId);
        call_ptr->rsrcRsrvTimerId = 0;
    }

    if (0 != call_ptr->rsrcRsrvRetryTimerId) {
        OSAL_tmrStop(call_ptr->rsrcRsrvRetryTimerId);
        OSAL_tmrDelete(call_ptr->rsrcRsrvRetryTimerId);
        call_ptr->rsrcRsrvRetryTimerId = 0;
    }

    return (SAPP_OK);
}

/*
 * ========_SAPP_sipDisconnect() ========
 * This is functiuon is send USSD BYE to peer
 */
static vint _SAPP_sipDisconnect(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       hDialog)
{
    char  *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint   numHdrFlds;

    numHdrFlds = 0;

    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }
    return (UA_HungUp(service_ptr->sipConfig.uaId, hDialog, hdrFlds_ptr, numHdrFlds,
            &service_ptr->sipConfig.localConn));
}

/*
 * ======== _SAPP_sipDestroyUssd() ========
 *
 * This function will mark a ussd as destroyed by clearing the
 * 'isiCallId' value (The ID used by ISI to specify a call) and
 * the 'dialogId' value (The ID/handle SIP uses to specify a SIP call).
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_sipDestroyUssd(
    SAPP_CallObj *call_ptr,
    SAPP_ServiceObj *service_ptr)
{
    OSAL_logMsg("%s\n", __FUNCTION__);
    service_ptr->ussd.id = 0;
    call_ptr->isiCallId = 0;
    call_ptr->dialogId = 0;
    call_ptr->blockCid = 0;
    /* Stop resource reservation timer anyway */
    _SAPP_sipUssdRsrcRsrvTimerStop(call_ptr); 
    MNS_clearSession(&call_ptr->mnsSession);
    return;
}

/*
 * ======== _SAPP_sipMakeUssd() ========
 *
 * This function is called when a ussd needs to be placed over SIP.
 * It will call the SIP API function for placing a ussd.
 *
 * Returns:
 *  SAPP_OK : The ussd was successfully placed and the call_ptr->dialogId
 *            will be correctly populated with a handle to the SIP dialog.
 *  SAPP_ERR : Function failed.  The SIP API failed.
 */
static vint _SAPP_sipMakeUssd(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    char            *to_ptr,
    char            *message_ptr,
    tSession        *sess_ptr)
{
    char  *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint   numHdrFlds;
    char  *from_ptr;
    char  *displayName_ptr;

    numHdrFlds = 0;

    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF,
            service_ptr->sipConfig.config.aor[0].szUri);

    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

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

    from_ptr = NULL;
    displayName_ptr = NULL;

    /*
     * Setting from as tel uri, if this call is emergency and there is tel uri.
     */
    if ((service_ptr->isEmergency) && (NULL != service_ptr->telUri_ptr) &&
            (0 != service_ptr->telUri_ptr[0])) {
        from_ptr = service_ptr->telUri_ptr;
    }
    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
        call_ptr->useIpSec = 1; 
    }

    /*Add Accept*/
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_ACCEPT_HF, _SAPP_USSD_ACCEPT_ARG);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    
    /*Add Recv-Info*/
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_RECV_INFO_HF, _SAPP_USSD_RECV_INFO_ARG);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

    OSAL_snprintf(sess_ptr->otherPayload, MAX_SESSION_PAYLOAD_SIZE, 
        "Content-Type: application/vnd.3gpp.ussd+xml%s%s", SIP_CRLF, SIP_CRLF);
    _SAPP_sipUssdEncodeXML(message_ptr, sess_ptr->otherPayload + OSAL_strlen(sess_ptr->otherPayload),
        MAX_SESSION_PAYLOAD_SIZE - OSAL_strlen(sess_ptr->otherPayload));

    call_ptr->dialogId = UA_MakeCall(service_ptr->sipConfig.uaId, to_ptr,
            from_ptr, displayName_ptr, hdrFlds_ptr, numHdrFlds, sess_ptr,
            service_ptr->exchangeCapabilitiesBitmap,
            &service_ptr->sipConfig.localConn, OSAL_FALSE);
    if (call_ptr->dialogId != 0) {
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== _SAPP_getUssdReasonDesc() ========
 *
 * This function is to get reason for ussd failure case.
 *
 * Returns:
 *   Nothing.
 */

static void _SAPP_getUssdReasonDesc(
    tUaAppEvent *uaEvt_ptr,
    ISIP_Ussd   *isi_ptr)
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
            "USSD FAILED: CODE:%d REASON:%s", respCode, reason_ptr);
    return;
}


/*
 * ======== _SAPP_sipUssdInitiateOutbound(() ========
 *
 * This function is called when ISI has commanded SAPP to initiate a
 * ussd.
 *
 * sip_ptr : A pointer to a SAPP_SipObj object used to manage the SIP stack.
 *
 * service_ptr : A pointer to the service that this ussd is being placed
 *               within.
 * call_ptr : A pointer to a SAPP_CallObj object used internally by
 *            SAPP to manage the ussd.
 *
 * cmd_ptr : A pointer to the command issued by ISI.  This object contains
 *           all the details of the command.
 *
 * Returns:
 *   SAPP_OK : Function was successful.  Ussd was placed.
 *   SAPP_ERR : Function failed.  The ussd could not be placed.
 */
static vint _SAPP_sipUssdInitiateOutbound(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message    *cmd_ptr)
{
    ISIP_Ussd       *u_ptr;
    ISIP_Call        call;
    char            *phoneContext_ptr;

    // setup the call object
    call_ptr->isiCallId = cmd_ptr->id;
    call_ptr->modify = SAPP_MODIFY_NONE;
    u_ptr = &cmd_ptr->msg.ussd;

    /* Set the RTP IP Address & port pair to use for this ussd */
    service_ptr->mnsService.aAddr = service_ptr->artpInfc.nextInfc;
    service_ptr->mnsService.aAddr.port = 0;
    service_ptr->mnsService.vAddr = service_ptr->vrtpInfc.nextInfc;
    service_ptr->mnsService.vAddr.port = 0;
    call.audioDirection = ISI_SESSION_DIR_INACTIVE;
    call.videoDirection = ISI_SESSION_DIR_INACTIVE;
    
    /* Let's prepare the details of the media for this invite */
    call.type = ISI_SESSION_TYPE_AUDIO;
    if (SAPP_OK != _SAPP_encodeMediaSapp2Sip(service_ptr, call_ptr, &call)) {
        return (SAPP_ERR);
    }

    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    /*
     * Set phone context, use phoneContext if it's configured,
     * otherwise uses realm
     */
    if (0 != service_ptr->phoneContext[0]) {
        phoneContext_ptr = service_ptr->phoneContext;
    }
    else {
        phoneContext_ptr =
                service_ptr->sipConfig.config.authCred[0].szAuthRealm;
    }
    SAPP_parseAddPhoneContext(u_ptr->to, ISI_ADDRESS_STRING_SZ,
            phoneContext_ptr);


    /* Try to place the call over the SIP stack */
    return _SAPP_sipMakeUssd(service_ptr, call_ptr, u_ptr->to, 
            u_ptr->message, call_ptr->mnsSession.sess_ptr);
}

/*
 * ======== _SAPP_ussdInfo(() ========
 *
 * This function is called when ISI has commanded SAPP to reply a INFO request.
 *
 * service_ptr : A pointer to the service that this ussd is being placed
 *               within.
 * call_ptr : A pointer to a SAPP_CallObj object used internally by
 *            SAPP to manage the call.
 *
 * cmd_ptr : A pointer to the command issued by ISI.  This object contains
 *           all the details of the command.
 *
 * Returns:
 *   SAPP_OK : Function was successful.
 *   SAPP_ERR : Function failed.  The ussd could not be placed.
 */

static vint _SAPP_ussdInfo(
    SAPP_ServiceObj     *service_ptr,
    ISIP_Message        *cmd_ptr,
    SAPP_CallObj        *call_ptr)
{
    ISIP_Ussd       *u_ptr;
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;
    tUaMsgBody       body;
    
    u_ptr = &cmd_ptr->msg.ussd;
    
    numHdrFlds = 0;
    /*Add Content-Type*/
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_CONTENT_TYPE_HF, _USSD_CONTENT_TYPE);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    
    /*Add Info-Package*/
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_INFO_PACKAGE_HF, _SAPP_USSD_RECV_INFO_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /*Add Content-Disposition*/
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_CONTENT_DISP_HF, "Info-Package");
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Prepare body. */
    _SAPP_sipUssdEncodeXML(u_ptr->message, service_ptr->payloadStratch, SAPP_PAYLOAD_SZ);
    body.length = OSAL_strlen(service_ptr->payloadStratch);
    body.pBody = service_ptr->payloadStratch;
    
    /* Send using SIP INFO */
    if (UA_Info(service_ptr->sipConfig.uaId, call_ptr->dialogId, 
            hdrFlds_ptr, numHdrFlds, &body, 
            &service_ptr->sipConfig.localConn) != SIP_OK) {
        return SAPP_ERR;
    }
    return SAPP_OK;
}

/*
 * ======== SAPP_isiUssdCmd() ========
 *
 * This function is the entry point for commands from ISI that are related
 * to placing a ussd.  This is the first place that ISI commands for Ussd
 * begin to be processed.
 *
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 *
 * sip_ptr : A pointer to an object used internally in SAPP to manage
 *           services/Ussds.
 *
 * evt_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 * Returns:
 *   Nothing.
 */
vint SAPP_isiUssdCmd(
    ISIP_Message     *cmd_ptr,
    SAPP_SipObj      *sip_ptr,
    SAPP_Event       *evt_ptr)
{
    SAPP_ServiceObj *service_ptr = NULL;
    SAPP_CallObj    *call_ptr = NULL;
    ISIP_Ussd       *u_ptr = NULL;
    ISIP_Message    *isi_ptr = NULL;
    vint             errcode = SAPP_ERR;

    u_ptr = &cmd_ptr->msg.ussd;
    isi_ptr = &evt_ptr->isiMsg;

    service_ptr = SAPP_findServiceViaServiceId(sip_ptr, u_ptr->serviceId);
    if (service_ptr == NULL) {
        /* Doesn't exist, simply ignore */
        goto errorExit;
    }
    
    OSAL_logMsg("%s: reason=%d\n", __FUNCTION__, u_ptr->reason);
    if (u_ptr->reason == ISIP_USSD_REASON_SEND) {
        if (service_ptr->ussd.aUssd.isiCallId != 0) {
            /* Only allow one USSD. */
            goto errorExit;
        }
        /*
              * Then it's a new ussd request.  See if there is an
              * available resource.
              */
        call_ptr = _SAPP_findUssdViaIsiId(service_ptr, 0);
        if (call_ptr == NULL) {
            /* Then there no available, tell ISI of the failure */
            goto errorExit;
        }
        /* Init the call object */
        OSAL_memSet(call_ptr, 0, sizeof(SAPP_CallObj));
    }
    else {
        if (service_ptr->ussd.aUssd.isiCallId == 0) {
            /* Should use ISIP_USSD_REASON_SEND first to setup USSD.*/
            goto errorExit;
        }
        call_ptr = &service_ptr->ussd.aUssd;
    }

    /* save ISID's id */
    service_ptr->ussd.id = cmd_ptr->id;
    
    /* Load the coders in the ISI command into the call object. */
    _SAPP_decodeCoderIsi2Sapp(u_ptr->coders, ISI_CODER_NUM,
            call_ptr->coders, ISI_CODER_NUM);

    /* Act on the 'reason' */
    switch (u_ptr->reason) {
    case ISIP_USSD_REASON_SEND:
        if (_SAPP_sipUssdInitiateOutbound(sip_ptr, service_ptr, call_ptr,
                cmd_ptr) != SAPP_OK) {
            goto errorExit;
        }
        break;
    case ISIP_USSD_REASON_DISCONNECT:
        /* Disconnect the SIP call */
        _SAPP_sipDisconnect(service_ptr, call_ptr->dialogId);
        /* Tell ISI that the call is disconnected */
        _SAPP_sipUssdIsiEvtHelper(service_ptr, ISIP_USSD_REASON_DISCONNECT, isi_ptr);
        _SAPP_sipDestroyUssd(call_ptr, service_ptr);
        break;
    case ISIP_USSD_REASON_REPLY:
        if ( _SAPP_ussdInfo(service_ptr, cmd_ptr, call_ptr) != 
            SAPP_OK) {
            goto errorExit;
        }
        break;
    default:
        goto errorExit;
    }
    errcode = SAPP_OK;
errorExit:
    if (errcode == SAPP_ERR) {
        /* This command got problem. reply send error*/
        _SAPP_getUssdReasonDesc(NULL, &isi_ptr->msg.ussd);
        _SAPP_sipUssdIsiEvt(u_ptr->serviceId, service_ptr->protocolId, 
            cmd_ptr->id, ISIP_USSD_REASON_SEND_ERROR, isi_ptr);
        if (call_ptr != NULL) {
            /* send out ISIP_USSD_REASON_SEND_ERROR*/
            SAPP_sendEvent(evt_ptr);
            /* Found an error. Disconnect USSD if exist*/
            _SAPP_sipDisconnect(service_ptr, call_ptr->dialogId);
            _SAPP_sipUssdIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                0, ISIP_USSD_REASON_DISCONNECT, isi_ptr);
            _SAPP_sipDestroyUssd(call_ptr, service_ptr);
        }
    }
    return errcode;
}

/*
 * ======== SAPP_sipUssdEvent() ========
 *
 * This function is the entry point for events from SIP that are related
 * to Ussds.  This is the first place that SIP events for Ussds
 * begin to be processed.
 *
 * sip_ptr : A pointer to a SAPP_SipObj object used to manage the SIP stack.
 *
 * service_ptr : A pointer to the service that this ussd event belongs to.
 *
 * hUa : A handle to the SIP UA that this event was received within.  This
 *       is the value returned when the UA was created via UA_Create().
 *
 * hDialog : A handle to a SIP dialog.  This is the value that SIP uses to
 *           represent this ussd (a.k.a. a 'dialog' in SIP terms).
 *
 * event : An enumerated value representing the type of event. These events
 *         are defined in sip_ua.h
 *
 * arg : A pointer to a tUaAppEvent object that is populated with all the
 *       details of the event.  Please see SIP interface document for details
 *       regarding the values in this object.
 *
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 * Returns:
 *   Nothing.
 */
vint SAPP_sipUssdEvent(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    tSipHandle       hUa,
    tSipHandle       hDialog,
    tUaEvtType       event,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    SAPP_CallObj    *call_ptr;
    vint             errcode = SAPP_ERR;
    ISIP_Message    *isi_ptr = &evt_ptr->isiMsg;
    char            *contentType_ptr;

    /* Is Ussd Event*/
    call_ptr = _SAPP_findUssdViaDialogId(service_ptr, hDialog);
    if (call_ptr == NULL) {
        return(SAPP_ERR);
    }

    OSAL_logMsg("%s: event=%d\n", __FUNCTION__, event);
    switch (event) {
    case eUA_RESPONSE:
        OSAL_logMsg("%s: respCode=%d\n", __FUNCTION__, uaEvt_ptr->resp.respCode);
        if (uaEvt_ptr->resp.respCode < 200) {
            /* Then it's only a provisional response. Quietly discard */
            break;
        }
        goto errorExit;
    case eUA_INFO:
    case eUA_CALL_DROP:
        isi_ptr->msg.ussd.message[0] = '\0';
        /* Get ussd XML content and parse it*/
        contentType_ptr = SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, uaEvt_ptr);
        if (contentType_ptr != NULL && 
            (0 == OSAL_strncasecmp(contentType_ptr, _USSD_CONTENT_TYPE,
                OSAL_strlen(_USSD_CONTENT_TYPE)))) {
            if (SAPP_OK != _SAPP_sipUssdDecodeXML(service_ptr, 
                uaEvt_ptr->msgBody.payLoad.data,
                uaEvt_ptr->msgBody.payLoad.length, isi_ptr)) {
                isi_ptr->msg.ussd.message[0] = '\0';
            }
        }
        if (event == eUA_CALL_DROP) {
            if (service_ptr->ussd.id != 0) {
                /* There is an ongoing request. Make this request as a error. */
                _SAPP_sipUssdIsiEvtHelper(service_ptr, ISIP_USSD_REASON_SEND_ERROR, isi_ptr);
                SAPP_sendEvent(evt_ptr);
            }
            _SAPP_sipUssdIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId, 
                0, ISIP_USSD_REASON_DISCONNECT, isi_ptr);
            _SAPP_sipDestroyUssd(call_ptr, service_ptr);
        }
        else {
            _SAPP_sipUssdIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId, 
                0, ISIP_USSD_REASON_REQUEST, isi_ptr);
        }
        break;
    case eUA_RINGING:
        break;
    case eUA_ANSWERED:
        /* If MNS is already in ACTIVE state, ignore the SDP in 200 OK */
        if (!MNS_isSessionActive(&call_ptr->mnsSession)) {
            if (uaEvt_ptr->sessNew) {
                if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                        &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                    /* Then we can't agree on any thing so disconnect the ussd */
                    goto errorExit;
                }
            }
        }

        if (!MNS_isSessionActive(&call_ptr->mnsSession)) {
            /* Got SDP offer in 200 OK, generate SDP answer in ACK. */

            /* Run MNS state machine */
            MNS_processCommand(&service_ptr->mnsService, 
                    &call_ptr->mnsSession, NULL);
        }

        /* Send ACK to peer */
        UA_Ack(service_ptr->sipConfig.uaId, hDialog, NULL, 0,
                call_ptr->mnsSession.sess_ptr, 
                &service_ptr->sipConfig.localConn);
        _SAPP_sipUssdIsiEvtHelper(service_ptr, ISIP_USSD_REASON_SEND_OK, isi_ptr);
        break;
    case eUA_INFO_COMPLETED:
        _SAPP_sipUssdIsiEvtHelper(service_ptr, ISIP_USSD_REASON_SEND_OK, isi_ptr);
        break;
    default:
        goto errorExit;
    }
    errcode = SAPP_OK;
errorExit:
    if (errcode == SAPP_ERR) {
        /* Request got a problem*/
        _SAPP_getUssdReasonDesc(uaEvt_ptr, &isi_ptr->msg.ussd);
        _SAPP_sipUssdIsiEvtHelper(service_ptr, ISIP_USSD_REASON_SEND_ERROR, isi_ptr);
        if (call_ptr != NULL) {
            /* send out ISIP_USSD_REASON_SEND_ERROR*/
            SAPP_sendEvent(evt_ptr);
            /* Found an error. Disconnect USSD*/
            _SAPP_sipDisconnect(service_ptr, call_ptr->dialogId);
            _SAPP_sipUssdIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId, 
                0, ISIP_USSD_REASON_DISCONNECT, isi_ptr);
            _SAPP_sipDestroyUssd(call_ptr, service_ptr);
        }
    }
    return (SAPP_OK);    
}


