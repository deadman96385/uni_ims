/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 7544 $ $Date: 2008-09-05 19:45:05 -0400 (Fri, 05 Sep 2008) $
 *
 */

/*
 * This file is the voice read interface
 */
#include "_ve_private.h"

/*
 * ======== _VE_sendEvent() ========
 *
 * place event in event q
 *
 * infc specifies the queue to place the event into
 * 
 */
void _VE_sendEvent(
    _VE_Queues   *q_ptr,
    VTSP_EventMsg  *msg_ptr,
    uvint           infc)
{

    if (NULL != msg_ptr) { 
        /* send this msg
         */
        if (OSAL_SUCCESS != (OSAL_msgQSend(q_ptr->eventQ, (char *)msg_ptr,
                        sizeof(VTSP_EventMsg), OSAL_NO_WAIT, NULL))) { 
            /* Queue Full error; application needs to service queue */
            _VE_TRACE(__FILE__, __LINE__);
        }

        /* After sending event, zero event buffer for next call
         */
        OSAL_memSet(msg_ptr, 0, sizeof(VTSP_EventMsg));
    }
}


