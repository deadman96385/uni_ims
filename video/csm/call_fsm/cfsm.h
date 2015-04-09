/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 30325 $ $Date: 2014-12-10 18:01:04 +0800 (Wed, 10 Dec 2014) $
 */
#ifndef CFSM_H_
#define CFSM_H_

/* Ringing timeout value in milli-second. */
#define CFSM_RINGING_TIMER_RETRY_MS (30000)
/*
 * Define generic pointer types for our FSM
 */
typedef struct _CFSM_Context *CFSM_Context_Ptr;
typedef struct _CFSM_State *CFSM_State_Ptr;

/*
 * Define function pointer types for FSM state instance methods
 */
typedef void (*CFSM_ProcessEvent)(CFSM_Context_Ptr, CSM_EventCall_Ptr);
typedef void (*CFSM_StateEnter)(CFSM_Context_Ptr, CSM_EventCall_Ptr);
typedef void (*CFSM_StateExit)(CFSM_Context_Ptr, CSM_EventCall_Ptr);

/*
 * Define the FSM State structure.  This is used by individual
 * states to implement instance methods
 */
typedef struct _CFSM_State {
    CFSM_ProcessEvent  processEvent;
    CFSM_StateEnter    stateEnter;
    CFSM_StateExit     stateExit;
    const char        *stateName_ptr;
} CFSM_State;

/*
 * Define the FSM State Context.  This re-entrant object is passed
 * by the FSM between the various states
 */
typedef struct _CFSM_Context {
    vint                 active;
    ISI_Id               callId;
    ISI_Id               serviceId;
    OSAL_Boolean         isIpConference;
    OSAL_Boolean         isFocusOwner;
    OSAL_Boolean         isPreconditionUsed;
    vint                 isEmergency;
    const char          *protoName_ptr;
    CSM_CallDirection    direction;
    CSM_CallState        state;
    vint                 supsrvNotication;
    CFSM_State_Ptr       currentState_ptr;
    CSM_IsiMngr         *isiMngr_ptr;
    const char          *remoteAddress_ptr;
    CSM_OutputEvent     *csmOutput_ptr;
    OSAL_TmrId           timerId; 
    OSAL_TmrId           retryTmrId; /* send message retry timer */
    vint                 lastErrCode;
    ISI_SessionDirection dir;
    char          *cnap_ptr;
    char          *remoteParticipants_ptr;
} CFSM_Context;

/*
 * Public methods for the FSM bass class
 */
CFSM_Context_Ptr CFSM_init(
    CFSM_Context_Ptr,
    CSM_OutputEvent*);

void CFSM_destroy(
    CFSM_Context_Ptr context_ptr);

void CFSM_processEvent(
    CFSM_Context_Ptr,
    CSM_EventCall_Ptr);

void _CFSM_sendCallIndex(
    CFSM_Context_Ptr  context_ptr);

CSM_CallStatus CFSM_getCallStatus(
    CFSM_Context_Ptr  context_ptr);

#endif /* CFSM_H_ */
