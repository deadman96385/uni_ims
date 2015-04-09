/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1.6 $ $Date: 2007/09/17 18:33:04 $
 */
#ifndef _RFSM_H_
#define _RFSM_H_

#include "rfsm.h"

/*
 * External pointers to Call FSM states
 */
extern const RFSM_State _RFSM_STATE_NONE;
extern const RFSM_State _RFSM_STATE_CS;
extern const RFSM_State _RFSM_STATE_LTE;
extern const RFSM_State _RFSM_STATE_WIFI;

/*
 * Private methods for Call FSM
 */
void _RFSM_setState(
    RFSM_Context *context_ptr,
    RFSM_State   *state_ptr,
    RPM_Event    *event_ptr);

const char *_RFSM_toString(
    RFSM_Context *context_ptr);

#endif /* _RFSM_H_ */
