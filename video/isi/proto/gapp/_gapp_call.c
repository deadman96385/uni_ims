/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30028 $ $Date: 2014-11-21 19:05:32 +0800 (Fri, 21 Nov 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>
#include <osal_log.h>

#include "isi.h"
#include "isip.h"

#include <csm_event.h>
#ifndef GAPP_DISABLE_GSM
#include "gsm.h"
#endif
#include "_gapp.h"
#include "_gapp_call.h"

/*
 * ======== GAPP_setCallBit() ========
 * This function is used set the 'active' and 'hold' bits used to track
 * if a call state is active and onhold.
 *
 * Return Values:
 * Nothing.
 */
void GAPP_setCallBit(
    uint32      *bits_ptr,
    vint         idx,
    OSAL_Boolean set)
{
    uint32 mask;
    uint32 bits;
    bits = *bits_ptr;
    mask = (1 << idx);
    if (OSAL_TRUE == set) {
        /* Then or it in. */
        bits |= mask;
    }
    else {
        bits &= (~mask);
    }
    *bits_ptr = bits;
    return;
}

/*
 * ======== GAPP_callIsiEvt() ========
 * This function is used by various other functions to populate a ISI event
 * related to calls. These events will be passed from GAPP to the ISI module.
 *
 * Return Values:
 * Nothing.
 */
void GAPP_callIsiEvt(
    ISI_Id              serviceId,
    ISI_Id              callId,
    vint                protocolId,
    ISIP_CallReason     reason,
    ISIP_Status         status,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = callId;
    isi_ptr->code = ISIP_CODE_CALL;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.call.reason = reason;
    isi_ptr->msg.call.status = status;
    isi_ptr->msg.call.serviceId = serviceId;
    /* Always set resource ready in GAPP */
    isi_ptr->msg.call.rsrcStatus = ISI_RESOURCE_STATUS_LOCAL_READY |
            ISI_RESOURCE_STATUS_REMOTE_READY;
    /* For GSM, no media provided, use audio as default. */
    if (0 == isi_ptr->msg.call.type) {
        isi_ptr->msg.call.type = ISI_SESSION_TYPE_AUDIO;
    }
    return;
}

/*
 * ======== GAPP_callVdxIsiEvt() ========
 * This function is used by various other functions to populate a ISI
 * event for reporting to ISI a change to the VDI/VDN routing numbers.
 * VDI and VDN is terminology used in the VCC world representing the
 * routing numbers to use if a call needs to be handed off.
 *
 * Return Values:
 * Nothing.
 */
void GAPP_callVdxIsiEvt(
    ISI_Id              serviceId,
    ISI_Id              callId,
    int                 protocolId,
    char               *target_ptr,
    ISIP_Message       *isi_ptr)
{
    GAPP_callIsiEvt(serviceId, callId, protocolId, ISIP_CALL_REASON_VDX,
        ISIP_STATUS_INVALID, isi_ptr);

    /* Copy in the target VDN/VDI to report to ISI */
    OSAL_snprintf(isi_ptr->msg.call.to, ISI_ADDRESS_STRING_SZ,
            "%s", target_ptr);
    return;
}
#ifndef GAPP_DISABLE_GSM

/*
 * ======== _GAPP_isCallBitSet() ========
 * This function is used determine if an 'active' or 'hold' bit is set for
 * a particular call.
 *
 * Return Values:
 * OSAL_TRUE : The bit is set at the index specified in 'idx'.
 * OSAL_FALSE: The bit is NOT set at the index specified in 'idx'.
 */
static OSAL_Boolean _GAPP_isCallBitSet(
    uint32      *bits_ptr,
    vint         idx)
{
    uint32 mask;

    mask = (1 << idx);
    if ((*bits_ptr) & mask) {
        return (OSAL_TRUE);
    }
    return (OSAL_FALSE);
}

static void _GAPP_setHoldState(
    GAPP_ServiceObj *service_ptr,
    uint32           active,
    OSAL_Boolean     onHold,
    GAPP_Event      *evt_ptr)
{
    vint          x;
    uint32       *hold_ptr;
    GAPP_CallObj *call_ptr;

    hold_ptr = &service_ptr->callBits.hold;

    if (OSAL_TRUE == onHold) {
        /* Then switch all calls to onhold */
        for (x = 0; x < GAPP_CALL_NUM; x++) {
            /* If calls are active and not on hold then switch to 'on hold' */
            call_ptr = &service_ptr->aCall[x];
            if (OSAL_TRUE == _GAPP_isCallBitSet(&active, call_ptr->idx) &&
                    OSAL_FALSE == _GAPP_isCallBitSet(hold_ptr, call_ptr->idx)) {
                GAPP_setCallBit(hold_ptr, call_ptr->idx, OSAL_TRUE);
                GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                        service_ptr->protocolId, ISIP_CALL_REASON_HOLD,
                        ISIP_STATUS_DONE, &evt_ptr->isiMsg);
                /* GAPP_sendEvent(evt_ptr); */
            }
        }
    }
    else {
        /* Then 'resume' all calls */
        for (x = 0; x < GAPP_CALL_NUM; x++) {
            /* If calls are active and on hold then take them off hold */
            call_ptr = &service_ptr->aCall[x];
            if (OSAL_TRUE == _GAPP_isCallBitSet(&active, call_ptr->idx) &&
                    OSAL_TRUE == _GAPP_isCallBitSet(hold_ptr, call_ptr->idx)) {
                GAPP_setCallBit(hold_ptr, call_ptr->idx, OSAL_FALSE);
                GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                        service_ptr->protocolId, ISIP_CALL_REASON_RESUME,
                        ISIP_STATUS_DONE, &evt_ptr->isiMsg);
                /* GAPP_sendEvent(evt_ptr); */
            }
        }
    }

    /*
     * Since we wrote event to the ISI.  Let's init the ISI event obj
     * for the next user.
     */
    evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
    return;
}

/*
 * ======== _GAPP_setConfState() ========
 * This function set up/clear all active call's bit mask.
 *
 * Return Values:
 * none
 */
static void _GAPP_setConfState(
    GAPP_ServiceObj *service_ptr,
    uint32           active,
    OSAL_Boolean     set)
{
    vint          x;
    uint32       *conf_ptr;
    GAPP_CallObj *call_ptr;

    conf_ptr = &service_ptr->callBits.conf;

    if (OSAL_TRUE == set) {
        /* Set up all active calls' conf bit mask*/
        for (x = 0; x < GAPP_CALL_NUM; x++) {
            call_ptr = &service_ptr->aCall[x];
            if (OSAL_TRUE == _GAPP_isCallBitSet(&active, call_ptr->idx) &&
                    OSAL_FALSE == _GAPP_isCallBitSet(conf_ptr, call_ptr->idx)) {
                GAPP_setCallBit(conf_ptr, call_ptr->idx, OSAL_TRUE);
            }
        }
    }
    else {
        /* Clear all active calls' conf mask */
        for (x = 0; x < GAPP_CALL_NUM; x++) {
            call_ptr = &service_ptr->aCall[x];
            if (OSAL_TRUE == _GAPP_isCallBitSet(&active, call_ptr->idx) &&
                    OSAL_TRUE == _GAPP_isCallBitSet(conf_ptr, call_ptr->idx)) {
                GAPP_setCallBit(conf_ptr, call_ptr->idx, OSAL_FALSE);
            }
        }
    }

    return;
}

/*
 * ======== _GAPP_callModifyAnswerCmd() ========
 * This function is to send AT+CCMMD to accept or reject the media type
 * of a call.
 *
 * Return Values:
 * none
 */
static void _GAPP_callModifyAnswerCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr,
    ISIP_Call       *c_ptr)
{
    if (GSM_MEDIA_NEG_STATUS_PROPOSE != call_ptr->negStatus) {
        /* Remote is not proposing media, do nothing. */
        return;
    }
    /*
     * Compare session type to know is accept or reject.
     */
    if ((call_ptr->sessionType != c_ptr->type)) {
        /* Type is different, reject proposal. */
        GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                call_ptr->idx, GSM_MEDIA_NEG_STATUS_REJECT, NULL);
    }
    else {
        GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                call_ptr->idx, GSM_MEDIA_NEG_STATUS_ACCEPT, NULL);
    }
    /* Update session type. */
    call_ptr->sessionType = c_ptr->type;
    call_ptr->negStatus = GSM_MEDIA_NEG_STATUS_NONE;
}

/*
 * ======== _GAPP_callModifyCmd() ========
 * This function is to send AT+CCMMD to modify media type of a call.
 *
 * Return Values:
 * none
 */
static void _GAPP_callModifyCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr,
    ISIP_Call       *c_ptr)
{

    /*
     * Compare session type to know is upgrade or downgrade.
     */
    if (!(ISI_SESSION_TYPE_VIDEO & call_ptr->sessionType) &&
            (ISI_SESSION_TYPE_VIDEO & c_ptr->type)) {
        /* 
         * Current call doesn't have video but user want to add video.
         * Propose to have video media 
         */
        GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                call_ptr->idx, GSM_MEDIA_NEG_STATUS_PROPOSE,
                GSM_MEDIA_PROFILE_STR_AUDIO_VIDEO);
    }
    else if ((ISI_SESSION_TYPE_VIDEO & call_ptr->sessionType) &&
            !(ISI_SESSION_TYPE_VIDEO & c_ptr->type)) {
        /* 
         * Current call has video but user want to remove it.
         * Set to audio.
         */
        GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                call_ptr->idx, GSM_MEDIA_NEG_STATUS_UNCONDITIONAL,
                GSM_MEDIA_PROFILE_STR_AUDIO);
        call_ptr->sessionType = ISI_SESSION_TYPE_AUDIO;
    }

    /* Set modify bit. */
    GAPP_setCallBit(&service_ptr->callBits.modify, call_ptr->idx, OSAL_TRUE);

}

/*
 * ======== _GAPP_callAnswerCmd() ========
 * This function determines the appropriate AT command to use to answer an
 * incoming call. The appropriate command is largely in part due to whether
 * or not there are other active calls.
 *
 * Return Values:
 * none
 */
static void _GAPP_callAnswerCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr,
    ISIP_Call       *c_ptr)
{ 
    uint32 active;
    uint32 hold;

    /* See if remote proposed media. */
    if (GSM_MEDIA_NEG_STATUS_PROPOSE == call_ptr->negStatus) {
        /*
         * Reject media description if session type is different from
         * remote proposed.
         */
        if (call_ptr->sessionType != c_ptr->type) {
            /* Reject media. */
            GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                    call_ptr->idx, GSM_MEDIA_NEG_STATUS_REJECT, NULL);
            /* Set media to aduio */
            GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                    call_ptr->idx, GSM_MEDIA_NEG_STATUS_UNCONDITIONAL,
                    GSM_MEDIA_PROFILE_STR_AUDIO);
        }
        else {
            /* Accept media */
            GSM_cmdMediaControl(&service_ptr->daemon, &call_ptr->gsmId,
                    call_ptr->idx, GSM_MEDIA_NEG_STATUS_ACCEPT, NULL);
        }
    }
    call_ptr->negStatus = GSM_MEDIA_NEG_STATUS_NONE;

    /* Update session type */
    call_ptr->sessionType = c_ptr->type;
    active = service_ptr->callBits.active;
    hold = service_ptr->callBits.hold;
    /* Remove this call from the active and hold bits */
    GAPP_setCallBit(&active, call_ptr->idx, OSAL_FALSE);
    GAPP_setCallBit(&hold, call_ptr->idx, OSAL_FALSE);

    /*
     * Send the event of ISIP_CALL_REASON_ACCEPT_ACK reason to ISI or it won't 
     * change the call state to ISI_CALL_STATE_ACTIVE.
     */
    GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
            service_ptr->protocolId, ISIP_CALL_REASON_ACCEPT_ACK,
            ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
    GAPP_sendEvent(evt_ptr);
    /* Do not send this event again */
    evt_ptr->isiMsg.code = ISIP_CODE_INVALID;

    /*
     *  Bug 7106 : Place all currently active calls on hold 
     *             iner loop will check onHold status
     */
    if (0 != active) { // && 0 == hold) {
        /*
         * There are other calls but none others on hold.
         * Place all currently active calls on hold and answer this one
        */
        _GAPP_setHoldState(service_ptr, active, OSAL_TRUE, evt_ptr);
        GSM_cmdCallHoldTwo(&service_ptr->daemon, &call_ptr->gsmId);
        return;
    }
    GSM_cmdCallAnswer(&service_ptr->daemon, &call_ptr->gsmId);
    
}

/*
 * ======== _GAPP_getHoldCmd() ========
 * This function determines the appropriate AT command to use to place a
 * a call on hold.
 *
 * Return Values:
 * none
 */
static void _GAPP_callHoldAnswerCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr)
{
    uint32           conf; 
    if (OSAL_TRUE == _GAPP_isCallBitSet(&service_ptr->callBits.hold,
            call_ptr->idx)) {
        /* Then we are already in that state, this is a no-op */
        call_ptr->state = GAPP_CALL_STATE_ACTIVE;
        GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                service_ptr->protocolId, ISIP_CALL_REASON_HOLD,
                ISIP_STATUS_DONE, &evt_ptr->isiMsg);
        return;
    }

    /*
     * If there are other calls ringing then this is a no-op.  This is because
     * an ISI user may try to place this call on hold immediately before
     * answering an incoming call.  If that's the case then this call
     * will automatically be placed on hold when they answer, so no need
     * to do it here.
     */
    if (0 != service_ptr->callBits.ring) {
        /* This is a no-op */
        call_ptr->state = GAPP_CALL_STATE_ACTIVE;
        GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                service_ptr->protocolId, ISIP_CALL_REASON_HOLD,
                ISIP_STATUS_DONE, &evt_ptr->isiMsg);
        return;
    }

    /* 
     * Since we are going on hold.
     * Then everything else is taken off hold.
     */
    _GAPP_setHoldState(service_ptr, service_ptr->callBits.active, OSAL_FALSE,
            evt_ptr);
    if (OSAL_TRUE == _GAPP_isCallBitSet(&service_ptr->callBits.conf,
        call_ptr->idx)) {
        conf = service_ptr->callBits.conf;
        /* Clear this call from a conf bit mask. */
        GAPP_setCallBit(&conf, call_ptr->idx, OSAL_FALSE);
        /* Place all the other calls in conference on hold */
        _GAPP_setHoldState(service_ptr, conf, OSAL_TRUE, evt_ptr);
        /* Set up this call's conf bit mask. */
        GAPP_setCallBit(&conf, call_ptr->idx, OSAL_TRUE);
    }

    /* Now change the this call to on hold */
    GAPP_setCallBit(&service_ptr->callBits.hold, call_ptr->idx, OSAL_TRUE);
    GSM_cmdCallHoldTwo(&service_ptr->daemon, &call_ptr->gsmId);
   
}

/*
 * ======== _GAPP_getResumeCmd() ========
 * This function determines the appropriate AT command to use to take a
 * call off hold.
 *
 * Return Values:
 * none
 */
static void _GAPP_callResumeCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    GAPP_Event      *evt_ptr)
{
    uint32           conf;

    if (OSAL_FALSE == _GAPP_isCallBitSet(&service_ptr->callBits.hold,
            call_ptr->idx)) {
        /* Then we are already in that state, this is a no-op */
        call_ptr->state = GAPP_CALL_STATE_ACTIVE;
        GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                service_ptr->protocolId, ISIP_CALL_REASON_RESUME,
                ISIP_STATUS_DONE, &evt_ptr->isiMsg);
        return;
    }

    /*
     * Since we are going off hold.
     * Then everything else is placed on hold.
     */
    _GAPP_setHoldState(service_ptr, service_ptr->callBits.active, OSAL_TRUE,
            evt_ptr);

    if (OSAL_TRUE == _GAPP_isCallBitSet(&service_ptr->callBits.conf,
        call_ptr->idx)) {
        conf = service_ptr->callBits.conf;
        /* Clear this call's conf bit mask. */
        GAPP_setCallBit(&conf, call_ptr->idx, OSAL_FALSE);
        /* Place all the other calls off hold */
        _GAPP_setHoldState(service_ptr, conf, OSAL_FALSE, evt_ptr);
        /* Set up this call's conf bit mask. */
        GAPP_setCallBit(&conf, call_ptr->idx, OSAL_TRUE);
    }

    /* Take this call off hold */
    GAPP_setCallBit(&service_ptr->callBits.hold, call_ptr->idx, OSAL_FALSE);
    GSM_cmdCallHoldTwo(&service_ptr->daemon, &call_ptr->gsmId);
    return;
}

/*
 * ======== _GAPP_getDisconnectCmd() ========
 * This function determines the appropriate AT command to use to disconnect a
 * call (a.k.a. hang up). The appropriate command is largely in part due to
 * whether or not there are other active calls.
 *
 * Return Values:
 * none
 */
static void _GAPP_callDisconnectCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr)
{
    uint32 active;
    uint32 hold;
    uint32 ring;

    active = service_ptr->callBits.active;
    hold = service_ptr->callBits.hold;
    ring = service_ptr->callBits.ring;

    /* Remove this call from the active bits */
    GAPP_setCallBit(&active, call_ptr->idx, OSAL_FALSE);
    /* Remove this call from the conf bits */
    GAPP_setCallBit(&service_ptr->callBits.conf, call_ptr->idx, OSAL_FALSE);    

    if (0 == active) {
        /* Then we are the only active call. */
        GSM_cmdHangup(&service_ptr->daemon, &call_ptr->gsmId);
        return;
    }

    if (OSAL_TRUE == _GAPP_isCallBitSet(&hold, call_ptr->idx) ||
            OSAL_TRUE == _GAPP_isCallBitSet(&ring, call_ptr->idx))  {
        /*
         * If we are on hold or ringing AND there are other active calls then...
         */
        GSM_cmdCallHoldZero(&service_ptr->daemon, &call_ptr->gsmId);
        return;
    }

    /*
     * If we are here then there are other calls and we are
     * 'active' (not holding or ringing). Drop this call only.
     */
    GSM_cmdCallHoldOneIndex(&service_ptr->daemon, &call_ptr->gsmId,
            call_ptr->idx);
}

/*
 * ======== _GAPP_getRejectCmd() ========
 * This function determines the appropriate AT command to use to reject an
 * incoming call. The appropriate command is largely in part due to
 * whether or not there are other active calls.
 *
 * Return Values:
 * none
 */
static void _GAPP_callRejectCmd(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr)
{
    uint32 active;

    active = service_ptr->callBits.active;
    GAPP_setCallBit(&active, call_ptr->idx, OSAL_FALSE);
    
    /* 
     * Android requires the use of the UDUB (User Determined User Busy)
     * command to 'reject'. If UDUB does not work in some cases then 
     * comment out the GSM_callReject() command and uncomment the code below.
     */
#if 0
    if (0 != active) {
        /* Then there are other active calls */
        GSM_cmdCallHoldZero(&service_ptr->daemon, &call_ptr->gsmId);
        return;
    }
    GSM_cmdHangup(&service_ptr->daemon, &call_ptr->gsmId);
#else
    GSM_cmdReject(&service_ptr->daemon, &call_ptr->gsmId);
#endif
}
#endif

/*
 * ======== GAPP_getCallByIdx() ========
 * This function searches for a call with a matching call index
 * within a service object.
 *
 * Return Values:
 * GAPP_CallObj*: A pointer to the call object.
 * NULL:  No calls with the call index exist.
 */
GAPP_CallObj* GAPP_getCallByIdx(
    GAPP_ServiceObj *service_ptr,
    vint             idx)
{
    vint x;
    for (x = 0; x < GAPP_CALL_NUM; x++) {
        if (idx == service_ptr->aCall[x].idx) {
            return &service_ptr->aCall[x];
        }
    }
    return (NULL);
}

/*
 * ======== GAPP_getCallByIsiId() ========
 * This function searches for a call with a matching ISI_Id (specified by "id")
 * within a service object.
 *
 * Return Values:
 * GAPP_CallObj*: A pointer to the call object.
 * NULL:  No calls with the ISI_Id of "id" exist.
 */
GAPP_CallObj* GAPP_getCallByIsiId(
    GAPP_ServiceObj *service_ptr,
    ISI_Id           id)
{
    vint x;
    for (x = 0; x < GAPP_CALL_NUM; x++) {
        if (id == service_ptr->aCall[x].isiCallId) {
            return &service_ptr->aCall[x];
        }
    }
    return (NULL);
}

/*
 * ======== GAPP_getCallIsVcc() ========
 * This function searches for a call whick is in VCC progress
 * within a service object.
 *
 * Return Values:
 * GAPP_CallObj*: A pointer to the call object.
 * NULL:  No calls is in VCC progress.
 */
static GAPP_CallObj* GAPP_getCallIsVcc(
    GAPP_ServiceObj *service_ptr)
{
    vint x;
    for (x = 0; x < GAPP_CALL_NUM; x++) {
        if (1 == service_ptr->aCall[x].isVcc) {
            return &service_ptr->aCall[x];
        }
    }
    return (NULL);
}

/*
 * ======== GAPP_getNumberIsiCalls() ========
 * This function searches for the number of isi calls with non-zero isi id.
 *
 * Return Value:
 * number of isi calls
 */
vint GAPP_getNumberIsiCalls(
    GAPP_ServiceObj *service_ptr)
{
    vint x, cnt;
    cnt = 0;
    for (x = 0; x < GAPP_CALL_NUM; x++) {
        if (service_ptr->aCall[x].isiCallId != 0) {
            ++cnt;
        }
    }
    return (cnt);
}

/*
 * ======== GAPP_getCallByGsmId() ========
 * This function searches for a call with a matching GSM_Id (specified by "id")
 * within a service object.
 *
 * Return Values:
 * GAPP_CallObj*: A pointer to the call object.
 * NULL:  No calls with the GSM_Id of "id" exist.
 */
GAPP_CallObj* GAPP_getCallByGsmId(
    GAPP_ServiceObj *service_ptr,
    GSM_Id           id)
{
    vint x;
    for (x = 0; x < GAPP_CALL_NUM; x++) {
        if (id == service_ptr->aCall[x].gsmId) {
            return &service_ptr->aCall[x];
        }
    }
    return (NULL);
}
#ifndef GAPP_DISABLE_GSM

/*
 * ======== _GAPP_callGetGsmExtEvent() ========
 * This function checks if a call specified by call_ptr is currently active
 * and what state the GSM module is reporting it's in.  The GSM reports this in
 * +CLCCS reports which include a list of all Current Calls along with an
 * enumerated value that specifies the calls current state. GAPP uses these
 * periodic +CLCCS reports and the state as an "event" that drives the call
 * state machine.
 *
 * Return Values:
 *  An enuerated values that represents the state of the call.  Possible values
 *  are below. See the GSM AT command spec for the +CLCCS command for more
 *  details:
 *    GAPP_CALL_EVT_EXT_IDLE = 1
 *    GAPP_CALL_EVT_EXT_CALLING_MO,
 *    GAPP_CALL_EVT_EXT_CONNECTING_MO,
 *    GAPP_CALL_EVT_EXT_ALERTING_MO,
 *    GAPP_CALL_EVT_EXT_ALERTING_MT,
 *    GAPP_CALL_EVT_EXT_ACTIVE,
 *    GAPP_CALL_EVT_EXT_RELEASED_MO,
 *    GAPP_CALL_EVT_EXT_RELEASED_MT,
 *    GAPP_CALL_EVT_EXT_USER_BUSY,
 *    GAPP_CALL_EVT_EXT_USER_DTMD_USER_BUSY,
 *    GAPP_CALL_EVT_EXT_CALL_WAITING_MO,
 *    GAPP_CALL_EVT_EXT_CALL_WAITING_MT,
 *    GAPP_CALL_EVT_EXT_CALL_HOLD_MO,
 *    GAPP_CALL_EVT_EXT_CALL_HOLD_MT,
 */
static vint _GAPP_callGetGsmExtEvent(
    vint         *callIdx_ptr,
    GAPP_Token   *token_ptr,
    vint         *event_ptr,
    char         *uri_ptr,
    uint32        uriLen,
    vint         *negStatus_ptr,
    char         *sdpMd_ptr)
{
    GAPP_Buffer buff;
    vint        idx;
    vint        event;
    uint32      len;
    vint        negStatusPresent;

    if (0 == token_ptr->length) {
        return (GAPP_ERR);
    }

    *negStatus_ptr = 0;
    sdpMd_ptr[0] = 0;

    if ((0 == OSAL_strncmp(GAPP_GSM_CALL_EXT_STATUS,
            token_ptr->start_ptr, sizeof(GAPP_GSM_CALL_EXT_STATUS) - 1)) ||
            (0 == OSAL_strncmp(GAPP_GSM_CALL_MONITOR,
            token_ptr->start_ptr, sizeof(GAPP_GSM_CALL_MONITOR) - 1))) {

        /* Init buffer */
        GAPP_initBuffer(token_ptr->start_ptr, token_ptr->length, &buff);
        /*
         * Get the end of the field name.
         * Then the first value after that is the call index
         */
        if (GAPP_OK != GAPP_getToken(&buff, ":")) {
            return (GAPP_ERR);
        }
        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }
        /* It can only be a single digit number */
        idx = (*buff.token.start_ptr) - 48;
        *callIdx_ptr = idx;
        /* Let's get the event */
        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }
        /* It's <dir> */
        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }
        /* It's <neg_status_present>, only one digit */
        negStatusPresent = (*buff.token.start_ptr) - 48;

        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }
        /* It's <neg_status> */
        *negStatus_ptr = (*buff.token.start_ptr) - 48;

        /*  It's <SDP_md> */
        if (1 == negStatusPresent) {
            if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
                if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
                    len = GAPP_CALL_MEDIA_DESC_STRING_SZ;
                    if (buff.token.length >= len) {
                        len = len - 1;
                    }
                    else {
                        len = buff.token.length;
                    }
                    OSAL_memCpy(sdpMd_ptr,
                            buff.token.start_ptr, len);
                    /* NULL terminate */
                    sdpMd_ptr[len] = 0;
                }
            }
        }
        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }

        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }
        /*  It's <cs_mode>, ignore it. */
        if (GAPP_OK != GAPP_getToken(&buff, ",")) {
            return (GAPP_ERR);
        }
        /*  It's <ccstatus> */
        event = OSAL_atoi(buff.token.start_ptr);
        /* We have +CLCCS event. */
        GAPP_dbgPrintf("%s: idx:%d event:%d\n", __FUNCTION__,
                idx, event);
        *event_ptr = event;

        if ((GAPP_CALL_EVT_EXT_ALERTING_MT == event) ||
                (GAPP_CALL_EVT_EXT_CALL_WAITING_MT == event)) {
            /* uri_ptr is null, no need to process the address. */
            if (NULL == uri_ptr) {
                return (GAPP_OK);
            }
            if (GAPP_OK != GAPP_getToken(&buff, ",")) {
                return (GAPP_ERR);
            }
            /*  It's empty, ignore it */
            if (GAPP_OK != GAPP_getToken(&buff, ",")) {
                return (GAPP_ERR);
            }
            /*  It's numbertype, ignore it */
            if (GAPP_OK != GAPP_getToken(&buff, ",")) {
                return (GAPP_ERR);
            }
            /*  It's ton, ignore it */

            if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
                if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
                    /* 
                     * Found the address, let's copy it to the 
                     * destination buffer 
                     */
                    len = uriLen;
                    if (buff.token.length >= len) {
                        len = len - 1;
                    }
                    else {
                        len = buff.token.length;
                    }
                    OSAL_memCpy(uri_ptr, 
                            buff.token.start_ptr, len);
                    /* NULL terminate */
                    uri_ptr[len] = 0;
                    return (GAPP_OK);
                }
            }
            /* There's no caller Info. caller ID is hidden. */
            *uri_ptr = 0;
            return (GAPP_OK);
        }
        return (GAPP_OK);
    }

    return (GAPP_ERR);
}

/*
 * ======== _GAPP_checkForIncomingCall() ========
 * This function searches for an incoming call
 *
 * Return Values:
 *   GAPP_ERR: no incoming found
 *   GAPP_OK: otherwise
 */
static vint _GAPP_checkForIncomingCall(
    GAPP_Buffer     *result_ptr,
    char            *outAddress_ptr,
    uint32           maxOutAddressLen,
    vint            *outCallerIdx_ptr,
    uint32          *isVcc_ptr,
    uint16          *sessionType_ptr,
    vint            *negStatus_ptr)
{
    GAPP_Buffer buff;
    vint        idx = 0;
    vint        event;
    uint32      len;
    char        sdpMd[GAPP_CALL_MEDIA_DESC_STRING_SZ];

    *isVcc_ptr = 0; // set default to false

    /* Reset session type. */
    *sessionType_ptr = 0;
    /*
     * Go thru all the entries in the status. They are in the following
     * format... +CLCC: 1,0,0,0,0.  Find a call that is either a new incoming
     * call or a call waiting.  then get the address of the remote party and the
     * index of the call.
     */
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            continue;
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_CALL_STATUS,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_CALL_STATUS) - 1)) {

            GAPP_initBuffer(result_ptr->token.start_ptr,
                    result_ptr->token.length, &buff);
            /*
             * Get the end of the field name.
             * Then the first value after that is the call index
             */
            if (GAPP_OK == GAPP_getToken(&buff, ":")) {
                if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                    /* It can only be a single digit number */
                    idx = (*buff.token.start_ptr) - 48;
                    /* Let's get the event */
                    event = GAPP_CALL_EVT_NONE;
                    if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                        if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                            /* It can only be a single digit number */
                            event = (*buff.token.start_ptr) - 48;
                        }
                    }
                    /* 
                     * If we have a match on the idx then we found the 
                     * call state for this call 
                     */
                    if ((GAPP_CALL_EVT_INCOMING == event) || 
                            (GAPP_CALL_EVT_WAITING == event)) {
                        if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
                            if (GAPP_OK == GAPP_getToken(&buff, "\"")) {
                                /* 
                                 * Found the address, let's copy it to the 
                                 * destination buffer 
                                 */
                                len = maxOutAddressLen;
                                if (buff.token.length >= len) {
                                    len = len - 1;
                                }
                                else {
                                    len = buff.token.length;
                                }
                                OSAL_memCpy(outAddress_ptr, 
                                        buff.token.start_ptr, len);
                                /* NULL terminate */
                                outAddress_ptr[len] = 0;
                                *outCallerIdx_ptr = idx;
                                return (GAPP_OK);
                            }
                        }
                        /* There's no caller Info. caller ID is hidden. */
                        *outAddress_ptr = 0;
                        *outCallerIdx_ptr = idx;
                        return (GAPP_OK);
                    }
                }
            }
        }
        else if (0 == OSAL_strncmp(GAPP_GSM_CALL_SRVCC_START,
                result_ptr->token.start_ptr, sizeof(GAPP_GSM_CALL_SRVCC_START) - 1)) {
             GAPP_dbgPrintf("%s: GAPP_GSM_CALL_SRVCC_START\n", __FUNCTION__);
             /* Get the caller ID. fake for now "vcc" */             
             if (GAPP_ERR == getAddressAndIndex(result_ptr->token.start_ptr, 
                     result_ptr->token.length, outAddress_ptr,
                     maxOutAddressLen, outCallerIdx_ptr, NULL, 0)) {
                /* Modem didn't add address or call ID */
                outAddress_ptr[0] = 0;
                *outCallerIdx_ptr = 0;
             } 
             /* Notify Caller this is VCC */
            *isVcc_ptr = ISI_SRVCC_STATUS_INCOMING_NOTIFY;
            return (GAPP_OK);
        }
        /* Check for +CLCCS */
        else if (GAPP_OK == _GAPP_callGetGsmExtEvent(&idx,
                &result_ptr->token,
                &event, outAddress_ptr, maxOutAddressLen, 
                negStatus_ptr, sdpMd)) {
            if ((GAPP_CALL_EVT_EXT_ALERTING_MT != event) &&
                    (GAPP_CALL_EVT_EXT_CALL_WAITING_MT != event)) {
                continue;
            }

            /* Got incoming event for +CLCCS. */
            GAPP_dbgPrintf("%s: Got incoming event:%d, idx:%d, from:%s\n",
                    __FUNCTION__, event, idx, outAddress_ptr);
            if (0 != negStatus_ptr) {
                *sessionType_ptr = 0;
                if (NULL != OSAL_strscan(sdpMd, PRXY_MEDIA_PROFILE_STR_AUDIO)) {
                    *sessionType_ptr |= ISI_SESSION_TYPE_AUDIO;
                }
                if (NULL != OSAL_strscan(sdpMd, PRXY_MEDIA_PROFILE_STR_VIDEO)) {
                    *sessionType_ptr |= ISI_SESSION_TYPE_VIDEO;
                }
                if (0 == *sessionType_ptr) {
                    /* No media provided, use audio as default. */
                    *sessionType_ptr = ISI_SESSION_TYPE_AUDIO;
                }
            }
            *outCallerIdx_ptr = idx;
            return (GAPP_OK);
        }
    } /* End of while */

    return (GAPP_ERR);
}

/*
 * ======== _GAPP_inboundCallAttempt() ========
 * This function is the handler for incoming call attempts. It will examine an
 * unsolicited result code and determine if it is indeed a call attempt, if it
 * is then it will attempt to reserve and initialize resources needed to
 * accomodate the call.
 *
 */
static void _GAPP_inboundCallAttempt(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    GSM_Id         gsmId;
    GAPP_CallObj  *call_ptr;
    vint           callIdx;
    vint           negStatus;


    if ((GAPP_OK != GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) ||
            (0 == result_ptr->token.length)) {
        /* Nothing to process */
        return;
    }

    /* Check if it's a call report. */
    if (0 != OSAL_strncmp(GAPP_GSM_CALL_REPORT,
            result_ptr->token.start_ptr, sizeof(GAPP_GSM_CALL_REPORT) - 1)) {
        return;
    }

    if (GAPP_OK != _GAPP_checkForIncomingCall(result_ptr,
            evt_ptr->isiMsg.msg.call.from, ISI_ADDRESS_STRING_SZ, &callIdx,
            &evt_ptr->isiMsg.msg.call.srvccStatus,
            &evt_ptr->isiMsg.msg.call.type, &negStatus)) {
        /* No incoming calls right now. */
        return;
    }

    /* If session type is 0, set default to audio. */
    if (0 == evt_ptr->isiMsg.msg.call.type) {
        evt_ptr->isiMsg.msg.call.type =  ISI_SESSION_TYPE_AUDIO;
    }

    /* XXX Currently always set direction for sendrecv. */ 
    evt_ptr->isiMsg.msg.call.audioDirection = ISI_SESSION_DIR_SEND_RECV;
    evt_ptr->isiMsg.msg.call.videoDirection = ISI_SESSION_DIR_SEND_RECV;

    /* Otherwise there are calls 'incoming' or 'waiting'. */
    /* 
     * Check if we already now about this call that is 'incoming' or 
     * 'waiting'. 
     */
    if (OSAL_TRUE == _GAPP_isCallBitSet(&service_ptr->callBits.ring, callIdx)) {
        /* We already know about this call. */
        return;
    }

    /* Let's make a call */
    if (NULL != (call_ptr = GAPP_getCallByIsiId(service_ptr, 0))) {
        call_ptr->idx = callIdx;
        call_ptr->negStatus = negStatus;
        /*
         * If there's caller ID data check and see if it should
         * be a blocked user
         */
        if (evt_ptr->isiMsg.msg.call.from[0] != 0) {
            if (GAPP_searchBlockedUser(&service_ptr->blockedUsers,
                    evt_ptr->isiMsg.msg.call.from, 0) == GAPP_OK) {
                /*
                 * Then the caller ID of the remote party is 'blocked'
                 * return an appropriate response.
                 */
                _GAPP_callRejectCmd(service_ptr, call_ptr);
                return;
            }
        }

        /*
         * We can accomodate this phone call, generate a unique
         * number for this call and fire the state machine.
         */
        call_ptr->isiCallId = GAPP_getUniqueIsiId(service_ptr->isiServiceId);
        call_ptr->isVcc = evt_ptr->isiMsg.msg.call.srvccStatus;
        /* Cache session type remote proposed. */
        call_ptr->sessionType = evt_ptr->isiMsg.msg.call.type;

        GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                service_ptr->protocolId, ISIP_CALL_REASON_INITIATE,
                ISIP_STATUS_INVALID, &evt_ptr->isiMsg);

        /* Load the ringTemplate from the service for this call  */
        evt_ptr->isiMsg.msg.call.ringTemplate = service_ptr->ringTemplate;
        GAPP_sendEvent(evt_ptr);

        /* Tell ISI what the vdi number is for this gsmAt call. */
        GAPP_callVdxIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                 service_ptr->protocolId, service_ptr->vdi, &evt_ptr->isiMsg);
        call_ptr->state = GAPP_CALL_STATE_RING;
        GAPP_setCallBit(&service_ptr->callBits.ring, call_ptr->idx, OSAL_TRUE);
        GAPP_setCallBit(&service_ptr->callBits.active, call_ptr->idx,
                OSAL_TRUE);
    }
    else {
        /* We can't accept it, so refuse it */
        GSM_cmdCallHoldZero(&service_ptr->daemon, &gsmId);
    }
    return;
}

/*
 * ======== _GAPP_ignoreCallRequests() ========
 * This function is the handler for incoming call attempts. It will examine an
 * unsolicited result code and determine if it is indeed a call attempt, if it
 * is then it will attempt to reserve and initialize resources needed to
 * accomodate the call.
 */
static vint _GAPP_ignoreCallRequests(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr)
{
    if (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            return (GAPP_ERR);
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_CALL_RING1, result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_CALL_RING1) - 1)) {
            return (GAPP_OK);
        }
        else if (0 == OSAL_strncmp(GAPP_GSM_CALL_RING2,
                result_ptr->token.start_ptr, sizeof(GAPP_GSM_CALL_RING2) - 1)) {
            return (GAPP_OK);
        }
        else if (0 == OSAL_strncmp(GAPP_GSM_CALL_CLIP, 
                result_ptr->token.start_ptr, sizeof(GAPP_GSM_CALL_CLIP) - 1)) {
            return (GAPP_OK);
        }
        else if (0 == OSAL_strncmp(GAPP_GSM_CALL_WAITING, 
                result_ptr->token.start_ptr, 
                sizeof(GAPP_GSM_CALL_WAITING) - 1)) {
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}

/*
 * ======== _GAPP_callGetGsmEvent() ========
 * This function checks if a call specified by call_ptr is currently active
 * and what state the GSM module is reporting it's in.  The GSM reports this in
 * +CLCC reports which include a list of all Current Calls along with an
 * enumerated value that specifies the calls current state. GAPP uses these
 * periodic +CLCC reports and the state as an "event" that drives the call
 * state machine.
 *
 * Return Values:
 *  An enuerated values that represents the state of the call.  Possible values
 *  are below. See the GSM AT command spec for the +CLCC command for more
 *  details:
 *   GAPP_CALL_EVT_ACTIVE = 0,
 *   GAPP_CALL_EVT_HELD = 1,
 *   GAPP_CALL_EVT_DIALING = 2,
 *   GAPP_CALL_EVT_ALERTING = 3,
 *   GAPP_CALL_EVT_INCOMING = 4,
 *   GAPP_CALL_EVT_WAITING = 5,
 *   GAPP_CALL_EVT_TERMINATED = 6,
 *   GAPP_CALL_EVT_NONE = 7,
 */
static vint _GAPP_callGetGsmEvent(
    GAPP_CallObj *call_ptr,
    GAPP_Buffer  *result_ptr)
{
    GAPP_Buffer buff;
    vint        idx;
    vint        event;

    GAPP_dbgPrintf("%s: result:%s\n", __FUNCTION__, result_ptr->curr_ptr);
    /*
     * Go threw all the entries in the status. They are in the following
     *  format... +CLCC: 1,0,0,0,0
     */
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            continue;
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_CALL_STATUS,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_CALL_STATUS) - 1)) {

            GAPP_initBuffer(result_ptr->token.start_ptr,
                result_ptr->token.length, &buff);
            /*
             * Get the end of the field name.
             * Then the first value after that is the call index
             */
            if (GAPP_OK == GAPP_getToken(&buff, ":")) {
                if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                    /* It can only be a single digit number */
                    idx = (*buff.token.start_ptr) - 48;
                    /* Let's get the event */
                    event = GAPP_CALL_EVT_NONE;
                    if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                        if (GAPP_OK == GAPP_getToken(&buff, ",")) {
                            /* It can only be a single digit number */
                            event = (*buff.token.start_ptr) - 48;
                        }
                    }
                    /* 
                     * If we have a match on the idx then we found the 
                     * call state for this call 
                     */
                    if (idx == call_ptr->idx) {
                        return (event);
                    }
                    /* 
                     * The idx might not be assigned from the underlying cell
                     * interface, so assign it.
                     */
                    if (GAPP_CALL_EVT_DIALING == event &&
                            GAPP_CALL_STATE_MAKE == call_ptr->state && 
                            call_ptr->idx == 0) {
                        call_ptr->idx = idx;
                        return (event);
                    }
                }
            }
        }
    } /* End of while */
    return (GAPP_CALL_EVT_TERMINATED);
}

#endif
/*
 * ======== GAPP_unsolicitedSrvccEvent() ========
 * This function is use to check the incoming event is for SRVCC result from GSM
 * device. The definition of different SRVCC commands are as below.
 * +CIREPH:1    The SRVCC handover is successful.
 * +CIREPH:2    The SRVCC handover is failed or canceled.
 */
vint GAPP_unsolicitedSrvccEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    ISIP_CallReason reason = ISIP_CALL_REASON_INVALID; 
    GAPP_CallObj   *call_ptr;
    if (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            return (GAPP_ERR);
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_CALL_SRVCC_FAILURE,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_CALL_SRVCC_FAILURE) - 1)) {
            reason = ISIP_CALL_REASON_HANDOFF_FAILED;
        }
        else if (0 == OSAL_strncmp(GAPP_GSM_CALL_SRVCC_SUCCESS,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_CALL_SRVCC_SUCCESS) - 1)) {
            reason = ISIP_CALL_REASON_HANDOFF_SUCCESS;
        }
    }

    if (reason != ISIP_CALL_REASON_INVALID) {
        if(NULL != (call_ptr = GAPP_getCallIsVcc(service_ptr))) {
            GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                    service_ptr->protocolId, reason, ISIP_STATUS_INVALID,
                    &evt_ptr->isiMsg);
            GAPP_sendEvent(evt_ptr);
            /* Clear it for next time */
            evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}

/*
 * ======== GAPP_callResultEvent() ========
 * This function determines if a result code is related to a command that was
 * previously issued related to call control. It will determine if it is
 * related to a previous call command and then perform any appropriate state
 * machines.
 *
 * Return Values:
 * GAPP_OK: The result code is related to a call command and was processed. No
 *        further processing of this result code is necessary.
 * GAPP_ERR: The result code is NOT related to any calls.  The code calling this
 *         function should continue to process the result code.
 */
vint GAPP_callResultEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GSM_Id           gsmId,
    ISIP_Message    *isi_ptr)
{
    GAPP_CallObj  *call_ptr;
    vint           idx;

    /* Find out if this event is for an existing call */
    if (NULL == (call_ptr = GAPP_getCallByGsmId(service_ptr, gsmId))) {
        return (GAPP_ERR);
    }

    if (OSAL_TRUE == call_ptr->isFmc) {
        /* Then this is not for us, the FMC sub-module should handle this */
        return (GAPP_ERR);
    }

    /* Then it's a result code to call's previous AT command */

    /* Switch on state and then call the state handler */
    switch (call_ptr->state) {
        case GAPP_CALL_STATE_MAKE:
            if (service_ptr->extDialCmdEnabled) {
                /* Check +CDU: <ccidx> */
                if (GAPP_OK == GAPP_chkResult(result_ptr,
                        GAPP_GSM_CALL_DIAL_URI,
                        sizeof(GAPP_GSM_CALL_DIAL_URI) - 1)) {
                    idx = OSAL_atoi(result_ptr->token.start_ptr +
                            sizeof(GAPP_GSM_CALL_DIAL_URI));
                    call_ptr->idx = idx;
                    break;
                }
            }
            else if (GAPP_OK != GAPP_chkResult(result_ptr, GAPP_GSM_OK,
                    sizeof(GAPP_GSM_OK) - 1)) {
                GAPP_callIsiEvt(service_ptr->isiServiceId,
                        call_ptr->isiCallId, service_ptr->protocolId,
                        ISIP_CALL_REASON_TERMINATE,
                        ISIP_STATUS_INVALID, isi_ptr);
                GAPP_callInit(service_ptr, call_ptr);
            }
            break;
        case GAPP_CALL_STATE_TERM:
            /*
             * Always consider the call terminated regardless of the
             * result code.  But lets do some debug logging.
             */
            if (GAPP_OK != GAPP_chkResult(result_ptr, GAPP_GSM_OK,
                    sizeof(GAPP_GSM_OK) - 1)) {
                GAPP_dbgPrintf("%s: GSM Warning! A command used to "
                        "terminate a call has failed.  Terminating anyway\n",
                        __FUNCTION__);
            }
            GAPP_callIsiEvt(service_ptr->isiServiceId,
                    call_ptr->isiCallId, service_ptr->protocolId,
                    ISIP_CALL_REASON_TERMINATE,
                    ISIP_STATUS_INVALID, isi_ptr);
            GAPP_callInit(service_ptr, call_ptr);
            break;
        case GAPP_CALL_STATE_ANSWER:
            if (GAPP_OK == GAPP_chkResult(result_ptr, GAPP_GSM_OK,
                    sizeof(GAPP_GSM_OK) - 1)) {
                /* Change the state to ACTIVE when we receive call report. */
                //call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                GAPP_setCallBit(&service_ptr->callBits.ring, call_ptr->idx,
                        OSAL_FALSE);
            }
            else {
                GAPP_callIsiEvt(service_ptr->isiServiceId,
                        call_ptr->isiCallId, service_ptr->protocolId,
                        ISIP_CALL_REASON_TERMINATE,
                        ISIP_STATUS_INVALID, isi_ptr);
                GAPP_callInit(service_ptr, call_ptr);
            }
            break;
        case GAPP_CALL_STATE_HOLD:
            if (GAPP_OK == GAPP_chkResult(result_ptr, GAPP_GSM_OK,
                    sizeof(GAPP_GSM_OK) - 1)) {
                call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                        service_ptr->protocolId, ISIP_CALL_REASON_HOLD,
                        ISIP_STATUS_DONE, isi_ptr);
            }
            break;
        case GAPP_CALL_STATE_RESUME:
            if (GAPP_OK == GAPP_chkResult(result_ptr, GAPP_GSM_OK,
                    sizeof(GAPP_GSM_OK) - 1)) {
                call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                GAPP_callIsiEvt(service_ptr->isiServiceId, call_ptr->isiCallId,
                        service_ptr->protocolId, ISIP_CALL_REASON_RESUME,
                        ISIP_STATUS_DONE, isi_ptr);
            }
            break;
        case GAPP_CALL_STATE_IDLE:
        case GAPP_CALL_STATE_RING:
        case GAPP_CALL_STATE_ACTIVE:
        case GAPP_CALL_STATE_LOGIN:
        case GAPP_CALL_STATE_DIAL:
            /* The login and dial state are only used for FMC */
        default:
            /*
             * There are no AT command result codes that should be handled in
             * these states.
             */
            return (GAPP_ERR);
    } /* End of switch */
    return (GAPP_OK);
}

/*
 * ======== GAPP_callInit() ========
 *
 * This function initializes a call object.  It is typically called
 * at system start up and every time an active call is disconnected.
 *
 * NOTE: The "idx" member of the call_ptr, should never change.
 *       DO NOT CLEAR THIS VALUE.
 *
 * Returns:
 * Nothing
 */
void GAPP_callInit(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr)
{
    call_ptr->isiCallId = 0;
    call_ptr->state = GAPP_CALL_STATE_IDLE;
    call_ptr->gsmId = 0;
    call_ptr->vdi[0] = 0;
    /*
     * Do not change call_ptr->idx here.
     * It must be a persistent value .
     */

    /* Clear the call bits */
    GAPP_setCallBit(&service_ptr->callBits.active, call_ptr->idx, OSAL_FALSE);
    GAPP_setCallBit(&service_ptr->callBits.hold, call_ptr->idx, OSAL_FALSE);
    GAPP_setCallBit(&service_ptr->callBits.ring, call_ptr->idx, OSAL_FALSE);
    GAPP_setCallBit(&service_ptr->callBits.conf, call_ptr->idx, OSAL_FALSE);
    GAPP_setCallBit(&service_ptr->callBits.modify, call_ptr->idx, OSAL_FALSE);
    
    call_ptr->idx = 0;
    call_ptr->isVcc = 0;
    call_ptr->sessionType = ISI_SESSION_TYPE_NONE;
    call_ptr->negStatus = 0;
    return;
}

#ifndef GAPP_DISABLE_GSM

/*
 * ======== _GAPP_callExtReportHandler() ========
 * This function is the entry point of the call state machine for events that
 * come from the GSM module regarding call state.  These events are signalled
 * from GSM in a +CLCCS call report.
 *
 * Return Values:
 *  Nothing.
 */
static void _GAPP_callExtReportHandler(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    vint             event,
    GAPP_Event      *evt_ptr,
    vint             negStatus)
{
    ISIP_Message *isi_ptr;

    GAPP_dbgPrintf("%s: idx:%d state:%d event:%d, negStatus:%d\n",
            __FUNCTION__, call_ptr->idx, call_ptr->state, event, negStatus);

    isi_ptr = &evt_ptr->isiMsg;
    isi_ptr->code = ISIP_CODE_INVALID;
    /* Switch on the state and handle the event within that state */
    switch (call_ptr->state) {
        case GAPP_CALL_STATE_ACTIVE:
            switch (event) {
                case GAPP_CALL_EVT_EXT_IDLE:
                case GAPP_CALL_EVT_EXT_RELEASED_MO:
                case GAPP_CALL_EVT_EXT_RELEASED_MT:
                case GAPP_CALL_EVT_EXT_USER_BUSY:
                case GAPP_CALL_EVT_EXT_USER_DTMD_USER_BUSY:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId,
                            service_ptr->protocolId,
                            ISIP_CALL_REASON_TERMINATE,
                            ISIP_STATUS_INVALID, isi_ptr);
                    GAPP_callInit(service_ptr, call_ptr);
                    break;
                case GAPP_CALL_EVT_EXT_ACTIVE:
                    /* Process local modified the media. */
                    if (OSAL_TRUE == _GAPP_isCallBitSet(
                            &service_ptr->callBits.modify,
                            call_ptr->idx)) {
                        if (0 == negStatus) {
                            /* No result yet. */
                            break;
                        }
                        /* There is result of the modification. */
                        /* Always populate done event */
                        GAPP_callIsiEvt(service_ptr->isiServiceId,
                                call_ptr->isiCallId,
                                service_ptr->protocolId,
                                ISIP_CALL_REASON_MODIFY,
                                ISIP_STATUS_DONE, isi_ptr);
                        call_ptr->sessionType = evt_ptr->isiMsg.msg.call.type;
                        /* XXX Currently always set direction for sendrecv. */ 
                        evt_ptr->isiMsg.msg.call.audioDirection =
                                ISI_SESSION_DIR_SEND_RECV;
                        evt_ptr->isiMsg.msg.call.videoDirection =
                                ISI_SESSION_DIR_SEND_RECV;
                        /* Clear modify bit. */
                        GAPP_setCallBit(&service_ptr->callBits.modify,
                                call_ptr->idx, OSAL_FALSE);
                        break;
                    }

                    /*
                     * Local is not modified the call, see if remote change
                     * the media.
                     */
                    if ((GSM_MEDIA_NEG_STATUS_UNCONDITIONAL == negStatus) &&
                            (call_ptr->sessionType ==
                            evt_ptr->isiMsg.msg.call.type)) {
                        /* Media no changed, ignore it. */
                        break;
                    }
                    if ((negStatus != call_ptr->negStatus) &&
                            ((GSM_MEDIA_NEG_STATUS_PROPOSE == negStatus) ||
                            (GSM_MEDIA_NEG_STATUS_UNCONDITIONAL ==
                            negStatus))) {
                        /* Popualte event */
                        GAPP_callIsiEvt(service_ptr->isiServiceId,
                                call_ptr->isiCallId,
                                service_ptr->protocolId,
                                ISIP_CALL_REASON_MODIFY,
                                ISIP_STATUS_INVALID, isi_ptr);
                        /* XXX Currently always set direction for sendrecv. */ 
                        evt_ptr->isiMsg.msg.call.audioDirection =
                                ISI_SESSION_DIR_SEND_RECV;
                        evt_ptr->isiMsg.msg.call.videoDirection =
                                ISI_SESSION_DIR_SEND_RECV;
                        call_ptr->negStatus = negStatus;
                        call_ptr->sessionType = evt_ptr->isiMsg.msg.call.type;
                    }
                    break;
                default:
                    /* Ignore all these */
                    break;
            }
            break;
        case GAPP_CALL_STATE_RING:
        case GAPP_CALL_STATE_ANSWER:
        case GAPP_CALL_STATE_TERM:
            switch (event) {
                case GAPP_CALL_EVT_EXT_IDLE:
                case GAPP_CALL_EVT_EXT_RELEASED_MO:
                case GAPP_CALL_EVT_EXT_RELEASED_MT:
                case GAPP_CALL_EVT_EXT_USER_BUSY:
                case GAPP_CALL_EVT_EXT_USER_DTMD_USER_BUSY:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_TERMINATE,
                            ISIP_STATUS_INVALID, isi_ptr);
                    GAPP_callInit(service_ptr, call_ptr);
                    break;
                case GAPP_CALL_EVT_EXT_ACTIVE:
                    /* Call is active. */
                    call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                    break;
                default:
                    /* Ignore all these */
                    break;
            }
            break;
        case GAPP_CALL_STATE_MAKE:
            switch (event) {
                case GAPP_CALL_EVT_EXT_IDLE:
                case GAPP_CALL_EVT_EXT_RELEASED_MO:
                case GAPP_CALL_EVT_EXT_RELEASED_MT:
                case GAPP_CALL_EVT_EXT_USER_BUSY:
                case GAPP_CALL_EVT_EXT_USER_DTMD_USER_BUSY:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_TERMINATE,
                            ISIP_STATUS_INVALID, isi_ptr);
                    GAPP_callInit(service_ptr, call_ptr);
                    break;
                case GAPP_CALL_EVT_EXT_ACTIVE:
                    GAPP_setCallBit(&service_ptr->callBits.active,
                            call_ptr->idx, OSAL_TRUE);
                    /* XXX Currently always set direction for sendrecv. */ 
                    evt_ptr->isiMsg.msg.call.audioDirection =
                            ISI_SESSION_DIR_SEND_RECV;
                    evt_ptr->isiMsg.msg.call.videoDirection =
                            ISI_SESSION_DIR_SEND_RECV;
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_ACCEPT,
                            ISIP_STATUS_INVALID, isi_ptr);
                    /* Let's send the current event and then send another */ 
                    GAPP_sendEvent(evt_ptr); 
                    GAPP_callIsiEvt(service_ptr->isiServiceId, 
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_ACCEPT_ACK, 
                            ISIP_STATUS_INVALID, isi_ptr);

                    call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                    break;
                case GAPP_CALL_EVT_EXT_CALLING_MO:
                    GAPP_setCallBit(&service_ptr->callBits.active,
                            call_ptr->idx, OSAL_TRUE);
                    break;
                case GAPP_CALL_EVT_EXT_CONNECTING_MO:
                    GAPP_setCallBit(&service_ptr->callBits.active,
                            call_ptr->idx, OSAL_TRUE);
                    break;
                case GAPP_CALL_EVT_EXT_ALERTING_MO:
                    GAPP_setCallBit(&service_ptr->callBits.active,
                            call_ptr->idx, OSAL_TRUE);
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_ACK,
                            ISIP_STATUS_INVALID, isi_ptr);
                    call_ptr->state = GAPP_CALL_STATE_ALERTING;
                    break;
                default:
                    break;
            }
            break;
        case GAPP_CALL_STATE_ALERTING:
            switch (event) {
                case GAPP_CALL_EVT_EXT_IDLE:
                case GAPP_CALL_EVT_EXT_RELEASED_MO:
                case GAPP_CALL_EVT_EXT_RELEASED_MT:
                case GAPP_CALL_EVT_EXT_USER_BUSY:
                case GAPP_CALL_EVT_EXT_USER_DTMD_USER_BUSY:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_TERMINATE,
                            ISIP_STATUS_INVALID, isi_ptr);
                    GAPP_callInit(service_ptr, call_ptr);
                    break;
                case GAPP_CALL_EVT_EXT_ACTIVE:
                    GAPP_setCallBit(&service_ptr->callBits.active,
                            call_ptr->idx, OSAL_TRUE);
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_ACCEPT,
                            ISIP_STATUS_INVALID, isi_ptr);
                    /* XXX Currently always set direction for sendrecv. */ 
                    evt_ptr->isiMsg.msg.call.audioDirection =
                            ISI_SESSION_DIR_SEND_RECV;
                    evt_ptr->isiMsg.msg.call.videoDirection =
                            ISI_SESSION_DIR_SEND_RECV;
                    /* Let's send the current event and then send another */
                    GAPP_sendEvent(evt_ptr);
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_ACCEPT_ACK,
                            ISIP_STATUS_INVALID, isi_ptr);

                    call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                    break;
                default:
                    break;
            }
            break;
        case GAPP_CALL_STATE_LOGIN:
        case GAPP_CALL_STATE_DIAL:
            /* The login and dial state are only used for FMC */
        case GAPP_CALL_STATE_IDLE:
        default:
            /* Then ignore this event */
            break;
    }
}

/*
 * ======== _GAPP_callProcessExtCallReport() ========
 * 
 * Private function to process extended call report +CLCCS.
 *
 * Return Values:
 *   GAPP_OK
 */
static vint _GAPP_callProcessExtCallReport(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    vint             callIdx;
    GAPP_CallObj    *call_ptr;
    vint             event;
    vint             negStatus;
    char             sdpMd[GAPP_CALL_MEDIA_DESC_STRING_SZ];
    uint16 *sessionType_ptr;

    GAPP_dbgPrintf("%s: result:%s\n", __FUNCTION__, result_ptr->curr_ptr);

    /*
     * Go threw all the entries in the status. They are in the following
     *  format... +CLCCS: 1,0,0,0,"",0,4,0,1,0,"sip:204@172.16.0.165"
     */
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (GAPP_OK != _GAPP_callGetGsmExtEvent(&callIdx,
                &result_ptr->token, &event, NULL, 0, &negStatus,
                sdpMd)) {
            /* No report for this call. Don't process it. */
            continue;
        }

        GAPP_dbgPrintf("%s: Get +CLCCS report. event:%d for Call idx:%d "
                "negStatus:%d, sdpMd:%s\n",
                __FUNCTION__, event, callIdx, negStatus, sdpMd);
        /*
         * Then we have valid call report for the call id. Check if the call
         * is there.
         */
        if (GAPP_CALL_NUM <= callIdx) {
            GAPP_dbgPrintf("Call index exceeds the maximum: %d\n", callIdx);
            continue;
        }

        /* Find the call */
        call_ptr = GAPP_getCallByIdx(service_ptr, callIdx);
        if ((NULL == call_ptr) || (GAPP_CALL_STATE_IDLE == call_ptr->state)) {
            GAPP_dbgPrintf("The call is idle, ignore it.\n");
            continue;
        }

        /* The call status is valid, process the event. */
        sessionType_ptr = &evt_ptr->isiMsg.msg.call.type;
        if (0 != negStatus) {
            *sessionType_ptr = 0;
            if (NULL != OSAL_strscan(sdpMd, PRXY_MEDIA_PROFILE_STR_AUDIO)) {
                *sessionType_ptr |= ISI_SESSION_TYPE_AUDIO;
            }
            if (NULL != OSAL_strscan(sdpMd, PRXY_MEDIA_PROFILE_STR_VIDEO)) {
                *sessionType_ptr |= ISI_SESSION_TYPE_VIDEO;
            }
        }
        /* Process the event. */
        _GAPP_callExtReportHandler(service_ptr, call_ptr, event,
                evt_ptr, negStatus);

        if (evt_ptr->isiMsg.code != ISIP_CODE_INVALID) {
            GAPP_sendEvent(evt_ptr);
            /* Clear it for next time */
            evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
        }
    }

    return (GAPP_OK);
}

/*
 * ======== _GAPP_callIsExtCallReport() ========
 * 
 * Private function to check if the call report is standard call report,
 * i.e +CLCC or extended call report, i.e. +CLCCS or +CMCCSI.
 *
 * Return Values:
 *   OSAL_TRUE: It's extended call report.
 *   OSAL_FALSE: It's standard call report.
 */
static OSAL_Boolean _GAPP_callIsExtCallReport(
    GAPP_Buffer *result_ptr)
{
    if ((NULL == OSAL_strnscan(result_ptr->curr_ptr,
            result_ptr->length, GAPP_GSM_CALL_EXT_STATUS)) &&
            (NULL == OSAL_strnscan(result_ptr->curr_ptr,
            result_ptr->length, GAPP_GSM_CALL_MONITOR))) {
        return (OSAL_FALSE);
    }
    return (OSAL_TRUE);
}

/*
 * ======== _GAPP_callReportHandler() ========
 * This function is the entry point of the call state machine for events that
 * come from the GSM module regarding call state.  These events are signalled
 * from GSM in a +CLCC call report.
 *
 * Return Values:
 *  Nothing.
 */
static void _GAPP_callReportHandler(
    GAPP_ServiceObj *service_ptr,
    GAPP_CallObj    *call_ptr,
    vint             event,
    GAPP_Event      *evt_ptr)
{
    ISIP_Message *isi_ptr;

    isi_ptr = &evt_ptr->isiMsg;
    /* Switch on the state and handle the event within that state */
    switch (call_ptr->state) {
        case GAPP_CALL_STATE_ACTIVE:
        case GAPP_CALL_STATE_RING:
        case GAPP_CALL_STATE_ANSWER:
        case GAPP_CALL_STATE_TERM:
            switch (event) {
                case GAPP_CALL_EVT_TERMINATED:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_TERMINATE,
                            ISIP_STATUS_INVALID, isi_ptr);
                    GAPP_callInit(service_ptr, call_ptr);
                    break;
                default:
                    /* Ignore all these */
                    break;
            }
            break;
        case GAPP_CALL_STATE_MAKE:
            switch (event) {
                case GAPP_CALL_EVT_TERMINATED:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                            call_ptr->isiCallId, service_ptr->protocolId,
                            ISIP_CALL_REASON_TERMINATE,
                            ISIP_STATUS_INVALID, isi_ptr);
                    GAPP_callInit(service_ptr, call_ptr);
                    break;
                case GAPP_CALL_EVT_ACTIVE:
                    GAPP_callIsiEvt(service_ptr->isiServiceId,
                        call_ptr->isiCallId, service_ptr->protocolId,
                        ISIP_CALL_REASON_ACCEPT,
                        ISIP_STATUS_INVALID, isi_ptr);
                        /* Let's send the current event and then send another */
                        GAPP_sendEvent(evt_ptr);
                        GAPP_callIsiEvt(service_ptr->isiServiceId,
                                call_ptr->isiCallId, service_ptr->protocolId,
                                ISIP_CALL_REASON_ACCEPT_ACK,
                                ISIP_STATUS_INVALID, isi_ptr);

                        call_ptr->state = GAPP_CALL_STATE_ACTIVE;
                    break;
                case GAPP_CALL_EVT_DIALING:
                    GAPP_setCallBit(&service_ptr->callBits.active,
                            call_ptr->idx, OSAL_TRUE);
                default:
                    break;
            }
            break;
        case GAPP_CALL_STATE_LOGIN:
        case GAPP_CALL_STATE_DIAL:
            /* The login and dial state are only used for FMC */
        case GAPP_CALL_STATE_IDLE:
        default:
            /* Then ignore this event */
            break;
    }
}

/*
 * ======== _GAPP_callReport() ========
 * This function parses +CLCC reports from the GMS module and then pumps events
 * into the call state machine based on the result of the +CLCC call reports.
 *
 * This function is typically used when a "NO CARRIER", "CALL STATUS", or
 * "RING STOP" event is received from the GSM module indicating that
 * an application should update the state of calls.
 *
 * Return Values:
 * None.
 */
static void _GAPP_callReport(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    vint             x;
    GAPP_CallObj    *call_ptr;
    vint             event;

    GAPP_dbgPrintf("%s: result:%s\n", __FUNCTION__, result_ptr->curr_ptr);
    /* Check if it's +CLCCS */
    if (_GAPP_callIsExtCallReport(result_ptr)) {
        /* Process +CLCCS. */
        _GAPP_callProcessExtCallReport(service_ptr, result_ptr, evt_ptr);
        /* Report processed, retrun. */
        return;
    }

    /* Then it's +CLCC. */
    for (x = 0; x < GAPP_CALL_NUM; x++) {
        call_ptr = &service_ptr->aCall[x];
        if (call_ptr->state != GAPP_CALL_STATE_IDLE) {
            event = _GAPP_callGetGsmEvent(call_ptr, result_ptr);
            GAPP_dbgPrintf(
                    "%s: Received GMS event: %d for Call idx:%d isiId:%d\n",
                    __FUNCTION__, event, x, call_ptr->isiCallId);
            /* Process the event in the normal call state machine */
            _GAPP_callReportHandler(service_ptr, call_ptr, event,
                    evt_ptr);
     
            /* Reset the buffer state for another round of parsing */
            GAPP_initBuffer(NULL, 0, result_ptr);
        }
        if (evt_ptr->isiMsg.code != ISIP_CODE_INVALID) {
            GAPP_sendEvent(evt_ptr);
            /* Clear it for next time */
            evt_ptr->isiMsg.code = ISIP_CODE_INVALID;
        }
    }
    return;
}

/*
 * ======== _GAPP_callStatus() ========
 * This function determines if a an unsolicited result code is related to
 * a call report containing the status (or state) of the call.  A call report
 * includes the current state of the call as reported by the GSM interface.
 * We process these states reported by the GSM interface as events that
 * drive the GAPP call state machine.  The possible states that
 * GSM may report are defined in the GSM ST Acommand spec under the +CLCC
 * command.
 *
 * Return Values:
 * GAPP_OK:  The result code is related to a call report.
 * GAPP_ERR: The result code is NOT related to a call report disconnecting.
 *           The code calling this function should continue to process the
 *           result code.
 */
static vint _GAPP_callStatus(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    while (GAPP_OK == GAPP_getToken(result_ptr, GAPP_GSM_CRLF)) {
        if (0 == result_ptr->token.length) {
            continue;
        }
        /* If we are here then we have a valid token */
        if (0 == OSAL_strncmp(GAPP_GSM_CALL_NO_CARRIER,
                result_ptr->token.start_ptr,
                sizeof(GAPP_GSM_CALL_NO_CARRIER) - 1) ||
            0 == OSAL_strncmp(GAPP_GSM_CALL_RINGSTOP,
                    result_ptr->token.start_ptr,
                    sizeof(GAPP_GSM_CALL_RINGSTOP) - 1) ||
            0 == OSAL_strncmp(GAPP_GSM_CALL_REPORT,
                    result_ptr->token.start_ptr,
                    sizeof(GAPP_GSM_CALL_REPORT) - 1)) {

            /* Check the call status report included in the payload */
            _GAPP_callReport(service_ptr, result_ptr, evt_ptr);
            return (GAPP_OK);
        }
    }
    return (GAPP_ERR);
}

/*
 * ======== GAPP_callUnsolicitedEvent() ========
 * This function determines if an unsolicited result code from the GSM module
 * is related to call control.  If so it will process it and handle
 * appropriate actions and state machines.
 *
 * Return Values:
 * GAPP_OK: The result code is related to call control and was processed. No
 *        further processing of this result code is necessary.
 * GAPP_ERR: The result code is NOT related to call control. The code calling
 *         this function should continue to process the result code.
 */
vint GAPP_callUnsolicitedEvent(
    GAPP_ServiceObj *service_ptr,
    GAPP_Buffer     *result_ptr,
    GAPP_Event      *evt_ptr)
{
    /* It's an 'unsolicited' result code. */
    /*
     * First check if it's a call related event that we want to ignore
     * but still filter out from Android.
     */
    if (GAPP_OK == _GAPP_ignoreCallRequests(service_ptr, result_ptr)) {
        /* then ignore it and prevent it from being processed any further. */
        return (GAPP_OK);
    }

    /* Reset the buffer object used to parse this event */
    GAPP_initBuffer(NULL, 0, result_ptr);

    /* Process any 'incoming' or 'waiting' calls. */
    _GAPP_inboundCallAttempt(service_ptr, result_ptr, evt_ptr);
    if (1 == evt_ptr->isiMsg.msg.call.srvccStatus) {
        return (GAPP_OK);
    }

    /* Reset the buffer object used to parse this event */
    GAPP_initBuffer(NULL, 0, result_ptr);

    /* Check if a call disconnected */
    if (GAPP_OK == _GAPP_callStatus(service_ptr, result_ptr, evt_ptr)) {
        /* Then the message was processed, so return */
        return (GAPP_OK);
    }
    return (GAPP_ERR);
}

/*
 * ======== GAPP_isiAudioCmd() ========
 * This function is the entry point for commands from ISI related to
 * GSM audio control.  Currently the only audio commands related to GSM are
 * for Call Conferencing.
 *
 * Return Values:
 * Nothing.
 */
void GAPP_isiAudioCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    GAPP_Event      *evt_ptr)
{
    ISIP_Conf       *conf_ptr;
    GAPP_CallObj    *call_ptr;
    vint             x;
    vint             y;
    uint32           active;
    OSAL_Boolean     found;
    GSM_Id           gsmId;
    uint32           conf;

    conf_ptr = &cmd_ptr->msg.media.media.conf;
    active = service_ptr->callBits.active;
    conf = service_ptr->callBits.conf;
    if (ISIP_MEDIA_REASON_CONFSTART == cmd_ptr->msg.media.reason) {
        /*
         * All held calls are going to be added as a conference so
         * switch all calls to active.
         */
        _GAPP_setHoldState(service_ptr, active, OSAL_FALSE, evt_ptr);
        /* Set up all active calls' conf bit mask. */
        _GAPP_setConfState(service_ptr, active, OSAL_TRUE);
        GSM_cmdHoldThree(&service_ptr->daemon, &gsmId);
    }
    else if (ISIP_MEDIA_REASON_CONFSTOP == cmd_ptr->msg.media.reason) {
        found = OSAL_FALSE;
        for (x = 0; x < GAPP_CALL_NUM; x++) {
            call_ptr = &service_ptr->aCall[x];
            if (OSAL_TRUE == _GAPP_isCallBitSet(&active, call_ptr->idx)) {
                for (y = 0 ; y < ISI_CONF_USERS_NUM; y++) {
                    if (call_ptr->isiCallId == conf_ptr->aCall[y]) {
                        /* Clear this call's conf bit mask. */
                        GAPP_setCallBit(&conf, call_ptr->idx, OSAL_FALSE);
                        /* Then this call should be made active */
                        found = OSAL_TRUE;
                        break;
                    }
                }
                if (OSAL_TRUE == found) {
                    break;
                }
            }
        }

        if (OSAL_TRUE == found) {
            /* Clear this call from a bit mask, so we don't place it on hold */
            GAPP_setCallBit(&active, call_ptr->idx, OSAL_FALSE);
            /* Place all the other calls on hold */
            _GAPP_setHoldState(service_ptr, active, OSAL_TRUE, evt_ptr);
            /* Make sure this one is active */
            GAPP_setCallBit(&service_ptr->callBits.hold, call_ptr->idx,
                    OSAL_FALSE);
            GSM_cmdCallHoldTwoIndex(&service_ptr->daemon, &gsmId, 
                    call_ptr->idx);
        }
    }
    return;
}

/*
 * ======== GAPP_isiCallCmd() ========
 * This function processes ISI commands related to call control.
 * It will perform any appropriate actions related to call control.
 *
 * Return Values:
 * Nothing.
 */
void GAPP_isiCallCmd(
    GAPP_ServiceObj *service_ptr,
    ISIP_Message    *cmd_ptr,
    GAPP_Event      *evt_ptr)
{ 
    GAPP_CallObj    *call_ptr;
    ISI_Id           id;
    ISIP_CallReason  reason;
    GSM_Id           gsmId;
    vint             mediaProfileId;
    ISIP_Call       *c_ptr;

    id = cmd_ptr->id;
    c_ptr = &cmd_ptr->msg.call;
    reason = c_ptr->reason;

    if (reason == ISIP_CALL_REASON_INITIATE) {
        /* Find an available call */
        if (NULL == (call_ptr = GAPP_getCallByIsiId(service_ptr, 0))) {
            /* No available call objects */
            GAPP_callIsiEvt(service_ptr->isiServiceId, id,
                    service_ptr->protocolId, ISIP_CALL_REASON_ERROR,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            return;
        }

        GAPP_dbgPrintf("%s: ISI Call Command reason: INITIATE\n", __FUNCTION__);
        
        /* Notify ISI that we are trying */
        GAPP_callIsiEvt(service_ptr->isiServiceId, id, service_ptr->protocolId,
                ISIP_CALL_REASON_TRYING, ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
        GAPP_sendEvent(evt_ptr);

        /* Cache the remote party */
        OSAL_snprintf(call_ptr->to, sizeof(call_ptr->to), "%s",
                cmd_ptr->msg.call.to);

        GSM_cmdCallMute(&service_ptr->daemon, &gsmId, 0);

        if (service_ptr->extDialCmdEnabled) {
            /* AT+CDU */
            if (0 != (cmd_ptr->msg.call.type & ISI_SESSION_TYPE_VIDEO)) {
                /* Use video media profile */
                mediaProfileId = GSM_MEDIA_PROFILE_ID_VIDEO;
            }
            else {
                /* Otherwise use audio media profile */
                mediaProfileId = GSM_MEDIA_PROFILE_ID_AUDIO;
            }
            GSM_cmdDialUri(&service_ptr->daemon, &call_ptr->gsmId,
                    call_ptr->to, mediaProfileId);
        }
        else {
            /* Standard ATD */
            GSM_cmdDial(&service_ptr->daemon, &call_ptr->gsmId, call_ptr->to,
                    cmd_ptr->msg.call.cidType);
        }
        /* Tell ISI what the vdi number is for this gsmAt call. */
        GAPP_callVdxIsiEvt(service_ptr->isiServiceId, id,
                service_ptr->protocolId, service_ptr->vdi, &evt_ptr->isiMsg);

        call_ptr->state = GAPP_CALL_STATE_MAKE;
        call_ptr->isiCallId = id;
        /* Indicate that this call is NOT an FMC call. */
        call_ptr->isFmc = OSAL_FALSE;
        /* Cache session type. */
        call_ptr->sessionType = cmd_ptr->msg.call.type;
        return;
    }

    /* Command is for an existing call */
    if (NULL == (call_ptr = GAPP_getCallByIsiId(service_ptr, id))) {
        /*
         * Ignore the command,it does not belong to any
         * active phone call
         */
        return;
    }
    switch (reason) {
        case ISIP_CALL_REASON_REJECT:
            _GAPP_callRejectCmd(service_ptr, call_ptr);
            call_ptr->state = GAPP_CALL_STATE_TERM;
            break;
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_FAILED:
            _GAPP_callDisconnectCmd(service_ptr, call_ptr);
            call_ptr->state = GAPP_CALL_STATE_TERM;
            break;
        case ISIP_CALL_REASON_ACCEPT:
            /* Clear proposed */
            _GAPP_callAnswerCmd(service_ptr, call_ptr, evt_ptr, c_ptr);
            call_ptr->state = GAPP_CALL_STATE_ANSWER;
            break;
        case ISIP_CALL_REASON_HOLD:
            call_ptr->state = GAPP_CALL_STATE_HOLD;
            _GAPP_callHoldAnswerCmd(service_ptr, call_ptr, evt_ptr);
            break;
        case ISIP_CALL_REASON_RESUME:
            call_ptr->state = GAPP_CALL_STATE_RESUME;
            _GAPP_callResumeCmd(service_ptr, call_ptr, evt_ptr);
            break;
        case ISIP_CALL_REASON_FORWARD:
            /*
             * Note "Call Deflection" AT command (AT+CTFR) is defined but the
             * ETSI Doc..."Digital cellular telecommunications system
             * (Phase 2+); Call Deflection Service description, Stage 1
             * (GSM 02.72)" has been "withdrawn".  Note that this AT command
             * will disconnect the call and then crash the GSM AT interface.
             * So for now, we will treat a "call forward" as a call terminate.
             */

            /* 
             * GSM_cmdForward(&service_ptr->daemon, &call_ptr->gsmId,
             * cmd_ptr->msg.call.to)
             */
            
            _GAPP_callDisconnectCmd(service_ptr, call_ptr);
            call_ptr->state = GAPP_CALL_STATE_TERM;
            break;
        case ISIP_CALL_REASON_TRANSFER_ATTENDED:
            /*
             * Attended call transfer is not possible with GSM, return failure
             */
            GAPP_callIsiEvt(service_ptr->isiServiceId, id,
                    service_ptr->protocolId, ISIP_CALL_REASON_TRANSFER_FAILED,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            break;
        case ISIP_CALL_REASON_TRANSFER_BLIND:
            
            /*
             * Attempt the transfer and then return.
             * A command to terminate will be coming immediately after this.
             *
             * Currently E28 doesn't support this.  For more information refer
             * to..."Digital cellular telecommunications system (Phase 2+);
             * Explicit Call Transfer (ECT) supplementary service; Stage 3
             * (GSM 04.91 version 7.0.1 Release 1998)", as well as the AT
             * command AT+CHLD
             */
            GSM_cmdBlindTransfer(&service_ptr->daemon, &call_ptr->gsmId,
                    cmd_ptr->msg.call.to);
            GAPP_callIsiEvt(service_ptr->isiServiceId, id,
                    service_ptr->protocolId,
                    ISIP_CALL_REASON_TRANSFER_COMPLETED,
                    ISIP_STATUS_INVALID, &evt_ptr->isiMsg);
            break;
        case ISIP_CALL_REASON_TRANSFER_ATTEMPT:
        case ISIP_CALL_REASON_TRANSFER_PROGRESS:
        case ISIP_CALL_REASON_TRANSFER_COMPLETED:
        case ISIP_CALL_REASON_TRANSFER_FAILED:
            /* These are not supported in GSM */
            break;
        case ISIP_CALL_REASON_MODIFY:
            /* Modidy call */
            _GAPP_callModifyCmd(service_ptr, call_ptr, evt_ptr, c_ptr);
            break;
        case ISIP_CALL_REASON_REJECT_MODIFY:
        case ISIP_CALL_REASON_ACCEPT_MODIFY:
            /* Respond call modification */
            _GAPP_callModifyAnswerCmd(service_ptr, call_ptr, evt_ptr, c_ptr);
            break;
        case ISIP_CALL_REASON_ACK:
        case ISIP_CALL_REASON_TRYING:
        case ISIP_CALL_REASON_CREDENTIALS:
        case ISIP_CALL_REASON_LAST:
            /* These should be no ops */
        default:
            /* No-op, Quietly discard these */
            break;
    } /* End of switch */
    return;
}
#endif
