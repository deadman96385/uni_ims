/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 988 $ $Date: 2006-11-02 15:47:08 -0800 (Thu, 02 Nov 2006) $
 */

#include <osal.h>
#include "rir_event.h"
#include "_rir_log.h" 

static OSAL_TmrId  _RIR_tmrId;
static OSAL_MsgQId _RIR_eventQueue;
RIR_EventMsg _RIR_tmrEvt;

/*
 * ======== _RIR_tmrSendRirEvent() ========
 *
 * Send an event to RIR.
 * 
 * Returns:
 * -1 : Failed
 *  0 : Success
 */
vint _RIR_tmrSendRirEvent(
    RIR_EventMsg *evt_ptr)
{
    OSAL_SelectTimeval time;

    /*
     * Time this event was sent.
     */
    OSAL_selectGetTime(&time);
    evt_ptr->tickusec = time.usec;
    evt_ptr->ticksec = time.sec;


    if (OSAL_SUCCESS != OSAL_msgQSend(_RIR_eventQueue, (char *)evt_ptr,
            sizeof(RIR_EventMsg), OSAL_NO_WAIT, NULL)) {
        return (-1);
    }

    return (0);
}


/*
 * ======== _RIR_tmrFunc() ========
 *
 * This is timer module to generate timer events to RIR.
 * It could be a separate module but for now it is bundled with
 * RIR as there is no timer module in vPort.
 * 
 * Timer main function.
 *  
 * Returns:
 *  0.
 */
OSAL_TmrReturn _RIR_tmrFunc(
    OSAL_TmrArg arg)
{    
    _RIR_tmrEvt.code = RIR_EVENT_MSG_CODE_TIME;
    _RIR_tmrEvt.msg.time.count = 1;
    _RIR_tmrSendRirEvent(&_RIR_tmrEvt);

    return ((OSAL_TmrReturn)0);
}

/*
 * ======== _RIR_tmrStart() ========
 *
 * main() function for timer.
 * 
 * Returns:
 * 
 */
void _RIR_tmrStart(
    void)
{
    
    _RIR_tmrId = OSAL_tmrCreate(); 
     
    if (OSAL_SUCCESS != OSAL_tmrPeriodicStart(
            _RIR_tmrId,
            _RIR_tmrFunc,
            (OSAL_TmrArg)NULL,
            1000)) {
        _RIR_logMsg("%s %d\n", __FILE__, __LINE__);
        return;
    }     

    /* Create RIR event queue */
    if (0 == (_RIR_eventQueue = OSAL_msgQCreate(RIR_EVENT_QUEUE_NAME,
            OSAL_MODULE_RIR, OSAL_MODULE_RIR, OSAL_DATA_STRUCT_RIR_EventMsg,
            RIR_EVENT_QUEUE_DEPTH, sizeof(RIR_EventMsg), 0))) {
        return;
    }

    /*
     * Go!
     */
    
    _RIR_tmrEvt.code = RIR_EVENT_MSG_CODE_STATE;
    _RIR_tmrEvt.msg.state.code = RIR_STATE_MSG_CODE_TIME;
    _RIR_tmrEvt.msg.state.up = 1;
    _RIR_tmrSendRirEvent(&_RIR_tmrEvt);
}

/*
 * ======== _RIR_tmrStop() ========
 *
 * Exit function for timer.
 * 
 * Returns:
 * 
 */
void _RIR_tmrStop(
    void)
{    
    RIR_EventMsg _RIR_tmrEvt;
    
    OSAL_tmrStop(_RIR_tmrId);
    OSAL_tmrDelete(_RIR_tmrId);
    
    _RIR_tmrEvt.code = RIR_EVENT_MSG_CODE_STATE;
    _RIR_tmrEvt.msg.state.code = RIR_STATE_MSG_CODE_TIME;
    _RIR_tmrEvt.msg.state.up = 0;
    _RIR_tmrSendRirEvent(&_RIR_tmrEvt);

    /* Delete RIR event queue */
    OSAL_msgQDelete(_RIR_eventQueue);
}
