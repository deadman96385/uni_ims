/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 24447 $ $Date: 2014-02-06 14:53:34 +0800 (Thu, 06 Feb 2014) $
 *
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <xcap.h>
#include "supsrv.h"
#include "_supsrv.h"


/*
 * =========SUPSRV_EventsTask()=========
 * 1. This task get event from XCAP and send it to the destination
 *    via callback function.
 * 2. supsrvMngr_ptr->qId should be assigned before creating SUPSRV_EventsTask.
 *
 * Return
 *   0
 */
OSAL_TaskReturn SUPSRV_EventsTask(
    OSAL_TaskArg arg_ptr)
{
    SUPSRV_XcapObj  *supSrvXcapObj_ptr;
    SUPSRV_Mngr     *supsrvMngr_ptr;
    XCAP_Evt        *msg_ptr;
    static SUPSRV_Evt       event;

    supsrvMngr_ptr    = (SUPSRV_Mngr *)arg_ptr;
    supSrvXcapObj_ptr = &supsrvMngr_ptr->supSrvXcap;

    /*
     * Typecast to separate message types.
     */
    msg_ptr               = &(event.xcap.evt);
    event.type            = SUPSRV_EVENT_TYPE;
    event.xcap.reason     = SUPSRV_REASON_XCAP_EVT;

SUPSRV_EVENT_TASK_LOOP:
    
    OSAL_memSet(msg_ptr, 0, sizeof(XCAP_Evt));
    /*
     * Check if there are any messages to process, and if yes, get a message.
     */
    while (!XCAP_getEvt(&(supSrvXcapObj_ptr->xcap), msg_ptr, -1)) {
        OSAL_logMsg("%s %d: Error getting event\n", __FUNCTION__, __LINE__);
        goto SUPSRV_EVENT_TASK_END;
    }
    _SUPSRV_dbgPrintf("%s %d: Got event.\n", __FUNCTION__, __LINE__);
   
    _SUPSRV_dbgPrintf("\n%s %d hdr=%s \nbody=%s\n errorcode=%d\n ", __FILE__, 
            __LINE__, msg_ptr->hdr_ptr, msg_ptr->body_ptr, msg_ptr->error);
    /* Send the XCAP event back. */
    if (supsrvMngr_ptr->onResultCB != NULL) {
        if (SUPSRV_OK != supsrvMngr_ptr->onResultCB(supsrvMngr_ptr->arg_ptr, &event)) {
            _SUPSRV_dbgPrintf("%s:%d Send Event Failed\n", __FUNCTION__, __LINE__);
        }
    }
    goto SUPSRV_EVENT_TASK_LOOP;
  
SUPSRV_EVENT_TASK_END:
    OSAL_logMsg("%s:%d SHUTDOWN SUPSRV_EventsTask\n", __FILE__, __LINE__);
    supsrvMngr_ptr->tId = 0;
    return(0);
}

