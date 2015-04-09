/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30168 $ $Date: 2014-12-02 16:40:06 +0800 (Tue, 02 Dec 2014) $
 *
 */

#include "_isi_port.h"
#include "isi.h"
#include "isip.h"
#include "_isi_db.h"
#include "_isi_dbg.h"

#ifdef ISI_DEBUG_LOG

static char *_ISIG_CallStateStrings[ISI_CALL_STATE_INVALID + 1] = {
    "ISI_CALL_STATE_INITIATING",
    "ISI_CALL_STATE_INCOMING",
    "ISI_CALL_STATE_ACTIVE",
    "ISI_CALL_STATE_ONHOLD",
    "ISI_CALL_STATE_XFERING",
    "ISI_CALL_STATE_XFEREE",
    "ISI_CALL_STATE_XFERWAIT",
    "ISI_CALL_STATE_INCOMING_RSRC_NOT_READY",
    "ISI_CALL_STATE_INVALID",
};

static char *_ISIG_CallReasonStrings[ISIP_CALL_REASON_LAST + 1] = {
    "ISIP_CALL_REASON_INVALID",
    "ISIP_CALL_REASON_INITIATE",
    "ISIP_CALL_REASON_TERMINATE",
    "ISIP_CALL_REASON_ACCEPT",
    "ISIP_CALL_REASON_ACK",
    "ISIP_CALL_REASON_REJECT",
    "ISIP_CALL_REASON_TRYING",
    "ISIP_CALL_REASON_ERROR",
    "ISIP_CALL_REASON_FAILED",
    "ISIP_CALL_REASON_CREDENTIALS",
    "ISIP_CALL_REASON_HOLD",
    "ISIP_CALL_REASON_RESUME",
    "ISIP_CALL_REASON_FORWARD",
    "ISIP_CALL_REASON_TRANSFER_PROGRESS",
    "ISIP_CALL_REASON_TRANSFER_ATTEMPT",
    "ISIP_CALL_REASON_TRANSFER_BLIND",
    "ISIP_CALL_REASON_TRANSFER_COMPLETED",
    "ISIP_CALL_REASON_TRANSFER_FAILED",
    "ISIP_CALL_REASON_TRANSFER_ATTENDED",
    "ISIP_CALL_REASON_TRANSFER_CONSULT",
    "ISIP_CALL_REASON_MODIFY",
    "ISIP_CALL_REASON_CANCEL_MODIFY",
    "ISIP_CALL_REASON_ACCEPT_MODIFY",
    "ISIP_CALL_REASON_REJECT_MODIFY",
    "ISIP_CALL_REASON_VDX",
    "ISIP_CALL_REASON_HANDOFF",
    "ISIP_CALL_REASON_BEING_FORWARDED",
    "ISIP_CALL_REASON_HANDOFF_SUCCESS",
    "ISIP_CALL_REASON_HANDOFF_FAILED",
    "ISIP_CALL_REASON_ACCEPT_ACK",
    "ISIP_CALL_REASON_CONF_KICK",
    "ISIP_CALL_REASON_RTP_INACTIVE_TMR_DISABLE",
    "ISIP_CALL_REASON_RTP_INACTIVE_TMR_ENABLE",
    "ISIP_CALL_REASON_TRANSFER_CONF",    
    "ISIP_CALL_REASON_INITIATE_CONF",     
    "ISIP_CALL_REASON_VIDEO_REQUEST_KEY",
    "ISIP_CALL_REASON_EARLY_MEDIA",
    "ISIP_CALL_REASON_LAST"
};

static char *_ISIG_SystemReasonStrings[ISIP_SYSTEM_REASON_LAST + 1] = {
    "ISIP_SYSTEM_REASON_START",
    "ISIP_SYSTEM_REASON_SHUTDOWN",
    "ISIP_SYSTEM_REASON_NOT_SUPPORTED",
    "ISIP_SYSTEM_REASON_LAST"
};

static char *_ISIG_ServiceReasonStrings[ISIP_SERVICE_REASON_LAST + 1] = {
    "ISIP_SERVICE_REASON_INVALID",
    "ISIP_SERVICE_REASON_CREATE",
    "ISIP_SERVICE_REASON_DESTROY",
    "ISIP_SERVICE_REASON_ACTIVATE",
    "ISIP_SERVICE_REASON_DEACTIVATE",
    "ISIP_SERVICE_REASON_HANDOFF",
    "ISIP_SERVICE_REASON_BLOCKUSER",
    "ISIP_SERVICE_REASON_IDENTITY",
    "ISIP_SERVICE_REASON_SERVER",
    "ISIP_SERVICE_REASON_CODERS",
    "ISIP_SERVICE_REASON_AUTH",
    "ISIP_SERVICE_REASON_URI",
    "ISIP_SERVICE_REASON_BSID",
    "ISIP_SERVICE_REASON_NET",
    "ISIP_SERVICE_REASON_FILE",
    "ISIP_SERVICE_REASON_AUTH_AKA_CHALLENGE",
    "ISIP_SERVICE_REASON_AUTH_AKA_RESPONSE",
    "ISIP_SERVICE_REASON_CAPABILITIES",
    "ISIP_SERVICE_REASON_PORT",
    "ISIP_SERVICE_REASON_EMERGENCY",
    "ISIP_SERVICE_REASON_IMEI_URI",
    "ISIP_SERVICE_REASON_IPSEC",
    "ISIP_SERVICE_REASON_INSTANCEID",
    "ISIP_SERVICE_REASON_SET_PROVISIONING_DATA",
    "ISIP_SERVICE_REASON_LAST"
};

static char *_ISIG_AudioReasonStrings[ISIP_MEDIA_REASON_LAST + 1] = {
    "ISIP_MEDIA_REASON_INVALID",
    "ISIP_MEDIA_REASON_RINGSTART",
    "ISIP_MEDIA_REASON_RINGSTOP",
    "ISIP_MEDIA_REASON_TONESTART",
    "ISIP_MEDIA_REASON_TONESTOP",
    "ISIP_MEDIA_REASON_TONESTART_CMD",
    "ISIP_MEDIA_REASON_TONESTOP_CMD",
    "ISIP_MEDIA_REASON_STREAMSTART",
    "ISIP_MEDIA_REASON_STREAMSTOP",
    "ISIP_MEDIA_REASON_STREAMMODIFY",
    "ISIP_MEDIA_REASON_CONFSTART",
    "ISIP_MEDIA_REASON_CONFSTOP",
    "ISIP_MEDIA_REASON_TIMER",
    "ISIP_MEDIA_REASON_PKT_RECV",
    "ISIP_MEDIA_REASON_PKT_SEND",
    "ISIP_MEDIA_REASON_SPEAKER",
    "ISIP_MEDIA_REASON_RTP_RTCP_INACTIVE",
    "ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_DISABLE",
    "ISIP_MEDIA_REASON_RTP_INACTIVE_TMR_ENABLE",
    "ISIP_MEDIA_REASON_AEC_ENABLE",
    "ISIP_MEDIA_REASON_GAIN_CONTROL",
    "ISIP_MEDIA_REASON_CN_GAIN_CONTROL",
    "ISIP_MEDIA_REASON_LAST",
};

static char *_ISIG_ChatStateStrings[ISI_CHAT_STATE_INVALID + 1] = {
    "ISI_CHAT_STATE_INITIATING",
    "ISI_CHAT_STATE_ACTIVE",
    "ISI_CHAT_STATE_INVALID"
};

static char *_ISIG_ChatReasonStrings[ISIP_CHAT_REASON_LAST + 1] = {
    "ISIP_CHAT_REASON_INVALID",
    "ISIP_CHAT_REASON_INITIATE",
    "ISIP_CHAT_REASON_TRYING",
    "ISIP_CHAT_REASON_ACK",
    "ISIP_CHAT_REASON_ACCEPT",
    "ISIP_CHAT_REASON_TERMINATE",
    "ISIP_CHAT_REASON_REJECT",
    "ISIP_CHAT_REASON_FAILED",
    "ISIP_GROUP_CHAT_REASON_INITIATE",
    "ISIP_GROUP_CHAT_REASON_JOIN",
    "ISIP_GROUP_CHAT_REASON_INVITE",
    "ISIP_GROUP_CHAT_REASON_KICK",
    "ISIP_GROUP_CHAT_REASON_DESTROY",
    "ISIP_GROUP_CHAT_REASON_NET",
    "ISIP_GROUP_CHAT_REASON_AUTH",
    "ISIP_DEFERRED_CHAT_REASON_INITIATE",
    "ISIP_CHAT_REASON_LAST"
};

static char *_ISIG_DiagReasonStrings[ISIP_DIAG_REASON_LAST + 1] = {
    "ISIP_DIAG_REASON_INVALID",
    "ISIP_DIAG_REASON_AUDIO_RECORD",
    "ISIP_DIAG_REASON_AUDIO_PLAY",
    "ISIP_DIAG_REASON_NOT_SUPPORTED",
};

void ISIG_logCall(
    uint32  callId,
    vint    state,
    vint    reason,
    uint32  serviceId)
{
    OSAL_logMsg("-------CALL EVENT FOR ID:%d--------\n", callId);

    if (state > ISI_CALL_STATE_INVALID || state < 0) {
        state = (vint)ISI_CALL_STATE_INVALID;
    }
    if (reason > ISIP_CALL_REASON_LAST || reason < 0) {
        reason = (vint)ISIP_CALL_REASON_INVALID;
    }
    OSAL_logMsg("state:%s, reason:%s serviceId:%d\n",
           _ISIG_CallStateStrings[state],
           _ISIG_CallReasonStrings[reason],
           serviceId);
    OSAL_logMsg("-------------------------------------\n");
}

void ISIG_logService(
    uint32  serviceId,
    vint    reason)
{
    OSAL_logMsg("-------SERVICE EVENT FOR ID:%d--------\n", serviceId);

    if (reason > ISIP_SERVICE_REASON_LAST || reason < 0) {
        reason = (vint)ISIP_SERVICE_REASON_INVALID;
    }
    OSAL_logMsg("reason:%s\n", _ISIG_ServiceReasonStrings[reason]);
    OSAL_logMsg("-------------------------------------\n");
}

void ISIG_logAudio(
    vint    reason)
{
    OSAL_logMsg("-------AUDIO EVENT--------\n");

    if (reason > ISIP_MEDIA_REASON_LAST || reason < 0) {
        reason = (vint)ISIP_MEDIA_REASON_INVALID;
    }
    OSAL_logMsg("reason:%s\n", _ISIG_AudioReasonStrings[reason]);
    OSAL_logMsg("-------------------------------------\n");
}

void ISIG_logSystem(
    vint    reason)
{
    OSAL_logMsg("-------SYSTEM LEVEL EVENT--------\n");

    if (reason > ISIP_SYSTEM_REASON_LAST || reason < 0) {
        reason = (vint)ISIP_SYSTEM_REASON_LAST;
    }
    OSAL_logMsg("reason:%s\n", _ISIG_SystemReasonStrings[reason]);
    OSAL_logMsg("-------------------------------------\n");
}

void ISIG_log(
    const char *funct_ptr,
    char       *str_ptr,
    uint32      arg1,
    uint32      arg2,
    uint32      arg3)
{
    vint len;
    char  buffer[ISI_DEBUG_MAX_STRING_SIZE + 129];


    if (funct_ptr != 0) {
        OSAL_logMsg("In the %s routine\n", funct_ptr);
    }
    if (str_ptr != 0) {
        len = OSAL_strlen(str_ptr);
        /* 128 + 1 for NULL termination */
        if (len > ISI_DEBUG_MAX_STRING_SIZE) {
            len = ISI_DEBUG_MAX_STRING_SIZE;
            /* Truncate it */
            str_ptr[len] = 0;
        }
        len = sprintf(buffer, str_ptr, arg1, arg2, arg3);
        if (len >  (vint)sizeof(buffer)) {
            OSAL_logMsg("Error in dbug logging.  Buffer overflow!\n");
        }
        else {
            OSAL_logMsg("%s\n", buffer);
        }
    }
}

void ISIG_logCoder(
    ISIP_Coder coders[],
    vint       numEntries)
{
    int x;
    for (x = 0 ; x < numEntries ; x++) {
        /* Only print specified ones */
        if (coders[x].szCoderName[0] != 0) {
            OSAL_logMsg("ISI Coder - idx:%d name:%s description=%s\n",
                    x, coders[x].szCoderName, coders[x].description);
        }
    }
}

void ISIG_logStream(ISIP_Stream *s_ptr)
{
    int x;

    OSAL_logMsg("ISI Stream Id:%d\taudio dir:%d \tvideo dir:%d\n",
           s_ptr->id, s_ptr->audioDirection, s_ptr->videoDirection);

    OSAL_logMsg("\nISI confMask:%X\n", s_ptr->confMask);

    if (ISI_SESSION_TYPE_SECURITY_AUDIO & s_ptr->type) {
        OSAL_logMsg("Audio Security Enabled\n");
    }
    if (ISI_SESSION_TYPE_SECURITY_VIDEO & s_ptr->type) {
        OSAL_logMsg("Video Security Enabled\n");
    }
    if (ISI_SESSION_TYPE_EMERGENCY & s_ptr->type) {
        OSAL_logMsg("EMERGENCY Enabled\n");
    }
    if (ISI_SESSION_TYPE_AUDIO & s_ptr->type) {
        OSAL_logMsg("audio\n");

        if (OSAL_NET_SOCK_UDP_V6 == s_ptr->audio.lclAddr.type ||
                OSAL_NET_SOCK_TCP_V6 == s_ptr->audio.lclAddr.type) {
            OSAL_logMsg("ISI rmtAddr:%x:%x:%x:%x:%x:%x:%x:%x\tISI rmtPort:%d\tISI rmtCntlPort:%d\n",
                   s_ptr->audio.rmtAddr.ipv6[0], s_ptr->audio.rmtAddr.ipv6[1],
                   s_ptr->audio.rmtAddr.ipv6[2], s_ptr->audio.rmtAddr.ipv6[3],
                   s_ptr->audio.rmtAddr.ipv6[4], s_ptr->audio.rmtAddr.ipv6[5],
                   s_ptr->audio.rmtAddr.ipv6[6], s_ptr->audio.rmtAddr.ipv6[7],
                   s_ptr->audio.rmtAddr.port, s_ptr->audio.rmtCntlPort);

            OSAL_logMsg("ISI lclAddr:%x:%x:%x:%x:%x:%x:%x:%x\tISI lclPort:%d\tISI lclCntlPort:%d\n",
                   s_ptr->audio.lclAddr.ipv6[0], s_ptr->audio.lclAddr.ipv6[1],
                   s_ptr->audio.lclAddr.ipv6[2], s_ptr->audio.lclAddr.ipv6[3],
                   s_ptr->audio.lclAddr.ipv6[4], s_ptr->audio.lclAddr.ipv6[5],
                   s_ptr->audio.lclAddr.ipv6[6], s_ptr->audio.lclAddr.ipv6[7],
                   s_ptr->audio.lclAddr.port, s_ptr->audio.lclCntlPort);
        }
        else {
            OSAL_logMsg("ISI rmtAddr:%x\tISI rmtPort:%d\tISI rmtCntlPort:%d\n",
                   s_ptr->audio.rmtAddr.ipv4, s_ptr->audio.rmtAddr.port, s_ptr->audio.rmtCntlPort);

            OSAL_logMsg("ISI lclAddr:%x\tISI lclPort:%d\tISI lclCntlPort:%d\n",
                   s_ptr->audio.lclAddr.ipv4, s_ptr->audio.lclAddr.port, s_ptr->audio.lclCntlPort);
        }
    }
    if (ISI_SESSION_TYPE_VIDEO & s_ptr->type) {
        OSAL_logMsg("video\n");

        if (OSAL_NET_SOCK_UDP_V6 == s_ptr->audio.lclAddr.type ||
                OSAL_NET_SOCK_TCP_V6 == s_ptr->audio.lclAddr.type) {
            OSAL_logMsg("ISI rmtAddr:%x:%x:%x:%x:%x:%x:%x:%x\tISI rmtPort:%d\tISI rmtCntlPort:%d\n",
                   s_ptr->video.rmtAddr.ipv6[0], s_ptr->video.rmtAddr.ipv6[1],
                   s_ptr->video.rmtAddr.ipv6[2], s_ptr->video.rmtAddr.ipv6[3],
                   s_ptr->video.rmtAddr.ipv6[4], s_ptr->video.rmtAddr.ipv6[5],
                   s_ptr->video.rmtAddr.ipv6[6], s_ptr->video.rmtAddr.ipv6[7],
                   s_ptr->video.rmtAddr.port, s_ptr->video.rmtCntlPort);

            OSAL_logMsg("ISI lclAddr:%x:%x:%x:%x:%x:%x:%x:%x\tISI lclPort:%d\tISI lclCntlPort:%d\n",
                   s_ptr->video.lclAddr.ipv6[0], s_ptr->video.lclAddr.ipv6[1],
                   s_ptr->video.lclAddr.ipv6[2], s_ptr->video.lclAddr.ipv6[3],
                   s_ptr->video.lclAddr.ipv6[4], s_ptr->video.lclAddr.ipv6[5],
                   s_ptr->video.lclAddr.ipv6[6], s_ptr->video.lclAddr.ipv6[7],
                   s_ptr->video.lclAddr.port, s_ptr->video.lclCntlPort);
        }
        else {
            OSAL_logMsg("ISI rmtAddr:%x\tISI rmtPort:%d\tISI rmtCntlPort:%d\n",
                   s_ptr->video.rmtAddr.ipv4, s_ptr->video.rmtAddr.port, s_ptr->video.rmtCntlPort);

            OSAL_logMsg("ISI lclAddr:%x\tISI lclPort:%d\tISI lclCntlPort:%d\n",
                   s_ptr->video.lclAddr.ipv4, s_ptr->video.lclAddr.port, s_ptr->video.lclCntlPort);
        }
    }

    for (x = 0; x < ISI_CODER_NUM; x++) {
        if (s_ptr->coders[x].szCoderName[0] != 0) {
            printf("Coder is %s, its description is %s, and its a %s coder\n",
                    s_ptr->coders[x].szCoderName,
                    s_ptr->coders[x].description,
                    s_ptr->coders[x].relates == ISI_SESSION_TYPE_AUDIO ?
                    "audio" :
                    s_ptr->coders[x].relates == ISI_SESSION_TYPE_VIDEO ?
                    "video" : "unknown");
        }
    }
}

void ISIG_logChat(
    uint32  chatId,
    uint32  state,
    uint32  reason,
    uint32  serviceId)
{
    OSAL_logMsg("-------CHAT EVENT FOR ID:%d--------\n", chatId);

    if (state > ISI_CHAT_STATE_INVALID) {
        state = ISI_CHAT_STATE_INVALID;
    }
    if (reason > ISIP_CHAT_REASON_LAST) {
        reason = ISIP_CHAT_REASON_LAST;
    }

    OSAL_logMsg("state:%s, reason:%s serviceId:%d\n",
           _ISIG_ChatStateStrings[state],
           _ISIG_ChatReasonStrings[reason],
           serviceId);
    OSAL_logMsg("-------------------------------------\n");
}

void ISIG_logDiag(
    vint    reason)
{
    OSAL_logMsg("-------Diagnostic LEVEL EVENT--------\n");

    if (reason > ISIP_DIAG_REASON_LAST || reason < 0) {
        reason = (vint)ISIP_DIAG_REASON_LAST;
    }
    OSAL_logMsg("reason:%s\n", _ISIG_DiagReasonStrings[reason]);
    OSAL_logMsg("-------------------------------------\n");
}
#endif




