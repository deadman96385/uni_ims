/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 27294 $ $Date: 2014-07-07 13:38:14 +0800 (Mon, 07 Jul 2014) $
 */
#ifndef _ISI_SERVICE_FSM_H_
#define _ISI_SERVICE_FSM_H_

#define FSM_ERROR_TIMER_RETRY_MS (20000)

typedef enum {
    FSM_EVENT_TYPE_NONE = 0,
    FSM_EVENT_TYPE_FAILURE,
    FSM_EVENT_TYPE_NEW_IP,
    FSM_EVENT_TYPE_ACTIVE,
    FSM_EVENT_TYPE_ACTIVATING,
    FSM_EVENT_TYPE_INACTIVE,
    FSM_EVENT_TYPE_TIMER,
    FSM_EVENT_TYPE_AUTH_FAIL,
    FSM_EVENT_TYPE_START,
    FSM_EVENT_TYPE_AKA_REQUIRED,
    FSM_EVENT_TYPE_AKA_RESPONSE,
    FSM_EVENT_TYPE_STOP,
    FSM_EVENT_TYPE_LAST,
} FSM_EventType;

/*
 * Define generic pointer types for our FSM
 */
typedef struct _FSM_Event {
    FSM_EventType   eventType;
    OSAL_NetAddress ipAddress;
    vint            errorCode; /*
                                * If there is an HTTP error (3xx-6xx) or a
                                * D2 error (0-99) then it will be populated here.
                                */
    char            reasonDesc[CSM_EVENT_STRING_SZ + 1];
} FSM_Event;

typedef struct _FSM_Context *FSM_Context_Ptr;
typedef struct _FSM_State *FSM_State_Ptr;
typedef struct _FSM_Event *FSM_Event_Ptr;

/*
 * Define function pointer types for FSM state instance methods
 */
typedef void (*FSM_ProcessEvent)(FSM_Context_Ptr, FSM_Event_Ptr);
typedef void (*FSM_StateEnter)(FSM_Context_Ptr, FSM_Event_Ptr);
typedef void (*FSM_StateExit)(FSM_Context_Ptr, FSM_Event_Ptr);

/*
 * Define the FSM State structure.  This is used by individual
 * states to implement instance methods
 */
typedef struct _FSM_State {
    FSM_ProcessEvent   processEvent;
    FSM_StateEnter     stateEnter;
    FSM_StateExit      stateExit;
    const char        *stateName_ptr;
} FSM_State;

/*
 * Define the FSM State Context.  This re-entrant object is passed
 * by the FSM between the various states
 */
typedef struct _FSM_Context {
    FSM_State_Ptr     currentState_ptr;
    void             *serviceMngr_ptr;
    ISI_Id            serviceId;
    CSM_OutputEvent  *csmOutput_ptr;
} FSM_Context;

/*
 * Public methods for the FSM bass class
 */
vint FSM_init(
    FSM_Context_Ptr  context_ptr,
    void            *acctMngr_ptr,
    ISI_Id           serviceId,
    CSM_OutputEvent *csmOutput_ptr);

void FSM_destroy(
    FSM_Context_Ptr context_ptr);

void FSM_processEvent(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr);

void FSM_getEventTypeFromReason(
    const char    *reason_ptr,
    FSM_Event_Ptr  event_ptr);

OSAL_Boolean _FSM_isActive(
    FSM_Context_Ptr context_ptr);

OSAL_Boolean _FSM_isServiceIp(
    FSM_Context_Ptr context_ptr);

OSAL_Boolean _FSM_isServiceLteIp(
    FSM_Context_Ptr context_ptr);

OSAL_Boolean _FSM_isServiceEmergencyIp(
    FSM_Context_Ptr context_ptr);
 
OSAL_Boolean _FSM_isServiceRcsIp(
    FSM_Context_Ptr context_ptr);

OSAL_Boolean _FSM_isServiceMasterIp(
    FSM_Context_Ptr context_ptr);

void _FSM_sendServiceStateEvent(
    CSM_ServiceState state,
    vint             errorCode,
    vint             isEmergency,
    CSM_OutputEvent *csmOutput_ptr);

OSAL_Boolean _FSM_isServiceReady(
    FSM_Context_Ptr context_ptr);

void _FSM_sendServiceStateEventToCall(
    FSM_Context_Ptr context_ptr,
    FSM_EventType   eventType);

void _FSM_sendServiceNotification(
    FSM_Context_Ptr context_ptr,
    FSM_Event_Ptr   event_ptr);

void _FSM_sendErrorEvent(
    FSM_EventType    eventType,
    char            *reasonDesc_ptr,
    CSM_OutputEvent *csmOutput_ptr);

#endif /* FSM_H_ */
