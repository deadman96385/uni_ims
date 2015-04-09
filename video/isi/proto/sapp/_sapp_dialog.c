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

#include <auth_b64.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>
#include <sip_session.h>

#include "isi.h"
#include "isi_errors.h"
#include "isip.h"

#include "mns.h"

#ifdef INCLUDE_SIMPLE
#include "xcap.h"
#include "_simple_types.h"
#endif

#include "_sapp.h"
#include "_sapp_parse_helper.h"
#include "_sapp_coder_helper.h"
#include "_sapp_dialog.h"
#include "_sapp_xml.h"
#include "_sapp_emergency.h"
#include "_sapp_conf.h"
#include "_sapp_ipsec.h"
#include "_sapp_reg.h"

/*
 * ======== _SAPP_sipGetRtpInfc() ========
 * This function is used to get the next available IP Address and port
 * that can be used for a new call. The range of IP ports seen here are
 * set during initialization time.
 *
 * Return Values:
 *  Nothing
 */
static void _SAPP_sipGetRtpInfc(
    SAPP_RtpInfc    *rtp_ptr,
    OSAL_NetAddress *addr_ptr,
    uint16          *rtcpPort_ptr)
{
    *addr_ptr = rtp_ptr->nextInfc;
    /* RTP/RTCP ports are x&x+1 */
    
    /* uncomment below to enable RTCP */
    *rtcpPort_ptr = rtp_ptr->nextInfc.port+1;
    /* *rtcpPort_ptr = 0; */

    /* Advance the port by 2 to advance over rtp & rtcp */
    
    /* uncomment below to enable RTCP */
    rtp_ptr->nextInfc.port += 2;
    /* rtp_ptr->nextInfc.port += 1; */

    if (rtp_ptr->nextInfc.port >= rtp_ptr->endPort) {
        /* Then roll it over */
        rtp_ptr->nextInfc.port = rtp_ptr->startPort;
    }
    return;
}

/*
 * ========_SAPP_populateIsiEvtSession()========
 * This function will populate an ISI event with details regarding a 'session'
 * (a.k.a. the media stream details of a call).
 *
 * Returns:
 *   Nothing.
 */
void  _SAPP_populateIsiEvtSession(
    MNS_ServiceObj  *service_ptr,
    MNS_SessionObj  *mns_ptr,
    ISIP_Call       *isi_ptr)
{
    int                  media;
    tMedia              *media_ptr;
    MNS_SecurityKeys    *audioKeys_ptr;
    MNS_SecurityKeys    *videoKeys_ptr;
    vint                 x;
    vint                 y;
    tSession            *lclSess_ptr;
    
    isi_ptr->type = ISI_SESSION_TYPE_NONE;

    /* Update resource status */
    isi_ptr->rsrcStatus = mns_ptr->rsrcStatus;

    lclSess_ptr = &mns_ptr->session;
    /*
     * Load key, addr, rtp port and rtcp port.
     */
    audioKeys_ptr = &mns_ptr->audioKeys;
    videoKeys_ptr = &mns_ptr->videoKeys;

    y = 0;
    for (media = 0; media < lclSess_ptr->numMedia; media++) {

        media_ptr = &lclSess_ptr->media[media];
        
        switch (media_ptr->mediaType) {
            case eSdpMediaAudio:
                isi_ptr->type |= ISI_SESSION_TYPE_AUDIO;
                 /* Load direction */
                isi_ptr->audioDirection = _SAPP_mapDirSip2Isi(media_ptr->lclDirection);
                
                if (0 != media_ptr->useSrtp) {
                    isi_ptr->type |= ISI_SESSION_TYPE_SECURITY_AUDIO;
                    /* Copy the keys */
                    isi_ptr->audioKeys.type = audioKeys_ptr->type;
                    if (audioKeys_ptr->type & SRTP_AES_80) {
                        OSAL_memCpy(isi_ptr->audioKeys.lclKey, 
                                audioKeys_ptr->lclAes80, 
                                ISI_SECURITY_KEY_STRING_SZ);
                        OSAL_memCpy(isi_ptr->audioKeys.rmtKey,
                                audioKeys_ptr->rmtAes80,
                                ISI_SECURITY_KEY_STRING_SZ);
                    }
                    else if (audioKeys_ptr->type & SRTP_AES_32) {
                        OSAL_memCpy(isi_ptr->audioKeys.lclKey, 
                                audioKeys_ptr->lclAes32, 
                                ISI_SECURITY_KEY_STRING_SZ);
                        OSAL_memCpy(isi_ptr->audioKeys.rmtKey, 
                                audioKeys_ptr->rmtAes32, 
                                ISI_SECURITY_KEY_STRING_SZ);
                    }
                }
                /* populate remote ip address and port info */
                _SAPP_addrSip2Sapp(&lclSess_ptr->rmtAddr, &isi_ptr->rmtAudioAddr);
                isi_ptr->rmtAudioAddr.port = media_ptr->rmtRtpPort;
                isi_ptr->rmtAudioCntlPort = media_ptr->rmtRtcpPort;

                _SAPP_addrSip2Sapp(&lclSess_ptr->lclAddr, &isi_ptr->lclAudioAddr);
                isi_ptr->lclAudioAddr.port = media_ptr->lclRtpPort;
                isi_ptr->lclAudioCntlPort = service_ptr->aRtcpPort;

                break;
            case eSdpMediaVideo:
                if (0 == media_ptr->rmtRtpPort ||
                        (0 == media_ptr->lclRtpPort)) {
                    break;
                }
                isi_ptr->type |= ISI_SESSION_TYPE_VIDEO;
                
                 /* Load direction */
                isi_ptr->videoDirection = _SAPP_mapDirSip2Isi(media_ptr->lclDirection);

                if (0 != media_ptr->useSrtp) {
                    isi_ptr->type |= ISI_SESSION_TYPE_SECURITY_VIDEO;
                    /* Copy the keys */
                    isi_ptr->videoKeys.type = videoKeys_ptr->type;
                    if (videoKeys_ptr->type & SRTP_AES_80) {
                        OSAL_memCpy(isi_ptr->videoKeys.lclKey, 
                                videoKeys_ptr->lclAes80,
                                ISI_SECURITY_KEY_STRING_SZ);
                        OSAL_memCpy(isi_ptr->videoKeys.rmtKey,
                                videoKeys_ptr->rmtAes80,
                                ISI_SECURITY_KEY_STRING_SZ);
                    }
                    else if (videoKeys_ptr->type & SRTP_AES_32) {
                        OSAL_memCpy(isi_ptr->videoKeys.lclKey,
                                videoKeys_ptr->lclAes32,
                                ISI_SECURITY_KEY_STRING_SZ);
                        OSAL_memCpy(isi_ptr->videoKeys.rmtKey,
                                videoKeys_ptr->rmtAes32,
                                ISI_SECURITY_KEY_STRING_SZ);
                    }
                }

                if (media_ptr->useAVPF) {
                    if (media_ptr->use_FB_NACK) {
                        /* Use RTCP-FB. Generic NACK. */
                        isi_ptr->videoRtcpFbMask |=  ISIP_MASK_RTCP_FB_NACK;
                    }
                    if (media_ptr->use_FB_PLI) {
                        /* Use RTCP-FB. PLI. */
                        isi_ptr->videoRtcpFbMask |=  ISIP_MASK_RTCP_FB_PLI;
                    }
                    if (media_ptr->use_FB_FIR) {
                        /* Use RTCP-FB. FIR. */
                        isi_ptr->videoRtcpFbMask |=  ISIP_MASK_RTCP_FB_FIR;
                    }
                    if (media_ptr->use_FB_TMMBR) {
                        /* Use RTCP-FB. TMMBR. */
                        isi_ptr->videoRtcpFbMask |=  ISIP_MASK_RTCP_FB_TMMBR;
                    }
                    if (media_ptr->use_FB_TMMBN) {
                        /* Use RTCP-FB. TMMBN. */
                        isi_ptr->videoRtcpFbMask |=  ISIP_MASK_RTCP_FB_TMMBN;
                    }
                }

                if (media_ptr->rmtAsBwKbps > 0) {
                    isi_ptr->rmtVideoAsBwKbps = media_ptr->rmtAsBwKbps;
                }

                /* populate remote ip address and port info */
                _SAPP_addrSip2Sapp(&lclSess_ptr->rmtAddr, &isi_ptr->rmtVideoAddr);
                isi_ptr->rmtVideoAddr.port = media_ptr->rmtRtpPort;
                isi_ptr->rmtVideoCntlPort = media_ptr->rmtRtcpPort;

                _SAPP_addrSip2Sapp(&lclSess_ptr->lclAddr, &isi_ptr->lclVideoAddr);
                isi_ptr->lclVideoAddr.port = media_ptr->lclRtpPort;
                isi_ptr->lclVideoCntlPort = service_ptr->vRtcpPort;

                break;
            default:
                break;
        }

        /* Load the coders */
        for (x = 0 ; x < media_ptr->numCoders &&
                y < ISI_CODER_NUM ; x++) {
            /* Turn sip/sdp coder descriptions into isi coder descriptions */

            /* Use 'x' as the priority */
            if (_SAPP_encodeCoder(&isi_ptr->coders[y],
                    &media_ptr->aCoders[x],
                    media_ptr->packetRate) == SAPP_OK) {
                /* Then ISI values were successfully written */
                y++;
            }
        }
    }
}

/*
 * =======_SAPP_updateMediaCoder() =======
 * Renew the media coders of the call.
 *
 */
void _SAPP_updateMediaCoder(
    SAPP_CallObj    *call_ptr)
{
    tSession    *sess_ptr;
    int         x, y;

    /* Remove unused new media that is create by re-invite. */
    sess_ptr = &call_ptr->mnsSession.session;
    for (x = 0; x < sess_ptr->numMedia; ++x) {
        for (y = 0; y < ISI_CODER_NUM; y++) {
            if (sess_ptr->media[x].mediaType ==
                    call_ptr->coders[y].relates) {
                break;
            }
        }
        if (ISI_CODER_NUM == y) {
            /*
             * Cannot find the type of coder same as this
             * media type. Therefore remove this media and
             * change other media position.
             */
            for (y = x; y < (sess_ptr->numMedia - 1); y++) {
                OSAL_memCpy(&sess_ptr->media[y],
                    &sess_ptr->media[y+1], sizeof(tMedia));
            }
            sess_ptr->numMedia--;
            x--;
            continue;
        }
    }
}

/*
 * ======== SAPP_sipCallIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * related to "UA" calls. These events will be passed from SAPP
 * to the ISI module.
 *
 * Returns:
 * Nothing.
 */
void SAPP_sipCallIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              callId,
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
    return;
}

/*
 * ======== SAPP_sipHandoff() ========
 * This function is called when a remote device has signalled via SIP
 * that a service and any active calls associated with the service should
 * perform a VCC handoff.  This function will generate events to ISI
 * notifying ISI of the handoff requests.
 *
 * Return Values:
 *  Nothing
 *
 */
void SAPP_sipHandoff(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event         *evt_ptr)
{
    vint x;

    ISIP_Message *isi_ptr;

    isi_ptr = &evt_ptr->isiMsg;

    /* First send events for any active calls */
    for (x = 0; x < SAPP_CALL_NUM; x++) {
        if (service_ptr->sipConfig.aCall[x].isiCallId != 0) {
            /* We have an active call */
            isi_ptr->msg.call.type = ISI_SESSION_TYPE_AUDIO;
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    service_ptr->sipConfig.aCall[x].isiCallId,
                    ISIP_CALL_REASON_HANDOFF, ISIP_STATUS_INVALID, isi_ptr);
            OSAL_snprintf(isi_ptr->msg.call.to, ISI_ADDRESS_STRING_SZ, "%s",
                    service_ptr->sipConfig.aCall[x].vccHandoffAddr);
            SAPP_sendEvent(evt_ptr);
        }
    }

    /* Now send the service 'signal' event */
    isi_ptr->id = service_ptr->isiServiceId;
    isi_ptr->code = ISIP_CODE_SERVICE;
    isi_ptr->protocol = service_ptr->protocolId;
    isi_ptr->msg.service.reason = ISIP_SERVICE_REASON_HANDOFF;
    isi_ptr->msg.service.status = ISIP_STATUS_INVALID;
    return;
}

/*
 * ======== _SAPP_populateEarlyMedia() ========
 * Check if need to send early media event to ISI.
 * There are two condition that will populate ISI event.
 *
 * Returns:
 *
 */
void _SAPP_populateEarlyMedia(
    SAPP_ServiceObj    *service_ptr,
    tUaAppEvent        *uaEvt_ptr,
    SAPP_CallObj       *call_ptr,
    SAPP_Event         *evt_ptr)
{
    /* case 1:
     * If there is no P-Early-Media header and there is SDP and
     * precondition is met, notify early media.
     */
    if ((!SAPP_parseHfExist(SAPP_P_EARLY_MEDIA_HF, uaEvt_ptr)) &&
            (MNS_isPreconditionMet(&call_ptr->mnsSession))) {
        /* Notify early media */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_EARLY_MEDIA, ISIP_STATUS_INVALID,
                &evt_ptr->isiMsg);
        SAPP_sendEvent(evt_ptr);
    }
    /* case 2:
     * If there is P-Early-Media and the direction is sendrev or sendonly,
     * notify ISI early media.
     */
    if (SAPP_parseHfValueExist(SAPP_P_EARLY_MEDIA_HF,
            _SAPP_DIR_SNEDREV_ARG, uaEvt_ptr) ||
            SAPP_parseHfValueExist(SAPP_P_EARLY_MEDIA_HF,
            _SAPP_DIR_SENDONLY_ARG, uaEvt_ptr)) {
        /* Notify early media */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_EARLY_MEDIA, ISIP_STATUS_INVALID,
                &evt_ptr->isiMsg);
        SAPP_sendEvent(evt_ptr);
    }
}

/*
 * ======== _SAPP_sipUnhold() ========
 * Send out an INVITE to the other end asking it to
 * start the flow of RTP.
 * Returns:
 * 0  : If success.
 * -1 : If failed.
 */
vint _SAPP_sipUnhold(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    ISIP_Message     *cmd_ptr)
{
    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    if (SIP_OK != UA_ModifyCall(
            service_ptr->sipConfig.uaId, call_ptr->dialogId,
            NULL, NULL, 0, call_ptr->mnsSession.sess_ptr,
            &service_ptr->sipConfig.localConn)) {
        SAPP_dbgPrintf("%s: Could not send re-invite for resume\n",
                __FUNCTION__);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}

/*
 * ======== _SAPP_rsrcRsrvTimerCb() ========
 * Resource reservation timer callback funtion.
 * This function is to send a timeout event to sapp to terminate the call.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
static int32 _SAPP_rsrcRsrvTimerCb(
    void *arg_ptr)
{
    SAPP_CallObj   *call_ptr = (SAPP_CallObj*)arg_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(call_ptr->event.isiEvt,
            (char *)&call_ptr->event.isiMsg,
            sizeof(ISIP_Message), OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED qId=%p\n", __FUNCTION__,
                __LINE__, call_ptr->event.isiEvt);
        /* Create retry timer to send message again */
        if (0 == call_ptr->rsrcRsrvRetryTimerId) {
            if (0 == (call_ptr->rsrcRsrvRetryTimerId = OSAL_tmrCreate())) {
                /* Timer create failed */
                OSAL_logMsg("%s:%d Create retry sending msg timer failed.\n",
                        __FUNCTION__, __LINE__);
                return (SAPP_ERR);
            }
            else {
                /* Timer exists, stop it */
                OSAL_tmrStop(call_ptr->rsrcRsrvRetryTimerId);
            }
        }
        /* Now start the timer */
        if (OSAL_SUCCESS != OSAL_tmrStart(call_ptr->rsrcRsrvRetryTimerId,
                _SAPP_rsrcRsrvTimerCb, call_ptr, SAPP_MESSAGE_RETRY_TIMER_MS)) {
            OSAL_tmrDelete(call_ptr->rsrcRsrvRetryTimerId);
            call_ptr->rsrcRsrvRetryTimerId = 0;
            return (SAPP_ERR);
        }
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipRsrcRsrvTimerStart() ========
 * Start resource reservation timer.
 * This timer is to prevent we stuck in the resource reservation state without any server update.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
vint _SAPP_sipRsrcRsrvTimerStart(
    SAPP_ServiceObj  *service_ptr,
    SAPP_SipObj      *sip_ptr,
    SAPP_CallObj     *call_ptr)
{
    ISIP_Message    *isi_ptr;

    if (0 == call_ptr->rsrcRsrvTimerId) {
        if (0 == (call_ptr->rsrcRsrvTimerId = OSAL_tmrCreate())) {
            /* Timer create failed */
            return (SAPP_ERR);
        }
    }
    else {
        /* Timer exists, stop it */
        OSAL_tmrStop(call_ptr->rsrcRsrvTimerId);
    }

    /* Constrcut the event */
    call_ptr->event.isiEvt = sip_ptr->event.isiEvt;

    isi_ptr = &call_ptr->event.isiMsg;

    SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
            call_ptr->isiCallId, ISIP_CALL_REASON_TERMINATE,
            ISIP_STATUS_INVALID, isi_ptr);

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrStart(call_ptr->rsrcRsrvTimerId,
            _SAPP_rsrcRsrvTimerCb, call_ptr, SAPP_RSRC_RSRV_TIMER_MS)) {
        OSAL_tmrDelete(call_ptr->rsrcRsrvTimerId);
        call_ptr->rsrcRsrvTimerId = 0;
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipRsrcRsrvTimerStop() ========
 * Stop resource reservation timer.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
vint _SAPP_sipRsrcRsrvTimerStop(
    SAPP_CallObj     *call_ptr)
{
    if ((0 == call_ptr->rsrcRsrvTimerId) &&
            (0 == call_ptr->rsrcRsrvRetryTimerId)) {
        /* Timer doesn't exist. */
        return (SAPP_ERR);
    }

    if (0 != call_ptr->rsrcRsrvTimerId) {
        OSAL_tmrStop(call_ptr->rsrcRsrvTimerId);
        OSAL_tmrDelete(call_ptr->rsrcRsrvTimerId);
        call_ptr->rsrcRsrvTimerId = 0;
    }

    if (0 != call_ptr->rsrcRsrvRetryTimerId) {
        OSAL_tmrStop(call_ptr->rsrcRsrvRetryTimerId);
        OSAL_tmrDelete(call_ptr->rsrcRsrvRetryTimerId);
        call_ptr->rsrcRsrvRetryTimerId = 0;
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipHold() ========
 * Send out an INVITE to the other end asking it to
 * stop the flow of RTP.
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
vint _SAPP_sipHold(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    ISIP_Message     *cmd_ptr)
{
    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    if (SIP_OK != UA_ModifyCall(service_ptr->sipConfig.uaId,
            call_ptr->dialogId, NULL, NULL, 0, call_ptr->mnsSession.sess_ptr,
            &service_ptr->sipConfig.localConn)) {
        SAPP_dbgPrintf("%s: Could not send re-invite for hold\n",
                __FUNCTION__);
        return (SAPP_ERR);
    }
    return (SAPP_OK);
}


/*
 * ======== _SAPP_sipModify() ========
 *
 * This function is called when ISI has commanded SAPP to modify the media
 * stream of the call.
 *
 * This function will stimulate a call to the appropriate SIP Stack API call
 * used to "modify" a call.
 *
 * Returns:
 *   SAPP_ERR : The SIP Stack API call for modifying the call was
 *              successfully issued.
 *
 *   SAPP_OK : The SIP Stack's API call for modifying the call was
 *             successfully issued and a SIP "Re-INVITE was sent.
 */
vint _SAPP_sipModify(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    ISIP_Message     *cmd_ptr,
    ISIP_Message     *isi_ptr)
{
    ISIP_Call *c_ptr;
    vint       useUpdate;
    char      *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint       numHdrFlds;

    numHdrFlds = 0;
    /* Determine is the modify acceptance is for Re-Invite or Update */
    if (MNS_isSessionEarly(&call_ptr->mnsSession)) {
        if (!MNS_isSessionEarlyIdle(&call_ptr->mnsSession)) {
            /* Offer sent or received. */
            SAPP_dbgPrintf("%s: Glare case, offer sent or received.\n",
                    __FUNCTION__);
            return (SAPP_ERR);
        }
        useUpdate = 1;
    }
    else {
        useUpdate = 0;
    }

    c_ptr = &cmd_ptr->msg.call;

    /* Let's prepare the details of the media for this invite */
    if (SAPP_OK != _SAPP_encodeMediaSapp2Sip(service_ptr, call_ptr, c_ptr)) {
        /*
         * Then the RTP interface is not available, return error
         */
        SAPP_dbgPrintf("%s: Could not send re-invite or update. Rtp issue.\n",
                __FUNCTION__);
        return (SAPP_ERR);
    }

    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    /* Send 580 Precondition failure if resource failure */
    if (ISI_RESOURCE_STATUS_FAILURE & c_ptr->rsrcStatus) {
        UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 580, 0, 0,
                    NULL, NULL, 0, call_ptr->mnsSession.sess_ptr, 0);
            /*
             * In case remote doesn't cancel the call, we have to terminate call
             * by ourselves.
             */

            /* Tell ISI that the call is disconnected */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_TERMINATE, ISIP_STATUS_INVALID, isi_ptr);
            SAPP_sipDestroyCall(call_ptr);
        return (SAPP_OK);
    }

    /* Prepare Reason header field if we have value in reasonDesc. */
    if ((NULL != c_ptr->reasonDesc) && (0 != c_ptr->reasonDesc[0])) {
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ,
                "%s %s", SAPP_REASON_HF, c_ptr->reasonDesc);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    /* Modify the call */
    if (useUpdate) {
        if (SIP_OK != UA_UpdateCall(
                service_ptr->sipConfig.uaId,
                call_ptr->dialogId, NULL, hdrFlds_ptr,
                numHdrFlds, call_ptr->mnsSession.sess_ptr,
                &service_ptr->sipConfig.localConn)) {
            SAPP_dbgPrintf("%s: Could not send update.\n",
                    __FUNCTION__);
            return (SAPP_ERR);
        }
    }
    else {
        if (SIP_OK != UA_ModifyCall(
                service_ptr->sipConfig.uaId,
                call_ptr->dialogId, NULL, hdrFlds_ptr,
                numHdrFlds, call_ptr->mnsSession.sess_ptr,
                &service_ptr->sipConfig.localConn)) {
            SAPP_dbgPrintf("%s: Could not send re-invite\n",
                    __FUNCTION__);
            return (SAPP_ERR);
        }
    }
    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipAccpetModify() ========
 *
 * This function is called when ISI has commanded SAPP to modify the media
 * stream of the call.
 *
 * This function will stimulate a call to the appropriate SIP Stack API call
 * used to "modify" a call.
 *
 * Returns:
 *   SAPP_ERR : The SIP Stack API call for modifying the call was
 *              successfully issued.
 *
 *   SAPP_OK : The SIP Stack's API call for modifying the call was
 *             successfully issued and a SIP "Re-INVITE was sent.
 */
vint _SAPP_sipAcceptModify(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    ISIP_Message     *cmd_ptr)
{
    ISIP_Call *c_ptr;
    vint       useUpdate;
    vint       respCode;
    char      *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint       numHdrFlds;

    numHdrFlds = 0;
    c_ptr = &cmd_ptr->msg.call;

    if (!MNS_isOfferReceived(&call_ptr->mnsSession)) {
        /* Do nothing if SDP answer is sent. */
        return (SAPP_OK);
    }

    /*
     * Determine it's a response to UDAPTE or INVITE by
     * if the session is in early state.
     */
    if (MNS_isSessionEarly(&call_ptr->mnsSession)) {
        useUpdate = 1;
        if (ISI_RESOURCE_STATUS_NOT_READY == c_ptr->rsrcStatus) {
            /* It's a modify accept for 183, no respose needed for it */
            return (SAPP_OK);
        }
    }
    else {
        useUpdate = 0;
    }


    /* Let's prepare the details of the media for this invite */
    if (SAPP_OK != _SAPP_encodeMediaSapp2Sip(service_ptr, call_ptr, c_ptr)) {
        /*
         * Then the RTP interface is not available, return error
         */
        SAPP_dbgPrintf("%s: Could not send 200 OK. Rtp issue.\n",
                __FUNCTION__);
        return (SAPP_ERR);
    }

    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    /* Modify the call */
    if (useUpdate) {
        /* Send 580 Precondition failure if resource failure */
        if (ISI_RESOURCE_STATUS_FAILURE & c_ptr->rsrcStatus) {
            respCode = 580;
        }
        else {
            respCode = 200;
        }
        /* Send response to Update */
        UA_UpdateResp(service_ptr->sipConfig.uaId, call_ptr->dialogId, respCode,
                NULL, NULL, 0, call_ptr->mnsSession.sess_ptr,
                &service_ptr->sipConfig.localConn);
    }
    else {
        /* Send 200 OK to Re-Invite */
        UA_Answer(service_ptr->sipConfig.uaId, call_ptr->dialogId, hdrFlds_ptr, 
                numHdrFlds, call_ptr->mnsSession.sess_ptr,
                service_ptr->exchangeCapabilitiesBitmap,
                &service_ptr->sipConfig.localConn);
    }
    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipForwardCall() ========
 *
 * This function is called when ISI has commanded SAPP to forward an
 * incoming/inbound call attempt.
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_sipForwardCall(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       dialogId,
    char            *to_ptr)
{
    char *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint  numHdrFlds;

    numHdrFlds = 0;
    /* Set up the "Contact" header field */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
        SAPP_STRING_SZ, "Contact: %s", to_ptr);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    SAPP_dbgPrintf("%s: Incoming call forward to %s\n",
            __FUNCTION__, to_ptr);

    UA_Respond(service_ptr->sipConfig.uaId, dialogId, 302, 0, 0, NULL,
            hdrFlds_ptr, numHdrFlds, NULL, 0);
    return;
}

/*
 * ========_SAPP_sipHungUp() ========
 * This is functiuon is call to send BYE to peer
 */
vint _SAPP_sipHungUp(
    SAPP_ServiceObj    *service_ptr,
    tSipHandle          hDialog,
    char               *reasonDesc_ptr)
{
    char           *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint            numHdrFlds;

    numHdrFlds = 0;

    if ((NULL != reasonDesc_ptr) && (0 != reasonDesc_ptr[0])) {
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ,
                "%s SIP ;text=\"%s\"", SAPP_REASON_HF, reasonDesc_ptr);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }
    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }
   
    return (UA_HungUp(service_ptr->sipConfig.uaId, hDialog, hdrFlds_ptr, numHdrFlds,
            &service_ptr->sipConfig.localConn));
}

/*
 * ======== _SAPP_sipAnswerCall() ========
 *
 * This function is called when ISI has commanded SAPP to answer the call.
 *
 * Return Values:
 *  Nothing
 */
void _SAPP_sipAnswerCall(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    ISIP_Message     *cmd_ptr)
{
    char            *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint             numHdrFlds;

    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession,
            cmd_ptr);

    /* Add the session timer if enalbe force session timer. */
    numHdrFlds = 0;
    if (0 != service_ptr->sipConfig.forceSessionTimer[0] &&
            0 != OSAL_strcasecmp(service_ptr->sipConfig.forceSessionTimer,
            "default")) {
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "Session-Expires: %s",
                service_ptr->sipConfig.forceSessionTimer);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }
    UA_Answer(service_ptr->sipConfig.uaId, call_ptr->dialogId, hdrFlds_ptr,
            numHdrFlds, call_ptr->mnsSession.sess_ptr,
            service_ptr->exchangeCapabilitiesBitmap,
            &service_ptr->sipConfig.localConn);
}

/* 
 * ======== _SAPP_sipInviteRetryTimerCb() ========
 *
 * This function is the callback function for INVITE retry timer timeout.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
static int32 _SAPP_sipInviteRetryTimerCb(
    void  *arg_ptr)
{
    SAPP_TmrEvent     tmrEvt;
    SAPP_CallObj     *call_ptr = (SAPP_CallObj*) arg_ptr;

    /* Setup the event to send */
    tmrEvt.type = SAPP_TMR_EVENT_INVITE_RETRY;
    tmrEvt.arg_ptr = call_ptr;

    if (OSAL_SUCCESS != OSAL_msgQSend(
            call_ptr->tmrEvtQ, (char *)&tmrEvt, sizeof(SAPP_TmrEvent),
            OSAL_NO_WAIT, NULL)) {
        OSAL_logMsg("%s:%d ERROR msgQ send FAILED\n", __FUNCTION__, __LINE__);
        /* Create retry timer to send message again */
        if (0 == call_ptr->retryAfterMsgTmrId) {
            if (0 == (call_ptr->retryAfterMsgTmrId = OSAL_tmrCreate())) {
                /* Timer create failed */
                OSAL_logMsg("%s:%d Create retry sending msg timer failed.\n",
                        __FUNCTION__, __LINE__);
                return (SAPP_ERR);
            }
            else {
                /* Timer exists, stop it */
                OSAL_tmrStop(call_ptr->retryAfterMsgTmrId);
            }
        }
        /* Now start the timer */
        if (OSAL_SUCCESS != OSAL_tmrStart(call_ptr->retryAfterMsgTmrId,
                _SAPP_sipInviteRetryTimerCb, call_ptr,
                SAPP_MESSAGE_RETRY_TIMER_MS)) {
            OSAL_tmrDelete(call_ptr->retryAfterMsgTmrId);
            call_ptr->retryAfterMsgTmrId = 0;
            return (SAPP_ERR);
        }
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/* 
 * ======== _SAPP_sipInviteRetryTmrStart() ========
 *
 * This function is to start INVITE request retry timer.
 *
 * Returns:
 *   SAPP_OK  : Process done.
 *   SAPP_ERR : Error in start timer.
 */
vint _SAPP_sipInviteRetryTmrStart(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    uint32 timeout)
{
    if (0 == call_ptr->retryAfterTimerId) {
        if (0 == (call_ptr->retryAfterTimerId = OSAL_tmrCreate())) {
            /* Timer create failed */
            OSAL_logMsg("%s:%d ERROR timer create FAILED\n",
                    __FUNCTION__, __LINE__);
            return (SAPP_ERR);
        }
    }
    else {
        /* Timer exists, stop it */
        OSAL_tmrStop(call_ptr->retryAfterTimerId);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrStart(call_ptr->retryAfterTimerId,
            _SAPP_sipInviteRetryTimerCb, call_ptr, timeout * 1000)) {
        OSAL_tmrDelete(call_ptr->retryAfterTimerId);
        call_ptr->retryAfterTimerId = 0;
        OSAL_logMsg("%s:%d ERROR timer start FAILED\n",
                __FUNCTION__, __LINE__);
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipInviteRetryTmrStop() ========
 * This function is to stop INVITE request retry timer.
 *
 * Returns:
 * SAPP_OK  : If success.
 * SAPP_ERR : If failed.
 */
vint _SAPP_sipInviteRetryTmrStop(
 SAPP_CallObj     *call_ptr)
{
    if ((0 == call_ptr->retryAfterTimerId) &&
            (0 == call_ptr->retryAfterMsgTmrId)) {
        /* Timer doesn't exist. */
        return (SAPP_ERR);
    }

    if (0 != call_ptr->retryAfterTimerId) {
        OSAL_tmrStop(call_ptr->retryAfterTimerId);
        OSAL_tmrDelete(call_ptr->retryAfterTimerId);
        call_ptr->retryAfterTimerId = 0;
    }

    if (0 != call_ptr->retryAfterMsgTmrId) {
        OSAL_tmrStop(call_ptr->retryAfterMsgTmrId);
        OSAL_tmrDelete(call_ptr->retryAfterMsgTmrId);
        call_ptr->retryAfterMsgTmrId = 0;
    }

    return (SAPP_OK);
}

/*
 * ======== SAPP_sipDestroyCall() ========
 *
 * This function will mark a call as destroyed by clearing the
 * 'isiCallId' value (The ID used by ISI to specify a call) and
 * the 'dialogId' value (The ID/handle SIP uses to specify a SIP call).
 *
 * Returns:
 *   Nothing.
 */
void SAPP_sipDestroyCall(
    SAPP_CallObj *call_ptr)
{
    call_ptr->isiCallId = 0;
    call_ptr->dialogId = 0;
    call_ptr->blockCid = 0;
    call_ptr->useAlias = OSAL_FALSE;
    call_ptr->modify = SAPP_MODIFY_NONE;
    /* Clear conf struct */
    call_ptr->conf.dialogId = 0;
    call_ptr->conf.identity[0] = 0;
    /* Stop resource reservation timer anyway */
    _SAPP_sipRsrcRsrvTimerStop(call_ptr); 
    MNS_clearSession(&call_ptr->mnsSession);
    _SAPP_sipInviteRetryTmrStop(call_ptr);
    return;
}

/*
 * ========_SAPP_sipSendPrack() ========
 * This is functiuon is call to send PRACK to peer
 */
vint _SAPP_sipSendPrack(
    SAPP_ServiceObj *service_ptr,
    tSipHandle       hDialog)
{
    char  *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint   numHdrFlds;

    numHdrFlds = 0;

    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }

    return (UA_Prack(service_ptr->sipConfig.uaId, hDialog, hdrFlds_ptr, numHdrFlds,
            NULL, &service_ptr->sipConfig.localConn));
}

/*
 * ======== _SAPP_sipMakeCall() ========
 *
 * This function is called when a call needs to be placed over SIP.
 * It will call the SIP API function for placing a call.
 *
 * added pDisplayName to allow dynamic changing it for each call.
 * Vic1 use it for cnap. But other usage should be possible and make this generic
 * parameter for _SAPP_sipMakeCall
 *
 * Returns:
 *  SAPP_OK : The call was successfully placed and the call_ptr->dialogId
 *            will be correctly populated with a handle to the SIP dialog.
 *  SAPP_ERR : Function failed.  The SIP API failed.
 */
vint _SAPP_sipMakeCall(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    char            *to_ptr,
    tSession        *sess_ptr,
    char            *pDisplayName,
    uint32          *srvccStatus)
{
    char           *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint            numHdrFlds;
    char           *from_ptr;
    char           *displayName_ptr;
    OSAL_Boolean    keep;
    vint            capsBitmap;

    keep       = OSAL_FALSE;
    numHdrFlds = 0;

    /* Add P-Preferred-Identity header field */
    if (1 == call_ptr->isEmergency && 0 !=
            service_ptr->emergencyObj.imeiUri[0] && !OSAL_strcmp(
            service_ptr->sipConfig.config.aor[0].szUri,
            SAPP_ANONYMOUS_CALL_ID_URI)) {
        /* Emergency anonymous call ,put imei uri to P-Preferred-Identity */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF,
                service_ptr->emergencyObj.imeiUri);
    }
    else {
        /* See if use alias uri. */
        if (call_ptr->useAlias && (0 != service_ptr->aliasUriList[0][0])) {
            OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                    SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF,
                    service_ptr->aliasUriList[0]);
        }
        else {
            OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                    SAPP_STRING_SZ, "%s <%s>", SAPP_P_PREFERRED_ID_HF,
                    service_ptr->sipConfig.config.aor[0].szUri);
        }
    }

    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;


    /*
     * Add a 'preconfigured route' if the registrar reported one during the
     * registration process
     */
    if (SAPP_OK == SAPP_sipAddPreconfiguredRoute(&service_ptr->registration,
            &service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ)) {
        /* Then a "Route" header field was added */
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    if (0 != service_ptr->sipConfig.sessionTimer[0] &&
            0 != OSAL_strcasecmp(service_ptr->sipConfig.sessionTimer, "default")) {
        /*
         * Then let's use session timers and we will specify the times
         * duration
         */
        /*
         * Add the minimum session timer durationm.
         * Make it the default of 90 seconds per rfc4028
         */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "Min-SE: %s", "90");
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        /* Add the session timer */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "Session-Expires: %s",
                service_ptr->sipConfig.sessionTimer);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    /* Block Cid */
    if (call_ptr->blockCid) {
        /* Then we block the caller Id */

        /* Set up the "Privacy" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "%s %s", SAPP_PRIVACY_HF, "id");
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
        from_ptr = SAPP_ANONYMOUS_CALL_ID_URI;
        displayName_ptr = SAPP_ANONYMOUS_CALL_ID;
    }
    else {
        /*
         * Setting these to NULL will force SIP to use the default info
         * which is the AOR and Display name set when the UA was
         * created via UA_Create().
         */
        from_ptr = NULL;
        displayName_ptr = pDisplayName;
        /* Set up the "Privacy" header field */
        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "%s %s", SAPP_PRIVACY_HF, "none");
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    /* 
     * Set up the "Accept-Contact" header field containing the 
     * g.3gpp.icsi-ref media feature tag with an ICSI value.
     */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
            SAPP_STRING_SZ, "%s *;%s=\"%s\"", SAPP_ACCEPT_CONTACT_HF,
            SIP_CAPS_ARG_ICSI_STR, SIP_CAPS_IP_VOICE_CALL_STR);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;
    /*Add Accept*/
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_ACCEPT_HF, _SAPP_3GPP_ACCEPT_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Add P-Early-Media */
    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], SAPP_STRING_SZ, "%s %s",
            SAPP_P_EARLY_MEDIA_HF, _SAPP_SUPPORTED_ARG);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /*
     * Setting from as tel uri, if this call is emergency and there is tel uri.
     */
    if ((service_ptr->isEmergency) && (NULL != service_ptr->telUri_ptr) &&
            (0 != service_ptr->telUri_ptr[0])) {
        from_ptr = service_ptr->telUri_ptr;
    }
    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
        call_ptr->useIpSec = 1;
    }

    /*
     * If this service is emergency and non-register,
     * set keep-alive depend on system configuration.
     */
    if ((service_ptr->isEmergency) && (0 ==
            service_ptr->sipConfig.config.szRegistrarProxy[0])) {
        keep = service_ptr->natKeepaliveEnable;
    }

    capsBitmap = service_ptr->exchangeCapabilitiesBitmap |
            service_ptr->srvccCapabilitiesBitmap;

    /* Record SRVCC capabilities. */
    if (service_ptr->srvccCapabilitiesBitmap & eSIP_CAPS_SRVCC_ALERTING) {
        *srvccStatus |= ISI_SRVCC_STATUS_ALERTING_SEND;
    }
    if (service_ptr->srvccCapabilitiesBitmap & eSIP_CAPS_SRVCC_MID_CALL) {
        *srvccStatus |= ISI_SRVCC_STATUS_MID_CALL_SEND;
    }

    call_ptr->dialogId = UA_MakeCall(service_ptr->sipConfig.uaId, to_ptr,
            from_ptr, displayName_ptr, hdrFlds_ptr, numHdrFlds, sess_ptr,
            capsBitmap, &service_ptr->sipConfig.localConn, keep);
    if (call_ptr->dialogId != 0) {
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}

/*
 * ======== _SAPP_getErrorReasonDesc ========
 *
 * This function is to prepare call error code/description to ISIP_Call reasonDesc.
 * The format is: "CALL FAILED: CODE:%d REASON:%s"
 *
 * uaEvt_ptr : A pointer to a tUaAppEvent object that is populated with all the
 *       details of the event.  Please see SIP interface document for details
 *       regarding the values in this object.
 * c_ptr : A pointer to a ISIP_Call where we will fill the reasonDesc
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_getErrorReasonDesc(
    tUaAppEvent *uaEvt_ptr,
    ISIP_Call   *c_ptr)
{
    char *reason_ptr = NULL;
    vint  respCode = SIP_RSP_CODE_UNKNOWN;

    if (NULL != uaEvt_ptr) {
        /* Let's see if there's a reason phrase */
        reason_ptr = SAPP_parseHfValue(SAPP_REASON_HF, uaEvt_ptr);
        respCode = uaEvt_ptr->resp.respCode;
    }

    if (SIP_RSP_CODE_XACT_TIMEOUT == respCode) {
        respCode = ISI_REQUEST_TIMED_OUT;
        reason_ptr = ISI_REQUEST_TIMED_OUT_STR;
    }
    else if (SIP_RSP_CODE_ACK_TIMEOUT == respCode) {
        respCode = ISI_ACK_RECEIPT_TIMED_OUT;
        reason_ptr = ISI_ACK_RECEIPT_TIMED_OUT_STR;
    }
    else if (SIP_RSP_CODE_INTERNAL_ERROR == respCode) {
        respCode = ISI_NO_AVAILABLE_RESOURCES;
        reason_ptr = ISI_NO_AVAILABLE_RESOURCES_STR;
    }
    else if (SIP_RSP_CODE_UNKNOWN == respCode) {
        respCode = ISI_NO_NETWORK_AVAILABLE;
        reason_ptr = ISI_NO_NETWORK_AVAILABLE_STR;
    }
    else if (NULL == reason_ptr) {
        reason_ptr = (char*)SAPP_GetResponseReason(respCode);
    }

    OSAL_snprintf(c_ptr->reasonDesc, ISI_EVENT_DESC_STRING_SZ,
            "CALL FAILED: CODE:%d REASON:%s", respCode, reason_ptr);
    return;
}

/*
 * ======== _SAPP_appendIncomingReasonDesc ========
 *
 * This function is to append key/value to ISIP_Call reasonDesc for incoming event.
 * The format is similar to JSON. For e.g.
 * 'ALIAS: 1234567, CWI:true, CNAP:"name 123", PRIORITY:1'
 *
 * c_ptr : A pointer to a ISIP_Call where we will append the key/value.
 * key_ptr : key string
 * value_ptr : value string
 *
 * Returns:
 *   SAPP_OK : appended ok
 *   SAPP_ERR: exceed reasonDesc max length or other error
 */
vint _SAPP_appendIncomingReasonDesc(
    ISIP_Call   *c_ptr,
    char        *key_ptr,
    char        *value_ptr)
{
    vint curPos;
    
    curPos = OSAL_strlen(c_ptr->reasonDesc);
    if ((OSAL_strlen(key_ptr) + OSAL_strlen(value_ptr)) >=
            (ISI_EVENT_DESC_STRING_SZ-curPos)) {
        return SAPP_ERR;
    }
    OSAL_snprintf(&c_ptr->reasonDesc[curPos], ISI_EVENT_DESC_STRING_SZ-curPos,
            ", %s:%s", key_ptr, value_ptr);
    return SAPP_OK;
}

/*
 * ======== _SAPP_sipIsAlternativeServiceEmgcy ========
 *
 * This function is to check if the alternative service is emergency.
 *
 * arg : A pointer to a tUaAppEvent object that is populated with all the
 *       details of the event.  Please see SIP interface document for details
 *       regarding the values in this object.
 *
 * Returns:
 *   SAPP_OK : The alternative service is emergency.
 *   SAPP_ERR: The alternative service is not emergency.
 */
vint _SAPP_sipIsAlternativeServiceEmgcy(
    SAPP_ServiceObj *service_ptr,
    tUaAppEvent     *uaEvt_ptr,
    char           **reason_ptr)
{
    char                *value_ptr;
    SAPP_3gppImsAction   xmlAction;

    /* Check if there is xml doc in SIP message */
    if ((NULL == uaEvt_ptr->msgBody.payLoad.data) ||
            (0 == uaEvt_ptr->msgBody.payLoad.length)) {
        return (SAPP_ERR);
    }

    /* Parse the Content-Type and message body (XML doc) */
    if (NULL == (value_ptr =
            SAPP_parseHfValue(SAPP_CONTENT_TYPE_HF, uaEvt_ptr))) {
        return (SAPP_ERR);
    }
    
    /* Check if it's application/3gpp-ims+xml */
    if (0 != OSAL_strncasecmp(value_ptr, _SAPP_3GPP_IMS_ARG,
            sizeof(_SAPP_3GPP_IMS_ARG) - 1)) {
        return (SAPP_ERR);
    }

    /* Decode xml doc */
    if (SAPP_OK != _SAPP_xmlDecode3gppImsDoc(
            uaEvt_ptr->msgBody.payLoad.data,
            uaEvt_ptr->msgBody.payLoad.length,
            reason_ptr,
            &xmlAction)) {
        /* Decode failed */
        return (SAPP_ERR);
    }
  
    /* Check if emergency registration is required. */
    if (SAPP_XML_ACTION_EMERGENCY_REG != xmlAction) {
        return (SAPP_ERR);
    }    
    return (SAPP_OK);
}




/*
 * ======== _SAPP_sipConfAddParty() ========
 *
 * This function is called when ISI has commanded SAPP to add a participant to 
 * an existing conference call via REFER
 * This function is used to implement the 3gpp requirement:
 *      24.147 5.3.1.5.3 User invites other user to a conference by sending a 
 *      REFER request to the conference focus
 *
 * Returns:
 *  SAPP_OK : Refer issued
 *  SAPP_ERR : Function failed.  The SIP API failed.
 */
vint _SAPP_sipConfAddParty(
    SAPP_ServiceObj  *service_ptr,
    SAPP_CallObj     *call_ptr,
    ISIP_Call        *c_ptr)
{
    char        *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint        numHdrFlds;

    numHdrFlds = 0;

    SAPP_dbgPrintf("%s: _SAPP_sipConfAddParty pTransferTarget %s\n",
            __FUNCTION__, c_ptr->to);

    /*
    Refer-To: passed as pTransferTarget to SIP UA, from parameter c_ptr->to
    Request URI: this conf-call dialog contact
    method: INVITE
    Referred-By: done in SIP UA
    */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0], SAPP_LONG_STRING_SZ,
            "%s %s", SAPP_PRIVACY_HF, "none");
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    if (UA_TransferCall(service_ptr->sipConfig.uaId, call_ptr->dialogId,
            c_ptr->to, NULL, hdrFlds_ptr, numHdrFlds,
            &service_ptr->sipConfig.localConn, eSIP_INVITE,
            NULL) != SIP_OK) {
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}

/*
 * ======== _SAPP_sipCallInitiateConf(() ========
 *
 * This function is called when ISI has commanded SAPP to initiate an
 * outbound conf call.
 * This is used to implement the 24.147 5.3.1.5.4 invite with URI list
 *
 * service_ptr : A pointer to the service that this call is being placed
 *               within.
 * call_ptr : A pointer to a SAPP_CallObj object used internally by
 *            SAPP to manage the call.
 *
 * cmd_ptr : A pointer to the command issued by ISI.  This object contains
 *           all the details of the command.
 *
 * isi_ptr : A pointer to a buffer where ISI events are written.  Events
 *          written here all ultimately sent up to the ISI module.
 *
 * Returns:
 *   SAPP_OK : Function was successful.  Call was placed.
 *   SAPP_ERR : Function failed.  The call could not be placed.
 *              STUN may have failed or the UA_MakeCall() API failed.
 */
vint _SAPP_sipCallInitiateConf(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message    *cmd_ptr,
    SAPP_Event      *evt_ptr)
{
    ISIP_Call       *c_ptr;
    ISIP_Message    *isi_ptr = &evt_ptr->isiMsg;
    char            *phoneContext_ptr;

    tSession       *sess_ptr;
    char           *target_ptr;
    vint            targetLen;
    char            number[13] = {0};
    vint            len;
    
    /*  setup the call object */
    call_ptr->isiCallId = cmd_ptr->id;
    call_ptr->modify = SAPP_MODIFY_NONE;
    /* Set the RTP IP Address & port pair to use for this call */
    _SAPP_sipGetRtpInfc(&service_ptr->artpInfc, &service_ptr->mnsService.aAddr,
                &service_ptr->mnsService.aRtcpPort);
    _SAPP_sipGetRtpInfc(&service_ptr->vrtpInfc, &service_ptr->mnsService.vAddr,
                &service_ptr->mnsService.vRtcpPort);
    /* Set the default routing number from the configured VDN */
    OSAL_snprintf(call_ptr->vccHandoffAddr, SAPP_STRING_SZ, "%s",
        service_ptr->vdn);

    c_ptr = &cmd_ptr->msg.call;
    isi_ptr->msg.call.type = c_ptr->type;
    isi_ptr->msg.call.cidType = c_ptr->cidType;

    /*
     * Block cid if call cid type set to invocation or
     * If the call cid type is not set then use service's cid setting.
     */
    if ((ISI_SESSION_CID_TYPE_INVOCATION == c_ptr->cidType) ||
                ((ISI_SESSION_CID_TYPE_NONE == c_ptr->cidType) &&
                (service_ptr->blockCid))) {
        call_ptr->blockCid = 1;
    }
    else {
        call_ptr->blockCid = 0;
        /* See if the call should use alias for the caller id. */
        if ((ISI_SESSION_CID_TYPE_USE_ALIAS == c_ptr->cidType) &&
                (0 != service_ptr->aliasUriList[0][0])) {
            call_ptr->useAlias = OSAL_TRUE;
        }
    }

    /* Let's prepare the details of the media for this invite */
    if (SAPP_OK != _SAPP_encodeMediaSapp2Sip(service_ptr, call_ptr, c_ptr)) {
        /*
         * Then the RTP interface is not available, write an error
         * event for ISI
         */
        _SAPP_getErrorReasonDesc(NULL, &isi_ptr->msg.call);
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_ERROR,
                ISIP_STATUS_INVALID, isi_ptr);
        SAPP_dbgPrintf("%s %d: Could not init conf. Rtp issue.\n",
                __FUNCTION__, __LINE__);
        return (SAPP_ERR);
    }
    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    /*
     * Set phone context, use phoneContext if it's configured,
     * otherwise uses realm
     */
    if (0 != service_ptr->phoneContext[0]) {
        phoneContext_ptr = service_ptr->phoneContext;
    }
    else {
        phoneContext_ptr =
                service_ptr->sipConfig.config.authCred[0].szAuthRealm;
    }
    SAPP_parseAddPhoneContext(c_ptr->to, ISI_ADDRESS_STRING_SZ,
            phoneContext_ptr);

    /* Replace the to with sos urn if the call is emergency call */
    if (call_ptr->isEmergency) {
        OSAL_strncpy(c_ptr->to, SAPP_URI_SOS_STR, ISI_ADDRESS_STRING_SZ);
    }

    sess_ptr = call_ptr->mnsSession.sess_ptr;
    /*
    Content-Type: multipart/mixed;boundary=b_003b2e2b-000d6e0f
    ...
    --b_003b2e2b-000d6e0f
    Content-Type: application/resource-lists+xml
    Content-Disposition: recipient-list
    */

    /* recipient list xml body */
    _SAPP_xmlEncodeRcRlsDoc(c_ptr->participants,
        service_ptr->payloadStratch, SAPP_PAYLOAD_SZ);
        
    target_ptr = sess_ptr->otherPayload;
    targetLen = MAX_SESSION_PAYLOAD_SIZE;

    if (SAPP_OK != SAPP_parseAddBoundry(0, SAPP_PAYLOAD_BOUNDRY, &target_ptr, &targetLen)) {
        return (SAPP_ERR);
    }
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_TYPE_HF, SAPP_APP_RESOURCE_LISTS,
            &target_ptr, &targetLen)) {
        return (SAPP_ERR);
    }
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_DISP_HF, SAPP_RECIPIENT_LIST,
            &target_ptr, &targetLen)) {
        return (SAPP_ERR);
    }
    len = OSAL_strlen(service_ptr->payloadStratch);
    OSAL_snprintf(number, 12, "%d", len + 2);
    if (SAPP_OK != SAPP_parseAddHf(SAPP_CONTENT_LENGTH_HF, number,
            &target_ptr, &targetLen)) {
        return (SAPP_ERR);
    }
    if (SAPP_OK != SAPP_parseAddEol(&target_ptr, & targetLen)) {
        return (SAPP_ERR);
    }
    if (SAPP_OK != SAPP_parseAddPayload(service_ptr->payloadStratch,
            &target_ptr, &targetLen, 1)) {
        return (SAPP_ERR);
    }

    SAPP_dbgPrintf("%s %d: conf-call xml prepared %s\n",
                __FUNCTION__, __LINE__, service_ptr->payloadStratch);
                
    /* Try to place the call over the SIP stack */
    if (SAPP_OK != _SAPP_sipMakeCall(service_ptr, call_ptr, c_ptr->to, 
            call_ptr->mnsSession.sess_ptr, c_ptr->subject,
            &c_ptr->srvccStatus)) {
        /* Then there was an error placing the call */
        _SAPP_getErrorReasonDesc(NULL, &isi_ptr->msg.call);
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_ERROR,
                ISIP_STATUS_INVALID, isi_ptr);
        SAPP_dbgPrintf("%s %d: conf-call sipMakeCall failed reasonDesc %s\n",
                __FUNCTION__, __LINE__, isi_ptr->msg.call.reasonDesc);
        return (SAPP_ERR);
    }

    /*
     * All is well. Tell ISI that we are 'trying' to contact the remote party
     */
    SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
            call_ptr->isiCallId, ISIP_CALL_REASON_TRYING,
            ISIP_STATUS_INVALID, isi_ptr);
    /*
     * Load the local RTP interface info also.
     * Make sure this info is NOT "stunned"
     */
    OSAL_netAddrPortCpy(&isi_ptr->msg.call.lclAudioAddr, &service_ptr->mnsService.aAddr);
    isi_ptr->msg.call.lclAudioCntlPort = service_ptr->mnsService.aRtcpPort;
    OSAL_netAddrPortCpy(&isi_ptr->msg.call.lclVideoAddr, &service_ptr->mnsService.vAddr);
    isi_ptr->msg.call.lclVideoCntlPort = service_ptr->mnsService.vRtcpPort;

    /* Send the event */
    SAPP_sendEvent(evt_ptr);

    /*
     * Notify ISI of the VDN/VDI routing number used for VCC.
     * The call_ptr->vccHandoffAddr field was set when the call
     * object was initialized
     */
    SAPP_sipCallVdxIsiEvt(service_ptr->isiServiceId,
            service_ptr->protocolId, call_ptr->isiCallId,
            call_ptr->vccHandoffAddr, isi_ptr);

    return (SAPP_OK);
}

/*
 * ======== SAPP_sipCallInitiateOutbound(() ========
 *
 * This function is called when ISI has commanded SAPP to initiate an
 * outbound phone call.
 *
 * service_ptr : A pointer to the service that this call is being placed
 *               within.
 * call_ptr : A pointer to a SAPP_CallObj object used internally by
 *            SAPP to manage the call.
 *
 * cmd_ptr : A pointer to the command issued by ISI.  This object contains
 *           all the details of the command.
 *
 * isi_ptr : A pointer to a buffer where ISI events are written.  Events
 *          written here all ultimately sent up to the ISI module.
 *
 * Returns:
 *   SAPP_OK : Function was successful.  Call was placed.
 *   SAPP_ERR : Function failed.  The call could not be placed.
 *              STUN may have failed or the UA_MakeCall() API failed.
 */
vint SAPP_sipCallInitiateOutbound(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    ISIP_Message    *cmd_ptr,
    SAPP_Event      *evt_ptr)
{
    ISIP_Call       *c_ptr;
    ISIP_Message    *isi_ptr = &evt_ptr->isiMsg;
    char            *phoneContext_ptr;

    /*  setup the call object */
    call_ptr->isiCallId = cmd_ptr->id;
    call_ptr->modify = SAPP_MODIFY_NONE;
    /* Set the RTP IP Address & port pair to use for this call */
    _SAPP_sipGetRtpInfc(&service_ptr->artpInfc, &service_ptr->mnsService.aAddr,
                &service_ptr->mnsService.aRtcpPort);
    _SAPP_sipGetRtpInfc(&service_ptr->vrtpInfc, &service_ptr->mnsService.vAddr,
                &service_ptr->mnsService.vRtcpPort);
    /* Set the default routing number from the configured VDN */
    OSAL_snprintf(call_ptr->vccHandoffAddr, SAPP_STRING_SZ, "%s",
        service_ptr->vdn);

    c_ptr = &cmd_ptr->msg.call;
    isi_ptr->msg.call.type = c_ptr->type;
    isi_ptr->msg.call.cidType = c_ptr->cidType;

    /*
     * Block cid if call cid type set to invocation or
     * If the call cid type is not set then use service's cid setting.
     */
    if ((ISI_SESSION_CID_TYPE_INVOCATION == c_ptr->cidType) ||
                ((ISI_SESSION_CID_TYPE_NONE == c_ptr->cidType) &&
                (service_ptr->blockCid))) {
        call_ptr->blockCid = 1;
    }
    else {
        call_ptr->blockCid = 0;
        /* See if the call should use alias for the caller id. */
        if ((ISI_SESSION_CID_TYPE_USE_ALIAS == c_ptr->cidType) &&
                (0 != service_ptr->aliasUriList[0][0])) {
            call_ptr->useAlias = OSAL_TRUE;
        }
    }

    /* Let's prepare the details of the media for this invite */
    if (SAPP_OK != _SAPP_encodeMediaSapp2Sip(service_ptr, call_ptr, c_ptr)) {
        /*
         * Then the RTP interface is not available, write an error
         * event for ISI
         */
        _SAPP_getErrorReasonDesc(NULL, &isi_ptr->msg.call);
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_ERROR,
                ISIP_STATUS_INVALID, isi_ptr);
        return (SAPP_ERR);
    }
    /* Process ISI command for MNS */
    MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, cmd_ptr);

    /*
     * Set phone context, use phoneContext if it's configured,
     * otherwise uses realm
     */
    if (0 != service_ptr->phoneContext[0]) {
        phoneContext_ptr = service_ptr->phoneContext;
    }
    else {
        phoneContext_ptr =
                service_ptr->sipConfig.config.authCred[0].szAuthRealm;
    }
    SAPP_parseAddPhoneContext(c_ptr->to, ISI_ADDRESS_STRING_SZ,
            phoneContext_ptr);

    /* Replace the to with sos urn if the call is emergency call */
    if (call_ptr->isEmergency) {
        OSAL_strncpy(c_ptr->to, SAPP_URI_SOS_STR, ISI_ADDRESS_STRING_SZ);
    }

    /* Try to place the call over the SIP stack */
    if (SAPP_OK != _SAPP_sipMakeCall(service_ptr, call_ptr, c_ptr->to, 
            call_ptr->mnsSession.sess_ptr, c_ptr->subject,
            &c_ptr->srvccStatus)) {
        /* Then there was an error placing the call */
        _SAPP_getErrorReasonDesc(NULL, &isi_ptr->msg.call);
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_ERROR,
                ISIP_STATUS_INVALID, isi_ptr);
        return (SAPP_ERR);
    }
    isi_ptr->msg.call.srvccStatus |= c_ptr->srvccStatus;
    /* 
     * Cache the infomations  for reattempt INVITE upon receiving SIP 503.
     */
    OSAL_memCpy(&call_ptr->event.isiMsg.msg.call, c_ptr, sizeof(ISIP_Call));
    call_ptr->tmrEvtQ = sip_ptr->queue.tmrEvt;

    /*
     * All is well. Tell ISI that we are 'trying' to contact the remote party
     */
    SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
            call_ptr->isiCallId, ISIP_CALL_REASON_TRYING,
            ISIP_STATUS_INVALID, isi_ptr);
    /*
     * Load the local RTP interface info also.
     * Make sure this info is NOT "stunned"
     */
    OSAL_netAddrPortCpy(&isi_ptr->msg.call.lclAudioAddr, &service_ptr->mnsService.aAddr);
    isi_ptr->msg.call.lclAudioCntlPort = service_ptr->mnsService.aRtcpPort;
    OSAL_netAddrPortCpy(&isi_ptr->msg.call.lclVideoAddr, &service_ptr->mnsService.vAddr);
    isi_ptr->msg.call.lclVideoCntlPort = service_ptr->mnsService.vRtcpPort;

    /* Send the event */
    SAPP_sendEvent(evt_ptr);

    /*
     * Notify ISI of the VDN/VDI routing number used for VCC.
     * The call_ptr->vccHandoffAddr field was set when the call
     * object was initialized
     */
    SAPP_sipCallVdxIsiEvt(service_ptr->isiServiceId,
            service_ptr->protocolId, call_ptr->isiCallId,
            call_ptr->vccHandoffAddr, isi_ptr);

    return (SAPP_OK);
}


/*
 * ======== _SAPP_sipCallInboundIsiEvt() ========
 *
 * This function is called to send ISI event when thre is an incoming call
 *
 */
vint _SAPP_sipCallInboundIsiEvt(
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    ISIP_Message   *isi_ptr;
    char           *value_ptr;
    
    isi_ptr = &evt_ptr->isiMsg;

    /* Populate the session part of the ISI event */
    _SAPP_populateIsiEvtSession(&service_ptr->mnsService, &call_ptr->mnsSession,
            &isi_ptr->msg.call);

    /* Populate the header of the ISI event */
    SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
            call_ptr->isiCallId, ISIP_CALL_REASON_INITIATE,
            ISIP_STATUS_INVALID, isi_ptr);

    /*
     * Also load the ring template into the event destined for ISI so
     * ISI knows what ringer to tell VTSP when it some time to ring the phone
     */
    isi_ptr->msg.call.ringTemplate = service_ptr->ringTemplate;

    /* Load the remote URI ("caller ID" info).  Let's look for P-Asserted-Id */
    if (NULL != (value_ptr = SAPP_parsePaiHfValue(NULL, uaEvt_ptr))) {
        /* Then rewrite the from field */
        OSAL_strncpy(uaEvt_ptr->szRemoteUri, value_ptr,
                SIP_URI_STRING_MAX_SIZE);
    }
    /*
     * Some vendors want the call to appear as anonymous if there is no PAI 
     * field. If this is desired then uncomment below so the remote 
     * URI ('from' header field) is over written. Otherwise the caller will 
     * be identified by the remote URI.
     */
    /*
    else {
        OSAL_strncpy(uaEvt_ptr->szRemoteUri, SAPP_ANONYMOUS_CALL_ID_URI,
                SIP_URI_STRING_MAX_SIZE);
    }
    */

    /*
     * If To header field is not the same as primary URI,
     * put it in reason and notify app.
     */
    if (!SAPP_parseCompareUsername(uaEvt_ptr->szToUri, 
            service_ptr->sipConfig.config.aor[0].szUri)) {
        OSAL_snprintf(isi_ptr->msg.call.reasonDesc,
                ISI_EVENT_DESC_STRING_SZ, "%s:\"%s\"",
                SAPP_ALIAS_URI_STR, uaEvt_ptr->szToUri);
    }

    if (NULL != (value_ptr = SAPP_parsePcpiHfValue(uaEvt_ptr))) {
        /* Then rewrite the to field */
        OSAL_strncpy(uaEvt_ptr->szToUri, value_ptr, SIP_URI_STRING_MAX_SIZE);
    }

    OSAL_strncpy(isi_ptr->msg.call.from, uaEvt_ptr->szRemoteUri,
            ISI_ADDRESS_STRING_SZ);
    
    OSAL_strncpy(isi_ptr->msg.call.to, uaEvt_ptr->szToUri,
                ISI_ADDRESS_STRING_SZ);
    
    /* Load any subject info */
    if (NULL != (value_ptr = SAPP_parseHfValue(SAPP_SUBJECT_HF, uaEvt_ptr))) {
        OSAL_snprintf(isi_ptr->msg.call.subject, ISI_SUBJECT_STRING_SZ,
            "%s", value_ptr);
    }

    /* parse history-info here */
    isi_ptr->msg.call.supsrvHfExist &= ~(ISI_SUPSRV_HFEXIST_HISTORY_INFO);
    if (NULL != (value_ptr = SAPP_parseHfValue(SAPP_HISTORY_INFO_HF, uaEvt_ptr))) {
        OSAL_snprintf(isi_ptr->msg.call.historyInfo, ISI_HISTORY_INFO_STRING_SZ,
            "%s", value_ptr);
        isi_ptr->msg.call.supsrvHfExist |= ISI_SUPSRV_HFEXIST_HISTORY_INFO;
    }

    /* Check if there is xml doc in SIP message */
    if ((NULL != uaEvt_ptr->msgBody.payLoad.data) &&
            (0 != uaEvt_ptr->msgBody.payLoad.length)) {
        /* Decode xml doc */
        OSAL_Boolean hasCwi;

        if ((SAPP_OK == _SAPP_xmlDecode3gppCwiHelper(
                uaEvt_ptr->msgBody.payLoad.data,
                uaEvt_ptr->msgBody.payLoad.length,
                &hasCwi)) && (OSAL_TRUE == hasCwi)) {
            _SAPP_appendIncomingReasonDesc(&isi_ptr->msg.call, "CWI", "true");
        }
    }


    /* Send the event */
    SAPP_sendEvent(evt_ptr);

    /*
     * Notify ISI of the VDN/VDI routing number used for VCC.
     * The call_ptr->vccHandoffAddr field was set when the call
     * object was initialized
     */
    SAPP_sipCallVdxIsiEvt(service_ptr->isiServiceId,
            service_ptr->protocolId,  call_ptr->isiCallId,
            call_ptr->vccHandoffAddr, isi_ptr);

    return (SAPP_OK);

}
/*
 * ======== _SAPP_sipCallInitiateInbound() ========
 *
 * This function is called when SIP has sent an event indicating that there
 * is a phone call attempt by a remote UA. This function will attempt
 * to set up the phone call.  If any errors are detected here then this
 * function will generate an appropriate response back to SIP.
 *
 * service_ptr : A pointer to the service that this call is being placed
 *               within.
 * call_ptr : A pointer to a SAPP_CallObj object used internally by
 *            SAPP to manage the call.
 *
 * dialogId : A handle containing a value that represents the SIP dialog
 *            for this call.
 *
 * evt_ptr : A pointer to the event object that came from SIP. This object
 *           contains are the details regarding the call attempt.
 *
 * isi_ptr : A pointer to a buffer where ISI events are written.  Events
 *          written here all ultimately sent up to the ISI module.
 *
 * Returns:
 *   SAPP_OK  : Function was successful.  Call notification was sent to ISI.
 *   SAPP_ERR : Function failed.  The call could not be accepted.
 *              The coders could not be negotiated or we could not respond
 *              successfully to the SIP stack.
 */
vint _SAPP_sipCallInitiateInbound(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    SAPP_CallObj    *call_ptr,
    tSipHandle       dialogId,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    ISIP_Message   *isi_ptr;
    
    isi_ptr = &evt_ptr->isiMsg;
    
    /*
     * First let's prepare the call object.  Note how for inbound calls the
     * dialog handle from SIP will represent both the SIP dialog and isiCallId
     */
    call_ptr->dialogId = dialogId;

    call_ptr->isiCallId = SAPP_getUniqueIsiId(service_ptr->isiServiceId);

    call_ptr->modify = SAPP_MODIFY_NONE;
    /* Set the RTP IP Address & port pair to use for this call */
    _SAPP_sipGetRtpInfc(&service_ptr->artpInfc, &service_ptr->mnsService.aAddr,
                &service_ptr->mnsService.aRtcpPort);
    _SAPP_sipGetRtpInfc(&service_ptr->vrtpInfc, &service_ptr->mnsService.vAddr,
                &service_ptr->mnsService.vRtcpPort);

    /* Set the default routing number from the configured VDN */
    OSAL_snprintf(call_ptr->vccHandoffAddr, SAPP_STRING_SZ, "%s",
        service_ptr->vdn);

    /* Set default prack flag for provisional response */
    call_ptr->usePrack = sip_ptr->usePrack;

    if (!uaEvt_ptr->sessNew) {
        /*
         * Handle INVITE without SDP here.
         * If INVITE without SDP offer, set usePrack and send 183 with SDP offer.
         * Inform ISI there is incoming call but resource is not ready.
         */
        call_ptr->usePrack = 1;
        isi_ptr->msg.call.rsrcStatus = ISI_RESOURCE_STATUS_NOT_READY;
        /* Run MNS */
        MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, NULL);

        UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 183,
                call_ptr->usePrack, 0, NULL, NULL, 0,
                call_ptr->mnsSession.sess_ptr,
                service_ptr->exchangeCapabilitiesBitmap);

        /* Start timer to prevent we stuck in current state */
        _SAPP_sipRsrcRsrvTimerStart(service_ptr, sip_ptr, call_ptr);

        /* Send ISI event */
        _SAPP_sipCallInboundIsiEvt(service_ptr, call_ptr, uaEvt_ptr, evt_ptr);
        return (SAPP_OK);
    }

    /* We have SDP in the INVITE. Run MNS */
    if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                &call_ptr->mnsSession, eUA_CALL_ATTEMPT, uaEvt_ptr, isi_ptr)) {
        UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 415, 0, 0,
                    NULL, NULL, 0, NULL, 0);
        return (SAPP_ERR);
    }

    /* Check if precondition met */
    if (!MNS_isPreconditionMet(&call_ptr->mnsSession)) {
        /*
         * Precondition is not met.
         * Set usePrack and send 183 with SDP offer.
         * Inform ISI there is incoming call but resource is not ready.
         */
        call_ptr->usePrack = 1;
        /* Run MNS */
        MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession, NULL);

        UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 183,
                    call_ptr->usePrack, 0, NULL, NULL, 0,
                    call_ptr->mnsSession.sess_ptr,
                    service_ptr->exchangeCapabilitiesBitmap);

        /* Start timer to prevent we stuck in current state */
        _SAPP_sipRsrcRsrvTimerStart(service_ptr, sip_ptr, call_ptr);
    }
    else {
        if (MNS_isPreconditionUsed(&call_ptr->mnsSession)) {
            SAPP_dbgPrintf("%s Precondition is used and met.\n", __FUNCTION__);
            call_ptr->usePrack = 1;
        }
        /* Resource is ready */
        isi_ptr->msg.call.rsrcStatus = (ISI_RESOURCE_STATUS_LOCAL_READY |
                ISI_RESOURCE_STATUS_REMOTE_READY);
    }

    /* parse SRVCC related tags here and check UE capabilities */
    if (SAPP_parseHfValueExist(
            SAPP_FEATURE_CAPS_HF,
            SIP_CAPS_SRVCC_ALERTING_STR,
            uaEvt_ptr)) {
        isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_ALERTING_RECEIVE;
        if (service_ptr->srvccCapabilitiesBitmap & eSIP_CAPS_SRVCC_ALERTING) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_ALERTING_SEND;
        }
    }
    if (SAPP_parseHfValueExist(
            SAPP_FEATURE_CAPS_HF,
            SIP_CAPS_SRVCC_MID_CALL_STR,
            uaEvt_ptr)) {
        isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_MID_CALL_RECEIVE;
        if (service_ptr->srvccCapabilitiesBitmap & eSIP_CAPS_SRVCC_MID_CALL) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_MID_CALL_SEND;
        }
    }

    /* Send ISI event */
    _SAPP_sipCallInboundIsiEvt(service_ptr, call_ptr, uaEvt_ptr, evt_ptr);
    return (SAPP_OK);
}
/*
 * ======== SAPP_sipCallVdxIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for reporting to ISI a change to the VDI/VDN routing numbers.
 * VDI and VDN is terminology used in the VCCC world representing the
 * routing numbers to use if a call needs to be handed off.
 *
 * Returns:
 * Nothing.
 */
void SAPP_sipCallVdxIsiEvt(
    ISI_Id              serviceId,
    vint                protocolId,
    ISI_Id              callId,
    char               *target_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->msg.call.type = ISI_SESSION_TYPE_AUDIO;
    SAPP_sipCallIsiEvt(serviceId, protocolId, callId, ISIP_CALL_REASON_VDX,
        ISIP_STATUS_INVALID, isi_ptr);

    /* Copy in the target VND/VDI to report to ISI */
    OSAL_snprintf(isi_ptr->msg.call.to, ISI_ADDRESS_STRING_SZ,
                 "%s", target_ptr);
    return;
}

/*
 * ======== SAPP_sipTerminateAllCalls() ========
 * This function is called to terminate all calls and send ISI event to
 * notify call's termination.
 *
 * Return Values:
 *  Nothing
 *
 */
void SAPP_sipTerminateAllCalls(
    SAPP_ServiceObj *service_ptr,
    SAPP_Event      *evt_ptr)
{
    vint          x;
    ISIP_Message *isi_ptr;
    SAPP_CallObj *call_ptr;

    isi_ptr = &evt_ptr->isiMsg;

    /* First send events for any active calls */
    for (x = 0; x < SAPP_CALL_NUM; x++) {
        if (service_ptr->sipConfig.aCall[x].isiCallId != 0) {
            /* We have an active call */
            call_ptr = &service_ptr->sipConfig.aCall[x];
            /* Send BYE */
            _SAPP_sipHungUp(service_ptr, call_ptr->dialogId, NULL);
            /* Populate ISI event */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    call_ptr->isiCallId,
                    ISIP_CALL_REASON_TERMINATE, ISIP_STATUS_INVALID, isi_ptr);
            /* Destroy the call */
            SAPP_sipDestroyCall(call_ptr);
            /* Send the ISI event */
            SAPP_sendEvent(evt_ptr);
        }
    }

    return;
}

/*
 * ======== SAPP_isiCallCmd() ========
 *
 * This function is the entry point for commands from ISI that are related
 * to placing a Call.  This is the first place that ISI commands for calls
 * begin to be processed.
 *
 * cmd_ptr : A pointer to the command block that came from ISI. All command
 *           details are inside here.
 *
 * sip_ptr : A pointer to an object used internally in SAPP to manage
 *           services/calls.
 *
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 * Returns:
 *   Nothing.
 */
void SAPP_isiCallCmd(
    ISIP_Message     *cmd_ptr,
    SAPP_SipObj      *sip_ptr,
    SAPP_Event       *evt_ptr)
{
    SAPP_ServiceObj *service_ptr;
    SAPP_CallObj    *call_ptr;
    SAPP_CallObj    *transferCall_ptr;
    ISIP_Call       *c_ptr;
    ISIP_Message    *isi_ptr;
    vint             x;
    tSipHandle       oriDialogId = NULL;
    vint             capsBitmap = 0;

    c_ptr = &cmd_ptr->msg.call;

    isi_ptr = &evt_ptr->isiMsg;
    isi_ptr->msg.call.type = c_ptr->type;

    service_ptr = SAPP_findServiceViaServiceId(sip_ptr, c_ptr->serviceId);
    if (service_ptr == NULL) {
        /* Doesn't exist, simply ignore */
        return;
    }

    if ((c_ptr->reason == ISIP_CALL_REASON_INITIATE) ||
            (c_ptr->reason == ISIP_CALL_REASON_INITIATE_CONF)) {
        /*
         * Then it's a new call request.  See if there is an
         * available resource.
         */
        call_ptr = SAPP_findCallViaIsiId(service_ptr, 0);
        if (call_ptr == NULL) {
            /* Then there no available, tell ISI of the failure */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, cmd_ptr->id,
                    ISIP_CALL_REASON_FAILED, ISIP_STATUS_INVALID, isi_ptr);
            return;
        }
        /* Init the call object */
        OSAL_memSet(call_ptr, 0, sizeof(SAPP_CallObj));
    }
    else {
        /*
         * Try to find the call this is for.  If it doesn't exist
         * then quietly ignore
         */
        if (NULL == (call_ptr = SAPP_findCallViaIsiId(service_ptr,
                cmd_ptr->id))) {
            return;
        }
    }

    /* If we are here then we have a valid call_ptr */

    /* Load the coders in the ISI command into the call object. */
    _SAPP_decodeCoderIsi2Sapp(c_ptr->coders, ISI_CODER_NUM,
            call_ptr->coders, ISI_CODER_NUM);

    SAPP_dbgPrintf("%s:%d c_ptr->reason = %d\n", __FUNCTION__, __LINE__, 
            c_ptr->reason);
    /* Act on the 'reason' */
    switch (c_ptr->reason) {
    case ISIP_CALL_REASON_INITIATE:
        /* Check if this call is emergency call */
        if (0 != (c_ptr->type & ISI_SESSION_TYPE_EMERGENCY)) {
            call_ptr->isEmergency = 1;
            /*
             * If the service is normal sip service, no need to do emergency
             * state machine process
             */
            if (service_ptr->isEmergency) {
                SAPP_emgcyCallInitiate(service_ptr, sip_ptr, cmd_ptr, call_ptr);
                break;
            }
        }
        if (SAPP_sipCallInitiateOutbound(sip_ptr, service_ptr, call_ptr,
                cmd_ptr, evt_ptr) != SAPP_OK) {
            /* Call failed.  The ISI event is already written */
            SAPP_sipDestroyCall(call_ptr);
        }
        break;
    case ISIP_CALL_REASON_INITIATE_CONF:
        if (_SAPP_sipCallInitiateConf(sip_ptr, service_ptr, call_ptr,
                cmd_ptr, evt_ptr) != SAPP_OK) {
            /* Call failed.  The ISI event is already written */
            SAPP_sipDestroyCall(call_ptr);
        }
        break;
    case ISIP_CALL_REASON_TERMINATE:
        /* Disconnect the SIP call */
        _SAPP_sipHungUp(service_ptr, call_ptr->dialogId, c_ptr->reasonDesc);
#ifdef INCLUDE_SIMPLE
        if (0 != call_ptr->conf.dialogId) {
            SAPP_conferenceUnsubscribe(&call_ptr->conf.dialogId, service_ptr, 
                    call_ptr->conf.identity,
                    service_ptr->sipConfig.config.aor[0].szUri, NULL);
        }
#endif
        /* Tell ISI that the call is disconnected */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_TERMINATE,
                ISIP_STATUS_INVALID, isi_ptr);
        if (call_ptr->isEmergency) {
            SAPP_emgcyCallTerminate(service_ptr, sip_ptr, cmd_ptr, call_ptr);
        }

        /*
         * If the Replaces header field matches a dialog which has already 
         * terminated, decline the request with a 603 Declined response.
         */
        if (call_ptr->replaceDialogId) {
            UA_Respond(service_ptr->sipConfig.uaId, call_ptr->replaceDialogId,
                    SIP_RSP_DECLINE, 0, 0, NULL, NULL, 0, NULL, 0);
            call_ptr->replaceDialogId = 0;
        }

        SAPP_sipDestroyCall(call_ptr);

        break;
    case ISIP_CALL_REASON_ACK:
        /* If UA has more call legs, send a waiting tone for CW supplementary */
        for (x = 1; x < SAPP_CALL_NUM; x++) {
            if (service_ptr->sipConfig.aCall[x].isiCallId != 0) {
                call_ptr->waitingTone = 1;
            }
        }
        capsBitmap = service_ptr->exchangeCapabilitiesBitmap;
        if (cmd_ptr->msg.call.srvccStatus & ISI_SRVCC_STATUS_ALERTING_RECEIVE) {
            capsBitmap |= eSIP_CAPS_SRVCC_ALERTING;
        }
        if (cmd_ptr->msg.call.srvccStatus & ISI_SRVCC_STATUS_MID_CALL_RECEIVE) {
            capsBitmap |= eSIP_CAPS_SRVCC_MID_CALL;
        }

        /*
         * This UA accepted, send a ringing response.  If the remote side
         * indicated the use of "prack" when the INVITE was received
         * Then send reliably
         */
        UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 180,
                call_ptr->usePrack, call_ptr->waitingTone, NULL, NULL, 0, NULL,
                capsBitmap);
        break;
    case ISIP_CALL_REASON_ACCEPT:
        _SAPP_updateMediaSapp2Sip(call_ptr, c_ptr);

        _SAPP_sipAnswerCall(service_ptr, call_ptr, cmd_ptr);
        break;
    case ISIP_CALL_REASON_REJECT:
        /* support app passed reason code in reasonDesc */
        if (0 == OSAL_strncmp(c_ptr->reasonDesc,
                SAPP_CALL_REJECT_REASON_BUSY, 3)) {
            UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 486,
                    0, 0, NULL, NULL, 0, NULL, 0);
        }
        else if (0 == OSAL_strncmp(c_ptr->reasonDesc,
                SAPP_CALL_REJECT_REASON_NOT_ACCEPTABLE, 3)) {
            UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 488,
                    0, 0, NULL, NULL, 0, NULL, 0);
        }
        /* default to be 486 Busy Here */
        else {
            UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 486,
                0, 0, NULL, NULL, 0, NULL, 0);
        }
        /* Tell ISI that the call is disconnected */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_TERMINATE,
                ISIP_STATUS_INVALID, isi_ptr);
        if (call_ptr->isEmergency) {
            SAPP_emgcyCallTerminate(service_ptr, sip_ptr, cmd_ptr, call_ptr);
        }
        SAPP_sipDestroyCall(call_ptr);
        break;
    case ISIP_CALL_REASON_HOLD:
        /* Load the media from ISI, they have/may not have changed */
        _SAPP_updateMediaSapp2Sip(call_ptr, c_ptr);

       if (_SAPP_sipHold(service_ptr, call_ptr, cmd_ptr) == SAPP_OK) {
            /* Indicate that there is a modify transaction in progress */
            call_ptr->modify = SAPP_MODIFY_HOLD;
        }
        break;
    case ISIP_CALL_REASON_RESUME:
        /* Load the media from ISI, they have/may not have changed */
        _SAPP_updateMediaSapp2Sip(call_ptr, c_ptr);

        if (_SAPP_sipUnhold(service_ptr, call_ptr, cmd_ptr) == SAPP_OK) {
            /* Indicate that there is a modify transaction in progress */
            call_ptr->modify = SAPP_MODIFY_RESUME;
        }
        break;
    case ISIP_CALL_REASON_MODIFY:
        /* Load the media from ISI, they have/may not have changed */
        _SAPP_updateMediaSapp2Sip(call_ptr, c_ptr);
        
        if (_SAPP_sipModify(service_ptr, call_ptr, cmd_ptr, isi_ptr)
                != SAPP_OK) {
            /* Then we failed, tell ISI about the failure */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_MODIFY, ISIP_STATUS_FAILED, isi_ptr);
            break;
        }
        call_ptr->modify = SAPP_MODIFY_OTHER;
        break;
     case ISIP_CALL_REASON_REJECT_MODIFY:
        /* Fall through */
     case ISIP_CALL_REASON_ACCEPT_MODIFY:
        /* Update media from ISI */
        _SAPP_updateMediaSapp2Sip(call_ptr, c_ptr);

        /*
         * If the new dialog id exist and the modification is accept, 
         * replace the original dialogId in SAPP_CallObj by the new one.
         */
        if (call_ptr->replaceDialogId) {
            oriDialogId = call_ptr->dialogId; 
            call_ptr->dialogId = call_ptr->replaceDialogId;
        }

        if (SAPP_OK != _SAPP_sipAcceptModify(service_ptr, call_ptr, cmd_ptr)) {
            SAPP_dbgPrintf("%s: %d Accept modify failed\n",
                    __FUNCTION__, __LINE__);
        }

        /*
         * If the new dialog id exist and the modification is accept, 
         * terminate the original call.
         */
        if (call_ptr->replaceDialogId) {
            call_ptr->replaceDialogId = 0;
            _SAPP_sipHungUp(service_ptr, oriDialogId, NULL);
        }
        break;

    case ISIP_CALL_REASON_FORWARD:
        _SAPP_sipForwardCall(service_ptr, call_ptr->dialogId, c_ptr->to);
        /*
         * When you forward a call, the call needs to destroyed.  Tell ISI
         * about it too.
         */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId, service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_TERMINATE,
                ISIP_STATUS_INVALID, isi_ptr);
        SAPP_sipDestroyCall(call_ptr);
        break;
    case ISIP_CALL_REASON_TRANSFER_BLIND:
    case ISIP_CALL_REASON_TRANSFER_ATTENDED:
        if (UA_TransferCall(service_ptr->sipConfig.uaId, call_ptr->dialogId,
                c_ptr->to, NULL, NULL, 0,
                &service_ptr->sipConfig.localConn, eSIP_INVITE, NULL) != SIP_OK) {
            /* Then the transfer failed, tell ISI */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    call_ptr->isiCallId,
                    ISIP_CALL_REASON_TRANSFER_FAILED, ISIP_STATUS_INVALID,
                    isi_ptr);
            break;
        }
        /*
         * We are going to send a status event back to ISI when we transfer.
         * For 'blind' transfer's we don't care about the success or failure
         * of the transfer, in which case we simply tell ISI it completed.
         * For attended call transfers we will indicate 'progress' back to ISI
         * and then report there success or failure of the transfer after SIP
         * tells us the status.
         */
        if (c_ptr->reason == ISIP_CALL_REASON_TRANSFER_BLIND) {
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_TRANSFER_COMPLETED, ISIP_STATUS_INVALID,
                    isi_ptr);
        }
        else {
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_TRANSFER_PROGRESS, ISIP_STATUS_INVALID,
                    isi_ptr);
        }
        break;

    case ISIP_CALL_REASON_TRANSFER_CONSULT:
        /* Let's get the transfer target. */
        if (NULL == (transferCall_ptr = SAPP_findCallViaIsiId(service_ptr, c_ptr->transferTargetCallId))) {
            /* Then the transfer failed, tell ISI */
           SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                   ISIP_CALL_REASON_TRANSFER_FAILED, ISIP_STATUS_INVALID,
                   isi_ptr);
           break;
        }
        if (UA_TransferCall(service_ptr->sipConfig.uaId, call_ptr->dialogId,
                c_ptr->to, transferCall_ptr->dialogId, NULL, 0,
                &service_ptr->sipConfig.localConn, eSIP_INVITE, NULL) != SIP_OK) {
            /* Then the transfer failed, tell ISI */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_TRANSFER_FAILED, ISIP_STATUS_INVALID,
                    isi_ptr);
            break;
        }
        /*
         * We are going to send a status event back to ISI when we transfer.
         * For consultative call transfers we will indicate 'progress' back to ISI
         * and then report there success or failure of the transfer after SIP
         * tells us the status.
         */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_TRANSFER_PROGRESS, ISIP_STATUS_INVALID,
                isi_ptr);
        break;
    case ISIP_CALL_REASON_TRANSFER_CONF:
        if ('\0' == c_ptr->participants[0]) {
            x = _SAPP_sipConfAddParty(service_ptr, call_ptr, c_ptr);
        }
        else {
            x = SAPP_ERR; /* not supported */
        }
        if (SAPP_OK != x) {
                /* Then the transfer failed, tell ISI */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_TRANSFER_FAILED, ISIP_STATUS_INVALID,
                    isi_ptr);
        }
        else {
            /*
             * We are going to send a status event back to ISI when we transfer.
             * For consultative call transfers we will indicate 'progress' back to ISI
             * and then report there success or failure of the transfer after SIP
             * tells us the status.
             */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_TRANSFER_PROGRESS, ISIP_STATUS_INVALID,
                    isi_ptr);
        }
        break;
    case ISIP_CALL_REASON_TRANSFER_COMPLETED:
        UA_TransferStatus(service_ptr->sipConfig.uaId, call_ptr->dialogId, 200,
                NULL, 0, &service_ptr->sipConfig.localConn);
        break;

    case ISIP_CALL_REASON_TRANSFER_FAILED:
        UA_TransferStatus(service_ptr->sipConfig.uaId, call_ptr->dialogId, 503,
                NULL, 0, &service_ptr->sipConfig.localConn);
        break;
    case ISIP_CALL_REASON_CONF_KICK:
        UA_TransferCall(service_ptr->sipConfig.uaId, call_ptr->dialogId,
                c_ptr->to, NULL, NULL, 0,
                &service_ptr->sipConfig.localConn, eSIP_BYE, NULL);
        break;

    default:
        break;
    }
    return;
}

/*
 * ======== SAPP_sipCallEvent() ========
 *
 * This function is the entry point for events from SIP that are related
 * to phone calls.  This is the first place that SIP events for calls
 * begin to be processed.
 *
 * sip_ptr : A pointer to a SAPP_SipObj object used to manage the SIP stack.
 *
 * service_ptr : A pointer to the service that this call event belongs to.
 *
 * hUa : A handle to the SIP UA that this event was received within.  This
 *       is the value returned when the UA was created via UA_Create().
 *
 * hDialog : A handle to a SIP dialog.  This is the value that SIP uses to
 *           represent this phone call (a.k.a. a 'dialog' in SIP terms).
 *
 * event : An enumerated value representing the type of event. These events
 *         are defined in sip_ua.h
 *
 * arg : A pointer to a tUaAppEvent object that is populated with all the
 *       details of the event.  Please see SIP interface document for details
 *       regarding the values in this object.
 *
 * isi_ptr : A pointer to a buffer area that is used to write ISI events to.
 *           Events that are written to this buffer are ultimately sent up to
 *           the ISI layer.
 * Returns:
 *   Nothing.
 */
void SAPP_sipCallEvent(
    SAPP_SipObj     *sip_ptr,
    SAPP_ServiceObj *service_ptr,
    tSipHandle       hUa,
    tSipHandle       hDialog,
    tUaEvtType       event,
    tUaAppEvent     *uaEvt_ptr,
    SAPP_Event      *evt_ptr)
{
    SAPP_CallObj    *call_ptr;
    ISIP_CallReason  reason = ISIP_CALL_REASON_INVALID;

    ISIP_Coder       coders[ISI_CODER_NUM];
    ISIP_Message    *isi_ptr = &evt_ptr->isiMsg;
    char            *value_ptr;
    char            *reason_ptr;
    vint             x;
    ISIP_Status      status;

    SAPP_dbgPrintf("%s:%d event = %d\n", __FUNCTION__, __LINE__, event);
    if (event == eUA_CALL_ATTEMPT) {
        /* Check if 'unconditional' call forwarding is on. If so forward call */
        if (service_ptr->callForward[0] != 0) {
            /* Then we must forward the call to this target */
            _SAPP_sipForwardCall(service_ptr, hDialog,
                    service_ptr->callForward);
            return;
        }

        /*
         * Check if the new incoming call is attempting to replace another
         * call.
         */
        if (uaEvt_ptr->hDialogReplaces != NULL) {
            /* Find the call to be replaced. */
            call_ptr = SAPP_findCallViaDialogId(service_ptr, 
                    uaEvt_ptr->hDialogReplaces);
            if (call_ptr != NULL) {
                /* Found the call, cache the new incoming call dialog id. */
                call_ptr->replaceDialogId = hDialog;
            }
            else {
                /* Cannot find the call to replace, reply 481. */
                UA_Respond(service_ptr->sipConfig.uaId, hDialog,
                        SIP_RSP_CALL_TRANS_NO_EXIST, 0, 0, NULL, NULL, 0, NULL,
                         0);
                return;
            }
        }
        else {
            /* Then try to allocate a new call */
            call_ptr = SAPP_findCallViaDialogId(service_ptr, 0);
            if (call_ptr == NULL) {
                /* Could not get one, return an error */
                UA_Respond(service_ptr->sipConfig.uaId, hDialog, 486,
                        0, 0, NULL, NULL, 0, NULL, 0);
                return;
            }
            /*  Init the new call object */
            OSAL_memSet(call_ptr, 0, sizeof(SAPP_CallObj));
        }
    }
    else {
        call_ptr = SAPP_findCallViaDialogId(service_ptr, hDialog);
        if (call_ptr == NULL) {
            /*
             * Not for an active call. Let's call UA_HangUp for safe keeping.
             * There no reason to tell ISI so hangup and return
             */
            _SAPP_sipHungUp(service_ptr, hDialog, NULL);
            return;
        }
    }

    /* Send PRACK if it's a provisional response(101~199) and prack or precondition is used. */
    if ((uaEvt_ptr->resp.respCode > 100) && (uaEvt_ptr->resp.respCode < 200)) {
         /* Check if PRACK is required in 1xx */
         call_ptr->usePrack = SAPP_parseHfValueExist("Require:", "100rel",
                uaEvt_ptr);

        if (call_ptr->usePrack) {
            _SAPP_sipSendPrack(service_ptr, hDialog);
        }
    }


    switch (event) {
    case eUA_RESPONSE:
        if (uaEvt_ptr->resp.respCode < 200) {
            /* Then it's only a provisional response. Quietly discard */
            return;
        }
        else {
           /*
            * It already destoyed the dialog in SIP.
            */
            call_ptr->dialogId = 0;
        }

        if (uaEvt_ptr->resp.respCode == 302) {
            /* Then it for call forwarding */

            /* Place a new call over SIP to the forwarded address */
            if (SAPP_OK ==
                    _SAPP_sipMakeCall(service_ptr, call_ptr, uaEvt_ptr->szToUri,
                    &call_ptr->mnsSession.session, NULL,
                    &isi_ptr->msg.call.srvccStatus)) {
                return;
            }
            reason = ISIP_CALL_REASON_ERROR;
        }
        else if (uaEvt_ptr->resp.respCode == 407 ||
                uaEvt_ptr->resp.respCode == 401) {
            /* Then we are not authorized to place the call */
            reason = ISIP_CALL_REASON_CREDENTIALS;
        }
        else if (uaEvt_ptr->resp.respCode == 486 ||
                uaEvt_ptr->resp.respCode == 603 ||
                uaEvt_ptr->resp.respCode == 480) {
            /*
             * Then the remote party has rejected the call due to it being
             * 'busy', 'declining', 'temporarily unavailable'.
             */
            reason = ISIP_CALL_REASON_REJECT;
        }
        else if (uaEvt_ptr->resp.respCode == 422) {
            /*
             * 422 Session interval too samll
             * Change interval to Min-SE and make call again.
             */
            if (NULL != (value_ptr = SAPP_parseHfValue(SAPP_MIN_SE_HF, uaEvt_ptr))) {
                OSAL_snprintf(service_ptr->sipConfig.sessionTimer, SAPP_STRING_SZ,
                    "%s", value_ptr);
            }
            /* Place a new call */
            if (SAPP_OK == _SAPP_sipMakeCall(service_ptr,
                    call_ptr,
                    uaEvt_ptr->szRemoteUri,
                    &call_ptr->mnsSession.session,
                    NULL, &isi_ptr->msg.call.srvccStatus)) {
                return;
            }
            reason = ISIP_CALL_REASON_FAILED;
        }
        else if (uaEvt_ptr->resp.respCode == 421) {
            /* check precondition require in HF */
            if (!OSAL_strcmp(SAPP_SUPPORTED_OPT_PRECONDITION,
                    SAPP_parseHfValue(SAPP_REQUIRE_HF, uaEvt_ptr))) {
                /* set Precondition true */
                call_ptr->mnsSession.usePrecondition = 1;
                call_ptr->mnsSession.state = MNS_STATE_IDLE;
               
                /* Run MNS */
                MNS_processCommand(&service_ptr->mnsService,
                        &call_ptr->mnsSession, NULL);
   
                /* Place a new call */
                if (SAPP_OK == _SAPP_sipMakeCall(service_ptr,
                        call_ptr,
                        uaEvt_ptr->szRemoteUri,
                        &call_ptr->mnsSession.session,
                        NULL, &isi_ptr->msg.call.srvccStatus)) {
                    return;
                }
               reason = ISIP_CALL_REASON_FAILED;
            }
        }
        else if (uaEvt_ptr->resp.respCode == 503) {
            /* Reattempt the INVITE request upon receiving a 503 
             * (Service Unavailable) response.
             */
            if (uaEvt_ptr->resp.retryAfterPeriod) {
                if (SAPP_OK == _SAPP_sipInviteRetryTmrStart(service_ptr,
                        call_ptr, uaEvt_ptr->resp.retryAfterPeriod)) {
                    return;
                }
            }
            reason = ISIP_CALL_REASON_FAILED;
        }
        else if (uaEvt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
            /* Server response 504 server time out. */
            if (SAPP_OK == _SAPP_sipServerTimeOut(service_ptr, &reason_ptr,
                    uaEvt_ptr, sip_ptr)) {
                return;
            }
            reason = ISIP_CALL_REASON_FAILED;
        }
        else {
            /* For all other reason, simply declare the call has "failed" */
            reason = ISIP_CALL_REASON_FAILED;
        }

        _SAPP_getErrorReasonDesc(uaEvt_ptr, &isi_ptr->msg.call);

        /* Handle 380 Alternative Service */
        if (uaEvt_ptr->resp.respCode == 380) {
            /*
             * From 3GPP TS 24.229 5.1.2A.1.1, need to decode the 3gpp ims
             * xml docucment and also need to compare P-Asserted-Identity and
             * the last entry of Path header field which is stored when
             * successful registration.
             */
            if (SAPP_OK == _SAPP_sipIsAlternativeServiceEmgcy(service_ptr,
                    uaEvt_ptr, &reason_ptr) &&
                    (NULL != (value_ptr = SAPP_parsePaiHfValue(SAPP_SIP_SCHEME,
                    uaEvt_ptr))) &&
                    (0 == OSAL_strncmp(service_ptr->lastPathUri, value_ptr,
                    OSAL_strlen(service_ptr->lastPathUri)))) {
                /*
                 * If the response is 380 alternative service and emergency,
                 * modify the reason description.
                 */
                OSAL_snprintf(isi_ptr->msg.call.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "CALL FAILED: CODE:%d REASON:%s %s",
                        uaEvt_ptr->resp.respCode,
                        "Alernative Service. Emergency registration required.",
                        reason_ptr);
            }
#if defined(PROVIDER_CMCC)
            else {
                /* fill the emgcy subservice type to reasonDesc */
                    if (NULL != OSAL_strscan(uaEvt_ptr->szToUri,"urn:service:sos")) {
                        OSAL_snprintf(isi_ptr->msg.call.reasonDesc,
                        ISI_EVENT_DESC_STRING_SZ, "CODE:%d REASON:%s %s",
                        uaEvt_ptr->resp.respCode,"Emergency subservice.",uaEvt_ptr->szToUri);
                    }
            }
#endif            
        }
        
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                reason, ISIP_STATUS_INVALID, isi_ptr);

        if (call_ptr->isEmergency) {
            SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL, call_ptr);
        }

        SAPP_sipDestroyCall(call_ptr);
        break;

    case eUA_ERROR:
        /*
         * Then tell ISI about the error.  An error here means that the
         * transaction failed. More than likely there is a network error.
         */

        _SAPP_getErrorReasonDesc(uaEvt_ptr, &isi_ptr->msg.call);

        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_ERROR, ISIP_STATUS_INVALID, isi_ptr);

        if (call_ptr->isEmergency) {
            SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL, call_ptr);
        }
        SAPP_sipDestroyCall(call_ptr);

        break;

    case eUA_CALL_DROP:
        /* Tell ISI that the call is disconnected */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_TERMINATE, ISIP_STATUS_INVALID, isi_ptr);
        if (call_ptr->isEmergency) {
            SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL, call_ptr);
        }
        SAPP_sipDestroyCall(call_ptr);
        break;

    case eUA_RINGING:
        /* parse alert-info-cw here */
        isi_ptr->msg.call.supsrvHfExist = 0;
        if (SAPP_parseHfValueExist(
                SAPP_ALERT_INFO_HF,
                SAPP_URN_ALERT_INFO_CW_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.supsrvHfExist |= ISI_SUPSRV_HFEXIST_ALERT_INFO_CW;
        }
        /* parse SRVCC related tags here */
        if (SAPP_parseHfValueExist(
                SAPP_FEATURE_CAPS_HF,
                SIP_CAPS_SRVCC_ALERTING_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_ALERTING_RECEIVE;
        }
        if (SAPP_parseHfValueExist(
                SAPP_FEATURE_CAPS_HF,
                SIP_CAPS_SRVCC_MID_CALL_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_MID_CALL_RECEIVE;
        }

        /* Check if there is SDP in 180 Ringing */
        if (uaEvt_ptr->sessNew) {
            /* Here we have SDP answer in 180 Ringing, run MNS */
            if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                    &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                /* Then we can't agree on any thing so disconnect the call */
                _SAPP_sipHungUp(service_ptr, hDialog, NULL);
                SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        call_ptr->isiCallId,
                        ISIP_CALL_REASON_FAILED, ISIP_STATUS_INVALID, isi_ptr);
                SAPP_sendEvent(evt_ptr);
                if (call_ptr->isEmergency) {
                    SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL,
                            call_ptr);
                }
                SAPP_sipDestroyCall(call_ptr);
                return;
            }
            /* Populate the session part of the ISI event */
            _SAPP_populateIsiEvtSession(&service_ptr->mnsService,
                    &call_ptr->mnsSession, &isi_ptr->msg.call);

            /* Populate early media event to ISI. */
            _SAPP_populateEarlyMedia(service_ptr, uaEvt_ptr, call_ptr, evt_ptr);

            /*
             * The media & session info is now populated in the ISI event
             * so let's populate the rest of hte ISI event.
             */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId, call_ptr->isiCallId,
                    ISIP_CALL_REASON_MODIFY, ISIP_STATUS_INVALID, isi_ptr);
            SAPP_sendEvent(evt_ptr);
        }

        /* Populate early media event to ISI. */
        _SAPP_populateEarlyMedia(service_ptr, uaEvt_ptr, call_ptr, evt_ptr);

        /* Send ISI ACK to notify remote is ringing */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_ACK, ISIP_STATUS_INVALID, isi_ptr);

        break;
    case eUA_CALL_IS_BEING_FORW:
        /* Send ISI ACK to notify remote is ringing */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_BEING_FORWARDED, ISIP_STATUS_INVALID, isi_ptr);

        break;
    case eUA_PRACK:
        /* parse SRVCC related tags here */
        if (SAPP_parseHfValueExist(
                SAPP_FEATURE_CAPS_HF,
                SIP_CAPS_SRVCC_ALERTING_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_ALERTING_RECEIVE;
        }
        if (SAPP_parseHfValueExist(
                SAPP_FEATURE_CAPS_HF,
                SIP_CAPS_SRVCC_MID_CALL_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_MID_CALL_RECEIVE;
        }

        if (!uaEvt_ptr->sessNew) {
            /* No sdp in PRACK, just responds OK. */
            UA_PrackResp(service_ptr->sipConfig.uaId, call_ptr->dialogId, 200,
                    NULL, NULL, 0, NULL, &service_ptr->sipConfig.localConn);
            break;
        }

        /*
         * There is SDP in PRACK. MT call cases.
         * Case 1) It's the SDP answer to 1xx-rel INVITE Resp. It's no SDP in
         *   the INVITE case. Need to send MODIFY event to ISI.
         * Case 2) It's SDP offer only if the reliable response that it
         *   acknowledges contains an answer to the previous offer/answer
         *   exchange. It's a precondition use case. Need to send MODIFY event
         *   to ISI to update resource status.
         */
        if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
            /* Then we can't agree on any coders so disconnect the call */
            _SAPP_sipHungUp(service_ptr, hDialog, NULL);
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    call_ptr->isiCallId,
                    ISIP_CALL_REASON_FAILED, ISIP_STATUS_INVALID, isi_ptr);
            if (call_ptr->isEmergency) {
                SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL, call_ptr);
            }
            SAPP_sipDestroyCall(call_ptr);
            return;
        }

        /* Response OK to PRACK. The OK might or might not have SDP. */
        UA_PrackResp(service_ptr->sipConfig.uaId, call_ptr->dialogId, 200,
                NULL, NULL, 0, call_ptr->mnsSession.sess_ptr,
                &service_ptr->sipConfig.localConn);
        break;
    case eUA_SESSION:
        if (!uaEvt_ptr->sessNew) {
            /* No sdp in PRACK or 183, do nothing */
            break;
        }

        /*
         * This event is generated when the SIP stack gets
         * INVITE responses with session info ('early' media).
         * Try to negotiate and update ISI.
         */
        if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
            /* Then we can't agree on any coders so disconnect the call */
            _SAPP_sipHungUp(service_ptr, hDialog, NULL);
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    call_ptr->isiCallId,
                    ISIP_CALL_REASON_FAILED, ISIP_STATUS_INVALID, isi_ptr);
            if (call_ptr->isEmergency) {
                SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL, call_ptr);
            }
            SAPP_sipDestroyCall(call_ptr);
            return;
        }

        /* Populate early media event to ISI. */
        _SAPP_populateEarlyMedia(service_ptr, uaEvt_ptr, call_ptr, evt_ptr);

        /* Stop resource reservation timer anyway */
        _SAPP_sipRsrcRsrvTimerStop(call_ptr); 

        /* Populate the session part of the ISI event */
        _SAPP_populateIsiEvtSession(&service_ptr->mnsService, &call_ptr->mnsSession,
                &isi_ptr->msg.call);

        /*
         * The media & session info is now populated in the ISI event
         * so let's populate the rest of the ISI event.
         */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_MODIFY, ISIP_STATUS_INVALID, isi_ptr);
        break;

    case eUA_CANCELED:
        /* Run MNS state machine */
        MNS_processCommand(&service_ptr->mnsService, 
                &call_ptr->mnsSession, NULL);

        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_CANCEL_MODIFY, ISIP_STATUS_INVALID, isi_ptr);
        break;
    case eUA_ANSWERED:
        /* Run MNS for 200 OK. */
        if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
            /* Then we can't agree on any thing so disconnect the call */
            _SAPP_sipHungUp(service_ptr, hDialog, NULL);
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    call_ptr->isiCallId,
                    ISIP_CALL_REASON_FAILED, ISIP_STATUS_INVALID, isi_ptr);
            if (call_ptr->isEmergency) {
                SAPP_emgcyCallTerminate(service_ptr, sip_ptr, NULL, call_ptr);
            }
            SAPP_sipDestroyCall(call_ptr);
            return;
        }
        /* parse SRVCC related tags here */
        if (SAPP_parseHfValueExist(
                SAPP_FEATURE_CAPS_HF,
                SIP_CAPS_SRVCC_ALERTING_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_ALERTING_RECEIVE;
        }
        if (SAPP_parseHfValueExist(
                SAPP_FEATURE_CAPS_HF,
                SIP_CAPS_SRVCC_MID_CALL_STR,
                uaEvt_ptr)) {
            isi_ptr->msg.call.srvccStatus |= ISI_SRVCC_STATUS_MID_CALL_RECEIVE;
        }

        /* Populate the session part of the ISI event */
        _SAPP_populateIsiEvtSession(&service_ptr->mnsService, &call_ptr->mnsSession,
            &isi_ptr->msg.call);

        /*
         * The media & session info is now populated in the ISI event
         * so let's populate the rest of the ISI event.
         */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_ACCEPT, ISIP_STATUS_INVALID, isi_ptr);

        /* Let's send the current event and then send another */
        SAPP_sendEvent(evt_ptr);

        /*
         * Generate ACCEPT_ACK event for notifying ISI later that SAPP sent
         * the ACK so that ISI could update the stream direction.
         */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_ACCEPT_ACK, ISIP_STATUS_INVALID, isi_ptr);

        if (!MNS_isSessionActive(&call_ptr->mnsSession)) {
            /*
             * Got SDP offer in 200 OK, generate SDP answer in ACK.
             * Run MNS state machine
             */
            MNS_processCommand(&service_ptr->mnsService, 
                    &call_ptr->mnsSession, isi_ptr);
        }

        /* Send ACK to peer */
        UA_Ack(service_ptr->sipConfig.uaId, hDialog, NULL, 0,
                call_ptr->mnsSession.sess_ptr,
                &service_ptr->sipConfig.localConn);

        if (uaEvt_ptr->isConference) {
            /* Let's get the 'identity' of the conference. */
            OSAL_snprintf(call_ptr->conf.identity, SAPP_STRING_SZ, "%s",
                    uaEvt_ptr->szContactUri);
            /* Then let's subscribe to the conference event package. */
            SAPP_dbgPrintf("%s:%d call SAPP_conferenceSubscribe\n",
                    __FUNCTION__, __LINE__);
            SAPP_conferenceSubscribe(&call_ptr->conf.dialogId, service_ptr,
                    call_ptr->conf.identity,
                    service_ptr->sipConfig.config.aor[0].szUri, NULL);
        }


        break;
    case eUA_CALL_ATTEMPT:
        /*
         * If the replace dialog exist, generate ISIP_CALL_REASON_MODIFY to 
         * modify the existing call.
         */
        if (call_ptr->replaceDialogId) {
            /*
             * This new incoming call is attempting to replace an existing
             * call.
             */
            if (MNS_OK == MNS_processEvent(&service_ptr->mnsService,
                    &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                /* Populate the session part of the ISI event */
                _SAPP_populateIsiEvtSession(&service_ptr->mnsService, 
                        &call_ptr->mnsSession,
                        &isi_ptr->msg.call);

                /* Cache Contact if it's from conference server. */
                if (uaEvt_ptr->isConference) {
                    /* Let's get the 'identity' of the conference. */
                    SAPP_dbgPrintf("%s:%d copy conf-uri in re-invite\n",
                            __FUNCTION__, __LINE__);
                    OSAL_snprintf(call_ptr->conf.identity, SAPP_STRING_SZ, "%s",
                            uaEvt_ptr->szContactUri);
                }

                SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                        ISIP_STATUS_INVALID, isi_ptr);
            }
            else {
                /* Negotiation failed. Respond 415 to the new incoming call. */
                UA_Respond(service_ptr->sipConfig.uaId,
                        call_ptr->replaceDialogId, 415, 0, 0,
                        NULL, NULL, 0, NULL, 0);
                call_ptr->replaceDialogId = 0;
            }
        }
        else {
            /* See if the remote side specified the use of prack, mark it if so */
            call_ptr->usePrack = SAPP_parseHfValueExist("Require:", "100rel",
                    uaEvt_ptr);
            /*
             * Check if this call is allowed by verifying that the remote user
             * (URI) is not blocked.
             */
            if (SAPP_searchBlockedUser(&service_ptr->blockedUsers,
                    uaEvt_ptr->szRemoteUri, 0) == SAPP_OK) {
                /*
                 * Then the URI of the remote party is 'blocked' return an
                 * appropriate response to SIP and destroy the call
                 */
                UA_Respond(service_ptr->sipConfig.uaId, hDialog, 603, 0, 0,
                        "You are denied by callee", NULL, 0, NULL, 0);
                SAPP_sipDestroyCall(call_ptr);
            }
            else {
                /* Cache Contact if it's from conference server. */
                if (uaEvt_ptr->isConference) {
                    /* Let's get the 'identity' of the conference. */
                    SAPP_dbgPrintf("%s:%d copy conf-uri in invite\n",
                            __FUNCTION__, __LINE__);
                    OSAL_snprintf(call_ptr->conf.identity, SAPP_STRING_SZ, "%s",
                            uaEvt_ptr->szContactUri);
                }
                if (_SAPP_sipCallInitiateInbound(sip_ptr, service_ptr, call_ptr,
                    hDialog, uaEvt_ptr, evt_ptr) != SAPP_OK) {
                    SAPP_sipDestroyCall(call_ptr);
                }
            }
        }
        break;

    case eUA_TRANSFER_RINGING:
        /* Let ISI know about it with a 'progress' indication */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_TRANSFER_PROGRESS, ISIP_STATUS_INVALID,
                isi_ptr);
        break;

    case eUA_TRANSFER_COMPLETED:
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_TRANSFER_COMPLETED, ISIP_STATUS_INVALID,
                isi_ptr);
        break;

    case eUA_TRANSFER_FAILED:
        if (uaEvt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
            /* Server response 504 server time out. */
            if (SAPP_OK == _SAPP_sipServerTimeOut(service_ptr, &reason_ptr,
                    uaEvt_ptr, sip_ptr)) {
                /* do nothing. */
            }
        }

        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_TRANSFER_FAILED, ISIP_STATUS_INVALID,
                isi_ptr);
        break;

    case eUA_MEDIA_INFO:
        if (uaEvt_ptr->resp.respCode != 0) {
            if (uaEvt_ptr->resp.respCode < 200) {
                /* Then it's only a provisional response. Quietly discard */
                return;
            }
            /* Then it's a response to a re-invite */
            if (call_ptr->modify == SAPP_MODIFY_HOLD) {
                /* Always send DONE for HOLD no matter 
                 * negotiation failed or not */
                SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        call_ptr->isiCallId, ISIP_CALL_REASON_HOLD,
                        ISIP_STATUS_DONE, isi_ptr);

                if (uaEvt_ptr->resp.respCode >= 200 &&
                        uaEvt_ptr->resp.respCode < 300) {
                    /* Successful response. Run MNS state machine */
                    if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                            &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                        /* do nothing */
                    }
                }
            }
            else if (call_ptr->modify == SAPP_MODIFY_RESUME) {
                /* Always send DONE for RESUME no matter negotiation 
                 * failed or not */
                SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId,
                        call_ptr->isiCallId, ISIP_CALL_REASON_RESUME,
                        ISIP_STATUS_DONE, isi_ptr);

                if (uaEvt_ptr->resp.respCode >= 200 &&
                        uaEvt_ptr->resp.respCode < 300) {
                    /* Successful response. Run MNS state machine */
                    if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                            &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                        /* MNS failed. Populate the session part of the 
                         * ISI event */
                        _SAPP_populateIsiEvtSession(&service_ptr->mnsService, 
                                &call_ptr->mnsSession,
                                &isi_ptr->msg.call);

                        /* Let's send the current event and then send another */
                        SAPP_sendEvent(evt_ptr);

                        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                                service_ptr->protocolId,
                                call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                                ISIP_STATUS_INVALID, isi_ptr);
                    }
                }
            }
            else if (call_ptr->modify == SAPP_MODIFY_OTHER) {
                if (uaEvt_ptr->resp.respCode >= 200 &&
                    uaEvt_ptr->resp.respCode < 300) {

                    if (MNS_OK == MNS_processEvent(&service_ptr->mnsService,
                            &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                        /* Populate the session part of the ISI event */
                        _SAPP_populateIsiEvtSession(&service_ptr->mnsService, 
                                &call_ptr->mnsSession,
                                &isi_ptr->msg.call);

                        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                                service_ptr->protocolId,
                                call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                                ISIP_STATUS_DONE, isi_ptr);
                    }
                    else {
                        /* Populate the session part of the ISI event */
                        _SAPP_populateIsiEvtSession(&service_ptr->mnsService, 
                                &call_ptr->mnsSession,
                                &isi_ptr->msg.call);

                        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                                service_ptr->protocolId,
                                call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                                ISIP_STATUS_FAILED, isi_ptr);
                    }
                }
                else {
                    /*
                     * Change MNS state from MNS_STATE_ACTIVE_OFFER_SENT to
                     * MNS_STATE_ACTIVE, because there is no SDP in SIP error
                     * response of re-invite.
                     */
                    if (MNS_OK == MNS_processEvent(&service_ptr->mnsService,
                            &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                        /* There is some problem, if return OK */
                        OSAL_logMsg("%s:%d MNS_processEvent shuld return "
                                "MNS_ERR because of no SDP\n",
                                __FUNCTION__, __LINE__);
                    }
                    _SAPP_updateMediaCoder(call_ptr);
                    
                    SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                            service_ptr->protocolId,
                            call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                            ISIP_STATUS_FAILED, isi_ptr);
                }
            }
            /*
             * reset the indicator that tracks the current
             * modify transaction type
             */
            call_ptr->modify = SAPP_MODIFY_NONE;
            return;
        }

        /*
         * If we have try to send re-invite and modify media but receive
         * media change from remote side first, we need restore origianl
         * media setting and tell ISI original modification fail.
         */
        if (SAPP_MODIFY_NONE != call_ptr->modify) {
            SAPP_dbgPrintf("%s:%d Reset media attr.\n", __FUNCTION__, __LINE__);
            if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                    &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                SAPP_dbgPrintf("%s:%d MNS_processEvent change state FAILED.\n",
                        __FUNCTION__, __LINE__);
            }
            /* Restore media parameters */
            _SAPP_updateMediaCoder(call_ptr);
            for (x = 0; x < call_ptr->mnsSession.session.numMedia; x++) {
                call_ptr->mnsSession.session.media[x].lclDirection =
                        call_ptr->mnsSession.session.media[x].lclOriDir;
            }
            /* tell ISI original modify failed */
            SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                    service_ptr->protocolId,
                    call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                    ISIP_STATUS_FAILED, isi_ptr);
            SAPP_sendEvent(evt_ptr);
            call_ptr->modify = SAPP_MODIFY_NONE;
        }

        /*
         * If we are here then this 'eUA_MEDIA_INFO' event represents a new
         * inbound re-invite or sdp answer in the ACK to 200OK
         */

        if (!uaEvt_ptr->sessNew) {
            /* No sdp in the re-INVITE */

            /* Run MNS */
            MNS_processCommand(&service_ptr->mnsService, &call_ptr->mnsSession,
                    NULL);

            /* Send 200 OK with SDP contains our capabilities */
            UA_Answer(service_ptr->sipConfig.uaId, call_ptr->dialogId, NULL,
                    0, call_ptr->mnsSession.sess_ptr,
                    service_ptr->exchangeCapabilitiesBitmap,
                    &service_ptr->sipConfig.localConn);
            return;
        }

        if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
            /* Negotiation failed, reply 415 */
            UA_Respond(service_ptr->sipConfig.uaId, hDialog, 415, 0, 0,
                NULL, NULL, 0, NULL, 0);
            return;
        }

        /* Populate the session part of the ISI event */
        _SAPP_populateIsiEvtSession(&service_ptr->mnsService, 
                &call_ptr->mnsSession,
                &isi_ptr->msg.call);

        /*
         * The ISI message object has all the details regarding the coders
         * Let's load these details back into the coders for the SIP Call
         * object before we automatically "answer" the Re-INVITE.
         * First make a copy of the coders array since the contents will be
         * parsed.
         */
        OSAL_memCpy(coders, isi_ptr->msg.call.coders,
                sizeof(ISIP_Coder) * ISI_CODER_NUM);
        /* Load the coders in the ISI command into the call object. */
        _SAPP_decodeCoderIsi2Sapp(coders, ISI_CODER_NUM,
                call_ptr->coders, ISI_CODER_NUM);

        /* Final preparation of the call session stuff before we "answer" */
        _SAPP_updateMediaSapp2Sip(call_ptr, &isi_ptr->msg.call);

        /* Cache Contact if it's from conference server. */
        if (uaEvt_ptr->isConference && (0 == call_ptr->conf.identity[0])) {
            /* Let's get the 'identity' of the conference. */
            SAPP_dbgPrintf("%s:%d doing copy conf-uri in media-info\n", __FUNCTION__, __LINE__);
            OSAL_snprintf(call_ptr->conf.identity, SAPP_STRING_SZ, "%s",
                    uaEvt_ptr->szContactUri);
        }

        /* Right an event to ISI */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
               service_ptr->protocolId, call_ptr->isiCallId,
               ISIP_CALL_REASON_MODIFY, ISIP_STATUS_INVALID, isi_ptr);

        /* Don't answer now, answer until ISI accepts/rejects the modify */
        break;
    case eUA_TRANSFER_ATTEMPT:
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_TRANSFER_ATTEMPT,
                ISIP_STATUS_INVALID, isi_ptr);
        /* Also load the transfer target */
        OSAL_snprintf(isi_ptr->msg.call.to, ISI_ADDRESS_STRING_SZ,
                "%s", uaEvt_ptr->szToUri);
        
        break;
    case eUA_UPDATE:
        /*
         * This event is generated when the SIP stack gets
         * UPDATE with with session info ('early' media).
         * Try to negotiate and update ISI.
         */
        status = ISIP_STATUS_INVALID;
        if (uaEvt_ptr->sessNew) {
            if (SAPP_MODIFY_OTHER == call_ptr->modify) {
                /*
                 * We've sent SDP offer, it must be a failure due to 491 and
                 * then received remote's SDP offer.
                 */
                SAPP_dbgPrintf("%s:%d Reset media attr.\n", __FUNCTION__, __LINE__);
                if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                        &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                    SAPP_dbgPrintf("%s:%d MNS_processEvent change state FAILED.\n",
                            __FUNCTION__, __LINE__);
                }
                /* Restore media parameters */
                _SAPP_updateMediaCoder(call_ptr);
                for (x = 0; x < call_ptr->mnsSession.session.numMedia; x++) {
                    call_ptr->mnsSession.session.media[x].lclDirection =
                            call_ptr->mnsSession.session.media[x].lclOriDir;
                }
                call_ptr->modify = SAPP_MODIFY_NONE;
                status = ISIP_STATUS_DONE;
            }
            /* There is sdp in the UPDATE, run MNS. */
            if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                        &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 415, 0,
                        0, NULL, NULL, 0, NULL, 0);
                SAPP_sipDestroyCall(call_ptr);
                return;
            }
        }

        /* Stop resource reservation timer anyway */
        _SAPP_sipRsrcRsrvTimerStop(call_ptr); 
        /*
         * Session update
         * Populate the session part of the ISI event
         */
        _SAPP_populateIsiEvtSession(&service_ptr->mnsService,
                &call_ptr->mnsSession,
                &isi_ptr->msg.call);

        /*
         * The media & session info is now populated in the ISI event
         * so let's populate the rest of hte ISI event.
         */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_MODIFY, status, isi_ptr);
        break;
    case eUA_UPDATE_COMPLETED:
        if (!uaEvt_ptr->sessNew) {
            /* No sdp in the resposne to UDPATE, ignore it */
            break;
        }
        /* Then we have SDP answer here */

        /* MNS process event */
        if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                    &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
            UA_Respond(service_ptr->sipConfig.uaId, call_ptr->dialogId, 415,
                    0, 0, NULL, NULL, 0, NULL, 0);
            SAPP_sipDestroyCall(call_ptr);
            return;
        }

        /*
         * Session update
         * Populate the session part of the ISI event
         */
        _SAPP_populateIsiEvtSession(&service_ptr->mnsService,
                &call_ptr->mnsSession,
                &isi_ptr->msg.call);
    
        /* Send ISI event */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId,
                call_ptr->isiCallId, ISIP_CALL_REASON_MODIFY,
                ISIP_STATUS_DONE, isi_ptr);

        break;
    case eUA_ACK:
        /* ACK to INVITE 200 OK. There might be SDP in the ACK */
        if (uaEvt_ptr->sessNew) {
            /* There is sdp in the ACK, run MNS */
            if (MNS_OK != MNS_processEvent(&service_ptr->mnsService,
                        &call_ptr->mnsSession, event, uaEvt_ptr, isi_ptr)) {
                /* Negogiation failed, generates a ISI call terminate event */
                SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                        service_ptr->protocolId, call_ptr->isiCallId,
                        ISIP_CALL_REASON_TERMINATE, ISIP_STATUS_INVALID,
                        isi_ptr);
                SAPP_sipDestroyCall(call_ptr);
                break;
            }
    
            /* Populate the session part of the ISI event */
            _SAPP_populateIsiEvtSession(&service_ptr->mnsService, &call_ptr->mnsSession,
                    &isi_ptr->msg.call);
            /*
             * The ISI message object has all the details regarding the coders
             * Let's load these details back into the coders for the SIP Call
             * object before we automatically "answer" the Re-INVITE.
             * First make a copy of the coders array since the contents will be
             * parsed.
             */
            OSAL_memCpy(coders, isi_ptr->msg.call.coders,
                    sizeof(ISIP_Coder) * ISI_CODER_NUM);
            /* Load the coders in the ISI command into the call object. */
            _SAPP_decodeCoderIsi2Sapp(coders, ISI_CODER_NUM,
                    call_ptr->coders, ISI_CODER_NUM);

            /* Final preparation of the call session stuff before we "answer" */
            _SAPP_updateMediaSapp2Sip(call_ptr, &isi_ptr->msg.call);
        }
        else {
            /* Populate the session part of the ISI event */
            _SAPP_populateIsiEvtSession(&service_ptr->mnsService, &call_ptr->mnsSession,
                    &isi_ptr->msg.call);
        }

        /* Notify ISI that SAPP gets the ACK */
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId, call_ptr->isiCallId,
                ISIP_CALL_REASON_ACCEPT_ACK, ISIP_STATUS_INVALID, isi_ptr);

        break;
    case eUA_UPDATE_FAILED:
        /* Failed to update then hung up the call and notify the call FAILED */
        _SAPP_sipHungUp(service_ptr, hDialog, NULL);
        SAPP_sipCallIsiEvt(service_ptr->isiServiceId,
                service_ptr->protocolId,
                call_ptr->isiCallId,
                ISIP_CALL_REASON_FAILED, ISIP_STATUS_INVALID, isi_ptr);
        break;
    case eUA_INFO:
        break;
    case eUA_PRACK_FAILED:
    case eUA_PRACK_COMPLETED:
    default:
        break;
    } /* End of switch statement */
    return;
}


