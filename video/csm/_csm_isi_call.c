/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include <csm_event.h>
#include <isi.h>
#include <osal.h>
#include <isi_rpc.h>
#include "_csm.h"
#include "_csm_isi.h"
#include "_csm_isi_call.h"

/*
 *  ======== _CSM_isiCallTypeEventHandler() ========
 *
 *  Private handler for processing ISI "call" type events.
 *
 *  RETURN:
 *      None
 */
void _CSM_isiCallTypeEventHandler(
    CSM_IsiMngr    *isiMngr_ptr,
    ISI_Id          isiServiceId,
    ISI_Id          isiCallId,
    ISI_Event       event,
    const char     *desc_ptr)
{
    CSM_PrivateInputEvt     *event_ptr = &isiMngr_ptr->csmInputEvent;
    CSM_IsiCallReason        reason;
    CSM_InputIsiCall        *call_ptr;

    call_ptr = &event_ptr->evt.call;

    CSM_dbgPrintf("event:%d\n", event);

    /* Contruct the CSM event mapped from ISI */
    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_CALL;
    switch (event) {
        case ISI_EVENT_CALL_INCOMING:
            reason = CSM_CALL_REASON_EVT_NEW_INCOMING;
            break;
        case ISI_EVENT_CALL_FAILED:
        case ISI_EVENT_NET_UNAVAILABLE:
            reason = CSM_CALL_REASON_EVT_FAILED;
            break;
        case ISI_EVENT_CALL_HOLD:
            reason = CSM_CALL_REASON_EVT_HOLD;
            break;
        case ISI_EVENT_CALL_RESUME:
            reason = CSM_CALL_REASON_EVT_RESUME;
             break;
        case ISI_EVENT_CALL_REJECTED:
            reason = CSM_CALL_REASON_EVT_REJECTED;
            break;
        case ISI_EVENT_CALL_ACCEPTED:
            reason = CSM_CALL_REASON_EVT_ACCEPTED;
            break;
        case ISI_EVENT_CALL_ACCEPT_ACK:
            reason = CSM_CALL_REASON_EVT_ACCEPT_ACK;
            break;
        case ISI_EVENT_CALL_ACKNOWLEDGED:
            reason = CSM_CALL_REASON_EVT_ACKNOWLEDGED;
            break;
        case ISI_EVENT_CALL_DISCONNECTED:
            reason = CSM_CALL_REASON_EVT_DISCONNECT;
            break;
        case ISI_EVENT_CALL_TRYING:
            reason = CSM_CALL_REASON_EVT_TRYING;
            break;
        case ISI_EVENT_TEL_EVENT_SEND_OK:
        case ISI_EVENT_TEL_EVENT_SEND_FAILED:
            reason = CSM_CALL_REASON_EVT_DIGIT_DONE;
            break;
        case ISI_EVENT_CALL_XFER_FAILED:
            reason = CSM_CALL_REASON_EVT_XFER_FAILED;
            break;
        case ISI_EVENT_CALL_XFER_COMPLETED:
            reason = CSM_CALL_REASON_EVT_XFER_DONE;
            break;
        case ISI_EVENT_CALL_MODIFY:
            reason = CSM_CALL_REASON_EVT_MODIFY;
            break;
        case ISI_EVENT_CALL_BEING_FORWARDED:
            reason = CSM_CALL_REASON_EVT_BEING_FORWARDED;
            break;
        case ISI_EVENT_CALL_HANDOFF_SUCCESS:
            reason = CSM_CALL_REASON_EVT_SRVCC_SUCCESS;
            break;
        case ISI_EVENT_CALL_HANDOFF_FAILED:
            reason = CSM_CALL_REASON_EVT_SRVCC_FAILURE;
            break;
        case ISI_EVENT_CALL_MODIFY_COMPLETED:
            reason = CSM_CALL_REASON_EVT_MODIFY_COMPLETED;
            break;
        case ISI_EVENT_CALL_MODIFY_FAILED:
            reason = CSM_CALL_REASON_EVT_MODIFY_FAILED;
            break;
        case ISI_EVENT_CALL_XFER_PROGRESS:
        case ISI_EVENT_CALL_XFER_REQUEST:
        case ISI_EVENT_CALL_VIDEO_REQUEST_KEY:
            reason = CSM_CALL_REASON_EVT_VIDEO_REQUEST_KEY;
            break;
        case ISI_EVENT_CALL_EARLY_MEDIA:
            reason = CSM_CALL_REASON_EVT_EARLY_MEDIA;
            break;
        default:
            return;
    };
    call_ptr->reason    = reason;
    call_ptr->id        = isiCallId;
    call_ptr->serviceId = isiServiceId;
    call_ptr->type      = CSM_CALL_EVENT_TYPE_ISI;
    OSAL_strncpy(call_ptr->reasonDesc, desc_ptr,
            sizeof(call_ptr->reasonDesc));
    /* Notify CSM Account package */
    CSM_isiSendEvent(isiMngr_ptr, event_ptr);
}

/*
 *  ======== _CSM_isiCallPresenceTypeEventHandler() ========
 *
 *  Private handler for processing ISI "presence" type events.
 *
 *  RETURN:
 *      None
 */
void _CSM_isiCallPresenceTypeEventHandler(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       isiServiceId,
    ISI_Id       isiPresId,
    ISI_Event    event,
    const char  *desc_ptr)
{
    CSM_PrivateInputEvt     *event_ptr = &isiMngr_ptr->csmInputEvent;
    CSM_CallReason           reason;
    CSM_InputIsiCall        *call_ptr;

    call_ptr = &event_ptr->evt.call;

    CSM_dbgPrintf("\n");

    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_CALL;
    switch (event) {
        case ISI_EVENT_PRES_RECEIVED:
        case ISI_EVENT_GROUP_CHAT_PRES_RECEIVED:
            reason = (CSM_CallReason)CSM_CALL_REASON_EVT_PARTICIPANT_INFO;
            break;
        default:
            return;
    };
    call_ptr->reason    = (CSM_IsiCallReason)reason;
    call_ptr->id        = isiPresId;
    call_ptr->serviceId = isiServiceId;
    call_ptr->type      = CSM_CALL_EVENT_TYPE_ISI;
    OSAL_strncpy(call_ptr->reasonDesc, desc_ptr, CSM_EVENT_STRING_SZ);
    /* Notify Account package */
    CSM_isiSendEvent(isiMngr_ptr, event_ptr);
}

/*
 *  ======== _CSM_isiAcceptCall() ========
 *
 *  Private helper method for accepting an ISI call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiAcceptCall(
    ISI_Id      callId,
    const char *proto_ptr)
{
    CSM_dbgPrintf("%s: ISI AcceptCall callId:%d for %s\n", IR92_DEBUG_TAG, 
            callId, proto_ptr);
    return ISI_acceptCall(callId);
}

/*
 *  ======== _CSM_isiAcknowledgeCall() ========
 *
 *  Private helper method for acknowledging an ISI call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiAcknowledgeCall(
    ISI_Id      callId,
    const char *proto_ptr)
{
    CSM_dbgPrintf("%s: ISI AcknowledgeCall callId:%d ofr %s\n",
            IR92_DEBUG_TAG, callId, proto_ptr);
    return ISI_acknowledgeCall(callId);
}

/*
 *  ======== _CSM_isiGetCallHeader() ========
 *
 *  Private helper method for getting call header info
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiGetCallHeader(
    ISI_Id  callId,
    char   *subject_ptr,
    char   *from_ptr)
{
    ISI_Return ret;
    ret = ISI_getCallHeader(callId, subject_ptr, from_ptr);
    CSM_dbgPrintf("ISI GetCallHeader %d from:%s subject:%s\n", callId, from_ptr, 
            subject_ptr);
    return ret;
}

/*
 *  ======== _CSM_isiCheckSupsrvHeader() ========
 *
 *  Private helper method for check if supsrv header exist.
 *
 *  RETURN:
 *      OSAL_TRUE
 *      OSAL_FALSE
 */
int _CSM_isiCheckSupsrvHeader(
    ISI_Id              callId,
    ISI_SupsrvHfExist   target)
{
    ISI_SupsrvHfExist supsrvHfExist;

    supsrvHfExist = ISI_SUPSRV_HFEXIST_NONE;

    if (ISI_RETURN_OK == ISI_getSupsrvHeader(callId, (int *)&supsrvHfExist)) {
        if (0 != (supsrvHfExist & target)) {
            return (OSAL_TRUE);
        }
    }
    return (OSAL_FALSE);
}

/*
 *  ======== _CSM_isiGetSupsrvHistoryInfo() ========
 *
 *  Private helper method for getting supsrv history info
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiGetSupsrvHistoryInfo(
    ISI_Id  callId,
    char   *historyInfo_ptr)
{
    return (ISI_getSupsrvHistoryInfo(callId, historyInfo_ptr));
}

/*
 *  ======== _CSM_isiGetCallResourceStatus() ========
 *
 *  Private helper method for getting call resource status
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiGetCallResourceStatus(
    ISI_Id                  callId,
    ISI_ResourceStatus     *rsrcStatus_ptr,
    CSM_RadioResourceMedia *media_ptr)
{
    ISI_Return ret;
    
    ret = ISI_getCallResourceStatus(callId, rsrcStatus_ptr,
            &media_ptr->audioRtpPort,
            &media_ptr->videoRtpPort);
    
    CSM_dbgPrintf("ISI_getCallResourceStatus %d rsrcStatus:%d\n",
            callId, *rsrcStatus_ptr);
    return ret;
}

/*
 *  ======== _CSM_isiSetCallResourceStatus() ========
 *
 *  Private helper method for setting call resource status.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiSetCallResourceStatus(
    ISI_Id              callId,
    ISI_ResourceStatus  rsrcStatus)
{
    ISI_Return ret;
    ret = ISI_setCallResourceStatus(callId, rsrcStatus);
    CSM_dbgPrintf("ISI_setCallResourceStatus %d rsrcStatus:%d\n",
            callId, rsrcStatus);
    return ret;
}

/*
 *  ======== _CSM_isiGetCallSrvccStatus() ========
 *
 *  Private helper method for getting call SRVCC status
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiGetCallSrvccStatus(
    ISI_Id              callId,
    ISI_SrvccStatus    *srvccStatus_ptr)
{
    ISI_Return ret;
    ret = ISI_getCallSrvccStatus(callId, srvccStatus_ptr);
    CSM_dbgPrintf("ISI_getCallSrvccStatus %d srvccStatus:%d\n",
            callId, *srvccStatus_ptr);
    return ret;
}

/*
 *  ======== _CSM_isiHoldCall() ========
 *
 *  Private helper method for holding a voice call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiHoldCall(
    ISI_Id      callId,
    const char *proto_ptr)
{
    CSM_dbgPrintf("%s: ISI HoldCall callId:%d for %s\n", IR92_DEBUG_TAG,
            callId, proto_ptr);
    return ISI_holdCall(callId);
}

/* ======== _CSM_constructSessionAttributesXml() ========
 * Construct a XML String that describes the state of the given media.
 *
 * @param media the media object using which the XML string will be constructed. 
 * @return the XML string that describes the state of the Media.
 * 
 * mediaAttr : A null terminated string in xml format. This string
 *          represents the changes to be done at the media level to the call
 *          session. Please refer to _isi_xml.h for more details on various
 *          tags and attributes used.
 * e.g -
 * <media>
 *     <audio enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         useRtpAVPF="false"
 *         maxBandwidth="0"/>
 *     <video enabled="true"
 *         direction="sendrecv"
 *         secure="false"
 *         useRtpAVPF="true"
 *         maxBandwidth="0"/>
 * </media>
 */
void CSM_constructSessionAttributesXml(
    char *mediaAttr,
    int   callType,
    int   length)
{
    OSAL_snprintf(mediaAttr, length, MEDIA_ATTR_FORMAT_STR,
            (ISI_SESSION_TYPE_AUDIO & callType) ? 
            MEDIA_ATTR_TRUE : MEDIA_ATTR_FALSE,    /* audio enabled */
            MEDIA_ATTR_SENDRECV,                   /* direction */
            (ISI_SESSION_TYPE_SECURITY_AUDIO & callType) ?
            MEDIA_ATTR_TRUE : MEDIA_ATTR_FALSE,    /* secure */
            (ISI_SESSION_TYPE_EMERGENCY & callType) ?
            MEDIA_ATTR_TRUE : MEDIA_ATTR_FALSE,    /* emergency */
            MEDIA_ATTR_FALSE,                      /* useRtpAVPF */
            0,                                     /* maxBandwidth */
            (ISI_SESSION_TYPE_VIDEO & callType) ?
            MEDIA_ATTR_TRUE : MEDIA_ATTR_FALSE,    /* video enabled */
            MEDIA_ATTR_SENDRECV,                   /* direction */
            (ISI_SESSION_TYPE_SECURITY_VIDEO & callType) ?
            MEDIA_ATTR_TRUE : MEDIA_ATTR_FALSE,    /* secure */
            MEDIA_ATTR_FALSE,                      /* useRtpAVPF */
            1024);                                 /* maxBandwidth */
}

/*
 *  ======== _CSM_isiInitiateCall() ========
 *
 *  Private helper method for initiating an outbound voice call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiInitiateCall(
    ISI_Id     *callId_ptr,
    ISI_Id      serviceId,
    const char *proto_ptr,
    char       *toUri_ptr,
    char       *subject_ptr,
    int         callType,
    int         cidType)
{
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];
    
    CSM_dbgPrintf("%s: ISI InitiateCall to:%s for %s\n",
            IR92_DEBUG_TAG, toUri_ptr, proto_ptr);
    /* construct media attribute */
    CSM_constructSessionAttributesXml(mediaAttr, callType,
            ISI_MEDIA_ATTR_STRING_SZ);
    return (ISI_initiateCall(callId_ptr, serviceId, toUri_ptr, subject_ptr, 
            (ISI_INP ISI_SessionCidType)cidType, mediaAttr));
}

/*
 *  ======== _CSM_isiInitiateConfCall() ========
 *
 *  Private helper method for initiating an outbound voice call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiInitiateConfCall(
    ISI_Id     *callId_ptr,
    ISI_Id      serviceId,
    const char *proto_ptr,
    char       *toUri_ptr,
    char       *subject_ptr,
    int         callType,
    int         cidType,
    char       *rsrcList_ptr)
{
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];
    
    CSM_dbgPrintf("%s: ISI InitiateCall to:%s for %s\n",
            IR92_DEBUG_TAG, toUri_ptr, proto_ptr);
    /* construct media attribute */
    CSM_constructSessionAttributesXml(mediaAttr, callType,
            ISI_MEDIA_ATTR_STRING_SZ);
    return (ISI_initiateConfCall(callId_ptr, serviceId, toUri_ptr, subject_ptr, 
            cidType, mediaAttr, rsrcList_ptr));
}

/*
 *  ======== _CSM_isiRejectCall() ========
 *
 *  Private helper method for rejecting an incoming call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiRejectCall(
    ISI_Id      callId,
    const char *proto_ptr,
    char       *reasonDesc)
{
    CSM_dbgPrintf("%s %s: ISI RejectCall callId:%d for %s\n",
            IR92_DEBUG_TAG,  __FUNCTION__, callId, proto_ptr);
    return (ISI_rejectCall(callId, reasonDesc));
}

/*
 *  ======== _CSM_isiHoldCall() ========
 *
 *  Private helper method for resuming a held voice call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiResumeCall(
    ISI_Id      callId,
    const char *proto_ptr)
{
    CSM_dbgPrintf("%s %s: ISI ResumeCall callId:%d for %s\n",
            IR92_DEBUG_TAG,  __FUNCTION__, callId, proto_ptr);
    return (ISI_resumeCall(callId));
}

/*
 *  ======== _CSM_isiHoldCall() ========
 *
 *  Private helper method for sending a DTMF digit to the remote party
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiSendDigit(
    ISI_Id      callId,
    const char *proto_ptr,
    char        digit,
    vint        duration)
{
    ISI_Id     evtId;
    CSM_dbgPrintf("%s: ISI Send Digit [%c] Duration[%d] callId:%d for %s\n",
            IR92_DEBUG_TAG, digit, duration, callId, proto_ptr);
    return (ISI_sendTelEventToRemote(&evtId, callId, ISI_TEL_EVENT_DTMF,
            digit, duration));

}

/*
 *  ======== _CSM_isiTerminateCall() ========
 *
 *  Private helper method for terminating an active voice call.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiTerminateCall(
    ISI_Id      callId,
    const char *proto_ptr,
    char       *reasonDesc)
{
    CSM_dbgPrintf("ISI TerminateCall callId:%d for %s\n", callId, proto_ptr);
    return (ISI_terminateCall(callId, reasonDesc));
}

/*
 * ======== _CSM_isiLocalTerminateCall ========
 *
 * Private helper method for terminating an active voice call
 * but doesn't send any message to network
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiLocalTerminateCall(
    CSM_CallObject *call_ptr)
{
    CSM_IsiService *service_ptr;
    OSAL_NetAddress addr;
    char            ipAddress[OSAL_NET_IPV6_STR_MAX];

    /* Get the service object for this service ID */
    service_ptr = CSM_isiGetServiceViaProtocol(call_ptr->callFsm.isiMngr_ptr,
            CSM_ISI_PROTOCOL_MODEM_IMS, 0);
    ISI_serviceSetInterface(service_ptr->serviceId, CSM_ISI_NETWORK_INFC_NAME_4G,
            CSM_ISI_IPV4_ZERO_STR);

    /* Convert to network byte order first. */
    OSAL_netAddrHton(&addr,
            &((CSM_ServiceMngr *) service_ptr->fsm.serviceMngr_ptr)->regIpAddress);
    /* Conver IP address to string */
    OSAL_netAddressToString((int8 *)ipAddress, &addr);

    ISI_serviceSetInterface(service_ptr->serviceId, CSM_ISI_NETWORK_INFC_NAME_4G,
            ipAddress);

    return 0;
}

/*
 *  ======== _CSM_isiGetCallState() ========
 *
 *  Private helper method for holding a voice call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiGetCallState(
    ISI_Id         callId,
    ISI_CallState *callState)
{
    CSM_dbgPrintf("%s: ISI Get Call State callId:%d\n", IR92_DEBUG_TAG, callId);
    return (ISI_getCallState(callId, callState));
}

/*
 *  ======== _CSM_isiGetCallSessionType() ========
 *
 *  Private helper method to get call session type
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiGetCallSessionType(
    ISI_Id           callId,
    uint16          *sessionType)
{
    CSM_dbgPrintf("%s: ISI Get Session Type callId:%d\n", IR92_DEBUG_TAG, callId);
    return (ISI_getCallSessionType(callId, sessionType));
}

/*
 *  ======== _CSM_isiConferenceStart() ========
 *
 *  Private helper method for starting a conference call.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiConferenceStart(
    ISI_Id *confId_ptr,
    ISI_Id  callIdOne,
    ISI_Id  callIdTwo)
{
    CSM_dbgPrintf("ISI ConferenceCall callIdOne:%d callIdTwo:%d\n",
            callIdOne, callIdTwo);
    return (ISI_startConfCall(confId_ptr, callIdOne, callIdTwo));
}

/*
 *  ======== _CSM_isiConferenceAdd() ========
 *
 *  Private helper method for adding a party to a conference
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiConferenceAdd(
    ISI_Id confId,
    ISI_Id callId)
{
    CSM_dbgPrintf("ISI ConferenceAdd confId:%d callId:%d\n", confId, callId);
    return (ISI_addCallToConf(confId, callId));
}

/*
 *  ======== _CSM_isiConferenceRemove() ========
 *
 *  Private helper method for removing a party from a conference call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiConferenceRemove(
    ISI_Id confId,
    ISI_Id callId)
{
    CSM_dbgPrintf("ISI ConferenceRemove confId:%d callId:%d\n", confId, callId);
    return (ISI_removeCallFromConf(confId, callId));
}

/*
 *  ======== _CSM_isiConsultativeTransfer() ========
 *
 *  Private helper method for removing a party from a conference call
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiConsultativeTransfer(
    ISI_Id confId,
    ISI_Id callId)
{
    CSM_dbgPrintf("ISI Consultative Transfer confId:%d callId:%d\n",
            confId, callId);
    return (ISI_consultativeTransferCall(confId, callId));
}

/*
 *  ======== _CSM_isiConfAddParties() ========
 *  Private helper method for doing refer to conference factory with participant(s)
 *
 *  rsrcList_ptr: NULL for single party in to_ptr only
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiConfAddParties(
    ISI_Id confId,
    char *to_ptr,
    char *rsrcList_ptr)
{
    CSM_dbgPrintf("ISI conference transfer confId:%d to_ptr:%s\n",
            confId, to_ptr);
    if (NULL != rsrcList_ptr) {
        CSM_dbgPrintf("_CSM_isiConfAddParties rsrcList:%s\n", rsrcList_ptr);
    }
    return (ISI_confTransferCall(confId, to_ptr, rsrcList_ptr));
}

/*
 *  ======== _CSM_isiConsultativeRemove() ========
 *
 *  Private helper method for removing a party from a PS conference call 
 *  by sending a REFER with BYE.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiConsultativeRemove(
    ISI_Id confId,
    char  *removeAddr_ptr)
{
    CSM_dbgPrintf("ISI Consultative Remove confId:%d removeAddr:%s\n",
            confId, removeAddr_ptr);
    return (ISI_conferenceKickCall(confId, removeAddr_ptr));
}

/*
 *  ======== _CSM_isiReadPresence() ========
 *
 *  Private helper method for removing a reading a presense document
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiReadPresence(
    ISI_Id  presId,
    ISI_Id *callId_ptr,
    char   *from_ptr,
    char   *presence_ptr,
    int     len)
{
    CSM_dbgPrintf("ISI Read Presence presId:%d\n", presId);
    return (ISI_readPresence(presId, callId_ptr, from_ptr, presence_ptr, len));
}

/*
 *  ======== _CSM_isiCallModify() ========
 *
 *  Private helper method to do the call re-invite.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiCallModify(
    ISI_Id      callId,
    char       *reason_ptr)
{
    CSM_dbgPrintf("CSM_isiCallModify callId:%d\n", callId);
    return (ISI_modifyCall(callId, reason_ptr));
}

/*
 *  ======== _CSM_isiCallRemoveAudio() ========
 *
 *  Private helper method to remove audio from session and modify.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiCallRemoveAudio(
    ISI_Id      callId)
{
    ISI_Return          ret;
    uint16              type;
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];

    CSM_dbgPrintf("_CSM_isiAudioRemoveCallModify callId:%d\n", callId);
    ret = ISI_getCallSessionType(callId, &type);
    if (ISI_RETURN_OK != ret) {
        return ret;
    }
    type = (ISI_SessionType)(type & 
            ~(ISI_SESSION_TYPE_AUDIO | ISI_SESSION_TYPE_SECURITY_AUDIO));
    CSM_constructSessionAttributesXml(mediaAttr, type, ISI_MEDIA_ATTR_STRING_SZ);
    ret = ISI_updateCallSession(callId, mediaAttr);
    if (ISI_RETURN_OK != ret) {
        return ret;
    }
    return (ISI_modifyCall(callId, NULL));
}

/*
 *  ======== _CSM_isiAcceptCallModify() ========
 *
 *  Private helper method for accepting a call modify event.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiAcceptCallModify(
    ISI_Id      callId,
    const char *proto_ptr)
{
    CSM_dbgPrintf("ISI AcceptCallModify callId:%d for %s\n", callId, proto_ptr);
    return (ISI_acceptCallModify(callId));
}

/*
 *  ======== _CSM_isiRejectCallModify() ========
 *
 *  Private helper method for rejecting a call modify event.
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiRejectCallModify(
    ISI_Id      callId,
    char       *reason_ptr,
    const char *proto_ptr)
{
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];

    CSM_dbgPrintf("ISI RejectCallModify callId:%d for %s\n", callId, proto_ptr);
    CSM_constructSessionAttributesXml(mediaAttr, ISI_SESSION_TYPE_AUDIO,
            ISI_MEDIA_ATTR_STRING_SZ);
    ISI_updateCallSession(callId, mediaAttr);
    return (ISI_rejectCallModify(callId, reason_ptr));
}

/*
 *  ======== _CSM_isiSetCallDirection() ========
 *
 *  Private helper method for setting call direction
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiSetCallDirection(
    ISI_Id               callId,
    ISI_SessionDirection dir, 
    const char          *proto_ptr)
{
    CSM_dbgPrintf("ISI SetCallDirection callId:%d for %s\n",
            callId, proto_ptr);
    return (ISI_setCallSessionDirection(callId, ISI_SESSION_TYPE_AUDIO, dir));
}

/*
 *  ======== _CSM_isiGetCallDirection() ========
 *
 *  Private helper method to get call direction
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiGetCallDirection(
    ISI_Id                callId,
    ISI_SessionType       sessionType,
    ISI_SessionDirection *dir_ptr)
{
    return (ISI_getCallSessionDirection(callId, sessionType, dir_ptr));
}

/*
 *  ======== _CSM_isiSetConfCallId() ========
 *
 *  Private helper method for setting conference call id
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiSetConfCallId(
    ISI_Id callId)
{
    CSM_dbgPrintf("ISI Set Conference Call callId:%d\n", callId);
    return (ISI_serverSetConfCallId(callId));
}

/*
 *  ======== _CSM_isiModifySessionType() ========
 *
 *  Private helper method to modify foreground session type
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiModifySessionType(
    ISI_Id               callId,
    CSM_CallSessionType  callSessionType)
{
    ISI_SessionType         sessionType = ISI_SESSION_TYPE_NONE;
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];

    CSM_dbgPrintf("_CSM_isiModifySessionType callId:%d for %d\n", callId,
            callSessionType);

    /* Convert CSM session type to ISI session type. */
    if (callSessionType & CSM_CALL_SESSION_TYPE_AUDIO) {
        sessionType |= ISI_SESSION_TYPE_AUDIO;
        ISI_setCallSessionDirection(callId, ISI_SESSION_TYPE_AUDIO, 
                ISI_SESSION_DIR_SEND_RECV); 
    }
    if (callSessionType & CSM_CALL_SESSION_TYPE_VIDEO) {
        sessionType |= ISI_SESSION_TYPE_VIDEO;
        ISI_setCallSessionDirection(callId, ISI_SESSION_TYPE_VIDEO, 
                    ISI_SESSION_DIR_SEND_RECV); 
    }
    CSM_constructSessionAttributesXml(mediaAttr, sessionType,
            ISI_MEDIA_ATTR_STRING_SZ);
    ISI_updateCallSession(callId, mediaAttr);

    return (ISI_modifyCall(callId, NULL));
}

/*
 *  ======== _CSM_isiCancelCallModify() ========
 *
 *  Private helper method to modify foreground session type
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiCancelCallModify(
    ISI_Id               callId,
    CSM_CallSessionType  callSessionType)
{
    ISI_SessionType         sessionType = ISI_SESSION_TYPE_NONE;
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];

    CSM_dbgPrintf("_CSM_isiCancelCallModify callId:%d for %d\n", callId,
            callSessionType);

    /* Convert CSM session type to ISI session type. */
    if (callSessionType & CSM_CALL_SESSION_TYPE_AUDIO) {
        sessionType |= ISI_SESSION_TYPE_AUDIO;
        ISI_setCallSessionDirection(callId, ISI_SESSION_TYPE_AUDIO, 
                ISI_SESSION_DIR_SEND_RECV); 
    }
    if (callSessionType & CSM_CALL_SESSION_TYPE_VIDEO) {
        sessionType |= ISI_SESSION_TYPE_VIDEO;
        ISI_setCallSessionDirection(callId, ISI_SESSION_TYPE_VIDEO, 
                    ISI_SESSION_DIR_SEND_RECV); 
    }
    CSM_constructSessionAttributesXml(mediaAttr, sessionType,
            ISI_MEDIA_ATTR_STRING_SZ);
    ISI_updateCallSession(callId, mediaAttr);
 
    return (ISI_cancelModifyCall(callId));
}

/*
 *  ======== _CSM_isiGetSessionType() ========
 *
 *  Private helper method to get session type
 *
 *  RETURN:
 *      ISI_Return
 */
CSM_CallSessionType _CSM_isiGetSessionType(
    ISI_Id callId)
{
    CSM_CallSessionType callSessionType = CSM_CALL_SESSION_TYPE_NONE;
    uint16  sessionType;

    ISI_getCallSessionType(callId, &sessionType);
    CSM_dbgPrintf("callId:%d, sessionType=0x%X\n",
            callId, sessionType);
    /* Convert ISI session type to CSM session type. */
    if (sessionType & ISI_SESSION_TYPE_AUDIO) {
        callSessionType |= CSM_CALL_SESSION_TYPE_AUDIO;
    }
    if (sessionType & ISI_SESSION_TYPE_VIDEO) {
        callSessionType |= CSM_CALL_SESSION_TYPE_VIDEO;
    }

    return (callSessionType);
}

/*
 *  ======== _CSM_isiSetSessionType() ========
 *
 *  Private helper method to modify foreground session type
 *
 *  RETURN:
 *      ISI_Return
 */
int _CSM_isiSetSessionType(
    ISI_Id               callId,
    CSM_CallSessionType  callSessionType)
{
    ISI_SessionType  sessionType = ISI_SESSION_TYPE_NONE;
    char mediaAttr[ISI_MEDIA_ATTR_STRING_SZ + 1];

    CSM_dbgPrintf("callId:%d, sessionType:%d\n",
            callId, callSessionType);

    /* Convert CSM session type to ISI session type. */
    if (callSessionType & CSM_CALL_SESSION_TYPE_AUDIO) {
        sessionType |= ISI_SESSION_TYPE_AUDIO;
    }
    if (callSessionType & CSM_CALL_SESSION_TYPE_VIDEO) {
        sessionType |= ISI_SESSION_TYPE_VIDEO;
    }
    CSM_constructSessionAttributesXml(mediaAttr, sessionType,
            ISI_MEDIA_ATTR_STRING_SZ);
    return (ISI_updateCallSession(callId, mediaAttr));
}

/*
 *  ======== _CSM_isiMediaCntrlAecByPass() ========
 *
 *  Private helper method to modify AEC enable or disable
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiMediaCntrlAecByPass(
    int     on)
{
    CSM_dbgPrintf("Media control AEC ByPass:%d\n", on);

    return (ISI_mediaControl(ISI_MEDIA_CNTRL_AEC_BYPASS, on));
}

/*
 *  ======== _CSM_isiMediaCntrlGain() ========
 *
 *  Private helper method to change software gain
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiMediaCntrlGain(
    int     txGain,
    int     rxGain)
{
    int gain = 0;

    CSM_dbgPrintf("Media control change software gain tx:%d, rx:%d\n",
            txGain, rxGain);

    gain = txGain << 16;
    gain |= (rxGain & 0x0000FFFF);
    return (ISI_mediaControl(ISI_MEDIA_CNTRL_GAIN_CTRL, gain));
}

/*
 *  ======== _CSM_isiMediaCntrlGain() ========
 *
 *  Private helper method to change software gain
 *
 *  RETURN:
 *      ISI_Return
 */
ISI_Return _CSM_isiMediaCntrlCnGain(
    int     txGain)
{
    int gain = 0;

    CSM_dbgPrintf("Media control change software cn gain tx:%d\n", txGain);

    gain = txGain;
    return (ISI_mediaControl(ISI_MEDIA_CNTRL_CN_GAIN_CTRL, gain));
}


