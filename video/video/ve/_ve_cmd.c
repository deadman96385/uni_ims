/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12730 $ $Date: 2010-08-09 20:55:01 -0400 (Mon, 09 Aug 2010) $
 *
 */

/*
 * This file handles commands sent to the driver which are pending
 * in the down-queue
 */


#include "_ve_private.h"
#include "video_net.h"

void _VE_videoPrintCommand(
    _VTSP_CmdMsgCode code) 
{
    switch (code) {
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_START:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_START\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_MODIFY\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_QUERY:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_QUERY\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_SYNC:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_SYNC\n"); 
            break;
        case _VTSP_CMD_STREAM_VIDEO_END:
            OSAL_logMsg("command _VTSP_CMD_STREAM_VIDEO_END\n"); 
            break;
        default:
            OSAL_logMsg("command <UNKNOWN> ???\n");
    }
}

/*
 * ======== _VE_activeInfcStreamFilterStart ========
 *
 * Limits the number of active streams/interfaces upon stream startup.
 *
 */
VTSP_Return _VE_activeInfcStreamFilterStart(
    _VE_Dsp *dsp_ptr,
    vint streamId,
    vint infc,
    _VE_Queues *q_ptr)
{
    _VE_StreamObj *stream_ptr;
    
    
    stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

    if (!stream_ptr->curActive) {
        if(dsp_ptr->curActiveStreams < _VTSP_STREAM_PER_INFC) {
            dsp_ptr->curActiveStreams++;
            stream_ptr->curActive = 1;

            /* Send stream unavailable event */
            if (dsp_ptr->curActiveStreams >= _VTSP_STREAM_PER_INFC) {
                q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
                q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
                q_ptr->eventMsg.msg.resource.reason = 
                        VTSP_EVENT_REC_STREAM_UNAVAILABLE;
                q_ptr->eventMsg.msg.resource.streamId = streamId;
                _VE_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
            }
        }

    }
    else {
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
        q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_UNAVAILABLE;
        q_ptr->eventMsg.msg.resource.streamId = streamId;
        _VE_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
        return (VTSP_E_RESOURCE);
    }
    
    return (VTSP_OK);
}

/*
 * ======== _VE_activeInfcStreamFilterStart ========
 *
 * Updates the number of active streams/interfaces upon stream end.
 *
 */
void _VE_activeInfcStreamFilterEnd(
    _VE_Dsp *dsp_ptr,
    vint streamId,
    vint infc,
    _VE_Queues *q_ptr)
{
    _VE_StreamObj *stream_ptr;
    
    stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
    
    /* Deactivate stream, if active */
    if (stream_ptr->curActive) {
        stream_ptr->curActive = 0;
        dsp_ptr->curActiveStreams--;
        
        /* Send stream available event */
        q_ptr->eventMsg.infc = VTSP_INFC_GLOBAL;
        q_ptr->eventMsg.code = VTSP_EVENT_MSG_CODE_RESOURCE;
        q_ptr->eventMsg.msg.resource.reason = VTSP_EVENT_REC_STREAM_AVAILABLE;
        q_ptr->eventMsg.msg.resource.streamId = streamId;
        _VE_sendEvent(q_ptr, &q_ptr->eventMsg, VTSP_INFC_GLOBAL);
    }
    return;
}

/*
 * ======== _VE_updateStream ========
 *
 * Update internal stream params based on VTSP command.
 *
 *
 * XXX Note this function may handle changes in stream direction,
 * currently, it does not.
 *
 */
void _VE_updateStream(
        _VE_Dsp       *dsp_ptr,
        vint             infc,
        _VE_StreamObj *stream_ptr,
        VTSP_StreamVideo     *newParam_ptr)
{
    /*
     * Copy stream params.
     */
    OSAL_memCpy(&stream_ptr->streamParam, newParam_ptr,
            sizeof(VTSP_StreamVideo));

    /*
     * Video encoder.
     * Turn on stream algorithms.
     * This will init where necessary.
     */
    _VE_algStateStream(dsp_ptr, infc,
            stream_ptr->streamParam.streamId,
            0,
            _VE_ALG_STREAM_JB);

    return;
}

/*
 * ======== _VE_runDnCmd() ========
 *
 * Get cmd from driver downstream queue
 * and run it
 */
void _VE_runDnCmd(
    _VE_Obj     *ve_ptr,
    _VE_Queues  *q_ptr,
    _VE_Dsp     *dsp_ptr,
    _VTSP_CmdMsg  *cmd_ptr)
{
    vint                 infc;              /* must be signed */
    _VE_StreamObj     *stream_ptr;
    _VE_RtcpObject   *rtcp_ptr;
    _VE_RtpObject    *rtp_ptr;
    vint                 coder;
    vint                 dir;
    vint                 errval;
    vint                 streamId;
    OSAL_NetAddress      localRtcpAddr;
    OSAL_NetAddress      remoteRtcpAddr;


    infc = cmd_ptr->infc;

    if (infc != VTSP_INFC_VIDEO) {
        _VE_TRACE(__FILE__, __LINE__);
        return;
    }

    _VE_videoPrintCommand(cmd_ptr->code);
    /*
     * Must stop the encoder task before updating any data / sockets.
     * 1 second is a reasonable timeout because the encoder is not
     * supposed to take that long to process a frame in that long.
     */
    if (_VTSP_CMD_STREAM_VIDEO_END != cmd_ptr->code) { // don't ignore END
        if (OSAL_FAIL == OSAL_semAcquire(ve_ptr->encStopSem, 10 * 1000)) {
            OSAL_logMsg("%s:%d ERROR dropping command",__FUNCTION__, __LINE__);
            return;
        }
    }

    switch (cmd_ptr->code) {
        case _VTSP_CMD_STREAM_VIDEO_QUERY:
        case _VTSP_CMD_NO_OP:
        default:
            _VE_TRACE(__FILE__, __LINE__);
            break;

        case _VTSP_CMD_CONFIG:
            switch (cmd_ptr->msg.config.templCode) {

                case VTSP_TEMPL_CODE_RTCP:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    rtcp_ptr = _VE_streamIdToRtcpPtr(ve_ptr->net_ptr,
                            infc, streamId);
                    _VE_rtcpSetControl(rtcp_ptr, cmd_ptr->msg.config.u.data[0],
                            cmd_ptr->msg.config.u.data[2]);
                    break;
                case VTSP_TEMPL_CODE_RTP:
                    streamId = cmd_ptr->msg.config.u.data[1];
                    rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr,
                            infc, streamId);
                    stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc,
                            streamId);
                    switch (cmd_ptr->msg.config.u.data[0]) { 
                        case VTSP_TEMPL_CONTROL_RTP_TOS:
                            rtp_ptr->tos = cmd_ptr->msg.config.u.data[2];
                            if (OSAL_FAIL == VIDEO_NET_SET_SOCKET_OPTIONS(
                                    &rtp_ptr->socket, OSAL_NET_IP_TOS,
                                    rtp_ptr->tos)) {
                                _VE_TRACE(__FILE__, __LINE__);
                            }
                            break;
                        case VTSP_TEMPL_CONTROL_RTP_TS_INIT:
                            rtp_ptr->tsRandom = cmd_ptr->msg.config.u.data[2];
                            break;
                        case VTSP_TEMPL_CONTROL_RTP_SN_INIT:
                            rtp_ptr->seqRandom = cmd_ptr->msg.config.u.data[2];
                            break;
                        default:
                            _VE_TRACE(__FILE__, __LINE__);
                            break;
                    }
                    break;
                    default:
                    _VE_TRACE(__FILE__, __LINE__);
                    break;
            }
            break;
        case _VTSP_CMD_QUERY:
            /* XXX */
            _VE_TRACE(__FILE__, __LINE__);
            break;

        case _VTSP_CMD_GET_VERSION:
            _VE_TRACE(__FILE__, __LINE__);
            break;
        case _VTSP_CMD_GET_INFO:
            /* XXX */
            _VE_TRACE(__FILE__, __LINE__);
            break;

        case _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started to gen key */
                _VE_TRACE(__FILE__, __LINE__);
                break;
            }

            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir)) {
                /* Stream must be send and active to gen key */
                _VE_TRACE(__FILE__, __LINE__);
                break;
            }
            stream_ptr->enc.requestKey = 1;
            break;

        case _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started to gen key */
                _VE_TRACE(__FILE__, __LINE__);
                break;
            }

            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_RECVONLY == stream_ptr->streamParam.dir)) {
                /* Stream must be send and active to change resolution */
                _VE_TRACE(__FILE__, __LINE__);
                break;
            }
            stream_ptr->enc.requestedWidth = cmd_ptr->msg.arg.arg1;
            stream_ptr->enc.requestedHeight = cmd_ptr->msg.arg.arg2;
            break;

        case _VTSP_CMD_STREAM_VIDEO_MODIFY:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                /* Stream must be started in order to modify */
                _VE_TRACE(__FILE__, __LINE__);
                break;
            }

            /*
             * If the stream direction changes from non-receive to receive, the
             * jitter buffer needs to reinitialize.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir)) {
                _VE_algStateStream(dsp_ptr, infc,
                        streamId, 0, _VE_ALG_STREAM_JB);
            }

            /*
             * Update internal stream data
             */
            _VE_updateStream(dsp_ptr, infc,
                    stream_ptr, &cmd_ptr->msg.streamVideo);

            /*
             * Update the ports used by RTP and RTCP.
             */
            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc,
                    streamId);

            _VE_rtpOpen(rtp_ptr, stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.remoteAddr,
                    stream_ptr->streamParam.localAddr,
                    stream_ptr->streamParam.rdnDynType,
                    stream_ptr->streamParam.srtpSecurityType,
                    stream_ptr->streamParam.srtpSendKey,
                    stream_ptr->streamParam.srtpRecvKey);

            rtcp_ptr = _VE_streamIdToRtcpPtr(ve_ptr->net_ptr, infc,
                    streamId);

            OSAL_memCpy(&localRtcpAddr, &stream_ptr->streamParam.localAddr,
                        sizeof(localRtcpAddr));
            localRtcpAddr.port = stream_ptr->streamParam.localControlPort;
            OSAL_memCpy(&remoteRtcpAddr, &stream_ptr->streamParam.remoteAddr,
                        sizeof(remoteRtcpAddr));
            remoteRtcpAddr.port = stream_ptr->streamParam.remoteControlPort;
            _VE_rtcpOpen(q_ptr, rtcp_ptr, remoteRtcpAddr, localRtcpAddr);
            break;

        case _VTSP_CMD_STREAM_VIDEO_START:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);

            /* Check active stream/interface limit */
            if (VTSP_OK != _VE_activeInfcStreamFilterStart(dsp_ptr, streamId, 
                    infc, q_ptr)) {
                _VE_TRACE(__FILE__, __LINE__);
                break;
            }
            
            /*
             * Set coder type to unavailable to force initialization when the
             * stream is started.
             */
            stream_ptr->dec.decoderType = VTSP_CODER_UNAVAIL;
            stream_ptr->enc.lastEncoder = VTSP_CODER_UNAVAIL;
            stream_ptr->enc.marker = 0;
            stream_ptr->enc.requestedWidth = 0;
            stream_ptr->enc.requestedHeight = 0;

            /*
             * Update internal stream data
             */
            _VE_updateStream(dsp_ptr, infc,
                    stream_ptr, &cmd_ptr->msg.streamVideo);

            /*
             * Initialize the stream for sending RTP packets over network
             * sockets.
             */
            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc,
                    streamId);
            errval = _VE_rtpOpen(rtp_ptr, stream_ptr->streamParam.dir,
                    stream_ptr->streamParam.remoteAddr,
                    stream_ptr->streamParam.localAddr,
                    stream_ptr->streamParam.rdnDynType,
                    stream_ptr->streamParam.srtpSecurityType,
                    stream_ptr->streamParam.srtpSendKey,
                    stream_ptr->streamParam.srtpRecvKey);

            if (_VE_RTP_ERROR == errval) {
                _VE_TRACE(__FILE__, __LINE__);
                _VE_TRACE("RTP Error infc", infc);
                _VE_TRACE("RTP Error remoteDataPort",
                OSAL_netNtohs(stream_ptr->streamParam.remoteAddr.port));
            }
            /*
             * Initialize the socket for send and receiving RTCP packets.
             */
            rtcp_ptr = _VE_streamIdToRtcpPtr(ve_ptr->net_ptr, infc,
                    streamId);
            OSAL_memCpy(&localRtcpAddr, &stream_ptr->streamParam.localAddr,
                        sizeof(localRtcpAddr));
            localRtcpAddr.port = stream_ptr->streamParam.localControlPort;
            OSAL_memCpy(&remoteRtcpAddr, &stream_ptr->streamParam.remoteAddr,
                        sizeof(remoteRtcpAddr));
            remoteRtcpAddr.port = stream_ptr->streamParam.remoteControlPort;
            _VE_rtcpOpen(q_ptr, rtcp_ptr, remoteRtcpAddr, localRtcpAddr);

            break;
        case _VTSP_CMD_STREAM_VIDEO_END:
            streamId = cmd_ptr->msg.streamVideo.streamId;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->streamParam.dir) {
                break;
            }

            /* Set stream direction to End. */
            stream_ptr->streamParam.dir = _VTSP_STREAM_DIR_ENDED;

            /* Turn off all stream algs */
            _VE_algStateStream(dsp_ptr, infc,
                    streamId,
                    _VE_ALG_STREAM_JB,
                    0);
            /*
             * Set coder type to unavailable to force initialization when the
             * stream is started.
             */
            stream_ptr->dec.decoderType = VTSP_CODER_UNAVAIL;
            stream_ptr->enc.lastEncoder = VTSP_CODER_UNAVAIL;

            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc,
                    streamId);
            _VE_rtpClose(rtp_ptr);
            _VE_rtcpClose(q_ptr, ve_ptr->net_ptr, infc,
                    streamId);

            
            /* Update active stream/interface counts */
            _VE_activeInfcStreamFilterEnd(dsp_ptr, streamId, infc, q_ptr);
            
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            if (!stream_ptr->curActive) {
                break;
            }

            /*
             * If the stream direction changes from non-receive to receive, the
             * jitter buffer needs to reinitialize. Based on old state of dir.
             */
            if ((VTSP_STREAM_DIR_INACTIVE == stream_ptr->streamParam.dir) ||
                    (VTSP_STREAM_DIR_SENDONLY == stream_ptr->streamParam.dir)) {
                _VE_algStateStream(dsp_ptr, infc,
                        cmd_ptr->msg.streamVideo.streamId, 0, _VE_ALG_STREAM_JB);
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
            _VE_updateStream(dsp_ptr, infc,
                    stream_ptr, &cmd_ptr->msg.streamVideo);

            /*
             * Tell RTP that direction has changed.
             */
            rtp_ptr = _VE_streamIdToRtpPtr(ve_ptr->net_ptr, infc,
                    streamId);
            _VE_rtpDir(rtp_ptr, cmd_ptr->msg.arg.arg1);
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            if (!stream_ptr->curActive) {
                break;
            }
            coder = cmd_ptr->msg.arg.arg1;
            /* No DTMF Relay */
            stream_ptr->streamParam.encoder = coder;
            break;
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK:
            streamId = cmd_ptr->msg.arg.arg0;
            stream_ptr = _VE_streamIdToStreamPtr(dsp_ptr, infc, streamId);
            if (!stream_ptr->curActive) {
                break;
            }
            stream_ptr->streamParam.confMask = cmd_ptr->msg.arg.arg1;
           break;
        case _VTSP_CMD_RTCP_CNAME:
            /*
             * Update the CNAME structure with the new string.
             */
            _VE_rtcpSetCname(ve_ptr->net_ptr, infc, (const char *)
                    (cmd_ptr->msg.cname.cname));
            break;
    }
    OSAL_logMsg("%s:%d END", __FUNCTION__, __LINE__);
    /*
     * Tell encoder to continue
     */
    if (OSAL_FAIL == OSAL_semGive(ve_ptr->encStopSem)) {
        _VE_TRACE(__FILE__, __LINE__);
        return;
    }

}

