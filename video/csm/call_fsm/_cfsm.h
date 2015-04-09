/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30280 $ $Date: 2014-12-09 14:24:57 +0800 (Tue, 09 Dec 2014) $
 */
#ifndef _CFSM_H_
#define _CFSM_H_

#include "cfsm.h"
#include "_csm_calls.h"

/*
 * External pointers to Call FSM states
 */
extern const CFSM_State _CFSM_STATE_CALL;
extern const CFSM_State _CFSM_STATE_DIALING;
extern const CFSM_State _CFSM_STATE_HANGINGUP;
extern const CFSM_State _CFSM_STATE_HOLD_LOCAL;
extern const CFSM_State _CFSM_STATE_RESET;
extern const CFSM_State _CFSM_STATE_RINGING;
extern const CFSM_State _CFSM_STATE_TERMINATED;
extern const CFSM_State _CFSM_STATE_TRYING;
extern const CFSM_State _CFSM_STATE_WAITING;
extern const CFSM_State _CFSM_STATE_CS_CONF;
extern const CFSM_State _CFSM_STATE_IP_CONF;
extern const CFSM_State _CFSM_STATE_INITIALIZING;
extern const CFSM_State _CFSM_STATE_ANSWERED;
extern const CFSM_State _CFSM_STATE_IMS_CONF;

/*
 * Private methods for Call FSM
 */
void _CFSM_setState(
    CFSM_Context_Ptr,
    CFSM_State_Ptr,
    CSM_EventCall_Ptr);

const char *_CFSM_toString(
    CFSM_Context_Ptr);

void _CFSM_initiateCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr);

void _CFSM_initiateConfCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr,
    char             *rsrcList_ptr);

void _CFSM_terminateCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr);

void _CFSM_rejectCall(
    CFSM_Context_Ptr    context_ptr,
    CSM_EventCall_Ptr   event_ptr,
    char               *reason_ptr);

void _CFSM_acceptCall(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr);

OSAL_Boolean _CFSM_isFailoverToCs(
    CFSM_Context_Ptr  context_ptr);

OSAL_Boolean _CFSM_failoverToCs(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr);

int _CFSM_getParticipantsCount(
    CSM_CallObject *call_ptr);

void _CFSM_processParticipantInfo(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr);

void _CFSM_processGroupParticipantInfo(
    CFSM_Context_Ptr  context_ptr,
    CSM_EventCall_Ptr event_ptr);


void  _CFSM_processCallModify(
    CSM_EventCall_Ptr      event_ptr,
    const CFSM_Context_Ptr context_ptr);

void _CFSM_processMediaControl(
    CSM_EventCall_Ptr      event_ptr,
    const CFSM_Context_Ptr context_ptr);

void _CFSM_setNegExchange(
    CFSM_Context_Ptr    context_ptr,
    CSM_CallNegExchange negExchange);

void _CFSM_processRemoteAnswer(
    const CFSM_Context_Ptr context_ptr);

void _CFSM_sendInitRegistration(
    const CFSM_Context_Ptr context_ptr);

vint _CFSM_sendRingingTimeout(
    const CFSM_Context_Ptr context_ptr);

#endif /* _CFSM_H_ */
