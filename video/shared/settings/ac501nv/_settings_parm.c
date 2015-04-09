/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#include <osal.h>
#include "settings.h"
#include "nvdata.h"

#define VALUE_ISIM_AUDIO_CONF_SERVER       "sip:ALU_CONF@foundry.att.com"
#define VALUE_ISIM_VIDEO_CONF_SERVER       "sip:ALU_CONF@foundry.att.com"
/*
 * ======== _getCsmServiceParmValue() ========
 * Get parameter value from CSM service tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getCsmServiceParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    CSM_nvKeeper *data = (CSM_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_PROVISIONING_ENABLED)) {
        OSAL_itoa(data->nvData->service_rcsPvEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_NAT_URL_FMT)) {
        OSAL_itoa(data->nvData->service_natUrlFmt, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_INT_URL_FMR)) {
        OSAL_itoa(data->nvData->service_intUrlFmt, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IM_CONF_SERVER)) {
        return (char *)data->nvData->service_confFctyUri;
    }

    return NULL;
}

/*
 * ======== _getCsmServiceSupSrvParmValue() ========
 * Get parameter value from CSM service/SupplementarySrv tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getCsmServiceSupSrvParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    CSM_nvKeeper *data = (CSM_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SERVER)) {
        return (char *)data->nvData->ss_server;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_USER)) {
        return (char *)data->nvData->ss_user;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PASSWORD)) {
        return (char *)data->nvData->ss_password;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMEOUT)) {
        OSAL_itoa(data->nvData->ss_timeout, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_URI)) {
        return (char *)data->nvData->ss_uri;
    }

    return NULL;
}

/*
 * ======== _getCsmServiceTransportParmValue() ========
 * Get parameter value from CSM service/transportProto tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getCsmServiceTransportParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    CSM_nvKeeper *data = (CSM_nvKeeper *)cfg_ptr;
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_SIGNALLING)) {
        return (char *)data->nvData->tspt_psSignalling;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_SIGNALLING)) {
        return (char *)data->nvData->tspt_wifiSignalling;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_MEDIA)) {
        return (char *)data->nvData->tspt_psMedia;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_MEDIA)) {
        return (char *)data->nvData->tspt_wifiMedia;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_RT_MEDIA)) {
        return (char *)data->nvData->tspt_psRtMedia;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_RT_MEDIA)) {
        return (char *)data->nvData->tspt_wifiRtMedia;
    }

    return NULL;
}

/*
 * ======== _getCsmServicePortParmValue() ========
 * Get parameter value from CSM service/port tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getCsmServicePortParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    CSM_nvKeeper *data = (CSM_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SIP_PORT)) {
        OSAL_itoa(data->nvData->port_sip, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO_PORT)) {
        OSAL_itoa(data->nvData->port_audio, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO_PORT_POOL_SIZE)) {
        OSAL_itoa(data->nvData->port_audioPoolSize, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_PORT)) {
        OSAL_itoa(data->nvData->port_video, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_PORT_POOL_SIZE)) {
        OSAL_itoa(data->nvData->port_videoPoolSize, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SIP_PROTECTED_PORT)) {
        OSAL_itoa(data->nvData->port_sipProtected, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SIP_PROTECTED_PORT_POOL_SIZE)) {
        OSAL_itoa(data->nvData->port_sipProtectedPoolSize, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IPSEC_SPI)) {
        OSAL_itoa(data->nvData->port_ipsec, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IPSEC_SPI_POOL_SIZE)) {
        OSAL_itoa(data->nvData->port_ipsecPoolSize, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getCsmSmsParmValue() ========
 * Get parameter value from CSM sms tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getCsmSmsParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    CSM_nvKeeper *data = (CSM_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_PDU_FMT)) {
        return (char *)data->nvData->sms_pduFmt;
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getCsmParmValue() ========
 * The entry to get CSM paramter value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _SETTINGS_getCsmParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (SETTINGS_NESTED_NONE == nestedMode) {
        if (0 == strcmp(tag_ptr, SETTINGS_TAG_SERVICE)) {
            /* service tag. */
            return _getCsmServiceParmValue(cfg_ptr, parm_ptr);
        }
        else if (0 == strcmp(tag_ptr, SETTINGS_TAG_SMS)) {
            /* SMS tag. */
            return _getCsmSmsParmValue(cfg_ptr, parm_ptr);
        }
    }
    else if (SETTINGS_NESTED_ONE == nestedMode) {
        if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_SUPPLEMENTARY_SRV)) {
            /* service/SupplementarySrv tag. */
            return _getCsmServiceSupSrvParmValue(cfg_ptr, parm_ptr);
        }
        else if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_TRANSPORT_PROTO)) {
            /* service/transportProto tag. */
            return _getCsmServiceTransportParmValue(cfg_ptr, parm_ptr);
        }
        else if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_PORT)) {
            /* service/port tag. */
            return _getCsmServicePortParmValue(cfg_ptr, parm_ptr);
        }
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getCsmAttrValue() ========
 * The entry to get CSM attribute value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attritube
 */
char* _SETTINGS_getCsmAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    return NULL;
}

/*
 * ======== _getMcInfcParmValue() ========
 * Get parameter value from MC interface tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getMcInfcParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    MC_nvKeeper *data = (MC_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_THIS)) {
        return (char *)data->nvData->infc_this;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO)) {
        return (char *)data->nvData->infc_audio;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_STREAM)) {
        return (char *)data->nvData->infc_stream;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISI)) {
        return (char *)data->nvData->infc_isi;
    }

    return NULL;
}

/*
 * ======== _getMcAudioParmValue() ========
 * Get parameter value from MC audio tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getMcAudioParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    MC_nvKeeper *data = (MC_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RTP_PORT)) {
        OSAL_itoa(data->nvData->audio_rtpPort, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RTCP_INTERVAL)) {
        OSAL_itoa(data->nvData->audio_rtpInterval, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TONE_AUTO_CALLPROGRESS)) {
        return (char *)data->nvData->audio_toneAutoCP;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_RTP_INACTIVITY)) {
        OSAL_itoa(data->nvData->audio_timerRtpInact, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getMcParmValue() ========
 * The entry to get MC paramter value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _SETTINGS_getMcParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_INTERFACE)) {
        /* interface tag. */
        return _getMcInfcParmValue(cfg_ptr, parm_ptr);
    }
    else if (0 == strcmp(tag_ptr, SETTINGS_TAG_AUDIO)) {
        /* audio tag. */
        return _getMcAudioParmValue(cfg_ptr, parm_ptr);
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getMcAttrValue() ========
 * The entry to get MC attribute value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attritube
 */
char* _SETTINGS_getMcAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    return NULL;
}

/*
 * ======== _getSappInfcParmValue() ========
 * Get parameter value from SAPP interface tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappInfcParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_THIS)) {
        return (char *)data->nvData->infc_this;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO)) {
        return (char *)data->nvData->infc_audio;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_STREAM)) {
        return (char *)data->nvData->infc_stream;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISI)) {
        return (char *)data->nvData->infc_isi;
    }

    return NULL;
}

/*
 * ======== _getSappAudioParmValue() ========
 * Get parameter value from SAPP audio tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappAudioParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RING_TEMPLATE)) {
        OSAL_itoa(data->nvData->audio_ringTemplate, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSipParmValue() ========
 * Get parameter value from SAPP protocol/sip tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoSipParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_KEEP_ALIVE_ENABLED)) {
        OSAL_itoa(data->nvData->sip_keepAliveEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_T1)) {
        OSAL_itoa(data->nvData->sip_timerT1, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_T2)) {
        OSAL_itoa(data->nvData->sip_timerT2, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_T4)) {
        OSAL_itoa(data->nvData->sip_timerT4, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_TCALL)) {
        OSAL_itoa(data->nvData->sip_timerTcall, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_RETRY_BASE_TIME)) {
        OSAL_itoa(data->nvData->sip_regRetryBaseTime, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_RETRY_MAX_TIME)) {
        OSAL_itoa(data->nvData->sip_regRetryMaxTime, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_Q_VALUE)) {
        return (char *)data->nvData->sip_qValue;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_EXPIRE_SEC)) {
        OSAL_itoa(data->nvData->sip_regExpireSec, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IPSEC_ENABLED)) {
        OSAL_itoa(data->nvData->sip_ipsecEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISIM_ENABLED)) {
        OSAL_itoa(data->nvData->sip_isimEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PRECONDITION_ENABLED)) {
        OSAL_itoa(data->nvData->sip_preconditionEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MWI_EXPIRE_SEC)) {
        OSAL_itoa(data->nvData->sip_mwiExpireSec, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_NAT_KEEP_ALIVE_SEC)) {
        OSAL_itoa(data->nvData->sip_NatKeepAliveSec, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PRACK_ENABLED)) {
        OSAL_itoa(data->nvData->sip_prackEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_CPIM_ENABLED)) {
        OSAL_itoa(data->nvData->sip_cpimEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SESSION_TIMER)) {
        return (char *)data->nvData->sip_sessionTimer;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FORCE_MT_SESSION_TIMER)) {
        return (char *)data->nvData->sip_forceMtSessionTimer;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_EVENT_ENABLED)) {
        OSAL_itoa(data->nvData->sip_regEventEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MWI_EVENT_ENABLED)) {
        OSAL_itoa(data->nvData->sip_mwiEventEnabled, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_UA_NAME)) {
        return (char *)data->nvData->sip_uaName;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MTU)) {
        OSAL_itoa(data->nvData->sip_mtu, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSipRegCapParmValue() ========
 * Get parameter value from SAPP protocol/sip/reg-capabilities
 * tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoSipRegCapParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VOICE_CALL)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.ipVoiceCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VIDEO_CALL)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.ipVideoCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_CHAT)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.chat,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMAGE_SHARE)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.imageShare,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.videoShare,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.fileTransfer,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_OVER_IP)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.smsOverIp,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MESSAGING)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.messaging,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_DISCOVERY_VIA_PRESENCE)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.discoverViaPresence,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.videoShareWoCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SOCIAL_PRESENCE)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.socialPresence,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PUSH)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.geolocationPush,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PULL)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.geolocationPull,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_HTTP)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.fileTransferHttp,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.fileTransferThumbnail,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_STORE_FWD)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.fileTransferStoreForward,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_CS)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.rcsTelephonyCs,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOLTE)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.rcsTelephonyVolte,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOHSPA)) {
        OSAL_itoa(data->nvData->sip_regCapabilities.rcsTelephonyVohspa,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSipExcCapParmValue() ========
 * Get parameter value from SAPP protocol/sip/exchange-capabilities
 * tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoSipExcCapParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VOICE_CALL)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.ipVoiceCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VIDEO_CALL)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.ipVideoCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_CHAT)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.chat,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMAGE_SHARE)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.imageShare,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.videoShare,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.fileTransfer,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_OVER_IP)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.smsOverIp,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MESSAGING)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.messaging,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_DISCOVERY_VIA_PRESENCE)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.discoverViaPresence,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.videoShareWoCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SOCIAL_PRESENCE)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.socialPresence,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PUSH)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.geolocationPush,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PULL)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.geolocationPull,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_HTTP)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.fileTransferHttp,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.fileTransferThumbnail,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_STORE_FWD)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.fileTransferStoreForward,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_CS)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.rcsTelephonyCs,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOLTE)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.rcsTelephonyVolte,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOHSPA)) {
        OSAL_itoa(data->nvData->sip_excCapabilities.rcsTelephonyVohspa,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSipSrvccCapParmValue() ========
 * Get parameter value from SAPP protocol/sip/srvcc-capabilities
 * tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoSipSrvccCapParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SRVCC_ALERTING)) {
        OSAL_itoa(data->nvData->sip_srvccCapabilities.alerting,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SRVCC_MID_CALL)) {
        OSAL_itoa(data->nvData->sip_srvccCapabilities.midCall,
                data->dataHolder, sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSimpleParmValue() ========
 * Get parameter value from SAPP protocol/simple tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoSimpleParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_PRESENCE_EXPIRE_SEC)) {
        OSAL_itoa(data->nvData->simple_presenceExpireSec, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_FLOW_CTRL_DEPTH)) {
        OSAL_itoa(data->nvData->simple_fileFlowCtrlDepth, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_PATH)) {
        return (char *)data->nvData->simple_filePath;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_PREPEND)) {
        return (char *)data->nvData->simple_filePrepend;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSimpleTransParmValue() ========
 * Get parameter value from SAPP protocol/simple/transportProto tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoSimpleTransParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_MEDIA)) {
        return (char *)data->nvData->simple_psMedia;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_MEDIA)) {
        return (char *)data->nvData->simple_wifiMedia;
    }

    return NULL;
}

/*
 * ======== _getSappProtoXcapParmValue() ========
 * Get parameter value from SAPP protocol/xcap tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoXcapParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_BLACK_LIST)) {
        return (char *)data->nvData->xcap_blackList;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WHITE_LIST)) {
        return (char *)data->nvData->xcap_whiteList;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMEOUT)) {
        OSAL_itoa(data->nvData->xcap_timeout, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoHandoffParmValue() ========
 * Get parameter value from SAPP protocol/handoff tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getSappProtoHandoffParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_VDN)) {
        return (char *)data->nvData->handoff_vdn;
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getSappParmValue() ========
 * The entry to get SAPP paramter value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _SETTINGS_getSappParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (SETTINGS_NESTED_NONE == nestedMode) {
        if (0 == strcmp(tag_ptr, SETTINGS_TAG_INTERFACE)) {
            /* interface tag. */
            return _getSappInfcParmValue(cfg_ptr, parm_ptr);
        }
        else if (0 == strcmp(tag_ptr, SETTINGS_TAG_AUDIO)) {
            /* audio tag. */
            return _getSappAudioParmValue(cfg_ptr, parm_ptr);
        }
    }
    else if (SETTINGS_NESTED_ONE == nestedMode) {
        if (0 == strcmp(tag_ptr, SETTINGS_TAG_PROTOCOL)) {
            if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_SIP)) {
                /* protocol/sip tag. */
                return _getSappProtoSipParmValue(cfg_ptr, parm_ptr);
            }
            else if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_SIMPLE)) {
                /* protocol/simple tag. */
                return _getSappProtoSimpleParmValue(cfg_ptr, parm_ptr);
            }
            else if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_XCAP)) {
                /* protocol/xcap tag. */
                return _getSappProtoXcapParmValue(cfg_ptr, parm_ptr);
            }
            else if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_HANDOFF)) {
                /* protocol/handoff tag. */
                return _getSappProtoHandoffParmValue(cfg_ptr, parm_ptr);
            }
        }
    }
    else if (SETTINGS_NESTED_TWO == nestedMode) {
        if (0 == strcmp(tag_ptr, SETTINGS_TAG_PROTOCOL)) {
            if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_SIP)) {
                if (0 == strcmp(chdTwo_ptr, SETTINGS_TAG_REG_CAPABILITIES)) {
                    /* protocol/sip/reg-capabilities tag. */
                    return _getSappProtoSipRegCapParmValue(cfg_ptr, parm_ptr);
                }
                else if (0 == strcmp(chdTwo_ptr, SETTINGS_TAG_EX_CAPABILITIES)) {
                    /* protocol/sip/exchange-capabilities tag. */
                    return _getSappProtoSipExcCapParmValue(cfg_ptr, parm_ptr);
                }
                else if (0 == strcmp(chdTwo_ptr, SETTINGS_TAG_SRVCC_CAPABILITIES)) {
                    /* protocol/sip/srvcc-capabilities tag. */
                    return _getSappProtoSipSrvccCapParmValue(cfg_ptr, parm_ptr);
                }
            }
            else if (0 == strcmp(chdOne_ptr, SETTINGS_TAG_SIMPLE)) {
                if (0 == strcmp(chdTwo_ptr, SETTINGS_TAG_TRANSPORT_PROTO)) {
                    /* protocol/simple/transportProto tag. */
                    return _getSappProtoSimpleTransParmValue(cfg_ptr, parm_ptr);
                }
            }
        }
    }

    return NULL;
}

/*
 * ======== _getSappProtoAttrValue() ========
 * Get attribute value from SAPP protocol tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _getSappProtoAttrValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_ATTR_ID)) {
        OSAL_itoa(data->nvData->protocol_id, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _getSappProtoSipExcCapAttrValue() ========
 * Get attribute value from SAPP protocol/sip/exchange-capabilities
 * tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _getSappProtoSipExcCapAttrValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    SAPP_nvKeeper *data = (SAPP_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_ATTR_CAP_DISCOVERY)) {
        OSAL_itoa(data->nvData->sip_capDiscovery, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_ATTR_COMMON_STACK)) {
        OSAL_itoa(data->nvData->sip_commonStack, data->dataHolder,
                sizeof(data->dataHolder));
        return data->dataHolder;
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getSappAttrValue() ========
 * The entry to get SAPP attribute value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attritube
 */
char* _SETTINGS_getSappAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (SETTINGS_NESTED_NONE == nestedMode) {
        if (0 == strcmp(tag_ptr, SETTINGS_TAG_PROTOCOL)) {
            /* protocol attribe. */
            return _getSappProtoAttrValue(cfg_ptr, parm_ptr);
        }
    }
    else if (SETTINGS_NESTED_TWO == nestedMode) {
        if (0 == strcmp(chdTwo_ptr, SETTINGS_TAG_EX_CAPABILITIES)) {
            /* protocol/sip/exchange-capabilities attribe. */
            return _getSappProtoSipExcCapAttrValue(cfg_ptr, parm_ptr);
        }
    }

    return NULL;
}

/*
 * ======== _getIsimServiceParmValue() ========
 * Get parameter value from ISIM service tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getIsimServiceParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    ISIM_nvKeeper *data = (ISIM_nvKeeper *)cfg_ptr;

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_DOMAIN)) {
        return (char *)data->nvData->domain;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PCSCF)) {
        return (char *)data->nvData->pcscf;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMPU)) {
        return (char *)data->nvData->impu;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMPI)) {
        return (char *)data->nvData->impi;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMSC)) {
        return (char *)data->nvData->smsc;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMEI_URI)) {
        return (char *)data->nvData->imeiUri;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_INSTANCE_ID)) {
        return (char *)data->nvData->instandId;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AKA_KI)) {
        return (char *)data->nvData->akaKi;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AKA_OP)) {
        return (char *)data->nvData->akaOp;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AKA_OPC)) {
        return (char *)data->nvData->akaOpc;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PASSWORD)) {
        return (char *)data->nvData->password;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO_CONF_SERVER)) {
        return (char *)data->nvData->audioConfSrv;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_CONF_SERVER)) {
        return (char *)data->nvData->videoConfSrv;;
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getIsimParmValue() ========
 * The entry to get ISIM paramter value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _SETTINGS_getIsimParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_SERVICE)) {
        /* service tag. */
        return _getIsimServiceParmValue(cfg_ptr, parm_ptr);
    }

    return NULL;
}

/*
 * ======== _SETTINGS_getIsimAttrValue() ========
 * The entry to get ISIM attribute value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attritube
 */
char* _SETTINGS_getIsimAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    return NULL;
}

