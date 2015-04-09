/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2010 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Revision: 1381 $ $Date: 2006-12-05 14:44:53 -0800 (Tue, 05 Dec 2006) $
 */

#include <osal.h>
#include <vpr_comm.h>
#include <vpad_vpmd.h>
#include "_vpad_mux_helper.h"

/*
 * ======== _VPAD_swapEndian16() ========
 *
 * This function is to swap byte order of 16 bit data.
 *
 * Return Values:
 *  Data value after swapped.
 */
uint16 _VPAD_swapEndian16(
    uint16 num)
{
    return ((num >> 8) | (num << 8));
}

/*
 * ======== _VPAD_swapEndian32() ========
 *
 * This function is to swap byte order of 32-bit data.
 *
 * Return Values:
 *  Data value after swapped.
 */
uint32 _VPAD_swapEndian32(
    uint32 num)
{
    return (((num >> 24) & 0xff) |    // move byte 3 to byte 0
            ((num << 8) & 0xff0000) |    // move byte 1 to byte 2
            ((num >> 8) & 0xff00) |    // move byte 2 to byte 1
            ((num << 24) & 0xff000000)); // byte 0 to byte 3
}

/*
 * ======== _VPAD_swapEndianAddr() ========
 *
 * This function is to swap byte order of OSAL_NetAddress struct.
 *
 * Return Values:
 *  Data value after swapped.
 */
void _VPAD_swapEndianAddr(
    OSAL_NetAddress *dst_ptr,
    OSAL_NetAddress *src_ptr)
{
    vint i;

    dst_ptr->type = _VPAD_swapEndian32(src_ptr->type);
    dst_ptr->port = _VPAD_swapEndian16(src_ptr->port);
    dst_ptr->ipv4 = _VPAD_swapEndian32(src_ptr->ipv4);
    for (i = 0; i < OSAL_NET_IPV6_WORD_SZ; i++) {
        dst_ptr->ipv6[i] = _VPAD_swapEndian16(src_ptr->ipv6[i]);
    }
}

/*
 * ======== _VPAD_swapEndianIpsecSa() ========
 *
 * This function is to swap byte order of OSAL_IpsecSa struct.
 *
 * Return Values:
 *  Data value after swapped.
 */
void _VPAD_swapEndianIpsecSa(
    OSAL_IpsecSa   *dst_ptr,
    OSAL_IpsecSa   *src_ptr)
{
    _VPAD_swapEndianAddr(&dst_ptr->srcAddr, &src_ptr->srcAddr);
    _VPAD_swapEndianAddr(&dst_ptr->dstAddr, &src_ptr->dstAddr);
    dst_ptr->protocol = _VPAD_swapEndian32(src_ptr->protocol);
    dst_ptr->mode     = _VPAD_swapEndian32(src_ptr->mode);
    dst_ptr->spi      = _VPAD_swapEndian32(src_ptr->spi);
    dst_ptr->reqId    = _VPAD_swapEndian32(src_ptr->reqId);
    dst_ptr->algAh    = _VPAD_swapEndian32(src_ptr->algAh);
    dst_ptr->algEsp   = _VPAD_swapEndian32(src_ptr->algEsp);
    /* keyAh and keyEsp do not need to swap, because they are defined as char */
}

/*
 *  ======== _VPAD_muxSwapVtspCmdEndian() ========
 *  This function is used to swap endian type of vtsp cmd.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapVtspCmdEndian(
    _VTSP_CmdMsg   *vtspCmdMsg_ptr,
    VPAD_DataDir    dir)
{
    vint                x;
    _VTSP_CmdMsgArg    *arg_ptr;
    VTSP_StreamVideo   *streamVideo_ptr;

    arg_ptr         = &vtspCmdMsg_ptr->msg.arg;
    streamVideo_ptr = &vtspCmdMsg_ptr->msg.streamVideo;

    if (VPAD_DATA_FROM_IPC == dir) {
        vtspCmdMsg_ptr->code = _VPAD_swapEndian32(
                vtspCmdMsg_ptr->code);
        vtspCmdMsg_ptr->infc = _VPAD_swapEndian16(
                vtspCmdMsg_ptr->infc);
    }

    switch (vtspCmdMsg_ptr->code) {
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_RESOLUTION:
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_DIR:
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_ENCODER:
        case _VTSP_CMD_STREAM_VIDEO_MODIFY_CONFMASK:
        case _VTSP_CMD_STREAM_VIDEO_QUERY:
        case _VTSP_CMD_STREAM_VIDEO_END:
            arg_ptr->arg0 = _VPAD_swapEndian32(arg_ptr->arg0);
            arg_ptr->arg1 = _VPAD_swapEndian32(arg_ptr->arg1);
            arg_ptr->arg2 = _VPAD_swapEndian32(arg_ptr->arg2);
            arg_ptr->arg3 = _VPAD_swapEndian32(arg_ptr->arg3);
            arg_ptr->arg4 = _VPAD_swapEndian32(arg_ptr->arg4);
            arg_ptr->arg5 = _VPAD_swapEndian32(arg_ptr->arg5);
            /* arg6 is uint8 array, so it does not need to convert */
            break;
        case _VTSP_CMD_STREAM_VIDEO_START:
        case _VTSP_CMD_STREAM_VIDEO_MODIFY:
            streamVideo_ptr->streamId = _VPAD_swapEndian32(
                    streamVideo_ptr->streamId);
            streamVideo_ptr->dir      = _VPAD_swapEndian32(
                    streamVideo_ptr->dir);
            streamVideo_ptr->peer     = _VPAD_swapEndian16(
                    streamVideo_ptr->peer);
            streamVideo_ptr->encoder  = _VPAD_swapEndian32(
                    streamVideo_ptr->encoder);
            for (x = 0; x < VTSP_ENCODER_VIDEO_NUM; x++) {
                streamVideo_ptr->encodeTime[x]           = _VPAD_swapEndian32(
                        streamVideo_ptr->encodeTime[x]);
                streamVideo_ptr->encodeMaxBps[x]         = _VPAD_swapEndian32(
                        streamVideo_ptr->encodeMaxBps[x]);
                streamVideo_ptr->encodePacketMode[x]     = _VPAD_swapEndian32(
                        streamVideo_ptr->encodePacketMode[x]);
                streamVideo_ptr->encodeProfileLevelId[x] = _VPAD_swapEndian32(
                        streamVideo_ptr->encodeProfileLevelId[x]);
                streamVideo_ptr->encodeType[x]           = _VPAD_swapEndian32(
                        streamVideo_ptr->encodeType[x]);
            }
            for (x = 0; x < VTSP_DECODER_VIDEO_NUM; x++) {
                streamVideo_ptr->decodeType[x] = _VPAD_swapEndian32(
                        streamVideo_ptr->decodeType[x]);
            }
            streamVideo_ptr->extension = _VPAD_swapEndian32(
                    streamVideo_ptr->extension);
            streamVideo_ptr->confMask  = _VPAD_swapEndian32(
                    streamVideo_ptr->confMask);
            /* 
             * ipv4, ipv6 and port on OSAL_NetAddress used Network order. 
             * Should use _VPAD_swapEndian32(ipv4) and htonl(ipv4).
             * Because of two conversion, we skip the process.
             */
            streamVideo_ptr->remoteAddr.type = 
                    _VPAD_swapEndian32(streamVideo_ptr->remoteAddr.type);
            streamVideo_ptr->remoteControlPort = _VPAD_swapEndian16(
                    streamVideo_ptr->remoteControlPort);
            streamVideo_ptr->localAddr.type = 
                    _VPAD_swapEndian32(streamVideo_ptr->localAddr.type);
            streamVideo_ptr->localControlPort  = _VPAD_swapEndian16(
                    streamVideo_ptr->localControlPort);
            streamVideo_ptr->srtpSecurityType  = _VPAD_swapEndian16(
                    streamVideo_ptr->srtpSecurityType);
            /*
             * rdnDynType, srtpSendKey, and srtpRecvKey are char or uint8,
             * so they do not need to convert
             */
            break;
        case _VTSP_CMD_STREAM_VIDEO_REQUEST_KEY:
        case _VTSP_CMD_STREAM_VIDEO_SYNC:
        default:
            break;
    }
    if (VPAD_DATA_TO_IPC == dir) {
        vtspCmdMsg_ptr->code = _VPAD_swapEndian32(
                vtspCmdMsg_ptr->code);
        vtspCmdMsg_ptr->infc = _VPAD_swapEndian16(
                vtspCmdMsg_ptr->infc);
    }
}

/*
 *  ======== _VPAD_muxSwapVtspEvtEndian() ========
 *  This function is used to swap endian type of vtsp event.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapVtspEvtEndian(
    VTSP_EventMsg  *vtspEvt_ptr,
    VPAD_DataDir    dir)
{
    uint32  x;
    uint32 *tmp_ptr;

    vtspEvt_ptr->code    = _VPAD_swapEndian32(vtspEvt_ptr->code);
    vtspEvt_ptr->infc    = _VPAD_swapEndian32(vtspEvt_ptr->infc);
    vtspEvt_ptr->handset = _VPAD_swapEndian32(vtspEvt_ptr->handset);
    vtspEvt_ptr->tick    = _VPAD_swapEndian32(vtspEvt_ptr->tick);

    tmp_ptr = (uint32 *)&vtspEvt_ptr->msg;
    for (x = 0; x < sizeof(vtspEvt_ptr->msg)/sizeof(uint32); x++) {
        *tmp_ptr = _VPAD_swapEndian32(*tmp_ptr);
        tmp_ptr++;
    }
}

/*
 *  ======== _VPAD_muxSwapRtcpCmdEndian() ========
 *  This function is used to swap endian type of rtcp command.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapRtcpCmdEndian(
    _VTSP_RtcpCmdMsg   *rtcpCmd_ptr,
    VPAD_DataDir        dir)
{
    vint    x;

    if (VPAD_DATA_FROM_IPC == dir) {
        rtcpCmd_ptr->command     = _VPAD_swapEndian32(rtcpCmd_ptr->command);
        rtcpCmd_ptr->infc        = _VPAD_swapEndian32(rtcpCmd_ptr->infc);
        rtcpCmd_ptr->streamId    = _VPAD_swapEndian32(rtcpCmd_ptr->streamId);
        rtcpCmd_ptr->payloadSize = _VPAD_swapEndian32(rtcpCmd_ptr->payloadSize);
    }

    if (_VTSP_RTCP_CMD_OPEN == rtcpCmd_ptr->command) {
        rtcpCmd_ptr->msg.open.infc = _VPAD_swapEndian32(
                rtcpCmd_ptr->msg.open.infc);
        rtcpCmd_ptr->msg.open.tos  = _VPAD_swapEndian32(
                rtcpCmd_ptr->msg.open.tos);
    }
    else {
        for (x = 0; x < _VTSP_RTCP_PAYLOAD_SZ; x++) {
            rtcpCmd_ptr->msg.payload[x] = _VPAD_swapEndian32(
                    rtcpCmd_ptr->msg.payload[x]);
        }
    }

    if (VPAD_DATA_TO_IPC == dir) {
        rtcpCmd_ptr->command     = _VPAD_swapEndian32(rtcpCmd_ptr->command);
        rtcpCmd_ptr->infc        = _VPAD_swapEndian32(rtcpCmd_ptr->infc);
        rtcpCmd_ptr->streamId    = _VPAD_swapEndian32(rtcpCmd_ptr->streamId);
        rtcpCmd_ptr->payloadSize = _VPAD_swapEndian32(rtcpCmd_ptr->payloadSize);
    }
}

/*
 *  ======== _VPAD_muxSwapIsipEndian() ========
 *  This function is used to swap endian type of isip message.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapIsipEndian(
    ISIP_Message   *isipMsg_ptr,
    VPAD_DataDir    dir)
{
    vint            x;
    ISIP_Service   *service_ptr;
    ISIP_Media     *media_ptr;

    service_ptr = &isipMsg_ptr->msg.service;
    media_ptr   = &isipMsg_ptr->msg.media;

    if (VPAD_DATA_FROM_IPC == dir) {
        isipMsg_ptr->id       = _VPAD_swapEndian32(isipMsg_ptr->id);
        isipMsg_ptr->code     = _VPAD_swapEndian32(isipMsg_ptr->code);
        isipMsg_ptr->protocol = _VPAD_swapEndian32(isipMsg_ptr->protocol);
    }

    switch (isipMsg_ptr->code) {
        case ISIP_CODE_SERVICE:
            if (VPAD_DATA_FROM_IPC == dir) {
                service_ptr->reason = _VPAD_swapEndian32(service_ptr->reason);
                service_ptr->status = _VPAD_swapEndian32(service_ptr->status);
                service_ptr->server = _VPAD_swapEndian32(service_ptr->server);
            }
            switch (service_ptr->reason) {
                case ISIP_SERVICE_REASON_BSID:
                    service_ptr->settings.bsId.type = _VPAD_swapEndian32(
                            service_ptr->settings.bsId.type);
                    break;
                case ISIP_SERVICE_REASON_EMERGENCY:
                    service_ptr->settings.isEmergency = _VPAD_swapEndian32(
                            service_ptr->settings.isEmergency);
                    break;
                case ISIP_SERVICE_REASON_IDENTITY:
                    service_ptr->settings.identityHide = _VPAD_swapEndian32(
                            service_ptr->settings.identityHide);
                    break;
                case ISIP_SERVICE_REASON_CODERS:
                    for (x = 0; x < ISI_CODER_NUM; x++) {
                        service_ptr->settings.coders[x].relates =
                            _VPAD_swapEndian32(
                            service_ptr->settings.coders[x].relates);
                    }
                    break;
                case ISIP_SERVICE_REASON_ACTIVATE:
                case ISIP_SERVICE_REASON_DEACTIVATE:
                    _VPAD_swapEndianAddr(
                            &service_ptr->settings.interface.address,
                            &service_ptr->settings.interface.address);
                    break;
                case ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE:
                    service_ptr->settings.akaAuthResp.result =
                            _VPAD_swapEndian32(
                            service_ptr->settings.akaAuthResp.result);
                    service_ptr->settings.akaAuthResp.resLength =
                            _VPAD_swapEndian32(
                            service_ptr->settings.akaAuthResp.resLength);
                    /* resp, auts, ck, and ik are char */
                    break;
                case ISIP_SERVICE_REASON_PORT:
                    service_ptr->settings.port.portType = _VPAD_swapEndian32(
                            service_ptr->settings.port.portType);
                    service_ptr->settings.port.portNum  = _VPAD_swapEndian32(
                            service_ptr->settings.port.portNum);
                    service_ptr->settings.port.poolSize = _VPAD_swapEndian32(
                            service_ptr->settings.port.poolSize);
                    break;
                case ISIP_SERVICE_REASON_IPSEC:
                    service_ptr->settings.ipsec.cfg.protectedPort =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.cfg.protectedPort);
                    service_ptr->settings.ipsec.cfg.protectedPortPoolSz =
                           _VPAD_swapEndian32(
                           service_ptr->settings.ipsec.cfg.protectedPortPoolSz);
                    service_ptr->settings.ipsec.cfg.spi =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.cfg.spi);
                    service_ptr->settings.ipsec.cfg.spiPoolSz =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.cfg.spiPoolSz);

                    service_ptr->settings.ipsec.resp.portUc =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.portUc);
                    service_ptr->settings.ipsec.resp.portUs =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.portUs);
                    service_ptr->settings.ipsec.resp.portPc =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.portPc);
                    service_ptr->settings.ipsec.resp.portPs =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.portPs);
                    service_ptr->settings.ipsec.resp.spiUc =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.spiUc);
                    service_ptr->settings.ipsec.resp.spiUs =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.spiUs);
                    service_ptr->settings.ipsec.resp.spiPc =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.spiPc);
                    service_ptr->settings.ipsec.resp.spiPs =
                            _VPAD_swapEndian32(
                            service_ptr->settings.ipsec.resp.spiPs);
                    break;
                case ISIP_SERVICE_REASON_CREATE:
                case ISIP_SERVICE_REASON_URI:
                case ISIP_SERVICE_REASON_INSTANCEID:
                case ISIP_SERVICE_REASON_CAPABILITIES:
                case ISIP_SERVICE_REASON_IMEI_URI:
                case ISIP_SERVICE_REASON_FILE:
                case ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE:
                case ISIP_SERVICE_REASON_SERVER:
                case ISIP_SERVICE_REASON_AUTH:
                case ISIP_SERVICE_REASON_SET_PROVISIONING_DATA:
                default:
                    break;
            }
            if (VPAD_DATA_TO_IPC == dir) {
                isipMsg_ptr->msg.service.reason = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.service.reason);
                isipMsg_ptr->msg.service.status = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.service.status);
                isipMsg_ptr->msg.service.server = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.service.server);
            }
            /* reasonDesc is char */
            break;
        case ISIP_CODE_CALL:
            isipMsg_ptr->msg.call.reason = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.reason);
            isipMsg_ptr->msg.call.status = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.status);
            isipMsg_ptr->msg.call.transferTargetCallId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.transferTargetCallId);
            isipMsg_ptr->msg.call.serviceId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.serviceId);
            isipMsg_ptr->msg.call.cidType = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.cidType);
            isipMsg_ptr->msg.call.type = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.type);
            isipMsg_ptr->msg.call.audioDirection = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.audioDirection);
            isipMsg_ptr->msg.call.videoDirection = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.videoDirection);
            for (x = 0; x < ISI_CODER_NUM; x++) {
                isipMsg_ptr->msg.call.coders[x].relates = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.call.coders[x].relates);
            }
            _VPAD_swapEndianAddr(&isipMsg_ptr->msg.call.lclAudioAddr,
                    &isipMsg_ptr->msg.call.lclAudioAddr);
            _VPAD_swapEndianAddr(&isipMsg_ptr->msg.call.lclVideoAddr,
                    &isipMsg_ptr->msg.call.lclVideoAddr);
            isipMsg_ptr->msg.call.lclAudioCntlPort = _VPAD_swapEndian16(
                    isipMsg_ptr->msg.call.lclAudioCntlPort);
            isipMsg_ptr->msg.call.lclVideoCntlPort = _VPAD_swapEndian16(
                    isipMsg_ptr->msg.call.lclVideoCntlPort);
            _VPAD_swapEndianAddr(&isipMsg_ptr->msg.call.rmtAudioAddr,
                    &isipMsg_ptr->msg.call.rmtAudioAddr);
            _VPAD_swapEndianAddr(&isipMsg_ptr->msg.call.rmtVideoAddr,
                    &isipMsg_ptr->msg.call.rmtVideoAddr);
            isipMsg_ptr->msg.call.rmtAudioCntlPort = _VPAD_swapEndian16(
                    isipMsg_ptr->msg.call.rmtAudioCntlPort);
            isipMsg_ptr->msg.call.rmtVideoCntlPort = _VPAD_swapEndian16(
                    isipMsg_ptr->msg.call.rmtVideoCntlPort);
            isipMsg_ptr->msg.call.srvccStatus = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.srvccStatus);
            isipMsg_ptr->msg.call.ringTemplate = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.ringTemplate);
            isipMsg_ptr->msg.call.audioKeys.type = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.audioKeys.type);
            isipMsg_ptr->msg.call.videoKeys.type = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.videoKeys.type);
            isipMsg_ptr->msg.call.supsrvHfExist = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.call.supsrvHfExist);
            break;
        case ISIP_CODE_MESSAGE:
            isipMsg_ptr->msg.message.reason    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.message.reason);
            isipMsg_ptr->msg.message.report    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.message.report);
            isipMsg_ptr->msg.message.serviceId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.message.serviceId);
            isipMsg_ptr->msg.message.chatId    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.message.chatId);
            isipMsg_ptr->msg.message.type      = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.message.type);
            isipMsg_ptr->msg.message.pduLen    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.message.pduLen);
            break;
        case ISIP_CODE_PRESENCE:
            isipMsg_ptr->msg.presence.reason    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.presence.reason);
            isipMsg_ptr->msg.presence.serviceId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.presence.serviceId);
            isipMsg_ptr->msg.presence.chatId    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.presence.chatId);
            break;
        case ISIP_CODE_TEL_EVENT:
            if (VPAD_DATA_FROM_IPC == dir) {
                isipMsg_ptr->msg.event.reason    = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.reason);
                isipMsg_ptr->msg.event.serviceId = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.serviceId);
                isipMsg_ptr->msg.event.callId    = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.callId);
                isipMsg_ptr->msg.event.evt       = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.evt);
            }
            if (ISIP_TEL_EVENT_REASON_NEW == isipMsg_ptr->msg.event.reason) {
                if (ISI_TEL_EVENT_GET_SERVICE_ATTIBUTE ==
                        isipMsg_ptr->msg.event.evt) {
                    isipMsg_ptr->msg.event.settings.service.cmd =
                            _VPAD_swapEndian32(
                            isipMsg_ptr->msg.event.settings.service.cmd);
                }
                else if (ISI_TEL_EVENT_CALL_FORWARD ==
                        isipMsg_ptr->msg.event.evt) {
                    isipMsg_ptr->msg.event.settings.forward.condition =
                            _VPAD_swapEndian32(
                            isipMsg_ptr->msg.event.settings.forward.condition);
                    isipMsg_ptr->msg.event.settings.forward.enable    =
                            _VPAD_swapEndian32(
                            isipMsg_ptr->msg.event.settings.forward.enable);
                    isipMsg_ptr->msg.event.settings.forward.timeout   =
                            _VPAD_swapEndian32(
                            isipMsg_ptr->msg.event.settings.forward.timeout);
                }
                else {
                    isipMsg_ptr->msg.event.settings.args.arg0 =
                            _VPAD_swapEndian32(
                            isipMsg_ptr->msg.event.settings.args.arg0);
                    isipMsg_ptr->msg.event.settings.args.arg1 =
                            _VPAD_swapEndian32(
                            isipMsg_ptr->msg.event.settings.args.arg1);
                }
            }
            else {
                isipMsg_ptr->msg.event.settings.args.arg0 =
                        _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.settings.args.arg0);
                isipMsg_ptr->msg.event.settings.args.arg1 =
                        _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.settings.args.arg1);
            }

            if (VPAD_DATA_TO_IPC == dir) {
                isipMsg_ptr->msg.event.reason    = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.reason);
                isipMsg_ptr->msg.event.serviceId = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.serviceId);
                isipMsg_ptr->msg.event.callId    = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.callId);
                isipMsg_ptr->msg.event.evt       = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.event.evt);
            }
            break;
        case ISIP_CODE_MEDIA:
            if (VPAD_DATA_FROM_IPC == dir) {
                media_ptr->reason    = _VPAD_swapEndian32(media_ptr->reason);
                media_ptr->serviceId = _VPAD_swapEndian32(media_ptr->serviceId);
            }
            switch (media_ptr->reason) {
                case ISIP_MEDIA_REASON_RINGSTART:
                case ISIP_MEDIA_REASON_RINGSTOP:
                    media_ptr->media.ring.ringtemplate = _VPAD_swapEndian32(
                            media_ptr->media.ring.ringtemplate);
                    media_ptr->media.ring.cadences = _VPAD_swapEndian32(
                            media_ptr->media.ring.cadences);
                    media_ptr->media.ring.make1 = _VPAD_swapEndian32(
                            media_ptr->media.ring.make1);
                    media_ptr->media.ring.break1 = _VPAD_swapEndian32(
                            media_ptr->media.ring.break1);
                    media_ptr->media.ring.make2 = _VPAD_swapEndian32(
                            media_ptr->media.ring.make2);
                    media_ptr->media.ring.break2 = _VPAD_swapEndian32(
                            media_ptr->media.ring.break2);
                    media_ptr->media.ring.make3 = _VPAD_swapEndian32(
                            media_ptr->media.ring.make3);
                    media_ptr->media.ring.break3 = _VPAD_swapEndian32(
                            media_ptr->media.ring.break3);
                    break;
                case ISIP_MEDIA_REASON_TONESTART:
                case ISIP_MEDIA_REASON_TONESTOP:
                case ISIP_MEDIA_REASON_TONESTART_CMD:
                case ISIP_MEDIA_REASON_TONESTOP_CMD:
                    media_ptr->media.tone.toneType = _VPAD_swapEndian32(
                            media_ptr->media.tone.toneType);
                    media_ptr->media.tone.freq1 = _VPAD_swapEndian32(
                            media_ptr->media.tone.freq1);
                    media_ptr->media.tone.freq2 = _VPAD_swapEndian32(
                            media_ptr->media.tone.freq2);
                    media_ptr->media.tone.cadences = _VPAD_swapEndian32(
                            media_ptr->media.tone.cadences);
                    media_ptr->media.tone.make1 = _VPAD_swapEndian32(
                            media_ptr->media.tone.make1);
                    media_ptr->media.tone.break1 = _VPAD_swapEndian32(
                            media_ptr->media.tone.break1);
                    media_ptr->media.tone.repeat1 = _VPAD_swapEndian32(
                            media_ptr->media.tone.repeat1);
                    media_ptr->media.tone.make2 = _VPAD_swapEndian32(
                            media_ptr->media.tone.make2);
                    media_ptr->media.tone.break2 = _VPAD_swapEndian32(
                            media_ptr->media.tone.break2);
                    media_ptr->media.tone.repeat2 = _VPAD_swapEndian32(
                            media_ptr->media.tone.repeat2);
                    media_ptr->media.tone.make3 = _VPAD_swapEndian32(
                            media_ptr->media.tone.make3);
                    media_ptr->media.tone.break3 = _VPAD_swapEndian32(
                            media_ptr->media.tone.break3);
                    media_ptr->media.tone.repeat3 = _VPAD_swapEndian32(
                            media_ptr->media.tone.repeat3);
                    break;
                case ISIP_MEDIA_REASON_STREAMSTART:
                case ISIP_MEDIA_REASON_STREAMSTOP:
                case ISIP_MEDIA_REASON_STREAMMODIFY:
                    media_ptr->media.stream.id = _VPAD_swapEndian32(
                            media_ptr->media.stream.id);
                    media_ptr->media.stream.audioDirection = _VPAD_swapEndian32(
                            media_ptr->media.stream.audioDirection);
                    media_ptr->media.stream.videoDirection = _VPAD_swapEndian32(
                            media_ptr->media.stream.videoDirection);
                    media_ptr->media.stream.type = _VPAD_swapEndian32(
                            media_ptr->media.stream.type);
                    media_ptr->media.stream.confMask = _VPAD_swapEndian32(
                            media_ptr->media.stream.confMask);

                    _VPAD_swapEndianAddr(&media_ptr->media.stream.audio.rmtAddr,
                            &media_ptr->media.stream.audio.rmtAddr);
                    media_ptr->media.stream.audio.rmtCntlPort =
                            _VPAD_swapEndian32(
                            media_ptr->media.stream.audio.rmtCntlPort);
                    _VPAD_swapEndianAddr(&media_ptr->media.stream.audio.lclAddr,
                            &media_ptr->media.stream.audio.lclAddr);
                    media_ptr->media.stream.audio.lclCntlPort =
                            _VPAD_swapEndian32(
                            media_ptr->media.stream.audio.lclCntlPort);
                    media_ptr->media.stream.audio.securityKeys.type =
                            _VPAD_swapEndian32(
                            media_ptr->media.stream.audio.securityKeys.type);
                    _VPAD_swapEndianAddr(&media_ptr->media.stream.video.rmtAddr,
                            &media_ptr->media.stream.video.rmtAddr);
                    media_ptr->media.stream.video.rmtCntlPort =
                            _VPAD_swapEndian32(
                            media_ptr->media.stream.video.rmtCntlPort);
                    _VPAD_swapEndianAddr(&media_ptr->media.stream.video.lclAddr,
                            &media_ptr->media.stream.video.lclAddr);
                    media_ptr->media.stream.video.lclCntlPort =
                            _VPAD_swapEndian32(
                            media_ptr->media.stream.video.lclCntlPort);
                    media_ptr->media.stream.video.securityKeys.type =
                            _VPAD_swapEndian32(
                            media_ptr->media.stream.video.securityKeys.type);

                    for (x = 0; x < ISI_CODER_NUM; x++) {
                        media_ptr->media.stream.coders[x].relates =
                                _VPAD_swapEndian32(
                                media_ptr->media.stream.coders[x].relates);
                    }
                    break;
                case ISIP_MEDIA_REASON_CONFSTART:
                case ISIP_MEDIA_REASON_CONFSTOP:
                    for (x = 0; x < ISI_CONF_USERS_NUM; x++) {
                        media_ptr->media.conf.aStreamId[x] = _VPAD_swapEndian32(
                                media_ptr->media.conf.aStreamId[x]);
                        media_ptr->media.conf.aConfMask[x] = _VPAD_swapEndian32(
                                media_ptr->media.conf.aConfMask[x]);
                        media_ptr->media.conf.aCall[x] = _VPAD_swapEndian32(
                                media_ptr->media.conf.aCall[x]);
                    }
                    break;
                case ISIP_MEDIA_REASON_PKT_RECV:
                case ISIP_MEDIA_REASON_PKT_SEND:
                    media_ptr->media.stun.lclAddr = _VPAD_swapEndian32(
                            media_ptr->media.stun.lclAddr);
                    media_ptr->media.stun.lclPort = _VPAD_swapEndian16(
                            media_ptr->media.stun.lclPort);
                    media_ptr->media.stun.rmtAddr = _VPAD_swapEndian32(
                            media_ptr->media.stun.rmtAddr);
                    media_ptr->media.stun.rmtPort = _VPAD_swapEndian16(
                            media_ptr->media.stun.rmtPort);
                    media_ptr->media.stun.pktSize = _VPAD_swapEndian32(
                            media_ptr->media.stun.pktSize);
                    break;
                case ISIP_MEDIA_REASON_SPEAKER:
                default:
                    break;
            }
            if (VPAD_DATA_TO_IPC == dir) {
                media_ptr->reason    = _VPAD_swapEndian32(media_ptr->reason);
                media_ptr->serviceId = _VPAD_swapEndian32(media_ptr->serviceId);
            }
            break;
        case ISIP_CODE_SYSTEM:
            isipMsg_ptr->msg.system.reason = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.system.reason);
            isipMsg_ptr->msg.system.status = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.system.status);
            /* protocolIpc, mediaIpc, streamIpc are char */
            break;
        case ISIP_CODE_CHAT:
            isipMsg_ptr->msg.groupchat.serviceId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.serviceId);
            isipMsg_ptr->msg.groupchat.reason    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.reason);
            isipMsg_ptr->msg.groupchat.status    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.status);
            isipMsg_ptr->msg.groupchat.passwordRequired = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.passwordRequired);
            isipMsg_ptr->msg.groupchat.type      = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.type);
            isipMsg_ptr->msg.groupchat.text.reason = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.text.reason);
            isipMsg_ptr->msg.groupchat.text.report = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.text.report);
            isipMsg_ptr->msg.groupchat.text.serviceId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.text.serviceId);
            isipMsg_ptr->msg.groupchat.text.chatId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.text.chatId);
            isipMsg_ptr->msg.groupchat.text.type = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.text.type);
            isipMsg_ptr->msg.groupchat.text.pduLen = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.groupchat.text.pduLen);
            break;
        case ISIP_CODE_FILE:
            isipMsg_ptr->msg.file.reason        = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.reason);
            isipMsg_ptr->msg.file.serviceId     = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.serviceId);
            isipMsg_ptr->msg.file.chatId        = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.chatId);
            isipMsg_ptr->msg.file.fileType      = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.fileType);
            isipMsg_ptr->msg.file.fileAttribute = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.fileAttribute);
            isipMsg_ptr->msg.file.fileSize      = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.fileSize);
            isipMsg_ptr->msg.file.progress      = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.file.progress);
            break;
        case ISIP_CODE_DIAGNOSTIC:
            isipMsg_ptr->msg.diag.reason = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.diag.reason);
            break;
        case ISIP_CODE_USSD:
            isipMsg_ptr->msg.ussd.reason    = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.ussd.reason);
            isipMsg_ptr->msg.ussd.serviceId = _VPAD_swapEndian32(
                    isipMsg_ptr->msg.ussd.serviceId);
            for (x = 0; x < ISI_CODER_NUM; x++) {
                isipMsg_ptr->msg.ussd.coders[x].relates = _VPAD_swapEndian32(
                        isipMsg_ptr->msg.ussd.coders[x].relates);
            }
            break;
        default:
            break;
    }

    if (VPAD_DATA_TO_IPC == dir) {
        isipMsg_ptr->id       = _VPAD_swapEndian32(isipMsg_ptr->id);
        isipMsg_ptr->code     = _VPAD_swapEndian32(isipMsg_ptr->code);
        isipMsg_ptr->protocol = _VPAD_swapEndian32(isipMsg_ptr->protocol);
    }
}

/*
 *  ======== _VPAD_muxSwapVprNetEndian() ========
 *  This function is used to swap endian type of vprnet.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapVprNetEndian(
    VPR_Net        *vprNet_ptr,
    VPAD_DataDir    dir)
{
    if (VPAD_DATA_FROM_IPC == dir) {
        vprNet_ptr->type        = _VPAD_swapEndian32(vprNet_ptr->type);
        vprNet_ptr->referenceId = _VPAD_swapEndian32(vprNet_ptr->referenceId);
    }

    if (VPR_NET_TYPE_ERROR == vprNet_ptr->type) {
        vprNet_ptr->u.status.evtType = _VPAD_swapEndian32(
                vprNet_ptr->u.status.evtType);
        vprNet_ptr->u.status.errnum  = _VPAD_swapEndian32(
                vprNet_ptr->u.status.errnum);
    }
    else if (VPR_NET_TYPE_REG_SEC_CFG == vprNet_ptr->type) {
        /* vprNet_ptr->u.regsecData.preconfiguredRoute and
         * vprNet_ptr->u.regsecData.secSrvHfs are defined as char,
         * so they do not need to swap endian
         */
        /* swap inboundSAc */
        _VPAD_swapEndianIpsecSa(&vprNet_ptr->u.regsecData.inboundSAc,
                &vprNet_ptr->u.regsecData.inboundSAc);
        _VPAD_swapEndianIpsecSa(&vprNet_ptr->u.regsecData.inboundSAs,
                &vprNet_ptr->u.regsecData.inboundSAs);
        _VPAD_swapEndianIpsecSa(&vprNet_ptr->u.regsecData.outboundSAc,
                &vprNet_ptr->u.regsecData.outboundSAc);
    }
    else {
        vprNet_ptr->u.packet.tosValue    = _VPAD_swapEndian32(
                vprNet_ptr->u.packet.tosValue);
        vprNet_ptr->u.packet.chunkNumber = _VPAD_swapEndian32(
                vprNet_ptr->u.packet.chunkNumber);
        vprNet_ptr->u.packet.packetEnd   = _VPAD_swapEndian32(
                vprNet_ptr->u.packet.packetEnd);
        vprNet_ptr->u.packet.packetLen   = _VPAD_swapEndian32(
                vprNet_ptr->u.packet.packetLen);
    }

    if (VPAD_DATA_TO_IPC == dir) {
        vprNet_ptr->type        = _VPAD_swapEndian32(vprNet_ptr->type);
        vprNet_ptr->referenceId = _VPAD_swapEndian32(vprNet_ptr->referenceId);
    }
    
    vprNet_ptr->localAddress.type = 
            _VPAD_swapEndian32(vprNet_ptr->localAddress.type);
    vprNet_ptr->remoteAddress.type = 
            _VPAD_swapEndian32(vprNet_ptr->remoteAddress.type);
}

/*
 *  ======== _VPAD_muxSwapCsmEvtEndian() ========
 *  This function is used to swap endian type of csm input event.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapCsmEvtEndian(
    CSM_InputEvent *csmEvt_ptr,
    VPAD_DataDir    dir)
{
    if (VPAD_DATA_FROM_IPC == dir) {
        csmEvt_ptr->type = _VPAD_swapEndian32(csmEvt_ptr->type);
    }

    switch (csmEvt_ptr->type) {
        case CSM_EVENT_TYPE_CALL:
            csmEvt_ptr->evt.call.type = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.type);
            csmEvt_ptr->evt.call.reason = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.reason);
            csmEvt_ptr->evt.call.extraArgument = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.extraArgument);
            csmEvt_ptr->evt.call.isEmergency = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.isEmergency);
            csmEvt_ptr->evt.call.emergencyType = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.emergencyType);
            csmEvt_ptr->evt.call.cidType = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.cidType);
            csmEvt_ptr->evt.call.isRsrcReady = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.call.isRsrcReady);
            break;
        case CSM_EVENT_TYPE_SMS:
            csmEvt_ptr->evt.sms.reason = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.sms.reason);
            csmEvt_ptr->evt.sms.type   = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.sms.type);
            csmEvt_ptr->evt.sms.msgLen = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.sms.msgLen);
            break;
        case CSM_EVENT_TYPE_SERVICE:
            if (VPAD_DATA_FROM_IPC == dir) {
                csmEvt_ptr->evt.service.reason = _VPAD_swapEndian32(
                        csmEvt_ptr->evt.service.reason);
            }

            switch (csmEvt_ptr->evt.service.reason) {
                case CSM_SERVICE_REASON_UPDATE_CGI:
                    csmEvt_ptr->evt.service.u.cgi.type = _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.cgi.type);
                    break;
                case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS:
                case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
                case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
                    csmEvt_ptr->evt.service.u.aka.resLength =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.aka.resLength);
                    break;
                case CSM_SERVICE_REASON_SET_IPSEC:
                    csmEvt_ptr->evt.service.u.ipsec.protectedPort =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.ipsec.protectedPort);
                    csmEvt_ptr->evt.service.u.ipsec.protectedPortPoolSz =
                           _VPAD_swapEndian32(
                           csmEvt_ptr->evt.service.u.ipsec.protectedPortPoolSz);
                    csmEvt_ptr->evt.service.u.ipsec.spi =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.ipsec.spi);
                    csmEvt_ptr->evt.service.u.ipsec.spiPoolSz =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.ipsec.spiPoolSz);
                    break;
                case CSM_SERVICE_REASON_SET_PORTS:
                    csmEvt_ptr->evt.service.u.port.sip = _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.port.sip);
                    csmEvt_ptr->evt.service.u.port.audio = _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.port.audio);
                    csmEvt_ptr->evt.service.u.port.audioPoolSize =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.port.audioPoolSize);
                    csmEvt_ptr->evt.service.u.port.video = _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.port.video);
                    csmEvt_ptr->evt.service.u.port.videoPoolSize =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.port.videoPoolSize);
                    break;
                case CSM_SERVICE_REASON_APPS_PROVISION:
                    csmEvt_ptr->evt.service.u.appsProvision.xcapAppInfo.
                            appAuthType = _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.appsProvision.xcapAppInfo.
                            appAuthType);
                    break;
                case CSM_SERVICE_REASON_SET_REREGISTER_PERIOD:
                case CSM_SERVICE_REASON_SET_RETRY_TIMER_PERIOD:
                    csmEvt_ptr->evt.service.u.extraArgument =
                            _VPAD_swapEndian32(
                            csmEvt_ptr->evt.service.u.extraArgument);
                    break;
                default:
                    break;
            }
            if (VPAD_DATA_TO_IPC == dir) {
                csmEvt_ptr->evt.service.reason = _VPAD_swapEndian32(
                        csmEvt_ptr->evt.service.reason);
            }
            break;
        case CSM_EVENT_TYPE_RADIO:
            csmEvt_ptr->evt.radio.reason = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.radio.reason);
            csmEvt_ptr->evt.radio.networkType = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.radio.networkType);
            csmEvt_ptr->evt.radio.isEmergencyFailoverToCs = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.radio.isEmergencyFailoverToCs);
            csmEvt_ptr->evt.radio.isEmergencyRegRequired = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.radio.isEmergencyRegRequired);
            break;
        case CSM_EVENT_TYPE_SUPSRV:
            csmEvt_ptr->evt.supSrv.reason = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.supSrv.reason);
            csmEvt_ptr->evt.supSrv.mode.cbMode = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.supSrv.mode.cbMode);
            csmEvt_ptr->evt.supSrv.status.oirReqStatus = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.supSrv.status.oirReqStatus);
            csmEvt_ptr->evt.supSrv.ruleParams.noReplyTimer = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.supSrv.ruleParams.noReplyTimer);
            csmEvt_ptr->evt.supSrv.ruleParams.addrType = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.supSrv.ruleParams.addrType);
            break;
        case CSM_EVENT_TYPE_USSD:
            csmEvt_ptr->evt.ussd.reason  = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.ussd.reason);
            csmEvt_ptr->evt.ussd.encType = _VPAD_swapEndian32(
                    csmEvt_ptr->evt.ussd.encType);
            break;
        default:
            break;
    }

    if (VPAD_DATA_TO_IPC == dir) {
        csmEvt_ptr->type = _VPAD_swapEndian32(csmEvt_ptr->type);
    }
}

/*
 *  ======== _VPAD_muxSwapVprCommEndian() ========
 *  This function is used to swap VPRCOMM endian type.
 *
 *  Return Values:
 *
 */
void _VPAD_muxSwapVprCommEndian(
    VPR_Comm       *vprComm_ptr,
    VPAD_DataDir    dir)
{
    uint32  x;
    uint32 *tmp_ptr;

    if (VPAD_DATA_FROM_IPC == dir) {
        vprComm_ptr->type         = _VPAD_swapEndian32(vprComm_ptr->type);
        vprComm_ptr->targetModule = _VPAD_swapEndian32(
                vprComm_ptr->targetModule);
        vprComm_ptr->targetSize   = _VPAD_swapEndian32(vprComm_ptr->targetSize);
    }

    /* If target is ISI RPC, does not need to swap endian type */
    if ((vprComm_ptr->targetModule != VPMD_ISI_EVT_RPC_TARGET) &&
            (vprComm_ptr->targetModule != VPMD_ISI_RPC_TARGET)) {
        switch (vprComm_ptr->type) {
            case VPR_TYPE_VTSP_CMD:
                _VPAD_muxSwapVtspCmdEndian(&vprComm_ptr->u.vtspCmd, dir);
                break;
            case VPR_TYPE_VTSP_EVT:
                _VPAD_muxSwapVtspEvtEndian(&vprComm_ptr->u.vtspEvt, dir);
                break;
            case VPR_TYPE_RTCP_CMD:
                _VPAD_muxSwapRtcpCmdEndian(&vprComm_ptr->u.vtspRtcpCmd, dir);
                break;
            case VPR_TYPE_RTCP_EVT:
                tmp_ptr = (uint32 *)&vprComm_ptr->u.vtspRtcpEvt;
                for (x = 0; x < (sizeof(_VTSP_RtcpEventMsg)/sizeof(uint32));
                        x++) {
                    *tmp_ptr = _VPAD_swapEndian32(*tmp_ptr);
                    tmp_ptr++;
                }
                break;
            case VPR_TYPE_ISIP:
                _VPAD_muxSwapIsipEndian(&vprComm_ptr->u.isipMsg, dir);
                break;
            case VPR_TYPE_NET:
                _VPAD_muxSwapVprNetEndian(&vprComm_ptr->u.vprNet, dir);
                break;
            case VPR_TYPE_CSM_EVT:
                _VPAD_muxSwapCsmEvtEndian(&vprComm_ptr->u.csmEvt, dir);
                break;
            case VPR_TYPE_NETWORK_MODE:
                vprComm_ptr->u.networkMode = _VPAD_swapEndian32(
                        vprComm_ptr->u.networkMode);
                break;
            default:
                break;
        }
    }
    if (VPAD_DATA_TO_IPC == dir) {
        vprComm_ptr->type         = _VPAD_swapEndian32(vprComm_ptr->type);
        vprComm_ptr->targetModule = _VPAD_swapEndian32(
                vprComm_ptr->targetModule);
        vprComm_ptr->targetSize   = _VPAD_swapEndian32(vprComm_ptr->targetSize);
    }
}

