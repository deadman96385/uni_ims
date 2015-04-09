/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 19339 $ $Date: 2012-12-15 03:31:08 +0800 (Sat, 15 Dec 2012) $
 */

#include <osal.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>

#include "isi.h"
#include "isi_errors.h"
#include "isip.h"
#include "isi.h"
#include "isip.h"

#include "_sapp.h"
#include "_sapp_reg.h"
#include "_sapp_dialog.h"
#include "_sapp_emergency.h"

typedef enum {
    EMG_EVT_NONE,
    EMG_EVT_REG_COMPLETE,
    EMG_EVT_REG_FAILED,
    EMG_EVT_REG_EXPIRED,
    EMG_EVT_CALL_INITIAL,
    EMG_EVT_CALL_TERMINATE,
    EMG_EVT_CALL_FAIL
} SAPP_emgcySmEvent;

#ifdef SAPP_DEBUG
static char *_SAPP_EmgcyStateStr[SAPP_EMGERGENCY_STATE_LAST + 1] = {
    "SAPP_EMGERGENCY_STATE_IDLE",
    "SAPP_EMGERGENCY_STATE_ACTIVE",
    "SAPP_EMGERGENCY_STATE_EXPIRED_REG",
    "SAPP_EMGERGENCY_STATE_WAIT_FOR_REG",
    "SAPP_EMGERGENCY_STATE_IN_CALL",
    "SAPP_EMGERGENCY_STATE_LAST"
};
#endif

/* 
 * ======== _SAPP_emgcySendRegTimerEvent() ========
 *
 * This function sends a timer related event to a queue used to serialize 
 * register related events.
 *
 * Returns: 
 *   SAPP_OK  : Event sent.
 *   SAPP_ERR : Error.
 */
static vint _SAPP_emgcySendRegTimerEvent(
    SAPP_ServiceObj  *service_ptr)
{
    SAPP_TmrEvent  tmrEvt;
    
    /* Setup the event to send */
    tmrEvt.type = SAPP_TMR_EVENT_REG_EXPIRED;
    tmrEvt.arg_ptr = service_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(service_ptr->emergencyObj.tmrEvtQ,
            (char *)&tmrEvt, sizeof(SAPP_TmrEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED\n", __FUNCTION__, __LINE__);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_emgcyRegTimerCb() ========
 *
 * This function is the callback functio of emergency re-registration expires.
 *
 * Returns: 
 *  Nothing.
 */
static int32 _SAPP_emgcyRegTimerCb(
    void *arg_ptr)
{
    SAPP_ServiceObj    *service_ptr = (SAPP_ServiceObj *)arg_ptr;

    if (SAPP_OK != _SAPP_emgcySendRegTimerEvent(service_ptr)) {
        /* Now start the timer */
        OSAL_tmrStart(service_ptr->emergencyObj.emergencyMsgTmrId,
                _SAPP_emgcyRegTimerCb, arg_ptr, SAPP_MESSAGE_RETRY_TIMER_MS);
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/* 
 * ======== _SAPP_emgcyStartReRegTmr() ========
 *
 * This function is to start re-registration timer.
 *
 * Returns: 
 *   SAPP_OK  : Process done.
 *   SAPP_ERR : Error in start timer.
 */
vint _SAPP_emgcyStartReRegTmr(
    SAPP_EmgcyObj   *emgcy_ptr,
    SAPP_ServiceObj *service_ptr,
    vint             expires)
{
    if (0 == emgcy_ptr->emergencyTmrId) {
        return (SAPP_ERR);
    }
    else {
        OSAL_tmrStop(emgcy_ptr->emergencyTmrId);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrStart(emgcy_ptr->emergencyTmrId,
            _SAPP_emgcyRegTimerCb, service_ptr, 1000 * (expires / 2))) {
        OSAL_tmrDelete(emgcy_ptr->emergencyTmrId);
        emgcy_ptr->emergencyTmrId = 0;
        return (SAPP_ERR);
    }
    return (SAPP_OK);   
}

/* 
 * ======== SAPP_emgcyInit() ========
 *
 * This function is used to initialize SAPP_EmgcyObj
 *
 * Returns: 
 *   SAPP_OK: Init ok.
 *   SAPP_ERR: Init failed.
 */
vint SAPP_emgcyInit(
    SAPP_EmgcyObj *emgcy_ptr,
    OSAL_MsgQId    tmrEvtQ)
{
    emgcy_ptr->state = SAPP_EMGERGENCY_STATE_IDLE;
    emgcy_ptr->call_ptr = NULL;
    emgcy_ptr->tmrEvtQ = tmrEvtQ;
    /* Create timer at init time */
    if (0 == (emgcy_ptr->emergencyTmrId = OSAL_tmrCreate())) {
        return (SAPP_ERR);
    }
    if (0 == (emgcy_ptr->emergencyMsgTmrId = OSAL_tmrCreate())) {
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/* 
 * ======== SAPP_emgcyDestroy() ========
 *
 * This function is used to destroy SAPP_EmgcyObj
 *
 * Returns: 
 *   Nothing.
 */
void SAPP_emgcyDestroy(
    SAPP_EmgcyObj *emgcy_ptr)
{
    /* Delete timer */
    if (0 != emgcy_ptr->emergencyTmrId) {
        OSAL_tmrDelete(emgcy_ptr->emergencyTmrId);
        emgcy_ptr->emergencyTmrId = 0;
    }
    if (0 != emgcy_ptr->emergencyMsgTmrId) {
        OSAL_tmrDelete(emgcy_ptr->emergencyMsgTmrId);
        emgcy_ptr->emergencyMsgTmrId = 0;
    }

    return;
}

/* 
 * ======== _SAPP_emgcyRegExpiredHandler() ========
 *
 * This function is to process emergency registration expiration.
 *
 * Returns: 
 *   Nothing.
 */
static void _SAPP_emgcyRegExpiredHandler(
    SAPP_EmgcyObj *emgcy_ptr)
{
    /* Stop re-registration timer */
    OSAL_tmrStop(emgcy_ptr->emergencyTmrId);
    OSAL_tmrStop(emgcy_ptr->emergencyMsgTmrId);

    return;
}

/* 
 * ======== _SAPP_emgcyIdleState() ========
 *
 * This function is to process emergency registration evnet for idle state.
 *
 * Returns: 
 *   Next emergency state.
 */
static SAPP_EmgcyState _SAPP_emgcyIdleState(
    SAPP_emgcySmEvent cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    SAPP_Event       *evt_ptr,
    vint              expires)
{
    SAPP_EmgcyState state;
    SAPP_EmgcyObj   *emgcy_ptr;

    emgcy_ptr = &service_ptr->emergencyObj;
    switch (cmd) {
        case EMG_EVT_REG_COMPLETE:
            state = SAPP_EMGERGENCY_STATE_ACTIVE;
            /* Start re-reg timer */
            _SAPP_emgcyStartReRegTmr(emgcy_ptr, service_ptr, expires);
            break;
        case EMG_EVT_CALL_INITIAL:
            SAPP_regStart(reg_ptr, service_ptr, evt_ptr, sip_ptr);
            state = SAPP_EMGERGENCY_STATE_WAIT_FOR_REG;
            break;
        case EMG_EVT_REG_FAILED:
        default:
            state = SAPP_EMGERGENCY_STATE_IDLE;
            break;
    }
    return (state);
}

/* 
 * ======== _SAPP_emgcyActiveState() ========
 *
 * This function is to process emergency registration evnet for active state.
 *
 * Returns: 
 *   Next emergency state.
 */
static SAPP_EmgcyState _SAPP_emgcyActiveState(
    SAPP_emgcySmEvent cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    vint              expires)
{
    SAPP_EmgcyState state;
    SAPP_CallObj    *call_ptr;
    SAPP_Event      *evt_ptr;
    SAPP_EmgcyObj   *emgcy_ptr;
    
    emgcy_ptr = &service_ptr->emergencyObj;
    evt_ptr = &sip_ptr->event;
    call_ptr = service_ptr->emergencyObj.call_ptr;
    switch (cmd) {
        case EMG_EVT_REG_EXPIRED:
            _SAPP_emgcyRegExpiredHandler(emgcy_ptr);
            state = SAPP_EMGERGENCY_STATE_EXPIRED_REG;
            break;
        case EMG_EVT_CALL_INITIAL:
            if (SAPP_OK != SAPP_sipCallInitiateOutbound(sip_ptr, service_ptr,
                    call_ptr, cmd_ptr, evt_ptr)) {
                /* Call failed.  The ISI event is already written */
                SAPP_sipDestroyCall(call_ptr);
                state = SAPP_EMGERGENCY_STATE_ACTIVE;
            }
            state = SAPP_EMGERGENCY_STATE_IN_CALL;
            break;
        case EMG_EVT_REG_COMPLETE:
            state = SAPP_EMGERGENCY_STATE_ACTIVE;
            /* Start re-reg timer */
            _SAPP_emgcyStartReRegTmr(emgcy_ptr, service_ptr, expires);
            break;
        default:
            state = SAPP_EMGERGENCY_STATE_ACTIVE;
            break;
    }
    return (state);
}

/* 
 * ======== _SAPP_emgcyExpiredState() ========
 *
 * This function is to process emergency registration evnet for expire state.
 *
 * Returns: 
 *   Next emergency state.
 */
static SAPP_EmgcyState _SAPP_emgcyRegExpiredState(
    SAPP_emgcySmEvent cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    SAPP_Event       *evt_ptr,
    vint              expires)
{
    SAPP_EmgcyState state;

    switch (cmd) {
        case EMG_EVT_CALL_INITIAL:
            SAPP_regReReg(reg_ptr, service_ptr, evt_ptr, sip_ptr);
            state = SAPP_EMGERGENCY_STATE_WAIT_FOR_REG;
            break;
        default:
            state = SAPP_EMGERGENCY_STATE_EXPIRED_REG;
            break;
    }
    return (state);
}

/* 
 * ======== _SAPP_emgcyWaitForRegState() ========
 *
 * This function is to process emergency registration evnet for wait
 * registration complete state.
 *
 * Returns: 
 *   Next emergency state.
 */
static SAPP_EmgcyState _SAPP_emgcyWaitForRegState(
    SAPP_emgcySmEvent cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    vint              expires)
{
    SAPP_EmgcyState state;
    SAPP_CallObj   *call_ptr;
    SAPP_Event     *evt_ptr;
    SAPP_EmgcyObj  *emgcy_ptr;

    emgcy_ptr = &service_ptr->emergencyObj;

    call_ptr = service_ptr->emergencyObj.call_ptr;
    evt_ptr = &call_ptr->event;
    
    switch (cmd) {
        case EMG_EVT_REG_COMPLETE:
            if (SAPP_OK != SAPP_sipCallInitiateOutbound(sip_ptr, service_ptr,
                    call_ptr, &call_ptr->event.isiMsg, evt_ptr)) {
                /* Call failed.  The ISI event is already written */
                SAPP_sipDestroyCall(call_ptr);
                state = SAPP_EMGERGENCY_STATE_ACTIVE;
            }
            state = SAPP_EMGERGENCY_STATE_IN_CALL;
            /* Start re-reg timer */
            _SAPP_emgcyStartReRegTmr(emgcy_ptr, service_ptr, expires);
            break;
        case EMG_EVT_REG_FAILED:
        default:
            state = SAPP_EMGERGENCY_STATE_IDLE;
            break;
    }
    return (state);
}

/* 
 * ======== _SAPP_emgcyInCallState() ========
 *
 * This function is to process emergency registration event for in call state.
 *
 * Returns: 
 *   Next emergency state.
 */
static SAPP_EmgcyState _SAPP_emgcyInCallState(
    SAPP_emgcySmEvent cmd,
    SAPP_RegObj      *reg_ptr,
    SAPP_ServiceObj  *service_ptr,
    SAPP_Event       *evt_ptr,
    vint              expires)
{
    SAPP_EmgcyState  state;
    SAPP_EmgcyObj   *emgcy_ptr;
    
    emgcy_ptr = &service_ptr->emergencyObj;

    switch (cmd) {
        case EMG_EVT_CALL_FAIL:
        case EMG_EVT_CALL_TERMINATE:
            state = SAPP_EMGERGENCY_STATE_ACTIVE;
            break;
        case EMG_EVT_REG_EXPIRED:
            _SAPP_emgcyRegExpiredHandler(emgcy_ptr);
            SAPP_regReReg(reg_ptr, service_ptr, evt_ptr, NULL);
            state = SAPP_EMGERGENCY_STATE_IN_CALL;
            break;
        case EMG_EVT_REG_COMPLETE:
            state = SAPP_EMGERGENCY_STATE_IN_CALL;
            /* Start re-reg timer */
            _SAPP_emgcyStartReRegTmr(emgcy_ptr, service_ptr, expires);
            break;
        default:
            state = SAPP_EMGERGENCY_STATE_IN_CALL;
            break;
    }
    return (state);
}

/* 
 * ======== SAPP_emgcyFSMProcessEvent() ========
 *
 * This function is to process emergency event
 *
 * Returns: 
 *   SAPP_OK: Event processed successfully.
 *   SAPP_ERR: Event processed failed.
 */
vint SAPP_emgcyFSMProcessEvent(
    SAPP_emgcySmEvent cmd,
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    SAPP_CallObj     *call_ptr,
    vint              expires)
{
    SAPP_RegObj      *reg_ptr;
    SAPP_EmgcyObj    *emgcy_ptr;
    SAPP_Event       *evt_ptr;
    
    SAPP_EmgcyState state = SAPP_EMGERGENCY_STATE_LAST;
    reg_ptr = &service_ptr->registration;
    emgcy_ptr = &service_ptr->emergencyObj;
    evt_ptr = &sip_ptr->event;

    /* Ignore the event if it's not emergency service */
    if (!service_ptr->isEmergency) {
        return (SAPP_OK);
    }

    SAPP_dbgPrintf("%s: Emergency state:%s\n", __FUNCTION__,
            _SAPP_EmgcyStateStr[emgcy_ptr->state]);
 
    /* if cmd_ptr is not null ,save to emgcy_ptr  */
    if (NULL != cmd_ptr) {
        emgcy_ptr->call_ptr = call_ptr;
        /* Cache the cmd to the SAPP_Event in the call object */
        OSAL_memCpy(&emgcy_ptr->call_ptr->event.isiMsg, cmd_ptr,
                sizeof(ISIP_Message));
        OSAL_memCpy(&emgcy_ptr->call_ptr->event.isiEvt, &sip_ptr->event.isiEvt,
                sizeof(OSAL_MsgQId));
    }

    switch (emgcy_ptr->state) {
        case SAPP_EMGERGENCY_STATE_IDLE:
            state = _SAPP_emgcyIdleState(cmd, reg_ptr, service_ptr,
                    sip_ptr, evt_ptr, expires);
            break;
        case SAPP_EMGERGENCY_STATE_ACTIVE:
            state = _SAPP_emgcyActiveState(cmd, reg_ptr, service_ptr,
                    sip_ptr, cmd_ptr, expires);
            break;
        case SAPP_EMGERGENCY_STATE_EXPIRED_REG:
            state = _SAPP_emgcyRegExpiredState(cmd, reg_ptr, service_ptr,
                    sip_ptr, evt_ptr, expires);
            break;
        case SAPP_EMGERGENCY_STATE_WAIT_FOR_REG:
            state = _SAPP_emgcyWaitForRegState(cmd, reg_ptr, service_ptr,
                    sip_ptr, expires);
            break;
        case SAPP_EMGERGENCY_STATE_IN_CALL:
            state = _SAPP_emgcyInCallState(cmd, reg_ptr, service_ptr, evt_ptr,
                    expires);
            break;
        default:
            break;
    }
    if (state != SAPP_EMGERGENCY_STATE_LAST && state != emgcy_ptr->state) {
        emgcy_ptr->state = state;
        /* There a state change */
        SAPP_dbgPrintf("%s: Emergency End state:%s \n", __FUNCTION__,
                _SAPP_EmgcyStateStr[emgcy_ptr->state]);
        return (SAPP_OK);
    }
    SAPP_dbgPrintf("%s: Emergency state:%s \n", __FUNCTION__,
                _SAPP_EmgcyStateStr[emgcy_ptr->state]);
    return (SAPP_ERR);
}

/* 
 * ======== SAPP_emgcyCallInitiate() ========
 *
 * This function is to process emergency call initiation.
 *
 * Returns: 
 *   None.
 */
void SAPP_emgcyCallInitiate(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    SAPP_CallObj     *call_ptr)
{ 
    SAPP_emgcyFSMProcessEvent(EMG_EVT_CALL_INITIAL, service_ptr, sip_ptr,
            cmd_ptr, call_ptr, 0);
    return;
}

/* 
 * ======== SAPP_emgcyCallTerminate() ========
 *
 * This function is to process emergency call terminate.
 *
 * Returns: 
 *   None.
 */
void SAPP_emgcyCallTerminate(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    SAPP_CallObj     *call_ptr)
{ 
    SAPP_emgcyFSMProcessEvent(EMG_EVT_CALL_TERMINATE, service_ptr, sip_ptr,
            cmd_ptr, call_ptr, 0);
    return;
}

/* 
 * ======== SAPP_emgcyRegComplete() ========
 *
 * This function is to process emergency registration completion
 *
 * Returns: 
 *   None.
 */
void SAPP_emgcyRegComplete(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr,
    vint              expires)
{ 
    SAPP_emgcyFSMProcessEvent(EMG_EVT_REG_COMPLETE, service_ptr, sip_ptr,
            cmd_ptr, NULL, expires);
    return;
}

/* 
 * ======== SAPP_emgcyRegFailed() ========
 *
 * This function is to process emergency registration failure
 *
 * Returns: 
 *   None.
 */
void SAPP_emgcyRegFailed(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr)
{ 
    SAPP_emgcyFSMProcessEvent(EMG_EVT_REG_FAILED, service_ptr, sip_ptr,
            cmd_ptr, NULL, 0);
    return;
}

/* 
 * ======== SAPP_emgcyRegExpire() ========
 *
 * This function is to process emergency registration expiration. 
 *
 * Returns: 
 *   None.
 */
void SAPP_emgcyRegExpired(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    ISIP_Message     *cmd_ptr)
{ 
    SAPP_emgcyFSMProcessEvent(EMG_EVT_REG_EXPIRED, service_ptr, sip_ptr,
            cmd_ptr, NULL, 0);
    return;
}
