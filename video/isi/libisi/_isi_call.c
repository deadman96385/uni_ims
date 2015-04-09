/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING 2ICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30168 $ $Date: 2014-12-02 16:40:06 +0800 (Tue, 02 Dec 2014) $
 *
 */
#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_queue.h"
#include "_isi_msg.h"
#include "_isi_call.h"
#include "_isi_dbg.h"

/* 
 * ======== _ISIC_RejectCall() ========
 * This function is used to signal back to the underlying protocol when an 
 * error has been detected with an incoming call request from the protocol.
 * 
 * msg_ptr : A pointer to a ISIP_Message object. This will actually
 *           point to the incoming request, however we rewrite it and reuse
 *           it to write a command back down to the underlying protocol.
 *
 * Returns: 
 *  nothing 
 */
static void _ISIC_RejectCall(
    ISIP_Message *msg_ptr)
{
    /* 
     * Re-write the msg_ptr with an error and send back
     * to service queue 
     */
    ISIM_call(msg_ptr, ISIP_CALL_REASON_REJECT, ISIP_STATUS_INVALID);
    ISIQ_writeProtocolQueue(msg_ptr);
}

/* 
 * ======== _ISIC_notifyCall() ========
 * This function will send a new event through the call FSM via 
 * ISIC_appMsg() function.
 *
 * Returns: 
 *   ISI_RETURN_OK : Call was successfully notified.
 *   ISI_RETURN_FAILED : Call NOT notified, call did not exist.
 */
static ISI_Return _ISIC_notifyCall(
    ISI_Id          callId,
    ISIP_CallReason reason,
    ISIP_Status     status)
{
    ISID_CallId  *call_ptr;
    ISIP_Message *msg_ptr;
    
    if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
        /* Then there is no active call */
        return (ISI_RETURN_FAILED);
    }

    if ((msg_ptr = ISIM_updateCall(call_ptr, reason, status, NULL, NULL)) != 
            NULL) {
        /* This call will free the msg_ptr, so don't worry about it here */
        ISIC_appMsg(call_ptr, msg_ptr);
    }
    return (ISI_RETURN_OK);
}

/* 
 * ======== _ISIC_transfereeCall() ========
 * This function is used to stimulate a new call when the 
 * a remote node tries to transfer.  This new call is the transfer target 
 *
 * Returns: 
 *  ISI_RETURN_FAILED: Could not create a new call 
 *  ISI_RETURN_OK : A call to the transfer target was successfully placed
 */
static ISI_Return _ISIC_transfereeCall(
    ISID_CallId     *call_ptr,
    char            *to_ptr)
{
    ISI_Id            callId;
    ISID_CallId      *c_ptr;
    ISIP_Message     *msg_ptr;

    if ((c_ptr = ISID_callCreate(call_ptr->service_ptr, to_ptr, 
            call_ptr->szSubject, call_ptr->type, call_ptr->cidType, 
            call_ptr->audioDir, call_ptr->videoDir)) == NULL) {
        return (ISI_RETURN_FAILED);
    }
            
    /* Add it to the database.  Init the callId pointer to zero
     * This indicates to the DB that we wish for the DB to determine the 
     * CallId 
     */
    callId = 0;
    if (ISID_callAdd(c_ptr, &callId) == ISI_RETURN_FAILED) {
        /* Free the memory and return */
        ISI_free(c_ptr, ISI_OBJECT_CALL_ID);
        return (ISI_RETURN_FAILED);
    }
            
    /* Now notify the protocol */
    if (ISIM_initiateCall(call_ptr->service_ptr, c_ptr, 
            &msg_ptr) == ISI_RETURN_INVALID_ADDRESS) {
        /* Then there was a problem with the to_ptr field */
        ISID_callDestroy(c_ptr->e.desc);
        return (ISI_RETURN_FAILED);
    }

    /* Set up the call so it knows that it's a transfer attempt */
    /* Let the new call know about the others callId during the transfer */
    if (call_ptr->e.desc2 != 0) {
        c_ptr->e.desc2 = call_ptr->e.desc2;
    }
    else {
        c_ptr->e.desc2 = call_ptr->e.desc;
    }
    c_ptr->transId = call_ptr->e.desc;
    call_ptr->transId = callId;

    /* Change the 'reason' as set in the call to ISIM_initiateCall() */
    msg_ptr->msg.call.reason = ISIP_CALL_REASON_TRANSFER_ATTEMPT;
    if (ISIC_appMsg(c_ptr, msg_ptr) != ISI_RETURN_OK) {
        ISID_callDestroy(c_ptr->e.desc);
        return (ISI_RETURN_FAILED);
    }
    
    return (ISI_RETURN_OK);
}

/* 
 * ======== _ISIC_loadCoders() ========
 * This function is used to copy coder data from one object to another 
 *
 * Returns: 
 *  nothing
 */
static void _ISIC_loadCoders(
    ISIP_Coder    aTo[], 
    ISIP_Coder    aFrom[])
{
    vint x;
    /* save the coders */
    for (x = 0 ; x < ISI_CODER_NUM ; x++) {
        OSAL_strncpy(aTo[x].szCoderName, 
                aFrom[x].szCoderName, sizeof(aTo[x].szCoderName));
        OSAL_strncpy(aTo[x].description, 
                aFrom[x].description, sizeof(aTo[x].description));
        aTo[x].relates = aFrom[x].relates;
    }
}

/* 
 * ======== _ISIC_loadSession() ========
 * This function is used to copy session data from a ISI command 
 * from the underlying protocol side to the call object.
 *
 * Returns: 
 *  nothing
 */
static void _ISIC_loadSession(
    ISID_CallId  *call_ptr, 
    ISIP_Call    *c_ptr)
{
    OSAL_logMsg("_ISIC_loadSession audio dir:%d video dir:%d type:%d "
            "rsrcStatus:%d\n", c_ptr->audioDirection, c_ptr->videoDirection,
            c_ptr->type, c_ptr->rsrcStatus);
    call_ptr->audioDir = c_ptr->audioDirection;
    call_ptr->videoDir = c_ptr->videoDirection;
    call_ptr->videoRtcpFbMask = c_ptr->videoRtcpFbMask;
    call_ptr->type = c_ptr->type;
    call_ptr->rsrcStatus |= c_ptr->rsrcStatus;
    /* save the coders */
    _ISIC_loadCoders(call_ptr->coders, c_ptr->coders); 
    
    /* If the call type indicates security then copy the key's */
    if (c_ptr->type & ISI_SESSION_TYPE_SECURITY_AUDIO) {
        call_ptr->audioKeys = c_ptr->audioKeys;
    }
    
    if(c_ptr->type & ISI_SESSION_TYPE_SECURITY_VIDEO) {
       call_ptr->videoKeys = c_ptr->videoKeys;
    }
    
    ISIG_logCoder(call_ptr->coders, ISI_CODER_NUM);
}

/* 
 * ======== _ISIC_loadLocalRtp() ========
 * This function is used to copy rtp media stream data from a ISI command 
 * from the underlying protocol side to the call object.
 *
 * Returns: 
 *  nothing
 */
static void _ISIC_loadLocalRtp(
    ISID_CallId  *call_ptr, 
    ISIP_Call    *c_ptr)
{
    OSAL_netAddrPortCpy(&call_ptr->rtpAudioLcl.addr, &c_ptr->lclAudioAddr);
    call_ptr->rtpAudioLcl.cntlPort   = c_ptr->lclAudioCntlPort;
    OSAL_netAddrPortCpy(&call_ptr->rtpVideoLcl.addr, &c_ptr->lclVideoAddr);
    call_ptr->rtpVideoLcl.cntlPort   = c_ptr->lclVideoCntlPort;

    if (c_ptr->lclVideoAsBwKbps > 0) {
        call_ptr->rtpVideoLcl.videoAsBwKbps = c_ptr->lclVideoAsBwKbps;
    }
}

/* 
 * ======== _ISIC_loadRemoteRtp() ========
 * This function is used to copy rtp media stream data from a ISI command 
 * from the underlying protocol side to the call object.
 *
 * Returns: 
 *  nothing
 */
static void _ISIC_loadRemoteRtp(
    ISID_CallId  *call_ptr, 
    ISIP_Call    *c_ptr)
{
    OSAL_netAddrPortCpy(&call_ptr->rtpAudioRmt.addr, &c_ptr->rmtAudioAddr);
    call_ptr->rtpAudioRmt.cntlPort   = c_ptr->rmtAudioCntlPort;
    OSAL_netAddrPortCpy(&call_ptr->rtpVideoRmt.addr, &c_ptr->rmtVideoAddr);
    call_ptr->rtpVideoRmt.cntlPort   = c_ptr->rmtVideoCntlPort;

    if (c_ptr->rmtVideoAsBwKbps > 0) {
        call_ptr->rtpVideoRmt.videoAsBwKbps = c_ptr->rmtVideoAsBwKbps;
    }
}

/* 
 * ======== _ISIC_loadRingTemplate() ========
 * This function is used to copy the "ring template" from an ISI  
 * event from the underlying protocol side to the call object.
 *
 * Returns: 
 *  nothing
 */
static void _ISIC_loadRingTemplate(
    ISID_CallId  *call_ptr, 
    ISIP_Call    *c_ptr)
{
    call_ptr->ringTemplate = c_ptr->ringTemplate;
}

/* 
 * ======== ISIC_appMsg() ========
 * This function is the FSM entry point of commands related to 'calls'.
 * These commands come as a result of an API call.
 *
 * Returns: 
 *  ISI_RETURN_OK       : The command successfully passed through the FSM
 *                        and was sent to the underlying protocol.
 *
 *  ISI_RETURN_FAILED   :  The FSM failed the command.  The caller should
 *                        free any memory used to send this command through 
 *                        the FSM.  The command did not get passed to the 
 *                        underlying protocol.
 */
ISI_Return ISIC_appMsg(
    ISID_CallId  *call_ptr, 
    ISIP_Message *msg_ptr)
{
    ISIP_Message   *stream_ptr;
    ISIP_Message   *tone_ptr;
    ISIP_Message   *ring_ptr;
    ISIP_Call      *c_ptr;
    ISI_CallState   state;
    ISIP_CallReason reason;
    ISI_Return      ret;
#ifdef ISI_DEBUG_LOG
    ISI_Id          serviceId = call_ptr->service_ptr->e.desc;
#endif
    
    stream_ptr      = NULL;
    tone_ptr        = NULL;
    ring_ptr        = NULL;
    c_ptr           = &msg_ptr->msg.call;
    state           = call_ptr->state;
    reason          = c_ptr->reason;
    ret             = ISI_RETURN_FAILED;

    ISIG_log((char *)__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logCall(msg_ptr->id, state, reason, serviceId);
    
    if (state == ISI_CALL_STATE_INITIATING) {
        switch (reason) {
        case ISIP_CALL_REASON_TERMINATE:
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                    ISI_TONE_LAST, 0);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            break;
        case ISIP_CALL_REASON_MODIFY:
        case ISIP_CALL_REASON_ACCEPT_MODIFY:
        case ISIP_CALL_REASON_REJECT_MODIFY:
            /* No change in state */
            ret = ISI_RETURN_OK;
            break;
        default:
            break;
        }
    }
    
    
    else if (state == ISI_CALL_STATE_INCOMING) {
        switch(reason) {
        case ISIP_CALL_REASON_ACCEPT:
            ret = ISI_RETURN_OK;

            /*
             * supsrv flags are not sticky and should be cleaned here
             * this moment alert-info and history-info should be all be cleaned
             */
            call_ptr->supsrvHfExist = 0;

            if (call_ptr->isRinging) {
                ring_ptr = ISIM_ring(call_ptr, ISIP_MEDIA_REASON_RINGSTOP);
            }
            else {
                /* stop call waiting tone */
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                        ISI_TONE_LAST, 0);
            }
            /*
             * Local accept the call, just start receiving thre stream but not
             * sending the stream until we get ACCEPT_ACK(the ACK to 200 OK)
             * event from remote.
             */
            call_ptr->audioDir = ISI_SESSION_DIR_RECV_ONLY;
            call_ptr->videoDir = ISI_SESSION_DIR_RECV_ONLY;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTART);
            break;
        case ISIP_CALL_REASON_ACK:
            /* Alert user when receiving ACK from application */

            /* 
             * Check for any active calls in the system, if there are then
             * we want to play a call waiting tone rather than 'ringing'
             */
            if (ISID_serviceCheck4Calls(0, call_ptr) == ISI_RETURN_OK) {
                /* 
                 * GSM is a special case.  Since the GSM network produces it's
                 * own call waiting tone then don't generate the tone 
                 * if both calls (the active and waiting calls) are GSM.
                 */
                if (!(ISI_PROTOCOL_GSM == call_ptr->service_ptr->protocol &&
                        ISID_serviceCheck4Calls(call_ptr->service_ptr->e.desc,
                        call_ptr) == ISI_RETURN_OK)) {
                    tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTART, 
                            ISI_TONE_CALL_WAITING, 0);
                }
            }
            else {
                call_ptr->isRinging = 1;
                ring_ptr = ISIM_ring(call_ptr, ISIP_MEDIA_REASON_RINGSTART);
            }

            /* No state change */
            ret = ISI_RETURN_OK;
            break;
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_REJECT:
        case ISIP_CALL_REASON_FORWARD:
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            if (call_ptr->isRinging) {
                ring_ptr = ISIM_ring(call_ptr, ISIP_MEDIA_REASON_RINGSTOP);
            }
            else {
                /* stop call waiting tone */
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                        ISI_TONE_LAST, 0);
            }
            /* Stop the stream. */
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            break;
        case ISIP_CALL_REASON_MODIFY:
        case ISIP_CALL_REASON_ACCEPT_MODIFY:
        case ISIP_CALL_REASON_REJECT_MODIFY:
            /* No change in state */
            ret = ISI_RETURN_OK;
        default:
            break;
        }
    }
    
    else if (state == ISI_CALL_STATE_ACTIVE) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            break;
        case ISIP_CALL_REASON_HOLD:
            state = ISI_CALL_STATE_ONHOLD;
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);

            /*
             * When commanding MC, if call_ptr direction is NOT SENDRECV, 
             * make the Stream INACTIVE.
             * This will ensure we do not send RTP but will send Null RTCP reports.
             */
            if (ISI_SESSION_DIR_SEND_RECV != 
                    stream_ptr->msg.media.media.stream.audioDirection) {
                /* Do this for Audio. */
                stream_ptr->msg.media.media.stream.audioDirection = 
                        ISI_SESSION_DIR_INACTIVE;
            }
            if (ISI_SESSION_DIR_SEND_RECV != 
                    stream_ptr->msg.media.media.stream.videoDirection) {
                /* Do this for Video. */
                stream_ptr->msg.media.media.stream.videoDirection = 
                        ISI_SESSION_DIR_INACTIVE;
            }
            break;
        case ISIP_CALL_REASON_MODIFY:
            /* No change in state */
            ret = ISI_RETURN_OK;
            /*
             * Don't issue the stream modify yet until we get a
             * ISIP_CALL_REASON_MODIFY_COMPLETED or
             * ISIP_CALL_REASON_MODIFY_FAILED
             */
            /* stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY); */
            break;
        case ISIP_CALL_REASON_TRANSFER_BLIND:
        case ISIP_CALL_REASON_TRANSFER_ATTENDED:
        case ISIP_CALL_REASON_TRANSFER_CONSULT:
        case ISIP_CALL_REASON_TRANSFER_CONF:
            /* 
             * Cache the last state, so we know what state to return to if 
             * the transfer fails.
             */
            call_ptr->previousState = state;
            state = ISI_CALL_STATE_XFERING;
            ret = ISI_RETURN_OK;
            break;
        case ISIP_CALL_REASON_ACCEPT_MODIFY:
            /* No change in state */
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_REJECT_MODIFY:
        case ISIP_CALL_REASON_CONF_KICK:
            /* No change in state */
            ret = ISI_RETURN_OK;
            break;
        default:
            break;
        }
    }

    else if (state == ISI_CALL_STATE_XFERWAIT) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
            /* First kill the other call that is in the XFEREE state */
            _ISIC_notifyCall(call_ptr->transId, ISIP_CALL_REASON_TERMINATE, 
                    ISIP_STATUS_DONE);
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(call_ptr->e.desc);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_TRANSFER_COMPLETED:
            ret = ISI_RETURN_OK;
            break;
        case ISIP_CALL_REASON_TRANSFER_FAILED:
            state = ISI_CALL_STATE_ACTIVE;
            ret = ISI_RETURN_OK;
            break;
        default:
            break;
        }
    }
    
    
    else if (state == ISI_CALL_STATE_ONHOLD) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_FAILED:
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            break;
        case ISIP_CALL_REASON_RESUME:
            state = ISI_CALL_STATE_ACTIVE;
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            /*
             * When commanding MC, if call_ptr direction is NOT SENDRECV, 
             * make the Stream INACTIVE.
             * This will ensure we do not send RTP but will 
             * send Null RTCP reports.
             */
            if (ISI_SESSION_DIR_SEND_RECV != 
                    stream_ptr->msg.media.media.stream.audioDirection) {
                /* Do this for Audio. */
                stream_ptr->msg.media.media.stream.audioDirection = 
                        ISI_SESSION_DIR_INACTIVE;
            }
            if (ISI_SESSION_DIR_SEND_RECV != 
                    stream_ptr->msg.media.media.stream.videoDirection) {
                /* Do this for Video. */
                stream_ptr->msg.media.media.stream.videoDirection = 
                        ISI_SESSION_DIR_INACTIVE;
            }
            break;
        case ISIP_CALL_REASON_TRANSFER_BLIND:
        case ISIP_CALL_REASON_TRANSFER_ATTENDED:
        case ISIP_CALL_REASON_TRANSFER_CONSULT:
        case ISIP_CALL_REASON_TRANSFER_CONF:
            /* 
             * Cache the last state, so we know what state to return to if 
             * the transfer fails.
             */
            call_ptr->previousState = state;
            state = ISI_CALL_STATE_XFERING;
            ret = ISI_RETURN_OK;
        case ISIP_CALL_REASON_ACCEPT_MODIFY:
            /* No change in state */
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_REJECT_MODIFY:
        case ISIP_CALL_REASON_CONF_KICK:
            /* No change in state */
            ret = ISI_RETURN_OK;
            break;
        default:
            break;
        }

    }
    
    
    else if (state == ISI_CALL_STATE_XFERING) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            break;
        default:
            break;
        }
    }

    else if (state == ISI_CALL_STATE_XFEREE) {
        switch (reason) {
        case ISIP_CALL_REASON_TERMINATE:
            state = ISI_CALL_STATE_INVALID;
            ret = ISI_RETURN_OK;
            tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                    ISI_TONE_LAST, 0);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            break;
        default:
            break;
        }
    }

    else if (state == ISI_CALL_STATE_INVALID) {
        switch(reason) {
        case ISIP_CALL_REASON_INITIATE:
            state = ISI_CALL_STATE_INITIATING;
            ret = ISI_RETURN_OK;
            break;
        case ISIP_CALL_REASON_TERMINATE:
            OSAL_logMsg("This should never happen! Something's SCREWED UP!");
            /*  May consider passing up a isi call terminate event. */
            break;
        case ISIP_CALL_REASON_INITIATE_CONF:
            state = ISI_CALL_STATE_INITIATING;
            ret = ISI_RETURN_OK;
            break;
        case ISIP_CALL_REASON_TRANSFER_ATTEMPT:
            /* 
             * Change the 'reason' to call ISIP_CALL_REASON_INITIATE 
             * so the underlying protocol knows to make a new call.
             */
            msg_ptr->msg.call.reason = ISIP_CALL_REASON_INITIATE;
            state = ISI_CALL_STATE_XFEREE;
            ret = ISI_RETURN_OK;
        default:
            break;
        }
        ISIG_logCoder(call_ptr->coders, ISI_CODER_NUM);
    }
    
    if (call_ptr) {
        call_ptr->state = state;
    }

    /* 
     * If the stream pointer to not NULL then we have a command for 
     * the audio controller.
     */
    if (stream_ptr) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(stream_ptr);
        /* make sure the audio_ptr is freed.  It came off the heap */
        ISIM_free(stream_ptr);
    }
    
    /* 
     * If the tone pointer to not NULL then we have a command for 
     * the audio controller 
     */
    if (tone_ptr) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(tone_ptr);
        /* make sure the audio_ptr is freed.  It came off the heap */
        ISIM_free(tone_ptr);
    }
    /* 
     * If the ring pointer to not NULL then we have a 
     * command for the audio controller 
     */
    if (ring_ptr) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(ring_ptr);
        /* make sure the audio_ptr is freed.  It came off the heap */
        ISIM_free(ring_ptr);
    }

    if (ret == ISI_RETURN_OK) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(msg_ptr);
    }

    /* make sure the msg_ptr is freed.  It came off the heap */
    ISIM_free(msg_ptr);
    return (ret);
}

/* 
 * ======== ISIC_protoMsg() ========
 * This function is the FSM entry point for commands that pertain to 
 * 'calls' that COME FROM the underlying protocol.
 * 
 * This function will "quietly discard" any commands that are not allowed.
 *
 * Returns: 
 *  nothing 
 */
void ISIC_protoMsg(
    ISI_Id        callId, 
    ISIP_Message *msg_ptr)
{
    ISID_CallId      *call_ptr;
    ISIP_Call        *c_ptr;
    ISI_CallState     state;
    ISIP_CallReason   reason;
    ISI_EventMessage  event;
    ISID_ServiceId   *service_ptr;
    ISI_Return        ret;
    ISIP_Message     *stream_ptr;
    ISIP_Message     *tone_ptr;
    ISIP_Message     *ring_ptr;
    ISIP_Status       status;
    
    c_ptr           = &msg_ptr->msg.call;
    reason          = c_ptr->reason;
    status          = c_ptr->status;

    if (ISID_lockCalls() != ISI_RETURN_OK) {
        return;
    }

    stream_ptr = NULL;
    tone_ptr = NULL;
    ring_ptr = NULL;

    /* IMPORTANT!  For incoming calls the callId is the handle to the UA */
    if (reason == ISIP_CALL_REASON_INITIATE) {
        /* Then this is a new call, so create it.  Note how the descriptor is 
         * the handle to the UA 
         */
        ret = ISID_serviceGet(c_ptr->serviceId, &service_ptr);
        if (ret == ISI_RETURN_FAILED || ret == ISI_RETURN_SERVICE_NOT_ACTIVE) {
            /* This service ID does not exist or has not been activated. */
            ISIG_log((char *)__FUNCTION__, "Failed to find a active service\n", 
                    0, 0 ,0);
            _ISIC_RejectCall(msg_ptr);
            ISID_unlockCalls();
            return;
        }
        else {
            /* All is well so create the call */
            if ((call_ptr = ISID_callCreate(service_ptr, c_ptr->from, 
                    c_ptr->subject, c_ptr->type, c_ptr->cidType, 
                    c_ptr->audioDirection, c_ptr->videoDirection)) == NULL) {
                ISIG_log((char *)__FUNCTION__, 
                        "Failed to create a new call\n", 0, 0 ,0);
                _ISIC_RejectCall(msg_ptr);
                ISID_unlockCalls();
                return;
            }
            /* copy supsrv info */
            call_ptr->supsrvHfExist = c_ptr->supsrvHfExist;
            OSAL_strncpy(call_ptr->historyInfo, c_ptr->historyInfo, 
                    ISI_HISTORY_INFO_STRING_SZ);
            /* Add the call to the database.  
             * Note that the callId value was set to the ST UA handle */
            if (ISID_callAdd(call_ptr, &callId) == ISI_RETURN_FAILED) {
                /* There's an error so free the memory and simply return */
                ISIG_log((char *)__FUNCTION__, 
                        "Failed to add a new call to the database\n", 0, 0 ,0);
                _ISIC_RejectCall(msg_ptr);
                ISI_free(call_ptr, ISI_OBJECT_CALL_ID);
                ISID_unlockCalls();
                return;
            }
        }
    }
    else {
        if (ISID_callGet(callId, &call_ptr) != ISI_RETURN_OK) {
            /* Call ID is not valid */
            ISIG_log((char *)__FUNCTION__, 
                    "Failed to find the call for this event. callId:%d\n",
                    callId, 0 ,0);
            ISID_unlockCalls();
            return;
        }
    }
    
    state = call_ptr->state;
    
    if (call_ptr->e.desc2 != 0) {
        /* Then use this */
        event.id = call_ptr->e.desc2;
    }
    else {
        event.id = call_ptr->e.desc;
    }
    event.serviceId = call_ptr->service_ptr->e.desc;
    event.idType    = ISI_ID_TYPE_CALL;
    event.event     = ISI_EVENT_NONE;
    event.eventDesc[0] = 0;
    if (0 != c_ptr->reasonDesc[0]) {
        OSAL_strncpy(event.eventDesc, c_ptr->reasonDesc,
                ISI_EVENT_DESC_STRING_SZ);
    }

    /* Record SRVCC status if any. */
    if (0 != c_ptr->srvccStatus) {
        call_ptr->srvccStatus |= c_ptr->srvccStatus;
    }

    ISIG_log((char *)__FUNCTION__, 0, 0, 0 ,0);
    ISIG_logCall(callId, state, reason, c_ptr->serviceId);
    
    if (state == ISI_CALL_STATE_INITIATING) {
        switch (reason) {
        case ISIP_CALL_REASON_ACCEPT:
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);

            /*
             * supsrv flags are not sticky and should be cleaned here
             * this moment alert-info and history-info should be all be cleaned
             */
            call_ptr->supsrvHfExist = 0;

            if (call_ptr->isRinging) {
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                        ISI_TONE_LAST, 0);
            }
            /*
             * Remote accepts the call, start receiving the stream but not
             * sending the stream until we get ACCEPT_ACK(the ACK to 200 OK)
             * event from underlying protocol.
             */
            if (ISI_SESSION_DIR_SEND_RECV == call_ptr->audioDir) {
                call_ptr->audioDir = ISI_SESSION_DIR_RECV_ONLY;
            }
            if (ISI_SESSION_DIR_SEND_RECV == call_ptr->videoDir) {
                call_ptr->videoDir = ISI_SESSION_DIR_RECV_ONLY;
            }
            /* Tell the application */
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_ACK:
            /* No state change */
            event.event = ISI_EVENT_CALL_ACKNOWLEDGED;
            
            /* 180 ringing may contain alert-info and we ORed the flag here */
            call_ptr->supsrvHfExist |= c_ptr->supsrvHfExist;
            
            /* 
             * This tone is for 'ring back'.  We track it's use in isRinging
             * and then 
             * only issue tone stop if we issued tone start for the ringback 
             */
            call_ptr->isRinging = 1;
            tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTART, 
                    ISI_TONE_RINGBACK, 0);
            break;
        case ISIP_CALL_REASON_BEING_FORWARDED:
            /* No state change */
            event.event = ISI_EVENT_CALL_BEING_FORWARDED;
            break;
        case ISIP_CALL_REASON_MODIFY:
            event.event = ISI_EVENT_CALL_MODIFY;
            /* No change in state but save all the code info */
            OSAL_strncpy(call_ptr->szRemoteUri, c_ptr->from, 
                    ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(call_ptr->szSubject, c_ptr->subject, 
                    ISI_ADDRESS_STRING_SZ);

            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_EARLY_MEDIA:
            event.event = ISI_EVENT_CALL_EARLY_MEDIA;
            break;
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_REJECT:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_CALL_REJECTED;
            if (call_ptr->isRinging) {
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                        ISI_TONE_LAST, 0);
            }
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_CREDENTIALS:
        case ISIP_CALL_REASON_ERROR:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_NET_UNAVAILABLE;
            if (call_ptr->isRinging) {
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP,
                        ISI_TONE_LAST, 0);
            }
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_FAILED:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_CALL_FAILED;
            if (call_ptr->isRinging) {
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                        ISI_TONE_LAST, 0);
            }
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_TRYING:
            /* No state change just cache the local IP address info */
            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            /* Update any change to the call type */
            call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
            call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
            event.event = ISI_EVENT_CALL_TRYING;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTART); 
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF:
            /* The message will have a target VDN/VDI routing number */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF;
            break;
        case ISIP_CALL_REASON_ACCEPT_ACK:
            /* copy supsrv info */
            call_ptr->supsrvHfExist = c_ptr->supsrvHfExist;
            OSAL_strncpy(call_ptr->historyInfo, c_ptr->historyInfo, 
                    ISI_HISTORY_INFO_STRING_SZ);
            /*
             * Got ACCEPT_ACK from underlying protocol.
             * Load the session's direction and set call state to ACTIVE.
             */
            state = ISI_CALL_STATE_ACTIVE;
            event.event = ISI_EVENT_CALL_ACCEPTED;
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_RTP_INACTIVE_TMR_DISABLE:
            event.event = ISI_EVENT_CALL_RTP_INACTIVE_TMR_DISABLE;
            stream_ptr = ISIM_stream(call_ptr,
                    ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_DISABLE);
            break;
        case ISIP_CALL_REASON_RTP_INACTIVE_TMR_ENABLE:
            event.event = ISI_EVENT_CALL_RTP_INACTIVE_TMR_ENABLE;
            stream_ptr = ISIM_stream(call_ptr,
                    ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_ENABLE);
            break;
        default:
            break;
        }
    }

    if (state == ISI_CALL_STATE_XFEREE) {
        switch (reason) {
        case ISIP_CALL_REASON_ACCEPT:
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);
            tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                    ISI_TONE_LAST, 0);
            /*
             * Remote accepts the call, start receiving the stream but not
             * sending the stream until we get ACCEPT_ACK(the ACK to 200 OK)
             * event from underlying protocol.
             */
            if (ISI_SESSION_DIR_SEND_RECV == call_ptr->audioDir) {
                call_ptr->audioDir = ISI_SESSION_DIR_RECV_ONLY;
            }
            if (ISI_SESSION_DIR_SEND_RECV == call_ptr->videoDir) {
                call_ptr->videoDir = ISI_SESSION_DIR_RECV_ONLY;
            }
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_ACK:
            /* No state change */
            tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTART, 
                    ISI_TONE_RINGBACK, 0);
            break;
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_FAILED:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_REJECT:
        case ISIP_CALL_REASON_CREDENTIALS:
            /* Notify the other call of the status */
            if (_ISIC_notifyCall(call_ptr->transId,
                    ISIP_CALL_REASON_TRANSFER_FAILED, ISIP_STATUS_DONE) != 
                    ISI_RETURN_OK) {
                /* 
                 * Then that means that the other call did not exist, he 
                 * probably already disconnected. Notify App of the terminated
                 * call.
                 */
                event.event = ISI_EVENT_CALL_DISCONNECTED;
            }
            state = ISI_CALL_STATE_INVALID;
            tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                    ISI_TONE_LAST, 0);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_TRYING:
            /* No state change just cache the local IP address info */
            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
            call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTART); 
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF:
            /* The message will have a target VDN/VDI routing number */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF;
            break;
        case ISIP_CALL_REASON_ACCEPT_ACK:
            /*
             * Got ACCEPT_ACK from underlying protocol.
             * Load the session's direction and set call state to ACTIVE.
             */
            state = ISI_CALL_STATE_ACTIVE;
            /* Notify the other call of the status */
            _ISIC_notifyCall(call_ptr->transId, 
                    ISIP_CALL_REASON_TRANSFER_COMPLETED, ISIP_STATUS_DONE); 
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
        default:
            break;
        }
    }
    else if (state == ISI_CALL_STATE_INCOMING) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_FAILED:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_CALL_DISCONNECTED;
            if (call_ptr->isRinging) {
                ring_ptr = ISIM_ring(call_ptr, ISIP_MEDIA_REASON_RINGSTOP);
            }
            else {
                /* stop call waiting tone */
                tone_ptr = ISIM_tone(call_ptr, ISIP_MEDIA_REASON_TONESTOP, 
                        ISI_TONE_LAST, 0);
            }
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF:
            /* The message will have a target VDN/VDI routing number */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF;
            break;
        case ISIP_CALL_REASON_ACCEPT_ACK:
            /*
             * Got ACCEPT_ACK from underlying protocol.
             * Load the session's direction and set call state to ACTIVE.
             */
            state = ISI_CALL_STATE_ACTIVE;
            /* Send event to notify the call is active. */
            event.event = ISI_EVENT_CALL_ACCEPT_ACK;
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            break;
        case ISIP_CALL_REASON_MODIFY:
            if (status == ISIP_STATUS_FAILED) {
                /*
                 * Do nothing to the stream, the modified settings
                 * did not take affect, so pump an event and do nothing else
                 */
                event.event = ISI_EVENT_CALL_MODIFY_FAILED;
                break;
            }

            OSAL_strncpy(call_ptr->szRemoteUri, c_ptr->from, 
                     ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(call_ptr->szSubject, c_ptr->subject, 
                     ISI_ADDRESS_STRING_SZ);

            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);

            if (status == ISIP_STATUS_DONE) {
                event.event = ISI_EVENT_CALL_MODIFY_COMPLETED;
                stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            }
            else {
                /*
                 * Do nothing to the stream.
                 * Modify stream when accept/reject call modify.
                 */
                event.event = ISI_EVENT_CALL_MODIFY;
            }
            break;
        default:
            break;
        }
    }
    else if (state == ISI_CALL_STATE_ACTIVE) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_FAILED:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_CALL_DISCONNECTED;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_CANCEL_MODIFY:
            event.event = ISI_EVENT_CALL_CANCEL_MODIFY;
            break;
        case ISIP_CALL_REASON_MODIFY:
            if (status == ISIP_STATUS_FAILED) {
                /*
                 * Do nothing to the stream, the modified settings
                 * did not take affect, so pump an event and do nothing else
                 */
                event.event = ISI_EVENT_CALL_MODIFY_FAILED;
                break;
            }

            OSAL_strncpy(call_ptr->szRemoteUri, c_ptr->from, 
                     ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(call_ptr->szSubject, c_ptr->subject, 
                     ISI_ADDRESS_STRING_SZ);

            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);

            if (status == ISIP_STATUS_DONE) {
                event.event = ISI_EVENT_CALL_MODIFY_COMPLETED;
                stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMMODIFY);
            }
            else {
                /*
                 * Do nothing to the stream.
                 * Modify stream when accept/reject call modify.
                 */
                event.event = ISI_EVENT_CALL_MODIFY;
            }
            break;
        case ISIP_CALL_REASON_HOLD:
            if (status == ISIP_STATUS_DONE) {
                event.event = ISI_EVENT_CALL_HOLD;
            }
            break;
        case ISIP_CALL_REASON_RESUME:
            if (status == ISIP_STATUS_DONE) {
                event.event = ISI_EVENT_CALL_RESUME;
            }
            break;
        case ISIP_CALL_REASON_TRANSFER_ATTEMPT:
            /* We are being requested to be transferred */
            if (_ISIC_transfereeCall(call_ptr, c_ptr->to) != ISI_RETURN_OK) {
                /* Then refuse this transfer */
                ISIM_call(msg_ptr, ISIP_CALL_REASON_TRANSFER_FAILED, 
                        ISIP_STATUS_DONE);
                ISIQ_writeProtocolQueue(msg_ptr);
            }
            else {
                /* Move the state to the XFERWAIT */
                state = ISI_CALL_STATE_XFERWAIT;
                event.event = ISI_EVENT_CALL_XFER_REQUEST;
                /* Make sure this guy can't hear your audio */
                call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
                call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
                stream_ptr = ISIM_stream(call_ptr,
                        ISIP_MEDIA_REASON_STREAMMODIFY);
            }
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF_SUCCESS:
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF_SUCCESS;
            break;
       case ISIP_CALL_REASON_HANDOFF_FAILED:
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF_FAILED;
            break;
        case ISIP_CALL_REASON_VIDEO_REQUEST_KEY:
            event.event = ISI_EVENT_CALL_VIDEO_REQUEST_KEY;
            break;
        default:
            break;
        }
    }
    else if (state == ISI_CALL_STATE_XFERWAIT) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_FAILED:
            state = ISI_CALL_STATE_INVALID;
            /* event.event = ISI_EVENT_CALL_DISCONNECTED; */
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_MODIFY:
            if (status == ISIP_STATUS_FAILED) {
                /*
                 * Do nothing to the stream, the modified settings
                 * did not take affect, so pump an event and do nothing else
                 */
                event.event = ISI_EVENT_CALL_MODIFY_FAILED;
                break;
            }
            if (status == ISIP_STATUS_DONE) {
                event.event = ISI_EVENT_CALL_MODIFY_COMPLETED;
            }
            else {
                event.event = ISI_EVENT_CALL_MODIFY;
            }

            /*
             * Don't update MC for now.
             * MC should be updated when accept/reject
             * the call modify from app.
             */
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF:
            /* The message will have a target VDN/VDI routing number */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF;
            break;
        default:
            break;
        }
    }
    else if (state == ISI_CALL_STATE_ONHOLD) {
        switch(reason) {
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_CALL_DISCONNECTED;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_HOLD:
            if (status == ISIP_STATUS_DONE) {
                /* Then the hold was successful */
                event.event = ISI_EVENT_CALL_HOLD;
            }
            break;
        case ISIP_CALL_REASON_TRANSFER_ATTEMPT:
            /* We are being requested to be transferred */
            if (_ISIC_transfereeCall(call_ptr, c_ptr->to) != ISI_RETURN_OK) {
                /* Then refuse this transfer */
                ISIM_call(msg_ptr, ISIP_CALL_REASON_TRANSFER_FAILED, 
                        ISIP_STATUS_DONE);
                ISIQ_writeProtocolQueue(msg_ptr);
            }
            else {
                /* Move the state to the XFERWAIT */
                state = ISI_CALL_STATE_XFERWAIT;
                event.event = ISI_EVENT_CALL_XFER_REQUEST;
            }
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF:
            /* The message will have a target VDN/VDI routing number */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF;
            break;
        case ISIP_CALL_REASON_MODIFY:
            /* Remote modify the call on HOLD state */
            OSAL_strncpy(call_ptr->szRemoteUri, c_ptr->from, 
                     ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(call_ptr->szSubject, c_ptr->subject, 
                     ISI_ADDRESS_STRING_SZ);

            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);
            _ISIC_loadSession(call_ptr, c_ptr);
            /*
             * Do nothing to the stream.
             * Modify stream when accept/reject call modify.
             */
            event.event = ISI_EVENT_CALL_MODIFY;
            break;
        default:
            break;
        }
    }
    else if (state == ISI_CALL_STATE_XFERING) {
        switch (reason) {
        case ISIP_CALL_REASON_TERMINATE:
        case ISIP_CALL_REASON_ERROR:
        case ISIP_CALL_REASON_FAILED:
            state = ISI_CALL_STATE_INVALID;
            event.event = ISI_EVENT_CALL_DISCONNECTED;
            stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTOP);
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        case ISIP_CALL_REASON_TRANSFER_FAILED:
            state = call_ptr->previousState;
            event.event = ISI_EVENT_CALL_XFER_FAILED;
            break;
        case ISIP_CALL_REASON_TRANSFER_COMPLETED:
            state = call_ptr->previousState;
            event.event = ISI_EVENT_CALL_XFER_COMPLETED;
            break;
        case ISIP_CALL_REASON_TRANSFER_PROGRESS:
            event.event = ISI_EVENT_CALL_XFER_PROGRESS;
            break;
        case ISIP_CALL_REASON_VDX:
            /* Cache the VDN/VDI target routing number. */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            break;
        case ISIP_CALL_REASON_HANDOFF:
            /* The message will have a target VDN/VDI routing number */
            OSAL_strncpy(call_ptr->szTargetUri, msg_ptr->msg.call.to,
                ISI_ADDRESS_STRING_SZ);
            /* Tell the APP */
            event.event = ISI_EVENT_CALL_HANDOFF;
            break;
        default:
            break;
        }
    }
    else if (state == ISI_CALL_STATE_INVALID) {
        switch(reason) {
        case ISIP_CALL_REASON_INITIATE:
            if (c_ptr->srvccStatus & ISI_SRVCC_STATUS_INCOMING_NOTIFY) {
                state = ISI_CALL_STATE_ACTIVE;
            }
            else {
                state = ISI_CALL_STATE_INCOMING;
                call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
                call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
            }
            event.event = ISI_EVENT_CALL_INCOMING;

            OSAL_strncpy(call_ptr->szRemoteUri, c_ptr->from, 
                    ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(call_ptr->szSubject, c_ptr->subject, 
                    ISI_SUBJECT_STRING_SZ);
            call_ptr->supsrvHfExist = c_ptr->supsrvHfExist;
            OSAL_strncpy(call_ptr->historyInfo, c_ptr->historyInfo, 
                    ISI_HISTORY_INFO_STRING_SZ);

            _ISIC_loadSession(call_ptr, c_ptr);
            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);

            _ISIC_loadRingTemplate(call_ptr, c_ptr);
            /* stream_ptr = ISIM_stream(call_ptr, ISIP_MEDIA_REASON_STREAMSTART); */
            break;
        case ISIP_CALL_REASON_INITIATE_CONF:
            if (c_ptr->srvccStatus & ISI_SRVCC_STATUS_INCOMING_NOTIFY) {
                state = ISI_CALL_STATE_ACTIVE;
            }
            else {
                state = ISI_CALL_STATE_INCOMING;
                call_ptr->audioDir = ISI_SESSION_DIR_INACTIVE;
                call_ptr->videoDir = ISI_SESSION_DIR_INACTIVE;
            }
            event.event = ISI_EVENT_CALL_INCOMING;

            OSAL_strncpy(call_ptr->szRemoteUri, c_ptr->from, 
                    ISI_ADDRESS_STRING_SZ);
            OSAL_strncpy(call_ptr->szSubject, c_ptr->subject, 
                    ISI_SUBJECT_STRING_SZ);
            call_ptr->supsrvHfExist = c_ptr->supsrvHfExist;
            OSAL_strncpy(call_ptr->historyInfo, c_ptr->historyInfo, 
                    ISI_HISTORY_INFO_STRING_SZ);

            _ISIC_loadSession(call_ptr, c_ptr);
            _ISIC_loadLocalRtp(call_ptr, c_ptr);
            _ISIC_loadRemoteRtp(call_ptr, c_ptr);

            _ISIC_loadRingTemplate(call_ptr, c_ptr);
            break;
        /* 
         * These transfer handlers are here for blind transfer. We went 
         * directly to the invalid state from the blind transfer.  But, we may
         * still get a transfer "PROGRESS" message etc...before we terminate 
         * the call.
         */
        case ISIP_CALL_REASON_TRANSFER_FAILED:
            event.event = ISI_EVENT_CALL_XFER_FAILED;
            break;
        case ISIP_CALL_REASON_TRANSFER_COMPLETED:
            event.event = ISI_EVENT_CALL_XFER_COMPLETED;
            break;
        case ISIP_CALL_REASON_TRANSFER_PROGRESS:
            event.event = ISI_EVENT_CALL_XFER_PROGRESS;
            break;
        case ISIP_CALL_REASON_TERMINATE:
            event.event = ISI_EVENT_CALL_DISCONNECTED;
            ISID_callDestroy(callId);
            call_ptr = NULL;
            break;
        default:
            break;
        }
    }
    
    if (event.event != ISI_EVENT_NONE) {
        /* Tell the application */
        ISIQ_writeAppQueue((char*)&event, sizeof(ISI_EventMessage));
    }

    if (call_ptr) {
        call_ptr->state = state;
    }
    /* If the stream pointer to not NULL then we have a 
     * command for the audio controller 
     */
    if (stream_ptr) {
        /* Tell the application */

        ISIQ_writeProtocolQueue(stream_ptr);

        /* make sure the audio_ptr is freed.  It came off the heap */
        ISIM_free(stream_ptr);
    }
    /* If the tone pointer to not NULL then we have a 
     * command for the audio controller 
     */
    if (tone_ptr) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(tone_ptr);
        /* make sure the audio_ptr is freed.  It came off the heap */
        ISIM_free(tone_ptr);
    }
    /* If the ring pointer to not NULL then we have a 
     * command for the audio controller 
     */
    if (ring_ptr) {
        /* Tell the application */
        ISIQ_writeProtocolQueue(ring_ptr);
        /* make sure the audio_ptr is freed.  It came off the heap */
        ISIM_free(ring_ptr);
    }
    ISID_unlockCalls();
    return;
}

