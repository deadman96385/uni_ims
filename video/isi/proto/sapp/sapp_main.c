/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30369 $ $Date: 2014-12-11 19:09:13 +0800 (Thu, 11 Dec 2014) $
 */

#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_dbase_sys.h>

#include "isi.h"
#include "isi_errors.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "msrp_dbg.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_dialog.h"
#include "_sapp_te.h"
#include "_sapp_im_page.h"
#include "_sapp_ussd.h"
#include "_sapp_capabilities.h"
#include "_sapp_emergency.h"
#include "_sapp_reg.h"
#include "sapp_main.h"


#ifdef INCLUDE_SIMPLE
#include "_simple.h"
#endif

#include <settings.h>

#include <vport_revinfo.c>
#include <sr.h>
#include <ims_net.h>

/* Global object. */
static SAPP_GlobalObj  *_SAPP_globalObj_ptr;

/* 
 * ======== _SAPP_sipDebugLog() ========
 * 
 * This function is registered with the SIP stack and is used when SIP 
 * debug logging is enabled.  When SIP wants to log a message this function
 * will be called to print the log message.
 *
 * Returns: 
 *   Nothing.
 */
#ifdef SAPP_DEBUG
static void _SAPP_sipDebugLog(
    char   *str_ptr,
    uint32  arg0,
    uint32  arg1,
    uint32  arg2)
{
    /* Uncomment out below line manually if running on pent1 */
    // return /* temporarily remove since this fails on 64-bit pent1 platform */ 

    char format[128];
    OSAL_SelectTimeval tv;
    OSAL_strncpy(format, "%lu.%lu: ", sizeof(format));
    strncat(format, str_ptr, sizeof(format));
    strncat(format, "\n", sizeof(format));
    OSAL_selectGetTime(&tv);
    /* Now print it */
    OSAL_logMsg(format, tv.sec, tv.usec, arg0, arg1, arg2);
    return;
}
#endif

/* 
 * ======== _SAPP_systemIsiEvt() ========
 *
 * This populates an ISI event related to 'system' level events.
 *
 * Return Values: 
 * None
 */   
static void _SAPP_systemIsiEvt(
    ISIP_SystemReason   reason,
    ISIP_Status         status,
    char               *ipcName_ptr,    
    char               *audioName_ptr,
    char               *streamName_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = 0;
    isi_ptr->code = ISIP_CODE_SYSTEM;
    isi_ptr->protocol = _SAPP_globalObj_ptr->protocolId;
    isi_ptr->msg.system.reason = reason;
    isi_ptr->msg.system.status = status;
    /* 
     * Tell ISI of the IPC names we are using for ISI communication, 
     * Audio control (tones) and stream control. 
     */
    isi_ptr->msg.system.protocolIpc[0] = 0;
    if (NULL != ipcName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.protocolIpc,
                ISI_ADDRESS_STRING_SZ, "%s", ipcName_ptr);
    }
        
    isi_ptr->msg.system.mediaIpc[0] = 0;
    if (NULL != audioName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.mediaIpc,
                ISI_ADDRESS_STRING_SZ, "%s", audioName_ptr);
    }
    
    isi_ptr->msg.system.streamIpc[0] = 0;
    if (NULL != streamName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.streamIpc,
                ISI_ADDRESS_STRING_SZ, "%s", streamName_ptr);
    }
    return;
}

/* 
 * ======== _SAPP_timerCb() ========
 *
 * This function is registered with a timer used for SAPP registration with
 * ISI.  The timer calls this function when it expires.  IT will send 
 * an ISI event indicating that it wants to register with ISI.
 *
 * Return Values: 
 * None
 */
static int32 _SAPP_timerCb(
    void *arg_ptr) 
{
    SAPP_Event *evt_ptr;

    evt_ptr = (SAPP_Event*)arg_ptr;

    /* This is a periodic timer, do not need to wait forever for sending msg. */
    if (OSAL_SUCCESS != OSAL_msgQSend(evt_ptr->isiEvt, (char *)&evt_ptr->isiMsg,
            sizeof(ISIP_Message), OSAL_WAIT_FOREVER, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, evt_ptr->isiEvt);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static vint _SAPP_sendTimerEvent(
    SAPP_TmrEventType type,
    uint32            cnt,
    void             *arg_ptr)
{
    SAPP_SipObj   *sip_ptr;
    SAPP_TmrEvent  tmrEvt;
    
    sip_ptr = (SAPP_SipObj *)arg_ptr;

    /* Setup the event to send */
    tmrEvt.type = type;
    tmrEvt.cnt = cnt;
    tmrEvt.arg_ptr = arg_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(sip_ptr->queue.tmrEvt, (char *)&tmrEvt,
            sizeof(SAPP_TmrEvent), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED\n", __FUNCTION__, __LINE__);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static int32 _SAPP_sleepWatchDogCb(
    void *arg_ptr)
{
    int32  diff;
    OSAL_TimeVal ts;
    OSAL_TimeVal now;
    /*SAPP_dbgPrintf("_SAPP_sleepWatchDogCb");*/ 
    /* get target ts that we need to track */
    SIPTIMER_getWakeUpTime(&ts);

    /* check it*/
    if (ts.sec > 0) {
        OSAL_timeGetTimeOfDay(&now);
        diff = ts.sec - now.sec;
        /*SAPP_dbgPrintf("Watch dog firing diff is :%d", diff);*/ 
        if (diff <= 0) {
            /* Send an event back on SAPP */
            SIPTIMER_AddWakeUpTime(0);
            _SAPP_sendTimerEvent(SAPP_TMR_SLEEP_DETECTED, 0, arg_ptr);
        }
    }
    return (0);
}

/* 
 * ======== _SAPP_timerInit() ========
 *
 * This function initializes a timer used to register SAPP with
 * ISI.  The timer fires at the specified interval until 
 * ISI returns a Command to SAPP indicating that it received
 * this event.
 *
 * Return Values: 
 * SAPP_OK The timer was succesfully initialized.
 * SAPP_ERR The timer failed to initialize
 */
static vint _SAPP_timerInit(
    SAPP_SipObj   *sip_ptr)
{
    if (0 == sip_ptr->tmr.id) {
        /* Launch a timer that will attempt to register to ISI */
        if (0 == (sip_ptr->tmr.id = OSAL_tmrCreate())) {
            /* Then we can't register with ISI! */
            return (SAPP_ERR);
        }
    }
    else {
        OSAL_tmrStop(sip_ptr->tmr.id);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrPeriodicStart(sip_ptr->tmr.id, _SAPP_timerCb, 
            &sip_ptr->tmr.event, SAPP_REGISTER_TIMER_MS)) {
        OSAL_tmrDelete(sip_ptr->tmr.id);
        sip_ptr->tmr.id = 0;
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

static vint _SAPP_sleepWatchDogInit(
    SAPP_SipObj   *sip_ptr)
{
    if (0 == sip_ptr->sleepWatchDog.id) {
        /* Launch a timer that will attempt to register to ISI */
        if (0 == (sip_ptr->sleepWatchDog.id = OSAL_tmrCreate())) {
            /* Then we can't register with ISI! */
            return (SAPP_ERR);
        }
    }
    else {
        OSAL_tmrStop(sip_ptr->sleepWatchDog.id);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrPeriodicStart(sip_ptr->sleepWatchDog.id,
            _SAPP_sleepWatchDogCb, sip_ptr, SAPP_SLEEP_WATCH_DOG_TIMER_MS)) {
        OSAL_tmrDelete(sip_ptr->sleepWatchDog.id);
        sip_ptr->sleepWatchDog.id = 0;
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_timerDestroy() ========
 *
 * This function kills/frees the timer used to register this app to ISI.
 *
 * Return Values: 
 * None.
 */
static void _SAPP_timerDestroy(
    SAPP_SipObj *sip_ptr)
{
    /* Kill/Free the timer */
    if (0 != sip_ptr->tmr.id) {
        OSAL_tmrStop(sip_ptr->tmr.id);
        OSAL_tmrDelete(sip_ptr->tmr.id);
        sip_ptr->tmr.id = 0;
    }   

    if (0 != sip_ptr->sleepWatchDog.id) {
        OSAL_tmrStop(sip_ptr->sleepWatchDog.id);
        OSAL_tmrDelete(sip_ptr->sleepWatchDog.id);
        sip_ptr->sleepWatchDog.id = 0;
    }

    return;
}

/* 
 * ======== _SAPP_isiSystemCmd() ========
 * This function is the entry point for commands from ISI related to 
 * the protocol or "system".
 *
 * Return Values:
 * None.
 */
static void _SAPP_isiSystemCmd(
    SAPP_SipObj     *sip_ptr, 
    ISIP_Message    *cmd_ptr)
{
    if (ISIP_SYSTEM_REASON_START == cmd_ptr->msg.system.reason &&
            ISIP_STATUS_DONE == cmd_ptr->msg.system.status) {
        /* Then SAPP successfully contacted/registered ISI */

        /* 03-08-2011. Disabling code below so isi sends 'refresh' */
        /* _SAPP_timerDestroy(sip_ptr);
        SAPP_dbgPrintf("%s: Successfully registered with ISI\n", __FUNCTION__); */
    }
    else if (ISIP_SYSTEM_REASON_SHUTDOWN == cmd_ptr->msg.system.reason &&
            ISIP_STATUS_DONE == cmd_ptr->msg.system.status) {
        /* Then restart the re-reg timer */
        _SAPP_timerInit(sip_ptr);
    }
    return;
}

static void _SAPP_WakeUp(SAPP_SipObj *sip_ptr) {
    /* Then let's tell SIP about it */
    vint x;
    for (x = 0 ; x < SAPP_SIP_MAX_UA ; x++) {
        if (0 != sip_ptr->service[x].isiServiceId) {
            UA_WakeUp(sip_ptr->service[x].sipConfig.uaId);
        }
    }
}

#ifndef INCLUDE_SIMPLE
/* 
 * ======== _SAPP_isiErrorEvt() ========
 *
 * This populates an ISI event related to IM's error events.
 *
 * Return Values: 
 * None
 */   
static void _SAPP_isiErrorEvt(
    ISIP_Code       code,
    ISIP_Message   *cmd_ptr,
    SAPP_Event     *sip_ptr)
{
    ISIP_Message       *isi_ptr;
    
    isi_ptr = &sip_ptr->isiMsg;
    isi_ptr->code = code;
    isi_ptr->protocol = cmd_ptr->protocol;
    isi_ptr->id = cmd_ptr->id;

    SAPP_dbgPrintf("%s:%d SAPP_ERR: not support SIMPLE\n",
            __FUNCTION__, __LINE__);    
    if (ISIP_CODE_MESSAGE == code) {
        isi_ptr->msg.message.reason = ISIP_TEXT_REASON_ERROR;
        isi_ptr->msg.message.serviceId = cmd_ptr->msg.message.serviceId;
        isi_ptr->msg.message.chatId = cmd_ptr->msg.message.chatId;
    }
    else if (ISIP_CODE_FILE == code){
        isi_ptr->msg.file.reason = ISIP_FILE_REASON_ERROR;
        isi_ptr->msg.file.serviceId = cmd_ptr->msg.file.serviceId;
    }
    else if (ISIP_CODE_PRESENCE == code){
        isi_ptr->msg.presence.reason = ISIP_PRES_REASON_ERROR;       
        isi_ptr->msg.presence.serviceId = cmd_ptr->msg.presence.serviceId;
    }
    else if (ISIP_CODE_CHAT == code) {
        isi_ptr->msg.groupchat.reason = ISIP_CHAT_REASON_FAILED;
        isi_ptr->msg.groupchat.serviceId = cmd_ptr->msg.groupchat.serviceId;
    }    
    return;
}
#endif

/* xxx considering moving SAPP_sipYyy functions into SAPP_sip.c.h 
 * the refactoring consider SAPP have following sub modules and underlining engine
 *      SAPP_sip module for SAPP talking to SIP
 *      MSRP for SIMPLE
 *      XCAP for SIMPLE
 *      SIP for underlining engine
 * SAPP_init will start all the sub module, while SAPP_sipInit will start only SAPP_sip.
 */

/* 
 * ======== _SAPP_processEvt() ========
 *
 * This is SAPP_sip event processing in task thread.
 * It handles events from UA and sends interesting ones to ISI
 *
 * Return Values: 
 * None.
 */
void _SAPP_processEvt(
    SAPP_SipObj  *sip_ptr,
    OSAL_MsgQId   fromq,
    SAPP_Msg     *msg_ptr)
{
    SAPP_ServiceObj *service_ptr;
    SAPP_CallObj    *call_ptr;

    /* 
     * Before we do anything clear our the buffer used to construct isi events.
     * Since isi_ptr->code will be ISIP_CODE_INVALID after this call, then 
     * We know if there is an event to write to isi at the bottom of this
     * function by seeing if anyone changed the isi event buffer from 
     * ISIP_CODE_INVALID to something else.
     */
    OSAL_memSet(&sip_ptr->event.isiMsg, 0, sizeof(ISIP_Message));

    /* SIP message. Deal with it in the UA's callback function. */
    if (sip_ptr->queue.sipEvt == fromq) {
        if (UA_GetEvent(&msg_ptr->u.sip, &sip_ptr->sipEvt) == SIP_OK) {
            SAPP_sipEvent(sip_ptr, &sip_ptr->sipEvt);
        }
    }

    /* Received a message from ISI. */
    else if (sip_ptr->queue.isiCmd == fromq) {
        
        if (_SAPP_globalObj_ptr->protocolId != msg_ptr->u.isi.protocol) {
            return;
        }
        
        /* Process ISI command. switch the message type ("code") and process */
        switch (msg_ptr->u.isi.code) {
        case ISIP_CODE_SYSTEM:
            _SAPP_isiSystemCmd(sip_ptr, &msg_ptr->u.isi);
            break;
        case ISIP_CODE_SERVICE:
            SAPP_isiServiceCmd(&msg_ptr->u.isi, sip_ptr, &sip_ptr->event);
            break;
        case ISIP_CODE_MESSAGE:
            if (0 == msg_ptr->u.isi.msg.message.chatId) {
                SAPP_isiImPageCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);
            }
#ifdef INCLUDE_SIMPLE
            else {
                SIMPL_isiImSessionImCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);
            }
#else
            /* Return fail to ISI */
            else {
                _SAPP_isiErrorEvt(msg_ptr->u.isi.code, &msg_ptr->u.isi,
                        &sip_ptr->event);
            }
#endif
            break;
        case ISIP_CODE_PRESENCE:

            /* capabilities uses Presence framework */
            if (msg_ptr->u.isi.msg.presence.reason ==
                    ISIP_PRES_REASON_CAPABILITIES_REQUEST) {

                _SAPP_isiCapabilitiesRequestCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);

                break; /* don't allow SIMPL module to process */
            }

#ifdef INCLUDE_SIMPLE
            if (0 == msg_ptr->u.isi.msg.presence.chatId) {
                SIMPL_isiPresCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);
            }

            else {
                SIMPL_isiPresSessionCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);
            }
#else
            /* Return fail to ISI */
            _SAPP_isiErrorEvt(msg_ptr->u.isi.code, &msg_ptr->u.isi,
                    &sip_ptr->event);
#endif
            break;
        case ISIP_CODE_CHAT:
#ifdef INCLUDE_SIMPLE
            SIMPL_isiImSessionCmd(&msg_ptr->u.isi, sip_ptr,
                    &sip_ptr->event);
#else
            /* Return fail to ISI */
            _SAPP_isiErrorEvt(msg_ptr->u.isi.code, &msg_ptr->u.isi,
                    &sip_ptr->event);
#endif
            break;

        case ISIP_CODE_FILE:
#ifdef INCLUDE_SIMPLE
            if (0 == msg_ptr->u.isi.msg.file.chatId) {
                SIMPL_isiFileTransferCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);
            }
            else {
                SIMPL_isiImSessionFileCmd(&msg_ptr->u.isi, sip_ptr,
                        &sip_ptr->event.isiMsg);
            }
#else
            /* Return fail to ISI */
            _SAPP_isiErrorEvt(msg_ptr->u.isi.code, &msg_ptr->u.isi,
                    &sip_ptr->event);
#endif
            break;
        case ISIP_CODE_TEL_EVENT:
            SAPP_isiTeCmd(&msg_ptr->u.isi, sip_ptr, &sip_ptr->event);
            break;
        case ISIP_CODE_CALL:

            SAPP_isiCallCmd(&msg_ptr->u.isi, sip_ptr, &sip_ptr->event);

            break;
        case ISIP_CODE_USSD:
            SAPP_isiUssdCmd(&msg_ptr->u.isi, sip_ptr, &sip_ptr->event);
            break;
        default:
            break;
        }
    }
    else if (sip_ptr->queue.tmrEvt == fromq) {
        /* Handle the SIMPLE related timer event  */
        if (SAPP_TMR_SLEEP_DETECTED == msg_ptr->u.tmr.type) {
            _SAPP_WakeUp(sip_ptr);
        }
        else if (SAPP_TMR_EVENT_REG_EXPIRED == msg_ptr->u.tmr.type) {
            service_ptr = (SAPP_ServiceObj *)msg_ptr->u.tmr.arg_ptr;
            SAPP_emgcyRegExpired(service_ptr, sip_ptr, NULL);
        }
        else if (SAPP_TMR_EVENT_REG_RETRY == msg_ptr->u.tmr.type) {
            service_ptr = (SAPP_ServiceObj *)msg_ptr->u.tmr.arg_ptr;
            /*
             * No need to register again if the registration had already
             * stopped and went to SAPP_REG_STATE_NONE state.
             */
            if (SAPP_REG_STATE_NONE != service_ptr->registration.state) {
                if (!_SAPP_sipServiceIsTransportReady(service_ptr) && 
                        (SAPP_OK != SAPP_sipServiceTransportInit(service_ptr,
                            sip_ptr))) {
                    /*
                     * Transport init failed.
                     * Advance PCSCF and start a retry timer.
                     */
                    _SAPP_regAdvancePcscf(service_ptr, sip_ptr);
                    _SAPP_regStartRetryTmr(service_ptr, NULL);
                    /* Prepare a NET event to send. */
                    OSAL_snprintf(sip_ptr->event.isiMsg.msg.service.reasonDesc,
                            ISI_EVENT_DESC_STRING_SZ, "REG FAILED: CODE:%d "
                            "REASON:%s",
                            ISI_IMS_XPORT_INIT_FAILURE,
                            ISI_IMS_XPORT_INIT_FAILURE_STR);
                    SAPP_serviceIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            ISIP_SERVICE_REASON_NET, ISIP_STATUS_FAILED,
                            &sip_ptr->event.isiMsg);
                }
                /* Transport is ready or initialized successfully. */
                SAPP_regStart(&service_ptr->registration, service_ptr,
                        &sip_ptr->event, sip_ptr);
            }
        }
        else if (SAPP_TMR_EVENT_REG_SUBSCRIBE_RETRY == msg_ptr->u.tmr.type) {
            service_ptr = (SAPP_ServiceObj *)msg_ptr->u.tmr.arg_ptr;
            _SAPP_regSubscribe(&service_ptr->registration, service_ptr);
        }
        else if (SAPP_TMR_EVENT_DE_REG == msg_ptr->u.tmr.type) {
            service_ptr = (SAPP_ServiceObj *)msg_ptr->u.tmr.arg_ptr;
            SAPP_regStop(&service_ptr->registration, service_ptr,
                    &sip_ptr->event, sip_ptr);
        }
        else if (SAPP_TMR_EVENT_INVITE_RETRY == msg_ptr->u.tmr.type) {
            call_ptr = (SAPP_CallObj *)msg_ptr->u.tmr.arg_ptr;
            /*
             * The call is destroyed if the isiCallId isn't existed.
             */
            if (call_ptr->isiCallId) {
                service_ptr = SAPP_findServiceViaServiceId(sip_ptr,
                    call_ptr->event.isiMsg.msg.call.serviceId);
                if (SAPP_OK != _SAPP_sipMakeCall(service_ptr, call_ptr,
                        call_ptr->event.isiMsg.msg.call.to,
                        &call_ptr->mnsSession.session, NULL,
                        &sip_ptr->event.isiMsg.msg.call.srvccStatus)) {
                    /* Then there was an error placing the call */
                    _SAPP_getErrorReasonDesc(NULL,
                            &sip_ptr->event.isiMsg.msg.call);
                    SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId, call_ptr->isiCallId,
                            ISIP_CALL_REASON_ERROR, ISIP_STATUS_INVALID,
                            &sip_ptr->event.isiMsg);
                    SAPP_sipDestroyCall(call_ptr);
                }
            }
        }
#ifdef INCLUDE_SIMPLE
        else {
            SIMPL_timerEvent(&msg_ptr->u.tmr, &sip_ptr->event);
        }
#endif
    }
    /* Handle the XCAP event.  Pump it to the simple xcap sub-module */
    else if (sip_ptr->queue.xcapEvt == fromq) {
#ifdef INCLUDE_SIMPLE
        SIMPL_xcapEvent(&msg_ptr->u.xcap, sip_ptr, &sip_ptr->event);
#endif
    }
    /* Handle the XCAP event.  Pump it to the simple xcap sub-module */
    else if (sip_ptr->queue.msrpEvt == fromq) {
#ifdef INCLUDE_SIMPLE
        SIMPL_msrpEvent(&msg_ptr->u.msrp, sip_ptr, &sip_ptr->event);
#endif
    }

    if (sip_ptr->event.isiMsg.code != ISIP_CODE_INVALID) {
        /* Then there is an event waiting to be written to ISI */
        SAPP_sendEvent(&sip_ptr->event);
    }
}


/* 
 * ======== _SAPP_prepare() ========
 * This function is the first function to be called in task thread.
 * It will do all needed preparation begin servicing commands from ISI and 
 * events from the GSM module
 *
 * Return Values:
 * SAPP_OK  SAPP is successfully prepared.
 * SAPP_ERR There was an error preparing the SAPP application.
 *
 */
static int _SAPP_prepare(
    SAPP_SipObj *sip_ptr)
{
    /*
     * First START event will have INVALID status, this status indicates
     * ISI for the first START event. Following START events are with
     * TRYING status. 
     * 
     * assumption: tmr.event is inited at allocation time
     */
    /* Send the first START event to ISI now. */
    SAPP_sendEvent(&sip_ptr->tmr.event);
    
    /* next timer event should be TRYING status */
    sip_ptr->tmr.event.isiMsg.msg.system.reason = ISIP_SYSTEM_REASON_START;
    sip_ptr->tmr.event.isiMsg.msg.system.status = ISIP_STATUS_TRYING;
    
    return (SAPP_OK);
}

/*
 * ======== SAPP_sipUaHandlerTask() ========
 * This is the SIP UA message handler task. It initializes a SIP UA
 * and contains the UA_Entry() function which is the entry point for SIP UA 
 * events.
 * arg_ptr: Global object pointer.
 *
 * Returns:
 *  Never returns.
 */
static void _SAPP_sipUaHandlerTask(
    SAPP_SipObj *sip_ptr)
{
    SAPP_Msg     *msg_ptr;
    OSAL_MsgQId   fromq;
    
    _SAPP_prepare(sip_ptr);

    msg_ptr = &sip_ptr->queue.msg;
    
_SAPP_SIP_UA_HANDLER_TASK_LOOP:

    /*
     * Check if there are any messages to process, and if yes, get a message.
     */
    while (0 >= OSAL_msgQGrpRecv(&sip_ptr->queue.group, (char *)msg_ptr,
            sizeof(SAPP_Msg), OSAL_WAIT_FOREVER, &fromq, NULL)) {
        OSAL_taskDelay(100);
    }

    /* Got a message. */
    _SAPP_processEvt(sip_ptr, fromq, msg_ptr);

    goto _SAPP_SIP_UA_HANDLER_TASK_LOOP;
}

/*
 * ======== _SAPP_allocateQueues() ========
 *
 * private routine for allocating the SAPP module queues
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _SAPP_allocateQueues(
    SAPP_SipObj *sip_ptr,
    char        *ipc_ptr,
    char        *isiIpc_ptr,
    char        *mediaIpc_ptr)
{
    /* Create SIP UA queues. */

    /* Command queue */
    if (0 == (sip_ptr->queue.isiCmd = OSAL_msgQCreate(ipc_ptr, 
            OSAL_MODULE_ISI, OSAL_MODULE_SAPP, OSAL_DATA_STRUCT_ISIP_Message,
            SAPP_MSGQ_LEN, sizeof(ISIP_Message), 0))) {
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }

    /* SIP internal message queue. */
    if (0 == (sip_ptr->queue.sipEvt = OSAL_msgQCreate(SAPP_SIP_EVENT_QUEUE_NAME,
            OSAL_MODULE_SAPP, OSAL_MODULE_SAPP, OSAL_DATA_STRUCT_tSipIpcMsg,
            SAPP_MSGQ_LEN, sizeof(tSipIpcMsg), 0))) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (OSAL_FAIL);
    }
    
    /* SIMPLE Timer Queue */
    if (0 == (sip_ptr->queue.tmrEvt = OSAL_msgQCreate(
            SAPP_SIMPLE_TMR_EVENT_QUEUE_NAME,
            OSAL_MODULE_SAPP, OSAL_MODULE_SAPP, OSAL_DATA_STRUCT_SAPP_TmrEvent,
            SAPP_MSGQ_LEN,
            sizeof(SAPP_TmrEvent), 0))) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (OSAL_FAIL);
    }


    /* ISI Application queue */
    if (0 == (sip_ptr->queue.isiEvt = OSAL_msgQCreate(isiIpc_ptr,
            OSAL_MODULE_SAPP, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
            SAPP_MSGQ_LEN, sizeof(ISIP_Message), 0))) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);        
        SIP_Destroy();
        return (OSAL_FAIL);
    }
    /* Cache the IPC queue ID of ISI so we can write events there */
    sip_ptr->event.isiEvt = sip_ptr->queue.isiEvt;

    /* Queue group. */
    if (OSAL_SUCCESS != OSAL_msgQGrpCreate(&sip_ptr->queue.group)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (OSAL_FAIL);
    }
    
    /* Now add queues. */
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&sip_ptr->queue.group,
            sip_ptr->queue.isiCmd)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (OSAL_FAIL);
    }
    
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&sip_ptr->queue.group,
            sip_ptr->queue.sipEvt)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (OSAL_FAIL);
    }
    
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&sip_ptr->queue.group,
            sip_ptr->queue.tmrEvt)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (OSAL_FAIL);
    }

    return (OSAL_SUCCESS);
}

/*
 * ======== _SAPP_deallocate() ========
 *
 * Internal routine for free up the SAPP_sip module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _SAPP_deallocate(
    SAPP_SipObj *sip_ptr)
{
    vint ret;
    
    ret = OSAL_SUCCESS;

    /* Delete queues. */
    if (OSAL_SUCCESS != OSAL_msgQDelete(sip_ptr->queue.isiCmd)) {
        ret = OSAL_FAIL;
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(sip_ptr->queue.isiEvt)) {
        ret = OSAL_FAIL;
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(sip_ptr->queue.sipEvt)) {
        ret = OSAL_FAIL;
    }
    if (OSAL_SUCCESS != OSAL_msgQDelete(sip_ptr->queue.tmrEvt)) {
        ret = OSAL_FAIL;
    }
    if (OSAL_SUCCESS != OSAL_msgQGrpDelete(&sip_ptr->queue.group)) {
        ret = OSAL_FAIL;
    }
    
    OSAL_memFree(_SAPP_globalObj_ptr, 0);
    _SAPP_globalObj_ptr = NULL;
    
    return ret;
}

/*
 * ======== _SAPP_stop() ========
 *
 * Internal routine for stoping the SAPP_sip module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _SAPP_stop(
    SAPP_SipObj *sip_ptr)
{
    vint ret;
    
    ret = OSAL_SUCCESS;
    if (OSAL_SUCCESS != OSAL_taskDelete(sip_ptr->task.tId)) {
        ret = OSAL_FAIL;
    }
    return ret;
}

/* 
 * ======== SAPP_sipInit() ========
 * Initialize and start the SIP.
 *
 * Returns: 
 * SAPP_OK  : Success  
 * SAPP_ERR : Error
 */
vint _SAPP_sipInit(
    SAPP_SipObj   *sip_ptr)
{
    tSipConfig     sipConfig;
    tSYSDB_Entry   dbEntry;
    char          *hfStr;
    vint           x;

    /* 
     * Initialize the SIP stack.
     * Max server ports are set as recommended by the SIP API doc.
     */
    sipConfig.randomGenSeed   = 0;
    sipConfig.maxUA           = SAPP_SIP_MAX_UA;
    sipConfig.maxTransactions = SAPP_SIP_TRANS_PER_INFC;
    /* 
     * NULL values here will disable the stack's ability 
     * to act like a proxy/registrar.  We are NOT a proxy.
     */
    sipConfig.pfProxy = NULL;
    sipConfig.pProxyFqdn = NULL; 
    /* 
     * This settings tells the SIP Stack what SIP message field to use 
     * when determining the existence of a UA.  Set to the stack's recommended
     * value.
     */
    sipConfig.matchType = SIP_REQUEST_MATCH_REQUEST_URI;

    /* Don't create sip server port */
    sipConfig.udpFd = 0;
    sipConfig.udpPort = 0;

    sipConfig.tcpFd = 0;
    sipConfig.tcpPort = 0;
    sipConfig.maxDialogsPerUa = SAPP_SIP_MAX_DIALOGS;
    sipConfig.mtu = sip_ptr->mtu;
    
    if (SIP_OK != SIP_Init(&sipConfig)) {
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (SAPP_ERR);
    }
    
    // Set any global default header fields below. 
    if (0 != sip_ptr->name[0]) {
        x = OSAL_strlen(sip_ptr->name) + 1;
        if (NULL != (hfStr = SIP_malloc(x))) {
            OSAL_snprintf(hfStr, x, "%s", sip_ptr->name);
            // then there is a value for the user agent header field
            dbEntry.type = eSIP_VALUE_TYPE_STR;
            dbEntry.u.pStr = hfStr;
            SYSDB_HF_Set(eSIP_USER_AGENT_HF, &dbEntry);
        }
    }

    if (0 != sip_ptr->supported[0]) {
        x = OSAL_strlen(sip_ptr->supported);
        /* Get rid of the last ',' if there is one */
        if (',' == sip_ptr->supported[x - 1]) {
            sip_ptr->supported[x - 1] = 0;
        }
        x = OSAL_strlen(sip_ptr->supported) + 1;
        if (NULL != (hfStr = SIP_malloc(x))) {
            OSAL_snprintf(hfStr, x, "%s", sip_ptr->supported);
            // then there is a value for the user agent header field
            dbEntry.type = eSIP_VALUE_TYPE_STR;
            dbEntry.u.pStr = hfStr;
            SYSDB_HF_Set(eSIP_SUPPORTED_HF, &dbEntry);
        }
    }

    /* Add any debug logging calls here */
#ifdef SAPP_DEBUG
    SIP_SetDebugLogCallBack(_SAPP_sipDebugLog);
    SIP_SetDebugLogLevel(SIP_DB_TRANSPORT_MODULE, 3);
    SIP_SetDebugLogLevel(SIP_DB_MEMORY_MODULE, 1);
    SIP_SetDebugLogLevel(SIP_DB_SESSION_MODULE, 1);
    SIP_SetDebugLogLevel(SIP_DB_DECODE_MODULE, 1);
    SIP_SetDebugLogLevel(SIP_DB_SDP_DEC_MODULE, 1);
    SIP_SetDebugLogLevel(SIP_DB_UA_MODULE, 3);
    SIP_SetDebugLogLevel(SIP_DB_TIMER_MODULE, 1);
    SIP_SetDebugLogLevel(SIP_DB_DIALOG_MODULE, 3);
#endif

    /* Set sip timers */
    if ((0 != sip_ptr->t1) || (0 != sip_ptr->t2) || (0 != sip_ptr->t4)) {
        SIP_setTimers(sip_ptr->t1, sip_ptr->t2, sip_ptr->t4);
    }

    return (SAPP_OK);
}

#ifdef INCLUDE_SIMPLE
/* 
 * ======== _SAPP_simpleInit() ========
 * Initialize and start the SIMPLE stack.
 *
 * Returns: 
 * SAPP_OK  : Success  
 * SAPP_ERR : Error
 */
vint _SAPP_simpleInit(
    SAPP_SipObj   *sip_ptr)
{    
    /*
     * Init the MSRP stack.  Multiply number of IM sessions by 2
     * to accommodate both normal IM sessions and file transfers.
     */
    if (MSRP_RETURN_OK != MSRP_init(SIMPL_MAX_IM_SESSIONS << 1,
            SIMPL_MAX_IM_SESSIONS << 1, sip_ptr->fileFlowControlDepth)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (SAPP_ERR);
    }

    MSRP_dbgSetLevel(MSRP_DGB_LOG_ALL);
    
    /* Get the event queue from MSRP and add it to the group */
    if (MSRP_RETURN_OK != MSRP_getEventQueue(&sip_ptr->queue.msrpEvt)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        MSRP_destroy();
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (SAPP_ERR);
    }

    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&sip_ptr->queue.group,
            sip_ptr->queue.msrpEvt)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);
        MSRP_destroy();
        XCAP_shutdown(&sip_ptr->xcap.xcap);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (SAPP_ERR);
    }


    /* Init the XCAP module */
    if (1 != XCAP_init(&sip_ptr->xcap.xcap,
            sip_ptr->xcap.timeout)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);        
        
        MSRP_destroy();
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (SAPP_ERR);
    }
    
    /* Wait for the XCAP event queue to initialize, but don't wait forever */
    /* no waiting in refactored init sequence
    x = 100;
    while (0 == sip_ptr->xcap.xcap.evtq && 0 < x) {
        OSAL_taskDelay(100);
        x--;
    }
    */
        
    /* Copy the xcap event queue ID */
    sip_ptr->queue.xcapEvt = sip_ptr->xcap.xcap.evtq;
    
    if (OSAL_SUCCESS != OSAL_msgQGrpAddQ(&sip_ptr->queue.group,
            sip_ptr->queue.xcapEvt)) {
        OSAL_msgQDelete(sip_ptr->queue.isiCmd);
        OSAL_msgQDelete(sip_ptr->queue.sipEvt);
        OSAL_msgQDelete(sip_ptr->queue.tmrEvt);
        OSAL_msgQDelete(sip_ptr->queue.isiEvt);
        MSRP_destroy();
        XCAP_shutdown(&sip_ptr->xcap.xcap);
        SIP_Destroy();
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__); 
        return (SAPP_ERR);
    }

    /* Gerenerate Certificate for MSRP/TLS */ 
    OSAL_memSet(&sip_ptr->cert, 0, sizeof(OSAL_NetSslCert));
    IMS_NET_SSL_CERT_GEN(&sip_ptr->cert,
            1024, /* Bit */
            0,    /* Serial number */
            365); /* Validate period */

    return (SAPP_OK);
}

/*
 * ======== _SAPP_simpleShutdown() ========
 * Shutdown the SIMPLE stack.
 *
 * Returns:
 * SAPP_OK  : Success
 * SAPP_ERR : Error
 */
static vint _SAPP_simpleShutdown(
    SAPP_SipObj *sip_ptr)
{
    /* 
     * Stop MSRP/XCAP
     */
    MSRP_destroy();
    XCAP_shutdown(&sip_ptr->xcap.xcap);
    /* Destroy certificate for MSRP/TLS */
    IMS_NET_SSL_CERT_DESTROY(&sip_ptr->cert);

    return (SAPP_OK);
}

#endif

static vint _SAPP_shutdown(
    SAPP_SipObj *sip_ptr)
{
    /* Delete tasks. */
    vint ret;

    ret = SAPP_OK;
    if (OSAL_SUCCESS != SAPP_destroy()) {
        ret = SAPP_ERR;
    }

    return (ret);
}

/* 
 * ======== _SAPP_getCapabilitiesSettings() ========
 * This function to get the capalibities setting from configuration
 * pointer cfg_ptr. And it converts the capabilites to bitmap and
 * return it.
 * capTag_ptr indicates the capabilities tag string, it could be registration
 * capabilities or exchange capabilities.
 *
 * Return Values:
 *  None
 */
static uint32 _SAPP_getCapabilitiesSettings(
    void       *cfg_ptr,
    const char *capTag_ptr)
{
    uint32 capsBitmap;
    char  *value_ptr;

    capsBitmap = 0;

    /* Get ip voice call */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_IP_VOICE_CALL))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_IP_VOICE_CALL;
        }
    } 

    /* Get ip video call */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_IP_VIDEO_CALL))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_IP_VIDEO_CALL;
        }
    } 

    /* Get sms over ip*/
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_SMS_OVER_IP))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_SMS;
        }
    } 

    /* Get chat */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_CHAT))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_CHAT;
        }
    } 

    /* Get discovery via presence */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr,
            SETTINGS_PARM_DISCOVERY_VIA_PRESENCE))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_DISCOVERY_VIA_PRESENCE;
        }
    } 

    /* Get messaging */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_MESSAGING))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_MESSAGING;
        }
    } 

    /* Get file transfer */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_FILE_TRANSFER))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_FILE_TRANSFER;
        }
    } 

    /* Get image share */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_IMAGE_SHARE))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_IMAGE_SHARE;
        }
    } 

    /* Get video share */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_VIDEO_SHARE))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_VIDEO_SHARE;
        }
    } 

    /* Get video share without call */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, 
            SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_VIDEO_SHARE_WITHOUT_CALL;
        }
    } 

    /* Get social presence */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_SOCIAL_PRESENCE))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_SOCIAL_PRESENCE;
        }
    } 

    /* Get geolocation push */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_GEOLOCATION_PUSH))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_GEOLOCATION_PUSH;
        }
    } 

    /* Get geolocation pull */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_GEOLOCATION_PULL))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_GEOLOCATION_PULL;
        }
    } 

    /* Get file transfer http */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_FILE_TRANSFER_HTTP))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_FILE_TRANSFER_HTTP;
        }
    }

    /* Get file transfer thumbnail */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, 
            SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_FILE_TRANSFER_THUMBNAIL;
        }
    }

    /* Get file transfer store and forward */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, 
            SETTINGS_PARM_FILE_TRANSFER_STORE_FWD))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_FILE_TRANSFER_STORE_FWD;
        }
    }

    /* Get RCS telephony capabilities. */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_RCS_TELEPHONY_CS))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_RCS_TELEPHONY_CS;
        }
    }

    /* Get RCS telephony capabilities. */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOLTE))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_RCS_TELEPHONY_VOLTE;
        }
    }

    /* Get RCS telephony capabilities. */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOHSPA))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_RCS_TELEPHONY_VOHSPA;
        }
    }

    return (capsBitmap);
}

/*
 * ======== _SAPP_getSrvccCapabilitiesSettings() ========
 * This function to get the SRVCC capalibities setting from configuration
 * pointer cfg_ptr. And it converts the capabilites to bitmap and
 * return it.
 * capTag_ptr indicates the capabilities tag string, it could be registration
 * capabilities or exchange capabilities.
 *
 * Return Values:
 *  None
 */
static uint32 _SAPP_getSrvccCapabilitiesSettings(
    void       *cfg_ptr,
    const char *capTag_ptr)
{
    uint32 capsBitmap;
    char  *value_ptr;

    capsBitmap = 0;

    /* SRVCC in alerting phase */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_SRVCC_ALERTING))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_SRVCC_ALERTING;
        }
    }

    /* SRVCC with mid-call feature */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, capTag_ptr, SETTINGS_PARM_SRVCC_MID_CALL))) {
        if ('1' == value_ptr[0]) {
            capsBitmap |= SAPP_CAPS_SRVCC_MID_CALL;
        }
    }

    return (capsBitmap);
}
/* 
 * ======== _SAPP_config() ========
 * It will initialize all needed system resources from settings
 *
 * Return Values:
 * SAPP_OK  SAPP is successfully running.
 * SAPP_ERR There was an error launchign the SAPP application.
 *
 */
static int _SAPP_config(
    void           *cfg_ptr,
    char           *hostName_ptr,
    SAPP_GlobalObj *global_ptr)
{
    char           *value_ptr;
    char           *ipc_ptr;
    char           *isiIpc_ptr;
    char           *mediaIpc_ptr;
    char           *streamIpc_ptr;
    char           *pos_ptr;
    vint            bytes;
    vint            len;
    
    /* Allocate a buffer for the "Supported header field" */
    bytes = OSAL_snprintf(global_ptr->sipObj.supported, SAPP_STRING_SZ,
            "%s,%s,%s,",
            SAPP_SUPPORTED_OPT_REPLACES,
            SAPP_SUPPORTED_OPT_100REL,
            SAPP_SUPPORTED_OPT_GRUU);
    len = SAPP_STRING_SZ - bytes;
    pos_ptr = global_ptr->sipObj.supported + bytes;

    /* Get the names of the ipc interfaces */
    ipc_ptr = NULL;
    isiIpc_ptr = NULL;
    mediaIpc_ptr = NULL;
    streamIpc_ptr = NULL;

    /* Get "this" */ 
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        ipc_ptr = value_ptr;
    }

    /* Get "isi" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_ISI))) {
        isiIpc_ptr = value_ptr;
    }

    /* Get "audio" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL,SETTINGS_PARM_AUDIO))) {
        mediaIpc_ptr = value_ptr;
    }

    /* Get "stream" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_STREAM))) {
        streamIpc_ptr = value_ptr;
    }

    if (NULL == ipc_ptr || NULL == isiIpc_ptr || NULL == mediaIpc_ptr ||
            NULL == streamIpc_ptr) {
        /* The IPC names are manditory */
        SAPP_dbgPrintf("%s:%d SAPP_ERR\n", __FUNCTION__, __LINE__);
        return (SAPP_ERR);
    }

    /* Set the host name for this SIP endpoint */
    OSAL_snprintf(global_ptr->sipObj.hostname, SAPP_STRING_SZ, "%s",
            hostName_ptr);

    /* Get the protocol ID to use. */
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            NULL, NULL, SETTINGS_ATTR_ID))) {
        global_ptr->protocolId = OSAL_atoi(value_ptr);
        if ((ISI_PROTOCOL_SIP != global_ptr->protocolId) &&
                (ISI_PROTOCOL_SIP_RCS != global_ptr->protocolId)) {
            SAPP_dbgPrintf("%s:%d Invalid protocol id %d\n",
                    __FUNCTION__, __LINE__, global_ptr->protocolId);
        }
    }

    /* Set the ring template number to use when ringing a phone */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RING_TEMPLATE))) {
        global_ptr->sipObj.ringTemplate = OSAL_atoi(value_ptr);
    }

    /* Get sip re-registration interval */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_EXPIRE_SEC))) {
        global_ptr->sipObj.keepAlive = OSAL_atoi(value_ptr);
    }

    /* Set mwi expire */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_MWI_EXPIRE_SEC))) {
        global_ptr->sipObj.mwiRefresh = OSAL_atoi(value_ptr);
    }

    /* 
     * Set the NAT mapping refresh rate. A.K.A. the rate to send dummy packets 
     * to refresh NAT mappings on a NAT enabled router.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_NAT_KEEP_ALIVE_SEC))) {
        global_ptr->sipObj.refresh = OSAL_atoi(value_ptr);
    }

    /* Set if keep-alive is enable, RFC6223. */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_KEEP_ALIVE_ENABLED))) {
        if (1 == OSAL_atoi(value_ptr)) {
            global_ptr->sipObj.keepAliveEnable = OSAL_TRUE;
        }
        else {
            global_ptr->sipObj.keepAliveEnable = OSAL_FALSE;
        }
    }

    /* Set RegRetryBaseTime and RegRetryMaxTime */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_RETRY_BASE_TIME))) {
        global_ptr->sipObj.regRetryBaseTime= OSAL_atoi(value_ptr);
    }
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_RETRY_MAX_TIME))) {
        global_ptr->sipObj.regRetryMaxTime = OSAL_atoi(value_ptr);
    }    

    /* Set MTU size */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_MTU))) {
        global_ptr->sipObj.mtu = OSAL_atoi(value_ptr);
    }

    /*
     * Set the session timer value.  If not '0' then session timers will be
     * enforced for all media calls.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_SESSION_TIMER))) {
        if ('0' != *value_ptr) {
            bytes = OSAL_snprintf(pos_ptr, len, "%s",
                    SAPP_SUPPORTED_OPT_TIMER);
            pos_ptr += bytes;
            len -= bytes;
            OSAL_snprintf(global_ptr->sipObj.sessionTimer,
                    SAPP_STRING_SZ, "%s", value_ptr);
        }
    }

    /*
     * Set the session timer value to force the MT side to enable session timer
     * no matter MO support it or not.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_FORCE_MT_SESSION_TIMER))) {
        if ('0' != *value_ptr) {
            /* Don't put "timer" again if timer is populated */
            if (0 == global_ptr->sipObj.sessionTimer[0]) {
                bytes = OSAL_snprintf(pos_ptr, len, "%s",
                        SAPP_SUPPORTED_OPT_TIMER);
                pos_ptr += bytes;
                len -= bytes;
            }
            OSAL_snprintf(global_ptr->sipObj.forceSessionTimer,
                    SAPP_STRING_SZ, "%s", value_ptr);
        }
    }

    /* 
     * Set the prack filed that indicates whether or not we should
     * force the use of prack
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_PRACK_ENABLED))) {
        global_ptr->sipObj.usePrack = OSAL_atoi(value_ptr);
    }

    /*
     * Set the cpim that indicates whether or not we use CPIM format when
     * encoding outbound page mode SMS.  If it's not set then plain/text is used.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_CPIM_ENABLED))) {
        global_ptr->sipObj.useCpim = OSAL_atoi(value_ptr);
    }

    /* 
     * Set the ipsec that indicates whether or not we use ipsec
     * and sec-agree.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_IPSEC_ENABLED))) {
        global_ptr->sipObj.useIpSec = OSAL_atoi(value_ptr);
    }

    /* 
     * Set the isim that indicates whether or not we have isim support.
     * Set this to 0 if there is no isim.
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_ISIM_ENABLED))) {
        global_ptr->sipObj.useIsim = OSAL_atoi(value_ptr);
    }

    /* Set whether or not to enable the "reg" event subscription package */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_REG_EVENT_ENABLED))) {
        global_ptr->sipObj.useRegEvt = OSAL_atoi(value_ptr);
    }

    /* Set whether or not to enable the "MWI" event subscription package */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_MWI_EVENT_ENABLED))) {
        global_ptr->sipObj.useMwiEvt = OSAL_atoi(value_ptr);
    }

    /* Set the default User Agent Name for the SIP endpoint */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_UA_NAME))) {
        OSAL_snprintf(global_ptr->sipObj.name, SAPP_STRING_SZ,
                "%s %s", value_ptr, D2_Release_VPORT);
    }

    /* Set the q that to prioritize addresses in a list of contact addresses */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_Q_VALUE))) {
        OSAL_snprintf(global_ptr->sipObj.qValue, SIP_Q_VALUE_STR_SIZE,
                "%s", value_ptr);
    }

    /*
     * Set the device's IMS stack's default capabilities.
     * These are used when registering to the network.
     * They represent the capabilities this device supports
     */
    global_ptr->sipObj.regCapabilitiesBitmap =
            _SAPP_getCapabilitiesSettings(cfg_ptr,
            SETTINGS_TAG_REG_CAPABILITIES);

    /* Set none if no RCS telephony feature tag set. */
    if (0 == (global_ptr->sipObj.regCapabilitiesBitmap &
            (SAPP_CAPS_RCS_TELEPHONY_VOHSPA |
            SAPP_CAPS_RCS_TELEPHONY_VOLTE |
            SAPP_CAPS_RCS_TELEPHONY_CS))) {
        global_ptr->sipObj.regCapabilitiesBitmap |=
                SAPP_CAPS_RCS_TELEPHONY_NONE;
    }

    /*
     * Set the User Agent's default capabilities for the SIP endpoint.
     * These may be updated dynamically by an application using
     * ISI_setCapabilities()
     */
    global_ptr->sipObj.exchangeCapabilitiesBitmap =
            _SAPP_getCapabilitiesSettings(cfg_ptr,
            SETTINGS_TAG_EX_CAPABILITIES);

    /*
     * Set the User Agent's SRVCC capabilities for the SIP endpoint.
     */
    global_ptr->sipObj.srvccCapabilitiesBitmap =
            _SAPP_getSrvccCapabilitiesSettings(cfg_ptr,
            SETTINGS_TAG_SRVCC_CAPABILITIES);

    /*
     * Get the User Agent's default capability discovery method.
     * Added to support RCS 5.0 specification.
     *
     * See SAPP_CapabilityDiscoveryType
     * 0 = no capability discovery
     * 1 = discovery via SIP OPTIONS method
     * 2 = discovery via PRESENCE method
     */
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_ATTR_CAP_DISCOVERY))) {
         global_ptr->sipObj.capDiscoveryMethod =
                 (SAPP_CapabilityDiscoveryType) OSAL_atoi(value_ptr);
    }
    else {
        global_ptr->sipObj.capDiscoveryMethod = 0;
    }
    switch (global_ptr->sipObj.capDiscoveryMethod) {
    case 0:
        SAPP_dbgPrintf("Default capability discovery method is DISABLED\n");
        break;
    case 1:
        SAPP_dbgPrintf("Default capability discovery method is via SIP OPTIONS method\n");
        break;
    case 2:
        SAPP_dbgPrintf("Default capability discovery method is via PRESENCE method\n");
        break;
    default:
        SAPP_dbgPrintf("Default capability discovery method is INVALID\n");
        break;
    }

    /*
     * Get the setting for Capability Discovery Via Common Stack, which
     * controls if SIP OPTIONS should be used when a query for capabilities
     * fails and the PRESENCE method was originally used.
     * Added to support RCS 5.0 specification.
     *
     * 0 = false, capability discovery via common stack disabled
     * 1 = true,  capability discovery via common stack enabled
     */
    if (NULL != (value_ptr = SETTINGS_getAttrValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_TWO, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, SETTINGS_TAG_EX_CAPABILITIES,
            SETTINGS_ATTR_COMMON_STACK))) {
        global_ptr->sipObj.capDiscoveryViaCommonStack = OSAL_atoi(value_ptr);
    }
    else {
        global_ptr->sipObj.capDiscoveryViaCommonStack = OSAL_TRUE;
    }
    switch (global_ptr->sipObj.capDiscoveryViaCommonStack) {
    case 0:
        SAPP_dbgPrintf("Capability discovery via common stack DISABLED\n");
        break;
    case 1:
        SAPP_dbgPrintf("Capability discovery via common stack ENABLED\n");
        break;
    default:
        SAPP_dbgPrintf("Capability discovery via common stack INVALID\n");
        break;
    }

    /* Set the precondition */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIP, NULL, SETTINGS_PARM_PRECONDITION_ENABLED))) {
        if ('0' != *value_ptr) {
            bytes = OSAL_snprintf(pos_ptr, len, "%s",
                    SAPP_SUPPORTED_OPT_PRECONDITION);
            pos_ptr += bytes;
            len -= bytes;
            global_ptr->sipObj.usePrecondition = OSAL_atoi(value_ptr);
        }
    }

    /* Set the presence refresh rate.  the interal at which we send PUBLISH */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_PRESENCE_EXPIRE_SEC))) {
        global_ptr->sipObj.presence.keepAlive = OSAL_atoi(value_ptr);
    }

    /* Get file path */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_FILE_PATH))) {
        OSAL_snprintf(global_ptr->sipObj.filePath, SAPP_FILE_PATH_SZ, "%s", 
                value_ptr);
    }

    /* Get file prepend */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_FILE_PREPEND))) {
        OSAL_snprintf(global_ptr->sipObj.filePrepend, SAPP_STRING_SZ, "%s", 
                value_ptr);
    }

    /* Get file flow control depth */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_SIMPLE, NULL, SETTINGS_PARM_FILE_FLOW_CTRL_DEPTH))) {
        global_ptr->sipObj.fileFlowControlDepth = OSAL_atoi(value_ptr);
    }

    /* Set the default 'vdn' for the wiFi SIP endpoint */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_HANDOFF, NULL, SETTINGS_PARM_VDN))) {
        OSAL_snprintf(global_ptr->sipObj.vdn, SAPP_STRING_SZ, "%s", value_ptr);
    }

#ifdef INCLUDE_SIMPLE    
    /* Set the xcap configuration */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_XCAP, NULL, SETTINGS_PARM_BLACK_LIST))) {
        OSAL_snprintf(global_ptr->sipObj.xcap.blacklistName, SIMPL_STRING_SZ,
                "%s", value_ptr);
    }

    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_XCAP, NULL, SETTINGS_PARM_WHITE_LIST))) {
        OSAL_snprintf(global_ptr->sipObj.xcap.whitelistName, SIMPL_STRING_SZ,
                "%s", value_ptr);
    }

    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_SAPP,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_PROTOCOL,
            SETTINGS_TAG_XCAP, NULL, SETTINGS_PARM_TIMEOUT))) {
        global_ptr->sipObj.xcap.timeout = OSAL_atoi(value_ptr);
    }
#endif

    /* SIP module init sequence is not refactored, so invokes special init here */
    /* init underlining SIP stack */
    if (SAPP_OK != _SAPP_sipInit(&global_ptr->sipObj)) {
        SAPP_dbgPrintf("%s: Error initializing protocol\n", __FUNCTION__);
    }

    /* all configurations gathered, start allocating system resources */
    _SAPP_allocateQueues(&global_ptr->sipObj, ipc_ptr, isiIpc_ptr, mediaIpc_ptr);

#ifdef INCLUDE_SIMPLE
    _SAPP_simpleInit(&global_ptr->sipObj);
#endif

    
    /*
     * Get and set sip timer sip settings
     * To reuse _SAPP_getSipTimerSettings(), get the "protocol" tag
     * and pass to the routine.
     */
    /* Get and set sip timer sip settings */
    _SAPP_getSipTimerSettings(cfg_ptr);

    /* Get transport protocol of im */
    _SAPP_getImTransportProto(&global_ptr->sipObj, cfg_ptr);

    /* 
     * Init the timer event object. Set the IPC name of where to send events
     * i.e. ISI and also write the actual message to send.  We only do this 
     * once so we don't have to re-write the event everytime we send it.
     */
    global_ptr->sipObj.tmr.event.isiEvt = global_ptr->sipObj.event.isiEvt;

    /*
     * Construct the ISI event that the timer will send.
     * First START event will have INVALID status as initial values
     */
    _SAPP_systemIsiEvt(ISIP_SYSTEM_REASON_START, 
            ISIP_STATUS_INVALID,
            ipc_ptr, mediaIpc_ptr, streamIpc_ptr,
            &global_ptr->sipObj.tmr.event.isiMsg);

    /* Launch the timer used to register with ISI */
    if (SAPP_OK != _SAPP_timerInit(&global_ptr->sipObj)) {
        /* Failed to init timer */
        _SAPP_shutdown(&global_ptr->sipObj);
        return (SAPP_ERR);
    }
    /* Launch the watch dog looking for sleep */
    if (SAPP_OK != _SAPP_sleepWatchDogInit(&global_ptr->sipObj)) {
        /* Failed to init timer */
        _SAPP_shutdown(&global_ptr->sipObj);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== _SAPP_init() ========
 * This function is the first function to be called.  It will initialize
 * all needed system resources and begin servicing commands from ISI and 
 * events from the GSM module
 *
 * Return Values:
 * SAPP_OK  SAPP is successfully running.
 * SAPP_ERR There was an error launchign the SAPP application.
 *
 */
static int _SAPP_init(
    void           *cfg_ptr,
    char           *hostName_ptr,
    SAPP_GlobalObj *global_ptr)
{
    /* init sapp and sip stack */
    _SAPP_config(cfg_ptr, hostName_ptr, global_ptr);
    
    if (OSAL_FAIL == SAPP_start()) {  
#ifdef INCLUDE_SIMPLE
        _SAPP_simpleShutdown(&global_ptr->sipObj);
#endif
        SIP_Destroy();
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/* 
 * ======== SAPP_allocate() ========
 * Public routine for allocating the SAPP module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint SAPP_allocate()
{
    void   *cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_SAPP);
    char   *hostName_ptr = "127.0.0.1";
    vint    idx;

#if 1    
    if (OSAL_SUCCESS != SR_allocate()) {
        OSAL_logMsg("Failed to allocate SR!\n");
        return (OSAL_FAIL);
    }
#endif    
    /* Init the global object that's used to manage this process */
    _SAPP_globalObj_ptr = OSAL_memCalloc(1, sizeof(SAPP_GlobalObj), 0);

    /* Set socket id as invalid */
    for (idx = 0; idx < SAPP_SIP_MAX_UA; idx++) {
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcTcpServerFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcTcpClientFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedServerFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedClientFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedTcpServerFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedTcpClientFd =
                OSAL_NET_SOCK_INVALID_ID;
    }

    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_SAPP,
            NULL, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);
        OSAL_logMsg("%s: Could not find the default SAPP init file\n", __FUNCTION__);
        return (OSAL_FAIL);
    }
    _SAPP_config(cfg_ptr, hostName_ptr, _SAPP_globalObj_ptr);
    SETTINGS_memFreeDoc(SETTINGS_TYPE_SAPP, cfg_ptr);

    return (OSAL_SUCCESS);
}


/*
 * ======== SAPP_start() ========
 *
 * Public routine for starting the SAPP  module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint SAPP_start(void)
{
    SAPP_SipObj   *sip_ptr = &_SAPP_globalObj_ptr->sipObj;
    SAPP_TaskObj  *task_ptr;
    if (OSAL_SUCCESS != SR_start()) {
        OSAL_logMsg("Failed to start SR!\n");
        return (OSAL_FAIL);
    }
   /* SIP UA handler task. */
    task_ptr = &sip_ptr->task;
    task_ptr->tId      = 0;      
    task_ptr->stackSz  = SAPP_TASK_STACK_BYTES;
    task_ptr->pri      = OSAL_TASK_PRIO_NIC;
    task_ptr->func_ptr = _SAPP_sipUaHandlerTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            SAPP_SIP_TASK_NAME);
    task_ptr->arg_ptr  = (void *)sip_ptr;
    if (0 == (task_ptr->tId = OSAL_taskCreate(task_ptr->name,
            (OSAL_TaskPrio)task_ptr->pri,
            task_ptr->stackSz,
            (OSAL_TaskPtr)task_ptr->func_ptr,
            (void *)task_ptr->arg_ptr))) {

        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}


/*
 * ======== SAPP_destroy() ========
 * Destroy the SAPP_sip module
 *
 * Return Values:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 *
 */
vint SAPP_destroy(void)
{
    SAPP_SipObj *sip_ptr = &_SAPP_globalObj_ptr->sipObj;
    _SAPP_stop(sip_ptr);
    _SAPP_deallocate(sip_ptr);

#ifdef INCLUDE_SIMPLE
    _SAPP_simpleShutdown(sip_ptr);
#endif
    SIP_Destroy();
        
    return (OSAL_SUCCESS);
}

/* 
 * ======== SAPP_init() ========
 * This function is to initialize SAPP.
 *
 * Return Values:
 * SAPP_OK  SAPP is successfully initialized
 * SAPP_ERR There was an error initializing SAPP
 */
int SAPP_init(void *cfg_ptr, char *hostname_ptr)
{
    vint    idx;

    /* Initialize SR */
    if (OSAL_SUCCESS != SR_init()) {
        OSAL_logMsg("%s:%d SR init failed.\n",  __FUNCTION__, __LINE__);
        return (-1);
    }

    if (NULL == hostname_ptr || 0 == hostname_ptr[0]) {
        /* Let's set a default */
        hostname_ptr = "127.0.0.1";
    }
    OSAL_logMsg("%s:%d host=%s\n", __FUNCTION__, __LINE__, hostname_ptr);

    /* Init the global object that's used to manage this process */
    _SAPP_globalObj_ptr = OSAL_memCalloc(1, sizeof(SAPP_GlobalObj), 0);

    /* Set socket id as invalid */
    for (idx = 0; idx < SAPP_SIP_MAX_UA; idx++) {
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcTcpServerFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcTcpClientFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedServerFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedClientFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedTcpServerFd =
                OSAL_NET_SOCK_INVALID_ID;
        _SAPP_globalObj_ptr->sipObj.service[idx].sipInfcProtectedTcpClientFd =
                OSAL_NET_SOCK_INVALID_ID;
    }

    if (SAPP_OK != _SAPP_init(cfg_ptr, hostname_ptr, _SAPP_globalObj_ptr)) {
        OSAL_logMsg("Failed to Init the SAPP Application\n");
        /* free queues and memory */
        _SAPP_deallocate(&_SAPP_globalObj_ptr->sipObj);
        return (-1);
    }

    return (0);
}

/* 
 * ======== SAPP_shutdown() ========
 * This function is to shutdown SAPP.
 *
 * Return Values:
 * NONE
 */
void SAPP_shutdown(void)
{
    SAPP_SipObj *sip_ptr = &_SAPP_globalObj_ptr->sipObj;

    /* Tell ISI that we are going down */
    _SAPP_systemIsiEvt(ISIP_SYSTEM_REASON_SHUTDOWN, 
            ISIP_STATUS_DONE,
            NULL, NULL, NULL, &sip_ptr->event.isiMsg);

    SAPP_sendEvent(&sip_ptr->event);

    /* Clean house and exit */
    _SAPP_shutdown(sip_ptr);
    /* Now kill the timer used for registration */
    _SAPP_timerDestroy(sip_ptr);
    /* Destroy services just in case it's not destroyed before */
    SAPP_sipServiceDestroyAll(sip_ptr);
    /* Shutdown SR */
    SR_shutdown();
}

