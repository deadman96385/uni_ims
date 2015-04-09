/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#include <osal.h>
#include "settings.h"
#include  "_settings_parm.h"

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

    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_PROVISIONING_ENABLED)) {
        return VALUE_RCS_PROVISIONING_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_NAT_URL_FMT)) {
        return VALUE_NAT_URL_FMT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_INT_URL_FMR)) {
        return VALUE_INT_URL_FMT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IM_CONF_SERVER)) {
        return VALUE_IM_CONF_SERVER;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SERVER)) {
        return VALUE_SUPSRV_SERVER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_USER)) {
        return VALUE_SUPSRV_USER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PASSWORD)) {
        return VALUE_SUPSRV_PASSWORD;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMEOUT)) {
        return VALUE_SUPSRV_TIMEOUT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_URI)) {
        return VALUE_SUPSRV_URI;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_SIGNALLING)) {
        return VALUE_PS_SIGNALLING;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_SIGNALLING)) {
        return VALUE_WIFI_SIGNALLING;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_MEDIA)) {
        return VALUE_PS_MEDIA;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_MEDIA)) {
        return VALUE_WIFI_MEDIA;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_RT_MEDIA)) {
        return VALUE_PS_RT_MEDIA;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_RT_MEDIA)) {
        return VALUE_WIFI_RT_MEDIA;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SIP_PORT)) {
        return VALUE_SIP_PORT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO_PORT)) {
        return VALUE_AUDIO_PORT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO_PORT_POOL_SIZE)) {
        return VALUE_AUDIO_PORT_POOL_SIZE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_PORT)) {
        return VALUE_VIDEO_PORT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_PORT_POOL_SIZE)) {
        return VALUE_VIDEO_PORT_POOL_SIZE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SIP_PROTECTED_PORT)) {
        return VALUE_SIP_PROTECTED_PORT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SIP_PROTECTED_PORT_POOL_SIZE)) {
        return VALUE_SIP_PROTECTED_PORT_POOL_SIZE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IPSEC_SPI)) {
        return VALUE_IPSEC_SPI;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IPSEC_SPI_POOL_SIZE)) {
        return VALUE_IPSEC_SPI_POOL_SIZE;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_PDU_FMT)) {
        return VALUE_SMS_PDU_FMT;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_THIS)) {
        return VALUE_MC_INFC_THIS;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO)) {
        return VALUE_MC_INFC_AUDIO;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_STREAM)) {
        return VALUE_MC_INFC_STREAM;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISI)) {
        return VALUE_MC_INFC_ISI;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RTP_PORT)) {
        return VALUE_RTP_PORT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RTCP_INTERVAL)) {
        return VALUE_RTCP_INTERVAL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TONE_AUTO_CALLPROGRESS)) {
        return VALUE_TONE_AUTO_CALLPROGRESS;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_RTP_INACTIVITY)) {
        return VALUE_TIMER_RTP_INACTIVITY;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_THIS)) {
        return VALUE_SAPP_INFC_THIS;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO)) {
        return VALUE_SAPP_INFC_AUDIO;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_STREAM)) {
        return VALUE_SAPP_INFC_STREAM;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISI)) {
        return VALUE_SAPP_INFC_ISI;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RING_TEMPLATE)) {
        return VALUE_RING_TEMPLATE;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_KEEP_ALIVE_ENABLED)) {
        return VALUE_KEEP_ALIVE_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_T1)) {
        return VALUE_PARM_TIMER_T1;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_T2)) {
        return VALUE_PARM_TIMER_T2;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_T4)) {
        return VALUE_PARM_TIMER_T4;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMER_TCALL)) {
        return VALUE_PARM_TIMER_TCALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_RETRY_BASE_TIME)) {
        return VALUE_REG_RETRY_BASE_TIME;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_RETRY_MAX_TIME)) {
        return VALUE_REG_RETRY_MAX_TIME;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_Q_VALUE)) {
        return VALUE_Q_VALUE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_EXPIRE_SEC)) {
        return VALUE_REG_EXPIRE_SEC;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IPSEC_ENABLED)) {
        return VALUE_IPSEC_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISIM_ENABLED)) {
        return VALUE_ISIM_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PRECONDITION_ENABLED)) {
        return VALUE_PRECONDITION_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MWI_EXPIRE_SEC)) {
        return VALUE_MWI_EXPIRE_SEC;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_NAT_KEEP_ALIVE_SEC)) {
        return VALUE_NAT_KEEP_ALIVE_SEC;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PRACK_ENABLED)) {
        return VALUE_PRACK_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_CPIM_ENABLED)) {
        return VALUE_CPIM_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SESSION_TIMER)) {
        return VALUE_SESSION_TIMER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FORCE_MT_SESSION_TIMER)) {
        return VALUE_FORCE_MT_SESSION_TIMER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_REG_EVENT_ENABLED)) {
        return VALUE_REG_EVENT_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MWI_EVENT_ENABLED)) {
        return VALUE_MWI_EVENT_ENABLED;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_UA_NAME)) {
        return VALUE_UA_NAME;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MTU)) {
        return VALUE_MTU;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VOICE_CALL)) {
        return VALUE_REG_IP_VOICE_CALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VIDEO_CALL)) {
        return VALUE_REG_IP_VIDEO_CALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_CHAT)) {
        return VALUE_REG_CHAT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMAGE_SHARE)) {
        return VALUE_REG_IMAGE_SHARE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE)) {
        return VALUE_REG_VIDEO_SHARE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER)) {
        return VALUE_REG_FILE_TRANSFER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_OVER_IP)) {
        return VALUE_REG_SMS_OVER_IP;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MESSAGING)) {
        return VALUE_REG_MESSAGING;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_DISCOVERY_VIA_PRESENCE)) {
        return VALUE_REG_DISCOVERY_VIA_PRESENCE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL)) {
        return VALUE_REG_VIDEO_SHARE_WITHOUT_CALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SOCIAL_PRESENCE)) {
        return VALUE_REG_SOCIAL_PRESENCE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PUSH)) {
        return VALUE_REG_GEOLOCATION_PUSH;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PULL)) {
        return VALUE_REG_GEOLOCATION_PULL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_HTTP)) {
        return VALUE_REG_FILE_TRANSFER_HTTP;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL)) {
        return VALUE_REG_FILE_TRANSFER_THUMBNAIL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_STORE_FWD)) {
        return VALUE_REG_TRANSFER_STORE_FWD;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_CS)) {
        return VALUE_REG_RCS_TELEPHONY_CS;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOLTE)) {
        return VALUE_REG_RCS_TELEPHONY_VOLTE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOHSPA)) {
        return VALUE_REG_RCS_TELEPHONY_VOHSPA;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VOICE_CALL)) {
        return VALUE_EXC_IP_VOICE_CALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IP_VIDEO_CALL)) {
        return VALUE_EXC_IP_VIDEO_CALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_CHAT)) {
        return VALUE_EXC_CHAT;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMAGE_SHARE)) {
        return VALUE_EXC_IMAGE_SHARE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE)) {
        return VALUE_EXC_VIDEO_SHARE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER)) {
        return VALUE_EXC_FILE_TRANSFER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_OVER_IP)) {
        return VALUE_EXC_SMS_OVER_IP;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_MESSAGING)) {
        return VALUE_EXC_MESSAGING;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_DISCOVERY_VIA_PRESENCE)) {
        return VALUE_EXC_DISCOVERY_VIA_PRESENCE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_SHARE_WITHOUT_CALL)) {
        return VALUE_EXC_VIDEO_SHARE_WITHOUT_CALL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SOCIAL_PRESENCE)) {
        return VALUE_EXC_SOCIAL_PRESENCE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PUSH)) {
        return VALUE_EXC_GEOLOCATION_PUSH;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_GEOLOCATION_PULL)) {
        return VALUE_EXC_GEOLOCATION_PULL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_HTTP)) {
        return VALUE_EXC_FILE_TRANSFER_HTTP;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_THUMBNAIL)) {
        return VALUE_EXC_FILE_TRANSFER_THUMBNAIL;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_TRANSFER_STORE_FWD)) {
        return VALUE_EXC_TRANSFER_STORE_FWD;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_CS)) {
        return VALUE_EXC_RCS_TELEPHONY_CS;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOLTE)) {
        return VALUE_EXC_RCS_TELEPHONY_VOLTE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_RCS_TELEPHONY_VOHSPA)) {
        return VALUE_EXC_RCS_TELEPHONY_VOHSPA;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SRVCC_ALERTING)) {
        return VALUE_SRVCC_ALERTING;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SRVCC_MID_CALL)) {
        return VALUE_SRVCC_MID_CALL;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_PRESENCE_EXPIRE_SEC)) {
        return VALUE_PRESENCE_EXPIRE_SEC;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_FLOW_CTRL_DEPTH)) {
        return VALUE_FILE_FLOW_CTRL_DEPTH;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_PATH)) {
        return VALUE_FILE_PATH;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_FILE_PREPEND)) {
        return VALUE_FILE_PREPEND;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_PS_MEDIA)) {
        return VALUE_SIMPLE_PS_MEDIA;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WIFI_MEDIA)) {
        return VALUE_SIMPLE_WIFI_MEDIA;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_BLACK_LIST)) {
        return VALUE_XCAP_BLACK_LIST;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_WHITE_LIST)) {
        return VALUE_XCAP_WHITE_LIST;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_TIMEOUT)) {
        return VALUE_SCAP_TIMEOUT;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_VDN)) {
        return VALUE_HANDOFF_VDN;
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
    if (0 == strcmp(parm_ptr, SETTINGS_ATTR_ID)) {
        return VALUE_ATTR_PROTOCOL_ID;
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
    if (0 == strcmp(parm_ptr, SETTINGS_ATTR_CAP_DISCOVERY)) {
        return VALUE_ATTR_CAP_DISCOVERY;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_ATTR_COMMON_STACK)) {
        return VALUE_ATTR_COMMON_STACK;
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
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_DOMAIN)) {
        return VALUE_ISIM_DOMAIN;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PCSCF)) {
        return VALUE_ISIM_PCSCF;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMPU)) {
        return VALUE_ISIM_IMPU;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMPI)) {
        return VALUE_ISIM_IMPI;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMSC)) {
        return VALUE_ISIM_SMSC;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_IMEI_URI)) {
        return VALUE_ISIM_IMEI_URI;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_INSTANCE_ID)) {
        return VALUE_ISIM_INSTANCE_ID;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AKA_KI)) {
        return VALUE_ISIM_AKA_KI;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AKA_OP)) {
        return VALUE_ISIM_AKA_OP;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AKA_OPC)) {
        return VALUE_ISIM_AKA_OPC;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_PASSWORD)) {
        return VALUE_ISIM_PASSWORD;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO_CONF_SERVER)) {
        return VALUE_ISIM_AUDIO_CONF_SERVER;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VIDEO_CONF_SERVER)) {
        return VALUE_ISIM_VIDEO_CONF_SERVER;
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
 * ======== _getGappInfcParmValue() ========
 * Get parameter value from GAPP Interface tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getGappInfcParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_THIS)) {
        return VALUE_GAPP_INTERFACE_THIS;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_AUDIO)) {
        return VALUE_GAPP_INTERFACE_AUDIO;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_STREAM)) {
        return VALUE_GAPP_INTERFACE_STREAM;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_ISI)) {
        return VALUE_GAPP_INTERFACE_ISI;
    }
    return NULL;
}

/*
 * ======== _getGappAudioParmValue() ========
 * Get parameter value from GAPP Audio tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getGappAudioParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_RING_TEMPLATE)) {
        return VALUE_GAPP_RING_TEMPLATE;
    }
    
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_VDI)) {
        return VALUE_GAPP_HANDOFF_VDI;
    }
    
    return NULL;
}

/*
 * ======== _geGappProtoAttrValue() ========
 * Get attribute value from GAPP protocol tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _getGappProtoParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(parm_ptr, SETTINGS_TAG_HANDOFF)) {
        return VALUE_GAPP_HANDOFF_VDI;
    }
    return NULL;
}

/*
 * ======== _geGappProtoAttrValue() ========
 * Get attribute value from GAPP protocol tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _getGappProtoAttrValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(parm_ptr, SETTINGS_ATTR_ID)) {
        return VALUE_GAPP_PROTOCLO_ID;
    }
    return NULL;
}

/*
 * ======== _geGappSmsAttrValue() ========
 * Get attribute value from GAPP protocol tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _geGappSmsParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_TYPE)) {
        return VALUE_GAPP_SMS_TYPE;
    }
    else if (0 == strcmp(parm_ptr, SETTINGS_PARM_SMS_PDU_FMT)) {
        return VALUE_GAPP_SMS_PDUFMT;
    }
    return NULL;
}

/*
 * ======== _getGappProxyParmValue() ========
 * Get parameter value from GAPP proxy tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _getGappProxyParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_TERMINAL)) {
        return VALUE_GAPP_PROXY_TERMINAL;
    }    
    return NULL;
}

/*
 * ======== _geGappProtoAttrValue() ========
 * Get attribute value from GAPP protocol tag.
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of attribute
 */
char* _getGappEmercyParmValue(
    void       *cfg_ptr,
    const char *parm_ptr)
{   
    if (0 == strcmp(parm_ptr, SETTINGS_PARM_EMERGENCY_NUMBER)) {
        return VALUE_GAPP_EMERGENCY_NUMBER;
    }  
    return NULL;
}

/*
 * ======== _SETTINGS_getGappAttrValue() ========
 * The entry to get Gapp paramter value
 *
 * Returns:
 *    NULL  : Unable to find the config by input information
 *    Char* : The value of parameter
 */
char* _SETTINGS_getGappParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_INTERFACE)) {
        /* service tag. */
        return _getGappInfcParmValue(cfg_ptr, parm_ptr);
    }
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_AUDIO)) {
        /* service tag. */
        return _getGappAudioParmValue(cfg_ptr, parm_ptr);
    }
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_PROTOCOL)) {
            /* protocol attribe. */
            return _getGappProtoParmValue(cfg_ptr, parm_ptr);
    }
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_PROXY)) {
            /* protocol attribe. */
            return _getGappProxyParmValue(cfg_ptr, parm_ptr);
    }

    if (0 == strcmp(tag_ptr, SETTINGS_TAG_SMS)) {
            /* protocol attribe. */
            return _geGappSmsParmValue(cfg_ptr, parm_ptr);
    }

    if (0 == strcmp(tag_ptr, SETTINGS_TAG_EMERGENCY)) {
            /* protocol attribe. */
            return _getGappEmercyParmValue(cfg_ptr, parm_ptr);
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
char* _SETTINGS_getGappAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr)
{
    if (0 == strcmp(tag_ptr, SETTINGS_TAG_PROTOCOL)) {
            /* protocol attribe. */
            return _getGappProtoAttrValue(cfg_ptr, parm_ptr);
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

