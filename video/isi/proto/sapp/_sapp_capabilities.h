/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 12486 $ $Date: 2010-07-07 17:10:49 -0500 (Wed, 07 Jul 2010) $
 */

#ifndef _SAPP_CAPS_H_
#define _SAPP_CAPS_H_

/*
 * Permitted values for _SAPP_CapabilityBitmapType
 */
typedef enum  {
    SAPP_CAPS_NONE                     = eSIP_CAPS_NONE,
    SAPP_CAPS_DISCOVERY_VIA_PRESENCE   = eSIP_CAPS_DISCOVERY_VIA_PRESENCE,
    SAPP_CAPS_IP_VOICE_CALL            = eSIP_CAPS_IP_VOICE_CALL,
    SAPP_CAPS_IP_VIDEO_CALL            = eSIP_CAPS_IP_VIDEO_CALL,
    SAPP_CAPS_MESSAGING                = eSIP_CAPS_MESSAGING,
    SAPP_CAPS_SMS                      = eSIP_CAPS_SMS,
    SAPP_CAPS_FILE_TRANSFER            = eSIP_CAPS_FILE_TRANSFER,
    SAPP_CAPS_IMAGE_SHARE              = eSIP_CAPS_IMAGE_SHARE,
    SAPP_CAPS_VIDEO_SHARE              = eSIP_CAPS_VIDEO_SHARE,
    SAPP_CAPS_VIDEO_SHARE_WITHOUT_CALL = eSIP_CAPS_VIDEO_SHARE_WITHOUT_CALL,
    SAPP_CAPS_CHAT                     = eSIP_CAPS_CHAT,
    SAPP_CAPS_SOCIAL_PRESENCE          = eSIP_CAPS_SOCIAL_PRESENCE, 
    SAPP_CAPS_GEOLOCATION_PUSH         = eSIP_CAPS_GEOLOCATION_PUSH,
    SAPP_CAPS_GEOLOCATION_PULL         = eSIP_CAPS_GEOLOCATION_PULL,
    SAPP_CAPS_FILE_TRANSFER_HTTP       = eSIP_CAPS_FILE_TRANSFER_HTTP,
    SAPP_CAPS_FILE_TRANSFER_THUMBNAIL  = eSIP_CAPS_FILE_TRANSFER_THUMBNAIL,
    SAPP_CAPS_FILE_TRANSFER_STORE_FWD  = eSIP_CAPS_FILE_TRANSFER_STORE_FWD,
    /* Used for emergency registration. */
    SAPP_CAPS_EMERGENCY_REG            = eSIP_CAPS_EMERGENCY_REG,
    SAPP_CAPS_RCS_TELEPHONY_NONE       = eSIP_CAPS_RCS_TELEPHONY_NONE,
    SAPP_CAPS_RCS_TELEPHONY_CS         = eSIP_CAPS_RCS_TELEPHONY_CS,
    SAPP_CAPS_RCS_TELEPHONY_VOLTE      = eSIP_CAPS_RCS_TELEPHONY_VOLTE,
    SAPP_CAPS_RCS_TELEPHONY_VOHSPA     = eSIP_CAPS_RCS_TELEPHONY_VOHSPA,
    /* Used for supporting SRVCC features. */
    SAPP_CAPS_SRVCC_ALERTING           = eSIP_CAPS_SRVCC_ALERTING,
    SAPP_CAPS_SRVCC_MID_CALL           = eSIP_CAPS_SRVCC_MID_CALL,
} tCapabilityBitmapType;

#define SAPP_CAPS_FIRST     ((tCapabilityBitmapType) SAPP_CAPS_DISCOVERY_VIA_PRESENCE)
#define SAPP_CAPS_LAST      ((tCapabilityBitmapType) SAPP_CAPS_FILE_TRANSFER_STORE_FWD)

/*
 * an identifier 'C language' type associated with the type/kind of capability
 * that is specified (e.g.-IMS Application Reference Identifier (IARI), IMS
 * Communication Service Identifier (ICSI), 3GPP tag, etc.)
 */
typedef uint16 tCapabilitiesIdType;

/* contains strings associated with the tCapabilitiesIdType identifiers */
extern const char* capabilitiesTypeStr[];


#define SAPP_MAX_CAPABILITIES_XML_STRING_SZ          32
#define SAPP_MAX_CAPABILITIES_VERSION_STRING_SZ      5


/*
 * type used for storing each of the possible capability values
 */
typedef struct {
    tCapabilityBitmapType maskValue;
    char                  xmlKey[SAPP_MAX_CAPABILITIES_XML_STRING_SZ];
    tCapabilitiesIdType   optionsIdType;
    char                  optionsName[SAPP_LONG_STRING_SZ];
    tCapabilitiesIdType   presenceIdType;
    char                  presenceName[SAPP_LONG_STRING_SZ];
    char                  presenceVersion[SAPP_MAX_CAPABILITIES_VERSION_STRING_SZ];
} _SAPP_CapabilitiesTableEntry;

void _SAPP_isiCapabilitiesSetCmd(
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr);

void _SAPP_isiCapabilitiesRequestCmd(
    ISIP_Message        *cmd_ptr,
    SAPP_SipObj         *sip_ptr,
    ISIP_Message        *isi_ptr);

void _SAPP_capabilitiesOptionsEvent(
    SAPP_ServiceObj     *service_ptr,
    ISIP_Message        *isi_ptr,
    tUaAppEvent         *evt_ptr,
    SAPP_Event          *isievt_ptr,
    SAPP_SipObj         *sip_ptr);

#ifdef SIP_CUSTOM_CAPABILITY_EXCHANGE
int _SAPP_capabilitiesMessageEvent(
    SAPP_ServiceObj     *service_ptr,
    tUaAppEvent         *evt_ptr,
    SAPP_Event          *isievt_ptr);
#endif

const _SAPP_CapabilitiesTableEntry* _SAPP_findCapabilityInfoByName(
    const char          *capName_ptr);

const _SAPP_CapabilitiesTableEntry* _SAPP_findCapabilityInfoByBitmask(
    const tCapabilityBitmapType capBitmap);

const char* _SAPP_findCapabilityXmlAttrNameByBitmask(
    const tCapabilityBitmapType capBitmap);

const char* getCapabilityTypes(int type);

uint32 _SAPP_capabilitiesStringToBitmap(
    const char *capsString_ptr);

#endif /* _SAPP_CAPS_H_ */
