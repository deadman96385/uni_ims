/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#ifndef FSM_H_
#define FSM_H_

#include <vce.h>
#include <codec.h>

/* Ringing timeout value in milli-second. */
#define FSM_RINGING_TIMER_RETRY_MS (30000)
/*
 * Define generic pointer types for our FSM
 */
typedef struct _FSM_Context *FSM_Context_Ptr;
typedef struct _FSM_State   *FSM_State_Ptr;

/*
 * Define function pointer types for FSM state instance methods
 */
typedef void (*FSM_ProcessEvent)(FSM_Context_Ptr, VCE_Event, vint, char*);
typedef void (*FSM_StateEnter)(FSM_Context_Ptr, VCE_Event, vint);
typedef void (*FSM_StateExit)(FSM_Context_Ptr, VCE_Event);

/*
 * Define the FSM State structure.  This is used by individual
 * states to implement instance methods
 */
typedef struct _FSM_State {
    FSM_ProcessEvent  processEvent;
    FSM_StateEnter    stateEnter;
    FSM_StateExit     stateExit;
    const char       *stateName_ptr;
} FSM_State;

/*
 * Define the FSM State Context.  This re-entrant object is passed
 * by the FSM between the various states
 */
typedef struct _FSM_Context {
    vint                active;
    FSM_State_Ptr       currentState_ptr;
    CODEC_Ptr           codec_ptr;
} FSM_Context;

/*
 * Public methods for the FSM bass class
 */
FSM_Context_Ptr FSM_init(
    FSM_Context_Ptr,
    CODEC_Ptr);

void FSM_destroy(
    FSM_Context_Ptr);

void FSM_processEvent(
    FSM_Context_Ptr,
    VCE_Event,
    vint,
    char*);

#endif /* FSM_H_ */
