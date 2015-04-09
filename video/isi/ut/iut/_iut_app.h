/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24732 $ $Date: 2014-02-21 17:55:42 +0800 (Fri, 21 Feb 2014) $
 */

#ifndef _IUT_APP_H_
#define _IUT_APP_H_

#define IUT_OK  (1)

#define IUT_ERR (0)

#define IUT_STR_SIZE (127)

#define IUT_ARG_SIZE (63)

#define IUT_MAX_LINES (6)

#define IUT_MAX_GUI_SIZE (4096)

#define IUT_MAX_NUM_ROSTERS (32)

#define IUT_MAX_IM_MSG_SIZE (255)

#define IUT_GSM_AT_SERVICE_TYPE (3)

#define IUT_TONE_DURATION_MS (2000)


typedef struct {
    char    contact[IUT_STR_SIZE + 1];
    char    jid[IUT_STR_SIZE + 1];
    char    group[IUT_STR_SIZE + 1];
    char    name[IUT_STR_SIZE + 1];
    char    statusDesc[IUT_STR_SIZE + 1];
    char    state[IUT_STR_SIZE + 1];
} IUT_Roster;

typedef struct {
    ISI_Id          serviceId;
    vint            isActive;
    vint            isDead;
    ISI_Id          callId[IUT_MAX_LINES];
    ISI_Id          textId;
    ISI_Id          fileId;
    ISI_Id          evtId;
    ISI_Id          confId;
    ISI_Id          presId;
    ISI_Id          proto;
    IUT_Roster      roster[IUT_MAX_NUM_ROSTERS];
    char            username[IUT_STR_SIZE + 1];
    char            password[IUT_STR_SIZE + 1];
    char            realm[IUT_STR_SIZE + 1];
    char            proxy[IUT_STR_SIZE + 1];
    char            stun[IUT_STR_SIZE + 1];
    char            relay[IUT_STR_SIZE + 1];
    char            xcap[IUT_STR_SIZE + 1];
    char            chat[IUT_STR_SIZE + 1];
    char            outProxy[IUT_STR_SIZE + 1];
    char            uri[IUT_STR_SIZE + 1];
    char            subject[IUT_STR_SIZE + 1];
    vint            cidPrivate;
    vint            toneDuration;
    ISI_Id          chatRoomId[IUT_MAX_LINES];
    ISI_Id          capabilityId[IUT_MAX_LINES];
    vint            isEmergency;
    char            imeiUri[IUT_STR_SIZE + 1];
    vint            audioPortNumber;
    vint            videoPortNumber;
    char            interfaceAddress[IUT_STR_SIZE + 1];
} IUT_HandSetObj;

typedef struct {
    vint            isDead;
    ISI_Id          proto;
} IUT_ServiceObj;

void IUT_appModuleInit(void);

IUT_HandSetObj *IUT_appServiceFind(
    ISI_Id serviceId);

void IUT_appServiceSet(
    ISI_Id service);

IUT_HandSetObj *IUT_appServiceGet(void);

ISI_Id IUT_appServiceNext(
    ISI_Id serviceId);

void IUT_appServicePrint(void);

void IUT_appPeerSet(
    vint peer);

vint IUT_appPeerGet(void);

vint IUT_appRosterAdd(
    IUT_HandSetObj *hs_ptr,
    char           *contact_ptr,
    char           *group_ptr);

vint IUT_appRosterRemove(
    IUT_HandSetObj *hs_ptr,
    char           *contact_ptr,
    char           *group_ptr);

void IUT_appProcessEvent(
    ISI_Id          serviceId,
    ISI_Id          id,
    ISI_IdType      idType,
    ISI_Event       event);

vint IUT_appServiceAlloc(
    IUT_HandSetObj *hs_ptr,
    vint            proto);

vint IUT_appServiceFree(
    IUT_HandSetObj *hs_ptr);

vint IUT_appInit(void);

vint IUT_appShutdown(void);

vint IUT_getEvent(
    IUT_HandSetObj *hs_ptr);

ISI_Id IUT_appCall(
    IUT_HandSetObj          *hs_ptr,
    vint                     peer,
    char                    *to_ptr,
    ISI_SessionType          type,
    ISI_SessionDirection     audioDirection,
    ISI_SessionDirection     videoDirection);

ISI_Id IUT_appFmcCall(
    IUT_HandSetObj          *hs_ptr,
    vint                     peer, 
    char                    *to_ptr,
    vint                     type,
    ISI_SessionDirection     audioDirection,
    ISI_SessionDirection     videoDirection);

vint IUT_appConfAdd(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    IUT_HandSetObj *hs2Add_ptr,
    vint            peer2Add);

vint IUT_appConfRemove(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    IUT_HandSetObj *hs2Rm_ptr,
    vint            peer2Rm);

vint IUT_appCallBlindTransfer(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr);

vint IUT_appCallAttendTransfer(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr);

vint IUT_appCallConsultativeTransfer(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            peerTarget);

vint IUT_appCallForward(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr);

vint IUT_appCallModifyVideo(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallModifyAudio(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallModify(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallTerminate(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallAck(
    IUT_HandSetObj *hs_ptr,
    vint           peer);

vint IUT_appCallAccept(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallReject(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallHold(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallResume(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appDigit(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char            digit,
    vint            isOob,
    vint            duration);

vint IUT_appFlashhook(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appFmcFeature(
    IUT_HandSetObj *hs_ptr,
    vint            peer, 
    char           *to_ptr,
    vint            feature,
    vint            isComplete);

vint IUT_appTone(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            tone,
    vint            duration);

vint IUT_appMessageSend(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    char           *msg_ptr,
    vint            deliveryReport,
    vint            displayReport);

vint IUT_appCapabilitiesSet(
    IUT_HandSetObj *hs_ptr,
    char           *xml_ptr);
    
vint IUT_appCapabilitiesSend(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    vint            peer,
    char           *capabilities_ptr);

vint IUT_appSubscribe(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr);

vint IUT_appUnsubscribe(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr);

vint IUT_appSubscribeAllow(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    vint            allow);

vint IUT_appPresenceSend(
    IUT_HandSetObj *hs_ptr,
    vint            state,
    char           *status_ptr);

void IUT_appSetCoders(
    IUT_HandSetObj *hs_ptr,
    vint            id,
    vint            pcmu,
    vint            ppcmu,
    vint            pcma,
    vint            ppcma,
    vint            g726,
    vint            pg726,
    vint            g729,
    vint            pg729,
    vint            iLBC,
    vint            piLBC,
    vint            silk,
    vint            psilk,
    vint            g722,
    vint            pg722,
    vint            dtmfr,
    vint            pdtmfr,
    vint            cn,
    vint            pcn,
    vint            h264,
    vint            ph264,
    vint            h263,
    vint            ph263,
    vint            amrnb,
    vint            pamrnb,
    vint            amrwb,
    vint            pamrwb);

void IUT_appSetFields(
    IUT_HandSetObj *hs_ptr,
    char const *proxy_ptr,
    char const *stun_ptr,
    char const *relay_ptr,
    char const *xcap_ptr,
    char const *chat_ptr,
    char const *outbound_ptr,
    char const *username_ptr,
    char const *password_ptr,
    char const *realm_ptr,
    char const *uri_ptr,
    char const *imei_ptr,
    vint        isEmergency,
    vint        audioPortNumber,
    vint        videoPortNumber,
    char const *interfaceAddress, 
    vint        cidPrivate);

void IUT_appSetActivation(
    IUT_HandSetObj *hs_ptr,
    vint            on);

void IUT_appBlockUser(
    IUT_HandSetObj *hs_ptr,
    char           *user_ptr,
    vint            blockUser);

void IUT_appForward(
    IUT_HandSetObj *hs_ptr,
    char           *target_ptr,
    vint            condition,
    vint            enable,
    vint            timeout);

vint IUT_appCallHandoff(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

void IUT_appForceHandoff(
    IUT_HandSetObj *hs_ptr);

int IUT_appCreateGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *roomName_ptr,
    char           *subject_ptr);

vint IUT_appCreateAdhocGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr,
    char           *conferenceUri_ptr,
    char           *subject_ptr);

vint IUT_appJoinGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *roomName_ptr,
    char           *password_ptr);

vint IUT_appDestroyGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *reason_ptr);

vint IUT_appInviteGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *participant_ptr,
    char           *reason_ptr);

vint IUT_appKickGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *participant_ptr,
    char           *reason_ptr);

vint IUT_appSendMessageChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *msg_ptr,
    uint8           requestDeliveryReports,
    uint8           requestDisplayReports);

vint IUT_appSendPresenceGroupChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            state,
    char           *status_ptr);

vint IUT_appSendFileChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *filePath_ptr,
    int             fileType);

vint IUT_appCallModifyAccept(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallModifyReject(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallAcceptAudioOnly(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appCallModifyDir(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    vint            type,
    vint            dir);

void IUT_appSetInterface(
    IUT_HandSetObj *hs_ptr,
    char           *name_ptr,
    char           *addr_ptr);

void IUT_appSetPort(
    IUT_HandSetObj *hs_ptr,
    int            port,
    int            poolSize,
    int            type);

vint IUT_appInitiateChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer,
    char           *to_ptr,
    char           *subject_ptr,
    char           *message_ptr,
    uint8           requestDeliveryReports,
    uint8           requestDisplayReports);

vint IUT_appAcceptChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appAcknowledgeChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appRejectChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appDisconnectChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appComposingMessageChat(
    IUT_HandSetObj *hs_ptr,
    vint            peer);

vint IUT_appAcknowledgeFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id);

vint IUT_appAcceptFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id);

vint IUT_appRejectFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id,
    const char     *rejectReason_ptr);

vint IUT_appCancelFileTransfer(
    IUT_HandSetObj *hs_ptr,
    ISI_Id          id,
    const char     *cancelReason_ptr);

vint IUT_appContentShareSendFile(
    IUT_HandSetObj *hs_ptr,
    char           *to_ptr,
    char           *subject_ptr,
    char           *filePath_ptr,
    int             fileType,
    int             fileAttribute,
    int             report);

vint IUT_appSetFeature(
    ISI_FeatureType features);

IUT_HandSetObj* IUT_appGetNextService(
    IUT_HandSetObj *hs_ptr);

vint IUT_appSetProvisioningData(
    IUT_HandSetObj *hs_ptr,
    char           *filename_ptr);

void IUT_appSetIpsec(
    IUT_HandSetObj *hs_ptr,
    int            port,
    int            poolSize,
    int            spi,
    int            spiPoolSize);

#endif
