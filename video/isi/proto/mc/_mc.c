/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30135 $ $Date: 2014-12-01 16:07:35 +0800 (Mon, 01 Dec 2014) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_net.h>
#include <osal_msg.h>
#include <settings.h>

#include "isi.h"
#include "isip.h"
#include "vtsp.h"
#include "_mc.h"
#include "_mc_coder.h"
#include "_mc_hw.h"
#ifndef MC_NO_TONE
#include "_mc_tone.h"
#endif

/* Global object. */
static MC_VtspObj *_MC_globalVtspObj_ptr;

/*
 * ======== MC_telEvtIsiEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for "telephone" related events (such as DTMF digits). These events
 * will be passed from MC to the ISI module.
 *
 * Returns:
 *   Nothing.
 */
void MC_telEvtIsiEvt(
    ISI_Id              telEvtId,
    ISI_Id              serviceId,
    ISIP_TelEvtReason   reason,
    ISI_TelEvent        evt,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = telEvtId;
    isi_ptr->code = ISIP_CODE_TEL_EVENT;
    isi_ptr->protocol = ISI_PROTOCOL_VTSP;
    isi_ptr->msg.event.reason = reason;
    isi_ptr->msg.event.serviceId = serviceId;
    isi_ptr->msg.event.callId = 0;
    isi_ptr->msg.event.evt = evt;
    return;
}

/* 
 * ======== _MC_sysEvtIsiEvt() ========
 *
 * This populates an ISI event related to 'system' level events.
 *
 * Return Values: 
 * None
 */   
static void _MC_sysEvtIsiEvt(
    ISIP_SystemReason  reason,
    ISIP_Status         status,
    char               *ipcName_ptr,    
    char               *audioName_ptr,
    char               *streamName_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = 0;
    isi_ptr->code = ISIP_CODE_SYSTEM;
    isi_ptr->protocol = ISI_PROTOCOL_VTSP;
    isi_ptr->msg.system.reason = reason;
    isi_ptr->msg.system.status = status;
    /* 
     * Tell ISI of the IPC names we are using for ISI communication, 
     * Audio control (tones) and stream control. 
     */
    isi_ptr->msg.system.protocolIpc[0] = 0;
    if (NULL != ipcName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.protocolIpc,
                ISI_ADDRESS_STRING_SZ, "%s", ipcName_ptr);
    }
        
    isi_ptr->msg.system.mediaIpc[0] = 0;
    if (NULL != audioName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.mediaIpc,
                ISI_ADDRESS_STRING_SZ, "%s", audioName_ptr);
    }
    
    isi_ptr->msg.system.streamIpc[0] = 0;
    if (NULL != streamName_ptr) {
        OSAL_snprintf(isi_ptr->msg.system.streamIpc,
                ISI_ADDRESS_STRING_SZ, "%s", streamName_ptr);
    }
    return;
}

/* 
 * ======== _MC_vtspTimerCb() ========
 *
 * This function is registered with a timer used to registration with
 * ISI.  The timer calls this function when it expires.  It will send 
 * an ISI event indicating that it wants to register with ISI.
 *
 * Return Values: 
 * None
 */
static int32 _MC_vtspTimerCb(
    void *arg_ptr) 
{
    MC_Event   *evt_ptr;

    evt_ptr = (MC_Event *)arg_ptr;

    /* This is a periodic timer, do not need to wait forever for sending msg. */
    if (OSAL_SUCCESS != OSAL_msgQSend(evt_ptr->isiEvt, (char *)&evt_ptr->isiMsg,
            sizeof(ISIP_Message), OSAL_NO_WAIT, NULL)) {
        MC_dbgPrintf("%s: ERROR to send event.\n", __FUNCTION__);
        return (MC_ERR);
    }
    return (MC_OK);
}
/* 
 * ======== _MC_vtspTimerInit() ========
 *
 * This function initializes a timer used to register the app with
 * ISI.  The timer fires at the specified interval until 
 * ISI returns a Command indicating that it receimcd this event.
 *
 * Return Values: 
 * MC_OK The timer was succesfully initialized.
 * MC_ERR The timer failed to initialize
 */
vint _MC_vtspTimerInit(
    MC_VtspObj  *vtspObj_ptr)
{
    if (0 == vtspObj_ptr->tmr.id) {
        /* Launch a timer that will attempt to register to ISI */
        if (0 == (vtspObj_ptr->tmr.id = OSAL_tmrCreate())) {
            /* Then we can't register with ISI! */
            MC_dbgPrintf("%s %d: Error initializing the mc vtsp timer\n",
                    __FUNCTION__, __LINE__);
            return (MC_ERR);
        }
    }
    else {
        OSAL_tmrStop(vtspObj_ptr->tmr.id);
    }

    /* Now start the timer */
    if (OSAL_SUCCESS != OSAL_tmrPeriodicStart(vtspObj_ptr->tmr.id, _MC_vtspTimerCb, 
            &vtspObj_ptr->tmr.event, MC_REGISTER_TIMER_MS)) {
        OSAL_tmrDelete(vtspObj_ptr->tmr.id);
        vtspObj_ptr->tmr.id = 0;
        MC_dbgPrintf("%s %d: Error initializing the mc vtsp timer\n",
                    __FUNCTION__, __LINE__);
        return (MC_ERR);
    }
    return (MC_OK);
}

/* 
 * ======== _MC_vtspTimerDestroy() ========
 *
 * This function kills/frees the timer used to register this app to ISI.
 *
 * Return Values: 
 * None.
 */
void _MC_vtspTimerDestroy(
    MC_VtspObj *vtspObj_ptr)
{
    /* Kill/Free the timer */
    if (0 != vtspObj_ptr->tmr.id) {
        OSAL_tmrStop(vtspObj_ptr->tmr.id);
        OSAL_tmrDelete(vtspObj_ptr->tmr.id);
        vtspObj_ptr->tmr.id = 0;
    }   
    return;
}

/*
 * ======== _MC_vtspSetStreamInfo() ========
 *
 * This function caches the stream/call/protocol info so this application
 * can associate calls, stun sessions, and protocol types together.
 *
 * Return Values:
 * None.
 */
static void _MC_vtspSetStreamInfo(
    MC_VtspObj   *vtspObj_ptr,
    ISI_Id        callId,
    vint          streamId,
    vint          protocol)
{
    MC_StreamInfo *info_ptr;

    if (MC_STREAM_NUM > streamId) {
        info_ptr = &vtspObj_ptr->vtsp.streamInfo[streamId];
        info_ptr->protocol = protocol;
        info_ptr->callId = callId;
        info_ptr->streamId = streamId;
    }
    return;
}

/*
 * ======== MC_vtspGetStreamInfoViaCallId() ========
 *
 * This function searches a list to find the stream info that is associated
 * with a Call ID.
 *
 * Return Values:
 * A pointer to an object containing the stream info of the stream for which
 * the callId belongs to.
 */
MC_StreamInfo* MC_vtspGetStreamInfoViaCallId(
    MC_VtspObj    *vtspObj_ptr,
    ISI_Id         callId)
{
    vint           x;
    MC_StreamInfo *info_ptr;
    /*
     * Loop and find the MC_StreamInfo ibject that matches
     * the provided local address and local port info.
     */
    for (x = 0 ; x < MC_STREAM_NUM ; x++) {
        info_ptr = &vtspObj_ptr->vtsp.streamInfo[x];
        /* MC_dbgPrintf("%s: Comparing calls - id:%d to id:%d\n", __FUNCTION__,
                    info_ptr->callId, callId); */
        if (info_ptr->callId == callId) {
            /* Found a match */
            return (info_ptr);
        }
    }
    return NULL;
}

/*
 * ======== _MC_vtspGetStreamInfoViaStreamId() ========
 *
 * This function searches a list to find the stream info that is associated
 * with a Stream ID.
 *
 * Return Values:
 * A pointer to an object containing the stream info of the stream for which
 * the streamId belongs to.
 */
static MC_StreamInfo* _MC_vtspGetStreamInfoViaStreamId(
    MC_VtspObj    *vtspObj_ptr,
    vint           streamId)
{
    if (MC_STREAM_NUM > streamId) {
        return (&vtspObj_ptr->vtsp.streamInfo[streamId]);
    }
    return (NULL);
}

/*
 * ======== _MC_sendPacketToVtsp() ========
 *
 * This function builds (populates) a VTSP_Stun object based on an ISI
 * Command from ISI.
 *
 * Return Values:
 * None.
 */
static void _MC_sendPacketToVtsp(
    MC_VtspObj    *vtspObj_ptr,
    ISIP_Message  *cmd_ptr)
{
    ISIP_Stun      *stun_ptr;
    MC_StreamInfo  *info_ptr;
    vint            size;
    VTSP_Stun      *pkt_ptr;

    pkt_ptr = &vtspObj_ptr->vtspPacketSend;
    stun_ptr = &cmd_ptr->msg.media.media.stun;

    /* Get the streamId associated with the callId in from the ISI Message */
    if (NULL == (info_ptr = MC_vtspGetStreamInfoViaCallId(
            vtspObj_ptr, cmd_ptr->id))) {
        /* Can't find stream info for this call */
        MC_dbgPrintf("%s: Got STUN packet but no call exists for callId:%d\n",
                __FUNCTION__, cmd_ptr->id);
        return;
    }
    
    /* Interface is always '0' */
    pkt_ptr->infc = 0;
    /* set the streamId in the stun packet */
    pkt_ptr->streamId = info_ptr->streamId;
    /* Populate the rest */
    pkt_ptr->localAddr.ipv4  = stun_ptr->lclAddr;
    pkt_ptr->localAddr.port  = stun_ptr->lclPort;
    pkt_ptr->remoteAddr.ipv4 = stun_ptr->rmtAddr;
    pkt_ptr->remoteAddr.port = stun_ptr->rmtPort;
    
    /* MC_dbgPrintf("%s: Sending VTSP_Stun to callId:%d, streamId:%d.",
            __FUNCTION__, msg_ptr->id, pkt_ptr->streamId);
    MC_dbgPrintf(" rmtAddr:%x rmtPort:%d\n", pkt_ptr->remoteIpAddr,
            pkt_ptr->remotePort); */

    size = (stun_ptr->pktSize > VTSP_STUN_PAYLOAD_SZ) ? VTSP_STUN_PAYLOAD_SZ :
        stun_ptr->pktSize;

    pkt_ptr->pktSize = size;

    OSAL_memCpy(pkt_ptr->payload, stun_ptr->payload, size);
    /* We will use 'size' to check the ret value */
    if (VTSP_OK != (size = VTSP_stunSend(pkt_ptr, VTSP_TIMEOUT_NO_WAIT, 0))) {
        MC_dbgPrintf("%s: VTSP failed to send a STUN Packet returned:%d\n",
                __FUNCTION__, size);
    }
    return;
}

/*
 * ======== _MC_sendVtspPacketToIsi() ========
 *
 * This function builds (populates) an ISI Event based on a VTSP_Stun object
 *
 * Return Values:
 * None.
 */
static void _MC_sendVtspPacketToIsi(
    vint          protocol,
    ISI_Id        callId,
    VTSP_Stun    *pkt_ptr,
    MC_Event     *evt_ptr)
{
    ISIP_Message *msg_ptr;
    ISIP_Stun    *stun_ptr;
    vint          size;

    msg_ptr = &evt_ptr->isiMsg;
    msg_ptr->id = callId;
    msg_ptr->code = ISIP_CODE_MEDIA;
    msg_ptr->protocol = protocol;
    msg_ptr->msg.media.serviceId = 0;
    msg_ptr->msg.media.reason = ISIP_MEDIA_REASON_PKT_RECV;
    msg_ptr->msg.media.media.stream.type = ISI_SESSION_TYPE_AUDIO;
    /* Copy IP interface info */
    stun_ptr = &msg_ptr->msg.media.media.stun;
    stun_ptr->lclAddr = pkt_ptr->localAddr.ipv4;
    stun_ptr->lclPort = pkt_ptr->localAddr.port;
    stun_ptr->rmtAddr = pkt_ptr->remoteAddr.ipv4;
    stun_ptr->rmtPort = pkt_ptr->remoteAddr.port;
    /* Copy the STUN packet into here */
    size = (pkt_ptr->pktSize > sizeof(stun_ptr->payload)) ?
            sizeof(stun_ptr->payload) : pkt_ptr->pktSize;
    stun_ptr->pktSize = size;
    OSAL_memCpy(stun_ptr->payload, pkt_ptr->payload, size);
    MC_sendEvent(evt_ptr);
    return;
}

/*
 * ======== MC_sendEvent() ========
 *
 * This function sends events to ISI.
 *
 * Returns:
 * MC_OK  : Event sent.
 * MC_ERR : Error.
 */
vint MC_sendEvent(
    MC_Event *evt_ptr)
{
    if (OSAL_SUCCESS != OSAL_msgQSend(evt_ptr->isiEvt, (char *)&evt_ptr->isiMsg,
            sizeof(ISIP_Message), OSAL_WAIT_FOREVER, NULL)) {
        return (MC_ERR);
    }
    return (MC_OK);
}

#ifdef MC_DEBUG
/*
 * ======== _MC_vtspPrintStream() ========
 * Used for debugging.  This function will print the contents
 * of a vtsp stream object.
 *
 * Return Values:
 *  Nothing.
 *
 */
static void _MC_vtspPrintStream(
    vint             infc,
    MC_StreamOp      reason,
    VTSP_Stream     *stream_ptr)
{
    vint x;
    char str[64];

    static char* _MC_vtspAudioStr[] = {
        "MC_STREAM_OP_DIR",
        "MC_STREAM_OP_START",
        "MC_STREAM_OP_MODIFY",
        "MC_STREAM_OP_END",
    };


    MC_dbgPrintf("\nAudio Media CMD:%s Infc:%d Stream ID:%d direction:%d\n",
            _MC_vtspAudioStr[reason], infc,
            stream_ptr->streamId, stream_ptr->dir);
    if (OSAL_netIsAddrIpv6(&stream_ptr->localAddr)) {
        /* ipv6 */
        OSAL_netAddressToString(str, &stream_ptr->localAddr);
        MC_dbgPrintf("\tlclAddr:%s lclPort:%d lclCntlPort:%d\n",
                str,
                OSAL_netNtohs(stream_ptr->localAddr.port),
                OSAL_netNtohs(stream_ptr->localControlPort));

        OSAL_netAddressToString(str, &stream_ptr->remoteAddr);
        MC_dbgPrintf("\trmtAddr:%s rmtPort:%d rmtCntlPort:%d\n",
                str,
                OSAL_netNtohs(stream_ptr->remoteAddr.port),
                OSAL_netNtohs(stream_ptr->remoteControlPort));
    }
    else {
        MC_dbgPrintf("\tlclAddr:%x lclPort:%d lclCntlPort:%d\n",
                OSAL_netNtohl(stream_ptr->localAddr.ipv4),
                OSAL_netNtohs(stream_ptr->localAddr.port),
                OSAL_netNtohs(stream_ptr->localControlPort));
        MC_dbgPrintf("\trmtAddr:%x rmtPort:%d rmtCntlPort:%d\n",
                OSAL_netNtohl(stream_ptr->remoteAddr.ipv4),
                OSAL_netNtohs(stream_ptr->remoteAddr.port),
                OSAL_netNtohs(stream_ptr->remoteControlPort));
    }
    MC_dbgPrintf("Chosen Encoder :%d\n", stream_ptr->encoder);
    MC_dbgPrintf("Encode Types: ");
    for (x = 0 ; x < VTSP_ENCODER_NUM ; x++) {
        MC_dbgPrintf("%d ", stream_ptr->encodeType[x]);
    }
    MC_dbgPrintf("\n");
    MC_dbgPrintf("Encode Time: ");
    for (x = 0 ; x < VTSP_ENCODER_NUM ; x++) {
        MC_dbgPrintf("%d ", stream_ptr->encodeTime[x]);
    }
    MC_dbgPrintf("\n");
    MC_dbgPrintf("Decode Type: ");
    for (x = 0 ; x < VTSP_DECODER_NUM ; x++) {
        MC_dbgPrintf("%d ", stream_ptr->decodeType[x]);
    }
    MC_dbgPrintf("\n");
    MC_dbgPrintf("Security type:%d\n", stream_ptr->srtpSecurityType);
    
    MC_dbgPrintf("send key: ");
    for (x = 0 ; x < VTSP_SRTP_KEY_STRING_MAX_LEN ; x++) {
        MC_dbgPrintf("%x ", stream_ptr->srtpSendKey[x]);
    }
    MC_dbgPrintf("\n");
    
    MC_dbgPrintf("recv key: ");
    for (x = 0 ; x < VTSP_SRTP_KEY_STRING_MAX_LEN ; x++) {
        MC_dbgPrintf("%x ", stream_ptr->srtpRecvKey[x]);
    }
    MC_dbgPrintf("\n");
    
    return;
}
/*
 * ======== _MC_vtspPrintStreamVideo() ========
 * Used for debugging.  This function will print the contents
 * of a vtsp video stream object.
 *
 * Return Values:
 *  Nothing.
 *
 */
static void _MC_vtspPrintStreamVideo(
    vint             infc,
    MC_StreamOp      reason,
    VTSP_StreamVideo *stream_ptr)
{
    vint x;
    char str[64];

    static char* _MC_vtspVideoStr[] = {
        "MC_STREAM_OP_DIR",
        "MC_STREAM_OP_START",
        "MC_STREAM_OP_MODIFY",
        "MC_STREAM_OP_END",
    };


    MC_dbgPrintf("\nVideo Media CMD:%s Infc:%d Stream ID:%d direction:%d\n",
            _MC_vtspVideoStr[reason], infc,
            stream_ptr->streamId, stream_ptr->dir);
    if (OSAL_netIsAddrIpv6(&stream_ptr->localAddr)) {
        /* ipv6 */
        OSAL_netAddressToString(str, &stream_ptr->localAddr);
        MC_dbgPrintf("\tlclAddr:%s lclPort:%d lclCntlPort:%d\n",
                str,
                OSAL_netNtohs(stream_ptr->localAddr.port),
                OSAL_netNtohs(stream_ptr->localControlPort));

        OSAL_netAddressToString(str, &stream_ptr->remoteAddr);
        MC_dbgPrintf("\trmtAddr:%s rmtPort:%d rmtCntlPort:%d\n",
                str,
                OSAL_netNtohs(stream_ptr->remoteAddr.port),
                OSAL_netNtohs(stream_ptr->remoteControlPort));
    }
    else {
        MC_dbgPrintf("\tlclAddr:%x lclPort:%d lclCntlPort:%d\n",
                OSAL_netNtohl(stream_ptr->localAddr.ipv4),
                OSAL_netNtohs(stream_ptr->localAddr.port),
                OSAL_netNtohs(stream_ptr->localControlPort));
        MC_dbgPrintf("\trmtAddr:%x rmtPort:%d rmtCntlPort:%d\n",
                OSAL_netNtohl(stream_ptr->remoteAddr.ipv4),
                OSAL_netNtohs(stream_ptr->remoteAddr.port),
                OSAL_netNtohs(stream_ptr->remoteControlPort));
    }

    MC_dbgPrintf("Chosen Encoder :%d\n", stream_ptr->encoder);
    MC_dbgPrintf("Encode Types: ");
    for (x = 0 ; x < VTSP_ENCODER_VIDEO_NUM ; x++) {
        MC_dbgPrintf("%d ", stream_ptr->encodeType[x]);
    }
    MC_dbgPrintf("\n");
    MC_dbgPrintf("Encode Time: ");
    for (x = 0 ; x < VTSP_ENCODER_VIDEO_NUM ; x++) {
        MC_dbgPrintf("%d ", stream_ptr->encodeTime[x]);
    }
    MC_dbgPrintf("\n");
    MC_dbgPrintf("Decode Type: ");
    for (x = 0 ; x < VTSP_DECODER_VIDEO_NUM ; x++) {
        MC_dbgPrintf("%d ", stream_ptr->decodeType[x]);
    }
    MC_dbgPrintf("\n");
    MC_dbgPrintf("Security type:%d\n", stream_ptr->srtpSecurityType);

    MC_dbgPrintf("send key: ");
    for (x = 0 ; x < VTSP_SRTP_KEY_STRING_MAX_LEN ; x++) {
        MC_dbgPrintf("%x ", stream_ptr->srtpSendKey[x]);
    }
    MC_dbgPrintf("\n");

    MC_dbgPrintf("recv key: ");
    for (x = 0 ; x < VTSP_SRTP_KEY_STRING_MAX_LEN ; x++) {
        MC_dbgPrintf("%x ", stream_ptr->srtpRecvKey[x]);
    }
    MC_dbgPrintf("\n");

    return;
}
#endif

/*
 * ======== _MC_vtspMapDir() ========
 * This function is used to map ISI values that represent a stream's
 * direction to an enumerated value that the VTSP module will understand.
 *
 * Return Values:
 *  direction : a value representing the VTSP stream "direction".
 *
 */
static vint _MC_vtspMapDir(
    ISI_SessionDirection  dir)
{
    vint direction;

    /* Convert the ISI session direction value to the correct VTSP value */
    switch (dir) {
        case ISI_SESSION_DIR_SEND_RECV:
            direction = VTSP_STREAM_DIR_SENDRECV;
            break;
        case ISI_SESSION_DIR_SEND_ONLY:
            direction = VTSP_STREAM_DIR_SENDONLY;
            break;
        case ISI_SESSION_DIR_RECV_ONLY:
            direction = VTSP_STREAM_DIR_RECVONLY;
            break;
        case ISI_SESSION_DIR_INACTIVE:
        default:
            direction = VTSP_STREAM_DIR_INACTIVE;
            break;
    }
    return (direction);
}

static void _MV_vtspRtpPrintEvent(
    VTSP_EventMsg *event_ptr)
{
#ifdef VIDEO_DEBUG_LOG
    switch (event_ptr->msg.rtp.reason) { 
        case VTSP_EVENT_RTCP_RR:
            OSAL_logMsg("RTP Event: infc %d: streamId%d ssrc=0x%x\n", 
            event_ptr->infc, event_ptr->msg.rtp.streamId,
            event_ptr->msg.rtp.ssrc);
            OSAL_logMsg("RTCP_RR frac_cum_lost=0x%x hi_seq=0x%x jit=%d\n", 
                    event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg3,
                    event_ptr->msg.rtp.arg4);
            OSAL_logMsg("  rx_pkts=%d rx_bytes=%d\n", event_ptr->msg.rtp.arg5,
                    event_ptr->msg.rtp.arg6);
            break;
        case VTSP_EVENT_RTCP_SR:
            OSAL_logMsg("RTCP_SR ntp_hi=0x%x ntp_lo=0x%x rtp_ts=0x%x "
                    "tx_pkts=%d tx_bytes=%d\n", event_ptr->msg.rtp.arg1, 
                    event_ptr->msg.rtp.arg2, event_ptr->msg.rtp.arg3, 
                    event_ptr->msg.rtp.arg4, event_ptr->msg.rtp.arg5);
            break;
        case VTSP_EVENT_RTCP_SS:
            OSAL_logMsg("RTCP_SS flags=0x%x beg_end_seq=0x%x "
                    "lost_pkt=0x%x dup_pkt=%d\n",
                    event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg2,
                    event_ptr->msg.rtp.arg3, event_ptr->msg.rtp.arg4);
            OSAL_logMsg("  mean_jit=%d min_jit=%d max_jit=%d dev_jit=%d\n", 
                    event_ptr->msg.rtp.arg5, event_ptr->msg.rtp.arg6,
                    event_ptr->msg.rtp.arg7, event_ptr->msg.rtp.arg8);
            break;
        case VTSP_EVENT_RTCP_MR:
            OSAL_logMsg("RTCP_MR loss_rate=0x%x discard_rate=%d "
                    "end_sys_delay=%d\n", event_ptr->msg.rtp.arg1,
                    event_ptr->msg.rtp.arg2, event_ptr->msg.rtp.arg3);
            OSAL_logMsg("  RERL=%d jb_nom=%d jb_max=%d jb_abs_max=%d\n", 
                    event_ptr->msg.rtp.arg4, event_ptr->msg.rtp.arg5,
                    event_ptr->msg.rtp.arg6, event_ptr->msg.rtp.arg7);
            break;
        case VTSP_EVENT_RTCP_CS:
            OSAL_logMsg("RTCP_CS enc_bytes=0x%x dec_bytes=0x%x "
                    "enc_pkts=%d dec_pkts=%d\n",
                    event_ptr->msg.rtp.arg1, event_ptr->msg.rtp.arg2,
                    event_ptr->msg.rtp.arg3, event_ptr->msg.rtp.arg4);
            OSAL_logMsg("  cn_enc=%d cn_dec=%d plc_runs=%d nse_runs=%d\n",
                        event_ptr->msg.rtp.arg5, event_ptr->msg.rtp.arg6,
                        event_ptr->msg.rtp.arg7, event_ptr->msg.rtp.arg8);
            OSAL_logMsg("  ticks=%d\n", event_ptr->msg.rtp.arg9);
                    break;
        default:;
    }
#endif
}

/*
 * ======== _MC_processRtpRtcpInactive() ========
 * This function process RTP/RTCP inactivity timeout.
 * This function will be called every second.
 *
 * Return Values:
 *
 */
static void _MC_processRtpRtcpInactive(
    MC_VtspObj     *vtspObj_ptr,
    VTSP_EventMsg  *event_ptr)
{
    MC_StreamInfo  *info_ptr;
    MC_AudioStream *audio_ptr;
    OSAL_Boolean    terminate;
    vint            streamId;

    terminate = OSAL_FALSE;
    streamId  = event_ptr->msg.rtp.streamId;

    /* If RTP inavtivity timeout checking is disable, return */
    if (1 == vtspObj_ptr->vtsp.streamInfo[streamId].rtpCheckingDisable) {
        return;
    }

    /* Process RTP/RTCP inactivity time per 1sec */
    audio_ptr = &vtspObj_ptr->vtsp.audio[streamId];
    if (VTSP_EVENT_RTCP_RR == event_ptr->msg.rtp.reason) {
        /* If stream dir is sendrecv, need to check rtp inactive timeout. */
        if (VTSP_STREAM_DIR_SENDRECV == audio_ptr->vtspStream.dir) {
            if (audio_ptr->recvPktCount != event_ptr->msg.rtp.arg5) {
                audio_ptr->rtpInactiveTmr = 0;
                audio_ptr->recvPktCount  = event_ptr->msg.rtp.arg5;
            }
            else {
                audio_ptr->rtpInactiveTmr++;
                if (audio_ptr->rtpInactiveTmr == vtspObj_ptr->inactivityExp) {
                    terminate = OSAL_TRUE;
                }
            }
        }
        /* If stream is in hold state, need to check RTP inactive timeout. */
        if (VTSP_STREAM_DIR_SENDRECV != audio_ptr->vtspStream.dir) {
            audio_ptr->rtcpInactiveTmr++;
            if (audio_ptr->rtcpInactiveTmr == vtspObj_ptr->inactivityExp) {
                terminate = OSAL_TRUE;
            }
        }
    }

    /* Send ISI event, if need to terminate the call. */
    if (OSAL_TRUE == terminate) {
        /* Build an ISI event and send it to ISI */
        if (NULL != (info_ptr = _MC_vtspGetStreamInfoViaStreamId(vtspObj_ptr,
                event_ptr->msg.rtp.streamId))) {
            ISIP_Message *msg_ptr;
            msg_ptr = &vtspObj_ptr->event.isiMsg;
            msg_ptr->id = info_ptr->callId;
            msg_ptr->code = ISIP_CODE_MEDIA;
            msg_ptr->protocol = info_ptr->protocol;
            msg_ptr->msg.media.serviceId = 0;
            msg_ptr->msg.media.reason = ISIP_MEDIA_REASON_RTP_RTCP_INACTIVE;
            msg_ptr->msg.media.media.stream.type = ISI_SESSION_TYPE_AUDIO;
            MC_sendEvent(&vtspObj_ptr->event);
        }
    }
}

/* 
 * ======== _MC_vtspPrepare() ========
 *
 * This is MC_vtsp init actions to be done at the task thread time
 *
 * Return Values: 
 * None.
 */
void _MC_vtspPrepare(
    MC_VtspObj *vtspObj_ptr)
{

#ifndef MC_NO_TONE 
    /* Set up the default tone templates used */
    MC_toneInit();
#endif
}

/* 
 * ======== _MC_vtspProcessEvt() ========
 *
 * This is MC_vtsp event processing in task thread.
 * It handles events from VTSP and sends interesting ones to ISI
 *
 * Return Values: 
 * None.
 */
void _MC_vtspProcessEvt(
    MC_VtspObj     *vtspObj_ptr,
    VTSP_EventMsg  *event_ptr)
{
    VTSP_Stun      *stun_ptr;
    MC_StreamInfo  *info_ptr;

    stun_ptr = &vtspObj_ptr->vtspPacketRecv;

    if (VTSP_EVENT_MSG_CODE_TIMER == event_ptr->code) {
        vtspObj_ptr->bitrate = event_ptr->msg.time.bitrate;
    }
    else if (VTSP_EVENT_MSG_CODE_JB == event_ptr->code) {
        /*
         * This is generated every second
         */

        if (VTSP_EVENT_ACTIVE == event_ptr->msg.jb.reason) {

            /* XXX In the future we could send JB info to RIR */
            /* Request the RTCP event */
            VTSP_streamQuery(event_ptr->infc, event_ptr->msg.jb.streamId);
        }
    }
    else if (VTSP_EVENT_MSG_CODE_RTP == event_ptr->code) {
        _MV_vtspRtpPrintEvent(event_ptr);
        /* Process RTP/RTCP inactive time, if inactivity timer is enable. */
        if (0 < vtspObj_ptr->inactivityExp) {
            _MC_processRtpRtcpInactive(vtspObj_ptr, event_ptr);
        }
    }
    else if (VTSP_EVENT_MSG_CODE_RTCP == event_ptr->code) {
        /* Reset RTCP Inactive timer */
        vtspObj_ptr->vtsp.audio[event_ptr->msg.rtcp.streamId].rtcpInactiveTmr = 0;
    }
    /* Check if there are STUN packets waiting to be read */
    else if (VTSP_EVENT_MSG_CODE_STUN_RECV == event_ptr->code) {
        while (VTSP_OK == VTSP_stunRecv(stun_ptr,
                VTSP_TIMEOUT_NO_WAIT, 0)) {
            /* Build an ISI event and send it to ISI */
            if (NULL != (info_ptr = _MC_vtspGetStreamInfoViaStreamId(
                    vtspObj_ptr, stun_ptr->streamId))) {

                _MC_sendVtspPacketToIsi(info_ptr->protocol,
                        info_ptr->callId, stun_ptr, &vtspObj_ptr->event);
            }
        }
    }
    else if (VTSP_EVENT_MSG_CODE_DIGIT_GENERATE == event_ptr->code) {
        if ((VTSP_EVENT_COMPLETE == event_ptr->msg.digitGenerate.reason) &&
                (ISI_TEL_EVENT_DTMF == vtspObj_ptr->telEvt.evt ||
                ISI_TEL_EVENT_DTMF_STRING == vtspObj_ptr->telEvt.evt) &&
                (0 != vtspObj_ptr->telEvt.isiId)) {
            MC_dbgPrintf("%s: VTSP_EVENT_MSG_CODE_DIGIT_GENERATE done\n",
                    __FUNCTION__);
            /* Count down till we've sent all pending digits */
            if (0 != --vtspObj_ptr->telEvt.numDigits) {
                /* Wait till it last event before send the event */
                return;
            }
            MC_dbgPrintf("%s: send ISIP_TEL_EVENT_REASON_COMPLETE\n",
                    __FUNCTION__);
            /* Then notify ISI */
            MC_telEvtIsiEvt(vtspObj_ptr->telEvt.isiId,
                    vtspObj_ptr->telEvt.isiServiceId,
                    ISIP_TEL_EVENT_REASON_COMPLETE,
                    vtspObj_ptr->telEvt.evt, &vtspObj_ptr->event.isiMsg);
            /* Clear this field for next time */
            vtspObj_ptr->telEvt.isiId = 0;
            MC_sendEvent(&vtspObj_ptr->event);
        }
    }
    else if (VTSP_EVENT_MSG_CODE_TONE_GENERATE == event_ptr->code) {
        if (VTSP_EVENT_TONE_DIR_STREAM == event_ptr->msg.toneGenerate.direction) {
            /*
             * Check if this event is for a pending tel event.  If so then
             * it must be reported to ISI.
             */
            if ((VTSP_EVENT_MAX_TIMEOUT == event_ptr->msg.toneGenerate.reason) &&
                    (ISI_TEL_EVENT_DTMF == vtspObj_ptr->telEvt.evt ||
                    ISI_TEL_EVENT_DTMF_STRING == vtspObj_ptr->telEvt.evt) &&
                    (0 != vtspObj_ptr->telEvt.isiId)) {
                MC_dbgPrintf("%s: got VTSP_EVENT_MSG_CODE_TONE_GENERATE done\n",
                        __FUNCTION__);
                /* Count down till we've sent all pending digits */
                if (0 != --vtspObj_ptr->telEvt.numDigits) {
                    /* Wait till it last event before send the event */
                    return;
                }
                MC_dbgPrintf("%s: send ISIP_TEL_EVENT_REASON_COMPLETE\n",
                        __FUNCTION__);
                /* Then notify ISI */
                MC_telEvtIsiEvt(vtspObj_ptr->telEvt.isiId,
                        vtspObj_ptr->telEvt.isiServiceId,
                        ISIP_TEL_EVENT_REASON_COMPLETE,
                        vtspObj_ptr->telEvt.evt, &vtspObj_ptr->event.isiMsg);
                /* Clear this field for next time */
                vtspObj_ptr->telEvt.isiId = 0;
                MC_sendEvent(&vtspObj_ptr->event);
            }
        }
        else {
            /*
             * Else, then it's just a locally generated tone. If 'auto' call
             * progress is enabled then we have to tell the codec controller
             * that we are done playing the tone.
             */
            if (VTSP_EVENT_MAX_TIMEOUT == event_ptr->msg.toneGenerate.reason ||
                VTSP_EVENT_HALTED == event_ptr->msg.toneGenerate.reason ||
                VTSP_EVENT_COMPLETE == event_ptr->msg.toneGenerate.reason) {

                if (0 != vtspObj_ptr->vtsp.autoCallProgressTones) {
                    /*
                     * Then 'auto' call progress is tones is enabled, so we
                     * should flip the codec when tones end.
                     */
                    MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
                }
            }
        }
    }
    else if (VTSP_EVENT_MSG_CODE_TONE_DETECT == event_ptr->code) {
        if (NULL != (info_ptr = _MC_vtspGetStreamInfoViaStreamId(vtspObj_ptr,
                event_ptr->msg.toneDetect.streamId))) {
            ISIP_Message *msg_ptr;
            msg_ptr = &vtspObj_ptr->event.isiMsg;
            msg_ptr->id       = info_ptr->callId;
            msg_ptr->code     = ISIP_CODE_TEL_EVENT;
            msg_ptr->protocol = info_ptr->protocol;
            msg_ptr->msg.event.reason    = ISIP_TEL_EVENT_REASON_NEW;
            msg_ptr->msg.event.serviceId = vtspObj_ptr->vtsp.audio[
                    info_ptr->streamId].serviceId;
            msg_ptr->msg.event.callId    = info_ptr->callId;
            msg_ptr->msg.event.evt       = ISI_TEL_EVENT_DTMF_DETECT;
            msg_ptr->msg.event.settings.dtmfDetect.digit =
                    event_ptr->msg.toneDetect.tone;
            if (VTSP_EVENT_LEADING == event_ptr->msg.toneDetect.edgeType) {
                msg_ptr->msg.event.settings.dtmfDetect.edgeType =
                        ISI_DTMFDECT_TYPE_LEADING;
            }
            else {
                msg_ptr->msg.event.settings.dtmfDetect.edgeType =
                        ISI_DTMFDECT_TYPE_TRAILING;
            }
            MC_sendEvent(&vtspObj_ptr->event);
        }
        else if (VTSP_EVENT_MSG_CODE_LIP_SYNC_AUDIO == event_ptr->code) {
            if (vtspObj_ptr->vtsp.syncEngine.enabled) {
                if (VTSP_EVENT_LIP_SYNC_RTCP_RECEIVED == event_ptr->msg.syncEngine.reason) {
                    vtspObj_ptr->vtsp.syncEngine.aNtp = event_ptr->msg.syncEngine.rtcpNtp;
                    vtspObj_ptr->vtsp.syncEngine.aRtcpRtp = event_ptr->msg.syncEngine.rtcpRtp;
                }
                else if (VTSP_EVENT_LIP_SYNC_RTP_TS == event_ptr->msg.syncEngine.reason) {
                    vtspObj_ptr->vtsp.syncEngine.aLatestRtp = event_ptr->msg.syncEngine.rtpTs;
                }
            }
        }
        else if (VTSP_EVENT_MSG_CODE_LIP_SYNC_VIDEO == event_ptr->code) {
            if (vtspObj_ptr->vtsp.syncEngine.enabled) {
                if (VTSP_EVENT_LIP_SYNC_RTCP_RECEIVED == event_ptr->msg.syncEngine.reason) {
                    vtspObj_ptr->vtsp.syncEngine.vNtp = event_ptr->msg.syncEngine.rtcpNtp;
                    vtspObj_ptr->vtsp.syncEngine.vRtcpRtp = event_ptr->msg.syncEngine.rtcpRtp;
                }
                else if (VTSP_EVENT_LIP_SYNC_RTP_TS == event_ptr->msg.syncEngine.reason) {
                    vtspObj_ptr->vtsp.syncEngine.vLatestRtp = event_ptr->msg.syncEngine.rtpTs;
                    vtspObj_ptr->vtsp.syncEngine.vStreamId = event_ptr->msg.syncEngine.streamId;
                    vtspObj_ptr->vtsp.syncEngine.vInfc = event_ptr->infc;
                    /* Upon Video RTP reception. Notify A/V skew to Video Engine. */
                    _MC_LipSync_calculateAudioVideoSkew(&vtspObj_ptr->vtsp.syncEngine);
                }
            }
        }
    }
}
/*
 * ======== _MC_eventTask() ========
 * This task reads events from VTSP and sends interesting ones to ISI.
 *
 * Return Values:
 *
 */
static void _MC_eventTask(
    void *arg_ptr)
{
    VTSP_EventMsg   event;
    MC_VtspObj     *vtspObj_ptr;

    vtspObj_ptr = (MC_VtspObj *)arg_ptr;

    _MC_vtspPrepare(vtspObj_ptr);

_MC_VTSP_EVENT_TASK_LOOP:

    if (VTSP_OK ==
            VTSP_getEvent(VTSP_INFC_ANY, &event, VTSP_TIMEOUT_FOREVER)) {

        /* MC_dbgPrintf("%s: Got Event from VTSP code:%d\n", __FUNCTION__,
                   event.code); */
        _MC_vtspProcessEvt(vtspObj_ptr, &event);

    }
    goto _MC_VTSP_EVENT_TASK_LOOP;
}

/*
 * ======== _MC_vtspSetRtcpCname() ========
 * This function sets up CNAME attribute of Audio RTCP.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspSetRtcpCname(
    vint  infc,
    char *cname_ptr)
{
    if (VTSP_OK != VTSP_rtcpCname(infc, cname_ptr)) {
        return (MC_ERR);
    }
    return (MC_OK);
}

/*
 * ======== _MC_vtspSetRtcpCnameVideo() ========
 * This function sets up CNAME attribute of Video RTCP.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspSetRtcpCnameVideo(
    vint  infc,
    char *cname_ptr)
{
    if (VTSP_OK != VTSP_rtcpCnameVideo(infc, cname_ptr)) {
        return (MC_ERR);
    }
    return (MC_OK);
}

/*
 * ======== _MC_vtspModifyStream() ========
 * This function modifies vTSP stream e.g. start, stop, direction.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspModifyStream(
    vint                   infc,
    MC_StreamOp            op,
    ISI_SessionDirection   dir,
    VTSP_Stream           *stream_ptr,
    MC_VtspCodecControl   *codec_ptr)
{
    VTSP_StreamDir  direction;
    
    direction = (VTSP_StreamDir)_MC_vtspMapDir(dir);

    MC_dbgPrintf("%s: Op:%d Direction:%d\n", __FUNCTION__, op, direction);

    switch (op) {

        /* Change direction of a stream. arg defines the direction. */
        case MC_STREAM_OP_DIR:
            if (VTSP_OK != VTSP_streamModifyDir(infc, stream_ptr->streamId,
                    direction)) {
                return (MC_ERR);
            }
            break;

        /* Start a stream. */
        case MC_STREAM_OP_START:
            /* Enable the IP (WiFi or ARM) codec */
            MC_vtspHwControl(codec_ptr, OSAL_TRUE);

            stream_ptr->dir = direction;
            if (VTSP_OK != VTSP_streamStart(infc, stream_ptr)) {
                /* Disable the IP (WiFi or ARM) codec. */    
                MC_vtspHwControl(codec_ptr, OSAL_FALSE);
                return (MC_ERR);
            }
            break;

        /* Stop a stream. */
        case MC_STREAM_OP_END:
            /* If the stream is not started, do not need to end it. */
            if (_VTSP_STREAM_DIR_ENDED == stream_ptr->dir) {
                break;
            }

            /* Disable the IP (WiFi or ARM) codec. */    
            MC_vtspHwControl(codec_ptr, OSAL_FALSE);
        
            if (VTSP_OK != VTSP_streamEnd(infc, stream_ptr->streamId)) {
                return (MC_ERR);
            }
            break;

        /* Modify a stream. */
        case MC_STREAM_OP_MODIFY:
            /* Map stream direction code. */
            stream_ptr->dir = direction;
            if (VTSP_OK != VTSP_streamModify(infc, stream_ptr)) {
                return (MC_ERR);
            }
            break;

        default:
            return (MC_ERR);
    }
    return (MC_OK);
}

/*
 * ======== _MC_vtspModifyStreamVideo() ========
 * This function modifies vTSP video stream e.g. start, stop, direction.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspModifyStreamVideo(
    vint                   infc,
    MC_StreamOp            op,
    ISI_SessionDirection   dir,
    VTSP_StreamVideo      *stream_ptr)
{
    VTSP_StreamDir  direction;

    direction = (VTSP_StreamDir)_MC_vtspMapDir(dir);

    MC_dbgPrintf("%s: Op:%d Direction:%d\n", __FUNCTION__, op, direction);

    switch (op) {

        /* Change direction of a stream. arg defines the direction. */
        /* Modify a stream. */
        case MC_STREAM_OP_START:
            stream_ptr->dir = direction;
            if (VTSP_OK != VTSP_streamVideoStart(infc, stream_ptr)) {
                return (MC_ERR);
            }
            break;

        case MC_STREAM_OP_DIR:
            if (VTSP_OK != VTSP_streamVideoModifyDir(infc, stream_ptr->streamId,
                    direction)) {
                return (MC_ERR);
            }
            break;

        /* Start a stream. */
        case MC_STREAM_OP_MODIFY:
            stream_ptr->dir = direction;
            if (VTSP_OK != VTSP_streamVideoModify(infc, stream_ptr)) {
                return (MC_ERR);
            }
            break;

        /* Stop a stream. */
        case MC_STREAM_OP_END:
            if (VTSP_OK != VTSP_streamVideoEnd(infc, stream_ptr->streamId)) {
                return (MC_ERR);
            }
            break;

        default:
            return (MC_ERR);
    }
    return (MC_OK);
}

/*
 * ======== _MC_vtspModifyStreamConfMask() ========
 * Sets the vTSP stream mix masks.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspModifyStreamConfMask(
    vint         infc,
    VTSP_Stream *stream_ptr,
    vint         streamId,
    uint32       mask)
{
    MC_dbgPrintf("%s:CONF streamId:%d mask:%x\n", __FUNCTION__, streamId, mask);

    /* Cache the conference bitmask inside the stream object */
    stream_ptr->confMask = mask;

    if (VTSP_OK != VTSP_streamModifyConf(infc, streamId, mask)) {
        return (MC_ERR);
    }

    return (MC_OK);
}

/*
 * ======== _MC_vtspModifyStreamVideoConfMask() ========
 * Sets the vTSP stream mix masks.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspModifyStreamVideoConfMask(
    vint         infc,
    VTSP_StreamVideo *stream_ptr,
    vint         streamId,
    uint32       mask)
{
    MC_dbgPrintf("%s:CONF streamId:%d mask:%x\n", __FUNCTION__, streamId, mask);

    /* Cache the conference bitmask inside the stream object */
    stream_ptr->confMask = mask;

    if (VTSP_OK != VTSP_streamVideoModifyConf(infc, streamId, mask)) {
        return (MC_ERR);
    }

    return (MC_OK);
}


/*
 * ======== _MC_vtspInitStream() ========
 * This function sets up vTSP stream network params e.g. IP addresses, and
 * direction. Also initializes stream.
 *
 * Return Values:
 *
 */
static void _MC_vtspInitStream(
    VTSP_Stream     *stream_ptr,
    OSAL_NetAddress *localIpAddr_ptr,
    uint16           localControlPort,
    OSAL_NetAddress *remoteIpAddr_ptr,
    uint16           remoteControlPort,
    uint16           securityType,
    char            *securityKeyLcl,
    char            *securityKeyRmt)
{
    vint c;

    /*  Init the IP interface settings. */
    OSAL_netAddrPortCpy(&stream_ptr->localAddr, localIpAddr_ptr);
    OSAL_netAddrPortCpy(&stream_ptr->remoteAddr, remoteIpAddr_ptr);
    stream_ptr->remoteControlPort = remoteControlPort;

    /* XXX: If RTCP can not be transmitted, it should not be received either. */
    if (0 == remoteControlPort) {
        stream_ptr->localControlPort = 0;
    }
    else {
        stream_ptr->localControlPort = localControlPort;
    }

    stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
    stream_ptr->extension = 0;

    /* G.711U is setup as default while setting other coders to unavailable. */
    for (c = 0; c < VTSP_ENCODER_NUM; c++) {
        stream_ptr->encodeType[c] = VTSP_CODER_UNAVAIL;
        stream_ptr->encodeTime[c] = 0;
    }
    for (c = 0; c < VTSP_DECODER_NUM; c++) {
        stream_ptr->decodeType[c] = VTSP_CODER_UNAVAIL;
    }
    stream_ptr->encodeType[VTSP_CODER_G711U] = 0;
    stream_ptr->decodeType[VTSP_CODER_G711U] = 0;
    stream_ptr->encodeTime[VTSP_CODER_G711U] = 10;
    
    stream_ptr->srtpSecurityType = securityType;
    if (securityType != VTSP_SRTP_SECURITY_SERVICE_NONE) {
        /* Then copy the keys to the stream object */
        OSAL_memCpy(stream_ptr->srtpSendKey, securityKeyLcl,
                VTSP_SRTP_KEY_STRING_MAX_LEN);
        OSAL_memCpy(stream_ptr->srtpRecvKey, securityKeyRmt,
                VTSP_SRTP_KEY_STRING_MAX_LEN);
    }
}

/*
 * ======== _MC_vtspInitStreamVideo() ========
 * This function sets up vTSP stream network params e.g. IP addresses, and
 * direction. Also initializes stream.
 *
 * Return Values:
 *
 */
static void _MC_vtspInitStreamVideo(
    VTSP_StreamVideo *stream_ptr,
    OSAL_NetAddress  *localIpAddr_ptr,
    uint16            localControlPort,
    OSAL_NetAddress  *remoteIpAddr_ptr,
    uint16            remoteControlPort,
    uint16            securityType,
    char             *securityKeyLcl,
    char             *securityKeyRmt)
{
    vint c;

    /*  Init the IP interface settings. */
    OSAL_netAddrPortCpy(&stream_ptr->localAddr, localIpAddr_ptr);
    OSAL_netAddrPortCpy(&stream_ptr->remoteAddr, remoteIpAddr_ptr);
    stream_ptr->remoteControlPort = remoteControlPort;

    /* XXX: If RTCP can not be transmitted, it should not be received either. */
    if (0 == remoteControlPort) {
        stream_ptr->localControlPort = 0;
    }
    else {
        stream_ptr->localControlPort = localControlPort;
    }

    stream_ptr->peer = VTSP_STREAM_PEER_NETWORK;
    stream_ptr->extension = 0;

    /* G.711U is setup as default while setting other coders to unavailable. */
    for (c = 0; c < VTSP_ENCODER_VIDEO_NUM; c++) {
        stream_ptr->encodeType[c] = VTSP_CODER_UNAVAIL;
        stream_ptr->encodeTime[c] = 0;
    }
    for (c = 0; c < VTSP_DECODER_VIDEO_NUM; c++) {
        stream_ptr->decodeType[c] = VTSP_CODER_UNAVAIL;
    }

    stream_ptr->srtpSecurityType = securityType;
    if (securityType != VTSP_SRTP_SECURITY_SERVICE_NONE) {
        /* Then copy the keys to the stream object */
        OSAL_memCpy(stream_ptr->srtpSendKey, securityKeyLcl,
                VTSP_SRTP_KEY_STRING_MAX_LEN);
        OSAL_memCpy(stream_ptr->srtpRecvKey, securityKeyRmt,
                VTSP_SRTP_KEY_STRING_MAX_LEN);
    }
}

/*
 * ======== _MC_vtspGetCoder() ========
 * This function is used to map ISI values that represent a coder type
 * (i.e. G711U, G729AB, etc.) to a value that the VTSP module will
 * understand.
 *
 * Return Values:
 *  c : a value representing a VTSP coder type.
 *
 */
static vint _MC_vtspGetCoder(
    MC_Coder       *coder_ptr,
    VTSP_Stream    *stream_ptr,
    vint            enablePrint)
{
    vint    c;
    vint    bitIdx;

    /*
     * Map to vTSP coder enum. Set silence compression & dtmf relay in vTSP,
     * if required.
     */
    if (!OSAL_strncasecmp(coder_ptr->szCoderName, "pcmu",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_G711U;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "pcma",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_G711A;
    } 
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "g726-32",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_G726_32K;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "silk-24k",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_SILK_20MS_24K;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "silk-16k",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_SILK_20MS_16K;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "silk-8k",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_SILK_20MS_8K;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "g722",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_G722;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "g7221",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_G722P1_20MS;
        stream_ptr->silenceComp |= VTSP_MASK_CODER_G722P1_20MS;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "cn",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_CN;
        stream_ptr->silenceComp |= (VTSP_MASK_CODER_G711U |
                VTSP_MASK_CODER_G711A | VTSP_MASK_CODER_G726_32K |
                VTSP_MASK_CODER_G722);
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "g729",
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_G729;
        stream_ptr->silenceComp &= ~VTSP_MASK_CODER_G729;
        if (coder_ptr->props.g729.annexb) {
            stream_ptr->silenceComp |= VTSP_MASK_CODER_G729;
        }
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "ilbc",
            sizeof(coder_ptr->szCoderName))) {
        if (coder_ptr->rate == 30) {
            c = VTSP_CODER_ILBC_30MS;
        }
        else {
            c = VTSP_CODER_ILBC_20MS;
        }
        stream_ptr->silenceComp &= ~VTSP_MASK_CODER_ILBC_30MS;
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "amr",
            sizeof(coder_ptr->szCoderName))) {
        if (coder_ptr->props.amr.octetAlign) {
            c = VTSP_CODER_GAMRNB_20MS_OA;
            stream_ptr->silenceComp |= VTSP_MASK_CODER_GAMRNB_20MS_OA;
        }
        else {
            c = VTSP_CODER_GAMRNB_20MS_BE;
            stream_ptr->silenceComp |= VTSP_MASK_CODER_GAMRNB_20MS_BE;
        }
        /* Set bit rate. select the highest bit rate in modeset */
        if (0 == coder_ptr->props.amr.modeSet) {
            stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_122;
        }
        else {
            for (bitIdx = 7; bitIdx >=0; bitIdx--) {
                if (coder_ptr->props.amr.modeSet & (1 << bitIdx)) {
                    break;
                }
            }
            switch (bitIdx) {
                case 0:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_475;
                    break;
                case 1:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_515;
                    break;
                case 2:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_59;
                    break;
                case 3:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_67;
                    break;
                case 4:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_74;
                    break;
                case 5:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_795;
                    break;
                case 6:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_102;
                    break;
                case 7:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_122;
                    break;
                default:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRNB_20MS_122;
                    break;
            }
        }
    }
    else if (!OSAL_strncasecmp(coder_ptr->szCoderName, "amr-wb",
            sizeof(coder_ptr->szCoderName))) {
        if (coder_ptr->props.amr.octetAlign) {
            c = VTSP_CODER_GAMRWB_20MS_OA;
            stream_ptr->silenceComp |= VTSP_MASK_CODER_GAMRWB_20MS_OA;
        }
        else {
            c = VTSP_CODER_GAMRWB_20MS_BE;
            stream_ptr->silenceComp |= VTSP_MASK_CODER_GAMRWB_20MS_BE;
        }
        /* Set bit rate. select the highest bit rate in modeset */
        if (0 == coder_ptr->props.amr.modeSet) {
            stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_2385;
        }
        else {
            for (bitIdx = 8; bitIdx >=0; bitIdx--) {
                if (coder_ptr->props.amr.modeSet & (1 << bitIdx)) {
                    break;
                }
            }
            switch (bitIdx) {
                case 0:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_660;
                    break;
                case 1:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_885;
                    break;
                case 2:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_1265;
                    break;
                case 3:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_1425;
                    break;
                case 4:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_1585;
                    break;
                case 5:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_1825;
                    break;
                case 6:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_1985;
                    break;
                case 7:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_2305;
                    break;
                case 8:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_2385;
                    break;
                default:
                    stream_ptr->extension |= VTSP_MASK_EXT_GAMRWB_20MS_2385;
                    break;
            }
        }
    }
    else if ((!OSAL_strncasecmp(coder_ptr->szCoderName, "telephone-event",
            sizeof(coder_ptr->szCoderName))) ||
            (!OSAL_strncasecmp(coder_ptr->szCoderName, "telephone-event-16k",
            sizeof(coder_ptr->szCoderName)))) {
        c = VTSP_CODER_DTMF;
        stream_ptr->dtmfRelay = (VTSP_MASK_CODER_G711U |
                VTSP_MASK_CODER_G711A | VTSP_MASK_CODER_G726_32K |
                VTSP_MASK_CODER_G729 | VTSP_MASK_CODER_ILBC_20MS |
                VTSP_MASK_CODER_ILBC_30MS | VTSP_MASK_CODER_SILK_20MS_8K |
                VTSP_MASK_CODER_SILK_20MS_16K | VTSP_MASK_CODER_SILK_20MS_24K |
                VTSP_MASK_CODER_G722 | VTSP_MASK_CODER_G722P1_20MS |
                VTSP_MASK_CODER_GAMRNB_20MS_OA | VTSP_MASK_CODER_GAMRNB_20MS_BE |
                VTSP_MASK_CODER_GAMRWB_20MS_OA | VTSP_MASK_CODER_GAMRWB_20MS_BE);
    }
    else {
        c = VTSP_CODER_UNAVAIL;
    }
    if ((VTSP_CODER_UNAVAIL != c) && enablePrint) {
        OSAL_logMsg("MC vTSP: CoderName=%s, vtsp coder=%d\n", 
                coder_ptr->szCoderName, c);
    }
    return (c);
}

/*
 * ======== _MC_vtspGetCoderVideo() ========
 * This function is used to map ISI values that represent a coder type
 * (i.e. H264 etc.) to a value that the VTSP module will understand.
 *
 * Return Values:
 *  c : a value representing a VTSP coder type.
 *
 */
static vint _MC_vtspGetCoderVideo(
    MC_Coder         *coder_ptr,
    VTSP_StreamVideo *stream_ptr)
{
    vint   c;

    /*
     * Map to vTSP coder enum.
     */
    if (MC_OK == MC_isH264(coder_ptr->szCoderName,
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_VIDEO_H264;
    }
    else if (MC_OK == MC_isH263(coder_ptr->szCoderName,
            sizeof(coder_ptr->szCoderName))) {
        c = VTSP_CODER_VIDEO_H263;
    }
    else {
        c = VTSP_CODER_UNAVAIL;
    }
    if (VTSP_CODER_UNAVAIL != c) {
        OSAL_logMsg("MC vTSP: CoderName=%s, vtsp coder=%d\n", 
                coder_ptr->szCoderName, c);
    }
    return (c);
}

/*
 * ======== _MC_vtspSetStreamDynamicEncoder() ========
 * This function sets up vTSP stream to support mapping of coder name to coder
 * number used in RTP. Coder name and corresponding number to use is provided
 * by signalling. Coder rate is also set.
 * Use < 0 to clear it.
 *
 * Return Values:
 *
 */
static void _MC_vtspSetStreamDynamicEncoder(
    VTSP_Stream   *stream_ptr,
    MC_Coder      *coder_ptr)
{
    vint vtspCoder;
    /*
     * Map to vTSP coder enum. Set silence compression & dtmf relay in vTSP,
     * if required.
     */
    vtspCoder = _MC_vtspGetCoder(coder_ptr, stream_ptr, 1);
    if (vtspCoder == VTSP_CODER_UNAVAIL) {
        /* Invalid coder, perform nothing */
        return;
    }

    /* Now set it in stream. */
    if (coder_ptr->coderNum >= 0) {
        stream_ptr->encodeType[vtspCoder] = coder_ptr->coderNum;
        stream_ptr->encodeTime[vtspCoder] = coder_ptr->rate;

        /* If this is the default coder, set it in stream params. */
        if (0 > stream_ptr->encoder) {
            stream_ptr->encoder = vtspCoder;
        }
    }
    else {
        stream_ptr->encodeType[vtspCoder] = VTSP_CODER_UNAVAIL;
        stream_ptr->encodeTime[vtspCoder] = 0;
    }
}

/*
 * ======== _MC_vtspSetStreamDynamicDecoder() ========
 * This function sets up vTSP stream to support mapping of coder name to coder
 * number used in RTP. Coder name and corresponding number to use is provided
 * by SIP.
 * Use < 0 to clear it.
 *
 * Return Values:
 *
 */
static void _MC_vtspSetStreamDynamicDecoder(
    VTSP_Stream     *stream_ptr,
    MC_Coder        *coder_ptr)
{
    vint vtspCoder;
    /*
     * Map to vTSP coder enum. Set silence compression /dtmf relay in vTSP,
     * if required.
     */
    vtspCoder = _MC_vtspGetCoder(coder_ptr, stream_ptr, 0);
    if (vtspCoder == VTSP_CODER_UNAVAIL) {
        /* Then we don't know this coder, perform nothing */
        return;
    }

    /* Now set it in stream. */
    if (coder_ptr->coderNum >= 0) {
        stream_ptr->decodeType[vtspCoder] = coder_ptr->coderNum;
    }
    else {
        stream_ptr->decodeType[vtspCoder] = VTSP_CODER_UNAVAIL;
    }
}

/*
 * ======== _MC_vtspSetStreamDynamicEncoderVideo() ========
 * This function sets up vTSP stream to support mapping of coder name to coder
 * number used in RTP. Coder name and corresponding number to use is provided
 * by signaling. Coder rate is also set.
 * Use < 0 to clear it.
 *
 * Return Values:
 *
 */
static void _MC_vtspSetStreamDynamicEncoderVideo(
    VTSP_StreamVideo *stream_ptr,
    MC_Coder         *coder_ptr)
{
    vint vtspCoder;
    /*
     * Map to vTSP coder enum.
     */
    vtspCoder = _MC_vtspGetCoderVideo(coder_ptr, stream_ptr);
    if (vtspCoder == VTSP_CODER_UNAVAIL) {
        /* Invalid coder, perform nothing */
        return;
    }

    /* Now set it in stream. */
    if (coder_ptr->coderNum >= 0) {
        stream_ptr->encodeType[vtspCoder] = coder_ptr->coderNum;
        stream_ptr->encodeTime[vtspCoder] = coder_ptr->rate;

        /* Check if we have some extmap parameters. */
        if (0 != coder_ptr->props.h264.extmap.id) {
            stream_ptr->extmapId = coder_ptr->props.h264.extmap.id;
        }
        if (0 != coder_ptr->props.h264.extmap.uri[0]) {
            OSAL_snprintf(stream_ptr->extmapUri, VTSP_EXTMAP_URI_SZ, "%s",
                    coder_ptr->props.h264.extmap.uri);
        }

        /* If this is the default coder, set it in stream params. */
        if (0 > stream_ptr->encoder) {
            stream_ptr->encoder = vtspCoder;
        }
    }
    else {
        stream_ptr->encodeType[vtspCoder] = VTSP_CODER_UNAVAIL;
        stream_ptr->encodeTime[vtspCoder] = 0;
    }
}

/*
 * ======== _MC_vtspSetStreamDynamicDecoderVideo() ========
 * This function sets up vTSP stream to support mapping of coder name to coder
 * number used in RTP. Coder name and corresponding number to use is provided
 * by SIP.
 * Use < 0 to clear it.
 *
 * Return Values:
 *
 */
static void _MC_vtspSetStreamDynamicDecoderVideo(
    VTSP_StreamVideo *stream_ptr,
    MC_Coder         *coder_ptr)
{
    vint vtspCoder;
    /*
     * Map to vTSP coder enum.
     */
    vtspCoder = _MC_vtspGetCoderVideo(coder_ptr, stream_ptr);
    if (vtspCoder == VTSP_CODER_UNAVAIL) {
        /* Then we don't know this coder, perform nothing */
        return;
    }

    /* Now set it in stream. */
    if (coder_ptr->coderNum >= 0) {
        stream_ptr->decodeType[vtspCoder] = coder_ptr->coderNum;
    }
    else {
        stream_ptr->decodeType[vtspCoder] = VTSP_CODER_UNAVAIL;
    }
}

/*
 * ======== _MC_vtspIsWbCoder() ========
 *
 * Determine if encoder type is wideband coder or not
 *
 * RETURN: true or false
 */
OSAL_Boolean _MC_vtspIsWbCoder(
    uvint   encType)
{
    switch (encType) {
        case VTSP_CODER_16K_MU:
        case VTSP_CODER_G722:
        case VTSP_CODER_G722P1_20MS:
        case VTSP_CODER_SILK_20MS_16K:
        case VTSP_CODER_G711P1U:
        case VTSP_CODER_G711P1A:
        case VTSP_CODER_GAMRWB_20MS_OA:
        case VTSP_CODER_GAMRWB_20MS_BE:
            return (OSAL_TRUE);
       default:
            return (OSAL_FALSE);
    }
}

/*
 * ======== _MC_vtspSetSreamTelEvtCoder() ========
 * This function  is used to configure decode/encode DTMF coder type.
 * If MC choose WB audio coder and there is 16K telephone event,
 * to set telephone event as 16K.
 * Otherwirse, set telephone event as 8K.
 *
 * Return Values:
 *
 */
void _MC_vtspSetSreamTelEvtCoder(
    VTSP_Stream    *stream_ptr,
    MC_VtspObj     *vtspObj_ptr)
{
    vint x;

    stream_ptr->encodeType[VTSP_CODER_DTMF] = VTSP_CODER_UNAVAIL;
    stream_ptr->encodeTime[VTSP_CODER_DTMF] = 0;
    stream_ptr->decodeType[VTSP_CODER_DTMF] = VTSP_CODER_UNAVAIL;

    if (OSAL_TRUE == _MC_vtspIsWbCoder(stream_ptr->encoder)) {
        for (x = 0; x < ISI_CODER_NUM; x++) {
            if (!OSAL_strncasecmp(vtspObj_ptr->coders[x].szCoderName,
                    "telephone-event-16k",
                    sizeof(vtspObj_ptr->coders[x].szCoderName))) {
                stream_ptr->encodeType[VTSP_CODER_DTMF] =
                        vtspObj_ptr->coders[x].coderNum;
                stream_ptr->encodeTime[VTSP_CODER_DTMF] =
                        vtspObj_ptr->coders[x].rate;
                stream_ptr->extension |= VTSP_MASK_EXT_DTMFR_16K;
            }
            if (!OSAL_strncasecmp(vtspObj_ptr->decoders[x].szCoderName,
                    "telephone-event-16k",
                    sizeof(vtspObj_ptr->decoders[x].szCoderName))) {
                stream_ptr->decodeType[VTSP_CODER_DTMF] =
                        vtspObj_ptr->decoders[x].coderNum;
            }
        }
    }
    else {
        for (x = 0; x < ISI_CODER_NUM; x++) {
            if (!OSAL_strncasecmp(vtspObj_ptr->coders[x].szCoderName,
                    "telephone-event",
                    sizeof(vtspObj_ptr->coders[x].szCoderName))) {
                stream_ptr->encodeType[VTSP_CODER_DTMF] =
                        vtspObj_ptr->coders[x].coderNum;
                stream_ptr->encodeTime[VTSP_CODER_DTMF] =
                        vtspObj_ptr->coders[x].rate;
                stream_ptr->extension |= VTSP_MASK_EXT_DTMFR_8K;
            }
            if (!OSAL_strncasecmp(vtspObj_ptr->decoders[x].szCoderName,
                    "telephone-event",
                    sizeof(vtspObj_ptr->decoders[x].szCoderName))) {
                stream_ptr->decodeType[VTSP_CODER_DTMF] =
                        vtspObj_ptr->decoders[x].coderNum;
            }
        }
    }
}

/*
 * ======== _MC_vtspInitVtspEngine() ========
 * Initializes and starts the vTSP.
 *
 * Return Values:
 * MC_OK  : Success
 * MC_ERR : Fail
 *
 */
vint _MC_vtspInitVtspEngine(
    MC_VtspObj *vtspObj_ptr)
{
    VTSP_TaskConfig  tcfg;
    int              ret;

    /* vTSP init/start sequence is not refactored to allocate/start/destroy style */
    /* Init and start the vTSP. */
    tcfg.vtspAddStackSize = 0;
    tcfg.rtcpAddStackSize = 0;
    tcfg.rtcpInternalPort = vtspObj_ptr->vtsp.firstRtpPort
            + ((MC_STREAM_NUM * MC_INFC_NUM) << 1) + 1;

    if (VTSP_OK != (ret = VTSP_init((VTSP_Context **)&vtspObj_ptr->vtsp.vtsp_ptr,
            NULL, &tcfg))) {
        OSAL_logMsg("%s: VTSP_init failed. Returned:%d\n", __FUNCTION__, ret);
        return (MC_ERR);
    }

    if (NULL == vtspObj_ptr->vtsp.vtsp_ptr) {
        MC_dbgPrintf("%s: vtsp_ptr is NULL\n", __FUNCTION__);
        VTSP_shutdown();
        return (MC_ERR);
    }
    /*
     * Place Audio codec path into correct initial state.
     * This is also the default state...GSM owns the codec.
     */
    vtspObj_ptr->codecControl.semaphore = 0;

    /* Init the Lip Sync Enable or Disabled flag. */
#ifdef LIP_SYNC_ENABLE
    vtspObj_ptr->vtsp.syncEngine.enabled = 1;
#else
    vtspObj_ptr->vtsp.syncEngine.enabled = 0;
#endif
    /* TODO This is platform dependent decode path latency. Tweak this value. Unit in ms.*/
    vtspObj_ptr->vtsp.syncEngine.aLatency = 30;
    vtspObj_ptr->vtsp.syncEngine.vLatency = 130;

    return (MC_OK);
}

int _MC_vtspConfig(
    void        *cfg_ptr,
    MC_VtspObj  *vtspObj_ptr)
{
    char    *value_ptr;
    char    *ipc_ptr;
    char    *isiIpc_ptr;
    char    *mediaIpc_ptr;
    char    *streamIpc_ptr;
    vint     stream;

    /* Get the names of the ipc interfaces */
    ipc_ptr = NULL;
    isiIpc_ptr = NULL;
    mediaIpc_ptr = NULL;
    streamIpc_ptr = NULL;

    /* Get "this" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_THIS))) {
        ipc_ptr = value_ptr;
    }

    /* Get "isi" */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_INTERFACE,
            NULL, NULL, SETTINGS_PARM_ISI))) {
        isiIpc_ptr = value_ptr;
    }


    if (NULL == ipc_ptr || NULL == isiIpc_ptr ) {
        /* The IPC names are manditory */
        return (MC_ERR);
    }    
    
    /* Get the starting RTP port.  vTSP needs to know the first RTP port */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RTP_PORT))) {
        vtspObj_ptr->vtsp.firstRtpPort = OSAL_atoi(value_ptr);
    }

    /* 
     * Determine if the MC should generate call progress tones when commanded
     * from the ISI call state machine.  If not, then these commands will be 
     * ignored.  In this case, only ISI tone commands stimulated from the ISI 
     * API will be acceptable.  
     */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_TONE_AUTO_CALLPROGRESS))) {
        if (0 == OSAL_strncasecmp(value_ptr, "on", 2)) {
            vtspObj_ptr->vtsp.autoCallProgressTones = 1;
        }
    }

    /* Get RTP/RTCP inactivity time, the unit is sec */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_TIMER_RTP_INACTIVITY))) {
        vtspObj_ptr->inactivityExp = OSAL_atoi(value_ptr);

        if (MC_MIN_RTP_RTCP_INACTIVE_TIMER >
                vtspObj_ptr->inactivityExp) {
            if (0 >= vtspObj_ptr->inactivityExp) {
                /* disable rtp/rtcp inactivity timer */
                vtspObj_ptr->inactivityExp = 0;
            }
            else {
                vtspObj_ptr->inactivityExp =
                        MC_MIN_RTP_RTCP_INACTIVE_TIMER;
            }
        }
        else if (MC_MAX_RTP_RTCP_INACTIVE_TIMER <
                vtspObj_ptr->inactivityExp) {
            vtspObj_ptr->inactivityExp = MC_MAX_RTP_RTCP_INACTIVE_TIMER;
        }
    }
    else {
        /* disable rtp/rtcp inactivity timer */
        vtspObj_ptr->inactivityExp = 0;
    }

    /* Get RTCP Interval time */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_MC,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_AUDIO,
            NULL, NULL, SETTINGS_PARM_RTCP_INTERVAL))) {
        vtspObj_ptr->rtcpInterval = OSAL_atoi(value_ptr);
        if (0 >= vtspObj_ptr->rtcpInterval) {
            vtspObj_ptr->rtcpInterval = 0;
        }
    }
    else {
        vtspObj_ptr->rtcpInterval = 0;
    }

    /* Cache the ISI IPC queue Id in the event object */
    if (0 == (vtspObj_ptr->event.isiEvt = OSAL_msgQCreate(isiIpc_ptr,
            OSAL_MODULE_MC, OSAL_MODULE_ISI, OSAL_DATA_STRUCT_ISIP_Message,
            MC_MAX_QUEUE_DEPTH, sizeof(ISIP_Message), 0))) {
        MC_dbgPrintf("%s: ERROR Q create failed.\n", __FUNCTION__);
        return (OSAL_FAIL);
    }
    
    /* Set stream */
    for (stream = 0; stream < MC_STREAM_NUM; stream++) {
        OSAL_memSet(&vtspObj_ptr->vtsp.audio[stream], 0, sizeof(MC_AudioStream));
        vtspObj_ptr->vtsp.audio[stream].vtspStream.streamId = stream;
    }

    for (stream = 0; stream < MC_STREAM_NUM; stream++) {
        OSAL_memSet(&vtspObj_ptr->vtsp.video[stream], 0, sizeof(MC_VideoStream));
        vtspObj_ptr->vtsp.video[stream].vtspStream.streamId = stream;
    }

    /* VTSP is not refactored. Init VTSP old way */
    if (MC_OK !=  _MC_vtspInitVtspEngine(vtspObj_ptr)) {
        MC_dbgPrintf("%s: Error initializing the MC task\n", __FUNCTION__);
        return (MC_ERR);
    }

    /* 
     * Init the timer event object. 
     */
    vtspObj_ptr->tmr.event.isiEvt = vtspObj_ptr->event.isiEvt;

    /* Construct the ISI event that the timer will send */
    _MC_sysEvtIsiEvt(ISIP_SYSTEM_REASON_START, 
            ISIP_STATUS_TRYING,
            ipc_ptr, mediaIpc_ptr, streamIpc_ptr,
            &vtspObj_ptr->tmr.event.isiMsg);

    /* Launch the timer that will handle registering this app with ISI */
    if (MC_OK != _MC_vtspTimerInit(vtspObj_ptr)) {
        return (MC_ERR);
    }
    
    
    return (MC_OK);
}



/*
 * ======== MC_vtspDupIsiEventQid() ========
 * This function duplicate vtspObj isiEvt queue ID to MC_isi module globals
 *
 * Return Values: N/A
 *
 */
void _MC_vtspDupIsiEventQid(
    MC_IsiObj   *isi_ptr)
{
    isi_ptr->queue.writeIpcId = _MC_globalVtspObj_ptr->event.isiEvt;
    isi_ptr->event.isiEvt     = _MC_globalVtspObj_ptr->event.isiEvt;
}

/*
 * ======== MC_vtspGetObj() ========
 * This function return vtspObj global pointer
 *
 * Return Values: N/A
 *
 */
MC_VtspObj *_MC_vtspGetObjPtr(void)
{
    return _MC_globalVtspObj_ptr;
}

/*
 * ======== _MC_vtspStop() ========
 *
 * Internal routine for stoping the MC_vtsp module task
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _MC_vtspStop(
    MC_VtspObj *vtspObj_ptr)
{
    if (OSAL_SUCCESS != OSAL_taskDelete(vtspObj_ptr->evtTask.tId)) {
        return (OSAL_FAIL);
    }
    return (OSAL_SUCCESS);
}

/*
 * ======== _MC_vtspDeallocate() ========
 *
 * Internal routine for free up the MC_vtsp module resources
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint _MC_vtspDeallocate(
    MC_VtspObj *vtspObj_ptr)
{
    OSAL_msgQDelete(vtspObj_ptr->event.isiEvt);
    return (OSAL_SUCCESS);
}

/*
 * ======== _MC_vtspMapRtcpFbMask() ========
 * This function is used to map ISI values that represent a stream's
 * RTCP feedback mask to an enumerated value that the VTSP module will understand.
 *
 * Return Values:
 *  outMask : a value representing the VTSP rtcp feedback mask.
 *
 */
static vint _MC_vtspMapRtcpFbMask(
    uint32 inMask)
{
    uint32 outMask = (VTSP_MASK_RTCP_SR | VTSP_MASK_RTCP_BYE);

    if (1 == (inMask && (ISIP_MASK_RTCP_FB_NACK))) {
        outMask |= VTSP_MASK_RTCP_FB_NACK;
    }
    if (1 == (inMask && (ISIP_MASK_RTCP_FB_PLI))) {
        outMask |= VTSP_MASK_RTCP_FB_PLI;
    }
    if (1 == (inMask && (ISIP_MASK_RTCP_FB_FIR))) {
        outMask |= VTSP_MASK_RTCP_FB_FIR;
    }
    if (1 == (inMask && (ISIP_MASK_RTCP_FB_TMMBR))) {
        outMask |= VTSP_MASK_RTCP_FB_TMMBR;
    }
    if (1 == (inMask && (ISIP_MASK_RTCP_FB_TMMBN))) {
        outMask |= VTSP_MASK_RTCP_FB_TMMBN;
    }

    return (outMask);
}

/*
 * ======== _MC_vtspSetVideoRtcpFeedbackParams() ========
 * This function sets up feedback mask of Video RTCP.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
static vint _MC_vtspSetVideoRtcpFeedbackParams(
    vint   infc,
    uint32 mask,
    vint   streamId)
{
    VTSP_RtcpTemplate   rtcpConfig;
    rtcpConfig.control  = VTSP_TEMPL_CONTROL_RTCP_MASK;
    rtcpConfig.streamId = streamId;
    rtcpConfig.mask = _MC_vtspMapRtcpFbMask(mask);

    if (VTSP_OK != VTSP_rtcpFeedbackMaskVideo(infc, &rtcpConfig)) {
        return (MC_ERR);
    }
    return (MC_OK);
}

/*
 * ======== MC_isiMediaCmd() ========
 * This function processes a ISI message and calls appropriate audio functions.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
void MC_isiMediaCmd(
    MC_VtspObj   *vtspObj_ptr,
    ISIP_Message *cmd_ptr)
{
    vint                infc;
    vint                streamId;
    vint                stream;
    uint32              mask;
    ISIP_Stream        *s_ptr;
    VTSP_Stream        *stream_ptr;
    VTSP_StreamVideo   *streamv_ptr;
    ISIP_Media         *media_ptr;
    int                 x;
    VTSP_AecTemplate    aecConfig;
    MC_StreamOp         streamOp;
    OSAL_NetAddress     tmpLclAddr;
    OSAL_NetAddress     tmpRmtAddr;
    VTSP_RtcpTemplate   rtcpConfig;
    VTSP_CnTemplate     cnConfig;

    /* The interface number 'infc' is always zero for the handset. */
    infc  = 0;

    media_ptr = &cmd_ptr->msg.media;
    s_ptr = &media_ptr->media.stream;

    /* MC_dbgPrintf("%s: Reason:%d\n", __FUNCTION__, media_ptr->reason); */

    /* Get reason, and perform related vTSP function. */
    switch (media_ptr->reason) {
        case ISIP_MEDIA_REASON_PKT_SEND:
            _MC_sendPacketToVtsp(vtspObj_ptr, cmd_ptr);
            break;
#ifndef MC_NO_RING /* Then disable all local ring tone generation */
        case ISIP_MEDIA_REASON_RINGSTART:
            MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_TRUE);
            if (MC_OK != MC_ringControl(infc, 
                    media_ptr->media.ring.ringtemplate)) {
                MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
                MC_dbgPrintf("%s Ring start failed\n", __FUNCTION__);
            }
            break;

        case ISIP_MEDIA_REASON_RINGSTOP:
            /*
             * Note that the codec will be commanded when VTSP reports that
             * the tone is complete, halted or has timed out.
             */
            if (MC_OK != MC_ringControl(infc, -1)) {
                /*
                 * We failed to command VTSP so let's flip the codec ourselves
                 */
                MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
                MC_dbgPrintf("%s: Ring stop failed\n", __FUNCTION__);
            }
            break;
#endif
#ifndef MC_NO_TONE /* Then disable all local tone generation */
        case ISIP_MEDIA_REASON_TONESTART:
            if (0 != vtspObj_ptr->vtsp.autoCallProgressTones) {
                /*
                 * Then 'auto' call progress is tones is enabled, so we
                 * will honor the request from ISI's call state machine.
                 */
                MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_TRUE);
                if (MC_OK != MC_toneControl(infc, OSAL_TRUE,
                        media_ptr->media.tone.toneType,
                        media_ptr->media.tone.make1)) {
                    MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
                    MC_dbgPrintf("%s: Tone start failed\n", __FUNCTION__);
                }
            }
            break;

        case ISIP_MEDIA_REASON_TONESTOP:
            if (0 != vtspObj_ptr->vtsp.autoCallProgressTones) {
                /*
                 * Then 'auto' call progress is tones is enabled, so we
                 * will honor the request from ISI's call state machine.
                 */

                /*
                 * Note that the codec will be commanded when VTSP reports that
                 * the tone is complete, halted or has timed out.
                 */
                if (MC_OK != MC_toneControl(infc, OSAL_FALSE,
                        media_ptr->media.tone.toneType,
                        media_ptr->media.tone.make1)) {
                    /*
                     * We failed to command VTSP so let's flip the codec ourselves
                     */
                    MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
                    MC_dbgPrintf("%s: Tone stop failed\n", __FUNCTION__);
                }
            }
            break;

        case ISIP_MEDIA_REASON_TONESTART_CMD:
            MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_TRUE);
            if (MC_OK != MC_toneControl(infc, OSAL_TRUE, 
                    media_ptr->media.tone.toneType,
                    media_ptr->media.tone.make1)) {
                MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
                MC_dbgPrintf("%s: Tone command start failed\n", __FUNCTION__);
            }
            break;

        case ISIP_MEDIA_REASON_TONESTOP_CMD:
            MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
            if (MC_OK != MC_toneControl(infc, OSAL_FALSE, 
                    media_ptr->media.tone.toneType,
                    media_ptr->media.tone.make1)) {
                MC_dbgPrintf("%s: Tone command stop failed\n", __FUNCTION__);
            }
            break;
#endif
        case ISIP_MEDIA_REASON_STREAMMODIFY:
        case ISIP_MEDIA_REASON_STREAMSTART:
            /*
             * Parse coders.
             */
            for (x = 0; x < ISI_CODER_NUM; x++) {
                vtspObj_ptr->coders[x].szCoderName[0] = 0;
                vtspObj_ptr->decoders[x].szCoderName[0] = 0;
                if (s_ptr->coders[x].szCoderName[0] != 0) {
                    OSAL_memSet(&vtspObj_ptr->coders[x], 0,
                            sizeof(vtspObj_ptr->coders[x]));
                    OSAL_memSet(&vtspObj_ptr->decoders[x], 0,
                            sizeof(vtspObj_ptr->decoders[x]));
                    OSAL_snprintf(vtspObj_ptr->coders[x].szCoderName, 
                            sizeof(vtspObj_ptr->coders[x].szCoderName),
                            "%s", s_ptr->coders[x].szCoderName);
                    OSAL_snprintf(vtspObj_ptr->decoders[x].szCoderName, 
                            sizeof(vtspObj_ptr->decoders[x].szCoderName),
                            "%s", s_ptr->coders[x].szCoderName);

                    MC_parseCoder(s_ptr->coders[x].szCoderName,
                            s_ptr->coders[x].description, &vtspObj_ptr->coders[x],
                            &vtspObj_ptr->decoders[x]);

                    if (ISI_SESSION_TYPE_AUDIO == s_ptr->coders[x].relates) {
                        vtspObj_ptr->coders[x].relates =  MC_MEDIA_TYPE_AUDIO;
                        vtspObj_ptr->decoders[x].relates =  MC_MEDIA_TYPE_AUDIO;
                    }
                    if (ISI_SESSION_TYPE_VIDEO == s_ptr->coders[x].relates) {
                        vtspObj_ptr->coders[x].relates =  MC_MEDIA_TYPE_VIDEO;
                        vtspObj_ptr->decoders[x].relates =  MC_MEDIA_TYPE_VIDEO;
                    }
                }
            }

            /*
             * Check a valid stream is present.
             */
            if (!((ISI_SESSION_TYPE_AUDIO & s_ptr->type) || (ISI_SESSION_TYPE_VIDEO & s_ptr->type))) {
                break;
            }

            streamId = s_ptr->id;
            if (((streamId < 0) || (streamId >= MC_STREAM_NUM))) {
                break;
            }
            /* Cache stream info, the call ID, streamID and protocol type */
            _MC_vtspSetStreamInfo(vtspObj_ptr, cmd_ptr->id, streamId,
                    cmd_ptr->protocol);

            if (ISI_SESSION_TYPE_AUDIO & s_ptr->type) {
                /* Cache service id for stream */
                vtspObj_ptr->vtsp.audio[streamId].serviceId = media_ptr->serviceId;

                stream_ptr = &vtspObj_ptr->vtsp.audio[streamId].vtspStream;

                /* Copy and covert ip address and port to network byte order */
                OSAL_netAddrPortHton(&tmpLclAddr, &s_ptr->audio.lclAddr);
                OSAL_netAddrPortHton(&tmpRmtAddr, &s_ptr->audio.rmtAddr);
                /* Init stream. */
                _MC_vtspInitStream(stream_ptr,
                        &tmpLclAddr,
                        OSAL_netHtons(s_ptr->audio.lclCntlPort),
                        &tmpRmtAddr,
                        OSAL_netHtons(s_ptr->audio.rmtCntlPort),
                        s_ptr->audio.securityKeys.type,
                        s_ptr->audio.securityKeys.lclKey,
                        s_ptr->audio.securityKeys.rmtKey);
                
                /*
                 * set coders.  Init the 'encoder' to invalid.
                 * _MC_vtspSetStreamDynamicEncoder will set it.
                 */
                stream_ptr->encoder = -1;
                OSAL_memSet(stream_ptr->encodeType, 0,
                        sizeof(stream_ptr->encodeType));
                OSAL_memSet(stream_ptr->decodeType, 0,
                        sizeof(stream_ptr->decodeType));
                for (x = 0 ; x < ISI_CODER_NUM; x++) {
                    _MC_vtspSetStreamDynamicEncoder(stream_ptr,
                            &vtspObj_ptr->coders[x]);

                    _MC_vtspSetStreamDynamicDecoder(stream_ptr,
                            &vtspObj_ptr->decoders[x]);
                }
                /*
                 * If the encoder is stil invalid, then just go for the
                 * zero indexed coder
                 */
                if (0 > stream_ptr->encoder) {
                    stream_ptr->encoder = 0;
                }
                else {
                    /* Set telephone-event 8K or 16K */
                    _MC_vtspSetSreamTelEvtCoder(stream_ptr, vtspObj_ptr);
                }

                /*
                 * These bits dont make sense if coders are disabled.
                 */
                if (VTSP_CODER_UNAVAIL == 
                        stream_ptr->encodeType[VTSP_CODER_DTMF]) {
                    stream_ptr->dtmfRelay = 0;
                }

                /* Set RTCP CNAME */
                if (MC_OK != _MC_vtspSetRtcpCname(infc, 
                        s_ptr->audio.rtcpCname)) {
                    MC_dbgPrintf("%s: Failed to set RTCP name\n", __FUNCTION__);
                }

                /* Now perform stream operation. */
                if (vtspObj_ptr->vtsp.audio[streamId].isStarted) {
                    /* Then let's modify the stream */
                    streamOp = MC_STREAM_OP_MODIFY;
                }
                else {
                    /* Then the stream should be be started */
                    streamOp = MC_STREAM_OP_START;
                    vtspObj_ptr->vtsp.audio[streamId].isStarted = 1;
                }
#ifdef MC_DEBUG
                _MC_vtspPrintStream(infc, streamOp, stream_ptr);
#endif
                if (MC_OK != _MC_vtspModifyStream(infc,
                        streamOp, s_ptr->audioDirection, stream_ptr,
                        &vtspObj_ptr->codecControl)) {
                    MC_dbgPrintf("%s: Failed to modify a stream\n", 
                            __FUNCTION__);
                }

                /* Set up RTCP interval */
                if (0 != vtspObj_ptr->rtcpInterval) {
                    rtcpConfig.control  = VTSP_TEMPL_CONTROL_RTCP_INTERVAL;
                    rtcpConfig.streamId = streamId;
                    /* the ve unit is 10 ms */
                    rtcpConfig.interval = vtspObj_ptr->rtcpInterval * 100;
                    if (VTSP_OK != VTSP_config(VTSP_TEMPL_CODE_RTCP, infc,
                            &rtcpConfig)) {
                        MC_dbgPrintf("%s: Failed to set up rtcp interval\n",
                                __FUNCTION__);
                    }
                }

                /* Reset RTP/RTCP inactivity timer of the stream */
                vtspObj_ptr->vtsp.audio[streamId].rtpInactiveTmr  = 0;
                vtspObj_ptr->vtsp.audio[streamId].rtcpInactiveTmr = 0;
                vtspObj_ptr->vtsp.audio[streamId].recvPktCount    = 0;
            }
            else {
                /* Stop audio if it's already started */
                if (vtspObj_ptr->vtsp.audio[streamId].isStarted) {
                    stream_ptr = &vtspObj_ptr->vtsp.audio[streamId].vtspStream;
                    /* Clean the stream object for use next time */
#ifdef MC_DEBUG
                    _MC_vtspPrintStream(infc, MC_STREAM_OP_END, stream_ptr);
#endif
                    if (MC_OK != _MC_vtspModifyStream(infc, MC_STREAM_OP_END, 
                            ISI_SESSION_DIR_INACTIVE,
                            stream_ptr, &vtspObj_ptr->codecControl)) {
                        MC_dbgPrintf("%s: Failed to stop a stream\n", __FUNCTION__);
                    }
                    OSAL_memSet(&vtspObj_ptr->vtsp.audio[streamId], 0, sizeof(MC_AudioStream));
                    /* Reinstate the stream ID */
                    stream_ptr->streamId = streamId;
                }
            }

            if (ISI_SESSION_TYPE_VIDEO & s_ptr->type) {
                /* Cache service id for stream */
                vtspObj_ptr->vtsp.video[streamId].serviceId = media_ptr->serviceId;

                streamv_ptr = &vtspObj_ptr->vtsp.video[streamId].vtspStream;

                /* Copy and covert ip address and port to network byte order */
                OSAL_netAddrPortHton(&tmpLclAddr, &s_ptr->video.lclAddr);
                OSAL_netAddrPortHton(&tmpRmtAddr, &s_ptr->video.rmtAddr);

                /* Init stream. */
                _MC_vtspInitStreamVideo(streamv_ptr,
                        &tmpLclAddr,
                        OSAL_netHtons(s_ptr->video.lclCntlPort),
                        &tmpRmtAddr,
                        OSAL_netHtons(s_ptr->video.rmtCntlPort),
                        s_ptr->video.securityKeys.type,
                        s_ptr->video.securityKeys.lclKey,
                        s_ptr->video.securityKeys.rmtKey);

                /* Set up RTCP Feedback params (mask) */
                if (0 != s_ptr->video.rtcpFbMask) {
                    if (MC_OK != _MC_vtspSetVideoRtcpFeedbackParams(VTSP_INFC_VIDEO,
                            s_ptr->video.rtcpFbMask, streamId)) {
                        MC_dbgPrintf("%s: Failed to set video RTCP feedback mask\n", __FUNCTION__);
                    }
                }

                /* Set up Local and Remote Video RTP AS bandwidth parameter. */
                if (s_ptr->video.lclVideoAsBwKbps > 0) {
                    streamv_ptr->localVideoAsBwKbps = s_ptr->video.lclVideoAsBwKbps;
                }
                if (s_ptr->video.rmtVideoAsBwKbps > 0) {
                    streamv_ptr->remoteVideoAsBwKbps = s_ptr->video.rmtVideoAsBwKbps;
                }
                /*
                 * set coders.  Init the 'encoder' to invalid.
                 * _MC_vtspSetStreamDynamicEncoderVideo will set it.
                 */
                streamv_ptr->encoder = -1;
                for (x = 0 ; x < ISI_CODER_NUM; x++) {
                    
                    _MC_vtspSetStreamDynamicEncoderVideo(streamv_ptr,
                            &vtspObj_ptr->coders[x]);
                    
                    _MC_vtspSetStreamDynamicDecoderVideo(streamv_ptr,
                            &vtspObj_ptr->decoders[x]);
                }
                /*
                 * If the encoder is stil invalid, then just go for the
                 * zero indexed coder
                 */
                if (0 > streamv_ptr->encoder) streamv_ptr->encoder = 0;

                /* Set RTCP CNAME */
                if (MC_OK != _MC_vtspSetRtcpCnameVideo(VTSP_INFC_VIDEO,
                        s_ptr->video.rtcpCname)) {
                    MC_dbgPrintf("%s: Failed to set video RTCP name\n", __FUNCTION__);
                }

                /* Now perform stream operation. */
                if (vtspObj_ptr->vtsp.video[streamId].isStarted) {
                    /* Then let's modify the stream */
                    streamOp = MC_STREAM_OP_MODIFY;
                }
                else {
                    /* Then the stream should be be started */
                    streamOp = MC_STREAM_OP_START;
                    vtspObj_ptr->vtsp.video[streamId].isStarted = 1;
                }

#ifdef MC_DEBUG
                _MC_vtspPrintStreamVideo(VTSP_INFC_VIDEO, streamOp, streamv_ptr);
#endif
                if (MC_OK != _MC_vtspModifyStreamVideo(VTSP_INFC_VIDEO,
                         streamOp, s_ptr->videoDirection, streamv_ptr)) {
                    MC_dbgPrintf("%s: Failed to modify a video stream\n", __FUNCTION__);
                }
            }
            else {
                /* Stop the video if it's already started */
                if (vtspObj_ptr->vtsp.video[streamId].isStarted) {
                   streamv_ptr = &vtspObj_ptr->vtsp.video[streamId].vtspStream;
                   /* Clean the stream object for use next time */
#ifdef MC_DEBUG
                   _MC_vtspPrintStreamVideo(VTSP_INFC_VIDEO, MC_STREAM_OP_END, streamv_ptr);
#endif
                    if (MC_OK != _MC_vtspModifyStreamVideo(VTSP_INFC_VIDEO, MC_STREAM_OP_END, 
                            ISI_SESSION_DIR_INACTIVE,
                            streamv_ptr)) {
                        MC_dbgPrintf("%s: Failed to stop a video stream\n", __FUNCTION__);
                    }
                    OSAL_memSet(&vtspObj_ptr->vtsp.video[streamId], 0, sizeof(MC_VideoStream));
                    /* Reinstate the stream ID */
                    streamv_ptr->streamId = streamId;
                }
            }
            break;

        case ISIP_MEDIA_REASON_STREAMSTOP:
            streamId = s_ptr->id;
            if (((streamId < 0) || (streamId >= MC_STREAM_NUM))) {
                break;
            }
            if (ISI_SESSION_TYPE_AUDIO & s_ptr->type) {
                stream_ptr = &vtspObj_ptr->vtsp.audio[streamId].vtspStream;
                /* Clean the stream object for use next time */

    #ifdef MC_DEBUG
                _MC_vtspPrintStream(infc, MC_STREAM_OP_END, stream_ptr);
    #endif
                if (MC_OK != _MC_vtspModifyStream(infc, MC_STREAM_OP_END, 
                        ISI_SESSION_DIR_INACTIVE,
                        stream_ptr, &vtspObj_ptr->codecControl)) {
                    MC_dbgPrintf("%s: Failed to stop a stream\n", __FUNCTION__);
                }
                OSAL_memSet(&vtspObj_ptr->vtsp.audio[streamId], 0, sizeof(MC_AudioStream));
                /* Reinstate the stream ID */
                stream_ptr->streamId = streamId;
            }
            if (ISI_SESSION_TYPE_VIDEO & s_ptr->type) {
                /*
                 * Video engine control.
                 */
                streamv_ptr = &vtspObj_ptr->vtsp.video[streamId].vtspStream;
                /* Clean the stream object for use next time */

    #ifdef MC_DEBUG
                _MC_vtspPrintStreamVideo(VTSP_INFC_VIDEO, MC_STREAM_OP_END, streamv_ptr);
    #endif
                if (MC_OK != _MC_vtspModifyStreamVideo(VTSP_INFC_VIDEO, MC_STREAM_OP_END, 
                        ISI_SESSION_DIR_INACTIVE,
                        streamv_ptr)) {
                    MC_dbgPrintf("%s: Failed to stop a video stream\n", __FUNCTION__);
                }
                OSAL_memSet(&vtspObj_ptr->vtsp.video[streamId], 0, sizeof(MC_VideoStream));
                /* Reinstate the stream ID */
                streamv_ptr->streamId = streamId;

            }
            /* Disable the flag */
            vtspObj_ptr->vtsp.streamInfo[streamId].rtpCheckingDisable = 0;
            break;

        case ISIP_MEDIA_REASON_CONFSTART:
        case ISIP_MEDIA_REASON_CONFSTOP:
            if (ISI_SESSION_TYPE_AUDIO & s_ptr->type) {
                /* Setup conf bits for VTSP.  Do not exceed MS or ISI stream limits. */
                for (stream = 0; stream < MC_STREAM_NUM &&
                        stream < ISI_CONF_USERS_NUM; stream++) {
                    streamId = media_ptr->media.conf.aStreamId[stream];
                    stream_ptr = &vtspObj_ptr->vtsp.audio[streamId].vtspStream;
                    mask = media_ptr->media.conf.aConfMask[stream];

                    if (MC_OK != _MC_vtspModifyStreamConfMask(infc, stream_ptr,
                            streamId, mask)) {
                        MC_dbgPrintf("%s: Conf mask set failed\n", __FUNCTION__);
                    }
                }
            }
            if (ISI_SESSION_TYPE_VIDEO & s_ptr->type) {
                for (stream = 0; stream < MC_STREAM_NUM &&
                        stream < ISI_CONF_USERS_NUM; stream++) {
                    streamId = media_ptr->media.conf.aStreamId[stream];
                    streamv_ptr = &vtspObj_ptr->vtsp.video[streamId].vtspStream;
                    mask = media_ptr->media.conf.aConfMask[stream];

                    if (MC_OK != _MC_vtspModifyStreamVideoConfMask(infc, streamv_ptr,
                            streamId, mask)) {
                        MC_dbgPrintf("%s: Conf mask set failed\n", __FUNCTION__);
                    }
                }
            }
            break;

        case ISIP_MEDIA_REASON_SPEAKER:
           MC_dbgPrintf("%s: ISIP_MEDIA_REASON_SPEAKER on=%d\n", __FUNCTION__,
                   media_ptr->media.speakerOn);            
            if (0 != media_ptr->media.speakerOn) {
                aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSFREE_MODE;
            }
            else {
                aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HANDSET_MODE;
            }
            VTSP_config(VTSP_TEMPL_CODE_AEC, 0, &aecConfig);
            break;
        case ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_DISABLE:
            streamId = s_ptr->id;
            if (((streamId < 0) || (streamId >= MC_STREAM_NUM))) {
                break;
            }
            /* This stream do not care RTP inactive */
            vtspObj_ptr->vtsp.streamInfo[streamId].rtpCheckingDisable = 1;
            break;
        case ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_ENABLE:
            streamId = s_ptr->id;
            if (((streamId < 0) || (streamId >= MC_STREAM_NUM))) {
                break;
            }
            /* This stream do not care RTP inactive */
            vtspObj_ptr->vtsp.streamInfo[streamId].rtpCheckingDisable = 0;
            break;
        case ISIP_MEDIA_REASON_AEC_ENABLE:
            if (0 != media_ptr->media.aecOn) {
                aecConfig.control = VTSP_TEMPL_CONTROL_AEC_HALF_DUPLEX;
            }
            else {
                aecConfig.control = VTSP_TEMPL_CONTROL_AEC_BYPASS;
            }
            VTSP_config(VTSP_TEMPL_CODE_AEC, infc, &aecConfig);
            break;
        case ISIP_MEDIA_REASON_GAIN_CONTROL:
            VTSP_infcControlGain(infc, media_ptr->media.gainCtrl.txGain,
                    media_ptr->media.gainCtrl.rxGain);
            break;
        case ISIP_MEDIA_REASON_CN_GAIN_CONTROL:
            cnConfig.control = VTSP_TEMPL_CONTROL_CN_POWER_ATTEN;
            cnConfig.cnPwrAttenDb = media_ptr->media.gainCtrl.txGain;
            VTSP_config(VTSP_TEMPL_CODE_CN, infc, &cnConfig);
            break;
        default:
            break;
    }
    return;
}

/*
 * ======== MC_vtspAllocate() ========
 *
 * Public routine for allocating the MC_vtsp module resource
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_vtspAllocate(void)
{
    void            *cfg_ptr;

    /* Init the global object that's used in MC_vtsp module */
    _MC_globalVtspObj_ptr = OSAL_memCalloc(1, sizeof(MC_VtspObj), 0);

    cfg_ptr = SETTINGS_cfgMemAlloc(SETTINGS_TYPE_MC);
    if (SETTINGS_RETURN_OK != SETTINGS_getContainer(SETTINGS_TYPE_MC,
            NULL, cfg_ptr)) {
        SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
        OSAL_logMsg("%s:%d ERROR opening settings\n",
                __FUNCTION__, __LINE__);
        return (OSAL_FAIL);
    }
    /*
     * config MC_vtsp
     */
    _MC_vtspConfig(cfg_ptr, _MC_globalVtspObj_ptr);

    SETTINGS_memFreeDoc(SETTINGS_TYPE_MC, cfg_ptr);
    
    return (OSAL_SUCCESS);
}

/*
 * ======== MC_vtspStart() ========
 *
 * Public routine for starting the MC_vtsp module tasks/actions
 *
 * Returns:
 *      OSAL_SUCCESS: function exits normally.
 *      OSAL_FAIL: in case of error
 */
vint MC_vtspStart(void)
{
    MC_VtspObj  *vtspObj_ptr = _MC_globalVtspObj_ptr;
    MC_TaskObj  *task_ptr;
    
#if defined(PROVIDER_LGUPLUS_PREPARE)
    VTSP_AecTemplate aecConfig;
#endif

    /* init hw to disabled first */
    MC_vtspHwControl(&vtspObj_ptr->codecControl, OSAL_FALSE);
        
    /* Start MC_vtsp getEvent task. */
    task_ptr = &vtspObj_ptr->evtTask;
    task_ptr->tId      = 0;
    task_ptr->stackSz  = MC_TASK_STACK_BYTES;
    task_ptr->pri      = OSAL_TASK_PRIO_NRT;
    task_ptr->func_ptr = _MC_eventTask;
    OSAL_snprintf(task_ptr->name, sizeof(task_ptr->name), "%s",
            MC_VTSP_EVT_TASK_NAME);
    task_ptr->arg_ptr  = (void *)vtspObj_ptr;
    if (0 == (task_ptr->tId = OSAL_taskCreate(task_ptr->name,
            (OSAL_TaskPrio)task_ptr->pri,
            task_ptr->stackSz,
            (OSAL_TaskPtr)task_ptr->func_ptr,
            (void *)task_ptr->arg_ptr))) {
        MC_dbgPrintf("%s: Failed to launch %s task.\n", __FUNCTION__,
                task_ptr->name);
        return (OSAL_FAIL);
    }
    
    /* start VTSP engine */
    if (VTSP_OK != VTSP_start()) {
        MC_dbgPrintf("%s %d: VTSP_start failed. Returned:\n", __FUNCTION__, __LINE__);
        _MC_vtspStop(vtspObj_ptr);
        VTSP_shutdown();
        return (OSAL_FAIL);
    }

#if defined(PROVIDER_LGUPLUS_PREPARE)
    /* AES default value is bypass */
    aecConfig.control = VTSP_TEMPL_CONTROL_AEC_BYPASS;
    VTSP_config(VTSP_TEMPL_CODE_AEC, 0, &aecConfig);
#endif

#ifdef MC_PREPARE
    /* Set up the default tone templates used */
    MC_toneInit();
#endif

    return (OSAL_SUCCESS);
}

/*
 * ======== MC_vtspDestroy() ========
 * Stops the vTSP.
 *
 * Return Values:
 * MC_OK : Success.
 * MC_ERR: Failed.
 *
 */
vint MC_vtspDestroy(void)
{
    MC_VtspObj  *vtspObj_ptr = _MC_globalVtspObj_ptr;
    vint ret = MC_OK;

    if (OSAL_SUCCESS != _MC_vtspStop(vtspObj_ptr)) {
        ret = MC_ERR;
    }

    if (VTSP_OK != VTSP_shutdown()) {
        ret =  MC_ERR;
    }

    _MC_vtspTimerDestroy(vtspObj_ptr);
    _MC_vtspDeallocate(vtspObj_ptr);

    return (ret);
}

/* deprecated */
int MC_vtspInit(void *cfg_ptr)
{
    /* Init the global object that's used in MC_vtsp module */
    _MC_globalVtspObj_ptr = OSAL_memCalloc(1, sizeof(MC_VtspObj), 0);

    if (MC_OK != _MC_vtspConfig(cfg_ptr, _MC_globalVtspObj_ptr)) {
        OSAL_logMsg("Failed to Init the MC Application\n");
        return (-1);
    }
    
    /* Init the task used to read and process commands from ISI */
    if (OSAL_FAIL == MC_vtspStart()) {
        return (MC_ERR);
    }
    
    return (0);
}
