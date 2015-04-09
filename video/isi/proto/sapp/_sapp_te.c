/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20829 $ $Date: 2013-05-27 12:17:43 +0800 (Mon, 27 May 2013) $
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
#include "_sapp_te.h"

/* 
 * ======== _SAPP_telEvtIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for "telephone event" (such as DTMF digits mwi in protocol signalling) 
 * related events. These events will be passed from SAPP to the ISI module. 
 *
 * Returns: 
 *   Nothing.
 */  
static void _SAPP_teIsiEvt(
    ISI_Id              telEvtId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              callId,
    ISIP_TelEvtReason   reason,
    ISI_TelEvent        evt,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = telEvtId;
    isi_ptr->code = ISIP_CODE_TEL_EVENT;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.event.reason = reason;
    isi_ptr->msg.event.serviceId = serviceId;
    isi_ptr->msg.event.callId = callId;
    isi_ptr->msg.event.evt = evt;
    return;
}

/* 
 * ======== _SAPP_teCallForwardCmd() ========
 * 
 * This function is the handler for a telelphone event for setting
 * "Conditional Call Forwarding" for a particular service.  This function
 * is called in response to an ISI command for setting "conditional call 
 * forwarding".
 *
 * service_ptr : A pointer to the SAPP_ServiceObj that manages this service.
 * 
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 * 
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 *
 * Returns: 
 *   Nothing.
 */
static void _SAPP_teCallForwardCmd(
    SAPP_TelEvt         *tel_ptr,
    SAPP_ServiceObj     *service_ptr,    
    ISIP_Message        *cmd_ptr, 
    ISIP_Message        *isi_ptr)
{
    ISIP_TelEvent   *te_ptr;
    
    te_ptr = &cmd_ptr->msg.event;
    
    /* 
     * Verify the call forward condition.  For SIP only 'unconditional'
     * call forward is allowed
     */
    if (ISI_FORWARD_UNCONDITIONAL != te_ptr->settings.forward.condition) {
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId, 
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        return;
    }
    if (0 != te_ptr->settings.forward.enable) {
        /* Then set the 'call forwarding target' address */
        OSAL_snprintf(service_ptr->callForward, SAPP_STRING_SZ, "%s", 
                te_ptr->to);
    }
    else {
        /* Then disable by clearing the 'call forwarding target' address */
        service_ptr->callForward[0] = 0;
        
    }
    _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
            service_ptr->protocolId, te_ptr->callId,
            ISIP_TEL_EVENT_REASON_COMPLETE, te_ptr->evt, isi_ptr);
    return;
}

/* 
 * ======== _SAPP_teDtmfCmd() ========
 * 
 * This function is the handler for a telelphone event for sending DTMF
 * digits to a remote endpoint.  This function is called in response to 
 * an ISI command for sending DTMRF digit telephone events.
 *
 * service_ptr : A pointer to the SAPP_ServiceObj that manages this service.
 * 
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 * 
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 *
 * Returns: 
 *   Nothing.
 */
static void _SAPP_teDtmfCmd(
    SAPP_TelEvt         *tel_ptr,
    SAPP_ServiceObj     *service_ptr,    
    ISIP_Message        *cmd_ptr, 
    ISIP_Message        *isi_ptr)
{
    SAPP_CallObj    *call_ptr;
    ISIP_TelEvent   *te_ptr;
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;
    tUaMsgBody       body;
    
    te_ptr = &cmd_ptr->msg.event;
    
    /* 
     * Try to find the call this is for.  If it doesn't exist 
     * then tell ISI about the error. 
     */
    call_ptr = SAPP_findCallViaIsiId(service_ptr, te_ptr->callId);
    if (call_ptr == NULL) {
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId,  
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        return;
    }

    /* cache the ID from ISI */
    tel_ptr->id = cmd_ptr->id;
    /* Cache the event type */
    tel_ptr->evt = te_ptr->evt;
    
    numHdrFlds = 0;
    /* Set up the "Content-Type" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0], 
        SAPP_STRING_SZ, "Content-Type: %s", "application/dtmf-relay");
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Prepare body. */
    body.length = OSAL_snprintf(service_ptr->payloadStratch, 
            SAPP_PAYLOAD_SZ, "Signal=%c%sDuration=%d",
            (char)te_ptr->settings.args.arg0, SAPP_END_OF_LINE,
            te_ptr->settings.args.arg1);
    if (body.length > SAPP_PAYLOAD_SZ) {
        /* Message was truncated but send anyway */
        body.length = SAPP_PAYLOAD_SZ;
    }
    body.pBody = service_ptr->payloadStratch;
    
    /* Send using SIP INFO */
    if (UA_Info(service_ptr->sipConfig.uaId, call_ptr->dialogId, 
            hdrFlds_ptr, numHdrFlds, &body, 
            &service_ptr->sipConfig.localConn) != SIP_OK) {
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId, 
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        SAPP_dbgPrintf("%s: Unable to send DTMF relay\n", __FUNCTION__);
    }
    return;
}

/*
 * ======== _SAPP_teFlashCmd() ========
 *
 * This function is the handler for a telelphone event for sending
 * a flash hook event to a remote endpoint.  This function is called in
 * response to  an ISI command for sending "flashhook" telephone events.
 *
 * tel_ptr : A pointer to a SAPP_TelEVt object used to manage this
 *           sip INFO transaction.  It's used to match responses to
 *           the original request.
 *
 * service_ptr : A pointer to the SAPP_ServiceObj that manages this service.
 *
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 *
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 *
 * Returns:
 *   Nothing.
 */
static void _SAPP_teFlashCmd(
    SAPP_TelEvt         *tel_ptr,
    SAPP_ServiceObj     *service_ptr,
    ISIP_Message        *cmd_ptr,
    ISIP_Message        *isi_ptr)
{
    SAPP_CallObj    *call_ptr;
    ISIP_TelEvent   *te_ptr;
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;
    tUaMsgBody       body;

    te_ptr = &cmd_ptr->msg.event;

    /*
     * Try to find the call this is for.  If it doesn't exist
     * then tell ISI about the error.
     */
    call_ptr = SAPP_findCallViaIsiId(service_ptr, te_ptr->callId);
    if (call_ptr == NULL) {
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        return;
    }

    /* cache the ID from ISI */
    tel_ptr->id = cmd_ptr->id;
    /* Cache the event type */
    tel_ptr->evt = te_ptr->evt;

    numHdrFlds = 0;
    /* Set up the "Content-Type" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
        SAPP_STRING_SZ, "Content-Type: %s", "application/broadsoft");
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Prepare body. */
    body.length = OSAL_snprintf(service_ptr->payloadStratch,
            SAPP_PAYLOAD_SZ, "%s", "event flashhook");
    if (body.length > SAPP_PAYLOAD_SZ) {
        /* Message was truncated but send anyway */
        body.length = SAPP_PAYLOAD_SZ;
    }
    body.pBody = service_ptr->payloadStratch;

    /* Send using SIP INFO */
    if (UA_Info(service_ptr->sipConfig.uaId, call_ptr->dialogId,
            hdrFlds_ptr, numHdrFlds, &body,
            &service_ptr->sipConfig.localConn) != SIP_OK) {
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, isi_ptr);
        SAPP_dbgPrintf("%s: Unable to send DTMF relay\n", __FUNCTION__);
    }
    return;
}

/* 
 * ======== SAPP_teInit() ========
 *
 * This function is used to initialize an object used to manage telephone event
 * transactions.  All internal object members will be reset to a good initial 
 * state.  This function is typically when ISI has instructed SAPP to init
 * a service.
 *
 * Returns: 
 *   Nothing.
 */
void SAPP_teInit(
    SAPP_TelEvt   *tel_ptr)
{
    tel_ptr->evt = ISI_TEL_EVENT_INVALID;
    tel_ptr->hTransaction = 0;
    tel_ptr->id = 0;
    return;
}


/* 
 * ======== _SAPP_isiTelEventCmd() ========
 * 
 * This function is the entry point for commands that come from ISI 
 * that pertain to "Telephone Events".  It's the first place a tel event 
 * command begins to be processed.
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
void SAPP_isiTeCmd(
    ISIP_Message        *cmd_ptr, 
    SAPP_SipObj         *sip_ptr,
    SAPP_Event          *evt_ptr)
{
    SAPP_ServiceObj *service_ptr;
    ISIP_TelEvent   *te_ptr;
    
    te_ptr = &cmd_ptr->msg.event;
    
    service_ptr = SAPP_findServiceViaServiceId(sip_ptr, te_ptr->serviceId);
    if (service_ptr == NULL) {
        /* Doesn't exist, tell ISI about this error */
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                cmd_ptr->protocol, te_ptr->callId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, &evt_ptr->isiMsg);
        return;
    }
    
    /* We only want the "new" reason */
    if (ISIP_TEL_EVENT_REASON_NEW != te_ptr->reason) {
        /* Then we are not interested in this event. Tell ISI about error. */
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, &evt_ptr->isiMsg);
        return;
    }
    
    if (ISI_TEL_EVENT_CALL_FORWARD == te_ptr->evt) {
        /* Currently we only support DTMF digits */
        _SAPP_teCallForwardCmd(&service_ptr->telEvt, service_ptr, cmd_ptr,
                &evt_ptr->isiMsg);
    }
    else if (ISI_TEL_EVENT_DTMF_OOB == te_ptr->evt) {
        _SAPP_teDtmfCmd(&service_ptr->telEvt, service_ptr, cmd_ptr,
                &evt_ptr->isiMsg);
    }
    else if (ISI_TEL_EVENT_FLASHHOOK == te_ptr->evt) {
        _SAPP_teFlashCmd(&service_ptr->telEvt, service_ptr, cmd_ptr,
                &evt_ptr->isiMsg);
    }
    else {
        /* Then the event is not supported return an error */
        _SAPP_teIsiEvt(cmd_ptr->id, te_ptr->serviceId,
                service_ptr->protocolId, te_ptr->callId,
                ISIP_TEL_EVENT_REASON_ERROR, te_ptr->evt, &evt_ptr->isiMsg);
    }
    return;
}

/* 
 * ======== SAPP_teEvent() ========
 * 
 * This function is the handler for NOTIFY or MESSAGE requests that are related to
 * telephone events. 
 *
 * Returns: 
 *  SAPP_ERR: The SIP NOTIFY/MESSAGE was not handled by this routine.  Further
 *            processing of this SIP event should continue.
 *  SAPP_OK:  The SIP NOTIFY/MESSAGE event was handled by this routine and no
 *            further processing of this SIP event is needed.
 */
vint SAPP_teEvent(
    SAPP_TelEvt     *tel_ptr,
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *sipEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    uint32           arg0;
    uint32           arg1;
    SAPP_CallObj    *call_ptr;
    vint             value;
    char            *value_ptr;
        
    switch (sipEvt_ptr->header.type) {
        case eUA_INFO_FAILED:
            _SAPP_teIsiEvt(tel_ptr->id, service_ptr->isiServiceId,
                    service_ptr->protocolId, 0, 
                    ISIP_TEL_EVENT_REASON_ERROR, tel_ptr->evt,
                    &evt_ptr->isiMsg);
            break;
        
        case eUA_INFO_COMPLETED:
            _SAPP_teIsiEvt(tel_ptr->id, service_ptr->isiServiceId,
                    service_ptr->protocolId, 0, 
                    ISIP_TEL_EVENT_REASON_COMPLETE, tel_ptr->evt,
                    &evt_ptr->isiMsg);
            break;
    
        case eUA_INFO:
            /* Get the call object associated with this dialog */
            if (NULL == (call_ptr = SAPP_findCallViaDialogId(service_ptr, 
                    sipEvt_ptr->header.hOwner))) {
                /* Not for an active call. Quietly ignore */
                return (SAPP_ERR);
            }
       
            /* 
             * Currently we are only interested in INFO's used for DTMF signalling.
             * So check the content type.
             */
            if (0 == sipEvt_ptr->msgBody.payLoad.length || 
                    0 == SAPP_parseHfValueExist("Content-Type:", "dtmf",
                    sipEvt_ptr)) {
                /* Then we don't know what this is for.  Quietly ignore */
                return (SAPP_ERR);
            }
        
            /* Check for the 'signal' a.k.a. the dtmf digit */
            if (SAPP_parsePayloadValue(sipEvt_ptr->msgBody.payLoad.data,
                    "Signal=", &value_ptr, &value) != SAPP_OK) {
                /* Couldn't find the dtmf digit value.  Quietly ignore */
                return (SAPP_ERR);
            }
            arg0 = (uint32)*value_ptr;
    
            /* Check for the 'duration' */
            if (SAPP_parsePayloadValue(sipEvt_ptr->msgBody.payLoad.data,
                    "Duration=", &value_ptr, &value) != SAPP_OK) {
                /* Couldn't find the dtmf digit value.  Quietly ignore */
                return (SAPP_ERR);
            }
            
            /* Calculate the max size, copy, NULL terminate, convert to number */
            value = (value >= SAPP_PAYLOAD_SZ) ? (SAPP_PAYLOAD_SZ - 1) : value;
            OSAL_memCpy(service_ptr->payloadStratch, value_ptr, value);
            service_ptr->payloadStratch[value] = 0;
            arg1 = (uint32)OSAL_atoi(service_ptr->payloadStratch);
    
            SAPP_dbgPrintf("%s: Received INFO DTMF digit:%c duration:%d\n", 
                    __FUNCTION__, arg0, arg1);
            
            /* Populate the ISI event and notify ISI  */
            _SAPP_teIsiEvt(0, service_ptr->isiServiceId, 
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_TEL_EVENT_REASON_NEW, ISI_TEL_EVENT_DTMF_OOB,
                    &evt_ptr->isiMsg);
            
            evt_ptr->isiMsg.msg.event.settings.args.arg0 = arg0;
            evt_ptr->isiMsg.msg.event.settings.args.arg1 = arg1;
            break;
        default:
            return (SAPP_ERR);
    }
    return (SAPP_OK);
}
