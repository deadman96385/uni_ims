/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2012 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 30256 $ $Date: 2014-12-08 17:30:17 +0800 (Mon, 08 Dec 2014) $
 */

#include "csm_event.h"
#include <isi.h>
#include <isi_rpc.h>
#include <osal.h>
#include "_csm.h"
#include "_csm_print.h"
#include "_csm_isi.h"
#include "_csm_isi_service.h"
#include "_csm_response.h"
#ifdef INCLUDE_GBA
#include "gbam.h"
#endif

/* Trasnport protocl strings */
static const char *_CSM_transportProtoStrings[CSM_TRASPORT_PROTO_LAST + 1] = {
    "",
    ";transport=UDP",
    ";transport=TCP",
    ";transport=TLS",
};

/*
 *  ======== _CSM_isiServiceTypeEventHandler() ========
 *
 *  handler for ISI "service" type events
 *
 *  RETURN:
 *      None
 */
void _CSM_isiServiceTypeEventHandler(
    CSM_IsiMngr *isiMngr_ptr,
    ISI_Id       serviceId,
    ISI_Event    event,
    char        *desc_ptr)
{
    CSM_PrivateInputEvt    *event_ptr = &isiMngr_ptr->csmInputEvent;
    CSM_IsiServiceReason    reason;

    CSM_dbgPrintf("\n");

    event_ptr->type = CSM_PRIVATE_EVENT_TYPE_SERVICE;
    switch (event) {
        case ISI_EVENT_SERVICE_INIT_OK:
            reason = CSM_SERVICE_REASON_SERVICE_INIT_OK;
            break;
        case ISI_EVENT_SERVICE_INIT_FAILED:
            reason = CSM_SERVICE_REASON_SERVICE_INIT_FAILED;
            break;
        case ISI_EVENT_SERVICE_ACTIVATING:
            reason = CSM_SERVICE_REASON_SERVICE_ACTIVATING;
            break;
        case ISI_EVENT_SERVICE_ACTIVE:
            reason = CSM_SERVICE_REASON_SERVICE_ACTIVE;
            break;
        case ISI_EVENT_CREDENTIALS_REJECTED:
            reason = CSM_SERVICE_REASON_SERVICE_AUTH_FAIL;
            break;
        //case ISI_EVENT_NET_UNAVAILABLE:
        case ISI_EVENT_SERVICE_INACTIVE:
            reason = CSM_SERVICE_REASON_SERVICE_INACTIVE;
            break;
        case ISI_EVENT_SERVICE_HANDOFF:
            reason = CSM_SERVICE_REASON_SERVICE_HANDOFF;
            break;
        case ISI_EVENT_AKA_AUTH_REQUIRED:
            reason = CSM_SERVICE_REASON_ISIM_AKA_CHALLENGE;
            break;
        case ISI_EVENT_IPSEC_SETUP:
            reason = CSM_SERVICE_REASON_IPSEC_SETUP;
            break;
        case ISI_EVENT_IPSEC_RELEASE:
            reason = CSM_SERVICE_REASON_IPSEC_RELEASE;
            break;
        case ISI_EVENT_RCS_PROVISIONING:
            reason = CSM_SERVICE_REASON_RCS_PROVISIONING;
            break;
        default:
            return;
    };

    event_ptr->evt.service.serviceId = serviceId;
    event_ptr->evt.service.reason = reason;

    OSAL_strncpy(event_ptr->evt.service.reasonDesc, desc_ptr,
            sizeof(event_ptr->evt.service.reasonDesc));
    /* Notify Account package */
    CSM_isiSendEvent(isiMngr_ptr, event_ptr);
}

/*
 *  ======== _CSM_isiServiceSetCoders() ========
 *
 *  Private helper routine for setting up VoIP service coders.
 *
 *  RETURN:
 *      None
 */
static void _CSM_isiServiceSetCoders(
    CSM_IsiService *isiSrvc_ptr)
{
    char desc[128];
    vint dynamicPt = 98;

    /* Setup GAMR-WB Coder */

    /*
     * For Verzion requirement.
     * Add ";mode-set=2" in desc as default bit rate and set payload type to
     * 104 for AMR-WB bandwidth-efficient mode.
     * Add ";mode-set=2" in desc as default bit rate and set payload type to
     * 110 for AMR-WB octet-align mode.
     * Add ";mode-set=7" in desc as default bit rate and set payload type to
     * 102 for AMR bandwidth-efficient mode.
     * Add ";mode-set=7" in desc as default bit rate. And set payload type to
     * 108 for AMR octet-align mode.
     * 
     * For telephone-event, set payload type to 100.
     * For telephone-event-16k, set payload type to 105.
     */
    OSAL_snprintf(desc, sizeof(desc), "enum=%d;rate=%d", dynamicPt++, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "AMR-WB", desc);
    OSAL_snprintf(desc, sizeof(desc), "enum=%d;rate=%d;octet-align=1", dynamicPt++, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "AMR-WB", desc);
    /* Setup GAMR-NB Coder */
    OSAL_snprintf(desc, sizeof(desc), "enum=%d;rate=%d", dynamicPt++, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "AMR", desc);
    OSAL_snprintf(desc, sizeof(desc), "enum=%d;rate=%d;octet-align=1", dynamicPt++, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "AMR", desc);
    /* Setup PCMU Coder (easier IOT) */
    OSAL_snprintf(desc, sizeof(desc), "enum=%d;rate=%d", 0, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "PCMU", desc);
    /* Setup 16000 DTMF Relay, if enable GAMR-WB */
    OSAL_snprintf(desc, sizeof(desc),
           "enum=%d;rate=%d", dynamicPt++, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "telephone-event-16k", desc);
    /* Setup 8000 DTMF Relay, if enable GAMR-NB */
    OSAL_snprintf(desc, sizeof(desc),
           "enum=%d;rate=%d", dynamicPt++, 20);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "telephone-event", desc);
    /* Set up a video codec. */
    OSAL_snprintf(desc, sizeof(desc),
            "enum=%d;rate=%d;profile-level-id=42e020;packetization-mode=0",
            dynamicPt++, 0);
    ISI_addCoderToService(isiSrvc_ptr->serviceId, "h264", desc);
    return;
}

/*
 * ======== _CSM_isiServiceDestroy() ========
 *
 * Private helper routine for destroy a ISI service associated to a protocol.
 * This function will be called when a protocol died.
 *
 *  RETURN:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint _CSM_isiServiceDestroy(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocol)
{
    ISI_Return r;
    vint       idx;
    vint       allocEmergency;

    CSM_dbgPrintf("\n");

    /*
     * Allocate both normal sip service and emergency sip service.
     * Don't need  to create emergency service for GSM.
     */
    if (CSM_ISI_PROTOCOL_SIP == protocol) {
        allocEmergency = 1;
    }
    else {
        allocEmergency = 0;
    }

    for (idx = 0; idx <= allocEmergency; idx++) {
        /* Get the service object from ISI manager.  Get one that is available. */
        CSM_IsiService *isiSrvc_ptr =
                CSM_isiGetServiceViaProtocol(serviceMngr_ptr->isiMngr_ptr,
                        protocol, idx);
        if (NULL == isiSrvc_ptr) {
            OSAL_logMsg("Could not find CSM ISI service\n");
            return (CSM_ERR);
        }
        /* Deactivate the service anyway before free the service. */
        ISI_deactivateService(isiSrvc_ptr->serviceId);
        
        /* Free the service */
        r = ISI_freeService(isiSrvc_ptr->serviceId);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not free service %d ERROR:%s\n",
                    isiSrvc_ptr->serviceId,
                    CSM_isiPrintReturnString(r));
            return (CSM_ERR);
        }

        FSM_destroy(&isiSrvc_ptr->fsm);
    }

    // All is well.
    return (CSM_OK);
}

/*
 *  ======== _CSM_isiServiceCreate() ========
 *
 *  Private helper routine for setting up VoIP service settings.  For example,
 *  Domain, IMPU, IMPI, PCSCF, SIP/RTP/RTCP ports, etc.
 *
 *  RETURN:
 *      CSM_OK: function exits normally.
 *      CSM_ERR: in case of error
 */
vint _CSM_isiServiceCreate(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocol,
    CSM_OutputEvent *csmOutput_ptr)
{
    ISI_Return r;
    vint       idx;
    vint       allocEmergency;

    CSM_dbgPrintf("\n");

    /*
     * Allocate both normal sip service and emergency sip service.
     * Don't need  to create emergency service for GSM.
     */
    if (CSM_ISI_PROTOCOL_SIP == protocol) {
        allocEmergency = 1;
    }
    else {
        allocEmergency = 0;
    }

    for (idx = 0; idx <= allocEmergency; idx++) {
        /* Get the service object from ISI manager.  Get one that is available. */
        CSM_IsiService *isiSrvc_ptr =
                CSM_isiGetServiceViaProtocol(serviceMngr_ptr->isiMngr_ptr,
                        protocol, idx);
        if (NULL == isiSrvc_ptr) {
            OSAL_logMsg("Could not init the CSM ISI service\n");
            return (CSM_ERR);
        }

        /* Allocate the service */
        r = ISI_allocService(&isiSrvc_ptr->serviceId, protocol);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not allocate service ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
            return (CSM_ERR);
        }
        OSAL_logMsg("An ISI service was allocated\n");

        /* Init the state machine for logging in. */
        if (CSM_OK != FSM_init(&isiSrvc_ptr->fsm, serviceMngr_ptr,
                isiSrvc_ptr->serviceId, csmOutput_ptr)) {
            OSAL_logMsg("Could not init FSM for the ISI service");
            return (CSM_ERR);
        }
    }

    // All is well.
    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceSetup() ========
 *
 * Private function to set the service and account information to ISI.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceSetup(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId)
{
    ISI_Return r;
    char *outbound_ptr;
    char *uname_ptr;

    /* Get the service object from ISI manager. */
    CSM_IsiService *isiSrvc_ptr = CSM_isiGetServiceViaId(
            serviceMngr_ptr->isiMngr_ptr, serviceId);

    CSM_dbgPrintf("\n");

    /* Set Emergency */
    r = ISI_serviceSetEmergency(isiSrvc_ptr->serviceId,
            isiSrvc_ptr->isEmergency);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set Emergency ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    /* Set URI */
    r = ISI_serviceSetUri(isiSrvc_ptr->serviceId, serviceMngr_ptr->uri);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set URI ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }
    OSAL_strncpy(isiSrvc_ptr->uri, serviceMngr_ptr->uri,
            sizeof(isiSrvc_ptr->uri));

    /* Set CID private */
    r = ISI_serviceMakeCidPrivate(isiSrvc_ptr->serviceId, 0);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set Cid Private ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    /*
     * If 'outbound proxy' is set then issue that command
     * but NOT the 'proxy' command.  IF 'outbound proxy is NOT set,
     * then don't issue the command; rather, issue the 'proxy' command
     */
    if (isiSrvc_ptr->isEmergency &&
            (0 == RPM_isEmergencyRegRequired())) {
        r = ISI_serviceSetServer(isiSrvc_ptr->serviceId, CSM_ISI_EMPTY_STRING,
                ISI_SERVER_TYPE_REGISTRAR);
    }
    else {
        r = ISI_serviceSetServer(isiSrvc_ptr->serviceId, serviceMngr_ptr->proxy,
            ISI_SERVER_TYPE_REGISTRAR);
    }
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set Registrar ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }
    OSAL_strncpy(isiSrvc_ptr->proxy, serviceMngr_ptr->proxy,
            sizeof(isiSrvc_ptr->proxy));

    /* Use eObProxy(emergency pcscf) as obproxy if the service is emergency */
    if (isiSrvc_ptr->isEmergency) {
        if (0 != serviceMngr_ptr->eObProxy[0]) {
            outbound_ptr = serviceMngr_ptr->eObProxy;
        }
        else {
            return (CSM_ERR);
        }
    }
    else {
        outbound_ptr = serviceMngr_ptr->obProxy;
    }

    if (outbound_ptr && outbound_ptr[0] != 0) {
        r = ISI_serviceSetServer(isiSrvc_ptr->serviceId,
                serviceMngr_ptr->obProxy,
                ISI_SERVER_TYPE_OUTBOUND_PROXY);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set Proxy ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
            return (CSM_ERR);
        }
    }
    else {
        r = ISI_serviceSetServer(isiSrvc_ptr->serviceId,
                serviceMngr_ptr->proxy,
                ISI_SERVER_TYPE_PROXY);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set Registrar ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
        }
    }
    OSAL_strncpy(isiSrvc_ptr->outProxy, outbound_ptr,
            sizeof(isiSrvc_ptr->outProxy));

    /* Set audioconf to isi service */
    OSAL_strncpy(isiSrvc_ptr->audioconf, serviceMngr_ptr->audioconf,
            sizeof(isiSrvc_ptr->audioconf));
    /* Set video conf server to isi service */
    if (0 != serviceMngr_ptr->videoconf[0]) {
        OSAL_strncpy(isiSrvc_ptr->videoconf, serviceMngr_ptr->videoconf,
                sizeof(isiSrvc_ptr->videoconf));
    }
    else {
        /* If video is not set, use audio conf server. */
        OSAL_strncpy(isiSrvc_ptr->videoconf, serviceMngr_ptr->audioconf,
                sizeof(isiSrvc_ptr->videoconf));
    }
    /* Set imconf to isi service */
    if (0 != serviceMngr_ptr->chatConfUri[0]) {
        r = ISI_serviceSetServer(isiSrvc_ptr->serviceId,
                serviceMngr_ptr->chatConfUri, ISI_SERVER_TYPE_CHAT);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set Chat Conference Server ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
            return (CSM_ERR);
        }
    }
    OSAL_strncpy(isiSrvc_ptr->chatConfUri, serviceMngr_ptr->chatConfUri,
            sizeof(isiSrvc_ptr->chatConfUri));
    /* Set up UserSettings parameters */
    if (0 != serviceMngr_ptr->authname[0]) {
        uname_ptr = &serviceMngr_ptr->authname[0];
    }
    else {
        uname_ptr = &serviceMngr_ptr->username[0];
    }
    r = ISI_serviceSetCredentials(isiSrvc_ptr->serviceId, uname_ptr,
            serviceMngr_ptr->password, serviceMngr_ptr->realm);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set Credentials ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }
    OSAL_strncpy(isiSrvc_ptr->username, uname_ptr,
            sizeof(isiSrvc_ptr->username));
    OSAL_strncpy(isiSrvc_ptr->realm, serviceMngr_ptr->realm,
            ISI_ADDRESS_STRING_SZ);
    OSAL_strncpy(isiSrvc_ptr->uri, serviceMngr_ptr->uri,
            sizeof(isiSrvc_ptr->uri));

    _CSM_isiServiceSetCoders(isiSrvc_ptr);

    /* Set IMEI */
    CSM_isiServiceSetImeiUri(serviceMngr_ptr, isiSrvc_ptr);
    /* Set sip and media ports */
    CSM_isiServiceSetPorts(serviceMngr_ptr, isiSrvc_ptr);
    /* Set protected ports and spis */
    CSM_isiServiceSetIpsec(serviceMngr_ptr, isiSrvc_ptr);
    /* Set instance id */
    CSM_isiServiceSetInstanceId(serviceMngr_ptr, isiSrvc_ptr);

    /* Set RCS provisioning data */
    CSM_isiServiceSetRcsProvisioningData(serviceMngr_ptr, isiSrvc_ptr);

    /* Set CGI */
    CSM_isiServiceUpdateCgi(serviceMngr_ptr, isiSrvc_ptr);

    // All is well.
    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceSetPorts() ========
 *
 * Private helper routine for setting up port number for VoIP service
 *
 * RETURN:
 *    None
 */
vint CSM_isiServiceSetPorts(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr)
{
    ISI_Return r;

    r = ISI_serviceSetPort(isiSrvc_ptr->serviceId,
            serviceMngr_ptr->sipPort, 1, ISI_PORT_TYPE_SIP);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set sip port ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    r = ISI_serviceSetPort(isiSrvc_ptr->serviceId,
            serviceMngr_ptr->audioRtpPort, serviceMngr_ptr->audioPoolSize,
            ISI_PORT_TYPE_AUDIO);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set audio rtp port ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    r = ISI_serviceSetPort(isiSrvc_ptr->serviceId,
            serviceMngr_ptr->videoRtpPort, serviceMngr_ptr->videoPoolSize,
            ISI_PORT_TYPE_VIDEO);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set video rtp port ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceSetIpsec() ========
 *
 * Private helper routine for setting up IPSec information
 *
 * RETURN:
 *     None
 */
vint CSM_isiServiceSetIpsec(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr)
{
    ISI_Return r;

    r = ISI_serviceSetIpsec(isiSrvc_ptr->serviceId,
            serviceMngr_ptr->ipsec.protectedPort,
            serviceMngr_ptr->ipsec.protectedPortPoolSz,
            serviceMngr_ptr->ipsec.spi,
            serviceMngr_ptr->ipsec.spiPoolSz);

    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set IPSec info ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceSetImeiUri() ========
 *
 * For setting up Imei URI for VoIP service
 *
 * RETURN:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint CSM_isiServiceSetImeiUri(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr)
{
    ISI_Return r;

    r = ISI_serviceSetImeiUri(isiSrvc_ptr->serviceId,
            serviceMngr_ptr->imeiUri);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set IMEI URI ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceSetRcsProvisioningData() ========
 *
 *  Function to set RCS provisioning data to underlying protocol.
 *
 * RETURN:
 *     CSM_OK: function exits normally.
 *     CSM_ERR: in case of error
 */
vint CSM_isiServiceSetRcsProvisioningData(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr)
{
    ISI_Return r;

    /* Set RCS provisioning data only when it's enabled and provisioned */
    if (OSAL_TRUE == serviceMngr_ptr->isRcsProvisioningEnabled) {
        if (OSAL_FALSE == serviceMngr_ptr->isRcsDataProvisioned) {
            OSAL_logMsg("%s %d: ERROR. Set RCS provisioning data but it's "
                    "not provisioned\n", __FUNCTION__, __LINE__);
            return (CSM_ERR);
        }
        /* Set it if it's not empty. */
        r = ISI_setProvisioningData(isiSrvc_ptr->serviceId,
                serviceMngr_ptr->rcsProvisioningData);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set RCS provisioning data. ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
            return (CSM_ERR);
        }
    }
    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceSetInstanceId() ========
 *
 *  for setting up the instance id for VoIP service
 *
 * RETURN:
 *     CSM_OK: function exits normally.
 *     CSM_ERR: in case of error
 */
vint CSM_isiServiceSetInstanceId(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr)
{
    ISI_Return r;

    /* Set UA instance Id */
    if (0 != serviceMngr_ptr->instanceId[0]) {
        /* Set it if it's not empty. */
        r = ISI_serviceSetInstanceId(isiSrvc_ptr->serviceId,
                serviceMngr_ptr->instanceId);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set UA Instance ID ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
            return (CSM_ERR);
        }
    }
    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceUpdateCgi() ========
 *
 * Update CGI (cell global identify) information
 *
 * RETURN:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint CSM_isiServiceUpdateCgi(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *isiSrvc_ptr)
{
    ISI_Return r;
    ISI_NetworkAccessType type;

    /* No need to set CGI if it's not given, i.e. empty string */
    if (0 == serviceMngr_ptr->cgi.id[0]) {
        return (CSM_OK);
    }

    /* Convert CSM network access type to ISI type */
    switch (serviceMngr_ptr->cgi.type) {
        case CSM_SERVICE_ACCESS_TYPE_3GPP_GERAN:
            type = ISI_NETWORK_ACCESS_TYPE_3GPP_GERAN;
            break;
        case CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_FDD:
            type = ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_FDD;
            break;
        case CSM_SERVICE_ACCESS_TYPE_3GPP_UTRAN_TDD:
            type = ISI_NETWORK_ACCESS_TYPE_3GPP_UTRAN_TDD;
            break;
        case CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_FDD:
            type = ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_FDD;
            break;
        case CSM_SERVICE_ACCESS_TYPE_3GPP_E_UTRAN_TDD:
            type = ISI_NETWORK_ACCESS_TYPE_3GPP_E_UTRAN_TDD;
            break;
        case CSM_SERVICE_ACCESS_TYPE_IEEE_802_11:
            type = ISI_NETWORK_ACCESS_TYPE_IEEE_802_11;
            break;
        default:
            type = ISI_NETWORK_ACCESS_TYPE_NONE;
            break;
    }

    r = ISI_serviceSetBsid(isiSrvc_ptr->serviceId,
            type, serviceMngr_ptr->cgi.id);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not update CGI ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceUpdateTransport() ========
 *
 * Private helper routine for updating the signalling's transport protocol
 *
 * RETURN:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
static vint _CSM_isiServiceUpdateTransport(
    CSM_ServiceMngr *serviceMngr_ptr,
    CSM_IsiService  *service_ptr,
    RPM_RadioType    radioType)
{
    char              *proxy_ptr;
    char              *pos_ptr;
    uint32             len;
    CSM_TransportProto proto;
    ISI_ServerType     serverType;
    ISI_Return         r;
    char              *tmp_ptr;
    char               proxyStr[CSM_EVENT_LONG_STRING_SZ + 1];
    CSM_IsiService    *isiSrvc_ptr;

    /* Get the service object for this service ID */
    isiSrvc_ptr = CSM_isiGetServiceViaId(serviceMngr_ptr->isiMngr_ptr,
            service_ptr->serviceId);
    /* Use eObProxy(emergency pcscf) as obproxy if the service is emergency */
    if (isiSrvc_ptr->isEmergency) {
        if (0 != serviceMngr_ptr->eObProxy[0]) {
            proxy_ptr = serviceMngr_ptr->eObProxy;
            len = sizeof(serviceMngr_ptr->eObProxy);
            serverType = ISI_SERVER_TYPE_OUTBOUND_PROXY;
         }
         else {
            return (CSM_ERR);
        }
    }
    else {
        /* Get proxy or outbound proxy */
        if (0 != serviceMngr_ptr->obProxy[0]) {
            proxy_ptr = serviceMngr_ptr->obProxy;
            len = sizeof(serviceMngr_ptr->obProxy);
            serverType = ISI_SERVER_TYPE_OUTBOUND_PROXY;
        }
        else {
            proxy_ptr = serviceMngr_ptr->proxy;
            len = sizeof(serviceMngr_ptr->proxy);
            serverType = ISI_SERVER_TYPE_PROXY;
        }
    }

    /* Get transport protocol depends on wifi or 4G */
    if (RPM_RADIO_TYPE_WIFI == radioType) {
        proto = serviceMngr_ptr->transportProto.wifiSignalling;
    }
    else {
        /* It's 4G mode */
        proto = serviceMngr_ptr->transportProto.psSignalling;
    }

    if (OSAL_strncasescan(proxy_ptr, len, ",")) {
        /* Process multiple p-cscf addresses */
        OSAL_memSet(proxyStr, 0, CSM_EVENT_LONG_STRING_SZ + 1);
        tmp_ptr = OSAL_strtok(proxy_ptr, ",");
        while (NULL != tmp_ptr) {
            /* Get rid of the transport parameter */
            if (NULL == (pos_ptr = OSAL_strncasescan(tmp_ptr, len, ";"))) {
                pos_ptr = tmp_ptr + OSAL_strlen(tmp_ptr);
            }

            /* Put the string */
            OSAL_snprintf(proxyStr + OSAL_strlen(proxyStr),
                    CSM_EVENT_LONG_STRING_SZ + 1, "%s", tmp_ptr);
            OSAL_snprintf(proxyStr + OSAL_strlen(proxyStr) - OSAL_strlen(pos_ptr),
                    CSM_EVENT_LONG_STRING_SZ + 1, "%s,",
                    _CSM_transportProtoStrings[proto]);

            /* Get next p-cscf address*/
            tmp_ptr = OSAL_strtok(NULL, ",");
        }
        proxyStr[OSAL_strlen(proxyStr) -1] = '\n';
        OSAL_snprintf(proxy_ptr, len, "%s", proxyStr);
    }
    else {
        /* Get rid of the transport parameter */
        if (NULL == (pos_ptr = OSAL_strncasescan(proxy_ptr, len,
                ";"))) {
            pos_ptr = proxy_ptr + OSAL_strlen(proxy_ptr);
        }

        /* put the string */
        OSAL_snprintf(proxy_ptr + (pos_ptr - proxy_ptr),
                len - (pos_ptr - proxy_ptr), "%s",
                _CSM_transportProtoStrings[proto]);
    }
    CSM_dbgPrintf("Proxy:%s\n", proxy_ptr);

    /* Set updated proxy to underlying protocol */
    r = ISI_serviceSetServer(service_ptr->serviceId,
            proxy_ptr, serverType);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not set Proxy ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceActivate() ========
 *
 * Private helper routine for activating an already initialized ISI service.
 *
 * RETURN:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceActivate(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId)
{
    CSM_IsiService    *service_ptr;
    RPM_RadioInterface radioInfc;
    char               ipAddress[OSAL_NET_IPV6_STR_MAX ];
    ISI_Return         r;
    RPM_FeatureType    featureType;
    char              *infcName_ptr;
    char              *registrar_ptr;
    OSAL_NetAddress    addr;

    /* Get the service object for this service ID */
    service_ptr = CSM_isiGetServiceViaId(serviceMngr_ptr->isiMngr_ptr,
            serviceId);

    if (NULL == service_ptr) {
        CSM_dbgPrintf("Cannot find service.\n");
        return (CSM_ERR);
    }

    if ((CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) ||
            (CSM_ISI_PROTOCOL_SIP_RCS == service_ptr->protocol)) {
        if (service_ptr->isEmergency) {
            featureType = RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE;
        }
        else {
            featureType = RPM_FEATURE_TYPE_IMS_SERVICE;
        }
        /* get ip address by RPM */
        RPM_getAvailableRadio(featureType, &radioInfc);

        /* Don't activate sip services if there is no ip address */
        if (OSAL_netIsAddrZero(&radioInfc.ipAddr)) {
            return (CSM_ERR);
        }

        /* Convert to network byte order first. */
        OSAL_netAddrHton(&addr, &radioInfc.ipAddr);
        /* Conver IP address to string */
        OSAL_netAddressToString((int8 *)ipAddress, &addr);
        /*
         * Handle register of ASAPP and MSAPP.
         * In 4G mode, don't do register on ASAPP.
         * In wifi mode, don't do register on MSAPP.
         */
         /* Set Real time media */
        if (RPM_RADIO_TYPE_WIFI == radioInfc.radioType) {
            infcName_ptr = CSM_ISI_NETWORK_INFC_NAME_WIFI;
            if (CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) {
                /*
                 * MSAPP on wifi mode, set empty registrar so that it won't
                 * register.
                 */
                registrar_ptr = CSM_ISI_EMPTY_STRING;
            }
            else {
                /*
                 * ASAPP on wifi mode, set registrar so that it will register.
                 */
                registrar_ptr = serviceMngr_ptr->proxy;
            }
            service_ptr->rtMedia = 
                    serviceMngr_ptr->transportProto.wifiRTMedia;
        }
        else {
            infcName_ptr = CSM_ISI_NETWORK_INFC_NAME_4G;
            if (CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) {
                /*
                 * MSAPP on 4G mode, set registrar so that it will register.
                 */
                registrar_ptr = serviceMngr_ptr->proxy;
                if (service_ptr->isEmergency &&
                        (0 == RPM_isEmergencyRegRequired())) {
                    registrar_ptr = CSM_ISI_EMPTY_STRING;
                }
                else {
                    registrar_ptr = serviceMngr_ptr->proxy;
                }           
            }
            else {
                /*
                 * ASAPP on 4G mode, set empty registrar so that it won't
                 * register.
                 */
                registrar_ptr = CSM_ISI_EMPTY_STRING;
            }
        }
        /* Set the service IP address and interface name */
        ISI_serviceSetInterface(serviceId, infcName_ptr, ipAddress);
        OSAL_netAddrCpy(&((CSM_ServiceMngr *) service_ptr->fsm.serviceMngr_ptr)
                ->regIpAddress, &radioInfc.ipAddr);
        /* Set registrar */
        r = ISI_serviceSetServer(service_ptr->serviceId, registrar_ptr,
                ISI_SERVER_TYPE_REGISTRAR);
        if (r != ISI_RETURN_OK) {
            OSAL_logMsg("Could not set Registrar ERROR:%s\n",
                    CSM_isiPrintReturnString(r));
        }
        service_ptr->rtMedia = 
                    serviceMngr_ptr->transportProto.psRTMedia;
    }

    /* Update signalling transport protocol */
    if (CSM_OK != _CSM_isiServiceUpdateTransport(serviceMngr_ptr, service_ptr,
        radioInfc.radioType)) {
        OSAL_logMsg("Update service transport error\n");
        return (CSM_ERR);
    }

    /* Activate the service */
    r = ISI_activateService(serviceId);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not activate the service ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 *  ======== _CSM_isiServiceDeactivate() ========
 *
 *  Private helper routine for deactivating an already activated ISI service.
 *
 *  RETURN:
 *      ISI_Return
 */
vint _CSM_isiServiceDeactivate(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId)
{
    ISI_Return      r;
    CSM_IsiService *service_ptr;

    /* Get the service object for this service ID */
    service_ptr = CSM_isiGetServiceViaId(serviceMngr_ptr->isiMngr_ptr,
            serviceId);

    if (NULL == service_ptr) {
        CSM_dbgPrintf("Cannot find service.\n");
        return (CSM_ERR);
    }
    r = ISI_deactivateService(serviceId);
    if (r != ISI_RETURN_OK) {
        OSAL_logMsg("Could not activate the service ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceSendAkaChallenge() ========
 *
 * Private function to get AKA challenge from ISI and sent it to ISIM
 * response queue.
 *
 * Returns:
 *   CSM_OK: Failed to get AKA challenge or send to ISIM response queue.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceSendAkaChallenge(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId,
    CSM_OutputEvent *csmOutput_ptr)
{
    char rand[CSM_AKA_RAND_STRING_SZ];
    char autn[CSM_AKA_AUTN_STRING_SZ];

    ISI_Return r;

    /* Get AKA challenge */
    r = ISI_getAkaAuthChallenge(serviceId, rand, autn);
    if (r != ISI_RETURN_OK) {
        CSM_dbgPrintf("Failed to get AKA auth challenge. ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

#ifdef INCLUDE_GBA
    /* added AKA arbiter for GBA could send AKA request as well */
    GBAM_akaArbitorSetTarget(GBAM_AKA_ISI_REQ);
#endif

    /* Send AKA challenge to ISIM module */
    CSM_sendAkaChallenge(rand, autn, csmOutput_ptr);
    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceSendIpsecEvent() ========
 *
 * Private function to send IPSec protected ports and SPIs information
 * to CSM output event.
 *
 * Returns:
 *   CSM_OK: Failed to send to output event queue.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceSendIpsecEvent(
    ISI_Id            serviceId,
    vint              reason,
    CSM_OutputEvent  *csmOutput_ptr)
{
    ISI_Return r;
    int  portUc, portUs, portPc, portPs;
    int  spiUc, spiUs, spiPc, spiPs;

    /* Get IPSec info */
    r = ISI_serviceGetIpsec(serviceId,
            &portUc, &portUs, &portPc, &portPs,
            &spiUc, &spiUs, &spiPc, &spiPs);

    if (r != ISI_RETURN_OK) {
        CSM_dbgPrintf("Failed to get IPSec auth information. ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }

    /* Send IPSec ports and spi info */
    CSM_sendIpsecEvent(reason, portUc, portUs, portPc, portPs,
            spiUc, spiUs, spiPc, spiPs, csmOutput_ptr);

    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceSetAkaResponse() ========
 *
 * Private function to set AKA resposne to ISI.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceSetAkaResponse(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId)
{
    ISI_Return r;

    r = ISI_setAkaAuthResp(serviceId,
            serviceMngr_ptr->aka.result,
            serviceMngr_ptr->aka.response,
            serviceMngr_ptr->aka.resLength,
            serviceMngr_ptr->aka.auts,
            serviceMngr_ptr->aka.ck,
            serviceMngr_ptr->aka.ik);

    if (r != ISI_RETURN_OK) {
        CSM_dbgPrintf("Could not set aka auth response ERROR:%s\n",
                CSM_isiPrintReturnString(r));
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceProcessFsmEvent() ========
 *
 * Private helper routine for processing CSM events in the Service FSM.
 *
 * RETURN:
 *   ISI_Return
 */
vint _CSM_isiServiceProcessFsmEvent(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId,
    vint             reason,
    const char      *reason_ptr)
{
    CSM_IsiService *service_ptr;
    FSM_Event event;

    CSM_dbgPrintf("\n");

    /* Get the service object for this service ID */
    service_ptr = CSM_isiGetServiceViaId(serviceMngr_ptr->isiMngr_ptr,
            serviceId);
    if (NULL != service_ptr) {
        /* Map the correct FSM Event type */
        switch (reason) {
            case CSM_SERVICE_EVT_REASON_SERVICE_ACTIVE:
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_SERVICE_ACTIVE\n");
                event.eventType = FSM_EVENT_TYPE_ACTIVE;
                OSAL_strncpy(event.reasonDesc, reason_ptr,
                        sizeof(event.reasonDesc));

                break;
            case CSM_SERVICE_EVT_REASON_SERVICE_ACTIVATING:
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_SERVICE_ACTIVATING\n");
                event.eventType = FSM_EVENT_TYPE_ACTIVATING;
                OSAL_strncpy(event.reasonDesc, reason_ptr,
                        sizeof(event.reasonDesc));

                break;
            case CSM_SERVICE_EVT_REASON_SERVICE_RETRY:
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_SERVICE_RETRY\n");
                event.eventType = FSM_EVENT_TYPE_TIMER;
                break;
            case CSM_SERVICE_EVT_REASON_SERVICE_INACTIVE:
                /* Let's get more details about the error and then feed that
                 * into the FSM. */
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_SERVICE_INACTIVE "
                        "Desc:%s\n", reason_ptr);
                FSM_getEventTypeFromReason(reason_ptr, &event);
                break;
            case CSM_SERVICE_EVT_REASON_SERVICE_AUTH_FAIL:
                /*
                 * Let's get more details about the error and then feed that
                 * into the FSM.
                 */
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_SERVICE_AUTH_FAIL "
                        "Desc:%s\n", reason_ptr);
                event.eventType = FSM_EVENT_TYPE_AUTH_FAIL;
                break;
            case CSM_SERVICE_EVT_REASON_SERVICE_INIT_OK:
            case CSM_SERVICE_EVT_REASON_RCS_PROVISIONING:
            case CSM_SERVICE_EVT_REASON_IMS_ENABLE:
            case CSM_SERVICE_EVT_REASON_ACTIVATE:
                event.eventType = FSM_EVENT_TYPE_START;
                break;
            case CSM_SERVICE_EVT_REASON_DEACTIVATE:
            case CSM_SERVICE_EVT_REASON_IMS_DISABLE:
                event.eventType = FSM_EVENT_TYPE_STOP;
                break;
            case CSM_SERVICE_EVT_REASON_ISIM_AKA_CHALLENGE:
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_ISIM_AKA_CHALLENGE "
                        "Desc:%s\n", reason_ptr);
                event.eventType = FSM_EVENT_TYPE_AKA_REQUIRED;
                break;
            case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SUCCESS:
            case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_NETWORK_FAILURE:
            case CSM_SERVICE_EVT_REASON_ISIM_AKA_RESPONSE_SYNC_FAILURE:
                CSM_dbgPrintf("CSM_ACCOUNT_REASON_ISIM_AKA_RESPONSE "
                        "Desc:%s\n", reason_ptr);
                event.eventType = FSM_EVENT_TYPE_AKA_RESPONSE;
                break;
            default:
                /* No event to process */
                return (CSM_OK);
        }
        /* Process the event */
        OSAL_strncpy(event.reasonDesc, reason_ptr, CSM_EVENT_STRING_SZ);
        FSM_processEvent(&service_ptr->fsm, &event);
    }
    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceProcessIpChange() ========
 *
 * Private helper routine for processing CSM "IP change" callbacks
 *
 * RETURN:
 *   none
 */
void _CSM_isiServiceProcessIpChange(
    CSM_ServiceMngr     *serviceMngr_ptr,
    RPM_RadioType        radioType,
    RPM_RadioInterface  *radioInfc_ptr)
{
    CSM_IsiService *service_ptr;
    CSM_IsiService *slaveSipService_ptr;
    FSM_Event       event;
    vint            isEmergency;
    vint            protocolId;
    vint            slaveSipProtocolId;
    char            ipAddress[OSAL_NET_IPV6_STR_MAX];
    OSAL_NetAddress addr = radioInfc_ptr->ipAddr;
    char           *infcName_ptr;

    CSM_dbgPrintf("\n");

    /* Get service by protocol id and isEmergency */
    if (RPM_RADIO_TYPE_LTE_EMERGENCY == radioType) {
        /* Emergency ip address */
        isEmergency = 1;
        protocolId = CSM_ISI_PROTOCOL_MODEM_IMS;
        slaveSipProtocolId = 0;
    }
    else if (RPM_RADIO_TYPE_LTE == radioType) {
        /* Lte ip address */
        isEmergency = 0;
        protocolId = CSM_ISI_PROTOCOL_MODEM_IMS;
        slaveSipProtocolId = CSM_ISI_PROTOCOL_SIP_RCS;
    }
    else if (RPM_RADIO_TYPE_WIFI == radioType) {
        /* Wifi ip address */
        isEmergency = 0;
        protocolId = CSM_ISI_PROTOCOL_SIP_RCS;
        slaveSipProtocolId = CSM_ISI_PROTOCOL_MODEM_IMS;
    }
    else {
        /* Others, no ip address, should notify both sip service */
        isEmergency = 0;
        protocolId = CSM_ISI_PROTOCOL_MODEM_IMS;
        slaveSipProtocolId = CSM_ISI_PROTOCOL_SIP_RCS;
    }
    service_ptr = CSM_isiGetServiceViaProtocol(serviceMngr_ptr->isiMngr_ptr,
            protocolId, isEmergency);
    slaveSipService_ptr = CSM_isiGetServiceViaProtocol(
            serviceMngr_ptr->isiMngr_ptr, slaveSipProtocolId, 0);

    /*
     * Notify slave sip service if needed
     * Need to clear previous master service's isMasterSip first otherwise
     * it will get wrong master service later in FSM_processEvent().
     */
    if (NULL != slaveSipService_ptr) {
        /* Update isMasterSip flag */
        slaveSipService_ptr->isMasterSip = 0;
    }

    /* Notify master sip service or emergency sip service */
    if (NULL != service_ptr) {
        if (!isEmergency) {
            /* Update isMasterSip flag */
            service_ptr->isMasterSip = 1;
        }
        /* Run FSM only when protocol regsiterred. */
        if (service_ptr->isRegistered && _CSM_serviceIsReady(serviceMngr_ptr,
                service_ptr)) {
            /* Convert to network byte order first. */
            OSAL_netAddrHton(&addr, &radioInfc_ptr->ipAddr);
            /* Conver IP address to string */
            OSAL_netAddressToString((int8 *)ipAddress, &addr);
             /* Set the service IP address and interface name */
            if (RPM_RADIO_TYPE_WIFI == radioInfc_ptr->radioType) {
                infcName_ptr = CSM_ISI_NETWORK_INFC_NAME_WIFI;
            }
            else {
                infcName_ptr = CSM_ISI_NETWORK_INFC_NAME_4G;
            }
            ISI_serviceSetInterface(service_ptr->serviceId, infcName_ptr,
                    ipAddress);

            /* Run master sip service's fsm */
            if (OSAL_FALSE == OSAL_netIsAddrZero(&addr)) {
                event.eventType = FSM_EVENT_TYPE_NEW_IP;
                OSAL_netAddrCpy(&event.ipAddress, &radioInfc_ptr->ipAddr);
                FSM_processEvent(&service_ptr->fsm, &event);
            }
        }
    }

    return;
}


/*
 * ======== _CSM_isiServiceProcessEmerRegChange() ========
 *
 * Private helper callback to process emergency registration required change.
 *
 * RETURN:
 *   none
 */
void _CSM_isiServiceProcessEmerRegChange(
    CSM_ServiceMngr     *serviceMngr_ptr)
{
    CSM_IsiService *service_ptr;
    FSM_Event       event;
    RPM_RadioInterface radioInfc;

    CSM_dbgPrintf("\n");

    service_ptr = CSM_isiGetServiceViaProtocol(serviceMngr_ptr->isiMngr_ptr,
            CSM_ISI_PROTOCOL_MODEM_IMS, 1);

    /* Notify master sip service or emergency sip service */
    if (NULL != service_ptr) {
        /* Run FSM only when protocol regsiterred. */
        if (service_ptr->isRegistered && _CSM_serviceIsReady(serviceMngr_ptr,
                service_ptr)) {
            /* get ip address by RPM */
            RPM_getAvailableRadio(RPM_FEATURE_TYPE_IMS_EMERGENCY_SERVICE,
                    &radioInfc);

            /* Don't activate sip services if there is no ip address */
            if (OSAL_FALSE == OSAL_netIsAddrZero(&radioInfc.ipAddr)) {
                /* Run service's fsm */
                event.eventType = FSM_EVENT_TYPE_START;
                FSM_processEvent(&service_ptr->fsm, &event);
            }
        }
    }
}

/*
 * ======== _CSM_isiServiceSendRetry() ========
 *
 * Private helper routine generating a retry event
 *
 * RETURN:
 *  None 
 */
void _CSM_isiServiceSendRetry(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId)
{
    CSM_PrivateInputEvt eventObj;
    /* Construct the event */
    eventObj.type = CSM_PRIVATE_EVENT_TYPE_SERVICE;
    eventObj.evt.service.serviceId = serviceId;
    eventObj.evt.service.reason = CSM_SERVICE_REASON_SERVICE_RETRY;
    eventObj.evt.service.reasonDesc[0] = 0;
    /* Notify Account package */
    CSM_isiSendEvent(serviceMngr_ptr->isiMngr_ptr, &eventObj);
}

/*
 * ======== _CSM_isiServiceDectivateSlaveSip() ========
 *
 * Private helper routine to send event to deactivate slave sip service.
 *
 * RETURN:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceDeactivateSlaveSip(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocolMasterSip)
{
    CSM_PrivateInputEvt     eventObj;
    CSM_IsiService         *service_ptr;
    vint                    protocolSlaveSip;

    if (CSM_ISI_PROTOCOL_MODEM_IMS == protocolMasterSip) {
        /* Master sip is the sip in modem process, then find rcs sip */
        protocolSlaveSip = CSM_ISI_PROTOCOL_SIP_RCS;
    }
    else {
        protocolSlaveSip = CSM_ISI_PROTOCOL_MODEM_IMS;
    }

    service_ptr = CSM_isiGetServiceViaProtocol(serviceMngr_ptr->isiMngr_ptr,
            protocolSlaveSip, 0);

    if (NULL == service_ptr) {
        /* No slave sip service found */
        CSM_dbgPrintf("No slave sip service found\n");
        return (CSM_ERR);
    }

    /* Construct the event */
    eventObj.type = CSM_PRIVATE_EVENT_TYPE_SERVICE;
    eventObj.evt.service.serviceId = service_ptr->serviceId;
    eventObj.evt.service.reason = CSM_SERVICE_REASON_DEACTIVATE;
    eventObj.evt.service.reasonDesc[0] = 0;
    /* Notify Account package */
    CSM_isiSendEvent(serviceMngr_ptr->isiMngr_ptr, &eventObj);

    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceActivateSlaveSip() ========
 *
 * Private helper routine to send event to activate slave sip service.
 *
 * RETURN:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceActivateSlaveSip(
    CSM_ServiceMngr *serviceMngr_ptr,
    vint             protocolMasterSip)
{
    CSM_PrivateInputEvt     eventObj;
    CSM_IsiService         *service_ptr;
    vint                    protocolSlaveSip;

    if (CSM_ISI_PROTOCOL_MODEM_IMS == protocolMasterSip) {
        /* Master sip is the sip in modem process, then find rcs sip */
        protocolSlaveSip = CSM_ISI_PROTOCOL_SIP_RCS;
    }
    else {
        protocolSlaveSip = CSM_ISI_PROTOCOL_MODEM_IMS;
    }

    service_ptr = CSM_isiGetServiceViaProtocol(serviceMngr_ptr->isiMngr_ptr,
            protocolSlaveSip, 0);

    if (NULL == service_ptr) {
        /* No slave sip service found */
        CSM_dbgPrintf("No slave sip service found\n");
        return (CSM_ERR);
    }

    /* Construct the event */
    eventObj.type = CSM_PRIVATE_EVENT_TYPE_SERVICE;
    eventObj.evt.service.serviceId = service_ptr->serviceId;
    eventObj.evt.service.reason = CSM_SERVICE_REASON_ACTIVATE;
    eventObj.evt.service.reasonDesc[0] = 0;
    /* Notify Account package */
    CSM_isiSendEvent(serviceMngr_ptr->isiMngr_ptr, &eventObj);

    return (CSM_OK);
}

/*
 * ======== _CSM_isiServiceSetFeature() ========
 *
 * Private helper routine to set features to a service.
 *
 * RETURN:
 *   ISI_Return.
 */
vint _CSM_isiServiceSetFeature(
    CSM_IsiService *service_ptr)
{
    int features;

    if (CSM_ISI_PROTOCOL_MODEM_IMS == service_ptr->protocol) {
        if (service_ptr->isMasterSip) {
            features = (ISI_FEATURE_TYPE_VOLTE_CALL | 
                ISI_FEATURE_TYPE_VOLTE_SMS);
        }
        else {
            features = (ISI_FeatureType)0;
        }
    }
    else {
        /* ASAPP */
        if (service_ptr->isMasterSip) {
            features = (ISI_FEATURE_TYPE_VOLTE_CALL |
                    ISI_FEATURE_TYPE_VOLTE_SMS | ISI_FEATURE_TYPE_RCS);
        }
        else {
            features = ISI_FEATURE_TYPE_RCS;
        }
    }

    return (ISI_serviceSetFeature(service_ptr->serviceId, features));
}

/*
 * ======== _CSM_isiServiceGetProvisioningData() ========
 *
 * Private function to get RCS provisioning data from ISI Server.
 *
 * Returns:
 *   CSM_OK: function exits normally.
 *   CSM_ERR: in case of error
 */
vint _CSM_isiServiceGetProvisioningData(
    CSM_ServiceMngr *serviceMngr_ptr,
    ISI_Id           serviceId)
{
    if (ISI_RETURN_OK != ISI_serverGetProvisioningData(serviceId,
            serviceMngr_ptr->rcsProvisioningData,
            serviceMngr_ptr->uri,
            serviceMngr_ptr->authname,
            serviceMngr_ptr->password,
            serviceMngr_ptr->realm,
            serviceMngr_ptr->proxy,
            serviceMngr_ptr->obProxy,
            serviceMngr_ptr->chatConfUri)) {
        return (CSM_ERR);
    }

    /* Informational print */
    CSM_dbgPrintf("RCS provisioning data:\n%s\n",
            serviceMngr_ptr->rcsProvisioningData);
    CSM_dbgPrintf("uri:%s\n", serviceMngr_ptr->uri);
    CSM_dbgPrintf("authname:%s\n", serviceMngr_ptr->authname);
    CSM_dbgPrintf("password:%s\n", serviceMngr_ptr->password);
    CSM_dbgPrintf("realm:%s\n", serviceMngr_ptr->realm);
    CSM_dbgPrintf("proxy:%s\n", serviceMngr_ptr->proxy);
    CSM_dbgPrintf("obProxy:%s\n", serviceMngr_ptr->obProxy);
    CSM_dbgPrintf("conf-fcty-uri:%s\n", serviceMngr_ptr->chatConfUri);
    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceSetMasterNetworkMode() ========
 *
 * Private function to notify current master network mode to ISI server,
 * lte or wifi.
 *
 * Returns:
 */
void CSM_isiServiceSetMasterNetworkMode(
    vint    mode)
{
    if (CSM_ISI_PROTOCOL_MODEM_IMS == mode) {
        ISI_serverSetNetworkMode(ISI_NETWORK_MODE_LTE);
    }
    else if (CSM_ISI_PROTOCOL_SIP_RCS == mode) {
        ISI_serverSetNetworkMode(ISI_NETWORK_MODE_WIFI);
    }
}

/*
 * ======== CSM_isiServiceSetMediaSessionType() ========
 *
 * Private function to set RCS provisioning data, media session type.
 *
 * Returns:
 *   CSM_OK : success to configure
 *   CSM_ERR: fail to configure
 */
vint CSM_isiServiceSetMediaSessionType(
    CSM_ServiceMngr    *serviceMngr_ptr)
{
    if (CSM_TRASPORT_PROTO_MSRP == serviceMngr_ptr->transportProto.psMedia) {
        if (ISI_RETURN_OK != ISI_serverSetMediaSessionType(
                ISI_NETWORK_MODE_LTE,
                ISI_SESSION_TYPE_CHAT)) {
            return (CSM_ERR);
        }
    }
    else if (CSM_TRASPORT_PROTO_MSRP_O_TLS ==
            serviceMngr_ptr->transportProto.psMedia) {
        if (ISI_RETURN_OK != ISI_serverSetMediaSessionType(
                ISI_NETWORK_MODE_LTE,
                ISI_SESSION_TYPE_SECURITY_CHAT)) {
            return (CSM_ERR);
        }
    }
    else {
        return (CSM_ERR);
    }

    if (CSM_TRASPORT_PROTO_MSRP == serviceMngr_ptr->transportProto.wifiMedia) {
        if (ISI_RETURN_OK != ISI_serverSetMediaSessionType(
                ISI_NETWORK_MODE_WIFI,
                ISI_SESSION_TYPE_CHAT)) {
            return (CSM_ERR);
        }
    }
    else if (CSM_TRASPORT_PROTO_MSRP_O_TLS ==
            serviceMngr_ptr->transportProto.wifiMedia) {
        if (ISI_RETURN_OK != ISI_serverSetMediaSessionType(
                ISI_NETWORK_MODE_WIFI,
                ISI_SESSION_TYPE_SECURITY_CHAT)) {
            return (CSM_ERR);
        }
    }
    else {
        return (CSM_ERR);
    }
    
    /* Set Real time Media Session Type */
    if (CSM_TRASPORT_PROTO_RT_MEDIA_RTP == serviceMngr_ptr->transportProto.psRTMedia) {
        if (ISI_RETURN_OK != ISI_serverSetRTMediaSessionType(
                ISI_NETWORK_MODE_LTE,
                ISI_TRASPORT_PROTO_RT_MEDIA_RTP)) {
            return (CSM_ERR);
        }
    }
    else if (CSM_TRASPORT_PROTO_RT_MEDIA_SRTP ==
            serviceMngr_ptr->transportProto.psRTMedia) {
        if (ISI_RETURN_OK != ISI_serverSetRTMediaSessionType(
                ISI_NETWORK_MODE_LTE,
                ISI_TRASPORT_PROTO_RT_MEDIA_SRTP)) {
            return (CSM_ERR);
        }
    }
    else {
        return (CSM_ERR);
    }

    if (CSM_TRASPORT_PROTO_RT_MEDIA_RTP == serviceMngr_ptr->transportProto.wifiRTMedia) {
        if (ISI_RETURN_OK != ISI_serverSetRTMediaSessionType(
                ISI_NETWORK_MODE_WIFI,
                ISI_TRASPORT_PROTO_RT_MEDIA_RTP)) {
            return (CSM_ERR);
        }
    }
    else if (CSM_TRASPORT_PROTO_RT_MEDIA_SRTP ==
            serviceMngr_ptr->transportProto.wifiRTMedia) {
        if (ISI_RETURN_OK != ISI_serverSetRTMediaSessionType(
                ISI_NETWORK_MODE_WIFI,
                ISI_TRASPORT_PROTO_RT_MEDIA_SRTP)) {
            return (CSM_ERR);
        }
    }
    else {
        return (CSM_ERR);
    }
    return (CSM_OK);
}

/*
 * ======== CSM_isiServiceIsActive() ========
 *
 * Public function to get if service is in active state.
 *
 * Returns:
 *   OSAL_TRUE : service active.
 *   OSAL_FALSE: service not active.
 */
OSAL_Boolean CSM_isiServiceIsActive(
    CSM_IsiService *isiSrvc_ptr)
{
    return (_FSM_isActive(&isiSrvc_ptr->fsm));
}

