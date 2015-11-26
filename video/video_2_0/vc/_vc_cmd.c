/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12730 $ $Date: 2010-08-09 20:55:01 -0400 (Mon, 09 Aug 2010) $
 *
 */

/*
 * This process the VTSP commands.
 */
#include "_vc_private.h"
#include "video_net.h"

void _VC_videoPrintCommand(
    _VTSP_CmdMsgCode code) 
{
    switch (code) {
        case _VTSP_CMD_STREAM_VIDEO_START:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_START\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_END:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_END\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_MODIFY\n");
            break;
        case _VTSP_CMD_RTCP_CNAME:
            OSAL_logMsg("command video _VTSP_CMD_RTCP_CNAME\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_QUERY:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_QUERY\n");
            break;
        case _VTSP_CMD_NO_OP:
            OSAL_logMsg("command video _VTSP_CMD_NO_OP\n");
            break;
        case _VTSP_CMD_CONFIG:
            OSAL_logMsg("command video _VTSP_CMD_CONFIG\n");
            break;
        case _VTSP_CMD_QUERY:
            OSAL_logMsg("command video _VTSP_CMD_QUERY\n");
            break;
        case _VTSP_CMD_GET_VERSION:
            OSAL_logMsg("command video _VTSP_CMD_GET_VERSION\n");
            break;
        case _VTSP_CMD_GET_INFO:
            OSAL_logMsg("command video _VTSP_CMD_GET_INFO\n");
            break;
        case _VTSP_CMD_STREAM_VIDEO_SYNC:
            OSAL_logMsg("command video _VTSP_CMD_STREAM_VIDEO_SYNC\n");
            break;
        default:
            OSAL_logMsg("command video VTSP code=%d\n", code);
    }
}

/*
 * ======== _VC_activeInfcStreamFilterStart ========
 *
 * Limits the number of active streams/interfaces upon stream startup.
 *
 */
VTSP_Return _VC_activeInfcStreamFilterStart(
    _VC_Dsp    *dsp_ptr,
    vint        streamId,
    _VC_Queues *q_ptr)
{
    _VC_StreamObj *stream_ptr;
    
    
    stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

    if (!stream_ptr->curActive) {
        if(dsp_ptr->curActiveStreams < _VTSP_STREAM_PER_INFC) {
            dsp_ptr->curActiveStreams++;
            stream_ptr->curActive = 1;
            stream_ptr->enc.VideoEncObj.pkt.num = -1;

            /* Send stream unavailable event */
            if (dsp_ptr->curActiveStreams >= _VTSP_STREAM_PER_INFC) {
                q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
                q_ptr->eventMsg.msg.resource.reason = 
                        VTSP_EVENT_REC_STREAM_UNAVAILABLE;
                q_ptr->eventMsg.msg.resource.streamId = streamId;
                _VC_sendVtspEvent(q_ptr, &q_ptr->eventMsg);
            }
        }

    }
    else {
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
        q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_UNAVAILABLE;
        q_ptr->eventMsg.msg.resource.streamId = streamId;
        _VC_sendVtspEvent(q_ptr, &q_ptr->eventMsg);
        return (VTSP_E_RESOURCE);
    }
    
    return (VTSP_OK);
}

/*
 * ======== _VC_activeInfcStreamFilterStart ========
 *
 * Updates the number of active streams/interfaces upon stream end.
 *
 */
void _VC_activeInfcStreamFilterEnd(
    _VC_Dsp    *dsp_ptr,
    vint        streamId,
    _VC_Queues *q_ptr)
{
    _VC_StreamObj *stream_ptr;
    
    stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
    
    /* Deactivate stream, if active */
    if (stream_ptr->curActive) {
        stream_ptr->curActive = 0;

        dsp_ptr->curActiveStreams--;
        
        /* Send stream available event */
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
        q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_AVAILABLE;
        q_ptr->eventMsg.msg.resource.streamId = streamId;
        _VC_sendVtspEvent(q_ptr, &q_ptr->eventMsg);
    }
    return;
}

static void _VC_notifyAppWhenStreamChange(
    _VC_Queues    *q_ptr,
    _VC_StreamObj *stream_ptr)

{
    /* Notify App about Coder start/stop. */
    if (VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir) {
        OSAL_logMsg("%s: the dir param is VTSP_STREAM_DIR_SENDONLY\n", __FUNCTION__);
        if (!stream_ptr->enc.encRunning) {
            /* Enc is currently NOT running. Notify App to Start Enc. */
            stream_ptr->enc.encRunning = 1;
            _VC_sendAppEvent(q_ptr, VC_EVENT_START_ENC, "VC - Start Enc", stream_ptr->streamParam.encoder);
        }
        else {
            /* Enc should be running and Enc IS currently running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }

        if (stream_ptr->dec.decRunning) {
            /* Dec is currently running. Notify App to Stop Dec. */
            stream_ptr->dec.decRunning = 0;
            _VC_sendAppEvent(q_ptr, VC_EVENT_STOP_DEC, "VC - Stop Dec", -1);
        }
        else {
            /* Dec should NOT be running and Dec is currently NOT running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }
    }
    else if (VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir) {
        OSAL_logMsg("%s: the dir param is VTSP_STREAM_DIR_RECVONLY\n", __FUNCTION__);
        if (stream_ptr->enc.encRunning) {
            /* Enc is currently running. Notify App to Stop Enc. */
            stream_ptr->enc.encRunning = 0;
            _VC_sendAppEvent(q_ptr, VC_EVENT_STOP_ENC, "VC - Stop Enc", -1);
        }
        else {
            /* Enc should NOT be running and Dec is currently NOT running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }

        if (!stream_ptr->dec.decRunning) {
            /* Dec is currently NOT running. */
            stream_ptr->dec.decRunning = 1;
            JBV_init(&stream_ptr->dec.jbObj);
            /* start the rtp receive task. */
            _VC_startRtpRecvTask(stream_ptr);
            /* Notify App to Start Dec. */
            _VC_sendAppEvent(q_ptr, VC_EVENT_START_DEC, "VC - Start Dec", stream_ptr->streamParam.encoder); // It's the same as EncoderType.
        }
        else {
            /* Dec should be running and Dec IS currently running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }
    }
    else if (VTSP_STREAM_DIR_SENDRECV == stream_ptr->streamParam.dir) {
        OSAL_logMsg("%s: the dir param is VTSP_STREAM_DIR_SENDRECV\n", __FUNCTION__);
        if (!stream_ptr->enc.encRunning) {
            /* Enc is currently not running. Notify App to Start Enc. */
            stream_ptr->enc.encRunning = 1;
            _VC_sendAppEvent(q_ptr, VC_EVENT_START_ENC, "VC - Start Enc", stream_ptr->streamParam.encoder);
        }
        else {
            /* Enc should be running and Enc IS currently running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }
        if (!stream_ptr->dec.decRunning) {
            /* Dec is currently NOT running. */
            stream_ptr->dec.decRunning = 1;
            JBV_init(&stream_ptr->dec.jbObj);
            /* start the rtp receive task. */
            _VC_startRtpRecvTask(stream_ptr);
            /* Notify App to Start Dec. */
            _VC_sendAppEvent(q_ptr, VC_EVENT_START_DEC, "VC - Start Dec", stream_ptr->streamParam.encoder); // It's the same as EncoderType.
        }
        else {
            /* Dec should be running and Dec IS currently running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }
    }
    else if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir)
            || (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir)) {
        OSAL_logMsg("%s: the dir param is VTSP_STREAM_DIR_INACTIVE or _VTSP_STREAM_DIR_ENDED\n", __FUNCTION__);
        if (stream_ptr->enc.encRunning) {
            /* Enc is currently running. Notify App to Stop Enc. */
            stream_ptr->enc.encRunning = 0;
            _VC_sendAppEvent(q_ptr, VC_EVENT_STOP_ENC, "VC - Stop Enc", -1);
        }
        else {
            /* Enc should NOT be running and Dec is currently NOT running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }

        if (stream_ptr->dec.decRunning) {
            /* Dec is currently running. Notify App to Stop Dec. */
            stream_ptr->dec.decRunning = 0;
            _VC_sendAppEvent(q_ptr, VC_EVENT_STOP_DEC, "VC - Stop Dec", -1);
        }
        else {
            /* Dec should NOT be running and Dec is currently NOT running. Ignore. */
            _VC_TRACE(__FILE__, __LINE__);
        }
    }
}

/*
 * ======== _VC_updateRtcpMinInterval ========
 *
 * Update RTCP Reduced min Internal parameter based on remote recv bw.
 *
 */
static void _VC_updateRtcpMinInterval(
    _VC_Queues *q_ptr,
    uint16 streamId)
{
    _VC_RtcpCmdMsg cmdMsg;

    cmdMsg.cmd = _VC_RTCP_CMD_UPDATE_MIN_INTERVAL;
    cmdMsg.streamId = streamId;

    _VC_sendRtcpCommand(q_ptr, &cmdMsg);
}

/*
 * ======== _VC_updateStream ========
 *
 * Update internal stream params based on VTSP command.
 *
 */
void _VC_updateStream(
    _VC_Queues           *q_ptr,
    _VC_Dsp              *dsp_ptr,
    _VC_StreamObj        *stream_ptr,
    VTSP_StreamVideo     *newParam_ptr)
{
    /* Copy stream params. */
    OSAL_memCpy(&stream_ptr->streamParam, newParam_ptr,
            sizeof(VTSP_StreamVideo));

    /* Notify Stream Changes to App. */
    _VC_notifyAppWhenStreamChange(q_ptr, stream_ptr);

    return;
}

/*
 * ======== _VC_runDnCmd() ========
 *
 * Process the VTSP Command.
 */
void _VC_runDnCmd(
    _VC_Obj     *vc_ptr)
{
    _VC_Queues         *q_ptr;
    _VC_Dsp            *dsp_ptr;
    _VTSP_CmdMsg       *cmd_ptr;
    _VC_StreamObj      *stream_ptr;
    _VC_RtpObject      *rtp_ptr;
    vint                infc;              /* must be signed */
    vint                coder;
    vint                dir;
    vint                errval;
    vint                streamId;
    int32               audioVideoSkew;
    OSAL_NetAddress     localRtcpAddr;
    OSAL_NetAddress     remoteRtcpAddr;

    q_ptr = vc_ptr->q_ptr;
    dsp_ptr = vc_ptr->dsp_ptr;
    cmd_ptr = &q_ptr->cmdMsg;

    infc = cmd_ptr->infc;
    /* Check if the infc is Video. */
    if (infc != VTSP_INFC_VIDEO) {
        OSAL_logMsg("%s: infc is not VTSP_INFC_VIDEO, return\n", __FUNCTION__);
        return;
    }

    _VC_videoPrintCommand(cmd_ptr->code);

    switch (cmd_ptr->code) {
        case _VTSP_CMD_STREAM_VIDEO_SYNC:
            streamId = cmd_ptr->msg.arg.arg0;
            audioVideoSkew = cmd_ptr->msg.arg.arg1;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            _VC_LipSync_fpsAdjust(audioVideoSkew, &stream_ptr->dec.jbObj);
            break;
        case _VTSP_CMD_CONFIG:
            switch (cmd_ptr->msg.config.templCode) {
                case VTSP_TEMPL_CODE_RTCP:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    _VC_rtcpSetControl(q_ptr, streamId, &cmd_ptr->msg.config);
                    break;
                case VTSP_TEMPL_CODE_RTP:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);
                    stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
                    switch (cmd_ptr->msg.config.u.data[0]) { 
                        case VTSP_TEMPL_CONTROL_RTP_TOS:
                            rtp_ptr->tos = cmd_ptr->msg.config.u.data[2];
                            if (OSAL_FAIL == VIDEO_NET_SET_SOCKET_OPTIONS(&rtp_ptr->socket,
                                    OSAL_NET_IP_TOS, rtp_ptr->tos)) {
                                _VC_TRACE(__FILE__, __LINE__);
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_RTP_TS_INIT:
                            rtp_ptr->tsRandom = cmd_ptr->msg.config.u.data[2];
                            break;
                        case VTSP_TEMPL_CONTROL_RTP_SN_INIT:
                            rtp_ptr->seqRandom = cmd_ptr->msg.config.u.data[2];
                            break;
                        default:
                            _VC_TRACE(__FILE__, __LINE__);
                            break;
                    }
                    break;
                    default:
                    _VC_TRACE(__FILE__, __LINE__);
                    break;
            }
            break;
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started to generate key frame. */
                _VC_TRACE(__FILE__, __LINE__);
                break;
            }

            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir)) {
                /* Stream must be sending and also active to generate key */
                _VC_TRACE(__FILE__, __LINE__);
                break;
            }
            /* NOTE: This is for legacy support. This does not run on dlc03 */
            _VC_sendAppEvent(q_ptr, VC_EVENT_SEND_KEY_FRAME, "VC - Send Key Frame", stream_ptr->streamParam.encoder);
            break;
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started to change resolution */
                _VC_TRACE(__FILE__, __LINE__);
                break;
            }

            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir)) {
                /* Stream must be sending and active to change resolution */
                _VC_TRACE(__FILE__, __LINE__);
                break;
            }
            stream_ptr->enc.requestedWidth = cmd_ptr->msg.arg.arg1;
            stream_ptr->enc.requestedHeight = cmd_ptr->msg.arg.arg2;
            /* XXX - Not implemented - Send Event to App to Request Resolution Change - Add code here. */
            break;

        case _VTSP_CMD_STREAM_VIDEO_MODIFY:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started in order to modify */
                //_VC_LOG("_VTSP_CMD_STREAM_VIDEO_MODIFY, dir is ENDED!\n");
                OSAL_logMsg("%s: _VTSP_CMD_STREAM_VIDEO_MODIFY, dir is ENDED!\n", __FUNCTION__);
                //_VC_TRACE(__FILE__, __LINE__);
                break;
            }

            /* Update internal stream data. */
            _VC_updateStream(q_ptr, dsp_ptr, stream_ptr, &cmd_ptr->msg.streamVideo);

            _VC_algStateStream(dsp_ptr, stream_ptr->streamParam.streamId,
                    0, _VC_ALG_STREAM_JB);

            /*
             * Update the ports used by RTP and RTCP.
             */
            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);

            _VC_rtpOpen(rtp_ptr, stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.remoteAddr,
                    stream_ptr->streamParam.localAddr,
                    stream_ptr->streamParam.rdnDynType,
                    stream_ptr->streamParam.srtpSecurityType,
                    stream_ptr->streamParam.srtpSendKey,
                    stream_ptr->streamParam.srtpRecvKey);

            /* Check and update RTCP min Interval. */
            _VC_updateRtcpMinInterval(q_ptr, streamId);
            OSAL_memCpy(&localRtcpAddr, &stream_ptr->streamParam.localAddr,
                        sizeof(localRtcpAddr));
            localRtcpAddr.port = stream_ptr->streamParam.localControlPort;
            OSAL_memCpy(&remoteRtcpAddr, &stream_ptr->streamParam.remoteAddr,
                        sizeof(remoteRtcpAddr));
            remoteRtcpAddr.port = stream_ptr->streamParam.remoteControlPort;
            _VC_rtcpOpen(vc_ptr, streamId, remoteRtcpAddr, localRtcpAddr);
            break;

        case _VTSP_CMD_STREAM_VIDEO_START:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            /* Initialize the encoder and decoder running flags to not running when stream starts. */
            stream_ptr->enc.encRunning = 0;
            stream_ptr->dec.decRunning = 0;

            /* Check active stream/interface limit */
            if (VTSP_OK != _VC_activeInfcStreamFilterStart(dsp_ptr,
                    streamId, q_ptr)) {
                _VC_TRACE(__FILE__, __LINE__);
                break;
            }
            
            /*
             * Set coder type to unavailable to force initialization when the
             * stream is started.
             */
            stream_ptr->enc.marker = 0;
            stream_ptr->enc.requestedWidth = 0;
            stream_ptr->enc.requestedHeight = 0;

            /*
             * Update internal stream data
             */
            _VC_updateStream(q_ptr, dsp_ptr, stream_ptr, &cmd_ptr->msg.streamVideo);

            _VC_algStateStream(dsp_ptr, stream_ptr->streamParam.streamId,
                    0, _VC_ALG_STREAM_JB);

            /*
             * Initialize the stream for sending RTP packets over network
             * sockets.
             */
            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);
            errval = _VC_rtpOpen(rtp_ptr, stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.remoteAddr,
                    stream_ptr->streamParam.localAddr,
                    stream_ptr->streamParam.rdnDynType,
                    stream_ptr->streamParam.srtpSecurityType,
                    stream_ptr->streamParam.srtpSendKey,
                    stream_ptr->streamParam.srtpRecvKey);

            if (_VC_RTP_ERROR == errval) {
                //DBG("RTP Error infc:%d", infc);
                //DBG("RTP Error remoteDataPort:%d",
                //OSAL_netNtohs(stream_ptr->streamParam.remoteAddr.port));
                OSAL_logMsg("RTP Error infc:%d", infc);
                OSAL_logMsg("RTP Error remoteDataPort:%d",
                OSAL_netNtohs(stream_ptr->streamParam.remoteAddr.port));
            }
            /*
             * Initialize the socket for send and receiving RTCP packets.
             */
            /* Check and update RTCP min Interval. */
            _VC_updateRtcpMinInterval(q_ptr, streamId);
            OSAL_memCpy(&localRtcpAddr, &stream_ptr->streamParam.localAddr,
                        sizeof(localRtcpAddr));
            localRtcpAddr.port = stream_ptr->streamParam.localControlPort;
            OSAL_memCpy(&remoteRtcpAddr, &stream_ptr->streamParam.remoteAddr,
                        sizeof(remoteRtcpAddr));
            remoteRtcpAddr.port = stream_ptr->streamParam.remoteControlPort;
            _VC_rtcpOpen(vc_ptr, streamId, remoteRtcpAddr, localRtcpAddr);
            break;
        case _VTSP_CMD_STREAM_VIDEO_END:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                break;
            }

            /* Set stream direction to End. */
            stream_ptr->streamParam.dir = _VTSP_STREAM_DIR_ENDED;

            /* Signal the RTCP Task to stop. */
            stream_ptr->rtcpEnable = 0;

            /* Notify App to stop coders as necessary. */
            _VC_notifyAppWhenStreamChange(q_ptr, stream_ptr);

            /* Turn off all stream algs */
            _VC_algStateStream(dsp_ptr, streamId, _VC_ALG_STREAM_JB, 0);

            /* Reset the encoder and decoder running flags when stream ends. */
            stream_ptr->enc.encRunning = 0;
            stream_ptr->dec.decRunning = 0;

            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);
            _VC_rtpClose(rtp_ptr);
            _VC_rtcpClose(q_ptr, streamId);

            /* Update active stream/interface counts */
            _VC_activeInfcStreamFilterEnd(dsp_ptr, streamId, q_ptr);
            
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            if (!stream_ptr->curActive) {
                break;
            }

            /*
             * If the stream direction changes from non-receive to receive, the
             * jitter buffer needs to reinitialize. Based on old state of dir.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir)) {
                _VC_algStateStream(dsp_ptr, cmd_ptr->msg.streamVideo.streamId,
                        0, _VC_ALG_STREAM_JB);
            }

            /*
             * Update direction by copying the current stream to the msg, then
             * updating the direction only and calling the main update
             * function.  This allows all changes to be made.
             */
            dir = cmd_ptr->msg.arg.arg1;
            OSAL_memCpy(&cmd_ptr->msg.streamVideo, &stream_ptr->streamParam,
                    sizeof(VTSP_StreamVideo));
            cmd_ptr->msg.streamVideo.dir = dir;
            _VC_updateStream(q_ptr, dsp_ptr, stream_ptr, &cmd_ptr->msg.streamVideo);

            _VC_algStateStream(dsp_ptr, stream_ptr->streamParam.streamId,
                    0, _VC_ALG_STREAM_JB);

            /*
             * Tell RTP that direction has changed.
             */
            rtp_ptr = _VC_streamIdToRtpPtr(vc_ptr->net_ptr, streamId);
            _VC_rtpDir(rtp_ptr, cmd_ptr->msg.arg.arg1);
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            if (!stream_ptr->curActive) {
                break;
            }
            coder = cmd_ptr->msg.arg.arg1;
            /* No DTMF Relay */
            stream_ptr->streamParam.encoder = coder;
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VC_streamIdToStreamPtr(dsp_ptr, streamId);
            if (!stream_ptr->curActive) {
                break;
            }
            stream_ptr->streamParam.confMask = cmd_ptr->msg.arg.arg1;
           break;
        case _VTSP_CMD_STREAM_VIDEO_QUERY:
        case _VTSP_CMD_NO_OP:
        case _VTSP_CMD_QUERY:
        case _VTSP_CMD_GET_VERSION:
        case _VTSP_CMD_GET_INFO:
        case _VTSP_CMD_RTCP_CNAME:
            /*
             * Update the CNAME structure with the new string.
             */
            _VC_rtcpSetCname(vc_ptr->net_ptr,
                    (const char *)(cmd_ptr->msg.cname.cname));
            break;
        default:
            _VC_LOG("Code:%d", cmd_ptr->code);
            break;
    }

    _VC_LOG("END");
}

