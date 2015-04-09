/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _CSM_CALLS_H_
#define _CSM_CALLS_H_

#include <osal.h>
#include <rpm.h>
#include "_csm_isi.h"
#include "call_fsm/cfsm.h"

#define CSM_NUMBER_OF_CALLS_MAX (6)
#define CSM_PRESENCE_STRING_SZ  (1024)

/* The timeout value to re-send message to queue */
#define CSM_CALL_SEND_MSG_RETRY_MS  (10)

/* The timeout value to wait call leg release from network side. */
#define CSM_CALL_SRVCC_NET_TIMEOUT_VALUE (5000)

#define CSM_CALL_USER_REJECT_MEDIA_CHANGE_STR "User rejects media change"
#define CSM_CALL_SRVCC_FAILURE_REASON_STR \
        "SIP ;cause=487 ;text=\"failure to transition to CS domain\""

/*
 * csm_event could pass these end call reasonDesc strings to specify sip error code
 * the first 3 chars must be the reason code digit for this purpose.
 */
#define CSM_CALL_REJECT_REASON_NOT_ACCEPTABLE   "488 Not Acceptable Here"
#define CSM_CALL_REJECT_REASON_BUSY             "486 Busy Here"

/* Call media negotiation exchange enumeration. */
typedef enum {
    CSM_CALL_NEG_EXCHANGE_NONE = 0,
    CSM_CALL_NEG_EXCHANGE_LOCAL_CHANGED,
    CSM_CALL_NEG_EXCHANGE_LOCAL_PROPOSED,
    CSM_CALL_NEG_EXCHANGE_LOCAL_ACCEPTED,
    CSM_CALL_NEG_EXCHANGE_LOCAL_REJECTED,
    CSM_CALL_NEG_EXCHANGE_REMOTE_PROPOSED,
    CSM_CALL_NEG_EXCHANGE_REMOTE_ACCEPTED,
    CSM_CALL_NEG_EXCHANGE_REMOTE_REJECTED,
    CSM_CALL_NEG_EXCHANGE_MEDIA_CHANGED,
} CSM_CallNegExchange;

/*
 * CSM_CallParticipants
 *
 * Object containing all information about a single participant
 */
typedef struct {
    vint                callIndex;
    CSM_CallAddressType type;
    CSM_CallNumberType  numberType;
    char                number[CSM_NUMBER_STRING_SZ + 1];
    char                alpha[CSM_ALPHA_STRING_SZ + 1];
    char                normalizedAddress[CSM_ALPHA_STRING_SZ + 1];    
} CSM_CallParticipants;

/*
 * Define the SRVCC info structure. This is used to reocrd
 * the CS call id to do call switch when and
 * PS call id if want to end video when SRVCC success
 */
typedef struct {
    vint                isVcc;
    ISI_Id              csCallId;
    ISI_Id              videoCallId;
} CSM_SrvccInfo;

/*
 * CSM_CallObject
 *
 * Object encapsulating all info relavant to a single voice call
 */
typedef struct {
    vint                  inUse;
    CSM_CallMode          mode;
    CFSM_Context          callFsm;   /* The state machine context */
    CSM_CallParticipants  participants[CSM_EVENT_MAX_CALL_LIST_SIZE];
    CSM_CallMultiParty    multiParty;
    vint                  numHistories;
    CSM_CallHistory       callHistories[CSM_HISTORY_MAX_LIST_SIZE];
    CSM_SrvccInfo         vccInfo;
                          /* Negotiation exchange status. */
    CSM_CallNegExchange   negExchange;
                          /* Cache session type remote proposed. */
    uint8                 negSessionType;
                          /* Current call session type. */
    uint8                 sessionType;
    CSM_PrivateInputEvt   csmInternalInputEvent;
} CSM_CallObject;

typedef struct {
    ISI_Id confId;
    vint   numCalls;
} CSM_CsConference;

typedef struct {
    vint invitations[CSM_NUMBER_OF_CALLS_MAX + 1]; // STEVE init
} CSM_IpConference;

typedef struct {
    union {
        CSM_IpConference ip;
        CSM_CsConference cs;
    }u;
    char   scratch[CSM_PRESENCE_STRING_SZ + 1];
} CSM_ConferenceManager;

/* 
 * Top level class of the Calls Package 
 */
typedef struct {
    CSM_ConferenceManager  confManager;
    CSM_CallObject         callObj[CSM_NUMBER_OF_CALLS_MAX + 1];
    int                    numCalls;
    CSM_IsiMngr           *isiMngr_ptr;
    vint                   availableCallIndex[CSM_EVENT_MAX_CALL_LIST_SIZE + 1];
} CSM_CallManager;

/* 
 * CSM Call Manager package public methods 
 */
vint CSM_callsInit(
    CSM_CallManager    *callManager_ptr,
    CSM_IsiMngr        *isiMngr_ptr);

vint CSM_callsProcessEvent(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

vint CSM_internalProcessEvent(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

vint CSM_callsShutdown(
    void);

CSM_CallObject* CSM_getCall(
    ISI_Id callId);

void CSM_decrementCsConference(
    void);

vint CSM_getCsConferenceId(
    void);

vint CSM_getCsConferenceCallNums(
    void);

vint CSM_getIpConferenceInvitation(
    void);

CSM_IsiMngr* CSM_getIsiMngr(
    void);

vint CSM_getActiveConference(
    void);

void CSM_cleanIpConference(
    void);

vint CSM_getCallIndex(
    void);

void CSM_clearCallIndex(
    vint callIndex);

void CSM_setCallIndex(
    vint callIndex);

void CSM_callsOnSrvccChange(
    RPM_EventType eventType);

OSAL_Boolean _CSM_isCallInUse(
    CSM_CallObject *call_ptr);

void _CSM_generateInitilizingCallEvent(
    CSM_CallObject  *callObject_ptr,
    OSAL_Boolean     isRsrcReady,
    OSAL_Boolean     isActive,
    CSM_OutputEvent *csmOutput_ptr);

void _CSM_generateClccReport(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void _CSM_generateMonitorReport(
    CSM_CallEvt     *callEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void _CSM_generateSupsrvReport(
    CSM_CallObject  *callObject_ptr,
    CSM_OutputEvent *csmOutput_ptr,
    char            *reasonDesc_ptr);

vint CSM_callsNumberInUse(
    void);

OSAL_Boolean CSM_isCallInRing(
    CSM_CallObject *call_ptr);

OSAL_Boolean _CSM_mapOtherConfCallState(
    CSM_CallObject *call_ptr,
    CSM_CallState      state);

void CSM_callsConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputCallEvt_ptr,
    CSM_CallEvt        *csmCallEvt_ptr);

void CSM_internalConvertToInternalEvt(
    void               *inputCallEvt_ptr,
    CSM_CallEvt        *csmCallEvt_ptr);

void _CSM_generateCallModification(
    CSM_CallObject  *callObject_ptr,
    CSM_OutputEvent *csmOutput_ptr);

void CSM_generateVideoRequestKeyEvent(
    CSM_OutputEvent *csmOutput_ptr);

/* 
 * CSM Call Manager private methods
 */

#endif //_CSM_CALLS_H_
