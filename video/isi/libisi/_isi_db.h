/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
 *
 */
 
#ifndef __ISI_DB_H__
#define __ISI_DB_H__

/*
 * NOTE: if more than 32 protocols must be supported, bitmaps used in _isi_db.c
 * to contain info about the protocols must be increased
 */
#define ISID_MAX_NUM_PROTOCOLS (32)
#define ISID_MAX_NUM_SERVICES  (ISID_MAX_NUM_PROTOCOLS) 
#define ISID_MAX_NUM_ACCOUNTS  (3)
#define ISID_MAX_NUM_CALLS     (6)
#define ISID_MAX_NUM_GCHATS    (100)
#define ISID_MAX_NUM_MSGS      (ISID_MAX_NUM_SERVICES * 8)
#define ISID_MAX_NUM_FILES     (ISID_MAX_NUM_SERVICES * 8)
#define ISID_MAX_NUM_EVTS      (ISID_MAX_NUM_SERVICES * 8)
#define ISID_MAX_NUM_PRESENCE  (ISID_MAX_NUM_SERVICES * 8)
#define ISID_MAX_NUM_CONFS     (ISID_MAX_NUM_SERVICES * ISID_MAX_NUM_CALLS)
#define ISID_MAX_NUM_STREAMS   (ISID_MAX_NUM_SERVICES * ISID_MAX_NUM_CALLS)

#define ISID_MAX_ID            (0x00FFFFFF)

typedef enum {
    ISID_PROTOCOLS_ACTIVE,
    ISID_PROTOCOLS_FAILED,
    ISID_PROTOCOLS_REQUESTED,
} ISID_ProtocolMask;

/*
 * States in which a text message can be.
 */
typedef enum {
    ISID_TEXT_STATE_NONE = 0,
    ISID_TEXT_STATE_ACTIVE,
    ISID_TEXT_STATE_WAITING
} ISID_TextState;

typedef enum {
    ISID_FILE_STATE_NONE = 0,
    ISID_FILE_STATE_SEND,
    ISID_FILE_STATE_RECV
} ISID_FileState;

typedef enum {
    ISID_EVENT_STATE_NONE = 0,
    ISID_EVENT_STATE_ACTIVE,
} ISID_EvtState;

typedef enum {
    ISID_PRES_STATE_NONE = 0,
    ISID_PRES_STATE_ACTIVE,
    ISID_PRES_STATE_WAITING
} ISID_PresState;

typedef enum {
    ISID_USSD_STATE_NONE = 0,
    ISID_USSD_STATE_ACTIVE,
    ISID_USSD_STATE_WAITING,
} ISID_UssdState;

typedef struct sISID_Stream {
    uint8 inUse;
    uint8 bitmask;
} ISID_Stream;

typedef struct sISID_Entry {
    struct sISID_Entry *next_ptr;
    ISI_Id              desc;
    ISI_Id              desc2;
} ISID_Entry;

typedef struct {
    ISID_Entry *head_ptr;
    ISID_Entry *tail_ptr;
    vint        numEntries;
    vint        descCnt;
    ISI_SemId   mutex;
} ISID_List;

typedef struct sISID_Account {
     struct sISID_Account *next_ptr;
     char   szRealm[ISI_ADDRESS_STRING_SZ + 1];
     char   szUsername[ISI_ADDRESS_STRING_SZ + 1];
     char   szPassword[ISI_ADDRESS_STRING_SZ + 1];
} ISID_Account;

typedef struct sISID_RtpRmtInfc {
    char             szUsername[ISI_ADDRESS_STRING_SZ + 1];
    OSAL_NetAddress  addr;
    uint16           cntlPort; /* i.e. RTCP */
    /* Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32           videoAsBwKbps;
} ISID_RtpRmtInfc;

typedef struct sISID_RtpLclInfc {
    OSAL_NetAddress  addr;
    uint16           cntlPort; /* i.e. RTCP */
    /* Video RTP Session bandwidth in kbps - AS bandwidth parameter. */
    uint32           videoAsBwKbps;
} ISID_RtpLclInfc;

/* Base station id or cell id information */
typedef struct sISID_BsId {
    ISI_NetworkAccessType  type;
    char                   szBsId[ISI_BSID_STRING_SZ + 1];
} ISID_BsId;

typedef struct {
    /* this value must be first */
    ISID_Entry          e;
    /* service attributes */
    vint                protocol;
    char                szCountry[ISI_COUNTRY_STRING_SZ + 1];
    char                szUri[ISI_ADDRESS_STRING_SZ + 1];
    char                szProxy[ISI_ADDRESS_STRING_SZ + 1];
    char                szRegistrar[ISI_ADDRESS_STRING_SZ + 1];
    char                szStunServer[ISI_ADDRESS_STRING_SZ + 1];
    char                szOutboundProxy[ISI_LONG_ADDRESS_STRING_SZ + 1];
    char                szRelayServer[ISI_ADDRESS_STRING_SZ + 1];
    char                szStorageServer[ISI_ADDRESS_STRING_SZ + 1];
    char                szChatServer[ISI_ADDRESS_STRING_SZ + 1];
    char                szNickname[ISI_ADDRESS_STRING_SZ + 1];
    char                szInstanceId[ISI_INSTANCE_STRING_SZ + 1];
    ISID_BsId           bsId; /* Base station id or cell id */
    ISID_Account       *accounts_ptr;
    vint                numAccounts;
    OSAL_NetAddress     infcAddress;
    char                szInfcName[ISI_ADDRESS_STRING_SZ];
    /* attributes related to call features */
    vint                privateCid;
    ISIP_Coder          coders[ISI_CODER_NUM];
    vint                isActivated;
    vint                vccEnabled;
    /* AKA authentication challenge */
    char                rand[ISI_AKA_AUTH_RAND_STRING_SZ];
    char                autn[ISI_AKA_AUTH_AUTN_STRING_SZ];
    vint                portUc;
    vint                portUs;
    vint                portPc;
    vint                portPs;
    vint                spiUc;
    vint                spiUs;
    vint                spiPc;
    vint                spiPs;
    vint                ipsecStatus;
    /* IPSec information */
    vint                protectedPort;
    vint                protectedPortPoolSz;
    vint                spi;
    vint                spiPoolSz;
    /* Ports */
    vint                sipPort;
    vint                audioRtpPort;
    vint                audioPoolSize;
    vint                videoRtpPort;
    vint                videoPoolSize;
    vint                isEmergency;
    char                imeiUri[ISI_ADDRESS_STRING_SZ + 1];
    /* Feature set */
    vint                features;
} ISID_ServiceId;

typedef struct {
    /* this value must be first */
    ISID_Entry           e;
    ISID_ServiceId      *service_ptr;
    ISI_CallState        state;
    ISI_CallState        previousState;
    ISI_Id               transId; /* used to track the other call 
                                   * handling an incoming transfer 
                                   */
    char                 szSubject[ISI_SUBJECT_STRING_SZ + 1];
    char                 szRemoteUri[ISI_ADDRESS_STRING_SZ + 1];
    char                 szTargetUri[ISI_ADDRESS_STRING_SZ + 1];
    /* Session info */
    ISIP_Coder           coders[ISI_CODER_NUM];
    ISI_SessionCidType   cidType;
    uint16               type;
    ISI_SessionDirection videoDir;
    ISI_SessionDirection audioDir;
    /* Bit mask indicating what RTCP-FB msgs should be used for video. */
    uint32               videoRtcpFbMask;
    ISID_RtpLclInfc      rtpAudioLcl;
    ISID_RtpLclInfc      rtpVideoLcl;
    ISID_RtpRmtInfc      rtpAudioRmt;
    ISID_RtpRmtInfc      rtpVideoRmt;
    ISIP_SecurityKeys    audioKeys;
    ISIP_SecurityKeys    videoKeys;
    uint16               confMask;
    uint8                streamId;
    vint                 isRinging;
    vint                 ringTemplate;
    ISI_ResourceStatus   rsrcStatus;
    ISI_SrvccStatus      srvccStatus;
    int                  supsrvHfExist;
    char                 historyInfo[ISI_HISTORY_INFO_STRING_SZ+1];
} ISID_CallId;

/* the name of the parameter used for the Contribution-ID in the
 * conference URI string.  The conference URI may be of the form, e.g.-
 * "sip:chatRoom_1@domain.com;contribId=<szContributionId>" */
#define ISI_CONTRIBUTION_ID_PARAM_STR "contribId"

typedef struct {
    /* this value must be first */
    ISID_Entry           e;
    ISID_ServiceId      *service_ptr;
    ISI_ChatState        state;
    vint                 requiresPassword;
    char                 szRemoteAddress[ISI_ADDRESS_STRING_SZ + 1];
    char                 szParticipants[ISI_LONG_ADDRESS_STRING_SZ + 1];
    char                 szInvitor[ISI_ADDRESS_STRING_SZ + 1];
    char                 szLocalIdentity[ISI_ADDRESS_STRING_SZ + 1];
    char                 szSubject[ISI_SUBJECT_STRING_SZ + 1];
    char                 szContributionId[ISI_ADDRESS_STRING_SZ + 1];
    vint                 isConference;
    ISI_Id               firstMessageId;
} ISID_GChatId;

typedef struct {
    /* this value must be first */
    ISID_Entry           e;
    ISID_ServiceId      *service_ptr;
    ISI_Id               chatId;
    ISID_TextState       state;
    vint                 msgLen;
    vint                 msgOffset;
    ISIP_Message         isiMsg;
} ISID_TextId;

typedef struct {
    /* this value must be first */
    ISID_Entry           e;
    ISID_ServiceId      *service_ptr;
    ISID_FileState       state;
    ISIP_Message         isiMsg;
} ISID_FileId;

typedef struct {
    /* this value must be first */
    ISID_Entry           e;
    ISID_ServiceId      *service_ptr;
    ISI_Id               chatId;
    ISID_PresState       state;
    ISIP_PresReason      reason;
    vint                 presLen;
    vint                 presOffset;
    ISIP_Message         isiMsg;
} ISID_PresId;

typedef struct {
    /* this value must be first */
    ISID_Entry        e;
    ISID_ServiceId   *service_ptr;
    ISID_CallId      *call_ptr;
    ISID_EvtState     state;
    ISIP_Message      isiMsg;     
} ISID_EvtId;

typedef struct {
    /* this value must be first */
    ISID_Entry       e;
    uint16           type;
    vint             aConfMask[ISI_CONF_USERS_NUM];
    ISID_CallId     *aCall[ISI_CONF_USERS_NUM];
} ISID_ConfId;

typedef struct {
    char            *name_ptr;
    char            *description_ptr;
    ISI_SessionType  relates;
} ISID_CoderE;

typedef struct {
    /* this value must be first */
    ISID_Entry           e;
    ISID_ServiceId      *service_ptr;
    ISID_UssdState       state;
    ISIP_Message         isiMsg;
} ISID_UssdId;


/* General config functions */
ISI_Return ISID_init(void);

void ISID_destroy(void);

vint ISID_getState(void);

void ISID_setState(vint state);

uint32 ISID_setProtocol(
    ISID_ProtocolMask maskType,
    vint              protocol, 
    vint              enable);

uint32 ISID_getProtocol(
    ISID_ProtocolMask maskType);

ISI_Return ISID_checkProtocol(vint protocol);

void ISID_setCountry(char *country_ptr);

/* All db mutex control functions */
ISI_Return ISID_lockServices(void);

ISI_Return ISID_lockCalls(void); 

ISI_Return ISID_lockConfs(void);

ISI_Return ISID_lockTexts(void);

ISI_Return ISID_lockEvts(void);

ISI_Return ISID_lockPres(void);

ISI_Return ISID_lockGChats(void);

void ISID_unlockServices(void);

void ISID_unlockCalls(void);

void ISID_unlockConfs(void);

void ISID_unlockTexts(void);

void ISID_unlockEvts(void);

void ISID_unlockPres(void);

void ISID_unlockGChats(void);

/* All functions pertaining to cleaning */

void ISID_accountClean(ISID_Account *acct_ptr);

void ISID_coderClean(ISIP_Coder *coder_ptr);

void ISID_textClean(ISID_TextId *text_ptr);

/* functions related to service db entry control */

ISID_ServiceId *ISID_serviceCreate(void);

ISI_Return ISID_serviceAdd(
    ISID_ServiceId *service_ptr,
    ISI_Id         *id_ptr);

ISI_Return ISID_serviceDestroy(
    ISI_Id serviceId);

ISI_Return ISID_serviceGet(
    ISI_Id           serviceId, 
    ISID_ServiceId **service_ptr);
    
ISI_Return ISID_serviceGetByProtocol(
    vint             protocolId,
    ISID_ServiceId **service_ptr);
    
ISI_Return ISID_serviceGetFirst(
    ISID_ServiceId **service_ptr);

ISI_Return ISID_serviceCheck4Calls(
    ISI_Id       serviceId,
    ISID_CallId *ignoreCall_ptr);

/* All functions pertaining to calls */

ISID_CallId *ISID_callCreate(
    ISID_ServiceId       *service_ptr,
    char                 *to_ptr,
    char                 *subject_ptr,
    uint16                callType,
    ISI_SessionCidType    cidType,
    ISI_SessionDirection  audioDirection,
    ISI_SessionDirection  videoDirection);

ISID_CallId *ISID_callCreateFromXml(
    ISID_ServiceId       *service_ptr,
    char                 *to_ptr,
    char                 *subject_ptr,
    ISI_SessionCidType    cidType,
    char                 *mediaAttribute_ptr);

ISI_Return ISID_callDestroy(
    ISI_Id callId);

ISI_Return ISID_callAdd(
    ISID_CallId    *call_ptr,
    ISI_Id         *id_ptr);

ISI_Return ISID_callGet(
    ISI_Id        callId, 
    ISID_CallId **call_ptr);

ISI_Return ISID_callGetFirst(
    ISID_CallId **call_ptr);

/* All functions pertaining to text messaging */

ISID_TextId *ISID_textCreate(void);

ISI_Return ISID_textAdd(
    ISID_TextId *text_ptr,
    ISI_Id      *id_ptr);

ISI_Return ISID_textDestroy(
    ISI_Id textId);

ISI_Return ISID_textGet(
    ISI_Id        textId, 
    ISID_TextId **text_ptr);

ISI_Return ISID_textGetFirst(
    ISID_TextId **text_ptr);

ISI_Return ISID_textGetViaServiceIdAndState(
    ISID_ServiceId  *service_ptr, 
    ISID_TextId    **text_ptr,
    ISID_TextState   state);

/* Functions pertaining to tel events */

ISID_EvtId *ISID_evtCreate(void);

ISI_Return ISID_evtAdd(
    ISID_EvtId *evt_ptr,
    ISI_Id     *id_ptr);

ISI_Return ISID_evtDestroy(
    ISI_Id evtId);

ISI_Return ISID_evtGet(
    ISI_Id          evtId, 
    ISID_EvtId **evt_ptr);

ISI_Return ISID_evtGetFirst(
    ISID_EvtId **evt_ptr);

ISI_Return ISID_evtGetViaCallIdAndState(
    ISID_CallId    *call_ptr, 
    ISID_EvtId    **evt_ptr,
    ISID_EvtState   state);

ISI_Return ISID_evtGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr, 
    ISID_EvtId      **evt_ptr,
    ISID_EvtState     state);

/* Functions pertaining to call Conferencing */

ISID_ConfId *ISID_confCreate(void);

ISI_Return ISID_confAdd(
    ISID_ConfId *conf_ptr,
    ISI_Id      *id_ptr);

ISI_Return ISID_confRemoveCall(
    ISID_ConfId *conf_ptr,
    ISID_CallId *call_ptr,
    vint         clearAll);

ISI_Return ISID_confAddCall(
    ISID_ConfId *conf_ptr,
    ISID_CallId *call_ptr);

ISI_Return ISID_confDestroy(ISI_Id confId);

ISI_Return ISID_confGet(
    ISI_Id        confId, 
    ISID_ConfId **conf_ptr);

/* Functions pertaining to call Presence */

ISID_PresId *ISID_presCreate(void);

ISI_Return ISID_presDestroy(
    ISI_Id presId);

ISI_Return ISID_presAdd(
    ISID_PresId *pres_ptr,
    ISI_Id      *id_ptr);

ISI_Return ISID_presAddFirst(
    ISID_PresId *pres_ptr,
    ISI_Id      *id_ptr);

ISI_Return ISID_presGet(
    ISI_Id        presId, 
    ISID_PresId **pres_ptr);

ISI_Return ISID_presGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr, 
    ISID_PresId     **pres_ptr,
    ISID_PresState    state);

ISI_Return ISID_presGetFirst(
    ISID_PresId **pres_ptr);

/* Functions pertaining to group chat room */
ISID_GChatId *ISID_gchatCreate(
    ISID_ServiceId  *service_ptr,
    char            *remoteAddress_ptr,
    char            *invitor_ptr,
    char            *localIdentity_ptr,
    char            *subject_ptr,
    char            *invitation_ptr,
    int              passwordRequired);

ISI_Return ISID_gchatDestroy(
    ISI_Id chatId);

ISI_Return ISID_gchatAdd(
    ISID_GChatId   *chat_ptr,
    ISI_Id         *id_ptr);

ISI_Return ISID_gchatGet(
    ISI_Id         chatId,
    ISID_GChatId **chat_ptr);

/* Misc. functions */
ISID_Account* ISID_accountFind(
    ISID_ServiceId *service_ptr,
    char           *str_ptr,
    vint            isStrRealm);

ISI_Return ISID_accountRemove(
    ISID_ServiceId *service_ptr,
    char           *realm_ptr);

ISI_Return ISID_accountAdd(
    ISID_ServiceId *service_ptr,
    ISID_Account   *acct_ptr);

ISID_Account* ISID_accountGet(
    ISID_ServiceId *service_ptr);

ISI_Return ISID_coderAdd(
    ISIP_Coder     aCoder[],
    vint           coderMaxSize,
    ISIP_Coder    *cdr_ptr);

ISI_Return ISID_coderRemove(
    ISIP_Coder aCoder[],
    vint       coderMaxSize,
    char      *coderName_ptr);

ISI_Return ISID_coderRemoveByPayloadType(
    ISIP_Coder aCoder[],
    vint       coderMaxSize,
    vint       payloadType);

ISIP_Coder* ISID_coderFind(
    ISIP_Coder aCoder[],
    vint       coderMaxSize,
    char      *coderName_ptr,
    char      *coderDescription_ptr);

ISID_CoderE* ISID_coderFindIsi(
    char *coderName_ptr);

ISI_Return ISID_lockFiles(void);

void ISID_unlockFiles(void);

ISID_FileId *ISID_fileCreate(void);

ISI_Return ISID_fileAdd(
    ISID_FileId *file_ptr,
    ISI_Id      *id_ptr);

ISI_Return ISID_fileDestroy(
    ISI_Id fileId);

ISI_Return ISID_fileGet(
    ISI_Id        fileId,
    ISID_FileId **file_ptr);

ISI_Return ISID_fileGetFirst(
    ISID_FileId **file_ptr);

ISI_Return ISID_gchatGetFirst(
    ISID_GChatId **chat_ptr);

ISI_Id ISID_fileGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_FileId     **file_ptr);

ISI_Id ISID_textGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_TextId     **text_ptr);

ISI_Return ISID_evtGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_EvtId      **evt_ptr);

ISI_Return ISID_presGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_PresId     **pres_ptr);

ISI_Return ISID_gchatGetFirstByServiceId(
    ISID_ServiceId *service_ptr,
    ISID_GChatId  **chat_ptr);

ISI_Return ISID_callGetFirstByServiceId(
    ISID_ServiceId *service_ptr,
    ISID_CallId   **call_ptr);

ISI_Return ISID_akaChallengeGet(
    ISID_ServiceId *service_ptr,
    char           *rand_ptr,
    char           *autn_ptr);

ISID_UssdId *ISID_ussdCreate(ISID_ServiceId *service_ptr);

void ISID_ussdClean(
    ISID_UssdId *ussd_ptr);

ISI_Return ISID_ussdAdd(
    ISID_UssdId *ussd_ptr,
    ISI_Id      *id_ptr);

ISI_Return ISID_ussdDestroy(
    ISI_Id ussdId);

ISI_Return ISID_ussdGet(
    ISI_Id        ussdId,
    ISID_UssdId **ussd_ptr);

ISI_Return ISID_ussdGetFirst(
    ISID_UssdId **ussd_ptr);

ISI_Id ISID_ussdGetFirstByServiceId(
    ISID_ServiceId   *service_ptr,
    ISID_UssdId     **ussd_ptr);

ISI_Return ISID_ussdGetViaServiceIdAndState(
    ISID_ServiceId   *service_ptr,
    ISID_UssdId     **ussd_ptr,
    ISID_UssdState    state);

ISI_Return ISID_lockUssds(void);

void ISID_unlockUssds(void);
#endif
