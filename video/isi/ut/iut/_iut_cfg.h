/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2006 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 20147 $ $Date: 2013-03-20 13:39:21 +0800 (Wed, 20 Mar 2013) $
 */

#ifndef _IUT_CFG_H_
#define _IUT_CFG_H_

#define IUT_CFG_STR_SZ          (127)

#define IUT_CFG_MAX_SERVICE     (4)

#define IUT_CONFIG_FILE_NAME    "iut_config"

#define IUT_CFG_STR_OUT         "%s%s%s\n"
#define IUT_CFG_INT_OUT         "%s%s%d\n"
#define IUT_CFG_SERVICE_STR     "SERVICE"
#define IUT_CFG_DELIMITER_STR   "="
#define IUT_CFG_BREAK_STR       "\n#####################\n"

#define IUT_CFG_NONE_STR        "none"
#define IUT_CFG_NAME_STR        "name"
#define IUT_CFG_PROXY_STR       "proxy"
#define IUT_CFG_STUN_STR        "stun"
#define IUT_CFG_RELAY_STR       "relay"
#define IUT_CFG_XCAP_STR        "xcap-root"
#define IUT_CFG_CHAT_STR        "chat"
#define IUT_CFG_OUTBOUND_STR    "outbound"
#define IUT_CFG_USERNAME_STR    "username"
#define IUT_CFG_PASSWORD_STR    "password"
#define IUT_CFG_REALM_STR       "realm"
#define IUT_CFG_URI_STR         "uri"
#define IUT_CFG_CID_PRIVATE_STR "cid-private"
#define IUT_CFG_SECURITY_STR    "security"
#define IUT_CFG_TO_URI_STR      "to-uri"
#define IUT_CFG_TO_MSG_URI_STR  "to-msg-uri"
#define IUT_CFG_FORWARD_URI_STR "forward-uri"
#define IUT_CFG_XFER_URI_STR    "xfer-uri"
#define IUT_CFG_PCMU_STR        "pcmu"
#define IUT_CFG_PCMA_STR        "pcma"
#define IUT_CFG_G729_STR        "g729"
#define IUT_CFG_G726_STR        "g726-32"
#define IUT_CFG_ILBC_STR        "ilbc"
#define IUT_CFG_SILK_STR        "silk"
#define IUT_CFG_G722_STR        "g722"
#define IUT_CFG_DTMFR_STR       "dtmf"
#define IUT_CFG_CN_STR          "cn"
#define IUT_CFG_H264_STR        "h264"
#define IUT_CFG_H263_STR        "h263"
#define IUT_CFG_AMRNB_STR       "amrnb"
#define IUT_CFG_AMRWB_STR       "amrwb"
#define IUT_CFG_PPCMU_STR       "ppcmu"
#define IUT_CFG_PPCMA_STR       "ppcma"
#define IUT_CFG_PG729_STR       "pg729"
#define IUT_CFG_PG726_STR       "pg726"
#define IUT_CFG_PILBC_STR       "pilbc"
#define IUT_CFG_PSILK_STR       "psilk"
#define IUT_CFG_PG722_STR       "pg722"
#define IUT_CFG_PDTMFR_STR      "pdtmf"
#define IUT_CFG_PCN_STR         "pcn"
#define IUT_CFG_PH264_STR       "ph264"
#define IUT_CFG_PH263_STR       "ph263"
#define IUT_CFG_PAMRNB_STR      "pamrnb"
#define IUT_CFG_PAMRWB_STR      "pamrwb"
#define IUT_CFG_ISEMERGENCY_STR "isEmergency"
#define IUT_CFG_IMEI_STR        "imeiUri"
#define IUT_CFG_AUDIO_PORT_STR  "audioPortNumber"
#define IUT_CFG_VIDEO_PORT_STR  "videoPortNumber"
#define IUT_CFG_IF_ADDRESS_STR  "interfaceAddress"

typedef enum {
    IUT_CFG_ID_F = 0,
    IUT_CFG_NAME_F,
    IUT_CFG_PROXY_F,
    IUT_CFG_STUN_F,
    IUT_CFG_RELAY_F,
    IUT_CFG_XCAP_F,
    IUT_CFG_CHAT_F,
    IUT_CFG_OUTBOUND_F,
    IUT_CFG_USERNAME_F,
    IUT_CFG_PASSWORD_F,
    IUT_CFG_REALM_F,
    IUT_CFG_URI_F,
    IUT_CFG_CID_PRIVATE_F,
    IUT_CFG_SECURITY_F,
    IUT_CFG_TO_URI_F,
    IUT_CFG_TO_MSG_URI_F,
    IUT_CFG_FORWARD_URI_F,
    IUT_CFG_XFER_URI_F,
    IUT_CFG_PCMU_F,
    IUT_CFG_PCMA_F,
    IUT_CFG_G729_F,
    IUT_CFG_G726_F,
    IUT_CFG_ILBC_F,
    IUT_CFG_SILK_F,
    IUT_CFG_G722_F,
    IUT_CFG_DTMFR_F,
    IUT_CFG_CN_F,
    IUT_CFG_H264_F,
    IUT_CFG_H263_F,
    IUT_CFG_AMRNB_F,
    IUT_CFG_AMRWB_F,
    IUT_CFG_PPCMU_F,
    IUT_CFG_PPCMA_F,
    IUT_CFG_PG729_F,
    IUT_CFG_PG726_F,
    IUT_CFG_PILBC_F,
    IUT_CFG_PSILK_F,
    IUT_CFG_PG722_F,
    IUT_CFG_PDTMFR_F,
    IUT_CFG_PCN_F,
    IUT_CFG_PH264_F,
    IUT_CFG_PH263_F,
    IUT_CFG_PAMRNB_F,
    IUT_CFG_PAMRWB_F,
    IUT_CFG_EMERGENCY_F,
    IUT_CFG_IMEI_F,
    IUT_CFG_AUDIO_PORT_F,
    IUT_CFG_VIDEO_PORT_F,
    IUT_CFG_IF_ADDRESS_F,
    IUT_CFG_LAST_F,
} IUT_CfgField;

typedef struct {
    ISI_Id  serviceId;
    char    name[IUT_CFG_STR_SZ + 1];
    char    proxy[IUT_CFG_STR_SZ + 1];
    char    stun[IUT_CFG_STR_SZ + 1];
    char    relay[IUT_CFG_STR_SZ + 1];
    char    xcap[IUT_CFG_STR_SZ + 1];
    char    chat[IUT_CFG_STR_SZ + 1];
    char    outbound[IUT_CFG_STR_SZ + 1];
    char    username[IUT_CFG_STR_SZ + 1];
    char    password[IUT_CFG_STR_SZ + 1];
    char    realm[IUT_CFG_STR_SZ + 1];
    char    uri[IUT_CFG_STR_SZ + 1];
    vint    cidPrivate;
    vint    security;
    char    toUri[IUT_CFG_STR_SZ + 1];
    char    toMsgUri[IUT_CFG_STR_SZ + 1];
    char    forwardUri[IUT_CFG_STR_SZ + 1];
    char    xferUri[IUT_CFG_STR_SZ + 1];
    char    imeiUri[IUT_CFG_STR_SZ + 1];
    vint    pcmu;
    vint    pcma; 
    vint    g726; 
    vint    g729; 
    vint    iLBC; 
    vint    silk; 
    vint    g722; 
    vint    dtmfr;
    vint    cn;
    vint    h264;
    vint    h263;
    vint    amrnb;
    vint    amrwb;
    vint    ppcmu;
    vint    ppcma; 
    vint    pg726; 
    vint    pg729; 
    vint    piLBC; 
    vint    psilk;
    vint    pg722; 
    vint    pdtmfr;
    vint    pcn;
    vint    ph264;
    vint    ph263;
    vint    pamrnb;
    vint    pamrwb;
    vint    isEmergency;
    vint    audioPortNumber;
    vint    videoPortNumber;
    char    interfaceAddress[IUT_CFG_STR_SZ + 1];
} _IUT_Cfg;


typedef void (*_IUT_CfgHandler)(
    _IUT_Cfg *cfg_ptr,
    char     *value_ptr);

typedef struct {
    IUT_CfgField    field;
    char           *str_ptr;
    _IUT_CfgHandler handler;
} _IUT_CfgEntry;

void IUT_cfgInit(void);

vint IUT_cfgWrite(
    char *cFile_ptr);

vint IUT_cfgRead(
    char *cFile_ptr);

vint IUT_cfgSetField(
    ISI_Id       serviceId,
    IUT_CfgField field,
    char        *value_ptr);

char* IUT_cfgGetStrField(
    ISI_Id       serviceId,
    IUT_CfgField field);

vint IUT_cfgGetIntField(
    ISI_Id       serviceId,
    IUT_CfgField field);

vint IUT_cfgSetServiceId(
    ISI_Id serviceId);

void IUT_cfgClearServiceId(
    ISI_Id serviceId);

#endif
