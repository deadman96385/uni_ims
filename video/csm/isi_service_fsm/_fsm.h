/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 24298 $ $Date: 2014-01-28 17:28:15 +0800 (Tue, 28 Jan 2014) $
 */
#ifndef __ISI_SERVICE_FSM_H__
#define __ISI_SERVICE_FSM_H__
#include "fsm.h"

/*
 * External pointers to Call FSM states
 */
extern const FSM_State _FSM_LOGIN_STATE;
extern const FSM_State _FSM_ACTIVE_STATE;
extern const FSM_State _FSM_AUTH_FAIL_STATE;
extern const FSM_State _FSM_ERROR_STATE;
extern const FSM_State _FSM_RESET_STATE;
extern const FSM_State _FSM_LOGOUT_STATE;

/*
 * Private FSM methods
 */
void _FSM_setState(
    FSM_Context_Ptr context_ptr,
    FSM_State_Ptr   state_ptr,
    FSM_Event_Ptr   event_ptr);

#endif /* _FSM_H_ */
