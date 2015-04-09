/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29683 $ $Date: 2014-11-04 14:18:29 +0800 (Tue, 04 Nov 2014) $
 */

#ifndef _GAPP_CALL_H_
#define _GAPP_CALL_H_

/* SDP media description string size. */
#define GAPP_CALL_MEDIA_DESC_STRING_SZ    (128)
#define GSM_MEDIA_NEG_STATUS_NONE          (0)
#define GSM_MEDIA_NEG_STATUS_UNCONDITIONAL (1)
#define GSM_MEDIA_NEG_STATUS_PROPOSE       (2)
#define GSM_MEDIA_NEG_STATUS_ACCEPT        (3)
#define GSM_MEDIA_NEG_STATUS_REJECT        (4)
enum {
    GAPP_CALL_STATE_IDLE = 0,
    GAPP_CALL_STATE_RING,
    GAPP_CALL_STATE_ACTIVE,
    GAPP_CALL_STATE_LOGIN,
    GAPP_CALL_STATE_MAKE,
    GAPP_CALL_STATE_ANSWER,
    GAPP_CALL_STATE_DIAL,
    GAPP_CALL_STATE_TERM,
    GAPP_CALL_STATE_HOLD,
    GAPP_CALL_STATE_RESUME,
    GAPP_CALL_STATE_ALERTING, /* Remote is ringing. */
};

/* 
 * Note these are call related events that pertain to a call's state.  
 * These enumerations must have values that match the values specified in the
 * AT Command spec for the +CLCC call status event.
 */
enum {
    GAPP_CALL_EVT_ACTIVE = 0,
    GAPP_CALL_EVT_HELD,
    GAPP_CALL_EVT_DIALING,
    GAPP_CALL_EVT_ALERTING,
    GAPP_CALL_EVT_INCOMING,
    GAPP_CALL_EVT_WAITING,
    GAPP_CALL_EVT_TERMINATED,
    GAPP_CALL_EVT_NONE,
};

enum {
    GAPP_CALL_EVT_EXT_IDLE = 1,
    GAPP_CALL_EVT_EXT_CALLING_MO,
    GAPP_CALL_EVT_EXT_CONNECTING_MO,
    GAPP_CALL_EVT_EXT_ALERTING_MO,
    GAPP_CALL_EVT_EXT_ALERTING_MT,
    GAPP_CALL_EVT_EXT_ACTIVE,
    GAPP_CALL_EVT_EXT_RELEASED_MO,
    GAPP_CALL_EVT_EXT_RELEASED_MT,
    GAPP_CALL_EVT_EXT_USER_BUSY,
    GAPP_CALL_EVT_EXT_USER_DTMD_USER_BUSY, /* User Determined User Busy. */
    GAPP_CALL_EVT_EXT_CALL_WAITING_MO,
    GAPP_CALL_EVT_EXT_CALL_WAITING_MT,
    GAPP_CALL_EVT_EXT_CALL_HOLD_MO,
    GAPP_CALL_EVT_EXT_CALL_HOLD_MT,
};

void GAPP_callInit(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr);

vint GAPP_unsolicitedSrvccEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr);

vint GAPP_callResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           resultId,
    ISIP_Message     *isi_ptr);

vint GAPP_callUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr);

void GAPP_isiCallCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    GAPP_Event      *evt_ptr);

void GAPP_isiAudioCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    GAPP_Event      *evt_ptr);

GAPP_CallObj* GAPP_getCallByIsiId(
    GAPP_ServiceObj *service_ptr,
    ISI_Id           id);

vint GAPP_getNumberIsiCalls(
    GAPP_ServiceObj *service_ptr);

void GAPP_callVdxIsiEvt(
    ISI_Id              serviceId,
    ISI_Id              callId,
    int                 protocolId,
    char               *target_ptr,
    ISIP_Message       *isi_ptr);

void GAPP_callIsiEvt(
    ISI_Id              serviceId,
    ISI_Id              callId,
    vint                protocolId,
    ISIP_CallReason     reason,
    ISIP_Status         status,
    ISIP_Message       *isi_ptr);

void GAPP_setCallBit(
    uint32      *bits_ptr,
    vint         idx,
    OSAL_Boolean set);

GAPP_CallObj* GAPP_getCallByGsmId(
    GAPP_ServiceObj *service_ptr,
    GSM_Id           id);

char* GAPP_getDisconnectCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr);

const char* GAPP_getResumeCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr);

const char* GAPP_getHoldCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr);

const char* GAPP_getAnswerCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr);

const char* GAPP_getRejectCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr);

#endif
