/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 25157 $ $Date: 2014-03-16 09:59:26 -0700 (Sun, 16 Mar 2014) $
 */
#ifndef _FSM_H_
#define _FSM_H_

#include "fsm.h"

/*
 * External pointers to Call FSM states
 */
extern const FSM_State _FSM_STATE_IDLE;
extern const FSM_State _FSM_STATE_DECODE;
extern const FSM_State _FSM_STATE_ENCODE;
extern const FSM_State _FSM_STATE_ENCODEDECODE;

/*
 * Private methods for Call FSM
 */
void _FSM_setState(
    FSM_Context_Ptr,
    FSM_State_Ptr,
    VCE_Event,
    vint);

const char *_FSM_toString(
    FSM_Context_Ptr);

#endif /* _FSM_H_ */
