/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */


#ifndef _CSM_ISI_CALL_H_
#define _CSM_ISI_CALL_H_

#include <osal.h>
#include "_csm_calls.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MEDIA_ATTR_TRUE      "true"
#define MEDIA_ATTR_FALSE     "false"
#define MEDIA_ATTR_SENDRECV  "sendrecv"

#define MEDIA_ATTR_FORMAT_STR "<media> \
<audio enabled=\"%s\"\
direction=\"%s\"\
secure=\"%s\"\
emergency=\"%s\"\
useRtpAVPF=\"%s\"\
maxBandwidth=\"%d\"/>\
<video enabled=\"%s\"\
direction=\"%s\"\
secure=\"%s\"\
useRtpAVPF=\"%s\"\
maxBandwidth=\"%d\"/>\
</media>"

/* 
 * CSM ISI Call private methods
 */
int _CSM_isiAcceptCall(
    ISI_Id      callId,
    const char *proto_ptr);

int _CSM_isiAcknowledgeCall(
    ISI_Id      callId,
    const char *proto_ptr);

int _CSM_isiGetCallHeader(
    ISI_Id  callid,
    char   *subject_ptr,
    char   *from_ptr);

int _CSM_isiCheckSupsrvHeader(
    ISI_Id              callId,
    ISI_SupsrvHfExist   target);

int _CSM_isiGetSupsrvHistoryInfo(
    ISI_Id  callId,
    char   *historyInfo_ptr);

int _CSM_isiHoldCall(
    ISI_Id      callId,
    const char *proto_ptr);

int _CSM_isiInitiateCall(
    ISI_Id     *callId_ptr,
    ISI_Id      serviceId,
    const char *proto_ptr,
    char       *toUri_ptr,
    char       *subject_ptr,
    int         callType,
    int         cidType);

int _CSM_isiRejectCall(
    ISI_Id      callId,
    const char *proto_ptr,
    char       *reasonDesc);

int _CSM_isiResumeCall(
    ISI_Id      callId,
    const char *proto_ptr);

int _CSM_isiSendDigit(
    ISI_Id      callId,
    const char *proto_ptr,
    char        digit,
    vint        duration);

int _CSM_isiLocalTerminateCall(
    CSM_CallObject *call_ptr);

int _CSM_isiTerminateCall(
    ISI_Id      callId,
    const char *proto_ptr,
    char       *reasonDesc);

int _CSM_isiGetCallState(
    ISI_Id         callId,
    ISI_CallState *callState);

int _CSM_isiGetCallSessionType(
    ISI_Id           callId,
    uint16          *sessionType);

int _CSM_isiConferenceStart(
    ISI_Id *confId_ptr,
    ISI_Id  callIdOne,
    ISI_Id  callIdTwo);

int _CSM_isiConferenceAdd(
    ISI_Id confId,
    ISI_Id callId);

int _CSM_isiConferenceRemove(
    ISI_Id confId,
    ISI_Id callId);

int _CSM_isiConsultativeTransfer(
    ISI_Id confId,
    ISI_Id callId);

int _CSM_isiConferenceRefer(
    ISI_Id confId,
    ISI_Id callId,
    char *rsrcList_ptr);

int _CSM_isiConsultativeRemove(
    ISI_Id confId,
    char  *removeAddr);

int _CSM_isiReadPresence(
    ISI_Id  presId,
    ISI_Id *callId_ptr,
    char   *from_ptr,
    char   *presence_ptr,
    int    len);

int _CSM_isiCallModify(
    ISI_Id      callId,
    char       *reason_ptr);

int _CSM_isiCallRemoveAudio(
    ISI_Id      callId);

int _CSM_isiAcceptCallModify(
    ISI_Id      callId,
    const char *proto_ptr);

int _CSM_isiAcceptCallModifyConditional(
    ISI_Id      callId,
    CSM_EventCall_Ptr event_ptr,
    const CFSM_Context_Ptr context_ptr);

int _CSM_isiRejectCallModify(
    ISI_Id      callId,
    char       *reason_ptr,
    const char *proto_ptr);

int _CSM_isiGetCallResourceStatus(
    ISI_Id                  callId,
    ISI_ResourceStatus     *rsrcStatus_ptr,
    CSM_RadioResourceMedia *media_ptr);

int _CSM_isiSetCallResourceStatus(
    ISI_Id              callId,
    ISI_ResourceStatus  rsrcStatus);

int _CSM_isiGetCallSrvccStatus(
    ISI_Id              callId,
    ISI_SrvccStatus    *srvccStatus_ptr);

ISI_Return _CSM_isiSetCallDirection(
    ISI_Id               callId,
    ISI_SessionDirection dir,
    const char          *proto_ptr);

ISI_Return _CSM_isiGetCallDirection(
    ISI_Id                callId,
    ISI_SessionType       sessionType,
    ISI_SessionDirection *dir_ptr);

ISI_Return _CSM_isiSetConfCallId (
    ISI_Id callId);

ISI_Return _CSM_isiCancelCallModify(
    ISI_Id               callId,
    CSM_CallSessionType         callSessionType);

int _CSM_isiModifySessionType(
    ISI_Id               callId,
    CSM_CallSessionType         callSessionType);

CSM_CallSessionType _CSM_isiGetSessionType(
    ISI_Id               callId);

int _CSM_isiSetSessionType(
    ISI_Id               callId,
    CSM_CallSessionType  callSessionType);

void CSM_constructSessionAttributesXml(
    char *mediaAttr,
    int   callType,
    int   length);

int _CSM_isiInitiateConfCall(
    ISI_Id     *callId_ptr,
    ISI_Id      serviceId,
    const char *proto_ptr,
    char       *toUri_ptr,
    char       *subject_ptr,
    int         callType,
    int         cidType,
    char       *rsrcList_ptr);

int _CSM_isiConfAddParties(
    ISI_Id confId,
    char *to_ptr,
    char *rsrcList_ptr);

ISI_Return _CSM_isiMediaCntrlAecByPass(
    int     on);

ISI_Return _CSM_isiMediaCntrlGain(
    int     txGain,
    int     rxGain);

ISI_Return _CSM_isiMediaCntrlCnGain(
    int     txGain);

#ifdef __cplusplus
}
#endif
#endif /* _CSM_ISI_CALL_H_ */
