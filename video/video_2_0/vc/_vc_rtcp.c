/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL AND
 * PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2008 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 10852 $ $Date: 2009-11-20 18:48:51 -0500 (Fri, 20 Nov 2009) $
 *
 */

#include "_vc_private.h"
#include "_vc_rtcp.h"
#include <vtsp_constant.h>
 
/*
 * ======== _VC_rtcpSetCname() ========
 *
 * This routine sets CNAME on the specificed interface to the given name.
 */
void _VC_rtcpSetCname(
    _VC_NetObj   *net_ptr,
    const char   *name_ptr)
{
    _VC_RtcpCname     *cname_ptr;
    char              *dst_ptr;
    char              *src_ptr;
    char              *len_ptr;
    uvint              loop;

    cname_ptr = &net_ptr->rtcpCname;

    dst_ptr = (char *)&(cname_ptr->cname[0]);
    *(dst_ptr++) = _VTSP_RTCP_SDES_TYPE_CNAME;
    len_ptr = dst_ptr++;
    src_ptr = (char *)name_ptr;

    for (loop = 0; loop < _VTSP_RTCP_CNAME_SZ * sizeof(uint32); loop++) {
        if (0 == *src_ptr) {
            break;
        }
        *(dst_ptr++) = *(src_ptr++);
    }
    *len_ptr = loop;
    
    switch (loop & 0x3) {
        case 0:
            *(dst_ptr++) = 0;
            *(dst_ptr++) = 0;
            loop += 4;
            break;
        case 1:
            *(dst_ptr++) = 0;
            loop += 3;
            break;
        case 2:
            *(dst_ptr++) = 0;
            *(dst_ptr++) = 0;
            *(dst_ptr++) = 0;
            *(dst_ptr++) = 0;
            loop += 6;
            break;
        case 3:
            *(dst_ptr++) = 0;
            *(dst_ptr++) = 0;
            *(dst_ptr++) = 0;
            loop += 5;
            break;
    }
    cname_ptr->length = (loop >> 2);
}

/*
 * ======== _VC_updateRtcpMinInterval ========
 *
 * Update RTCP Reduced min Internal parameter based on remote recv bw.
 *
 */
void _VC_rtcpHandleUpdateRtcpMinInterval(
    _VC_Queues         *q_ptr,
    _VC_RtcpObject     *rtcp_ptr,
    VTSP_StreamVideo   *video_ptr)
{
    char        buffer[20];
    vint        rtcpMinIntervalMillis;

    DBG("localVideoAsBwKbps:%d, remoteVideoAsBwKbps:%d",
            video_ptr->localVideoAsBwKbps,
            video_ptr->remoteVideoAsBwKbps);
    if (video_ptr->remoteVideoAsBwKbps > 0) {
        /* Calculate Reduced min RTCP interval based on Remote Session Bandwidth. */
        rtcpMinIntervalMillis = _VC_RTCP_REDUCED_MIN_INTERVAL_MILLIS(
                video_ptr->remoteVideoAsBwKbps);

        /* Check if there is a change in rtcp min interval. */
        if (rtcp_ptr->configure.reducedMinIntervalMillis != rtcpMinIntervalMillis) {

            /* Update RTCP interval as it changed. */
            rtcp_ptr->configure.reducedMinIntervalMillis = rtcpMinIntervalMillis;
            DBG("reducedMinIntervalMillis:%d", rtcpMinIntervalMillis);
            /* Send Event to JVCE to notify Remote AS. */
            OSAL_itoa(video_ptr->remoteVideoAsBwKbps, buffer, sizeof(buffer));
            _VC_sendAppEvent(q_ptr, VC_EVENT_REMOTE_RECV_BW_KBPS, buffer, -1);
        }
    }
    if (video_ptr->localVideoAsBwKbps > 0) {
        /* Store the Local AS bandwidth parameter in the _VC_RtcpObject. */
        rtcp_ptr->localVideoAsBwKbps = video_ptr->localVideoAsBwKbps;
        /* To start with TMMBR = Local AS Bandwidth parameter. */
        rtcp_ptr->feedback.sendTmmbrInKbps = 0; //video_ptr->localVideoAsBwKbps;
        OSAL_logMsg("%s: sendTmmbrInKbps init %u, localVideoAsBwKbps %u\n",
                __FUNCTION__, rtcp_ptr->feedback.sendTmmbrInKbps, rtcp_ptr->localVideoAsBwKbps);
    }
}

/*
 * ======== _VC_rtcpSetControl() ========
 *
 * Configure the RTCP control param sent vfrom  
 * VTSP_config() 
 */
void _VC_rtcpSetControl(
    _VC_Queues         *q_ptr,
    uint16              streamId,
    _VTSP_CmdMsgConfig *config_ptr)
{
    _VC_RtcpCmdMsg cmdMsg;

    cmdMsg.cmd      = _VC_RTCP_CMD_CONFIGURE;
    cmdMsg.streamId = streamId;
    cmdMsg.config   = *config_ptr;

    _VC_sendRtcpCommand(q_ptr, &cmdMsg);
}

/*
 * ======== _VC_rtcpHandleSetControl() ========
 *
 * Handle the 'SetControl' command on the RTCP thread.
 */
void _VC_rtcpHandleSetControl(
    _VC_RtcpObject     *rtcp_ptr,
    _VTSP_CmdMsgConfig  config)
{
    switch (config.u.data[0]) {
        case VTSP_TEMPL_CONTROL_RTCP_INTERVAL:
            /* TODO: sendCountFixed is not used. AS BW parameter is used to decide RTCP min interval. */
            rtcp_ptr->sendCountFixed = config.u.data[2];
            break;
        case VTSP_TEMPL_CONTROL_RTCP_MASK:
            rtcp_ptr->configure.enableMask |= config.u.data[2];
            break;
        case VTSP_TEMPL_CONTROL_RTCP_TOS:
            rtcp_ptr->tos = config.u.data[2];
            break;
    }
}

vint _VC_rtcpReadCommand(
    _VC_Queues      *q_ptr,
    _VC_Dsp         *dsp_ptr,
    _VC_NetObj      *net_ptr,
    _VC_StreamObj   *stream_ptr)
{
    _VC_RtcpObject     *rtcp_ptr;
    _VC_RtcpCmdMsg      message;
    vint                retVal;

   while ((retVal = OSAL_msgQRecv(q_ptr->rtcpCmdQ, (char *)&message,
           sizeof(_VC_RtcpCmdMsg), OSAL_NO_WAIT, NULL)) > 0) {
       rtcp_ptr = _VC_streamIdToRtcpPtr(net_ptr, message.streamId);
       switch (message.cmd) {
           case _VC_RTCP_CMD_OPEN:
               _VC_rtcpHandleOpen(q_ptr, dsp_ptr, rtcp_ptr, &message);
               break;
           case _VC_RTCP_CMD_CLOSE:
               _VC_rtcpHandleClose(q_ptr, net_ptr, message.streamId);
               break;
           case _VC_RTCP_CMD_SEND_RTCP_FB:
               rtcp_ptr->configure.rtcpFeedbackSendMask |= message.feedbackMask;
               OSAL_logMsg("%s: _VC_RTCP_CMD_SEND_RTCP_FB\n", __FUNCTION__);
               break;
           case _VC_RTCP_CMD_CONFIGURE:
               _VC_rtcpHandleSetControl(rtcp_ptr, message.config);
               break;
           case _VC_RTCP_CMD_UPDATE_MIN_INTERVAL:
               _VC_rtcpHandleUpdateRtcpMinInterval(q_ptr, rtcp_ptr, &stream_ptr->streamParam);
               break;
           default:
               break;
       }
   }
   return _VC_OK;
}
