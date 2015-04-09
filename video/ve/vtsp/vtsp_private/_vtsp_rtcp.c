/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 29711 $ $Date: 2014-11-06 12:42:22 +0800 (Thu, 06 Nov 2014) $
 *
 */

#include "_vtsp_private.h"
#include <osal_net.h>
#include <osal_select.h>
#include "voice_net.h"

/*
 * ======== _VTSP_rtcpProcess() ========
 */
static void _VTSP_rtcpProcess(
    vint          infc,
    vint          streamId,
    vint          cPktLen,
    OSAL_MsgQId   qId,
    unsigned char *msg_ptr)
{
    _VTSP_RtcpEventMsg  event;
    unsigned char      *pkt_ptr;
    unsigned char       cVal;
    vint                curLen;
    vint                pktLen;
    uint32             *payload_ptr;
    uint32              temp32;
    uint32              temp;
    vint                count;
    vint                pktType;
    vint                payloadType;
    /* The current time will be converted to a 32 bit timestamp
     * fixed point NTP timestamp. The first 16 bits are in
     * seconds, and the last 16 are the fractional portion. */
    OSAL_SelectTimeval  currentTime;    /* OSAL format (sec,usec) */
    uint32              ntpMsw;         /* NTP Most  Significant Word */
    uint32              ntpLsw;         /* NTP Least Significant Word */

    /*
     * Check for a valid RTCP packet. The following checks are performed:
     * 1. RTP Version field must be 2.
     * 2. The payload type of the first packet must be an RR or SR.
     * 3. The padding bit should be 0 for all but the last packet.
     * 4. The overall length of the compound packet must be equal to the sum of
     *    the lengths of the individual packets.
     */

    curLen = 0;
    pkt_ptr = msg_ptr;

    cVal = *(pkt_ptr + 1);
    if ((cVal == _VTSP_RTCP_PTYPE_SR) || (cVal == _VTSP_RTCP_PTYPE_RR)) { 
        /* For SR or RR, check version and compound length */
        while (curLen < cPktLen) {
            /*
             * The first 3 bits of the packet are the version followed by the
             * padding flag. 
             */
            cVal = *pkt_ptr;
            if ((0xe0 & cVal) != 0x80) {
                /*
                 * Invalid packet detected.
                 */
                _VTSP_TRACE(__FILE__, __LINE__);
                return;
            }
            pktLen = (*(pkt_ptr + 2) << 8) + *(pkt_ptr + 3);
            curLen += (pktLen + 1) << 2;

            if (pktLen == 0) {
                _VTSP_TRACE(__FILE__, __LINE__);
                return;
            }
            pkt_ptr = msg_ptr + curLen;
        }

        if (curLen != cPktLen) {
            _VTSP_TRACE(__FILE__, __LINE__);
            return;
        }
    }
    else if (cVal == _VTSP_RTCP_PTYPE_XR) { 
        if (cPktLen < 8) { 
            return;
        }
    }
    else {
        /*
         * Invalid payload type for first packet
         */
        _VTSP_TRACE(__FILE__, __LINE__);
        return;
    }

    /*

     * Since the compound packet is valid, parse the appropriate packets and
     * place them on the queue.
     */
    payload_ptr = (uint32 *)msg_ptr;
    curLen = 0;

    OSAL_selectGetTime(&currentTime);
    ntpMsw = _VTSP_SEC_TO_NTP_MSW(currentTime.sec);
    ntpLsw = _VTSP_USEC_TO_NTP_LSW(currentTime.usec);
    event.receivedTime = _VTSP_NTP_64_TO_32(ntpMsw, ntpLsw);

    while (curLen < cPktLen) {
        temp   = *payload_ptr++;
        temp32 = OSAL_netHtonl(temp);
        pktLen = temp32 & 0xffff;
        count = (temp32 >> 24) & 0x1f;
        pktType = (temp32 & 0x00ff0000) >> 16;
        payloadType = (temp32 & 0x1f000000) >> 24;
        curLen += (pktLen + 1) << 2;

        switch (pktType) {
            case _VTSP_RTCP_PTYPE_SR:
                event.streamId = streamId;
                event.infc = infc;
                event.reason = VTSP_EVENT_RTCP_SR;
                temp = *payload_ptr++;
                event.ssrc = OSAL_netNtohl(temp);
                temp = *payload_ptr++;
                event.arg1 = OSAL_netNtohl(temp);
                temp = *payload_ptr++;
                event.arg2 = OSAL_netNtohl(temp);
                temp = *payload_ptr++;
                event.arg3 = OSAL_netNtohl(temp);
                temp = *payload_ptr++;
                event.arg4 = OSAL_netNtohl(temp);
                temp = *payload_ptr++;
                event.arg5 = OSAL_netNtohl(temp);
                event.arg6 = 0;
                pktLen -= 6;

                /*
                 * Send SR.
                 */
                if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                        _VTSP_Q_RTCP_EVENT_SZ,
                        OSAL_NO_WAIT, NULL)) {
                    _VTSP_TRACE(__FILE__, __LINE__);
                }
                /*
                 * An SR may consist of a sender block plus one or more receiver
                 * blocks.
                 */
                event.streamId = streamId;
                event.infc = infc;
                event.reason = VTSP_EVENT_RTCP_RR;
                while (pktLen > 0) {
                    temp = *payload_ptr++;
                    event.ssrc = OSAL_netHtonl(temp);
                    temp = *payload_ptr++;
                    temp32 = OSAL_netHtonl(temp);
                    event.arg1 = ((temp32) >> 24) & 0xff;   /* Fraction lost */
                    event.arg2 = temp32 & 0x00ffffff;       /* Cumulative number of of packets lost */
                    temp = *payload_ptr++;
                    event.arg3 = OSAL_netHtonl(temp);       /* Extended highest sequence number */
                    temp = *payload_ptr++;
                    event.arg4 = OSAL_netHtonl(temp);       /* Interarrival Jitter */
                    temp = *payload_ptr++;
                    event.arg5 = OSAL_netHtonl(temp);       /* Last SR */
                    temp = *payload_ptr++;
                    event.arg6 = OSAL_netHtonl(temp);       /* Delay since last SR (DLSR) */
                    pktLen -= 6;

                    if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                            _VTSP_Q_RTCP_EVENT_SZ,
                            OSAL_NO_WAIT, NULL)) {
                        _VTSP_TRACE(__FILE__, __LINE__);
                        payload_ptr += pktLen;
                        pktLen = 0;
                    }
                }
                break;
            case _VTSP_RTCP_PTYPE_RR:
                event.streamId = streamId;
                event.infc = infc;
                event.reason = VTSP_EVENT_RTCP_RR;
                payload_ptr++;
                pktLen--;
                while (pktLen > 0) {
                    temp = *payload_ptr++;
                    event.ssrc = OSAL_netHtonl(temp);
                    temp = *payload_ptr++;
                    temp32 = OSAL_netHtonl(temp);
                    event.arg1 = ((temp32) >> 24) & 0xff;   /* Fraction lost */
                    event.arg2 = temp32 & 0x00ffffff;       /* Cumulative number of of packets lost */
                    temp = *payload_ptr++;
                    event.arg3 = OSAL_netHtonl(temp);       /* Extended highest sequence number */
                    temp = *payload_ptr++;
                    event.arg4 = OSAL_netHtonl(temp);       /* Interarrival Jitter */
                    temp = *payload_ptr++;
                    event.arg5 = OSAL_netHtonl(temp);       /* Last SR */
                    temp = *payload_ptr++;
                    event.arg6 = OSAL_netHtonl(temp);       /* Delay since last SR (DLSR) */
                    pktLen -= 6;

                    if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                            _VTSP_Q_RTCP_EVENT_SZ, OSAL_NO_WAIT, NULL)) {
                        _VTSP_TRACE(__FILE__, __LINE__);
                        payload_ptr += pktLen;
                        pktLen = 0;
                    }
                }
                break;
            case _VTSP_RTCP_PTYPE_BYE:
                event.streamId = streamId;
                event.infc = infc;
                event.reason = VTSP_EVENT_RTCP_BYE;
                while (count > 0) {
                    temp = *payload_ptr++;
                    event.ssrc = OSAL_netHtonl(temp);
                    event.arg1 = 0;
                    event.arg2 = 0;
                    event.arg3 = 0;
                    event.arg4 = 0;
                    event.arg5 = 0;
                    event.arg6 = 0;
                    pktLen -= 1;
                    count -= 1;

                    if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                            _VTSP_Q_RTCP_EVENT_SZ, OSAL_NO_WAIT, NULL)) {
                        _VTSP_TRACE(__FILE__, __LINE__);
                        payload_ptr += pktLen;
                        pktLen = 0;
                    }
                }
                payload_ptr += pktLen;
                pktLen = 0;
                break;
            case _VTSP_RTCP_PTYPE_RTPFB:
                event.streamId = streamId;
                event.infc = infc;
                switch (payloadType) {
                    case _VTSP_RTCP_FMT_NACK:
                        event.reason = VTSP_EVENT_RTCP_FB_GNACK;
                        temp = *payload_ptr++;
                        event.ssrc = OSAL_netNtohl(temp);
                        pktLen --;
                        

                        temp = *payload_ptr++;
                        event.arg1 = OSAL_netNtohl(temp); // media ssrc
                        pktLen --;

                        /* 
                         * The FCI field MUST contain at least one and MAY contain more than one
                         * Generic NACK.
                         *
                         * Here will generate one event for each FCI.
                         */
                        while (pktLen > 0) {
                            temp = *payload_ptr++;
                            temp32 = OSAL_netHtonl(temp);
                            event.arg2 = ((temp32) >> 16) & 0xffff; // NACK PID
                            event.arg3 = temp32 & 0x0000ffff; // NACK BLP
                            pktLen --;

                            event.arg4 = 0;
                            event.arg5 = 0;
                            event.arg6 = 0;
                            /*
                             * Send NACK.
                             */
                            if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                                    _VTSP_Q_RTCP_EVENT_SZ,
                                    OSAL_NO_WAIT, NULL)) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                            }
                        }
                        break;
                    case _VTSP_RTCP_FMT_TMMBR:
                        event.reason = VTSP_EVENT_RTCP_FB_TMMBR;
                        temp = *payload_ptr++;
                        event.ssrc = OSAL_netNtohl(temp);

                        temp = *payload_ptr++;
                        event.arg1 = OSAL_netNtohl(temp); // media ssrc

                        /* 
                         * The FCI field of a Temporary Maximum Media Stream Bit Rate Request
                         * (TMMBR) message SHALL contain one or more FCI entries.
                         * However, we only support one video stream that will only contains 
                         * exactly one FCI field.
                         */
                        temp = *payload_ptr++;
                        event.arg2 = OSAL_netNtohl(temp); // TMMBR ssrc

                        temp = *payload_ptr++;
                        event.arg3 = OSAL_netNtohl(temp); // MxTBR Exp/MxTBR Mantissa/Measured Overhead

                        event.arg4 = 0;
                        event.arg5 = 0;
                        event.arg6 = 0;
                        /*
                         * Send TMMBR.
                         */
                        if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                                _VTSP_Q_RTCP_EVENT_SZ,
                                OSAL_NO_WAIT, NULL)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        break;
                    case _VTSP_RTCP_FMT_TMMBN:
                        event.reason = VTSP_EVENT_RTCP_FB_TMMBN;
                        temp = *payload_ptr++;
                        event.ssrc = OSAL_netNtohl(temp);
                        pktLen--;

                        temp = *payload_ptr++;
                        event.arg1 = OSAL_netNtohl(temp); // media ssrc
                        pktLen--;

                        /* 
                         * The FCI field of the TMMBN feedback message may contain zero, 
                         * one, or more TMMBN FCI entries. 
                         * However, we only support one video stream that will only contains 
                         * zero or one FCI field.
                         */
                        event.arg2 = 0;
                        event.arg3 = 0;
                        event.arg4 = 0;
                        event.arg5 = 0;
                        event.arg6 = 0;
                        if (pktLen > 0) {
                            temp = *payload_ptr++;
                            event.arg2 = OSAL_netNtohl(temp); // TMMBR ssrc

                            temp = *payload_ptr++;
                            event.arg3 = OSAL_netNtohl(temp); // MxTBR Exp/MxTBR Mantissa/Measured Overhead
                        }
                        /*
                         * Send TMMBN.
                         */
                        if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                                _VTSP_Q_RTCP_EVENT_SZ,
                                OSAL_NO_WAIT, NULL)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        break;
                    default:
                        /* unsupported payloadType, just ignore it. */
                        payload_ptr += pktLen;
                        break;
                }
                break;
            case _VTSP_RTCP_PTYPE_PSFB:
                event.streamId = streamId;
                event.infc = infc;
                switch (payloadType) {
                    case _VTSP_RTCP_FMT_PLI:
                        event.reason = VTSP_EVENT_RTCP_FB_PLI;
                        temp = *payload_ptr++;
                        event.ssrc = OSAL_netNtohl(temp);

                        temp = *payload_ptr++;
                        event.arg1 = OSAL_netNtohl(temp); // media ssrc

                        /* No FCI fields for PLI */
                        event.arg2 = 0;
                        event.arg3 = 0;
                        event.arg4 = 0;
                        event.arg5 = 0;
                        event.arg6 = 0;
                        /*
                         * Send PLI.
                         */
                        if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                                _VTSP_Q_RTCP_EVENT_SZ,
                                OSAL_NO_WAIT, NULL)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        break;
                    case _VTSP_RTCP_FMT_FIR:
                        event.reason = VTSP_EVENT_RTCP_FB_FIR;
                        temp = *payload_ptr++;
                        event.ssrc = OSAL_netNtohl(temp);

                        temp = *payload_ptr++;
                        event.arg1 = OSAL_netNtohl(temp); // media ssrc

                        /* FCI fields */
                        /* 
                         * The FCI field MUST contain one or more FIR entries.
                         * However, we only support one video stream that will only contains 
                         * exactly one FCI field.
                         */
                            
                        temp = *payload_ptr++;
                        event.arg2 = OSAL_netNtohl(temp); // FIR ssrc

                        temp = *payload_ptr++;
                        temp32 = OSAL_netHtonl(temp);
                        event.arg3 = ((temp32) >> 24) & 0xff; // sequence number

                        event.arg4 = 0;
                        event.arg5 = 0;
                        event.arg6 = 0;
                        /*
                         * Send FIR.
                         */
                        if (OSAL_SUCCESS != OSAL_msgQSend(qId, (char *)&event,
                                _VTSP_Q_RTCP_EVENT_SZ,
                                OSAL_NO_WAIT, NULL)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        break;
                    default:
                        /* unsupported payloadType, just ignore it. */
                        payload_ptr += pktLen;
                        break;
                }
                break;
            default:
                /*
                 * Ignore any other RTCP packet types.
                 */
                payload_ptr += pktLen;
                break;
        }
    }
    return;
}
/*
 * ======== _VTSP_rtcpInit() ========
 */
VTSP_Return _VTSP_rtcpInit(
    _RTCP_TaskContext *rtcpContext_ptr,
    uvint              numInfc,
    uvint              numStreamId)
{
    _RTCP_MsgTaskContext *msgTask_ptr;
    _RTCP_SktTaskContext *sktTask_ptr;
    uvint                 index;
    OSAL_TaskArg taskArg;

    rtcpContext_ptr->numInfc     = numInfc;
    rtcpContext_ptr->numStreamId = numStreamId;
    rtcpContext_ptr->numRtcp     = numInfc * numStreamId;
    
    msgTask_ptr = &(rtcpContext_ptr->msgTaskContext);
    sktTask_ptr = &(rtcpContext_ptr->sktTaskContext);

    msgTask_ptr->taskPriority = _VTSP_TASK_RTCP_MSG_PRIORITY;
    sktTask_ptr->taskPriority = _VTSP_TASK_RTCP_SKT_PRIORITY;

    /* Malloc RTCP socket object pointers */
    sktTask_ptr->rtcpSkt_ptr
            = (_VTSP_RtcpSktObj **) OSAL_memCalloc(rtcpContext_ptr->numRtcp,
            sizeof(_VTSP_RtcpSktObj *), 0);

    /* Malloc for video too */
    sktTask_ptr->rtcpSktVideo_ptr
            = (_VTSP_RtcpSktObj **) OSAL_memCalloc(_VTSP_STREAM_PER_INFC,
            sizeof(_VTSP_RtcpSktObj *), 0);
    
    /* 
     * Malloc RTCP socket objects
     */
    for (index = 0; index < rtcpContext_ptr->numRtcp; index++) {
        if (NULL == (sktTask_ptr->rtcpSkt_ptr[index]
                = (_VTSP_RtcpSktObj *)OSAL_memCalloc(1,
                sizeof(_VTSP_RtcpSktObj), 0))) {
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_INIT);
        }
    }

    /*
     * Malloc for video too
     */
    for (index = 0; index < _VTSP_STREAM_PER_INFC; index++) {
        if (NULL == (sktTask_ptr->rtcpSktVideo_ptr[index]
                = (_VTSP_RtcpSktObj *)OSAL_memCalloc(1,
                sizeof(_VTSP_RtcpSktObj), 0))) {
            _VTSP_TRACE(__FILE__, __LINE__);
            return (VTSP_E_INIT);
        }
    }

    /*
     * Create RTCP task semaphores to synchronize shutdown.
     */
    if (NULL == (msgTask_ptr->finishSemId = OSAL_semCountCreate(0))) { 
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INIT);
    }
    if (NULL == (sktTask_ptr->finishSemId = OSAL_semCountCreate(0))) { 
            _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_INIT);
    }
    taskArg = rtcpContext_ptr;
    if (NULL == (msgTask_ptr->taskId = OSAL_taskCreate("rtcpMsg",
             (OSAL_TaskPrio)msgTask_ptr->taskPriority, msgTask_ptr->stackSize,
             (OSAL_TaskPtr)_VTSP_rtcpTaskMsg, taskArg))) {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }
    if (NULL == (sktTask_ptr->taskId = OSAL_taskCreate("rtcpSocket",
             (OSAL_TaskPrio)sktTask_ptr->taskPriority, sktTask_ptr->stackSize,
             (OSAL_TaskPtr)_VTSP_rtcpTaskSocket, taskArg))) {
        _VTSP_TRACE(__FILE__,__LINE__);
        return (VTSP_E_INIT);
    }

    return (VTSP_OK);
}

/*
 * ======== _VTSP_rtcpShutdown() ========
 */
VTSP_Return _VTSP_rtcpShutdown(
        _RTCP_TaskContext *rtcpContext_ptr)
{
    _VTSP_RtcpCmdMsg      msg;
    _RTCP_MsgTaskContext *msgTask_ptr;
    _RTCP_SktTaskContext *sktTask_ptr;

    msgTask_ptr = &(rtcpContext_ptr->msgTaskContext);
    sktTask_ptr = &(rtcpContext_ptr->sktTaskContext);

    msg.command  = _VTSP_RTCP_CMD_SHUTDOWN;
    msg.infc     = 0;
    msg.streamId = 0;
    if (OSAL_SUCCESS != OSAL_msgQSend(msgTask_ptr->msgQId, (char *)&msg,
             sizeof(_VTSP_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        return (VTSP_E_RESOURCE);
    }

    if (NULL != msgTask_ptr->finishSemId) {
        OSAL_semAcquire(msgTask_ptr->finishSemId, 2000);
        OSAL_semDelete(msgTask_ptr->finishSemId);
    }

    if (NULL != sktTask_ptr->finishSemId) {
        OSAL_semAcquire(sktTask_ptr->finishSemId, 2000);
        OSAL_semDelete(sktTask_ptr->finishSemId);
    }

    OSAL_msgQDelete(msgTask_ptr->msgQId);
    OSAL_msgQDelete(msgTask_ptr->msgQVideoId);
    OSAL_msgQDelete(sktTask_ptr->msgQId);
    OSAL_msgQGrpDelete(&(msgTask_ptr->msgQGrp));


    return (VTSP_OK);
}

/*
 * ======== _VTSP_rtcpTaskMsg() ========
 */
OSAL_TaskReturn _VTSP_rtcpTaskMsg(
    OSAL_TaskArg taskArg)
{
    _RTCP_TaskContext    *rtcpContext_ptr; 
    _RTCP_MsgTaskContext *msgTask_ptr;
    OSAL_MsgQId           msgQId;
    vint                  msgSize;
    OSAL_NetSockId        msgSocket;
    OSAL_NetAddress       msgAddr;
    void                 *msg_ptr;
    uint16                controlPort;

    rtcpContext_ptr = (_RTCP_TaskContext *)taskArg;
    controlPort = rtcpContext_ptr->controlPort;
    msgTask_ptr = &(rtcpContext_ptr->msgTaskContext);

    msg_ptr = NULL;

    /*
     * Allocate memory for messages.
     */
    if (NULL == (msg_ptr = OSAL_memAlloc(_VTSP_Q_RTCP_MSG_SZ, 0))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    /*
     * Create message queue.
     */
    if (NULL == (msgTask_ptr->msgQId
            = OSAL_msgQCreate(
            "vtsp-rtcpmsgq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
            _VTSP_Q_RTCP_NUM_MSG,
            _VTSP_Q_RTCP_MSG_SZ,
            0))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    if (NULL == (msgTask_ptr->msgQVideoId
            = OSAL_msgQCreate(
            "vtsp-rtcpmsgqVideo",
#ifdef INCLUDE_4G_PLUS
            OSAL_MODULE_VPR,
#else
            OSAL_MODULE_VIDEO_VE,
#endif
            OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT__VTSP_RtcpCmdMsg,
            _VTSP_Q_RTCP_NUM_MSG,
            _VTSP_Q_RTCP_MSG_SZ,
            0))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    if (OSAL_FAIL == OSAL_msgQGrpCreate(
              &msgTask_ptr->msgQGrp)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    if (OSAL_FAIL == OSAL_msgQGrpAddQ(&msgTask_ptr->msgQGrp,
            msgTask_ptr->msgQId)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    if (OSAL_FAIL == OSAL_msgQGrpAddQ(&msgTask_ptr->msgQGrp,
            msgTask_ptr->msgQVideoId)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    /*
     * Open socket for sending messages to rtcpTaskSocket.
     */
    msgAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    msgAddr.port = controlPort;

    if (OSAL_FAIL == VOICE_NET_SOCKET(&msgSocket, OSAL_NET_SOCK_UDP)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }

    if (OSAL_FAIL == VOICE_NET_CONNECT_SOCKET(&msgSocket, &msgAddr)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }


    while ((msgSize = OSAL_msgQGrpRecv(
            &msgTask_ptr->msgQGrp,
            msg_ptr,
            _VTSP_Q_RTCP_MSG_SZ,
            OSAL_WAIT_FOREVER,
            &msgQId,
            NULL)) >= 0) {
        if (((_VTSP_RtcpCmdMsg *)msg_ptr)->command == _VTSP_RTCP_CMD_SHUTDOWN) {
            
            if (OSAL_FAIL == VOICE_NET_SOCKET_SEND_TO(&msgSocket, msg_ptr, &msgSize, 
                    &msgAddr)) {
                
                _VTSP_TRACE(__FILE__, __LINE__);
            }
          
            goto rtcpMsg_cleanup;
        }

        if (OSAL_FAIL == VOICE_NET_SOCKET_SEND_TO(&msgSocket, msg_ptr, &msgSize, 
                &msgAddr)) {
            _VTSP_TRACE(__FILE__, __LINE__);
            goto rtcpMsg_cleanup;
        }
    }
    if (msgSize < 0) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpMsg_cleanup;
    }
rtcpMsg_cleanup:
    if (NULL != msg_ptr) {
        OSAL_memFree(msg_ptr, 0);
    }
    OSAL_semGive(msgTask_ptr->finishSemId);

    _VTSP_TRACE("_VTSP_rtcpTaskMsg return", __LINE__);
    
    return (VTSP_OK);
}

/*
 * ======== _VTSP_rtcpTaskSocket() ========
 */
OSAL_TaskReturn _VTSP_rtcpTaskSocket(
    OSAL_TaskArg taskArg)
{
    void                 *msg_ptr;
    _VTSP_RtcpCmdMsg     *cmd_ptr;
    int                   pktLen;
    OSAL_NetSockId        msgSocket;
    OSAL_NetAddress       msgAddr;
    _RTCP_TaskContext    *rtcpContext_ptr;
    _RTCP_SktTaskContext *sktTask_ptr;
    _VTSP_RtcpSktObj     *rtcpSkt_ptr;
    uint16                controlPort;
    OSAL_SelectSet        currentReadSet;
    OSAL_SelectSet        masterReadSet;
    OSAL_Status           retVal;
    OSAL_Boolean          timeout;
    OSAL_Boolean          isset;
    vint                  infc;
    vint                  numInfc;
    vint                  streamId;
    vint                  numStreamId;
    vint                  rebind;
    vint                  index;
    
    msg_ptr = NULL;
    rtcpSkt_ptr = NULL;
    rtcpContext_ptr = (_RTCP_TaskContext *)taskArg;

    controlPort = rtcpContext_ptr->controlPort;
    numInfc     = rtcpContext_ptr->numInfc;
    numStreamId = rtcpContext_ptr->numStreamId;

    sktTask_ptr = &(rtcpContext_ptr->sktTaskContext);

    /*
     * Create message queue for communicating to VTSPR.
     */
    if (NULL == (sktTask_ptr->msgQId
            = OSAL_msgQCreate(
            "vtsp-rtcpevtq",
            OSAL_MODULE_AUDIO_VE, OSAL_MODULE_AUDIO_VE,
            OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
            _VTSP_Q_RTCP_NUM_EVENT,
            _VTSP_Q_RTCP_EVENT_SZ,
            0))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpSocket_cleanup;
    }

    if (NULL == (sktTask_ptr->msgQVideoId
            = OSAL_msgQCreate(
            "vtsp-rtcpevtqVideo",
            OSAL_MODULE_AUDIO_VE,
#ifdef INCLUDE_4G_PLUS
            OSAL_MODULE_VPR,
#else
            OSAL_MODULE_VIDEO_VE,
#endif
            OSAL_DATA_STRUCT__VTSP_RtcpEventMsg,
            _VTSP_Q_RTCP_NUM_EVENT,
            _VTSP_Q_RTCP_EVENT_SZ,
            0))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpSocket_cleanup;
    }

    /*
     * Allocate memory for message.
     */
    if (NULL == (msg_ptr = OSAL_memAlloc(_VTSP_Q_RTCP_MSG_SZ, 0))) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpSocket_cleanup;
    }
    cmd_ptr = (_VTSP_RtcpCmdMsg *)msg_ptr;

    /*
     * Open socket for sending messages to rtcpTaskSocket.
     */

    msgAddr.ipv4 = OSAL_netHtonl(OSAL_NET_INADDR_LOOPBACK);
    msgAddr.port = controlPort;

    if (OSAL_FAIL == VOICE_NET_SOCKET(&msgSocket, OSAL_NET_SOCK_UDP)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpSocket_cleanup;
    }
    
    if (OSAL_FAIL == VOICE_NET_BIND_SOCKET(&msgSocket, &msgAddr)) {
        _VTSP_TRACE(__FILE__, __LINE__);
        goto rtcpSocket_cleanup;
    }


    OSAL_selectSetInit(&masterReadSet);
    OSAL_selectAddId(&msgSocket, &masterReadSet);


    while (1) {
        /*
         * Re-establish descriptor list and timeouts each time through the loop.
         */
        currentReadSet = masterReadSet;
        /*
         * Wait for an active socket to become ready.
         */
        retVal = OSAL_select(&currentReadSet, NULL, NULL, &timeout);

        /*
         * Find the descriptor that woke up the process.
         */
        if ((OSAL_FAIL == retVal) || (OSAL_TRUE == timeout)) {
            /*
             * Select error.
             */
             _VTSP_TRACE(__FILE__, __LINE__);
             goto rtcpSocket_cleanup;
        }
        /*
         * Messages to this socket are commands.
         */
        OSAL_selectIsIdSet(&msgSocket, &currentReadSet, &isset);
        if (OSAL_TRUE == isset) {
            pktLen = _VTSP_Q_RTCP_MSG_SZ;
            retVal = VOICE_NET_SOCKET_RECEIVE_FROM(&msgSocket, (char *)cmd_ptr,
                    &pktLen, &msgAddr);
            if ((pktLen <= 0) || (OSAL_FAIL == retVal)) {
                _VTSP_TRACE(__FILE__, __LINE__);
            }
            else {
                /*
                 * process message
                 */
                infc = cmd_ptr->infc;
                streamId = cmd_ptr->streamId;
                rtcpSkt_ptr = _VTSP_streamIdToRtcpSktPtr(sktTask_ptr, infc,
                        streamId);

                switch (cmd_ptr->command) {
                    case _VTSP_RTCP_CMD_CLOSE:
                        if (rtcpSkt_ptr->active !=
                                _VTSP_RTCP_SOCKET_STATE_CLOSED) {
                            rtcpSkt_ptr->active
                                    = _VTSP_RTCP_SOCKET_STATE_CLOSED;
                            OSAL_selectRemoveId(&rtcpSkt_ptr->socketFd,
                                    &masterReadSet);
                            VOICE_NET_CLOSE_SOCKET(&rtcpSkt_ptr->socketFd);
                        }
                        break;
                    case _VTSP_RTCP_CMD_OPEN:
                        OSAL_logMsg("%s:%d RTCP CMD OPEN: %d\n", __FILE__, __LINE__, infc);
                        /*
                         * Initialize Interface
                         */
                        rtcpSkt_ptr->infc = cmd_ptr->msg.open.infc;
                        /*
                         * If an active port changes address, close the port
                         * then open and bind it.
                         */
                        rebind = 0;
                        if ((rtcpSkt_ptr->active ==
                                _VTSP_RTCP_SOCKET_STATE_ACTIVE) &&
                                (( rtcpSkt_ptr->ipAddr.ipv4 !=
                                cmd_ptr->msg.open.localAddr.ipv4) || (
                                rtcpSkt_ptr->ipAddr.port !=
                                cmd_ptr->msg.open.localAddr.port))) {
                            rtcpSkt_ptr->active
                                    = _VTSP_RTCP_SOCKET_STATE_CLOSED;
                            OSAL_selectRemoveId(&rtcpSkt_ptr->socketFd,
                                    &masterReadSet);
                            VOICE_NET_CLOSE_SOCKET(&rtcpSkt_ptr->socketFd);
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }

                        /*
                         * Open socket.
                         */
                        if (rtcpSkt_ptr->active ==
                                _VTSP_RTCP_SOCKET_STATE_CLOSED) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                            if (OSAL_FAIL == VOICE_NET_SOCKET(
                                    &rtcpSkt_ptr->socketFd,
                                    cmd_ptr->msg.open.localAddr.type)) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                                break;
                            }
                            rtcpSkt_ptr->active = _VTSP_RTCP_SOCKET_STATE_OPEN;
                        }

                        /*
                         * Configure the IP TOS for RTCP
                         */
                        if (OSAL_FAIL == VOICE_NET_SET_SOCKET_OPTIONS(
                                &rtcpSkt_ptr->socketFd, OSAL_NET_IP_TOS,
                                cmd_ptr->msg.open.tos)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                            break;
                        }

                        if (rtcpSkt_ptr->active ==
                                _VTSP_RTCP_SOCKET_STATE_OPEN) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                            rebind = 1;
                        }
                        /*
                         * Bind the address to the specified socket if it is
                         * new.
                         */
                        if ((cmd_ptr->msg.open.localAddr.port != 0)
                                && (rebind == 1)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                            /*
                             * Create the new bind address.
                             */
                            OSAL_memCpy(&rtcpSkt_ptr->ipAddr,
                                        &cmd_ptr->msg.open.localAddr,
                                        sizeof(OSAL_NetAddress));

                            if (OSAL_FAIL == VOICE_NET_BIND_SOCKET(
                                    &rtcpSkt_ptr->socketFd,
                                    &rtcpSkt_ptr->ipAddr)) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                            }
                            else {
                                /*
                                 * Set sendto address.
                                 */
                                OSAL_memCpy(&rtcpSkt_ptr->ipAddr,
                                            &cmd_ptr->msg.open.remoteAddr,
                                            sizeof(OSAL_NetAddress));
                                rtcpSkt_ptr->active =
                                        _VTSP_RTCP_SOCKET_STATE_ACTIVE;
                                OSAL_selectAddId(&rtcpSkt_ptr->socketFd,
                                        &masterReadSet);
                            }
                        }
                        else {
                            _VTSP_TRACE(__FILE__, __LINE__);
                            if (rtcpSkt_ptr->active !=
                                    _VTSP_RTCP_SOCKET_STATE_CLOSED) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                                rtcpSkt_ptr->active =
                                        _VTSP_RTCP_SOCKET_STATE_CLOSED;
                                OSAL_selectRemoveId(&rtcpSkt_ptr->socketFd,
                                    &masterReadSet);
                                VOICE_NET_CLOSE_SOCKET(&rtcpSkt_ptr->socketFd);
                            }
                        }
                        break;
                    case _VTSP_RTCP_CMD_SEND:
                        OSAL_logMsg("%s:%d RTCP CMD SEND: %d\n", __FILE__, __LINE__, infc);
                        if (_VTSP_RTCP_SOCKET_STATE_ACTIVE
                                == rtcpSkt_ptr->active) {
                            pktLen = cmd_ptr->payloadSize;
                            retVal = VOICE_NET_SOCKET_SEND_TO(
                                    &rtcpSkt_ptr->socketFd,
                                    (char *)cmd_ptr->msg.payload,
                                    &pktLen,
                                    &rtcpSkt_ptr->ipAddr);
                            if ((cmd_ptr->payloadSize != pktLen) ||
                                    (OSAL_FAIL == retVal)) {
                                _VTSP_TRACE(__FILE__, __LINE__);
                            }
                        }
                        break;
                    case _VTSP_RTCP_CMD_SHUTDOWN:
                        goto rtcpSocket_cleanup;
                    default:
                        break;
                }
            }
        }
        /*
         * Scan through the list of network sockets. Only check those sockets
         * that have been activated.
         */
        for (infc = 0; infc < numInfc; infc++) {
            for (streamId =0; streamId < numStreamId; streamId++) {
                rtcpSkt_ptr = _VTSP_streamIdToRtcpSktPtr(sktTask_ptr, infc,
                        streamId);
                if (_VTSP_RTCP_SOCKET_STATE_ACTIVE == rtcpSkt_ptr->active) {
                    /*
                     * If the socket has data, process it.
                     */
                    OSAL_selectIsIdSet(&rtcpSkt_ptr->socketFd, &currentReadSet,
                            &isset);
                    if (OSAL_TRUE == isset) {
                        pktLen = _VTSP_Q_RTCP_MSG_SZ;
                        retVal = VOICE_NET_SOCKET_RECEIVE_FROM(
                                &rtcpSkt_ptr->socketFd, msg_ptr, &pktLen,
                                &msgAddr);
                        if ((pktLen <= 0) || (OSAL_FAIL == retVal)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        else {
                            /*
                             * After processing data, send it.
                             */
                            _VTSP_rtcpProcess(infc, streamId, pktLen,
                                    sktTask_ptr->msgQId, msg_ptr);
                        }
                    }
                }
            }
        }
        /*
         * For video as well.
         */
        for (infc = VTSP_INFC_VIDEO; VTSP_INFC_VIDEO == infc; infc++) {
            for (streamId =0; streamId < _VTSP_STREAM_PER_INFC; streamId++) {
                rtcpSkt_ptr = _VTSP_streamIdToRtcpSktPtr(sktTask_ptr, infc,
                        streamId);
                if (_VTSP_RTCP_SOCKET_STATE_ACTIVE == rtcpSkt_ptr->active) {
                    /*
                     * If the socket has data, process it.
                     */
                    OSAL_selectIsIdSet(&rtcpSkt_ptr->socketFd, &currentReadSet,
                            &isset);
                    if (OSAL_TRUE == isset) {
                        pktLen = _VTSP_Q_RTCP_MSG_SZ;
                        retVal = VOICE_NET_SOCKET_RECEIVE_FROM(
                                &rtcpSkt_ptr->socketFd, msg_ptr, &pktLen,
                                &msgAddr);
                        if ((pktLen <= 0) || (OSAL_FAIL == retVal)) {
                            _VTSP_TRACE(__FILE__, __LINE__);
                        }
                        else {
                            /*
                             * After processing data, send it.
                             */
                            _VTSP_rtcpProcess(infc, streamId, pktLen,
                                    sktTask_ptr->msgQVideoId, msg_ptr);
                        }
                    }
                }
            }
        }
    }
rtcpSocket_cleanup:
    if (NULL != msg_ptr) {
        OSAL_memFree(msg_ptr, 0);
    }

    if (NULL != sktTask_ptr->rtcpSkt_ptr) {
        for (index = 0; index < rtcpContext_ptr->numRtcp; index++) {
            if (NULL != sktTask_ptr->rtcpSkt_ptr[index]) {
                OSAL_memFree(sktTask_ptr->rtcpSkt_ptr[index], 0);
            }
        }
        OSAL_memFree(sktTask_ptr->rtcpSkt_ptr, 0);
    }
    if (NULL != sktTask_ptr->rtcpSktVideo_ptr) {
        for (index = 0; index < _VTSP_STREAM_PER_INFC; index++) {
            if (NULL != sktTask_ptr->rtcpSktVideo_ptr[index]) {
                OSAL_memFree(sktTask_ptr->rtcpSktVideo_ptr[index], 0);
            }
        }
        OSAL_memFree(sktTask_ptr->rtcpSktVideo_ptr, 0);
    }

    OSAL_semGive(sktTask_ptr->finishSemId);
    
    _VTSP_TRACE("_VTSP_rtcpTaskSocket return", __LINE__);
    
    return (VTSP_OK);
}

