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
#include "_vc_private.h"

/*
 * ======== _VC_sendVtspEvent() ========
 *
 * place event in vtsp event q
 * 
 */
void _VC_sendVtspEvent(
    _VC_Queues   *q_ptr,
    VTSP_EventMsg  *msg_ptr)
{
    if (NULL != msg_ptr) { 
        /* send this msg */
        if (OSAL_SUCCESS != (OSAL_msgQSend(q_ptr->eventQ, (char *)msg_ptr,
                        sizeof(VTSP_EventMsg), OSAL_NO_WAIT, NULL))) { 
            /* Queue Full error; vtsp needs to service queue */
            _VC_TRACE(__FILE__, __LINE__);
        }

        /* After sending event, zero event buffer for next call
         */
        OSAL_memSet(msg_ptr, 0, sizeof(VTSP_EventMsg));
    }
}

/*
 * ======== _VC_sendRtcpCommand() ========
 *
 * Send a command to RTCP via the command queue.
 *
 */
void _VC_sendRtcpCommand(
    _VC_Queues      *q_ptr,
    _VC_RtcpCmdMsg  *msg_ptr)
{
    if (NULL != msg_ptr) {
        /* send this msg */
        if (OSAL_SUCCESS != (OSAL_msgQSend(q_ptr->rtcpCmdQ, (char *)msg_ptr,
                        sizeof(_VC_RtcpCmdMsg), OSAL_NO_WAIT, NULL))) {
            /* Queue Full error; vtsp needs to service queue */
            _VC_TRACE(__FILE__, __LINE__);
        }
    }
}

/*
 * ======== _VC_sendVtspEvent() ========
 *
 * place event in App event q.
 * The events in this q will be polled by (Java Video Codec Engine - VCE)
 */
void _VC_sendAppEvent(
    _VC_Queues   *q_ptr,
    VC_Event    event,
    const char *eventDesc_ptr,
    vint        codecType)
{
    _VC_AppEventMsg    appEventMsg;

    appEventMsg.event = event;
    appEventMsg.codec = codecType;
    OSAL_strncpy(appEventMsg.eventDesc, eventDesc_ptr,
            VCI_EVENT_DESC_STRING_SZ);

    if (OSAL_SUCCESS != OSAL_msgQSend(q_ptr->appEventQ, (char *) &appEventMsg,
                    _VC_APP_Q_EVENT_MSG_SZ, OSAL_NO_WAIT, NULL)) {
        /* Queue Full error; application needs to service queue */
        _VC_TRACE(__FILE__, __LINE__);
    }
}

/* ======== _VC_LipSync_rtpTs() ========
 *
 * update video rtp packet timestamp.
 */
void _VC_LipSync_rtpTs(
    uvint        streamId,
    _VC_Queues      *q_ptr,
    JBV_Pkt       *pkt_ptr)
{
#ifdef LIP_SYNC_ENABLE
    q_ptr->eventMsg.infc = VTSP_INFC_VIDEO;
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_LIP_SYNC_VIDEO;
    q_ptr->eventMsg.msg.syncEngine.streamId = streamId;
    q_ptr->eventMsg.msg.syncEngine.reason = VTSP_EVENT_LIP_SYNC_RTP_TS;
    q_ptr->eventMsg.msg.syncEngine.rtpTs = pkt_ptr->tsOrig / _VC_VIDEO_CLOCK_RATE_IN_KHZ;
    _VC_sendVtspEvent(q_ptr, &q_ptr->eventMsg);
#endif
}

/* ======== _VC_LipSync_fpsAdjust() ========
 *
 * Adjust JBV fps.
 */
void _VC_LipSync_fpsAdjust(
    int32     audioVideoSkew,
    JBV_Obj    *obj_ptr)
{
#ifdef LIP_SYNC_ENABLE
    JBV_fpsAdjust(obj_ptr, audioVideoSkew); // Adjust Frame rate
#endif
}


/* ======== _VC_LipSync_rtcpRecv() ========
 *
 * Send Video RTCP received event to VTSP
 */
void _VC_LipSync_rtcpRecv(
    _VC_Queues       *q_ptr,
    _VC_RtcpObject   *rtcp_ptr)
{
#ifdef LIP_SYNC_ENABLE
    uint32 ntpTimeMs = (rtcp_ptr->ntpTime.sec * 1000) + (rtcp_ptr->ntpTime.usec / 1000);

    q_ptr->eventMsg.infc = VTSP_INFC_VIDEO;
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_LIP_SYNC_VIDEO;
    q_ptr->eventMsg.msg.syncEngine.reason = VTSP_EVENT_LIP_SYNC_RTCP_RECEIVED;
    q_ptr->eventMsg.msg.syncEngine.rtcpNtp = ntpTimeMs;
    q_ptr->eventMsg.msg.syncEngine.rtcpRtp = rtcp_ptr->rtpTime / _VC_VIDEO_CLOCK_RATE_IN_KHZ;
    _VC_sendVtspEvent(q_ptr, &q_ptr->eventMsg);
#endif
}
