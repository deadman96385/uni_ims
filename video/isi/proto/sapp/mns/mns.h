/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef _MNS_H_
#define _MNS_H_

#ifndef MNS_DEBUG
#define MNS_dbgPrintf(x, ...)
#else
#define MNS_dbgPrintf OSAL_logMsg
#endif

#define MNS_STRING_SZ              (128)

typedef enum {
    MNS_ERR = -1,
    MNS_OK,
    MNS_PRECONDITION_MET,
    MNS_PRECONDITION_NOT_MET,
    MNS_PRECONDITION_NOT_USED,
    MNS_PRECONDITION_UPDATE_REQUIRED,
} MNS_Return;

typedef enum {
    MNS_STATE_IDLE,
    MNS_STATE_EARLY_OFFER_SENT,
    MNS_STATE_EARLY_OFFER_RECEIVED,
    MNS_STATE_EARLY,
    MNS_STATE_ACTIVE,
    MNS_STATE_ACTIVE_OFFER_SENT,
    MNS_STATE_ACTIVE_OFFER_RECEIVED,
    MNS_STATE_LAST,
} MNS_State;

typedef enum {
    MNS_NEGTYPE_OFFER,
    MNS_NEGTYPE_ANSWER,
} MNS_NegType;

typedef struct {
    vint          type;
    char          lclAes80[MAX_SESSION_SRTP_PARAMS];
    char          lclAes32[MAX_SESSION_SRTP_PARAMS];
    char          rmtAes80[MAX_SESSION_SRTP_PARAMS];
    char          rmtAes32[MAX_SESSION_SRTP_PARAMS];
} MNS_SecurityKeys;

typedef struct {
    MNS_State            state;
    tSession             session;
    tSession            *sess_ptr;
    MNS_SecurityKeys     audioKeys;
    MNS_SecurityKeys     videoKeys;
    OSAL_Boolean         usePrecondition;
    OSAL_Boolean         isPreconditionMet;
    OSAL_Boolean         isPreconditionChanged;
    uint8                rsrcStatus;
} MNS_SessionObj;

typedef struct {
    struct {
        tMedia         amedia;
        tMedia         vmedia;
        vint           usePrecondition;
    } sipConfig;
    OSAL_NetAddress    aAddr;     /* audio address and port */
    OSAL_NetAddress    vAddr;     /* video address and port */
    uint16             aRtcpPort;
    uint16             vRtcpPort;
    MNS_SessionObj     sessStore;
} MNS_ServiceObj;

vint MNS_initService(
    MNS_ServiceObj *mnsService_ptr,
    vint            usePrecondition);

vint MNS_setLocalAddress(
    MNS_ServiceObj  *mnsService_ptr,
    OSAL_NetAddress *address_ptr,
    OSAL_NetSockId   fd);

void MNS_clearSession(
    MNS_SessionObj *mns_ptr);

vint MNS_processEvent(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj *mns_ptr,
    tUaEvtType      event,
    tUaAppEvent    *uaEvt_ptr,
    ISIP_Message   *isi_ptr);

vint MNS_processCommand(
    MNS_ServiceObj *service_ptr,
    MNS_SessionObj *mns_ptr,
    ISIP_Message   *m_ptr);

vint MNS_ip2ipUpdate(
    MNS_SessionObj  *mns_ptr,
    OSAL_NetAddress *addr_ptr);

OSAL_Boolean MNS_isOfferReceived(
    MNS_SessionObj  *mns_ptr);

OSAL_Boolean MNS_isSessionActive(
    MNS_SessionObj  *mns_ptr);

OSAL_Boolean MNS_isSessionEarly(
    MNS_SessionObj  *mns_ptr);

OSAL_Boolean MNS_isSessionEarlyIdle(
    MNS_SessionObj  *mns_ptr);

OSAL_Boolean MNS_isPreconditionUsed(
    MNS_SessionObj  *mns_ptr);

OSAL_Boolean MNS_isPreconditionMet(
    MNS_SessionObj  *mns_ptr);

void MNS_loadDefaultVideo(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr);

void MNS_loadDefaultAudio(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr);

vint MNS_isNewMediaAdded(
    MNS_SessionObj  *mns_ptr);

#endif
