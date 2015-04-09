/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30336 $ $Date: 2014-12-11 10:24:15 +0800 (Thu, 11 Dec 2014) $
 */

#ifndef __SETTINGS_PARM_H_
#define __SETTINGS_PARM_H_

/* CSM related. */
#define VALUE_RCS_PROVISIONING_ENABLED     "0"
#define VALUE_NAT_URL_FMT                  "1"
#define VALUE_INT_URL_FMT                  "1"
#define VALUE_IM_CONF_SERVER               ""
#define VALUE_SUPSRV_SERVER                "http://172.16.0.100:8080"
#define VALUE_SUPSRV_USER                  "user1"
#define VALUE_SUPSRV_PASSWORD              "12345678"
#define VALUE_SUPSRV_TIMEOUT               "100"
#define VALUE_SUPSRV_URI                   "user1@iskratel.com"
#define VALUE_PS_SIGNALLING                "SIPoTCP"
#define VALUE_WIFI_SIGNALLING              "SIPoTCP"
#define VALUE_PS_MEDIA                     "MSRPoTLS"
#define VALUE_WIFI_MEDIA                   "MSRP"
#define VALUE_PS_RT_MEDIA                  "RTP"
#define VALUE_WIFI_RT_MEDIA                "RTP"
#define VALUE_SIP_PORT                     "5060"
#define VALUE_AUDIO_PORT                   "59000"
#define VALUE_AUDIO_PORT_POOL_SIZE         "20"
#define VALUE_VIDEO_PORT                   "60000"
#define VALUE_VIDEO_PORT_POOL_SIZE         "20"
#define VALUE_SIP_PROTECTED_PORT           "7000"
#define VALUE_SIP_PROTECTED_PORT_POOL_SIZE "20"
#define VALUE_IPSEC_SPI                    "1234"
#define VALUE_IPSEC_SPI_POOL_SIZE          "20"
#define VALUE_SMS_PDU_FMT                  "TPDU"

/* SAPP related. */
#define VALUE_SAPP_INFC_THIS               "sapp.ipc"
#define VALUE_SAPP_INFC_AUDIO              "mc.ipc"
#define VALUE_SAPP_INFC_STREAM             "mc.ipc"
#define VALUE_SAPP_INFC_ISI                "isi.ipc.protocol"
#define VALUE_RING_TEMPLATE                "12"
#define VALUE_KEEP_ALIVE_ENABLED           "0"
#define VALUE_PARM_TIMER_T1                "2000"
#define VALUE_PARM_TIMER_T2                "16000"
#define VALUE_PARM_TIMER_T4                "17000"
#define VALUE_PARM_TIMER_TCALL             "10000"
#define VALUE_REG_RETRY_BASE_TIME          "30"
#define VALUE_REG_RETRY_MAX_TIME           "360"
#define VALUE_Q_VALUE                      "0.5"
#define VALUE_REG_EXPIRE_SEC               "3600"
#define VALUE_IPSEC_ENABLED                "0"
#define VALUE_ISIM_ENABLED                 "0"
#define VALUE_PRECONDITION_ENABLED         "0"
#define VALUE_MWI_EXPIRE_SEC               "0"
#define VALUE_NAT_KEEP_ALIVE_SEC           "0"
#define VALUE_PRACK_ENABLED                "0"
#define VALUE_CPIM_ENABLED                 "0"
#define VALUE_SESSION_TIMER                "0"
#define VALUE_FORCE_MT_SESSION_TIMER       "0"
#define VALUE_REG_EVENT_ENABLED            "0"
#define VALUE_MWI_EVENT_ENABLED            "0"
#define VALUE_UA_NAME                      "D2 IMS 4G"
#define VALUE_MTU                          "1300"
#define VALUE_REG_IP_VOICE_CALL            "1"
#define VALUE_REG_IP_VIDEO_CALL            "0"
#define VALUE_REG_CHAT                     "0"
#define VALUE_REG_IMAGE_SHARE              "0"
#define VALUE_REG_VIDEO_SHARE              "0"
#define VALUE_REG_FILE_TRANSFER            "0"
#define VALUE_REG_SMS_OVER_IP              "1"
#define VALUE_REG_MESSAGING                "0"
#define VALUE_REG_DISCOVERY_VIA_PRESENCE   "0"
#define VALUE_REG_VIDEO_SHARE_WITHOUT_CALL "0"
#define VALUE_REG_SOCIAL_PRESENCE          "0"
#define VALUE_REG_GEOLOCATION_PUSH         "0"
#define VALUE_REG_GEOLOCATION_PULL         "0"
#define VALUE_REG_FILE_TRANSFER_HTTP       "0"
#define VALUE_REG_FILE_TRANSFER_THUMBNAIL  "0"
#define VALUE_REG_TRANSFER_STORE_FWD       "0"
#define VALUE_REG_RCS_TELEPHONY_CS         "1"
#define VALUE_REG_RCS_TELEPHONY_VOLTE      "1"
#define VALUE_REG_RCS_TELEPHONY_VOHSPA     "0"
#define VALUE_EXC_IP_VOICE_CALL            "0"
#define VALUE_EXC_IP_VIDEO_CALL            "1"
#define VALUE_EXC_CHAT                     "1"
#define VALUE_EXC_IMAGE_SHARE              "0"
#define VALUE_EXC_VIDEO_SHARE              "0"
#define VALUE_EXC_FILE_TRANSFER            "1"
#define VALUE_EXC_SMS_OVER_IP              "1"
#define VALUE_EXC_MESSAGING                "1"
#define VALUE_EXC_DISCOVERY_VIA_PRESENCE   "0"
#define VALUE_EXC_VIDEO_SHARE_WITHOUT_CALL "0"
#define VALUE_EXC_SOCIAL_PRESENCE          "0"
#define VALUE_EXC_GEOLOCATION_PUSH         "0"
#define VALUE_EXC_GEOLOCATION_PULL         "0"
#define VALUE_EXC_FILE_TRANSFER_HTTP       "0"
#define VALUE_EXC_FILE_TRANSFER_THUMBNAIL  "0"
#define VALUE_EXC_TRANSFER_STORE_FWD       "0"
#define VALUE_EXC_RCS_TELEPHONY_CS         "0"
#define VALUE_EXC_RCS_TELEPHONY_VOLTE      "0"
#define VALUE_EXC_RCS_TELEPHONY_VOHSPA     "0"
#define VALUE_SRVCC_ALERTING               "1"
#define VALUE_SRVCC_MID_CALL               "0"
#define VALUE_PRESENCE_EXPIRE_SEC          "3600"
#define VALUE_FILE_FLOW_CTRL_DEPTH         "8"
#define VALUE_FILE_PATH                    "/var/tmp/osal"
#define VALUE_FILE_PREPEND                 "PATH_"
#define VALUE_SIMPLE_PS_MEDIA              "MSRPoTLS"
#define VALUE_SIMPLE_WIFI_MEDIA            "MSRP"
#define VALUE_XCAP_BLACK_LIST              ""
#define VALUE_XCAP_WHITE_LIST              ""
#define VALUE_SCAP_TIMEOUT                 "0"
#define VALUE_HANDOFF_VDN                  "6472885452"
#define VALUE_ATTR_PROTOCOL_ID             "1"
#define VALUE_ATTR_CAP_DISCOVERY           "1"
#define VALUE_ATTR_COMMON_STACK            "1"

/* MC related. */
#define VALUE_MC_INFC_THIS                 "mc.ipc"
#define VALUE_MC_INFC_AUDIO                "mc.ipc"
#define VALUE_MC_INFC_STREAM               "mc.ipc"
#define VALUE_MC_INFC_ISI                  "isi.ipc.protocol"
#define VALUE_RTP_PORT                     "59000"
#define VALUE_RTCP_INTERVAL                "2"
#define VALUE_TONE_AUTO_CALLPROGRESS       "on"
#define VALUE_TIMER_RTP_INACTIVITY         "10"

/* ISIM related. */
#define VALUE_ISIM_DOMAIN                  "foundry.att.com"
#define VALUE_ISIM_PCSCF                   "sbc.core.foundry.att.com;transport=TCP"
#define VALUE_ISIM_AUDIO_CONF_SERVER       "sip:ALU_CONF@foundry.att.com"
#define VALUE_ISIM_VIDEO_CONF_SERVER       "sip:ALU_CONF@foundry.att.com"
#define VALUE_ISIM_IMPU                    "sip:+14043351655@foundry.att.com"
#define VALUE_ISIM_IMPI                    "4043351655@private.att.net"
#define VALUE_ISIM_PASSWORD                "password"
#define VALUE_ISIM_SMSC                    ""
#define VALUE_ISIM_IMEI_URI                "sip:imei219551288888888@sos.invalid"
#define VALUE_ISIM_INSTANCE_ID             ""
#define VALUE_ISIM_AKA_KI                  ""
#define VALUE_ISIM_AKA_OP                  ""
#define VALUE_ISIM_AKA_OPC                 ""

/* GAPP related */
#define VALUE_GAPP_INTERFACE_THIS          "gapp.ipc"
#define VALUE_GAPP_INTERFACE_AUDIO         "mc.ipc"
#define VALUE_GAPP_INTERFACE_STREAM        "gapp.ipc"
#define VALUE_GAPP_INTERFACE_ISI           "isi.ipc.protocol"
#define VALUE_GAPP_RING_TEMPLATE           "12" 
#define VALUE_GAPP_PROTOCLO_ID             "3"
#define VALUE_GAPP_HANDOFF_VDI             "sip:16784732700@216.234.148.28"
#define VALUE_GAPP_PROXY_TERMINAL          "/var/tmp/osal/smd0" 
#define VALUE_GAPP_SMS_TYPE                "3GPP"
#define VALUE_GAPP_SMS_PDUFMT              "TPDU" 
#define VALUE_GAPP_EMERGENCY_NUMBER        "112"

char* _SETTINGS_getCsmParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getCsmAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getSappParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getSappAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getMcParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getMcAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getIsimParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getIsimAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getGappParmValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

char* _SETTINGS_getGappAttrValue(
    int         nestedMode,
    void       *cfg_ptr,
    const char *tag_ptr,
    const char *chdOne_ptr,
    const char *chdTwo_ptr,
    const char *parm_ptr);

#endif
