/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 28012 $ $Date: 2014-08-08 17:36:29 +0800 (Fri, 08 Aug 2014) $
 *
 */

/*
 * This file is the voice read interface
 */
#include "vtspr.h"
#include "_vtspr_private.h"

/* ======== _VTSPR_LipSync_rtpTs() ========
 *
 * Notify the audio rtp packet timestamp.
 */
void _VTSPR_LipSync_rtpTs(
    uvint           infc,
    VTSPR_Queues    *q_ptr,
    JB_Pkt          *pkt_ptr)
{
#ifdef LIP_SYNC_ENABLE
    vint decType = _VTSPR_jbCoderToLocalCoder(pkt_ptr->type, pkt_ptr->pSize);
    q_ptr->eventMsg.infc = infc;
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_LIP_SYNC_AUDIO;
    q_ptr->eventMsg.msg.syncEngine.reason = VTSP_EVENT_LIP_SYNC_RTP_TS;
    q_ptr->eventMsg.msg.syncEngine.rtpTs = pkt_ptr->tsOrig / _VTSPR_getRtpClockRate(decType);
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
#endif
}

/* ======== _VTSPR_LipSync_rtcpRecv() ========
 *
 * Notify the audio RTCP SR recv.
 */
void _VTSPR_LipSync_rtcpRecv(
    uvint       infc,
    VTSPR_Queues      *q_ptr,
    _VTSPR_RtpObject   *rtp_ptr)
{
#ifdef LIP_SYNC_ENABLE
    uint32 ntpTimeMs = (rtp_ptr->ntpTime.sec * 1000) + (rtp_ptr->ntpTime.usec / 1000);
    uint32 rtpTimeMs = rtp_ptr->rtcpRtpTime / _VTSPR_getRtpClockRate(rtp_ptr->localDecoder);
    q_ptr->eventMsg.infc = infc;
    q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_LIP_SYNC_AUDIO;
    q_ptr->eventMsg.msg.syncEngine.reason = VTSP_EVENT_LIP_SYNC_RTCP_RECEIVED;
    q_ptr->eventMsg.msg.syncEngine.rtcpNtp = ntpTimeMs;
    q_ptr->eventMsg.msg.syncEngine.rtcpRtp = rtpTimeMs;
    VTSPR_sendEvent(q_ptr, &q_ptr->eventMsg, infc);
#endif
}

/*
 * ======== VTSPR_sendEvent() ========
 *
 * place event in event q
 *
 * infc specifies the queue to place the event into
 * 
 */
void VTSPR_sendEvent(
    VTSPR_Queues   *q_ptr,
    VTSP_EventMsg  *msg_ptr,
    uvint           infc)
{
    OSAL_MsgQId      qId;

    if (VTSP_INFC_GLOBAL == infc) { 
        qId = q_ptr->eventGlobalQ;
    }
    else { 
        qId = q_ptr->eventInfcQ[infc];
    }

    if (NULL != msg_ptr) { 
        /* send this msg */
        if (OSAL_SUCCESS != (OSAL_msgQSend(qId, (char *)msg_ptr, 
                        sizeof(VTSP_EventMsg), OSAL_NO_WAIT, NULL))) { 
            /* Queue Full error; application needs to service queue */
            _VTSP_TRACE(__FILE__, __LINE__);
        }

        /* After sending event, zero event buffer for next call
         */
        OSAL_memSet(msg_ptr, 0, sizeof(VTSP_EventMsg));
    }
}


