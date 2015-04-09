/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1.6 $ $Date: 2007/09/17 18:33:04 $
 */
#ifndef RFSM_H_
#define RFSM_H_

#include <rpm.h>

/*
 * Define generic pointer types for our FSM
 */
typedef struct _RFSM_Context *RFSM_Context_Ptr;

/*
 * Define function pointer types for FSM state instance methods
 */
typedef void (*RFSM_ProcessEvent)(RFSM_Context_Ptr, RPM_Event*);
typedef void (*RFSM_StateEnter)(RFSM_Context_Ptr, RPM_Event*);
typedef void (*RFSM_StateExit)(RFSM_Context_Ptr, RPM_Event*);

/*
 * Define the FSM State structure.  This is used by individual
 * states to implement instance methods
 */
typedef struct _RFSM_State {
    RFSM_ProcessEvent  processEvent;
    RFSM_StateEnter    stateEnter;
    RFSM_StateExit     stateExit;
    const char        *stateName_ptr;
} RFSM_State;

/*
 * Define the FSM State Context.  This re-entrant object is passed
 * by the FSM between the various states
 */
typedef struct _RFSM_Context {
    RFSM_State         *currentState_ptr;
    RPM_RadioInterface  radioInfc;
    RPM_RadioInterface  emergencyRadioInfc;
    RPM_RadioInterface  lteRadioInfc; /* Store current lte ip address */
    RPM_RadioInterface  wifiRadioInfc; /* Store current wifi ip address */
    RPM_RadioInterface  lteSupsrvRadioInfc; /* Store current lte supsrv ip address */
    RPM_ServiceState    csState;
    RPM_ServiceState    lteState; /* lte state */
} RFSM_Context;

/*
 * Public methods for the FSM bass class
 */
void RFSM_init(
    RFSM_Context *context_ptr);

void RFSM_processEvent(
    RFSM_Context *context_ptr,
    RPM_Event    *event_ptr);

#endif /* RFSM_H_ */
