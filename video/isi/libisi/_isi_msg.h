/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28791 $ $Date: 2014-09-11 15:28:46 +0800 (Thu, 11 Sep 2014) $
 *
 */

#ifndef __ISI_MSG_H__
#define __ISI_MSG_H__

ISIP_Message *ISIM_alloc(void);

void ISIM_free(ISIP_Message *msg_ptr);

ISIP_Message* ISIM_activation(
    ISID_ServiceId *service_ptr, 
    vint            activate);

ISIP_Message* ISIM_service(
    ISID_ServiceId *service_ptr, 
    vint            create);
 
ISIP_Message* ISIM_server(
    ISID_ServiceId *service_ptr, 
    ISI_ServerType  server);

ISIP_Message* ISIM_port(
    ISID_ServiceId *service_ptr,
    ISI_PortType    portType);

ISIP_Message* ISIM_ipsec(
    ISID_ServiceId *service_ptr);

ISIP_Message* ISIM_uri(
    ISID_ServiceId *service_ptr);

ISIP_Message* ISIM_credentials(
    ISID_ServiceId *service_ptr, 
    ISID_Account   *acct_ptr);

ISIP_Message* ISIM_coders(
    ISID_ServiceId *service_ptr);

ISIP_Message* ISIM_block(
    ISID_ServiceId *service_ptr,
    char           *userName_ptr,
    vint            block);

void ISIM_forward(
    ISID_EvtId     *evt_ptr,
    ISI_FwdCond     condition,
    int             enable,
    char           *to_ptr,
    int             timeout);
    
void ISIM_sendUSSD(
    ISID_EvtId     *evt_ptr,
    char           *ussd);

void ISIM_getServiceAttribute(
    ISID_EvtId          *evt_ptr,
    ISI_SeviceAttribute  cmd,
    char                *arg1,
    char                *arg2);

ISIP_Message* ISIM_setAkaAuthResp(
    ISID_ServiceId *service_ptr,
    int             result,
    char           *resp_ptr,
    int             resLength,
    char           *auts_ptr,
    char           *ck_ptr,
    char           *ik_ptr);

ISIP_Message* ISIM_bsid(
    ISID_ServiceId *service_ptr);

ISIP_Message* ISIM_imei(
    ISID_ServiceId *service_ptr);

ISIP_Message* ISIM_privateCid(
    ISID_ServiceId *service_ptr,
    vint            makePrivate);

void ISIM_call(
    ISIP_Message   *msg_ptr,
    ISIP_CallReason reason,
    ISIP_Status     status);

ISI_Return ISIM_initiateCall(
    ISID_ServiceId   *service_ptr, 
    ISID_CallId      *call_ptr,
    ISIP_Message    **msg_ptr);

ISIP_Message* ISIM_updateCall(
    ISID_CallId     *call_ptr, 
    ISIP_CallReason  reason,
    ISIP_Status      status,
    char            *to_ptr,
    char            *subject_ptr);

void ISIM_sendText(
    ISIP_TextReason reason,
    ISID_TextId    *text_ptr,
    ISI_Id          serviceId,
    ISI_Id          chatId);

void ISIM_telEvt(
    ISID_EvtId  *evt_ptr,
    ISI_TelEvent telEvent,
    const char  *to_ptr,
    int          arg0,
    int          arg1);

void ISIM_telEvtString(
    ISID_EvtId  *evt_ptr,
    ISI_TelEvent telEvent,
    const char  *to_ptr,
    const char  *string_ptr,
    int          durationMs);

ISIP_Message *ISIM_conf(
    ISID_ConfId     *conf_ptr,
    ISIP_MediaReason reason);

ISIP_Message *ISIM_stream(
    ISID_CallId      *call_ptr,
    ISIP_MediaReason  reason);

ISIP_Message *ISIM_tone(
    ISID_CallId      *call_ptr,
    ISIP_MediaReason  reason,
    ISI_AudioTone     toneType,
    vint              duration);

ISIP_Message *ISIM_ring(
    ISID_CallId      *call_ptr,
    ISIP_MediaReason  reason);

void ISIM_pres(
    ISID_PresId     *pres_ptr,
    ISIP_PresReason  reason,
    char            *to_ptr);

ISIP_Message *ISIM_system(
    vint               protocol,
    ISIP_SystemReason  reason,
    ISIP_Status        status);

void ISIM_chat(
    ISIP_Message        *msg_ptr,
    ISIP_ChatReason      reason,
    ISIP_Status          status);

ISIP_Message *ISIM_initiateChat(
    ISID_GChatId        *chat_ptr,
    ISIP_ChatReason      reason,
    char                *to_ptr,
    char                *password_ptr,
    ISI_SessionType      type);

ISIP_Message *ISIM_inviteChat(
    ISID_GChatId     *chat_ptr,
    char             *participant_ptr,
    char             *reason_ptr);

ISIP_Message *ISIM_acceptChat(
    ISID_GChatId     *chat_ptr,
    char             *password_ptr);

ISIP_Message *ISIM_rejectChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr);

ISIP_Message *ISIM_ackChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr);

ISIP_Message *ISIM_kickChat(
    ISID_GChatId     *chat_ptr,
    char             *participant_ptr,
    char             *reason_ptr);

ISIP_Message *ISIM_terminateChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr);

ISIP_Message *ISIM_destroyChat(
    ISID_GChatId     *chat_ptr,
    char             *reason_ptr);
        
ISIP_Message *ISIM_media(
    ISI_MediaControl mediaCmd,
    vint             arg);

ISIP_Message *ISIM_initiateOtherChat(
    ISID_GChatId        *chat_ptr,
    ISIP_ChatReason      reason,
    char                *to_ptr,
    char                *factory_ptr);

void ISIM_sendFile(
    ISID_FileId    *file_ptr,
    ISI_Id          serviceId);

void ISIM_acceptFileTransfer(
    ISID_FileId    *file_ptr);

void ISIM_beginSendingFileTransfer(
        ISID_FileId    *file_ptr);

void ISIM_rejectFileTransfer(
    ISID_FileId    *file_ptr,
    const char     *rejectReason_ptr);

void ISIM_cancelFileTransfer(
    ISID_FileId    *file_ptr,
    const char     *cancelReason_ptr);

void ISIM_shutdownFileTransfer(
    ISID_FileId    *file_ptr);


ISIP_Message* ISIM_filePath(
    ISID_ServiceId *service_ptr,
    char           *filePath_ptr,
    char           *filePrepend_ptr);

ISIP_Message* ISIM_diagAudioRecord(
    vint            protocol,
    char           *file_ptr);

ISIP_Message* ISIM_diagAudioPlay(
    vint            protocol,
    char           *file_ptr);

void ISIM_acknowledgeFileTransfer(
    ISID_FileId    *file_ptr);

ISIP_Message* ISIM_setCapabilties(
    ISID_ServiceId *service_ptr,
    char           *capabilities_ptr);
ISIP_Message* ISIM_emergency(
    ISID_ServiceId *service_ptr);

void ISIM_sendUssd(
    ISIP_UssdReason reason,
    ISID_UssdId    *ussd_ptr,
    ISI_Id          serviceId);

ISIP_Message* ISIM_instanceId(
    ISID_ServiceId *service_ptr);

ISIP_Message* ISIM_setProvisioningData(
    ISID_ServiceId *service_ptr,
    const char     *xmlDoc_ptr);

ISIP_Message* ISIM_net(
    ISID_ServiceId *service_ptr);

#endif
