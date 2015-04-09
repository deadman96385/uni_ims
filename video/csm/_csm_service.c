/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004-2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30357 $ $Date: 2014-12-11 16:11:02 +0800 (Thu, 11 Dec 2014) $
 *
 */

#include "csm_event.h"
#include <rpm.h>
#include "_csm.h"
#include "_csm_service.h"
#include "_csm_isi.h"
#include "_csm_isi_service.h"
#ifdef INCLUDE_GBA
#include "gbam.h"
#endif
#ifdef INCLUDE_SUPSRV
#include "_csm_utils.h"
#endif
#include <isi_rpc.h>
#include <settings.h>

CSM_ServiceMngr *mServiceMngr_ptr;

/*
 * ======== _CSM_serviceGetIMConfServer() ========
 * Private helper to get the chat conference server from the xml doc.
 * The xml doc could csm configuration.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
static vint _CSM_serviceGetIMConfServer(
    void            *cfg_ptr,
    CSM_ServiceMngr *serviceMngr_ptr)
{
    char *value_ptr;

    /* Get im conf server */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_IM_CONF_SERVER))) {
        OSAL_strncpy(serviceMngr_ptr->chatConfUri, value_ptr,
                sizeof(serviceMngr_ptr->chatConfUri));
        CSM_dbgPrintf("IM Factory Conference URI=%s\n",
                serviceMngr_ptr->chatConfUri);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_serviceXmlGetUrlFmtSettings() ========
 * Private helper to get the url format settings from the xml doc.
 * This function is using for RCS provisioning data only.
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
static vint _CSM_serviceXmlGetUrlFmtSettings(
    ezxml_t          xml_ptr,
    CSM_ServiceMngr *serviceMngr_ptr)
{
    char *value_ptr;

    /* Get nat url fmt */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_PARM_NAT_URL_FMT))) {
        if ('1' == value_ptr[0]) {
            serviceMngr_ptr->urlFmt.natUrlFmt = CSM_URL_FORMAT_SIP_URI;
        }
        else {
            serviceMngr_ptr->urlFmt.natUrlFmt = CSM_URL_FORMAT_TEL_URI;
        }
    }

    /* Get int url fmt */
    if (NULL != (value_ptr = SETTINGS_xmlGetParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_PARM_INT_URL_FMR))) {
        if ('1' == value_ptr[0]) {
            serviceMngr_ptr->urlFmt.intUrlFmt = CSM_URL_FORMAT_SIP_URI;
        }
        else {
            serviceMngr_ptr->urlFmt.intUrlFmt = CSM_URL_FORMAT_TEL_URI;
        }
    }

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceGetUrlFmtSettings() ========
 * Private helper to get the url format settings from the xml doc.
 * The function is using for csm configuration only.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
static vint _CSM_serviceGetUrlFmtSettings(
    void            *cfg_ptr,
    CSM_ServiceMngr *serviceMngr_ptr)
{
    char *value_ptr;

    /* Get nat url fmt */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_NAT_URL_FMT))) {
        if ('1' == value_ptr[0]) {
            serviceMngr_ptr->urlFmt.natUrlFmt = CSM_URL_FORMAT_SIP_URI;
        }
        else {
            serviceMngr_ptr->urlFmt.natUrlFmt = CSM_URL_FORMAT_TEL_URI;
        }
    }

    /* Get int url fmt */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE, NULL,
            NULL, SETTINGS_PARM_INT_URL_FMR))) {
        if ('1' == value_ptr[0]) {
            serviceMngr_ptr->urlFmt.intUrlFmt = CSM_URL_FORMAT_SIP_URI;
        }
        else {
            serviceMngr_ptr->urlFmt.intUrlFmt = CSM_URL_FORMAT_TEL_URI;
        }
    }
    return (CSM_OK);
}

/* ======== _CSM_serviceXmlGetTransportSettings() ========
 * Private helper to get the tranport protocol settings from the xml doc.
 * The function is using for RCS provisioning data.
 *
 * Returns:
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
static vint _CSM_serviceXmlGetTransportSettings(
    ezxml_t          xml_ptr,
    CSM_ServiceMngr *serviceMngr_ptr)
{
    char *value_ptr;

    /* Get ps transport protocol */
    if (NULL != (value_ptr = SETTINGS_xmlGetNestedParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_PS_SIGNALLING))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_UDP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_UDP))) {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_UDP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TCP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TCP))) {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_TCP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TLS))) {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("psSignalling=%s\n", value_ptr);
    }

    /* Get wifi transport protocol */
    if (NULL != (value_ptr = SETTINGS_xmlGetNestedParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_WIFI_SIGNALLING))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_UDP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_UDP))) {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_UDP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TCP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TCP))) {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_TCP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TLS))) {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("wifiSignalling=%s\n", value_ptr);
    }

    /* Get ps transport protocol of media */
    if (NULL != (value_ptr = SETTINGS_xmlGetNestedParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_PS_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP,
                sizeof(SETTINGS_PARM_VALUE_MSRP))) {
            serviceMngr_ptr->transportProto.psMedia =
                    CSM_TRASPORT_PROTO_MSRP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            serviceMngr_ptr->transportProto.psMedia =
                    CSM_TRASPORT_PROTO_MSRP_O_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.psMedia = CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("psMedia=%s\n", value_ptr);
    }

    /* Get wifi transport protocol of media */
    if (NULL != (value_ptr = SETTINGS_xmlGetNestedParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_WIFI_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP,
                sizeof(SETTINGS_PARM_VALUE_MSRP))) {
            serviceMngr_ptr->transportProto.wifiMedia =
                    CSM_TRASPORT_PROTO_MSRP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            serviceMngr_ptr->transportProto.wifiMedia =
                    CSM_TRASPORT_PROTO_MSRP_O_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.wifiMedia = CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("wifiMedia=%s\n", value_ptr);
    }

    /* Get wifi transport protocol of RT Media */
    if (NULL != (value_ptr = SETTINGS_xmlGetNestedParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_WIFI_RT_MEDIA))) {
        if (0 == OSAL_strcmp(value_ptr, SETTINGS_PARM_VALUE_SRTP)) {
            serviceMngr_ptr->transportProto.wifiRTMedia =
                    CSM_TRASPORT_PROTO_RT_MEDIA_SRTP;
        }
        else {
            serviceMngr_ptr->transportProto.wifiRTMedia =
                    CSM_TRASPORT_PROTO_RT_MEDIA_RTP;
        }
    }

    /* Get ps transport protocol of RT Media */
    if (NULL != (value_ptr = SETTINGS_xmlGetNestedParmValue(xml_ptr,
            SETTINGS_TAG_SERVICE, SETTINGS_TAG_TRANSPORT_PROTO,
            SETTINGS_PARM_PS_RT_MEDIA))) {
        if (0 == OSAL_strcmp(value_ptr, SETTINGS_PARM_VALUE_SRTP)) {
            serviceMngr_ptr->transportProto.psRTMedia =
                    CSM_TRASPORT_PROTO_RT_MEDIA_SRTP;
        }
        else {
            serviceMngr_ptr->transportProto.psRTMedia =
                    CSM_TRASPORT_PROTO_RT_MEDIA_RTP;
        }
    }
    CSM_isiServiceSetMediaSessionType(serviceMngr_ptr);

    return (CSM_OK);
}

/* ======== _CSM_serviceGetTransportSettings() ========
 * Private helper to get the tranport protocol settings from the xml doc.
 * This function is using for csm configuration.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
static vint _CSM_serviceGetTransportSettings(
    void            *cfg_ptr,
    CSM_ServiceMngr *serviceMngr_ptr)
{
    char *value_ptr;

    /* Get ps transport protocol */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_PS_SIGNALLING))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_UDP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_UDP))) {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_UDP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TCP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TCP))) {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_TCP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TLS))) {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.psSignalling =
                    CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("psSignalling=%s\n", value_ptr);
    }

    /* Get wifi transport protocol */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_WIFI_SIGNALLING))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_UDP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_UDP))) {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_UDP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TCP,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TCP))) {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_TCP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_SIP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_SIP_O_TLS))) {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.wifiSignalling =
                    CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("wifiSignalling=%s\n", value_ptr);
    }

    /* Get ps transport protocol of media */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_PS_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP,
                sizeof(SETTINGS_PARM_VALUE_MSRP))) {
            serviceMngr_ptr->transportProto.psMedia =
                    CSM_TRASPORT_PROTO_MSRP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            serviceMngr_ptr->transportProto.psMedia =
                    CSM_TRASPORT_PROTO_MSRP_O_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.psMedia = CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("psMedia=%s\n", value_ptr);
    }

    /* Get wifi transport protocol of media */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_WIFI_MEDIA))) {
        if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP,
                sizeof(SETTINGS_PARM_VALUE_MSRP))) {
            serviceMngr_ptr->transportProto.wifiMedia =
                    CSM_TRASPORT_PROTO_MSRP;
        }
        else if (0 == OSAL_strncmp(value_ptr, SETTINGS_PARM_VALUE_MSRP_O_TLS,
                sizeof(SETTINGS_PARM_VALUE_MSRP_O_TLS))) {
            serviceMngr_ptr->transportProto.wifiMedia =
                    CSM_TRASPORT_PROTO_MSRP_O_TLS;
        }
        else {
            serviceMngr_ptr->transportProto.wifiMedia = CSM_TRASPORT_PROTO_NONE;
        }
        CSM_dbgPrintf("wifiMedia=%s\n", value_ptr);
    }

    /* Get wifi transport protocol of RT Media */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_WIFI_RT_MEDIA))) {
        if (0 == OSAL_strcmp(value_ptr, SETTINGS_PARM_VALUE_SRTP)) {
            serviceMngr_ptr->transportProto.wifiRTMedia = 
                    CSM_TRASPORT_PROTO_RT_MEDIA_SRTP;
        }
        else {
            serviceMngr_ptr->transportProto.wifiRTMedia = 
                    CSM_TRASPORT_PROTO_RT_MEDIA_RTP;
        }
    }

    /* Get ps transport protocol of RT Media */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_TRANSPORT_PROTO, NULL,
            SETTINGS_PARM_PS_RT_MEDIA))) {
        if (0 == OSAL_strcmp(value_ptr, SETTINGS_PARM_VALUE_SRTP)) {
            serviceMngr_ptr->transportProto.psRTMedia = 
                    CSM_TRASPORT_PROTO_RT_MEDIA_SRTP;
        }
        else {
            serviceMngr_ptr->transportProto.psRTMedia = 
                    CSM_TRASPORT_PROTO_RT_MEDIA_RTP;
        }
    }
    CSM_isiServiceSetMediaSessionType(serviceMngr_ptr);
    return (CSM_OK);
}

/* ======== _CSM_serviceGetPostSettings() ========
 * Private helper to get port settings, sip, audio, video, and ipsec,
 * from the xml doc.
 * The xml doc could csm configuration or RCS provisioning data.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
static vint _CSM_serviceGetPortSettings(
    void            *cfg_ptr,
    CSM_ServiceMngr *serviceMngr_ptr)
{
    char           *value_ptr;
    CSM_IsiService *isiService_ptr;

    /* Set default values */
    serviceMngr_ptr->sipPort        = 5060;
    serviceMngr_ptr->audioRtpPort   = 59000;
    serviceMngr_ptr->audioPoolSize  = 20;
    serviceMngr_ptr->videoRtpPort   = 60000;
    serviceMngr_ptr->videoPoolSize  = 20;
    serviceMngr_ptr->ipsec.protectedPort = 60100;
    serviceMngr_ptr->ipsec.protectedPortPoolSz = 20;
    serviceMngr_ptr->ipsec.spi = 1234;
    serviceMngr_ptr->ipsec.spiPoolSz = 20;

    /* Get sip port */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_SIP_PORT))) {
        serviceMngr_ptr->sipPort = OSAL_atoi(value_ptr);
    }

    /* Get audio port */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_AUDIO_PORT))) {
        serviceMngr_ptr->audioRtpPort = OSAL_atoi(value_ptr);
    }

    /* Get audio port pool size */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_AUDIO_PORT_POOL_SIZE))) {
        serviceMngr_ptr->audioPoolSize = OSAL_atoi(value_ptr);
    }

    /* Get video port */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_VIDEO_PORT))) {
        serviceMngr_ptr->videoRtpPort = OSAL_atoi(value_ptr);
    }

    /* Get video port pool size */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_VIDEO_PORT_POOL_SIZE))) {
        serviceMngr_ptr->videoPoolSize = OSAL_atoi(value_ptr);
    }

    /* Get sip protected port */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_SIP_PROTECTED_PORT))) {
        serviceMngr_ptr->ipsec.protectedPort = OSAL_atoi(value_ptr);
    }

    /* Get sip protected port pool size */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL,
            SETTINGS_PARM_SIP_PROTECTED_PORT_POOL_SIZE))) {
        serviceMngr_ptr->ipsec.protectedPortPoolSz = OSAL_atoi(value_ptr);
    }

    /* Get ipsec spi */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL, SETTINGS_PARM_IPSEC_SPI))) {
        serviceMngr_ptr->ipsec.spi = OSAL_atoi(value_ptr);
    }

    /* Get ipsec spi pool size */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_PORT, NULL,
            SETTINGS_PARM_IPSEC_SPI_POOL_SIZE))) {
        serviceMngr_ptr->ipsec.spiPoolSz = OSAL_atoi(value_ptr);
    }

    /* Informational print */
    CSM_dbgPrintf("sip port=%d\n", serviceMngr_ptr->sipPort);
    CSM_dbgPrintf("audio rtp port=%d\n", serviceMngr_ptr->audioRtpPort);
    CSM_dbgPrintf("audio rtp pool size=%d\n", serviceMngr_ptr->audioPoolSize);
    CSM_dbgPrintf("video rtp port=%d\n", serviceMngr_ptr->videoRtpPort);
    CSM_dbgPrintf("audio rtp pool size=%d\n", serviceMngr_ptr->videoPoolSize);
    CSM_dbgPrintf("IPSec protected port =%d\n",
            serviceMngr_ptr->ipsec.protectedPort);
    CSM_dbgPrintf("IPSec port protected port pool size=%d\n",
            serviceMngr_ptr->ipsec.protectedPortPoolSz);
    CSM_dbgPrintf("IPSec spi=%d\n", serviceMngr_ptr->ipsec.spi);
    CSM_dbgPrintf("IPSec spi poll size=%d\n", serviceMngr_ptr->ipsec.spiPoolSz);

    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
        /* Got the SIP service */
        CSM_isiServiceSetPorts(serviceMngr_ptr, isiService_ptr);
    }
    return (CSM_OK);
}

/*
 * ======== CSM_serviceInit() ========
 *
 * Public sub package function for initializing the accounts management package.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_serviceInit(
    CSM_ServiceMngr    *serviceMngr_ptr,
    CSM_IsiMngr        *isiMngr_ptr,
#ifdef INCLUDE_SUPSRV
    SUPSRV_Mngr        *supSrvManager_ptr,
#endif
    void               *cfg_ptr)
{
    char      *value_ptr;

    CSM_dbgPrintf("\n");
    /* Cache the manager pointer for use in this package */
    mServiceMngr_ptr = serviceMngr_ptr;
    OSAL_memSet(serviceMngr_ptr, 0, sizeof(CSM_ServiceMngr));

    /* Account package needs handle to ISI Manager */
    serviceMngr_ptr->isiMngr_ptr = isiMngr_ptr;
    /* Account package needs handle to Radio Policy */
    serviceMngr_ptr->isReady = OSAL_FALSE;
    /* Set RCS provisioning disabled for default settting */
    serviceMngr_ptr->isRcsProvisioningEnabled = OSAL_FALSE;
    serviceMngr_ptr->isRcsDataProvisioned = OSAL_FALSE;
    serviceMngr_ptr->urlFmt.natUrlFmt = CSM_URL_FORMAT_TEL_URI;
    serviceMngr_ptr->urlFmt.intUrlFmt = CSM_URL_FORMAT_TEL_URI;
    serviceMngr_ptr->transportProto.psSignalling = CSM_TRASPORT_PROTO_NONE;
    serviceMngr_ptr->transportProto.wifiSignalling = CSM_TRASPORT_PROTO_NONE;
    /* The default IMS is disabled. */
    serviceMngr_ptr->isImsEnabled = OSAL_FALSE;

    /*
     * Set default values for GSM service to activate without ISIM account 
     * data.
     */

    /* Default anonymous uri for emergency call w/o registration */
    OSAL_strcpy(serviceMngr_ptr->uri, "sip:anonymous@anonymous.invalid");
    OSAL_strcpy(serviceMngr_ptr->authname, "default");
    OSAL_strcpy(serviceMngr_ptr->password, "default");
    OSAL_strcpy(serviceMngr_ptr->username, "");
    OSAL_strcpy(serviceMngr_ptr->realm, "default");
    OSAL_strcpy(serviceMngr_ptr->obProxy, "default");
    OSAL_strcpy(serviceMngr_ptr->audioconf, 
            "sip:conf-factory@ims.mncXXX.mccYYY.3gppnetwork.org");
    OSAL_strcpy(serviceMngr_ptr->videoconf, "");
    /*
     * Empty default proxy, this is for anonymous emergency call
     * w/o emergency registration
     */
    serviceMngr_ptr->proxy[0] = 0;

    /*
     * Read the account service settings from csm.xml configuration file
     */

    /* Get rcs provisioning enabled */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_NONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            NULL, NULL, SETTINGS_PARM_RCS_PROVISIONING_ENABLED))) {
        if ('1' == value_ptr[0]) {
            serviceMngr_ptr->isRcsProvisioningEnabled = OSAL_TRUE;
        }
    }

    /* Get url format settings */
    _CSM_serviceGetUrlFmtSettings(cfg_ptr, serviceMngr_ptr);

    /* Get transport protocol settings */
    _CSM_serviceGetTransportSettings(cfg_ptr, serviceMngr_ptr);

    /* Get ports settings */
    _CSM_serviceGetPortSettings(cfg_ptr, serviceMngr_ptr);

    /* Get IM conference server */
    _CSM_serviceGetIMConfServer(cfg_ptr, serviceMngr_ptr);

    /*
     * Read the supplementary service user settings from csm.xml configuration
     * file
     */

#ifdef INCLUDE_SUPSRV
    /* Get server */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_SERVER))) {
        OSAL_strncpy(supSrvManager_ptr->supSrvXcap.server,
                value_ptr, sizeof(supSrvManager_ptr->supSrvXcap.server));
    }

    /* Get user */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_USER))) {
        OSAL_strncpy(supSrvManager_ptr->supSrvXcap.username,
                value_ptr, sizeof(supSrvManager_ptr->supSrvXcap.username));
    }

    /* Get password */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_PASSWORD))) {
        OSAL_strncpy(supSrvManager_ptr->supSrvXcap.password,
                value_ptr, sizeof(supSrvManager_ptr->supSrvXcap.password));
    }

    /* Get uri */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_URI))) {
        OSAL_strncpy(supSrvManager_ptr->supSrvXcap.uri,
                value_ptr, sizeof(supSrvManager_ptr->supSrvXcap.uri));
    }

    /* Get timeout */
    if (NULL != (value_ptr = SETTINGS_getParmValue(SETTINGS_TYPE_CSM,
            SETTINGS_NESTED_ONE, cfg_ptr, SETTINGS_TAG_SERVICE,
            SETTINGS_TAG_SUPPLEMENTARY_SRV, NULL, SETTINGS_PARM_TIMEOUT))) {
        supSrvManager_ptr->supSrvXcap.timeout = OSAL_atoi(value_ptr);
    }
#endif

    /* Informational print */
    CSM_dbgPrintf("Audio conference server=%s\n", serviceMngr_ptr->audioconf);
    CSM_dbgPrintf("RCS provisioning enabled: %d\n",
            serviceMngr_ptr->isRcsProvisioningEnabled);

#ifdef INCLUDE_SUPSRV
    /* SUPSRV Informational print */
    CSM_dbgPrintf("SUPSRV: server=%s\n", supSrvManager_ptr->supSrvXcap.server);
    CSM_dbgPrintf("SUPSRV: username=%s\n", 
            supSrvManager_ptr->supSrvXcap.username);
    CSM_dbgPrintf("SUPSRV: password=%s\n", 
            supSrvManager_ptr->supSrvXcap.password);
    CSM_dbgPrintf("SUPSRV: uri=%s\n", 
                supSrvManager_ptr->supSrvXcap.uri);
    CSM_dbgPrintf("SUPSRV: timeout=%d\n", 
                supSrvManager_ptr->supSrvXcap.timeout);
#endif

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceAkaResp() ========
 *
 * Private function to set the AKA response from event to CSM_ServiceMngr.
 * Those information will be used to pass to ISI.
 *
 * Returns:
 *   None.
 */
void _CSM_serviceAkaResp(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_dbgPrintf("\n");

    switch (serviceEvt_ptr->reason) {
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SUCCESS:
            serviceMngr_ptr->aka.result = ISI_SERVICE_AKA_RESPONSE_SUCCESS;
            break;
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
            serviceMngr_ptr->aka.result =
                    ISI_SERVICE_AKA_RESPONSE_NETWORK_FAILURE;
            break;
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
            serviceMngr_ptr->aka.result = ISI_SERVICE_AKA_RESPONSE_SYNC_FAILURE;
            break;
        default:
            CSM_dbgPrintf("Invalid reason:%d\n", serviceEvt_ptr->reason);
            return;
    }

    /* Copy response, auts, ik and ck no matter what's result */
    OSAL_memCpy(serviceMngr_ptr->aka.response, serviceEvt_ptr->u.aka.response,
            sizeof(serviceMngr_ptr->aka.response));
    serviceMngr_ptr->aka.resLength = serviceEvt_ptr->u.aka.resLength;
    OSAL_memCpy(serviceMngr_ptr->aka.auts, serviceEvt_ptr->u.aka.auts,
            sizeof(serviceMngr_ptr->aka.auts));
    OSAL_memCpy(serviceMngr_ptr->aka.ik,
            serviceEvt_ptr->u.aka.ik, sizeof(serviceMngr_ptr->aka.ik));
    OSAL_memCpy(serviceMngr_ptr->aka.ck,
            serviceEvt_ptr->u.aka.ck, sizeof(serviceMngr_ptr->aka.ck));
}

/*
 * ======== _CSM_serviceAcctReady() ========
 *
 * Private function to check if the account data is all ready.
 *
 * Returns:
 *   CSM_OK: account is ready.
 *   CSM_ERR: account is not ready.
 */
vint _CSM_serviceAcctReady(
    CSM_ServiceMngr *serviceMngr_ptr)
{
    if (OSAL_TRUE == serviceMngr_ptr->isReady) {
        return (CSM_OK);
    }
    if ((0 == serviceMngr_ptr->uri[0]) || (0 == serviceMngr_ptr->authname[0]) ||
            (0 == serviceMngr_ptr->realm[0]) ||
            (0 == serviceMngr_ptr->proxy[0]) ||
            (0 == serviceMngr_ptr->obProxy[0])) {
        CSM_dbgPrintf("Account data is not ready.\n");
        return (CSM_ERR);
    }

    serviceMngr_ptr->isReady = OSAL_TRUE;
    return (CSM_OK);
}

/*
 * ======== _CSM_serviceSetPorts() ========
 *
 * Private function to set the ports from event to CSM_ServiceMngr.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_serviceSetPorts(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_IsiService *isiService_ptr;

    CSM_dbgPrintf("\n");

    serviceMngr_ptr->sipPort = serviceEvt_ptr->u.port.sip;
    serviceMngr_ptr->audioRtpPort = serviceEvt_ptr->u.port.audio;
    serviceMngr_ptr->audioPoolSize = serviceEvt_ptr->u.port.audioPoolSize;
    serviceMngr_ptr->videoRtpPort = serviceEvt_ptr->u.port.video;
    serviceMngr_ptr->videoPoolSize = serviceEvt_ptr->u.port.videoPoolSize;

    /* Informational print */
    CSM_dbgPrintf("sip port=%d\n", serviceMngr_ptr->sipPort);
    CSM_dbgPrintf("audio rtp port=%d\n", serviceMngr_ptr->audioRtpPort);
    CSM_dbgPrintf("audio rtp pool size=%d\n", serviceMngr_ptr->audioPoolSize);
    CSM_dbgPrintf("video rtp port=%d\n", serviceMngr_ptr->videoRtpPort);
    CSM_dbgPrintf("audio rtp pool size=%d\n", serviceMngr_ptr->videoPoolSize);

    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
        /* Got the SIP service */
        CSM_isiServiceSetPorts(serviceMngr_ptr, isiService_ptr);
    }

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceSetImeiUri() ========
 *
 * Private function to set the IMEI Uri from event to CSM_ServiceMngr.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_serviceSetImeiUri(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_IsiService *isiService_ptr;

    CSM_dbgPrintf("\n");

    OSAL_strncpy(serviceMngr_ptr->imeiUri, serviceEvt_ptr->u.imeiUri,
            CSM_SERVICE_STR_SZ);

    /* Informational print */
    CSM_dbgPrintf("IMEI URI=%s\n", 
            serviceMngr_ptr->imeiUri);
     /* Get the service object for this service ID */
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 1))) {
        CSM_isiServiceSetImeiUri(serviceMngr_ptr, isiService_ptr);
    }

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceSetNetAppProvision() ========
 *
 * Private function to set the bsf/naf info from event to gba.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_serviceSetNetAppProvision(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
#ifdef INCLUDE_GBA
    GBA_setup(serviceEvt_ptr->u.appsProvision.bsf, serviceMngr_ptr->authname);
    GBA_registerNetApp(
            (GBA_NetAppObj *)&serviceEvt_ptr->u.appsProvision.xcapAppInfo);
#else
#ifdef INCLUDE_SUPSRV
    /* could do xcap only provision for digest auth */
    CSM_supSrvProvisioning(serviceEvt_ptr->u.appsProvision.xcapAppInfo.appUri,
            serviceEvt_ptr->u.appsProvision.xcapAppInfo.appAuthName,
            serviceEvt_ptr->u.appsProvision.xcapAppInfo.appAuthSecret);
#endif
#endif

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceSetIpsec() ========
 *
 * Private function to set IPSec information from event to CSM_ServiceMngr.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_serviceSetIpsec(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_dbgPrintf("\n");

    serviceMngr_ptr->ipsec.protectedPort =
            serviceEvt_ptr->u.ipsec.protectedPort;
    serviceMngr_ptr->ipsec.protectedPortPoolSz =
            serviceEvt_ptr->u.ipsec.protectedPortPoolSz;
    serviceMngr_ptr->ipsec.spi = serviceEvt_ptr->u.ipsec.spi;
    serviceMngr_ptr->ipsec.spiPoolSz = serviceEvt_ptr->u.ipsec.spiPoolSz;

    /* Informational print */
    CSM_dbgPrintf("IPSec protected port =%d\n",
            serviceMngr_ptr->ipsec.protectedPort);
    CSM_dbgPrintf("IPSec port protected port pool size=%d\n",
            serviceMngr_ptr->ipsec.protectedPortPoolSz);
    CSM_dbgPrintf("IPSec spi=%d\n",
            serviceMngr_ptr->ipsec.spi);
    CSM_dbgPrintf("IPSec spi poll size=%d\n",
            serviceMngr_ptr->ipsec.spiPoolSz);

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceUpdateCgi() ========
 *
 * Private function to update cell global identify
 *
 * Returns:
 *   CSM_OK:  function exits normally.
 *   CSM_ERR: in case of error.
 */
vint _CSM_serviceUpdateCgi(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_IsiService *isiService_ptr;

    CSM_dbgPrintf("\n");

    serviceMngr_ptr->cgi.type = serviceEvt_ptr->u.cgi.type;
    OSAL_strncpy(serviceMngr_ptr->cgi.id, serviceEvt_ptr->u.cgi.id,
            CSM_CGI_STRING_SZ);

    /* Informational print */
    CSM_dbgPrintf("CGI type=%d, id=%s\n", serviceMngr_ptr->cgi.type,
            serviceMngr_ptr->cgi.id);

    /* Get the service object for this service ID */
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
        CSM_isiServiceUpdateCgi(serviceMngr_ptr, isiService_ptr);
    }
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 1))) {
        CSM_isiServiceUpdateCgi(serviceMngr_ptr, isiService_ptr);
    }
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_SIP_RCS, 0))) {
        CSM_isiServiceUpdateCgi(serviceMngr_ptr, isiService_ptr);
    }

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceSetInstanceId() ========
 *
 * Private function to set instance id.
 *
 * Returns:
 *   CSM_OK:  function exits normally.
 *   CSM_ERR: in case of error.
 */
vint _CSM_serviceSetInstanceId(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_dbgPrintf("\n");

    OSAL_strncpy(serviceMngr_ptr->instanceId,
            serviceEvt_ptr->u.instanceId,
            sizeof(serviceMngr_ptr->instanceId));

    /* Informational print */
    CSM_dbgPrintf("INSTANCE ID=%s\n", serviceMngr_ptr->instanceId);

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceIsReady() ========
 *
 * Private function to check if the account information is provisioned.
 *
 * Returns:
 *   CSM_OK: The account information is provisioned.
 *   CSM_ERR: The account information is not provisioned.
 */
OSAL_Boolean _CSM_serviceIsReady(
    CSM_ServiceMngr  *serviceMngr_ptr,
    CSM_IsiService   *isiService_ptr)
{
    /* Always ready for GSM service */
    if (CSM_ISI_PROTOCOL_GSM == isiService_ptr->protocol) {
        return (OSAL_TRUE);
    }

    if (OSAL_TRUE == serviceMngr_ptr->isRcsProvisioningEnabled) {
        return (OSAL_Boolean)(serviceMngr_ptr->isReady &&
                serviceMngr_ptr->isRcsDataProvisioned &&
                isiService_ptr->isInitialized);
    }

    return (serviceMngr_ptr->isReady);
}

/*
 * ======== _CSM_serviceParseRcsProvisioningData() ========
 *
 * Private function to parse RCS provisioning data
 *
 * Returns:
 *   CSM_OK:  function exits normally.
 *   CSM_ERR: in case of error.
 */
static vint _CSM_serviceParseRcsProvisioningData(
    CSM_ServiceMngr  *serviceMngr_ptr)
{
    char    xmlDoc[ISI_PROVISIONING_DATA_STRING_SZ];
    int     xmlDocLen;
    ezxml_t xml_ptr;

    /*
     * rcsProvisioningData will be used later to pass down to protocol,
     * so make a copy for xml parsing.
     */
    OSAL_strncpy(xmlDoc, serviceMngr_ptr->rcsProvisioningData, 
            ISI_PROVISIONING_DATA_STRING_SZ);
    /* Parse the paramter xml doc */
    xmlDocLen = OSAL_strlen(xmlDoc);
    xml_ptr = ezxml_parse_str(xmlDoc, xmlDocLen);

    /* Parse CSM interested RCS provisioning data */

    /* Get transport protocol settings */
    _CSM_serviceXmlGetTransportSettings(xml_ptr, serviceMngr_ptr);

    /* Get url format settings */
    _CSM_serviceXmlGetUrlFmtSettings(xml_ptr, serviceMngr_ptr);
    RPM_setUrlFmt((RPM_UrlFmtVal)serviceMngr_ptr->urlFmt.intUrlFmt, 
            (RPM_UrlFmtVal)serviceMngr_ptr->urlFmt.natUrlFmt);
    ezxml_free(xml_ptr);
    return (CSM_OK);
}

/*
 * ======== _CSM_serviceProcessRcsProvisioning() ========
 *
 * Private function to process RCS provisioning event.
 *
 * Returns:
 *   CSM_OK:  function exits normally.
 *   CSM_ERR: in case of error.
 */
static vint _CSM_serviceProcessRcsProvisioning(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_dbgPrintf("\n");

    /* If RCS provisioning is not enabled, ignore the event. */
    if (OSAL_TRUE != serviceMngr_ptr->isRcsProvisioningEnabled) {
        CSM_dbgPrintf("RCS provisioning is not enabled.\n");
        return (CSM_ERR);
    }

    /* Currently CSM doesn't accept multiple RCS data provisioning */
    if (OSAL_TRUE == serviceMngr_ptr->isRcsDataProvisioned) {
        CSM_dbgPrintf("RCS data already provisioned.\n");
        return (CSM_ERR);
    }

    /* Get provisioning data */
    if (CSM_ERR == _CSM_isiServiceGetProvisioningData(serviceMngr_ptr,
            serviceEvt_ptr->serviceId )) {
        CSM_dbgPrintf("Failed to get RCS provisioning data.\n");
        return (CSM_ERR);
    }

    /* Parsing CSM interested provisioning data */
    if (CSM_ERR == _CSM_serviceParseRcsProvisioningData(serviceMngr_ptr)) {
        CSM_dbgPrintf("Failed to parse RCS provisioning data.\n");
        return (CSM_ERR);
    }

    /* Set provisioned flag */
    serviceMngr_ptr->isRcsDataProvisioned = OSAL_TRUE;
    CSM_dbgPrintf("RCS data provisioned.\n");

    return (CSM_OK);
}

/*
 * ======== _CSM_serviceProcessFsmEventForAll() ========
 *
 * Private function to run a event for all isi services.
 *
 * Returns:
 *   CSM_OK:  function exits normally.
 *   CSM_ERR: in case of error.
 */
static vint _CSM_serviceProcessFsmEventForAll(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    CSM_IsiService *isiService_ptr;
    /* Find all services and run fsm if it's ready. */

    /* Normal sip */
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
        /* Don't run fsm if service is not ready yet */
        if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                isiService_ptr)) {
            /* Run FSM */
            _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                    isiService_ptr->serviceId,
                    serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);
        }
    }

    /* Emergency sip */
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 1))) {
        /* Don't run fsm if service is not ready yet */
        if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                isiService_ptr)) {
            /* Run FSM */
            _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                    isiService_ptr->serviceId,
                    serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);
        }
    }

    /* RCS sip, i.e. ASAPP */
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_SIP_RCS, 0))) {
        /* Don't run fsm if service is not ready yet */
        if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                isiService_ptr)) {
            /* Run FSM */
            _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                    isiService_ptr->serviceId,
                    serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);
        }
    }

    /* GSM service */
    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_GSM, 0))) {
        /* Don't run fsm if service is not ready yet */
        if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                isiService_ptr)) {
            /* Run FSM */
            _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                    isiService_ptr->serviceId,
                    serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);
        }
    }

    return (CSM_OK);
}
static vint _CSM_serviceSetCoders(
    int serviceId,
    CSM_ServiceEvt  *serviceEvt_ptr)
{
    int index;
    CSM_dbgPrintf("_CSM_serviceSetCoders num=%d\n", 
        serviceEvt_ptr->u.coder.coderNum);
    /* Remove coder first and add coder later. So, the order will be kept */
    for(index = 0; index < serviceEvt_ptr->u.coder.coderNum && 
            index < CSM_CODER_NUM; index++) {
        ISI_removeCoderFromServiceByPayloadType(serviceId,
                serviceEvt_ptr->u.coder.coderPayloadType[index]);
    }
    for(index = 0; index < serviceEvt_ptr->u.coder.coderNum &&
            index < CSM_CODER_NUM; index++) {
        ISI_addCoderToService(serviceId,
                serviceEvt_ptr->u.coder.coderName[index],
                serviceEvt_ptr->u.coder.coderDescription[index]);
    }
    return (CSM_OK);
}

#ifdef INCLUDE_SUPSRV
vint _CSM_serviceParseAndSetXui(
    char *reasonDesc)
{
    char     *xui_ptr;
    char      _xui_ptr[SUPSRV_STR_SZ];
    uvint     size;

    if (CSM_ERR != _CSM_utilGetValue(reasonDesc, "URI", &xui_ptr, &size)) {
        /*
         * Parsing value would be "xxxxx".
         * We don't need double quote: "".
         */
        OSAL_strncpy(_xui_ptr, xui_ptr + 1, size - 1);
        CSM_supSrvSetXcapUri(_xui_ptr);
    }
    return (CSM_OK);
}
#endif
/*
 * ======== CSM_serviceProcessEvent() ========
 *
 * Event entry point into the Service Package.
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_serviceProcessEvent(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_ServiceEvt  *serviceEvt_ptr,
    CSM_OutputEvent *csmOutput_ptr)
{
    CSM_IsiService *isiService_ptr;

    CSM_dbgPrintf("reason=%d desc:%s\n",
            serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);

    switch (serviceEvt_ptr->reason) {
        case CSM_SERVICE_EVT_REASON_PROTOCOL_REGISTERED:
            /* A Protocol has registered with ISI.  It's ready to be used. */
            _CSM_isiServiceCreate(serviceMngr_ptr,
                    serviceEvt_ptr->protocol, csmOutput_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_PROTOCOL_DIED:
            /* A protocol has de-registred from ISI. Something died or killed. */
            _CSM_isiServiceDestroy(serviceMngr_ptr,
                    serviceEvt_ptr->protocol);
            break;
        case CSM_SERVICE_EVT_REASON_SERVICE_INIT_OK:
            /* Get the isi service */
            isiService_ptr = CSM_isiGetServiceViaId(
                    serviceMngr_ptr->isiMngr_ptr, serviceEvt_ptr->serviceId);
            isiService_ptr->isInitialized = OSAL_TRUE;
            /* Don't run fsm if account info is not provisioned  */
            if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                    isiService_ptr)) {
                _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr, 
                        serviceEvt_ptr->serviceId, serviceEvt_ptr->reason, 
                        serviceEvt_ptr->reasonDesc);
            }
            break;
        case CSM_SERVICE_EVT_REASON_SET_IMPU:
            OSAL_strncpy(serviceMngr_ptr->uri, serviceEvt_ptr->u.impu,
                    sizeof(serviceMngr_ptr->uri));
#ifdef INCLUDE_SUPSRV
            CSM_supSrvSetXcapUri(serviceEvt_ptr->u.impu);
            CSM_supSrvSetImpu(serviceEvt_ptr->u.impu);
#endif
            break;
        case CSM_SERVICE_EVT_REASON_SET_IMPI:
            OSAL_strncpy(serviceMngr_ptr->authname, serviceEvt_ptr->u.impi,
                    sizeof(serviceMngr_ptr->authname));
            break;
        case CSM_SERVICE_EVT_REASON_SET_DOMAIN:
            OSAL_strncpy(serviceMngr_ptr->realm, serviceEvt_ptr->u.domain,
                        sizeof(serviceMngr_ptr->realm));
            OSAL_snprintf(serviceMngr_ptr->proxy, CSM_SERVICE_STR_SZ,
                    "sip:%s", serviceEvt_ptr->u.domain);
            /* 
             * Tell ISI Server about the domain, which will be used for
             * normalizing outbound call remote address.
             */
            ISI_serverSetDomain(serviceMngr_ptr->realm);
            break;
        case CSM_SERVICE_EVT_REASON_SET_PASSWORD:
            OSAL_strncpy(serviceMngr_ptr->password, serviceEvt_ptr->u.password,
                    sizeof(serviceMngr_ptr->password));
            break;
        case CSM_SERVICE_EVT_REASON_SET_PCSCF:
            OSAL_strncpy(serviceMngr_ptr->obProxy, serviceEvt_ptr->u.pcscf,
                    sizeof(serviceMngr_ptr->obProxy));
            break;
        case CSM_SERVICE_EVT_REASON_SET_EMGCY_PCSCF:
            OSAL_strncpy(serviceMngr_ptr->eObProxy, serviceEvt_ptr->u.pcscf,
                    sizeof(serviceMngr_ptr->eObProxy));
            break;
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SUCCESS:
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
#ifdef INCLUDE_GBA
            /* added aka arbiter because gba would use aka as well */
            switch (GBAM_akaArbitorGetTarget()) {
                case GBAM_AKA_ISI_REQ:
                    /* Set aka response to service manager */
                    _CSM_serviceAkaResp(serviceMngr_ptr, serviceEvt_ptr);
                    if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
                            serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
                        /* Got the SIP service */
                        _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                                isiService_ptr->serviceId,
                                serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);
                    }
                    break;
                case GBAM_AKA_GBA_REQ:
                    GBAM_sendAkaAuthResp(serviceEvt_ptr);
                    break;
            }
            break;
#else
            /* Set aka response to service manager */
            _CSM_serviceAkaResp(serviceMngr_ptr, serviceEvt_ptr);
            if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
                    serviceMngr_ptr->isiMngr_ptr, CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
                /* Got the SIP service */
                _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                        isiService_ptr->serviceId,
                        serviceEvt_ptr->reason, serviceEvt_ptr->reasonDesc);
            }
            break;
#endif
        case CSM_SERVICE_EVT_REASON_SERVICE_ACTIVE:
#ifdef INCLUDE_SUPSRV
            _CSM_serviceParseAndSetXui(serviceEvt_ptr->reasonDesc);
#endif
            _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                    serviceEvt_ptr->serviceId, serviceEvt_ptr->reason,
                    serviceEvt_ptr->reasonDesc);
            break;
        case CSM_SERVICE_EVT_REASON_SERVICE_ACTIVATING:
        case CSM_SERVICE_EVT_REASON_SERVICE_AUTH_FAIL:
        case CSM_SERVICE_EVT_REASON_SERVICE_INACTIVE:
        case CSM_SERVICE_EVT_REASON_SERVICE_RETRY:
        case CSM_SERVICE_EVT_REASON_ISIM_AKA_CHALLENGE:
        case CSM_SERVICE_EVT_REASON_ACTIVATE:
        case CSM_SERVICE_EVT_REASON_DEACTIVATE:
            _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                    serviceEvt_ptr->serviceId, serviceEvt_ptr->reason,
                    serviceEvt_ptr->reasonDesc);
            break;
        case CSM_SERVICE_EVT_REASON_IPSEC_SETUP:
        case CSM_SERVICE_EVT_REASON_IPSEC_RELEASE:
            /* Send ipsec setup or release event */
            _CSM_isiServiceSendIpsecEvent(serviceEvt_ptr->serviceId,
                    serviceEvt_ptr->reason, csmOutput_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_SET_PORTS:
            /* Store ports to service manager */
            _CSM_serviceSetPorts(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_SET_IPSEC:
            _CSM_serviceSetIpsec(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_UPDATE_CGI:
            /* Update both sip and emergency sip service. */
            _CSM_serviceUpdateCgi(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_SET_IMEI_URI:
            /* Store IMEI to service manager */
            _CSM_serviceSetImeiUri(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_APPS_PROVISION:
            /* setup bsf and xcap */
            _CSM_serviceSetNetAppProvision(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_SET_INSTANCE_ID:
            /* setup the instance id of ua */
            _CSM_serviceSetInstanceId(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_RCS_PROVISIONING:
            /* Process RCS provisioning */
            if (CSM_ERR == _CSM_serviceProcessRcsProvisioning(serviceMngr_ptr,
                    serviceEvt_ptr)) {
                break;
            }
            /* Run FSM for all services */
            _CSM_serviceProcessFsmEventForAll(serviceMngr_ptr, serviceEvt_ptr);
            break;
        case CSM_SERVICE_EVT_REASON_IMS_ENABLE:
            if (OSAL_FALSE ==  serviceMngr_ptr->isImsEnabled) {
                if (CSM_OK == _CSM_serviceAcctReady(serviceMngr_ptr)) {
                    serviceMngr_ptr->isImsEnabled = OSAL_TRUE;
                    /* Run FSM for all services */
                    _CSM_serviceProcessFsmEventForAll(serviceMngr_ptr,
                            serviceEvt_ptr);
                }
            }
            break;
        case CSM_SERVICE_EVT_REASON_IMS_DISABLE:
            if (OSAL_TRUE ==  serviceMngr_ptr->isImsEnabled) {
                serviceMngr_ptr->isImsEnabled = OSAL_FALSE;

                /* Normal sip */
                if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
                        serviceMngr_ptr->isiMngr_ptr,
                        CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
                    /* Don't run fsm if service is not ready yet */
                    if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                            isiService_ptr)) {
                        /* Run FSM */
                        _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                                isiService_ptr->serviceId,
                                serviceEvt_ptr->reason,
                                serviceEvt_ptr->reasonDesc);
                    }
                }

                /* Emergency sip */
                if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
                        serviceMngr_ptr->isiMngr_ptr,
                        CSM_ISI_PROTOCOL_MODEM_IMS, 1))) {
                    /* Don't run fsm if service is not ready yet */
                    if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                            isiService_ptr)) {
                        /* Run FSM */
                        _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                                isiService_ptr->serviceId,
                                serviceEvt_ptr->reason,
                                serviceEvt_ptr->reasonDesc);
                    }
                }

                /* RCS sip, i.e. ASAPP */
                if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
                        serviceMngr_ptr->isiMngr_ptr,
                        CSM_ISI_PROTOCOL_SIP_RCS, 0))) {
                    /* Don't run fsm if service is not ready yet */
                    if (OSAL_TRUE == _CSM_serviceIsReady(serviceMngr_ptr,
                            isiService_ptr)) {
                        /* Run FSM */
                        _CSM_isiServiceProcessFsmEvent(serviceMngr_ptr,
                                isiService_ptr->serviceId,
                                serviceEvt_ptr->reason,
                                serviceEvt_ptr->reasonDesc);
                    }
                }
            }
            break;
        case CSM_SERVICE_EVT_REASON_SET_CODERS:
            if (NULL != (isiService_ptr = CSM_isiGetServiceViaProtocol(
                    serviceMngr_ptr->isiMngr_ptr,
                    CSM_ISI_PROTOCOL_MODEM_IMS, 0))) {
                _CSM_serviceSetCoders(isiService_ptr->serviceId, serviceEvt_ptr);
            }
            break;
        case CSM_SERVICE_EVT_REASON_SET_AUDIO_CONF_SERVER:
            OSAL_strncpy(serviceMngr_ptr->audioconf,
                    serviceEvt_ptr->u.audioConfServer,
                    sizeof(serviceMngr_ptr->audioconf));
            if (NULL != (isiService_ptr = CSM_isiGetServiceViaId(
                    serviceMngr_ptr->isiMngr_ptr, serviceEvt_ptr->serviceId))) {
                OSAL_strncpy(isiService_ptr->audioconf,
                        serviceMngr_ptr->audioconf,
                        sizeof(isiService_ptr->audioconf));
            }
            break;
        case CSM_SERVICE_EVT_REASON_SET_VIDEO_CONF_SERVER:
            OSAL_strncpy(serviceMngr_ptr->videoconf,
                    serviceEvt_ptr->u.videoConfServer,
                    sizeof(serviceMngr_ptr->videoconf));
            if (NULL != (isiService_ptr = CSM_isiGetServiceViaId(
                    serviceMngr_ptr->isiMngr_ptr, serviceEvt_ptr->serviceId))) {
                OSAL_strncpy(isiService_ptr->videoconf,
                        serviceMngr_ptr->videoconf,
                        sizeof(isiService_ptr->videoconf));
            }
            break;
        default:
            break;
    };
    return (CSM_OK);
}

/*
 * ======== CSM_serviceOnImsRadioChange() ========
 *
 * Callback from RPM to notify the CSM ServiceManager that 
 *     a change to the IMS radio has occurred
 *
 * Returns: 
 *      none
 */
void CSM_serviceOnImsRadioChange(
    RPM_RadioInterface *radioInfc_ptr,
    RPM_RadioType       radioType)
{
    CSM_dbgPrintf("current radio type=%d\n", radioInfc_ptr->radioType);

    _CSM_isiServiceProcessIpChange(mServiceMngr_ptr, radioType,
            radioInfc_ptr);
}

/*
 * ======== CSM_serviceEmerRegRequiredChange() ========
 *
 * Callback from RPM to notify the CSM ServiceManager that
 * emergency registration changed.
 *
 * Returns:
 *   RPM_RETURN_OK: function exits normally.
 *   RPM_RETURN_ERROR: in case of error
 */
void CSM_serviceEmerRegRequiredChange(
    vint    isEmergencyRegRequired)
{  
    CSM_dbgPrintf("isEmergencyRegRequired=%d\n", isEmergencyRegRequired);

    _CSM_isiServiceProcessEmerRegChange(mServiceMngr_ptr);
}

/*
 * ======== CSM_serviceShutdown() ========
 *
 * Public de-initialization routine for the accounts manager package
 *
 * Returns: 
 * CSM_OK: function exits normally.
 * CSM_ERR: in case of error
 */
vint CSM_serviceShutdown(
    CSM_ServiceMngr *serviceMngr_ptr)
{
    CSM_dbgPrintf("\n");

    /* Currently nothing */
    return (CSM_OK);
}

/*
 * ======== CSM_serviceConvertToInternalEvt() ========
 *
 * This function is used to convert CSM input event to CSM call internal event.
 *
 * Returns:
 *
 */
void CSM_serviceConvertToInternalEvt(
    CSM_InputEvtType    type,
    void               *inputServiceEvt_ptr,
    CSM_ServiceEvt     *csmServiceEvt_ptr)
{
    CSM_InputService       *csmExtSrvEvt_ptr;
    CSM_InputIsiService    *csmIntSrvEvt_ptr;
    CSM_ServiceEvtReason    reason;

    OSAL_memSet(csmServiceEvt_ptr, 0, sizeof(CSM_ServiceEvt));

    if (CSM_INPUT_EVT_TYPE_EXT == type) {
        csmExtSrvEvt_ptr = (CSM_InputService*) inputServiceEvt_ptr;
        switch (csmExtSrvEvt_ptr->reason) {
            case CSM_SERVICE_REASON_SET_IMPU:
                reason = CSM_SERVICE_EVT_REASON_SET_IMPU;
                break;
            case CSM_SERVICE_REASON_SET_IMPI:
                reason = CSM_SERVICE_EVT_REASON_SET_IMPI;
                break;
            case CSM_SERVICE_REASON_SET_DOMAIN:
                reason = CSM_SERVICE_EVT_REASON_SET_DOMAIN;
                break;
            case CSM_SERVICE_REASON_SET_PASSWORD:
                reason = CSM_SERVICE_EVT_REASON_SET_PASSWORD;
                break;
            case CSM_SERVICE_REASON_SET_PCSCF:
                reason = CSM_SERVICE_EVT_REASON_SET_PCSCF;
                break;
            case CSM_SERVICE_REASON_SET_EMGCY_PCSCF:
                reason = CSM_SERVICE_EVT_REASON_SET_EMGCY_PCSCF;
                break;
            case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SUCCESS:
                reason = CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SUCCESS;
                break;
            case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
                reason =
                       CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE;
                break;
            case CSM_SERVICE_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
                reason = CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE;
                break;
            case CSM_SERVICE_REASON_SET_PORTS:
                reason = CSM_SERVICE_EVT_REASON_SET_PORTS;
                break;
            case CSM_SERVICE_REASON_SET_IPSEC:
                reason = CSM_SERVICE_EVT_REASON_SET_IPSEC;
                break;
            case CSM_SERVICE_REASON_UPDATE_CGI:
                reason = CSM_SERVICE_EVT_REASON_UPDATE_CGI;
                break;
            case CSM_SERVICE_REASON_SET_IMEI_URI:
                reason = CSM_SERVICE_EVT_REASON_SET_IMEI_URI;
                break;
            case CSM_SERVICE_REASON_APPS_PROVISION:
                reason = CSM_SERVICE_EVT_REASON_APPS_PROVISION;
                break;
            case CSM_SERVICE_REASON_SET_INSTANCE_ID:
                reason = CSM_SERVICE_EVT_REASON_SET_INSTANCE_ID;
                break;
            case CSM_SERVICE_REASON_IMS_ENABLE:
                reason = CSM_SERVICE_EVT_REASON_IMS_ENABLE;
                break;
            case CSM_SERVICE_REASON_IMS_DISABLE:
                reason = CSM_SERVICE_EVT_REASON_IMS_DISABLE;
                break;
            case CSM_SERVICE_REASON_SET_REREGISTER_PERIOD:
                reason = CSM_SERVICE_EVT_REASON_SET_REREGISTER_PERIOD;
                break;
            case CSM_SERVICE_REASON_SET_RETRY_TIMER_PERIOD:
                reason = CSM_SERVICE_EVT_REASON_SET_RETRY_TIMER_PERIOD;
                break;
            case CSM_SERVICE_REASON_SET_REG_CAPABILITIES:
                reason = CSM_SERVICE_EVT_REASON_SET_REG_CAPABILITIES;
                break;
            case CSM_SERVICE_REASON_SET_CODERS:
                reason = CSM_SERVICE_EVT_REASON_SET_CODERS;
                break;
            case CSM_SERVICE_REASON_SET_AUDIO_CONF_SERVER:
                reason = CSM_SERVICE_EVT_REASON_SET_AUDIO_CONF_SERVER;
                break;
            case CSM_SERVICE_REASON_SET_VIDEO_CONF_SERVER:
                reason = CSM_SERVICE_EVT_REASON_SET_VIDEO_CONF_SERVER;
                break;
            default:
                reason = CSM_SERVICE_EVT_REASON_INVALID;
                OSAL_logMsg("Invalid CSM service reason: %d \n",
                        csmExtSrvEvt_ptr->reason);
        }
        csmServiceEvt_ptr->reason   = reason;
        OSAL_memCpy(csmServiceEvt_ptr->reasonDesc, csmExtSrvEvt_ptr->reasonDesc,
                sizeof(csmServiceEvt_ptr->reasonDesc));
        OSAL_memCpy(&csmServiceEvt_ptr->u, &csmExtSrvEvt_ptr->u,
                sizeof(csmServiceEvt_ptr->u));
    }
    else if (CSM_INPUT_EVT_TYPE_INT == type) {
        csmIntSrvEvt_ptr = (CSM_InputIsiService*) inputServiceEvt_ptr;
        switch (csmIntSrvEvt_ptr->reason) {
            case CSM_SERVICE_REASON_PROTOCOL_REGISTERED:
                reason = CSM_SERVICE_EVT_REASON_PROTOCOL_REGISTERED;
                break;
            case CSM_SERVICE_REASON_PROTOCOL_DIED:
                reason = CSM_SERVICE_EVT_REASON_PROTOCOL_DIED;
                break;
            case CSM_SERVICE_REASON_SERVICE_INIT_OK:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_INIT_OK;
                break;
            case CSM_SERVICE_REASON_SERVICE_INIT_FAILED:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_INIT_FAILED;
                break;
            case CSM_SERVICE_REASON_SERVICE_ACTIVE:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_ACTIVE;
                break;
            case CSM_SERVICE_REASON_SERVICE_ACTIVATING:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_ACTIVATING;
                break;
            case CSM_SERVICE_REASON_SERVICE_INACTIVE:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_INACTIVE;
                break;
            case CSM_SERVICE_REASON_SERVICE_HANDOFF:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_HANDOFF;
                break;
            case CSM_SERVICE_REASON_SERVICE_RETRY:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_RETRY;
                break;
            case CSM_SERVICE_REASON_SERVICE_AUTH_FAIL:
                reason = CSM_SERVICE_EVT_REASON_SERVICE_AUTH_FAIL;
                break;
            case CSM_SERVICE_REASON_ISIM_AKA_CHALLENGE:
                reason = CSM_SERVICE_EVT_REASON_ISIM_AKA_CHALLENGE;
                break;
            case CSM_SERVICE_REASON_IPSEC_SETUP:
                reason = CSM_SERVICE_EVT_REASON_IPSEC_SETUP;
                break;
            case CSM_SERVICE_REASON_IPSEC_RELEASE:
                reason = CSM_SERVICE_EVT_REASON_IPSEC_RELEASE;
                break;
            case CSM_SERVICE_REASON_RCS_PROVISIONING:
                reason = CSM_SERVICE_EVT_REASON_RCS_PROVISIONING;
                break;
            case CSM_SERVICE_REASON_ACTIVATE:
                reason = CSM_SERVICE_EVT_REASON_ACTIVATE;
                break;
            case CSM_SERVICE_REASON_DEACTIVATE:
                reason = CSM_SERVICE_EVT_REASON_DEACTIVATE;
                break;
            default:
                reason = CSM_SERVICE_EVT_REASON_INVALID;
                OSAL_logMsg("Invalid CSM service reason: %d \n",
                        csmIntSrvEvt_ptr->reason);
        }
        csmServiceEvt_ptr->protocol  = csmIntSrvEvt_ptr->protocol;
        csmServiceEvt_ptr->serviceId = csmIntSrvEvt_ptr->serviceId;
        csmServiceEvt_ptr->reason    = reason;
        OSAL_memCpy(csmServiceEvt_ptr->reasonDesc, csmIntSrvEvt_ptr->reasonDesc,
                sizeof(csmServiceEvt_ptr->reasonDesc));
    }
}

