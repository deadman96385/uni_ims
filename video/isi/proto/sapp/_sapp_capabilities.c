/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 13916 $ $Date: 2011-01-29 15:54:59 -0600 (Sat, 29 Jan 2011) $
 */
#include <osal_types.h>
#include <osal.h>
#include <osal_msg.h>

#include <ezxml.h>

#include <sip_sip.h>
#include <sip_xport.h>
#include <sip_ua.h>
#include <sip_app.h>
#include <sip_debug.h>


#include "isi.h"
#include "isip.h"

#include "_sapp.h"
#ifdef INCLUDE_SIMPLE
#include "_simple.h"
#endif // SIMPLE
#include "_sapp_xml.h"
#include "_sapp_capabilities.h"
#include "_sapp_ipsec.h"
#include "_sapp_dialog.h"

/* capabilitiesTypeStr identifiers used as array indices, except for INVALID */
#define ORG_GSMA                    ((tCapabilitiesIdType) 0)
#define ICSI                        ((tCapabilitiesIdType) 1)
#define ORG_3GPP_URN                ((tCapabilitiesIdType) 2)
#define IARI                        ((tCapabilitiesIdType) 3)
#define ORG_OPENMOBILEALLIANCE      ((tCapabilitiesIdType) 4)
#define PLUS_3GPP                   ((tCapabilitiesIdType) 5)
#define INVALID                     ((tCapabilitiesIdType) 99)

const char* capabilitiesTypeStr[] = {
    "org.gsma.",
    "+g.3gpp.icsi-ref=",
    "org.3gpp.urn:",
    "+g.3gpp.iari-ref=",
    "org.openmobilealliance:",
    "+g.3gpp."
};

#define SAPP_MAX_CAPABILITIES_TYPES \
    ((int)(sizeof(capabilitiesTypeStr)/sizeof(capabilitiesTypeStr[0])))

static const char SAPP_OPTIONS_ACCEPT_CONTACT[] = "*;+g.oma.sip-im";

/*
 * The capabilities table specifies each of the strings that may appear in
 * messages conveying a device's capabilities.
 * Strings are specified which can be used with either the PRESENCE
 * or OPTIONS methods to communicate capabilities.
 */
const _SAPP_CapabilitiesTableEntry capabilitiesTable[] = {
    /* Dummy placeholder at array index 0 used to simplify other coding */
    {SAPP_CAPS_NONE, "dummy",
        INVALID, "dummy",
        INVALID, "dummy", "-1"},

    /* Discovery Via Presence */
    {SAPP_CAPS_DISCOVERY_VIA_PRESENCE, "discovery via presence",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.dp",
        ORG_3GPP_URN, "urn-7:3gpp-application.ims.iari.rcse.dp", "1.0"},

    /* IP Voice Call */
    {SAPP_CAPS_IP_VOICE_CALL, "ip voice call",
        ICSI, "urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel",
        ORG_3GPP_URN, "urn-7:3gpp-service.ims.icsi.mmtel", "1.0"},

    /* IP Video Call (includes IP Voice Call) */
    {SAPP_CAPS_IP_VIDEO_CALL, "ip video call",
        ICSI, "urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel;video",
        ORG_3GPP_URN, "urn-7:3gpp-service.ims.icsi.mmtel", "1.0"},

    /* Standalone Messaging (SMS/MMS --> Converged IP Messaging) */
    {SAPP_CAPS_MESSAGING, "messaging",
        ICSI, "urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.msg;" \
              "urn%3Aurn-7%3A3gpp-service.ims.icsi.oma.cpm.largemsg",
        ORG_3GPP_URN, "urn-7:3gpp-application.ims.iari.rcs.sm", "1.0"},

    /* File Transfer */
    {SAPP_CAPS_FILE_TRANSFER, "file transfer",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.ft",
        ORG_OPENMOBILEALLIANCE, "File-Transfer", "1.0"},

    /* Image Share */
    {SAPP_CAPS_IMAGE_SHARE, "image share",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.gsma-is",
        ORG_GSMA, "imageshare", "1.0"},

    /* Video Share */
    {SAPP_CAPS_VIDEO_SHARE, "video share",
        PLUS_3GPP, "cs-voice",
        ORG_GSMA, "videoshare", "1.0"},

    /* Video Share (outside context of a call) */
    {SAPP_CAPS_VIDEO_SHARE_WITHOUT_CALL, "video share without call",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.gsma-vs",
        ORG_GSMA, "videoshare", "2.0"},

    /* Instant Messaging/Chat */
    {SAPP_CAPS_CHAT, "chat",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.im",
        ORG_OPENMOBILEALLIANCE, "IM-session", "1.0"},

    /* Social Presence */
    {SAPP_CAPS_SOCIAL_PRESENCE, "social presence",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.sp",
        ORG_3GPP_URN, "urn-7:3gpp-application.ims.iari.rcse.sp", "1.0"},

    /* Geolocation Push */
    {SAPP_CAPS_GEOLOCATION_PUSH, "geolocation push",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopush",
        ORG_3GPP_URN, "urn-7:3gpp-application.ims.iari.rcs.geopush", "1.0"},

    /* Geolocation Pull */
    {SAPP_CAPS_GEOLOCATION_PULL, "geolocation pull",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopull",
        ORG_3GPP_URN, "urn-7:3gpp-application.ims.iari.rcs.geopull", "1.0"},

    /* File Transfer via HTTP */
    {SAPP_CAPS_FILE_TRANSFER_HTTP, "file transfer via http",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.fthttp",
        ORG_OPENMOBILEALLIANCE, "File-Transfer-HTTP", "1.0"},

    /* File Transfer Thumbnail */
    {SAPP_CAPS_FILE_TRANSFER_THUMBNAIL, "file transfer with thumbnail",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.ftthumb",
        ORG_OPENMOBILEALLIANCE, "File-Transfer-thumb", "2.0"},

    /* File Transfer Thumbnail */
    {SAPP_CAPS_FILE_TRANSFER_STORE_FWD, "file transfer store and forward",
        IARI, "urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.ftstandfw",
        ORG_OPENMOBILEALLIANCE, "File-Transfer-standfw", "2.0"},

};

#define SAPP_MAX_SUPPORTED_CAPABILITIES \
    ((int)(sizeof(capabilitiesTable)/sizeof(capabilitiesTable[0])))

/* forward declaration for _SAPP_isiCapabilitiesEvt() */
static void _SAPP_isiCapabilitiesEvt(
    ISI_Id              presId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_PresReason     reason,
    const char         *to_ptr,
    const char         *from_ptr,
    ISIP_Message       *isi_ptr);


/* forward declaration for _SAPP_cacheCapabilities() */
static vint _SAPP_cacheCapabilities(
        const char      *caps_ptr,
        SAPP_ServiceObj *service_ptr);
/*
 * In order to build the final string of capabilities, the following types
 * and variables are used to assist in the parsing.
 * _capOptionsString can be envisioned to look like the following structure:
 * INDEX                     ARRAY ENTRY
 * -------------             --------------------
 * (ORG_GSMA)                [NULL, NULL]
 * (ICSI)                    ["+g.3gpp.icsi-ref=", -->]  ["urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel", NULL]
 * (ORG_3GPP_URN)            [NULL, NULL]
 * (IARI)                    ["+g.3gpp.iari-ref=", -->]  ["urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopush", -->] ["urn%3Aurn-7%3A3gpp-application.ims.iari.rcse.im", NULL]
 * (ORG_OPENMOBILEALLIANCE)  [NULL, NULL]
 * (PLUS_3GPP)               ["+g.3gpp.", -->] ["cs-voice", NULL]
 *
 * to represent the string:
 * "+g.3gpp.icsi-ref="urn%3Aurn-7%3A3gpp-service.ims.icsi.mmtel";+g.3gpp.iari\
 * -ref="urn%3Aurn-7%3A3gpp-application.ims.iari.rcs.geopush,urn%3Aurn-7%3A3\
 * gpp-application.ims.iari.rcse.im";+g.3gpp.cs-voice"
 *
 * NOTE that the strings shown in the example array are actually only pointers
 * to the string in the capabilitiesTable.
 */
typedef struct tag_capToken {
    /* name_ptr points to string in capabilitiesTable or NULL */
    const char             *name_ptr;
    /* next points to next token or NULL */
    struct tag_capToken    *next_ptr;
} capToken, *capToken_ptr;

/*
 * This object used to store pointers to the string fragments in the
 * capabilitiesTable.  It is used during conversions from a bitmap
 * to a string.
 *
 * This is statically over-allocating the space since there should never be
 * more _capTokens than SAPP_MAX_SUPPORTED_CAPABILITIES.  However, we allocate
 * as many entries per row/type.  Since the sizeof the _capToken is relatively
 * small, this is being done in favor of dynamic allocations.
 */
static capToken capOptionsString[SAPP_MAX_CAPABILITIES_TYPES]
                         [SAPP_MAX_SUPPORTED_CAPABILITIES];



const char* getCapabilityTypes(int type) {
    if (type <  SAPP_MAX_CAPABILITIES_TYPES) {
        return capabilitiesTypeStr[type];
    }
    return "";
}



/*
 * ======== strBuilderAddToken() ========
 *
 * Utility function used to update the capOptionsString table by
 * adding the specified token (with the type and name provided)
 * to the appropriate location in the table.
 *
 * NOTE: since capOptionsString is an array, the use of the "next_ptr"
 * is primarily used to detect used entries rather than as it would
 * be in a linked list of dis-associated memory blocks.
 *
 *
 * idType: the token's identifier type, used to determine the row in the
 *         table that the token must be added
 *
 * idName: the pointer to the string containing the token's name
 */
void strBuilderAddToken(
    tCapabilitiesIdType idType, 
    const char *idName)
{
    int column;

    for (column = 0; column < SAPP_MAX_SUPPORTED_CAPABILITIES; column++) {
        if (capOptionsString[idType][column].name_ptr == NULL) {
            /* free slot, add token here */
            capOptionsString[idType][column].name_ptr = idName;
            /* and set previous entry to point to this one */
            if (column > 0) {
                capOptionsString[idType][column-1].next_ptr =
                        &capOptionsString[idType][column];
            }
            break;
        }
    }
    return;
}

/*
 * ======== _SAPP_cmdSendOptions() ========
 * Sends this device's capabilities.
 * This function will stimulate a SIP OPTIONS request which
 * is expected to result in the peer device providing its
 * capabilities.
 *
 * pres_ptr    : ISIP_Presence object.
 *
 * service_ptr : The service object.
 *
 * isi_ptr     : The ISIP_Message object.
 *
 * Returns:
 *  SAPP_ERR: The capabilities request could not be made.
 *  SAPP_OK : The capabilities request was made.
 */
vint _SAPP_cmdSendOptions(
    ISIP_Presence   *caps_ptr,
    SAPP_ServiceObj *service_ptr,
    ISIP_Message    *isi_ptr)
{
    char *hdrFlds_ptr[SAPP_HF_MAX_NUM_SZ];
    vint  numHdrFlds = 0;
    vint  ret = SAPP_OK;
    tUaMsgBody body;

    /* Stores this device's capabilities in the service object */
    if ((ret = _SAPP_cacheCapabilities(caps_ptr->presence, service_ptr)) != SAPP_OK) {
        return (SAPP_ERR);
    }

    OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], ISI_ADDRESS_STRING_SZ,
        "%s %s", SAPP_TO_HF, caps_ptr->to);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    if ((NULL != service_ptr->telUri_ptr) &&
            (0 != service_ptr->telUri_ptr[0])) {
        OSAL_snprintf(service_ptr->hfStratch[numHdrFlds], ISI_ADDRESS_STRING_SZ,
                "%s %s", SAPP_FROM_HF, service_ptr->telUri_ptr);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;

        OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
                SAPP_STRING_SZ, "%s %s", SAPP_P_PREFERRED_ID_HF,
                service_ptr->telUri_ptr);
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    /*
     * Add a 'preconfigured route' if the registrar reported one during the
     * registration process
     */
    if (SAPP_OK == SAPP_sipAddPreconfiguredRoute(&service_ptr->registration,
            &service_ptr->hfStratch[numHdrFlds][0], SAPP_STRING_SZ)) {
        /* Then a "Route" header field was added */
        hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
        numHdrFlds++;
    }

    /* Add Accept-Contact */
    OSAL_snprintf(&service_ptr->hfStratch[numHdrFlds][0],
        SAPP_STRING_SZ, "%s %s", SAPP_ACCEPT_CONTACT_HF, SAPP_OPTIONS_ACCEPT_CONTACT);
    hdrFlds_ptr[numHdrFlds] = &service_ptr->hfStratch[numHdrFlds][0];
    numHdrFlds++;

    /* Check if we should support IPSEC and add Security-Verify HF if existed */
    if (service_ptr->sipConfig.useIpSec) {
        _SAPP_ipsecAddSecurityVerify(service_ptr, hdrFlds_ptr, &numHdrFlds);
        _SAPP_ipsecAddSecAgreeRequire(service_ptr, hdrFlds_ptr, &numHdrFlds);
    }
    
    /*
     * Send OPTIONS
     */
#ifdef SIP_CUSTOM_CAPABILITY_EXCHANGE
    if (NULL != OSAL_strscan(caps_ptr->to, SIP_CUSTOM_CAPABILITY_EXCHANGE)) {
        body.pBody = SIP_CUSTOM_CAPABILITY_EXCHANGE;
        body.length = sizeof(SIP_CUSTOM_CAPABILITY_EXCHANGE) - 1;
        ret = UA_Message(
                service_ptr->sipConfig.uaId,
                NULL,
                &service_ptr->transObj.hTransaction,
                hdrFlds_ptr,
                numHdrFlds,
                &body,
                service_ptr->exchangeCapabilitiesBitmap,
                &service_ptr->sipConfig.localConn);
    }
    else {
        ret = UA_Options(
            service_ptr->sipConfig.uaId,
            NULL,
            &service_ptr->transObj.hTransaction,
            hdrFlds_ptr,
            numHdrFlds,
            NULL,
            service_ptr->exchangeCapabilitiesBitmap,
            &service_ptr->sipConfig.localConn);
    }
#else
    ret = UA_Options(
            service_ptr->sipConfig.uaId,
            NULL,
            &service_ptr->transObj.hTransaction,
            hdrFlds_ptr,
            numHdrFlds,
            NULL,
            service_ptr->exchangeCapabilitiesBitmap,
            &service_ptr->sipConfig.localConn);

#endif
    /* convert SIP return value to SAPP return value */
    if (ret != SIP_OK) {
        ret = SAPP_ERR;
    }
    else {
        ret = SAPP_OK;
    }

    return (ret);

}



/*
 * ======== _SAPP_isiCapabilitiesSetCmd() ========
 * Stores this device's capabilities.
 * This function will notify the SIP UA with the set
 * of capabilities that the SIP UA should use in
 * response to SIP OPTIONS query.
 *
 * service_ptr : The service object.
 *
 * s_ptr     : The ISIP_Message.service object.
 *
 */
void _SAPP_isiCapabilitiesSetCmd(
    SAPP_ServiceObj *service_ptr,
    ISIP_Service    *s_ptr)
{
    /* Stores this device's capabilities in the service object */
    if (SAPP_OK != _SAPP_cacheCapabilities(s_ptr->settings.capabilities, service_ptr)) {
        return;
    }

    if (SIP_OK != UA_Modify(
            service_ptr->sipConfig.uaId, /* hUa */
            NULL, /* no proxy update */
            NULL, /* no outbound proxy update */
            NULL, /* no Wimax proxy update */
            NULL, /* no reg proxy update */
            NULL, /* no FQDN update */
            NULL, /* no AOR update */
            NULL, /* no auth credentials update */
            NULL, /* no codec update */
            0,    /* no packet rate update */
            service_ptr->exchangeCapabilitiesBitmap)) { /* new set of capabilities */
    }
    return;
}


/*
 * ======== _SAPP_cacheCapabilities() ========
 * Stores this device's capabilities in the service object.
 *
 * caps_ptr    : ISIP_Presence object.
 *
 * service_ptr : The service object.
 *
 * Returns:
 *  SAPP_OK, or SAPP_ERR if fails
 */
static vint _SAPP_cacheCapabilities(
    const char      *caps_ptr,
    SAPP_ServiceObj *service_ptr)
{
    OSAL_snprintf(service_ptr->payloadStratch, SAPP_PAYLOAD_SZ,
            "%s", caps_ptr);

    /* Cache the capabilities info */
    if (SAPP_OK != (_SAPP_xmlDecodeIsiCapabilitiesDoc(
            service_ptr->payloadStratch,
            OSAL_strlen(service_ptr->payloadStratch),
            &service_ptr->exchangeCapabilitiesBitmap))) {
        /* The XML document could not be understood, return error */
        return (SAPP_ERR);
    }

    return (SAPP_OK);
}


/*
 * ======== _SAPP_isiCapabilitiesRequestCmd() ========
 * Processes the command to request another device's
 * capabilities.
 *
 * cmd_ptr    : ISIP_Message object received from ISI containing the
 *              details about the command.
 *
 * sip_ptr    : The SAPP_SipObj object.
 *
 * isi_ptr    : The ISIP_Message object pointer, which points to a buffer area
 *              that is used to write ISI events to.  Events that are written
 *              to this buffer are ultimately sent up to the ISI layer.
 *
 * Returns:
 *  nothing.  Sends event to ISI indicating either
 *  ISIP_PRES_REASON_ERROR or ISIP_PRES_REASON_COMPLETE
 */
void _SAPP_isiCapabilitiesRequestCmd(
    ISIP_Message        *cmd_ptr,
    SAPP_SipObj         *sip_ptr,
    ISIP_Message        *isi_ptr)
{
    ISIP_Presence   *p_ptr;
    SAPP_ServiceObj *service_ptr;
    vint             ret;

    p_ptr = &cmd_ptr->msg.presence;

    service_ptr = SAPP_findServiceViaServiceId(sip_ptr, p_ptr->serviceId);
    if (service_ptr == NULL) {
        isi_ptr->id = cmd_ptr->id;
        isi_ptr->code = ISIP_CODE_PRESENCE;
        isi_ptr->protocol = cmd_ptr->protocol;
        isi_ptr->msg.presence.reason = ISIP_PRES_REASON_ERROR;
        isi_ptr->msg.presence.serviceId = p_ptr->serviceId;
        isi_ptr->msg.presence.to[0] = 0;
        isi_ptr->msg.presence.from[0] = 0;
        return;
    }

    if (sip_ptr->capDiscoveryMethod ==
            SAPP_CAPABILITIES_DISCOVERY_OPTIONS) {
        ret = _SAPP_cmdSendOptions(p_ptr, service_ptr, isi_ptr);
    }
#ifdef INCLUDE_SIMPLE
    else if (sip_ptr->capDiscoveryMethod ==
                SAPP_CAPABILITIES_DISCOVERY_PRESENCE) {

        /*
         * fill the service object with our capability info in case
         * it is needed to send SIP OPTIONS, which will be the case
         * if the PRESENCE method for fetching capabilities fails
         */
        _SAPP_cacheCapabilities(p_ptr->presence, service_ptr);

        /* convert the reason in the message to a Subscription Request */
        p_ptr->reason = ISIP_PRES_REASON_SUB_TO_PRES;
        /*
         * and perform an Anonymous Fetch.  The URI specified by RCS 5.0
         * is that specified in RFC3323
         */
        OSAL_snprintf(p_ptr->from, ISI_ADDRESS_STRING_SZ, "%s",
            SAPP_ANONYMOUS_URI);
        SIMPL_isiPresCmd(cmd_ptr, sip_ptr, isi_ptr);

        /* convert the reason in the message to cause a PUBLISH */
        p_ptr->reason = ISIP_PRES_REASON_PRESENCE;
        /* modify setting to cause PUBLISH instead of NOTIFY */
        service_ptr->simple.useStorageServer = OSAL_TRUE;
        SIMPL_isiPresCmd(cmd_ptr, sip_ptr, isi_ptr);
        ret = SAPP_OK;
    }
#endif
    else {
        /*
         * discovery method is SAPP_CAPABILITIES_DISCOVERY_NONE or is
         * invalid, so ensure an error is returned
         */
        ret = SAPP_ERR;
    }

    if (SAPP_OK != ret) {
        /* 
         * Tell ISI about the error, otherwise wait until a response is
         * received from SIP to notify ISI 
         */
        _SAPP_isiCapabilitiesEvt(cmd_ptr->id, p_ptr->serviceId,
                service_ptr->protocolId, ISIP_PRES_REASON_ERROR,
                NULL, NULL, isi_ptr);
    }
    else {
        /* 
         * store ISI's identifier for this transaction so ISI may
         * be notified later when the SIP response arrives 
         */
        service_ptr->transObj.id = cmd_ptr->id;
    }

}

/*
 * ======== _SAPP_isiCapabilitiesEvt() ========
 *
 * This function is used by various other functions to populate a ISI event
 * for "capabilities" related events. These events will be passed
 * from SAPP to the ISI module.
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_isiCapabilitiesEvt(
    ISI_Id              presId,
    ISI_Id              serviceId,
    vint                protocolId,
    ISIP_PresReason     reason,
    const char         *to_ptr,
    const char         *from_ptr,
    ISIP_Message       *isi_ptr)
{
    isi_ptr->id = presId;
    isi_ptr->code = ISIP_CODE_PRESENCE;
    isi_ptr->protocol = protocolId;
    isi_ptr->msg.presence.reason = reason;
    isi_ptr->msg.presence.serviceId = serviceId;
    if (NULL != to_ptr) {
        OSAL_snprintf(isi_ptr->msg.presence.to, ISI_ADDRESS_STRING_SZ, "%s",
                to_ptr);
    }
    else {
        isi_ptr->msg.presence.to[0] = 0;
    }
    if (NULL != from_ptr) {
        OSAL_snprintf(isi_ptr->msg.presence.from, ISI_ADDRESS_STRING_SZ, "%s",
                from_ptr);
    }
    else {
        isi_ptr->msg.presence.from[0] = 0;
    }
    return;
}

/*
 * ======== _SAPP_capabilitiesOptionsEvent() ========
 * This function handles events which are generated that relate to
 * capabilities exchanged via SIP OPTIONS, which is specified in
 * Rich Communications Suite (RCS) release 5.0.
 *
 * If the received event is to handle an incoming SIP OPTIONS request,
 * the capabilities string that arrived in the Contact: header
 * is converted to a small XML file which is then provided to ISI
 * so that the upper layers can associate the set of capabilities
 * with the remote party (also specified in the event).
 *
 * If the event received signifies a response to SIP OPTIONs which
 * this device initially sent (that is, the transaction ID matches
 * that of one we created when sending), then two events are
 * provided to ISI - the first being a status of the request and the
 * second will indicate the remote party's capabilities (as in the
 * case of the inbound SIP OPTIONS described above).
 *
 *
 * service_ptr -
 *
 * isi_ptr -
 *
 * evt_ptr - contains event info, including capabilities string,
 *     remote party address, and transaction ID (which can be
 *     compared to the recent transaction initiated by this device)
 *
 * Returns:
 *   Nothing.
 */
void _SAPP_capabilitiesOptionsEvent(
    SAPP_ServiceObj     *service_ptr,
    ISIP_Message        *isi_ptr,
    tUaAppEvent         *evt_ptr,
    SAPP_Event          *isievt_ptr,
    SAPP_SipObj         *sip_ptr)
{
    ISIP_PresReason reason = ISIP_PRES_REASON_INVALID;
    ISIP_Presence  *presence_ptr = &isi_ptr->msg.presence;
    uint32          capsBitmap;
    char           *reason_ptr; 
    if (evt_ptr == NULL) {
        /* no event to process */
        return;
    }

    if (evt_ptr->header.type == eUA_OPTIONS) {

        capsBitmap = evt_ptr->capabilitiesBitmap;
        /*
         * If no capabilities were specified, even though a response
         * was received, then mark the capabilities as "unknown".
         * Otherwise, they are considered "valid".
         */
        if (capsBitmap == SAPP_CAPS_NONE) {
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "unknown",
                presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }
        else {
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "valid",
                presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }

        _SAPP_isiCapabilitiesEvt(0,
                service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_PRES_REASON_CAPABILITIES,
                evt_ptr->szToUri, evt_ptr->szRemoteUri, isi_ptr);

    }
    /*
     * The following handles the case of a response to capabilities sent via
     * SIP OPTIONS.  If the transaction ID matches that of the one which
     * was created when the SIP OPTIONS were sent, we have our response
     * and must notify ISI.
     */
    else if (evt_ptr->resp.hTransaction == service_ptr->transObj.hTransaction) {

        /*
         * In the case of CapEx, any response is considered a successful
         * completion so we set the reason to 'ISIP_PRES_REASON_COMPLETE'.
         * This is because even errors end up meaning 'non-RCS' or 'Unknown'
         * Capabilities. A CapEx 'failure' only happens if the request
         * could not event be sent.
         */
        reason = ISIP_PRES_REASON_COMPLETE;

        capsBitmap = evt_ptr->capabilitiesBitmap;
        if ((evt_ptr->resp.respCode == 408) ||
            (evt_ptr->resp.respCode == 480)) {
            /* 408 request timeout */
            /* 480 temporarily unavailable */
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "unknown",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }
        else if (evt_ptr->resp.respCode == 404) {
            /* 408 request timeout */

            /* consider a Non-RCS user, per RCS 5.0 spec */
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "non-RCS",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }
        else if (evt_ptr->resp.respCode == SIP_RSP_SERVER_TIMEOUT) {
            /* 504 Server Timeout. */
            _SAPP_sipServerTimeOut(service_ptr, &reason_ptr,
                    evt_ptr, sip_ptr);
        }
        else {
            /*
             * Valid response.
             * If no capabilities were specified, even though a response
             * was received, then mark the capabilities as "unknown".
             * Otherwise, they are considered "valid".
             */
            if (capsBitmap == SAPP_CAPS_NONE) {
                _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "unknown",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
            }
            else {
                _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "valid",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
            }
        }

        /*
         * build up event to send to ISI indicating status of the
         * SIP OPTIONS request this device initiated
         */
        _SAPP_isiCapabilitiesEvt(service_ptr->transObj.id,
                service_ptr->isiServiceId, service_ptr->protocolId, reason,
                evt_ptr->szToUri, evt_ptr->szRemoteUri, isi_ptr);


        /*
         * Send the event
         * This additional call to send the event is required in this
         * case where we are sending a second event up to ISI.
         */
        SAPP_sendEvent(isievt_ptr);

        /*
         * build up event to send to ISI indicating capabilities of the
         * remote party that responded to our SIP OPTIONS request.
         */
        _SAPP_isiCapabilitiesEvt(0,
                service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_PRES_REASON_CAPABILITIES,
                evt_ptr->szToUri, evt_ptr->szRemoteUri, isi_ptr);

        service_ptr->transObj.hTransaction = 0;
    }

    return;
}

#ifdef SIP_CUSTOM_CAPABILITY_EXCHANGE
int _SAPP_capabilitiesMessageEvent(
    SAPP_ServiceObj     *service_ptr,
    tUaAppEvent         *evt_ptr,
    SAPP_Event          *isievt_ptr)
{
    ISIP_PresReason  reason = ISIP_PRES_REASON_INVALID;
    ISIP_Message    *isi_ptr = &isievt_ptr->isiMsg;
    ISIP_Presence   *presence_ptr = &isi_ptr->msg.presence;
    uint32 capsBitmap;

    if (evt_ptr == NULL) {
        /* no event to process */
        return (SAPP_ERR);
    }

    if (evt_ptr->header.type == eUA_TEXT_MESSAGE) {

        if (0 == evt_ptr->msgBody.payLoad.length) {
            return (SAPP_ERR);
        }

        if (0 != OSAL_strncmp(evt_ptr->msgBody.payLoad.data,
                SIP_CUSTOM_CAPABILITY_EXCHANGE, sizeof(SIP_CUSTOM_CAPABILITY_EXCHANGE)-1)) {
            return (SAPP_ERR);
        }

        capsBitmap = evt_ptr->capabilitiesBitmap;
        /*
         * If no capabilities were specified, even though a response
         * was received, then mark the capabilities as "unknown".
         * Otherwise, they are considered "valid".
         */
        if (capsBitmap == SAPP_CAPS_NONE) {
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "unknown",
                presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }
        else {
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "valid",
                presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }

        _SAPP_isiCapabilitiesEvt(0,
                service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_PRES_REASON_CAPABILITIES,
                evt_ptr->szToUri, evt_ptr->szRemoteUri, isi_ptr);
        return (SAPP_OK);
    }
    /*
     * The following handles the case of a response to capabilities sent via
     * SIP OPTIONS.  If the transaction ID matches that of the one which
     * was created when the SIP OPTIONS were sent, we have our response
     * and must notify ISI.
     */
    else if (evt_ptr->resp.hTransaction == service_ptr->transObj.hTransaction) {

        /*
         * In the case of CapEx, any response is considered a successful
         * completion so we set the reason to 'ISIP_PRES_REASON_COMPLETE'.
         * This is because even errors end up meaning 'non-RCS' or 'Unknown'
         * Capabilities. A CapEx 'failure' only happens if the request
         * could not event be sent.
         */
        reason = ISIP_PRES_REASON_COMPLETE;

        capsBitmap = evt_ptr->capabilitiesBitmap;

        if ((evt_ptr->resp.respCode == 408) ||
            (evt_ptr->resp.respCode == 480)) {
            /* 408 request timeout */
            /* 480 temporarily unavailable */
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "unknown",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }
        else if (evt_ptr->resp.respCode == 404) {
            /* 408 request timeout */

            /* consider a Non-RCS user, per RCS 5.0 spec */
            _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "non-RCS",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
        }
        else {
            /*
             * Valid response.
             * If no capabilities were specified, even though a response
             * was received, then mark the capabilities as "unknown".
             * Otherwise, they are considered "valid".
             */
            if (capsBitmap == SAPP_CAPS_NONE) {
                _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "unknown",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
            }
            else {
                _SAPP_xmlEncodeCapabilitiesIsiDoc(capsBitmap, "valid",
                    presence_ptr->presence, ISI_PRESENCE_STRING_SZ);
            }
        }

        /*
         * build up event to send to ISI indicating status of the
         * SIP OPTIONS request this device initiated
         */
        _SAPP_isiCapabilitiesEvt(service_ptr->transObj.id,
                service_ptr->isiServiceId, service_ptr->protocolId, reason,
                evt_ptr->szToUri, evt_ptr->szRemoteUri, isi_ptr);

        /*
         * Send the event
         * This additional call to send the event is required in this
         * case where we are sending a second event up to ISI.
         */
        SAPP_sendEvent(isievt_ptr);

        /*
         * build up event to send to ISI indicating capabilities of the
         * remote party that responded to our SIP OPTIONS request.
         */
        _SAPP_isiCapabilitiesEvt(0,
                service_ptr->isiServiceId, service_ptr->protocolId,
                ISIP_PRES_REASON_CAPABILITIES,
                evt_ptr->szToUri, evt_ptr->szRemoteUri, isi_ptr);

        service_ptr->transObj.hTransaction = 0;
        return (SAPP_OK);
    }
    return (SAPP_ERR);
}
#endif


/*
 * ======== _SAPP_findCapabilityInfoByName() ========
 *
 * This function returns a pointer to the entry in the capabilitiesTable
 * which corresponds to that with the same name (i.e.-xmlKey field) as
 * the one which is supplied.
 *
 * capName_ptr    : string identifying the name (xmlKey field) of the
 *                  requested entry
 *
 * Returns:
 *   pointer to _SAPP_CapabilitiesTableEntry .
 */
const _SAPP_CapabilitiesTableEntry*
        _SAPP_findCapabilityInfoByName(const char *capName_ptr)
{
    vint x;

    /*
     * simple loop to check each entry, comparing the provided string with
     * that for the entry in the table.  exit loop when found or
     * all entries have been checked.
     */
    for (x = 0; x < SAPP_MAX_SUPPORTED_CAPABILITIES; x++) {
        if (0 == OSAL_strncmp(capName_ptr, capabilitiesTable[x].xmlKey,
                OSAL_strlen(capName_ptr))) {
            return (&capabilitiesTable[x]);
        }
    }

    return (NULL);
}



/*
 * ======== _SAPP_findCapabilityInfoByBitmask() ========
 *
 * This function returns a pointer to the entry in the capabilitiesTable
 * which corresponds to that with the same bitmask (i.e.-maskValue field) as
 * the one which is supplied.
 *
 * capBitmap    : string identifying the bitmask (maskValue field) of the
 *                requested entry
 *
 * Returns:
 *   pointer to _SAPP_CapabilitiesTableEntry .
 */
const _SAPP_CapabilitiesTableEntry*
        _SAPP_findCapabilityInfoByBitmask(const tCapabilityBitmapType capBitmap)
{
    vint x;

    /*
     * simple loop to check each entry, comparing the provided bitmap with
     * that for the entry in the table.  exit loop when found or
     * all entries have been checked.
     */
    for (x = 0; x < SAPP_MAX_SUPPORTED_CAPABILITIES; x++) {
        if (capBitmap == capabilitiesTable[x].maskValue) {
            return (&capabilitiesTable[x]);
        }
    }

    return (NULL);
}


/*
 * ======== _SAPP_findCapabilityXmlAttrNameByBitmask() ========
 *
 * This function returns a pointer to the XML attr name
 * which corresponds to that with the same bitmask (i.e.-maskValue field) as
 * the one which is supplied.
 *
 * capBitmap    : string identifying the bitmask (maskValue field) of the
 *                requested entry
 *
 * Returns:
 *   NULL if not found, or pointer to string name
 */
const char*  _SAPP_findCapabilityXmlAttrNameByBitmask(
        const tCapabilityBitmapType capBitmap)
{
    vint x;

    /*
     * simple loop to check each entry, comparing the provided bitmap with
     * that for the entry in the table.  exit loop when found or
     * all entries have been checked.
     */
    for (x = 0; x < SAPP_MAX_SUPPORTED_CAPABILITIES; x++) {
        if (capBitmap == capabilitiesTable[x].maskValue) {
            return ((char*)(&capabilitiesTable[x].xmlKey));
        }
    }

    return (NULL);
}

/*
 * ======== _SAPP_capabilitiesStringToBitmap() ========
 *
 * This function convert capabilities string to bitmap.
 *
 * capsString_ptr: NULL terminated capabilities string.
 *
 * Returns:
 *   The uint32 bitmap of capabilities.
 */
uint32 _SAPP_capabilitiesStringToBitmap(
    const char *capsString_ptr)
{
    vint idx; /* index to capabilitiesTable when looping */ 
    vint capsStringLength;
    uint32 capsBitmap;

    capsBitmap = SAPP_CAPS_NONE;
    capsStringLength = OSAL_strlen(capsString_ptr);

    /*
     * loop over each of the possible capabilities, searching for
     * the presence of that capability in the received string.
     */
    for (idx = 0; idx < SAPP_MAX_SUPPORTED_CAPABILITIES; idx++) {
        if (NULL != OSAL_strnscan(capsString_ptr, capsStringLength,
                capabilitiesTable[idx].optionsName)) {
            /* found it */
            capsBitmap |= capabilitiesTable[idx].maskValue;
        }
    }

    return (capsBitmap);
}
